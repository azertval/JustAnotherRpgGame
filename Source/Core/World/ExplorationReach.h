// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <vector>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/TileMap.h"

/**
 * @file Core/World/ExplorationReach.h
 * @brief Les cases qu'un héros **en exploration** peut atteindre depuis un point de départ
 *        (`LOT-EDITOR-07`).
 *
 * ## La règle, et pourquoi c'est celle du jeu
 *
 * `core::ExplorationSession` déplace le héros **axe par axe**, et refuse un pas si l'un des quatre
 * coins de son gabarit (une demi-largeur de `HERO_HALF_SIZE_CELLS`) tombe hors de la carte ou sur
 * une case solide (`core::TileMap::isSolid`). Ce gabarit est plus petit qu'une case : le héros
 * passe donc sur **toute case non solide**, et il ne passe d'une case à l'autre que par un côté,
 * jamais par un coin — deux murs en diagonale ferment le passage. D'où la règle d'ici : les cases
 * non solides reliées **en quatre voisins** à un départ.
 *
 * Ce n'est pas la `core::ReachableArea` du combat (`LOT-19`) : en combat, on coupe les diagonales
 * libres et le terrain difficile coûte double. Les deux règles répondent à deux questions.
 *
 * L'interaction (`core::findInteractionTarget`) vise une case **voisine par un côté** et non
 * solide : un PNJ qu'on peut aborder se tient donc lui-même sur une case atteinte.
 */

namespace core {

/// @brief Les cases atteintes depuis des départs, sur une grille de collision.
class ExplorationReach {
public:
    /**
     * @brief Parcourt @p collision depuis @p starts.
     *
     * Un départ hors de la carte ou sur une case solide n'atteint rien, pas même sa case.
     */
    ExplorationReach(const TileMap& collision, const std::vector<GridPosition>& starts);

    /// @return Vrai si le héros peut se tenir sur @p cell.
    [[nodiscard]] bool reaches(GridPosition cell) const;

    /// @return Le nombre de cases atteintes.
    [[nodiscard]] std::size_t count() const noexcept {
        return _count;
    }

private:
    int _width = 0;
    int _height = 0;
    std::vector<bool> _reached;
    std::size_t _count = 0;
};

}  // namespace core
