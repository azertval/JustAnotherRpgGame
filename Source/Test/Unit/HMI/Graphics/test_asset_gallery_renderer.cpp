// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_asset_gallery_renderer.cpp
 * @brief Le rendu de la galerie des assets **hors écran** : ne charger que ce qui est voulu,
 *        étaler les chargements, libérer le reste après son délai.
 *
 * Se saute proprement si la machine n'offre aucune interface QRhi.
 */

#include <QColor>
#include <QImage>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <rhi/qrhi.h>

#include "HMI/Graphics/AssetGalleryRenderer.h"

namespace {

constexpr int TARGET_SIZE = 256;
constexpr float CLEAR[4] = {1.0f, 0.0f, 1.0f, 1.0f};

std::unique_ptr<QRhi> createOffscreenRhi() {
#ifdef Q_OS_WIN
    QRhiD3D11InitParams params;
    if (QRhi* const rhi = QRhi::create(QRhi::D3D11, &params)) {
        return std::unique_ptr<QRhi>(rhi);
    }
#endif
    return nullptr;
}

struct OffscreenTarget {
    std::unique_ptr<QRhiTexture> texture;
    /// Un tampon de profondeur : la galerie dessine des modèles (`LOT-1006`).
    std::unique_ptr<QRhiRenderBuffer> depth;
    std::unique_ptr<QRhiTextureRenderTarget> renderTarget;
    std::unique_ptr<QRhiRenderPassDescriptor> pass;

    explicit OffscreenTarget(QRhi& rhi)
        : texture(rhi.newTexture(QRhiTexture::RGBA8, QSize(TARGET_SIZE, TARGET_SIZE), 1,
                                 QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource)) {
        EXPECT_TRUE(texture->create());
        depth.reset(rhi.newRenderBuffer(QRhiRenderBuffer::DepthStencil,
                                        QSize(TARGET_SIZE, TARGET_SIZE)));
        EXPECT_TRUE(depth->create());
        QRhiTextureRenderTargetDescription description{{texture.get()}};
        description.setDepthStencilBuffer(depth.get());
        renderTarget.reset(rhi.newTextureRenderTarget(description));
        pass.reset(renderTarget->newCompatibleRenderPassDescriptor());
        renderTarget->setRenderPassDescriptor(pass.get());
        EXPECT_TRUE(renderTarget->create());
    }
};

QImage renderFrame(QRhi& rhi, hmi::AssetGalleryRenderer& renderer, OffscreenTarget& target,
                   float deltaSeconds) {
    QRhiCommandBuffer* commandBuffer = nullptr;
    if (rhi.beginOffscreenFrame(&commandBuffer) != QRhi::FrameOpSuccess) {
        ADD_FAILURE() << "beginOffscreenFrame";
        return {};
    }
    renderer.render(commandBuffer, target.renderTarget.get(), deltaSeconds, CLEAR);
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

std::filesystem::path assets() {
    return std::filesystem::path(JADG_TEST_DATA_DIR) / "Assets";
}

/// Un bloc de figure en (0, 0), sans grille ni emprise : seul le sprite peint.
hmi::AssetGalleryFrame figureFrame(const std::string& path) {
    hmi::AssetGalleryFrame frame;
    // Une case de 68 pixels pour un lieu qui declare un losange de 68 : l'art a sa taille.
    frame.cellPixels = 68.0f;
    frame.pixelScale = 1.0f;
    frame.showGrid = false;
    frame.showFootprint = false;
    frame.drawn.push_back(hmi::AssetGalleryDrawnBloc{
        .path = path, .frameWidth = 48, .frameHeight = 64, .tilePixels = 68});
    frame.wanted = {path};
    return frame;
}

}  // namespace

/**
 * @brief Une texture voulue est chargée et dessinée ; plus voulue, elle est libérée après délai.
 * \castest{<b>La galerie ne garde que les textures voulues.</b><br/>
 * \tcat Unitaire · Galerie des assets (rendu)<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Dessiner le portrait d'un PNJ. 2. Ne plus rien vouloir, 1 s puis 1,5 s.<br/>
 * \tattendu Une texture en mémoire et des pixels peints ; gardée à 1 s ; libérée à 2,5 s.
 * }
 */
TEST(AssetGalleryRendererTest, ChargementEtLiberation) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    OffscreenTarget target(*rhi);
    hmi::AssetGalleryRenderer renderer(assets());
    ASSERT_TRUE(renderer.ensureResources(rhi.get()));

    renderer.setFrame(figureFrame("Npc/figurant/portrait.png"));
    const QImage image = renderFrame(*rhi, renderer, target, 0.016f);
    EXPECT_EQ(renderer.cachedTextureCount(), 1U);
    EXPECT_GT(paintedPixels(image), 100U);

    renderer.setFrame(hmi::AssetGalleryFrame{});
    renderFrame(*rhi, renderer, target, 1.0f);
    EXPECT_EQ(renderer.cachedTextureCount(), 1U);
    renderFrame(*rhi, renderer, target, 1.5f);
    EXPECT_EQ(renderer.cachedTextureCount(), 0U);
}

/**
 * @brief Un grand saut étale ses chargements ; un fichier absent est retenu, dessiné en damier.
 * \castest{<b>Les chargements s'étalent, un fichier absent ne bloque rien.</b><br/>
 * \tcat Unitaire · Galerie des assets (rendu)<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Vouloir 30 textures d'un coup. 2. Dessiner une texture absente.<br/>
 * \tattendu 24 chargées puis 30 ; la texture absente est retenue et peint quand même.
 * }
 */
TEST(AssetGalleryRendererTest, ChargementsEtalesEtFichierAbsent) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    OffscreenTarget target(*rhi);
    hmi::AssetGalleryRenderer renderer(assets());
    ASSERT_TRUE(renderer.ensureResources(rhi.get()));

    // Vingt-huit images voulues d'un coup, plus que UPLOADS_PER_FRAME : c'est tout ce que ce test
    // demande a la donnee. Le portrait existe ; les autres sont absentes, et une image absente
    // est retenue comme une autre.
    hmi::AssetGalleryFrame many;
    many.wanted.emplace_back("Npc/figurant/portrait.png");
    for (int rank = 1; rank < 28; ++rank) {
        many.wanted.push_back("Npc/absente-" + std::to_string(rank) + "/portrait.png");
    }
    renderer.setFrame(many);
    renderFrame(*rhi, renderer, target, 0.016f);
    EXPECT_EQ(renderer.cachedTextureCount(),
              static_cast<std::size_t>(hmi::AssetGalleryRenderer::UPLOADS_PER_FRAME));
    EXPECT_TRUE(renderer.loading());
    renderFrame(*rhi, renderer, target, 0.016f);
    EXPECT_EQ(renderer.cachedTextureCount(), 28U);
    EXPECT_FALSE(renderer.loading());

    hmi::AssetGalleryRenderer absent(assets());
    ASSERT_TRUE(absent.ensureResources(rhi.get()));
    absent.setFrame(figureFrame("Npc/personne/portrait.png"));
    const QImage image = renderFrame(*rhi, absent, target, 0.016f);
    EXPECT_EQ(absent.cachedTextureCount(), 1U);
    EXPECT_GT(paintedPixels(image), 100U);
}

/**
 * @brief Un modèle de personnage se charge et se dessine en volume, et son clip le fait bouger
 *        (`LOT-1006`).
 * \castest{<b>La galerie dessine un modele de personnage, anime par son clip.</b><br/>
 * \tcat Unitaire · Galerie des assets (rendu)<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Dessiner le modèle du PNJ d'essai au début de son attaque, puis à son image clé.<br/>
 * 2. Dessiner un modèle absent.<br/>
 * \tattendu Un modèle en mémoire, aucune texture ; des pixels peints, du vert du pantin ; l'image
 * à l'image clé diffère de celle du début. Le modèle absent est retenu, ne peint rien et ne
 * plante rien.
 * }
 */
TEST(AssetGalleryRendererTest, UnModeleSeDessineEtSAnime) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    OffscreenTarget target(*rhi);
    hmi::AssetGalleryRenderer renderer(assets());
    ASSERT_TRUE(renderer.ensureResources(rhi.get()));

    const auto modelFrame = [](const std::string& path, float seconds) {
        hmi::AssetGalleryFrame frame;
        frame.cellPixels = 68.0f;
        frame.pixelScale = 1.0f;
        frame.showGrid = false;
        frame.showFootprint = false;
        frame.drawn.push_back(hmi::AssetGalleryDrawnBloc{.path = path,
                                                         .x = 34.0f,
                                                         .y = 0.0f,
                                                         .columns = 4,
                                                         .rows = 3,
                                                         .footprintColumn = 1,
                                                         .footprintRow = 1,
                                                         .frameWidth = 102,
                                                         .frameHeight = 68,
                                                         .tilePixels = 68,
                                                         .mesh = true,
                                                         .clip = "attack",
                                                         .clipSeconds = seconds});
        frame.wanted = {path};
        return frame;
    };
    const auto green = [](const QImage& image) {
        std::size_t count = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                count += pixel.green() > pixel.red() + 25 && pixel.green() > pixel.blue() + 25;
            }
        }
        return count;
    };

    renderer.setFrame(modelFrame("Npc/figurant/figurant.glb", 0.0f));
    const QImage start = renderFrame(*rhi, renderer, target, 0.016f);
    EXPECT_EQ(renderer.cachedModelCount(), 1U);
    EXPECT_EQ(renderer.cachedTextureCount(), 0U);
    EXPECT_GT(green(start), 200U);

    renderer.setFrame(modelFrame("Npc/figurant/figurant.glb", 0.4f));
    const QImage strike = renderFrame(*rhi, renderer, target, 0.016f);
    EXPECT_GT(green(strike), 200U);
    EXPECT_NE(start, strike) << "le buste s'est penche";

    renderer.setFrame(modelFrame("Npc/personne/personne.glb", 0.0f));
    const QImage absent = renderFrame(*rhi, renderer, target, 0.016f);
    EXPECT_EQ(renderer.cachedModelCount(), 2U);
    EXPECT_EQ(green(absent), 0U);
}
