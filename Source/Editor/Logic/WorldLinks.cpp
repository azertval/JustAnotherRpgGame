// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/WorldLinks.h"

#include <algorithm>
#include <array>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/MapEntity.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/ExplorationReach.h"

namespace hmi {

namespace {

/// Le fichier de la carte @p mapId sous @p dataRoot.
[[nodiscard]] std::filesystem::path mapFileOf(const std::filesystem::path& dataRoot,
                                              std::string_view mapId) {
    return dataRoot / "Levels" / (std::string{mapId} + ".json");
}

/// Le dernier segment d'un identifiant de carte : `capital/martpart` -> `martpart`.
[[nodiscard]] std::string lastSegment(std::string_view mapId) {
    const std::size_t slash = mapId.rfind('/');
    return std::string{slash == std::string_view::npos ? mapId : mapId.substr(slash + 1)};
}

/// Les noms des points d'arrivee de @p level.
[[nodiscard]] std::vector<std::string> arrivalNames(const core::Level& level) {
    std::vector<std::string> names;
    for (const core::MapEntity& entity : level.entities()) {
        if (entity.type != core::SPAWN_POINT_ENTITY_TYPE) {
            continue;
        }
        const auto found = entity.properties.find(std::string{core::SPAWN_POINT_NAME_PROPERTY});
        if (found == entity.properties.end()) {
            continue;
        }
        if (const auto* const text = std::get_if<std::string>(&found->second)) {
            names.push_back(*text);
        }
    }
    return names;
}

/// Un portail vers @p targetMap, au point d'arrivee @p arrival.
[[nodiscard]] core::MapEntity portalEntity(core::GridPosition cell, std::string_view targetMap,
                                           std::string_view arrival) {
    return core::MapEntity{
        .type = std::string{core::PORTAL_ENTITY_TYPE},
        .position = cell,
        .properties = {{std::string{core::PORTAL_TARGET_MAP_PROPERTY}, std::string{targetMap}},
                       {std::string{core::PORTAL_ARRIVAL_PROPERTY}, std::string{arrival}}}};
}

/// Un point d'arrivee nomme @p name.
[[nodiscard]] core::MapEntity arrivalEntity(core::GridPosition cell, std::string_view name) {
    return core::MapEntity{
        .type = std::string{core::SPAWN_POINT_ENTITY_TYPE},
        .position = cell,
        .properties = {{std::string{core::SPAWN_POINT_NAME_PROPERTY}, std::string{name}}}};
}

/// Le carre de la distance de @p cell a @p entry : ce qui ordonne les cases candidates.
[[nodiscard]] long long distanceSquared(core::GridPosition cell, core::GridPosition entry) {
    const long long dc = cell.column - entry.column;
    const long long dr = cell.row - entry.row;
    return (dc * dc) + (dr * dr);
}

/// Une carte lue, prete a recevoir les deux entites d'un bout de lien.
struct LinkedMap {
    std::filesystem::path file;
    core::LevelDraft draft;
    core::GridPosition portalCell;
    core::GridPosition arrivalCell;
    std::vector<std::string> arrivals;
};

}  // namespace

std::string arrivalNameFrom(std::string_view fromMap, const std::vector<std::string>& taken) {
    const std::string base = "from-" + lastSegment(fromMap);
    std::string name = base;
    int suffix = 1;
    while (std::ranges::find(taken, name) != taken.end()) {
        ++suffix;
        name = base + "-" + std::to_string(suffix);
    }
    return name;
}

bool linkCells(const core::Level& level, core::GridPosition& portalCell,
               core::GridPosition& arrivalCell) {
    const core::TileMap& collision = level.tileMap();
    const core::ExplorationReach reach(collision, {level.entry()});
    std::set<std::pair<int, int>> occupied{{level.entry().column, level.entry().row}};
    for (const core::MapEntity& entity : level.entities()) {
        occupied.emplace(entity.position.column, entity.position.row);
        for (const core::GridPosition cell : entity.cells) {
            occupied.emplace(cell.column, cell.row);
        }
    }

    std::vector<core::GridPosition> candidates;
    for (int row = 0; row < collision.height(); ++row) {
        for (int column = 0; column < collision.width(); ++column) {
            const core::GridPosition cell{.column = column, .row = row};
            if (collision.isSolid(column, row) || occupied.contains({column, row}) ||
                !reach.reaches(cell)) {
                continue;
            }
            candidates.push_back(cell);
        }
    }
    if (candidates.size() < 2) {
        return false;
    }
    // Au plus pres de l'entree ; a distance egale, l'ordre de lecture de la carte (ligne, colonne),
    // deja celui de `candidates` -- un tri stable le garde.
    const core::GridPosition entry = level.entry();
    std::ranges::stable_sort(candidates, [entry](core::GridPosition lhs, core::GridPosition rhs) {
        return distanceSquared(lhs, entry) < distanceSquared(rhs, entry);
    });
    portalCell = candidates[0];
    arrivalCell = candidates[1];
    return true;
}

RefactorPlan planLinkMaps(const std::filesystem::path& dataRoot, std::string_view fromMap,
                          std::string_view toMap, MapLink* link) {
    RefactorPlan plan;
    if (fromMap.empty() || toMap.empty()) {
        plan.error = "A link needs two maps.";
        return plan;
    }
    if (fromMap == toMap) {
        plan.error = "A map cannot be linked to itself: the graph would gain nothing.";
        return plan;
    }

    std::vector<LinkedMap> ends;
    for (const std::string_view mapId : {fromMap, toMap}) {
        const std::filesystem::path file = mapFileOf(dataRoot, mapId);
        core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(file);
        if (!loaded.ok()) {
            plan.error = std::string{mapId} + " cannot be read: " + loaded.error;
            return plan;
        }
        core::GridPosition portalCell;
        core::GridPosition arrivalCell;
        if (!linkCells(*loaded.level, portalCell, arrivalCell)) {
            plan.error =
                std::string{mapId} +
                " has no two free cells reachable from its entry: place the portal by hand.";
            return plan;
        }
        ends.push_back(LinkedMap{.file = file,
                                 .draft = core::LevelDraft::fromLevel(*loaded.level),
                                 .portalCell = portalCell,
                                 .arrivalCell = arrivalCell,
                                 .arrivals = arrivalNames(*loaded.level)});
    }

    // Le nom du point d'arrivee de chaque carte dit d'ou l'on vient : c'est ce que cite le portail
    // d'en face.
    const std::array<std::string_view, 2> ids{fromMap, toMap};
    const std::array<std::string_view, 2> others{toMap, fromMap};
    const std::array<std::string, 2> arrivalsHere{arrivalNameFrom(toMap, ends[0].arrivals),
                                                  arrivalNameFrom(fromMap, ends[1].arrivals)};
    const std::array<std::string, 2> arrivalsThere{arrivalsHere[1], arrivalsHere[0]};

    MapLink built;
    const std::array<MapLinkEnd*, 2> sides{&built.from, &built.to};
    for (std::size_t side = 0; side < ends.size(); ++side) {
        LinkedMap& end = ends[side];
        const std::optional<std::size_t> portal =
            end.draft.placeEntity(portalEntity(end.portalCell, others[side], arrivalsThere[side]));
        const std::optional<std::size_t> arrival =
            end.draft.placeEntity(arrivalEntity(end.arrivalCell, arrivalsHere[side]));
        if (!portal || !arrival) {
            plan.error = std::string{ids[side]} + ": the chosen cells are out of the map.";
            return plan;
        }
        *sides[side] = MapLinkEnd{.mapId = std::string{ids[side]},
                                  .portalCell = end.portalCell,
                                  .arrivalCell = end.arrivalCell,
                                  .arrivalName = arrivalsHere[side],
                                  .portalId = end.draft.entities()[*portal].id,
                                  .arrivalId = end.draft.entities()[*arrival].id};
        plan.edits.push_back(ProjectEdit{.file = end.file, .text = end.draft.toJson()});
        plan.changes.push_back(Citation{.file = end.file,
                                        .mapId = std::string{ids[side]},
                                        .entityId = sides[side]->portalId,
                                        .cell = end.portalCell,
                                        .what = "portal " + sides[side]->portalId + " -> " +
                                                std::string{others[side]} + " (" +
                                                arrivalsThere[side] + ")"});
        plan.changes.push_back(Citation{.file = end.file,
                                        .mapId = std::string{ids[side]},
                                        .entityId = sides[side]->arrivalId,
                                        .cell = end.arrivalCell,
                                        .what = "arrival point " + arrivalsHere[side]});
    }
    if (link != nullptr) {
        *link = built;
    }
    return plan;
}

}  // namespace hmi
