// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/CityBlockRender.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>

#include <rhi/qrhi.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/World/CityBlock.h"
#include "HMI/Graphics/Camera2D.h"
#include "HMI/Graphics/GraphicsLog.h"
#include "HMI/Graphics/ScenePieces.h"
#include "HMI/Graphics/WorldSceneRenderer.h"

namespace hmi {

namespace {

// Fond de l'image : celui de l'ecran de jeu, que le dehors d'une carte laisse voir.
constexpr std::array<float, 4> FOND = {0.043F, 0.043F, 0.043F, 1.0F};

std::unique_ptr<QRhi> interfaceHorsEcran() {
#ifdef Q_OS_WIN
    QRhiD3D11InitParams parametres;
    if (QRhi* const rhi = QRhi::create(QRhi::D3D11, &parametres)) {
        return std::unique_ptr<QRhi>(rhi);
    }
#endif
    return nullptr;
}

}  // namespace

CityBlockFraming cityBlockFraming(const core::IsoProjection& projection,
                                  const core::CityBlock& block, float maximumRise,
                                  float tilePixels) {
    const auto gauche = static_cast<float>(block.origin.column);
    const auto haut = static_cast<float>(block.origin.row);
    const float droite = gauche + static_cast<float>(block.columns);
    const float bas = haut + static_cast<float>(block.rows);
    const std::array<core::Vector2, 4> coins = {
        projection.gridToWorld({gauche, haut}), projection.gridToWorld({droite, haut}),
        projection.gridToWorld({gauche, bas}), projection.gridToWorld({droite, bas})};
    float minX = coins[0].x;
    float maxX = coins[0].x;
    float minY = coins[0].y;
    float maxY = coins[0].y;
    for (const core::Vector2& coin : coins) {
        minX = std::min(minX, coin.x);
        maxX = std::max(maxX, coin.x);
        minY = std::min(minY, coin.y);
        maxY = std::max(maxY, coin.y);
    }
    minY -= std::max(0.0F, maximumRise) * projection.tileWidth();

    const core::Vector2 centre{(minX + maxX) / 2.0F, (minY + maxY) / 2.0F};
    const float pixelsParUnite = std::max(1.0F, tilePixels) / projection.tileWidth();
    return CityBlockFraming{
        .focus = projection.worldToGrid(centre),
        .pixelWidth = static_cast<int>(std::ceil((maxX - minX) * pixelsParUnite)),
        .pixelHeight = static_cast<int>(std::ceil((maxY - minY) * pixelsParUnite))};
}

QImage renderCityBlock(const std::filesystem::path& assetsDirectory,
                       const WorldSceneSnapshot& snapshot, const core::CityBlock& block) {
    const std::unique_ptr<QRhi> rhi = interfaceHorsEcran();
    if (!rhi) {
        GRAPHICS_LOG_WARNING("Plan : aucune interface QRhi hors ecran, l'ilot ne se dessine pas.");
        return {};
    }
    const core::IsoProjection projection(snapshot.columns, snapshot.rows,
                                         core::ARENA_TILE_WIDTH_UNITS, snapshot.diamondRatio);
    const CityBlockFraming cadrage = cityBlockFraming(projection, block, snapshot.maximumRise);
    const QSize taille(std::max(1, cadrage.pixelWidth), std::max(1, cadrage.pixelHeight));

    const std::unique_ptr<QRhiTexture> texture(
        rhi->newTexture(QRhiTexture::RGBA8, taille, 1,
                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    if (!texture->create()) {
        return {};
    }
    const std::unique_ptr<QRhiTextureRenderTarget> cible(
        rhi->newTextureRenderTarget({{texture.get()}}));
    const std::unique_ptr<QRhiRenderPassDescriptor> passe(
        cible->newCompatibleRenderPassDescriptor());
    cible->setRenderPassDescriptor(passe.get());
    if (!cible->create()) {
        return {};
    }

    QImage image;
    {
        // Le rendu avant l'interface : ses ressources appartiennent a `rhi`, et le detruire apres
        // elle liberait des objets deja morts.
        WorldSceneRenderer rendu(assetsDirectory);
        if (!rendu.ensureResources(rhi.get())) {
            return {};
        }
        rendu.setSnapshot(snapshot);
        rendu.setFocus(cadrage.focus);
        rendu.setTilePixels(CITY_BLOCK_TILE_PIXELS);

        QRhiCommandBuffer* commandes = nullptr;
        if (rhi->beginOffscreenFrame(&commandes) != QRhi::FrameOpSuccess) {
            return {};
        }
        rendu.render(commandes, cible.get(), FOND.data());
        QRhiReadbackResult relecture;
        QRhiResourceUpdateBatch* const lot = rhi->nextResourceUpdateBatch();
        lot->readBackTexture({texture.get()}, &relecture);
        commandes->resourceUpdate(lot);
        if (rhi->endOffscreenFrame() != QRhi::FrameOpSuccess) {
            return {};
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): QImage lit des `uchar`.
        image = QImage(reinterpret_cast<const uchar*>(relecture.data.constData()),
                       relecture.pixelSize.width(), relecture.pixelSize.height(),
                       QImage::Format_RGBA8888)
                    .copy();
        rendu.release();
    }
    return image;
}

}  // namespace hmi
