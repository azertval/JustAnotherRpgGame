// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_world_scene_composer.cpp
 * @brief Tests de la composition d'un lieu parcouru (LOT-09) : le sol vient du type de tuile, le
 *        relief de la piece nommee a la case, et la figurine se pose au pied de sa case.
 */

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/RenderLayer.h"
#include "HMI/Graphics/ScenePiecePlacement.h"
#include "HMI/Graphics/ScenePieces.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

/**
 * @brief Le placement historique reste inchangé.
 * \castest{<b>Le placement historique reste inchangé.</b><br/>
 * \tcat Unitaire · Rendu du Colisée<br/>
 * \tcrit Critique<br/>
 * \tetapes Lire un manifeste sans placementVersion et composer une pièce.<br/>
 * \tattendu Aucune ancre explicite et coordonnées historiques conservées.
 * }
 */
TEST(ScenePiecePlacement, PreservesLegacyPlacementWithoutOptIn) {
    const nlohmann::json manifest = {
        {"textures", {{"wall", {{"file", "wall.png"}, {"anchor", {128, 162.5}}}}}}};
    EXPECT_FALSE(hmi::scenePieceAnchor(manifest, "wall.png"));
    const hmi::SceneTexture texture{.width = 256, .height = 256};
    const auto quad = hmi::standingPieceQuad(texture, {100, 200}, 2);
    EXPECT_FLOAT_EQ(quad.x, 32);
    EXPECT_FLOAT_EQ(quad.y, -228);
}

/**
 * @brief Une ancre fractionnaire aligne la pièce.
 * \castest{<b>Une ancre fractionnaire aligne la pièce.</b><br/>
 * \tcat Unitaire · Rendu du Colisée<br/>
 * \tcrit Critique<br/>
 * \tetapes Lire une origine fractionnaire puis composer la pièce.<br/>
 * \tattendu Ancrage exact et dimensions inchangées.
 * }
 */
TEST(ScenePiecePlacement, AlignsFractionalOriginWithoutChangingDimensions) {
    const nlohmann::json manifest = {
        {"placementVersion", 1},
        {"textures", {{"corner", {{"file", "corner.png"}, {"anchor", {128, 162.5}}}}}}};
    const auto anchor = hmi::scenePieceAnchor(manifest, "corner.png");
    ASSERT_TRUE(anchor);
    const hmi::SceneTexture texture{.width = 256, .height = 256, .anchor = anchor};
    const auto quad = hmi::standingPieceQuad(texture, {100, 200}, 2);
    EXPECT_FLOAT_EQ(quad.x + anchor->x * 2, 100);
    EXPECT_FLOAT_EQ(quad.y + anchor->y * 2, 200);
    EXPECT_FLOAT_EQ(quad.width, 512);
    EXPECT_FLOAT_EQ(quad.height, 512);
    EXPECT_FALSE(hmi::scenePieceAnchor(manifest, "unknown.png"));
}

/**
 * @brief Une ancre invalide est ignorée.
 * \castest{<b>Une ancre invalide est ignorée.</b><br/>
 * \tcat Unitaire · Rendu du Colisée<br/>
 * \tcrit Critique<br/>
 * \tetapes Lire une ancre contenant une chaîne à la place d’un nombre.<br/>
 * \tattendu Aucune ancre retenue.
 * }
 */
TEST(ScenePiecePlacement, RejectsMalformedAnchor) {
    const nlohmann::json manifest = {
        {"placementVersion", 1},
        {"textures", {{"wall", {{"file", "wall.png"}, {"anchor", {"128", 162}}}}}}};
    EXPECT_FALSE(hmi::scenePieceAnchor(manifest, "wall.png"));
}

/**
 * @brief La projection explicite est contrôlée.
 * \castest{<b>La projection explicite est contrôlée.</b><br/>
 * \tcat Unitaire · Rendu du Colisée<br/>
 * \tcrit Critique<br/>
 * \tetapes Lire les tables sans ratio, avec ratio valide et avec ratios invalides.<br/>
 * \tattendu Ancienne projection par défaut, rapport 42/68 accepté et valeurs invalides refusées.
 * }
 */
TEST(ScenePiecePlacement, ProjectionIsOptInAndRejectsInvalidRatios) {
    auto normal = hmi::PlaceAppearance::loadFromString(R"({"version":1,"place":"old"})");
    ASSERT_TRUE(normal.ok());
    EXPECT_FLOAT_EQ(normal.appearance.diamondRatio(), core::ARENA_DIAMOND_RATIO);
    auto modular = hmi::PlaceAppearance::loadFromString(
        R"({"version":1,"place":"new","diamondRatio":0.6176470588235294})");
    ASSERT_TRUE(modular.ok());
    EXPECT_FLOAT_EQ(modular.appearance.diamondRatio(), 42.0F / 68.0F);
    EXPECT_FALSE(
        hmi::PlaceAppearance::loadFromString(R"({"version":1,"place":"bad","diamondRatio":0})")
            .ok());
    EXPECT_FALSE(hmi::PlaceAppearance::loadFromString(
                     R"({"version":1,"place":"bad","diamondRatio":"wrong"})")
                     .ok());
}

// Trois pieces de sol pour le sable, une dalle pour la pierre, un mur pour le relief.
constexpr const char* TABLE_JSON = R"({
  "version": 1,
  "place": "coliseum",
  "floors": {
    "sand": ["sand", "sand-2", "sand-3"],
    "solid": ["stone-slab"]
  },
  "relief": {
    "wall": ["wall-left"]
  }
})";

[[nodiscard]] hmi::PlaceAppearance table() {
    hmi::PlaceAppearanceResult lue = hmi::PlaceAppearance::loadFromString(TABLE_JSON);
    EXPECT_TRUE(lue.ok()) << lue.message;
    return std::move(lue.appearance);
}

// Une carte 4 x 3 : sol de sable, une case de pierre, une couche de decor avec un mur.
[[nodiscard]] core::Level carte() {
    core::TileMap collision{4, 3};
    core::TileMap sol{4, 3};
    for (int ligne = 0; ligne < 3; ++ligne) {
        for (int colonne = 0; colonne < 4; ++colonne) {
            sol.setTile(colonne, ligne, core::TileType::Sand);
        }
    }
    sol.setTile(3, 0, core::TileType::Solid);
    core::TileMap decor{4, 3};
    decor.setTile(0, 0, core::TileType::Wall);

    core::LevelData donnees{.name = "colisee", .tileMap = std::move(collision)};
    donnees.layers.push_back(core::TileLayer{.name = "sol",
                                             .kind = core::LayerKind::Ground,
                                             .tiles = std::move(sol),
                                             .properties = {{"scene", std::string{"coliseum"}}}});
    donnees.layers.push_back(core::TileLayer{.name = "decor",
                                             .kind = core::LayerKind::Decor,
                                             .tiles = std::move(decor),
                                             .properties = {}});
    donnees.layers.back().setPiece(2, 2, "torch-left");
    return core::Level{std::move(donnees)};
}

/// Des textures factices : la composition ne demande rien au GPU, elle resout des chemins.
[[nodiscard]] hmi::ScenePieceTextures textures(const std::vector<std::string>& chemins) {
    hmi::ScenePieceTextures resolues;
    int rang = 1;
    for (const std::string& chemin : chemins) {
        resolues.byPath.emplace(chemin,
                                hmi::SceneTexture{.texture = reinterpret_cast<hmi::TextureHandle>(
                                                      static_cast<std::uintptr_t>(rang++)),
                                                  .width = 68,
                                                  .height = 100,
                                                  .frameWidth = 0});
    }
    return resolues;
}

}  // namespace

/**
 * @brief L'instantane tire le sol du type de tuile et le relief de la case.
 * \castest{<b>Le sol vient du type de tuile, le relief de la piece nommee a la case.</b><br/>
 * \tcat Unitaire · Lieu compose<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Batir une carte de sable avec une case de pierre, un mur de decor et une piece
 * nommee a une case du decor.<br/>
 * 2. En tirer l'instantane.<br/>
 * \tattendu Chaque case porte la piece de son type ; la case qui nomme une piece porte SA piece.
 * }
 */
TEST(WorldSceneComposerTest, LInstantaneTireLeSolDuTypeEtLeReliefDeLaCase) {
    const hmi::WorldSceneSnapshot instantane = hmi::snapshotWorldScene(carte(), table(), {});

    EXPECT_EQ(instantane.columns, 4);
    EXPECT_EQ(instantane.rows, 3);
    EXPECT_EQ(instantane.place, "coliseum");

    // Le sable a trois variantes, et la variante ne depend que de la case.
    EXPECT_EQ(instantane.floorAt({0, 0}), "sand");
    EXPECT_EQ(instantane.floorAt({1, 0}), "sand-2");
    EXPECT_EQ(instantane.floorAt({2, 0}), "sand-3");
    EXPECT_EQ(instantane.floorAt({3, 0}), "stone-slab");
    EXPECT_EQ(instantane.floorAt({0, 0}),
              hmi::snapshotWorldScene(carte(), table(), {}).floorAt({0, 0}));

    // Le relief : le mur de la couche de decor, et la piece nommee a la case.
    EXPECT_EQ(instantane.reliefAt({0, 0}), "wall-left");
    EXPECT_EQ(instantane.reliefAt({2, 2}), "torch-left");
    EXPECT_TRUE(instantane.reliefAt({1, 1}).empty());
    EXPECT_TRUE(instantane.floorAt({9, 9}).empty()) << "hors grille : rien, jamais un acces fautif";
}

/**
 * @brief Les chemins demandes sont ceux du lieu et des figurines, sans doublon.
 * \castest{<b>La liste des textures a charger couvre exactement ce que la composition
 * resout.</b><br/>
 * \tcat Unitaire · Lieu compose<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Tirer l'instantane d'une carte avec une figurine.<br/>
 * 2. Lister les chemins de texture.<br/>
 * \tattendu Les pieces du lieu sous `Scene/<lieu>/`, les deux bandes de la figurine sous
 * `Npc/<slug>/`, chaque chemin une seule fois.
 * }
 */
TEST(WorldSceneComposerTest, LesCheminsCouvrentLeLieuEtLesFigurines) {
    const hmi::WorldSceneSnapshot instantane = hmi::snapshotWorldScene(
        carte(), table(),
        {hmi::WorldFigureSnapshot{
            .figure = "anariel", .clip = "walk", .point = {1.5F, 1.5F}, .frame = 0}});

    const std::vector<std::string> chemins = hmi::worldTexturePaths(instantane);
    EXPECT_TRUE(std::ranges::is_sorted(chemins));
    EXPECT_NE(std::ranges::find(chemins, "Scene/coliseum/sand.png"), chemins.end());
    EXPECT_NE(std::ranges::find(chemins, "Scene/coliseum/stone-slab.png"), chemins.end());
    EXPECT_NE(std::ranges::find(chemins, "Scene/coliseum/wall-left.png"), chemins.end());
    EXPECT_NE(std::ranges::find(chemins, "Scene/coliseum/torch-left.png"), chemins.end());
    EXPECT_NE(std::ranges::find(chemins, "Npc/anariel/idle.png"), chemins.end());
    EXPECT_NE(std::ranges::find(chemins, "Npc/anariel/walk.png"), chemins.end());
    EXPECT_EQ(std::ranges::count(chemins, "Scene/coliseum/sand.png"), 1);
}

/**
 * @brief La composition pose le sol, le relief et la figurine sur leurs calques.
 * \castest{<b>Sol, relief et figurine tombent sur les calques Tile, Object et Player.</b><br/>
 * \tcat Unitaire · Lieu compose<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Composer une carte de douze cases avec un mur, une torche et une figurine.<br/>
 * \tattendu Douze quads de sol, deux de relief, un de figurine ; la figurine est sur le calque des
 * personnages, et son quad monte AU-DESSUS du pied de sa case.
 * }
 */
TEST(WorldSceneComposerTest, LaCompositionPoseChaquePieceSurSonCalque) {
    const hmi::WorldSceneSnapshot instantane = hmi::snapshotWorldScene(
        carte(), table(),
        {hmi::WorldFigureSnapshot{
            .figure = "anariel", .clip = "idle", .point = {1.5F, 1.5F}, .frame = 0}});
    const core::IsoProjection projection{instantane.columns, instantane.rows};
    const hmi::ComposedScene scene = hmi::composeWorldScene(
        instantane, projection, textures(hmi::worldTexturePaths(instantane)));

    int sols = 0;
    int reliefs = 0;
    int figurines = 0;
    for (const hmi::ComposedQuad& quad : scene.quads()) {
        switch (quad.layer) {
            case hmi::RenderLayer::Tile:
                ++sols;
                break;
            case hmi::RenderLayer::Object:
                ++reliefs;
                break;
            case hmi::RenderLayer::Player:
                ++figurines;
                break;
            default:
                break;
        }
    }
    EXPECT_EQ(sols, 12);
    EXPECT_EQ(reliefs, 2);
    ASSERT_EQ(figurines, 1);

    // La figurine se pose au pied de sa case : son bas est au-dessus du sommet bas du losange.
    const float piedDeLaCase = projection.gridToWorld(core::Vector2{2.0F, 2.0F}).y;
    for (const hmi::ComposedQuad& quad : scene.quads()) {
        if (quad.layer == hmi::RenderLayer::Player) {
            EXPECT_LT(quad.sprite.y + quad.sprite.height, piedDeLaCase);
        }
    }
}

/**
 * @brief Une figurine sans image a une cle de marqueur, une piece de planche n'en a pas.
 * \castest{<b>La cle du marqueur d'une figurine se tire de son chemin de bande.</b><br/>
 * \tcat Unitaire · Rendu du lieu<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander la cle de marqueur de chemins de figurine, de piece et de chemins
 * malformes.<br/>
 * \tattendu `npc/<figurine>` pour une bande de figurine, quelle que soit la bande ; rien pour
 * une piece de planche ni pour un chemin sans figurine (LOT-96).
 * }
 */
TEST(WorldSceneComposerTest, UneFigurineSansImageAUneCleDeMarqueur) {
    EXPECT_EQ(hmi::figureMarkerKey("Npc/sentinelle-ironhand/idle.png"), "npc/sentinelle-ironhand");
    EXPECT_EQ(hmi::figureMarkerKey("Npc/sentinelle-ironhand/walk.png"), "npc/sentinelle-ironhand");
    EXPECT_EQ(hmi::figureMarkerKey("Scene/martpart/street.png"), "");
    EXPECT_EQ(hmi::figureMarkerKey("Npc/"), "");
    EXPECT_EQ(hmi::figureMarkerKey("Npc//idle.png"), "");
    EXPECT_EQ(hmi::figureMarkerKey("Npc/jade"), "");
    EXPECT_EQ(hmi::figureMarkerKey("Monsters/ironhand-soldier/idle.png"),
              "monsters/ironhand-soldier");
}

/**
 * @brief Une figurine se nomme par son slug de PNJ, ou par son dossier depuis les assets
 * (`LOT-93`).
 * \castest{<b>Le soldat Ironhand se lit dans les monstres, Anariel dans les PNJ.</b><br/>
 * \tcat Unitaire · Scène du monde<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander la bande idle d'« anariel », puis celle de « Monsters/ironhand-soldier
 * ».<br/>
 * \tattendu `Npc/anariel/idle.png` et `Monsters/ironhand-soldier/idle.png` ; un clip vide vaut
 * idle.
 * }
 */
TEST(WorldSceneComposerTest, UneFigurineSeNommeParSlugOuParDossier) {
    EXPECT_EQ(hmi::figureStripPath("anariel", "idle"), "Npc/anariel/idle.png");
    EXPECT_EQ(hmi::figureStripPath("anariel", ""), "Npc/anariel/idle.png");
    EXPECT_EQ(hmi::figureStripPath("Monsters/ironhand-soldier", "walk"),
              "Monsters/ironhand-soldier/walk.png");
}

/**
 * @brief Une pièce nommée sur une case de **sol** l'emporte sur la table ; un ancien nom se montre
 *        sous le nom courant (format v4, `LOT-EDITOR-12`).
 * \castest{<b>La pièce nommée l'emporte, sous son nom courant.</b><br/>
 * \tcat Unitaire · Lieu compose<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Nommer `stone-slab` sur une case de sable, et `old-torch`, ancien nom de
 * `torch-left`, sur le décor.<br/>2. Tirer l'instantané avec le manifeste des alias.<br/>
 * \tattendu Le sol montre `stone-slab`, le relief `torch-left`.
 * }
 */
TEST(WorldSceneComposerTest, LaPieceNommeeLEmporteSousSonNomCourant) {
    core::LevelData donnees = carte().data();
    donnees.layers[0].setPiece(0, 1, "stone-slab");
    donnees.layers[1].setPiece(2, 2, "old-torch");
    hmi::PlaceAppearance appearance = table();
    const core::ScenePieceManifestResult manifeste = core::ScenePieceManifest::loadFromString(R"({
      "version": 1, "textures": {
        "scene/coliseum/torch-left": {"file": "torch-left.png", "class": "tall",
                                      "aliases": ["old-torch"]}}})");
    ASSERT_TRUE(manifeste.ok()) << manifeste.message;
    appearance.adoptManifest(manifeste.manifest);

    const hmi::WorldSceneSnapshot instantane =
        hmi::snapshotWorldScene(core::Level{std::move(donnees)}, appearance, {});

    EXPECT_EQ(instantane.floorAt({0, 1}), "stone-slab");
    EXPECT_EQ(instantane.reliefAt({2, 2}), "torch-left");
}

/**
 * @brief Une pièce **large** se trie au pied de son emprise, pas de sa case d'ancrage (constat A4).
 * \castest{<b>Une pièce large se trie au pied de son emprise.</b><br/>
 * \tcat Unitaire · Lieu compose<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser un étal 2 × 1 et un mur 1 × 1 sur la même case d'ancrage de deux cartes.<br/>
 * 2. Composer les deux.<br/>
 * \tattendu L'étal se trie après le mur : son pied est plus bas à l'écran.
 * }
 */
TEST(WorldSceneComposerTest, UnePieceLargeSeTrieAuPiedDeSonEmprise) {
    core::LevelData donnees = carte().data();
    donnees.layers[1].setPiece(0, 0, "stall");
    hmi::PlaceAppearance appearance = table();
    const core::ScenePieceManifestResult manifeste = core::ScenePieceManifest::loadFromString(R"({
      "version": 1, "textures": {
        "scene/coliseum/stall": {"file": "stall.png", "class": "wide", "footprint": [2, 1]}}})");
    ASSERT_TRUE(manifeste.ok()) << manifeste.message;
    appearance.adoptManifest(manifeste.manifest);

    const hmi::WorldSceneSnapshot large =
        hmi::snapshotWorldScene(core::Level{std::move(donnees)}, appearance, {});
    const hmi::WorldSceneSnapshot simple = hmi::snapshotWorldScene(carte(), appearance, {});
    ASSERT_EQ(large.footprints.at("stall"), (core::PieceFootprint{.columns = 2, .rows = 1}));

    const core::IsoProjection projection{4, 3};
    const auto ordreDuRelief = [&projection](const hmi::WorldSceneSnapshot& instantane) {
        const hmi::ComposedScene scene = hmi::composeWorldScene(
            instantane, projection, textures(hmi::worldTexturePaths(instantane)));
        // Le relief le plus en arriere est celui de la case (0, 0) : le mur, ou l'etal.
        std::int32_t plusEnArriere = std::numeric_limits<std::int32_t>::max();
        for (const hmi::ComposedQuad& quad : scene.quads()) {
            if (quad.layer == hmi::RenderLayer::Object) {
                plusEnArriere = std::min(plusEnArriere, quad.sortOrder);
            }
        }
        return plusEnArriere;
    };
    EXPECT_GT(ordreDuRelief(large), ordreDuRelief(simple));
}

/**
 * @brief La profondeur exige un manifeste de placement valide.
 * \castest{<b>La profondeur exige un manifeste de placement valide.</b><br/>
 * \tcat Unitaire · Rendu du Colisée<br/>
 * \tcrit Critique<br/>
 * \tetapes Lire une pièce valide, une inconnue et un manifeste historique.<br/>
 * \tattendu Profondeur présente uniquement pour la pièce valide.
 * }
 */
TEST(ScenePiecePlacement, DepthRequiresOptInAndValidAnchor) {
    const auto manifest = nlohmann::json::parse(R"({"placementVersion":1,"textures":{
        "bad":{"file":3},"gate":{"file":"gate.png","anchor":[12,96],"depthOffset":4.5}}})");
    EXPECT_EQ(hmi::scenePieceDepthOffset(manifest, "gate.png"), 4.5F);
    EXPECT_FALSE(hmi::scenePieceDepthOffset(manifest, "unknown.png"));
    auto legacy = manifest;
    legacy.erase("placementVersion");
    EXPECT_FALSE(hmi::scenePieceDepthOffset(legacy, "gate.png"));
}
