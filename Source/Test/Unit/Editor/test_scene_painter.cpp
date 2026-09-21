// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_scene_painter.cpp
 * @brief Le canevas de l'éditeur peint le lieu comme le jeu le dessine (`LOT-EDITOR-02`).
 *
 * Comparer les listes de primitives ne prouve pas que `QPainter` pose les ancres, l'échelle des
 * pièces et leurs miroirs comme le GPU (acceptation du lot) : ce test rend **la même carte,
 * avec la même caméra**, une fois par le rendu QRhi du jeu (`hmi::WorldSceneRenderer`, hors
 * écran), une fois par le peintre de l'éditeur (`hmi::renderComposedScene`), et compare les deux
 * images pixel à pixel, à une tolérance près. Les deux images sont écrites à côté de l'exécutable
 * (`editor-captures/`) pour être relues à l'œil.
 *
 * La tolérance est celle de deux rasteriseurs : sur l'arête d'un quad étiré, l'échantillonnage au
 * plus proche d'une pièce agrandie de 68 à 86 pixels peut choisir le texel voisin. Une ancre
 * fausse, une échelle fausse ou un ordre de dessin faux, eux, déplacent des pans entiers de l'image
 * et dépassent la tolérance.
 *
 * Les cartes sont celles de la racine d'essai de l'éditeur (`LOT-123`) ; c'étaient les cartes
 * **livrées**, que la table rase du `LOT-102` emporte.
 */

#include <QColor>
#include <QDir>
#include <QImage>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <gtest/gtest.h>
#include <rhi/qrhi.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Editor/Ui/SceneImages.h"
#include "Editor/Ui/ScenePainter.h"
#include "HMI/Graphics/Camera2D.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/Graphics/WorldSceneRenderer.h"

namespace {

constexpr int TARGET_WIDTH = 640;
constexpr int TARGET_HEIGHT = 400;

/// Un fond qu'aucune pièce ne reproduit.
constexpr float CLEAR[4] = {1.0F, 0.0F, 1.0F, 1.0F};

/// Écart par canal au-delà duquel deux pixels diffèrent vraiment.
constexpr int CHANNEL_TOLERANCE = 48;
/// Part des pixels qui peuvent différer : les arêtes des quads (voir l'en-tête du fichier). Mesuré
/// le 18 septembre 2026 : 0,06 % au pire (la place du marché de la carte d'essai) ; 2 % avec un
/// `drawImage` agrandi, qui ouvrait des jours entre les losanges du sol.
constexpr double DIFFERING_PIXELS_TOLERANCE = 0.005;

[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path(JADG_EDITOR_DATA_DIR);
}

[[nodiscard]] std::filesystem::path assets() {
    return dataRoot() / "Assets";
}

[[nodiscard]] std::unique_ptr<QRhi> createOffscreenRhi() {
#ifdef Q_OS_WIN
    QRhiD3D11InitParams params;
    if (QRhi* const rhi = QRhi::create(QRhi::D3D11, &params)) {
        return std::unique_ptr<QRhi>(rhi);
    }
#endif
    return nullptr;
}

/// La cible hors écran : elle vit plus longtemps que le rendu qui la dessine.
struct OffscreenTarget {
    std::unique_ptr<QRhiTexture> texture;
    std::unique_ptr<QRhiTextureRenderTarget> renderTarget;
    std::unique_ptr<QRhiRenderPassDescriptor> pass;

    explicit OffscreenTarget(QRhi& rhi)
        : texture(rhi.newTexture(QRhiTexture::RGBA8, QSize(TARGET_WIDTH, TARGET_HEIGHT), 1,
                                 QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource)) {
        EXPECT_TRUE(texture->create());
        renderTarget.reset(rhi.newTextureRenderTarget({{texture.get()}}));
        pass.reset(renderTarget->newCompatibleRenderPassDescriptor());
        renderTarget->setRenderPassDescriptor(pass.get());
        EXPECT_TRUE(renderTarget->create());
    }
};

/// Une image du rendu GPU du jeu, lue en retour.
[[nodiscard]] QImage renderWithGpu(QRhi& rhi, hmi::WorldSceneRenderer& renderer,
                                   OffscreenTarget& target) {
    QRhiCommandBuffer* commandBuffer = nullptr;
    if (rhi.beginOffscreenFrame(&commandBuffer) != QRhi::FrameOpSuccess) {
        ADD_FAILURE() << "beginOffscreenFrame";
        return {};
    }
    renderer.render(commandBuffer, target.renderTarget.get(), CLEAR);
    QRhiReadbackResult readback;
    QRhiResourceUpdateBatch* const batch = rhi.nextResourceUpdateBatch();
    batch->readBackTexture({target.texture.get()}, &readback);
    commandBuffer->resourceUpdate(batch);
    if (rhi.endOffscreenFrame() != QRhi::FrameOpSuccess) {
        ADD_FAILURE() << "endOffscreenFrame";
        return {};
    }
    // Copie profonde : l'image lue en place pointe dans `readback`, qui meurt au retour.
    const QImage image =
        QImage(reinterpret_cast<const uchar*>(readback.data.constData()),
               readback.pixelSize.width(), readback.pixelSize.height(), QImage::Format_RGBA8888)
            .copy();
    // Le GPU compte ses lignes depuis le bas quand l'interface le veut (OpenGL) ; Direct3D, non.
    return rhi.isYUpInFramebuffer() ? image.flipped(Qt::Vertical) : image;
}

struct Comparison {
    std::size_t differing = 0;
    std::size_t painted = 0;
    std::size_t total = 0;
};

[[nodiscard]] Comparison compare(const QImage& gpu, const QImage& painter) {
    Comparison result;
    for (int y = 0; y < gpu.height(); ++y) {
        for (int x = 0; x < gpu.width(); ++x) {
            const QColor a = gpu.pixelColor(x, y);
            const QColor b = painter.pixelColor(x, y);
            ++result.total;
            if (a != QColor(255, 0, 255)) {
                ++result.painted;
            }
            if (std::abs(a.red() - b.red()) > CHANNEL_TOLERANCE ||
                std::abs(a.green() - b.green()) > CHANNEL_TOLERANCE ||
                std::abs(a.blue() - b.blue()) > CHANNEL_TOLERANCE) {
                ++result.differing;
            }
        }
    }
    return result;
}

[[nodiscard]] hmi::WorldSceneSnapshot mapOnDisk(const std::string& relativePath,
                                                const std::string& place) {
    core::LevelLoadResult map =
        core::LevelLoader::loadFromFile(dataRoot() / "Levels" / relativePath);
    hmi::PlaceAppearanceResult table =
        hmi::PlaceAppearance::loadFromFile(assets() / "Scene" / place / "appearance.json");
    if (!map.ok() || !table.ok()) {
        throw std::runtime_error(relativePath + " : " + map.error + table.message);
    }
    return hmi::snapshotWorldScene(*map.level, table.appearance,
                                   hmi::npcFigures(map.level->entities(), 0));
}

/// Rend @p snapshot des deux façons, cadré sur @p focus (en cases), et compare.
void expectSamePicture(QRhi& rhi, const hmi::WorldSceneSnapshot& snapshot, core::Vector2 focus,
                       const std::string& name) {
    OffscreenTarget target(rhi);
    hmi::WorldSceneRenderer renderer(assets());
    ASSERT_TRUE(renderer.ensureResources(&rhi));
    renderer.setSnapshot(snapshot);
    renderer.setFocus(focus);
    const QImage gpu = renderWithGpu(rhi, renderer, target);
    ASSERT_EQ(gpu.size(), QSize(TARGET_WIDTH, TARGET_HEIGHT));

    const core::IsoProjection projection(snapshot.columns, snapshot.rows);
    const hmi::Camera2D camera =
        hmi::worldCamera(projection, projection.gridToWorld(focus), TARGET_WIDTH, TARGET_HEIGHT);
    hmi::SceneImages images(assets());
    images.ensure(hmi::worldTexturePaths(snapshot));
    const hmi::ComposedScene scene =
        hmi::composeWorldScene(snapshot, projection, images.textures());
    const QImage painted =
        hmi::renderComposedScene(scene, camera, TARGET_WIDTH, TARGET_HEIGHT, QColor(255, 0, 255))
            .convertToFormat(QImage::Format_RGBA8888);

    const QDir captures(QDir::current().filePath(QStringLiteral("editor-captures")));
    QDir().mkpath(captures.path());
    gpu.save(captures.filePath(QString::fromStdString(name + "-jeu.png")));
    painted.save(captures.filePath(QString::fromStdString(name + "-editeur.png")));

    const Comparison result = compare(gpu, painted);
    EXPECT_GT(result.painted, result.total / 2) << name << " : l'image n'est pas que le fond";
    EXPECT_LT(static_cast<double>(result.differing) / static_cast<double>(result.total),
              DIFFERING_PIXELS_TOLERANCE)
        << name << " : " << result.differing << " pixels sur " << result.total
        << " different au-dela de la tolerance";
}

}  // namespace

/**
 * @brief La Place peinte par l'éditeur égale, à une tolérance près, le rendu GPU du jeu.
 * \castest{<b>Le canevas de l'editeur peint une carte comme le jeu la dessine.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Composer la carte d'essai, ses PNJ compris.<br/>
 *          2. La rendre hors ecran par le rendu QRhi du jeu, cadree sur trois points (grand-
 *             place, coin nord, porte est).<br/>
 *          3. La peindre par le peintre QPainter de l'editeur avec la meme camera.<br/>
 * \tattendu Pour chaque cadrage, moins de 0,5 % des pixels different de plus de 48 sur un canal ;
 *           l'image est peinte sur plus de la moitie de sa surface.
 * }
 */
TEST(ScenePainterTest, UneCartePeinteEgaleLeRenduDuJeu) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    const hmi::WorldSceneSnapshot place = mapOnDisk("bourg/place.json", "bourg");
    expectSamePicture(*rhi, place, {22.0F, 20.0F}, "place-centre");
    expectSamePicture(*rhi, place, {6.0F, 4.0F}, "place-nord");
    expectSamePicture(*rhi, place, {44.0F, 30.0F}, "place-porte-est");
}

/**
 * @brief La seconde carte, et ses pièces larges et miroirs, peinte comme dans le jeu.
 * \castest{<b>Le canevas de l'editeur peint la seconde carte comme le jeu la dessine.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Composer le donjon.<br/>
 *          2. Le rendre par le jeu et par l'editeur, cadre sur sa porte.<br/>
 * \tattendu Moins de 0,5 % des pixels different au-dela de la tolerance.
 * }
 */
TEST(ScenePainterTest, LaSecondeCartePeinteEgaleLeRenduDuJeu) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    expectSamePicture(*rhi, mapOnDisk("donjon.json", "bourg"), {19.5F, 30.5F},
                      "donjon-porte");
}
