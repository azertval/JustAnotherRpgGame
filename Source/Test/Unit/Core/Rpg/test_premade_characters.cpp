// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_premade_characters.cpp
 * @brief Les quatre fiches pre-tirees du Player's Guide to Tanares, valeur pour valeur (LOT-130).
 *
 * Un test par fiche (p. 195, 199, 203, 207). Chacun charge le fichier de `Rpg/characters/`, qui
 * ne porte que des CHOIX, et redemande au moteur chaque valeur DERIVEE que la page imprime :
 * caracteristiques finales, points de vie, classe d'armure, initiative, vitesse, Perception
 * passive, les six sauvegardes, les dix-huit competences, les attaques. Ce que le moteur ne sait
 * pas encore calculer -- une capacite de classe, un sort -- est un ecart ECRIT dans la fiche du
 * lot, et le test dit la valeur du moteur d'aujourd'hui, jamais une valeur devinee.
 */

#include <array>
#include <filesystem>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/Attack.h"
#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Equipment.h"
#include "Core/Rpg/Inventory.h"
#include "Core/Rpg/RpgEnums.h"
#include "Core/Rpg/Skill.h"

namespace {

const std::filesystem::path RPG{JADG_RPG_DIR};

struct Catalogues {
    core::CharacterOptions options;
    core::SkillCatalog skills;
    core::ExperienceTable experience;
    core::CharacterCreationRules rules;
    core::EquipmentCatalog equipment;
    core::ItemCatalog items;
    core::EncumbranceRules encumbrance;
};

[[nodiscard]] const Catalogues& catalogues() {
    static const Catalogues charges = [] {
        Catalogues lus;
        // Capacites et sorts compris (LOT-132) : la CA de la page 195 vient de Tough as Nails.
        lus.options = core::loadCharacterOptions(RPG);
        lus.skills = core::loadSkills(RPG / "skills");
        lus.experience = core::loadExperienceTable(RPG / "rules" / "experience.json");
        lus.rules = core::loadCharacterCreationRules(RPG / "rules" / "character-creation.json");
        lus.equipment = core::loadEquipment(RPG / "weapons", RPG / "armors");
        lus.items = core::loadItems(RPG / "items");
        lus.encumbrance = core::loadEncumbranceRules(RPG / "rules" / "encumbrance.json");
        return lus;
    }();
    return charges;
}

// Les dix-huit competences dans l'ordre de la fiche imprimee, avec la valeur en face. Toutes
// sont comparees, maitrisees ou non : une competence non maitrisee a la mauvaise valeur trahit
// une caracteristique fausse aussi surement qu'une maitrisee.
struct CompetenceImprimee {
    const char* id;
    int valeur;
    bool maitrisee;
};

using Sauvegardes = std::array<int, 6>;

// Une fiche chargee, avec ce que le test en verifie en commun : les erreurs, le niveau, les
// valeurs finales, les sauvegardes et les competences. Le reste est propre a chaque page.
[[nodiscard]] core::LoadedCharacterSheet charger(const char* fichier) {
    const Catalogues& lus = catalogues();
    core::LoadedCharacterSheet charge = core::loadCharacterSheet(
        RPG / "characters" / fichier, lus.options, lus.rules, lus.experience);
    for (const std::string& erreur : charge.errors) {
        ADD_FAILURE() << erreur;
    }
    EXPECT_EQ(charge.sheet.level, 1) << "les quatre fiches sont de niveau 1";
    EXPECT_EQ(core::proficiencyBonus(charge.sheet, lus.experience), 2) << "maitrise +2 au niveau 1";
    return charge;
}

void verifierSauvegardes(const core::CharacterSheet& fiche, const Sauvegardes& imprimees) {
    const Catalogues& lus = catalogues();
    for (const core::Ability caracteristique : core::allAbilities()) {
        EXPECT_EQ(core::savingThrowModifier(fiche, lus.experience, caracteristique),
                  imprimees[static_cast<std::size_t>(caracteristique)])
            << fiche.name << " : sauvegarde de " << core::abilityName(caracteristique);
    }
}

void verifierCompetences(const core::CharacterSheet& fiche,
                         const std::vector<CompetenceImprimee>& imprimees) {
    const Catalogues& lus = catalogues();
    ASSERT_EQ(imprimees.size(), 18U) << "la fiche imprime les dix-huit competences";
    for (const CompetenceImprimee& competence : imprimees) {
        const core::SkillCheckModifier calcul =
            core::skillModifier(fiche, lus.experience, lus.skills, competence.id);
        EXPECT_TRUE(calcul.found) << competence.id;
        EXPECT_EQ(calcul.proficient, competence.maitrisee)
            << fiche.name << " : pastille de " << competence.id;
        EXPECT_EQ(calcul.value, competence.valeur) << fiche.name << " : " << competence.id;
    }
}

// La Perception passive de la page : 10 + le modificateur de Perception, la regle du livre --
// celle que l'ecran de la fiche (hmi::characterSheetValues) applique.
[[nodiscard]] int perceptionPassive(const core::CharacterSheet& fiche) {
    const Catalogues& lus = catalogues();
    return 10 + core::skillModifier(fiche, lus.experience, lus.skills, "perception").value;
}

[[nodiscard]] int classeDArmure(const core::LoadedCharacterSheet& charge) {
    const Catalogues& lus = catalogues();
    const core::ItemLookup catalogue{.items = &lus.items, .equipment = &lus.equipment};
    return core::derivedStatsFor(charge.sheet, charge.inventory, catalogue, lus.rules,
                                 lus.encumbrance)
        .armorClass;
}

// Le bonus au toucher tel que la page l'imprime : la somme des modificateurs de l'attaque.
[[nodiscard]] int bonusAuToucher(const core::AttackProfile& attaque) {
    return std::accumulate(attaque.modifiers.begin(), attaque.modifiers.end(), 0,
                           [](int total, const core::Modifier& m) { return total + m.value; });
}

struct AttaqueImprimee {
    const char* arme;
    int toucher;
    int desCount;
    int desFaces;
    int degatsFixe;
    core::DamageType type;
};

// L'attaque d'une arme du catalogue, au corps a corps ou au tir selon l'arme. La maitrise se LIT
// dans la fiche (LOT-131) : la classe et l'espece la donnent, et la page la suppose -- si la donnee
// ne la donnait pas, le bonus au toucher tomberait de 2 et le test le dirait.
[[nodiscard]] core::AttackProfile attaque(const core::CharacterSheet& fiche, const char* arme) {
    const Catalogues& lus = catalogues();
    const core::Weapon* const catalogue = lus.equipment.findWeapon(arme);
    EXPECT_NE(catalogue, nullptr) << arme;
    return core::weaponAttackFor(fiche, catalogue, core::proficiencyBonus(fiche, lus.experience),
                                 core::isProficientWith(fiche, *catalogue));
}

void verifierAttaque(const core::AttackProfile& profil, const AttaqueImprimee& imprimee) {
    EXPECT_EQ(bonusAuToucher(profil), imprimee.toucher) << imprimee.arme << " : bonus au toucher";
    ASSERT_EQ(profil.damage.size(), 1U) << imprimee.arme;
    EXPECT_EQ(profil.damage.front().dice.count, imprimee.desCount) << imprimee.arme;
    EXPECT_EQ(profil.damage.front().dice.faces, imprimee.desFaces) << imprimee.arme;
    EXPECT_EQ(profil.damage.front().dice.modifier, imprimee.degatsFixe) << imprimee.arme;
    EXPECT_EQ(profil.damage.front().type, imprimee.type) << imprimee.arme;
}

// Une arme lancee : la meme attaque, a distance, aux portees de la page (en pieds, converties
// ici en cases de 5 ft : 20/60 ft = 4/12 cases).
void verifierLancer(const core::CharacterSheet& fiche, const char* arme, int normaleCases,
                    int longueCases) {
    const Catalogues& lus = catalogues();
    const core::Weapon* const catalogue = lus.equipment.findWeapon(arme);
    ASSERT_NE(catalogue, nullptr) << arme;
    const std::optional<core::AttackProfile> lancer =
        core::thrownAttackFor(fiche, *catalogue, core::proficiencyBonus(fiche, lus.experience),
                              core::isProficientWith(fiche, *catalogue));
    ASSERT_TRUE(lancer.has_value()) << arme << " se lance";
    ASSERT_TRUE(lancer->range.has_value()) << arme;
    EXPECT_EQ(lancer->range->normal, normaleCases) << arme;
    EXPECT_EQ(lancer->range->maximum, longueCases) << arme;
}

}  // namespace

/**
 * @brief La fiche du Brawler (p. 195) se recalcule valeur pour valeur.
 * \castest{<b>Le Brawler pre-tire redonne chaque valeur de la page 195.</b><br/>
 * \tcat Unitaire · Fiches pre-tirees<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger Rpg/characters/heros-brawler.json.<br/>2. Recalculer caracteristiques,
 * points de vie, initiative, vitesse, Perception passive, sauvegardes, dix-huit competences,
 * greataxe, handaxe et javelin.<br/>
 * \tattendu For 16 Dex 13 Con 16 Int 10 Sag 12 Cha 8 ; 15 PV ; initiative +1 ; 30 ft ; Perception
 * passive 11 ; sauvegardes +5 +1 +5 0 +1 -1 ; les cinq pastilles ; +5 au toucher partout, 1d12+3,
 * 1d6+3 lancer 20/60, 1d6+3 lancer 30/120 ; CA 14 par Tough as Nails (LOT-132).
 * }
 */
TEST(PremadeCharactersTest, LeBrawlerEstLaPage195) {
    const core::LoadedCharacterSheet charge = charger("heros-brawler.json");
    const core::CharacterSheet& fiche = charge.sheet;

    EXPECT_EQ(fiche.speciesId, "demi-orc");
    EXPECT_EQ(fiche.backgroundId, "dragon-hunter");
    EXPECT_EQ(fiche.abilities, (std::array<int, 6>{16, 13, 16, 10, 12, 8}));
    EXPECT_EQ(fiche.maximumHitPoints, 15) << "1d12 au maximum, +3 de Constitution";
    EXPECT_EQ(fiche.modifier(core::Ability::Dexterity), 1) << "initiative +1";
    EXPECT_FLOAT_EQ(fiche.speedMeters, 9.0F) << "30 ft";
    EXPECT_EQ(perceptionPassive(fiche), 11);
    EXPECT_EQ(fiche.languages, (std::set<std::string>{"common", "draconic", "orc"}));

    // Tough as Nails (LOT-132) : la page imprime CA 14 = 10 + Dex 1 + Con 3. L'ecart ecrit au
    // registre du LOT-130 est referme.
    EXPECT_EQ(classeDArmure(charge), 14) << "Tough as Nails : 10 + Dex 1 + Con 3";

    verifierSauvegardes(fiche, {5, 1, 5, 0, 1, -1});
    verifierCompetences(fiche, {
                                   {"acrobatics", 1, false},
                                   {"animal-handling", 3, true},
                                   {"arcana", 0, false},
                                   {"athletics", 5, true},
                                   {"deception", -1, false},
                                   {"history", 0, false},
                                   {"insight", 1, false},
                                   {"intimidation", 1, true},
                                   {"investigation", 2, true},
                                   {"medicine", 1, false},
                                   {"nature", 2, true},
                                   {"perception", 1, false},
                                   {"performance", -1, false},
                                   {"persuasion", -1, false},
                                   {"religion", 0, false},
                                   {"sleight-of-hand", 1, false},
                                   {"stealth", 1, false},
                                   {"survival", 1, false},
                               });

    verifierAttaque(attaque(fiche, "hache-a-deux-mains"),
                    {"Greataxe", 5, 1, 12, 3, core::DamageType::Slashing});
    verifierAttaque(attaque(fiche, "hachette"),
                    {"Handaxe", 5, 1, 6, 3, core::DamageType::Slashing});
    verifierLancer(fiche, "hachette", 4, 12);
    verifierAttaque(attaque(fiche, "javeline"),
                    {"Javelin", 5, 1, 6, 3, core::DamageType::Piercing});
    verifierLancer(fiche, "javeline", 6, 24);
}

/**
 * @brief La fiche du Mage (p. 199) se recalcule valeur pour valeur.
 * \castest{<b>Le Mage pre-tire redonne chaque valeur de la page 199.</b><br/>
 * \tcat Unitaire · Fiches pre-tirees<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger Rpg/characters/heros-mage.json.<br/>2. Recalculer caracteristiques (dont le
 * +1 au choix de l'elfe d'automne, pose en Constitution), points de vie, CA, initiative, vitesse,
 * Perception passive, sauvegardes, dix-huit competences, quarterstaff.<br/>
 * \tattendu For 8 Dex 15 Con 15 Int 16 Sag 12 Cha 10 ; 8 PV ; CA 12 ; initiative +2 ; 30 ft ;
 * Perception passive 13 ; sauvegardes -1 +2 +2 +5 +3 0 ; les cinq pastilles ; quarterstaff +1,
 * 1d6-1 contondant. Fire bolt n'est pas une attaque d'arme : il attend les sorts du LOT-133.
 * }
 */
TEST(PremadeCharactersTest, LeMageEstLaPage199) {
    const core::LoadedCharacterSheet charge = charger("heros-mage.json");
    const core::CharacterSheet& fiche = charge.sheet;

    EXPECT_EQ(fiche.speciesId, "elfe-d-automne");
    EXPECT_EQ(fiche.backgroundId, "cartographer");
    // DECISION ECRITE (registre du LOT-130) : la page imprime Con 14 ; l'elfe d'automne accorde
    // +1 a une caracteristique au choix hors Dex et Int, que la page n'applique pas. La regle
    // prime : le +1 va en Constitution, ou il ne change aucun modificateur -- tout le reste de la
    // page reste juste.
    EXPECT_EQ(fiche.abilities, (std::array<int, 6>{8, 15, 15, 16, 12, 10}));
    EXPECT_EQ(fiche.maximumHitPoints, 8) << "1d6 au maximum, +2 de Constitution";
    EXPECT_EQ(classeDArmure(charge), 12) << "sans armure, 10 + Dex 2 ; Arcane Protection au N2";
    EXPECT_EQ(fiche.modifier(core::Ability::Dexterity), 2) << "initiative +2";
    EXPECT_FLOAT_EQ(fiche.speedMeters, 9.0F) << "30 ft";
    EXPECT_EQ(perceptionPassive(fiche), 13);
    EXPECT_EQ(fiche.languages, (std::set<std::string>{"common", "elvish"}));

    verifierSauvegardes(fiche, {-1, 2, 2, 5, 3, 0});
    verifierCompetences(fiche, {
                                   {"acrobatics", 2, false},
                                   {"animal-handling", 1, false},
                                   {"arcana", 5, true},
                                   {"athletics", -1, false},
                                   {"deception", 0, false},
                                   {"history", 5, true},
                                   {"insight", 1, false},
                                   {"intimidation", 0, false},
                                   {"investigation", 5, true},
                                   {"medicine", 1, false},
                                   {"nature", 3, false},
                                   {"perception", 3, true},
                                   {"performance", 0, false},
                                   {"persuasion", 0, false},
                                   {"religion", 3, false},
                                   {"sleight-of-hand", 2, false},
                                   {"stealth", 2, false},
                                   {"survival", 3, true},
                               });

    // La page imprime 1d6-1 / 1d8-1 : la seconde forme est la propriete polyvalente, que le
    // moteur ne joue pas encore ; la premiere se verifie.
    verifierAttaque(attaque(fiche, "baton"),
                    {"Quarterstaff", 1, 1, 6, -1, core::DamageType::Bludgeoning});
}

/**
 * @brief La fiche du Priest (p. 203) se recalcule valeur pour valeur.
 * \castest{<b>Le Priest pre-tire redonne chaque valeur de la page 203.</b><br/>
 * \tcat Unitaire · Fiches pre-tirees<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger Rpg/characters/heros-priest.json.<br/>2. Recalculer caracteristiques,
 * points de vie (avec la Tenacite naine), CA (ecailles + bouclier), initiative, vitesse,
 * Perception passive, sauvegardes, dix-huit competences, warhammer et handaxe.<br/>
 * \tattendu For 13 Dex 12 Con 16 Int 10 Sag 16 Cha 8 ; 12 PV ; CA 17 ; initiative +1 ; 25 ft ;
 * Perception passive 13 ; sauvegardes +1 +1 +3 0 +5 +1 ; les quatre pastilles ; warhammer +3
 * 1d8+1 contondant ; handaxe +3 1d6+1 TRANCHANT (la page ecrit « Piercing » : coquille).
 * Sacred Flame est jouee depuis le LOT-134 (test_class_priest.cpp).
 * }
 */
TEST(PremadeCharactersTest, LePriestEstLaPage203) {
    const core::LoadedCharacterSheet charge = charger("heros-priest.json");
    const core::CharacterSheet& fiche = charge.sheet;

    EXPECT_EQ(fiche.speciesId, "nain-des-collines");
    EXPECT_EQ(fiche.backgroundId, "community-leader");
    EXPECT_EQ(fiche.abilities, (std::array<int, 6>{13, 12, 16, 10, 16, 8}));
    EXPECT_EQ(fiche.hitPointsPerLevelBonus, 1) << "Tenacite naine, lue dans l'espece";
    EXPECT_EQ(fiche.maximumHitPoints, 12) << "1d8 au maximum, +3 de Constitution, +1 de Tenacite";
    EXPECT_EQ(classeDArmure(charge), 17) << "ecailles 14 + Dex 1 (plafond 2) + bouclier 2";
    EXPECT_EQ(fiche.modifier(core::Ability::Dexterity), 1) << "initiative +1";
    EXPECT_FLOAT_EQ(fiche.speedMeters, 7.5F) << "25 ft";
    EXPECT_EQ(perceptionPassive(fiche), 13);
    // ECART ECRIT : Community Leader accorde deux langues au choix ; la page n'en choisit
    // aucune, la fiche non plus.
    EXPECT_EQ(fiche.languages, (std::set<std::string>{"common", "dwarvish"}));

    verifierSauvegardes(fiche, {1, 1, 3, 0, 5, 1});
    verifierCompetences(fiche, {
                                   {"acrobatics", 1, false},
                                   {"animal-handling", 3, false},
                                   {"arcana", 0, false},
                                   {"athletics", 1, false},
                                   {"deception", -1, false},
                                   {"history", 2, true},
                                   {"insight", 5, true},
                                   {"intimidation", -1, false},
                                   {"investigation", 0, false},
                                   {"medicine", 3, false},
                                   {"nature", 0, false},
                                   {"perception", 3, false},
                                   {"performance", -1, false},
                                   {"persuasion", 1, true},
                                   {"religion", 2, true},
                                   {"sleight-of-hand", 1, false},
                                   {"stealth", 1, false},
                                   {"survival", 3, false},
                               });

    verifierAttaque(attaque(fiche, "marteau-de-guerre"),
                    {"Warhammer", 3, 1, 8, 1, core::DamageType::Bludgeoning});
    // COQUILLE DU LIVRE (registre du LOT-130) : la page ecrit « Piercing » pour la hachette ; la
    // table des armes la donne tranchante, et c'est le catalogue qui fait foi.
    verifierAttaque(attaque(fiche, "hachette"),
                    {"Handaxe", 3, 1, 6, 1, core::DamageType::Slashing});
}

/**
 * @brief La fiche du Scoundrel (p. 207) se recalcule valeur pour valeur.
 * \castest{<b>Le Scoundrel pre-tire redonne chaque valeur de la page 207.</b><br/>
 * \tcat Unitaire · Fiches pre-tirees<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger Rpg/characters/heros-scoundrel.json.<br/>2. Recalculer caracteristiques,
 * points de vie, CA (cuir), initiative, vitesse, Perception passive, sauvegardes, dix-huit
 * competences, rapier, shortbow et dagger.<br/>
 * \tattendu For 9 Dex 16 Con 15 Int 10 Sag 14 Cha 14 ; 10 PV ; CA 14 ; initiative +3 ;
 * sauvegardes -1 +5 +2 +2 +2 +2 ; les six pastilles ; rapier +5 1d8+3, shortbow +5 1d6+3 portee
 * 80/320, dagger +5 1d4+3 lancer 20/60. Deux coquilles ecrites : la Perception passive vaut 14
 * (la page imprime 13) ; la vitesse vaut 40 ft par Scoundrel's Agility (LOT-135), la page
 * imprime 30.
 * }
 */
TEST(PremadeCharactersTest, LeScoundrelEstLaPage207) {
    const core::LoadedCharacterSheet charge = charger("heros-scoundrel.json");
    const core::CharacterSheet& fiche = charge.sheet;

    EXPECT_EQ(fiche.speciesId, "humain");
    EXPECT_EQ(fiche.backgroundId, "undercover");
    EXPECT_EQ(fiche.abilities, (std::array<int, 6>{9, 16, 15, 10, 14, 14}));
    EXPECT_EQ(fiche.maximumHitPoints, 10) << "1d8 au maximum, +2 de Constitution";
    EXPECT_EQ(classeDArmure(charge), 14) << "cuir 11 + Dex 3";
    EXPECT_EQ(fiche.modifier(core::Ability::Dexterity), 3) << "initiative +3";
    // ECART ECRIT (registre du LOT-130) : la page imprime 30 ft ; Scoundrel's Agility donne +10 ft
    // des le niveau 1, la regle prime (40 ft), jouee depuis le LOT-135.
    EXPECT_FLOAT_EQ(fiche.speedMeters, 9.0F) << "30 ft de l'humain";
    EXPECT_FLOAT_EQ(fiche.effectiveSpeedMeters(), 12.0F) << "40 ft par Scoundrel's Agility";
    // COQUILLE DU LIVRE : la page imprime 13 ; Perception +4 donne 14, et le calcul prime.
    EXPECT_EQ(perceptionPassive(fiche), 14);
    EXPECT_EQ(fiche.languages, (std::set<std::string>{"common", "elvish"}));

    verifierSauvegardes(fiche, {-1, 5, 2, 2, 2, 2});
    verifierCompetences(fiche, {
                                   {"acrobatics", 5, true},
                                   {"animal-handling", 2, false},
                                   {"arcana", 0, false},
                                   {"athletics", -1, false},
                                   {"deception", 4, true},
                                   {"history", 0, false},
                                   {"insight", 4, true},
                                   {"intimidation", 2, false},
                                   {"investigation", 2, true},
                                   {"medicine", 2, false},
                                   {"nature", 0, false},
                                   {"perception", 4, true},
                                   {"performance", 2, false},
                                   {"persuasion", 2, false},
                                   {"religion", 0, false},
                                   {"sleight-of-hand", 5, true},
                                   {"stealth", 3, false},
                                   {"survival", 2, false},
                               });

    verifierAttaque(attaque(fiche, "rapiere"), {"Rapier", 5, 1, 8, 3, core::DamageType::Piercing});
    const core::AttackProfile arc = attaque(fiche, "arc-court");
    verifierAttaque(arc, {"Shortbow", 5, 1, 6, 3, core::DamageType::Piercing});
    ASSERT_TRUE(arc.range.has_value()) << "le shortbow tire a 80/320 ft";
    EXPECT_EQ(arc.range->normal, 16) << "80 ft = 16 cases";
    EXPECT_EQ(arc.range->maximum, 64) << "320 ft = 64 cases";
    verifierAttaque(attaque(fiche, "dague"), {"Dagger", 5, 1, 4, 3, core::DamageType::Piercing});
    verifierLancer(fiche, "dague", 4, 12);
}

/**
 * @brief La Tenacite naine compte a chaque niveau, hors du plancher de 1 par niveau.
 * \castest{<b>Les points de vie par niveau d'une espece s'ajoutent au niveau 1 et a chaque
 * montee.</b><br/>
 * \tcat Unitaire · Fiches pre-tirees<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Calculer les points de vie d'un d8 de niveau 1 puis 5, avec et sans le bonus.<br/>
 * 2. Faire monter le Priest au niveau 5 par l'experience.<br/>
 * \tattendu Un point de plus par niveau dans les deux cas ; le bonus survit a une Constitution
 * desastreuse, que le plancher ne l'absorbe pas.
 * }
 */
TEST(PremadeCharactersTest, LaTenaciteNaineCompteAChaqueNiveau) {
    EXPECT_EQ(core::maximumHitPointsFor(8, 1, 3, 1), core::maximumHitPointsFor(8, 1, 3) + 1);
    EXPECT_EQ(core::maximumHitPointsFor(8, 5, 3, 1), core::maximumHitPointsFor(8, 5, 3) + 5);
    // Constitution -5 : chaque niveau apres le premier tombe au plancher de 1, et le bonus
    // s'ajoute PAR-DESSUS -- 2 par niveau, pas 1.
    EXPECT_EQ(core::maximumHitPointsFor(8, 3, -5, 1), core::maximumHitPointsFor(8, 3, -5) + 3);

    core::LoadedCharacterSheet charge = charger("heros-priest.json");
    const Catalogues& lus = catalogues();
    const core::PlayableClass* const priest = lus.options.findClass("priest");
    ASSERT_NE(priest, nullptr);
    const core::LevelUpResult montee = core::gainExperience(
        charge.sheet, lus.experience, priest->hitDie, lus.experience.thresholdAt(5));
    EXPECT_EQ(montee.newLevel, 5);
    // 8 + 3 + 1 au niveau 1, puis quatre fois (5 + 3 + 1).
    EXPECT_EQ(charge.sheet.maximumHitPoints, 12 + 4 * 9);
}

/**
 * @brief Le +1 au choix d'une espece s'applique depuis la fiche, sous le plafond.
 * \castest{<b>Le choix d'augmentation d'espece d'une fiche s'ajoute apres la table de l'espece,
 * sans depasser le plafond.</b><br/>
 * \tcat Unitaire · Fiches pre-tirees<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Construire un elfe d'automne avec Con 14 et un choix de +1 en Constitution.<br/>
 * 2. Construire le meme avec Con 20 et le meme choix.<br/>
 * \tattendu Con 15 dans le premier cas ; Con 20 dans le second, le plafond de la donnee tient.
 * }
 */
TEST(PremadeCharactersTest, LeChoixDEspeceSAjouteSousLePlafond) {
    const Catalogues& lus = catalogues();
    const core::Species* const elfe = lus.options.findSpecies("elfe-d-automne");
    ASSERT_NE(elfe, nullptr);
    constexpr std::array<int, 6> CHOIX_CON = {0, 0, 1, 0, 0, 0};

    const core::CharacterSheet ordinaire =
        core::buildCharacterSheet("Essai", {8, 13, 14, 15, 12, 10}, elfe, nullptr, nullptr,
                                  lus.rules, lus.experience, CHOIX_CON);
    EXPECT_EQ(ordinaire.ability(core::Ability::Constitution), 15);
    EXPECT_EQ(ordinaire.ability(core::Ability::Intelligence), 16) << "la table de l'espece d'abord";

    const core::CharacterSheet auPlafond =
        core::buildCharacterSheet("Essai", {8, 13, 20, 15, 12, 10}, elfe, nullptr, nullptr,
                                  lus.rules, lus.experience, CHOIX_CON);
    EXPECT_EQ(auPlafond.ability(core::Ability::Constitution), lus.rules.maximumAbilityScore);
}

/**
 * @brief Une sous-espece herite de son espece parente au chargement.
 * \castest{<b>Le nain des collines est un nain : +2 de Constitution, le commun et le nain, la
 * vision dans le noir, puis ce qui est le sien.</b><br/>
 * \tcat Unitaire · Fiches pre-tirees<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger les especes.<br/>2. Lire le nain des collines et l'elfe d'automne.<br/>
 * \tattendu Con +2 et Sag +1, 1 PV par niveau, deux langues et les traits du nain avant les
 * siens ; Dex +2 et Int +1 pour l'elfe, avec son mecanisme requis conserve.
 * }
 */
TEST(PremadeCharactersTest, UneSousEspeceHeriteDeSonParent) {
    const Catalogues& lus = catalogues();
    const core::Species* const nain = lus.options.findSpecies("nain-des-collines");
    ASSERT_NE(nain, nullptr);
    EXPECT_EQ(nain->increase(core::Ability::Constitution), 2) << "herite du nain";
    EXPECT_EQ(nain->increase(core::Ability::Wisdom), 1) << "le sien";
    EXPECT_EQ(nain->hitPointsPerLevel, 1);
    EXPECT_EQ(nain->languages, (std::vector<std::string>{"common", "dwarvish"}));
    ASSERT_GE(nain->traits.size(), 2U);
    EXPECT_EQ(nain->traits.front().name, "Vision dans le noir") << "les traits du parent d'abord";
    EXPECT_EQ(nain->traits.back().name, "Ténacité naine");

    const core::Species* const elfe = lus.options.findSpecies("elfe-d-automne");
    ASSERT_NE(elfe, nullptr);
    EXPECT_EQ(elfe->increase(core::Ability::Dexterity), 2);
    EXPECT_EQ(elfe->increase(core::Ability::Intelligence), 1);
    EXPECT_EQ(elfe->requiredMechanisms,
              (std::vector<std::string>{"augmentation-de-caracteristique-au-choix"}));

    const core::Species* const parent = lus.options.findSpecies("nain");
    ASSERT_NE(parent, nullptr);
    EXPECT_EQ(parent->increase(core::Ability::Wisdom), 0) << "le parent ne recoit rien en retour";
}
