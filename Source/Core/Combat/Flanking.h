// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/Flanking.h
 * @brief La prise en tenaille, règle optionnelle du *Guide du Maître* (`LOT-23`).
 *
 * ## Ce que dit le Guide
 *
 * Chapitre 8, « Règle optionnelle : la prise en tenaille » (PDF p. 251) : « Quand une créature et
 * au moins un de ses alliés sont adjacents à un ennemi et placés sur des côtés ou des angles
 * opposés de son emplacement, ils le prennent en tenaille et ils bénéficient tous deux d'un
 * avantage lors de leurs jets d'attaque au corps à corps contre cet ennemi. » Pour trancher un
 * doute, « tracez une ligne imaginaire entre les centres des emplacements de chacune de ces deux
 * créatures. Si la ligne passe par des côtés ou des angles opposés de l'emplacement de
 * l'adversaire, alors ce dernier est bien pris en tenaille. »
 *
 * Deux restrictions : on ne prend pas en tenaille un ennemi **qu'on ne voit pas**, ni quand on est
 * **neutralisé** ; et une créature de Grande taille ou plus prend en tenaille « tant que l'une des
 * cases qu'elle occupe remplit les conditions ».
 *
 * ## Comment la grille le mesure
 *
 * Le doute est tranché **toujours** par la ligne des centres : c'est la seule des deux formulations
 * qui se calcule sans interprétation. Les centres sont des points de grille en demi-cases
 * (`core::centerOf`), les bords de l'emplacement des coordonnées paires ; la ligne « passe par deux
 * côtés opposés » si elle touche le bord gauche **et** le bord droit, ou le haut **et** le bas,
 * coins compris — ce qui couvre les angles opposés. Tout est entier et exact.
 *
 * La règle est **optionnelle** : c'est la donnée de l'arène qui l'active
 * (`core::ArenaBout::flanking`), pas le moteur.
 */

#include "Core/Combat/CombatState.h"
#include "Core/Combat/LineOfSight.h"

namespace core {

/**
 * @brief Vrai si la ligne entre les centres des cases @p a et @p b passe par deux côtés opposés de
 *        l'emplacement @p target.
 *
 * La géométrie seule : ni l'adjacence, ni la vue, ni l'état des créatures.
 */
[[nodiscard]] bool crossesOppositeSides(GridPosition a, GridPosition b, Footprint target) noexcept;

/**
 * @brief Vrai si @p attacker, ancré en @p attackerAnchor, prend @p target en tenaille avec au moins
 *        un allié debout.
 *
 * Chacun des deux doit être debout, adjacent à la cible (distance 1, emprises comprises) et la voir
 * (`core::hasLineOfSight`) ; une case de l'emprise de l'un et une case de l'emprise de l'autre
 * doivent être alignées par leurs centres sur deux côtés opposés de la cible. L'ancre supposée sert
 * l'IA (`LOT-23`), qui juge une case avant d'y aller.
 */
[[nodiscard]] bool isFlankedFrom(const CombatState& combat, CombatantId attacker,
                                 GridPosition attackerAnchor, CombatantId target);

/// @brief La même règle, l'attaquant à sa place sur la grille. Faux s'il n'y est pas.
[[nodiscard]] bool isFlanked(const CombatState& combat, CombatantId attacker, CombatantId target);

}  // namespace core
