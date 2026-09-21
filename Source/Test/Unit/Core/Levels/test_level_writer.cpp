// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_level_writer.cpp
 * @brief Tests unitaires de la sérialisation de niveau (round-trip avec LevelLoader, EX-EDIT-011).
 */

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelWriter.h"
#include "Core/Levels/TileType.h"

namespace {

constexpr const char* LEVEL_WITH_TERRAIN = R"({
  "name": "Tutoriel",
  "width": 4,
  "height": 3,
  "tiles": [
    { "x": 0, "y": 0, "type": "solid" },
    { "x": 1, "y": 1, "type": "entry" },
    { "x": 0, "y": 1, "type": "water" },
    { "x": 2, "y": 0, "type": "wall" },
    { "x": 3, "y": 0, "type": "bridge" }
  ]
})";

}  // namespace

/**
 * @brief Le JSON produit porte toujours le champ `"version"`, et aucune des clés que le format ne
 * lit plus (fond, jeu de skins, cadrage).
 * \castest{<b>Le JSON produit porte la version et aucune clé retirée du format.</b><br/>
 * \tcat Unitaire · Level Writer<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Le JSON produit porte la version et aucune clé retirée du format.
 * }
 */
TEST(LevelWriterTest, LeJsonProduitPorteLaVersionEtAucuneCleRetiree) {
    const core::LevelLoadResult loaded = core::LevelLoader::loadFromString(R"({
        "width": 3, "height": 3,
        "tiles": [ {"x":0,"y":0,"type":"entry"} ] })");
    ASSERT_TRUE(loaded.ok()) << loaded.error;

    const std::string json = core::LevelWriter::toJsonString(*loaded.level);
    EXPECT_EQ(json.find("\"background\""), std::string::npos);
    EXPECT_EQ(json.find("\"skinSet\""), std::string::npos);
    EXPECT_EQ(json.find("\"cameraFraming\""), std::string::npos);
    EXPECT_NE(json.find("\"version\""), std::string::npos);
}

/**
 * @brief saveToFile écrit un fichier qui se recharge à l'identique (round-trip disque).
 * \castest{<b>saveToFile écrit un fichier qui se recharge à l'identique (round-trip
 * disque).</b><br/>
 * \tcat Unitaire · Level Writer<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu saveToFile écrit un fichier qui se recharge à l'identique (round-trip disque).
 * }
 */
TEST(LevelWriterTest, SaveToFileEcritUnFichierRechargeable) {
    const core::LevelLoadResult loaded = core::LevelLoader::loadFromString(LEVEL_WITH_TERRAIN);
    ASSERT_TRUE(loaded.ok()) << loaded.error;

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "jadg_test_save_to_file.json";
    ASSERT_TRUE(core::LevelWriter::saveToFile(*loaded.level, path));

    const core::LevelLoadResult reloaded = core::LevelLoader::loadFromFile(path);
    std::filesystem::remove(path);

    ASSERT_TRUE(reloaded.ok()) << reloaded.error;
    EXPECT_EQ(reloaded.level->name(), loaded.level->name());
    EXPECT_EQ(reloaded.level->entry(), loaded.level->entry());
    for (int row = 0; row < loaded.level->tileMap().height(); ++row) {
        for (int column = 0; column < loaded.level->tileMap().width(); ++column) {
            EXPECT_EQ(reloaded.level->tileMap().tile(column, row),
                      loaded.level->tileMap().tile(column, row));
        }
    }
}

/**
 * @brief saveToFile vers un dossier inexistant échoue proprement (récupérable, EX-NFR-040).
 * \castest{<b>saveToFile vers un dossier inexistant échoue proprement (récupérable,
 * EX-NFR-040).</b><br/>
 * \tcat Unitaire · Level Writer<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu saveToFile vers un dossier inexistant échoue proprement (récupérable, EX-NFR-040).
 * }
 */
TEST(LevelWriterTest, SaveToFileVersDossierInexistantEchoueProprement) {
    const core::LevelLoadResult loaded = core::LevelLoader::loadFromString(LEVEL_WITH_TERRAIN);
    ASSERT_TRUE(loaded.ok()) << loaded.error;

    const std::filesystem::path path = "chemin/inexistant/pas_la/niveau.json";
    EXPECT_FALSE(core::LevelWriter::saveToFile(*loaded.level, path));
}

/**
 * @brief Les propriétés de **carte** (`LOT-EDITOR-09`) : région et ambiance traversent l'écriture,
 *        une clé racine inconnue est gardée, et le brouillon les défait comme le reste.
 * \castest{<b>Une carte garde sa région, son ambiance et ses clés inconnues.</b><br/>
 * \tcat Unitaire · Level Writer<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Charger une carte qui porte `region`, `ambience` et une clé inconnue.<br/>
 * 2. L'écrire, la relire.<br/>3. Changer l'ambiance sur un brouillon, puis défaire.<br/>
 * \tattendu Les trois clés sont là après le tour ; l'ambiance changée s'écrit, et `undo` la
 * rend ; une chaîne vide retire la propriété.
 * }
 */
TEST(LevelWriterTest, UneCarteGardeSaRegionSonAmbianceEtSesClesInconnues) {
    const std::string source = R"({
  "version": 4,
  "name": "Tutoriel",
  "region": "central-empire",
  "ambience": "market",
  "authoredBy": "valentin",
  "width": 2,
  "height": 1,
  "tiles": [ { "x": 0, "y": 0, "type": "entry" }, { "x": 1, "y": 0, "type": "dirt" } ]
})";
    const core::LevelLoadResult loaded = core::LevelLoader::loadFromString(source);
    ASSERT_TRUE(loaded.ok()) << loaded.error;
    EXPECT_EQ(std::get<std::string>(
                  loaded.level->properties().at(std::string{core::MAP_REGION_PROPERTY})),
              "central-empire");

    const core::LevelLoadResult reloaded =
        core::LevelLoader::loadFromString(core::LevelWriter::toJsonString(*loaded.level));
    ASSERT_TRUE(reloaded.ok()) << reloaded.error;
    EXPECT_EQ(reloaded.level->properties(), loaded.level->properties());
    EXPECT_EQ(std::get<std::string>(reloaded.level->properties().at("authoredBy")), "valentin");

    core::LevelDraft draft = core::LevelDraft::fromLevel(*loaded.level);
    EXPECT_TRUE(draft.setProperty(std::string{core::MAP_AMBIENCE_PROPERTY}, std::string{"night"}));
    EXPECT_FALSE(draft.setProperty(std::string{core::MAP_AMBIENCE_PROPERTY}, std::string{"night"}));
    EXPECT_NE(draft.toJson().find("\"ambience\": \"night\""), std::string::npos);
    EXPECT_TRUE(draft.undo());
    EXPECT_EQ(
        std::get<std::string>(draft.properties().at(std::string{core::MAP_AMBIENCE_PROPERTY})),
        "market");

    EXPECT_TRUE(draft.setProperty(std::string{core::MAP_REGION_PROPERTY}, std::string{}));
    EXPECT_FALSE(draft.properties().contains(std::string{core::MAP_REGION_PROPERTY}));
    EXPECT_EQ(draft.toJson().find("\"region\""), std::string::npos);
}
