// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/OffscreenRender.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <optional>
#include <utility>

#include <rhi/qrhi.h>

#include "HMI/Graphics/GraphicsLog.h"

namespace hmi {

namespace {

/// Octets d'un pixel `RGBA8`.
constexpr int PIXEL_BYTES = 4;

[[nodiscard]] std::unique_ptr<QRhi> createRhi() {
#ifdef Q_OS_WIN
    QRhiD3D11InitParams params;
    if (QRhi* const rhi = QRhi::create(QRhi::D3D11, &params)) {
        return std::unique_ptr<QRhi>(rhi);
    }
#endif
    return nullptr;
}

}  // namespace

OffscreenRhi::OffscreenRhi(std::unique_ptr<QRhi> rhi) : _rhi(std::move(rhi)) {}

OffscreenRhi::~OffscreenRhi() {
    // Les rendus gardes, puis la cible, sa passe et sa texture, puis l'interface.
    _renderers.clear();
    _target.reset();
    _pass.reset();
    _texture.reset();
    _depth.reset();
    _rhi.reset();
}

std::shared_ptr<OffscreenRhi> OffscreenRhi::shared() {
    // Un seul fil : aucun verrou (voir la classe).
    static std::weak_ptr<OffscreenRhi> instance;
    if (std::shared_ptr<OffscreenRhi> alive = instance.lock()) {
        return alive;
    }
    std::unique_ptr<QRhi> rhi = createRhi();
    if (!rhi) {
        GRAPHICS_LOG_WARNING("Rendu hors ecran : aucune interface QRhi sur cette machine.");
        return nullptr;
    }
    std::shared_ptr<OffscreenRhi> created(new OffscreenRhi(std::move(rhi)));
    instance = created;
    return created;
}

WorldSceneRenderer* OffscreenRhi::renderer(const std::filesystem::path& assetsDirectory) {
    std::error_code error;
    std::filesystem::path key = std::filesystem::weakly_canonical(assetsDirectory, error);
    if (error) {
        key = assetsDirectory.lexically_normal();
    }
    std::unique_ptr<WorldSceneRenderer>& kept = _renderers[key];
    if (!kept) {
        kept = std::make_unique<WorldSceneRenderer>(assetsDirectory);
    } else if (kept->textureBytes() > OFFSCREEN_TEXTURE_BUDGET_BYTES) {
        // Le budget : tout rendre, et ne recharger que ce que la prochaine carte demande.
        kept->release();
    }
    return kept->ensureResources(_rhi.get()) ? kept.get() : nullptr;
}

bool OffscreenRhi::ensureTarget(QSize size) {
    if (_target && _texture && _texture->pixelSize() == size) {
        return true;
    }
    _target.reset();
    _pass.reset();
    _texture.reset(_rhi->newTexture(QRhiTexture::RGBA8, size, 1,
                                    QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    // Le tampon de profondeur (LOT-1003) : les maillages d'un lieu s'y departagent, et ses images
    // s'y comparent. Une carte sans volume ne le lit ni ne l'ecrit.
    _depth.reset(_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size));
    if (!_texture->create() || !_depth->create()) {
        _texture.reset();
        _depth.reset();
        return false;
    }
    QRhiTextureRenderTargetDescription description{{_texture.get()}};
    description.setDepthStencilBuffer(_depth.get());
    _target.reset(_rhi->newTextureRenderTarget(description));
    _pass.reset(_target->newCompatibleRenderPassDescriptor());
    _target->setRenderPassDescriptor(_pass.get());
    if (!_target->create()) {
        _target.reset();
        _pass.reset();
        _texture.reset();
        _depth.reset();
        return false;
    }
    return true;
}

QImage OffscreenRhi::render(WorldSceneRenderer& renderer, QSize size, const WorldFraming& framing,
                            const QColor& clear, int tileSide) {
    if (size.isEmpty() || framing.pixelsPerUnit <= 0.0F || !renderer.ensureResources(_rhi.get())) {
        return {};
    }
    // Toutes les tuiles ont la taille de la premiere : celles du bord droit et du bas depassent de
    // l'image, et seule leur part utile est recopiee. Une image qui tient dans une tuile se rend
    // d'un coup, dans une cible a sa taille.
    const int side = std::max(1, tileSide);
    const QSize tile(std::min(size.width(), side), std::min(size.height(), side));
    if (!ensureTarget(tile)) {
        GRAPHICS_LOG_WARNING("Rendu hors ecran : la cible de rendu ne se cree pas.");
        return {};
    }
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    if (image.isNull()) {
        return {};
    }
    // Le fond, premultiplie comme tout ce que le pipeline ecrit.
    const float alpha = clear.alphaF();
    const std::array<float, 4> background = {clear.redF() * alpha, clear.greenF() * alpha,
                                             clear.blueF() * alpha, alpha};
    const std::optional<WorldFraming> previous = renderer.framing();
    const auto scale = static_cast<double>(framing.pixelsPerUnit);
    bool complete = true;
    for (int top = 0; top < size.height() && complete; top += tile.height()) {
        for (int left = 0; left < size.width() && complete; left += tile.width()) {
            // Le centre de la tuile : celui de l'image, decale de l'ecart entre leurs milieux.
            const double shiftX = (left + (tile.width() / 2.0)) - (size.width() / 2.0);
            const double shiftY = (top + (tile.height() / 2.0)) - (size.height() / 2.0);
            renderer.setFraming(WorldFraming{
                .center =
                    {static_cast<float>(static_cast<double>(framing.center.x) + (shiftX / scale)),
                     static_cast<float>(static_cast<double>(framing.center.y) + (shiftY / scale))},
                .pixelsPerUnit = framing.pixelsPerUnit});

            QRhiCommandBuffer* commands = nullptr;
            if (_rhi->beginOffscreenFrame(&commands) != QRhi::FrameOpSuccess) {
                complete = false;
                break;
            }
            renderer.render(commands, _target.get(), background.data());
            QRhiReadbackResult readback;
            QRhiResourceUpdateBatch* const batch = _rhi->nextResourceUpdateBatch();
            batch->readBackTexture({_texture.get()}, &readback);
            commands->resourceUpdate(batch);
            if (_rhi->endOffscreenFrame() != QRhi::FrameOpSuccess || readback.pixelSize != tile) {
                complete = false;
                break;
            }
            const int columns = std::min(tile.width(), size.width() - left);
            const int rows = std::min(tile.height(), size.height() - top);
            const char* const pixels = readback.data.constData();
            for (int row = 0; row < rows; ++row) {
                std::memcpy(
                    image.scanLine(top + row) + (static_cast<qsizetype>(left) * PIXEL_BYTES),
                    pixels + (static_cast<qsizetype>(row) * tile.width() * PIXEL_BYTES),
                    static_cast<std::size_t>(columns) * PIXEL_BYTES);
            }
        }
    }
    renderer.setFraming(previous);
    return complete ? image : QImage{};
}

}  // namespace hmi
