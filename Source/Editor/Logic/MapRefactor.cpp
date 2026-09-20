// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/MapRefactor.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <system_error>
#include <utility>
#include <variant>

#include <nlohmann/json.hpp>

#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelWriter.h"
#include "Core/Levels/MapEntity.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/WorldGraph.h"
#include "Editor/Logic/EditorSidecar.h"
#include "Editor/Logic/EntityReferences.h"
#include "Editor/Logic/LevelNameValidation.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/MapTexts.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace hmi {

namespace {

/// Nom du format d'une table de correspondance (champ `format`).
constexpr std::string_view PIECE_TABLE_FORMAT = "jadg-piece-table";

[[nodiscard]] std::string readText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

[[nodiscard]] std::filesystem::path levelsOf(const std::filesystem::path& dataRoot) {
    return dataRoot / "Levels";
}

[[nodiscard]] std::filesystem::path mapFileOf(const std::filesystem::path& dataRoot,
                                              std::string_view mapId) {
    return levelsOf(dataRoot) / (std::string{mapId} + ".json");
}

[[nodiscard]] std::string entityLabel(const core::MapEntity& entity) {
    return entity.type + " " + entity.id;
}

// --- Le projet, lu une fois ------------------------------------------------------------------

struct ProjectMap {
    std::string id;
    std::filesystem::path file;
    core::LevelData data;
};

struct Project {
    std::vector<ProjectMap> maps;
    std::string error;

    [[nodiscard]] ProjectMap* find(std::string_view id) {
        const auto found =
            std::ranges::find_if(maps, [id](const ProjectMap& map) { return map.id == id; });
        return found != maps.end() ? &*found : nullptr;
    }
};

// Toutes les cartes : une carte illisible refuse l'opération, ses citations ne se verraient pas.
[[nodiscard]] Project loadProject(const std::filesystem::path& dataRoot) {
    Project project;
    const std::filesystem::path levels = levelsOf(dataRoot);
    for (const std::filesystem::path& file : mapFiles(dataRoot)) {
        core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(file);
        if (!loaded.ok()) {
            project.error = core::mapIdOf(levels, file) + " cannot be read: " + loaded.error;
            return project;
        }
        project.maps.push_back(ProjectMap{
            .id = core::mapIdOf(levels, file), .file = file, .data = loaded.level->data()});
    }
    return project;
}

// --- Propriétés d'entité, par leur source ------------------------------------------------------

// Appelle @p visit(entité, clé, valeur) pour chaque propriété texte dont la famille déclare la
// source @p source : ce qui cite une carte, un point d'arrivée, une entité (règle 2).
void forEachReference(
    std::vector<core::MapEntity>& entities, core::EntityChoiceSource source,
    const std::function<void(core::MapEntity&, std::string_view, std::string&)>& visit) {
    for (core::MapEntity& entity : entities) {
        const core::EntityKind* kind = core::findEntityKind(entity.type);
        if (kind == nullptr) {
            continue;
        }
        for (const core::EntityPropertySpec& spec : kind->properties) {
            if (spec.source != source) {
                continue;
            }
            const auto found = entity.properties.find(std::string{spec.key});
            if (found == entity.properties.end()) {
                continue;
            }
            if (std::string* text = std::get_if<std::string>(&found->second)) {
                visit(entity, spec.key, *text);
            }
        }
    }
}

// La carte que nomme l'entité (sa propriété de source `Maps`), vide sinon : c'est de là que ses
// points d'arrivée sont tirés.
[[nodiscard]] std::string targetMapOf(const core::MapEntity& entity) {
    const core::EntityKind* kind = core::findEntityKind(entity.type);
    if (kind == nullptr) {
        return {};
    }
    for (const core::EntityPropertySpec& spec : kind->properties) {
        if (spec.source != core::EntityChoiceSource::Maps) {
            continue;
        }
        const auto found = entity.properties.find(std::string{spec.key});
        if (found != entity.properties.end()) {
            if (const std::string* text = std::get_if<std::string>(&found->second)) {
                return *text;
            }
        }
    }
    return {};
}

[[nodiscard]] bool isSpawnPoint(const core::MapEntity& entity, std::string_view name) {
    if (entity.type != core::SPAWN_POINT_ENTITY_TYPE) {
        return false;
    }
    const auto found = entity.properties.find(std::string{core::SPAWN_POINT_NAME_PROPERTY});
    const std::string* text =
        found != entity.properties.end() ? std::get_if<std::string>(&found->second) : nullptr;
    return text != nullptr && *text == name;
}

// --- Les villes --------------------------------------------------------------------------------

struct CityFile {
    std::filesystem::path file;
    nlohmann::ordered_json json;
};

[[nodiscard]] std::vector<CityFile> loadCities(const std::filesystem::path& dataRoot) {
    std::vector<CityFile> cities;
    std::error_code error;
    const std::filesystem::path directory = dataRoot / "World" / "cities";
    std::vector<std::filesystem::path> files;
    for (auto entry = std::filesystem::directory_iterator(directory, error);
         !error && entry != std::filesystem::directory_iterator(); entry.increment(error)) {
        if (entry->is_regular_file() && entry->path().extension() == ".json") {
            files.push_back(entry->path());
        }
    }
    std::ranges::sort(files);
    for (const std::filesystem::path& file : files) {
        nlohmann::ordered_json json =
            nlohmann::ordered_json::parse(readText(file), nullptr, /*allow_exceptions=*/false);
        if (json.is_object()) {
            cities.push_back(CityFile{.file = file, .json = std::move(json)});
        }
    }
    return cities;
}

// Le texte d'un fichier de données (une ville, une table d'apparence) tel que le dépôt l'écrit :
// indenté de deux espaces, UTF-8 brut.
[[nodiscard]] std::string jsonText(const nlohmann::ordered_json& json) {
    return json.dump(2) + "\n";
}

// Appelle @p visit(champ, citation) pour chaque carte qu'une ville nomme : `map` d'un quartier,
// `guard.map` d'une porte gardée.
void forEachCityMap(nlohmann::ordered_json& city,
                    const std::function<void(nlohmann::ordered_json&, std::string)>& visit) {
    if (!city.contains("districts") || !city["districts"].is_array()) {
        return;
    }
    for (nlohmann::ordered_json& district : city["districts"]) {
        const std::string id = district.value("id", std::string{});
        if (district.contains("map") && district["map"].is_string()) {
            visit(district["map"], "district " + id + ": map");
        }
        if (district.contains("guard") && district["guard"].is_object() &&
            district["guard"].contains("map") && district["guard"]["map"].is_string()) {
            visit(district["guard"]["map"], "district " + id + ": guard map");
        }
    }
}

// La carte du quartier de départ d'une ville, vide s'il n'en a pas.
[[nodiscard]] std::string cityStartMap(const nlohmann::ordered_json& city) {
    if (!city.contains("start") || !city["start"].is_object() || !city.contains("districts")) {
        return {};
    }
    const std::string district = city["start"].value("district", std::string{});
    for (const nlohmann::ordered_json& entry : city["districts"]) {
        if (entry.value("id", std::string{}) == district) {
            return entry.value("map", std::string{});
        }
    }
    return {};
}

// --- Les catalogues ----------------------------------------------------------------------------

[[nodiscard]] std::string trimmed(std::string_view text) {
    const std::size_t first = text.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(" \t\r");
    return std::string{text.substr(first, last - first + 1)};
}

// La clé d'une ligne de catalogue, vide pour un commentaire ou une ligne sans `=`.
[[nodiscard]] std::string catalogKey(std::string_view line) {
    const std::string content = trimmed(line);
    if (content.empty() || content.front() == '#') {
        return {};
    }
    const std::size_t equal = line.find('=');
    return equal == std::string_view::npos ? std::string{} : trimmed(line.substr(0, equal));
}

// Le texte d'un catalogue dont la clé @p from devient @p to, ligne gardée à sa place ; vide si la
// clé n'y est pas.
[[nodiscard]] std::optional<std::string> renameCatalogKey(const std::string& text,
                                                          std::string_view from,
                                                          std::string_view to) {
    std::string result;
    bool renamed = false;
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t end = text.find('\n', start);
        end = end == std::string::npos ? text.size() : end + 1;
        std::string line = text.substr(start, end - start);
        if (catalogKey(line) == from) {
            const std::size_t at = line.find(from);
            line.replace(at, from.size(), to);
            renamed = true;
        }
        result += line;
        start = end;
    }
    return renamed ? std::optional<std::string>{result} : std::nullopt;
}

[[nodiscard]] bool hasCatalogKey(const std::string& text, std::string_view key) {
    return renameCatalogKey(text, key, key).has_value();
}

[[nodiscard]] std::vector<std::filesystem::path> catalogFiles(
    const std::filesystem::path& dataRoot) {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    for (auto entry = std::filesystem::directory_iterator(localizationDirectory(dataRoot), error);
         !error && entry != std::filesystem::directory_iterator(); entry.increment(error)) {
        if (entry->is_regular_file() && entry->path().extension() == ".lang") {
            files.push_back(entry->path());
        }
    }
    std::ranges::sort(files);
    return files;
}

// --- Plans -------------------------------------------------------------------------------------

[[nodiscard]] RefactorPlan refused(std::string error) {
    RefactorPlan plan;
    plan.error = std::move(error);
    return plan;
}

[[nodiscard]] Citation mapCitation(const ProjectMap& map, const core::MapEntity& entity,
                                   std::string_view key) {
    return Citation{.file = map.file,
                    .mapId = map.id,
                    .entityId = entity.id,
                    .cell = entity.position,
                    .what = entityLabel(entity) + ": " + std::string{key}};
}

// Ajoute au plan le texte des cartes changées, par l'écrivain canonique.
void writeMaps(RefactorPlan& plan, const Project& project, const std::set<std::string>& changed) {
    for (const ProjectMap& map : project.maps) {
        if (changed.contains(map.id)) {
            plan.edits.push_back(
                ProjectEdit{.file = map.file, .text = core::LevelWriter::buildJson(map.data)});
        }
    }
}

// Un identifiant de carte : des segments de nom valides, séparés par `/`.
[[nodiscard]] bool isValidMapId(std::string_view id) {
    if (id.empty() || id.front() == '/' || id.back() == '/') {
        return false;
    }
    std::size_t start = 0;
    while (start <= id.size()) {
        std::size_t end = id.find('/', start);
        end = end == std::string_view::npos ? id.size() : end;
        const std::string segment{id.substr(start, end - start)};
        if (segment.empty() || segment == "." || segment == ".." || !isValidLevelName(segment) ||
            trimLevelName(segment) != segment || segment.ends_with(".editor") ||
            segment.starts_with("sequence-")) {
            return false;
        }
        start = end + 1;
    }
    return true;
}

[[nodiscard]] PlaceAssets placeOf(const std::filesystem::path& dataRoot,
                                  const core::LevelData& data) {
    return loadPlaceAssets(dataRoot, scenePlaceOf(data.layers));
}

[[nodiscard]] core::LevelDraft draftOf(const core::LevelData& data, const PlaceAssets& assets) {
    core::LevelDraft draft = core::LevelDraft::fromLevel(core::Level(data));
    if (assets.manifest) {
        draft.setPieceManifest(std::make_shared<const core::ScenePieceManifest>(*assets.manifest));
    }
    return draft;
}

// Les pièces que posent les couches visuelles, triées.
[[nodiscard]] std::set<std::string, std::less<>> citedPieces(
    const std::vector<core::TileLayer>& layers) {
    std::set<std::string, std::less<>> pieces;
    for (const core::TileLayer& layer : layers) {
        if (!core::isVisualLayerKind(layer.kind)) {
            continue;
        }
        for (const std::string& piece : layer.pieces) {
            if (!piece.empty()) {
                pieces.insert(piece);
            }
        }
    }
    return pieces;
}

// Les cases de @p layer qui posent @p piece.
[[nodiscard]] std::vector<core::GridPosition> cellsOf(const core::TileLayer& layer,
                                                      std::string_view piece) {
    std::vector<core::GridPosition> cells;
    for (int row = 0; row < layer.tiles.height(); ++row) {
        for (int column = 0; column < layer.tiles.width(); ++column) {
            if (layer.pieceAt(column, row) == piece) {
                cells.push_back({.column = column, .row = row});
            }
        }
    }
    return cells;
}

[[nodiscard]] std::filesystem::path appearanceFile(const std::filesystem::path& dataRoot,
                                                   std::string_view place) {
    return dataRoot / "Assets" / "Scene" / std::string{place} / "appearance.json";
}

// La table d'apparence de @p to, faite de celle de @p from traduite par @p table ; rien si @p to
// en a déjà une ou si @p from n'en a pas.
[[nodiscard]] std::optional<std::string> translatedAppearance(const std::filesystem::path& dataRoot,
                                                              std::string_view from,
                                                              std::string_view to,
                                                              const core::PieceRenaming& table) {
    std::error_code error;
    if (from.empty() || std::filesystem::exists(appearanceFile(dataRoot, to), error)) {
        return std::nullopt;
    }
    nlohmann::ordered_json json = nlohmann::ordered_json::parse(
        readText(appearanceFile(dataRoot, from)), nullptr, /*allow_exceptions=*/false);
    if (!json.is_object()) {
        return std::nullopt;
    }
    json["place"] = std::string{to};
    json["comment"] = "Made from the table of " + std::string{from} +
                      " when a map changed sheet (LOT-EDITOR-14), pieces renamed by its table.";
    for (const char* section : {"floors", "relief"}) {
        if (!json.contains(section) || !json[section].is_object()) {
            continue;
        }
        for (const auto& entry : json[section].items()) {
            nlohmann::ordered_json& pieces = entry.value();
            if (!pieces.is_array()) {
                continue;
            }
            for (nlohmann::ordered_json& piece : pieces) {
                if (const auto found = table.find(piece.get<std::string>()); found != table.end()) {
                    piece = found->second;
                }
            }
        }
    }
    return jsonText(json);
}

}  // namespace

std::string formatCitation(const Citation& citation, const std::filesystem::path& dataRoot) {
    std::string where = citation.mapId;
    if (where.empty()) {
        std::error_code error;
        const std::filesystem::path relative =
            std::filesystem::relative(citation.file, dataRoot, error);
        where = (error || relative.empty() ? citation.file : relative).generic_string();
    }
    if (citation.cell) {
        where += " (" + std::to_string(citation.cell->column) + ", " +
                 std::to_string(citation.cell->row) + ")";
    }
    return where + ": " + citation.what;
}

bool applyRefactorPlan(const RefactorPlan& plan, std::string& error) {
    std::error_code code;
    for (const ProjectEdit& edit : plan.edits) {
        if (!edit.text) {
            continue;
        }
        std::filesystem::create_directories(edit.file.parent_path(), code);
        std::ofstream stream(edit.file, std::ios::binary);
        stream.write(edit.text->data(), static_cast<std::streamsize>(edit.text->size()));
        stream.close();
        if (!stream.good()) {
            error = "cannot write " + edit.file.string();
            return false;
        }
    }
    for (const ProjectEdit& edit : plan.edits) {
        if (edit.text) {
            continue;
        }
        std::filesystem::remove(edit.file, code);
        // Un dossier de cartes vidé par un déplacement ne reste pas.
        const std::filesystem::path parent = edit.file.parent_path();
        if (parent.filename() != "Levels" && std::filesystem::is_empty(parent, code) && !code) {
            std::filesystem::remove(parent, code);
        }
    }
    return true;
}

// --- Qui cite ceci ? -----------------------------------------------------------------------------

std::vector<Citation> citationsOfMap(const std::filesystem::path& dataRoot,
                                     std::string_view mapId) {
    return planRenameMap(dataRoot, mapId, "").changes;
}

std::vector<Citation> citationsOfArrival(const std::filesystem::path& dataRoot,
                                         std::string_view mapId, std::string_view arrival) {
    return planRenameArrival(dataRoot, mapId, arrival, "").changes;
}

std::vector<Citation> citationsOfEntity(const std::filesystem::path& dataRoot,
                                        std::string_view mapId, std::string_view entityId) {
    return planRenameEntityId(dataRoot, mapId, entityId, "").changes;
}

namespace {

// Une citation par couche visuelle de @p map qui place @p piece.
std::vector<Citation> pieceCitationsIn(const ProjectMap& map, std::string_view piece) {
    std::vector<Citation> citations;
    for (const core::TileLayer& layer : map.data.layers) {
        if (!core::isVisualLayerKind(layer.kind) || !layer.hasPieces()) {
            continue;
        }
        const std::vector<core::GridPosition> cells = cellsOf(layer, piece);
        if (!cells.empty()) {
            citations.push_back(Citation{.file = map.file,
                                         .mapId = map.id,
                                         .cell = cells.front(),
                                         .what = "layer " + layer.name + ": " +
                                                 std::to_string(cells.size()) +
                                                 (cells.size() == 1 ? " cell" : " cells")});
        }
    }
    return citations;
}

}  // namespace

std::vector<Citation> citationsOfPiece(const std::filesystem::path& dataRoot,
                                       std::string_view piece) {
    std::vector<Citation> citations;
    Project project = loadProject(dataRoot);
    for (const ProjectMap& map : project.maps) {
        std::vector<Citation> found = pieceCitationsIn(map, piece);
        citations.insert(citations.end(), std::make_move_iterator(found.begin()),
                         std::make_move_iterator(found.end()));
    }
    return citations;
}

// --- Renommer ------------------------------------------------------------------------------------

namespace {

// Pourquoi la carte @p oldId ne se renomme pas en @p newId (projet illisible, carte inconnue, nom
// pris) ; std::nullopt si rien ne s'y oppose.
std::optional<std::string> mapRenameRefusal(Project& project, const std::filesystem::path& dataRoot,
                                            std::string_view oldId, std::string_view newId) {
    if (!project.error.empty()) {
        return project.error;
    }
    if (project.find(oldId) == nullptr) {
        return "no map \"" + std::string{oldId} + "\"";
    }
    if (!newId.empty()) {
        std::error_code error;
        if (project.find(newId) != nullptr ||
            std::filesystem::exists(mapFileOf(dataRoot, newId), error)) {
            return "a map \"" + std::string{newId} + "\" already exists";
        }
    }
    return std::nullopt;
}

// Réécrit, dans les cartes du projet, ce qui cite @p oldId (référence de carte, d'entité, base de
// variante) vers @p newId, et range les citations et les cartes touchées.
void renameMapInMaps(Project& project, std::string_view oldId, std::string_view newId,
                     RefactorPlan& plan, std::set<std::string>& changed) {
    const std::string entityPrefix = std::string{oldId} + "#";
    for (ProjectMap& map : project.maps) {
        forEachReference(map.data.entities, core::EntityChoiceSource::Maps,
                         [&](core::MapEntity& entity, std::string_view key, std::string& value) {
                             if (value == oldId) {
                                 plan.changes.push_back(mapCitation(map, entity, key));
                                 value = std::string{newId};
                                 changed.insert(map.id);
                             }
                         });
        forEachReference(map.data.entities, core::EntityChoiceSource::EntityRefs,
                         [&](core::MapEntity& entity, std::string_view key, std::string& value) {
                             if (value.starts_with(entityPrefix)) {
                                 plan.changes.push_back(mapCitation(map, entity, key));
                                 value = std::string{newId} + value.substr(oldId.size());
                                 changed.insert(map.id);
                             }
                         });
        if (map.data.base == oldId) {
            plan.changes.push_back(
                Citation{.file = map.file, .mapId = map.id, .what = "variant base"});
            map.data.base = std::string{newId};
            changed.insert(map.id);
        }
    }
}

// Idem pour les plans de ville (`start.map`, quartiers…).
void renameMapInCities(const std::filesystem::path& dataRoot, std::string_view oldId,
                       std::string_view newId, RefactorPlan& plan) {
    for (CityFile& city : loadCities(dataRoot)) {
        bool cityChanged = false;
        forEachCityMap(city.json, [&](nlohmann::ordered_json& field, std::string what) {
            if (field.get<std::string>() == oldId) {
                plan.changes.push_back(Citation{.file = city.file, .what = std::move(what)});
                field = std::string{newId};
                cityChanged = true;
            }
        });
        if (cityChanged) {
            plan.edits.push_back(ProjectEdit{.file = city.file, .text = jsonText(city.json)});
        }
    }
}

// La clé du nom : la carte la cite si son nom est la clé de son identifiant (LOT-EDITOR-07).
// @return Le refus, si un catalogue a déjà la nouvelle clé ; std::nullopt sinon.
std::optional<std::string> renameMapNameKey(const std::filesystem::path& dataRoot,
                                            std::string_view oldId, std::string_view newId,
                                            RefactorPlan& plan) {
    const bool renaming = !newId.empty();
    const std::string oldKey = mapNameKey(oldId);
    const std::string newKey = mapNameKey(newId);
    for (const std::filesystem::path& catalog : catalogFiles(dataRoot)) {
        const std::string text = readText(catalog);
        if (!renaming) {
            if (hasCatalogKey(text, oldKey)) {
                plan.changes.push_back(Citation{.file = catalog, .what = oldKey});
            }
            continue;
        }
        const std::optional<std::string> renamedText = renameCatalogKey(text, oldKey, newKey);
        if (!renamedText) {
            continue;
        }
        if (hasCatalogKey(text, newKey)) {
            return catalog.filename().string() + " already has \"" + newKey + "\"";
        }
        plan.changes.push_back(Citation{.file = catalog, .what = oldKey});
        plan.edits.push_back(ProjectEdit{.file = catalog, .text = *renamedText});
    }
    return std::nullopt;
}

}  // namespace

// Un nom vide ne renomme pas : il ne fait que recenser ce qui cite (« qui cite ceci ? »).
RefactorPlan planRenameMap(const std::filesystem::path& dataRoot, std::string_view oldId,
                           std::string_view newId) {
    const bool renaming = !newId.empty();
    if (renaming && !isValidMapId(newId)) {
        return refused("\"" + std::string{newId} + "\" is not a valid map id");
    }
    if (oldId == newId) {
        return refused("the map is already named \"" + std::string{newId} + "\"");
    }
    Project project = loadProject(dataRoot);
    if (const std::optional<std::string> refusal =
            mapRenameRefusal(project, dataRoot, oldId, newId)) {
        return refused(*refusal);
    }
    ProjectMap* renamed = project.find(oldId);
    RefactorPlan plan;
    std::set<std::string> changed;
    renameMapInMaps(project, oldId, newId, plan, changed);
    renameMapInCities(dataRoot, oldId, newId, plan);
    if (const std::optional<std::string> refusal = renameMapNameKey(dataRoot, oldId, newId, plan)) {
        return refused(*refusal);
    }

    if (!renaming) {
        return plan;
    }
    const std::string oldKey = mapNameKey(oldId);
    const std::string newKey = mapNameKey(newId);
    if (renamed->data.name == oldKey) {
        renamed->data.name = newKey;
    }
    changed.erase(renamed->id);
    writeMaps(plan, project, changed);
    // La carte change de chemin, son annexe avec elle.
    const std::filesystem::path target = mapFileOf(dataRoot, newId);
    plan.edits.push_back(
        ProjectEdit{.file = target, .text = core::LevelWriter::buildJson(renamed->data)});
    plan.edits.push_back(ProjectEdit{.file = renamed->file, .text = std::nullopt});
    std::error_code error;
    if (std::filesystem::exists(sidecarPath(renamed->file), error)) {
        plan.edits.push_back(
            ProjectEdit{.file = sidecarPath(target), .text = readText(sidecarPath(renamed->file))});
        plan.edits.push_back(ProjectEdit{.file = sidecarPath(renamed->file), .text = std::nullopt});
    }
    return plan;
}

RefactorPlan planRenameArrival(const std::filesystem::path& dataRoot, std::string_view mapId,
                               std::string_view oldName, std::string_view newName) {
    const bool renaming = !newName.empty();
    if (oldName == newName) {
        return refused("the arrival point is already named \"" + std::string{newName} + "\"");
    }
    Project project = loadProject(dataRoot);
    if (!project.error.empty()) {
        return refused(project.error);
    }
    ProjectMap* owner = project.find(mapId);
    if (owner == nullptr) {
        return refused("no map \"" + std::string{mapId} + "\"");
    }
    const auto spawn = std::ranges::find_if(owner->data.entities, [oldName](const auto& entity) {
        return isSpawnPoint(entity, oldName);
    });
    if (spawn == owner->data.entities.end()) {
        return refused(std::string{mapId} + " has no arrival point \"" + std::string{oldName} +
                       "\"");
    }
    if (renaming && std::ranges::any_of(owner->data.entities, [newName](const auto& entity) {
            return isSpawnPoint(entity, newName);
        })) {
        return refused(std::string{mapId} + " already has an arrival point \"" +
                       std::string{newName} + "\"");
    }
    RefactorPlan plan;
    std::set<std::string> changed;
    for (ProjectMap& map : project.maps) {
        forEachReference(map.data.entities, core::EntityChoiceSource::ArrivalPoints,
                         [&](core::MapEntity& entity, std::string_view key, std::string& value) {
                             if (value == oldName && targetMapOf(entity) == mapId) {
                                 plan.changes.push_back(mapCitation(map, entity, key));
                                 value = std::string{newName};
                                 changed.insert(map.id);
                             }
                         });
    }
    for (CityFile& city : loadCities(dataRoot)) {
        if (cityStartMap(city.json) != mapId ||
            city.json["start"].value("arrival", std::string{}) != oldName) {
            continue;
        }
        plan.changes.push_back(Citation{.file = city.file, .what = "start: arrival"});
        city.json["start"]["arrival"] = std::string{newName};
        plan.edits.push_back(ProjectEdit{.file = city.file, .text = jsonText(city.json)});
    }
    if (!renaming) {
        return plan;
    }
    for (core::MapEntity& entity : owner->data.entities) {
        if (isSpawnPoint(entity, oldName)) {
            entity.properties[std::string{core::SPAWN_POINT_NAME_PROPERTY}] = std::string{newName};
        }
    }
    changed.insert(std::string{mapId});
    writeMaps(plan, project, changed);
    return plan;
}

RefactorPlan planRenameEntityId(const std::filesystem::path& dataRoot, std::string_view mapId,
                                std::string_view oldId, std::string_view newId) {
    const bool renaming = !newId.empty();
    if (renaming && newId.find_first_of("#/ \t") != std::string_view::npos) {
        return refused("\"" + std::string{newId} + "\" is not a valid entity id");
    }
    if (oldId == newId) {
        return refused("the entity is already named \"" + std::string{newId} + "\"");
    }
    Project project = loadProject(dataRoot);
    if (!project.error.empty()) {
        return refused(project.error);
    }
    ProjectMap* owner = project.find(mapId);
    if (owner == nullptr) {
        return refused("no map \"" + std::string{mapId} + "\"");
    }
    std::vector<core::MapEntity>& entities = owner->data.entities;
    const auto entity = std::ranges::find_if(
        entities, [oldId](const core::MapEntity& candidate) { return candidate.id == oldId; });
    if (entity == entities.end()) {
        return refused(std::string{mapId} + " has no entity \"" + std::string{oldId} + "\"");
    }
    if (renaming) {
        if (std::ranges::any_of(entities, [newId](const core::MapEntity& candidate) {
                return candidate.id == newId;
            })) {
            return refused(std::string{mapId} + " already has an entity \"" + std::string{newId} +
                           "\"");
        }
        if (const std::optional<int> number = core::entityIdNumber(newId);
            number && *number >= owner->data.nextEntityId) {
            return refused("\"" + std::string{newId} +
                           "\" could be given again by the editor (nextEntityId is " +
                           std::to_string(owner->data.nextEntityId) + ")");
        }
    }
    const std::string oldRef = entityRef(mapId, oldId);
    RefactorPlan plan;
    std::set<std::string> changed;
    for (ProjectMap& map : project.maps) {
        forEachReference(map.data.entities, core::EntityChoiceSource::EntityRefs,
                         [&](core::MapEntity& citing, std::string_view key, std::string& value) {
                             if (value == oldRef) {
                                 plan.changes.push_back(mapCitation(map, citing, key));
                                 value = entityRef(mapId, newId);
                                 changed.insert(map.id);
                             }
                         });
    }
    if (!renaming) {
        return plan;
    }
    entity->id = std::string{newId};
    changed.insert(std::string{mapId});
    writeMaps(plan, project, changed);
    return plan;
}

// --- Remplacer, changer de planche -------------------------------------------------------------

namespace {

// Ajoute à @p plan la réécriture de @p map (@p from devient @p to), ou dit pourquoi elle est
// refusée : std::nullopt si tout va bien.
std::optional<std::string> replacePieceInMap(const std::filesystem::path& dataRoot,
                                             const ProjectMap& map, std::string_view from,
                                             std::string_view to, RefactorPlan& plan) {
    const PlaceAssets assets = placeOf(dataRoot, map.data);
    const core::ScenePiece* next = assets.manifest ? assets.manifest->find(to) : nullptr;
    if (next == nullptr) {
        return "\"" + std::string{to} + "\" is not on the sheet of " + map.id;
    }
    const core::ScenePiece* previous = assets.manifest->find(from);
    if (previous != nullptr && (previous->pieceClass == core::ScenePieceClass::Floor) !=
                                   (next->pieceClass == core::ScenePieceClass::Floor)) {
        return "\"" + std::string{from} + "\" and \"" + std::string{to} +
               "\" are not both floors, or both standing pieces";
    }
    core::LevelDraft draft = draftOf(map.data, assets);
    const core::PieceRenaming renaming{{std::string{from}, std::string{to}}};
    if (!draft.replacePieces(renaming)) {
        return "\"" + std::string{to} + "\" would overflow " + map.id;
    }
    const core::LevelLoadResult validated = draft.toLevel();
    if (!validated.ok()) {
        return map.id + " would not be valid: " + validated.error;
    }
    std::vector<Citation> cited = pieceCitationsIn(map, from);
    plan.changes.insert(plan.changes.end(), std::make_move_iterator(cited.begin()),
                        std::make_move_iterator(cited.end()));
    plan.edits.push_back(
        ProjectEdit{.file = map.file, .text = core::LevelWriter::toJsonString(*validated.level)});
    return std::nullopt;
}

}  // namespace

RefactorPlan planReplacePiece(const std::filesystem::path& dataRoot, std::string_view from,
                              std::string_view to, const std::vector<std::string>& maps) {
    if (from.empty() || to.empty()) {
        return refused("name the piece to replace and the one that replaces it");
    }
    Project project = loadProject(dataRoot);
    if (!project.error.empty()) {
        return refused(project.error);
    }
    for (const std::string& id : maps) {
        if (project.find(id) == nullptr) {
            return refused("no map \"" + id + "\"");
        }
    }
    RefactorPlan plan;
    for (const ProjectMap& map : project.maps) {
        if (!maps.empty() && std::ranges::find(maps, map.id) == maps.end()) {
            continue;
        }
        if (!citedPieces(map.data.layers).contains(from)) {
            continue;
        }
        if (const std::optional<std::string> refusal =
                replacePieceInMap(dataRoot, map, from, to, plan)) {
            return refused(*refusal);
        }
    }
    if (plan.edits.empty()) {
        return refused("no map places \"" + std::string{from} + "\"");
    }
    return plan;
}

core::PieceRenaming proposedPieceTable(const std::vector<core::TileLayer>& layers,
                                       const core::ScenePieceManifest& target) {
    core::PieceRenaming table;
    for (const std::string& piece : citedPieces(layers)) {
        if (const core::ScenePiece* found = target.find(piece)) {
            table.emplace(piece, found->name);
        }
    }
    return table;
}

std::vector<std::string> piecesMissingFrom(const std::vector<core::TileLayer>& layers,
                                           const core::PieceRenaming& table,
                                           const core::ScenePieceManifest& target) {
    std::vector<std::string> missing;
    for (const std::string& piece : citedPieces(layers)) {
        const auto renamed = table.find(piece);
        const std::string_view name = renamed != table.end() ? renamed->second : piece;
        if (target.find(name) == nullptr) {
            missing.push_back(piece);
        }
    }
    return missing;
}

namespace {

// Le refus quand la planche @p place n'a pas de pièce pour @p missing.
std::string missingPiecesRefusal(std::string_view place, const std::vector<std::string>& missing,
                                 bool variant) {
    std::string list;
    for (const std::string& piece : missing) {
        list += (list.empty() ? "" : ", ") + piece;
    }
    return "no match on the sheet \"" + std::string{place} + "\" for: " + list +
           (variant ? " (pieces of the base map)" : " (add them to the table)");
}

}  // namespace

RefactorPlan planChangeScene(const std::filesystem::path& dataRoot, std::string_view mapId,
                             std::string_view place, const core::PieceRenaming& table) {
    Project project = loadProject(dataRoot);
    if (!project.error.empty()) {
        return refused(project.error);
    }
    ProjectMap* map = project.find(mapId);
    if (map == nullptr) {
        return refused("no map \"" + std::string{mapId} + "\"");
    }
    const PlaceAssets target = loadPlaceAssets(dataRoot, place);
    if (!target.manifest) {
        return refused("no sheet \"" + std::string{place} + "\" (Assets/Scene/" +
                       std::string{place} + "/manifest.json)");
    }
    core::PieceRenaming merged = proposedPieceTable(map->data.layers, *target.manifest);
    for (const auto& [from, to] : table) {
        merged.insert_or_assign(from, to);
    }
    // Une variante ne porte pas de cases : ce sont celles de sa base qu'elle montrera.
    const bool variant = !map->data.base.empty();
    const std::vector<std::string> missing = piecesMissingFrom(
        map->data.layers, variant ? core::PieceRenaming{} : merged, *target.manifest);
    if (!missing.empty()) {
        return refused(missingPiecesRefusal(place, missing, variant));
    }
    RefactorPlan plan;
    std::string text;
    if (variant) {
        if (map->data.scene == place) {
            return refused(std::string{mapId} + " already shows \"" + std::string{place} + "\"");
        }
        map->data.scene = std::string{place};
        text = core::LevelWriter::buildJson(map->data);
    } else {
        core::LevelDraft draft = draftOf(map->data, placeOf(dataRoot, map->data));
        if (!draft.changeScene(std::string{place},
                               std::make_shared<const core::ScenePieceManifest>(*target.manifest),
                               merged)) {
            return refused(std::string{mapId} + " already shows \"" + std::string{place} +
                           "\", or a piece would overflow it");
        }
        const core::LevelLoadResult validated = draft.toLevel();
        if (!validated.ok()) {
            return refused(std::string{mapId} + " would not be valid: " + validated.error);
        }
        text = core::LevelWriter::toJsonString(*validated.level);
        for (const auto& [from, to] : merged) {
            if (from != to) {
                plan.changes.push_back(
                    Citation{.file = map->file,
                             .mapId = map->id,
                             .what = std::string{"piece "}.append(from).append(" -> ").append(to)});
            }
        }
    }
    const std::string previousPlace = scenePlaceOf(map->data.layers);
    plan.changes.insert(plan.changes.begin(),
                        Citation{.file = map->file,
                                 .mapId = map->id,
                                 .what = "sheet " + previousPlace + " -> " + std::string{place}});
    plan.edits.push_back(ProjectEdit{.file = map->file, .text = std::move(text)});
    // La table d'apparence suit : le rendu des îlots et les cases sans pièce la lisent.
    if (!variant) {
        if (std::optional<std::string> appearance =
                translatedAppearance(dataRoot, previousPlace, place, merged)) {
            const std::filesystem::path file = appearanceFile(dataRoot, place);
            plan.changes.push_back(Citation{
                .file = file, .what = "appearance table made from " + previousPlace + "'s"});
            plan.edits.push_back(ProjectEdit{.file = file, .text = std::move(*appearance)});
        }
    }
    return plan;
}

PieceTableResult readPieceTable(const std::filesystem::path& file) {
    PieceTableResult result;
    const nlohmann::json json =
        nlohmann::json::parse(readText(file), nullptr, /*allow_exceptions=*/false);
    if (!json.is_object() || json.value("format", std::string{}) != PIECE_TABLE_FORMAT ||
        json.value("version", 0) != 1 || !json.contains("pieces") || !json["pieces"].is_object()) {
        result.error = file.string();
        result.error += R"(: not a piece table (format ")";
        result.error += PIECE_TABLE_FORMAT;
        result.error += R"(", version 1, "pieces"))";
        return result;
    }
    for (const auto& [from, to] : json["pieces"].items()) {
        if (!to.is_string() || to.get<std::string>().empty()) {
            result.error = file.string() + ": piece \"" + from + "\" has no replacement";
            return result;
        }
        result.table.emplace(from, to.get<std::string>());
    }
    return result;
}

// --- Sans fenêtre --------------------------------------------------------------------------------

namespace {

// Les arguments qui suivent @p option, jusqu'à la prochaine option.
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

[[nodiscard]] int listCitations(const std::vector<Citation>& citations,
                                const std::filesystem::path& dataRoot, std::string& output) {
    for (const Citation& citation : citations) {
        output += formatCitation(citation, dataRoot) + "\n";
    }
    output +=
        std::to_string(citations.size()) + (citations.size() == 1 ? " citation\n" : " citations\n");
    return 0;
}

[[nodiscard]] int whoCites(const std::vector<std::string>& values,
                           const std::filesystem::path& dataRoot, std::string& output) {
    const std::string what = values.empty() ? std::string{} : values.front();
    if (what == "map" && values.size() == 2) {
        return listCitations(citationsOfMap(dataRoot, values[1]), dataRoot, output);
    }
    if (what == "arrival" && values.size() == 3) {
        return listCitations(citationsOfArrival(dataRoot, values[1], values[2]), dataRoot, output);
    }
    if (what == "entity" && values.size() == 3) {
        return listCitations(citationsOfEntity(dataRoot, values[1], values[2]), dataRoot, output);
    }
    if (what == "piece" && values.size() == 2) {
        return listCitations(citationsOfPiece(dataRoot, values[1]), dataRoot, output);
    }
    output +=
        "usage: --who-cites map <map> | arrival <map> <point> | entity <map> <id> | "
        "piece <piece>\n";
    return 2;
}

[[nodiscard]] int carryOut(const RefactorPlan& plan, const std::filesystem::path& dataRoot,
                           std::string& output) {
    if (!plan.ok()) {
        output += "error: " + plan.error + "\nnothing written\n";
        return 1;
    }
    for (const Citation& change : plan.changes) {
        output += formatCitation(change, dataRoot) + "\n";
    }
    std::string error;
    if (!applyRefactorPlan(plan, error)) {
        output += "error: " + error + "\n";
        return 1;
    }
    output += std::to_string(plan.edits.size()) + " files written or removed\n";
    return 0;
}

}  // namespace

std::optional<int> runRefactorCommand(const std::vector<std::string>& arguments,
                                      const std::filesystem::path& dataRoot, std::string& output) {
    const auto usage = [&output](std::string_view text) {
        output += "usage: " + std::string{text} + "\n";
        return 2;
    };
    if (const auto values = valuesOf(arguments, "--who-cites")) {
        return whoCites(*values, dataRoot, output);
    }
    if (const auto map = valuesOf(arguments, "--rename-map")) {
        if (map->size() != 2) {
            return usage("--rename-map <old> <new>");
        }
        return carryOut(planRenameMap(dataRoot, (*map)[0], (*map)[1]), dataRoot, output);
    }
    if (const auto arrival = valuesOf(arguments, "--rename-arrival")) {
        if (arrival->size() != 3) {
            return usage("--rename-arrival <map> <old> <new>");
        }
        return carryOut(planRenameArrival(dataRoot, (*arrival)[0], (*arrival)[1], (*arrival)[2]),
                        dataRoot, output);
    }
    if (const auto id = valuesOf(arguments, "--rename-id")) {
        if (id->size() != 3) {
            return usage("--rename-id <map> <old> <new>");
        }
        return carryOut(planRenameEntityId(dataRoot, (*id)[0], (*id)[1], (*id)[2]), dataRoot,
                        output);
    }
    if (const auto piece = valuesOf(arguments, "--replace-piece")) {
        if (piece->size() < 2) {
            return usage("--replace-piece <old> <new> [map...]");
        }
        const std::vector<std::string> maps(std::next(piece->begin(), 2), piece->end());
        return carryOut(planReplacePiece(dataRoot, (*piece)[0], (*piece)[1], maps), dataRoot,
                        output);
    }
    if (const auto scene = valuesOf(arguments, "--change-scene")) {
        const auto tableFile = valuesOf(arguments, "--table");
        if (scene->size() != 2 || (tableFile && tableFile->size() != 1)) {
            return usage("--change-scene <map> <place> [--table <table.json>]");
        }
        PieceTableResult table;
        if (tableFile) {
            table = readPieceTable(tableFile->front());
        }
        if (!table.ok()) {
            output += "error: " + table.error + "\n";
            return 1;
        }
        return carryOut(planChangeScene(dataRoot, (*scene)[0], (*scene)[1], table.table), dataRoot,
                        output);
    }
    return std::nullopt;
}

}  // namespace hmi
