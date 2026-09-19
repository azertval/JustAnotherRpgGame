// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/ArenaViewportItem.h"

#include <QQuickWindow>
#include <algorithm>
#include <array>
#include <chrono>
#include <optional>

#include <rhi/qrhi.h>

#include "Core/Combat/IsoProjection.h"
#include "HMI/Graphics/ArenaSceneComposer.h"
#include "HMI/Graphics/ArenaSceneRenderer.h"
#include "HMI/Graphics/Camera2D.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"

namespace hmi {
namespace {

/// Au-delà, une image en retard (fenêtre déplacée, point d'arrêt) ne fait pas sauter l'animation.
constexpr float MAXIMUM_FRAME_SECONDS = 0.25F;

/**
 * @brief Le peintre, côté **fil de rendu**.
 *
 * Il ne partage aucun état avec l'élément : tout ce dont il a besoin lui est remis par copie dans
 * `synchronize()`. Qt Quick le détruit sur le fil de rendu quand le graphe de scène de l'élément
 * est libéré (élément retiré, fenêtre changée, graphe invalidé), l'interface QRhi encore vivante :
 * le destructeur de `ArenaSceneRenderer` libère alors tout, dans l'ordre.
 */
class ArenaViewportRenderer : public QQuickRhiItemRenderer {
public:
    ArenaViewportRenderer() : _arena(executableDirectory() / "Assets" / "Coliseum", true) {}

    void initialize(QRhiCommandBuffer* commandBuffer) override;
    void synchronize(QQuickRhiItem* item) override;
    void render(QRhiCommandBuffer* commandBuffer) override;

private:
    using Clock = std::chrono::steady_clock;

    ArenaSceneRenderer _arena;
    /// Numéro de la scène copiée : 0 tant qu'aucun instantané n'a été pris.
    quint64 _sceneRevision = 0;
    QColor _clearColor;
    Clock::time_point _previousFrame = Clock::now();
};

void ArenaViewportRenderer::initialize(QRhiCommandBuffer* /*commandBuffer*/) {
    // Appelée à chaque synchronisation où la texture d'appui a pu changer — souvent pour une simple
    // taille. `ensureResources` ne recrée que si l'interface QRhi n'est plus la même.
    const QRhi* const previous = _arena.rhi();
    if (_arena.ensureResources(rhi()) && previous != rhi()) {
        HMI_LOG_INFO(std::string("Viewport de l'arene : interface de rendu QRhi initialisee (") +
                     rhi()->backendName() + ").");
    }
}

void ArenaViewportRenderer::synchronize(QQuickRhiItem* item) {
    // Le SEUL instant où les deux fils se parlent : le fil graphique est bloqué. Rien de ce qui est
    // lu ici n'est gardé par référence.
    // Le moteur ne passe que l'élément qui a créé ce rendu (createRenderer) : qobject_cast ne
    // peut échouer, et reste une vérification bon marché plutôt qu'un transtypage aveugle.
    auto* const viewport = qobject_cast<ArenaViewportItem*>(item);
    if (viewport == nullptr) {
        return;
    }
    _clearColor = viewport->clearColor();

    if (viewport->sceneRevision() == _sceneRevision) {
        return;
    }
    _sceneRevision = viewport->sceneRevision();
    const ArenaModel* const model = viewport->model();
    const core::ArenaSession* const session = model != nullptr ? model->session() : nullptr;
    _arena.setSnapshot(session != nullptr ? snapshotArenaScene(*session) : ArenaSceneSnapshot{});
}

void ArenaViewportRenderer::render(QRhiCommandBuffer* commandBuffer) {
    const Clock::time_point now = Clock::now();
    const float elapsed =
        std::min(std::chrono::duration<float>(now - _previousFrame).count(), MAXIMUM_FRAME_SECONDS);
    _previousFrame = now;

    const std::array<float, 4> clear = {_clearColor.redF(), _clearColor.greenF(),
                                        _clearColor.blueF(), 1.0F};
    _arena.render(commandBuffer, renderTarget(), elapsed, clear.data());

    // Des figurines à l'écran : elles respirent, l'image suivante est demandée. Sans elles, la
    // surface ne se redessine qu'à un changement de scène, de taille ou de couleur.
    if (_arena.animating()) {
        update();
    }
}

}  // namespace

struct ArenaViewportItem::Framing {
    core::IsoProjection projection{0, 0};
    Camera2D camera;
    /// Pixels de texture par unité d'élément, sur chaque axe.
    qreal pixelsPerItemX = 1.0;
    qreal pixelsPerItemY = 1.0;
};

ArenaViewportItem::ArenaViewportItem(QQuickItem* parent) : QQuickRhiItem(parent) {
    // La texture d'appui suit la taille et le ratio de l'écran : c'est sa taille que le rendu
    // cadre.
    connect(this, &QQuickRhiItem::effectiveColorBufferSizeChanged, this,
            &ArenaViewportItem::framingChanged);
    connect(this, &QQuickItem::widthChanged, this, &ArenaViewportItem::framingChanged);
    connect(this, &QQuickItem::heightChanged, this, &ArenaViewportItem::framingChanged);
}

ArenaViewportItem::Framing ArenaViewportItem::framing() const {
    const ArenaModel* const model = _model.data();
    core::IsoProjection projection(model != nullptr ? model->gridColumns() : 0,
                                   model != nullptr ? model->gridRows() : 0,
                                   core::ARENA_TILE_WIDTH_UNITS, 42.0F / 68.0F);
    // La taille réelle de la texture une fois le rendu passé ; avant, celle qu'il prendra.
    QSize pixels = effectiveColorBufferSize();
    if (pixels.isEmpty()) {
        const qreal ratio = window() != nullptr ? window()->effectiveDevicePixelRatio() : 1.0;
        pixels = QSize(qRound(width() * ratio), qRound(height() * ratio));
    }
    Camera2D camera = arenaCamera(projection, pixels.width(), pixels.height());
    return Framing{
        .projection = projection,
        .camera = camera,
        .pixelsPerItemX = width() > 0.0 ? std::max(1, pixels.width()) / width() : 1.0,
        .pixelsPerItemY = height() > 0.0 ? std::max(1, pixels.height()) / height() : 1.0};
}

qreal ArenaViewportItem::tileWidth() const {
    const Framing f = framing();
    const core::Vector2 left = f.camera.worldToScreen({0.0F, 0.0F});
    const core::Vector2 right = f.camera.worldToScreen({f.projection.tileWidth(), 0.0F});
    return (right.x - left.x) / f.pixelsPerItemX;
}

qreal ArenaViewportItem::tileHeight() const {
    const Framing f = framing();
    const core::Vector2 top = f.camera.worldToScreen({0.0F, 0.0F});
    const core::Vector2 bottom = f.camera.worldToScreen({0.0F, f.projection.tileHeight()});
    return (bottom.y - top.y) / f.pixelsPerItemY;
}

qreal ArenaViewportItem::originX() const {
    const Framing f = framing();
    return f.camera.worldToScreen(f.projection.origin()).x / f.pixelsPerItemX;
}

qreal ArenaViewportItem::originY() const {
    const Framing f = framing();
    return f.camera.worldToScreen(f.projection.origin()).y / f.pixelsPerItemY;
}

QPoint ArenaViewportItem::cellAt(qreal x, qreal y) const {
    const Framing f = framing();
    const core::Vector2 world = f.camera.screenToWorld(
        {static_cast<float>(x * f.pixelsPerItemX), static_cast<float>(y * f.pixelsPerItemY)});
    const std::optional<core::GridPosition> cell = f.projection.worldToTile(world);
    return cell.has_value() ? QPoint(cell->column, cell->row) : QPoint(-1, -1);
}

void ArenaViewportItem::setModel(ArenaModel* model) {
    if (_model == model) {
        return;
    }
    disconnect(_modelChangedConnection);
    disconnect(_modelGridConnection);
    disconnect(_modelDestroyedConnection);
    _model = model;
    if (model != nullptr) {
        _modelGridConnection =
            connect(model, &ArenaModel::changed, this, &ArenaViewportItem::framingChanged);
        // `combatSceneChanged`, pas `changed` : un geste de composition (enrôler, retirer, marquer,
        // graine, IA) ne mute encore aucune grille, et ne doit pas faire reprendre un instantané.
        _modelChangedConnection = connect(model, &ArenaModel::combatSceneChanged, this,
                                          &ArenaViewportItem::invalidateScene);
        // Le QPointer se vide seul ; il reste à redessiner une scène vide.
        _modelDestroyedConnection =
            connect(model, &QObject::destroyed, this, &ArenaViewportItem::invalidateScene);
    }
    invalidateScene();
    emit modelChanged();
    emit framingChanged();
}

void ArenaViewportItem::setClearColor(const QColor& color) {
    if (_clearColor == color) {
        return;
    }
    _clearColor = color;
    emit clearColorChanged();
    update();  // sans quoi la couleur ne parvient au fil de rendu qu'à la prochaine image demandée.
}

void ArenaViewportItem::invalidateScene() {
    ++_sceneRevision;
    update();
}

QQuickRhiItemRenderer* ArenaViewportItem::createRenderer() {
    return new ArenaViewportRenderer;
}

}  // namespace hmi
