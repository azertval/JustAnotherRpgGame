// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/BattleGrid.h"

#include <utility>
#include <variant>

#include "Core/Levels/Level.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileType.h"

namespace core {

namespace {

// Emprise maximale : Gigantesque, 4 × 4 (`footprintSide`). Au-delà, le côté est une erreur
// d'appel, pas une créature.
constexpr int LARGEST_SIDE = 4;

[[nodiscard]] std::size_t cellCount(int width, int height) {
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

// Vrai seulement pour un booléen `true` : un `"difficultTerrain": 1` ou `"oui"` est une faute de
// saisie, et la lire comme vraie ferait d'une coquille une règle.
[[nodiscard]] bool marksDifficult(const PropertyMap& properties) {
    const auto found = properties.find(std::string(DIFFICULT_TERRAIN_PROPERTY));
    if (found == properties.end()) {
        return false;
    }
    const bool* value = std::get_if<bool>(&found->second);
    return value != nullptr && *value;
}

}  // namespace

BattleGrid::BattleGrid(const TileMap& collision)
    : _width(collision.width()),
      _height(collision.height()),
      _terrain(cellCount(_width, _height), Terrain::Open),
      _difficult(cellCount(_width, _height), false),
      _occupants(cellCount(_width, _height)) {
    for (int row = 0; row < _height; ++row) {
        for (int column = 0; column < _width; ++column) {
            const TileType type = collision.tile(column, row);
            if (!isSolid(type)) {
                continue;
            }
            // Ce que le vol franchit se nomme ici, et nulle part ailleurs : tout autre type plein
            // arrête aussi un volant, y compris une porte fermée, que le contrôleur de mécanismes
            // écrit comme de la matière pleine.
            const bool groundOnly = type == TileType::DeepWater || type == TileType::Cliff;
            _terrain[indexOf({.column = column, .row = row})] =
                groundOnly ? Terrain::GroundObstacle : Terrain::Solid;
        }
    }
}

BattleGrid::BattleGrid(const Level& level, const TileMap& collision) : BattleGrid(collision) {
    addZones(level);
}

BattleGrid::BattleGrid(const Level& level) : BattleGrid(level, level.tileMap()) {}

void BattleGrid::addZones(const Level& level) {
    addLayerZones(level);
    // Les zones posees comme entites (decision D13) : un rectangle ou des cases peintes, apres les
    // couches -- `zonesAt` les rend dans l'ordre des couches, puis des entites.
    addEntityZones(level);
}

void BattleGrid::addLayerZones(const Level& level) {
    for (const TileLayer& layer : level.layers()) {
        if (layer.properties.empty() || layer.tiles.width() != _width ||
            layer.tiles.height() != _height) {
            continue;
        }
        Zone zone{.properties = layer.properties,
                  .cells = std::vector<bool>(_terrain.size(), false)};
        const bool difficult = marksDifficult(layer.properties);
        for (int row = 0; row < _height; ++row) {
            for (int column = 0; column < _width; ++column) {
                if (layer.tiles.tile(column, row) == TileType::Empty) {
                    continue;
                }
                const std::size_t index = indexOf({.column = column, .row = row});
                zone.cells[index] = true;
                if (difficult) {
                    _difficult[index] = true;
                }
            }
        }
        _zones.push_back(std::move(zone));
    }
}

void BattleGrid::addEntityZones(const Level& level) {
    for (const MapEntity& entity : level.entities()) {
        if (entity.type != ZONE_ENTITY_TYPE) {
            continue;
        }
        Zone zone{.properties = entity.properties,
                  .cells = std::vector<bool>(_terrain.size(), false)};
        const bool difficult = marksDifficult(entity.properties);
        for (const GridPosition cell : zoneCells(entity)) {
            if (!inBounds(cell)) {
                continue;
            }
            const std::size_t index = indexOf(cell);
            zone.cells[index] = true;
            if (difficult) {
                _difficult[index] = true;
            }
        }
        _zones.push_back(std::move(zone));
    }
}

std::size_t BattleGrid::indexOf(GridPosition cell) const noexcept {
    return (static_cast<std::size_t>(cell.row) * static_cast<std::size_t>(_width)) +
           static_cast<std::size_t>(cell.column);
}

bool BattleGrid::inBounds(GridPosition cell) const noexcept {
    return cell.column >= 0 && cell.row >= 0 && cell.column < _width && cell.row < _height;
}

bool BattleGrid::isObstructed(GridPosition cell, Locomotion locomotion) const {
    if (!inBounds(cell)) {
        return true;
    }
    const std::size_t index = indexOf(cell);
    const Terrain terrain = _terrain[index];
    if (terrain == Terrain::Solid ||
        (terrain == Terrain::GroundObstacle && locomotion == Locomotion::Walk)) {
        return true;
    }
    const auto object = _objects.find(index);
    return object != _objects.end() && object->second.blocksMovement;
}

bool BattleGrid::isDifficult(GridPosition cell) const {
    return inBounds(cell) && _difficult[indexOf(cell)];
}

bool BattleGrid::blocksSight(GridPosition cell) const {
    if (!inBounds(cell)) {
        return false;
    }
    const std::size_t index = indexOf(cell);
    return _terrain[index] == Terrain::Solid || objectCoverAt(cell) == Cover::Total;
}

Cover BattleGrid::objectCoverAt(GridPosition cell) const {
    const GridObject* object = objectAt(cell);
    return object == nullptr ? Cover::None : object->cover;
}

void BattleGrid::setDifficult(GridPosition cell, bool difficult) {
    if (inBounds(cell)) {
        _difficult[indexOf(cell)] = difficult;
    }
}

std::vector<const PropertyMap*> BattleGrid::zonesAt(GridPosition cell) const {
    std::vector<const PropertyMap*> zones;
    if (!inBounds(cell)) {
        return zones;
    }
    const std::size_t index = indexOf(cell);
    for (const Zone& zone : _zones) {
        if (zone.cells[index]) {
            zones.push_back(&zone.properties);
        }
    }
    return zones;
}

bool BattleGrid::isClear(GridPosition anchor, int side, Locomotion locomotion) const {
    if (side < 1 || side > LARGEST_SIDE) {
        return false;
    }
    for (int row = anchor.row; row < anchor.row + side; ++row) {
        for (int column = anchor.column; column < anchor.column + side; ++column) {
            if (isObstructed({.column = column, .row = row}, locomotion)) {
                return false;
            }
        }
    }
    return true;
}

bool BattleGrid::canStand(GridPosition anchor, int side, std::optional<CombatantId> self,
                          Locomotion locomotion) const {
    if (!isClear(anchor, side, locomotion)) {
        return false;
    }
    for (int row = anchor.row; row < anchor.row + side; ++row) {
        for (int column = anchor.column; column < anchor.column + side; ++column) {
            const std::optional<CombatantId>& occupant =
                _occupants[indexOf({.column = column, .row = row})];
            if (occupant.has_value() && occupant != self) {
                return false;
            }
        }
    }
    return true;
}

PlacementResult BattleGrid::check(GridPosition anchor, int side, CombatantId self,
                                  Locomotion locomotion) const {
    const GridPosition farCorner{.column = anchor.column + side - 1, .row = anchor.row + side - 1};
    if (!inBounds(anchor) || !inBounds(farCorner)) {
        return PlacementResult::OutOfBounds;
    }
    if (!isClear(anchor, side, locomotion)) {
        return PlacementResult::Obstructed;
    }
    // Le combattant ne se gêne pas lui-même : une créature de 2 × 2 qui avance d'une case
    // recouvre la moitié de son ancienne emprise.
    if (!canStand(anchor, side, self, locomotion)) {
        return PlacementResult::Occupied;
    }
    return PlacementResult::Placed;
}

void BattleGrid::fill(const Placement& placement, std::optional<CombatantId> occupant) {
    for (int row = placement.anchor.row; row < placement.anchor.row + placement.side; ++row) {
        for (int column = placement.anchor.column;
             column < placement.anchor.column + placement.side; ++column) {
            _occupants[indexOf({.column = column, .row = row})] = occupant;
        }
    }
}

PlacementResult BattleGrid::place(CombatantId combatant, GridPosition anchor, int side,
                                  Locomotion locomotion) {
    if (_placements.contains(combatant) || side < 1 || side > LARGEST_SIDE) {
        return PlacementResult::InvalidCombatant;
    }
    const PlacementResult result = check(anchor, side, combatant, locomotion);
    if (result != PlacementResult::Placed) {
        return result;
    }
    const Placement placement{.anchor = anchor, .side = side};
    _placements.emplace(combatant, placement);
    fill(placement, combatant);
    return PlacementResult::Placed;
}

PlacementResult BattleGrid::moveTo(CombatantId combatant, GridPosition anchor,
                                   Locomotion locomotion) {
    const auto found = _placements.find(combatant);
    if (found == _placements.end()) {
        return PlacementResult::InvalidCombatant;
    }
    const PlacementResult result = check(anchor, found->second.side, combatant, locomotion);
    if (result != PlacementResult::Placed) {
        return result;
    }
    fill(found->second, std::nullopt);
    found->second.anchor = anchor;
    fill(found->second, combatant);
    return PlacementResult::Placed;
}

bool BattleGrid::remove(CombatantId combatant) {
    const auto found = _placements.find(combatant);
    if (found == _placements.end()) {
        return false;
    }
    fill(found->second, std::nullopt);
    _placements.erase(found);
    return true;
}

std::optional<CombatantId> BattleGrid::occupantAt(GridPosition cell) const {
    if (!inBounds(cell)) {
        return std::nullopt;
    }
    return _occupants[indexOf(cell)];
}

std::optional<GridPosition> BattleGrid::positionOf(CombatantId combatant) const {
    const auto found = _placements.find(combatant);
    if (found == _placements.end()) {
        return std::nullopt;
    }
    return found->second.anchor;
}

int BattleGrid::sideOf(CombatantId combatant) const {
    const auto found = _placements.find(combatant);
    return found == _placements.end() ? 0 : found->second.side;
}

PlacementResult BattleGrid::placeObject(GridPosition cell, GridObject object) {
    if (!inBounds(cell)) {
        return PlacementResult::OutOfBounds;
    }
    const std::size_t index = indexOf(cell);
    if (_terrain[index] != Terrain::Open) {
        return PlacementResult::Obstructed;
    }
    if (_objects.contains(index) || (object.blocksMovement && _occupants[index].has_value())) {
        return PlacementResult::Occupied;
    }
    _objects.emplace(index, std::move(object));
    return PlacementResult::Placed;
}

const GridObject* BattleGrid::objectAt(GridPosition cell) const {
    if (!inBounds(cell)) {
        return nullptr;
    }
    const auto found = _objects.find(indexOf(cell));
    return found == _objects.end() ? nullptr : &found->second;
}

bool BattleGrid::damageObject(GridPosition cell, int damage) {
    if (!inBounds(cell) || damage <= 0) {
        return false;
    }
    const auto found = _objects.find(indexOf(cell));
    if (found == _objects.end()) {
        return false;
    }
    found->second.hitPoints -= damage;
    if (found->second.hitPoints > 0) {
        return false;
    }
    _objects.erase(found);
    return true;
}

std::vector<CombatantId> BattleGrid::combatants() const {
    std::vector<CombatantId> identifiers;
    identifiers.reserve(_placements.size());
    for (const auto& [identifier, placement] : _placements) {
        identifiers.push_back(identifier);
    }
    return identifiers;
}

}  // namespace core
