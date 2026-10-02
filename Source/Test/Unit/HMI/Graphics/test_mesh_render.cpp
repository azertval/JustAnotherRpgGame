// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_mesh_render.cpp
 * @brief La passe de maillages et le tampon de profondeur, sur la carte d'essai (`LOT-1003`).
 *
 * La carte d'essai (`Fixtures/Meshes`) — une cour dallée, un îlot de murs en anneau, un toit à
 * l'étage, le tout en maillages au milieu d'un sol en images — rendue hors écran par le rendu du
 * jeu, sur un `QRhi` Direct3D 11 sans fenêtre. La figurine témoin porte une teinte que rien
 * d'autre n'a : ce que la profondeur en laisse voir se compte, pixel par pixel.
 */

#include <QColor>
#include <QImage>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "HMI/Graphics/OffscreenRender.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/PlaceCamera.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/Graphics/WorldSceneRenderer.h"

namespace {

const QColor BACKGROUND(24, 26, 30);
const QSize SIZE(640, 480);

std::filesystem::path assets() {
    return std::filesystem::path(JADG_MESH_FIXTURE_DIR) / "Assets";
}

/// La carte d'essai, avec les figurines @p figures et sans celles de ses PNJ.
hmi::WorldSceneSnapshot ilot(std::vector<hmi::WorldFigureSnapshot> figures = {}) {
    const core::LevelLoadResult map = core::LevelLoader::loadFromFile(
        std::filesystem::path(JADG_MESH_FIXTURE_DIR) / "Levels" / "ilot.json");
    EXPECT_TRUE(map.ok()) << map.error;
    const hmi::PlaceAppearanceResult appearance =
        hmi::PlaceAppearance::loadForPlace(assets(), "ilot");
    EXPECT_TRUE(appearance.ok()) << appearance.message;
    if (!map.ok() || !appearance.ok()) {
        return {};
    }
    return hmi::snapshotWorldScene(*map.level, appearance.appearance, std::move(figures));
}

core::IsoProjection projectionOf(const hmi::WorldSceneSnapshot& snapshot) {
    return {snapshot.columns, snapshot.rows, core::ARENA_TILE_WIDTH_UNITS, snapshot.diamondRatio};
}

/// Le cadrage qui centre l'îlot, une case à 64 pixels : l'art du lieu d'essai à sa taille.
hmi::WorldFraming framing(const hmi::WorldSceneSnapshot& snapshot) {
    const core::IsoProjection projection = projectionOf(snapshot);
    return hmi::WorldFraming{.center = projection.gridToWorld({5.5F, 3.5F}),
                             .pixelsPerUnit = 64.0F / projection.tileWidth()};
}

/// La figurine témoin, au centre de la case (@p column, @p row).
hmi::WorldFigureSnapshot witness(int column, int row) {
    return hmi::WorldFigureSnapshot{
        .figure = "Npc/temoin",
        .point = {static_cast<float>(column) + 0.5F, static_cast<float>(row) + 0.5F}};
}

/// Le nombre de pixels de @p image que @p matches reconnaît.
template <class Predicate>
std::size_t countPixels(const QImage& image, Predicate matches) {
    std::size_t count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            count += matches(image.pixelColor(x, y)) ? 1U : 0U;
        }
    }
    return count;
}

/// L'eau sourde du corps de la figurine témoin : plus de vert et de bleu que de rouge.
bool isWitness(const QColor& color) {
    return color.green() > color.red() + 40 && color.blue() > color.red() + 40;
}

/// Le bourgogne des tuiles du toit.
bool isRoof(const QColor& color) {
    return color.red() > 100 && color.green() < 70 && color.blue() < 90;
}

/// L'ivoire de la pierre des murs.
bool isWall(const QColor& color) {
    return color.red() > 205 && color.green() > 195 && color.blue() > 175;
}

void capture(const QImage& image, const char* name) {
    const std::filesystem::path captures(JADG_RENDER_CAPTURES_DIR);
    std::filesystem::create_directories(captures);
    EXPECT_TRUE(image.save(QString::fromStdWString((captures / name).wstring())));
}

}  // namespace

/**
 * @brief La carte d'essai se rend en volumes : ses trois maillages se chargent et se dessinent,
 *        au milieu d'un sol en images.
 * \castest{<b>La carte d'essai se rend en maillages.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Maillages<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Donner la carte d'essai au rendu du jeu, hors écran.<br/>
 *          2. La rendre, cadrée sur l'îlot.<br/>
 * \tattendu Les trois fichiers `.glb` sont chargés, aucun n'est demandé comme texture ; l'image
 *           composée a trente-quatre maillages ; elle montre la pierre ivoire des murs et les
 *           tuiles bourgogne du toit, et le sol en images autour. La capture est écrite pour l'œil.
 * }
 */
TEST(MeshRenderTest, LaCarteDEssaiSeRendEnMaillages) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneRenderer renderer(assets());
    const hmi::WorldSceneSnapshot snapshot = ilot({witness(5, 6), witness(2, 3)});
    renderer.setSnapshot(snapshot);
    const QImage image = offscreen->render(renderer, SIZE, framing(snapshot), BACKGROUND);
    ASSERT_EQ(image.size(), SIZE);
    capture(image, "ilot-maillages.png");

    EXPECT_EQ(renderer.textures().meshes.size(), 3U);
    for (const auto& [path, texture] : renderer.textures().byPath) {
        EXPECT_FALSE(path.ends_with(".glb")) << path;
    }
    EXPECT_EQ(renderer.composed().meshes().size(), 34U);
    EXPECT_GT(renderer.textureBytes(), 0U);

    EXPECT_GT(countPixels(image, isRoof), 3000U) << "le toit, à l'étage, couvre l'îlot";
    EXPECT_GT(countPixels(image, isWall), 3000U) << "les flancs des murs se voient sous le toit";
    EXPECT_GT(countPixels(image, isWitness), 300U) << "les figurines se dressent devant";
    // Le fond ne se voit qu'autour de la carte, dix cases sur huit au milieu de l'image : elle est
    // peinte.
    EXPECT_LT(countPixels(image, [](const QColor& color) { return color == BACKGROUND; }),
              static_cast<std::size_t>(SIZE.width() * SIZE.height() * 3 / 4));
}

/**
 * @brief Une figurine passe devant puis derrière un mur en maillage, et c'est le tampon de
 *        profondeur qui décide : elle est soumise les deux fois, dans le même ordre.
 * \castest{<b>Une figurine passe devant puis derriere un mur en maillage.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Maillages<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Rendre la carte d'essai, la figurine témoin devant l'îlot, case (5, 5).<br/>
 *          2. La rendre, la figurine derrière l'îlot, case (5, 1).<br/>
 *          3. Rendre de nouveau la figurine derrière, les maillages éteints.<br/>
 * \tattendu Devant, la figurine se voit entière. Derrière, elle est soumise au dessin comme
 *           devant — aucune liste triée ne contient les murs — et moins d'un dixième d'elle
 *           paraît : les murs l'ont masquée par la profondeur. Les maillages éteints, elle se
 *           voit de nouveau entière au même endroit.
 * }
 */
TEST(MeshRenderTest, UneFigurinePasseDevantPuisDerriereUnMur) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneRenderer renderer(assets());
    const auto figureQuads = [&renderer] {
        std::size_t count = 0;
        for (const hmi::ComposedQuad& quad : renderer.composed().quads()) {
            count += quad.layer == hmi::RenderLayer::Player ? 1U : 0U;
        }
        return count;
    };

    const hmi::WorldSceneSnapshot inFront = ilot({witness(5, 5)});
    renderer.setSnapshot(inFront);
    const QImage front = offscreen->render(renderer, SIZE, framing(inFront), BACKGROUND);
    ASSERT_FALSE(front.isNull());
    const std::size_t visibleInFront = countPixels(front, isWitness);
    EXPECT_EQ(figureQuads(), 1U);
    capture(front, "ilot-figurine-devant.png");

    const hmi::WorldSceneSnapshot behindWalls = ilot({witness(5, 1)});
    renderer.setSnapshot(behindWalls);
    const QImage behind = offscreen->render(renderer, SIZE, framing(behindWalls), BACKGROUND);
    ASSERT_FALSE(behind.isNull());
    const std::size_t visibleBehind = countPixels(behind, isWitness);
    EXPECT_EQ(figureQuads(), 1U) << "la figurine est soumise : seul le tampon la masque";
    EXPECT_FALSE(renderer.composed().meshes().empty());
    capture(behind, "ilot-figurine-derriere.png");

    // Les maillages eteints : plus rien n'ecrit la profondeur, la figurine reparait.
    renderer.setQuadOpacity([](const hmi::ComposedQuad& quad) {
        return quad.layer == hmi::RenderLayer::Object ? 0.0F : 1.0F;
    });
    const QImage alone = offscreen->render(renderer, SIZE, framing(behindWalls), BACKGROUND);
    const std::size_t visibleAlone = countPixels(alone, isWitness);
    renderer.setQuadOpacity({});

    std::cout << "figurine temoin : " << visibleInFront << " pixels devant, " << visibleBehind
              << " derriere, " << visibleAlone << " derriere sans les murs\n";
    EXPECT_GT(visibleInFront, 250U);
    EXPECT_LT(visibleBehind, visibleInFront / 10);
    EXPECT_GT(visibleAlone, (visibleInFront * 9) / 10);
    EXPECT_LT(visibleAlone, (visibleInFront * 11) / 10);
}

/**
 * @brief L'opacité des calques de l'éditeur vaut pour un mur en volume comme pour un mur peint.
 * \castest{<b>L'opacite des calques vaut pour les maillages.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Maillages<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Rendre la carte d'essai sans réglage.<br/>
 *          2. La rendre le relief éteint, puis l'étage seul éteint.<br/>
 * \tattendu Le relief éteint, ni mur ni toit, et la cour dallée se voit à leur place ; l'étage
 *           éteint, le toit a disparu et les murs restent. Le réglage retiré, l'image d'origine
 *           revient au pixel.
 * }
 */
TEST(MeshRenderTest, LOpaciteDesCalquesVautPourLesMaillages) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneRenderer renderer(assets());
    const hmi::WorldSceneSnapshot snapshot = ilot();
    renderer.setSnapshot(snapshot);
    const hmi::WorldFraming frame = framing(snapshot);
    const QImage plain = offscreen->render(renderer, SIZE, frame, BACKGROUND);
    ASSERT_GT(countPixels(plain, isRoof), 3000U);

    renderer.setQuadOpacity([](const hmi::ComposedQuad& quad) {
        return quad.layer == hmi::RenderLayer::Object ? 0.0F : 1.0F;
    });
    const QImage flat = offscreen->render(renderer, SIZE, frame, BACKGROUND);
    EXPECT_EQ(countPixels(flat, isRoof), 0U);
    EXPECT_EQ(countPixels(flat, isWall), 0U);
    EXPECT_EQ(renderer.composed().meshes().size(), 25U) << "il ne reste que les dalles";

    renderer.setQuadOpacity(
        [](const hmi::ComposedQuad& quad) { return quad.storey > 0 ? 0.0F : 1.0F; });
    const QImage open = offscreen->render(renderer, SIZE, frame, BACKGROUND);
    EXPECT_EQ(countPixels(open, isRoof), 0U);
    EXPECT_GT(countPixels(open, isWall), 3000U);

    renderer.setQuadOpacity({});
    const QImage again = offscreen->render(renderer, SIZE, frame, BACKGROUND);
    EXPECT_EQ(countPixels(again, isRoof), countPixels(plain, isRoof));
    std::size_t differing = 0;
    for (int y = 0; y < plain.height(); ++y) {
        for (int x = 0; x < plain.width(); ++x) {
            differing += plain.pixel(x, y) != again.pixel(x, y) ? 1U : 0U;
        }
    }
    EXPECT_EQ(differing, 0U);
}

/**
 * @brief Une image plus grande qu'une tuile se rend par tuiles, maillages compris, sans couture.
 * \castest{<b>Une carte en maillages rendue par tuiles est la meme image.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Maillages<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Rendre la carte d'essai en 640 × 480 d'un coup.<br/>
 *          2. La rendre par tuiles de 256 pixels.<br/>
 * \tattendu Moins d'un pixel sur cent diffère : les arêtes des volumes tombent au même endroit
 *           d'une tuile à l'autre, et la profondeur y est la même.
 * }
 */
TEST(MeshRenderTest, UneCarteEnMaillagesRendueParTuilesEstLaMemeImage) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneRenderer renderer(assets());
    const hmi::WorldSceneSnapshot snapshot = ilot({witness(5, 5)});
    renderer.setSnapshot(snapshot);
    const hmi::WorldFraming frame = framing(snapshot);
    const QImage whole = offscreen->render(renderer, SIZE, frame, BACKGROUND);
    const QImage tiled = offscreen->render(renderer, SIZE, frame, BACKGROUND, 256);
    ASSERT_EQ(whole.size(), tiled.size());
    std::size_t differing = 0;
    for (int y = 0; y < whole.height(); ++y) {
        for (int x = 0; x < whole.width(); ++x) {
            differing += whole.pixel(x, y) != tiled.pixel(x, y) ? 1U : 0U;
        }
    }
    std::cout << differing << " pixels different entre l'image entiere et ses tuiles\n";
    EXPECT_LT(differing, static_cast<std::size_t>(SIZE.width() * SIZE.height() / 100));
}

/**
 * @brief Un maillage dont le fichier manque ne plante rien : sa pièce se voit sur le damier, et
 *        le fichier n'est demandé qu'une fois.
 * \castest{<b>Un fichier de maillage absent laisse voir le damier.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Maillages<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Donner au rendu la carte d'essai, le fichier du mur remplacé par un `.glb` qui
 *             n'existe pas.<br/>2. La rendre deux fois.<br/>
 * \tattendu Deux maillages chargés sur trois ; le fichier absent a été demandé, et ne l'est pas de
 *           nouveau ; l'image est peinte — la pièce manquante se voit.
 * }
 */
TEST(MeshRenderTest, UnFichierDeMaillageAbsentLaisseVoirLeDamier) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneSnapshot snapshot = ilot();
    ASSERT_TRUE(snapshot.pieceFiles.contains("wall"));
    snapshot.pieceFiles["wall"] = "Scene/ilot/absent.glb";

    hmi::WorldSceneRenderer renderer(assets());
    renderer.setSnapshot(snapshot);
    const QImage image = offscreen->render(renderer, SIZE, framing(snapshot), BACKGROUND);
    ASSERT_FALSE(image.isNull());
    EXPECT_EQ(renderer.textures().meshes.size(), 2U);
    EXPECT_TRUE(renderer.requested().contains("Scene/ilot/absent.glb"));
    EXPECT_EQ(countPixels(image, isWall), 0U);
    // Les huit murs sont des images sur le damier : des primitives du relief.
    std::size_t relief = 0;
    for (const hmi::ComposedQuad& quad : renderer.composed().quads()) {
        relief += quad.layer == hmi::RenderLayer::Object ? 1U : 0U;
    }
    EXPECT_EQ(relief, 8U);
    const QImage again = offscreen->render(renderer, SIZE, framing(snapshot), BACKGROUND);
    EXPECT_EQ(again.size(), image.size());
}

/**
 * @brief Le pointage d'une case au sol donne la même case qu'avant le lot, sur toute la carte
 *        d'Arenarea : la caméra du lieu cadre comme la caméra 2D qu'elle remplace.
 * \castest{<b>Le pointage d'une case au sol est celui d'avant, sur toute la carte
 * d'Arenarea.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Maillages<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire la carte d'Arenarea livrée.<br/>
 *          2. Pour chaque case, cadrer le jeu sur elle à 1080p, avec et sans étendue de
 *             profondeur.<br/>
 *          3. Envoyer à l'écran le centre et les quatre quarts de la case par la caméra, puis par
 *             la formule de la caméra 2D d'avant le lot ; revenir de l'écran à la case.<br/>
 * \tattendu Les positions à l'écran sont identiques au bit près à celles de la formule d'avant ;
 *           chaque point revient à sa case ; l'étendue de profondeur ne déplace rien.
 * }
 */
TEST(MeshRenderTest, LePointageDUneCaseEstCeluiDAvant) {
    const core::LevelLoadResult map = core::LevelLoader::loadFromFile(
        std::filesystem::path(JADG_LEVELS_DIR) / "central-empire" / "capital" / "arenarea.json");
    ASSERT_TRUE(map.ok()) << map.error;
    const core::IsoProjection projection(map.level->tileMap().width(),
                                         map.level->tileMap().height());
    const hmi::IsoView view(projection);
    constexpr int WIDTH = 1920;
    constexpr int HEIGHT = 1080;
    std::size_t checked = 0;
    for (int row = 0; row < projection.rows(); ++row) {
        for (int column = 0; column < projection.columns(); ++column) {
            const core::GridPosition cell{.column = column, .row = row};
            const hmi::PlaceCamera camera =
                hmi::worldCamera(projection, projection.tileToWorld(cell), WIDTH, HEIGHT);
            hmi::PlaceCamera deep = camera;
            deep.setDepthRange(view.depthRange());
            // La camera 2D d'avant le lot : (monde - centre) x echelle + demi-ecran.
            const float scale = hmi::PlaceCamera::PIXELS_PER_UNIT * camera.zoom();
            const auto before = [&](core::Vector2 world) {
                return core::Vector2{((world.x - camera.center().x) * scale) + (WIDTH * 0.5F),
                                     ((world.y - camera.center().y) * scale) + (HEIGHT * 0.5F)};
            };
            for (const core::Vector2 offset :
                 {core::Vector2{0.5F, 0.5F}, core::Vector2{0.25F, 0.5F}, core::Vector2{0.75F, 0.5F},
                  core::Vector2{0.5F, 0.25F}, core::Vector2{0.5F, 0.75F}}) {
                const core::Vector2 world = projection.gridToWorld(
                    {static_cast<float>(column) + offset.x, static_cast<float>(row) + offset.y});
                const core::Vector2 screen = camera.worldToScreen(world);
                ASSERT_EQ(screen.x, before(world).x);
                ASSERT_EQ(screen.y, before(world).y);
                ASSERT_EQ(deep.worldToScreen(world).x, screen.x);
                ASSERT_EQ(deep.worldToScreen(world).y, screen.y);
                const std::optional<core::GridPosition> picked =
                    projection.worldToTile(deep.screenToWorld(screen));
                ASSERT_TRUE(picked.has_value());
                ASSERT_EQ(*picked, cell);
                ++checked;
            }
        }
    }
    EXPECT_EQ(checked, static_cast<std::size_t>(projection.columns()) *
                           static_cast<std::size_t>(projection.rows()) * 5U);
}
