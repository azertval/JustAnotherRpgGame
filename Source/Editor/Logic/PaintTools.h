// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Editor/Logic/BrushGesture.h"
#include "Editor/Logic/LayerView.h"

/**
 * @file Editor/Logic/PaintTools.h
 * @brief Les outils du peintre : trait, rectangle, ligne, seau, pipette, miroir et mesure
 *        (`LOT-EDITOR-04`, `EX-EDIT-066`, `EX-EDIT-067`, `EX-EDIT-069`).
 *
 * Chaque outil est une **fonction pure** : il calcule les cases qu'il touche (`lineCells`,
 * `floodRegion`) ou ce qu'il lit (`pickBrush`, `measureBetween`), puis le geste passe par le
 * pinceau du `LOT-EDITOR-03` (`hmi::applyBrush`), case par case, **dans un seul geste du
 * brouillon** (`core::GestureScope`) : un geste se défait en un pas, quel que soit le nombre de
 * cases. Le canevas les appelle à la souris ; l'éditeur sans fenêtre (`LOT-EDITOR-13`,
 * `hmi::applyGestureScript`) les appelle telles quelles (règle 4 de la feuille de route).
 *
 * ## Le miroir
 *
 * Le miroir reflète chaque geste de l'autre côté d'un **axe vertical de l'écran iso**. En cases,
 * c'est la diagonale `colonne − ligne = k` : la case (c, r) a pour reflet (r + k, c − k), une
 * emprise de c × r cases devient r × c. C'est le seul miroir que les planches connaissent : une
 * pièce et sa **jumelle** (`mirrorOf` du manifeste, `wall-left` et `wall-right`, `front-left` 1 × 2
 * et `front-right` 2 × 1) sont l'image l'une de l'autre dans ce miroir-là. Le reflet pose la
 * jumelle ; une pièce sans jumelle se pose telle quelle. Un geste qui **chevauche son reflet** —
 * une case sur l'axe, une emprise ou un rectangle qui le traverse — ne se reflète pas : le reflet
 * écraserait ce qu'on vient de poser.
 */

namespace core {
class LevelDraft;
class ScenePieceManifest;
}  // namespace core

namespace hmi {

class PlaceAppearance;

/// @brief Nombre de pieds par case : la règle du Manuel des Joueurs (« Jouer sur un quadrillage »).
inline constexpr int FEET_PER_CELL = 5;

/// @brief L'axe du miroir : la diagonale `colonne − ligne = offset`, verticale à l'écran iso.
struct MirrorAxis {
    int offset = 0;

    [[nodiscard]] bool operator==(const MirrorAxis&) const = default;
};

/// @return L'axe qui passe par le centre de @p cell.
[[nodiscard]] MirrorAxis mirrorAxisThrough(core::GridPosition cell) noexcept;

/// @return Le reflet de @p cell dans @p axis.
[[nodiscard]] core::GridPosition mirrorCell(const MirrorAxis& axis,
                                            core::GridPosition cell) noexcept;

/**
 * @return La jumelle de @p piece dans @p manifest : la pièce dont elle est le miroir, ou celle qui
 *         est le sien ; @p piece elle-même si elle n'en a pas, ou sans manifeste.
 */
[[nodiscard]] std::string mirrorPieceName(const core::ScenePieceManifest* manifest,
                                          std::string_view piece);

/**
 * @brief Le pinceau d'une pièce choisie dans la palette : la pièce, le type qu'écrit sa case
 *        d'ancrage (`hmi::pieceCellType`), sa couche.
 * @param appearance La table du lieu, `nullptr` sans lieu.
 * @param piece      Le nom court de la pièce.
 * @param floor      La pièce est un sol.
 */
[[nodiscard]] CanvasBrush pieceBrush(const PlaceAppearance* appearance, std::string piece,
                                     bool floor);

/// @brief Ce qui accompagne un geste : le miroir s'il est actif, la table du lieu pour les types.
struct StrokeContext {
    /// Le miroir actif, ou rien.
    std::optional<MirrorAxis> mirror;
    /// La table du lieu : le type que la case d'ancrage d'une jumelle écrit
    /// (`hmi::pieceCellType`). `nullptr` sans lieu.
    const PlaceAppearance* appearance = nullptr;
};

/**
 * @brief Donne @p brush sur chacune de @p cells, dans l'ordre, en **un** geste, reflets compris.
 * @param draft      Le brouillon.
 * @param brush      Le pinceau.
 * @param active     La couche active.
 * @param view       Les réglages des couches (verrous).
 * @param cells      Les cases, dans l'ordre du trait.
 * @param continuing La première case prolonge un trait déjà commencé (le glisser du pinceau) :
 *                   une pièce ne se repose pas sur une case que la même pièce couvre déjà (voir
 *                   `hmi::applyBrush`). Les cases suivantes prolongent toujours le trait.
 * @param context    Le miroir et la table du lieu.
 * @return Changé si une case au moins a changé ; le premier refus sinon.
 */
BrushResult applyStroke(core::LevelDraft& draft, const CanvasBrush& brush, LayerSlot active,
                        const LayerViewState& view, const std::vector<core::GridPosition>& cells,
                        bool continuing, const StrokeContext& context);

/**
 * @brief Donne @p brush sur le rectangle [@p first, @p last], en un geste, reflet compris (le
 *        rectangle reflété, qui pave au pas de l'emprise de la jumelle).
 */
BrushResult applyRectangleStroke(core::LevelDraft& draft, const CanvasBrush& brush,
                                 LayerSlot active, const LayerViewState& view,
                                 core::GridPosition first, core::GridPosition last,
                                 const StrokeContext& context);

/**
 * @return Les cases de la ligne de @p from à @p to, bornes incluses, dans l'ordre du tracé
 *         (Bresenham, huit voisins : une case par pas sur l'axe dominant).
 */
[[nodiscard]] std::vector<core::GridPosition> lineCells(core::GridPosition from,
                                                        core::GridPosition to);

/**
 * @brief La région que le seau remplit : les cases reliées à @p seed par un côté (quatre voisins)
 *        qui portent **le même contenu** que lui sur la couche @p slot.
 *
 * Sur une couche visuelle, le contenu d'une case est son type et sa pièce ; une case couverte par
 * une pièce large n'est égale qu'aux autres cases de **cette** pièce. Sur la grille de collision
 * (@p slot vide), c'est son type. Triées ligne par ligne ; vide si @p seed est hors de la carte.
 */
[[nodiscard]] std::vector<core::GridPosition> floodRegion(const core::LevelDraft& draft,
                                                          LayerSlot slot, core::GridPosition seed);

/// @return La couche que vise @p brush : celle de la pièce pour une pièce, @p active sinon.
[[nodiscard]] LayerSlot brushTargetLayer(const core::LevelDraft& draft, const CanvasBrush& brush,
                                         LayerSlot active);

/**
 * @brief Remplit au seau depuis @p seed, en un geste, reflet compris.
 *
 * Le seau ne pose que des pièces d'une case : une pièce large déborderait de la région. Il refuse
 * aussi une couche verrouillée, comme le pinceau.
 */
BrushResult applyBucket(core::LevelDraft& draft, const CanvasBrush& brush, LayerSlot active,
                        const LayerViewState& view, core::GridPosition seed,
                        const StrokeContext& context);

/// @brief Ce que la pipette a pris : le pinceau, et la couche où il a été lu.
struct PickedBrush {
    CanvasBrush brush;
    LayerSlot layer;

    [[nodiscard]] bool operator==(const PickedBrush&) const = default;
};

/**
 * @brief La pipette : le pinceau qui repeindrait ce qu'on voit en @p cell.
 *
 * La collision active, c'est son type. Sinon, la couche active d'abord, puis les autres couches
 * visuelles de l'avant vers l'arrière : la pièce qui couvre la case (son ancre, pour une pièce
 * large), à défaut le type de la case. Une carte sans couche visuelle rend le type de sa grille.
 * @param draft      Le brouillon.
 * @param active     La couche active.
 * @param cell       La case piquée.
 * @param appearance La table du lieu (le type de la case d'ancrage), `nullptr` sans lieu.
 * @return Rien si aucune couche n'a quoi que ce soit en @p cell, ou si elle est hors de la carte.
 */
[[nodiscard]] std::optional<PickedBrush> pickBrush(const core::LevelDraft& draft, LayerSlot active,
                                                   core::GridPosition cell,
                                                   const PlaceAppearance* appearance);

/// @brief Une mesure entre deux cases.
struct Measure {
    /// Écart en colonnes et en lignes (valeurs absolues).
    int columns = 0;
    int rows = 0;
    /// Distance en cases : une diagonale coûte une case, comme au combat (`core::ReachableArea`).
    int cells = 0;
    /// La même distance en pieds (`FEET_PER_CELL`).
    int feet = 0;

    [[nodiscard]] bool operator==(const Measure&) const = default;
};

/// @return La mesure de @p from à @p to.
[[nodiscard]] Measure measureBetween(core::GridPosition from, core::GridPosition to) noexcept;

/// @return La mesure lisible : l'étendue en cases, bornes comprises, puis la distance —
///         `7 × 4 · 6 cells = 30 ft`.
[[nodiscard]] std::string measureLabel(const Measure& measure);

}  // namespace hmi
