// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/CanvasPicking.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "Core/Combat/IsoProjection.h"

namespace hmi {

namespace {

/// Décalage vertical d'un niveau d'élévation, en unités monde.
[[nodiscard]] float elevationLift(const core::IsoProjection& projection, int elevation) noexcept {
    return static_cast<float>(elevation) * ELEVATION_STEP_DIAMONDS * projection.tileHeight();
}

/// Un indice de case, par défaut (vers −∞) : le point (−0,2 ; 1) est dans la case (−1, 1), pas 0.
[[nodiscard]] int cellIndex(float coordinate) noexcept {
    // Au-delà de ce que tient un `int`, la case est de toute façon hors de la grille.
    constexpr auto LIMIT = static_cast<float>(std::numeric_limits<int>::max()) / 2.0F;
    return static_cast<int>(std::floor(std::clamp(coordinate, -LIMIT, LIMIT)));
}

[[nodiscard]] core::GridPosition clampToGrid(int column, int row, int columns, int rows) noexcept {
    return core::GridPosition{.column = std::clamp(column, 0, std::max(0, columns - 1)),
                              .row = std::clamp(row, 0, std::max(0, rows - 1))};
}

[[nodiscard]] CellRange clampRange(float minColumn, float minRow, float maxColumn, float maxRow,
                                   int columns, int rows) noexcept {
    CellRange range;
    range.firstColumn = std::max(0, cellIndex(minColumn));
    range.firstRow = std::max(0, cellIndex(minRow));
    range.lastColumn = std::min(columns - 1, cellIndex(maxColumn));
    range.lastRow = std::min(rows - 1, cellIndex(maxRow));
    return range;
}

}  // namespace

core::Vector2 isoGridPoint(const core::IsoProjection& projection, core::Vector2 world,
                           int elevation) noexcept {
    // Une case élevée est dessinée plus haut : on redescend le point d'autant avant d'inverser.
    return projection.worldToGrid({world.x, world.y + elevationLift(projection, elevation)});
}

std::optional<core::GridPosition> pickIsoCell(const core::IsoProjection& projection,
                                              core::Vector2 world, int elevation) noexcept {
    const core::Vector2 grid = isoGridPoint(projection, world, elevation);
    const core::GridPosition cell{.column = cellIndex(grid.x), .row = cellIndex(grid.y)};
    if (!projection.contains(cell)) {
        return std::nullopt;
    }
    return cell;
}

core::GridPosition clampedIsoCell(const core::IsoProjection& projection, core::Vector2 world,
                                  int elevation) noexcept {
    const core::Vector2 grid = isoGridPoint(projection, world, elevation);
    return clampToGrid(cellIndex(grid.x), cellIndex(grid.y), projection.columns(),
                       projection.rows());
}

std::optional<core::GridPosition> pickFlatCell(core::Vector2 world, int columns,
                                               int rows) noexcept {
    const core::GridPosition cell{.column = cellIndex(world.x), .row = cellIndex(world.y)};
    if (cell.column < 0 || cell.row < 0 || cell.column >= columns || cell.row >= rows) {
        return std::nullopt;
    }
    return cell;
}

core::GridPosition clampedFlatCell(core::Vector2 world, int columns, int rows) noexcept {
    return clampToGrid(cellIndex(world.x), cellIndex(world.y), columns, rows);
}

std::array<core::Vector2, 4> isoCellDiamond(const core::IsoProjection& projection,
                                            core::GridPosition cell, int elevation) noexcept {
    const auto column = static_cast<float>(cell.column);
    const auto row = static_cast<float>(cell.row);
    const float lift = elevationLift(projection, elevation);
    const auto at = [&](float gridColumn, float gridRow) {
        const core::Vector2 point = projection.gridToWorld({gridColumn, gridRow});
        return core::Vector2{point.x, point.y - lift};
    };
    // Le coin (c, r) est le sommet haut, (c+1, r) le droit, (c+1, r+1) le bas, (c, r+1) le gauche.
    return {at(column, row), at(column + 1.0F, row), at(column + 1.0F, row + 1.0F),
            at(column, row + 1.0F)};
}

CellRange isoCellsCovering(const core::IsoProjection& projection,
                           const core::Rect& world) noexcept {
    // L'image d'un rectangle dans la grille est un parallélogramme : sa boîte englobante contient
    // toute case dont le losange coupe le rectangle.
    const std::array<core::Vector2, 4> corners = {{
        projection.worldToGrid(world.position),
        projection.worldToGrid({world.position.x + world.size.x, world.position.y}),
        projection.worldToGrid({world.position.x, world.position.y + world.size.y}),
        projection.worldToGrid({world.position.x + world.size.x, world.position.y + world.size.y}),
    }};
    float minColumn = corners[0].x;
    float maxColumn = corners[0].x;
    float minRow = corners[0].y;
    float maxRow = corners[0].y;
    for (const core::Vector2& corner : corners) {
        minColumn = std::min(minColumn, corner.x);
        maxColumn = std::max(maxColumn, corner.x);
        minRow = std::min(minRow, corner.y);
        maxRow = std::max(maxRow, corner.y);
    }
    return clampRange(minColumn, minRow, maxColumn, maxRow, projection.columns(),
                      projection.rows());
}

CellRange flatCellsCovering(const core::Rect& world, int columns, int rows) noexcept {
    return clampRange(world.position.x, world.position.y, world.position.x + world.size.x,
                      world.position.y + world.size.y, columns, rows);
}

}  // namespace hmi
