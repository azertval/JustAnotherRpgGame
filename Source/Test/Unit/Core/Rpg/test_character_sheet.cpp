// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_character_sheet.cpp
 * @brief Tests de la fiche de personnage (LOT-13).
 *
 * L'acceptation du lot tient en quatre phrases : trois classes definies en JSON se chargent et
 * donnent les bons modificateurs ; la montee de niveau est reproductible et testee AUX BORNES ;
 * aucune valeur de regle n'est codee en dur ; et un test construit QUATRE fiches independantes,
 * parce que rien ne doit supposer l'unicite du personnage.
 */

#include <array>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Ecs/Components/RpgActor.h"
#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Check.h"
#include "Core/Rpg/Scale.h"
#include "Core/Rpg/Skill.h"

namespace {

const std::filesystem::path RPG{JADG_RPG_DIR};

struct Catalogues {
    core::CharacterOptions options;
    core::SkillCatalog skills;
    core::ExperienceTable experience;
    core::CharacterCreationRules rules;
};

[[nodiscard]] const Catalogues& catalogues() {
    static const Catalogues charges = [] {
        Catalogues lus;
        lus.options =
            core::loadCharacterOptions(RPG / "species", RPG / "backgrounds", RPG / "classes");
        lus.skills = core::loadSkills(RPG / "skills");
        lus.experience = core::loadExperienceTable(RPG / "rules" / "experience.json");
        lus.rules = core::loadCharacterCreationRules(RPG / "rules" / "character-creation.json");
        return lus;
    }();
    return charges;
}

// Six valeurs de caracteristique, dans l'ordre de core::Ability.
constexpr std::array<int, 6> VALEURS_STANDARD = {15, 14, 13, 12, 10, 8};

[[nodiscard]] core::CharacterSheet fiche(const std::string& nom, const char* espece,
                                         const char* classe, const char* historique,
                                         const std::array<int, 6>& valeurs = VALEURS_STANDARD) {
    const Catalogues& lus = catalogues();
    return core::buildCharacterSheet(
        nom, valeurs, lus.options.findSpecies(espece), lus.options.findClass(classe),
        lus.options.findBackground(historique), lus.rules, lus.experience);
}

}  // namespace

/**
 * @brief Les catalogues et les deux tables de regles se chargent.
 * \castest{<b>La table d'experience et les constantes de creation se chargent depuis la
 * donnee.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger rules/experience.json et rules/character-creation.json.<br/>
 * \tattendu Vingt niveaux, seuils strictement croissants, et les deux constantes lues.
 * }
 */
TEST(CharacterSheetTest, LesTablesDeReglesSeChargent) {
    const Catalogues& lus = catalogues();
    for (const std::string& erreur : lus.experience.errors) {
        ADD_FAILURE() << erreur;
    }
    for (const std::string& erreur : lus.rules.errors) {
        ADD_FAILURE() << erreur;
    }
    ASSERT_EQ(lus.experience.levels.size(), 20U) << "la table du livre porte vingt niveaux";
    EXPECT_EQ(lus.experience.maximumLevel(), 20);
    for (std::size_t i = 1; i < lus.experience.levels.size(); ++i) {
        EXPECT_GT(lus.experience.levels[i].experience, lus.experience.levels[i - 1].experience)
            << "deux seuils inverses rendent une montee de niveau infranchissable";
        EXPECT_GE(lus.experience.levels[i].proficiencyBonus,
                  lus.experience.levels[i - 1].proficiencyBonus)
            << "le bonus de maitrise ne decroit jamais";
    }
    EXPECT_TRUE(lus.rules.ok());
    EXPECT_FALSE(lus.skills.skills.empty()) << "aucune competence chargee";
}

/**
 * @brief Quatre fiches independantes coexistent sans se connaitre.
 * \castest{<b>Quatre fiches de personnage coexistent, chacune avec ses propres valeurs.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Construire quatre fiches d'especes et de classes differentes.<br/>
 * 2. Modifier les points de vie de la premiere et faire monter la deuxieme de niveau.<br/>
 * \tattendu Les quatre gardent des valeurs distinctes ; aucune modification n'en touche une autre.
 * }
 */
TEST(CharacterSheetTest, QuatreFichesIndependantes) {
    // La decision de cadrage est << un heros au depart, quatre a terme >> : ce test est ce qui
    // empeche qu'une fonction prenne << le personnage >> implicitement. Si `CharacterSheet`
    // devenait un singleton, ce cas serait le premier a tomber.
    std::vector<core::CharacterSheet> groupe = {
        fiche("Bruenor", "nain", "brawler", "soldat"),
        fiche("Thalia", "elfe", "mage", "sage"),
        fiche("Pip", "halfelin", "scoundrel", "criminel"),
        fiche("Sereth", "tieffelin", "priest", "acolyte"),
    };
    ASSERT_EQ(groupe.size(), 4U);
    for (const core::CharacterSheet& personnage : groupe) {
        EXPECT_FALSE(personnage.speciesId.empty()) << personnage.name;
        EXPECT_FALSE(personnage.classId.empty()) << personnage.name;
        EXPECT_FALSE(personnage.backgroundId.empty()) << personnage.name;
        EXPECT_GT(personnage.maximumHitPoints, 0) << personnage.name;
        EXPECT_GT(personnage.speedMeters, 0.0F) << personnage.name;
    }

    const int pvAvant = groupe[0].maximumHitPoints;
    const int niveauAvant = groupe[0].level;
    groupe[0].currentHitPoints -= 3;
    core::gainExperience(groupe[1], catalogues().experience, 6,
                         catalogues().experience.thresholdAt(3));

    EXPECT_EQ(groupe[0].maximumHitPoints, pvAvant) << "le maximum de la premiere n'a pas bouge";
    EXPECT_EQ(groupe[0].level, niveauAvant) << "la montee de la deuxieme a touche la premiere";
    EXPECT_EQ(groupe[1].level, 3);
    EXPECT_EQ(groupe[2].level, 1) << "la troisieme n'a rien gagne";
    EXPECT_EQ(groupe[3].level, 1) << "la quatrieme n'a rien gagne";
    EXPECT_NE(groupe[0].currentHitPoints, groupe[0].maximumHitPoints);
    EXPECT_EQ(groupe[2].currentHitPoints, groupe[2].maximumHitPoints)
        << "la blessure de la premiere a touche la troisieme";
}

/**
 * @brief Trois classes JSON donnent les bons modificateurs.
 * \castest{<b>Trois classes chargees depuis JSON produisent les bons points de vie et jets de
 * sauvegarde.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Construire une fiche pour le brawler (d12), le mage (d6) et le priest (d8).<br/>
 * 2. Comparer points de vie et modificateurs de sauvegarde au calcul du livre.<br/>
 * \tattendu Les points de vie valent le de plus le modificateur de Constitution ; les sauvegardes
 * maitrisees portent le bonus de maitrise, les autres non.
 * }
 */
TEST(CharacterSheetTest, TroisClassesDonnentLesBonsModificateurs) {
    const Catalogues& lus = catalogues();
    // VALEURS_STANDARD donne 13 en Constitution, et l'humain augmente les six caracteristiques de
    // 1 : elle vaut donc 14 sur la fiche, pour un modificateur de +2. Le modificateur est LU sur
    // la fiche et non ecrit ici -- l'ecrire ferait de ce test la copie d'un calcul, alors qu'il
    // doit verifier que le de de vie vient de la DONNEE de la classe.
    constexpr int CONSTITUTION_DE_BASE = 13;
    constexpr int AUGMENTATION_HUMAINE = 1;
    const int modificateurAttendu =
        core::abilityModifier(CONSTITUTION_DE_BASE + AUGMENTATION_HUMAINE);

    for (const char* identifiant : {"brawler", "mage", "priest"}) {
        const core::PlayableClass* classe = lus.options.findClass(identifiant);
        ASSERT_NE(classe, nullptr) << identifiant;
        const core::CharacterSheet personnage = fiche("Essai", "humain", identifiant, "sage");

        EXPECT_EQ(personnage.modifier(core::Ability::Constitution), modificateurAttendu);
        EXPECT_EQ(personnage.maximumHitPoints, classe->hitDie + modificateurAttendu)
            << identifiant << " : au niveau 1, les points de vie valent le maximum du de";
        EXPECT_EQ(personnage.currentHitPoints, personnage.maximumHitPoints);

        const int maitrise = core::proficiencyBonus(personnage, lus.experience);
        EXPECT_EQ(maitrise, lus.experience.proficiencyBonusAt(1));
        for (const core::Ability caracteristique : classe->savingThrowProficiencies) {
            EXPECT_EQ(core::savingThrowModifier(personnage, lus.experience, caracteristique),
                      personnage.modifier(caracteristique) + maitrise)
                << identifiant << " : sauvegarde maitrisee";
        }
        for (const core::Ability caracteristique : core::allAbilities()) {
            if (personnage.savingThrowProficiencies.contains(caracteristique)) {
                continue;
            }
            EXPECT_EQ(core::savingThrowModifier(personnage, lus.experience, caracteristique),
                      personnage.modifier(caracteristique))
                << identifiant << " : sauvegarde non maitrisee, aucun bonus";
        }
    }
}

/**
 * @brief La montee de niveau est testee aux bornes : seuil exact, depassement, multi-niveaux.
 * \castest{<b>La montee de niveau franchit le seuil exact, le depassement et plusieurs niveaux
 * d'un coup.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Donner exactement le seuil du niveau 2, puis un point de moins, puis le seuil du
 * niveau 5 d'un coup.<br/>
 * \tattendu Le seuil exact fait monter ; un point de moins ne fait pas monter ; le seuil du niveau
 * 5 fait passer directement au niveau 5.
 * }
 */
TEST(CharacterSheetTest, LaMonteeDeNiveauEstTesteeAuxBornes) {
    const core::ExperienceTable& table = catalogues().experience;
    const core::PlayableClass* classe = catalogues().options.findClass("brawler");
    ASSERT_NE(classe, nullptr);
    const int seuil2 = table.thresholdAt(2);
    const int seuil5 = table.thresholdAt(5);
    ASSERT_GT(seuil2, 0);
    ASSERT_GT(seuil5, seuil2);

    // Le seuil EXACT fait monter. C'est la borne que l'on rate en ecrivant `>` au lieu de `>=`,
    // et le defaut ne se voit qu'au moment ou un joueur reste au niveau 1 avec 300 points.
    core::CharacterSheet juste = fiche("Juste", "humain", "brawler", "soldat");
    const core::LevelUpResult montee = core::gainExperience(juste, table, classe->hitDie, seuil2);
    EXPECT_TRUE(montee.gainedLevel());
    EXPECT_EQ(montee.previousLevel, 1);
    EXPECT_EQ(montee.newLevel, 2);
    EXPECT_GT(montee.hitPointsGained, 0);

    // Un point de MOINS ne fait pas monter.
    core::CharacterSheet presque = fiche("Presque", "humain", "brawler", "soldat");
    const core::LevelUpResult sansMontee =
        core::gainExperience(presque, table, classe->hitDie, seuil2 - 1);
    EXPECT_FALSE(sansMontee.gainedLevel());
    EXPECT_EQ(presque.level, 1);
    EXPECT_EQ(sansMontee.hitPointsGained, 0);

    // PLUSIEURS niveaux d'un coup : le personnage atteint le niveau que son total lui donne, pas
    // le suivant. Monter d'un seul cran a chaque gain laisserait un heros au niveau 2 avec de quoi
    // etre au niveau 5.
    core::CharacterSheet bond = fiche("Bond", "humain", "brawler", "soldat");
    const core::LevelUpResult saut = core::gainExperience(bond, table, classe->hitDie, seuil5);
    EXPECT_EQ(saut.newLevel, 5);
    EXPECT_EQ(bond.level, 5);
    EXPECT_GT(saut.newProficiencyBonus, saut.previousProficiencyBonus)
        << "le bonus de maitrise passe de +2 a +3 au niveau 5, selon la table";
    EXPECT_EQ(
        bond.maximumHitPoints,
        core::maximumHitPointsFor(classe->hitDie, 5, bond.modifier(core::Ability::Constitution)));

    // Un gain nul ou negatif ne fait rien : perdre de l'experience n'est pas une regle de ce jeu.
    core::CharacterSheet stable = fiche("Stable", "humain", "brawler", "soldat");
    stable.experiencePoints = seuil5;
    const core::LevelUpResult rien = core::gainExperience(stable, table, classe->hitDie, -1000);
    EXPECT_FALSE(rien.gainedLevel());
    EXPECT_EQ(stable.experiencePoints, seuil5) << "un gain negatif ne retire pas d'experience";
}

/**
 * @brief Deux montees successives donnent le meme resultat qu'une seule du meme total.
 * \castest{<b>La montee de niveau est reproductible : le chemin ne change pas le resultat.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Donner le seuil du niveau 5 en une fois a une fiche, en deux fois a une autre.<br/>
 * \tattendu Meme niveau, memes points de vie, meme total d'experience.
 * }
 */
TEST(CharacterSheetTest, LaMonteeDeNiveauEstReproductible) {
    const core::ExperienceTable& table = catalogues().experience;
    const core::PlayableClass* classe = catalogues().options.findClass("priest");
    ASSERT_NE(classe, nullptr);
    const int seuil5 = table.thresholdAt(5);

    core::CharacterSheet enUneFois = fiche("Une", "nain", "priest", "acolyte");
    core::gainExperience(enUneFois, table, classe->hitDie, seuil5);

    core::CharacterSheet enDeuxFois = fiche("Deux", "nain", "priest", "acolyte");
    core::gainExperience(enDeuxFois, table, classe->hitDie, table.thresholdAt(3));
    core::gainExperience(enDeuxFois, table, classe->hitDie, seuil5 - table.thresholdAt(3));

    EXPECT_EQ(enUneFois.level, enDeuxFois.level);
    EXPECT_EQ(enUneFois.maximumHitPoints, enDeuxFois.maximumHitPoints);
    EXPECT_EQ(enUneFois.experiencePoints, enDeuxFois.experiencePoints);
}

/**
 * @brief Le modificateur de competence vient du catalogue, pas d'un switch.
 * \castest{<b>Un jet de competence emploie la caracteristique que le catalogue lui donne.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Calculer le modificateur d'une competence maitrisee et d'une autre non
 * maitrisee.<br/>
 * 2. Demander une competence inconnue du catalogue.<br/>
 * \tattendu La maitrise ajoute le bonus, l'absence de maitrise non ; une competence inconnue est
 * signalee et non devinee.
 * }
 */
TEST(CharacterSheetTest, LeModificateurDeCompetenceVientDuCatalogue) {
    const Catalogues& lus = catalogues();
    const core::CharacterSheet moine = fiche("Sereth", "tieffelin", "priest", "acolyte");
    ASSERT_FALSE(moine.skillProficiencies.empty()) << "l'acolyte maitrise deux competences";

    for (const std::string& identifiant : moine.skillProficiencies) {
        const core::SkillDefinition* competence = lus.skills.find(identifiant);
        ASSERT_NE(competence, nullptr) << identifiant << " absente du catalogue";
        const core::SkillCheckModifier calcul =
            core::skillModifier(moine, lus.experience, lus.skills, identifiant);
        EXPECT_TRUE(calcul.found);
        EXPECT_TRUE(calcul.proficient);
        EXPECT_EQ(calcul.value, moine.modifier(competence->ability) +
                                    core::proficiencyBonus(moine, lus.experience));
    }

    const core::SkillCheckModifier inconnue =
        core::skillModifier(moine, lus.experience, lus.skills, "nage-synchronisee");
    EXPECT_FALSE(inconnue.found)
        << "une competence inconnue est signalee, jamais devinee : un modificateur nu rendu en "
           "silence passerait pour une maitrise absente";
}

/**
 * @brief Le heros de la demo est la fiche pre-tiree du Brawler, chiffre pour chiffre (LOT-112).
 * \castest{<b>La fiche du heros redonne les nombres du livre, et sa Persuasion a DD 18 reussit
 * une fois sur dix.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger Rpg/characters/heros-brawler.json.<br/>2. Comparer caracteristiques, points
 * de vie, sauvegardes et competences a la fiche du Player's Guide to Tanares, p. 195.<br/>
 * 3. Compter les faces du d20 qui font reussir la Persuasion a DD 18.<br/>
 * \tattendu For 16, Dex 13, Con 16, Int 10, Sag 12, Cha 8 ; 15 PV ; For et Con +5 ; les cinq
 * competences maitrisees du livre a leur valeur ; Persuasion -1, donc deux faces sur vingt.
 * }
 */
TEST(CharacterSheetTest, LeHerosDeLaDemoEstLaFicheDuLivre) {
    const Catalogues& lus = catalogues();
    const core::LoadedCharacterSheet charge = core::loadCharacterSheet(
        RPG / "characters" / "heros-brawler.json", lus.options, lus.rules, lus.experience);
    ASSERT_TRUE(charge.errors.empty()) << charge.errors.front();
    const core::CharacterSheet& heros = charge.sheet;

    EXPECT_EQ(heros.level, 1);
    EXPECT_EQ(heros.abilities, (std::array<int, 6>{16, 13, 16, 10, 12, 8}))
        << "valeurs du livre, augmentation du demi-orc comprise";
    EXPECT_EQ(heros.maximumHitPoints, 15) << "12 au de de vie du Brawler, +3 de Constitution";
    EXPECT_EQ(core::savingThrowModifier(heros, lus.experience, core::Ability::Strength), 5);
    EXPECT_EQ(core::savingThrowModifier(heros, lus.experience, core::Ability::Constitution), 5);

    // Les pastilles pleines de la fiche du livre, et la valeur inscrite en face.
    const std::vector<std::pair<std::string, int>> maitrisees = {
        {"animal-handling", 3}, {"athletics", 5}, {"intimidation", 1},
        {"investigation", 2},   {"nature", 2},
    };
    for (const auto& [competence, valeur] : maitrisees) {
        const core::SkillCheckModifier calcul =
            core::skillModifier(heros, lus.experience, lus.skills, competence);
        EXPECT_TRUE(calcul.proficient) << competence;
        EXPECT_EQ(calcul.value, valeur) << competence;
    }

    // La Persuasion n'est pas maitrisee : Charisme 8, -1. La quete de la demo la demande a DD 18
    // (LOT-120), il faut donc un 19 ou un 20 au de.
    const core::SkillCheckModifier persuasion =
        core::skillModifier(heros, lus.experience, lus.skills, "persuasion");
    EXPECT_FALSE(persuasion.proficient);
    EXPECT_EQ(persuasion.value, -1);
    constexpr int DD_DU_GARDE = 18;
    int faces = 0;
    for (int de = 1; de <= core::D20_FACES; ++de) {
        faces += de + persuasion.value >= DD_DU_GARDE ? 1 : 0;
    }
    EXPECT_EQ(faces, 2) << "une fois sur dix";
}

/**
 * @brief La vitesse de la fiche se convertit en cases avec l'echelle du LOT-12.
 * \castest{<b>La vitesse d'un personnage se lit en cases via l'echelle unique du projet.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Convertir la vitesse d'un nain (7,50 m) et d'un humain (9 m) en cases.<br/>
 * \tattendu 5 et 6 cases, via core::tilesFromMeters et non une division ecrite sur place.
 * }
 */
TEST(CharacterSheetTest, LaVitesseSeLitEnCases) {
    const core::CharacterSheet nain = fiche("Bruenor", "nain", "brawler", "soldat");
    const core::CharacterSheet humain = fiche("Aria", "humain", "mage", "sage");
    EXPECT_FLOAT_EQ(nain.speedInTiles(), core::tilesFromMeters(nain.speedMeters));
    EXPECT_FLOAT_EQ(humain.speedInTiles(), core::tilesFromMeters(humain.speedMeters));
    // Les deux vitesses du corpus, en cases : 7,50 m et 9 m pour une case de 1,50 m.
    EXPECT_FLOAT_EQ(nain.speedInTiles(), 5.0F);
    EXPECT_FLOAT_EQ(humain.speedInTiles(), 6.0F);
}

/**
 * @brief Le composant ECS distingue une entite sans fiche d'une entite liee a la premiere.
 * \castest{<b>Une entite sans fiche ne se confond pas avec une entite liee a la fiche
 * zero.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Construire un RpgActor par defaut, puis un lie a l'indice 0.<br/>
 * \tattendu Le premier n'a pas de fiche, le second en a une.
 * }
 */
TEST(CharacterSheetTest, LeComposantDistingueLAbsenceDeFiche) {
    const core::RpgActor sansFiche;
    EXPECT_FALSE(sansFiche.hasSheet())
        << "confondre << pas de fiche >> et << la fiche zero >> ferait attaquer un tonneau avec "
           "les caracteristiques du heros";
    const core::RpgActor premiere{0};
    EXPECT_TRUE(premiere.hasSheet());
    EXPECT_EQ(premiere.sheetIndex, 0U);
}

/**
 * @brief Aucune valeur de regle n'est ecrite dans le C++ du lot.
 * \castest{<b>La classe d'armure sans armure et le plafond de caracteristique viennent de la
 * donnee.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire les deux constantes dans rules/character-creation.json.<br/>
 * 2. Verifier que la CA d'une fiche sans armure vaut la constante lue plus le modificateur de
 * Dexterite.<br/>
 * \tattendu L'egalite tient pour la constante LUE, quelle qu'elle soit : changer le fichier
 * change la fiche, sans recompiler.
 * }
 */
TEST(CharacterSheetTest, AucuneValeurDeRegleNEstEcriteDansLeCpp) {
    const Catalogues& lus = catalogues();
    ASSERT_TRUE(lus.rules.ok());
    const core::CharacterSheet personnage = fiche("Aria", "elfe", "mage", "sage");
    // L'assertion porte sur la constante LUE, pas sur 10 : si le C++ portait sa propre copie de la
    // regle, ce test resterait vert tout en la contredisant.
    EXPECT_EQ(personnage.armorClass,
              lus.rules.unarmoredArmorClass + personnage.modifier(core::Ability::Dexterity));
    for (const core::Ability caracteristique : core::allAbilities()) {
        EXPECT_LE(personnage.ability(caracteristique), lus.rules.maximumAbilityScore)
            << "une augmentation d'espece ne franchit pas le plafond lu dans la donnee";
    }
}
