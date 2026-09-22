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
#include "HMI/Graphics/MaquettePalette.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/RenderLayer.h"
#include "HMI/Graphics/ScenePieces.h"
#include "HMI/Graphics/SceneTextureTraits.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

/// Rapport du losange de la projection par défaut.
constexpr float RATIO = core::ARENA_DIAMOND_RATIO;

/**
 * @brief Une pièce se met à l'échelle de **son lieu** : le losange que déclare son manifeste occupe
 *        celui de la case (`LOT-103`).
 * \castest{<b>Une piece prend l'echelle que son lieu declare.</b><br/>
 * \tcat Unitaire · Rendu HD<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Poser une piece de 512 x 400 d'un lieu qui declare un losange de 256 x 159, sur une
 * case de 100 unites.<br/>
 * \tattendu La piece mesure 200 x 156,25 unites ; son ancre par defaut est le milieu du losange du
 * bas.
 * }
 */
TEST(ScenePiecePlacement, APieceTakesTheScaleItsPlaceDeclares) {
    const hmi::SceneTexture texture{.width = 512, .height = 400, .artTile = {256.0F, 159.0F}};
    const auto quad = hmi::standingPieceQuad(texture, {1000, 500}, 100.0F, RATIO);
    EXPECT_FLOAT_EQ(quad.width, 200.0F);
    EXPECT_FLOAT_EQ(quad.height, 156.25F);
    // Ancre par défaut : (128, 400 - 159) pixels, soit (50, 94,140625) unités.
    EXPECT_FLOAT_EQ(quad.x, 1000.0F - 50.0F);
    EXPECT_FLOAT_EQ(quad.y, 500.0F - (241.0F * 100.0F / 256.0F));
}

/**
 * @brief Le même lieu livré deux fois plus fin se dessine à la même taille : l'échelle est une
 *        donnée, pas une constante.
 * \castest{<b>Un lieu plus fin se dessine a la meme taille.</b><br/>
 * \tcat Unitaire · Rendu HD<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser la meme piece a 256 px de losange, puis a 512 px, deux fois plus grande.<br/>
 * \tattendu Les deux quads sont identiques.
 * }
 */
TEST(ScenePiecePlacement, AFinerPlaceDrawsAtTheSameSize) {
    const hmi::SceneTexture coarse{.width = 256, .height = 300, .artTile = {256.0F, 159.0F}};
    const hmi::SceneTexture fine{.width = 512, .height = 600, .artTile = {512.0F, 318.0F}};
    const auto a = hmi::standingPieceQuad(coarse, {10, 20}, 5.375F, RATIO);
    const auto b = hmi::standingPieceQuad(fine, {10, 20}, 5.375F, RATIO);
    EXPECT_FLOAT_EQ(a.x, b.x);
    EXPECT_FLOAT_EQ(a.y, b.y);
    EXPECT_FLOAT_EQ(a.width, b.width);
    EXPECT_FLOAT_EQ(a.height, b.height);
}

/**
 * @brief Sans échelle déclarée, une pièce se suppose d'une case de large.
 * \castest{<b>Une piece sans echelle declaree a la largeur d'une case.</b><br/>
 * \tcat Unitaire · Rendu HD<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser une piece de 256 x 256 sans losange declare, sur une case de 512 unites.<br/>
 * \tattendu Elle occupe exactement la largeur de la case.
 * }
 */
TEST(ScenePiecePlacement, WithoutADeclaredScaleAPieceIsOneCellWide) {
    const hmi::SceneTexture texture{.width = 256, .height = 256};
    const auto quad = hmi::standingPieceQuad(texture, {100, 200}, 512.0F, RATIO);
    EXPECT_FLOAT_EQ(quad.width, 512.0F);
    EXPECT_FLOAT_EQ(quad.x, 100.0F - 256.0F);
}

/**
 * @brief Une ancre fractionnaire aligne la pièce, et se lit **sans** opt-in depuis le `LOT-103`.
 * \castest{<b>Une ancre fractionnaire aligne la pièce.</b><br/>
 * \tcat Unitaire · Rendu du Colisée<br/>
 * \tcrit Critique<br/>
 * \tetapes Lire une origine fractionnaire d'un manifeste sans placementVersion puis composer la
 * pièce.<br/>
 * \tattendu Ancrage exact et dimensions inchangées.
 * }
 */
TEST(ScenePiecePlacement, AlignsFractionalOriginWithoutChangingDimensions) {
    const nlohmann::json manifest = {
        {"textures", {{"corner", {{"file", "corner.png"}, {"anchor", {128, 162.5}}}}}}};
    const auto anchor = hmi::scenePieceAnchor(manifest, "corner.png");
    ASSERT_TRUE(anchor);
    const hmi::SceneTexture texture{
        .width = 256, .height = 256, .anchor = anchor, .artTile = {256.0F, 159.0F}};
    const auto quad = hmi::standingPieceQuad(texture, {100, 200}, 512.0F, RATIO);
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
        {"textures", {{"wall", {{"file", "wall.png"}, {"anchor", {"128", 162}}}}}}};
    EXPECT_FALSE(hmi::scenePieceAnchor(manifest, "wall.png"));
}

/**
 * @brief Le losange d'art se lit dans le manifeste, et un losange mal formé ne dit rien.
 * \castest{<b>Le losange d'art se lit dans le manifeste.</b><br/>
 * \tcat Unitaire · Rendu HD<br/>
 * \tcrit Majeur<br/>
 * \tetapes Lire `tile` d'un manifeste valide, absent, nul et non numerique.<br/>
 * \tattendu (256, 159) pour le premier, (0, 0) pour les trois autres.
 * }
 */
TEST(ScenePiecePlacement, ReadsTheArtTileOfAManifest) {
    const core::Vector2 tile = hmi::manifestArtTile(nlohmann::json::parse(R"({"tile":[256,159]})"));
    EXPECT_FLOAT_EQ(tile.x, 256.0F);
    EXPECT_FLOAT_EQ(tile.y, 159.0F);
    for (const char* text : {R"({})", R"({"tile":[0,159]})", R"({"tile":["a",1]})"}) {
        EXPECT_FLOAT_EQ(hmi::manifestArtTile(nlohmann::json::parse(text)).x, 0.0F) << text;
    }
}

/**
 * @brief Une figurine de 192 × 256 s'affiche **entière**, à l'échelle de son art (`LOT-103`).
 * \castest{<b>Une figurine de 192 x 256 s'affiche entiere.</b><br/>
 * \tcat Unitaire · Rendu HD<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Poser l'image 2 d'une bande de six cellules de 192 x 256, a l'echelle d'un losange de
 * 256, sur une case de 100 unites.<br/>
 * \tattendu Le quad lit toute la hauteur de la cellule (v de 0 a 1), mesure 75 x 100 unites et
 * lit la troisieme cellule.
 * }
 */
TEST(ScenePiecePlacement, AFigureOf192By256IsDrawnWhole) {
    const hmi::SceneTexture band{.width = 192 * 6,
                                 .height = 256,
                                 .frameWidth = 192,
                                 .frameHeight = 256,
                                 .artTile = {256.0F, 159.0F}};
    ASSERT_EQ(hmi::frameCountOf(band), 6);
    const auto quad = hmi::figureQuad(band, 2, 500.0F, 400.0F, 100.0F);
    EXPECT_FLOAT_EQ(quad.v0, 0.0F);
    EXPECT_FLOAT_EQ(quad.v1, 1.0F);
    EXPECT_FLOAT_EQ(quad.width, 75.0F);
    EXPECT_FLOAT_EQ(quad.height, 100.0F);
    EXPECT_FLOAT_EQ(quad.u0, 2.0F / 6.0F);
    EXPECT_FLOAT_EQ(quad.u1, 3.0F / 6.0F);
    EXPECT_FLOAT_EQ(quad.y + quad.height, 400.0F);
    EXPECT_FLOAT_EQ(quad.x + (quad.width / 2.0F), 500.0F);
}

/**
 * @brief Une grande créature de 384 × 384 s'affiche entière, une fois et demie une case.
 * \castest{<b>Une creature de 384 x 384 s'affiche entiere.</b><br/>
 * \tcat Unitaire · Rendu HD<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Poser une bande de quatre cellules de 384 x 384 a l'echelle d'un losange de 256.<br/>
 * \tattendu Le quad lit toute la cellule et mesure 1,5 case de cote.
 * }
 */
TEST(ScenePiecePlacement, ACreatureOf384By384IsDrawnWhole) {
    const hmi::SceneTexture band{.width = 384 * 4,
                                 .height = 384,
                                 .frameWidth = 384,
                                 .frameHeight = 384,
                                 .artTile = {256.0F, 159.0F}};
    const auto quad = hmi::figureQuad(band, 3, 0.0F, 0.0F, 100.0F);
    EXPECT_FLOAT_EQ(quad.v1, 1.0F);
    EXPECT_FLOAT_EQ(quad.u1, 1.0F);
    EXPECT_FLOAT_EQ(quad.width, 150.0F);
    EXPECT_FLOAT_EQ(quad.height, 150.0F);
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
 * @brief La profondeur exige une ancre valide.
 * \castest{<b>La profondeur exige une ancre valide.</b><br/>
 * \tcat Unitaire · Rendu du Colisée<br/>
 * \tcrit Critique<br/>
 * \tetapes Lire une pièce valide, une inconnue et une pièce sans ancre.<br/>
 * \tattendu Profondeur présente uniquement pour la pièce valide.
 * }
 */
TEST(ScenePiecePlacement, DepthRequiresAValidAnchor) {
    const auto manifest = nlohmann::json::parse(R"({"textures":{
        "bad":{"file":3},"gate":{"file":"gate.png","anchor":[12,96],"depthOffset":4.5},
        "loose":{"file":"loose.png","depthOffset":2}}})");
    EXPECT_EQ(hmi::scenePieceDepthOffset(manifest, "gate.png"), 4.5F);
    EXPECT_FALSE(hmi::scenePieceDepthOffset(manifest, "unknown.png"));
    EXPECT_FALSE(hmi::scenePieceDepthOffset(manifest, "loose.png"));
}

// --- Le rendu de maquette (LOT-128) ---------------------------------------------------------

namespace {

/// L'aplat blanc, tel que les deux rendus le fournissent a la composition.
hmi::TextureHandle aplat() {
    static int pixel = 0;
    return &pixel;
}

/// Une carte 2 x 1 **sans lieu** : de l'eau, puis un mur. Aucune couche ne nomme de `scene`.
[[nodiscard]] core::Level carteNue() {
    core::TileMap collision{2, 1};
    collision.setTile(0, 0, core::TileType::Water);
    collision.setTile(1, 0, core::TileType::Wall);
    return core::Level{core::LevelData{.name = "maquette", .tileMap = std::move(collision)}};
}

/// La projection des cartes d'essai, au rapport du losange de l'atelier.
[[nodiscard]] core::IsoProjection projectionDe(const hmi::WorldSceneSnapshot& instantane) {
    return core::IsoProjection{instantane.columns, instantane.rows, core::ARENA_TILE_WIDTH_UNITS,
                               instantane.diamondRatio};
}

}  // namespace

/**
 * @brief Une carte sans lieu se compose en losanges de couleur, un par case, a la teinte de son
 *        type : c'est le rendu de maquette.
 * \castest{<b>Une carte sans lieu se compose en losanges de couleur.</b><br/>
 * \tcat Unitaire · Rendu de maquette<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Composer une carte de deux cases qui ne nomme aucun lieu.<br/>
 * \tattendu Deux primitives Poly sur le calque des tuiles, aux teintes de l'eau et du mur.
 * }
 */
TEST(MaquetteRenderTest, UneCarteSansLieuSeComposeEnLosangesDeCouleur) {
    const hmi::WorldSceneSnapshot instantane =
        hmi::snapshotWorldScene(carteNue(), hmi::PlaceAppearance{}, {});
    ASSERT_EQ(instantane.typeAt({0, 0}), core::TileType::Water);
    ASSERT_EQ(instantane.typeAt({1, 0}), core::TileType::Wall);

    hmi::ScenePieceTextures resolues;
    resolues.solid = hmi::SceneTexture{.texture = aplat(), .width = 1, .height = 1};
    const hmi::ComposedScene scene =
        hmi::composeWorldScene(instantane, projectionDe(instantane), resolues);

    // L'eau est un losange plat sur le calque des tuiles ; le mur, un bloc de trois faces sur le
    // calque du decor (LOT-128, decision D6).
    ASSERT_EQ(scene.size(), 4U);
    for (const hmi::ComposedQuad& quad : scene.quads()) {
        EXPECT_EQ(quad.kind, hmi::QuadKind::Poly);
        EXPECT_EQ(quad.texture, aplat());
    }
    EXPECT_EQ(scene.quads()[0].layer, hmi::RenderLayer::Tile);
    const hmi::MaquetteColor eau = hmi::maquetteColor(core::TileType::Water);
    EXPECT_FLOAT_EQ(scene.quads()[0].poly.r, eau.r);
    EXPECT_FLOAT_EQ(scene.quads()[0].poly.b, eau.b);
    for (std::size_t i = 1; i < scene.size(); ++i) {
        EXPECT_EQ(scene.quads()[i].layer, hmi::RenderLayer::Object);
    }
}

/**
 * @brief Sur une carte **avec** lieu, un type que la table ne couvre pas n'est plus invisible : il
 *        prend le losange de maquette, les cases couvertes gardant leur piece.
 * \castest{<b>Un type absent de la table du lieu prend le rendu de maquette.</b><br/>
 * \tcat Unitaire · Rendu de maquette<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Peindre une case d'eau sur la carte du Colisee, dont la table ne couvre que le
 * sable et la pierre.<br/>2. Composer.<br/>
 * \tattendu La case d'eau est un losange de couleur ; les autres restent des pieces texturees.
 * }
 */
TEST(MaquetteRenderTest, UnTypeNonCouvertParLeLieuPrendLaMaquette) {
    core::TileMap collision{2, 1};
    core::TileMap sol{2, 1};
    sol.setTile(0, 0, core::TileType::Sand);
    sol.setTile(1, 0, core::TileType::Water);  // la table du Colisee ne couvre pas l'eau
    core::LevelData donnees{.name = "colisee", .tileMap = std::move(collision)};
    donnees.layers.push_back(core::TileLayer{.name = "sol",
                                             .kind = core::LayerKind::Ground,
                                             .tiles = std::move(sol),
                                             .properties = {{"scene", std::string{"coliseum"}}}});

    const hmi::WorldSceneSnapshot instantane =
        hmi::snapshotWorldScene(core::Level{std::move(donnees)}, table(), {});
    EXPECT_FALSE(instantane.floorAt({0, 0}).empty());
    EXPECT_TRUE(instantane.floorAt({1, 0}).empty());

    hmi::ScenePieceTextures resolues = textures({"Scene/coliseum/sand.png"});
    resolues.solid = hmi::SceneTexture{.texture = aplat(), .width = 1, .height = 1};
    const hmi::ComposedScene scene =
        hmi::composeWorldScene(instantane, projectionDe(instantane), resolues);

    ASSERT_EQ(scene.size(), 2U);
    int sprites = 0;
    int losanges = 0;
    for (const hmi::ComposedQuad& quad : scene.quads()) {
        quad.kind == hmi::QuadKind::Poly ? ++losanges : ++sprites;
    }
    EXPECT_EQ(sprites, 1);
    EXPECT_EQ(losanges, 1);
}

/**
 * @brief Sans aplat, rien n'est dessine plutot que quelque chose de faux : la maquette est une
 *        primitive de couleur, et une couleur sans texture liee ne se soumet pas.
 * \castest{<b>Sans aplat, la maquette ne compose rien.</b><br/>
 * \tcat Unitaire · Rendu de maquette<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Composer une carte sans lieu avec une table de textures sans aplat.<br/>
 * \tattendu Aucune primitive.
 * }
 */
TEST(MaquetteRenderTest, SansAplatRienNEstCompose) {
    const hmi::WorldSceneSnapshot instantane =
        hmi::snapshotWorldScene(carteNue(), hmi::PlaceAppearance{}, {});
    const hmi::ComposedScene scene =
        hmi::composeWorldScene(instantane, projectionDe(instantane), hmi::ScenePieceTextures{});

    EXPECT_EQ(scene.size(), 0U);
}

/**
 * @brief Un type qui bloque se compose en bloc extrude : trois faces, d'eclairements distincts,
 *        montant d'une case au-dessus du losange, sur le calque du decor.
 * \castest{<b>Un mur se compose en bloc de trois faces, haut d'une case.</b><br/>
 * \tcat Unitaire · Rendu de maquette<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Composer une carte d'une seule case de mur, sans lieu.<br/>
 * \tattendu Trois primitives sur le calque du decor ; le dessus monte d'une hauteur de losange
 * au-dessus du sommet de la case, et les trois faces n'ont pas la meme teinte.
 * }
 */
TEST(MaquetteRenderTest, UnMurSeComposeEnBlocDeTroisFaces) {
    core::TileMap collision{1, 1};
    collision.setTile(0, 0, core::TileType::Wall);
    const core::Level carte{core::LevelData{.name = "mur", .tileMap = std::move(collision)}};

    const hmi::WorldSceneSnapshot instantane =
        hmi::snapshotWorldScene(carte, hmi::PlaceAppearance{}, {});
    const core::IsoProjection projection = projectionDe(instantane);
    hmi::ScenePieceTextures resolues;
    resolues.solid = hmi::SceneTexture{.texture = aplat(), .width = 1, .height = 1};
    const hmi::ComposedScene scene = hmi::composeWorldScene(instantane, projection, resolues);

    ASSERT_EQ(scene.size(), 3U);
    for (const hmi::ComposedQuad& quad : scene.quads()) {
        EXPECT_EQ(quad.layer, hmi::RenderLayer::Object);
        EXPECT_EQ(quad.kind, hmi::QuadKind::Poly);
    }
    // Trois eclairements distincts : sans cet ecart, le bloc redevient une tache plate.
    const float premiere = scene.quads()[0].poly.r;
    const float deuxieme = scene.quads()[1].poly.r;
    const float troisieme = scene.quads()[2].poly.r;
    EXPECT_NE(premiere, deuxieme);
    EXPECT_NE(deuxieme, troisieme);

    // Le point le plus haut du bloc est une hauteur de losange au-dessus du sommet de la case.
    const core::Rect bounds = projection.tileBounds({.column = 0, .row = 0});
    float plusHaut = bounds.position.y;
    for (const hmi::ComposedQuad& quad : scene.quads()) {
        for (const float y : quad.poly.y) {
            plusHaut = std::min(plusHaut, y);
        }
    }
    EXPECT_FLOAT_EQ(plusHaut, bounds.position.y - bounds.size.y);
}

/**
 * @brief L'eau profonde bloque le pas mais n'est pas de la matiere : elle reste un losange plat,
 *        plus sombre que l'eau vive, et l'on voit par-dessus.
 * \castest{<b>L'eau profonde reste un losange plat, plus sombre que l'eau vive.</b><br/>
 * \tcat Unitaire · Rendu de maquette<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Interroger l'extrusion et la palette pour l'eau profonde.<br/>
 * \tattendu Elle ne s'extrude pas, et sa teinte est plus sombre que celle de l'eau.
 * }
 */
TEST(MaquetteRenderTest, LEauProfondeNeSExtrudePas) {
    EXPECT_FALSE(hmi::maquetteExtrudes(core::TileType::DeepWater));
    EXPECT_TRUE(hmi::maquetteExtrudes(core::TileType::Wall));
    EXPECT_TRUE(hmi::maquetteExtrudes(core::TileType::Solid));
    EXPECT_TRUE(hmi::maquetteExtrudes(core::TileType::Cliff));

    const hmi::MaquetteColor vive = hmi::maquetteColor(core::TileType::Water);
    const hmi::MaquetteColor profonde = hmi::maquetteColor(core::TileType::DeepWater);
    EXPECT_LT(profonde.r + profonde.g + profonde.b, vive.r + vive.g + vive.b);
}

/**
 * @brief La couleur d'un jeton se déduit de ce que le format dit déjà, sans propriété nouvelle
 *        (décision D3) : le dialogue fait le jaune, la rencontre le rouge, le camp d'une entrée
 *        d'arène l'un ou l'autre.
 * \castest{<b>La couleur d'un jeton se deduit de ce que le format dit deja.</b><br/>
 * \tcat Unitaire · Jetons de maquette<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser un PNJ avec dialogue, un sans, un avec figurine, une rencontre, deux entrees
 * d'arene, un point d'apparition, un portail et un coffre.<br/>2. En tirer les marques.<br/>
 * \tattendu Chaque jeton porte la nature attendue ; le PNJ qui a deja sa figurine n'a pas de
 * jeton.
 * }
 */
TEST(MaquetteRenderTest, LaCouleurDuJetonSeDeduitDeLEntite) {
    const auto entite = [](std::string type, core::PropertyMap properties) {
        return core::MapEntity{.type = std::move(type),
                               .position = {.column = 0, .row = 0},
                               .properties = std::move(properties)};
    };
    const std::vector<core::MapEntity> entites = {
        entite("npc", {{"dialogue", std::string{"market-mother"}}}),
        entite("npc", {}),
        entite("npc", {{"figure", std::string{"anariel"}}}),
        entite("encounter", {{"encounterId", std::string{"wolves"}}}),
        entite("arenaEntry", {{"side", std::string{"enemies"}}}),
        entite("arenaEntry", {{"side", std::string{"allies"}}}),
        entite("spawnPoint", {{"name", std::string{"gate"}}}),
        entite("portal", {{"targetMap", std::string{"arenarea"}}}),
        entite("chest", {}),
    };

    const hmi::MaquetteMarks marques = hmi::maquetteMarks(entites, /*maquette=*/true);

    // Huit jetons : le PNJ qui porte deja sa figurine se dessine par elle, pas par un jeton.
    ASSERT_EQ(marques.tokens.size(), 8U);
    EXPECT_EQ(marques.tokens[0].kind, hmi::MaquetteTokenKind::Talker);
    EXPECT_EQ(marques.tokens[0].letter, 'M');
    EXPECT_EQ(marques.tokens[1].kind, hmi::MaquetteTokenKind::Neutral);
    EXPECT_EQ(marques.tokens[1].letter, 'N');  // a defaut de nom, son type
    EXPECT_EQ(marques.tokens[2].kind, hmi::MaquetteTokenKind::Hostile);
    EXPECT_EQ(marques.tokens[2].letter, 'W');
    EXPECT_EQ(marques.tokens[3].kind, hmi::MaquetteTokenKind::Hostile);
    EXPECT_EQ(marques.tokens[4].kind, hmi::MaquetteTokenKind::Player);
    EXPECT_EQ(marques.tokens[5].kind, hmi::MaquetteTokenKind::Player);
    EXPECT_EQ(marques.tokens[5].letter, 'G');
    EXPECT_EQ(marques.tokens[6].kind, hmi::MaquetteTokenKind::Portal);
    EXPECT_EQ(marques.tokens[6].letter, 'A');
    EXPECT_TRUE(marques.tokens[6].arrow);
    EXPECT_EQ(marques.tokens[7].kind, hmi::MaquetteTokenKind::Object);
}

/**
 * @brief Les jetons se posent toujours ; les tracés et la flèche du portail ne paraissent qu'en
 *        maquette — une carte finie ne montre pas ses déclencheurs.
 * \castest{<b>Une carte habillee garde ses jetons mais perd ses traces.</b><br/>
 * \tcat Unitaire · Jetons de maquette<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Tirer les marques d'un portail, d'une zone de combat et d'un trajet, en maquette
 * puis hors maquette.<br/>
 * \tattendu En maquette : un jeton a fleche et deux traces. Hors maquette : le jeton sans sa
 * fleche, et aucune trace.
 * }
 */
TEST(MaquetteRenderTest, LesTracesNeParaissentQuEnMaquette) {
    std::vector<core::MapEntity> entites = {
        core::MapEntity{.type = "portal",
                        .position = {.column = 1, .row = 1},
                        .properties = {{"targetMap", std::string{"arenarea"}}}},
        core::MapEntity{.type = "combatZone",
                        .position = {.column = 2, .row = 2},
                        .properties = {{"name", std::string{"duel"}},
                                       {"width", std::int64_t{3}},
                                       {"height", std::int64_t{2}}}},
        core::MapEntity{.type = "route",
                        .position = {.column = 0, .row = 0},
                        .properties = {{"name", std::string{"ronde"}}}},
    };
    entites.back().cells = {{.column = 0, .row = 0}, {.column = 0, .row = 3}};

    const hmi::MaquetteMarks maquette = hmi::maquetteMarks(entites, /*maquette=*/true);
    EXPECT_EQ(maquette.tokens.size(), 1U);
    EXPECT_TRUE(maquette.tokens.front().arrow);
    ASSERT_EQ(maquette.traces.size(), 2U);
    // La zone de combat couvre bien ses 3 x 2 cases.
    EXPECT_EQ(maquette.traces[0].shape, hmi::MaquetteTraceShape::Outline);
    EXPECT_EQ(maquette.traces[0].cells.size(), 6U);
    EXPECT_EQ(maquette.traces[1].shape, hmi::MaquetteTraceShape::Path);

    const hmi::MaquetteMarks habillee = hmi::maquetteMarks(entites, /*maquette=*/false);
    EXPECT_EQ(habillee.tokens.size(), 1U);
    EXPECT_FALSE(habillee.tokens.front().arrow);
    EXPECT_TRUE(habillee.traces.empty());
}

/**
 * @brief Un jeton se demande par un chemin, comme une planche : le rendu n'a rien de neuf à
 *        apprendre, il voit un chemin de plus.
 * \castest{<b>Les chemins de textures d'une carte contiennent ceux de ses jetons.</b><br/>
 * \tcat Unitaire · Jetons de maquette<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Batir une carte sans lieu portant une rencontre.<br/>2. Lister ses chemins de
 * texture.<br/>
 * \tattendu Le chemin du jeton rouge « W » y figure.
 * }
 */
TEST(MaquetteRenderTest, LesCheminsContiennentLesJetons) {
    core::TileMap collision{2, 1};
    collision.setTile(0, 0, core::TileType::Grass);
    collision.setTile(1, 0, core::TileType::Grass);
    core::LevelData donnees{.name = "maquette", .tileMap = std::move(collision)};
    donnees.entities.push_back(
        core::MapEntity{.type = "encounter",
                        .position = {.column = 1, .row = 0},
                        .properties = {{"encounterId", std::string{"wolves"}}}});

    const hmi::WorldSceneSnapshot instantane =
        hmi::snapshotWorldScene(core::Level{std::move(donnees)}, hmi::PlaceAppearance{}, {});
    const std::vector<std::string> chemins = hmi::worldTexturePaths(instantane);

    EXPECT_NE(
        std::ranges::find(chemins, hmi::maquetteTokenPath(hmi::MaquetteTokenKind::Hostile, 'W')),
        chemins.end());
}
