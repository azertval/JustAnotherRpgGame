// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/BrushGesture.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <utility>

#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileTypeName.h"
#include "Editor/Logic/PieceCatalog.h"

namespace hmi {

namespace {

[[nodiscard]] bool hasVisualLayers(const core::LevelDraft& draft) {
    return std::ranges::any_of(draft.layers(), [](const core::TileLayer& layer) {
        return core::isVisualLayerKind(layer.kind);
    });
}

[[nodiscard]] BrushResult refused(std::string reason) {
    return BrushResult{.changed = false, .refusal = std::move(reason)};
}

[[nodiscard]] BrushResult changedIf(bool changed) {
    return BrushResult{.changed = changed, .refusal = {}};
}

[[nodiscard]] bool locked(const core::LevelDraft& draft, const LayerViewState& view,
                          LayerSlot slot) {
    return view.display(slot, hasVisualLayers(draft)).locked;
}

constexpr const char* LOCKED_LAYER = "The active layer is locked.";

BrushResult applyPiece(core::LevelDraft& draft, const CanvasBrush& brush, LayerSlot active,
                       const LayerViewState& view, core::GridPosition first,
                       core::GridPosition last, bool continuing) {
    const std::optional<std::size_t> layer = pieceTargetLayer(draft.layers(), brush.floor, active);
    if (!layer) {
        return refused(brush.floor ? "This map has no ground layer: add one in the Layers panel."
                                   : "This map has no decor layer: add one in the Layers panel.");
    }
    if (locked(draft, view, *layer)) {
        return refused(brush.floor ? "The ground layer is locked." : "The decor layer is locked.");
    }
    if (first == last) {
        if (continuing) {
            const std::optional<core::GridPosition> anchor = draft.pieceAnchorAt(*layer, first);
            if (anchor &&
                draft.layers()[*layer].pieceAt(anchor->column, anchor->row) == brush.piece) {
                return changedIf(false);
            }
        }
        const bool fits =
            draft.tileMap().inBounds(first.column, first.row) &&
            draft.tileMap().inBounds(first.column + draft.pieceFootprint(brush.piece).columns - 1,
                                     first.row + draft.pieceFootprint(brush.piece).rows - 1);
        if (!fits) {
            return refused("\"" + brush.piece + "\" does not fit here: it would overflow the map.");
        }
        return changedIf(draft.placePiece(*layer, first, brush.piece, brush.type));
    }
    return changedIf(draft.placePieceRegion(*layer, first, last, brush.piece, brush.type));
}

BrushResult applyEraser(core::LevelDraft& draft, LayerSlot active, const LayerViewState& view,
                        core::GridPosition first, core::GridPosition last) {
    if (locked(draft, view, active)) {
        return refused(LOCKED_LAYER);
    }
    if (active) {
        return changedIf(draft.eraseLayerRegion(*active, first, last));
    }
    const int minColumn = std::min(first.column, last.column);
    const int maxColumn = std::max(first.column, last.column);
    const int minRow = std::min(first.row, last.row);
    const int maxRow = std::max(first.row, last.row);
    if (!hasVisualLayers(draft)) {
        // Une grille unique vaut image et collision : la gommer, c'est la vider.
        const std::vector<std::vector<core::TileType>> block(
            static_cast<std::size_t>(maxRow - minRow + 1),
            std::vector<core::TileType>(static_cast<std::size_t>(maxColumn - minColumn + 1),
                                        core::TileType::Empty));
        const std::uint64_t before = draft.revision();
        draft.paintRegion(minColumn, minRow, block);
        return changedIf(draft.revision() != before);
    }
    // Sur la collision d'une carte à couches, la gomme retire le forçage.
    std::vector<core::GridPosition> cells;
    for (int row = minRow; row <= maxRow; ++row) {
        for (int column = minColumn; column <= maxColumn; ++column) {
            cells.push_back({.column = column, .row = row});
        }
    }
    return changedIf(draft.unforceCollision(cells));
}

}  // namespace

BrushResult paintTypeBlock(core::LevelDraft& draft, LayerSlot active, const LayerViewState& view,
                           core::GridPosition origin,
                           const std::vector<std::vector<core::TileType>>& block) {
    if (locked(draft, view, active)) {
        return refused(LOCKED_LAYER);
    }
    const std::uint64_t before = draft.revision();
    if (!active) {
        draft.paintRegion(origin.column, origin.row, block);
        return changedIf(draft.revision() != before);
    }
    for (const std::vector<core::TileType>& row : block) {
        for (const core::TileType type : row) {
            if (!core::isVisualLayerTileType(type)) {
                return refused("\"" + std::string{core::tileTypeName(type)} +
                               "\" cannot be painted on a visual layer: the entry lives in the "
                               "collision grid.");
            }
        }
    }
    const bool single = block.size() == 1 && block.front().size() == 1;
    return changedIf(
        single ? draft.paintLayerTile(*active, origin.column, origin.row, block.front().front())
               : draft.paintLayerRegion(*active, origin.column, origin.row, block));
}

std::string brushLabel(const CanvasBrush& brush) {
    switch (brush.kind) {
        case BrushKind::Piece:
            return brush.piece;
        case BrushKind::Eraser:
            return "Eraser";
        case BrushKind::Type:
            break;
    }
    return std::string{core::tileTypeName(brush.type)};
}

BrushResult applyBrush(core::LevelDraft& draft, const CanvasBrush& brush, LayerSlot active,
                       const LayerViewState& view, core::GridPosition first,
                       core::GridPosition last, bool continuing) {
    switch (brush.kind) {
        case BrushKind::Piece:
            return applyPiece(draft, brush, active, view, first, last, continuing);
        case BrushKind::Eraser:
            return applyEraser(draft, active, view, first, last);
        case BrushKind::Type:
            break;
    }
    const core::GridPosition origin{.column = std::min(first.column, last.column),
                                    .row = std::min(first.row, last.row)};
    const std::vector<std::vector<core::TileType>> block(
        static_cast<std::size_t>(std::abs(last.row - first.row) + 1),
        std::vector<core::TileType>(
            static_cast<std::size_t>(std::abs(last.column - first.column) + 1), brush.type));
    return paintTypeBlock(draft, active, view, origin, block);
}

}  // namespace hmi
