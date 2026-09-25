// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_map_encounter.cpp
 * @brief Tests d'une rencontre posée sur la carte (`LOT-118`) : la zone choisie, la grille
 *        découpée, les places ramenées dans la zone.
 */

#include <algorithm>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/MapEncounter.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileMap.h"

namespace {

/// Une carte de 30 x 20, ouverte, avec une zone de combat « cour » de 10 x 8 en (10, 5) et un mur
/// sur la case (12, 6).
[[nodiscard]] core::Level carte() {
    core::TileMap sol{30, 20};
    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 30; ++x) {
            sol.setTile(x, y, core::TileType::Grass);
        }
    }
    sol.setTile(12, 6, core::TileType::Wall);
    core::LevelData donnees{.name = "essai", .tileMap = std::move(sol)};
    donnees.entities.push_back(
        core::MapEntity{.type = "combatZone",
                        .position = {.column = 10, .row = 5},
                        .properties = {{"name", std::string{"cour"}},
                                       {"width", std::int64_t{10}},
                                       {"height", std::int64_t{8}}}});
    return core::Level{std::move(donnees)};
}

[[nodiscard]] core::Encounter rencontre() {
    return core::Encounter{.id = "loups",
                           .name = "Deux loups",
                           .source = "essai",
                           .combatants = {{.creatureId = "wolf", .columnOffset = 0, .rowOffset = -1},
                                          {.creatureId = "wolf", .columnOffset = 1, .rowOffset = 0}},
                           .escapable = false};
}

[[nodiscard]] core::ExplorationSnapshot exploration() {
    return core::ExplorationSnapshot{.playerPosition = {15.5F, 10.5F},
                                     .playerFacing = {1.0F, 0.0F},
                                     .cameraPosition = {15.5F, 10.5F},
                                     .captured = true};
}

}  // namespace

/**
 * @brief La rencontre se monte sur la zone qui contient le déclencheur, en cases de la grille.
 * \castest{<b>Une rencontre se pose sur la zone de combat du declencheur, cases translatees.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Preparer une rencontre de deux loups, declenchee en (14, 8), heros en (15, 10),
 * sur une carte dont la zone « cour » couvre (10, 5) a (19, 12).<br/>
 * \tattendu La zone « cour » est choisie ; la grille fait 10 x 8 ; le heros est en (5, 5) de la
 * grille ; le loup « un pas devant » est en (4, 2), l'autre en (5, 3) ; la rencontre n'est pas
 * fuyable ; aucune place n'a bouge.
 * }
 */
TEST(MapEncounterTest, LaZoneDuDeclencheurEstChoisieEtLesCasesTranslatees) {
    const core::MapEncounterResult resultat =
        core::prepareMapEncounter(carte(), "essai", rencontre(), {.column = 14, .row = 8},
                                  {.column = 15, .row = 10}, exploration(), "essai/loups");
    ASSERT_TRUE(resultat.ok()) << resultat.issue;
    const core::MapEncounterSetup& montage = *resultat.setup;
    EXPECT_EQ(montage.zone.name, "cour");
    EXPECT_EQ(montage.battlefield.tileMap().width(), 10);
    EXPECT_EQ(montage.battlefield.tileMap().height(), 8);
    EXPECT_EQ(montage.heroCell, (core::GridPosition{.column = 5, .row = 5}));
    ASSERT_EQ(montage.run.placements.size(), 2U);
    EXPECT_EQ(montage.run.placements[0].position, (core::GridPosition{.column = 4, .row = 2}));
    EXPECT_EQ(montage.run.placements[1].position, (core::GridPosition{.column = 5, .row = 3}));
    EXPECT_FALSE(montage.run.escapable);
    EXPECT_EQ(montage.run.defeatFlagKey, "essai/loups");
    EXPECT_TRUE(montage.notes.empty());
    EXPECT_EQ(core::zoneToMap(montage.zone, montage.heroCell),
              (core::GridPosition{.column = 15, .row = 10}));
    EXPECT_EQ(core::mapToZone(montage.zone, {.column = 15, .row = 10}), montage.heroCell);
}

/**
 * @brief Une place qui tombe dans un mur, hors de la zone ou sur une case prise se rapproche de
 *        ce qu'elle voulait, et la note le dit.
 * \castest{<b>Une place impossible est rapprochee de la case voulue, et notee.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Declencher en (12, 7) : le loup « un pas devant » vise (12, 6), un mur.<br/>
 * 2. Declencher en (10, 5) avec le heros hors de la zone, en (2, 2) : le second loup vise
 * (11, 5), la place du heros vise une case hors zone.<br/>
 * \tattendu Le loup du mur est pose sur une case libre voisine de (12, 6), et une note le dit ;
 * le heros hors zone est pose sur la case libre de la zone la plus proche de lui, (10, 5) ; deux
 * combattants ne partagent jamais une case.
 * }
 */
TEST(MapEncounterTest, UnePlaceImpossibleSeRapprocheEtSeNote) {
    const core::MapEncounterResult mur =
        core::prepareMapEncounter(carte(), "essai", rencontre(), {.column = 12, .row = 7},
                                  {.column = 15, .row = 10}, exploration(), {});
    ASSERT_TRUE(mur.ok()) << mur.issue;
    const core::GridPosition posee = core::zoneToMap(mur.setup->zone, mur.setup->run.placements[0].position);
    EXPECT_NE(posee, (core::GridPosition{.column = 12, .row = 6}));
    EXPECT_LE(std::max(std::abs(posee.column - 12), std::abs(posee.row - 6)), 1);
    ASSERT_EQ(mur.setup->notes.size(), 1U);
    EXPECT_NE(mur.setup->notes.front().find("wolf"), std::string::npos);

    const core::MapEncounterResult dehors =
        core::prepareMapEncounter(carte(), "essai", rencontre(), {.column = 10, .row = 5},
                                  {.column = 2, .row = 2}, exploration(), {});
    ASSERT_TRUE(dehors.ok()) << dehors.issue;
    EXPECT_EQ(core::zoneToMap(dehors.setup->zone, dehors.setup->heroCell),
              (core::GridPosition{.column = 10, .row = 5}));
    std::vector<core::GridPosition> cases{dehors.setup->heroCell};
    for (const core::CombatantPlacement& place : dehors.setup->run.placements) {
        EXPECT_EQ(std::ranges::find(cases, place.position), cases.end()) << "case partagee";
        cases.push_back(place.position);
        EXPECT_TRUE(dehors.setup->zone.contains(core::zoneToMap(dehors.setup->zone, place.position)));
    }
}

/**
 * @brief Sans zone de combat autour du déclencheur ni du héros, la rencontre est refusée, et la
 *        raison est écrite.
 * \castest{<b>Une rencontre hors de toute zone de combat est refusee.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Declencher en (2, 2), heros en (3, 3), hors de la zone « cour ».<br/>
 * \tattendu Pas de montage ; la raison nomme la carte et les deux cases.
 * }
 */
TEST(MapEncounterTest, SansZoneLaRencontreEstRefusee) {
    const core::MapEncounterResult resultat =
        core::prepareMapEncounter(carte(), "essai", rencontre(), {.column = 2, .row = 2},
                                  {.column = 3, .row = 3}, exploration(), {});
    EXPECT_FALSE(resultat.ok());
    EXPECT_NE(resultat.issue.find("essai"), std::string::npos);
    EXPECT_NE(resultat.issue.find("2,2"), std::string::npos);
    EXPECT_NE(resultat.issue.find("3,3"), std::string::npos);
}
