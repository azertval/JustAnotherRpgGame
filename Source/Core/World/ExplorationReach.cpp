// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/World/ExplorationReach.h"

#include <array>
#include <deque>

#include "Core/World/ExplorationSession.h"

namespace core {

// La regle de l'en-tete tient tant que le gabarit du heros est plus petit qu'une case : au-dela,
// un couloir d'une case ne se passerait plus, et ce parcours mentirait.
static_assert(ExplorationSession::HERO_HALF_SIZE_CELLS < 0.5F,
              "le heros doit tenir dans une case pour que l'atteinte se lise case par case");

namespace {

[[nodiscard]] std::size_t indexOf(GridPosition cell, int width) {
    return (static_cast<std::size_t>(cell.row) * static_cast<std::size_t>(width)) +
           static_cast<std::size_t>(cell.column);
}

}  // namespace

ExplorationReach::ExplorationReach(const TileMap& collision,
                                   const std::vector<GridPosition>& starts)
    : _width(collision.width()),
      _height(collision.height()),
      _reached(static_cast<std::size_t>(_width) * static_cast<std::size_t>(_height), false) {
    const auto walkable = [&collision](GridPosition cell) {
        return collision.inBounds(cell.column, cell.row) &&
               !collision.isSolid(cell.column, cell.row);
    };
    std::deque<GridPosition> pending;
    const auto visit = [&](GridPosition cell) {
        if (!walkable(cell) || _reached[indexOf(cell, _width)]) {
            return;
        }
        _reached[indexOf(cell, _width)] = true;
        ++_count;
        pending.push_back(cell);
    };
    for (const GridPosition start : starts) {
        visit(start);
    }
    static constexpr std::array<std::array<int, 2>, 4> SIDES = {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
    while (!pending.empty()) {
        const GridPosition cell = pending.front();
        pending.pop_front();
        for (const auto& [dx, dy] : SIDES) {
            visit({.column = cell.column + dx, .row = cell.row + dy});
        }
    }
}

bool ExplorationReach::reaches(GridPosition cell) const {
    if (cell.column < 0 || cell.row < 0 || cell.column >= _width || cell.row >= _height) {
        return false;
    }
    return _reached[indexOf(cell, _width)];
}

}  // namespace core
