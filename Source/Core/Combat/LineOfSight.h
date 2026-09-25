// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/LineOfSight.h
 * @brief Ligne de vue et abri sur la grille tactique (`LOT-22`, `EX-CBT-021`).
 *
 * ## Ce que dit le Manuel, et ce qu'il laisse à trancher
 *
 * Chapitre 9, « Abri » : les murs, les arbres, les créatures et autres obstacles abritent ; un abri
 * partiel donne +2 à la CA et aux sauvegardes de Dextérité, un abri important +5, un abri total
 * interdit d'être ciblé directement ; **les abris ne s'additionnent pas**, seul le meilleur compte.
 * Chapitre 10, « Un chemin dégagé jusqu'à la cible » : pour viser une chose, il ne faut pas qu'elle
 * soit derrière un abri total.
 *
 * Le Manuel ne dit pas **comment** mesurer ces fractions de corps sur un quadrillage. Ce fichier
 * prend la méthode du *Guide du Maître*, la seule que les tables connaissent : choisir un coin de
 * l'emprise de l'attaquant, tracer des lignes vers les quatre coins d'une case de la cible, et
 * compter celles qu'un obstacle coupe — une ou deux : abri partiel ; trois : abri important ;
 * quatre depuis chaque coin : abri total. L'attaquant prend le coin et la case qui l'arrangent.
 *
 * ## La symétrie, par construction
 *
 * Un tracé qui part de A, avance case par case et s'arrête au premier obstacle ne donne pas le même
 * résultat depuis B : c'est le défaut classique, et le joueur le découvre en tirant sur un ennemi
 * qui ne peut pas riposter. Ici, rien n'avance : la question est « ce **segment** coupe-t-il cette
 * case ? », posée en arithmétique entière exacte sur des points à coordonnées en demi-cases. Un
 * segment n'a pas de sens de parcours, et l'ensemble des segments examinés — tous les points de
 * grille d'une emprise vers tous ceux de l'autre — est le même dans les deux sens. A voit B si et
 * seulement si B voit A ; les tests le vérifient sur des grilles générées.
 *
 * ## Ce qui coupe un segment
 *
 * - **La vue** (`BattleGrid::blocksSight` : matière pleine, objet à abri total) coupe un segment
 * qui
 *   **touche** la case, bord et coin compris, extrémités exceptées. Un segment qui rase la face
 * d'un mur ne voit donc pas au travers, et deux murs en diagonale ne laissent pas passer le regard
 * par leur coin commun — la même règle que le déplacement (« Coins », chapitre 9).
 * - **Un corps** — une créature interposée, un objet à abri partiel ou important — coupe un segment
 *   qui **traverse** sa case : il abrite, il n'aveugle pas.
 *
 * L'altitude n'y entre pas (`core::Locomotion`) : un volant se voit et se vise sur la même grille.
 */

#include <span>
#include <string_view>

#include "Core/Combat/BattleGrid.h"
#include "Core/Levels/GridPosition.h"

namespace core {

class CombatState;

/**
 * @brief Un point de la grille, en **demi-cases** : la case (c, r) va de (2c, 2r) à (2c+2, 2r+2).
 *
 * Les coins de case sont aux coordonnées paires, les centres aux impaires, les milieux d'arêtes à
 * une de chaque : tout point d'origine qu'une règle nomme s'écrit en entiers, et aucun calcul de
 * vue n'a besoin d'un flottant.
 */
struct GridPoint {
    int x = 0;
    int y = 0;
};

[[nodiscard]] constexpr bool operator==(GridPoint left, GridPoint right) noexcept {
    return left.x == right.x && left.y == right.y;
}

/// @brief Le coin haut-gauche de la case.
[[nodiscard]] constexpr GridPoint cornerOf(GridPosition cell) noexcept {
    return {.x = 2 * cell.column, .y = 2 * cell.row};
}

/// @brief Le centre de la case.
[[nodiscard]] constexpr GridPoint centerOf(GridPosition cell) noexcept {
    return {.x = 2 * cell.column + 1, .y = 2 * cell.row + 1};
}

/// @brief Une emprise carrée : coin haut-gauche et côté, en cases.
struct Footprint {
    GridPosition anchor;
    int side = 1;
};

/// @brief Le bonus d'un abri à la CA et aux sauvegardes de Dextérité : 0, +2, +5 ; 0 pour l'abri
/// total, qui n'est pas un bonus mais une interdiction.
[[nodiscard]] constexpr int coverBonus(Cover cover) noexcept {
    switch (cover) {
        case Cover::Half:
            return 2;
        case Cover::ThreeQuarters:
            return 5;
        case Cover::None:
        case Cover::Total:
            return 0;
    }
    return 0;
}

/// @brief Le nom d'un abri tel que le journal l'écrit : « abri partiel », « abri important ».
[[nodiscard]] std::string_view coverLabel(Cover cover) noexcept;

/**
 * @brief Vrai si rien n'arrête la vue sur le segment @p a — @p b (extrémités exceptées).
 *
 * Symétrique : `isSightClear(g, a, b) == isSightClear(g, b, a)`. Un segment réduit à un point est
 * dégagé sauf si ce point touche une case qui arrête la vue — le coin commun de deux murs en
 * diagonale.
 */
[[nodiscard]] bool isSightClear(const BattleGrid& grid, GridPoint a, GridPoint b);

/**
 * @brief Vrai si les deux emprises se voient : un segment dégagé relie un point de grille de l'une
 *        à un point de grille de l'autre.
 *
 * Tous les points de grille de chaque emprise, et pas seulement ses quatre coins : c'est ce qui
 * rend la relation symétrique pour une créature de grande taille, dont une case intérieure peut
 * regarder par une meurtrière que ses coins ne voient pas.
 */
[[nodiscard]] bool hasLineOfSight(const BattleGrid& grid, Footprint a, Footprint b);

/// @brief La même relation entre deux combattants ; faux si l'un des deux n'est pas sur la grille.
[[nodiscard]] bool hasLineOfSight(const CombatState& combat, CombatantId a, CombatantId b);

/**
 * @brief L'abri de @p target contre ce qui vient de @p attacker.
 *
 * Trois familles d'obstacles, comptées **séparément** puis comparées, parce que les abris ne
 * s'additionnent pas. Ce qui arrête la vue abrite **selon les lignes coupées** (une ou deux :
 * partiel ; trois : important ; quatre : total). Un corps abrite **selon sa nature**, dès qu'il
 * coupe une ligne : un objet à abri important — la herse, la meurtrière du Manuel — donne
 * `Cover::ThreeQuarters` ; une créature de @p interposed, « amie ou ennemie », ou un objet à abri
 * partiel donne `Cover::Half`. Compter ses lignes n'aurait pas de sens : une herse d'une case n'en
 * coupe jamais plus de deux. Un mur qui coupe deux lignes et un allié qui coupe les deux autres
 * font un abri partiel, pas un abri total.
 *
 * `Cover::Total` si et seulement si `!hasLineOfSight(grid, attacker, target)`.
 */
[[nodiscard]] Cover coverFrom(const BattleGrid& grid, Footprint attacker, Footprint target,
                              std::span<const Footprint> interposed = {});

/**
 * @brief L'abri de @p target contre un effet qui part d'un **point** — l'origine d'une zone.
 *
 * Même compte, depuis un seul point au lieu des coins d'une emprise : c'est l'abri qui s'ajoute à
 * une sauvegarde de Dextérité contre une boule de feu.
 */
[[nodiscard]] Cover coverFromPoint(const BattleGrid& grid, GridPoint origin, Footprint target,
                                   std::span<const Footprint> interposed = {});

/**
 * @brief L'abri d'un combattant contre un autre, les autres combattants placés faisant corps.
 *
 * Un combattant à terre reste sur la grille et abrite encore. `Cover::Total` si l'un des deux n'est
 * pas sur la grille : ce qui n'y est pas ne se vise pas.
 */
[[nodiscard]] Cover coverBetween(const CombatState& combat, CombatantId attacker,
                                 CombatantId target);

}  // namespace core
