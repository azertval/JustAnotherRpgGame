// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_rhi_offscreen.cpp
 * @brief Non-régression **visuelle** du pipeline QRhi (`EX-REN-050`, `EX-ARCH-022`) : rendu
 *        **hors écran** de motifs témoins, relus pixel par pixel.
 *
 * Le filtrage suit la nature de l'image (`LOT-103`), et chacune a son témoin : une image
 * **engendrée** (`TextureFiltering::Sharp`) agrandie reste en blocs **exactement** égaux au texel
 * source ; l'art **peint** (`TextureFiltering::Smooth`) réduit se moyenne par ses mipmaps au lieu
 * de scintiller, et sa transparence est **prémultipliée**. Chacun de ces défauts passerait toute la
 * CI logique : seul l'œil — ou ce test — le verrait.
 *
 * Le test s'exécute sur une interface QRhi hors écran (aucune fenêtre) et se **saute** proprement
 * si aucune n'est disponible sur la machine (`EX-NFR-004` borne ce qui est automatisable).
 */

#include <QImage>
#include <cstdint>
#include <memory>
#include <vector>

#include <gtest/gtest.h>
#include <rhi/qrhi.h>

#include "HMI/Graphics/Quad.h"
#include "HMI/Graphics/RhiContext.h"
#include "HMI/Graphics/SpriteBatch.h"
#include "HMI/Graphics/TextureLoader.h"

namespace {

/// Côté, en texels, de la texture témoin.
constexpr int SOURCE_SIZE = 4;
/// Facteur d'agrandissement **entier** appliqué au rendu (`EX-ARCH-022`).
constexpr int ZOOM = 4;
/// Côté, en pixels, de la cible de rendu : la texture témoin agrandie.
constexpr int TARGET_SIZE = SOURCE_SIZE * ZOOM;

/// Couleur RGBA (octets) telle que `hmi::DecodedImage` la stocke : R en poids faible.
constexpr std::uint32_t rgba(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    return static_cast<std::uint32_t>(r) | (static_cast<std::uint32_t>(g) << 8) |
           (static_cast<std::uint32_t>(b) << 16) | (static_cast<std::uint32_t>(a) << 24);
}

/// Motif témoin 4x4 : quatre couleurs pures, franches, sans dégradé — toute couleur intermédiaire
/// relue est donc nécessairement le produit d'un filtrage.
std::vector<std::uint32_t> witnessPixels() {
    const std::uint32_t red = rgba(255, 0, 0, 255);
    const std::uint32_t green = rgba(0, 255, 0, 255);
    const std::uint32_t blue = rgba(0, 0, 255, 255);
    const std::uint32_t white = rgba(255, 255, 255, 255);
    return {
        red,  red,  green, green,  //
        red,  red,  green, green,  //
        blue, blue, white, white,  //
        blue, blue, white, white,  //
    };
}

/// Projection pixel : une unité monde = un pixel de la cible, origine en haut à gauche, Y vers le
/// bas — même convention (ligne-major, `position * matrice`) que `hmi::Camera2D::projectionMatrix`.
DirectX::XMFLOAT4X4 pixelProjection(int width, int height) {
    const float scaleX = 2.0f / static_cast<float>(width);
    const float scaleY = 2.0f / static_cast<float>(height);
    return DirectX::XMFLOAT4X4(scaleX, 0.0f, 0.0f, 0.0f,   //
                               0.0f, -scaleY, 0.0f, 0.0f,  //
                               0.0f, 0.0f, 1.0f, 0.0f,     //
                               -1.0f, 1.0f, 0.0f, 1.0f);
}

/// Crée une interface QRhi hors écran, ou `nullptr` si la machine n'en offre aucune.
std::unique_ptr<QRhi> createOffscreenRhi() {
#ifdef Q_OS_WIN
    QRhiD3D11InitParams params;
    if (QRhi* const rhi = QRhi::create(QRhi::D3D11, &params)) {
        return std::unique_ptr<QRhi>(rhi);
    }
#endif
    return nullptr;
}

/**
 * @brief Dessine un quad de @p source couvrant toute une cible de @p targetSize de côté, effacée à
 *        @p clear, et relit la cible.
 */
QImage drawFullQuad(QRhi& rhi, int sourceSize, const std::vector<std::uint32_t>& pixels,
                    hmi::TextureFiltering filtering, int targetSize, const float* clear) {
    const std::unique_ptr<QRhiTexture> target(
        rhi.newTexture(QRhiTexture::RGBA8, QSize(targetSize, targetSize), 1,
                       QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    EXPECT_TRUE(target->create());
    const std::unique_ptr<QRhiTextureRenderTarget> renderTarget(
        rhi.newTextureRenderTarget({{target.get()}}));
    const std::unique_ptr<QRhiRenderPassDescriptor> pass(
        renderTarget->newCompatibleRenderPassDescriptor());
    renderTarget->setRenderPassDescriptor(pass.get());
    EXPECT_TRUE(renderTarget->create());

    hmi::SpriteBatch batch(&rhi);
    QRhiCommandBuffer* commandBuffer = nullptr;
    EXPECT_EQ(rhi.beginOffscreenFrame(&commandBuffer), QRhi::FrameOpSuccess);
    hmi::RhiContext context;
    context.rhi = &rhi;
    context.updates = rhi.nextResourceUpdateBatch();
    const std::optional<hmi::LoadedTexture> source =
        hmi::createTexture(context, sourceSize, sourceSize, pixels, filtering);
    EXPECT_TRUE(source.has_value());

    hmi::SpriteQuad quad;
    quad.width = static_cast<float>(targetSize);
    quad.height = static_cast<float>(targetSize);
    quad.u1 = 1.0f;
    quad.v1 = 1.0f;
    batch.beginFrame();
    batch.begin(pixelProjection(targetSize, targetSize), source->handle());
    batch.draw(quad);
    batch.end();
    batch.submit(commandBuffer, renderTarget.get(), context.updates, clear);

    QRhiReadbackResult readback;
    QRhiResourceUpdateBatch* const readbackBatch = rhi.nextResourceUpdateBatch();
    readbackBatch->readBackTexture({target.get()}, &readback);
    commandBuffer->resourceUpdate(readbackBatch);
    EXPECT_EQ(rhi.endOffscreenFrame(), QRhi::FrameOpSuccess);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): QImage lit des `uchar`.
    return QImage(reinterpret_cast<const uchar*>(readback.data.constData()),
                  readback.pixelSize.width(), readback.pixelSize.height(), QImage::Format_RGBA8888)
        .copy();
}

}  // namespace

/**
 * @brief L'art peint réduit au quart se **moyenne** par ses mipmaps : un damier d'un pixel devient
 *        un gris uniforme, là où le plus proche voisin ne garderait qu'une case sur quatre — et
 *        scintillerait au moindre déplacement.
 * \castest{<b>L'art peint reduit ne scintille pas : ses mipmaps le moyennent.</b><br/>
 * \tcat Unitaire · Rendu QRhi<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Creer une texture lissee de 64 x 64, damier noir et blanc d'un pixel.<br/>
 * 2. La dessiner reduite dans une cible de 16 x 16.<br/>
 * \tattendu Chaque pixel de la cible est un gris moyen (entre 96 et 160), jamais noir ni blanc.
 * }
 */
TEST(RhiOffscreenTest, LArtPeintReduitSeMoyenneParSesMipmaps) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    constexpr int SIDE = 64;
    std::vector<std::uint32_t> checker(static_cast<std::size_t>(SIDE) * SIDE);
    for (int y = 0; y < SIDE; ++y) {
        for (int x = 0; x < SIDE; ++x) {
            checker[(static_cast<std::size_t>(y) * SIDE) + x] =
                (x + y) % 2 == 0 ? rgba(0, 0, 0, 255) : rgba(255, 255, 255, 255);
        }
    }
    const float clear[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    const QImage rendered =
        drawFullQuad(*rhi, SIDE, checker, hmi::TextureFiltering::Smooth, 16, clear);
    ASSERT_EQ(rendered.size(), QSize(16, 16));
    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            const QColor pixel = rendered.pixelColor(x, y);
            EXPECT_GE(pixel.green(), 96) << "pixel (" << x << ", " << y << ')';
            EXPECT_LE(pixel.green(), 160) << "pixel (" << x << ", " << y << ')';
        }
    }
}

/**
 * @brief La transparence est **prémultipliée** : un rouge à demi transparent sur un fond bleu
 *        donne un pourpre à parts égales, pas un rouge plein fondu dans du bleu.
 * \castest{<b>Une texture a demi transparente se melange en alpha premultiplie.</b><br/>
 * \tcat Unitaire · Rendu QRhi<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Dessiner une texture rouge d'alpha 128 sur un fond bleu.<br/>
 * \tattendu Rouge et bleu valent chacun la moitie, a deux niveaux pres.
 * }
 */
TEST(RhiOffscreenTest, LaTransparenceEstPremultipliee) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    const std::vector<std::uint32_t> halfRed(16, rgba(255, 0, 0, 128));
    const float clear[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    for (const hmi::TextureFiltering filtering :
         {hmi::TextureFiltering::Sharp, hmi::TextureFiltering::Smooth}) {
        const QImage rendered = drawFullQuad(*rhi, 4, halfRed, filtering, 8, clear);
        const QColor pixel = rendered.pixelColor(4, 4);
        EXPECT_NEAR(pixel.red(), 128, 2);
        EXPECT_EQ(pixel.green(), 0);
        EXPECT_NEAR(pixel.blue(), 127, 2);
    }
}

/**
 * @brief Une image engendrée agrandie d'un facteur entier reste **nette** : chaque texel devient
 * un bloc de couleur uniforme, sans la moindre teinte intermédiaire.
 * \castest{<b>Le rendu QRhi garde nette une image engendree agrandie.</b><br/>
 * \tcat Unitaire · Rendu QRhi<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Rendre hors ecran une texture temoin 4x4 agrandie 4 fois.<br/>2. Relire les pixels
 * de la cible.<br/>
 * \tattendu Chaque texel donne un bloc 4x4 uniforme, et aucune couleur hors des quatre du motif
 * n'apparait.
 * }
 */
TEST(RhiOffscreenTest, ZoomEntierResteNetEnFiltrageNearest) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }

    // Cible de rendu hors ecran, relisible.
    const std::unique_ptr<QRhiTexture> target(
        rhi->newTexture(QRhiTexture::RGBA8, QSize(TARGET_SIZE, TARGET_SIZE), 1,
                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    ASSERT_TRUE(target->create());
    const std::unique_ptr<QRhiTextureRenderTarget> renderTarget(
        rhi->newTextureRenderTarget({{target.get()}}));
    const std::unique_ptr<QRhiRenderPassDescriptor> pass(
        renderTarget->newCompatibleRenderPassDescriptor());
    renderTarget->setRenderPassDescriptor(pass.get());
    ASSERT_TRUE(renderTarget->create());

    hmi::SpriteBatch batch(rhi.get());

    QRhiCommandBuffer* commandBuffer = nullptr;
    ASSERT_EQ(rhi->beginOffscreenFrame(&commandBuffer), QRhi::FrameOpSuccess);

    hmi::RhiContext context;
    context.rhi = rhi.get();
    context.updates = rhi->nextResourceUpdateBatch();
    const std::optional<hmi::LoadedTexture> source =
        hmi::createTexture(context, SOURCE_SIZE, SOURCE_SIZE, witnessPixels());
    ASSERT_TRUE(source.has_value());

    // Un seul quad, couvrant exactement la cible, UV pleines : l'agrandissement est donc
    // strictement entier (4 pixels de cible par texel).
    hmi::SpriteQuad quad;
    quad.x = 0.0f;
    quad.y = 0.0f;
    quad.width = static_cast<float>(TARGET_SIZE);
    quad.height = static_cast<float>(TARGET_SIZE);
    quad.u1 = 1.0f;
    quad.v1 = 1.0f;

    batch.beginFrame();
    batch.begin(pixelProjection(TARGET_SIZE, TARGET_SIZE), source->handle());
    batch.draw(quad);
    batch.end();
    const float clear[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    batch.submit(commandBuffer, renderTarget.get(), context.updates, clear);
    context.updates = nullptr;

    QRhiReadbackResult readback;
    QRhiResourceUpdateBatch* const readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({target.get()}, &readback);
    commandBuffer->resourceUpdate(readbackBatch);
    ASSERT_EQ(rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
    ASSERT_EQ(readback.pixelSize, QSize(TARGET_SIZE, TARGET_SIZE));

    const QImage rendered(reinterpret_cast<const uchar*>(readback.data.constData()),
                          readback.pixelSize.width(), readback.pixelSize.height(),
                          QImage::Format_RGBA8888);
    const std::vector<std::uint32_t> expected = witnessPixels();

    for (int y = 0; y < TARGET_SIZE; ++y) {
        for (int x = 0; x < TARGET_SIZE; ++x) {
            const std::uint32_t source_texel =
                expected[static_cast<std::size_t>(y / ZOOM) * SOURCE_SIZE +
                         static_cast<std::size_t>(x / ZOOM)];
            const QColor pixel = rendered.pixelColor(x, y);
            const auto red = static_cast<std::uint8_t>(source_texel & 0xFF);
            const auto green = static_cast<std::uint8_t>((source_texel >> 8) & 0xFF);
            const auto blue = static_cast<std::uint8_t>((source_texel >> 16) & 0xFF);
            // Egalite EXACTE, pas une tolerance : un filtrage lineaire ne produirait pas
            // exactement la couleur du texel aux frontieres de blocs, et c'est precisement ce que
            // ce test doit refuser.
            EXPECT_EQ(pixel.red(), red) << "pixel (" << x << ", " << y << ')';
            EXPECT_EQ(pixel.green(), green) << "pixel (" << x << ", " << y << ')';
            EXPECT_EQ(pixel.blue(), blue) << "pixel (" << x << ", " << y << ')';
        }
    }
}

/**
 * @brief La teinte du quad multiplie la texture échantillonnée, et le fond d'effacement subsiste
 * là où rien n'est dessiné — les deux invariants de composition du pipeline porté.
 * \castest{<b>La teinte multiplie la texture et l'effacement subsiste hors du quad.</b><br/>
 * \tcat Unitaire · Rendu QRhi<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Rendre hors ecran un quad teinte couvrant un quart de la cible.<br/>2. Relire un
 * pixel dans le quad et un pixel hors du quad.<br/>
 * \tattendu Le pixel du quad vaut la couleur source multipliee par la teinte ; le pixel hors du
 * quad vaut la couleur d'effacement.
 * }
 */
TEST(RhiOffscreenTest, TeinteMultiplieeEtEffacementConserve) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }

    const std::unique_ptr<QRhiTexture> target(
        rhi->newTexture(QRhiTexture::RGBA8, QSize(TARGET_SIZE, TARGET_SIZE), 1,
                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    ASSERT_TRUE(target->create());
    const std::unique_ptr<QRhiTextureRenderTarget> renderTarget(
        rhi->newTextureRenderTarget({{target.get()}}));
    const std::unique_ptr<QRhiRenderPassDescriptor> pass(
        renderTarget->newCompatibleRenderPassDescriptor());
    renderTarget->setRenderPassDescriptor(pass.get());
    ASSERT_TRUE(renderTarget->create());

    hmi::SpriteBatch batch(rhi.get());

    QRhiCommandBuffer* commandBuffer = nullptr;
    ASSERT_EQ(rhi->beginOffscreenFrame(&commandBuffer), QRhi::FrameOpSuccess);

    hmi::RhiContext context;
    context.rhi = rhi.get();
    context.updates = rhi->nextResourceUpdateBatch();
    // Texture entierement blanche : la couleur relue est alors exactement la teinte.
    const std::vector<std::uint32_t> white(SOURCE_SIZE * SOURCE_SIZE, rgba(255, 255, 255, 255));
    const std::optional<hmi::LoadedTexture> source =
        hmi::createTexture(context, SOURCE_SIZE, SOURCE_SIZE, white);
    ASSERT_TRUE(source.has_value());

    hmi::SpriteQuad quad;
    quad.x = 0.0f;
    quad.y = 0.0f;
    quad.width = static_cast<float>(TARGET_SIZE / 2);
    quad.height = static_cast<float>(TARGET_SIZE / 2);
    quad.u1 = 1.0f;
    quad.v1 = 1.0f;
    quad.r = 0.0f;
    quad.g = 1.0f;
    quad.b = 0.0f;
    quad.a = 1.0f;

    batch.beginFrame();
    batch.begin(pixelProjection(TARGET_SIZE, TARGET_SIZE), source->handle());
    batch.draw(quad);
    batch.end();
    const float clear[4] = {0.0f, 0.0f, 1.0f, 1.0f};  // bleu franc : distinct de la teinte verte
    batch.submit(commandBuffer, renderTarget.get(), context.updates, clear);
    context.updates = nullptr;

    QRhiReadbackResult readback;
    QRhiResourceUpdateBatch* const readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({target.get()}, &readback);
    commandBuffer->resourceUpdate(readbackBatch);
    ASSERT_EQ(rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);

    const QImage rendered(reinterpret_cast<const uchar*>(readback.data.constData()),
                          readback.pixelSize.width(), readback.pixelSize.height(),
                          QImage::Format_RGBA8888);

    const QColor inside = rendered.pixelColor(1, 1);
    EXPECT_EQ(inside.red(), 0);
    EXPECT_EQ(inside.green(), 255);
    EXPECT_EQ(inside.blue(), 0);

    const QColor outside = rendered.pixelColor(TARGET_SIZE - 1, TARGET_SIZE - 1);
    EXPECT_EQ(outside.red(), 0);
    EXPECT_EQ(outside.green(), 0);
    EXPECT_EQ(outside.blue(), 255);
}
