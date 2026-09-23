// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/GestureScript.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelWriter.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileTypeName.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "Core/World/EntityKinds.h"
#include "Editor/Logic/EntityGesture.h"
#include "Editor/Logic/EntityShapes.h"
#include "Editor/Logic/PieceCatalog.h"
#include "Editor/Logic/Stamps.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace hmi {

namespace {

// Un geste refusé : le message remonte jusqu'à applyGestureScript, qui y ajoute le geste. Ne
// quitte jamais ce fichier (EX-NFR-040).
class GestureRefused : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[nodiscard]] std::string cellText(core::GridPosition cell) {
    return "[" + std::to_string(cell.column) + ", " + std::to_string(cell.row) + "]";
}

// Rejoue un geste : lit ses champs, arme, puis fait ce que la main ferait.
class GesturePlayer {
public:
    GesturePlayer(core::LevelDraft& draft, EditorSidecar& sidecar, const PlaceAssets& assets,
                  std::filesystem::path dataRoot, GestureScriptResult& result)
        : _draft(draft),
          _sidecar(sidecar),
          _assets(assets),
          _dataRoot(std::move(dataRoot)),
          _result(result) {
        _state.view.sync(_draft.layers().size());
    }

    void play(const nlohmann::json& gesture) {
        if (!gesture.is_object()) {
            throw GestureRefused("a gesture is a JSON object");
        }
        _gesture = &gesture;
        arm();
        if (!gesture.contains("tool")) {
            return;  // un geste qui ne fait qu'armer.
        }
        const std::string tool = text("tool");
        if (tool == "paint" || tool == "eraser") {
            stroke(tool == "eraser");
        } else if (tool == "rectangle") {
            report(applyRectangleStroke(_draft, currentBrush(), _state.activeLayer, _state.view,
                                        cell("from"), cell("to"), strokeContext()));
        } else if (tool == "line") {
            report(applyStroke(_draft, currentBrush(), _state.activeLayer, _state.view,
                               lineCells(cell("from"), cell("to")), false, strokeContext()));
        } else if (tool == "bucket") {
            report(applyBucket(_draft, currentBrush(), _state.activeLayer, _state.view, cell("at"),
                               strokeContext()));
        } else if (tool == "pipette") {
            pick(cell("at"));
        } else if (tool == "selection") {
            select();
        } else if (tool == "paste") {
            paste();
        } else if (tool == "entity") {
            entity();
        } else if (tool == "shape") {
            shape();
        } else if (tool == "measure") {
            const core::GridPosition from = cell("from");
            const core::GridPosition to = cell("to");
            _result.log.push_back("measure " + cellText(from) + " → " + cellText(to) + ": " +
                                  measureLabel(measureBetween(from, to)));
        } else if (tool == "note") {
            _result.notesChanged =
                setNote(_sidecar, cell("at"), text("text")) || _result.notesChanged;
        } else {
            throw GestureRefused("unknown tool \"" + tool + "\"");
        }
    }

private:
    // --- Lecture des champs ------------------------------------------------------------------

    [[nodiscard]] const nlohmann::json* field(const char* name) const {
        const auto found = _gesture->find(name);
        return found == _gesture->end() ? nullptr : &*found;
    }

    [[nodiscard]] const nlohmann::json& required(const char* name) const {
        const nlohmann::json* value = field(name);
        if (value == nullptr) {
            throw GestureRefused(std::string{"\""} + name + "\" is missing");
        }
        return *value;
    }

    [[nodiscard]] std::string text(const char* name) const {
        const nlohmann::json& value = required(name);
        if (!value.is_string()) {
            throw GestureRefused(std::string{"\""} + name + "\" must be a string");
        }
        return value.get<std::string>();
    }

    [[nodiscard]] bool flag(const char* name) const {
        const nlohmann::json* value = field(name);
        if (value == nullptr) {
            return false;
        }
        if (!value->is_boolean()) {
            throw GestureRefused(std::string{"\""} + name + "\" must be true or false");
        }
        return value->get<bool>();
    }

    // Une case [colonne, ligne], qui doit être dans la carte : la main ne clique pas à côté.
    [[nodiscard]] core::GridPosition cellOf(const nlohmann::json& value, const char* name) const {
        if (!value.is_array() || value.size() != 2 || !value[0].is_number_integer() ||
            !value[1].is_number_integer()) {
            throw GestureRefused(std::string{"\""} + name + "\" must be a cell [column, row]");
        }
        const core::GridPosition position{.column = value[0].get<int>(),
                                          .row = value[1].get<int>()};
        if (!_draft.tileMap().inBounds(position.column, position.row)) {
            throw GestureRefused("cell " + cellText(position) + " is outside the map (" +
                                 std::to_string(_draft.tileMap().width()) + " × " +
                                 std::to_string(_draft.tileMap().height()) + ")");
        }
        return position;
    }

    [[nodiscard]] core::GridPosition cell(const char* name) const {
        return cellOf(required(name), name);
    }

    // Le trajet d'un glisser : `path`, ou la seule case `at`.
    [[nodiscard]] std::vector<core::GridPosition> path() const {
        std::vector<core::GridPosition> cells;
        if (const nlohmann::json* value = field("path")) {
            if (!value->is_array() || value->empty()) {
                throw GestureRefused("\"path\" must be a non-empty list of cells");
            }
            for (const nlohmann::json& item : *value) {
                cells.push_back(cellOf(item, "path"));
            }
            return cells;
        }
        cells.push_back(cell("at"));
        return cells;
    }

    // --- Armer -------------------------------------------------------------------------------

    [[nodiscard]] LayerSlot layerNamed(const std::string& name) const {
        if (name == "collision") {
            return std::nullopt;
        }
        const std::vector<core::TileLayer>& layers = _draft.layers();
        for (std::size_t index = 0; index < layers.size(); ++index) {
            if (layers[index].name == name && core::isVisualLayerKind(layers[index].kind)) {
                return index;
            }
        }
        throw GestureRefused("no visual layer named \"" + name + "\"");
    }

    void lockLayers(const char* name, bool locked) {
        const nlohmann::json* value = field(name);
        if (value == nullptr) {
            return;
        }
        if (!value->is_array()) {
            throw GestureRefused(std::string{"\""} + name + "\" must be a list of layer names");
        }
        for (const nlohmann::json& layer : *value) {
            if (!layer.is_string()) {
                throw GestureRefused(std::string{"\""} + name + "\" must be a list of layer names");
            }
            _state.view.setLocked(layerNamed(layer.get<std::string>()), locked);
        }
    }

    // Ce que fait EditorViewport::setActiveLayer : une sélection d'une autre couche tromperait le
    // collage.
    void setActiveLayer(LayerSlot slot) {
        const LayerSlot valid = validActiveLayer(_draft.layers(), slot);
        if (valid != _state.activeLayer) {
            _state.activeLayer = valid;
            _state.selection.reset();
        }
    }

    // Ce que fait EditorViewport::setActivePiece : la pièce va sur sa couche, montrée active.
    void armPiece(const std::string& piece, bool floor) {
        _state.brush = pieceBrush(appearance(), piece, floor);
        if (const std::optional<std::size_t> layer =
                pieceTargetLayer(_draft.layers(), floor, _state.activeLayer)) {
            setActiveLayer(*layer);
        }
    }

    void arm() {
        lockLayers("lock", true);
        lockLayers("unlock", false);
        if (field("layer") != nullptr) {
            setActiveLayer(layerNamed(text("layer")));
        }
        if (const nlohmann::json* mirror = field("mirror")) {
            if (mirror->is_boolean() && !mirror->get<bool>()) {
                _state.mirror.reset();
            } else {
                _state.mirror = mirrorAxisThrough(cellOf(*mirror, "mirror"));
            }
        }
        armBrush();
        armEntities();
        armPrefab();
    }

    // La bibliotheque du lieu (LOT-EDITOR-08) : un prefabrique devient le tampon a poser.
    void armPrefab() {
        if (field("prefab") == nullptr) {
            return;
        }
        const std::string name = text("prefab");
        if (_dataRoot.empty()) {
            throw GestureRefused("no data root: a prefab is read under <data>/Editor/Prefabs");
        }
        const std::string place = scenePlaceOf(_draft.layers());
        std::string error;
        std::optional<Stamp> stamp = readPrefab(_dataRoot, place, name, error);
        if (!stamp) {
            throw GestureRefused("prefab \"" + name + "\": " + error);
        }
        _state.clipboard = std::move(*stamp);
    }

    // La palette : un type, ou une pièce de la planche.
    void armBrush() {
        if (field("type") != nullptr) {
            const std::string name = text("type");
            const std::optional<core::TileType> type = core::parseTileType(name);
            if (!type) {
                throw GestureRefused("unknown tile type \"" + name + "\"");
            }
            _state.brush =
                CanvasBrush{.kind = BrushKind::Type, .type = *type, .piece = {}, .floor = false};
        }
        if (field("piece") != nullptr) {
            const std::string name = text("piece");
            const core::ScenePiece* piece =
                _assets.manifest ? _assets.manifest->find(name) : nullptr;
            if (piece == nullptr && field("floor") == nullptr) {
                throw GestureRefused("piece \"" + name +
                                     "\" is not on the sheet of this place: say \"floor\" to "
                                     "place it anyway");
            }
            const bool floor = field("floor") != nullptr
                                   ? flag("floor")
                                   : piece->pieceClass == core::ScenePieceClass::Floor;
            armPiece(name, floor);
        }
    }

    // L'outil Entité : la famille à poser, la sélection.
    void armEntities() {
        if (field("kind") != nullptr) {
            const std::string kind = text("kind");
            if (!kind.empty() && core::findEntityKind(kind) == nullptr) {
                throw GestureRefused("unknown entity kind \"" + kind + "\"");
            }
            _state.kindToPlace = kind;
        }
        if (const nlohmann::json* select = field("select")) {
            if (!select->is_array()) {
                throw GestureRefused("\"select\" must be a list of entity ids");
            }
            std::vector<std::size_t> indices;
            for (const nlohmann::json& id : *select) {
                if (!id.is_string()) {
                    throw GestureRefused("\"select\" must be a list of entity ids");
                }
                indices.push_back(entityIndex(id.get<std::string>()));
            }
            const std::optional<std::size_t> primary =
                indices.empty() ? std::nullopt : std::make_optional(indices.back());
            setEntitySelection(std::move(indices), primary);
        }
    }

    [[nodiscard]] std::size_t entityIndex(const std::string& id) const {
        const std::vector<core::MapEntity>& entities = _draft.entities();
        for (std::size_t index = 0; index < entities.size(); ++index) {
            if (entities[index].id == id) {
                return index;
            }
        }
        throw GestureRefused("no entity with id \"" + id + "\"");
    }

    // Ce que fait EditorViewport::setEntitySelection.
    void setEntitySelection(std::vector<std::size_t> indices, std::optional<std::size_t> primary) {
        std::ranges::sort(indices);
        const auto [first, last] = std::ranges::unique(indices);
        indices.erase(first, last);
        if (!primary || !std::ranges::binary_search(indices, *primary)) {
            primary = indices.empty() ? std::nullopt : std::make_optional(indices.back());
        }
        _state.selectedEntities = std::move(indices);
        _state.selectedEntity = primary;
    }

    // --- Les outils du peintre ---------------------------------------------------------------

    [[nodiscard]] const PlaceAppearance* appearance() const {
        return _assets.appearance ? &*_assets.appearance : nullptr;
    }

    [[nodiscard]] StrokeContext strokeContext() const {
        return StrokeContext{.mirror = _state.mirror, .appearance = appearance()};
    }

    // Ce que fait EditorViewport::currentBrush, hors de la gomme.
    [[nodiscard]] CanvasBrush currentBrush() const {
        CanvasBrush brush = _state.brush;
        if (brush.kind == BrushKind::Piece) {
            brush.type = pieceCellType(appearance(), brush.piece, brush.floor);
        }
        return brush;
    }

    static void report(const BrushResult& result) {
        if (!result.changed && !result.refusal.empty()) {
            throw GestureRefused(result.refusal);
        }
    }

    // Le pinceau ou la gomme, du clic au relâchement : un geste du brouillon ; l'appui sur la
    // première case, puis une case par mouvement, comme EditorViewport::paintAt.
    void stroke(bool erase) {
        const CanvasBrush brush =
            erase ? CanvasBrush{.kind = BrushKind::Eraser, .type = {}, .piece = {}, .floor = false}
                  : currentBrush();
        const std::vector<core::GridPosition> cells = path();
        const core::GestureScope gesture(_draft);
        bool continuing = false;
        for (const core::GridPosition cell : cells) {
            report(applyStroke(_draft, brush, _state.activeLayer, _state.view, {cell}, continuing,
                               strokeContext()));
            continuing = true;
        }
    }

    // Ce que fait EditorViewport::pickAt.
    void pick(core::GridPosition at) {
        const std::optional<PickedBrush> picked =
            pickBrush(_draft, _state.activeLayer, at, appearance());
        if (!picked) {
            throw GestureRefused("nothing to pick at " + cellText(at));
        }
        if (picked->brush.kind == BrushKind::Piece) {
            armPiece(picked->brush.piece, picked->brush.floor);
        } else {
            _state.brush = picked->brush;
            setActiveLayer(picked->layer);
        }
    }

    [[nodiscard]] const core::TileMap& activeLayerTiles() const {
        if (_state.activeLayer && *_state.activeLayer < _draft.layers().size()) {
            return _draft.layers()[*_state.activeLayer].tiles;
        }
        return _draft.tileMap();
    }

    void select() {
        const core::GridPosition from = cell("from");
        const core::GridPosition to = cell("to");
        _state.selection =
            std::make_pair(core::GridPosition{.column = std::min(from.column, to.column),
                                              .row = std::min(from.row, to.row)},
                           core::GridPosition{.column = std::max(from.column, to.column),
                                              .row = std::max(from.row, to.row)});
        if (field("then") == nullptr) {
            return;
        }
        const std::string then = text("then");
        if (then == "copy") {
            // Le tampon entier (LOT-EDITOR-08) : couches, pieces, entites, cases forcees.
            _state.clipboard = cutStamp(_draft, _state.selection->first, _state.selection->second);
        } else if (then == "delete") {
            const CanvasBrush eraser{
                .kind = BrushKind::Eraser, .type = {}, .piece = {}, .floor = false};
            report(applyRectangleStroke(_draft, eraser, _state.activeLayer, _state.view,
                                        _state.selection->first, _state.selection->second,
                                        StrokeContext{}));
        } else {
            throw GestureRefused("a selection is followed by then: copy, or then: delete");
        }
    }

    void paste() {
        if (_state.clipboard.empty()) {
            throw GestureRefused("nothing to paste: copy a selection, or arm a prefab, first");
        }
        const Stamp stamp =
            flag("flip") ? mirrorStamp(_state.clipboard, manifest()) : _state.clipboard;
        const StampPasteResult result = pasteStamp(_draft, stamp, cell("at"), _state.view);
        if (!result.refusal.empty()) {
            throw GestureRefused(result.refusal);
        }
        if (!result.entities.empty()) {
            setEntitySelection(result.entities, result.entities.back());
        }
    }

    [[nodiscard]] const core::ScenePieceManifest* manifest() const {
        return _assets.manifest ? &*_assets.manifest : nullptr;
    }

    // --- Entités et formes -------------------------------------------------------------------

    // Le glisser de l'outil Entité ou Forme, relâché en `to` (la case d'appui à défaut).
    void release(const EntityDrag& drag, core::GridPosition at) {
        const core::GridPosition to = field("to") != nullptr ? cell("to") : at;
        const EntityDragResult result = dragEntities(
            drag, _draft.entities(), to, _draft.tileMap().width(), _draft.tileMap().height());
        if (result.refused) {
            throw GestureRefused("move refused: an entity would leave the map");
        }
        const EntityDragApplied applied = applyEntityDrag(_draft, result);
        if (applied.placed) {
            setEntitySelection({*applied.placed}, *applied.placed);
        }
    }

    // Ce que font EditorViewport::handleEntityPress puis handleEntityRelease.
    void press(core::GridPosition at) {
        const EntityGestureDecision decision = resolveEntityPress(
            _draft, at, _state.selectedEntities, _state.kindToPlace,
            EntityPressModifiers{.force = flag("ctrl"), .toggle = flag("shift")});
        switch (decision.action) {
            case EntityGestureAction::Ignore:
                break;
            case EntityGestureAction::Deselect:
                setEntitySelection({}, std::nullopt);
                break;
            case EntityGestureAction::Toggle:
                setEntitySelection(toggledSelection(_state.selectedEntities, decision.entityIndex),
                                   decision.entityIndex);
                break;
            case EntityGestureAction::Grab:
                if (!std::ranges::binary_search(_state.selectedEntities, decision.entityIndex)) {
                    setEntitySelection({decision.entityIndex}, decision.entityIndex);
                } else {
                    _state.selectedEntity = decision.entityIndex;
                }
                release(EntityDrag{.mode = decision.handle ? EntityDrag::Mode::Reshape
                                                           : EntityDrag::Mode::Move,
                                   .indices = decision.handle
                                                  ? std::vector<std::size_t>{decision.entityIndex}
                                                  : _state.selectedEntities,
                                   .handle = decision.handle,
                                   .kind = {},
                                   .from = at},
                        at);
                break;
            case EntityGestureAction::Draw:
                release(EntityDrag{.mode = EntityDrag::Mode::Draw,
                                   .indices = {},
                                   .handle = std::nullopt,
                                   .kind = _state.kindToPlace,
                                   .from = at},
                        at);
                break;
            case EntityGestureAction::Place:
                if (const std::optional<std::size_t> placed =
                        placeEntityOfKind(_draft, _state.kindToPlace, decision.cell)) {
                    setEntitySelection({*placed}, *placed);
                }
                break;
        }
    }

    [[nodiscard]] static core::PropertyValue propertyValue(const std::string& key,
                                                           const nlohmann::json& value) {
        if (value.is_boolean()) {
            return value.get<bool>();
        }
        if (value.is_number_integer()) {
            return value.get<std::int64_t>();
        }
        if (value.is_number_float()) {
            return value.get<double>();
        }
        if (value.is_string()) {
            return value.get<std::string>();
        }
        throw GestureRefused("property \"" + key + "\" must be a boolean, a number or a string");
    }

    [[nodiscard]] std::size_t primaryEntity() const {
        if (!_state.selectedEntity || *_state.selectedEntity >= _draft.entities().size()) {
            throw GestureRefused("no entity is selected");
        }
        return *_state.selectedEntity;
    }

    void entity() {
        if (field("at") != nullptr) {
            press(cell("at"));
        }
        if (const nlohmann::json* set = field("set")) {
            if (!set->is_object()) {
                throw GestureRefused("\"set\" must be an object of properties");
            }
            const std::size_t index = primaryEntity();
            const core::GestureScope gesture(_draft);
            for (const auto& [key, value] : set->items()) {
                if (!_draft.setEntityProperty(index, key, propertyValue(key, value))) {
                    if (_draft.entities()[index].properties.contains(key)) {
                        continue;  // déjà cette valeur.
                    }
                    throw GestureRefused("property \"" + key + "\" cannot be set");
                }
            }
        }
        if (field("then") != nullptr) {
            if (text("then") != "delete") {
                throw GestureRefused("the entity tool is followed by then: delete, only");
            }
            if (_state.selectedEntities.empty()) {
                throw GestureRefused("no entity is selected");
            }
            static_cast<void>(removeEntities(_draft, _state.selectedEntities));
            setEntitySelection({}, std::nullopt);
        }
    }

    // Ce que font EditorViewport::handleShapePress et paintShapeAt.
    void shape() {
        const std::size_t index = primaryEntity();
        const std::vector<core::GridPosition> cells = path();
        const ShapeGestureDecision decision =
            resolveShapePress(_draft.entities()[index], cells.front(), flag("ctrl"));
        switch (decision.action) {
            case ShapeGestureAction::Ignore:
                throw GestureRefused(
                    "the selected entity has no cells to paint: resize it with the entity tool");
            case ShapeGestureAction::PaintCells:
            case ShapeGestureAction::EraseCells: {
                const bool paint = decision.action == ShapeGestureAction::PaintCells;
                const core::GestureScope gesture(_draft);
                for (const core::GridPosition cell : cells) {
                    static_cast<void>(_draft.replaceEntity(
                        index, paintArea(_draft.entities()[index], {cell}, paint)));
                }
                break;
            }
            case ShapeGestureAction::AppendWaypoint:
                static_cast<void>(_draft.replaceEntity(
                    index, withWaypointAdded(_draft.entities()[index], cells.front())));
                break;
            case ShapeGestureAction::GrabWaypoint:
                release(EntityDrag{.mode = EntityDrag::Mode::Reshape,
                                   .indices = {index},
                                   .handle = EntityHandle{.kind = HandleKind::Waypoint,
                                                          .cell = cells.front(),
                                                          .waypoint = decision.waypoint},
                                   .kind = {},
                                   .from = cells.front()},
                        cells.front());
                break;
            case ShapeGestureAction::RemoveWaypoint:
                static_cast<void>(_draft.replaceEntity(
                    index, withWaypointRemoved(_draft.entities()[index], decision.waypoint)));
                break;
        }
    }

    core::LevelDraft& _draft;
    EditorSidecar& _sidecar;
    const PlaceAssets& _assets;
    std::filesystem::path _dataRoot;
    GestureScriptResult& _result;
    GestureState _state;
    const nlohmann::json* _gesture = nullptr;
};

[[nodiscard]] std::string gestureName(const nlohmann::json& gesture, std::size_t index) {
    std::string name = "gesture " + std::to_string(index + 1);
    if (gesture.is_object()) {
        const auto tool = gesture.find("tool");
        if (tool != gesture.end() && tool->is_string()) {
            name += " (" + tool->get<std::string>() + ")";
        }
    }
    return name;
}

[[nodiscard]] std::string readText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

}  // namespace

GestureScriptResult applyGestureScript(const nlohmann::json& script, core::LevelDraft& draft,
                                       EditorSidecar& sidecar, const PlaceAssets& assets,
                                       const std::filesystem::path& dataRoot) {
    GestureScriptResult result;
    if (!script.is_object() || script.value("format", std::string{}) != GESTURE_SCRIPT_FORMAT) {
        result.error =
            "not a gesture file: its format must be " + std::string{GESTURE_SCRIPT_FORMAT};
        return result;
    }
    const auto version = script.find("version");
    if (version == script.end() || !version->is_number_integer() ||
        version->get<int>() != GESTURE_SCRIPT_VERSION) {
        result.error = "gesture file version must be " + std::to_string(GESTURE_SCRIPT_VERSION);
        return result;
    }
    const auto gestures = script.find("gestures");
    if (gestures == script.end() || !gestures->is_array()) {
        result.error = "\"gestures\" must be a list";
        return result;
    }
    GesturePlayer player(draft, sidecar, assets, dataRoot, result);
    for (std::size_t index = 0; index < gestures->size(); ++index) {
        const nlohmann::json& gesture = (*gestures)[index];
        const std::uint64_t before = draft.revision();
        try {
            player.play(gesture);
        } catch (const GestureRefused& refused) {
            result.error = gestureName(gesture, index) + ": " + refused.what();
            return result;
        } catch (const nlohmann::json::exception& invalid) {
            result.error = gestureName(gesture, index) + ": " + invalid.what();
            return result;
        }
        ++result.gestures;
        if (draft.revision() != before) {
            ++result.steps;
        }
    }
    return result;
}

GestureFileResult applyGestureFile(const std::filesystem::path& scriptFile, std::string_view map,
                                   const std::filesystem::path& dataRoot,
                                   std::filesystem::path& mapFile) {
    GestureFileResult file;
    const std::string origin = scriptFile.string();
    const nlohmann::json script = nlohmann::json::parse(readText(scriptFile), nullptr, false);
    if (script.is_discarded()) {
        file.script.error = origin + ": not a JSON file (or it cannot be read)";
        return file;
    }
    file.mapId = std::string{map};
    if (file.mapId.empty() && script.is_object()) {
        file.mapId = script.value("map", std::string{});
    }
    if (file.mapId.empty()) {
        file.script.error = origin + ": no map to edit (name one, or give \"map\")";
        return file;
    }
    const std::filesystem::path asPath{file.mapId};
    mapFile = std::filesystem::is_regular_file(asPath)
                  ? asPath
                  : dataRoot / "Levels" / (file.mapId + ".json");
    const core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(mapFile);
    if (!loaded.ok()) {
        file.script.error = mapFile.string() + ": " + loaded.error;
        return file;
    }
    const PlaceAssets assets = loadPlaceAssets(dataRoot, scenePlaceOf(loaded.level->layers()));
    core::LevelDraft draft = core::LevelDraft::fromLevel(*loaded.level);
    if (assets.manifest) {
        draft.setPieceManifest(std::make_shared<const core::ScenePieceManifest>(*assets.manifest));
    }
    const std::filesystem::path sidecarFile = sidecarPath(mapFile);
    EditorSidecar sidecar = readSidecar(sidecarFile).sidecar;

    file.script = applyGestureScript(script, draft, sidecar, assets, dataRoot);
    if (!file.script.ok()) {
        file.script.error = origin + ": " + file.script.error;
        return file;
    }
    // Ce que l'enregistrement de la fenêtre fait : la carte se valide avant de s'écrire.
    const core::LevelLoadResult validated = draft.toLevel();
    if (!validated.ok()) {
        file.script.error = origin + ": the edited map is not valid: " + validated.error;
        return file;
    }
    file.mapText = core::LevelWriter::toJsonString(*validated.level);
    if (file.script.notesChanged) {
        file.sidecar = std::move(sidecar);
    }
    return file;
}

}  // namespace hmi
