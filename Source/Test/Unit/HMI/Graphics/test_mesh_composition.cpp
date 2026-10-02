// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_mesh_composition.cpp
 * @brief Les pièces en maillage d'un lieu, de son manifeste à la scène composée, **sans GPU**
 *        (`LOT-1003`).
 *
 * La carte d'essai (`Fixtures/Meshes`) : une cour dallée de maillages au milieu d'un sol en
 * images, un îlot de huit murs en anneau, un toit de trois cases de côté à l'étage. Les maillages
 * sont des identités sans GPU, à la boîte que leur fichier déclare : la composition est une
 * fonction pure, et produit une liste de maillages placés à côté de la liste de primitives.
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Resources/MeshFile.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/IsoView.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/StaticWorldScene.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

constexpr float TOLERANCE = 1e-3F;
constexpr const char* FLOOR = "Scene/ilot/floor.glb";
constexpr const char* WALL = "Scene/ilot/wall.glb";
constexpr const char* ROOF = "Scene/ilot/roof.glb";
constexpr const char* PAVING = "Scene/ilot/paving.png";

std::filesystem::path assets() {
    return std::filesystem::path(JADG_MESH_FIXTURE_DIR) / "Assets";
}

/// La carte d'essai prête à composer : son instantané, et ses pièces sans GPU.
struct Ilot {
    hmi::WorldSceneSnapshot snapshot;
    hmi::ScenePieceTextures textures;
    /// Les identités des textures et des maillages factices : leur adresse est la poignée.
    std::array<std::uint8_t, 8> identities{};

    [[nodiscard]] core::IsoProjection projection() const {
        return {snapshot.columns, snapshot.rows, core::ARENA_TILE_WIDTH_UNITS,
                snapshot.diamondRatio};
    }
};

/// @param meshes Faux pour une carte dont les maillages ne se sont pas chargés.
Ilot ilot(bool meshes = true) {
    Ilot place;
    const core::LevelLoadResult map = core::LevelLoader::loadFromFile(
        std::filesystem::path(JADG_MESH_FIXTURE_DIR) / "Levels" / "ilot.json");
    const hmi::PlaceAppearanceResult appearance =
        hmi::PlaceAppearance::loadForPlace(assets(), "ilot");
    EXPECT_TRUE(map.ok()) << map.error;
    EXPECT_TRUE(appearance.ok()) << appearance.message;
    if (!map.ok() || !appearance.ok()) {
        return place;
    }
    place.snapshot = hmi::snapshotWorldScene(*map.level, appearance.appearance, {});
    place.textures.byPath[PAVING] =
        hmi::SceneTexture{.texture = &place.identities[0], .width = 64, .height = 40};
    place.textures.missing =
        hmi::SceneTexture{.texture = &place.identities[1], .width = 64, .height = 64};
    place.textures.solid =
        hmi::SceneTexture{.texture = &place.identities[2], .width = 1, .height = 1};
    if (meshes) {
        std::size_t next = 3;
        for (const char* const path : {FLOOR, WALL, ROOF}) {
            const core::MeshFileResult read = core::readMeshFile(assets() / path);
            EXPECT_TRUE(read.ok()) << read.message;
            place.textures.meshes[path] = hmi::SceneMesh{.mesh = &place.identities[next++],
                                                         .minimum = read.mesh.minimum,
                                                         .maximum = read.mesh.maximum,
                                                         .storeyTiles = 0.875F};
        }
    }
    return place;
}

std::size_t countMeshes(const hmi::ComposedScene& scene, const hmi::ScenePieceTextures& textures,
                        const char* path) {
    const hmi::MeshHandle handle = textures.meshes.at(path).mesh;
    return static_cast<std::size_t>(std::ranges::count_if(
        scene.meshes(), [handle](const hmi::ComposedMesh& mesh) { return mesh.mesh == handle; }));
}

}  // namespace

/**
 * @brief Une clé de pièce cite un maillage ou une image, et le lieu lit les deux.
 * \castest{<b>Le manifeste d'un lieu cite un maillage ou une image.</b><br/>
 * \tcat Unitaire · Maillages d'un lieu<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire un manifeste dont une clé cite `"mesh"`, une `"file"`, une les deux, une
 *             aucun.<br/>
 *          2. Lire le lieu d'essai par `PlaceAppearance::loadForPlace`.<br/>
 * \tattendu La pièce en maillage dit `isMesh()` et son chemin finit en `.glb` ; la pièce en image
 *           garde le sien ; qui cite les deux est un maillage ; qui ne cite rien est ignorée. Le
 *           lieu rend le fichier `.glb` de `wall` et le `.png` de `paving`, et l'emprise 3 × 3 du
 *           toit.
 * }
 */
TEST(MeshCompositionTest, LeManifesteCiteUnMaillageOuUneImage) {
    const core::ScenePieceManifestResult read = core::ScenePieceManifest::loadFromString(R"({
        "version": 1, "tile": [256, 159],
        "textures": {
          "scene/essai/mur": {"mesh": "walls/mur.glb", "class": "tall", "footprint": [2, 1]},
          "scene/essai/banc": {"file": "banc.png", "class": "tall"},
          "scene/essai/les-deux": {"mesh": "a.glb", "file": "a.png"},
          "scene/essai/rien": {"class": "tall"},
          "scene/essai/vide": {"mesh": ""}
        }})");
    ASSERT_TRUE(read.ok()) << read.message;
    ASSERT_EQ(read.manifest.pieces().size(), 3U);
    const core::ScenePiece* const mur = read.manifest.find("mur");
    ASSERT_NE(mur, nullptr);
    EXPECT_TRUE(mur->isMesh());
    EXPECT_EQ(mur->path(), "walls/mur.glb");
    EXPECT_TRUE(core::isMeshPath(mur->path()));
    EXPECT_EQ(mur->footprintColumns, 2);
    EXPECT_EQ(mur->tactical, core::PieceTactical::Solid);
    const core::ScenePiece* const banc = read.manifest.find("banc");
    ASSERT_NE(banc, nullptr);
    EXPECT_FALSE(banc->isMesh());
    EXPECT_EQ(banc->path(), "banc.png");
    ASSERT_NE(read.manifest.find("les-deux"), nullptr);
    EXPECT_EQ(read.manifest.find("les-deux")->path(), "a.glb");
    EXPECT_EQ(read.manifest.find("rien"), nullptr);
    EXPECT_EQ(read.manifest.find("vide"), nullptr);

    const hmi::PlaceAppearanceResult place = hmi::PlaceAppearance::loadForPlace(assets(), "ilot");
    ASSERT_TRUE(place.ok()) << place.message;
    EXPECT_EQ(place.appearance.pieceFile("wall"), WALL);
    EXPECT_EQ(place.appearance.pieceFile("paving"), PAVING);
    EXPECT_EQ(place.appearance.pieceFootprint("roof"),
              (core::PieceFootprint{.columns = 3, .rows = 3}));
}

/**
 * @brief La carte demande ses maillages à part de ses textures.
 * \castest{<b>Une carte demande ses maillages a part de ses textures.</b><br/>
 * \tcat Unitaire · Maillages d'un lieu<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Tirer l'instantané de la carte d'essai.<br/>
 *          2. Lister ses chemins de texture, puis ses chemins de maillage.<br/>
 * \tattendu Trois maillages — le sol, le mur, le toit ; une seule texture de pièce, la dalle en
 *           image, et aucun `.glb` parmi les textures.
 * }
 */
TEST(MeshCompositionTest, UneCarteDemandeSesMaillagesAPart) {
    const Ilot place = ilot();
    EXPECT_EQ(hmi::worldMeshPaths(place.snapshot), (std::vector<std::string>{FLOOR, ROOF, WALL}));
    const std::vector<std::string> textures = hmi::worldTexturePaths(place.snapshot);
    EXPECT_NE(std::ranges::find(textures, PAVING), textures.end());
    for (const std::string& path : textures) {
        EXPECT_FALSE(core::isMeshPath(path)) << path;
    }
}

/**
 * @brief La composition produit une liste de maillages placés à côté de la liste de primitives.
 * \castest{<b>La composition produit une liste de maillages places.</b><br/>
 * \tcat Unitaire · Maillages d'un lieu<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Composer la carte d'essai, ses trois maillages chargés.<br/>
 *          2. Compter les maillages placés et les primitives, par pièce.<br/>
 *          3. Relever la pose d'un mur et celle du toit.<br/>
 * \tattendu Vingt-cinq dalles en maillage sur le calque du sol, huit murs et un toit sur le calque
 *           du décor ; cinquante-cinq dalles en image, et aucune primitive de relief. Le mur est
 *           posé au centre de sa case, au sol ; le toit au centre de son emprise de trois cases,
 *           élevé d'un étage, à l'étage 1.
 * }
 */
TEST(MeshCompositionTest, LaCompositionProduitUneListeDeMaillagesPlaces) {
    const Ilot place = ilot();
    const core::IsoProjection projection = place.projection();
    const hmi::ComposedScene scene =
        hmi::composeWorldScene(place.snapshot, projection, place.textures);

    EXPECT_EQ(countMeshes(scene, place.textures, FLOOR), 25U);
    EXPECT_EQ(countMeshes(scene, place.textures, WALL), 8U);
    EXPECT_EQ(countMeshes(scene, place.textures, ROOF), 1U);
    std::size_t floorImages = 0;
    for (const hmi::ComposedQuad& quad : scene.quads()) {
        EXPECT_NE(quad.layer, hmi::RenderLayer::Object) << "aucun relief n'est une image";
        if (quad.layer == hmi::RenderLayer::Tile) {
            ++floorImages;
            EXPECT_EQ(quad.stance, hmi::QuadStance::Ground);
        }
    }
    EXPECT_EQ(floorImages, 55U);

    const hmi::IsoView view(projection);
    const hmi::MeshHandle wall = place.textures.meshes.at(WALL).mesh;
    const hmi::MeshHandle roof = place.textures.meshes.at(ROOF).mesh;
    const hmi::MeshHandle floor = place.textures.meshes.at(FLOOR).mesh;
    bool firstWall = true;
    for (const hmi::ComposedMesh& mesh : scene.meshes()) {
        if (mesh.mesh == floor) {
            EXPECT_EQ(mesh.layer, hmi::RenderLayer::Tile);
            EXPECT_EQ(mesh.storey, 0);
        } else if (mesh.mesh == wall && std::exchange(firstWall, false)) {
            // Le premier mur composé est celui de la case (4, 2), le coin nord de l'anneau.
            EXPECT_EQ(mesh.layer, hmi::RenderLayer::Object);
            EXPECT_EQ(mesh.toView, view.meshTransform({4.5F, 2.5F}, 0.0F));
            const core::Rect tile = projection.tileBounds({.column = 4, .row = 2});
            EXPECT_NEAR(mesh.bounds.position.x, tile.position.x, TOLERANCE);
            EXPECT_NEAR(mesh.bounds.size.x, tile.size.x, TOLERANCE);
        } else if (mesh.mesh == roof) {
            EXPECT_EQ(mesh.layer, hmi::RenderLayer::Object);
            EXPECT_EQ(mesh.storey, 1);
            EXPECT_EQ(mesh.toView,
                      view.meshTransform({5.5F, 3.5F}, 0.875F * projection.tileWidth()));
            // Trois cases de large, et plus haut que les murs.
            EXPECT_NEAR(mesh.bounds.size.x, 3.0F * projection.tileWidth(), TOLERANCE);
        }
    }
}

/**
 * @brief Un maillage qui ne s'est pas chargé laisse voir le damier : la pièce manquante se voit.
 * \castest{<b>Un maillage manquant se voit, par le damier.</b><br/>
 * \tcat Unitaire · Maillages d'un lieu<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Composer la carte d'essai sans qu'aucun maillage ne soit chargé.<br/>
 * \tattendu Aucun maillage placé ; chaque pièce en maillage est composée en image sur le damier de
 *           repli — vingt-cinq dalles, huit murs, un toit —, jamais omise.
 * }
 */
TEST(MeshCompositionTest, UnMaillageManquantSeVoit) {
    const Ilot place = ilot(false);
    const hmi::ComposedScene scene =
        hmi::composeWorldScene(place.snapshot, place.projection(), place.textures);
    EXPECT_TRUE(scene.meshes().empty());
    const auto checker = static_cast<std::size_t>(
        std::ranges::count_if(scene.quads(), [&place](const hmi::ComposedQuad& quad) {
            return quad.texture == place.textures.missing.texture;
        }));
    EXPECT_EQ(checker, 25U + 8U + 1U);
}

/**
 * @brief Les images portent la ligne où elles se dressent, et un bloc de maquette l'élévation de
 *        ses sommets : de quoi leur donner une profondeur quand la scène a des volumes.
 * \castest{<b>Les images disent comment elles se tiennent dans la scene en volume.</b><br/>
 * \tcat Unitaire · Maillages d'un lieu<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Composer la carte d'essai avec une figurine au centre de la case (5, 6).<br/>
 *          2. Relever la tenue de la figurine, d'un jeton, d'une dalle.<br/>
 *          3. Composer une carte de maquette et relever l'élévation des sommets d'un bloc.<br/>
 * \tattendu La figurine est dressée sur ses pieds, au centre de sa position ; une dalle est à
 *           plat ; un jeton est une marque. Les sommets du dessus d'un bloc portent sa hauteur,
 *           ceux de sa base zéro.
 * }
 */
TEST(MeshCompositionTest, LesImagesDisentCommentEllesSeTiennent) {
    Ilot place = ilot();
    const core::IsoProjection projection = place.projection();
    // La figurine n'a pas d'image : le damier la dessine, comme dans les autres tests sans GPU.
    place.snapshot.figures = {
        hmi::WorldFigureSnapshot{.figure = "Npc/temoin", .point = {5.5F, 6.5F}}};
    const hmi::ComposedScene scene =
        hmi::composeWorldScene(place.snapshot, projection, place.textures);
    bool figureSeen = false;
    for (const hmi::ComposedQuad& quad : scene.quads()) {
        if (quad.layer == hmi::RenderLayer::Player) {
            figureSeen = true;
            EXPECT_EQ(quad.stance, hmi::QuadStance::Upright);
            EXPECT_NEAR(quad.footY, projection.gridToWorld({5.5F, 6.5F}).y, TOLERANCE);
        }
    }
    EXPECT_TRUE(figureSeen);
    EXPECT_EQ(hmi::defaultStance(hmi::RenderLayer::UI), hmi::QuadStance::Overlay);
    EXPECT_EQ(hmi::defaultStance(hmi::RenderLayer::Tile), hmi::QuadStance::Ground);
    EXPECT_EQ(hmi::defaultStance(hmi::RenderLayer::Object), hmi::QuadStance::Upright);

    // Une maquette : un seul mur, extrudé en bloc de trois faces.
    hmi::WorldSceneSnapshot maquette;
    maquette.columns = 3;
    maquette.rows = 3;
    maquette.types.assign(9, core::TileType::Grass);
    maquette.types[4] = core::TileType::Wall;
    const core::IsoProjection small(3, 3);
    const hmi::ComposedScene blocks = hmi::composeWorldScene(maquette, small, place.textures);
    std::size_t raisedVertices = 0;
    std::size_t groundVertices = 0;
    for (const hmi::ComposedQuad& quad : blocks.quads()) {
        if (quad.kind != hmi::QuadKind::Poly || quad.layer != hmi::RenderLayer::Object) {
            continue;
        }
        EXPECT_EQ(quad.stance, hmi::QuadStance::Ground);
        for (const float rise : quad.poly.rise) {
            raisedVertices += rise > 0.0F ? 1U : 0U;
            groundVertices += rise == 0.0F ? 1U : 0U;
        }
    }
    // Deux flancs (deux sommets au sol, deux élevés) et le dessus (quatre élevés).
    EXPECT_EQ(groundVertices, 4U);
    EXPECT_EQ(raisedVertices, 8U);
}

/**
 * @brief Le lieu composé une fois garde ses maillages, et n'en rend que ce que le cadrage montre.
 * \castest{<b>Le lieu compose une fois garde ses maillages et les cadre.</b><br/>
 * \tcat Unitaire · Maillages d'un lieu<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Composer la carte d'essai dans une `StaticWorldScene`.<br/>
 *          2. En tirer une image sans cadrage, puis une image cadrée loin de l'îlot.<br/>
 * \tattendu Sans cadrage, les trente-quatre maillages ; cadrée sur le coin ouest de la carte,
 *           aucun mur ni toit, et la scène n'est pas dite vide tant qu'elle a des maillages.
 * }
 */
TEST(MeshCompositionTest, LeLieuComposeUneFoisGardeSesMaillages) {
    const Ilot place = ilot();
    const core::IsoProjection projection = place.projection();
    hmi::StaticWorldScene statics;
    statics.build(place.snapshot, projection, place.textures);
    EXPECT_FALSE(statics.empty());

    hmi::ComposedScene whole;
    statics.compose(whole, {}, place.textures);
    EXPECT_EQ(whole.meshes().size(), 34U);

    hmi::ComposedScene corner;
    const core::Vector2 west = projection.gridToWorld({0.5F, 7.5F});
    corner.setVisibleBounds(core::Rect{{west.x - 2.0F, west.y - 1.0F}, {4.0F, 2.0F}});
    statics.compose(corner, {}, place.textures);
    EXPECT_EQ(countMeshes(corner, place.textures, WALL), 0U);
    EXPECT_EQ(countMeshes(corner, place.textures, ROOF), 0U);
    EXPECT_LT(corner.meshes().size(), whole.meshes().size());

    // Une seconde image ne cumule pas les maillages de la première.
    corner.clear();
    statics.compose(corner, {}, place.textures);
    EXPECT_EQ(countMeshes(corner, place.textures, WALL), 0U);
}
