// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_scene_lighting.cpp
 * @brief L'éclairage d'une image, **sans GPU** (`LOT-1007`) : ce que la lumière d'une heure, les
 *        sources d'un lieu et le cadrage donnent aux shaders, et ce qu'une carte relève de ses
 *        pièces — leurs lumières, leur éclat, leurs boîtes d'ombre.
 */

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileMap.h"
#include "Core/Rpg/Scale.h"
#include "Core/World/DayLight.h"
#include "Core/World/LightSource.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/IsoView.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/SceneLighting.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

using Vec3 = std::array<float, 3>;

const core::IsoProjection PROJECTION{24, 13};

/// Ce que la caméra montre : un rectangle de 40 unités sur 24 au milieu de la carte.
core::Rect visible(float left = 20.0F, float top = 10.0F) {
    return core::Rect{{left, top}, {40.0F, 24.0F}};
}

hmi::WorldLighting lightingAt(float minutes, bool shadows = true) {
    return hmi::WorldLighting{.light = core::DayLightTable::factory().sample(minutes),
                              .shadows = shadows,
                              .shadowSize = 2048,
                              .seconds = 0.0F};
}

/// Le point `m` × (x, y, z, 1) d'une matrice rangée par colonnes.
Vec3 mapped(const hmi::LightMatrix& m, const Vec3& p) {
    return {(m[0] * p[0]) + (m[4] * p[1]) + (m[8] * p[2]) + m[12],
            (m[1] * p[0]) + (m[5] * p[1]) + (m[9] * p[2]) + m[13],
            (m[2] * p[0]) + (m[6] * p[1]) + (m[10] * p[2]) + m[14]};
}

float length(const std::array<float, 4>& v) {
    return std::sqrt((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]));
}

core::LightSource lamp(float column, float row, bool always = false, bool flicker = false) {
    return core::LightSource{.column = column,
                             .row = row,
                             .emission = {.color = {1.0F, 0.8F, 0.6F},
                                          .radius = 6.0F,
                                          .height = 2.5F,
                                          .intensity = 1.0F,
                                          .flicker = flicker,
                                          .always = always}};
}

/// Un lieu d'essai écrit dans un dossier temporaire : sa table, et le manifeste de ses pièces.
struct PlaceFiles {
    std::filesystem::path directory;

    PlaceFiles() {
        directory = std::filesystem::temp_directory_path() /
                    ("jadg-lumiere-" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        std::filesystem::create_directories(directory);
        std::ofstream(directory / "appearance.json")
            << R"({"version": 1, "place": "essai", "floors": {}, "relief": {}})";
        std::ofstream(directory / "manifest.json") << R"({
            "version": 1, "disposition": "essai", "tile": [256, 159],
            "textures": {
                "scene/essai/paving": { "file": "paving.png", "class": "floor",
                    "size": [256, 159], "anchor": [128, 0] },
                "scene/essai/wall": { "file": "wall.png", "class": "tall", "family": "02",
                    "size": [256, 386], "anchor": [128, 227] },
                "scene/essai/lamppost": { "file": "lamppost.png", "class": "tall", "family": "08",
                    "size": [64, 420], "anchor": [32, 284],
                    "light": { "color": "#ffd090", "radius": 7.5, "height": 3.0 } },
                "scene/essai/brazier": { "file": "brazier.png", "class": "tall", "family": "08",
                    "size": [128, 150], "anchor": [64, 70],
                    "light": { "radius": 5.0, "height": 1.2, "flicker": true }, "glow": 0.8 },
                "scene/essai/rug": { "file": "rug.png", "class": "tall", "family": "08",
                    "size": [256, 170], "anchor": [128, 11] }
            } })";
    }
    ~PlaceFiles() {
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
    }
    PlaceFiles(const PlaceFiles&) = delete;
    PlaceFiles& operator=(const PlaceFiles&) = delete;

    [[nodiscard]] hmi::PlaceAppearance appearance() const {
        hmi::PlaceAppearanceResult read =
            hmi::PlaceAppearance::loadFromFile(directory / "appearance.json");
        EXPECT_TRUE(read.ok()) << read.message;
        return std::move(read.appearance);
    }
};

core::TileLayer decorLayer(std::string name, int floor,
                           const std::vector<std::pair<core::GridPosition, std::string>>& pieces) {
    core::TileLayer layer{.name = std::move(name),
                          .kind = core::LayerKind::Decor,
                          .tiles = core::TileMap{4, 4},
                          .properties = {},
                          .floor = floor};
    for (const auto& [cell, piece] : pieces) {
        layer.tiles.setTile(cell.column, cell.row, core::TileType::Wall);
        layer.setPiece(cell.column, cell.row, piece);
    }
    return layer;
}

/// Une carte de 4 × 4 : un mur en (0, 0) et un autre à l'étage au-dessus, un lampadaire en
/// (1, 1), un brasero en (2, 2), un tapis en (3, 3), et une entité `light` en (3, 0).
core::Level litMap() {
    core::LevelData data{.name = "essai", .tileMap = core::TileMap{4, 4}};
    core::TileLayer ground{.name = "sol",
                           .kind = core::LayerKind::Ground,
                           .tiles = core::TileMap{4, 4},
                           .properties = {{"scene", std::string{"essai"}}}};
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            ground.tiles.setTile(column, row, core::TileType::Pavement);
            ground.setPiece(column, row, "paving");
        }
    }
    data.layers.push_back(std::move(ground));
    data.layers.push_back(decorLayer("rez", 0,
                                     {{{.column = 0, .row = 0}, "wall"},
                                      {{.column = 1, .row = 1}, "lamppost"},
                                      {{.column = 2, .row = 2}, "brazier"},
                                      {{.column = 3, .row = 3}, "rug"}}));
    data.layers.push_back(decorLayer("etage", 1, {{{.column = 0, .row = 0}, "wall"}}));
    data.entities.push_back(
        core::MapEntity{.type = std::string{core::LIGHT_ENTITY_TYPE},
                        .position = {.column = 3, .row = 0},
                        .properties = {{std::string{core::LIGHT_ALWAYS_PROPERTY}, true}}});
    return core::Level{std::move(data)};
}

}  // namespace

/**
 * @brief Le bloc neutre laisse les shaders dessiner comme avant le lot, et un éclairage réglé dit
 *        la teinte, l'ambiance et un soleil unitaire dans la vue.
 * \castest{<b>Le bloc d'eclairage est neutre par defaut, et dit la lumiere de l'heure.</b><br/>
 * \tcat Unitaire · Rendu d'un lieu · Lumière<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire un bloc `LightingUniforms` d'usine.<br/>
 *          2. Bâtir l'éclairage de midi sur le cadrage d'essai.<br/>
 *          3. Le bâtir sans ombres, puis à une heure où la lumière dirigée est noire (20:30).<br/>
 * \tattendu Le bloc d'usine est inactif (`tint.a` nul) et sans lumière. À midi il est actif, sa
 *           teinte est blanche, ses vecteurs vers le soleil et vers le haut sont unitaires, et
 *           une passe d'ombres est demandée, son texel valant 1/2048. Sans ombres, ou sous une
 *           lumière dirigée noire, aucune passe n'est demandée et les shaders ne lisent pas la
 *           carte.
 * }
 */
TEST(SceneLightingTest, LeBlocEstNeutreParDefautEtDitLaLumiereDeLHeure) {
    const hmi::LightingUniforms neutral;
    EXPECT_FLOAT_EQ(neutral.tint[3], 0.0F);
    EXPECT_FLOAT_EQ(neutral.sun[3], 0.0F);
    EXPECT_FLOAT_EQ(neutral.up[3], 0.0F);

    const hmi::IsoView view(PROJECTION);
    const hmi::SceneLightFrame noon =
        hmi::buildSceneLighting(view, visible(), lightingAt(720.0F), {}, true);
    EXPECT_FLOAT_EQ(noon.uniforms.tint[3], 1.0F);
    EXPECT_FLOAT_EQ(noon.uniforms.tint[0], 1.0F);
    EXPECT_NEAR(length(noon.uniforms.toSun), 1.0F, 1e-4F);
    EXPECT_NEAR(length(noon.uniforms.up), 1.0F, 1e-4F);
    // La verticale du lieu monte à l'écran : y vers le bas, donc négatif.
    EXPECT_LT(noon.uniforms.up[1], 0.0F);
    EXPECT_TRUE(noon.shadows);
    EXPECT_FLOAT_EQ(noon.uniforms.sun[3], 1.0F);
    EXPECT_FLOAT_EQ(noon.uniforms.toSun[3], 1.0F / 2048.0F);
    EXPECT_FLOAT_EQ(noon.uniforms.ambient[3], lightingAt(720.0F).light.shadow);

    const hmi::SceneLightFrame flat =
        hmi::buildSceneLighting(view, visible(), lightingAt(720.0F, false), {}, true);
    EXPECT_FALSE(flat.shadows);
    EXPECT_FLOAT_EQ(flat.uniforms.sun[3], 0.0F);
    EXPECT_FLOAT_EQ(flat.uniforms.tint[3], 1.0F) << "sans ombres, le lieu reste éclairé";

    const hmi::SceneLightFrame dark =
        hmi::buildSceneLighting(view, visible(), lightingAt(1230.0F), {}, true);
    EXPECT_FALSE(dark.shadows) << "une lumière dirigée noire ne jette pas d'ombre";
}

/**
 * @brief La carte d'ombres couvre ce que la caméra montre : le centre de l'image tombe au milieu
 *        de la carte, un point élevé est plus près du soleil, et la carte glisse par texels
 *        entiers.
 * \castest{<b>La carte d'ombres couvre l'image et glisse par texels entiers.</b><br/>
 * \tcat Unitaire · Rendu d'un lieu · Lumière<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Bâtir l'éclairage de midi et amener dans la carte d'ombres le point du sol au
 *             centre de l'image, ses quatre coins, et le centre élevé de deux mètres.<br/>
 *          2. Décaler le cadrage de quelques centièmes d'unité, cent fois, et relever à chaque
 *             fois où tombe un même point du lieu.<br/>
 * \tattendu Le centre tombe à moins d'un texel du milieu (0,5 ; 0,5) ; les coins sont dans la
 *           carte ; toutes les profondeurs sont dans ]0, 1[ et le point élevé est plus proche
 *           du soleil que le sol sous lui. D'un cadrage au suivant, le point ne se déplace dans
 *           la carte que d'un nombre entier de texels.
 * }
 */
TEST(SceneLightingTest, LaCarteDOmbresCouvreLImageEtGlisseParTexels) {
    const hmi::IsoView view(PROJECTION);
    const core::Rect shown = visible();
    const hmi::SceneLightFrame frame =
        hmi::buildSceneLighting(view, shown, lightingAt(720.0F), {}, true);
    ASSERT_TRUE(frame.shadows);
    const float texel = 1.0F / 2048.0F;

    const auto ground = [&view](float x, float y) { return Vec3{x, y, view.groundDepth(y)}; };
    const float centreX = shown.position.x + (shown.size.x / 2.0F);
    const float centreY = shown.position.y + (shown.size.y / 2.0F);
    const Vec3 centre = mapped(frame.uniforms.viewToShadow, ground(centreX, centreY));
    EXPECT_NEAR(centre[0], 0.5F, 2.0F * texel);
    EXPECT_NEAR(centre[1], 0.5F, 2.0F * texel);
    EXPECT_GT(centre[2], 0.0F);
    EXPECT_LT(centre[2], 1.0F);

    for (const auto& [x, y] :
         {std::pair{shown.position.x, shown.position.y},
          std::pair{shown.position.x + shown.size.x, shown.position.y},
          std::pair{shown.position.x + shown.size.x, shown.position.y + shown.size.y},
          std::pair{shown.position.x, shown.position.y + shown.size.y}}) {
        const Vec3 corner = mapped(frame.uniforms.viewToShadow, ground(x, y));
        EXPECT_GT(corner[0], 0.0F);
        EXPECT_LT(corner[0], 1.0F);
        EXPECT_GT(corner[1], 0.0F);
        EXPECT_LT(corner[1], 1.0F);
        EXPECT_GT(corner[2], 0.0F);
        EXPECT_LT(corner[2], 1.0F);
    }

    // Deux mètres au-dessus du centre : la même colonne du lieu, plus près du soleil.
    const float rise = view.riseOf(2.0F);
    const Vec3 raised = mapped(frame.uniforms.viewToShadow,
                               {centreX, centreY - rise, view.raisedDepth(centreY, rise)});
    EXPECT_LT(raised[2], centre[2]);

    // La même matrice par les mètres : le clip de la passe d'ombres, puis sa mise en texture.
    const hmi::LightMatrix toMetres = hmi::invertedAffine(hmi::metresToView(view));
    const Vec3 metres = mapped(toMetres, ground(centreX, centreY));
    EXPECT_NEAR(metres[1], 0.0F, 1e-3F) << "un point du sol est à la hauteur zéro";
    const Vec3 clip = mapped(frame.metresToShadowClip, metres);
    EXPECT_NEAR((clip[0] * 0.5F) + 0.5F, centre[0], 1e-4F);
    EXPECT_NEAR((clip[2] * 0.5F) + 0.5F, centre[2], 1e-4F);

    // Le calage : un point fixe du lieu ne bouge dans la carte que par texels entiers.
    const Vec3 fixed = ground(centreX, centreY);
    for (int step = 1; step <= 100; ++step) {
        const float shift = static_cast<float>(step) * 0.037F;
        const hmi::SceneLightFrame moved = hmi::buildSceneLighting(
            view, visible(20.0F + shift, 10.0F + (shift / 3.0F)), lightingAt(720.0F), {}, true);
        const Vec3 now = mapped(moved.uniforms.viewToShadow, fixed);
        const float texelsX = (now[0] - centre[0]) / texel;
        const float texelsY = (now[1] - centre[1]) / texel;
        EXPECT_NEAR(texelsX, std::round(texelsX), 0.02F) << step;
        EXPECT_NEAR(texelsY, std::round(texelsY), 0.02F) << step;
    }
}

/**
 * @brief Les lumières de nuit s'allument au crépuscule, sauf celles qui restent toujours
 *        allumées ; seules celles qui touchent l'image sont gardées, seize au plus, les plus
 *        proches du centre.
 * \castest{<b>Les lumieres de nuit sont choisies pour l'image.</b><br/>
 * \tcat Unitaire · Rendu d'un lieu · Lumière<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Donner deux sources, dont une toujours allumée, à midi puis à minuit.<br/>
 *          2. Donner une source hors de l'image, au-delà de sa portée.<br/>
 *          3. Donner quarante sources en ligne, de plus en plus loin du centre.<br/>
 *          4. Donner une flamme et lire sa couleur à cent instants.<br/>
 * \tattendu À midi, seule la source toujours allumée est gardée ; à minuit, les deux. La source
 *           hors de l'image est écartée. Des quarante, seize sont gardées, et ce sont les plus
 *           proches du centre. La flamme tremble : sa couleur varie d'un instant à l'autre, sans
 *           jamais dépasser sa pleine intensité ni tomber sous les quatre cinquièmes.
 * }
 */
TEST(SceneLightingTest, LesLumieresDeNuitSontChoisiesPourLImage) {
    const hmi::IsoView view(PROJECTION);
    // Le centre de l'image, en cases : là où les sources d'essai se posent.
    const core::Rect shown = visible();
    const std::optional<core::GridPosition> middle = PROJECTION.worldToTile(
        {shown.position.x + (shown.size.x / 2.0F), shown.position.y + (shown.size.y / 2.0F)});
    ASSERT_TRUE(middle.has_value());
    const auto column = static_cast<float>(middle->column);
    const auto row = static_cast<float>(middle->row);

    const std::vector<core::LightSource> pair{lamp(column, row), lamp(column + 1.0F, row, true)};
    const hmi::SceneLightFrame noon =
        hmi::buildSceneLighting(view, shown, lightingAt(720.0F), pair, true);
    EXPECT_FLOAT_EQ(noon.uniforms.up[3], 1.0F) << "à midi, seule la source toujours allumée";
    const hmi::SceneLightFrame midnight =
        hmi::buildSceneLighting(view, shown, lightingAt(0.0F), pair, true);
    EXPECT_FLOAT_EQ(midnight.uniforms.up[3], 2.0F);
    EXPECT_NEAR(midnight.uniforms.lightPosition[0][3], 6.0F * view.unitsPerMetre(), 1e-3F)
        << "la portée est en unités de la vue";
    EXPECT_NEAR(midnight.uniforms.lightColor[0][0], 1.0F, 1e-4F);
    EXPECT_NEAR(midnight.uniforms.lightColor[0][2], 0.6F, 1e-4F);

    const std::vector<core::LightSource> far{lamp(column + 400.0F, row)};
    EXPECT_FLOAT_EQ(
        hmi::buildSceneLighting(view, shown, lightingAt(0.0F), far, true).uniforms.up[3], 0.0F);

    std::vector<core::LightSource> many;
    for (int index = 39; index >= 0; --index) {
        many.push_back(lamp(column + (static_cast<float>(index) * 0.1F), row));
    }
    const hmi::SceneLightFrame crowded =
        hmi::buildSceneLighting(view, shown, lightingAt(0.0F), many, true);
    EXPECT_FLOAT_EQ(crowded.uniforms.up[3], static_cast<float>(hmi::MAXIMUM_SCENE_LIGHTS));
    // Les seize plus proches du centre : leurs abscisses tiennent dans celles des seize premières.
    const hmi::LightMatrix toView = hmi::metresToView(view);
    const Vec3 beyond = mapped(
        toView, {(column + 2.0F) * core::METERS_PER_TILE, 2.5F, row * core::METERS_PER_TILE});
    for (std::size_t index = 0; index < hmi::MAXIMUM_SCENE_LIGHTS; ++index) {
        EXPECT_LT(crowded.uniforms.lightPosition[index][0], beyond[0]) << index;
    }

    const std::vector<core::LightSource> flame{lamp(column, row, true, true)};
    float lowest = 2.0F;
    float highest = 0.0F;
    for (int step = 0; step < 100; ++step) {
        hmi::WorldLighting lighting = lightingAt(0.0F);
        lighting.seconds = static_cast<float>(step) * 0.07F;
        const float red =
            hmi::buildSceneLighting(view, shown, lighting, flame, true).uniforms.lightColor[0][0];
        lowest = std::min(lowest, red);
        highest = std::max(highest, red);
    }
    EXPECT_LE(highest, 1.0F + 1e-4F);
    EXPECT_GE(lowest, 0.8F);
    EXPECT_GT(highest - lowest, 0.03F) << "une flamme tremble";
}

/**
 * @brief L'inverse d'une matrice affine la défait, et la pose d'un maillage se range par colonnes.
 * \castest{<b>Les matrices de l'eclairage s'inversent et se composent.</b><br/>
 * \tcat Unitaire · Rendu d'un lieu · Lumière<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Prendre la matrice qui amène le lieu en mètres dans la vue, et son inverse.<br/>
 *          2. Amener trois points dans la vue, puis les ramener.<br/>
 *          3. Comparer la vue du point de grille (3, 2) à celle de sa projection.<br/>
 * \tattendu Chaque point revient à sa place, au millième près ; le produit des deux matrices est
 *           l'identité ; le point de grille (3, 2), au sol, tombe où `core::IsoProjection` le
 *           met, à la profondeur du sol.
 * }
 */
TEST(SceneLightingTest, LesMatricesSInversentEtSeComposent) {
    const hmi::IsoView view(PROJECTION);
    const hmi::LightMatrix toView = hmi::metresToView(view);
    const hmi::LightMatrix toMetres = hmi::invertedAffine(toView);
    for (const Vec3& point :
         {Vec3{0.0F, 0.0F, 0.0F}, Vec3{4.5F, 2.0F, 9.0F}, Vec3{-3.0F, 0.5F, 12.25F}}) {
        const Vec3 back = mapped(toMetres, mapped(toView, point));
        EXPECT_NEAR(back[0], point[0], 1e-3F);
        EXPECT_NEAR(back[1], point[1], 1e-3F);
        EXPECT_NEAR(back[2], point[2], 1e-3F);
    }
    const hmi::LightMatrix identity = hmi::multiplied(toMetres, toView);
    for (std::size_t index = 0; index < identity.size(); ++index) {
        EXPECT_NEAR(identity[index], index % 5 == 0 ? 1.0F : 0.0F, 1e-4F) << index;
    }
    const core::Vector2 projected = PROJECTION.gridToWorld({3.0F, 2.0F});
    const Vec3 seen =
        mapped(toView, {3.0F * core::METERS_PER_TILE, 0.0F, 2.0F * core::METERS_PER_TILE});
    EXPECT_NEAR(seen[0], projected.x, 1e-3F);
    EXPECT_NEAR(seen[1], projected.y, 1e-3F);
    EXPECT_NEAR(seen[2], view.groundDepth(projected.y), 1e-3F);
}

/**
 * @brief Une carte relève de ses pièces leurs lumières, leur éclat et leurs boîtes d'ombre, et
 *        de ses entités `light` les leurs.
 * \castest{<b>Une carte releve ses lumieres, ses eclats et ses boites d'ombre.</b><br/>
 * \tcat Unitaire · Rendu d'un lieu · Lumière<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Écrire un lieu d'essai : un mur, un lampadaire qui éclaire, un brasero qui éclaire
 *             et garde son éclat, un tapis, un dallage.<br/>
 *          2. Composer une carte de 4 × 4 : le mur en (0, 0) et à l'étage au-dessus, le
 *             lampadaire en (1, 1), le brasero en (2, 2), le tapis en (3, 3), une entité `light`
 *             en (3, 0).<br/>
 * \tattendu Trois sources : le lampadaire au centre de sa case, à 3 m ; le brasero, tremblant ;
 *           l'entité, toujours allumée. Un seul éclat, celui du brasero. Quatre boîtes d'ombre :
 *           le mur du rez — toute sa case, 2,4 m de haut, droit — ; le mur d'étage, sur une base
 *           de 2,4 m ; le lampadaire, un quart de case de large, 3 m de haut, resserré ; le
 *           brasero. Ni le dallage ni le tapis, trop bas, n'en ont.
 * }
 */
TEST(SceneLightingTest, UneCarteReleveSesLumieresEtSesBoitesDOmbre) {
    const PlaceFiles place;
    const hmi::PlaceAppearance appearance = place.appearance();
    ASSERT_NE(appearance.pieceManifest(), nullptr);
    const hmi::WorldSceneSnapshot snapshot = hmi::snapshotWorldScene(litMap(), appearance, {});

    ASSERT_EQ(snapshot.lights.size(), 3U);
    const core::LightSource& lamppost = snapshot.lights[0];
    EXPECT_FLOAT_EQ(lamppost.column, 1.5F);
    EXPECT_FLOAT_EQ(lamppost.row, 1.5F);
    EXPECT_FLOAT_EQ(lamppost.emission.height, 3.0F);
    EXPECT_FLOAT_EQ(lamppost.emission.radius, 7.5F);
    EXPECT_FALSE(lamppost.emission.flicker);
    EXPECT_TRUE(snapshot.lights[1].emission.flicker) << "le brasero";
    EXPECT_FLOAT_EQ(snapshot.lights[1].column, 2.5F);
    EXPECT_TRUE(snapshot.lights[2].emission.always) << "l'entité";
    EXPECT_FLOAT_EQ(snapshot.lights[2].column, 3.5F);
    EXPECT_FLOAT_EQ(snapshot.lights[2].row, 0.5F);

    ASSERT_EQ(snapshot.glows.size(), 1U);
    EXPECT_FLOAT_EQ(snapshot.glows.at("brazier"), 0.8F);

    ASSERT_EQ(snapshot.shadowBoxes.size(), 4U);
    // Un mètre de hauteur : 256 px de losange sur la diagonale d'une case, sous le cosinus de
    // l'élévation de la caméra — 94,7 px.
    const float ratio = snapshot.diamondRatio;
    const float pixelsPerMetre =
        256.0F / (core::METERS_PER_TILE * std::sqrt(2.0F)) * std::sqrt(1.0F - (ratio * ratio));
    const hmi::WorldShadowBox& wall = snapshot.shadowBoxes[0];
    EXPECT_FLOAT_EQ(wall.column, 0.0F);
    EXPECT_FLOAT_EQ(wall.columns, 1.0F);
    EXPECT_FLOAT_EQ(wall.base, 0.0F);
    EXPECT_NEAR(wall.height, 227.0F / pixelsPerMetre, 1e-3F);
    EXPECT_NEAR(wall.height, 2.4F, 0.05F);
    EXPECT_FLOAT_EQ(wall.top, 1.0F) << "un mur monte droit";

    const hmi::WorldShadowBox& upper = snapshot.shadowBoxes[1];
    EXPECT_FLOAT_EQ(upper.column, 0.0F);
    EXPECT_FLOAT_EQ(upper.base, 2.4F) << "le mur d'étage part d'un étage";

    const hmi::WorldShadowBox& pole = snapshot.shadowBoxes[2];
    EXPECT_NEAR(pole.columns, 0.25F, 1e-4F) << "64 px sur un losange de 256";
    EXPECT_NEAR(pole.column, 1.375F, 1e-4F) << "centrée dans sa case";
    EXPECT_NEAR(pole.height, 3.0F, 0.05F);
    EXPECT_LT(pole.top, 1.0F) << "le mobilier se resserre";

    EXPECT_NEAR(snapshot.shadowBoxes[3].column, 2.25F, 1e-4F) << "le brasero";
}

/**
 * @brief Chaque primitive dit ce qu'elle reçoit de la lumière : le sol prend la lumière et les
 *        ombres, une pièce dressée la lumière seule, un effet garde son éclat, une marque ne
 *        reçoit rien.
 * \castest{<b>Chaque primitive dit ce qu'elle recoit de la lumiere.</b><br/>
 * \tcat Unitaire · Rendu d'un lieu · Lumière<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Composer un sol, une pièce dressée, une pièce dressée d'éclat 0,8, un effet (éclat
 *             1), une marque d'interface, une face de bloc de maquette élevée et son losange à
 *             plat.<br/>
 *          2. Lire ce que chacune reçoit (`ComposedQuad::shading`).<br/>
 * \tattendu Le sol : toute la lumière, et les ombres. La pièce dressée : toute la lumière, sans
 *           ombres. L'éclat 0,8 : un cinquième de la lumière. L'effet et la marque : rien, ils
 *           gardent leur éclat. La face élevée : la lumière, sans l'ombre de sa propre boîte ;
 *           le losange à plat : les ombres.
 * }
 */
TEST(SceneLightingTest, ChaquePrimitiveDitCeQuElleRecoit) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    const auto texture = reinterpret_cast<hmi::TextureHandle>(std::uintptr_t{1});
    hmi::ComposedScene scene;
    const hmi::SpriteQuad quad{.x = 0.0F, .y = 0.0F, .width = 4.0F, .height = 4.0F};
    ASSERT_TRUE(scene.addSprite(hmi::RenderLayer::Tile, texture, 0, quad));
    ASSERT_TRUE(scene.addSprite(hmi::RenderLayer::Object, texture, 1, quad));
    ASSERT_TRUE(
        scene.addSprite(hmi::RenderLayer::Object, texture, 2, quad, 0, {}, std::nullopt, 0.8F));
    ASSERT_TRUE(
        scene.addSprite(hmi::RenderLayer::Player, texture, 3, quad, 0, {}, std::nullopt, 1.0F));
    ASSERT_TRUE(scene.addSprite(hmi::RenderLayer::UI, texture, 4, quad));
    hmi::PolyQuad raised;
    raised.x = {0.0F, 4.0F, 4.0F, 0.0F};
    raised.y = {0.0F, 0.0F, 4.0F, 4.0F};
    raised.rise = {2.0F, 2.0F, 0.0F, 0.0F};
    ASSERT_TRUE(scene.addPoly(hmi::RenderLayer::Tile, texture, 5, raised));
    hmi::PolyQuad flat = raised;
    flat.rise = {};
    ASSERT_TRUE(scene.addPoly(hmi::RenderLayer::Tile, texture, 6, flat));

    const std::vector<hmi::ComposedQuad>& quads = scene.quads();
    ASSERT_EQ(quads.size(), 7U);
    EXPECT_EQ(quads[0].shading(), (hmi::SpriteShading{.lit = 1.0F, .shadowed = true}));
    EXPECT_EQ(quads[1].shading(), (hmi::SpriteShading{.lit = 1.0F, .shadowed = false}));
    EXPECT_NEAR(quads[2].shading().lit, 0.2F, 1e-5F);
    EXPECT_FALSE(quads[2].shading().shadowed);
    EXPECT_FLOAT_EQ(quads[3].shading().lit, 0.0F);
    EXPECT_EQ(quads[4].shading(), hmi::SpriteShading{});
    EXPECT_EQ(quads[5].shading(), (hmi::SpriteShading{.lit = 1.0F, .shadowed = false}));
    EXPECT_EQ(quads[6].shading(), (hmi::SpriteShading{.lit = 1.0F, .shadowed = true}));
}
