// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Levels/CollisionDerivation.h"

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

#include "Core/Levels/PieceFootprint.h"

namespace core {

namespace {

[[nodiscard]] PieceTactical strongest(PieceTactical left, PieceTactical right) noexcept {
    return static_cast<int>(left) >= static_cast<int>(right) ? left : right;
}

}  // namespace

TileType collisionTileOf(PieceTactical tactical) noexcept {
    switch (tactical) {
        case PieceTactical::Solid:
            return TileType::Wall;
        case PieceTactical::Obstacle:
            return TileType::Cliff;
        case PieceTactical::Open:
        case PieceTactical::Difficult:
        case PieceTactical::Cover:
            return TileType::Empty;
    }
    return TileType::Empty;
}

PieceTactical tacticalOfTileType(TileType type) noexcept {
    switch (type) {
        case TileType::Solid:
        case TileType::Wall:
            return PieceTactical::Solid;
        case TileType::DeepWater:
        case TileType::Cliff:
            return PieceTactical::Obstacle;
        case TileType::Empty:
        case TileType::Entry:
        case TileType::Grass:
        case TileType::Dirt:
        case TileType::Sand:
        case TileType::Water:
        case TileType::Bridge:
        case TileType::Stairs:
            return PieceTactical::Open;
    }
    return PieceTactical::Open;
}

namespace {

// Ce que les couches apportent, case par case : la contribution la plus forte, et les cases dont
// la pièce est inconnue.
class Contributions {
public:
    Contributions(int width, int height, const ScenePieceManifest* manifest)
        : _width(width),
          _height(height),
          _manifest(manifest),
          _cells(static_cast<std::size_t>(width) * static_cast<std::size_t>(height)),
          _unknown(_cells.size(), false),
          _bounds(width, height) {}

    [[nodiscard]] std::size_t indexOf(int column, int row) const {
        return (static_cast<std::size_t>(row) * static_cast<std::size_t>(_width)) +
               static_cast<std::size_t>(column);
    }

    // Ajoute ce que la couche @p layer apporte, si elle est visuelle et de la taille de la carte.
    void addLayer(const TileLayer& layer) {
        if (!isVisualLayerKind(layer.kind) || layer.tiles.width() != _width ||
            layer.tiles.height() != _height) {
            return;
        }
        for (int row = 0; row < _height; ++row) {
            for (int column = 0; column < _width; ++column) {
                addCell(layer, column, row);
            }
        }
    }

    [[nodiscard]] PieceTactical tacticalAt(std::size_t index) const {
        return _cells[index].value_or(PieceTactical::Solid);
    }

    [[nodiscard]] bool isUnknown(std::size_t index) const {
        return _unknown[index];
    }

private:
    void contribute(int column, int row, PieceTactical tactical) {
        if (!_bounds.inBounds(column, row)) {
            return;  // une emprise qui déborde de la carte n'y occupe que ce qui y tient
        }
        std::optional<PieceTactical>& cell = _cells[indexOf(column, row)];
        cell = cell ? strongest(*cell, tactical) : tactical;
    }

    // Ce que la case (column, row) apporte : l'emprise de sa pièce si elle est connue, sinon son
    // type ; une pièce inconnue est relevée et couvre au moins « ouvert ».
    void addCell(const TileLayer& layer, int column, int row) {
        const std::string_view name = layer.pieceAt(column, row);
        const ScenePiece* piece =
            name.empty() || _manifest == nullptr ? nullptr : _manifest->find(name);
        if (piece != nullptr) {
            for (const GridPosition cell :
                 footprintCells(GridPosition{.column = column, .row = row}, piece->footprint())) {
                contribute(cell.column, cell.row, piece->tactical);
            }
            return;
        }
        if (!name.empty()) {
            _unknown[indexOf(column, row)] = true;
        }
        const TileType type = layer.tiles.tile(column, row);
        if (type != TileType::Empty) {
            contribute(column, row, tacticalOfTileType(type));
        } else if (!name.empty()) {
            contribute(column, row, PieceTactical::Open);  // une pièce inconnue couvre
        }
    }

    int _width;
    int _height;
    const ScenePieceManifest* _manifest;
    // std::nullopt tant que rien ne couvre la case (vide).
    std::vector<std::optional<PieceTactical>> _cells;
    std::vector<bool> _unknown;
    TileMap _bounds;
};

}  // namespace

CollisionDerivation deriveCollision(const std::vector<TileLayer>& layers, int width, int height,
                                    const ScenePieceManifest* manifest) {
    Contributions contributions(width, height, manifest);
    for (const TileLayer& layer : layers) {
        contributions.addLayer(layer);
    }

    CollisionDerivation derivation{.collision = TileMap(width, height)};
    for (int row = 0; row < height; ++row) {
        for (int column = 0; column < width; ++column) {
            const std::size_t index = contributions.indexOf(column, row);
            const PieceTactical tactical = contributions.tacticalAt(index);
            derivation.collision.setTile(column, row, collisionTileOf(tactical));
            const GridPosition cell{.column = column, .row = row};
            if (tactical == PieceTactical::Difficult || tactical == PieceTactical::Cover) {
                derivation.unplayed.push_back(cell);
            }
            if (contributions.isUnknown(index)) {
                derivation.unknownPieces.push_back(cell);
            }
        }
    }
    return derivation;
}

TileType canonicalCollisionTile(TileType type) noexcept {
    return collisionTileOf(tacticalOfTileType(type));
}

bool collisionAgrees(const TileMap& written, const TileMap& derived, int column, int row) {
    return canonicalCollisionTile(written.tile(column, row)) ==
           canonicalCollisionTile(derived.tile(column, row));
}

}  // namespace core
