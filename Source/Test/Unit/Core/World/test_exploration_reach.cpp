// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_exploration_reach.cpp
 * @brief Tests des cases qu'un héros en exploration atteint (`LOT-EDITOR-07`) : la règle de marche
 *        du jeu, lue case par case.
 */

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Core/World/ExplorationReach.h"

namespace {

// Une grille lue ligne à ligne : `#` un mur, `.` du sol.
[[nodiscard]] core::TileMap grille(const std::vector<std::string>& lignes) {
    core::TileMap carte(static_cast<int>(lignes.front().size()), static_cast<int>(lignes.size()));
    for (int row = 0; row < carte.height(); ++row) {
        for (int column = 0; column < carte.width(); ++column) {
            carte.setTile(
                column, row,
                lignes[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] == '#'
                    ? core::TileType::Wall
                    : core::TileType::Dirt);
        }
    }
    return carte;
}

}  // namespace

/**
 * @brief Un couloir d'une case se passe ; un mur le ferme.
 * \castest{<b>Le héros suit un couloir d'une case, jusqu'au mur.</b><br/>
 * \tcat Unitaire · Atteignabilité<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Une salle, un couloir d'une case, une salle derrière un mur.<br/>2. Parcourir
 * depuis la première salle.<br/>
 * \tattendu Le couloir et son bout sont atteints ; la salle murée ne l'est pas.
 * }
 */
TEST(ExplorationReachTest, UnCouloirSePasseUnMurLeFerme) {
    const core::TileMap carte = grille({
        "..#....",
        "......#",
        "..#.#.#",
        "#####.#",
        "..#####",
    });
    const core::ExplorationReach atteinte(carte, {{.column = 0, .row = 0}});

    EXPECT_TRUE(atteinte.reaches({.column = 5, .row = 3}));   // le bout du couloir
    EXPECT_TRUE(atteinte.reaches({.column = 3, .row = 2}));   // l'alcôve
    EXPECT_FALSE(atteinte.reaches({.column = 0, .row = 4}));  // la salle murée
    EXPECT_FALSE(atteinte.reaches({.column = 2, .row = 0}));  // un mur
    EXPECT_FALSE(atteinte.reaches({.column = 9, .row = 9}));  // hors de la carte
}

/**
 * @brief Deux murs en diagonale ferment le passage : le héros ne passe pas par un coin.
 * \castest{<b>On ne passe pas entre deux murs en diagonale.</b><br/>
 * \tcat Unitaire · Atteignabilité<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Deux cases libres qui ne se touchent que par un coin, entre deux murs.<br/>
 * \tattendu La seconde n'est pas atteinte : `ExplorationSession` refuse le pas en diagonale.
 * }
 */
TEST(ExplorationReachTest, DeuxMursEnDiagonaleFermentLePassage) {
    const core::TileMap carte = grille({
        ".#",
        "#.",
    });
    const core::ExplorationReach atteinte(carte, {{.column = 0, .row = 0}});

    EXPECT_TRUE(atteinte.reaches({.column = 0, .row = 0}));
    EXPECT_FALSE(atteinte.reaches({.column = 1, .row = 1}));
    EXPECT_EQ(atteinte.count(), 1U);
}

/**
 * @brief Un départ muré ou hors de la carte n'atteint rien ; plusieurs départs s'additionnent.
 * \castest{<b>Un départ muré n'atteint rien.</b><br/>
 * \tcat Unitaire · Atteignabilité<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Partir d'un mur, puis de hors de la carte, puis des deux côtés d'un mur.<br/>
 * \tattendu Rien, rien, puis les deux côtés.
 * }
 */
TEST(ExplorationReachTest, UnDepartMureNAtteintRien) {
    const core::TileMap carte = grille({".#."});

    EXPECT_EQ(core::ExplorationReach(carte, {{.column = 1, .row = 0}}).count(), 0U);
    EXPECT_EQ(core::ExplorationReach(carte, {{.column = -1, .row = 0}}).count(), 0U);
    EXPECT_EQ(
        core::ExplorationReach(carte, {{.column = 0, .row = 0}, {.column = 2, .row = 0}}).count(),
        2U);
}
