// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/MapEncounter.h"

#include <algorithm>
#include <cstdlib>
#include <utility>

namespace core {

GridPosition zoneToMap(const CombatZone& zone, GridPosition cell) noexcept {
    return GridPosition{.column = cell.column + zone.origin.column,
                        .row = cell.row + zone.origin.row};
}

GridPosition mapToZone(const CombatZone& zone, GridPosition cell) noexcept {
    return GridPosition{.column = cell.column - zone.origin.column,
                        .row = cell.row - zone.origin.row};
}

namespace {

// L'ordre des cases libres : de la plus proche de @p voulue a la plus lointaine (distance de
// Tchebychev, celle de la grille), a egalite dans l'ordre de lecture -- deterministe, donc
// rejouable.
[[nodiscard]] int distance(GridPosition a, GridPosition b) noexcept {
    return std::max(std::abs(a.column - b.column), std::abs(a.row - b.row));
}

// La case libre de @p libres la plus proche de @p voulue, hors de @p prises ; rien si tout est
// pris.
[[nodiscard]] std::optional<GridPosition> plusProcheLibre(const std::vector<GridPosition>& libres,
                                                          const std::vector<GridPosition>& prises,
                                                          GridPosition voulue) {
    std::optional<GridPosition> meilleure;
    int meilleureDistance = 0;
    for (const GridPosition cell : libres) {
        if (std::ranges::find(prises, cell) != prises.end()) {
            continue;
        }
        const int d = distance(cell, voulue);
        if (!meilleure.has_value() || d < meilleureDistance) {
            meilleure = cell;
            meilleureDistance = d;
        }
    }
    return meilleure;
}

[[nodiscard]] std::string nomDe(GridPosition cell) {
    return std::to_string(cell.column) + "," + std::to_string(cell.row);
}

}  // namespace

MapEncounterResult prepareMapEncounter(const Level& map, std::string_view mapId,
                                       const Encounter& encounter, GridPosition trigger,
                                       GridPosition heroCell,
                                       const ExplorationSnapshot& exploration,
                                       std::string defeatFlagKey) {
    MapEncounterResult result;

    // La zone : celle du declencheur, a defaut celle du heros. Le terrain deja analyse par
    // l'editeur (`analyzeCombatZones`) dit si elle est jouable et quelles cases y sont libres.
    const std::vector<CombatZoneTerrain> terrains =
        analyzeCombatZones(map.tileMap(), map.entities());
    const CombatZoneTerrain* choisie = nullptr;
    for (const GridPosition point : {trigger, heroCell}) {
        for (const CombatZoneTerrain& terrain : terrains) {
            if (terrain.zone.contains(point)) {
                choisie = &terrain;
                break;
            }
        }
        if (choisie != nullptr) {
            break;
        }
    }
    if (choisie == nullptr) {
        result.issue = std::string{mapId} + " : aucune zone de combat ne contient le declencheur " +
                       nomDe(trigger) + " ni le heros " + nomDe(heroCell) + ".";
        return result;
    }
    if (choisie->issue.has_value() || choisie->freeCells.empty()) {
        result.issue = std::string{mapId} + " : la zone de combat « " + choisie->zone.name +
                       " » n'est pas jouable.";
        return result;
    }
    const CombatZone& zone = choisie->zone;

    MapEncounterSetup setup{
        .zone = zone,
        .battlefield = cropLevelToZone(map, zone),
        .heroCell = {},
        .run = beginEncounter(encounter, exploration, trigger, std::move(defeatFlagKey)),
        .notes = {}};

    // Les cases libres, en cases de la carte ; ce qui est pris s'y retire au fil des places.
    std::vector<GridPosition> prises;
    const auto libre = [&](GridPosition cell) {
        return std::ranges::find(prises, cell) == prises.end() &&
               std::ranges::find(choisie->freeCells, cell) != choisie->freeCells.end();
    };
    const auto placer = [&](GridPosition voulue, std::string_view qui) -> GridPosition {
        if (libre(voulue)) {
            prises.push_back(voulue);
            return voulue;
        }
        // Une place hors de la zone, dans un mur ou deja prise se rapproche de ce qu'elle voulait.
        // La zone a au moins une case libre, et l'on a moins de combattants que de cases : le cas
        // ou tout est pris se repli sur la case voulue, que le montage refusera en le disant.
        const std::optional<GridPosition> proche =
            plusProcheLibre(choisie->freeCells, prises, voulue);
        if (!proche.has_value()) {
            return voulue;
        }
        prises.push_back(*proche);
        setup.notes.push_back(std::string{qui} + " : place " + nomDe(voulue) +
                              " indisponible, pose en " + nomDe(*proche));
        return *proche;
    };

    // Le heros d'abord : c'est lui que la rencontre entoure.
    setup.heroCell = mapToZone(zone, placer(heroCell, "heros"));
    for (CombatantPlacement& placement : setup.run.placements) {
        placement.position = mapToZone(zone, placer(placement.position, placement.creatureId));
    }
    result.setup = std::move(setup);
    return result;
}

}  // namespace core
