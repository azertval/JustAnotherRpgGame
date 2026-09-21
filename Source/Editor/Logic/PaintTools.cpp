// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/PaintTools.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <tuple>
#include <utility>

#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/PieceFootprint.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "Editor/Logic/PieceCatalog.h"

namespace hmi {

namespace {

/// @brief Un rectangle de cases, bornes incluses.
struct CellBox {
    int minColumn = 0;
    int minRow = 0;
    int maxColumn = 0;
    int maxRow = 0;

    [[nodiscard]] bool intersects(const CellBox& other) const noexcept {
        return minColumn <= other.maxColumn && other.minColumn <= maxColumn &&
               minRow <= other.maxRow && other.minRow <= maxRow;
    }
};

[[nodiscard]] CellBox boxOf(core::GridPosition first, core::GridPosition last) noexcept {
    return CellBox{.minColumn = std::min(first.column, last.column),
                   .minRow = std::min(first.row, last.row),
                   .maxColumn = std::max(first.column, last.column),
                   .maxRow = std::max(first.row, last.row)};
}

[[nodiscard]] CellBox footprintBox(core::GridPosition anchor, core::PieceFootprint footprint) {
    return CellBox{.minColumn = anchor.column,
                   .minRow = anchor.row,
                   .maxColumn = anchor.column + footprint.columns - 1,
                   .maxRow = anchor.row + footprint.rows - 1};
}

/// @return L'emprise que @p brush occupe ancré en @p cell : celle de sa pièce, une case sinon.
[[nodiscard]] CellBox brushBox(const core::LevelDraft& draft, const CanvasBrush& brush,
                               core::GridPosition cell) {
    if (brush.kind == BrushKind::Piece) {
        return footprintBox(cell, draft.pieceFootprint(brush.piece));
    }
    return footprintBox(cell, core::PieceFootprint{});
}

/// @return @p brush vu dans le miroir : la jumelle d'une pièce, et le type de sa case d'ancrage.
[[nodiscard]] CanvasBrush mirroredBrush(const core::LevelDraft& draft, const CanvasBrush& brush,
                                        const StrokeContext& context) {
    if (brush.kind != BrushKind::Piece) {
        return brush;
    }
    CanvasBrush twin = brush;
    twin.piece = mirrorPieceName(draft.pieceManifest(), brush.piece);
    if (twin.piece != brush.piece) {
        twin.type = pieceCellType(context.appearance, twin.piece, brush.floor);
    }
    return twin;
}

/// Ajoute le résultat d'un pas au résultat du geste : changé si un pas a changé, premier refus.
void merge(BrushResult& total, const BrushResult& step) {
    total.changed = total.changed || step.changed;
    if (total.refusal.empty() && !step.refusal.empty()) {
        total.refusal = step.refusal;
    }
}

[[nodiscard]] BrushResult finish(BrushResult total) {
    if (total.changed) {
        total.refusal.clear();  // un refus partiel (une case au bord) ne se dit pas.
    }
    return total;
}

[[nodiscard]] bool hasVisualLayers(const core::LevelDraft& draft) {
    return std::ranges::any_of(draft.layers(), [](const core::TileLayer& layer) {
        return core::isVisualLayerKind(layer.kind);
    });
}

/// @brief Le contenu d'une case pour le seau (voir `floodRegion`).
struct CellContent {
    core::TileType type = core::TileType::Empty;
    std::string_view piece;
    /// L'ancre de la pièce large qui couvre la case, s'il y en a une.
    std::optional<core::GridPosition> wideAnchor;

    [[nodiscard]] bool operator==(const CellContent&) const = default;
};

[[nodiscard]] CellContent contentAt(const core::LevelDraft& draft, LayerSlot slot,
                                    core::GridPosition cell) {
    if (!slot) {
        return CellContent{.type = draft.tileMap().tile(cell.column, cell.row),
                           .piece = {},
                           .wideAnchor = std::nullopt};
    }
    const core::TileLayer& layer = draft.layers()[*slot];
    CellContent content{.type = layer.tiles.tile(cell.column, cell.row),
                        .piece = layer.pieceAt(cell.column, cell.row),
                        .wideAnchor = std::nullopt};
    if (const std::optional<core::GridPosition> anchor = draft.pieceAnchorAt(*slot, cell)) {
        const core::PieceFootprint footprint =
            draft.pieceFootprint(layer.pieceAt(anchor->column, anchor->row));
        if (footprint.columns > 1 || footprint.rows > 1) {
            // Toutes les cases d'une pièce large se valent : elles ne portent que cette pièce.
            return CellContent{.type = core::TileType::Empty, .piece = {}, .wideAnchor = anchor};
        }
    }
    return content;
}

/// @return Le pinceau qui repeint ce que la couche visuelle @p index montre en @p cell.
[[nodiscard]] std::optional<CanvasBrush> brushFromLayer(const core::LevelDraft& draft,
                                                        std::size_t index, core::GridPosition cell,
                                                        const PlaceAppearance* appearance) {
    const core::TileLayer& layer = draft.layers()[index];
    if (const std::optional<core::GridPosition> anchor = draft.pieceAnchorAt(index, cell)) {
        const std::string piece{layer.pieceAt(anchor->column, anchor->row)};
        const core::ScenePieceManifest* const manifest = draft.pieceManifest();
        const core::ScenePiece* const known = manifest != nullptr ? manifest->find(piece) : nullptr;
        // Une pièce que le manifeste ignore est un sol si elle est sur une couche de sol.
        const bool floor = known != nullptr ? known->pieceClass == core::ScenePieceClass::Floor
                                            : layer.kind == core::LayerKind::Ground;
        return CanvasBrush{.kind = BrushKind::Piece,
                           .type = pieceCellType(appearance, piece, floor),
                           .piece = piece,
                           .floor = floor};
    }
    const core::TileType type = layer.tiles.tile(cell.column, cell.row);
    if (type == core::TileType::Empty) {
        return std::nullopt;
    }
    return CanvasBrush{.kind = BrushKind::Type, .type = type, .piece = {}, .floor = false};
}

}  // namespace

MirrorAxis mirrorAxisThrough(core::GridPosition cell) noexcept {
    return MirrorAxis{.offset = cell.column - cell.row};
}

core::GridPosition mirrorCell(const MirrorAxis& axis, core::GridPosition cell) noexcept {
    return core::GridPosition{.column = cell.row + axis.offset, .row = cell.column - axis.offset};
}

std::string mirrorPieceName(const core::ScenePieceManifest* manifest, std::string_view piece) {
    if (manifest == nullptr) {
        return std::string{piece};
    }
    const core::ScenePiece* const known = manifest->find(piece);
    if (known == nullptr) {
        return std::string{piece};
    }
    if (!known->mirrorOf.empty()) {
        return known->mirrorOf;
    }
    for (const core::ScenePiece& other : manifest->pieces()) {
        if (other.mirrorOf == known->name) {
            return other.name;
        }
    }
    return std::string{piece};
}

BrushResult applyStroke(core::LevelDraft& draft, const CanvasBrush& brush, LayerSlot active,
                        const LayerViewState& view, const std::vector<core::GridPosition>& cells,
                        bool continuing, const StrokeContext& context) {
    const core::GestureScope gesture(draft);
    const CanvasBrush twin = mirroredBrush(draft, brush, context);
    BrushResult total;
    bool first = true;
    for (const core::GridPosition cell : cells) {
        const bool prolongs = continuing || !first;
        first = false;
        merge(total, applyBrush(draft, brush, active, view, cell, cell, prolongs));
        if (!context.mirror) {
            continue;
        }
        const core::GridPosition reflected = mirrorCell(*context.mirror, cell);
        if (!draft.tileMap().inBounds(reflected.column, reflected.row) ||
            brushBox(draft, brush, cell).intersects(brushBox(draft, twin, reflected))) {
            continue;  // hors de la carte, ou le reflet chevaucherait le geste.
        }
        merge(total, applyBrush(draft, twin, active, view, reflected, reflected, prolongs));
    }
    return finish(std::move(total));
}

BrushResult applyRectangleStroke(core::LevelDraft& draft, const CanvasBrush& brush,
                                 LayerSlot active, const LayerViewState& view,
                                 core::GridPosition first, core::GridPosition last,
                                 const StrokeContext& context) {
    const core::GestureScope gesture(draft);
    BrushResult total = applyBrush(draft, brush, active, view, first, last);
    if (context.mirror) {
        const core::GridPosition reflectedFirst = mirrorCell(*context.mirror, first);
        const core::GridPosition reflectedLast = mirrorCell(*context.mirror, last);
        if (!boxOf(first, last).intersects(boxOf(reflectedFirst, reflectedLast))) {
            merge(total, applyBrush(draft, mirroredBrush(draft, brush, context), active, view,
                                    reflectedFirst, reflectedLast));
        }
    }
    return finish(std::move(total));
}

std::vector<core::GridPosition> lineCells(core::GridPosition from, core::GridPosition to) {
    std::vector<core::GridPosition> cells;
    const int deltaColumns = std::abs(to.column - from.column);
    const int deltaRows = -std::abs(to.row - from.row);
    const int stepColumn = from.column < to.column ? 1 : -1;
    const int stepRow = from.row < to.row ? 1 : -1;
    int error = deltaColumns + deltaRows;
    core::GridPosition cell = from;
    while (true) {
        cells.push_back(cell);
        if (cell == to) {
            break;
        }
        const int doubled = 2 * error;
        if (doubled >= deltaRows) {
            error += deltaRows;
            cell.column += stepColumn;
        }
        if (doubled <= deltaColumns) {
            error += deltaColumns;
            cell.row += stepRow;
        }
    }
    return cells;
}

std::vector<core::GridPosition> floodRegion(const core::LevelDraft& draft, LayerSlot slot,
                                            core::GridPosition seed) {
    const core::TileMap& map = draft.tileMap();
    if (!map.inBounds(seed.column, seed.row) || (slot && *slot >= draft.layers().size())) {
        return {};
    }
    const CellContent wanted = contentAt(draft, slot, seed);
    const auto width = static_cast<std::size_t>(map.width());
    std::vector<bool> seen(width * static_cast<std::size_t>(map.height()), false);
    const auto indexOf = [width](core::GridPosition cell) {
        return (static_cast<std::size_t>(cell.row) * width) + static_cast<std::size_t>(cell.column);
    };
    std::vector<core::GridPosition> region;
    std::vector<core::GridPosition> pending{seed};
    seen[indexOf(seed)] = true;
    while (!pending.empty()) {
        const core::GridPosition cell = pending.back();
        pending.pop_back();
        region.push_back(cell);
        const std::array<core::GridPosition, 4> neighbours{{
            {.column = cell.column + 1, .row = cell.row},
            {.column = cell.column - 1, .row = cell.row},
            {.column = cell.column, .row = cell.row + 1},
            {.column = cell.column, .row = cell.row - 1},
        }};
        for (const core::GridPosition next : neighbours) {
            if (!map.inBounds(next.column, next.row) || seen[indexOf(next)]) {
                continue;
            }
            seen[indexOf(next)] = true;
            if (contentAt(draft, slot, next) == wanted) {
                pending.push_back(next);
            }
        }
    }
    std::ranges::sort(region, [](core::GridPosition left, core::GridPosition right) {
        return std::tie(left.row, left.column) < std::tie(right.row, right.column);
    });
    return region;
}

LayerSlot brushTargetLayer(const core::LevelDraft& draft, const CanvasBrush& brush,
                           LayerSlot active) {
    if (brush.kind == BrushKind::Piece) {
        return pieceTargetLayer(draft.layers(), brush.floor);
    }
    return active;
}

BrushResult applyBucket(core::LevelDraft& draft, const CanvasBrush& brush, LayerSlot active,
                        const LayerViewState& view, core::GridPosition seed,
                        const StrokeContext& context) {
    if (brush.kind == BrushKind::Piece) {
        const core::PieceFootprint footprint = draft.pieceFootprint(brush.piece);
        if (footprint.columns > 1 || footprint.rows > 1) {
            return BrushResult{.changed = false,
                               .refusal =
                                   "The bucket fills with single-cell pieces; use the "
                                   "rectangle for \"" +
                                   brush.piece + "\"."};
        }
    }
    const LayerSlot target = brushTargetLayer(draft, brush, active);
    if (brush.kind == BrushKind::Piece && !target) {
        // Le pinceau dit pourquoi : pas de couche où la pièce irait.
        return applyBrush(draft, brush, active, view, seed, seed);
    }
    if (view.display(target, hasVisualLayers(draft)).locked) {
        return BrushResult{.changed = false, .refusal = "The target layer is locked."};
    }
    return applyStroke(draft, brush, active, view, floodRegion(draft, target, seed), false,
                       context);
}

std::optional<PickedBrush> pickBrush(const core::LevelDraft& draft, LayerSlot active,
                                     core::GridPosition cell, const PlaceAppearance* appearance) {
    if (!draft.tileMap().inBounds(cell.column, cell.row)) {
        return std::nullopt;
    }
    const std::vector<core::TileLayer>& layers = draft.layers();
    const bool visual = hasVisualLayers(draft);
    if (!active || !visual) {
        return PickedBrush{.brush = CanvasBrush{.kind = BrushKind::Type,
                                                .type = draft.tileMap().tile(cell.column, cell.row),
                                                .piece = {},
                                                .floor = false},
                           .layer = std::nullopt};
    }
    std::vector<std::size_t> order;
    if (*active < layers.size() && core::isVisualLayerKind(layers[*active].kind)) {
        order.push_back(*active);
    }
    for (std::size_t index = layers.size(); index-- > 0;) {
        if (core::isVisualLayerKind(layers[index].kind) && index != *active) {
            order.push_back(index);
        }
    }
    for (const std::size_t index : order) {
        if (const std::optional<CanvasBrush> brush =
                brushFromLayer(draft, index, cell, appearance)) {
            return PickedBrush{.brush = *brush, .layer = index};
        }
    }
    return std::nullopt;
}

CanvasBrush pieceBrush(const PlaceAppearance* appearance, std::string piece, bool floor) {
    const core::TileType type = pieceCellType(appearance, piece, floor);
    return CanvasBrush{
        .kind = BrushKind::Piece, .type = type, .piece = std::move(piece), .floor = floor};
}

Measure measureBetween(core::GridPosition from, core::GridPosition to) noexcept {
    const int columns = std::abs(to.column - from.column);
    const int rows = std::abs(to.row - from.row);
    const int cells = std::max(columns, rows);
    return Measure{.columns = columns, .rows = rows, .cells = cells, .feet = cells * FEET_PER_CELL};
}

std::string measureLabel(const Measure& measure) {
    return std::to_string(measure.columns + 1) + " × " + std::to_string(measure.rows + 1) + " · " +
           std::to_string(measure.cells) + (measure.cells == 1 ? " cell = " : " cells = ") +
           std::to_string(measure.feet) + " ft";
}

}  // namespace hmi
