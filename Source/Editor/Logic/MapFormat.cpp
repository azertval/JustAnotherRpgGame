// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/MapFormat.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <system_error>
#include <utility>

#include "Core/Levels/CollisionDerivation.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelWriter.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/PieceFootprint.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileTypeName.h"
#include "Core/Resources/ScenePlace.h"
#include "Core/World/WorldGraph.h"
#include "Core/World/WorldTravel.h"
#include "Editor/Logic/ContentCheck.h"
#include "Editor/Logic/EditorSidecar.h"
#include "Editor/Logic/GestureScript.h"
#include "Editor/Logic/Stamps.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace hmi {

namespace {

/// Au-delà, les cases fautives d'une même sorte se résument : trois cents lignes n'apprennent rien
/// de plus que vingt et un compte.
constexpr std::size_t CELLS_LISTED_PER_KIND = 20;

[[nodiscard]] std::string readText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

[[nodiscard]] bool hasVisualLayer(const std::vector<core::TileLayer>& layers) {
    return std::ranges::any_of(
        layers, [](const core::TileLayer& layer) { return core::isVisualLayerKind(layer.kind); });
}

[[nodiscard]] std::string cellText(core::GridPosition cell) {
    return "(" + std::to_string(cell.column) + ", " + std::to_string(cell.row) + ")";
}

// Accumule les constats d'une carte, en résumant les longues séries de cases.
class Findings {
public:
    explicit Findings(std::string_view mapId) : _mapId(mapId) {}

    void add(MapCheckSeverity severity, std::string message,
             std::optional<core::GridPosition> cell = std::nullopt) {
        _findings.push_back(MapCheckFinding{
            .severity = severity, .mapId = _mapId, .cell = cell, .message = std::move(message)});
    }

    // Un constat par case, les CELLS_LISTED_PER_KIND premières, puis un résumé.
    void addCells(MapCheckSeverity severity, const std::vector<core::GridPosition>& cells,
                  const std::string& message) {
        for (std::size_t index = 0; index < cells.size() && index < CELLS_LISTED_PER_KIND;
             ++index) {
            add(severity, message, cells[index]);
        }
        if (cells.size() > CELLS_LISTED_PER_KIND) {
            add(severity, message + " (" + std::to_string(cells.size()) + " cells in all)");
        }
    }

    [[nodiscard]] std::vector<MapCheckFinding> take() {
        return std::move(_findings);
    }

private:
    std::string _mapId;
    std::vector<MapCheckFinding> _findings;
};

[[nodiscard]] const char* worldIssueText(core::WorldIssueCode code) {
    switch (code) {
        case core::WorldIssueCode::UnreadableMap:
            return "the map cannot be read";
        case core::WorldIssueCode::MissingTargetMap:
            return "a portal names no target map";
        case core::WorldIssueCode::UnknownTargetMap:
            return "a portal leads to a map that does not exist";
        case core::WorldIssueCode::UnreadableTargetMap:
            return "a portal leads to a map that cannot be read";
        case core::WorldIssueCode::MissingArrivalPoint:
            return "a portal names no arrival point";
        case core::WorldIssueCode::UnknownArrivalPoint:
            return "the target map has no such arrival point";
        case core::WorldIssueCode::DuplicateArrivalPoint:
            return "two arrival points share a name";
        case core::WorldIssueCode::CombatZoneDegenerate:
            return "a combat zone has no width or no height";
        case core::WorldIssueCode::CombatZoneOutOfBounds:
            return "a combat zone overflows the map";
        case core::WorldIssueCode::CombatZoneBlocked:
            return "a combat zone has no free cell";
    }
    return "world issue";
}

// --- Pièces ----------------------------------------------------------------------------------

// Ce que le parcours des pièces d'une carte relève.
struct PieceScan {
    std::vector<core::GridPosition> missing;
    std::map<std::string, std::vector<core::GridPosition>> aliased;
    std::vector<core::GridPosition> overlapping;
};

// Range la pièce nommée en @p cell : absente du manifeste, citée par un alias, et son emprise dans
// @p occupants (qui occupe chaque case de la couche).
void scanPieceCell(const core::TileLayer& layer, const PlaceAssets& assets,
                   const core::GridPosition cell, const std::string_view name,
                   std::vector<int>& occupants, PieceScan& scan) {
    const core::ScenePiece* piece = assets.manifest ? assets.manifest->find(name) : nullptr;
    if (piece == nullptr) {
        scan.missing.push_back(cell);
        return;
    }
    if (piece->name != name) {
        scan.aliased[std::string{name}].push_back(cell);
    }
    for (const core::GridPosition covered : core::footprintCells(cell, piece->footprint())) {
        if (!layer.tiles.inBounds(covered.column, covered.row)) {
            continue;
        }
        const std::size_t index = (static_cast<std::size_t>(covered.row) *
                                   static_cast<std::size_t>(layer.tiles.width())) +
                                  static_cast<std::size_t>(covered.column);
        if (++occupants[index] == 2) {
            scan.overlapping.push_back(covered);
        }
    }
}

// Les cases dont le LIEU ne dit rien : celles qui ne nomment aucune piece et dont la table ne
// couvre pas le type (LOT-128). Elles ne sont plus invisibles -- elles prennent le rendu de
// maquette --, mais sur une carte qu'on veut habillee, c'est un trou d'habillage, pas un choix.
void checkUncoveredTypes(const core::Level& level, const PlaceAssets& assets,
                         const std::string& place, Findings& findings) {
    if (place.empty() || !assets.appearance) {
        return;  // une carte sans lieu EST une maquette : il n'y a rien a signaler.
    }
    std::map<core::TileType, std::vector<core::GridPosition>> uncovered;
    for (const core::TileLayer& layer : level.layers()) {
        if (!core::isVisualLayerKind(layer.kind)) {
            continue;
        }
        const bool floor = layer.kind == core::LayerKind::Ground;
        for (int row = 0; row < layer.tiles.height(); ++row) {
            for (int column = 0; column < layer.tiles.width(); ++column) {
                const core::GridPosition cell{.column = column, .row = row};
                const core::TileType type = layer.tiles.tile(column, row);
                if (type == core::TileType::Empty || !layer.pieceAt(column, row).empty()) {
                    continue;
                }
                const std::string_view piece = floor ? assets.appearance->floorPiece(type, cell)
                                                     : assets.appearance->reliefPiece(type, cell);
                if (piece.empty() && floor) {
                    uncovered[type].push_back(cell);
                }
            }
        }
    }
    for (const auto& [type, cells] : uncovered) {
        findings.addCells(MapCheckSeverity::Warning, cells,
                          "tile type \"" + core::tileTypeName(type) +
                              "\" is not covered by the appearance tables of \"" + place +
                              "\" (shown as a mock-up)");
    }
}

void checkPieces(const core::Level& level, const PlaceAssets& assets, const std::string& place,
                 Findings& findings) {
    PieceScan scan;
    for (const core::TileLayer& layer : level.layers()) {
        if (!core::isVisualLayerKind(layer.kind) || !layer.hasPieces()) {
            continue;
        }
        // Qui occupe chaque case : une pièce large en couvre plusieurs, et deux emprises qui se
        // recouvrent dessinent deux pièces au même endroit.
        std::vector<int> occupants(layer.pieces.size(), 0);
        for (int row = 0; row < layer.tiles.height(); ++row) {
            for (int column = 0; column < layer.tiles.width(); ++column) {
                const std::string_view name = layer.pieceAt(column, row);
                if (!name.empty()) {
                    scanPieceCell(layer, assets, {.column = column, .row = row}, name, occupants,
                                  scan);
                }
            }
        }
    }
    const auto& [missing, aliased, overlapping] = scan;
    const std::string sheet = place.empty()
                                  ? std::string{"(no scene declared)"}
                                  : "the pieces of \"" + place + "\" and its common levels";
    findings.addCells(MapCheckSeverity::Error, missing,
                      "piece missing from " + sheet + " (shown as a checkerboard)");
    for (const auto& [name, cells] : aliased) {
        findings.addCells(MapCheckSeverity::Warning, cells,
                          "piece \"" + name + "\" is cited by an old name (alias of \"" +
                              std::string{assets.manifest->find(name)->name} + "\")");
    }
    findings.addCells(MapCheckSeverity::Warning, overlapping,
                      "two piece footprints cover the same cell");
    checkUncoveredTypes(level, assets, place, findings);
}

// --- Collision -------------------------------------------------------------------------------

void checkCollision(const core::Level& level, const PlaceAssets& assets, Findings& findings) {
    if (!hasVisualLayer(level.layers())) {
        return;  // une grille unique (v2) vaut image et collision : rien à déduire.
    }
    const core::TileMap& written = level.tileMap();
    const core::CollisionDerivation derived =
        core::deriveCollision(level.layers(), written.width(), written.height(),
                              assets.manifest ? &*assets.manifest : nullptr);
    const std::set<std::pair<int, int>> forced = [&level] {
        std::set<std::pair<int, int>> cells;
        for (const core::GridPosition cell : level.forcedCollision()) {
            cells.emplace(cell.column, cell.row);
        }
        return cells;
    }();

    std::vector<core::GridPosition> differing;
    std::vector<core::GridPosition> uselessForced;
    for (int row = 0; row < written.height(); ++row) {
        for (int column = 0; column < written.width(); ++column) {
            const core::GridPosition cell{.column = column, .row = row};
            const bool agrees = core::collisionAgrees(written, derived.collision, column, row);
            if (forced.contains({column, row})) {
                if (agrees) {
                    uselessForced.push_back(cell);
                }
            } else if (!agrees) {
                differing.push_back(cell);
            }
        }
    }
    if (written.tile(level.entry().column, level.entry().row) == core::TileType::Entry &&
        derived.collision.tile(level.entry().column, level.entry().row) != core::TileType::Empty &&
        !forced.contains({level.entry().column, level.entry().row})) {
        findings.add(MapCheckSeverity::Error, "the entry stands on a blocked cell", level.entry());
    }
    findings.addCells(MapCheckSeverity::Error, differing,
                      "collision differs from what the pieces give, and the cell is not forced");
    findings.addCells(MapCheckSeverity::Warning, uselessForced,
                      "forced cell agrees with the derivation: it no longer needs forcing");
    findings.addCells(MapCheckSeverity::Warning, derived.unplayed,
                      "difficult or cover piece or tile: not played yet, derived as open");
}

// --- Entités, réserve de hauteur -------------------------------------------------------------

void checkEntities(const core::Level& level, Findings& findings) {
    for (const core::MapEntity& entity : level.entities()) {
        if (entity.id.empty()) {
            findings.add(MapCheckSeverity::Error,
                         "entity \"" + entity.type + "\" has no id (run --migrate)",
                         entity.position);
            continue;
        }
        const std::optional<int> number = core::entityIdNumber(entity.id);
        if (number && *number >= level.nextEntityId()) {
            findings.add(MapCheckSeverity::Error,
                         "entity id \"" + entity.id + "\" is not below nextEntityId (" +
                             std::to_string(level.nextEntityId()) + "): it could be given again",
                         entity.position);
        }
        if (entity.elevation != 0) {
            findings.add(MapCheckSeverity::Warning,
                         "entity \"" + entity.id + "\" has an elevation: reserved, not played yet",
                         entity.position);
        }
    }
}

void checkHeightReserve(const core::Level& level, Findings& findings) {
    for (const core::TileLayer& layer : level.layers()) {
        // Un étage se joue sur une couche de décor, de 1 à MAX_STOREY_FLOOR (LOT-129) ; ailleurs,
        // il est gardé mais ignoré.
        const bool playedFloor = layer.kind == core::LayerKind::Decor && layer.floor >= 0 &&
                                 layer.floor <= core::MAX_STOREY_FLOOR;
        if (layer.floor != 0 && !playedFloor) {
            findings.add(MapCheckSeverity::Warning,
                         "layer \"" + layer.name + "\" is on floor " + std::to_string(layer.floor) +
                             ": only a decor layer rises, from floor 1 to " +
                             std::to_string(core::MAX_STOREY_FLOOR) + "; ignored");
        }
        std::vector<core::GridPosition> raised;
        for (int row = 0; row < layer.tiles.height(); ++row) {
            for (int column = 0; column < layer.tiles.width(); ++column) {
                if (layer.elevationAt(column, row) != 0) {
                    raised.push_back({.column = column, .row = row});
                }
            }
        }
        findings.addCells(
            MapCheckSeverity::Warning, raised,
            "cell of layer \"" + layer.name + "\" has an elevation: reserved, not played yet");
    }
}

// --- Migration -------------------------------------------------------------------------------

// Étape 2 : chaque case de couche d'un type non vide nomme la pièce que la table lui donnait.
std::size_t namePieces(std::vector<core::TileLayer>& layers, const PlaceAppearance& appearance) {
    std::size_t named = 0;
    for (core::TileLayer& layer : layers) {
        if (!core::isVisualLayerKind(layer.kind)) {
            continue;
        }
        const bool floor = layer.kind == core::LayerKind::Ground;
        for (int row = 0; row < layer.tiles.height(); ++row) {
            for (int column = 0; column < layer.tiles.width(); ++column) {
                const core::TileType type = layer.tiles.tile(column, row);
                if (type == core::TileType::Empty || !layer.pieceAt(column, row).empty()) {
                    continue;
                }
                const core::GridPosition cell{.column = column, .row = row};
                const std::string_view piece =
                    floor ? appearance.floorPiece(type, cell) : appearance.reliefPiece(type, cell);
                if (!piece.empty()) {
                    layer.setPiece(column, row, std::string{piece});
                    ++named;
                }
            }
        }
    }
    return named;
}

// Étape 3 : un identifiant pour chaque entité qui n'en a pas.
std::size_t giveIds(core::LevelData& data) {
    int next = data.nextEntityId;
    for (const core::MapEntity& entity : data.entities) {
        if (const std::optional<int> number = core::entityIdNumber(entity.id)) {
            next = std::max(next, *number + 1);
        }
    }
    std::size_t given = 0;
    for (core::MapEntity& entity : data.entities) {
        if (entity.id.empty()) {
            entity.id = core::entityIdFor(next++);
            ++given;
        }
    }
    data.nextEntityId = next;
    return given;
}

// Étape 4 : la grille écrite prend la déduction partout où elle s'y accorde ; ailleurs, elle garde
// sa valeur et la case devient forcée.
std::size_t alignCollision(core::LevelData& data, const PlaceAssets& assets) {
    core::TileMap& written = data.tileMap;
    const core::CollisionDerivation derived =
        core::deriveCollision(data.layers, written.width(), written.height(),
                              assets.manifest ? &*assets.manifest : nullptr);
    std::set<std::pair<int, int>> forced;
    for (const core::GridPosition cell : data.forcedCollision) {
        forced.emplace(cell.row, cell.column);
    }
    std::size_t added = 0;
    for (int row = 0; row < written.height(); ++row) {
        for (int column = 0; column < written.width(); ++column) {
            if (forced.contains({row, column})) {
                continue;
            }
            if (!core::collisionAgrees(written, derived.collision, column, row)) {
                forced.emplace(row, column);
                ++added;
            } else if (written.tile(column, row) != core::TileType::Entry) {
                written.setTile(column, row, derived.collision.tile(column, row));
            }
        }
    }
    data.forcedCollision.clear();
    for (const auto& [row, column] : forced) {
        data.forcedCollision.push_back({.column = column, .row = row});
    }
    // L'entrée promue en tête des couches reflète la grille racine : elle la suit.
    for (core::TileLayer& layer : data.layers) {
        if (layer.kind == core::LayerKind::Collision || layer.kind == core::LayerKind::Legacy) {
            layer.tiles = written;
        }
    }
    return added;
}

[[nodiscard]] std::filesystem::path levelsOf(const std::filesystem::path& dataRoot) {
    return dataRoot / "Levels";
}

// Les fichiers de carte d'un dossier de niveaux, triés : pas les séquences, pas les annexes de
// l'éditeur.
[[nodiscard]] std::vector<std::filesystem::path> listMapFiles(const std::filesystem::path& levels) {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    for (auto entry = std::filesystem::recursive_directory_iterator(levels, error);
         !error && entry != std::filesystem::recursive_directory_iterator();
         entry.increment(error)) {
        const std::filesystem::path& path = entry->path();
        const std::string name = path.filename().string();
        if (!entry->is_regular_file() || path.extension() != ".json" ||
            name.starts_with("sequence-") || name.ends_with(".editor.json")) {
            continue;
        }
        files.push_back(path);
    }
    std::ranges::sort(files);
    return files;
}

}  // namespace

std::vector<std::filesystem::path> mapFiles(const std::filesystem::path& dataRoot) {
    return listMapFiles(levelsOf(dataRoot));
}

PlaceAssets loadPlaceAssets(const std::filesystem::path& dataRoot, std::string_view place) {
    PlaceAssets assets;
    if (place.empty()) {
        // Une maquette n'a ni pièce ni table ; ses PNJ prennent les figurines du monde (LOT-124).
        PlaceAppearanceResult world = PlaceAppearance::loadForPlace(dataRoot / "Assets", {});
        if (world.ok()) {
            assets.appearance = std::move(world.appearance);
        }
        return assets;
    }
    // Le catalogue RESOLU : le lieu et ses niveaux communs, du plus propre au monde (LOT-124).
    const std::filesystem::path directory = dataRoot / "Assets";
    core::ScenePieceManifestResult manifest = core::ScenePieceManifest::resolve(directory, place);
    if (manifest.ok()) {
        assets.manifest = std::move(manifest.manifest);
    } else {
        assets.manifestError = std::move(manifest.message);
    }
    PlaceAppearanceResult appearance = PlaceAppearance::loadForPlace(directory, place);
    if (appearance.ok()) {
        assets.appearance = std::move(appearance.appearance);
    }
    return assets;
}

std::vector<std::string> scenePlaces(const std::filesystem::path& dataRoot) {
    return core::scenePlaces(dataRoot / "Assets");
}

std::string formatFinding(const MapCheckFinding& finding) {
    std::string line = finding.mapId;
    if (finding.cell) {
        line += " " + cellText(*finding.cell);
    }
    line += finding.severity == MapCheckSeverity::Error ? ": error: " : ": warning: ";
    line += finding.message;
    return line;
}

std::vector<MapCheckFinding> checkMapFile(std::string_view mapId, const std::filesystem::path& file,
                                          const std::filesystem::path& dataRoot) {
    return checkMapFile(mapId, file, dataRoot, loadContentContext(dataRoot));
}

std::vector<MapCheckFinding> checkMapFile(std::string_view mapId, const std::filesystem::path& file,
                                          const std::filesystem::path& dataRoot,
                                          const ContentContext& context) {
    Findings findings(mapId);
    const std::string text = readText(file);
    const core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(file);
    if (!loaded.ok()) {
        findings.add(MapCheckSeverity::Error, "cannot be read: " + loaded.error);
        return findings.take();
    }
    const core::Level& level = *loaded.level;

    // Une carte d'une version passée se lit, mais le dépôt n'en garde pas : elle se migre.
    if (text.find("\"version\": " + std::to_string(core::LEVEL_FORMAT_VERSION)) ==
        std::string::npos) {
        findings.add(MapCheckSeverity::Error, "not in format version " +
                                                  std::to_string(core::LEVEL_FORMAT_VERSION) +
                                                  " (run LevelEditor --migrate)");
    } else if (core::LevelWriter::toJsonString(level) != text) {
        findings.add(MapCheckSeverity::Error,
                     "not canonical: loading then saving it changes the file (run --migrate)");
    }

    checkEntities(level, findings);
    checkHeightReserve(level, findings);
    for (const core::WorldIssue& issue : core::validateWorldMap(mapId, level)) {
        findings.add(MapCheckSeverity::Error,
                     std::string{worldIssueText(issue.code)} +
                         (issue.value.empty() ? std::string{} : " (\"" + issue.value + "\")"),
                     issue.position);
    }
    // Une variante reprend les cases de sa base : elles se contrôlent sur la base.
    if (level.base().empty()) {
        const std::string place = scenePlaceOf(level.layers());
        const PlaceAssets assets = loadPlaceAssets(dataRoot, place);
        if (!place.empty() && !assets.manifest) {
            findings.add(MapCheckSeverity::Error,
                         "scene \"" + place + "\": " + assets.manifestError);
        }
        checkPieces(level, assets, place, findings);
        checkCollision(level, assets, findings);
    }
    std::vector<MapCheckFinding> found = findings.take();
    std::vector<MapCheckFinding> content = checkMapContent(mapId, level, context);
    found.insert(found.end(), std::make_move_iterator(content.begin()),
                 std::make_move_iterator(content.end()));
    return found;
}

std::size_t MapCheckReport::count(MapCheckSeverity severity) const {
    return static_cast<std::size_t>(
        std::ranges::count(findings, severity, &MapCheckFinding::severity));
}

MapCheckReport checkAllMaps(const std::filesystem::path& dataRoot) {
    MapCheckReport report;
    const std::filesystem::path levels = levelsOf(dataRoot);
    const ContentContext context = loadContentContext(dataRoot);
    for (const std::filesystem::path& file : listMapFiles(levels)) {
        ++report.maps;
        std::vector<MapCheckFinding> found =
            checkMapFile(core::mapIdOf(levels, file), file, dataRoot, context);
        report.findings.insert(report.findings.end(), found.begin(), found.end());
    }
    // Le récit, hors de toute carte (LOT-116) : un drapeau lu que rien ne pose.
    std::vector<MapCheckFinding> story = checkStoryContent(dataRoot);
    report.findings.insert(report.findings.end(), story.begin(), story.end());
    // Les portails, d'une carte à l'autre : ce qu'aucune carte seule ne voit.
    for (const core::WorldIssue& issue : core::validateWorldGraph(core::loadWorldGraph(levels))) {
        if (issue.code == core::WorldIssueCode::UnreadableMap) {
            continue;  // déjà dit, carte par carte
        }
        report.findings.push_back(MapCheckFinding{
            .severity = MapCheckSeverity::Error,
            .mapId = issue.mapId,
            .cell = issue.position,
            .message = std::string{worldIssueText(issue.code)} +
                       (issue.value.empty() ? std::string{} : " (\"" + issue.value + "\")")});
    }
    return report;
}

MapMigration migrateLevel(const core::Level& level, const PlaceAssets& assets) {
    MapMigration migration;
    core::LevelData data = level.data();
    // Une variante n'écrit que ses entités : ses cases sont celles de sa base, migrée à part.
    if (data.base.empty()) {
        if (assets.appearance) {
            migration.namedPieces = namePieces(data.layers, *assets.appearance);
        }
        if (hasVisualLayer(data.layers)) {
            migration.newForcedCells = alignCollision(data, assets);
        }
    }
    migration.newIds = giveIds(data);
    migration.text = core::LevelWriter::buildJson(data);
    return migration;
}

MapMigration migrateMapFile(const std::filesystem::path& file,
                            const std::filesystem::path& dataRoot) {
    const core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(file);
    if (!loaded.ok()) {
        MapMigration failed;
        failed.error = file.string() + ": " + loaded.error;
        return failed;
    }
    return migrateLevel(*loaded.level,
                        loadPlaceAssets(dataRoot, scenePlaceOf(loaded.level->layers())));
}

namespace {

// La ligne de commande des commandes sans fenêtre.
struct MapCommandLine {
    bool check = false;
    bool migrate = false;
    std::optional<std::filesystem::path> gestures;
    std::filesystem::path dataRoot;
    std::optional<std::filesystem::path> outputFile;
    std::vector<std::string> targets;
};

[[nodiscard]] MapCommandLine parseMapCommand(const std::vector<std::string>& arguments,
                                             const std::filesystem::path& defaultDataRoot) {
    MapCommandLine line;
    line.dataRoot = defaultDataRoot;
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        const std::string& argument = arguments[index];
        const bool hasValue = index + 1 < arguments.size();
        if (argument == "--check") {
            line.check = true;
        } else if (argument == "--migrate") {
            line.migrate = true;
        } else if (argument == "--apply" && hasValue) {
            line.gestures = arguments[++index];
        } else if (argument == "--data" && hasValue) {
            line.dataRoot = arguments[++index];
        } else if (argument == "--output" && hasValue) {
            line.outputFile = arguments[++index];
        } else if ((line.migrate || line.gestures) && !argument.starts_with("--")) {
            line.targets.push_back(argument);
        }
    }
    return line;
}

// --apply : rejoue les gestes, puis écrit la carte et son annexe — rien si un geste est refusé.
int applyGestureCommand(const MapCommandLine& line, std::string& output) {
    if (line.migrate || line.targets.size() > 1) {
        output += "--apply edits one map, alone\n";
        return 2;
    }
    const std::string map = line.targets.empty() ? std::string{} : line.targets.front();
    std::filesystem::path mapFile;
    const GestureFileResult applied = applyGestureFile(*line.gestures, map, line.dataRoot, mapFile);
    for (const std::string& logged : applied.script.log) {
        output += logged + "\n";
    }
    if (!applied.script.ok()) {
        output += "error: " + applied.script.error + "\n";
        output += "nothing written\n";
        return 1;
    }
    const std::filesystem::path destination = line.outputFile.value_or(mapFile);
    std::ofstream stream(destination, std::ios::binary);
    stream.write(applied.mapText.data(), static_cast<std::streamsize>(applied.mapText.size()));
    stream.close();
    if (!stream.good()) {
        output += "error: cannot write " + destination.string() + "\n";
        return 1;
    }
    if (applied.sidecar && !writeSidecar(sidecarPath(destination), *applied.sidecar)) {
        output += "error: cannot write " + sidecarPath(destination).string() + "\n";
        return 1;
    }
    output += "applied " + line.gestures->string() + " to " + applied.mapId + ": " +
              std::to_string(applied.script.gestures) + " gestures, " +
              std::to_string(applied.script.steps) + " undo steps, written to " +
              destination.string() + "\n";
    return 0;
}

// --migrate : les cartes nommées, toutes à défaut, en place ou dans --output.
int migrateCommand(const MapCommandLine& line, std::string& output) {
    const std::filesystem::path levels = levelsOf(line.dataRoot);
    std::vector<std::filesystem::path> files;
    for (const std::string& target : line.targets) {
        const std::filesystem::path asPath{target};
        files.push_back(std::filesystem::is_regular_file(asPath) ? asPath
                                                                 : levels / (target + ".json"));
    }
    if (line.targets.empty()) {
        files = listMapFiles(levels);
    }
    if (line.outputFile && files.size() != 1) {
        output += "--output needs exactly one map to migrate\n";
        return 2;
    }
    for (const std::filesystem::path& file : files) {
        const MapMigration migration = migrateMapFile(file, line.dataRoot);
        if (!migration.ok()) {
            output += "error: " + migration.error + "\n";
            return 1;
        }
        const std::filesystem::path destination = line.outputFile.value_or(file);
        std::ofstream stream(destination, std::ios::binary);
        stream.write(migration.text.data(), static_cast<std::streamsize>(migration.text.size()));
        if (!stream.good()) {
            output += "error: cannot write " + destination.string() + "\n";
            return 1;
        }
        output += "migrated " + file.string() + ": " + std::to_string(migration.namedPieces) +
                  " pieces named, " + std::to_string(migration.newIds) + " ids given, " +
                  std::to_string(migration.newForcedCells) + " cells forced\n";
    }
    return 0;
}

// --check : toutes les cartes ; 1 à la première erreur.
int checkCommand(const std::filesystem::path& dataRoot, std::string& output) {
    const MapCheckReport report = checkAllMaps(dataRoot);
    for (const MapCheckFinding& finding : report.findings) {
        output += formatFinding(finding) + "\n";
    }
    // La bibliotheque de l'editeur (LOT-EDITOR-08) : un prefabrique ou un modele illisible est
    // une erreur, comme une carte illisible.
    const std::vector<LibraryFinding> library = checkEditorLibrary(dataRoot);
    for (const LibraryFinding& finding : library) {
        output += finding.file.string() + ": error: " + finding.message + "\n";
    }
    // LOT-123 : une base SANS AUCUNE CARTE n'est pas une erreur. La table rase (LOT-102) vide
    // `Levels/`, et l'etape CI du controle doit rester verte ce jour-la : l'absence de carte se
    // DIT -- ou le controle a cherche, et « 0 map » --, elle ne fait plus echouer. Ce qui garde
    // contre un dossier mal designe, c'est `--data`, qui nomme la racine.
    if (report.maps == 0) {
        output += "no map under " + levelsOf(dataRoot).string() + "\n";
    }
    output += "checked " + std::to_string(report.maps) +
              " maps: " + std::to_string(report.count(MapCheckSeverity::Error)) + " errors, " +
              std::to_string(report.count(MapCheckSeverity::Warning)) + " warnings\n";
    if (!library.empty()) {
        output += std::to_string(library.size()) + " unreadable editor library files\n";
        return 1;
    }
    return report.ok() ? 0 : 1;
}

}  // namespace

std::optional<int> runMapCommand(const std::vector<std::string>& arguments,
                                 const std::filesystem::path& defaultDataRoot,
                                 std::string& output) {
    const MapCommandLine line = parseMapCommand(arguments, defaultDataRoot);
    if (!line.check && !line.migrate && !line.gestures) {
        return std::nullopt;
    }
    if (line.gestures) {
        const int code = applyGestureCommand(line, output);
        if (code != 0 || !line.check) {
            return code;
        }
    }
    if (line.migrate) {
        const int code = migrateCommand(line, output);
        if (code != 0) {
            return code;
        }
    }
    return line.check ? checkCommand(line.dataRoot, output) : 0;
}

}  // namespace hmi
