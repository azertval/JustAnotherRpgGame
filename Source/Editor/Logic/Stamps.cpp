// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/Stamps.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <variant>

#include "Core/Levels/Level.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/PieceFootprint.h"
#include "Core/Levels/TileTypeName.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/PaintTools.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace hmi {

namespace {

using Json = nlohmann::json;

// Un fichier de la bibliotheque qu'on ne sait pas relire : le message remonte au lecteur, qui le
// rend a l'appelant. Ne quitte jamais ce fichier (EX-NFR-040).
class StampInvalid : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// --- Petites aides de lecture/ecriture JSON -----------------------------------------------------
//
// Le format d'un prefabrique est le sien : il ne passe pas par core::LevelWriter, qui ecrit une
// carte. Les quelques conversions qu'il partage avec lui tiennent ici, en une dizaine de lignes.

[[nodiscard]] Json cellJson(core::GridPosition cell) {
    return Json::array({cell.column, cell.row});
}

[[nodiscard]] core::GridPosition cellOf(const Json& value) {
    if (!value.is_array() || value.size() != 2 || !value[0].is_number_integer() ||
        !value[1].is_number_integer()) {
        throw StampInvalid("a cell is [column, row]");
    }
    return core::GridPosition{.column = value[0].get<int>(), .row = value[1].get<int>()};
}

void writeProperties(const core::PropertyMap& properties, Json& object) {
    for (const auto& [key, value] : properties) {
        if (const auto* held = std::get_if<bool>(&value)) {
            object[key] = *held;
        } else if (const auto* heldInt = std::get_if<std::int64_t>(&value)) {
            object[key] = *heldInt;
        } else if (const auto* heldDouble = std::get_if<double>(&value)) {
            object[key] = *heldDouble;
        } else if (const auto* heldString = std::get_if<std::string>(&value)) {
            object[key] = *heldString;
        }
    }
}

// Les cles reservees d'une entite : tout le reste est une propriete libre.
[[nodiscard]] bool isEntityField(std::string_view key) {
    return key == "type" || key == "x" || key == "y" || key == "elevation" || key == "cells";
}

void readProperties(const Json& object, core::PropertyMap& properties) {
    for (const auto& [key, value] : object.items()) {
        if (isEntityField(key)) {
            continue;
        }
        if (value.is_boolean()) {
            properties[key] = value.get<bool>();
        } else if (value.is_number_integer()) {
            properties[key] = value.get<std::int64_t>();
        } else if (value.is_number_float()) {
            properties[key] = value.get<double>();
        } else if (value.is_string()) {
            properties[key] = value.get<std::string>();
        }
        // Une valeur composite ne rentre pas dans core::PropertyValue : le format n'en produit
        // aucune (voir Core/Levels/LevelProperties.h).
    }
}

[[nodiscard]] core::TileType typeOf(const Json& value) {
    if (!value.is_string()) {
        throw StampInvalid("a tile type is a string");
    }
    const std::optional<core::TileType> type = core::parseTileType(value.get<std::string>());
    if (!type) {
        throw StampInvalid("unknown tile type \"" + value.get<std::string>() + "\"");
    }
    return *type;
}

[[nodiscard]] std::string readText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// --- Le rectangle du tampon ---------------------------------------------------------------------

struct Rect {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    [[nodiscard]] bool contains(core::GridPosition cell) const noexcept {
        return cell.column >= left && cell.column <= right && cell.row >= top && cell.row <= bottom;
    }
    [[nodiscard]] int width() const noexcept {
        return right - left + 1;
    }
    [[nodiscard]] int height() const noexcept {
        return bottom - top + 1;
    }
};

[[nodiscard]] Rect normalized(core::GridPosition first, core::GridPosition last) {
    return Rect{.left = std::min(first.column, last.column),
                .top = std::min(first.row, last.row),
                .right = std::max(first.column, last.column),
                .bottom = std::max(first.row, last.row)};
}

[[nodiscard]] std::size_t typeIndex(const Rect& rect, core::GridPosition cell) {
    return (static_cast<std::size_t>(cell.row - rect.top) *
            static_cast<std::size_t>(rect.width())) +
           static_cast<std::size_t>(cell.column - rect.left);
}

// Les pieces ancrees dans le rectangle l'agrandissent jusqu'a leur emprise entiere : un etal
// 2 x 1 choisi au bord ne perd pas sa moitie droite (voir l'en-tete).
void growRectForAnchoredPieces(const core::LevelDraft& draft,
                               const std::vector<core::TileLayer>& layers,
                               const core::TileMap& root, Rect& rect) {
    for (const core::TileLayer& layer : layers) {
        if (!core::isVisualLayerKind(layer.kind)) {
            continue;
        }
        for (int row = rect.top; row <= rect.bottom; ++row) {
            for (int column = rect.left; column <= rect.right; ++column) {
                const std::string_view piece = layer.pieceAt(column, row);
                if (piece.empty()) {
                    continue;
                }
                const core::PieceFootprint footprint = draft.pieceFootprint(piece);
                rect.right = std::max(rect.right, column + std::max(footprint.columns, 1) - 1);
                rect.bottom = std::max(rect.bottom, row + std::max(footprint.rows, 1) - 1);
            }
        }
    }
    rect.right = std::min(rect.right, root.width() - 1);
    rect.bottom = std::min(rect.bottom, root.height() - 1);
}

[[nodiscard]] StampLayer extractLayer(const core::TileLayer& layer, const Rect& rect,
                                      core::GridPosition origin, int width, int height) {
    StampLayer taken;
    taken.name = layer.name;
    taken.kind = layer.kind;
    taken.floor = layer.floor;
    taken.types.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height),
                       core::TileType::Empty);
    for (int row = rect.top; row <= rect.bottom; ++row) {
        for (int column = rect.left; column <= rect.right; ++column) {
            const core::GridPosition cell{.column = column, .row = row};
            taken.types[typeIndex(rect, cell)] = layer.tiles.tile(column, row);
            const std::string_view piece = layer.pieceAt(column, row);
            if (!piece.empty()) {
                taken.pieces.push_back(
                    StampPiece{.anchor = core::GridPosition{.column = column - origin.column,
                                                            .row = row - origin.row},
                               .piece = std::string{piece},
                               .type = layer.tiles.tile(column, row)});
            }
        }
    }
    return taken;
}

[[nodiscard]] std::vector<core::MapEntity> extractEntities(const core::LevelDraft& draft,
                                                           const Rect& rect,
                                                           core::GridPosition origin) {
    std::vector<core::MapEntity> taken;
    for (const core::MapEntity& entity : draft.entities()) {
        if (!rect.contains(entity.position)) {
            continue;
        }
        core::MapEntity copy = entity;
        copy.id.clear();  // chaque pose en donne un neuf (decision D8).
        copy.position = core::GridPosition{.column = entity.position.column - origin.column,
                                           .row = entity.position.row - origin.row};
        for (core::GridPosition& cell : copy.cells) {
            cell = core::GridPosition{.column = cell.column - origin.column,
                                      .row = cell.row - origin.row};
        }
        taken.push_back(std::move(copy));
    }
    return taken;
}

[[nodiscard]] std::vector<StampForcedCell> extractForced(const core::LevelDraft& draft,
                                                         const core::TileMap& root,
                                                         const Rect& rect,
                                                         core::GridPosition origin) {
    std::vector<StampForcedCell> taken;
    for (const core::GridPosition forced : draft.forcedCollision()) {
        if (!rect.contains(forced)) {
            continue;
        }
        taken.push_back(
            StampForcedCell{.cell = core::GridPosition{.column = forced.column - origin.column,
                                                       .row = forced.row - origin.row},
                            .type = root.tile(forced.column, forced.row)});
    }
    return taken;
}

}  // namespace

// --- Decouper ------------------------------------------------------------------------------------

Stamp cutStamp(const core::LevelDraft& draft, core::GridPosition first, core::GridPosition last) {
    const core::TileMap& root = draft.tileMap();
    Rect rect = normalized(first, last);
    rect.left = std::max(rect.left, 0);
    rect.top = std::max(rect.top, 0);
    rect.right = std::min(rect.right, root.width() - 1);
    rect.bottom = std::min(rect.bottom, root.height() - 1);
    if (rect.width() <= 0 || rect.height() <= 0) {
        return Stamp{};
    }

    const std::vector<core::TileLayer>& layers = draft.layers();
    growRectForAnchoredPieces(draft, layers, root, rect);

    Stamp stamp;
    stamp.width = rect.width();
    stamp.height = rect.height();
    stamp.place = scenePlaceOf(layers);
    const core::GridPosition origin{.column = rect.left, .row = rect.top};

    for (const core::TileLayer& layer : layers) {
        if (!core::isVisualLayerKind(layer.kind)) {
            continue;
        }
        stamp.layers.push_back(extractLayer(layer, rect, origin, stamp.width, stamp.height));
    }

    stamp.entities = extractEntities(draft, rect, origin);
    stamp.forced = extractForced(draft, root, rect, origin);
    return stamp;
}

// --- Le miroir -----------------------------------------------------------------------------------

Stamp mirrorStamp(const Stamp& stamp, const core::ScenePieceManifest* manifest) {
    // Le miroir du tampon est la diagonale de son coin haut gauche : la case (c, r) passe en
    // (r, c), et une emprise de w x h devient h x w -- exactement ce qui fait d'une piece et de sa
    // jumelle l'image l'une de l'autre (hmi::mirrorPieceName).
    Stamp mirrored;
    mirrored.width = stamp.height;
    mirrored.height = stamp.width;
    mirrored.place = stamp.place;
    const auto flip = [](core::GridPosition cell) {
        return core::GridPosition{.column = cell.row, .row = cell.column};
    };
    const Rect rect{
        .left = 0, .top = 0, .right = mirrored.width - 1, .bottom = mirrored.height - 1};

    for (const StampLayer& layer : stamp.layers) {
        StampLayer taken;
        taken.name = layer.name;
        taken.kind = layer.kind;
        taken.floor = layer.floor;
        taken.types.assign(layer.types.size(), core::TileType::Empty);
        for (int row = 0; row < stamp.height; ++row) {
            for (int column = 0; column < stamp.width; ++column) {
                const std::size_t source =
                    (static_cast<std::size_t>(row) * static_cast<std::size_t>(stamp.width)) +
                    static_cast<std::size_t>(column);
                if (source >= layer.types.size()) {
                    continue;
                }
                taken.types[typeIndex(rect,
                                      flip(core::GridPosition{.column = column, .row = row}))] =
                    layer.types[source];
            }
        }
        for (const StampPiece& piece : layer.pieces) {
            taken.pieces.push_back(StampPiece{.anchor = flip(piece.anchor),
                                              .piece = mirrorPieceName(manifest, piece.piece),
                                              .type = piece.type});
        }
        mirrored.layers.push_back(std::move(taken));
    }

    for (const core::MapEntity& entity : stamp.entities) {
        core::MapEntity taken = entity;
        taken.position = flip(entity.position);
        for (core::GridPosition& cell : taken.cells) {
            cell = flip(cell);
        }
        // Une zone rectangle est decrite par des proprietes : sa largeur et sa hauteur s'echangent.
        const auto width = taken.properties.find(std::string{core::ZONE_WIDTH_PROPERTY});
        const auto height = taken.properties.find(std::string{core::ZONE_HEIGHT_PROPERTY});
        if (width != taken.properties.end() && height != taken.properties.end()) {
            std::swap(width->second, height->second);
        }
        mirrored.entities.push_back(std::move(taken));
    }

    for (const StampForcedCell& forced : stamp.forced) {
        mirrored.forced.push_back(StampForcedCell{.cell = flip(forced.cell), .type = forced.type});
    }
    return mirrored;
}

// --- Poser ---------------------------------------------------------------------------------------

namespace {

// La couche de la carte qui recoit celle du tampon : meme nom d'abord, meme role et meme etage
// ensuite -- un toit ne se pose jamais au rez (LOT-129).
[[nodiscard]] std::optional<std::size_t> targetLayer(const std::vector<core::TileLayer>& layers,
                                                     const StampLayer& stamped) {
    for (std::size_t index = 0; index < layers.size(); ++index) {
        if (core::isVisualLayerKind(layers[index].kind) && layers[index].name == stamped.name &&
            layers[index].floor == stamped.floor) {
            return index;
        }
    }
    for (std::size_t index = 0; index < layers.size(); ++index) {
        if (core::isVisualLayerKind(layers[index].kind) && layers[index].kind == stamped.kind &&
            layers[index].floor == stamped.floor) {
            return index;
        }
    }
    return std::nullopt;
}

struct PasteTargets {
    std::string refusal;
    std::vector<std::size_t> indices;
};

// Toutes les couches d'abord : un refus ne doit rien avoir ecrit.
[[nodiscard]] PasteTargets resolvePasteTargets(const core::LevelDraft& draft, const Stamp& stamp,
                                               const LayerViewState& view) {
    PasteTargets result;
    result.indices.reserve(stamp.layers.size());
    const bool hasVisual = std::ranges::any_of(draft.layers(), [](const core::TileLayer& layer) {
        return core::isVisualLayerKind(layer.kind);
    });
    for (const StampLayer& layer : stamp.layers) {
        const std::optional<std::size_t> target = targetLayer(draft.layers(), layer);
        if (!target) {
            result.refusal = "this map has no layer for \"" + layer.name + "\".";
            return result;
        }
        if (view.display(*target, hasVisual).locked) {
            result.refusal = "layer \"" + draft.layers()[*target].name + "\" is locked.";
            return result;
        }
        result.indices.push_back(*target);
    }
    return result;
}

[[nodiscard]] bool pasteLayers(core::LevelDraft& draft, const Stamp& stamp, core::GridPosition at,
                               const core::TileMap& root, const std::vector<std::size_t>& targets) {
    bool changed = false;
    for (std::size_t index = 0; index < stamp.layers.size(); ++index) {
        const StampLayer& layer = stamp.layers[index];
        std::vector<std::vector<core::TileType>> block(
            static_cast<std::size_t>(stamp.height),
            std::vector<core::TileType>(static_cast<std::size_t>(stamp.width),
                                        core::TileType::Empty));
        for (int row = 0; row < stamp.height; ++row) {
            for (int column = 0; column < stamp.width; ++column) {
                const std::size_t source =
                    (static_cast<std::size_t>(row) * static_cast<std::size_t>(stamp.width)) +
                    static_cast<std::size_t>(column);
                if (source < layer.types.size()) {
                    block[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] =
                        layer.types[source];
                }
            }
        }
        changed = draft.paintLayerRegion(targets[index], at.column, at.row, block) || changed;
        for (const StampPiece& piece : layer.pieces) {
            const core::GridPosition anchor{.column = at.column + piece.anchor.column,
                                            .row = at.row + piece.anchor.row};
            if (!root.inBounds(anchor.column, anchor.row)) {
                continue;  // decoupe aux bords, comme un bloc de types.
            }
            changed = draft.placePiece(targets[index], anchor, piece.piece, piece.type) || changed;
        }
    }
    return changed;
}

[[nodiscard]] bool pasteEntities(core::LevelDraft& draft, const Stamp& stamp, core::GridPosition at,
                                 const core::TileMap& root,
                                 std::vector<std::size_t>& placedIndices) {
    bool changed = false;
    for (const core::MapEntity& entity : stamp.entities) {
        core::MapEntity placed = entity;
        placed.position = core::GridPosition{.column = at.column + entity.position.column,
                                             .row = at.row + entity.position.row};
        if (!root.inBounds(placed.position.column, placed.position.row)) {
            continue;
        }
        placed.cells.clear();
        for (const core::GridPosition cell : entity.cells) {
            const core::GridPosition moved{.column = at.column + cell.column,
                                           .row = at.row + cell.row};
            if (root.inBounds(moved.column, moved.row)) {
                placed.cells.push_back(moved);
            }
        }
        if (const std::optional<std::size_t> index = draft.placeEntity(std::move(placed))) {
            placedIndices.push_back(*index);
            changed = true;
        }
    }
    return changed;
}

// Les cases forcees en dernier : la collision des cases posees vient d'etre deduite.
[[nodiscard]] bool pasteForcedCells(core::LevelDraft& draft, const Stamp& stamp,
                                    core::GridPosition at, const core::TileMap& root) {
    bool changed = false;
    for (const StampForcedCell& forced : stamp.forced) {
        const core::GridPosition cell{.column = at.column + forced.cell.column,
                                      .row = at.row + forced.cell.row};
        if (!root.inBounds(cell.column, cell.row) || forced.type == core::TileType::Entry) {
            continue;
        }
        if (root.tile(cell.column, cell.row) != forced.type) {
            draft.paintTile(cell.column, cell.row, forced.type);
            changed = true;
        }
    }
    return changed;
}

}  // namespace

StampPasteResult pasteStamp(core::LevelDraft& draft, const Stamp& stamp, core::GridPosition at,
                            const LayerViewState& view) {
    StampPasteResult result;
    if (stamp.empty()) {
        result.refusal = "nothing to paste.";
        return result;
    }
    const core::TileMap& root = draft.tileMap();
    if (at.column > root.width() - 1 || at.row > root.height() - 1 ||
        at.column + stamp.width <= 0 || at.row + stamp.height <= 0) {
        result.refusal = "the stamp would fall outside the map.";
        return result;
    }

    PasteTargets targets = resolvePasteTargets(draft, stamp, view);
    if (!targets.refusal.empty()) {
        result.refusal = std::move(targets.refusal);
        return result;
    }

    // Un seul geste : toute la pose se defait d'un Ctrl+Z (EX-EDIT-066).
    const core::GestureScope gesture(draft);
    bool changed = pasteLayers(draft, stamp, at, root, targets.indices);
    changed = pasteEntities(draft, stamp, at, root, result.entities) || changed;
    changed = pasteForcedCells(draft, stamp, at, root) || changed;

    result.changed = changed;
    return result;
}

std::string stampLabel(const Stamp& stamp) {
    if (stamp.empty()) {
        return "empty";
    }
    std::size_t pieces = 0;
    for (const StampLayer& layer : stamp.layers) {
        pieces += layer.pieces.size();
    }
    std::string label = std::to_string(stamp.width) + " × " + std::to_string(stamp.height);
    if (pieces > 0) {
        label.append(" · ")
            .append(std::to_string(pieces))
            .append(pieces == 1 ? " piece" : " pieces");
    }
    if (!stamp.entities.empty()) {
        label.append(" · ")
            .append(std::to_string(stamp.entities.size()))
            .append(stamp.entities.size() == 1 ? " entity" : " entities");
    }
    return label;
}

// --- La bibliotheque -----------------------------------------------------------------------------

namespace {

// Une couche du tampon, ses types puis ses pieces.
[[nodiscard]] Json layerToJson(const StampLayer& layer) {
    Json layerJson;
    layerJson["name"] = layer.name;
    layerJson["kind"] = core::layerKindName(layer.kind);
    if (layer.floor != 0) {
        layerJson["floor"] = layer.floor;
    }
    Json types = Json::array();
    for (const core::TileType type : layer.types) {
        types.push_back(core::tileTypeName(type));
    }
    layerJson["types"] = std::move(types);
    if (!layer.pieces.empty()) {
        Json pieces = Json::array();
        for (const StampPiece& piece : layer.pieces) {
            Json pieceJson;
            pieceJson["at"] = cellJson(piece.anchor);
            pieceJson["piece"] = piece.piece;
            pieceJson["type"] = core::tileTypeName(piece.type);
            pieces.push_back(std::move(pieceJson));
        }
        layerJson["pieces"] = std::move(pieces);
    }
    return layerJson;
}

// Une entite du tampon, a sa position relative, avec ses cases et ses proprietes.
[[nodiscard]] Json entityToJson(const core::MapEntity& entity) {
    Json entityJson;
    entityJson["type"] = entity.type;
    entityJson["x"] = entity.position.column;
    entityJson["y"] = entity.position.row;
    if (entity.elevation != 0) {
        entityJson["elevation"] = entity.elevation;
    }
    if (!entity.cells.empty()) {
        Json cells = Json::array();
        for (const core::GridPosition cell : entity.cells) {
            cells.push_back(cellJson(cell));
        }
        entityJson["cells"] = std::move(cells);
    }
    writeProperties(entity.properties, entityJson);
    return entityJson;
}

}  // namespace

nlohmann::json stampToJson(const Stamp& stamp) {
    Json json;
    json["format"] = std::string{PREFAB_FORMAT};
    json["version"] = PREFAB_VERSION;
    json["width"] = stamp.width;
    json["height"] = stamp.height;
    if (!stamp.place.empty()) {
        json["place"] = stamp.place;
    }
    Json layers = Json::array();
    for (const StampLayer& layer : stamp.layers) {
        layers.push_back(layerToJson(layer));
    }
    json["layers"] = std::move(layers);
    if (!stamp.entities.empty()) {
        Json entities = Json::array();
        for (const core::MapEntity& entity : stamp.entities) {
            entities.push_back(entityToJson(entity));
        }
        json["entities"] = std::move(entities);
    }
    if (!stamp.forced.empty()) {
        Json forced = Json::array();
        for (const StampForcedCell& cell : stamp.forced) {
            Json cellJsonValue;
            cellJsonValue["at"] = cellJson(cell.cell);
            cellJsonValue["type"] = core::tileTypeName(cell.type);
            forced.push_back(std::move(cellJsonValue));
        }
        json["forced"] = std::move(forced);
    }
    return json;
}

namespace {

[[nodiscard]] StampLayer layerFromJson(const Json& layerJson, std::size_t cells) {
    StampLayer layer;
    layer.name = layerJson.value("name", std::string{});
    const std::string kind = layerJson.value("kind", std::string{"ground"});
    layer.kind = kind == "decor" ? core::LayerKind::Decor : core::LayerKind::Ground;
    layer.floor = layerJson.value("floor", 0);
    if (layer.floor < 0 || layer.floor > core::MAX_STOREY_FLOOR ||
        (layer.floor != 0 && layer.kind != core::LayerKind::Decor)) {
        throw StampInvalid(R"(layer ")" + layer.name + R"(": "floor" is 0, or 1 to )" +
                           std::to_string(core::MAX_STOREY_FLOOR) + " on a decor layer");
    }
    const auto types = layerJson.find("types");
    if (types == layerJson.end() || !types->is_array() || types->size() != cells) {
        throw StampInvalid("layer \"" + layer.name +
                           "\": \"types\" must hold width × height names");
    }
    for (const Json& type : *types) {
        layer.types.push_back(typeOf(type));
    }
    const auto pieces = layerJson.find("pieces");
    if (pieces != layerJson.end()) {
        if (!pieces->is_array()) {
            throw StampInvalid(R"("pieces" must be a list)");
        }
        for (const Json& pieceJson : *pieces) {
            layer.pieces.push_back(StampPiece{.anchor = cellOf(pieceJson.at("at")),
                                              .piece = pieceJson.value("piece", std::string{}),
                                              .type = typeOf(pieceJson.at("type"))});
        }
    }
    return layer;
}

[[nodiscard]] std::vector<StampLayer> layersFromJson(const Json& json, std::size_t cells) {
    std::vector<StampLayer> layers;
    const auto found = json.find("layers");
    if (found == json.end()) {
        return layers;
    }
    if (!found->is_array()) {
        throw StampInvalid(R"("layers" must be a list)");
    }
    for (const Json& layerJson : *found) {
        layers.push_back(layerFromJson(layerJson, cells));
    }
    return layers;
}

[[nodiscard]] core::MapEntity entityFromJson(const Json& entityJson) {
    core::MapEntity entity;
    entity.type = entityJson.value("type", std::string{});
    entity.position =
        core::GridPosition{.column = entityJson.value("x", 0), .row = entityJson.value("y", 0)};
    entity.elevation = entityJson.value("elevation", 0);
    const auto cellList = entityJson.find("cells");
    if (cellList != entityJson.end() && cellList->is_array()) {
        for (const Json& cell : *cellList) {
            entity.cells.push_back(cellOf(cell));
        }
    }
    readProperties(entityJson, entity.properties);
    return entity;
}

[[nodiscard]] std::vector<core::MapEntity> entitiesFromJson(const Json& json) {
    std::vector<core::MapEntity> entities;
    const auto found = json.find("entities");
    if (found == json.end()) {
        return entities;
    }
    if (!found->is_array()) {
        throw StampInvalid(R"("entities" must be a list)");
    }
    for (const Json& entityJson : *found) {
        entities.push_back(entityFromJson(entityJson));
    }
    return entities;
}

[[nodiscard]] std::vector<StampForcedCell> forcedFromJson(const Json& json) {
    std::vector<StampForcedCell> forced;
    const auto found = json.find("forced");
    if (found == json.end()) {
        return forced;
    }
    if (!found->is_array()) {
        throw StampInvalid(R"("forced" must be a list)");
    }
    for (const Json& cell : *found) {
        forced.push_back(
            StampForcedCell{.cell = cellOf(cell.at("at")), .type = typeOf(cell.at("type"))});
    }
    return forced;
}

// Le corps d'un tampon, partage par le prefabrique et le modele de carte : tout sauf l'en-tete.
[[nodiscard]] Stamp stampBodyFromJson(const Json& json) {
    Stamp stamp;
    stamp.width = json.value("width", 0);
    stamp.height = json.value("height", 0);
    if (stamp.width <= 0 || stamp.height <= 0) {
        throw StampInvalid(R"("width" and "height" must be positive)");
    }
    stamp.place = json.value("place", std::string{});
    const std::size_t cells =
        static_cast<std::size_t>(stamp.width) * static_cast<std::size_t>(stamp.height);
    stamp.layers = layersFromJson(json, cells);
    stamp.entities = entitiesFromJson(json);
    stamp.forced = forcedFromJson(json);
    return stamp;
}

[[nodiscard]] bool headerIs(const Json& json, std::string_view format, int version,
                            std::string& error) {
    if (!json.is_object() || json.value("format", std::string{}) != format) {
        error = "not a " + std::string{format} + " file";
        return false;
    }
    if (json.value("version", 0) != version) {
        error = std::string{format} + " version must be " + std::to_string(version);
        return false;
    }
    return true;
}

}  // namespace

std::optional<Stamp> stampFromJson(const nlohmann::json& json, std::string& error) {
    error.clear();
    if (!headerIs(json, PREFAB_FORMAT, PREFAB_VERSION, error)) {
        return std::nullopt;
    }
    try {
        return stampBodyFromJson(json);
    } catch (const std::exception& invalid) {
        error = invalid.what();
        return std::nullopt;
    }
}

bool isValidPrefabName(std::string_view name) {
    if (name.empty() || name.size() > 64) {
        return false;
    }
    return std::ranges::all_of(name, [](unsigned char letter) {
        return (letter >= 'a' && letter <= 'z') || (letter >= '0' && letter <= '9') ||
               letter == '-' || letter == '_';
    });
}

std::filesystem::path prefabsDir(const std::filesystem::path& dataRoot, std::string_view place) {
    return dataRoot / "Editor" / "Prefabs" / std::filesystem::path(std::string{place});
}

std::vector<std::string> prefabNames(const std::filesystem::path& dataRoot,
                                     std::string_view place) {
    std::vector<std::string> names;
    std::error_code error;
    const std::filesystem::path dir = prefabsDir(dataRoot, place);
    if (!std::filesystem::is_directory(dir, error)) {
        return names;
    }
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(dir, error)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            names.push_back(entry.path().stem().string());
        }
    }
    std::ranges::sort(names);
    return names;
}

std::string writePrefab(const std::filesystem::path& dataRoot, std::string_view place,
                        std::string_view name, const Stamp& stamp) {
    if (!isValidPrefabName(name)) {
        return "a prefab name is made of lowercase letters, digits, dashes and underscores.";
    }
    if (stamp.empty()) {
        return "nothing to save: select a region first.";
    }
    const std::filesystem::path dir = prefabsDir(dataRoot, place);
    std::error_code error;
    std::filesystem::create_directories(dir, error);
    if (error) {
        return "cannot create " + dir.string() + ": " + error.message();
    }
    const std::filesystem::path file = dir / (std::string{name} + ".json");
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (!out) {
        return "cannot write " + file.string();
    }
    out << stampToJson(stamp).dump(2) << '\n';
    if (!out) {
        return "cannot write " + file.string();
    }
    return {};
}

std::optional<Stamp> readPrefab(const std::filesystem::path& dataRoot, std::string_view place,
                                std::string_view name, std::string& error) {
    const std::filesystem::path file = prefabsDir(dataRoot, place) / (std::string{name} + ".json");
    const Json json = Json::parse(readText(file), nullptr, false);
    if (json.is_discarded()) {
        error = file.string() + ": not a JSON file (or it cannot be read)";
        return std::nullopt;
    }
    const std::optional<Stamp> stamp = stampFromJson(json, error);
    if (!stamp) {
        error = file.string() + ": " + error;
    }
    return stamp;
}

// --- Les modeles de carte
// -------------------------------------------------------------------------

std::optional<MapTemplate> mapTemplateFromJson(const nlohmann::json& json, std::string& error) {
    error.clear();
    if (!headerIs(json, MAP_TEMPLATE_FORMAT, MAP_TEMPLATE_VERSION, error)) {
        return std::nullopt;
    }
    try {
        MapTemplate model;
        model.id = json.value("id", std::string{});
        model.label = json.value("label", model.id);
        model.description = json.value("description", std::string{});
        model.width = json.value("width", 0);
        model.height = json.value("height", 0);
        if (model.id.empty()) {
            throw StampInvalid(R"("id" is missing)");
        }
        if (model.width <= 0 || model.height <= 0) {
            throw StampInvalid(R"("width" and "height" must be positive)");
        }
        const auto layers = json.find("layers");
        if (layers == json.end() || !layers->is_array() || layers->empty()) {
            throw StampInvalid(R"("layers" must list at least one layer)");
        }
        for (const Json& layerJson : *layers) {
            MapTemplateLayer layer;
            layer.name = layerJson.value("name", std::string{});
            const std::string kind = layerJson.value("kind", std::string{"ground"});
            if (kind != "ground" && kind != "decor") {
                throw StampInvalid(R"(a template layer is "ground" or "decor")");
            }
            layer.kind = kind == "decor" ? core::LayerKind::Decor : core::LayerKind::Ground;
            layer.scene = layerJson.value("scene", false);
            if (layer.name.empty()) {
                throw StampInvalid("a template layer needs a name");
            }
            model.layers.push_back(std::move(layer));
        }
        const auto entry = json.find("entry");
        if (entry != json.end()) {
            model.entry = cellOf(*entry);
        }
        const auto stamp = json.find("stamp");
        if (stamp != json.end()) {
            model.stamp = stampBodyFromJson(*stamp);
            for (const StampLayer& layer : model.stamp.layers) {
                if (!layer.pieces.empty()) {
                    throw StampInvalid("a template names no piece: it serves every place");
                }
            }
        }
        return model;
    } catch (const std::exception& invalid) {
        error = invalid.what();
        return std::nullopt;
    }
}

namespace {

[[nodiscard]] std::filesystem::path templatesDir(const std::filesystem::path& dataRoot) {
    return dataRoot / "Editor" / "Templates";
}

}  // namespace

std::vector<MapTemplate> mapTemplates(const std::filesystem::path& dataRoot) {
    std::vector<MapTemplate> models;
    std::error_code error;
    const std::filesystem::path dir = templatesDir(dataRoot);
    if (!std::filesystem::is_directory(dir, error)) {
        return models;
    }
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(dir, error)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }
        const Json json = Json::parse(readText(entry.path()), nullptr, false);
        if (json.is_discarded()) {
            continue;
        }
        std::string ignored;
        if (std::optional<MapTemplate> model = mapTemplateFromJson(json, ignored)) {
            models.push_back(std::move(*model));
        }
    }
    std::ranges::sort(models, [](const MapTemplate& left, const MapTemplate& right) {
        return left.id < right.id;
    });
    return models;
}

std::vector<LibraryFinding> checkEditorLibrary(const std::filesystem::path& dataRoot) {
    std::vector<LibraryFinding> findings;
    const auto readAll = [&findings](const std::filesystem::path& dir, bool templates) {
        std::error_code walked;
        if (!std::filesystem::is_directory(dir, walked)) {
            return;
        }
        std::vector<std::filesystem::path> files;
        for (const std::filesystem::directory_entry& entry :
             std::filesystem::recursive_directory_iterator(dir, walked)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                files.push_back(entry.path());
            }
        }
        std::ranges::sort(files);
        for (const std::filesystem::path& file : files) {
            const Json json = Json::parse(readText(file), nullptr, false);
            if (json.is_discarded()) {
                findings.push_back(LibraryFinding{
                    .file = file, .message = "not a JSON file (or it cannot be read)"});
                continue;
            }
            std::string error;
            const bool ok = templates ? mapTemplateFromJson(json, error).has_value()
                                      : stampFromJson(json, error).has_value();
            if (!ok) {
                findings.push_back(LibraryFinding{.file = file, .message = error});
            }
        }
    };
    readAll(dataRoot / "Editor" / "Prefabs", false);
    readAll(templatesDir(dataRoot), true);
    return findings;
}

// --- La bibliotheque sans fenetre ------------------------------------------------------------

namespace {

// Les valeurs qui suivent @p option, jusqu'a la prochaine option (meme lecture que MapRefactor).
[[nodiscard]] std::optional<std::vector<std::string>> valuesOf(
    const std::vector<std::string>& arguments, std::string_view option) {
    const auto found = std::ranges::find(arguments, option);
    if (found == arguments.end()) {
        return std::nullopt;
    }
    std::vector<std::string> values;
    for (auto value = std::next(found); value != arguments.end() && !value->starts_with("--");
         ++value) {
        values.push_back(*value);
    }
    return values;
}

// Une case ecrite « c,r » en ligne de commande.
[[nodiscard]] std::optional<core::GridPosition> cellArgument(const std::string& text) {
    const std::size_t comma = text.find(',');
    if (comma == std::string::npos) {
        return std::nullopt;
    }
    try {
        return core::GridPosition{.column = std::stoi(text.substr(0, comma)),
                                  .row = std::stoi(text.substr(comma + 1))};
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

}  // namespace

std::optional<int> runPrefabCommand(const std::vector<std::string>& arguments,
                                    const std::filesystem::path& dataRoot, std::string& output) {
    const auto usage = [&output](std::string_view text) {
        output += "usage: " + std::string{text} + "\n";
        return 2;
    };
    if (const auto places = valuesOf(arguments, "--list-prefabs")) {
        std::vector<std::string> wanted = *places;
        if (wanted.empty()) {
            wanted = scenePlaces(dataRoot);
        }
        std::size_t total = 0;
        for (const std::string& place : wanted) {
            for (const std::string& name : prefabNames(dataRoot, place)) {
                std::string error;
                const std::optional<Stamp> stamp = readPrefab(dataRoot, place, name, error);
                output.append(place).append("/").append(name).append(": ");
                output.append(stamp ? stampLabel(*stamp) : "unreadable — " + error).append("\n");
                ++total;
            }
        }
        output += std::to_string(total) + (total == 1 ? " prefab\n" : " prefabs\n");
        return 0;
    }
    const auto save = valuesOf(arguments, "--save-prefab");
    if (!save) {
        return std::nullopt;
    }
    const auto from = valuesOf(arguments, "--from");
    const auto to = valuesOf(arguments, "--to");
    if (save->size() != 2 || !from || from->size() != 1 || !to || to->size() != 1) {
        return usage("--save-prefab <map> <name> --from <c,r> --to <c,r>");
    }
    const std::optional<core::GridPosition> first = cellArgument(from->front());
    const std::optional<core::GridPosition> last = cellArgument(to->front());
    if (!first || !last) {
        return usage("--save-prefab <map> <name> --from <c,r> --to <c,r>");
    }
    const std::filesystem::path asPath{(*save)[0]};
    const std::filesystem::path mapFile = std::filesystem::is_regular_file(asPath)
                                              ? asPath
                                              : dataRoot / "Levels" / ((*save)[0] + ".json");
    const core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(mapFile);
    if (!loaded.ok()) {
        output += "error: " + mapFile.string() + ": " + loaded.error + "\n";
        return 1;
    }
    const std::string place = scenePlaceOf(loaded.level->layers());
    core::LevelDraft draft = core::LevelDraft::fromLevel(*loaded.level);
    const PlaceAssets assets = loadPlaceAssets(dataRoot, place);
    if (assets.manifest) {
        draft.setPieceManifest(std::make_shared<const core::ScenePieceManifest>(*assets.manifest));
    }
    const Stamp stamp = cutStamp(draft, *first, *last);
    const std::string error = writePrefab(dataRoot, place, (*save)[1], stamp);
    if (!error.empty()) {
        output += "error: " + error + "\n";
        return 1;
    }
    output += "saved " + place + "/" + (*save)[1] + ": " + stampLabel(stamp) + "\n";
    return 0;
}

}  // namespace hmi
