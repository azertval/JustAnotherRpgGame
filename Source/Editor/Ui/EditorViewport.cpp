// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/EditorViewport.h"

#include <QEvent>
#include <QFont>
#include <QFontMetricsF>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPolygonF>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

#include "Core/Gameplay/Quest.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelWriter.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileTypeName.h"
#include "Core/Math/Vector2.h"
#include "Core/World/EntityPresence.h"
#include "Core/World/WorldTravel.h"
#include "Editor/Logic/DataRoot.h"
#include "Editor/Logic/EntityGesture.h"
#include "Editor/Logic/EntityReferences.h"
#include "Editor/Logic/EntityShapes.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/WorldState.h"
#include "Editor/Ui/DraftRenderer.h"
#include "Editor/Ui/SceneImages.h"
#include "Editor/Ui/ScenePainter.h"
#include "HMI/Game/WorldPlay.h"
#include "HMI/Graphics/Camera2D.h"
#include "HMI/Graphics/EntityMarkers.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"

namespace hmi {

namespace {

// Fond du canevas en édition (gris ardoise) et pendant l'essai (parchemin, la couleur du jeu) : les
// valeurs que portaient les jetons de la charte, retirés de l'éditeur (LOT-EDITOR-01).
const QColor EDIT_BACKGROUND(0x1e, 0x22, 0x2b);
const QColor PLAYTEST_BACKGROUND(0xd0, 0xc0, 0xa0);

/// Le vide laissé autour de ce qui est peint, en largeurs de case.
constexpr double FRAME_PADDING_TILES = 0.25;

/// Le losange entier de la carte d'un instantané, en unités monde.
[[nodiscard]] core::Rect snapshotRect(const WorldSceneSnapshot& snapshot) {
    const core::IsoProjection iso(snapshot.columns, snapshot.rows, core::ARENA_TILE_WIDTH_UNITS,
                                  snapshot.diamondRatio);
    return core::Rect{{0.0F, 0.0F}, iso.sceneSize()};
}

/// Un cran de molette agrandit ou réduit d'autant.
constexpr double ZOOM_STEP = 1.25;
/// Agrandissement maximal : une unité monde fait alors 8 × 16 pixels.
constexpr double MAX_PIXELS_PER_UNIT = 8.0 * Camera2D::PIXELS_PER_UNIT;
/// Cadence de l'essai : celle du jeu (60 images par seconde).
constexpr int PLAYTEST_FRAME_MS = 16;
/// Opacité du marqueur d'une entité que l'état de partie rend absente (`LOT-126`).
constexpr double ABSENT_ENTITY_OPACITY = 0.3;

[[nodiscard]] std::filesystem::path keybindingsPath() {
    return hmi::executableDirectory() / "Settings" / "keybindings.json";
}

[[nodiscard]] std::filesystem::path levelsDirectory() {
    return hmi::editorDataRoot() / "Levels";
}

[[nodiscard]] std::filesystem::path assetsDirectory() {
    return hmi::editorDataRoot() / "Assets";
}

/// Carte ouverte au lancement : la première carte du jeu.
constexpr const char* START_MAP_ID = "capital/arena-of-brave";

// Révision « jamais enregistrée » : celle d'un brouillon repris, qui reste modifié quoi qu'on
// fasse jusqu'à son enregistrement. Aucune révision réelle ne l'atteint.
constexpr std::uint64_t NEVER_SAVED = std::numeric_limits<std::uint64_t>::max();

/// @return L'identifiant de carte de @p path : son chemin sous `Levels/`, sans extension, en
///         barres obliques — celui qu'un portail écrit. Hors du dossier : le nom du fichier.
[[nodiscard]] std::string mapIdOf(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::path relative = std::filesystem::relative(path, levelsDirectory(), error);
    if (error || relative.empty() || *relative.begin() == "..") {
        relative = path.filename();
    }
    relative.replace_extension();
    return relative.generic_string();
}

/// Touches du déplacement de l'essai : celles du jeu (`GameView.qml`), azerty comme qwerty.
[[nodiscard]] bool isUpKey(int key) {
    return key == Qt::Key_Up || key == Qt::Key_W || key == Qt::Key_Z;
}
[[nodiscard]] bool isDownKey(int key) {
    return key == Qt::Key_Down || key == Qt::Key_S;
}
[[nodiscard]] bool isLeftKey(int key) {
    return key == Qt::Key_Left || key == Qt::Key_A || key == Qt::Key_Q;
}
[[nodiscard]] bool isRightKey(int key) {
    return key == Qt::Key_Right || key == Qt::Key_D;
}

[[nodiscard]] QPointF toQt(core::Vector2 point) {
    return {static_cast<double>(point.x), static_cast<double>(point.y)};
}

[[nodiscard]] QPolygonF diamondOf(const core::IsoProjection& projection, core::GridPosition cell) {
    const std::array<core::Vector2, 4> vertices = isoCellDiamond(projection, cell);
    return QPolygonF{{toQt(vertices[0]), toQt(vertices[1]), toQt(vertices[2]), toQt(vertices[3])}};
}

/// Le parallélogramme iso d'un rectangle de cases, bornes incluses.
[[nodiscard]] QPolygonF isoRegion(const core::IsoProjection& projection, core::GridPosition first,
                                  core::GridPosition last) {
    const auto at = [&](int column, int row) {
        return toQt(projection.gridToWorld({static_cast<float>(column), static_cast<float>(row)}));
    };
    return QPolygonF{{at(first.column, first.row), at(last.column + 1, first.row),
                      at(last.column + 1, last.row + 1), at(first.column, last.row + 1)}};
}

/// Un crayon d'une largeur en **pixels d'écran**, quel que soit l'agrandissement.
[[nodiscard]] QPen screenPen(const QColor& color, double width) {
    QPen pen(color, width);
    pen.setCosmetic(true);
    pen.setJoinStyle(Qt::MiterJoin);
    return pen;
}

[[nodiscard]] QColor withAlpha(QColor color, float alpha) {
    color.setAlphaF(std::clamp(alpha, 0.0F, 1.0F));
    return color;
}

/// Ajoute à @p snapshot les figurines de la formation de la rencontre @p selected, s'il y en a une.
void appendFormation(WorldSceneSnapshot& snapshot,
                     const std::vector<core::EncounterTerrain>& terrains, std::size_t selected,
                     const std::vector<std::string>& figures) {
    const auto terrain =
        std::ranges::find(terrains, selected, &core::EncounterTerrain::entityIndex);
    if (terrain != terrains.end()) {
        std::ranges::move(formationFigures(*terrain, figures),
                          std::back_inserter(snapshot.figures));
    }
}

/// La teinte d'une famille d'entités à forme : tirée de son type, stable d'une session à l'autre,
/// sans table par famille (LOT-EDITOR-05).
[[nodiscard]] QColor kindColor(const std::string& type) {
    std::uint32_t hash = 2166136261U;  // FNV-1a : court, et assez dispersé pour quelques familles.
    for (const char character : type) {
        hash = (hash ^ static_cast<unsigned char>(character)) * 16777619U;
    }
    return QColor::fromHsv(static_cast<int>(hash % 360U), 170, 240);
}

/// La police des étiquettes du canevas, en pixels d'écran.
[[nodiscard]] QFont labelFont(const QPainter& painter) {
    QFont font = painter.font();
    font.setPixelSize(11);
    return font;
}

/// Le cadre, en pixels d'écran, de l'étiquette @p text posée au-dessus de @p at (coordonnées du
/// monde).
[[nodiscard]] QRectF screenLabelBox(const QPainter& painter, QPointF at, const QString& text) {
    const QPointF device = painter.transform().map(at);
    const QRectF box =
        QFontMetricsF(labelFont(painter)).boundingRect(text).adjusted(-3.0, -1.0, 3.0, 1.0);
    return box.translated(device.x() - (box.width() / 2.0) - box.left(),
                          device.y() - box.height() - box.top());
}

/// Écrit @p text à @p at (coordonnées du monde), en pixels d'écran : une étiquette garde sa taille
/// quel que soit l'agrandissement.
void drawScreenLabel(QPainter& painter, QPointF at, const QString& text, const QColor& color) {
    const QRectF placed = screenLabelBox(painter, at, text);
    painter.save();
    painter.resetTransform();
    painter.setFont(labelFont(painter));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(13, 13, 13, 190));
    painter.drawRoundedRect(placed, 3.0, 3.0);
    painter.setPen(color);
    painter.drawText(placed, Qt::AlignCenter, text);
    painter.restore();
}

}  // namespace

/**
 * @brief L'élément unique de la scène : il délègue sa peinture au canevas (décision D2).
 *
 * `ItemUsesExtendedStyleOption` donne le rectangle exposé : le canevas ne peint que ce qui se voit.
 */
class EditorViewport::CanvasItem final : public QGraphicsItem {
public:
    explicit CanvasItem(EditorViewport& owner) : _owner(owner) {
        setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
    }

    [[nodiscard]] QRectF boundingRect() const override {
        return _bounds;
    }

    void setBounds(const QRectF& bounds) {
        if (bounds != _bounds) {
            prepareGeometryChange();
            _bounds = bounds;
        }
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* /*widget*/) override {
        _owner.paintCanvas(*painter, option->exposedRect);
    }

private:
    EditorViewport& _owner;
    QRectF _bounds;
};

EditorViewport::EditorViewport(StartContent content, QWidget* parent)
    : QGraphicsView(parent),
      _canvasScene(new QGraphicsScene(this)),
      _item(new CanvasItem(*this)),
      _images(SceneImages::shared(assetsDirectory())),
      _editorBindings(hmi::EditorKeyBindings::load(keybindingsPath())),
      _draft(core::LevelDraft::empty("New map", 24, 14)),
      _mapId(_draft.name()) {
    _flat = std::make_unique<DraftRenderer>(
        DraftTextures{.atlas = sceneImageHandle(&_images->atlas()),
                      .atlasWidth = _images->atlas().width(),
                      .atlasHeight = _images->atlas().height(),
                      .solid = SceneImages::solid(),
                      .marker = [images = _images.get()](const std::string& key) -> TextureHandle {
                          return sceneImageHandle(images->marker(key));
                      }});

    _canvasScene->addItem(_item);
    setScene(_canvasScene);
    setBackgroundBrush(EDIT_BACKGROUND);
    // Une seule peinture par image, de tout ce qui se voit : l'élément unique couvre la scène.
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setRenderHint(QPainter::SmoothPixmapTransform, false);
    setRenderHint(QPainter::Antialiasing, false);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);

    _playTimer.setInterval(PLAYTEST_FRAME_MS);
    connect(&_playTimer, &QTimer::timeout, this, [this] { stepPlaytest(); });

    if (content == StartContent::Blank) {
        return;  // un onglet neuf qui va recevoir une carte nommée (LOT-EDITOR-09).
    }
    // La première carte du jeu, comme brouillon. Échec récupérable : on garde le brouillon vierge.
    const std::filesystem::path startPath =
        levelsDirectory() / (std::string{START_MAP_ID} + ".json");
    if (!openLevel(startPath)) {
        HMI_LOG_WARNING("Editeur : echec du chargement de la carte de depart.");
        markDraftMutated();
    }
}

EditorViewport::~EditorViewport() {
    // L'élément appelle le canevas : il part avant que les membres ne meurent.
    _canvasScene->removeItem(_item);
    delete _item;
}

void EditorViewport::setTool(hmi::EditorTool tool) {
    if (_tool == tool) {
        return;
    }
    endPainting();
    _dragging = false;
    _tool = tool;
    if (paintsWithBrush(tool)) {
        _paintTool = tool;  // la pipette y revient.
    }
    if (tool != hmi::EditorTool::Measure && _measure) {
        _measure.reset();
        emit toolStateChanged();
    }
    _entityDrag.reset();
    if (_shapePainting) {
        _shapePainting = false;
        _draft.endGesture();
    }
    emit toolChanged(tool);
    invalidateScene();  // la formation de la rencontre ne se montre qu'à l'outil Entité.
}

// --- Cadrage
// --------------------------------------------------------------------------------------

core::IsoProjection EditorViewport::projection() const {
    return {_draft.tileMap().width(), _draft.tileMap().height(), core::ARENA_TILE_WIDTH_UNITS,
            _appearance.diamondRatio()};
}

QRectF EditorViewport::contentBounds() const {
    // Le cadre se mesure sur ce qui est peint (`hmi::composedSceneBounds`, `LOT-125`) : une pièce
    // haute n'est plus rognée par une marge d'un losange supposée assez grande.
    const auto framed = [](const core::Rect& bounds, double padding) {
        return QRectF(static_cast<double>(bounds.position.x) - padding,
                      static_cast<double>(bounds.position.y) - padding,
                      static_cast<double>(bounds.size.x) + (2 * padding),
                      static_cast<double>(bounds.size.y) + (2 * padding));
    };
    if (_play) {
        return framed(_playBounds, FRAME_PADDING_TILES * core::ARENA_TILE_WIDTH_UNITS);
    }
    if (_view == CanvasView::Flat) {
        return {-1.0, -1.0, static_cast<double>(_draft.tileMap().width()) + 2.0,
                static_cast<double>(_draft.tileMap().height()) + 2.0};
    }
    return framed(_isoBounds, FRAME_PADDING_TILES * projection().tileWidth());
}

void EditorViewport::refreshBounds() {
    if (!_play && _view != CanvasView::Flat) {
        ensureIsoScene();
    }
    const QRectF bounds = contentBounds();
    _item->setBounds(bounds);
    _canvasScene->setSceneRect(bounds);
    viewport()->update();
}

void EditorViewport::resetCamera() {
    if (!_play && _view != CanvasView::Flat) {
        ensureIsoScene();
    }
    fitInView(contentBounds(), Qt::KeepAspectRatio);
    _framed = false;
    emitZoomIfChanged();
    emit framingChanged();
}

float EditorViewport::zoom() const noexcept {
    return static_cast<float>(transform().m11()) / Camera2D::PIXELS_PER_UNIT;
}

void EditorViewport::emitZoomIfChanged() {
    const float current = zoom();
    if (current != _lastEmittedZoom) {
        _lastEmittedZoom = current;
        emit zoomChanged(current);
    }
}

void EditorViewport::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    // Tant que l'auteur n'a pas cadré lui-même, la carte entière reste dans la vue.
    if (!_framed && !_play) {
        resetCamera();
    }
    emit framingChanged();
}

void EditorViewport::scrollContentsBy(int dx, int dy) {
    QGraphicsView::scrollContentsBy(dx, dy);
    emit framingChanged();
}

void EditorViewport::setCanvasView(CanvasView view) {
    if (_view == view) {
        return;
    }
    _view = view;
    refreshBounds();
    if (!_play) {
        resetCamera();
    }
    emit canvasViewChanged(_view);
}

void EditorViewport::setSeeThroughRelief(bool enabled) {
    _seeThroughRelief = enabled;
    viewport()->update();
}

std::array<core::Vector2, 4> EditorViewport::visibleGridCorners() const {
    const QRect pixels = viewport()->rect();
    const std::array<QPointF, 4> corners = {
        mapToScene(pixels.topLeft()), mapToScene(pixels.topRight()),
        mapToScene(pixels.bottomRight()), mapToScene(pixels.bottomLeft())};
    std::array<core::Vector2, 4> grid{};
    const core::IsoProjection iso = projection();
    for (std::size_t index = 0; index < grid.size(); ++index) {
        const core::Vector2 world{static_cast<float>(corners[index].x()),
                                  static_cast<float>(corners[index].y())};
        grid[index] = _view == CanvasView::Flat ? world : iso.worldToGrid(world);
    }
    return grid;
}

void EditorViewport::centerOnGridPoint(core::Vector2 gridPoint) {
    if (_play) {
        return;
    }
    centerOn(toQt(_view == CanvasView::Flat ? gridPoint : projection().gridToWorld(gridPoint)));
    _framed = true;
    emit framingChanged();
}

void EditorViewport::revealCell(core::GridPosition cell) {
    _revealedCell = cell;
    centerOnGridPoint(
        {static_cast<float>(cell.column) + 0.5F, static_cast<float>(cell.row) + 0.5F});
    viewport()->update();
}

core::Vector2 EditorViewport::worldPosition(const QMouseEvent* event) const {
    const QPointF scenePoint = mapToScene(event->position().toPoint());
    return {static_cast<float>(scenePoint.x()), static_cast<float>(scenePoint.y())};
}

std::optional<core::GridPosition> EditorViewport::cellAt(const QMouseEvent* event) const {
    const core::Vector2 world = worldPosition(event);
    if (_view == CanvasView::Flat) {
        return pickFlatCell(world, _draft.tileMap().width(), _draft.tileMap().height());
    }
    return pickIsoCell(projection(), world);
}

core::GridPosition EditorViewport::clampedCell(const QMouseEvent* event) const {
    const core::Vector2 world = worldPosition(event);
    if (_view == CanvasView::Flat) {
        return clampedFlatCell(world, _draft.tileMap().width(), _draft.tileMap().height());
    }
    return clampedIsoCell(projection(), world);
}

// --- Peinture
// -------------------------------------------------------------------------------------

void EditorViewport::invalidateScene() {
    _isoSceneDirty = true;
    viewport()->update();
}

void EditorViewport::loadPlaceAssets(const std::string& place) {
    _appearancePlace = place;
    _placeAssetsLoaded = true;
    _appearance = PlaceAppearance{};
    _manifest.reset();
    if (place.empty()) {
        // Une maquette : ni pièce ni table, mais ses PNJ prennent les figurines du monde.
        _appearance = hmi::loadPlaceAssets(hmi::editorDataRoot(), place)
                          .appearance.value_or(PlaceAppearance{});
        return;
    }
    // Ce que --check lit, lu de la même façon : la table et le manifeste du lieu.
    PlaceAssets assets = hmi::loadPlaceAssets(hmi::editorDataRoot(), place);
    if (assets.appearance) {
        _appearance = std::move(*assets.appearance);
    } else {
        HMI_LOG_WARNING("Editeur : table d'apparence du lieu " + place + " illisible.");
    }
    if (assets.manifest) {
        _manifest = std::make_shared<const core::ScenePieceManifest>(std::move(*assets.manifest));
    } else {
        HMI_LOG_WARNING("Editeur : manifeste des pieces du lieu " + place + " illisible.");
    }
}

void EditorViewport::ensureIsoScene() {
    if (!_isoSceneDirty) {
        return;
    }
    const std::string place = scenePlaceOf(_draft.layers());
    if (!_placeAssetsLoaded || place != _appearancePlace) {
        loadPlaceAssets(place);
    }
    // Un brouillon remplacé (ouverture, reprise) repart sans manifeste : on le lui redonne.
    _draft.setPieceManifest(_manifest);
    _snapshot = canvasSnapshot(_draft, _appearance, _statePreview ? &_stateFlags : nullptr);
    // La formation de la rencontre sélectionnée, par ses figurines (LOT-EDITOR-05).
    if (_tool == hmi::EditorTool::Entity && _selectedEntity && _references != nullptr) {
        appendFormation(_snapshot, _terrains, *_selectedEntity, _references->figures);
    }
    _images->ensure(worldTexturePaths(_snapshot));
    _isoScene.clear();
    composeWorldScene(_isoScene, _snapshot,
                      core::IsoProjection(_snapshot.columns, _snapshot.rows,
                                          core::ARENA_TILE_WIDTH_UNITS, _snapshot.diamondRatio),
                      _images->textures());
    _isoScene.sort();
    _isoBounds = composedSceneBounds(_isoScene, snapshotRect(_snapshot));
    _isoSceneDirty = false;
}

void EditorViewport::paintCanvas(QPainter& painter, const QRectF& exposed) {
    if (_play) {
        paintPlaytest(painter, exposed);
    } else if (_view == CanvasView::Flat) {
        paintFlat(painter, exposed);
    } else {
        paintIso(painter, exposed);
    }
}

void EditorViewport::paintPlaytest(QPainter& painter, const QRectF& exposed) {
    const core::Rect visible{
        {static_cast<float>(exposed.x()), static_cast<float>(exposed.y())},
        {static_cast<float>(exposed.width()), static_cast<float>(exposed.height())}};
    paintComposedScene(painter, _playScene, visible);
}

void EditorViewport::paintFlat(QPainter& painter, const QRectF& exposed) {
    const core::Rect visible{
        {static_cast<float>(exposed.x()), static_cast<float>(exposed.y())},
        {static_cast<float>(exposed.width()), static_cast<float>(exposed.height())}};
    DraftEntityOverlay overlay;
    overlay.selectedEntity = _selectedEntity;
    overlay.terrains = &_terrains;
    overlay.showTerrain = _tool == hmi::EditorTool::Entity;
    _flat->setLayerView(_layerView);
    paintComposedScene(painter, _flat->compose(_draft, visible, _showGrid, highlight(), overlay),
                       visible);
    const CellRange flatCells{
        .firstColumn = std::max(0, static_cast<int>(exposed.left())),
        .firstRow = std::max(0, static_cast<int>(exposed.top())),
        .lastColumn = std::min(_draft.tileMap().width() - 1, static_cast<int>(exposed.right())),
        .lastRow = std::min(_draft.tileMap().height() - 1, static_cast<int>(exposed.bottom()))};
    if (!_activeLayer && hasVisualLayers()) {
        paintForcedMask(painter, flatCells, false);
    }
    paintZoneVerdict(painter, false);
    paintEntities(painter, flatCells, false);
    paintNotes(painter, flatCells, false);
    paintDragPreview(painter, false);
    paintMirrorAxis(painter, false);
    if (_hoverCell) {
        painter.setPen(screenPen(QColor(255, 236, 140), 2.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(QRectF(_hoverCell->column, _hoverCell->row, 1.0, 1.0));
    }
    if (_revealedCell) {
        painter.setPen(screenPen(QColor(255, 70, 200), 3.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(QRectF(_revealedCell->column, _revealedCell->row, 1.0, 1.0));
    }
}

void EditorViewport::paintIso(QPainter& painter, const QRectF& exposed) {
    ensureIsoScene();
    const core::IsoProjection iso = projection();
    const core::Rect visible{
        {static_cast<float>(exposed.x()), static_cast<float>(exposed.y())},
        {static_cast<float>(exposed.width()), static_cast<float>(exposed.height())}};
    const CellRange cells = isoCellsCovering(iso, visible);
    const IsoBandOpacity bands =
        isoBandOpacity(_draft.layers(), _layerView, _activeLayer, _seeThroughRelief);
    paintComposedScene(painter, _isoScene, visible,
                       [&bands](const ComposedQuad& quad) { return bandOpacity(bands, quad); });
    paintIsoOverlays(painter, cells, bands);
}

void EditorViewport::paintIsoOverlays(QPainter& painter, const CellRange& cells,
                                      const IsoBandOpacity& bands) {
    const core::IsoProjection iso = projection();
    if (cells.empty()) {
        return;
    }
    // Masque de collision : une teinte par catégorie de règle, comme la vue à plat.
    if (bands.collision > 0.0F) {
        paintIsoCollisionMask(painter, cells, bands.collision);
    }
    if (_showGrid) {
        paintIsoGrid(painter, cells);
    }
    // Aperçu du rectangle ou de la sélection.
    if (const auto zone = highlight()) {
        painter.setPen(screenPen(QColor(110, 190, 255), 1.0));
        painter.setBrush(QColor::fromRgbF(0.3F, 0.7F, 1.0F, 0.28F));
        painter.drawPolygon(isoRegion(iso, zone->first, zone->second));
    }
    // Terrain de la rencontre sélectionnée (outil Entité).
    if (_tool == hmi::EditorTool::Entity && _selectedEntity) {
        paintEncounterTerrain(painter);
    }
    paintZoneVerdict(painter, true);
    paintEntities(painter, cells, true);
    paintNotes(painter, cells, true);
    paintDragPreview(painter, true);
    paintMirrorAxis(painter, true);
    // La case survolée, par son losange : c'est elle que le prochain geste touchera.
    if (_hoverCell) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(screenPen(QColor(255, 236, 140), 2.0));
        painter.drawPolygon(diamondOf(iso, *_hoverCell));
    }
    // La case d'un constat, d'une couleur qu'aucun autre repère ne prend.
    if (_revealedCell) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(screenPen(QColor(255, 70, 200), 3.0));
        painter.drawPolygon(diamondOf(iso, *_revealedCell));
    }
}

void EditorViewport::paintIsoCollisionMask(QPainter& painter, const CellRange& cells,
                                           float opacity) {
    const core::IsoProjection iso = projection();
    painter.setPen(Qt::NoPen);
    const core::TileMap& map = _draft.tileMap();
    for (int row = cells.firstRow; row <= cells.lastRow; ++row) {
        for (int column = cells.firstColumn; column <= cells.lastColumn; ++column) {
            const core::TileType type = map.tile(column, row);
            QColor tint;
            if (core::isSolid(type)) {
                tint = QColor::fromRgbF(0.85F, 0.20F, 0.20F);
            } else if (type == core::TileType::Entry) {
                tint = QColor::fromRgbF(0.20F, 0.85F, 0.30F);
            } else {
                continue;
            }
            painter.setBrush(withAlpha(tint, opacity));
            painter.drawPolygon(diamondOf(iso, {.column = column, .row = row}));
        }
    }
    paintForcedMask(painter, cells, true);
}

void EditorViewport::paintIsoGrid(QPainter& painter, const CellRange& cells) {
    const core::IsoProjection iso = projection();
    painter.setPen(screenPen(QColor(255, 255, 255, 46), 1.0));
    const auto at = [&](int column, int row) {
        return toQt(iso.gridToWorld({static_cast<float>(column), static_cast<float>(row)}));
    };
    for (int column = cells.firstColumn; column <= cells.lastColumn + 1; ++column) {
        painter.drawLine(at(column, cells.firstRow), at(column, cells.lastRow + 1));
    }
    for (int row = cells.firstRow; row <= cells.lastRow + 1; ++row) {
        painter.drawLine(at(cells.firstColumn, row), at(cells.lastColumn + 1, row));
    }
}

void EditorViewport::paintEncounterTerrain(QPainter& painter) {
    const core::IsoProjection iso = projection();
    painter.setPen(Qt::NoPen);
    for (const core::EncounterTerrain& terrain : _terrains) {
        if (terrain.entityIndex != *_selectedEntity) {
            continue;
        }
        const bool narrow =
            std::ranges::any_of(terrain.issues, [](const core::TacticalIssue& issue) {
                return issue.code == core::TacticalIssueCode::AreaTooNarrow;
            });
        painter.setBrush(narrow ? QColor::fromRgbF(1.0F, 0.55F, 0.10F, 0.18F)
                                : QColor::fromRgbF(0.30F, 0.70F, 1.00F, 0.18F));
        for (const core::GridPosition& cell : terrain.area) {
            painter.drawPolygon(diamondOf(iso, cell));
        }
        for (const core::CombatantPlacement& placement : terrain.placements) {
            const bool refused =
                std::ranges::any_of(terrain.issues, [&placement](const core::TacticalIssue& issue) {
                    return issue.code != core::TacticalIssueCode::AreaTooNarrow &&
                           issue.cell == placement.position;
                });
            painter.setBrush(refused ? QColor::fromRgbF(0.95F, 0.20F, 0.20F, 0.55F)
                                     : QColor::fromRgbF(0.25F, 0.85F, 0.35F, 0.55F));
            painter.drawPolygon(diamondOf(iso, placement.position));
        }
    }
}

void EditorViewport::paintNotes(QPainter& painter, const CellRange& cells, bool iso) {
    // Une pastille ambre cerclée de sombre, en haut de la case : lisible sur tout sol.
    painter.setPen(screenPen(QColor(40, 30, 10), 1.0));
    painter.setBrush(QColor(255, 196, 60));
    const core::IsoProjection projected = projection();
    for (const AuthorNote& note : _sidecar.notes) {
        if (!cells.contains(note.cell)) {
            continue;
        }
        if (iso) {
            const QPointF center = toQt(projected.tileToWorld(note.cell));
            const double radius = projected.tileHeight() * 0.18;
            painter.drawEllipse(QPointF(center.x(), center.y() - (projected.tileHeight() * 0.2)),
                                radius, radius);
        } else {
            painter.drawEllipse(QPointF(note.cell.column + 0.75, note.cell.row + 0.25), 0.16, 0.16);
        }
    }
}

void EditorViewport::paintMirrorAxis(QPainter& painter, bool iso) {
    if (!_mirror) {
        return;
    }
    // Les centres des cases c − r = k, d'un bord de la carte à l'autre : une verticale en iso.
    const int k = _mirror->offset;
    const int first = std::max(0, -k);
    const int last = std::min(_draft.tileMap().height() - 1, _draft.tileMap().width() - 1 - k);
    if (first > last) {
        return;
    }
    const core::Vector2 from{static_cast<float>(k + first), static_cast<float>(first)};
    const core::Vector2 to{static_cast<float>(k + last + 1), static_cast<float>(last + 1)};
    const core::IsoProjection projected = projection();
    painter.setPen(screenPen(QColor(120, 255, 200), 2.0));
    painter.drawLine(toQt(iso ? projected.gridToWorld(from) : from),
                     toQt(iso ? projected.gridToWorld(to) : to));
}

void EditorViewport::paintDragPreview(QPainter& painter, bool iso) {
    std::optional<std::pair<core::GridPosition, core::GridPosition>> segment;
    if (_dragging && (_tool == hmi::EditorTool::Line || _tool == hmi::EditorTool::Measure)) {
        segment = std::make_pair(_dragStart, _dragCurrent);
    } else if (_tool == hmi::EditorTool::Measure) {
        segment = _measure;
    }
    if (!segment) {
        return;
    }
    const core::IsoProjection projected = projection();
    const auto shape = [&](core::GridPosition cell) {
        return iso ? diamondOf(projected, cell)
                   : QPolygonF(QRectF(cell.column, cell.row, 1.0, 1.0));
    };
    painter.setPen(screenPen(QColor(110, 190, 255), 1.0));
    painter.setBrush(QColor::fromRgbF(0.3F, 0.7F, 1.0F, 0.28F));
    if (_tool == hmi::EditorTool::Line) {
        for (const core::GridPosition cell : lineCells(segment->first, segment->second)) {
            painter.drawPolygon(shape(cell));
        }
        return;
    }
    // La mesure : ses deux cases, et le trait qui joint leurs centres.
    painter.drawPolygon(shape(segment->first));
    painter.drawPolygon(shape(segment->second));
    const auto center = [&](core::GridPosition cell) {
        const core::Vector2 point{static_cast<float>(cell.column) + 0.5F,
                                  static_cast<float>(cell.row) + 0.5F};
        return toQt(iso ? projected.gridToWorld(point) : point);
    };
    painter.setPen(screenPen(QColor(110, 190, 255), 2.0));
    painter.drawLine(center(segment->first), center(segment->second));
}

QColor EditorViewport::tileColor(core::TileType type) {
    return SceneImages::tileColor(type);
}

std::string EditorViewport::hoveredPieces() const {
    if (!_hoverCell || _play) {
        return {};
    }
    return cellPieces(_snapshot, *_hoverCell);
}

// --- Brouillon
// ------------------------------------------------------------------------------------

bool EditorViewport::hasVisualLayers() const {
    return std::ranges::any_of(_draft.layers(), [](const core::TileLayer& layer) {
        return core::isVisualLayerKind(layer.kind);
    });
}

void EditorViewport::paintAt(const QMouseEvent* event, bool continuing) {
    if (const std::optional<core::GridPosition> cell = cellAt(event)) {
        reportBrush(applyStroke(_draft, currentBrush(), _activeLayer, _layerView, {*cell},
                                continuing, strokeContext()));
    }
}

StrokeContext EditorViewport::strokeContext() const {
    return StrokeContext{.mirror = _mirror,
                         .appearance = _appearancePlace.empty() ? nullptr : &_appearance};
}

void EditorViewport::endPainting() {
    if (_painting) {
        _painting = false;
        _draft.endGesture();
    }
}

void EditorViewport::pickAt(core::GridPosition cell) {
    const std::optional<PickedBrush> picked =
        pickBrush(_draft, _activeLayer, cell, _appearancePlace.empty() ? nullptr : &_appearance);
    if (!picked) {
        emit statusMessage(QStringLiteral("Nothing to pick here."));
        return;
    }
    if (picked->brush.kind == BrushKind::Piece) {
        setActivePiece(picked->brush.piece, picked->brush.floor);
    } else {
        _brush = picked->brush;
        setActiveLayer(picked->layer);
    }
    emit brushPicked(_brush);
    emit statusMessage(
        QStringLiteral("Picked %1.").arg(QString::fromStdString(brushLabel(_brush))));
}

void EditorViewport::setMirror(bool enabled) {
    if (!enabled) {
        _mirror.reset();
    } else {
        const core::GridPosition through = _hoverCell.value_or(core::GridPosition{
            .column = _draft.tileMap().width() / 2, .row = _draft.tileMap().height() / 2});
        _mirror = mirrorAxisThrough(through);
        emit statusMessage(QStringLiteral("Mirror across the vertical through (%1, %2).")
                               .arg(through.column)
                               .arg(through.row));
    }
    viewport()->update();
    emit toolStateChanged();
}

std::string EditorViewport::measureText() const {
    if (_dragging && _tool == hmi::EditorTool::Measure) {
        return measureLabel(measureBetween(_dragStart, _dragCurrent));
    }
    return _measure && _tool == hmi::EditorTool::Measure
               ? measureLabel(measureBetween(_measure->first, _measure->second))
               : std::string{};
}

void EditorViewport::reloadSidecar() {
    const SidecarReadResult read = readSidecar(sidecarPath(levelPath()));
    _sidecar = read.sidecar;
    if (!read.warning.empty()) {
        HMI_LOG_WARNING("Editeur : annexe illisible : " + read.warning);
        emit statusMessage(
            QStringLiteral("Author notes: %1").arg(QString::fromStdString(read.warning)));
    }
    viewport()->update();
    emit toolStateChanged();
}

void EditorViewport::setNote(core::GridPosition cell, const std::string& text) {
    if (!hmi::setNote(_sidecar, cell, text)) {
        return;
    }
    if (!writeSidecar(sidecarPath(levelPath()), _sidecar)) {
        HMI_LOG_ERROR("Editeur : echec d'ecriture de l'annexe de " + _mapId);
        emit statusMessage(QStringLiteral("Failed to write the author notes."));
    }
    viewport()->update();
    emit toolStateChanged();
}

void EditorViewport::setMapProperty(const std::string& key, core::PropertyValue value) {
    if (_draft.setProperty(key, std::move(value))) {
        markDraftMutated();
    }
}

void EditorViewport::setMapProperties(
    const std::vector<std::pair<std::string, core::PropertyValue>>& values) {
    const core::GestureScope gesture(_draft);
    bool changed = false;
    for (const auto& [key, value] : values) {
        changed = _draft.setProperty(key, value) || changed;
    }
    if (changed) {
        markDraftMutated();
    }
}

void EditorViewport::setMapState(MapState state) {
    if (_sidecar.state == state) {
        return;
    }
    _sidecar.state = state;
    if (!writeSidecar(sidecarPath(levelPath()), _sidecar)) {
        HMI_LOG_ERROR("Editeur : echec d'ecriture de l'annexe de " + _mapId);
        emit statusMessage(QStringLiteral("Failed to write the editor file."));
    }
    emit toolStateChanged();
}

std::string EditorViewport::hoveredNote() const {
    if (!_hoverCell || _play) {
        return {};
    }
    const AuthorNote* const note = noteAt(_sidecar, *_hoverCell);
    if (note == nullptr) {
        return {};
    }
    std::string line = note->text;
    std::ranges::replace(line, '\n', ' ');
    constexpr std::size_t MAX_LENGTH = 60;
    if (line.size() > MAX_LENGTH) {
        line = line.substr(0, MAX_LENGTH) + "…";
    }
    return line;
}

CanvasBrush EditorViewport::currentBrush() const {
    if (_tool == hmi::EditorTool::Eraser) {
        return CanvasBrush{.kind = BrushKind::Eraser, .type = {}, .piece = {}, .floor = false};
    }
    CanvasBrush brush = _brush;
    if (brush.kind == BrushKind::Piece) {
        // La table du lieu peut avoir changé depuis le choix (une autre carte ouverte).
        brush.type = pieceCellType(&_appearance, brush.piece, brush.floor);
    }
    return brush;
}

void EditorViewport::reportBrush(const BrushResult& result) {
    if (result.changed) {
        markDraftMutated();
    } else if (!result.refusal.empty() && !_refusalReported) {
        _refusalReported = true;  // une fois par geste, pas à chaque case glissée
        emit statusMessage(QString::fromStdString(result.refusal));
    }
}

void EditorViewport::setActiveTile(core::TileType type) {
    _brush = CanvasBrush{.kind = BrushKind::Type, .type = type, .piece = {}, .floor = false};
    // Choisir dans la palette, c'est vouloir peindre : l'outil du peintre reprend la main.
    if (!paintsWithBrush(_tool)) {
        setTool(_paintTool);
    }
}

void EditorViewport::setActivePiece(const std::string& piece, bool floor) {
    _brush = pieceBrush(&_appearance, piece, floor);
    // La pièce va sur sa couche : on la montre active, verrou et opacité compris.
    // Une couche d'étage active le reste : un toit se choisit en peignant l'étage (LOT-129).
    if (const std::optional<std::size_t> layer =
            pieceTargetLayer(_draft.layers(), floor, _activeLayer)) {
        setActiveLayer(*layer);
    }
    if (!paintsWithBrush(_tool)) {
        setTool(_paintTool);
    }
}

std::vector<PieceCatalogGroup> EditorViewport::pieceCatalog() const {
    return hmi::pieceCatalog(_manifest.get(), _draft.layers());
}

std::filesystem::path EditorViewport::pieceImagesDirectory() const {
    return _appearancePlace.empty() ? std::filesystem::path{} : assetsDirectory();
}

bool EditorViewport::hoveredCellForced() const {
    return _hoverCell && !_play && _draft.isCollisionForced(*_hoverCell);
}

void EditorViewport::paintForcedMask(QPainter& painter, const CellRange& cells, bool iso) {
    // Les écarts forcés à la main (EX-EDIT-065) : un aplat magenta et son contour, qui ne se
    // confondent avec aucune teinte de règle.
    painter.setBrush(QColor(236, 64, 200, 110));
    painter.setPen(screenPen(QColor(236, 64, 200), 1.0));
    const core::IsoProjection projected = projection();
    for (const core::GridPosition cell : _draft.forcedCollision()) {
        if (!cells.contains(cell)) {
            continue;
        }
        if (iso) {
            painter.drawPolygon(diamondOf(projected, cell));
        } else {
            painter.drawRect(QRectF(cell.column, cell.row, 1.0, 1.0));
        }
    }
}

const core::TileMap& EditorViewport::activeLayerTiles() const {
    if (_activeLayer && *_activeLayer < _draft.layers().size()) {
        return _draft.layers()[*_activeLayer].tiles;
    }
    return _draft.tileMap();
}

void EditorViewport::applyRectangle(core::GridPosition a, core::GridPosition b) {
    // Un seul pas d'annulation pour tout le rectangle ; une pièce le pave au pas de son emprise.
    reportBrush(applyRectangleStroke(_draft, currentBrush(), _activeLayer, _layerView, a, b,
                                     strokeContext()));
}

Stamp EditorViewport::selectionStamp() const {
    if (!_selection) {
        return {};
    }
    return cutStamp(_draft, _selection->first, _selection->second);
}

void EditorViewport::copySelection() {
    // Le tampon entier (LOT-EDITOR-08) : couches, pieces, entites, cases forcees.
    Stamp stamp = selectionStamp();
    if (stamp.empty()) {
        return;
    }
    const QString label = QString::fromStdString(stampLabel(stamp));
    _clipboard = std::move(stamp);
    emit statusMessage(QStringLiteral("Copied: %1.").arg(label));
}

void EditorViewport::setClipboardStamp(Stamp stamp) {
    const QString label = QString::fromStdString(stampLabel(stamp));
    _clipboard = std::move(stamp);
    if (!_clipboard.empty()) {
        emit statusMessage(QStringLiteral("Stamp armed: %1. Ctrl+V to place it.").arg(label));
    }
    emit draftChanged();
}

void EditorViewport::pasteClipboard(bool mirrored) {
    if (_clipboard.empty() || !_hoverCell) {
        return;
    }
    const Stamp stamp = mirrored ? mirrorStamp(_clipboard, _manifest.get()) : _clipboard;
    const StampPasteResult result = pasteStamp(_draft, stamp, *_hoverCell, _layerView);
    if (!result.refusal.empty()) {
        emit statusMessage(
            QStringLiteral("Paste refused: %1").arg(QString::fromStdString(result.refusal)));
        return;
    }
    if (result.changed) {
        markDraftMutated();
        // Les entites posees sont selectionnees : l'inspecteur montre la derniere, prete a etre
        // renommee, et un Suppr les retire toutes.
        if (!result.entities.empty()) {
            setEntitySelection(result.entities, result.entities.back());
        }
        emit statusMessage(
            QStringLiteral("Pasted: %1%2")
                .arg(QString::fromStdString(stampLabel(stamp)),
                     mirrored ? QStringLiteral(" (mirrored).") : QStringLiteral(".")));
    }
    _refusalReported = false;
}

std::optional<std::pair<core::GridPosition, core::GridPosition>> EditorViewport::highlight() const {
    if (_dragging) {
        return std::make_pair(
            core::GridPosition{.column = std::min(_dragStart.column, _dragCurrent.column),
                               .row = std::min(_dragStart.row, _dragCurrent.row)},
            core::GridPosition{.column = std::max(_dragStart.column, _dragCurrent.column),
                               .row = std::max(_dragStart.row, _dragCurrent.row)});
    }
    return _selection;
}

void EditorViewport::markDraftMutated() {
    syncEditingState();
    // La scène iso est recomposée tout de suite : la barre d'état lit les pièces de la case.
    _isoSceneDirty = true;
    ensureIsoScene();
    refreshBounds();
    emit draftChanged();
}

// --- Essai immédiat
// -------------------------------------------------------------------------------

core::Vector2 EditorViewport::heldDirection() const {
    const auto held = [this](bool (*matches)(int)) {
        return std::ranges::any_of(_heldKeys, matches);
    };
    float x = (held(isRightKey) ? 1.0F : 0.0F) - (held(isLeftKey) ? 1.0F : 0.0F);
    float y = (held(isDownKey) ? 1.0F : 0.0F) - (held(isUpKey) ? 1.0F : 0.0F);
    const float length = std::sqrt((x * x) + (y * y));
    if (length > 0.0F) {
        x /= length;
        y /= length;
    }
    return {x, y};
}

void EditorViewport::stepPlaytest() {
    if (!_play) {
        return;
    }
    const Clock::time_point now = Clock::now();
    const float elapsedSeconds = std::chrono::duration<float>(now - _previousFrame).count();
    _previousFrame = now;

    const int steps = _timestep.advance(elapsedSeconds);
    for (int step = 0; step < steps && _play; ++step) {
        const core::ExplorationIntent intent{.move = heldDirection(),
                                             .interact = _interactRequested};
        _interactRequested = false;
        const WorldPlayStep result = _play->step(intent, _timestep.fixedDeltaSeconds());
        _playSceneDirty =
            _playSceneDirty || result.sceneChanged || result.figuresChanged || result.heroMoved;
        for (const core::ExplorationEvent& event : result.events) {
            // L'éditeur n'ouvre ni dialogue ni combat : il dit ce que le jeu ferait, et l'essai
            // continue. C'est l'information qu'on vient chercher en essayant une carte.
            QString message;
            switch (event.kind) {
                case core::ExplorationEventKind::MapEntered:
                    message = QStringLiteral("Playtest: entered map %1.");
                    break;
                case core::ExplorationEventKind::Dialogue:
                    message = QStringLiteral("Playtest: dialogue %1 would open.");
                    break;
                case core::ExplorationEventKind::Encounter:
                    message = QStringLiteral("Playtest: encounter %1 would start.");
                    break;
                case core::ExplorationEventKind::PortalLocked:
                    message = QStringLiteral("Playtest: portal locked, requires %1.");
                    break;
                case core::ExplorationEventKind::PortalBroken:
                    message = QStringLiteral("Playtest: broken portal to %1.");
                    break;
                case core::ExplorationEventKind::Interacted:
                    message = QStringLiteral("Playtest: interacted with %1.");
                    break;
                case core::ExplorationEventKind::PortalSealed:
                    message = QStringLiteral("Playtest: sealed portal %1.");
                    break;
                case core::ExplorationEventKind::QuestAdvanced:
                    message = QStringLiteral("Playtest: quest step %1 reached.");
                    break;
            }
            emit statusMessage(message.arg(QString::fromStdString(event.value)));
        }
    }
    if (!_playSceneDirty) {
        return;
    }
    _playSceneDirty = false;
    // La carte : composée une fois, tant qu'elle ne change pas — comme dans le jeu
    // (`hmi::WorldSceneRenderer`). Un portail, un drapeau la refont.
    const std::shared_ptr<const WorldSceneSnapshot> map = _play->scene();
    const core::IsoProjection played(map->columns, map->rows, core::ARENA_TILE_WIDTH_UNITS,
                                     map->diamondRatio);
    if (map != _playMap) {
        const bool resized =
            _playMap == nullptr || map->columns != _playMap->columns || map->rows != _playMap->rows;
        _playMap = map;
        _images->ensure(worldTexturePaths(*map));
        _playStatics.build(*map, played, _images->textures());
        _playScene.clear();
        _playScene.clearVisibleBounds();
        _playStatics.compose(_playScene, {}, _images->textures());
        _playBounds = composedSceneBounds(_playScene, snapshotRect(*map));
        if (resized) {
            refreshBounds();  // un portail a mené sur une autre carte.
        }
    }
    // La caméra suit le héros, comme en jeu (`hmi::worldCamera`).
    const core::CellPoint hero = _play->session().heroPoint();
    centerOn(toQt(played.gridToWorld({hero.column, hero.row})));
    // L'image : ce que la vue montre de la carte, et les figurines de l'instant.
    const std::vector<WorldFigureSnapshot> figures = _play->figures();
    _images->ensure(worldFigureTexturePaths(*map, figures));
    const QRectF shown = mapToScene(viewport()->rect()).boundingRect();
    _playScene.clear();
    _playScene.setVisibleBounds(
        core::Rect{{static_cast<float>(shown.x()), static_cast<float>(shown.y())},
                   {static_cast<float>(shown.width()), static_cast<float>(shown.height())}});
    _playStatics.compose(_playScene, figures, _images->textures());
    viewport()->update();
}

void EditorViewport::startPlaytestHere() {
    startPlaytest(_hoverCell);
}

namespace {

// L'essai part de la case : une copie du brouillon, sans historique, dont l'entrée y est déplacée.
// Le brouillon, lui, ne bouge pas. Rend le message d'échec, vide si l'essai peut partir.
[[nodiscard]] QString moveEntryTo(core::LevelLoadResult& validated, core::GridPosition from,
                                  const std::shared_ptr<const core::ScenePieceManifest>& manifest) {
    const core::TileMap& tiles = validated.level->tileMap();
    if (!tiles.inBounds(from.column, from.row) ||
        core::isSolid(tiles.tile(from.column, from.row))) {
        return QStringLiteral("Cannot playtest from (%1, %2): the cell blocks the way.")
            .arg(from.column)
            .arg(from.row);
    }
    core::LevelDraft moved = core::LevelDraft::fromLevel(*validated.level);
    moved.setPieceManifest(manifest);
    moved.setEntry(from.column, from.row);
    validated = moved.toLevel();
    if (!validated.ok()) {
        return QStringLiteral("Cannot playtest from here: %1")
            .arg(QString::fromStdString(validated.error));
    }
    return {};
}

// Le brouillon est servi sous l'identifiant de sa carte ; toute autre carte vient du disque,
// comme en jeu. Un portail qui ramène ici retrouve donc le brouillon, pas le fichier d'avant.
[[nodiscard]] core::WorldTravel::MapLoader draftLoader(std::string mapId,
                                                       std::shared_ptr<const core::Level> edited,
                                                       core::WorldTravel::MapLoader fromDisk) {
    return [mapId = std::move(mapId), edited = std::move(edited),
            fromDisk = std::move(fromDisk)](std::string_view requested) {
        if (requested == mapId) {
            core::LevelLoadResult served;
            served.level = *edited;
            return served;
        }
        return fromDisk(requested);
    };
}

// L'essai part de l'état de partie (LOT-126) : les quêtes déclarent leurs drapeaux, l'état les
// règle, et les quêtes avancent d'autant — comme le jeu lancé avec `--flags=`.
void applyWorldState(WorldPlay& play, const std::vector<std::string>& stateEntries) {
    play.session().setQuests(core::loadQuests(hmi::editorDataRoot() / "World" / "quests"));
    for (const std::string& entry : stateEntries) {
        const WorldStateEntry read = parseWorldStateEntry(entry);
        if (read.value) {
            play.session().flags().setValue(read.flag, *read.value);
        } else {
            play.session().flags().set(read.flag);
        }
    }
    static_cast<void>(play.session().refreshFromFlags());
}

}  // namespace

void EditorViewport::startPlaytest(std::optional<core::GridPosition> from) {
    if (_play) {
        return;
    }
    endPainting();
    core::LevelLoadResult validated = _draft.toLevel();
    if (!validated.ok()) {
        HMI_LOG_WARNING("Editeur : essai refuse (brouillon invalide) : " + validated.error);
        emit statusMessage(
            QStringLiteral("Cannot playtest: %1").arg(QString::fromStdString(validated.error)));
        return;
    }
    if (from) {
        const QString failure = moveEntryTo(validated, *from, _manifest);
        if (!failure.isEmpty()) {
            emit statusMessage(failure);
            return;
        }
    }
    auto edited = std::make_shared<const core::Level>(std::move(*validated.level));
    auto play = std::make_unique<WorldPlay>(
        draftLoader(_mapId, std::move(edited),
                    core::WorldTravel::directoryLoader(levelsDirectory())),
        assetsDirectory());
    applyWorldState(*play, _stateEntries);
    if (!play->enter(_mapId, {})) {
        HMI_LOG_WARNING("Editeur : essai refuse, la carte ne s'ouvre pas.");
        emit statusMessage(QStringLiteral("Cannot playtest: the map does not open."));
        return;
    }
    _editTransform = transform();
    _editCenter = mapToScene(viewport()->rect().center());
    _play = std::move(play);
    _playMap.reset();
    _playStatics.clear();
    _playSceneDirty = true;
    _heldKeys.clear();
    _interactRequested = false;
    _timestep = core::FixedTimestep{};
    _previousFrame = Clock::now();
    setBackgroundBrush(PLAYTEST_BACKGROUND);
    // Le cadrage du jeu : une case à la hauteur de la vue divisée par 10,8 (`hmi::worldCamera`).
    const double scale = static_cast<double>(worldTilePixels(viewport()->height())) /
                         static_cast<double>(core::ARENA_TILE_WIDTH_UNITS);
    setTransform(QTransform::fromScale(scale, scale));
    stepPlaytest();
    _playTimer.start();
    setFocus();
    HMI_LOG_INFO("Editeur : essai immediat demarre.");
    emit statusMessage(from
                           ? QStringLiteral("Playtesting from (%1, %2) — Esc to return to editing.")
                                 .arg(from->column)
                                 .arg(from->row)
                           : QStringLiteral("Playtesting — Esc to return to editing."));
}

void EditorViewport::stopPlaytest() {
    if (!_play) {
        return;
    }
    _playTimer.stop();
    _play.reset();
    _playMap.reset();
    _playStatics.clear();
    _playScene.clear();
    _playScene.clearVisibleBounds();
    _heldKeys.clear();
    setBackgroundBrush(EDIT_BACKGROUND);
    refreshBounds();
    setTransform(_editTransform);
    centerOn(_editCenter);
    emit statusMessage(QStringLiteral("Back to editing."));
}

// --- Événements
// -----------------------------------------------------------------------------------

bool EditorViewport::viewportEvent(QEvent* event) {
    if (event->type() == QEvent::Leave && _hoverCell) {
        _hoverCell.reset();
        emit hoveredCellChanged(std::nullopt);
        viewport()->update();
    }
    return QGraphicsView::viewportEvent(event);
}

void EditorViewport::focusOutEvent(QFocusEvent* event) {
    _heldKeys.clear();
    endPainting();
    QGraphicsView::focusOutEvent(event);
}

void EditorViewport::keyPressEvent(QKeyEvent* event) {
    if (_play) {
        if (event->key() == Qt::Key_Escape) {
            stopPlaytest();
            return;
        }
        if (event->isAutoRepeat()) {
            return;
        }
        if (event->key() == Qt::Key_E || event->key() == Qt::Key_Space) {
            _interactRequested = true;
            return;
        }
        _heldKeys.insert(event->key());
        return;
    }
    // Annuler/refaire/enregistrer/essai/grille/recadrer/copier/coller sont des actions Qt uniques
    // (`hmi::EditorActions`) : aucun second traitement ici, sous peine de double déclenchement.
    if (event->isAutoRepeat()) {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    // Retrait des entités sélectionnées (outil Entité, LOT-11) : Suppr, comme dans tout éditeur.
    if (event->key() == Qt::Key_Delete && _tool == hmi::EditorTool::Entity &&
        !_selectedEntities.empty()) {
        removeSelectedEntities();
        return;
    }
    // Suppr gomme la sélection, sur la couche active, en un pas (LOT-EDITOR-04).
    if (event->key() == Qt::Key_Delete && _tool == hmi::EditorTool::Selection && _selection) {
        const CanvasBrush eraser{
            .kind = BrushKind::Eraser, .type = {}, .piece = {}, .floor = false};
        reportBrush(applyRectangleStroke(_draft, eraser, _activeLayer, _layerView,
                                         _selection->first, _selection->second, StrokeContext{}));
        _refusalReported = false;
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void EditorViewport::keyReleaseEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        return;
    }
    _heldKeys.erase(event->key());
}

void EditorViewport::mousePressEvent(QMouseEvent* event) {
    if (_play) {
        return;
    }
    if (event->button() == Qt::RightButton) {
        _rightDragging = true;  // le bouton droit déplace la vue.
        _rightDragLast = event->position().toPoint();
        return;
    }
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const std::optional<core::GridPosition> cell = cellAt(event);
    // Alt + clic : la pipette, depuis n'importe quel outil (LOT-EDITOR-04).
    if (event->modifiers().testFlag(Qt::AltModifier)) {
        if (cell) {
            pickAt(*cell);
        }
        return;
    }
    switch (_tool) {
        case hmi::EditorTool::Paint:
        case hmi::EditorTool::Eraser:
            // Du clic au relâchement, un seul geste : un seul pas d'annulation.
            _painting = true;
            _draft.beginGesture();
            paintAt(event, false);
            break;
        case hmi::EditorTool::Rectangle:
        case hmi::EditorTool::Line:
        case hmi::EditorTool::Selection:
        case hmi::EditorTool::Measure:
            _dragging = true;
            _dragStart = clampedCell(event);
            _dragCurrent = _dragStart;
            if (_tool == hmi::EditorTool::Measure) {
                _measure.reset();
                emit toolStateChanged();
            }
            viewport()->update();
            break;
        case hmi::EditorTool::Bucket:
            if (cell) {
                reportBrush(applyBucket(_draft, currentBrush(), _activeLayer, _layerView, *cell,
                                        strokeContext()));
            }
            break;
        case hmi::EditorTool::Pipette:
            if (cell) {
                pickAt(*cell);
                setTool(_paintTool);  // pris : on repeint aussitôt.
            }
            break;
        case hmi::EditorTool::Entity:
            handleEntityPress(event);
            break;
        case hmi::EditorTool::Shape:
            handleShapePress(event);
            break;
        case hmi::EditorTool::Note:
            if (cell) {
                emit noteRequested(*cell);
            }
            break;
    }
}

void EditorViewport::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::RightButton) {
        _rightDragging = false;
        return;
    }
    if (event->button() != Qt::LeftButton || _play) {
        return;
    }
    if (_tool == hmi::EditorTool::Entity || _tool == hmi::EditorTool::Shape) {
        handleEntityRelease(event);
    }
    _refusalReported = false;
    if (_dragging) {
        _dragCurrent = clampedCell(event);
        _dragging = false;
        if (_tool == hmi::EditorTool::Rectangle) {
            applyRectangle(_dragStart, _dragCurrent);
        } else if (_tool == hmi::EditorTool::Line) {
            reportBrush(applyStroke(_draft, currentBrush(), _activeLayer, _layerView,
                                    lineCells(_dragStart, _dragCurrent), false, strokeContext()));
        } else if (_tool == hmi::EditorTool::Measure) {
            _measure = std::make_pair(_dragStart, _dragCurrent);
            emit toolStateChanged();
        } else if (_tool == hmi::EditorTool::Selection) {
            _selection = std::make_pair(
                core::GridPosition{.column = std::min(_dragStart.column, _dragCurrent.column),
                                   .row = std::min(_dragStart.row, _dragCurrent.row)},
                core::GridPosition{.column = std::max(_dragStart.column, _dragCurrent.column),
                                   .row = std::max(_dragStart.row, _dragCurrent.row)});
        }
        viewport()->update();
    }
    endPainting();
}

void EditorViewport::mouseMoveEvent(QMouseEvent* event) {
    if (_play) {
        return;
    }
    if (_rightDragging) {
        const QPoint current = event->position().toPoint();
        const QPoint delta = current - _rightDragLast;
        _rightDragLast = current;
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        _framed = true;
    }
    const std::optional<core::GridPosition> cell = cellAt(event);  // cible du collage (Ctrl+V)
    if (cell != _hoverCell) {
        _hoverCell = cell;
        emit hoveredCellChanged(_hoverCell);
        viewport()->update();
    }
    if (_entityDrag) {
        const core::GridPosition current = clampedCell(event);
        if (current != _entityDragTo) {
            _entityDragTo = current;
            viewport()->update();
        }
    } else if (_shapePainting) {
        if (cell) {
            paintShapeAt(*cell);
        }
    } else if (_painting) {
        paintAt(event, true);
    } else if (_dragging) {
        const core::GridPosition current = clampedCell(event);
        if (current != _dragCurrent) {
            _dragCurrent = current;
            viewport()->update();
            if (_tool == hmi::EditorTool::Measure) {
                emit toolStateChanged();
            }
        }
    }
}

void EditorViewport::wheelEvent(QWheelEvent* event) {
    if (_play) {
        return;
    }
    const int notches = event->angleDelta().y() / 120;  // 120 = un cran de molette.
    if (notches == 0) {
        return;
    }
    // Plus petit que la carte entière n'a pas d'usage ; plus grand que 8 pixels d'art non plus.
    const QRectF bounds = contentBounds();
    const double fit = std::min(static_cast<double>(viewport()->width()) / bounds.width(),
                                static_cast<double>(viewport()->height()) / bounds.height());
    const double current = transform().m11();
    const double wanted = std::clamp(current * std::pow(ZOOM_STEP, notches),
                                     std::min(fit, MAX_PIXELS_PER_UNIT), MAX_PIXELS_PER_UNIT);
    const double factor = wanted / current;
    scale(factor, factor);  // ancré sous le pointeur (`AnchorUnderMouse`).
    _framed = true;
    emitZoomIfChanged();
    emit framingChanged();
}

// --- Fichier
// --------------------------------------------------------------------------------------

bool EditorViewport::save() {
    const core::LevelLoadResult validated = _draft.toLevel();
    if (!validated.ok()) {
        HMI_LOG_WARNING("Editeur : enregistrement refuse (brouillon invalide) : " +
                        validated.error);
        emit statusMessage(
            QStringLiteral("Cannot save: %1").arg(QString::fromStdString(validated.error)));
        return false;
    }
    const std::filesystem::path path = levelPath();
    if (core::LevelWriter::saveToFile(*validated.level, path)) {
        _savedRevision = _draft.revision();
        _diskFingerprint = fingerprintFile(path);
        HMI_LOG_INFO("Editeur : carte enregistree : " + path.string());
        emit statusMessage(
            QStringLiteral("Map saved: %1").arg(QString::fromStdString(path.filename().string())));
    } else {
        HMI_LOG_ERROR("Editeur : echec d'ecriture de la carte : " + path.string());
        emit statusMessage(QStringLiteral("Failed to write file."));
        return false;
    }
    emit draftChanged();  // la barre d'état relit l'indicateur de modification.
    return true;
}

bool EditorViewport::replacePieces(const core::PieceRenaming& renaming) {
    if (!_draft.replacePieces(renaming)) {
        return false;
    }
    markDraftMutated();
    return true;
}

bool EditorViewport::changeScene(const std::string& place, const core::PieceRenaming& table) {
    PlaceAssets assets = hmi::loadPlaceAssets(hmi::editorDataRoot(), place);
    if (!assets.manifest) {
        return false;
    }
    // Le manifeste neuf sert à redéduire la collision ; la scène le relit par son lieu.
    if (!_draft.changeScene(
            place, std::make_shared<const core::ScenePieceManifest>(std::move(*assets.manifest)),
            table)) {
        return false;
    }
    markDraftMutated();
    return true;
}

bool EditorViewport::openLevel(const std::filesystem::path& path) {
    core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(path);
    if (!loaded.ok()) {
        HMI_LOG_WARNING("Editeur : ouverture impossible (" + path.string() + ") : " + loaded.error);
        emit statusMessage(
            QStringLiteral("Cannot open: %1").arg(QString::fromStdString(loaded.error)));
        return false;
    }
    stopPlaytest();
    _painting = false;  // un geste en cours visait l'ancien brouillon.
    _dragging = false;
    _draft = core::LevelDraft::fromLevel(*loaded.level);
    _mapId = mapIdOf(path);
    _revealedCell.reset();  // une case d'une autre carte.
    // Une carte ouverte repart de sa collision, tout affiché, rien de sélectionné : les réglages
    // de la carte précédente n'ont aucun sens pour celle-ci.
    _layerView.reset();
    setActiveLayer(std::nullopt);
    selectEntity(std::nullopt);
    _selection.reset();
    _savedRevision = _draft.revision();
    _diskFingerprint = fingerprintFile(path);
    _measure.reset();
    reloadSidecar();
    markDraftMutated();
    resetCamera();
    HMI_LOG_INFO("Editeur : carte ouverte : " + path.string());
    emit statusMessage(
        QStringLiteral("Map opened: %1").arg(QString::fromStdString(path.filename().string())));
    return true;
}

std::filesystem::path EditorViewport::levelPath() const {
    return levelsDirectory() / (_mapId + ".json");
}

bool EditorViewport::restoreDraft(const std::string& mapId, const std::string& draftJson) {
    core::LevelLoadResult loaded = core::LevelLoader::loadFromString(draftJson);
    if (!loaded.ok()) {
        HMI_LOG_WARNING("Editeur : brouillon de reprise illisible (" + mapId +
                        ") : " + loaded.error);
        return false;
    }
    stopPlaytest();
    _painting = false;  // un geste en cours visait l'ancien brouillon.
    _dragging = false;
    _draft = core::LevelDraft::fromLevel(*loaded.level);
    _mapId = mapId;
    _layerView.reset();
    setActiveLayer(std::nullopt);
    selectEntity(std::nullopt);
    _selection.reset();
    // Le brouillon repris n'est pas le fichier : il reste modifié jusqu'à l'enregistrement. Le
    // fichier, lui, est pris tel qu'il est maintenant -- c'est contre lui que la garde compare.
    _savedRevision = NEVER_SAVED;
    _diskFingerprint = fingerprintFile(levelPath());
    reloadSidecar();
    markDraftMutated();
    resetCamera();
    HMI_LOG_INFO("Editeur : brouillon repris : " + mapId);
    emit statusMessage(
        QStringLiteral("Draft recovered: %1 (not saved yet).").arg(QString::fromStdString(mapId)));
    return true;
}

DiskChange EditorViewport::diskChange() const {
    return compareFingerprints(_diskFingerprint, fingerprintFile(levelPath()));
}

void EditorViewport::acceptDiskVersion() {
    _diskFingerprint = fingerprintFile(levelPath());
}

void EditorViewport::undo() {
    if (_draft.undo()) {
        markDraftMutated();
    }
}

void EditorViewport::redo() {
    if (_draft.redo()) {
        markDraftMutated();
    }
}

void EditorViewport::toggleGrid() noexcept {
    _showGrid = !_showGrid;
    viewport()->update();
}

void EditorViewport::resizeLevel(int width, int height) {
    _draft.resize(width, height);
    markDraftMutated();
    emit statusMessage(QStringLiteral("Map resized: %1 × %2").arg(width).arg(height));
}

bool EditorViewport::wouldResizeDrop(int width, int height) const {
    return _draft.wouldResizeDropContent(width, height);
}

int EditorViewport::levelWidth() const {
    return _draft.tileMap().width();
}

int EditorViewport::levelHeight() const {
    return _draft.tileMap().height();
}

// --- Couches et entités (LOT-11)
// ------------------------------------------------------------------

void EditorViewport::syncEditingState() {
    const LayerSlot active = validActiveLayer(_draft.layers(), _activeLayer);
    _layerView.sync(_draft.layers().size());
    if (active != _activeLayer) {
        _activeLayer = active;
        emit activeLayerChanged(_activeLayer);
    }
    const std::size_t count = _draft.entities().size();
    std::erase_if(_selectedEntities, [count](std::size_t index) { return index >= count; });
    if (_selectedEntity && *_selectedEntity >= count) {
        _selectedEntity =
            _selectedEntities.empty() ? std::nullopt : std::make_optional(_selectedEntities.back());
        emit entitySelectionChanged(_selectedEntity);
    }
    if (_entityDrag && std::ranges::any_of(_entityDrag->indices,
                                           [count](std::size_t index) { return index >= count; })) {
        _entityDrag.reset();
    }
    refreshDiagnostics();
}

void EditorViewport::refreshDiagnostics() {
    static const hmi::EditorReferences emptyReferences;
    const hmi::EditorReferences& references =
        _references != nullptr ? *_references : emptyReferences;
    _referenceContext = hmi::referenceContext(references, _mapId, _draft.entities(),
                                              scenePlaceOf(_draft.layers()), _manifest.get());
    const std::vector<core::EntityIssue> issues =
        core::validateMapEntities(_draft.entities(), _referenceContext);
    _terrains = core::analyzeEncounterTerrain(_draft.tileMap(), _draft.entities(),
                                              references.encounters, &references.bestiary);
    _zoneVerdicts = core::analyzeCombatZones(_draft.tileMap(), _draft.entities());
    _diagnostics = hmi::editorDiagnostics(_draft.entities(), issues, _terrains, _zoneVerdicts);
}

void EditorViewport::setActiveLayer(LayerSlot slot) {
    const LayerSlot valid = validActiveLayer(_draft.layers(), slot);
    if (valid == _activeLayer) {
        return;
    }
    _activeLayer = valid;
    _selection.reset();    // une sélection copiée d'une autre couche tromperait le collage.
    viewport()->update();  // le masque de collision iso suit la couche active.
    emit activeLayerChanged(_activeLayer);
}

void EditorViewport::setMapLayerVisible(LayerSlot slot, bool visible) {
    _layerView.setVisible(slot, visible);
    viewport()->update();
    emit layerViewChanged();
}

void EditorViewport::setMapLayerOpacity(LayerSlot slot, float opacity) {
    _layerView.setOpacity(slot, opacity);
    viewport()->update();
    emit layerViewChanged();
}

void EditorViewport::setMapLayerDimmed(LayerSlot slot, bool dimmed) {
    _layerView.setDimmed(slot, dimmed);
    viewport()->update();
    emit layerViewChanged();
}

void EditorViewport::setMapLayerLocked(LayerSlot slot, bool locked) {
    _layerView.setLocked(slot, locked);
    emit layerViewChanged();
}

void EditorViewport::addMapLayer(core::LayerKind kind, const std::string& name) {
    const std::optional<std::size_t> index = _draft.addLayer(kind, name);
    if (!index) {
        return;
    }
    markDraftMutated();
    setActiveLayer(*index);
}

void EditorViewport::removeMapLayer(std::size_t index) {
    if (_draft.removeLayer(index)) {
        markDraftMutated();
    }
}

void EditorViewport::moveMapLayer(std::size_t index, bool forward) {
    const std::optional<std::size_t> moved = _draft.moveLayer(index, forward);
    if (!moved || *moved == index) {
        return;
    }
    _layerView.swap(index, *moved);
    const bool followActive = _activeLayer == index;
    markDraftMutated();
    if (followActive) {
        setActiveLayer(*moved);
    }
}

void EditorViewport::renameMapLayer(std::size_t index, const std::string& name) {
    if (!name.empty() && _draft.renameLayer(index, name)) {
        markDraftMutated();
    }
}

void EditorViewport::setMapLayerFloor(std::size_t index, int floor) {
    if (_draft.setLayerFloor(index, floor)) {
        markDraftMutated();
    }
}

void EditorViewport::setWorldState(std::vector<std::string> entries, bool preview) {
    _stateEntries = std::move(entries);
    _statePreview = preview;
    _stateFlags =
        worldStateFlags(_stateEntries, _references != nullptr ? _references->declaredFlags
                                                              : std::vector<core::QuestFlag>{})
            .flags;
    invalidateScene();
    viewport()->update();
}

void EditorViewport::setEditorReferences(const EditorReferences* references) {
    _references = references;
    // Les declarations des quetes ont pu changer : l'etat se relit contre elles.
    setWorldState(_stateEntries, _statePreview);
    refreshDiagnostics();
    viewport()->update();
    emit draftChanged();  // les panneaux relisent avertissements et choix proposés.
}

void EditorViewport::setEntityKindToPlace(std::string type) {
    _entityKindToPlace = std::move(type);
}

void EditorViewport::selectEntity(std::optional<std::size_t> index) {
    setEntitySelection(index ? std::vector<std::size_t>{*index} : std::vector<std::size_t>{},
                       index);
}

void EditorViewport::setEntitySelection(std::vector<std::size_t> indices,
                                        std::optional<std::size_t> primary) {
    const std::size_t count = _draft.entities().size();
    std::erase_if(indices, [count](std::size_t index) { return index >= count; });
    std::ranges::sort(indices);
    const auto [first, last] = std::ranges::unique(indices);
    indices.erase(first, last);
    if (!primary || !std::ranges::binary_search(indices, *primary)) {
        primary = indices.empty() ? std::nullopt : std::make_optional(indices.back());
    }
    if (indices == _selectedEntities && primary == _selectedEntity) {
        return;
    }
    _selectedEntities = std::move(indices);
    _selectedEntity = primary;
    invalidateScene();  // la formation de la rencontre sélectionnée suit la sélection.
    emit entitySelectionChanged(_selectedEntity);
}

void EditorViewport::setEntityProperty(std::size_t index, const std::string& key,
                                       core::PropertyValue value) {
    // Choisir la pièce d'une entité qui en pose une (LOT-126) lui donne l'emprise de la pièce, dans
    // le même pas d'annulation : sa collision est celle de ce qu'elle dessine.
    std::optional<core::PieceFootprint> extent;
    if (index < _draft.entities().size() && _manifest) {
        const core::EntityKind* const kind = core::findEntityKind(_draft.entities()[index].type);
        const std::string* const piece = std::get_if<std::string>(&value);
        if (kind != nullptr && kind->pieceProperty == key &&
            kind->shape == core::EntityShape::Rectangle && piece != nullptr) {
            if (const core::ScenePiece* const found = _manifest->find(*piece)) {
                extent = found->footprint();
            }
        }
    }
    const core::GestureScope gesture(_draft);
    bool changed = _draft.setEntityProperty(index, key, std::move(value));
    if (extent) {
        changed = _draft.setEntityProperty(index, std::string{core::SHAPE_WIDTH_PROPERTY},
                                           std::int64_t{extent->columns}) ||
                  changed;
        changed = _draft.setEntityProperty(index, std::string{core::SHAPE_HEIGHT_PROPERTY},
                                           std::int64_t{extent->rows}) ||
                  changed;
    }
    if (changed) {
        markDraftMutated();
    }
}

void EditorViewport::removeEntity(std::size_t index) {
    if (!_draft.removeEntity(index)) {
        return;
    }
    // La sélection garde les mêmes entités : celles d'après remontent d'un rang.
    std::vector<std::size_t> kept;
    for (const std::size_t selected : _selectedEntities) {
        if (selected != index) {
            kept.push_back(selected > index ? selected - 1 : selected);
        }
    }
    std::optional<std::size_t> primary = _selectedEntity;
    if (primary && *primary > index) {
        primary = *primary - 1;
    } else if (primary == index) {
        primary.reset();
    }
    _entityDrag.reset();
    markDraftMutated();
    setEntitySelection(std::move(kept), primary);
    emit statusMessage(QStringLiteral("Entity removed."));
}

void EditorViewport::removeSelectedEntities() {
    if (_selectedEntities.empty()) {
        return;
    }
    const std::size_t removed = removeEntities(_draft, _selectedEntities);
    _entityDrag.reset();
    _selectedEntities.clear();
    _selectedEntity.reset();
    markDraftMutated();
    invalidateScene();
    emit entitySelectionChanged(_selectedEntity);
    emit statusMessage(removed == 1 ? QStringLiteral("Entity removed.")
                                    : QStringLiteral("%1 entities removed.").arg(removed));
}

void EditorViewport::handleEntityPress(const QMouseEvent* event) {
    const std::optional<core::GridPosition> cell = cellAt(event);
    if (!cell) {
        return;
    }
    const hmi::EntityPressModifiers modifiers{
        .force = event->modifiers().testFlag(Qt::ControlModifier),
        .toggle = event->modifiers().testFlag(Qt::ShiftModifier)};
    const hmi::EntityGestureDecision decision =
        hmi::resolveEntityPress(_draft, *cell, _selectedEntities, _entityKindToPlace, modifiers);
    _entityDrag.reset();
    _entityDragTo = *cell;
    switch (decision.action) {
        case hmi::EntityGestureAction::Ignore:
            break;
        case hmi::EntityGestureAction::Deselect:
            selectEntity(std::nullopt);
            break;
        case hmi::EntityGestureAction::Toggle: {
            std::vector<std::size_t> toggled =
                toggledSelection(_selectedEntities, decision.entityIndex);
            setEntitySelection(std::move(toggled), decision.entityIndex);
            break;
        }
        case hmi::EntityGestureAction::Grab: {
            // Prendre une entité de la sélection emporte toute la sélection ; une autre la
            // remplace.
            if (std::ranges::binary_search(_selectedEntities, decision.entityIndex)) {
                setEntitySelection(_selectedEntities, decision.entityIndex);
            } else {
                selectEntity(decision.entityIndex);
            }
            _entityDrag = EntityDrag{
                .mode = decision.handle ? EntityDrag::Mode::Reshape : EntityDrag::Mode::Move,
                .indices = decision.handle ? std::vector<std::size_t>{decision.entityIndex}
                                           : _selectedEntities,
                .handle = decision.handle,
                .kind = {},
                .from = *cell};
            break;
        }
        case hmi::EntityGestureAction::Draw:
            _entityDrag = EntityDrag{.mode = EntityDrag::Mode::Draw,
                                     .indices = {},
                                     .handle = std::nullopt,
                                     .kind = _entityKindToPlace,
                                     .from = *cell};
            break;
        case hmi::EntityGestureAction::Place: {
            if (const std::optional<std::size_t> placed =
                    placeEntityOfKind(_draft, _entityKindToPlace, decision.cell)) {
                markDraftMutated();
                selectEntity(*placed);
                emit statusMessage(QStringLiteral("%1 placed at (%2, %3).")
                                       .arg(QString::fromStdString(_entityKindToPlace))
                                       .arg(decision.cell.column)
                                       .arg(decision.cell.row));
            }
            break;
        }
    }
    viewport()->update();
}

void EditorViewport::handleEntityRelease(const QMouseEvent* event) {
    if (_shapePainting) {
        _shapePainting = false;
        _draft.endGesture();
        return;
    }
    if (!_entityDrag) {
        return;
    }
    _entityDragTo = clampedCell(event);
    const EntityDragResult result = pendingEntityDrag();
    _entityDrag.reset();
    applyEntityDrag(result);
    viewport()->update();
}

void EditorViewport::handleShapePress(const QMouseEvent* event) {
    const std::optional<core::GridPosition> cell = cellAt(event);
    if (!cell) {
        return;
    }
    if (!_selectedEntity) {
        emit statusMessage(QStringLiteral("Select a zone or a route first, with the Entity tool."));
        return;
    }
    const std::size_t index = *_selectedEntity;
    const core::MapEntity& entity = _draft.entities()[index];
    const hmi::ShapeGestureDecision decision =
        hmi::resolveShapePress(entity, *cell, event->modifiers().testFlag(Qt::ControlModifier));
    switch (decision.action) {
        case hmi::ShapeGestureAction::Ignore:
            emit statusMessage(
                QStringLiteral("The selected entity has no cells to paint: "
                               "resize it with its handles (Entity tool)."));
            break;
        case hmi::ShapeGestureAction::PaintCells:
        case hmi::ShapeGestureAction::EraseCells:
            // Du clic au relâchement, un seul geste : un seul pas d'annulation.
            _shapePainting = true;
            _shapeErasing = decision.action == hmi::ShapeGestureAction::EraseCells;
            _draft.beginGesture();
            paintShapeAt(*cell);
            break;
        case hmi::ShapeGestureAction::AppendWaypoint:
            if (_draft.replaceEntity(index, withWaypointAdded(entity, *cell))) {
                markDraftMutated();
            }
            break;
        case hmi::ShapeGestureAction::GrabWaypoint:
            _entityDrag = EntityDrag{.mode = EntityDrag::Mode::Reshape,
                                     .indices = {index},
                                     .handle = EntityHandle{.kind = HandleKind::Waypoint,
                                                            .cell = *cell,
                                                            .waypoint = decision.waypoint},
                                     .kind = {},
                                     .from = *cell};
            _entityDragTo = *cell;
            break;
        case hmi::ShapeGestureAction::RemoveWaypoint:
            if (_draft.replaceEntity(index, withWaypointRemoved(entity, decision.waypoint))) {
                markDraftMutated();
            }
            break;
    }
    viewport()->update();
}

void EditorViewport::paintShapeAt(core::GridPosition cell) {
    if (!_selectedEntity || *_selectedEntity >= _draft.entities().size()) {
        return;
    }
    const std::size_t index = *_selectedEntity;
    if (_draft.replaceEntity(index, paintArea(_draft.entities()[index], {cell}, !_shapeErasing))) {
        markDraftMutated();
    }
}

EntityDragResult EditorViewport::pendingEntityDrag() const {
    if (!_entityDrag) {
        return {};
    }
    return dragEntities(*_entityDrag, _draft.entities(), _entityDragTo, _draft.tileMap().width(),
                        _draft.tileMap().height());
}

void EditorViewport::applyEntityDrag(const EntityDragResult& result) {
    if (result.refused) {
        emit statusMessage(QStringLiteral("Move refused: an entity would leave the map."));
        return;
    }
    if (result.empty()) {
        return;
    }
    // Un groupe déplacé, une zone tirée : un geste, un pas d'annulation.
    const EntityDragApplied applied = hmi::applyEntityDrag(_draft, result);
    if (!applied.changed) {
        return;
    }
    markDraftMutated();
    if (applied.placed) {
        selectEntity(*applied.placed);
        emit statusMessage(QStringLiteral("%1 drawn at (%2, %3).")
                               .arg(QString::fromStdString(result.placed->type))
                               .arg(result.placed->position.column)
                               .arg(result.placed->position.row));
    } else {
        emit statusMessage(result.replaced.size() == 1
                               ? QStringLiteral("Entity changed.")
                               : QStringLiteral("%1 entities moved.").arg(result.replaced.size()));
    }
}

namespace {

/// La géométrie d'une vue du canevas, en iso ou à plat : ce que les aides d'édition dessinent.
struct CanvasGeometry {
    CanvasGeometry(core::IsoProjection projection, bool isometric)
        : projected(projection), iso(isometric) {}

    core::IsoProjection projected;
    bool iso;

    [[nodiscard]] QPolygonF cell(core::GridPosition at) const {
        return iso ? diamondOf(projected, at) : QPolygonF(QRectF(at.column, at.row, 1.0, 1.0));
    }
    [[nodiscard]] QPolygonF rect(const CellRect& area) const {
        return iso ? isoRegion(projected, area.origin, area.last())
                   : QPolygonF(
                         QRectF(area.origin.column, area.origin.row, area.columns, area.rows));
    }
    [[nodiscard]] QPointF point(float column, float row) const {
        const core::Vector2 grid{column, row};
        return toQt(iso ? projected.gridToWorld(grid) : grid);
    }
    [[nodiscard]] QPointF center(core::GridPosition at) const {
        return point(static_cast<float>(at.column) + 0.5F, static_cast<float>(at.row) + 0.5F);
    }
    /// Le point d'où part l'étiquette d'une case : au-dessus de son marqueur.
    [[nodiscard]] QPointF above(core::GridPosition at) const {
        if (!iso) {
            return point(static_cast<float>(at.column) + 0.5F, static_cast<float>(at.row) + 0.1F);
        }
        const QPointF middle = center(at);
        return {middle.x(), middle.y() - (projected.tileHeight() * 0.45)};
    }
};

/// Un trajet : la ligne brisée qui joint les centres de ses points.
void paintPath(QPainter& painter, const CanvasGeometry& geometry, const core::MapEntity& entity,
               const QColor& tint, bool chosen) {
    const std::vector<core::GridPosition> points = entityCells(entity);
    painter.setPen(screenPen(withAlpha(tint, 0.9F), chosen ? 3.0 : 2.0));
    for (std::size_t point = 1; point < points.size(); ++point) {
        painter.drawLine(geometry.center(points[point - 1]), geometry.center(points[point]));
    }
}

/// Les formes, sous les marqueurs : trajets, rectangles, zones peintes.
void paintEntityShapes(QPainter& painter, const CanvasGeometry& geometry,
                       const std::vector<core::MapEntity>& entities, const CellRange& cells,
                       const std::function<bool(std::size_t)>& selected) {
    for (std::size_t index = 0; index < entities.size(); ++index) {
        const core::MapEntity& entity = entities[index];
        const core::EntityShape shape = entityShape(entity);
        const QColor tint = kindColor(entity.type);
        const bool chosen = selected(index);
        if (shape == core::EntityShape::Point) {
            continue;
        }
        if (shape == core::EntityShape::Path) {
            paintPath(painter, geometry, entity, tint, chosen);
        } else if (const std::optional<CellRect> rect = entityRectangle(entity)) {
            painter.setPen(screenPen(tint, chosen ? 2.0 : 1.0));
            painter.setBrush(withAlpha(tint, chosen ? 0.20F : 0.08F));
            painter.drawPolygon(geometry.rect(*rect));
        } else {
            painter.setPen(Qt::NoPen);
            painter.setBrush(withAlpha(tint, chosen ? 0.38F : 0.22F));
            for (const core::GridPosition cell : entity.cells) {
                if (cells.contains(cell)) {
                    painter.drawPolygon(geometry.cell(cell));
                }
            }
        }
    }
}

/// Le cadre d'une entité sélectionnée, sur sa case : jaune cerclé de sombre.
void paintSelectedCell(QPainter& painter, const CanvasGeometry& geometry, core::GridPosition cell) {
    painter.setBrush(Qt::NoBrush);
    painter.setPen(screenPen(QColor(13, 13, 13), 4.0));
    painter.drawPolygon(geometry.cell(cell));
    painter.setPen(screenPen(QColor(255, 242, 89), 2.0));
    painter.drawPolygon(geometry.cell(cell));
}

/// Les étiquettes (la carte cible d'un portail, le nom d'une zone), au-dessus de la case. Une
/// étiquette qui en couvrirait une autre déjà écrite se tait — celles de la sélection passent en
/// premier : huit entrées d'arène côte à côte ne font pas un pâté.
void paintEntityLabels(QPainter& painter, const CanvasGeometry& geometry,
                       const std::vector<core::MapEntity>& entities, const CellRange& cells,
                       const std::function<bool(std::size_t)>& selected) {
    std::vector<QRectF> written;
    for (const bool pass : {true, false}) {
        for (std::size_t index = 0; index < entities.size(); ++index) {
            const QString label = QString::fromStdString(entityLabel(entities[index]));
            if (selected(index) != pass || label.isEmpty() ||
                !cells.contains(entities[index].position)) {
                continue;
            }
            const QPointF at = geometry.above(entities[index].position);
            const QRectF box = screenLabelBox(painter, at, label);
            if (std::ranges::any_of(
                    written, [&box](const QRectF& other) { return other.intersects(box); })) {
                continue;
            }
            written.push_back(box);
            drawScreenLabel(painter, at, label,
                            pass ? QColor(255, 242, 89) : QColor(235, 235, 235));
        }
    }
}

/// Les poignées de la sélection : un carré fixe à l'écran, au centre de sa case.
void paintEntityHandles(QPainter& painter, const CanvasGeometry& geometry,
                        const std::vector<core::MapEntity>& entities,
                        const std::function<bool(std::size_t)>& selected) {
    for (std::size_t index = 0; index < entities.size(); ++index) {
        if (!selected(index)) {
            continue;
        }
        for (const EntityHandle& handle : entityHandles(entities[index])) {
            const QPointF device = painter.transform().map(geometry.center(handle.cell));
            painter.save();
            painter.resetTransform();
            painter.setPen(QPen(QColor(13, 13, 13), 1.0));
            painter.setBrush(QColor(255, 255, 255));
            painter.drawRect(QRectF(device.x() - 4.0, device.y() - 4.0, 8.0, 8.0));
            painter.restore();
        }
    }
}

}  // namespace

std::vector<core::MapEntity> EditorViewport::previewEntities(bool withPlaced) const {
    // Ce qu'on voit est ce que le geste en cours ferait : l'aperçu remplace les entités qu'il
    // touche, et l'entité tirée s'ajoute en fin de liste.
    std::vector<core::MapEntity> entities = _draft.entities();
    const EntityDragResult pending = pendingEntityDrag();
    for (const auto& [index, entity] : pending.replaced) {
        if (index < entities.size()) {
            entities[index] = entity;
        }
    }
    if (withPlaced && pending.placed) {
        entities.push_back(*pending.placed);
    }
    return entities;
}

void EditorViewport::paintEntities(QPainter& painter, const CellRange& cells, bool iso) {
    const CanvasGeometry geometry(projection(), iso);
    const std::vector<core::MapEntity> entities = previewEntities(true);
    const bool drawing = entities.size() > _draft.entities().size();
    const std::function<bool(std::size_t)> selected = [&](std::size_t index) {
        return std::ranges::binary_search(_selectedEntities, index) ||
               (drawing && index + 1 == entities.size());
    };
    paintEntityShapes(painter, geometry, entities, cells, selected);

    // Les marqueurs, à la case de chaque entité. À plat, `DraftRenderer` les a déjà posés ; en
    // iso, une entité dont la figurine existe est déjà dessinée par la scène, comme dans le jeu.
    const double markerSide = geometry.projected.tileHeight() * 0.7;
    for (std::size_t index = 0; index < entities.size(); ++index) {
        const core::MapEntity& entity = entities[index];
        if (!cells.contains(entity.position)) {
            continue;
        }
        // Sous l'état de partie (LOT-126), une entité absente n'est pas dans la scène : son
        // marqueur la remplace, grisé.
        const bool absent = _statePreview && !core::isEntityPresent(entity, _stateFlags);
        const std::string figure = entityFigure(entity);
        const bool drawnByScene = !absent && !figure.empty() &&
                                  _referenceContext.figures.contains(figure) &&
                                  index < _draft.entities().size() &&
                                  _draft.entities()[index].position == entity.position;
        if (iso && !drawnByScene) {
            const QPointF center = geometry.center(entity.position);
            const QRectF target(center.x() - (markerSide / 2.0), center.y() - (markerSide / 2.0),
                                markerSide, markerSide);
            painter.setOpacity(absent ? ABSENT_ENTITY_OPACITY : 1.0);
            if (const SceneImage* const marker = _images->marker(entityMarkerKey(entity.type))) {
                painter.drawImage(target, marker->pinned());
            } else {
                painter.fillRect(target, QColor(255, 0, 255, 204));
            }
            painter.setOpacity(1.0);
        }
        if (selected(index)) {
            paintSelectedCell(painter, geometry, entity.position);
        }
    }
    paintEntityLabels(painter, geometry, entities, cells, selected);
    paintEntityHandles(painter, geometry, entities, selected);
}

void EditorViewport::paintZoneVerdict(QPainter& painter, bool iso) {
    if (_tool != hmi::EditorTool::Entity || !_selectedEntity) {
        return;
    }
    // Pendant qu'on tire la zone, le verdict est celui de l'aperçu.
    const std::vector<core::MapEntity> entities = previewEntities(false);
    const std::vector<core::CombatZoneTerrain> previewed =
        _entityDrag ? core::analyzeCombatZones(_draft.tileMap(), entities)
                    : std::vector<core::CombatZoneTerrain>{};
    const std::vector<core::CombatZoneTerrain>& verdicts = _entityDrag ? previewed : _zoneVerdicts;
    const auto found =
        std::ranges::find(verdicts, *_selectedEntity, &core::CombatZoneTerrain::entityIndex);
    if (found == verdicts.end()) {
        return;
    }
    const CanvasGeometry geometry(projection(), iso);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor::fromRgbF(0.30F, 0.70F, 1.00F, 0.14F));
    for (const core::GridPosition cell : found->freeCells) {
        painter.drawPolygon(geometry.cell(cell));
    }
    painter.setBrush(QColor::fromRgbF(0.95F, 0.20F, 0.20F, 0.30F));
    for (const core::GridPosition cell : found->blockedCells) {
        painter.drawPolygon(geometry.cell(cell));
    }
    painter.setBrush(Qt::NoBrush);
    const auto ring = [&](const std::vector<std::size_t>& entries, const QColor& color) {
        painter.setPen(screenPen(color, 2.0));
        for (const std::size_t entry : entries) {
            if (entry < entities.size()) {
                painter.drawPolygon(geometry.cell(entities[entry].position));
            }
        }
    };
    ring(found->entriesInside, QColor(64, 220, 90));
    ring(found->entriesOutside, QColor(240, 60, 60));
    // Le verdict en une ligne, au coin haut-gauche de la zone.
    const core::GridPosition origin = found->zone.origin;
    drawScreenLabel(
        painter, geometry.point(static_cast<float>(origin.column), static_cast<float>(origin.row)),
        QString::fromStdString(combatZoneSummary(*found)),
        found->issue ? QColor(255, 120, 120) : QColor(160, 220, 255));
}

}  // namespace hmi
