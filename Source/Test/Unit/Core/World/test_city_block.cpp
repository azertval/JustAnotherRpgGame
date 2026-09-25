// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_city_block.cpp
 * @brief Les ilots d'un quartier, lus sur sa carte (LOT-96).
 */

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/Level.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileMap.h"
#include "Core/World/CityBlock.h"

namespace {

[[nodiscard]] core::MapEntity ilot(const std::string& nom, int x, int y, std::int64_t largeur,
                                   std::int64_t hauteur) {
    core::MapEntity entite;
    entite.type = std::string{core::CITY_BLOCK_ENTITY_TYPE};
    entite.position = {x, y};
    entite.properties[std::string{core::CITY_BLOCK_NAME_PROPERTY}] = nom;
    entite.properties[std::string{core::CITY_BLOCK_WIDTH_PROPERTY}] = largeur;
    entite.properties[std::string{core::CITY_BLOCK_HEIGHT_PROPERTY}] = hauteur;
    return entite;
}

}  // namespace

/**
 * @brief Les ilots se lisent dans l'ordre de la carte, et les saisies fautives sont ecartees.
 * \castest{<b>Un ilot est un rectangle nomme de la carte ; un ilot sans nom ou vide est
 * ecarte.</b><br/>
 * \tcat Unitaire · Plan de la ville<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser sur une carte deux ilots valides, un sans nom, un de largeur nulle, et une
 * entite d'un autre type.<br/>
 * 2. Lire les ilots ; demander a chacun s'il contient quelques cases.<br/>
 * \tattendu Les deux ilots valides, dans l'ordre des entites ; un ilot contient ses bords haut
 * et gauche, pas ceux d'apres (LOT-96).
 * }
 */
TEST(CityBlockTest, LesIlotsSeLisentEtLesSaisiesFautivesSontEcartees) {
    core::LevelData donnees{.tileMap = core::TileMap{20, 20}};
    donnees.entities = {ilot("place", 2, 3, 5, 4), ilot("", 0, 0, 2, 2), ilot("vide", 1, 1, 0, 3),
                        ilot("ruelles", 10, 10, 3, 3)};
    core::MapEntity autre;
    autre.type = "spawnPoint";
    donnees.entities.push_back(autre);
    const core::Level carte{std::move(donnees)};

    const std::vector<core::CityBlock> ilots = core::cityBlocksOf(carte);
    ASSERT_EQ(ilots.size(), 2U);
    EXPECT_EQ(ilots[0].name, "place");
    EXPECT_EQ(ilots[1].name, "ruelles");
    EXPECT_EQ(ilots[0].columns, 5);
    EXPECT_EQ(ilots[0].rows, 4);

    EXPECT_TRUE(ilots[0].contains({2, 3}));
    EXPECT_TRUE(ilots[0].contains({6, 6}));
    EXPECT_FALSE(ilots[0].contains({7, 3}));
    EXPECT_FALSE(ilots[0].contains({2, 7}));
}
