// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_scene_folders.cpp
 * @brief Un kit rangé en sous-dossiers (`LOT-129`) : le manifeste du lieu cite chaque pièce par son
 *        chemin (`roofs/l/d3/roof-l-d3-ne-c0r0.png`), et le rendu la retrouve, ancre comprise.
 *
 * La clé de la pièce (`scene/<lieu>/<nom>`) ne change pas : les cartes non plus. Seul le fichier
 * descend dans l'arborescence que l'auteur a choisie.
 */

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/Level.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileMap.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/SceneTextureTraits.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

constexpr const char* MANIFEST = R"({
  "version": 1, "disposition": "ville", "tile": [256, 159],
  "textures": {
    "scene/ville/floor-paving-01": {"file": "floors/floor-paving-01.png", "class": "floor",
                                    "footprint": [1, 1], "size": [256, 159], "anchor": [128, 0]},
    "scene/ville/roof-l-d3-ne-c0r0": {"file": "roofs/l/d3/roof-l-d3-ne-c0r0.png", "class": "tall",
                                      "footprint": [1, 1], "size": [200, 120], "anchor": [97, 40]},
    "scene/ville/prop-barrel": {"file": "prop-barrel.png", "class": "tall",
                                "footprint": [1, 1], "size": [60, 90], "anchor": [30, 70]}
  }
})";

}  // namespace

/**
 * @brief L'instantané retient le fichier d'une pièce rangée en sous-dossier, et le chemin de son
 *        image le suit ; une pièce à plat garde `<nom>.png`.
 * \castest{<b>Une piece rangee en sous-dossier se retrouve par son chemin.</b><br/>
 * \tcat Unitaire · Lieu compose · Arborescence<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Adopter un manifeste dont un sol et un toit sont ranges en sous-dossiers, un tonneau
 * a plat.<br/>
 *          2. Tirer l'instantane d'une carte qui les pose, et ses chemins d'images.<br/>
 * \tattendu `Scene/ville/floors/floor-paving-01.png`, `Scene/ville/roofs/l/d3/…` et
 *           `Scene/ville/prop-barrel.png`.
 * }
 */
TEST(SceneFoldersTest, UnePieceRangeeSeRetrouveParSonChemin) {
    hmi::PlaceAppearanceResult read = hmi::PlaceAppearance::loadFromString(
        R"({"version": 1, "place": "ville", "floors": {}, "relief": {}})");
    ASSERT_TRUE(read.ok()) << read.message;
    const core::ScenePieceManifestResult manifest =
        core::ScenePieceManifest::loadFromString(MANIFEST);
    ASSERT_TRUE(manifest.ok()) << manifest.message;
    read.appearance.adoptManifest(manifest.manifest);
    // Le fichier est relatif a Assets/ (LOT-124) : un manifeste lu seul vit dans le dossier propre
    // du lieu que la table nomme.
    EXPECT_EQ(read.appearance.pieceFile("roof-l-d3-ne-c0r0"),
              "Scene/ville/roofs/l/d3/roof-l-d3-ne-c0r0.png");
    EXPECT_EQ(read.appearance.pieceFile("prop-barrel"), "Scene/ville/prop-barrel.png");

    core::LevelData data{.name = "ville", .tileMap = core::TileMap{2, 1}};
    core::TileLayer ground{.name = "sol",
                           .kind = core::LayerKind::Ground,
                           .tiles = core::TileMap{2, 1},
                           .properties = {{"scene", std::string{"ville"}}}};
    ground.setPiece(0, 0, "floor-paving-01");
    ground.setPiece(1, 0, "floor-paving-01");
    core::TileLayer decor{
        .name = "decor", .kind = core::LayerKind::Decor, .tiles = core::TileMap{2, 1}};
    decor.setPiece(0, 0, "prop-barrel");
    core::TileLayer roof{.name = "toit",
                         .kind = core::LayerKind::Decor,
                         .tiles = core::TileMap{2, 1},
                         .properties = {},
                         .floor = 1};
    roof.setPiece(1, 0, "roof-l-d3-ne-c0r0");
    data.layers = {ground, decor, roof};

    const hmi::WorldSceneSnapshot snapshot =
        hmi::snapshotWorldScene(core::Level{std::move(data)}, read.appearance, {});
    const std::vector<std::string> paths = hmi::worldTexturePaths(snapshot);
    EXPECT_EQ(paths, (std::vector<std::string>{"Scene/ville/floors/floor-paving-01.png",
                                               "Scene/ville/prop-barrel.png",
                                               "Scene/ville/roofs/l/d3/roof-l-d3-ne-c0r0.png"}));
}

/**
 * @brief Les traits d'une image rangée en sous-dossier se lisent dans le manifeste du lieu, plus
 *        haut, qui la cite par son chemin : ancre, losange, hauteur d'étage.
 * \castest{<b>L'ancre d'une piece rangee se lit dans le manifeste du lieu.</b><br/>
 * \tcat Unitaire · Lieu compose · Arborescence<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Ecrire le manifeste d'un lieu, et un toit sous roofs/l/d3/.<br/>
 *          2. Lire les traits de l'image du toit.<br/>
 * \tattendu L'ancre (97, 40), le losange (256, 159) et la hauteur d'etage du lieu.
 * }
 */
TEST(SceneFoldersTest, LAncreDUnePieceRangeeSeLitDansLeManifesteDuLieu) {
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "jadg-lot129-arborescence";
    std::filesystem::remove_all(root);
    const std::filesystem::path place = root / "Scene" / "ville";
    std::filesystem::create_directories(place / "roofs" / "l" / "d3");
    nlohmann::json manifest = nlohmann::json::parse(MANIFEST);
    manifest["storey"] = 224;
    std::ofstream(place / "manifest.json") << manifest.dump();
    std::ofstream(place / "roofs" / "l" / "d3" / "roof-l-d3-ne-c0r0.png") << "png";

    const hmi::SceneTextureTraits traits =
        hmi::readSceneTextureTraits(root, "Scene/ville/roofs/l/d3/roof-l-d3-ne-c0r0.png");
    ASSERT_TRUE(traits.anchor.has_value());
    EXPECT_FLOAT_EQ(traits.anchor->x, 97.0F);
    EXPECT_FLOAT_EQ(traits.anchor->y, 40.0F);
    EXPECT_FLOAT_EQ(traits.artTile.x, 256.0F);
    ASSERT_TRUE(traits.storeyHeight.has_value());
    EXPECT_FLOAT_EQ(*traits.storeyHeight, 224.0F);
    std::filesystem::remove_all(root);
}
