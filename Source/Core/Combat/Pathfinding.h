// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/Pathfinding.h
 * @brief Le déplacement d'un tour : le budget, les cases atteignables et le chemin
 *        (`LOT-19`, `EX-CBT-020`, `EX-REG-051`).
 */

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "Core/Combat/BattleGrid.h"
#include "Core/Levels/GridPosition.h"

namespace core {

struct CharacterSheet;
struct Creature;

/**
 * @brief Le budget de déplacement d'une vitesse de @p speedMeters, en cases entières.
 *
 * Une case vaut 1,5 m (`EX-REG-051`, `core::METERS_PER_TILE`) : 9 m font 6 cases. Une vitesse qui
 * ne tombe pas juste — 10 m après un malus d'encombrement de 3 m sur 13 m — est **arrondie à la
 * case inférieure** : le livre dépense la vitesse « par segments de 1,50 mètre », et un segment
 * entamé n'en est pas un. Arrondir au plus proche ferait gagner une case à qui porte trop.
 * Une vitesse négative ou nulle donne 0.
 */
[[nodiscard]] int movementBudget(float speedMeters) noexcept;

/// @brief Le budget d'une fiche, sur sa vitesse de base (`core::CharacterSheet::speedMeters`).
[[nodiscard]] int movementBudget(const CharacterSheet& sheet) noexcept;

/**
 * @brief Le budget d'une créature du bestiaire pour @p locomotion : sa vitesse de marche, ou de
 *        vol — 0 pour une créature qui ne vole pas.
 */
[[nodiscard]] int movementBudget(const Creature& creature, Locomotion locomotion) noexcept;

/**
 * @brief Qui se déplace, comment, et à travers qui.
 *
 * Le combattant doit être **placé** sur la grille : son point de départ et son emprise en sont lus,
 * jamais recopiés, pour qu'une requête ne parte pas d'une case qu'il a déjà quittée.
 */
struct Mover {
    CombatantId combatant{};
    Locomotion locomotion = Locomotion::Walk;
    /**
     * @brief Vrai pour un combattant dont on peut **traverser** la case sans s'y arrêter.
     *
     * Le Manuel permet de traverser l'espace d'une créature non hostile, et la grille ne connaît
     * pas les camps : c'est `core::CombatState::moverFor` (`LOT-20`) qui le dit. Vide, personne ne
     * se traverse — le parti prudent, qui ne fait jamais passer à travers un ennemi.
     */
    std::function<bool(CombatantId)> canPassThrough;
};

/// @brief Un chemin : les cases franchies, départ exclu et arrivée incluse, et ce qu'il coûte.
struct Path {
    std::vector<GridPosition> steps;
    int cost = 0;
};

/**
 * @brief Les cases qu'un combattant peut atteindre ce tour-ci, et par où.
 *
 * ## La règle du Manuel, et pourquoi elle n'est pas un parcours en largeur
 *
 * Manuel des Joueurs, « Jouer sur un quadrillage » : entrer dans une case coûte **1**, même en
 * **diagonale** ; entrer dans une case de terrain difficile coûte **2**, et il faut qu'il reste de
 * quoi payer ; on ne passe pas en diagonale par le **coin** d'un mur. Trois faits qu'un parcours en
 * largeur à quatre voisins ne sait pas exprimer : il ignore les diagonales, et tous ses pas coûtent
 * un. La case d'une créature qu'on **traverse** compte, elle aussi, pour du terrain difficile
 * (« Se déplacer au milieu d'autres créatures », PDF p. 193), en vol comme au sol. Ce calcul-ci est
 * donc un Dijkstra sur les huit voisins.
 *
 * ## Le départage est une règle, pas un hasard
 *
 * Deux chemins de même coût existent presque toujours. L'exploration retient **tous** les
 * prédécesseurs qui atteignent une case à son meilleur coût ; c'est la remontée, qui connaît
 * l'arrivée, qui choisit : **le prédécesseur le plus proche de la droite qui joint le départ à
 * l'arrivée**, et à égalité celui d'indice de case le plus petit (ligne, puis colonne). Le chemin
 * ne monte donc pas pour redescendre quand un autre chemin de même coût suit la droite — c'est lui
 * que la prévisualisation du déplacement dessine (`LOT-24`). Cette définition ne dépend pas de
 * l'ordre d'exploration : `core::findPath` la partage, et le chemin qu'il rend vers une case
 * atteignable est celui que rend `pathTo`, pas un autre de même coût.
 */
class ReachableArea {
public:
    /**
     * @param grid   La grille, lue à la construction et plus jamais ensuite.
     * @param mover  Qui se déplace. Non placé, l'aire est vide.
     * @param budget Les cases de déplacement disponibles (`core::movementBudget`).
     */
    ReachableArea(const BattleGrid& grid, const Mover& mover, int budget);

    /// @brief Le coin haut-gauche de l'emprise au départ.
    [[nodiscard]] GridPosition origin() const noexcept {
        return _origin;
    }
    /// @brief Les cases de déplacement dont l'aire a été calculée.
    [[nodiscard]] int budget() const noexcept {
        return _budget;
    }

    /**
     * @brief Le coût du meilleur chemin jusqu'à @p anchor, s'il tient dans le budget.
     *
     * Défini aussi pour une case qu'on **traverse** sans pouvoir s'y arrêter (un allié l'occupe) :
     * le coût dit qu'on y passe, `canEndAt` dit qu'on n'y reste pas.
     */
    [[nodiscard]] std::optional<int> costTo(GridPosition anchor) const;

    /// @brief Vrai si le combattant peut **finir** son déplacement avec son emprise ancrée en
    /// @p anchor : dans le budget, et sur une place libre. Faux pour le point de départ.
    [[nodiscard]] bool canEndAt(GridPosition anchor) const;

    /**
     * @brief Toutes les places où finir le déplacement, par indice de case croissant — ce que
     *        l'IHM de combat surligne (`LOT-24`) avant que le joueur ne s'engage.
     */
    [[nodiscard]] std::vector<GridPosition> destinations() const;

    /// @brief Le chemin jusqu'à @p anchor, ou `std::nullopt` si l'on ne peut pas y finir.
    [[nodiscard]] std::optional<Path> pathTo(GridPosition anchor) const;

private:
    [[nodiscard]] std::size_t indexOf(GridPosition cell) const noexcept;

    int _width = 0;
    int _height = 0;
    GridPosition _origin{};
    int _budget = 0;
    std::vector<int> _costs;
    /// Par case, le masque de ses prédécesseurs au meilleur coût (un bit par voisin).
    std::vector<std::uint8_t> _predecessors;
    std::vector<bool> _endable;
};

/**
 * @brief Le meilleur chemin de @p mover jusqu'à @p destination, sans limite de budget.
 *
 * A* sur la même règle que `core::ReachableArea` — mêmes coûts, mêmes coins, même départage —
 * pour qui planifie au-delà du tour : l'IA (`LOT-23`) qui marche vers une cible lointaine. Le
 * même appel sur la même grille rend **toujours** le même chemin, sans quoi aucun test de combat
 * ne tiendrait.
 *
 * @return Le chemin, ou `std::nullopt` si @p destination est hors d'atteinte, ne peut pas
 *         accueillir l'emprise du combattant, ou si celui-ci n'est pas placé.
 */
[[nodiscard]] std::optional<Path> findPath(const BattleGrid& grid, const Mover& mover,
                                           GridPosition destination);

}  // namespace core
