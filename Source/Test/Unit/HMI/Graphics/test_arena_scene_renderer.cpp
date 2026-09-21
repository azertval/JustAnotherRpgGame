// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_arena_scene_renderer.cpp
 * @brief Le rendu QRhi de la scène du Colisée (`LOT-86` Phase 5), **hors écran** : cycle de vie
 *        des ressources et une vraie image de la scène composée.
 *
 * `hmi::ArenaViewportItem` n'est qu'un hôte Qt Quick ; ce qui peut fuir ou planter — créer,
 * libérer, recréer sur une autre interface QRhi — vit dans `hmi::ArenaSceneRenderer`, que ce test
 * fait tourner sur un `QRhi` Direct3D 11 sans fenêtre, avec les pièces **livrées** de la planche.
 * Se saute proprement si la machine n'offre aucune interface QRhi (`EX-NFR-004`).
 */

#include <QImage>
#include <QString>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <gtest/gtest.h>
#include <rhi/qrhi.h>

#include "Core/Combat/Arena.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileMap.h"
#include "Core/World/CombatZone.h"
#include "HMI/Graphics/ArenaSceneComposer.h"
#include "HMI/Graphics/ArenaSceneRenderer.h"

namespace {

using core::CombatSide;

/// Côté, en pixels, de la cible de rendu.
constexpr int TARGET_SIZE = 256;

/// Fond franc, qu'aucune pièce de la planche ne reproduit à l'identique.
constexpr float CLEAR[4] = {1.0f, 0.0f, 1.0f, 1.0f};

/// Le kit d'arene de la racine d'essai : manifeste, figurines, et le lieu dont il tire ses pieces.
std::filesystem::path kit() {
    return std::filesystem::path(JADG_TEST_DATA_DIR) / "Assets" / "Arena";
}

std::unique_ptr<QRhi> createOffscreenRhi() {
#ifdef Q_OS_WIN
    QRhiD3D11InitParams params;
    if (QRhi* const rhi = QRhi::create(QRhi::D3D11, &params)) {
        return std::unique_ptr<QRhi>(rhi);
    }
#endif
    return nullptr;
}

/// Une cible de rendu relisible, et ce qui la tient.
struct OffscreenTarget {
    std::unique_ptr<QRhiTexture> texture;
    std::unique_ptr<QRhiTextureRenderTarget> renderTarget;
    std::unique_ptr<QRhiRenderPassDescriptor> pass;

    explicit OffscreenTarget(QRhi& rhi, QSize size = QSize(TARGET_SIZE, TARGET_SIZE))
        : texture(rhi.newTexture(QRhiTexture::RGBA8, size, 1,
                                 QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource)) {
        EXPECT_TRUE(texture->create());
        renderTarget.reset(rhi.newTextureRenderTarget({{texture.get()}}));
        pass.reset(renderTarget->newCompatibleRenderPassDescriptor());
        renderTarget->setRenderPassDescriptor(pass.get());
        EXPECT_TRUE(renderTarget->create());
    }
};

/// Dessine une image de @p renderer dans @p target, et la relit.
QImage renderFrame(QRhi& rhi, hmi::ArenaSceneRenderer& renderer, OffscreenTarget& target) {
    QRhiCommandBuffer* commandBuffer = nullptr;
    if (rhi.beginOffscreenFrame(&commandBuffer) != QRhi::FrameOpSuccess) {
        ADD_FAILURE() << "beginOffscreenFrame";
        return {};
    }
    renderer.render(commandBuffer, target.renderTarget.get(), 1.0f / 60.0f, CLEAR);

    QRhiReadbackResult readback;
    QRhiResourceUpdateBatch* const readbackBatch = rhi.nextResourceUpdateBatch();
    readbackBatch->readBackTexture({target.texture.get()}, &readback);
    commandBuffer->resourceUpdate(readbackBatch);
    if (rhi.endOffscreenFrame() != QRhi::FrameOpSuccess) {
        ADD_FAILURE() << "endOffscreenFrame";
        return {};
    }
    return QImage(reinterpret_cast<const uchar*>(readback.data.constData()),
                  readback.pixelSize.width(), readback.pixelSize.height(), QImage::Format_RGBA8888)
        .copy();
}

/// Nombre de pixels qui ne sont pas le fond d'effacement.
std::size_t paintedPixels(const QImage& image) {
    std::size_t painted = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (pixel.red() != 255 || pixel.green() != 0 || pixel.blue() != 255) {
                ++painted;
            }
        }
    }
    return painted;
}

/// La piste 5x4 ceinte de murs du test du composeur, une porte en (0, 2).
core::Level piste() {
    core::TileMap carte(5, 4);
    for (int x = 0; x < 5; ++x) {
        carte.setTile(x, 0, core::TileType::Wall);
        carte.setTile(x, 3, core::TileType::Wall);
    }
    for (int y = 0; y < 4; ++y) {
        carte.setTile(0, y, core::TileType::Wall);
        carte.setTile(4, y, core::TileType::Wall);
    }
    carte.setTile(0, 2, core::TileType::Empty);
    return core::Level(core::LevelData{
        .name = "piste", .tileMap = std::move(carte), .entities = {}, .entry = {1, 1}});
}

core::ArenaContestant concurrent(const std::string& nom, CombatSide camp, int colonne, int ligne) {
    const core::CombatantProfile profil{
        .name = nom, .side = camp, .maximumHitPoints = 10, .currentHitPoints = 10, .movement = 6};
    return {.profile = profil,
            .attacks = {},
            .position = core::GridPosition{.column = colonne, .row = ligne},
            .markId = {}};
}

/// L'instantané de la piste, deux allies et deux ennemis montés. La session meurt ici : le rendu
/// ne dessine que l'instantané.
hmi::ArenaSceneSnapshot snapshotDePiste() {
    core::ArenaSession session{piste()};
    core::ArenaBout bout{.seed = 7, .lethal = false, .heroicMark = false};
    bout.contestants.push_back(concurrent("Bram", CombatSide::Allies, 1, 1));
    bout.contestants.push_back(concurrent("Eve", CombatSide::Allies, 3, 1));
    bout.contestants.push_back(concurrent("Orc", CombatSide::Enemies, 1, 2));
    bout.contestants.push_back(concurrent("Rat", CombatSide::Enemies, 2, 2));
    const core::ArenaMount mount = session.mount(bout);
    EXPECT_TRUE(mount.refusals.empty());
    return hmi::snapshotArenaScene(session);
}

/// Sols (20), enceinte (14 : une piece par case de mur, et l'arche) et figurines (4) de la piste.
constexpr std::size_t PISTE_QUADS = 20 + 14 + 4;

}  // namespace

/**
 * @brief Créer, libérer deux fois, recréer : les ressources suivent, sans fuite de lot ni plantage.
 * \castest{<b>Le cycle de vie des ressources QRhi de l'arene est sur.</b><br/>
 * \tcat Unitaire · Rendu QRhi de l'arene<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Creer les ressources sur une interface QRhi hors ecran, sans jamais dessiner.<br/>
 *          2. Liberer, puis liberer encore.<br/>
 *          3. Recreer, dessiner une image, detruire le rendu avant l'interface.<br/>
 * \tattendu Toutes les pieces livrees sont chargees et le damier existe ; apres liberation plus
 *           rien n'est cree ; la recreation dessine une image valide.
 * }
 */
TEST(ArenaSceneRendererTest, CreationLiberationRecreation) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    OffscreenTarget target(*rhi);
    {
        hmi::ArenaSceneRenderer renderer(kit());
        ASSERT_FALSE(renderer.catalog().heroes().empty()) << "manifeste du kit illisible";
        EXPECT_FALSE(renderer.ensureResources(nullptr));
        EXPECT_FALSE(renderer.created());

        ASSERT_TRUE(renderer.ensureResources(rhi.get()));
        EXPECT_TRUE(renderer.created());
        EXPECT_EQ(renderer.rhi(), rhi.get());
        EXPECT_EQ(renderer.textures().byPath.size(),
                  hmi::arenaTexturePaths(renderer.catalog()).size())
            << "une piece du kit n'a pas pu etre chargee";
        EXPECT_NE(renderer.textures().missing.texture, nullptr);
        // Idempotent sur la meme interface.
        const hmi::TextureHandle sand =
            renderer.textures().resolve("../Scene/bourg/sand.png").texture;
        EXPECT_TRUE(renderer.ensureResources(rhi.get()));
        EXPECT_EQ(renderer.textures().resolve("../Scene/bourg/sand.png").texture, sand);

        // Liberee sans avoir jamais dessine : le lot de creation doit etre rendu, pas perdu.
        renderer.release();
        EXPECT_FALSE(renderer.created());
        EXPECT_EQ(renderer.rhi(), nullptr);
        EXPECT_TRUE(renderer.textures().byPath.empty());
        renderer.release();

        ASSERT_TRUE(renderer.ensureResources(rhi.get()));
        renderer.setSnapshot(snapshotDePiste());
        const QImage image = renderFrame(*rhi, renderer, target);
        EXPECT_EQ(image.size(), QSize(TARGET_SIZE, TARGET_SIZE));
    }
    // Le rendu est detruit ici, l'interface encore vivante : c'est l'ordre de Qt Quick.
}

/**
 * @brief Une grille de test donne une scène composée complète, et une image qui n'est pas que le
 *        fond.
 * \castest{<b>La scene de l'arene devient des pixels.</b><br/>
 * \tcat Unitaire · Rendu QRhi de l'arene<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Tirer l'instantane de la piste 5x4 (quatre combattants), puis detruire la
 *             session.<br/>2. Dessiner une image hors ecran et la relire.<br/>
 * \tattendu 38 quads composes, tous sur une piece chargee (aucun damier) ; une part notable de
 *           l'image est peinte, et le fond subsiste dans les coins.
 * }
 */
TEST(ArenaSceneRendererTest, SceneNonVideSurUneGrilleDeTest) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    OffscreenTarget target(*rhi);
    hmi::ArenaSceneRenderer renderer(kit());
    ASSERT_TRUE(renderer.ensureResources(rhi.get()));

    renderer.setSnapshot(snapshotDePiste());
    EXPECT_TRUE(renderer.animating());
    const QImage image = renderFrame(*rhi, renderer, target);
    ASSERT_EQ(image.size(), QSize(TARGET_SIZE, TARGET_SIZE));

    ASSERT_EQ(renderer.composed().size(), PISTE_QUADS);
    for (const hmi::ComposedQuad& quad : renderer.composed().quads()) {
        EXPECT_NE(quad.texture, nullptr);
        EXPECT_NE(quad.texture, renderer.textures().missing.texture)
            << "piece tombee sur le damier";
    }

    const std::size_t painted = paintedPixels(image);
    EXPECT_GT(painted, static_cast<std::size_t>(TARGET_SIZE * TARGET_SIZE / 5))
        << "la scene cadree devrait couvrir une part notable de la cible";
    EXPECT_LT(painted, static_cast<std::size_t>(TARGET_SIZE * TARGET_SIZE))
        << "le losange de la scene laisse le fond visible dans les coins";

    // Une scene vide ne dessine que le fond, et n'appelle plus d'image.
    renderer.setSnapshot({});
    EXPECT_FALSE(renderer.animating());
    const QImage empty = renderFrame(*rhi, renderer, target);
    EXPECT_EQ(renderer.composed().size(), 0U);
    EXPECT_EQ(paintedPixels(empty), 0U);
}

/**
 * @brief L'interface QRhi change (fenêtre changée) : tout est libéré puis recréé sur la nouvelle,
 * et le dessin reprend.
 * \castest{<b>Le rendu de l'arene se recree sur une nouvelle interface QRhi.</b><br/>
 * \tcat Unitaire · Rendu QRhi de l'arene<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Creer et dessiner sur une premiere interface.<br/>2. Appeler ensureResources avec une
 *          seconde interface, puis dessiner dessus.<br/>
 * \tattendu Le rendu designe la seconde interface, ses textures sont neuves, et l'image est peinte.
 * }
 */
TEST(ArenaSceneRendererTest, RecreationSurUneAutreInterface) {
    const std::unique_ptr<QRhi> first = createOffscreenRhi();
    const std::unique_ptr<QRhi> second = createOffscreenRhi();
    if (!first || !second) {
        GTEST_SKIP() << "Deux interfaces QRhi hors ecran indisponibles sur cette machine.";
    }
    OffscreenTarget firstTarget(*first);
    OffscreenTarget secondTarget(*second);

    hmi::ArenaSceneRenderer renderer(kit());
    renderer.setSnapshot(snapshotDePiste());
    ASSERT_TRUE(renderer.ensureResources(first.get()));
    EXPECT_GT(paintedPixels(renderFrame(*first, renderer, firstTarget)), 0U);

    ASSERT_TRUE(renderer.ensureResources(second.get()));
    EXPECT_EQ(renderer.rhi(), second.get());
    EXPECT_EQ(renderer.textures().byPath.size(), hmi::arenaTexturePaths(renderer.catalog()).size());
    EXPECT_GT(paintedPixels(renderFrame(*second, renderer, secondTarget)), 0U);
    EXPECT_EQ(renderer.composed().size(), PISTE_QUADS);
}

/**
 * @brief Le cadrage du rendu ramène chaque case à elle-même : le pointeur tombe sur la case
 * dessinée.
 * \castest{<b>Le centre de chaque case, projete a l'ecran par le cadrage du rendu, redevient la
 * meme case.</b><br/>
 * \tcat Unitaire · Rendu QRhi de l'arene<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Pour plusieurs grilles et surfaces (zoom entier et zoom inferieur a 1), cadrer par
 *          arenaCamera.<br/>2. Projeter le centre de chaque case a l'ecran, puis revenir au monde
 *          et a la case.<br/>3. Revenir d'un coin de la surface, hors de la scene.<br/>
 * \tattendu Chaque centre redevient sa case, dans la surface ; le coin n'est aucune case.
 * }
 */
TEST(ArenaSceneRendererTest, LeCadrageRameneChaqueCaseAElleMeme) {
    struct Cas {
        int columns;
        int rows;
        int width;
        int height;
    };
    for (const Cas cas : {Cas{12, 9, 1280, 720}, Cas{12, 9, 200, 150}, Cas{5, 14, 777, 1003}}) {
        const core::IsoProjection projection(cas.columns, cas.rows);
        const hmi::Camera2D camera = hmi::arenaCamera(projection, cas.width, cas.height);
        for (int row = 0; row < cas.rows; ++row) {
            for (int column = 0; column < cas.columns; ++column) {
                const core::Vector2 screen =
                    camera.worldToScreen(projection.tileToWorld({.column = column, .row = row}));
                EXPECT_GE(screen.x, 0.0f);
                EXPECT_LE(screen.x, static_cast<float>(cas.width));
                EXPECT_GE(screen.y, 0.0f);
                EXPECT_LE(screen.y, static_cast<float>(cas.height));
                const std::optional<core::GridPosition> cell =
                    projection.worldToTile(camera.screenToWorld(screen));
                ASSERT_TRUE(cell.has_value()) << column << ", " << row;
                EXPECT_EQ(cell->column, column);
                EXPECT_EQ(cell->row, row);
            }
        }
        EXPECT_FALSE(projection.worldToTile(camera.screenToWorld({0.0f, 0.0f})).has_value());
    }
}

/**
 * @brief Outil de revue, pas un contrôle : une arène de 20 × 14 cases ceinte de murs, rendue hors
 *        écran et enregistrée sous le chemin de `JADG_ARENA_CAPTURE` (sauté sans la variable).
 * \castest{<b>Capture de l'arène pour relecture à l'œil (LOT-92).</b><br/>
 * \tcat Unitaire · Rendu QRhi de l'arene<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Définir JADG_ARENA_CAPTURE, lancer ce test.<br/>2. Ouvrir l'image.<br/>
 * \tattendu Une image 1600 × 1000 est écrite : sable, enceinte, portes, quatre figurines.
 * }
 */
TEST(ArenaSceneRendererTest, CaptureDeLArenePourRelecture) {
    const QString destination = qEnvironmentVariable("JADG_ARENA_CAPTURE");
    if (destination.isEmpty()) {
        GTEST_SKIP() << "JADG_ARENA_CAPTURE non définie : aucune capture demandée.";
    }
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    OffscreenTarget target(*rhi, QSize(1600, 1000));
    hmi::ArenaSceneRenderer renderer(kit());
    ASSERT_TRUE(renderer.ensureResources(rhi.get()));

    // Une arène de 20 × 14, l'enceinte au bord, une porte au milieu de chaque côté.
    constexpr int columns = 20;
    constexpr int rows = 14;
    hmi::ArenaSceneSnapshot snapshot;
    snapshot.columns = columns;
    snapshot.rows = rows;
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const bool border = row == 0 || column == 0 || row == rows - 1 || column == columns - 1;
            const bool gate = (column == 0 || column == columns - 1) && row == rows / 2;
            snapshot.obstructed.push_back(border && !gate);
        }
    }
    const auto figure = [](std::uint32_t id, const char* name, CombatSide side, int column,
                           int row) {
        return hmi::ArenaFigureSnapshot{.id = core::CombatantId{id},
                                        .name = name,
                                        .side = side,
                                        .down = false,
                                        .anchor = {.column = column, .row = row},
                                        .footprint = 1};
    };
    snapshot.figures = {
        figure(1, "Bram", CombatSide::Allies, 8, 6), figure(2, "Eve", CombatSide::Allies, 9, 8),
        figure(3, "Orc", CombatSide::Enemies, 11, 6), figure(4, "Rat", CombatSide::Enemies, 12, 7)};
    renderer.setSnapshot(std::move(snapshot));

    const QImage image = renderFrame(*rhi, renderer, target);
    ASSERT_FALSE(image.isNull());
    EXPECT_TRUE(image.save(destination)) << destination.toStdString();
}

/**
 * @brief Le combat utilise le décor et les départs de la nouvelle arène.
 * \castest{<b>Le combat utilise le décor et les départs de la nouvelle arène.</b><br/>
 * \tcat Unitaire · Rendu du Colisée<br/>
 * \tcrit Critique<br/>
 * \tetapes Charger la zone sable, monter deux concurrents et rendre la scène.<br/>
 * \tattendu Zone 20 × 14, départs conservés et décor du nouveau kit sans texture manquante.
 * }
 */
TEST(ArenaSceneRendererTest, LeCombatUtiliseLeDecorEtLesDepartsDeLaZone) {
    const auto loaded = core::LevelLoader::loadFromFile(std::filesystem::path(JADG_TEST_DATA_DIR) /
                                                        "Levels" / "donjon.json");
    ASSERT_TRUE(loaded.ok()) << loaded.error;
    const auto zones = core::combatZonesOf(*loaded.level);
    const auto* zone = core::findCombatZone(zones, "salle");
    ASSERT_NE(zone, nullptr);
    ASSERT_EQ(zone->columns, 20);
    ASSERT_EQ(zone->rows, 14);
    EXPECT_TRUE(core::validateCombatZones("donjon", *loaded.level).empty());
    core::ArenaSession session(core::cropLevelToZone(*loaded.level, *zone));
    core::ArenaBout bout{.seed = 7, .lethal = false, .heroicMark = false};
    auto ally = concurrent("Bram", CombatSide::Allies, 1, 5);
    auto enemy = concurrent("Orc", CombatSide::Enemies, 18, 5);
    ally.position.reset();
    enemy.position.reset();
    bout.contestants = {ally, enemy};
    const auto mounted = session.mount(bout);
    ASSERT_TRUE(mounted.refusals.empty());
    const auto snapshot = hmi::snapshotArenaScene(session);
    ASSERT_EQ(snapshot.figures.size(), 2U);
    // Les entrees de la salle, ramenees au repere de la zone decoupee (origine 10, 10).
    EXPECT_EQ(snapshot.figures[0].anchor.column, 1);
    EXPECT_EQ(snapshot.figures[1].anchor.column, 18);
    const auto rhi = createOffscreenRhi();
    ASSERT_NE(rhi, nullptr);
    hmi::ArenaSceneRenderer renderer(kit(), true);
    renderer.setSnapshot(snapshot);
    ASSERT_TRUE(renderer.ensureResources(rhi.get()));
    OffscreenTarget target(*rhi, QSize(1600, 1000));
    const auto image = renderFrame(*rhi, renderer, target);
    // Le decor derriere la zone est celui de la carte : bien plus de quads que les 280 cases de
    // la zone, et pas un seul damier.
    EXPECT_GT(renderer.composed().quads().size(), 280U);
    for (const auto& quad : renderer.composed().quads()) {
        EXPECT_NE(quad.texture, renderer.textures().missing.texture);
    }
    EXPECT_TRUE(image.save("salle-combat.png"));
    EXPECT_TRUE(session.start());
}
