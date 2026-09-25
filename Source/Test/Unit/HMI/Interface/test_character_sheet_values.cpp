// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_character_sheet_values.cpp
 * @brief Tests unitaires des valeurs affichées par l'écran de fiche (`LOT-38`). Logique pure,
 *        sans Qt ni fichier.
 */

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

#include <gtest/gtest.h>

#include "HMI/Presentation/CharacterSheetValues.h"

namespace {

using core::Ability;

/// Une table d'expérience minimale : trois niveaux, bonus de maîtrise +2 puis +3. Écrite ici et
/// non lue dans la donnée livrée — un test qui dépend d'un fichier vérifie deux choses à la fois,
/// et échoue pour la mauvaise raison le jour où la donnée bouge.
[[nodiscard]] core::ExperienceTable tableDExperience() {
    core::ExperienceTable table;
    table.levels = {{.level = 1, .experience = 0, .proficiencyBonus = 2},
                    {.level = 2, .experience = 300, .proficiencyBonus = 2},
                    {.level = 3, .experience = 900, .proficiencyBonus = 2},
                    {.level = 5, .experience = 6500, .proficiencyBonus = 3}};
    return table;
}

/// Les dix-huit competences, comme le catalogue livre les porte : la planche les affiche TOUTES,
/// et un catalogue de trois entrees ferait passer quinze champs vides pour un comportement normal.
[[nodiscard]] core::SkillCatalog catalogueDeCompetences() {
    core::SkillCatalog catalogue;
    catalogue.skills = {
        {.id = "acrobatics", .name = "Acrobaties", .ability = Ability::Dexterity},
        {.id = "animal-handling", .name = "Dressage", .ability = Ability::Wisdom},
        {.id = "arcana", .name = "Arcanes", .ability = Ability::Intelligence},
        {.id = "athletics", .name = "Athlétisme", .ability = Ability::Strength},
        {.id = "deception", .name = "Tromperie", .ability = Ability::Charisma},
        {.id = "history", .name = "Histoire", .ability = Ability::Intelligence},
        {.id = "insight", .name = "Intuition", .ability = Ability::Wisdom},
        {.id = "intimidation", .name = "Intimidation", .ability = Ability::Charisma},
        {.id = "investigation", .name = "Investigation", .ability = Ability::Intelligence},
        {.id = "medicine", .name = "Médecine", .ability = Ability::Wisdom},
        {.id = "nature", .name = "Nature", .ability = Ability::Intelligence},
        {.id = "perception", .name = "Perception", .ability = Ability::Wisdom},
        {.id = "performance", .name = "Représentation", .ability = Ability::Charisma},
        {.id = "persuasion", .name = "Persuasion", .ability = Ability::Charisma},
        {.id = "religion", .name = "Religion", .ability = Ability::Intelligence},
        {.id = "sleight-of-hand", .name = "Escamotage", .ability = Ability::Dexterity},
        {.id = "stealth", .name = "Discrétion", .ability = Ability::Dexterity},
        {.id = "survival", .name = "Survie", .ability = Ability::Wisdom},
    };
    return catalogue;
}

/// Les trois catalogues, reduits a l'entree que la fiche d'essai reference. Ecrits ici : un test
/// qui lit les catalogues livres verifie deux choses a la fois, et tombe le jour ou la donnee
/// bouge.
[[nodiscard]] core::CharacterOptions catalogues() {
    core::CharacterOptions options;
    options.species.push_back({.id = "demi-elfe", .name = "Demi-elfe"});
    options.backgrounds.push_back({.id = "cartographer", .name = "Cartographer"});
    core::PlayableClass classe;
    classe.id = "brawler";
    classe.name = "Brawler";
    classe.hitDie = 12;
    options.classes.push_back(std::move(classe));
    return options;
}

[[nodiscard]] core::CharacterSheet fiche() {
    core::CharacterSheet personnage;
    personnage.name = "Brenna Vaugris";
    personnage.speciesId = "demi-elfe";
    personnage.classId = "brawler";
    personnage.backgroundId = "cartographer";
    personnage.level = 3;
    personnage.abilities = {16, 12, 14, 10, 13, 8};  // FOR DEX CON INT SAG CHA
    personnage.maximumHitPoints = 30;
    personnage.currentHitPoints = 25;
    personnage.armorClass = 15;
    personnage.speedMeters = 9.0F;
    personnage.skillProficiencies = {"athletics"};
    personnage.savingThrowProficiencies = {Ability::Strength};
    return personnage;
}

}  // namespace

/**
 * @brief Un modificateur s'affiche avec son signe, et une valeur de caractéristique avec le sien
 *        entre parenthèses.
 *
 * Le signe n'est pas une coquetterie : `3` nu se lit comme une valeur de caractéristique, et les
 * deux se côtoient sur la même ligne de la feuille.
 * \castest{<b>Les modificateurs s'affichent signes, les caracteristiques avec leur
 * modificateur.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Produire les valeurs d'une fiche de Force 16 et de Charisme 8.<br/>2. Lire les deux
 * lignes de caracteristique.<br/>
 * \tattendu « 16 (+3) » et « 8 (-1) ».
 * }
 */
TEST(CharacterSheetValuesTest, LesModificateursSAffichentAvecLeurSigne) {
    const core::CharacterSheet personnage = fiche();
    const core::ExperienceTable table = tableDExperience();
    const std::map<std::string, std::string> valeurs =
        hmi::characterSheetValues({.sheet = &personnage, .experience = &table});

    EXPECT_EQ(valeurs.at("sheet.ability.strength"), "16 (+3)");
    EXPECT_EQ(valeurs.at("sheet.ability.charisma"), "8 (-1)");
    // L'initiative EST le modificateur de Dextérité, et se lit comme un modificateur.
    EXPECT_EQ(valeurs.at("sheet.initiative"), "+1");
}

/**
 * @brief Un jet de sauvegarde maîtrisé porte le bonus de maîtrise ; les autres non.
 * \castest{<b>Un jet de sauvegarde maitrise porte le bonus de maitrise.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Produire les valeurs d'une fiche maitrisant la sauvegarde de Force.<br/>2. Comparer
 * les lignes de Force et de Dexterite.<br/>
 * \tattendu Force = modificateur + maitrise, Dexterite = modificateur seul.
 * }
 */
TEST(CharacterSheetValuesTest, LeJetDeSauvegardeMaitriseComptteLaMaitrise) {
    const core::CharacterSheet personnage = fiche();
    const core::ExperienceTable table = tableDExperience();
    const std::map<std::string, std::string> valeurs =
        hmi::characterSheetValues({.sheet = &personnage, .experience = &table});

    EXPECT_EQ(valeurs.at("sheet.proficiency_bonus"), "+2");
    EXPECT_EQ(valeurs.at("sheet.save.strength"), "+5");   // +3 de Force, +2 de maîtrise
    EXPECT_EQ(valeurs.at("sheet.save.dexterity"), "+1");  // +1 de Dextérité, sans maîtrise
}

/**
 * @brief Une compétence maîtrisée porte son bonus **et** se signale ; la Perception passive vaut
 *        10 + le modificateur de Perception.
 * \castest{<b>Une competence maitrisee se signale, et la Perception passive en derive.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Produire les valeurs d'une fiche maitrisant l'Athletisme.<br/>2. Lire Athletisme,
 * Discretion et la Perception passive.<br/>
 * \tattendu Athletisme signale sa maitrise, Discretion non, et la Perception passive vaut 11.
 * }
 */
TEST(CharacterSheetValuesTest, LaCompetenceMaitriseeSeSignaleEtLaPerceptionPassiveEnDerive) {
    const core::CharacterSheet personnage = fiche();
    const core::ExperienceTable table = tableDExperience();
    const core::SkillCatalog competences = catalogueDeCompetences();
    const std::map<std::string, std::string> valeurs = hmi::characterSheetValues(
        {.sheet = &personnage, .experience = &table, .skills = &competences});

    EXPECT_EQ(valeurs.at("sheet.skill.athletics"), "+5 •");  // +3 de Force, +2 de maîtrise
    EXPECT_EQ(valeurs.at("sheet.skill.stealth"), "+1");      // Dextérité seule
    // Sagesse 13 -> +1 ; Perception non maîtrisée -> 10 + 1.
    EXPECT_EQ(valeurs.at("sheet.passive_perception"), "11");
}

/**
 * @brief Les points de vie se lisent **contre leur maximum**, et la vitesse porte son unité.
 * \castest{<b>Les points de vie se lisent contre leur maximum.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Produire les valeurs d'une fiche a 25 points de vie sur 30.<br/>
 * \tattendu « 25 / 30 », et une vitesse en metres.
 * }
 */
TEST(CharacterSheetValuesTest, LesPointsDeVieSeLisentContreLeurMaximum) {
    const core::CharacterSheet personnage = fiche();
    const std::map<std::string, std::string> valeurs =
        hmi::characterSheetValues({.sheet = &personnage});

    EXPECT_EQ(valeurs.at("sheet.hit_points"), "25 / 30");
    EXPECT_EQ(valeurs.at("sheet.hit_points_max"), "30");
    EXPECT_EQ(valeurs.at("sheet.speed"), "9 m");
}

/**
 * @brief Sans fiche, la table est **vide** — et l'écran garde donc ses tirets.
 *
 * Rendre des zéros serait pire qu'inutile : « 0 point de vie » se lit comme un personnage mort,
 * là où la vérité est qu'il n'y a pas de personnage.
 * \castest{<b>Sans fiche, aucune valeur n'est produite.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Produire les valeurs d'un contexte sans fiche.<br/>
 * \tattendu La table est vide.
 * }
 */
TEST(CharacterSheetValuesTest, SansFicheAucuneValeurNEstProduite) {
    EXPECT_TRUE(hmi::characterSheetValues({}).empty());
}
