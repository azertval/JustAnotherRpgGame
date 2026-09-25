// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/WorldViewportItem.h"

#include <QQuickWindow>
#include <algorithm>
#include <array>
#include <optional>

#include <rhi/qrhi.h>

#include "Core/Combat/IsoProjection.h"
#include "HMI/Graphics/Camera2D.h"
#include "HMI/Graphics/WorldSceneRenderer.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"

namespace hmi {
namespace {

// Le peintre du lieu, côté **fil de rendu**.
//
// Il ne partage aucun état avec l'élément : tout lui est remis par copie dans `synchronize()`.
class WorldViewportRenderer : public QQuickRhiItemRenderer {
public:
    WorldViewportRenderer() : _world(dataDirectory() / "Assets") {}

    void initialize(QRhiCommandBuffer* commandBuffer) override;
    void synchronize(QQuickRhiItem* item) override;
    void render(QRhiCommandBuffer* commandBuffer) override;

private:
    WorldSceneRenderer _world;
    // Numéros de la carte et des figurines reprises : 0 tant que rien n'a été pris.
    quint64 _sceneRevision = 0;
    quint64 _figuresRevision = 0;
    QColor _clearColor;
};

void WorldViewportRenderer::initialize(QRhiCommandBuffer* /*commandBuffer*/) {
    const QRhi* const previous = _world.rhi();
    if (_world.ensureResources(rhi()) && previous != rhi()) {
        HMI_LOG_INFO(std::string("Viewport du lieu : interface de rendu QRhi initialisee (") +
                     rhi()->backendName() + ").");
    }
}

void WorldViewportRenderer::synchronize(QQuickRhiItem* item) {
    auto* const viewport = qobject_cast<WorldViewportItem*>(item);
    if (viewport == nullptr) {
        return;
    }
    _clearColor = viewport->clearColor();

    const WorldModel* const model = viewport->model();
    if (model == nullptr) {
        _sceneRevision = 0;
        _figuresRevision = 0;
        _world.setSnapshot(WorldSceneSnapshot{});
        return;
    }
    // Le point suivi traverse à chaque image : la caméra suit le héros entre deux changements de
    // scène, et c'est une paire de flottants, pas une scène à recomposer.
    _world.setFocus(
        {static_cast<float>(model->heroColumn()), static_cast<float>(model->heroRow())});
    // La carte : un pointeur partagé, pris seulement quand elle a changé — jamais recopiée.
    if (model->sceneRevision() != _sceneRevision) {
        _sceneRevision = model->sceneRevision();
        _world.setScene(model->scene());
    }
    // Les figurines : quelques valeurs, prises à chaque pas qui les change.
    if (model->figuresRevision() != _figuresRevision) {
        _figuresRevision = model->figuresRevision();
        _world.setFigures(model->figures());
    }
}

void WorldViewportRenderer::render(QRhiCommandBuffer* commandBuffer) {
    const std::array<float, 4> clear = {_clearColor.redF(), _clearColor.greenF(),
                                        _clearColor.blueF(), 1.0F};
    _world.render(commandBuffer, renderTarget(), clear.data());
}

}  // namespace

struct WorldViewportItem::Framing {
    core::IsoProjection projection{0, 0};
    Camera2D camera;
    // Pixels de texture par unité d'élément, sur chaque axe.
    qreal pixelsPerItemX = 1.0;
    qreal pixelsPerItemY = 1.0;
};

WorldViewportItem::WorldViewportItem(QQuickItem* parent) : QQuickRhiItem(parent) {
    connect(this, &QQuickRhiItem::effectiveColorBufferSizeChanged, this,
            &WorldViewportItem::framingChanged);
    connect(this, &QQuickItem::widthChanged, this, &WorldViewportItem::framingChanged);
    connect(this, &QQuickItem::heightChanged, this, &WorldViewportItem::framingChanged);
}

WorldViewportItem::Framing WorldViewportItem::framing() const {
    const WorldModel* const model = _model.data();
    core::IsoProjection projection(
        model != nullptr ? model->columns() : 0, model != nullptr ? model->rows() : 0,
        core::ARENA_TILE_WIDTH_UNITS,
        model != nullptr ? model->diamondRatio() : core::ARENA_DIAMOND_RATIO);
    QSize pixels = effectiveColorBufferSize();
    if (pixels.isEmpty()) {
        const qreal ratio = window() != nullptr ? window()->effectiveDevicePixelRatio() : 1.0;
        pixels = QSize(qRound(width() * ratio), qRound(height() * ratio));
    }
    const core::Vector2 focus =
        model != nullptr ? projection.gridToWorld({static_cast<float>(model->heroColumn()),
                                                   static_cast<float>(model->heroRow())})
                         : core::Vector2{};
    return Framing{
        .projection = projection,
        .camera = worldCamera(projection, focus, pixels.width(), pixels.height()),
        .pixelsPerItemX = width() > 0.0 ? std::max(1, pixels.width()) / width() : 1.0,
        .pixelsPerItemY = height() > 0.0 ? std::max(1, pixels.height()) / height() : 1.0};
}

qreal WorldViewportItem::tileWidth() const {
    const Framing f = framing();
    const core::Vector2 left = f.camera.worldToScreen({0.0F, 0.0F});
    const core::Vector2 right = f.camera.worldToScreen({f.projection.tileWidth(), 0.0F});
    return (right.x - left.x) / f.pixelsPerItemX;
}

qreal WorldViewportItem::tileHeight() const {
    const Framing f = framing();
    const core::Vector2 top = f.camera.worldToScreen({0.0F, 0.0F});
    const core::Vector2 bottom = f.camera.worldToScreen({0.0F, f.projection.tileHeight()});
    return (bottom.y - top.y) / f.pixelsPerItemY;
}

qreal WorldViewportItem::originX() const {
    const Framing f = framing();
    return f.camera.worldToScreen(f.projection.origin()).x / f.pixelsPerItemX;
}

qreal WorldViewportItem::originY() const {
    const Framing f = framing();
    return f.camera.worldToScreen(f.projection.origin()).y / f.pixelsPerItemY;
}

QPoint WorldViewportItem::cellAt(qreal x, qreal y) const {
    const Framing f = framing();
    const core::Vector2 world = f.camera.screenToWorld(
        {static_cast<float>(x * f.pixelsPerItemX), static_cast<float>(y * f.pixelsPerItemY)});
    const std::optional<core::GridPosition> cell = f.projection.worldToTile(world);
    return cell.has_value() ? QPoint(cell->column, cell->row) : QPoint(-1, -1);
}

void WorldViewportItem::setModel(WorldModel* model) {
    if (_model == model) {
        return;
    }
    disconnect(_modelChangedConnection);
    disconnect(_modelMovedConnection);
    disconnect(_modelFiguresConnection);
    disconnect(_modelDestroyedConnection);
    _model = model;
    if (model != nullptr) {
        _modelChangedConnection =
            connect(model, &WorldModel::changed, this, &WorldViewportItem::onSceneChanged);
        // Le héros bouge à chaque pas : la caméra suit, et la scène se redessine.
        _modelMovedConnection =
            connect(model, &WorldModel::heroMoved, this, &WorldViewportItem::onSceneChanged);
        _modelFiguresConnection =
            connect(model, &WorldModel::figuresChanged, this, &WorldViewportItem::onSceneChanged);
        // Le QPointer se vide seul ; il reste à redessiner une scène vide.
        _modelDestroyedConnection =
            connect(model, &QObject::destroyed, this, &WorldViewportItem::onSceneChanged);
    }
    onSceneChanged();
    emit modelChanged();
}

void WorldViewportItem::setClearColor(const QColor& color) {
    if (_clearColor == color) {
        return;
    }
    _clearColor = color;
    emit clearColorChanged();
    update();
}

void WorldViewportItem::onSceneChanged() {
    emit framingChanged();
    update();
}

QQuickRhiItemRenderer* WorldViewportItem::createRenderer() {
    return new WorldViewportRenderer;
}

}  // namespace hmi
