// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_level_loader.cpp
 * @brief Tests unitaires du chargement de niveau (format JSON, erreurs récupérables).
 */

#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileType.h"

namespace {

// Un niveau JSON valide et minimal (une entrée, un solide, un mur).
constexpr const char* VALID_LEVEL = R"({
  "name": "Tutoriel",
  "width": 4,
  "height": 3,
  "tiles": [
    { "x": 0, "y": 0, "type": "solid" },
    { "x": 1, "y": 1, "type": "entry" },
    { "x": 3, "y": 0, "type": "wall" }
  ]
})";

}  // namespace

/**
 * @brief Un niveau valide est chargé avec ses dimensions, ses tuiles et son entrée.
 * \castest{<b>Un niveau valide est chargé avec ses dimensions, ses tuiles et son entrée.</b><br/>
 * \tcat Unitaire · Level Loader<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Un niveau valide est chargé avec ses dimensions, ses tuiles et son entrée.
 * }
 */
TEST(LevelLoaderTest, ChargeUnNiveauValide) {
    const core::LevelLoadResult result = core::LevelLoader::loadFromString(VALID_LEVEL);
    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_EQ(result.errorCode, core::LevelValidationError::None);

    const core::Level& level = *result.level;
    EXPECT_EQ(level.name(), "Tutoriel");
    EXPECT_EQ(level.tileMap().width(), 4);
    EXPECT_EQ(level.tileMap().height(), 3);
    EXPECT_EQ(level.tileMap().tile(0, 0), core::TileType::Solid);
    EXPECT_EQ(level.tileMap().tile(1, 1), core::TileType::Entry);
    EXPECT_EQ(level.tileMap().tile(3, 0), core::TileType::Wall);
    EXPECT_EQ(level.entry(), (core::GridPosition{1, 1}));
}

/**
 * @brief Un niveau sans champ `"version"` se charge sans erreur ni avertissement, comme la
 * version initiale du format (`EX-LVL-005`, rétrocompatibilité des niveaux antérieurs à ce
 * champ).
 * \castest{<b>Un niveau sans champ version se charge sans erreur.</b><br/>
 * \tcat Unitaire · Level Loader<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Un niveau sans champ version se charge sans erreur.
 * }
 */
TEST(LevelLoaderTest, NiveauSansVersionSeChargeSansErreur) {
    const core::LevelLoadResult result = core::LevelLoader::loadFromString(VALID_LEVEL);
    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_EQ(result.errorCode, core::LevelValidationError::None);
}

/**
 * @brief Un niveau dont la version dépasse celle gérée échoue avec une erreur exploitable
 * (`EX-LVL-005`), pas une lecture au mieux.
 * \castest{<b>Un niveau dont la version depasse celle geree echoue proprement.</b><br/>
 * \tcat Unitaire · Level Loader<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Un niveau dont la version depasse celle geree echoue proprement.
 * }
 */
TEST(LevelLoaderTest, VersionSuperieureALaVersionGereeEchoueProprement) {
    constexpr const char* LEVEL = R"({
      "version": 999,
      "width": 4,
      "height": 3,
      "tiles": [
        { "x": 1, "y": 1, "type": "entry" }
      ]
    })";
    const core::LevelLoadResult result = core::LevelLoader::loadFromString(LEVEL);
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errorCode, core::LevelValidationError::UnsupportedFormatVersion);
}

/**
 * @brief Une carte aux dimensions aberrantes est refusée avant d'allouer sa grille.
 * \castest{<b>Une carte plus grande que MAX_LEVEL_SIDE est refusée.</b><br/>
 * \tcat Unitaire · Level Loader<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Charger une carte de 100 000 × 100 000 cases.<br/>2. Charger une carte d'une case
 * de plus que la borne en hauteur.<br/>
 * \tattendu Les deux chargements échouent avec `LevelValidationError::ParseError`, sans allouer
 * la grille.
 * }
 */
TEST(LevelLoaderTest, DimensionsAberrantesRefuseesSansAllouer) {
    const std::string tooLargeHeight = std::to_string(core::MAX_LEVEL_SIDE + 1);
    const std::vector<std::string> levels = {
        R"({ "width": 100000, "height": 100000, "tiles": [] })",
        R"({ "width": 4, "height": )" + tooLargeHeight + R"(, "tiles": [] })"};
    for (const std::string& levelJson : levels) {
        const core::LevelLoadResult result = core::LevelLoader::loadFromString(levelJson);
        EXPECT_FALSE(result.ok());
        EXPECT_EQ(result.errorCode, core::LevelValidationError::ParseError);
    }
}

/**
 * @brief Une carte sans entrée, ou qui en porte deux, est refusée avec une erreur exploitable.
 * \castest{<b>Une carte doit porter exactement une entrée.</b><br/>
 * \tcat Unitaire · Level Loader<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger une carte sans tuile d'entrée.<br/>2. Charger une carte à deux tuiles
 * d'entrée.<br/>
 * \tattendu Les deux chargements échouent avec `LevelValidationError::InvalidEntryCount`.
 * }
 */
TEST(LevelLoaderTest, UneCarteDoitPorterExactementUneEntree) {
    constexpr const char* NO_ENTRY = R"({
      "width": 4, "height": 3,
      "tiles": [ { "x": 0, "y": 0, "type": "wall" } ]
    })";
    constexpr const char* TWO_ENTRIES = R"({
      "width": 4, "height": 3,
      "tiles": [ { "x": 0, "y": 0, "type": "entry" }, { "x": 3, "y": 2, "type": "entry" } ]
    })";

    for (const char* levelJson : {NO_ENTRY, TWO_ENTRIES}) {
        const core::LevelLoadResult result = core::LevelLoader::loadFromString(levelJson);
        EXPECT_FALSE(result.ok());
        EXPECT_EQ(result.errorCode, core::LevelValidationError::InvalidEntryCount);
    }
}

/**
 * @brief Un nom de tuile que le format ne connaît pas est refusé, jamais deviné.
 * \castest{<b>Un type de tuile inconnu est refusé au chargement.</b><br/>
 * \tcat Unitaire · Level Loader<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Charger une carte dont une tuile porte le type « exit ».<br/>
 * \tattendu Le chargement échoue avec `LevelValidationError::UnknownTileType`.
 * }
 */
TEST(LevelLoaderTest, TypeDeTuileInconnuRefuse) {
    constexpr const char* LEVEL = R"({
      "width": 4, "height": 3,
      "tiles": [ { "x": 1, "y": 1, "type": "entry" }, { "x": 3, "y": 2, "type": "exit" } ]
    })";
    const core::LevelLoadResult result = core::LevelLoader::loadFromString(LEVEL);
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errorCode, core::LevelValidationError::UnknownTileType);
}
