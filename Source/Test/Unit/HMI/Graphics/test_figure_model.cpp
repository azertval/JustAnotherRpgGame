// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_figure_model.cpp
 * @brief Une figurine en **modèle**, de l'instantané à la scène composée, sans GPU (`LOT-1005`).
 *
 * Le pantin de la carte d'essai (`Fixtures/Meshes`) : la composition en fait un maillage placé,
 * tourné vers son cap, avec la pose de ses os à l'instant de son clip. Une figurine en bandes
 * garde son chemin : la même carte en porte une de chaque forme.
 */

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <numbers>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Resources/MeshFile.h"
#include "Core/Resources/SkeletonFile.h"
#include "Core/Resources/SkeletonPose.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/IsoView.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

constexpr float TOLERANCE = 1e-3F;
constexpr const char* MODEL = "Npc/pantin/pantin.glb";
constexpr const char* STRIP = "Npc/temoin/idle.png";

std::filesystem::path assets() {
    return std::filesystem::path(JADG_MESH_FIXTURE_DIR) / "Assets";
}

/// Une carte nue de six cases sur six, et ce que le rendu aurait chargé : le pantin en modèle, la
/// figurine témoin en bande.
struct Stage {
    hmi::WorldSceneSnapshot snapshot;
    hmi::ScenePieceTextures textures;
    std::array<std::uint8_t, 4> identities{};

    [[nodiscard]] core::IsoProjection projection() const {
        return {snapshot.columns, snapshot.rows, core::ARENA_TILE_WIDTH_UNITS,
                snapshot.diamondRatio};
    }
};

Stage stage() {
    Stage place;
    place.snapshot.columns = 6;
    place.snapshot.rows = 6;
    core::MeshFileResult read = core::readMeshFile(assets() / MODEL);
    EXPECT_TRUE(read.ok()) << read.message;
    core::SkeletonFileResult skeleton =
        core::readSkeletonFile(assets() / core::skeletonFilePath("pantin"));
    EXPECT_TRUE(skeleton.ok()) << skeleton.message;
    place.textures.figures[MODEL] = hmi::SceneFigureModel{
        .mesh = &place.identities[0],
        .minimum = read.mesh.minimum,
        .maximum = read.mesh.maximum,
        .rig = std::make_shared<const core::MeshRig>(std::move(read.mesh.rig)),
        .skeleton =
            std::make_shared<const core::SkeletonDescription>(std::move(skeleton.skeleton))};
    place.textures.byPath[STRIP] = hmi::SceneTexture{
        .texture = &place.identities[1], .width = 32, .height = 48, .frameWidth = 32};
    place.textures.missing =
        hmi::SceneTexture{.texture = &place.identities[2], .width = 64, .height = 64};
    return place;
}

hmi::WorldFigureSnapshot puppet(std::string clip, float seconds) {
    return hmi::WorldFigureSnapshot{.figure = "Npc/pantin",
                                    .clip = std::move(clip),
                                    .point = {2.5F, 3.5F},
                                    .seconds = seconds,
                                    .model = MODEL};
}

hmi::ComposedScene compose(const Stage& place, std::vector<hmi::WorldFigureSnapshot> figures) {
    hmi::ComposedScene scene;
    hmi::composeWorldFigures(scene, place.snapshot, figures, place.projection(), place.textures);
    return scene;
}

/// Où la pose du maillage @p mesh met le sommet du crâne du pantin, lié à sa tête (os 2).
std::array<float, 3> crown(const hmi::ComposedScene& scene, const hmi::ComposedMesh& mesh) {
    return core::skinnedPosition(
        scene.poseOf(mesh),
        core::MeshSkinVertex{.joints = {2, 0, 0, 0}, .weights = {1.0F, 0.0F, 0.0F, 0.0F}},
        {0.0F, 1.8F, 0.0F});
}

}  // namespace

/**
 * @brief Une figurine en modèle se compose en maillage placé avec la pose de son clip ; une
 *        figurine en bandes, à côté, reste une image.
 * \castest{<b>Une figurine en modele se compose en maillage, avec la pose de son clip.</b><br/>
 * \tcat Unitaire · Rendu d'un lieu · Squelette<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Composer le pantin a 0,4 s de son attaque et la figurine temoin, en bandes.<br/>
 *          2. Lire la scene composee.<br/>
 * \tattendu Un maillage sur le calque des figurines, avec une pose de trois os ou le sommet du
 *           crane est penche de 60 degres ; une primitive pour la figurine en bandes ; les chemins
 *           de texture demandes ne citent aucune bande du pantin, son modele est demande a part.
 * }
 */
TEST(FigureModelTest, UneFigurineEnModeleSeComposeEnMaillage) {
    const Stage place = stage();
    const std::vector<hmi::WorldFigureSnapshot> figures = {
        puppet("attack", 0.4F),
        hmi::WorldFigureSnapshot{.figure = "Npc/temoin", .point = {4.5F, 1.5F}}};
    const hmi::ComposedScene scene = compose(place, figures);

    ASSERT_EQ(scene.meshes().size(), 1U);
    const hmi::ComposedMesh& mesh = scene.meshes().front();
    EXPECT_EQ(mesh.layer, hmi::RenderLayer::Player);
    EXPECT_EQ(mesh.mesh, &place.identities[0]);
    ASSERT_EQ(mesh.poseFloats, 3U * 16U);
    const std::array<float, 3> top = crown(scene, mesh);
    EXPECT_NEAR(top[1], 0.9F + (0.9F * 0.5F), TOLERANCE);
    EXPECT_NEAR(top[2], 0.9F * std::sin(std::numbers::pi_v<float> / 3.0F), TOLERANCE);

    ASSERT_EQ(scene.quads().size(), 1U);
    EXPECT_EQ(scene.quads().front().texture, &place.identities[1]);

    const std::vector<std::string> textures = hmi::worldFigureTexturePaths(place.snapshot, figures);
    for (const std::string& path : textures) {
        EXPECT_FALSE(path.starts_with("Npc/pantin")) << path;
    }
    EXPECT_EQ(hmi::worldFigureModelPaths(figures), (std::vector<std::string>{MODEL}));
}

/**
 * @brief La boucle et la fin d'un clip sont des données du squelette : la marche revient à son
 *        début, la chute se fige, et un clip que le modèle n'a pas retombe sur son repos.
 * \castest{<b>Un clip boucle ou se fige selon ce que son squelette declare.</b><br/>
 * \tcat Unitaire · Rendu d'un lieu · Squelette<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Composer le pantin a 0,125 s puis a 0,625 s de sa marche (clip de 0,5 s, en
 *          boucle).<br/>2. Le composer a 0,8 s puis a 30 s de sa chute (jouee une fois).<br/>
 *          3. Le composer sur un tir, qu'il n'a pas, puis sur un clip inconnu.<br/>
 * \tattendu Les deux poses de marche sont les memes ; les deux poses de chute aussi, le crane a
 *           0,2 m du sol ; le tir joue l'attaque ; le clip inconnu joue le repos.
 * }
 */
TEST(FigureModelTest, UnClipBoucleOuSeFige) {
    const Stage place = stage();
    const auto poseOf = [&place](std::string clip, float seconds) {
        const hmi::ComposedScene scene = compose(place, {puppet(std::move(clip), seconds)});
        EXPECT_EQ(scene.meshes().size(), 1U);
        const std::span<const float> pose = scene.poseOf(scene.meshes().front());
        return std::vector<float>(pose.begin(), pose.end());
    };
    const std::vector<float> step = poseOf("walk", 0.125F);
    const std::vector<float> nextCycle = poseOf("walk", 0.625F);
    ASSERT_EQ(step.size(), nextCycle.size());
    for (std::size_t index = 0; index < step.size(); ++index) {
        EXPECT_NEAR(step[index], nextCycle[index], TOLERANCE);
    }
    EXPECT_NE(step, poseOf("walk", 0.0F)) << "a 0,125 s le buste est tourne";

    EXPECT_EQ(poseOf("death", 0.8F), poseOf("death", 30.0F));
    {
        const hmi::ComposedScene fallen = compose(place, {puppet("death", 30.0F)});
        EXPECT_NEAR(crown(fallen, fallen.meshes().front())[1], 0.2F, TOLERANCE);
    }
    EXPECT_EQ(poseOf("ranged", 0.4F), poseOf("attack", 0.4F));
    EXPECT_EQ(poseOf("dance", 0.25F), poseOf("idle", 0.25F));
}

/**
 * @brief Un modèle s'oriente librement : il fait face à son cap, quel qu'il soit, et à défaut à
 *        la diagonale de son orientation.
 * \castest{<b>Un modele fait face a son cap, sans table de quatre orientations.</b><br/>
 * \tcat Unitaire · Rendu d'un lieu · Squelette<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Composer le pantin avec un cap vers les colonnes croissantes, vers les lignes
 *          croissantes, puis a 30 degres entre les deux.<br/>2. Le composer sans cap, oriente au
 *          nord-ouest.<br/>3. Lire ou sa pose dans la vue met un point situe un metre devant
 *          lui.<br/>
 * \tattendu Le point devant lui tombe a l'ecran ou tombe le point de la grille situe un metre
 *           plus loin dans la direction du cap ; le pied reste sur sa position dans tous les cas ;
 *           le cap de 30 degres n'est aucune des quatre diagonales.
 * }
 */
TEST(FigureModelTest, UnModeleFaitFaceASonCap) {
    const Stage place = stage();
    const core::IsoProjection projection = place.projection();
    const float cell = 1.0F / core::METERS_PER_TILE;  // un metre, en cases
    const auto ahead = [&](hmi::WorldFigureSnapshot figure) {
        const hmi::ComposedScene scene = compose(place, {std::move(figure)});
        EXPECT_EQ(scene.meshes().size(), 1U);
        const hmi::ViewTransform& toView = scene.meshes().front().toView;
        const std::array<float, 3> foot = hmi::IsoView::apply(toView, 0.0F, 0.0F, 0.0F);
        const core::Vector2 expected = projection.gridToWorld({2.5F, 3.5F});
        EXPECT_NEAR(foot[0], expected.x, TOLERANCE);
        EXPECT_NEAR(foot[1], expected.y, TOLERANCE);
        return hmi::IsoView::apply(toView, 0.0F, 0.0F, 1.0F);
    };
    const auto expectAhead = [&](const std::array<float, 3>& point, float heading) {
        const core::Vector2 target = projection.gridToWorld(
            {2.5F + (cell * std::cos(heading)), 3.5F + (cell * std::sin(heading))});
        EXPECT_NEAR(point[0], target.x, TOLERANCE);
        EXPECT_NEAR(point[1], target.y, TOLERANCE);
    };

    constexpr float QUARTER = std::numbers::pi_v<float> / 2.0F;
    for (const float heading : {0.0F, QUARTER, QUARTER / 3.0F, -2.0F}) {
        hmi::WorldFigureSnapshot figure = puppet("idle", 0.0F);
        figure.heading = heading;
        expectAhead(ahead(std::move(figure)), heading);
    }
    hmi::WorldFigureSnapshot northWest = puppet("idle", 0.0F);
    northWest.facing = hmi::FigureFacing::NorthWest;
    expectAhead(ahead(std::move(northWest)), 2.0F * QUARTER);

    EXPECT_NEAR(hmi::figureHeadingFor({1.0F, 1.0F}, 0.0F), QUARTER / 2.0F, TOLERANCE);
    EXPECT_FLOAT_EQ(hmi::figureHeadingFor({0.0F, 0.0F}, 1.25F), 1.25F) << "a l'arret, il le garde";
}

/**
 * @brief Un modèle nommé mais pas chargé ne plante rien : la figurine retombe sur ses bandes,
 *        donc sur le damier.
 * \castest{<b>Un modele qui ne s'est pas charge laisse voir le damier.</b><br/>
 * \tcat Unitaire · Rendu d'un lieu · Squelette<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Composer une figurine dont le modele n'est pas parmi ceux du rendu.<br/>
 * \tattendu Aucun maillage ; une primitive, sur le damier.
 * }
 */
TEST(FigureModelTest, UnModeleAbsentLaisseVoirLeDamier) {
    const Stage place = stage();
    hmi::WorldFigureSnapshot figure = puppet("idle", 0.0F);
    figure.model = "Npc/pantin/absent.glb";
    const hmi::ComposedScene scene = compose(place, {std::move(figure)});
    EXPECT_TRUE(scene.meshes().empty());
    ASSERT_EQ(scene.quads().size(), 1U);
    EXPECT_EQ(scene.quads().front().texture, &place.identities[2]);
}
