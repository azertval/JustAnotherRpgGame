// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <vector>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Core/Resources/ScenePieceManifest.h"

/**
 * @file Core/Levels/CollisionDerivation.h
 * @brief La collision d'une carte, **déduite** de ce qu'elle montre (`LOT-EDITOR-12`, décision
 *        D10, `EX-LVL-020`).
 *
 * Jusqu'à la v3, la grille de collision se peignait à part, et rien ne la tenait d'accord avec les
 * pièces posées : un mur dessiné pouvait se traverser. En v4, elle se **déduit** des couches
 * visuelles et du manifeste des pièces, puis s'**écrit** dans le fichier — le jeu la lit sans
 * manifeste — avec la liste des cases que l'auteur a **forcées** à la main. `LevelEditor --check`
 * vérifie que le fichier égale la déduction, cases forcées mises à part.
 *
 * ## La règle, case par case
 *
 * Chaque case reçoit la contribution **la plus forte** (`core::PieceTactical`) de :
 *
 * 1. chaque pièce nommée par une couche visuelle dont l'**emprise** la couvre
 *    (`core::footprintCells`) : son type tactique au manifeste ;
 * 2. chaque case de couche visuelle **sans pièce** (ou dont la pièce est absente du manifeste) mais
 *    d'un type non vide : la règle du type — mur et matière pleine arrêtent la vue, eau profonde et
 *    falaise arrêtent le pas, le reste passe ;
 * 3. une case que **rien** ne couvre — ni type, ni pièce, sur aucune couche — est du vide : elle
 *    arrête la vue. On ne se tient pas là où il n'y a pas de sol.
 *
 * La contribution s'écrit dans le vocabulaire de la grille de collision que le jeu lit déjà :
 * `wall` (arrête la vue), `cliff` (arrête le pas), vide sinon. La gêne et l'abri se déduisent
 * vides : aucune règle du jeu ne les joue encore depuis une pièce, et la déduction les relève
 * (`CollisionDerivation::unplayed`) pour que le contrôle le dise.
 *
 * L'entrée n'est pas déduite : elle est un repère posé dans la grille, et l'appelant la replace.
 */

namespace core {

/// @brief Ce que la déduction a produit, et ce qu'elle a relevé en chemin.
struct CollisionDerivation {
    /// La grille de collision déduite, aux dimensions de la carte, sans entrée.
    TileMap collision;
    /// Cases dont la contribution la plus forte est une gêne ou un abri, déduits vides.
    std::vector<GridPosition> unplayed{};
    /// Cases nommant une pièce que le manifeste ne connaît pas (ni par son nom ni par un alias).
    std::vector<GridPosition> unknownPieces{};
};

/// @return La valeur de grille de collision d'un type tactique : `Wall`, `Cliff` ou `Empty`.
[[nodiscard]] TileType collisionTileOf(PieceTactical tactical) noexcept;

/// @return Le type tactique que la règle des types donne à @p type (étape 2 ci-dessus).
[[nodiscard]] PieceTactical tacticalOfTileType(TileType type) noexcept;

/**
 * @brief Déduit la grille de collision d'une carte.
 * @param layers   Les couches de la carte ; seules les couches visuelles comptent.
 * @param width    Largeur de la carte, en cases.
 * @param height   Hauteur de la carte, en cases.
 * @param manifest Le manifeste des pièces du lieu, ou `nullptr` : toute pièce compte alors comme
 *                 inconnue, et la case suit la règle de son type.
 */
[[nodiscard]] CollisionDerivation deriveCollision(const std::vector<TileLayer>& layers, int width,
                                                  int height, const ScenePieceManifest* manifest);

/**
 * @return La valeur canonique de collision de @p type : ce qu'il oppose, écrit `wall`, `cliff` ou
 *         vide. `dirt`, `entry` ou `bridge` y valent une case vide, `solid` un mur.
 */
[[nodiscard]] TileType canonicalCollisionTile(TileType type) noexcept;

/**
 * @brief Vrai si la grille de collision écrite @p written s'accorde avec @p derived en
 *        (@p column, @p row) : elles opposent la même chose (`canonicalCollisionTile`). Une case
 *        `dirt` écrite s'accorde avec une case vide déduite — la v3 écrivait l'une pour l'autre —,
 *        et l'entrée compte pour une case vide.
 */
[[nodiscard]] bool collisionAgrees(const TileMap& written, const TileMap& derived, int column,
                                   int row);

}  // namespace core
