// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_encounter_difficulty.cpp
 * @brief Tests du budget d'une rencontre (`LOT-139`, `EX-CBT-063`) : les cinq étapes du *Guide du
 *        Maître* (p. 82-83), rejouées sur les règles livrées et sur les exemples du livre.
 */

#include <array>
#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "Core/Combat/EncounterDifficulty.h"

namespace {

const std::filesystem::path RULES =
    std::filesystem::path{JADG_RPG_RULES_DIR} / "encounter-difficulty.json";
const std::filesystem::path CREATURES{JADG_RPG_CREATURES_DIR};
const std::filesystem::path RPG{JADG_RPG_DIR};

[[nodiscard]] core::EncounterDifficultyRules regles() {
    core::EncounterDifficultyRules rules = core::loadEncounterDifficultyRules(RULES);
    EXPECT_TRUE(rules.errors.empty()) << (rules.errors.empty() ? "" : rules.errors.front());
    return rules;
}

}  // namespace

/**
 * @brief Les règles livrées portent les quatre catégories, vingt niveaux et la table des PX.
 * \castest{<b>Les regles du budget de rencontre se chargent.</b><br/>
 * \tcat Unitaire · Combat · Budget<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger `Rpg/rules/encounter-difficulty.json`.<br/>
 * \tattendu Quatre categories dans l'ordre du livre ; vingt niveaux ; un indice 1/8 vaut 25 PX,
 * un indice 5 en vaut 1 800, un indice inconnu 0.
 * }
 */
TEST(EncounterDifficultyTest, LesReglesSeChargent) {
    const core::EncounterDifficultyRules rules = regles();
    ASSERT_TRUE(rules.ok());
    EXPECT_EQ(rules.categories,
              (std::vector<std::string>{"facile", "moyenne", "difficile", "mortelle"}));
    EXPECT_EQ(rules.thresholds.size(), 20U);
    EXPECT_EQ(rules.thresholds.front().experience.at("facile"), 25);
    EXPECT_EQ(rules.thresholds.back().experience.at("mortelle"), 12700);
    EXPECT_EQ(core::experienceForChallenge(rules, 0.125F), 25);
    EXPECT_EQ(core::experienceForChallenge(rules, 5.0F), 1800);
    EXPECT_EQ(core::experienceForChallenge(rules, 42.0F), 0);
}

/**
 * @brief L'exemple du livre : trois personnages de niveau 3 et un de niveau 2 ont pour seuils
 *        275, 550, 825 et 1 400 PX ; un gobelours et trois hobgobelins (1 000 PX modifiés) sont
 *        une rencontre difficile.
 * \castest{<b>Les seuils du groupe et le multiplicateur suivent l'exemple du Guide du
 * Maitre.</b><br/>
 * \tcat Unitaire · Combat · Budget<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Sommer les seuils de trois niveaux 3 et un niveau 2.<br/>2. Multiplier 500 PX de
 * quatre monstres.<br/>3. Comparer le multiplicateur d'un groupe de deux et de six.<br/>
 * \tattendu 275 / 550 / 825 / 1 400 ; x2 pour quatre monstres, donc 1 000 ; un groupe de deux
 * contre un monstre seul prend x1,5 ; un groupe de six contre un monstre seul prend x0,5.
 * }
 */
TEST(EncounterDifficultyTest, LExempleDuLivreTient) {
    const core::EncounterDifficultyRules rules = regles();
    const std::array<int, 4> niveaux{3, 3, 3, 2};
    const std::map<std::string, int> seuils = core::partyThresholds(rules, niveaux);
    EXPECT_EQ(seuils.at("facile"), 275);
    EXPECT_EQ(seuils.at("moyenne"), 550);
    EXPECT_EQ(seuils.at("difficile"), 825);
    EXPECT_EQ(seuils.at("mortelle"), 1400);

    EXPECT_DOUBLE_EQ(core::encounterMultiplierFor(rules, 4, 4), 2.0);
    EXPECT_DOUBLE_EQ(core::encounterMultiplierFor(rules, 1, 4), 1.0);
    EXPECT_DOUBLE_EQ(core::encounterMultiplierFor(rules, 2, 4), 1.5);
    EXPECT_DOUBLE_EQ(core::encounterMultiplierFor(rules, 15, 4), 4.0);
    // La taille du groupe (p. 83).
    EXPECT_DOUBLE_EQ(core::encounterMultiplierFor(rules, 1, 2), 1.5);
    EXPECT_DOUBLE_EQ(core::encounterMultiplierFor(rules, 15, 2), 5.0);
    EXPECT_DOUBLE_EQ(core::encounterMultiplierFor(rules, 1, 6), 0.5);
    EXPECT_DOUBLE_EQ(core::encounterMultiplierFor(rules, 4, 6), 1.5);
}

/**
 * @brief La rencontre de la démo, six bandits contre le groupe de quatre au niveau 1, est une
 *        rencontre **difficile** ; une créature inconnue compte pour rien et se dit.
 * \castest{<b>Les bandits de l'Arena of Fate sont une rencontre difficile pour le groupe de
 * depart.</b><br/>
 * \tcat Unitaire · Combat · Budget<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Juger `arene-bandits` pour quatre niveaux 1.<br/>2. Juger une rencontre d'une
 * creature inconnue.<br/>
 * \tattendu 6 x 25 PX, x2 : 300 PX modifies, seuils 100 / 200 / 300 / 400 : « difficile » ; la
 * creature inconnue est nommee, la categorie est vide.
 * }
 */
TEST(EncounterDifficultyTest, LesBanditsDeLaDemoSontUneRencontreDifficile) {
    const core::EncounterDifficultyRules rules = regles();
    const core::Bestiary bestiaire = core::loadBestiary(CREATURES);
    const core::EncounterCatalog rencontres = core::loadEncounters(RPG / "encounters");
    const core::Encounter* const bandits = rencontres.find("arene-bandits");
    ASSERT_NE(bandits, nullptr);
    const std::array<int, 4> niveaux{1, 1, 1, 1};
    const core::EncounterBudget budget = core::rateEncounter(rules, *bandits, bestiaire, niveaux);
    EXPECT_EQ(budget.monsters, 6);
    EXPECT_EQ(budget.monsterExperience, 150);
    EXPECT_DOUBLE_EQ(budget.multiplier, 2.0);
    EXPECT_EQ(budget.adjustedExperience, 300);
    EXPECT_EQ(budget.thresholds.at("difficile"), 300);
    EXPECT_EQ(budget.thresholds.at("mortelle"), 400);
    EXPECT_EQ(budget.category, "difficile");
    EXPECT_TRUE(budget.unknownCreatures.empty());

    core::Encounter inconnue;
    inconnue.id = "essai";
    inconnue.combatants = {{.creatureId = "dragon-de-papier", .columnOffset = 0, .rowOffset = 0}};
    const core::EncounterBudget vide = core::rateEncounter(rules, inconnue, bestiaire, niveaux);
    EXPECT_EQ(vide.monsters, 0);
    EXPECT_TRUE(vide.category.empty());
    ASSERT_EQ(vide.unknownCreatures.size(), 1U);
    EXPECT_EQ(vide.unknownCreatures.front(), "dragon-de-papier");
}
