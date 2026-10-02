// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/SceneSurface.h"

#include <array>
#include <utility>

#include <rhi/qrhi.h>

namespace hmi {

SceneSurface::SceneSurface(std::filesystem::path assetsDirectory, QWidget* parent)
    : QRhiWidget(parent), _renderer(std::move(assetsDirectory)), _clear(Qt::black) {
    // Les gestes sont ceux du canevas, au-dessus : la surface ne prend ni souris ni clavier.
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
}

SceneSurface::~SceneSurface() {
    // Les ressources appartiennent à l'interface de la fenêtre, encore vivante ici.
    _renderer.release();
}

void SceneSurface::setClearColor(const QColor& color) {
    if (color != _clear) {
        _clear = color;
        update();
    }
}

void SceneSurface::setBlank(bool blank) {
    if (blank != _blank) {
        _blank = blank;
        update();
    }
}

core::Rect SceneSurface::paintedBounds(const core::Rect& base) {
    return _renderer.paintedBounds(base);
}

void SceneSurface::initialize(QRhiCommandBuffer* /*commandBuffer*/) {
    // Appelé aussi à chaque changement de taille : seule une interface neuve est un événement.
    if (_renderer.created() && _renderer.rhi() == rhi()) {
        return;
    }
    if (_renderer.ensureResources(rhi())) {
        emit resourcesChanged();
    }
}

void SceneSurface::render(QRhiCommandBuffer* commandBuffer) {
    const std::array<float, 4> clear = {_clear.redF(), _clear.greenF(), _clear.blueF(), 1.0F};
    if (_blank || !_renderer.created()) {
        commandBuffer->beginPass(renderTarget(), _clear, {1.0F, 0});
        commandBuffer->endPass();
        return;
    }
    _renderer.render(commandBuffer, renderTarget(), clear.data());
}

void SceneSurface::releaseResources() {
    _renderer.release();
}

}  // namespace hmi
