// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/MapRender.h"

#include <QPainter>
#include <QPolygonF>
#include <QString>
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <utility>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileLayer.h"
#include "Core/World/WorldGraph.h"
#include "Editor/Logic/CanvasPicking.h"
#include "Editor/Logic/LayerView.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Ui/SceneImages.h"
#include "Editor/Ui/ScenePainter.h"
#include "HMI/Graphics/Camera2D.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace hmi {

namespace {

constexpr double MAX_SCALE = 4.0;

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

}  // namespace

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

QImage renderMap(const core::Level& level, const std::filesystem::path& dataRoot,
                 const MapRenderOptions& options) {
    const core::LevelDraft draft = core::LevelDraft::fromLevel(level);
    const std::string place = scenePlaceOf(level.layers());
    const PlaceAppearance appearance = [&] {
        PlaceAssets assets = loadPlaceAssets(dataRoot, place);
        return assets.appearance ? std::move(*assets.appearance) : PlaceAppearance{};
    }();
    const WorldSceneSnapshot snapshot = canvasSnapshot(draft, appearance);
    const core::IsoProjection projection(snapshot.columns, snapshot.rows,
                                         core::ARENA_TILE_WIDTH_UNITS, snapshot.diamondRatio);

    SceneImages images(dataRoot / "Assets");
    images.ensure(worldTexturePaths(snapshot));
    ComposedScene scene;
    composeWorldScene(scene, snapshot, projection, images.textures());
    scene.sort();

    // Le cadre du canevas : la carte et une marge d'un losange, où dépassent les reliefs.
    const core::Vector2 size = projection.sceneSize();
    const double margin = projection.tileWidth();
    const double scale = static_cast<double>(Camera2D::PIXELS_PER_UNIT) *
                         std::clamp(options.scale, 1.0 / 64.0, MAX_SCALE);
    const int width = std::max(
        1, static_cast<int>(std::ceil((static_cast<double>(size.x) + (2 * margin)) * scale)));
    const int height = std::max(
        1, static_cast<int>(std::ceil((static_cast<double>(size.y) + (2 * margin)) * scale)));

    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(options.background);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setTransform(QTransform(scale, 0.0, 0.0, scale, margin * scale, margin * scale));

    const IsoBandOpacity& bands = options.bands;
    if (place.empty() && bands.floors > 0.0F) {
        // Une carte sans lieu n'a aucune pièce : ses types, en couleurs, comme dans le canevas.
        const auto typeColor = [&images, &bands](core::TileType type) {
            return type == core::TileType::Empty ? QColor{}
                                                 : withAlpha(images.tileColor(type), bands.floors);
        };
        bool visual = false;
        for (const core::TileLayer& layer : draft.layers()) {
            if (core::isVisualLayerKind(layer.kind)) {
                visual = true;
                paintDiamonds(painter, projection, layer.tiles, typeColor);
            }
        }
        if (!visual) {
            paintDiamonds(painter, projection, draft.tileMap(), typeColor);
        }
    }
    paintComposedScene(painter, scene, std::nullopt, [&bands](const ComposedQuad& quad) {
        return bandOpacity(bands, quad.layer);
    });
    if (bands.collision > 0.0F) {
        // Le masque du canevas : rouge ce qui arrête, vert l'entrée.
        paintDiamonds(painter, projection, draft.tileMap(), [&bands](core::TileType type) {
            if (core::isSolid(type)) {
                return withAlpha(QColor::fromRgbF(0.85F, 0.20F, 0.20F),
                                 bands.collision * DEFAULT_COLLISION_OVERLAY_OPACITY);
            }
            if (type == core::TileType::Entry) {
                return withAlpha(QColor::fromRgbF(0.20F, 0.85F, 0.30F),
                                 bands.collision * DEFAULT_COLLISION_OVERLAY_OPACITY);
            }
            return QColor{};
        });
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
            const std::optional<IsoBandOpacity> bands = parseRenderLayers(arguments[++index]);
            if (bands) {
                line.options.bands = *bands;
            } else {
                line.error = "--layers takes floors, relief, figures, collision";
            }
        } else if (argument == "--scale" && hasValue) {
            bool ok = false;
            line.options.scale = QString::fromStdString(arguments[++index]).toDouble(&ok);
            if (!ok || line.options.scale <= 0.0 || line.options.scale > MAX_SCALE) {
                line.error = "--scale takes a number in ]0, 4]";
            }
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
    const bool singleFile = line.destination && line.destination->extension() == ".png";
    if (singleFile && files.size() != 1) {
        output += "--output names a .png: render exactly one map, or give a directory\n";
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
        const QImage image = renderMap(*loaded.level, line.dataRoot, line.options);
        if (!image.save(QString::fromStdWString(png.wstring()), "PNG")) {
            output += "error: cannot write " + png.string() + "\n";
            return 1;
        }
        output += "rendered " + mapId + " (" + std::to_string(image.width()) + " × " +
                  std::to_string(image.height()) + ") to " + png.string() + "\n";
    }
    return 0;
}

}  // namespace hmi
