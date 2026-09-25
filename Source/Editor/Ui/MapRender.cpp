// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/MapRender.h"

#include <QPainter>
#include <QPoint>
#include <QPolygonF>
#include <QSize>
#include <QString>
#include <QStringList>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileTypeName.h"
#include "Core/World/WorldGraph.h"
#include "Editor/Logic/CanvasPicking.h"
#include "Editor/Logic/LayerView.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Ui/SceneImages.h"
#include "Editor/Ui/ScenePainter.h"
#include "HMI/Graphics/Camera2D.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/EntityMarkers.h"
#include "HMI/Graphics/MaquettePalette.h"
#include "HMI/Graphics/MaquetteTokens.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace hmi {

namespace {

constexpr double MAX_SCALE = 4.0;
/// L'échelle minimale d'un rendu : un soixante-quatrième de la carte vue à 1080p.
constexpr double MIN_SCALE = 1.0 / 64.0;
/// Chiffres après la virgule d'une fraction de la grille (`world-maps.json`).
constexpr int GRID_DECIMALS = 5;
/// La moitié : le centre d'une étendue.
constexpr double HALF = 0.5;
/// Le masque de collision : intensité forte et faible d'un canal, et le vert du milieu.
constexpr float OVERLAY_STRONG = 0.85F;
constexpr float OVERLAY_WEAK = 0.20F;
constexpr float OVERLAY_MID = 0.30F;

/// La définition de référence de `--render` : l'échelle 1 est la carte vue à 1080p (`LOT-125`).
constexpr int REFERENCE_VIEW_HEIGHT = 1080;

/// Le vide laissé autour de ce qui est peint, en largeurs de case.
constexpr double FRAME_PADDING_TILES = 0.25;

[[nodiscard]] QPolygonF diamondOf(const core::IsoProjection& projection, core::GridPosition cell) {
    const std::array<core::Vector2, 4> vertices = isoCellDiamond(projection, cell);
    QPolygonF polygon;
    for (const core::Vector2 vertex : vertices) {
        polygon << QPointF(static_cast<double>(vertex.x), static_cast<double>(vertex.y));
    }
    return polygon;
}

// Une grille de types, case par case, en losanges de la couleur que @p color donne (invalide :
// la case n'est pas peinte).
void paintDiamonds(QPainter& painter, const core::IsoProjection& projection,
                   const core::TileMap& tiles, const std::function<QColor(core::TileType)>& color) {
    painter.setPen(Qt::NoPen);
    for (int row = 0; row < tiles.height(); ++row) {
        for (int column = 0; column < tiles.width(); ++column) {
            const QColor tint = color(tiles.tile(column, row));
            if (!tint.isValid()) {
                continue;
            }
            painter.setBrush(tint);
            painter.drawPolygon(diamondOf(projection, {.column = column, .row = row}));
        }
    }
}

[[nodiscard]] QColor withAlpha(QColor color, float alpha) {
    color.setAlphaF(std::clamp(alpha, 0.0F, 1.0F));
    return color;
}

[[nodiscard]] std::string imageNameOf(const std::string& mapId) {
    std::string name = mapId;
    std::ranges::replace(name, '/', '-');
    return name + ".png";
}

/// La qualité des JPEG de l'onglet « Carte » : celle des cartes peintes, sans bloc visible.
constexpr int JPEG_QUALITY = 92;

/// Le plus grand côté d'un cadre imposé (`--canvas`), en pixels.
constexpr int MAX_CANVAS_SIDE = 8192;

// `1920x1080` : un cadre, chaque côté dans [1, MAX_CANVAS_SIDE] ; rien sinon.
[[nodiscard]] std::optional<QSize> parseCanvasSize(const std::string& text) {
    const QStringList sides = QString::fromStdString(text).split(QLatin1Char('x'));
    if (sides.size() != 2) {
        return std::nullopt;
    }
    bool widthOk = false;
    bool heightOk = false;
    const int width = sides[0].toInt(&widthOk);
    const int height = sides[1].toInt(&heightOk);
    if (!widthOk || !heightOk || width < 1 || height < 1 || width > MAX_CANVAS_SIDE ||
        height > MAX_CANVAS_SIDE) {
        return std::nullopt;
    }
    return QSize(width, height);
}

[[nodiscard]] bool isImageFile(const std::filesystem::path& path) {
    const std::filesystem::path extension = path.extension();
    return extension == ".png" || extension == ".jpg" || extension == ".jpeg";
}

// La grille telle que `world-maps.json` l'écrit : trois couples de fractions, à 1e-5 près.
[[nodiscard]] std::string gridJson(const MapImageGrid& grid) {
    const auto pair = [](double x, double y) {
        return QStringLiteral("[%1, %2]")
            .arg(QString::number(x, 'f', GRID_DECIMALS), QString::number(y, 'f', GRID_DECIMALS))
            .toStdString();
    };
    return R"({"origin": )" + pair(grid.originX, grid.originY) + R"(, "column": )" +
           pair(grid.columnX, grid.columnY) + R"(, "row": )" + pair(grid.rowX, grid.rowY) + "}";
}

}  // namespace

double renderPixelsPerUnit(double scale) {
    return std::clamp(scale, MIN_SCALE, MAX_SCALE) *
           static_cast<double>(worldTilePixels(REFERENCE_VIEW_HEIGHT)) /
           static_cast<double>(core::ARENA_TILE_WIDTH_UNITS);
}

std::optional<IsoBandOpacity> parseRenderLayers(std::string_view list) {
    IsoBandOpacity bands{.floors = 0.0F, .relief = 0.0F, .figures = 0.0F, .collision = 0.0F};
    std::size_t start = 0;
    while (start <= list.size()) {
        const std::size_t comma = std::min(list.find(',', start), list.size());
        const std::string_view name = list.substr(start, comma - start);
        if (name == "floors") {
            bands.floors = 1.0F;
        } else if (name == "relief") {
            bands.relief = 1.0F;
        } else if (name == "figures") {
            bands.figures = 1.0F;
        } else if (name == "collision") {
            bands.collision = 1.0F;
        } else if (!name.empty()) {
            return std::nullopt;
        }
        start = comma + 1;
    }
    return bands;
}

QImage renderStamp(const Stamp& stamp, const std::filesystem::path& dataRoot,
                   const std::string& place, int maxSide) {
    if (stamp.empty() || maxSide <= 0) {
        return {};
    }
    // Une carte jetable a la taille du tampon : ses couches sont celles qu'il nomme, dans l'ordre
    // ou il les porte, et le peintre du canevas fait le reste (regle 4 : un seul chemin de rendu).
    core::LevelDraft draft = core::LevelDraft::empty("prefab", stamp.width, stamp.height);
    bool namedPlace = false;
    for (const StampLayer& layer : stamp.layers) {
        const std::optional<std::size_t> added = draft.addLayer(layer.kind, layer.name);
        if (added && layer.floor != 0) {
            draft.setLayerFloor(*added, layer.floor);  // un toit reste a l'etage (LOT-129)
        }
        if (added && !namedPlace && layer.kind == core::LayerKind::Ground && !place.empty()) {
            draft.setLayerProperty(*added, std::string{SCENE_PLACE_PROPERTY}, place);
            namedPlace = true;
        }
    }
    PlaceAssets assets = loadPlaceAssets(dataRoot, place);
    if (assets.manifest) {
        draft.setPieceManifest(std::make_shared<const core::ScenePieceManifest>(*assets.manifest));
    }
    const LayerViewState libre;
    if (!pasteStamp(draft, stamp, core::GridPosition{.column = 0, .row = 0}, libre)
             .refusal.empty()) {
        return {};
    }
    // La carte doit avoir une entree pour se valider ; elle ne se peint pas, la bande des
    // figurines etant eteinte.
    draft.setEntry(0, stamp.height - 1);
    const core::LevelLoadResult level = draft.toLevel();
    if (!level.ok()) {
        return {};
    }
    QImage image =
        renderMap(*level.level, dataRoot,
                  MapRenderOptions{.bands = IsoBandOpacity{},
                                   .scale = 1.0,
                                   // Deux fois la vignette au plus : la réduction lisse le reste,
                                   // sans peindre une image de plein format pour la jeter.
                                   .maxSide = 2 * maxSide,
                                   .background = QColor(0, 0, 0, 0),
                                   .canvas = std::nullopt});
    if (image.isNull()) {
        return image;
    }
    return image.scaled(maxSide, maxSide, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

namespace {

// --- Legende du plan de principe (--plan, LOT-128) ---------------------------------------------

// Les types que la carte emploie vraiment, dans l'ordre de l'enumeration : une legende ne nomme que
// ce qu'on voit sur l'image.
[[nodiscard]] std::vector<core::TileType> typesOf(const WorldSceneSnapshot& snapshot) {
    std::set<int> seen;
    for (const std::vector<core::TileType>* couche : {&snapshot.types, &snapshot.reliefTypes}) {
        for (const core::TileType type : *couche) {
            if (type != core::TileType::Empty) {
                seen.insert(static_cast<int>(type));
            }
        }
    }
    std::vector<core::TileType> types;
    types.reserve(seen.size());
    for (const int value : seen) {
        types.push_back(static_cast<core::TileType>(value));
    }
    return types;
}

// Les natures de jeton posees sur la carte, sans doublon, dans l'ordre de leur premiere apparition.
[[nodiscard]] std::vector<MaquetteTokenKind> tokenKindsOf(const WorldSceneSnapshot& snapshot) {
    std::vector<MaquetteTokenKind> kinds;
    for (const MaquetteTokenSnapshot& token : snapshot.marks.tokens) {
        if (std::ranges::find(kinds, token.kind) == kinds.end()) {
            kinds.push_back(token.kind);
        }
    }
    return kinds;
}

// La legende : une pastille de couleur et son nom, par type puis par nature de jeton.
//
// Peinte en coordonnees ECRAN, la transformation du monde mise de cote : elle doit garder la meme
// taille quelle que soit l'echelle du rendu, comme la legende d'un plan sur le papier.
//
// Ses libelles s'ecrivent avec la table de glyphes des jetons, et non avec QPainter::drawText :
// `--render` tourne sans QApplication (LOT-EDITOR-13, decision D9), donc sans aucune police.
void paintPlanLegend(QPainter& painter, const WorldSceneSnapshot& snapshot, double scale) {
    const std::vector<core::TileType> types = typesOf(snapshot);
    const std::vector<MaquetteTokenKind> kinds = tokenKindsOf(snapshot);
    if (types.empty() && kinds.empty()) {
        return;
    }
    painter.save();
    painter.resetTransform();
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    const int glyph = std::max(2, static_cast<int>(std::lround(scale / 24.0)));
    const int step = glyph * 11;
    const int swatch = glyph * 7;
    const int left = step;
    int top = step;

    const auto line = [&](const QColor& color, std::string_view label, bool round) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        if (round) {
            painter.drawEllipse(QRect(left, top, swatch, swatch));
        } else {
            painter.drawRect(QRect(left, top, swatch, swatch));
        }
        const core::MarkerImage text = maquetteTextImage(
            label, glyph, core::MarkerColor{.r = 0xef, .g = 0xe6, .b = 0xd2, .a = 255});
        if (!text.isEmpty()) {
            // Les pixels sont nommes : une QImage enveloppe leur memoire sans la posseder, et un
            // temporaire mourrait avant le dessin.
            const std::vector<std::uint32_t> pixels = markerPixelsRgba8(text);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            const QImage image(reinterpret_cast<const uchar*>(pixels.data()), text.width,
                               text.height, static_cast<qsizetype>(text.width) * 4,
                               QImage::Format_RGBA8888);
            painter.drawImage(QPoint(left + swatch + (glyph * 3), top), image);
        }
        top += step;
    };

    for (const core::TileType type : types) {
        const MaquetteColor tint = maquetteColor(type);
        line(QColor::fromRgbF(tint.r, tint.g, tint.b), core::tileTypeName(type), false);
    }
    for (const MaquetteTokenKind kind : kinds) {
        const MaquetteColor tint = maquetteTokenColor(kind);
        line(QColor::fromRgbF(tint.r, tint.g, tint.b), maquetteTokenKindKey(kind), true);
    }
    painter.restore();
}

}  // namespace

QImage renderMap(const core::Level& level, const std::filesystem::path& dataRoot,
                 const MapRenderOptions& options, MapImageGrid* grid) {
    const core::LevelDraft draft = core::LevelDraft::fromLevel(level);
    const std::string place = scenePlaceOf(level.layers());
    const PlaceAppearance appearance = [&] {
        PlaceAssets assets = loadPlaceAssets(dataRoot, place);
        return assets.appearance ? std::move(*assets.appearance) : PlaceAppearance{};
    }();
    const WorldSceneSnapshot snapshot = canvasSnapshot(draft, appearance);
    const core::IsoProjection projection(snapshot.columns, snapshot.rows,
                                         core::ARENA_TILE_WIDTH_UNITS, snapshot.diamondRatio);

    const std::shared_ptr<SceneImages> images = SceneImages::shared(dataRoot / "Assets");
    images->ensure(worldTexturePaths(snapshot));
    ComposedScene scene;
    composeWorldScene(scene, snapshot, projection, images->textures(),
                      WorldComposeOptions{.flatBlocks = options.plan});
    scene.sort();

    // Le cadre : ce qui est peint, reliefs compris, et un peu de vide autour (`LOT-125`).
    const core::Rect painted =
        composedSceneBounds(scene, core::Rect{{0.0F, 0.0F}, projection.sceneSize()});
    const double padding = FRAME_PADDING_TILES * static_cast<double>(projection.tileWidth());
    const double left = static_cast<double>(painted.position.x) - padding;
    const double top = static_cast<double>(painted.position.y) - padding;
    const double worldWidth = static_cast<double>(painted.size.x) + (2 * padding);
    const double worldHeight = static_cast<double>(painted.size.y) + (2 * padding);
    double scale = std::min(
        renderPixelsPerUnit(options.scale),
        static_cast<double>(std::max(1, options.maxSide)) / std::max(worldWidth, worldHeight));
    int width = std::clamp(static_cast<int>(std::ceil(worldWidth * scale)), 1,
                           std::max(1, options.maxSide));
    int height = std::clamp(static_cast<int>(std::ceil(worldHeight * scale)), 1,
                            std::max(1, options.maxSide));
    // Le cadre imposé : la carte y tient entière, centrée, le reste est du fond (LOT-121).
    double offsetX = 0.0;
    double offsetY = 0.0;
    if (options.canvas && !options.canvas->isEmpty()) {
        width = options.canvas->width();
        height = options.canvas->height();
        scale = std::min(width / worldWidth, height / worldHeight);
        offsetX = (width - (worldWidth * scale)) * HALF;
        offsetY = (height - (worldHeight * scale)) * HALF;
    }
    if (grid != nullptr) {
        const auto fraction = [&](core::Vector2 gridPoint) {
            const core::Vector2 world = projection.gridToWorld(gridPoint);
            return QPointF((offsetX + ((static_cast<double>(world.x) - left) * scale)) / width,
                           (offsetY + ((static_cast<double>(world.y) - top) * scale)) / height);
        };
        const QPointF origin = fraction({0.0F, 0.0F});
        const QPointF column = fraction({1.0F, 0.0F}) - origin;
        const QPointF row = fraction({0.0F, 1.0F}) - origin;
        *grid = MapImageGrid{.originX = origin.x(),
                             .originY = origin.y(),
                             .columnX = column.x(),
                             .columnY = column.y(),
                             .rowX = row.x(),
                             .rowY = row.y()};
    }

    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(options.background);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setTransform(
        QTransform(scale, 0.0, 0.0, scale, offsetX - (left * scale), offsetY - (top * scale)));

    // Une carte sans lieu n'a plus de chemin de peinture a part : le rendu de maquette est dans la
    // composition, que le jeu, le canevas et `--render` partagent (LOT-128).
    const IsoBandOpacity& bands = options.bands;
    paintComposedScene(painter, scene, std::nullopt,
                       [&bands](const ComposedQuad& quad) { return bandOpacity(bands, quad); });
    if (bands.collision > 0.0F) {
        // Le masque du canevas : rouge ce qui arrête, vert l'entrée.
        paintDiamonds(painter, projection, draft.tileMap(), [&bands](core::TileType type) {
            if (core::isSolid(type)) {
                return withAlpha(QColor::fromRgbF(OVERLAY_STRONG, OVERLAY_WEAK, OVERLAY_WEAK),
                                 bands.collision * DEFAULT_COLLISION_OVERLAY_OPACITY);
            }
            if (type == core::TileType::Entry) {
                return withAlpha(QColor::fromRgbF(OVERLAY_WEAK, OVERLAY_STRONG, OVERLAY_MID),
                                 bands.collision * DEFAULT_COLLISION_OVERLAY_OPACITY);
            }
            return QColor{};
        });
    }
    if (options.plan) {
        paintPlanLegend(painter, snapshot, scale);
    }
    painter.end();
    return image;
}

namespace {

// La ligne de commande de --render ; `error` dit pourquoi elle est fausse.
struct RenderCommandLine {
    bool render = false;
    std::filesystem::path dataRoot;
    std::optional<std::filesystem::path> destination;
    MapRenderOptions options;
    std::vector<std::string> targets;
    std::string error;
};

// `--layers <liste>` : les bandes à peindre.
void applyLayersOption(RenderCommandLine& line, std::string_view list) {
    const std::optional<IsoBandOpacity> bands = parseRenderLayers(list);
    if (bands) {
        line.options.bands = *bands;
    } else {
        line.error = "--layers takes floors, relief, figures, collision";
    }
}

// `--canvas <l>x<h>` : le cadre imposé.
void applyCanvasOption(RenderCommandLine& line, const std::string& size) {
    line.options.canvas = parseCanvasSize(size);
    if (!line.options.canvas) {
        line.error = "--canvas takes <width>x<height>, each in [1, 8192]";
    }
}

// `--scale <n>` : l'échelle, dans ]0, 4].
void applyScaleOption(RenderCommandLine& line, const std::string& value) {
    bool ok = false;
    line.options.scale = QString::fromStdString(value).toDouble(&ok);
    if (!ok || line.options.scale <= 0.0 || line.options.scale > MAX_SCALE) {
        line.error = "--scale takes a number in ]0, 4]";
    }
}

[[nodiscard]] RenderCommandLine parseRenderCommand(const std::vector<std::string>& arguments,
                                                   const std::filesystem::path& defaultDataRoot) {
    RenderCommandLine line;
    line.dataRoot = defaultDataRoot;
    for (std::size_t index = 0; index < arguments.size() && line.error.empty(); ++index) {
        const std::string& argument = arguments[index];
        const bool hasValue = index + 1 < arguments.size();
        if (argument == "--render") {
            line.render = true;
        } else if (argument == "--data" && hasValue) {
            line.dataRoot = arguments[++index];
        } else if (argument == "--output" && hasValue) {
            line.destination = arguments[++index];
        } else if (argument == "--layers" && hasValue) {
            applyLayersOption(line, arguments[++index]);
        } else if (argument == "--plan") {
            line.options.plan = true;
        } else if (argument == "--canvas" && hasValue) {
            applyCanvasOption(line, arguments[++index]);
        } else if (argument == "--scale" && hasValue) {
            applyScaleOption(line, arguments[++index]);
        } else if (line.render && !argument.starts_with("--")) {
            line.targets.push_back(argument);
        }
    }
    return line;
}

// Chaque carte, et le nom de son image : son identifiant, ou le nom de son fichier. Un fichier
// sous Levels/ garde son identifiant (capital/martpart) ; sans carte nommée, toutes.
[[nodiscard]] std::vector<std::pair<std::filesystem::path, std::string>> renderTargets(
    const RenderCommandLine& line) {
    const std::filesystem::path levels = line.dataRoot / "Levels";
    std::vector<std::pair<std::filesystem::path, std::string>> files;
    for (const std::string& target : line.targets) {
        const std::filesystem::path asPath{target};
        if (!std::filesystem::is_regular_file(asPath)) {
            files.emplace_back(levels / (target + ".json"), target);
            continue;
        }
        const std::string mapId =
            core::mapIdOf(std::filesystem::absolute(levels).lexically_normal(),
                          std::filesystem::absolute(asPath).lexically_normal());
        files.emplace_back(
            asPath, mapId.empty() || mapId.starts_with("..") ? asPath.stem().string() : mapId);
    }
    if (line.targets.empty()) {
        for (const std::filesystem::path& file : mapFiles(line.dataRoot)) {
            files.emplace_back(file, core::mapIdOf(levels, file));
        }
    }
    return files;
}

}  // namespace

std::optional<int> runRenderCommand(const std::vector<std::string>& arguments,
                                    const std::filesystem::path& defaultDataRoot,
                                    std::string& output) {
    const RenderCommandLine line = parseRenderCommand(arguments, defaultDataRoot);
    if (!line.render) {
        return std::nullopt;
    }
    if (!line.error.empty()) {
        output += line.error + "\n";
        return 2;
    }
    const std::vector<std::pair<std::filesystem::path, std::string>> files = renderTargets(line);
    const bool singleFile = line.destination && isImageFile(*line.destination);
    if (singleFile && files.size() != 1) {
        output += "--output names an image: render exactly one map, or give a directory\n";
        return 2;
    }
    const std::filesystem::path directory =
        singleFile ? std::filesystem::path{} : line.destination.value_or(".");
    if (!singleFile) {
        std::error_code error;
        std::filesystem::create_directories(directory, error);
    }
    for (const auto& [file, mapId] : files) {
        const core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(file);
        if (!loaded.ok()) {
            output += "error: " + file.string() + ": " + loaded.error + "\n";
            return 1;
        }
        const std::filesystem::path png =
            singleFile ? *line.destination : directory / imageNameOf(mapId);
        MapImageGrid grid;
        const QImage image = renderMap(*loaded.level, line.dataRoot, line.options, &grid);
        // Le format suit l'extension : un JPEG pour l'onglet « Carte » (LOT-121), un PNG sinon.
        const bool jpeg = png.extension() == ".jpg" || png.extension() == ".jpeg";
        if (!image.save(QString::fromStdWString(png.wstring()), jpeg ? "JPG" : "PNG",
                        jpeg ? JPEG_QUALITY : -1)) {
            output += "error: cannot write " + png.string() + "\n";
            return 1;
        }
        output += "rendered " + mapId + " (" + std::to_string(image.width()) + " × " +
                  std::to_string(image.height()) + ") to " + png.string() + "\n";
        if (line.options.canvas) {
            output += "grid " + gridJson(grid) + "\n";
        }
    }
    return 0;
}

}  // namespace hmi
