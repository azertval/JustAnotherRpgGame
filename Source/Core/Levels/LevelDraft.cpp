// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Levels/LevelDraft.h"

#include <algorithm>
#include <utility>
#include <variant>

#include "Core/Levels/CollisionDerivation.h"
#include "Core/Levels/LevelVariant.h"
#include "Core/Levels/LevelWriter.h"

namespace core {

namespace {

// Copie @p source dans une grille de @p width x @p height, tronquee aux bords.
[[nodiscard]] TileMap resizedCopy(const TileMap& source, int width, int height) {
    TileMap resized(width, height);
    const int copyWidth = (std::min)(width, source.width());
    const int copyHeight = (std::min)(height, source.height());
    for (int row = 0; row < copyHeight; ++row) {
        for (int column = 0; column < copyWidth; ++column) {
            resized.setTile(column, row, source.tile(column, row));
        }
    }
    return resized;
}

}  // namespace

LevelDraft::LevelDraft(std::string name, TileMap tileMap)
    : _name(std::move(name)), _tileMap(std::move(tileMap)) {}

LevelDraft LevelDraft::empty(std::string name, int width, int height) {
    return {std::move(name), TileMap(width, height)};
}

LevelDraft LevelDraft::fromLevel(const Level& level) {
    LevelDraft draft(level.name(), level.tileMap());
    // INVARIANT DE CE CONSTRUCTEUR : fromLevel recopie TOUS les champs de Level, sans exception.
    // Un brouillon reenregistre doit etre equivalent a la carte d'origine -- un champ non recopie
    // ne fait echouer aucun appel, il s'efface simplement au premier enregistrement. Ajouter un
    // champ a Level, c'est l'ajouter ici.
    draft._entry = level.entry();
    draft._layers = level.layers();
    draft._entities = level.entities();
    draft._forcedCollision = level.forcedCollision();
    draft._nextEntityId = level.nextEntityId();
    draft._base = level.base();
    draft._scene = level.scene();
    draft._properties = level.properties();
    return draft;
}

bool LevelDraft::paintChanges(int column, int row, TileType type) const {
    if (!_tileMap.inBounds(column, row)) {
        return false;
    }
    if (type == TileType::Entry) {
        return _entry != GridPosition{.column = column, .row = row};
    }
    return _tileMap.tile(column, row) != type;
}

void LevelDraft::paintTile(int column, int row, TileType type) {
    // Repasser le pinceau sur une case déjà du bon type ne fait rien : ni pas d'historique, ni
    // révision neuve -- la carte n'en est pas « modifiée ».
    if (!paintChanges(column, row, type)) {
        return;
    }
    pushUndo();
    paintTileInternal(column, row, type);
    updateForcing({GridPosition{.column = column, .row = row}});
}

void LevelDraft::paintRegion(int originColumn, int originRow,
                             const std::vector<std::vector<TileType>>& block) {
    bool changes = false;
    for (std::size_t rowOffset = 0; rowOffset < block.size() && !changes; ++rowOffset) {
        for (std::size_t columnOffset = 0; columnOffset < block[rowOffset].size(); ++columnOffset) {
            if (paintChanges(originColumn + static_cast<int>(columnOffset),
                             originRow + static_cast<int>(rowOffset),
                             block[rowOffset][columnOffset])) {
                changes = true;
                break;
            }
        }
    }
    if (!changes) {
        return;
    }
    pushUndo();
    std::vector<GridPosition> painted;
    for (std::size_t rowOffset = 0; rowOffset < block.size(); ++rowOffset) {
        const std::vector<TileType>& rowTiles = block[rowOffset];
        for (std::size_t columnOffset = 0; columnOffset < rowTiles.size(); ++columnOffset) {
            const int column = originColumn + static_cast<int>(columnOffset);
            const int row = originRow + static_cast<int>(rowOffset);
            if (!_tileMap.inBounds(column, row)) {
                continue;  // decoupe silencieuse aux bords, meme principe que resize()
            }
            paintTileInternal(column, row, rowTiles[columnOffset]);
            painted.push_back({.column = column, .row = row});
        }
    }
    updateForcing(painted);
}

void LevelDraft::paintTileInternal(int column, int row, TileType type) {
    if (type == TileType::Entry) {
        setEntryInternal(column, row);
        return;
    }

    const GridPosition position{.column = column, .row = row};
    if (_entry && *_entry == position) {
        _entry.reset();
    }
    _tileMap.setTile(column, row, type);
}

void LevelDraft::setEntry(int column, int row) {
    pushUndo();
    setEntryInternal(column, row);
}

void LevelDraft::setEntryInternal(int column, int row) {
    const GridPosition position{.column = column, .row = row};
    const std::optional<GridPosition> previous = _entry;
    if (previous && *previous != position) {
        _tileMap.setTile(previous->column, previous->row, TileType::Empty);
    }
    _tileMap.setTile(column, row, TileType::Entry);
    _entry = position;
    if (previous && *previous != position) {
        // La case quittée par l'entrée reprend ce que ses couches lui donnent.
        followCollision({*previous});
    }
}

// --- Couches de tuiles (LOT-11) ---
//
// Les validations d'abord, le pushUndo() ensuite : jamais un pas d'historique pour un geste refuse
// ou sans effet.

namespace {

// Vrai si @p layers porte au moins une couche visuelle.
[[nodiscard]] bool hasVisualLayer(const std::vector<TileLayer>& layers) {
    return std::ranges::any_of(
        layers, [](const TileLayer& layer) { return isVisualLayerKind(layer.kind); });
}

// Aligne l'entree de la grille racine sur ce que ferait le chargeur a la relecture : `Collision` en
// tete des qu'une couche visuelle existe, `Legacy` sinon. Sans cet alignement, le brouillon
// divergerait du niveau qu'il produit -- et l'editeur afficherait l'une quand le jeu lirait
// l'autre.
void alignRootLayer(std::vector<TileLayer>& layers, const TileMap& root) {
    const auto rootEntry = std::ranges::find_if(layers, [](const TileLayer& layer) {
        return layer.kind == LayerKind::Collision || layer.kind == LayerKind::Legacy;
    });
    if (!hasVisualLayer(layers)) {
        if (rootEntry != layers.end()) {
            rootEntry->kind = LayerKind::Legacy;
        }
        return;
    }
    if (rootEntry != layers.end()) {
        rootEntry->kind = LayerKind::Collision;
        return;
    }
    layers.insert(
        layers.begin(),
        TileLayer{.name = {}, .kind = LayerKind::Collision, .tiles = root, .properties = {}});
}

// Peint @p type sur une case de couche. Un type DIFFERENT retire la piece de la case : elle
// habillait l'ancien type, et la garder montrerait un mur sur une rue fraichement peinte. Reposer
// le meme type la garde -- un coup de pinceau involontaire ne l'efface pas.
void setLayerCellType(TileLayer& layer, int column, int row, TileType type) {
    if (layer.tiles.tile(column, row) != type) {
        layer.setPiece(column, row, {});
    }
    layer.tiles.setTile(column, row, type);
}

// Vrai si chaque type de @p block se peint sur une couche visuelle.
[[nodiscard]] bool blockIsVisual(const std::vector<std::vector<TileType>>& block) {
    return std::ranges::all_of(block, [](const std::vector<TileType>& row) {
        return std::ranges::all_of(row, isVisualLayerTileType);
    });
}

}  // namespace

bool LevelDraft::isVisualLayerIndex(std::size_t index) const noexcept {
    return index < _layers.size() && isVisualLayerKind(_layers[index].kind);
}

std::optional<std::size_t> LevelDraft::addLayer(LayerKind kind, std::string name) {
    if (!isVisualLayerKind(kind)) {
        return std::nullopt;
    }
    pushUndo();
    TileMap tiles(_tileMap.width(), _tileMap.height());
    if (!hasVisualLayer(_layers)) {
        // Promotion (voir l'en-tete) : la premiere couche visuelle reprend l'image de la grille
        // unique, sans les types qui n'ont de sens qu'en collision.
        for (int row = 0; row < _tileMap.height(); ++row) {
            for (int column = 0; column < _tileMap.width(); ++column) {
                const TileType type = _tileMap.tile(column, row);
                if (isVisualLayerTileType(type)) {
                    tiles.setTile(column, row, type);
                }
            }
        }
    }
    _layers.push_back(TileLayer{
        .name = std::move(name), .kind = kind, .tiles = std::move(tiles), .properties = {}});
    alignRootLayer(_layers, _tileMap);
    // L'alignement peut avoir insere l'entree de collision en tete : la couche creee est toujours
    // la derniere.
    return _layers.size() - 1;
}

bool LevelDraft::removeLayer(std::size_t index) {
    if (!isVisualLayerIndex(index)) {
        return false;
    }
    pushUndo();
    // Ce que la couche couvrait change de collision : ses cases peintes, emprises comprises.
    std::vector<GridPosition> covered;
    const TileLayer& removed = _layers[index];
    for (int row = 0; row < removed.tiles.height(); ++row) {
        for (int column = 0; column < removed.tiles.width(); ++column) {
            const std::string_view piece = removed.pieceAt(column, row);
            if (removed.tiles.tile(column, row) == TileType::Empty && piece.empty()) {
                continue;
            }
            const std::vector<GridPosition> cells =
                footprintCells({.column = column, .row = row}, pieceFootprint(piece));
            covered.insert(covered.end(), cells.begin(), cells.end());
        }
    }
    _layers.erase(_layers.begin() + static_cast<std::ptrdiff_t>(index));
    alignRootLayer(_layers, _tileMap);
    followCollision(covered);
    return true;
}

bool LevelDraft::renameLayer(std::size_t index, std::string name) {
    if (!isVisualLayerIndex(index) || _layers[index].name == name) {
        return false;
    }
    pushUndo();
    _layers[index].name = std::move(name);
    return true;
}

bool LevelDraft::setLayerKind(std::size_t index, LayerKind kind) {
    if (!isVisualLayerIndex(index) || !isVisualLayerKind(kind) || _layers[index].kind == kind) {
        return false;
    }
    pushUndo();
    _layers[index].kind = kind;
    return true;
}

bool LevelDraft::setLayerFloor(std::size_t index, int floor) {
    if (!isVisualLayerIndex(index) || _layers[index].kind != LayerKind::Decor || floor < 0 ||
        floor > MAX_STOREY_FLOOR || _layers[index].floor == floor) {
        return false;
    }
    pushUndo();
    _layers[index].floor = floor;
    // La couche entre dans la collision, ou en sort : toutes ses cases se redéduisent.
    std::vector<GridPosition> cells;
    cells.reserve(static_cast<std::size_t>(_tileMap.width()) *
                  static_cast<std::size_t>(_tileMap.height()));
    for (int row = 0; row < _tileMap.height(); ++row) {
        for (int column = 0; column < _tileMap.width(); ++column) {
            cells.push_back({.column = column, .row = row});
        }
    }
    followCollision(cells);
    return true;
}

bool LevelDraft::setLayerProperty(std::size_t index, const std::string& key, PropertyValue value) {
    if (!isVisualLayerIndex(index) || key.empty()) {
        return false;
    }
    const PropertyMap& properties = _layers[index].properties;
    if (const auto found = properties.find(key);
        found != properties.end() && found->second == value) {
        return false;
    }
    pushUndo();
    _layers[index].properties[key] = std::move(value);
    return true;
}

std::optional<std::size_t> LevelDraft::moveLayer(std::size_t index, bool forward) {
    if (!isVisualLayerIndex(index)) {
        return std::nullopt;
    }
    const bool atEnd = forward ? index + 1 >= _layers.size() : index == 0;
    if (atEnd) {
        return index;
    }
    const std::size_t neighbour = forward ? index + 1 : index - 1;
    if (!isVisualLayerIndex(neighbour)) {
        return index;  // l'entree de collision ne se franchit pas.
    }
    pushUndo();
    std::swap(_layers[index], _layers[neighbour]);
    return neighbour;
}

bool LevelDraft::paintLayerTile(std::size_t index, int column, int row, TileType type) {
    if (!isVisualLayerIndex(index) || !isVisualLayerTileType(type)) {
        return false;
    }
    TileMap& tiles = _layers[index].tiles;
    if (!tiles.inBounds(column, row) || tiles.tile(column, row) == type) {
        return false;
    }
    pushUndo();
    std::vector<GridPosition> touched{{.column = column, .row = row}};
    if (const std::optional<GridPosition> anchor =
            pieceAnchorAt(index, GridPosition{.column = column, .row = row})) {
        // Un autre type retire la pièce : toute son emprise change de collision.
        const std::vector<GridPosition> covered = footprintCells(
            *anchor, pieceFootprint(_layers[index].pieceAt(anchor->column, anchor->row)));
        touched.insert(touched.end(), covered.begin(), covered.end());
    }
    setLayerCellType(_layers[index], column, row, type);
    followCollision(touched);
    return true;
}

bool LevelDraft::paintLayerRegion(std::size_t index, int originColumn, int originRow,
                                  const std::vector<std::vector<TileType>>& block) {
    if (!isVisualLayerIndex(index) || block.empty() || !blockIsVisual(block)) {
        return false;
    }
    pushUndo();
    TileMap& tiles = _layers[index].tiles;
    std::vector<GridPosition> touched;
    for (std::size_t rowOffset = 0; rowOffset < block.size(); ++rowOffset) {
        const std::vector<TileType>& rowTiles = block[rowOffset];
        for (std::size_t columnOffset = 0; columnOffset < rowTiles.size(); ++columnOffset) {
            const int column = originColumn + static_cast<int>(columnOffset);
            const int row = originRow + static_cast<int>(rowOffset);
            if (!tiles.inBounds(column, row)) {
                continue;
            }
            const std::string_view piece = _layers[index].pieceAt(column, row);
            const std::vector<GridPosition> covered =
                footprintCells({.column = column, .row = row}, pieceFootprint(piece));
            touched.insert(touched.end(), covered.begin(), covered.end());
            setLayerCellType(_layers[index], column, row, rowTiles[columnOffset]);
        }
    }
    followCollision(touched);
    return true;
}

// --- Pieces de couche (LOT-EDITOR-03) ---

namespace {

/// Plus grande emprise qu'on cherche autour d'une case : au-dela, une piece couvrant la case
/// depuis son ancre serait plus grande que tout ce que les planches livrent.
constexpr int MAX_FOOTPRINT_SEARCH = 8;

[[nodiscard]] bool byRow(GridPosition left, GridPosition right) noexcept {
    return left.row != right.row ? left.row < right.row : left.column < right.column;
}

}  // namespace

PieceFootprint LevelDraft::pieceFootprint(std::string_view piece) const noexcept {
    const ScenePiece* const found =
        piece.empty() || _manifest == nullptr ? nullptr : _manifest->find(piece);
    return found != nullptr ? found->footprint() : PieceFootprint{};
}

std::optional<GridPosition> LevelDraft::pieceAnchorAt(std::size_t index, GridPosition cell) const {
    if (!isVisualLayerIndex(index)) {
        return std::nullopt;
    }
    const TileLayer& layer = _layers[index];
    if (!layer.tiles.inBounds(cell.column, cell.row)) {
        return std::nullopt;
    }
    if (!layer.pieceAt(cell.column, cell.row).empty()) {
        return cell;
    }
    // Une piece large couvre la case depuis une ancre en haut a gauche d'elle : la plus proche
    // d'abord.
    for (int rowOffset = 0; rowOffset < MAX_FOOTPRINT_SEARCH; ++rowOffset) {
        for (int columnOffset = 0; columnOffset < MAX_FOOTPRINT_SEARCH; ++columnOffset) {
            if (rowOffset == 0 && columnOffset == 0) {
                continue;
            }
            const GridPosition anchor{.column = cell.column - columnOffset,
                                      .row = cell.row - rowOffset};
            const std::string_view piece = layer.pieceAt(anchor.column, anchor.row);
            if (piece.empty()) {
                continue;
            }
            const PieceFootprint footprint = pieceFootprint(piece);
            if (columnOffset < footprint.columns && rowOffset < footprint.rows) {
                return anchor;
            }
        }
    }
    return std::nullopt;
}

bool LevelDraft::pieceFits(GridPosition anchor, std::string_view piece) const noexcept {
    const PieceFootprint footprint = pieceFootprint(piece);
    return _tileMap.inBounds(anchor.column, anchor.row) &&
           _tileMap.inBounds(anchor.column + footprint.columns - 1,
                             anchor.row + footprint.rows - 1);
}

void LevelDraft::removePieceInternal(std::size_t index, GridPosition anchor,
                                     std::vector<GridPosition>& touched) {
    TileLayer& layer = _layers[index];
    const std::vector<GridPosition> covered =
        footprintCells(anchor, pieceFootprint(layer.pieceAt(anchor.column, anchor.row)));
    touched.insert(touched.end(), covered.begin(), covered.end());
    layer.setPiece(anchor.column, anchor.row, {});
    layer.tiles.setTile(anchor.column, anchor.row, TileType::Empty);
}

void LevelDraft::placePieceInternal(std::size_t index, GridPosition anchor,
                                    const std::string& piece, TileType type,
                                    std::vector<GridPosition>& touched) {
    const std::vector<GridPosition> covered = footprintCells(anchor, pieceFootprint(piece));
    // Deux emprises ne se recouvrent pas sur une couche : ce que la piece couvrirait part, piece
    // entiere, meme si son ancre est hors de l'emprise nouvelle.
    for (const GridPosition cell : covered) {
        while (const std::optional<GridPosition> other = pieceAnchorAt(index, cell)) {
            removePieceInternal(index, *other, touched);
        }
    }
    TileLayer& layer = _layers[index];
    // Les autres cases de l'emprise sont occupees par la piece : un type qu'on y laisserait
    // dessinerait sa piece par defaut sous elle.
    for (const GridPosition cell : covered) {
        layer.tiles.setTile(cell.column, cell.row, TileType::Empty);
    }
    layer.setPiece(anchor.column, anchor.row, piece);
    layer.tiles.setTile(anchor.column, anchor.row, type);
    touched.insert(touched.end(), covered.begin(), covered.end());
}

bool LevelDraft::placePiece(std::size_t index, GridPosition anchor, const std::string& piece,
                            TileType type) {
    if (!isVisualLayerIndex(index) || piece.empty() || !isVisualLayerTileType(type) ||
        !pieceFits(anchor, piece)) {
        return false;
    }
    const TileLayer& layer = _layers[index];
    if (layer.pieceAt(anchor.column, anchor.row) == piece &&
        layer.tiles.tile(anchor.column, anchor.row) == type) {
        return false;  // reposer la meme piece ne modifie pas la carte (EX-EDIT-058)
    }
    pushUndo();
    std::vector<GridPosition> touched;
    placePieceInternal(index, anchor, piece, type, touched);
    followCollision(touched);
    return true;
}

bool LevelDraft::placePieceRegion(std::size_t index, GridPosition first, GridPosition last,
                                  const std::string& piece, TileType type) {
    if (!isVisualLayerIndex(index) || piece.empty() || !isVisualLayerTileType(type)) {
        return false;
    }
    const PieceFootprint footprint = pieceFootprint(piece);
    const int minColumn = (std::max)(0, (std::min)(first.column, last.column));
    const int maxColumn = (std::min)(_tileMap.width() - 1, (std::max)(first.column, last.column));
    const int minRow = (std::max)(0, (std::min)(first.row, last.row));
    const int maxRow = (std::min)(_tileMap.height() - 1, (std::max)(first.row, last.row));
    std::vector<GridPosition> anchors;
    for (int row = minRow; row + footprint.rows - 1 <= maxRow; row += footprint.rows) {
        for (int column = minColumn; column + footprint.columns - 1 <= maxColumn;
             column += footprint.columns) {
            anchors.push_back({.column = column, .row = row});
        }
    }
    // Un pavage deja en place n'empile rien : chaque ancre porte la piece et son type, et le reste
    // de chaque emprise est libre.
    const TileLayer& layer = _layers[index];
    const bool unchanged = std::ranges::all_of(anchors, [&](GridPosition anchor) {
        if (layer.pieceAt(anchor.column, anchor.row) != piece ||
            layer.tiles.tile(anchor.column, anchor.row) != type) {
            return false;
        }
        return std::ranges::all_of(footprintCells(anchor, footprint), [&](GridPosition cell) {
            return cell == anchor || (layer.pieceAt(cell.column, cell.row).empty() &&
                                      layer.tiles.tile(cell.column, cell.row) == TileType::Empty);
        });
    });
    if (anchors.empty() || unchanged) {
        return false;
    }
    pushUndo();
    std::vector<GridPosition> touched;
    for (const GridPosition anchor : anchors) {
        placePieceInternal(index, anchor, piece, type, touched);
    }
    followCollision(touched);
    return true;
}

bool LevelDraft::eraseLayerRegion(std::size_t index, GridPosition first, GridPosition last) {
    if (!isVisualLayerIndex(index)) {
        return false;
    }
    const int minColumn = (std::max)(0, (std::min)(first.column, last.column));
    const int maxColumn = (std::min)(_tileMap.width() - 1, (std::max)(first.column, last.column));
    const int minRow = (std::max)(0, (std::min)(first.row, last.row));
    const int maxRow = (std::min)(_tileMap.height() - 1, (std::max)(first.row, last.row));
    const auto holdsSomething = [this, index](GridPosition cell) {
        return pieceAnchorAt(index, cell).has_value() ||
               _layers[index].tiles.tile(cell.column, cell.row) != TileType::Empty;
    };
    bool changes = false;
    for (int row = minRow; row <= maxRow && !changes; ++row) {
        for (int column = minColumn; column <= maxColumn && !changes; ++column) {
            changes = holdsSomething({.column = column, .row = row});
        }
    }
    if (!changes) {
        return false;
    }
    pushUndo();
    std::vector<GridPosition> touched;
    for (int row = minRow; row <= maxRow; ++row) {
        for (int column = minColumn; column <= maxColumn; ++column) {
            const GridPosition cell{.column = column, .row = row};
            if (const std::optional<GridPosition> anchor = pieceAnchorAt(index, cell)) {
                removePieceInternal(index, *anchor, touched);
            } else if (_layers[index].tiles.tile(column, row) != TileType::Empty) {
                _layers[index].tiles.setTile(column, row, TileType::Empty);
                touched.push_back(cell);
            }
        }
    }
    followCollision(touched);
    return true;
}

bool LevelDraft::isCollisionForced(GridPosition cell) const noexcept {
    return std::ranges::binary_search(_forcedCollision, cell, byRow);
}

bool LevelDraft::unforceCollision(const std::vector<GridPosition>& cells) {
    std::vector<GridPosition> released;
    for (const GridPosition cell : cells) {
        if (isCollisionForced(cell)) {
            released.push_back(cell);
        }
    }
    if (released.empty()) {
        return false;
    }
    pushUndo();
    std::erase_if(_forcedCollision, [&released](GridPosition cell) {
        return std::ranges::find(released, cell) != released.end();
    });
    followCollision(released);
    return true;
}

// --- Remplacer, changer de planche (LOT-EDITOR-14) ---

namespace {

// L'emprise de @p piece selon @p manifest, 1 x 1 si elle y est inconnue.
[[nodiscard]] PieceFootprint footprintIn(const ScenePieceManifest* manifest,
                                         std::string_view piece) noexcept {
    const ScenePiece* const found =
        piece.empty() || manifest == nullptr ? nullptr : manifest->find(piece);
    return found != nullptr ? found->footprint() : PieceFootprint{};
}

// Renomme dans @p layers les pieces que @p renaming nomme ; @p touched recoit les cases des
// anciennes emprises (lues par @p before) et des nouvelles (lues par @p after). Faux si une
// nouvelle emprise deborde de la carte : @p layers est alors a jeter.
[[nodiscard]] bool renamePieces(std::vector<TileLayer>& layers, const PieceRenaming& renaming,
                                const ScenePieceManifest* before, const ScenePieceManifest* after,
                                std::vector<GridPosition>& touched, std::size_t& renamed) {
    for (TileLayer& layer : layers) {
        if (!isVisualLayerKind(layer.kind) || !layer.hasPieces()) {
            continue;
        }
        for (int row = 0; row < layer.tiles.height(); ++row) {
            for (int column = 0; column < layer.tiles.width(); ++column) {
                const auto found = renaming.find(layer.pieceAt(column, row));
                if (found == renaming.end() || found->second == found->first) {
                    continue;
                }
                const GridPosition anchor{.column = column, .row = row};
                const PieceFootprint next = footprintIn(after, found->second);
                if (!layer.tiles.inBounds(column + next.columns - 1, row + next.rows - 1)) {
                    return false;
                }
                const std::vector<GridPosition> old =
                    footprintCells(anchor, footprintIn(before, found->first));
                const std::vector<GridPosition> covered = footprintCells(anchor, next);
                touched.insert(touched.end(), old.begin(), old.end());
                touched.insert(touched.end(), covered.begin(), covered.end());
                layer.setPiece(column, row, found->second);
                ++renamed;
            }
        }
    }
    return true;
}

}  // namespace

bool LevelDraft::replacePieces(const PieceRenaming& renaming) {
    std::vector<TileLayer> layers = _layers;
    std::vector<GridPosition> touched;
    std::size_t renamed = 0;
    if (!renamePieces(layers, renaming, _manifest.get(), _manifest.get(), touched, renamed) ||
        renamed == 0) {
        return false;
    }
    pushUndo();
    _layers = std::move(layers);
    followCollision(touched);
    return true;
}

bool LevelDraft::changeScene(const std::string& place,
                             std::shared_ptr<const ScenePieceManifest> manifest,
                             const PieceRenaming& renaming) {
    if (!derivesCollision() || place.empty()) {
        return false;
    }
    std::vector<TileLayer> layers = _layers;
    std::vector<GridPosition> touched;
    std::size_t renamed = 0;
    if (!renamePieces(layers, renaming, _manifest.get(), manifest.get(), touched, renamed)) {
        return false;
    }
    // La planche se dit sur les couches qui la nomment ; a defaut, sur la premiere couche de sol.
    const std::string key{SCENE_LAYER_PROPERTY};
    bool named = false;
    bool changed = renamed > 0;
    for (TileLayer& layer : layers) {
        const auto found = layer.properties.find(key);
        if (found == layer.properties.end()) {
            continue;
        }
        named = true;
        if (found->second != PropertyValue{place}) {
            found->second = place;
            changed = true;
        }
    }
    if (!named) {
        auto target = std::ranges::find_if(
            layers, [](const TileLayer& layer) { return layer.kind == LayerKind::Ground; });
        if (target == layers.end()) {
            // derivesCollision() : la carte a une couche visuelle, donc un decor.
            target = std::ranges::find_if(
                layers, [](const TileLayer& layer) { return isVisualLayerKind(layer.kind); });
        }
        target->properties[key] = place;
        changed = true;
    }
    if (!changed) {
        return false;
    }
    pushUndo();
    _layers = std::move(layers);
    _manifest = std::move(manifest);
    // Une piece qui garde son nom peut changer d'emprise ou de type tactique : tout se rededuit.
    std::vector<GridPosition> all;
    all.reserve(static_cast<std::size_t>(_tileMap.width()) *
                static_cast<std::size_t>(_tileMap.height()));
    for (int row = 0; row < _tileMap.height(); ++row) {
        for (int column = 0; column < _tileMap.width(); ++column) {
            all.push_back({.column = column, .row = row});
        }
    }
    followCollision(all);
    return true;
}

bool LevelDraft::derivesCollision() const noexcept {
    return hasVisualLayer(_layers);
}

void LevelDraft::followCollision(const std::vector<GridPosition>& cells) {
    if (cells.empty() || !derivesCollision()) {
        return;
    }
    const CollisionDerivation derived =
        deriveCollision(_layers, _tileMap.width(), _tileMap.height(), _manifest.get());
    for (const GridPosition cell : cells) {
        if (!_tileMap.inBounds(cell.column, cell.row) || isCollisionForced(cell) ||
            _entry == cell) {
            continue;
        }
        // Une case qui s'accorde deja garde son ecriture : un geste qui ne change rien a ce qu'elle
        // oppose ne change pas le fichier.
        if (!collisionAgrees(_tileMap, derived.collision, cell.column, cell.row)) {
            _tileMap.setTile(cell.column, cell.row, derived.collision.tile(cell.column, cell.row));
        }
    }
}

void LevelDraft::updateForcing(const std::vector<GridPosition>& cells) {
    if (cells.empty() || !derivesCollision()) {
        return;
    }
    const CollisionDerivation derived =
        deriveCollision(_layers, _tileMap.width(), _tileMap.height(), _manifest.get());
    for (const GridPosition cell : cells) {
        if (_entry == cell) {
            continue;  // l'entree est un repere, pas une collision : elle ne se force pas.
        }
        const bool forced = isCollisionForced(cell);
        const bool agrees = collisionAgrees(_tileMap, derived.collision, cell.column, cell.row);
        if (agrees && forced) {
            std::erase(_forcedCollision, cell);
        } else if (!agrees && !forced) {
            _forcedCollision.insert(std::ranges::upper_bound(_forcedCollision, cell, byRow), cell);
        }
    }
}

// --- Entites de carte (LOT-11) ---

std::optional<std::size_t> LevelDraft::placeEntity(MapEntity entity) {
    if (!_tileMap.inBounds(entity.position.column, entity.position.row)) {
        return std::nullopt;
    }
    pushUndo();
    if (entity.id.empty()) {
        // Le compteur ne recule jamais, pas meme a l'annulation : un identifiant donne une fois ne
        // designera jamais une autre entite (decision D8).
        entity.id = entityIdFor(_nextEntityId++);
    }
    _entities.push_back(std::move(entity));
    return _entities.size() - 1;
}

bool LevelDraft::moveEntity(std::size_t index, GridPosition position) {
    if (!isEntityIndex(index) || !_tileMap.inBounds(position.column, position.row) ||
        _entities[index].position == position) {
        return false;
    }
    pushUndo();
    _entities[index].position = position;
    return true;
}

bool LevelDraft::replaceEntity(std::size_t index, MapEntity entity) {
    if (!isEntityIndex(index)) {
        return false;
    }
    const auto inGrid = [this](GridPosition cell) {
        return _tileMap.inBounds(cell.column, cell.row);
    };
    if (!inGrid(entity.position) || !std::ranges::all_of(entity.cells, inGrid)) {
        return false;
    }
    const MapEntity& current = _entities[index];
    entity.type = current.type;
    entity.id = current.id;
    if (entity.position == current.position && entity.properties == current.properties &&
        entity.cells == current.cells && entity.elevation == current.elevation) {
        return false;
    }
    pushUndo();
    _entities[index] = std::move(entity);
    return true;
}

bool LevelDraft::removeEntity(std::size_t index) {
    if (!isEntityIndex(index)) {
        return false;
    }
    pushUndo();
    _entities.erase(_entities.begin() + static_cast<std::ptrdiff_t>(index));
    return true;
}

bool LevelDraft::setEntityProperty(std::size_t index, const std::string& key, PropertyValue value) {
    if (!isEntityIndex(index) || key.empty()) {
        return false;
    }
    const PropertyMap& properties = _entities[index].properties;
    if (const auto found = properties.find(key);
        found != properties.end() && found->second == value) {
        return false;
    }
    pushUndo();
    _entities[index].properties[key] = std::move(value);
    return true;
}

bool LevelDraft::removeEntityProperty(std::size_t index, const std::string& key) {
    if (!isEntityIndex(index) || !_entities[index].properties.contains(key)) {
        return false;
    }
    pushUndo();
    _entities[index].properties.erase(key);
    return true;
}

std::optional<std::size_t> LevelDraft::entityAt(GridPosition position) const {
    for (std::size_t index = _entities.size(); index > 0; --index) {
        if (_entities[index - 1].position == position) {
            return index - 1;
        }
    }
    return std::nullopt;
}

void LevelDraft::resize(int width, int height) {
    pushUndo();
    const int previousWidth = _tileMap.width();
    const int previousHeight = _tileMap.height();
    _tileMap = resizedCopy(_tileMap, width, height);
    // Les couches suivent la grille racine (LOT-04) : toutes les couches d'une carte partagent ses
    // dimensions, c'est ce que le chargeur verifie a la relecture.
    for (TileLayer& layer : _layers) {
        layer = resizedLayer(layer, width, height);
    }

    if (_entry && !_tileMap.inBounds(_entry->column, _entry->row)) {
        _entry.reset();
    }
    std::erase_if(_forcedCollision,
                  [this](GridPosition cell) { return !_tileMap.inBounds(cell.column, cell.row); });
    // Une entite est keyee par sa case : hors de la nouvelle grille, elle n'a plus de place ou
    // exister, et la garder rendrait la carte irrecuperable (EX-LVL-017).
    std::erase_if(_entities, [this](const MapEntity& entity) {
        return !_tileMap.inBounds(entity.position.column, entity.position.row);
    });
    for (MapEntity& entity : _entities) {
        std::erase_if(entity.cells, [this](GridPosition cell) {
            return !_tileMap.inBounds(cell.column, cell.row);
        });
    }
    // Les cases gagnées n'ont rien sous elles : leur collision est celle du vide.
    std::vector<GridPosition> gained;
    for (int row = 0; row < height; ++row) {
        for (int column = 0; column < width; ++column) {
            if (column >= previousWidth || row >= previousHeight) {
                gained.push_back({.column = column, .row = row});
            }
        }
    }
    followCollision(gained);
}

bool LevelDraft::wouldResizeDropContent(int width, int height) const noexcept {
    const auto outOfBounds = [width, height](GridPosition position) {
        return position.column < 0 || position.column >= width || position.row < 0 ||
               position.row >= height;
    };
    if (_entry && outOfBounds(*_entry)) {
        return true;
    }
    return std::ranges::any_of(_entities, [&outOfBounds](const MapEntity& entity) {
        return outOfBounds(entity.position);
    });
}

bool LevelDraft::undo() {
    if (_undoHistory.empty()) {
        return false;
    }
    _redoHistory.push_back(snapshot());
    restore(std::move(_undoHistory.back()));
    _undoHistory.pop_back();
    _gesturePushed = false;  // une mutation qui suit, meme dans le geste, est un pas neuf.
    return true;
}

bool LevelDraft::redo() {
    if (_redoHistory.empty()) {
        return false;
    }
    _undoHistory.push_back(snapshot());
    restore(std::move(_redoHistory.back()));
    _redoHistory.pop_back();
    _gesturePushed = false;
    return true;
}

bool LevelDraft::setProperty(const std::string& key, PropertyValue value) {
    if (key.empty()) {
        return false;
    }
    const auto found = _properties.find(key);
    const bool clearing =
        std::holds_alternative<std::string>(value) && std::get<std::string>(value).empty();
    if (clearing) {
        if (found == _properties.end()) {
            return false;  // rien a retirer : ni pas d'historique, ni revision neuve.
        }
        pushUndo();
        _properties.erase(found);
        return true;
    }
    if (found != _properties.end() && found->second == value) {
        return false;
    }
    pushUndo();
    _properties[key] = std::move(value);
    return true;
}

LevelDraft::State LevelDraft::snapshot() const {
    return State{.name = _name,
                 .tileMap = _tileMap,
                 .entry = _entry,
                 .layers = _layers,
                 .entities = _entities,
                 .forcedCollision = _forcedCollision,
                 .properties = _properties,
                 .revision = _revision};
}

void LevelDraft::restore(State state) {
    _name = std::move(state.name);
    _tileMap = std::move(state.tileMap);
    _entry = state.entry;
    _layers = std::move(state.layers);
    _entities = std::move(state.entities);
    _forcedCollision = std::move(state.forcedCollision);
    _properties = std::move(state.properties);
    _revision = state.revision;
}

void LevelDraft::beginGesture() noexcept {
    if (_gestureDepth == 0) {
        _gesturePushed = false;
    }
    ++_gestureDepth;
}

void LevelDraft::endGesture() noexcept {
    if (_gestureDepth == 0) {
        return;
    }
    --_gestureDepth;
    if (_gestureDepth == 0) {
        _gesturePushed = false;
    }
}

void LevelDraft::pushUndo() {
    // Dans un geste, seule la premiere mutation empile l'etat d'avant : le geste entier se defait
    // en un pas. La revision change a chaque mutation, elle.
    if (_gestureDepth > 0 && _gesturePushed) {
        _redoHistory.clear();
        _revision = ++_lastRevision;
        return;
    }
    _gesturePushed = _gestureDepth > 0;
    _undoHistory.push_back(snapshot());
    if (_undoHistory.size() > UNDO_HISTORY_LIMIT) {
        _undoHistory.erase(_undoHistory.begin());
    }
    _redoHistory.clear();  // une nouvelle mutation invalide la branche de refaire
    _revision = ++_lastRevision;
}

std::string LevelDraft::toJson() const {
    // La grille editee EST la couche de collision de la carte (LOT-04) : le brouillon n'en peint
    // qu'une, et laisser la couche de collision figee sur l'etat du fichier d'origine produirait
    // une carte ou l'on traverse un mur qu'on voit. Les couches visuelles -- sol, decor -- sont
    // celles que les mutateurs de couches ont peintes (LOT-11).
    std::vector<TileLayer> layers = _layers;
    for (TileLayer& layer : layers) {
        if (layer.kind == LayerKind::Collision || layer.kind == LayerKind::Legacy) {
            layer.tiles = _tileMap;
        }
    }
    return LevelWriter::buildJson(LevelData{.name = _name,
                                            .tileMap = _tileMap,
                                            .layers = std::move(layers),
                                            .entities = _entities,
                                            .forcedCollision = _forcedCollision,
                                            .nextEntityId = _nextEntityId,
                                            .base = _base,
                                            .scene = _scene,
                                            .properties = _properties});
}

LevelLoadResult LevelDraft::toLevel() const {
    return LevelLoader::loadFromString(toJson());
}

}  // namespace core
