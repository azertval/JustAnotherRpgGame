// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/AssetGalleryItem.h"

#include <QQuickWindow>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <set>
#include <utility>

#include <rhi/qrhi.h>

#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"

namespace hmi {
namespace {

constexpr float MAXIMUM_FRAME_SECONDS = 0.25F;
constexpr qreal MINIMUM_ZOOM = 0.25;
constexpr qreal MAXIMUM_ZOOM = 8.0;
/// Une image toutes les 16 ms tant que les animations jouent.
constexpr int CLOCK_INTERVAL_MS = 16;

QString decimal(double value) {
    return QString::number(value, 'g', 3).replace(u'.', u',');
}

/// Bouclée, jouée une fois puis tenue, ou « — » pour une image fixe.
QString loopText(const AssetGalleryEntry& entry) {
    if (entry.frameCount() <= 1) {
        return QStringLiteral("—");
    }
    if (entry.loop) {
        return QStringLiteral("oui");
    }
    return QStringLiteral("non (tenue %1 s)").arg(decimal(ASSET_GALLERY_ONE_SHOT_HOLD_SECONDS));
}

QString visibilityText(AssetGalleryVisibility visibility) {
    switch (visibility) {
        case AssetGalleryVisibility::Drawn:
            return QStringLiteral("dessiné");
        case AssetGalleryVisibility::Preloaded:
            return QStringLiteral("préchargé");
        default:
            return QStringLiteral("déchargé");
    }
}

class AssetGalleryViewportRenderer : public QQuickRhiItemRenderer {
public:
    AssetGalleryViewportRenderer() : _gallery(executableDirectory() / "Assets") {}

    void initialize(QRhiCommandBuffer* /*commandBuffer*/) override {
        const QRhi* const previous = _gallery.rhi();
        if (_gallery.ensureResources(rhi()) && previous != rhi()) {
            HMI_LOG_INFO(std::string("Galerie des assets : interface QRhi initialisee (") +
                         rhi()->backendName() + ").");
        }
    }

    void synchronize(QQuickRhiItem* item) override {
        // Le seul instant où les deux fils se parlent : tout est copié en valeurs.
        // Le moteur ne passe que l'élément qui a créé ce rendu (createRenderer) : qobject_cast ne
        // peut échouer, et reste une vérification bon marché plutôt qu'un transtypage aveugle.
        auto* const gallery = qobject_cast<AssetGalleryItem*>(item);
        if (gallery == nullptr) {
            return;
        }
        _clearColor = gallery->clearColor();
        const int pixelWidth = gallery->effectiveColorBufferSize().width();
        const qreal pixelsPerItem =
            gallery->width() > 0.0 && pixelWidth > 0 ? pixelWidth / gallery->width() : 1.0;
        AssetGalleryFrame frame = gallery->frameFor(static_cast<float>(pixelsPerItem));
        _wantedCount = frame.wanted.size();
        _gallery.setFrame(std::move(frame));
    }

    void render(QRhiCommandBuffer* commandBuffer) override {
        const Clock::time_point now = Clock::now();
        const float elapsed =
            std::min(std::chrono::duration<float>(now - _previous).count(), MAXIMUM_FRAME_SECONDS);
        _previous = now;
        const std::array<float, 4> clear = {_clearColor.redF(), _clearColor.greenF(),
                                            _clearColor.blueF(), 1.0F};
        _gallery.render(commandBuffer, renderTarget(), elapsed, clear.data());
        // Des chargements étalés, ou des textures qui attendent leur libération : l'image suivante
        // est demandée. L'horloge de l'item s'occupe des animations.
        if (_gallery.loading() || _gallery.cachedTextureCount() > _wantedCount) {
            update();
        }
    }

private:
    using Clock = std::chrono::steady_clock;

    AssetGalleryRenderer _gallery;
    QColor _clearColor;
    /// Textures voulues à la dernière synchronisation.
    std::size_t _wantedCount = 0;
    Clock::time_point _previous = Clock::now();
};

}  // namespace

AssetGalleryItem::AssetGalleryItem(QQuickItem* parent) : QQuickRhiItem(parent) {
    const std::filesystem::path root = executableDirectory() / "Assets";
    _catalog = AssetGalleryCatalog::load(root);
    _layout = layoutAssetGallery(_catalog);
    for (const std::string& error : _catalog.errors) {
        _errors.push_back(QString::fromStdString(error));
        HMI_LOG_WARNING("Galerie des assets : " + error);
    }
    // EX-CNT-042 : un asset livré que la galerie ne montre pas se dit au lancement, pas seulement
    // en intégration continue.
    for (const std::string& path : assetGalleryUnlisted(root, _catalog)) {
        const std::string message = path + " : image livree absente de la galerie";
        _errors.push_back(QString::fromStdString(message));
        HMI_LOG_WARNING("Galerie des assets : " + message);
    }
    HMI_LOG_INFO("Galerie des assets : " + std::to_string(_layout.blocs.size()) + " formes, " +
                 std::to_string(_catalog.families.size()) + " familles.");
    if (!_layout.blocs.empty()) {
        _selectedIndex = 0;
    }

    _clock.setInterval(CLOCK_INTERVAL_MS);
    connect(&_clock, &QTimer::timeout, this, &AssetGalleryItem::tick);
    _elapsed.start();
    _clock.start();

    connect(this, &QQuickItem::widthChanged, this, &AssetGalleryItem::refreshView);
    connect(this, &QQuickItem::heightChanged, this, &AssetGalleryItem::refreshView);
    refreshView();
}

const AssetGalleryEntry& AssetGalleryItem::entryOf(const AssetGalleryBloc& bloc) const {
    return _catalog.families[static_cast<std::size_t>(bloc.family)]
        .entries[static_cast<std::size_t>(bloc.entry)];
}

AssetGalleryView AssetGalleryItem::view() const noexcept {
    return AssetGalleryView{
        .column = viewColumn(), .row = viewRow(), .columns = viewColumns(), .rows = viewRows()};
}

qreal AssetGalleryItem::viewColumn() const noexcept {
    return -_offsetX / cellSize();
}
qreal AssetGalleryItem::viewRow() const noexcept {
    return -_offsetY / cellSize();
}
qreal AssetGalleryItem::viewColumns() const noexcept {
    return width() / cellSize();
}
qreal AssetGalleryItem::viewRows() const noexcept {
    return height() / cellSize();
}

void AssetGalleryItem::refreshView() {
    const AssetGalleryView current = view();
    const qreal cell = cellSize();
    _drawnCount = 0;
    _preloadedCount = 0;
    _labels.clear();
    for (int index = 0; std::cmp_less(index, _layout.blocs.size()); ++index) {
        const AssetGalleryBloc& bloc = _layout.blocs[static_cast<std::size_t>(index)];
        switch (assetGalleryVisibility(bloc, current)) {
            case AssetGalleryVisibility::Drawn: {
                ++_drawnCount;
                const AssetGalleryEntry& entry = entryOf(bloc);
                QString meta = QStringLiteral("%1×%2").arg(entry.frameWidth).arg(entry.frameHeight);
                if (entry.frameCount() > 1) {
                    meta += QStringLiteral(" · %1 img · %2 s")
                                .arg(entry.frameCount())
                                .arg(decimal(entry.frameDuration));
                }
                _labels.push_back(
                    QVariantMap{{QStringLiteral("index"), index},
                                {QStringLiteral("x"), _offsetX + (bloc.column * cell)},
                                {QStringLiteral("y"), _offsetY + (bloc.row * cell)},
                                {QStringLiteral("width"), bloc.columns * cell},
                                {QStringLiteral("height"), bloc.rows * cell},
                                {QStringLiteral("title"),
                                 QString::fromStdString(entry.model + " · " + entry.form)},
                                {QStringLiteral("meta"), meta}});
                break;
            }
            case AssetGalleryVisibility::Preloaded:
                ++_preloadedCount;
                break;
            case AssetGalleryVisibility::Unloaded:
                break;
        }
    }
    emit viewChanged();
    // L'état de chargement de la forme sélectionnée suit la vue.
    emit selectionChanged();
    update();
}

void AssetGalleryItem::tick() {
    const double delta =
        std::min(static_cast<double>(_elapsed.restart()) / 1000.0, double{MAXIMUM_FRAME_SECONDS});
    if (!_playing) {
        return;
    }
    _seconds += delta * _speed;
    emit frameChanged();
    if (_drawnCount > 0) {
        update();
    }
}

void AssetGalleryItem::setZoom(qreal zoom) {
    zoomAt(zoom, width() / 2.0, height() / 2.0);
}

void AssetGalleryItem::setOffsetX(qreal offset) {
    if (qFuzzyCompare(_offsetX, offset)) {
        return;
    }
    _offsetX = offset;
    refreshView();
}

void AssetGalleryItem::setOffsetY(qreal offset) {
    if (qFuzzyCompare(_offsetY, offset)) {
        return;
    }
    _offsetY = offset;
    refreshView();
}

void AssetGalleryItem::setShowGrid(bool show) {
    if (_showGrid != show) {
        _showGrid = show;
        emit displayChanged();
        update();
    }
}

void AssetGalleryItem::setShowFootprint(bool show) {
    if (_showFootprint != show) {
        _showFootprint = show;
        emit displayChanged();
        update();
    }
}

void AssetGalleryItem::setPlaying(bool playing) {
    if (_playing != playing) {
        _playing = playing;
        _elapsed.restart();
        emit playbackChanged();
    }
}

void AssetGalleryItem::setSpeed(qreal speed) {
    speed = std::max(0.0, speed);
    if (!qFuzzyCompare(_speed, speed)) {
        _speed = speed;
        emit playbackChanged();
    }
}

void AssetGalleryItem::setClearColor(const QColor& color) {
    if (_clearColor != color) {
        _clearColor = color;
        emit displayChanged();
        update();
    }
}

QVariantMap AssetGalleryItem::selected() const {
    if (_selectedIndex < 0 || _selectedIndex >= blocCount()) {
        return {};
    }
    const AssetGalleryBloc& bloc = _layout.blocs[static_cast<std::size_t>(_selectedIndex)];
    const AssetGalleryEntry& entry = entryOf(bloc);
    const AssetGalleryVisibility visibility = assetGalleryVisibility(bloc, view());
    return QVariantMap{
        {QStringLiteral("family"), QString::fromStdString(entry.family)},
        {QStringLiteral("model"), QString::fromStdString(entry.model)},
        {QStringLiteral("form"), QString::fromStdString(entry.form)},
        {QStringLiteral("path"), QString::fromStdString(entry.path)},
        {QStringLiteral("frameSize"),
         QStringLiteral("%1×%2").arg(entry.frameWidth).arg(entry.frameHeight)},
        {QStringLiteral("frameCount"), entry.frameCount()},
        {QStringLiteral("duration"),
         entry.frameCount() > 1 ? QStringLiteral("%1 s (%2 s)")
                                      .arg(decimal(entry.frameDuration),
                                           decimal(entry.frameDuration * entry.frameCount()))
                                : QStringLiteral("—")},
        {QStringLiteral("loop"), loopText(entry)},
        {QStringLiteral("footprint"),
         QStringLiteral("%1×%2").arg(entry.footprintColumns).arg(entry.footprintRows)},
        {QStringLiteral("anchor"),
         entry.anchorX >= 0 ? QStringLiteral("%1, %2").arg(entry.anchorX).arg(entry.anchorY)
                            : QStringLiteral("—")},
        {QStringLiteral("bloc"), QStringLiteral("%1×%2").arg(bloc.columns).arg(bloc.rows)},
        {QStringLiteral("state"), visibilityText(visibility)}};
}

int AssetGalleryItem::selectedFrame() const {
    if (_selectedIndex < 0 || _selectedIndex >= blocCount()) {
        return 0;
    }
    return assetGalleryFrameRank(entryOf(_layout.blocs[static_cast<std::size_t>(_selectedIndex)]),
                                 _seconds);
}

QVariantList AssetGalleryItem::siblings() const {
    QVariantList list;
    if (_selectedIndex < 0 || _selectedIndex >= blocCount()) {
        return list;
    }
    const AssetGalleryBloc& chosen = _layout.blocs[static_cast<std::size_t>(_selectedIndex)];
    const std::string& model = entryOf(chosen).model;
    for (int index = 0; index < blocCount(); ++index) {
        const AssetGalleryBloc& bloc = _layout.blocs[static_cast<std::size_t>(index)];
        if (bloc.family == chosen.family && entryOf(bloc).model == model) {
            list.push_back(
                QVariantMap{{QStringLiteral("index"), index},
                            {QStringLiteral("form"), QString::fromStdString(entryOf(bloc).form)}});
        }
    }
    return list;
}

QVariantList AssetGalleryItem::bands() const {
    QVariantList list;
    for (const AssetGalleryBand& band : _layout.bands) {
        const AssetGalleryFamily& family = _catalog.families[static_cast<std::size_t>(band.family)];
        list.push_back(
            QVariantMap{{QStringLiteral("title"), QString::fromStdString(family.title)},
                        {QStringLiteral("directory"), QString::fromStdString(family.directory)},
                        {QStringLiteral("count"), static_cast<int>(family.entries.size())},
                        {QStringLiteral("row"), band.row}});
    }
    return list;
}

QVariantList AssetGalleryItem::minimap() const {
    QVariantList list;
    for (const AssetGalleryBloc& bloc : _layout.blocs) {
        list.push_back(QVariantMap{{QStringLiteral("column"), bloc.column},
                                   {QStringLiteral("row"), bloc.row},
                                   {QStringLiteral("columns"), bloc.columns},
                                   {QStringLiteral("rows"), bloc.rows}});
    }
    return list;
}

void AssetGalleryItem::panBy(qreal dx, qreal dy) {
    _offsetX += dx;
    _offsetY += dy;
    refreshView();
}

void AssetGalleryItem::zoomAt(qreal zoom, qreal x, qreal y) {
    zoom = std::clamp(zoom, MINIMUM_ZOOM, MAXIMUM_ZOOM);
    if (qFuzzyCompare(_zoom, zoom)) {
        return;
    }
    const qreal ratio = zoom / _zoom;
    _offsetX = x - ((x - _offsetX) * ratio);
    _offsetY = y - ((y - _offsetY) * ratio);
    _zoom = zoom;
    refreshView();
}

int AssetGalleryItem::blocAt(qreal x, qreal y) const {
    const qreal column = (x - _offsetX) / cellSize();
    const qreal row = (y - _offsetY) / cellSize();
    for (int index = 0; index < blocCount(); ++index) {
        const AssetGalleryBloc& bloc = _layout.blocs[static_cast<std::size_t>(index)];
        if (column >= bloc.column && column < bloc.column + bloc.columns && row >= bloc.row &&
            row < bloc.row + bloc.rows) {
            return index;
        }
    }
    return -1;
}

void AssetGalleryItem::select(int index) {
    index = index >= 0 && index < blocCount() ? index : -1;
    if (_selectedIndex == index) {
        return;
    }
    _selectedIndex = index;
    emit selectionChanged();
    emit frameChanged();
    update();
}

void AssetGalleryItem::centerOn(int index) {
    if (index < 0 || index >= blocCount()) {
        return;
    }
    const AssetGalleryBloc& bloc = _layout.blocs[static_cast<std::size_t>(index)];
    centerOnCell(bloc.column + (bloc.columns / 2.0), bloc.row + (bloc.rows / 2.0));
}

void AssetGalleryItem::centerOnCell(qreal column, qreal row) {
    _offsetX = (width() / 2.0) - (column * cellSize());
    _offsetY = (height() / 2.0) - (row * cellSize());
    refreshView();
}

void AssetGalleryItem::showBand(int band) {
    if (band < 0 || std::cmp_greater_equal(band, _layout.bands.size())) {
        return;
    }
    _offsetX = cellSize() / 3.0;
    _offsetY = -_layout.bands[static_cast<std::size_t>(band)].row * cellSize();
    refreshView();
}

void AssetGalleryItem::step(int frames) {
    if (_selectedIndex < 0 || _selectedIndex >= blocCount()) {
        return;
    }
    setPlaying(false);
    const AssetGalleryEntry& entry =
        entryOf(_layout.blocs[static_cast<std::size_t>(_selectedIndex)]);
    if (entry.frameDuration <= 0.0) {
        return;
    }
    // Au milieu de l'image visée : un arrondi flottant ne retombe pas sur la précédente.
    const double current = std::floor(_seconds / entry.frameDuration);
    _seconds = std::max(0.0, (current + frames + 0.5) * entry.frameDuration);
    emit frameChanged();
    update();
}

AssetGalleryFrame AssetGalleryItem::frameFor(float pixelsPerItem) const {
    AssetGalleryFrame frame;
    frame.pixelScale = static_cast<float>(_zoom) * pixelsPerItem;
    frame.cellPixels = frame.pixelScale * static_cast<float>(ASSET_GALLERY_CELL_PIXELS);
    frame.showGrid = _showGrid;
    frame.showFootprint = _showFootprint;

    const AssetGalleryView current = view();
    std::set<std::string> wanted;
    for (int index = 0; index < blocCount(); ++index) {
        const AssetGalleryBloc& bloc = _layout.blocs[static_cast<std::size_t>(index)];
        const AssetGalleryVisibility visibility = assetGalleryVisibility(bloc, current);
        if (visibility == AssetGalleryVisibility::Unloaded) {
            continue;
        }
        const AssetGalleryEntry& entry = entryOf(bloc);
        wanted.insert(entry.path);
        if (visibility != AssetGalleryVisibility::Drawn) {
            continue;
        }
        const int rank = assetGalleryFrameRank(entry, _seconds);
        frame.drawn.push_back(AssetGalleryDrawnBloc{
            .path = entry.path,
            .x = std::round(static_cast<float>(_offsetX + (bloc.column * cellSize())) *
                            pixelsPerItem),
            .y = std::round(static_cast<float>(_offsetY + (bloc.row * cellSize())) * pixelsPerItem),
            .columns = bloc.columns,
            .rows = bloc.rows,
            .footprintColumn = bloc.footprintColumn,
            .footprintRow = bloc.footprintRow,
            .footprintColumns = std::max(1, entry.footprintColumns),
            .footprintRows = std::max(1, entry.footprintRows),
            .frameWidth = entry.frameWidth,
            .frameHeight = entry.frameHeight,
            .frameIndex = entry.frames.empty() ? 0 : entry.frames[static_cast<std::size_t>(rank)],
            .selected = index == _selectedIndex,
            .tilePixels = entry.tileWidthPixels()});
    }
    frame.wanted.assign(wanted.begin(), wanted.end());
    return frame;
}

QQuickRhiItemRenderer* AssetGalleryItem::createRenderer() {
    return new AssetGalleryViewportRenderer;
}

}  // namespace hmi
