// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/Pathfinding.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <queue>
#include <utility>

#include "Core/Rpg/Bestiary.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Scale.h"

namespace core {

namespace {

constexpr int UNREACHED = std::numeric_limits<int>::max();

// Tolérance de l'arrondi à la case inférieure : 9 / 1,5 tombe juste en binaire, mais une vitesse
// issue d'une soustraction de flottants (malus d'encombrement) peut valoir 5,9999 cases, et en
// perdre une sur une erreur d'arrondi serait un défaut invisible.
constexpr float SPEED_EPSILON = 1.0e-3F;

// Les huit voisins, dans un ordre **fixe**. Le départage ne dépend pas de cet ordre (il porte sur
// la géométrie, puis sur l'indice de case), mais un ordre fixe garde la file identique d'une
// exécution à l'autre, et c'est le rang d'un voisin ici qui numérote son bit dans le masque des
// prédécesseurs.
constexpr std::array<std::pair<int, int>, 8> NEIGHBOURS{{
    {0, -1},
    {-1, 0},
    {1, 0},
    {0, 1},
    {-1, -1},
    {1, -1},
    {-1, 1},
    {1, 1},
}};

// Coût d'entrée normal et en terrain difficile (Manuel des Joueurs, « Jouer sur un quadrillage »).
constexpr int NORMAL_COST = 1;
constexpr int DIFFICULT_COST = 2;

// Ce qu'il faut savoir d'un combattant pour dire si un pas est permis, et ce qu'il coûte.
struct StepRules {
    const BattleGrid& grid;
    const Mover& mover;
    int side = 1;

    // Retourne : Le coût d'entrée de @p to depuis @p from, voisin, ou `std::nullopt` si le pas est
    // interdit.
    [[nodiscard]] std::optional<int> cost(GridPosition from, GridPosition to) const {
        if (!grid.isClear(to, side, mover.locomotion)) {
            return std::nullopt;
        }
        const bool diagonal = from.column != to.column && from.row != to.row;
        // Le coin d'un mur : on regarde les deux places orthogonales que la diagonale enjambe. Le
        // coin se juge en vol — seule la matière qui « remplit l'espace » l'interdit, pas une eau
        // profonde qu'on ne fait que longer.
        if (diagonal &&
            (!grid.isClear({.column = to.column, .row = from.row}, side, Locomotion::Fly) ||
             !grid.isClear({.column = from.column, .row = to.row}, side, Locomotion::Fly))) {
            return std::nullopt;
        }
        bool difficult = false;
        bool crowded = false;
        for (int row = to.row; row < to.row + side; ++row) {
            for (int column = to.column; column < to.column + side; ++column) {
                const GridPosition cell{.column = column, .row = row};
                const std::optional<CombatantId> occupant = grid.occupantAt(cell);
                if (occupant.has_value() && *occupant != mover.combatant) {
                    if (!(mover.canPassThrough && mover.canPassThrough(*occupant))) {
                        return std::nullopt;
                    }
                    crowded = true;
                }
                difficult = difficult || grid.isDifficult(cell);
            }
        }
        // « L'emplacement occupé par une autre créature est aussi considéré comme un terrain
        // difficile » (Manuel des Joueurs, PDF p. 192 et 193) : traverser un allié coûte double, en
        // vol comme au sol — c'est la créature qui gêne, pas le sol.
        if (crowded) {
            return DIFFICULT_COST;
        }
        // Le terrain difficile est une gêne de sol : un volant ne la paie pas (`core::Locomotion`).
        return difficult && mover.locomotion == Locomotion::Walk ? DIFFICULT_COST : NORMAL_COST;
    }
};

// Élément de file : clé de priorité, puis indice de case — l'ordre total qui rend la file
// déterministe.
using QueueEntry = std::pair<int, std::size_t>;
using MinQueue = std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<>>;

// Le masque des prédécesseurs optimaux d'une case : le bit k est levé si le voisin k de
// `NEIGHBOURS`, **relativement à la case**, l'atteint à son meilleur coût. Tout ce qu'un
// prédécesseur a de plus qu'un autre se juge à la remontée (`rebuild`), quand la destination est
// connue.
using Predecessors = std::vector<std::uint8_t>;

// Relâche l'arc qui mène à @p next par le pas de rang @p neighbour, au coût cumulé @p candidate.
//
// Retourne : Vrai si le coût de @p next a **baissé** — il faut alors le remettre en file, et ses
// anciens prédécesseurs ne valent plus rien. À coût égal, le pas s'ajoute aux prédécesseurs : le
// choix entre eux n'appartient pas à l'exploration.
bool relax(std::vector<int>& costs, Predecessors& predecessors, std::size_t next,
           std::size_t neighbour, int candidate) {
    const auto bit = static_cast<std::uint8_t>(1U << neighbour);
    if (candidate < costs[next]) {
        costs[next] = candidate;
        predecessors[next] = bit;
        return true;
    }
    if (candidate == costs[next]) {
        predecessors[next] = static_cast<std::uint8_t>(predecessors[next] | bit);
    }
    return false;
}

// Remonte le chemin de @p origin à @p target, prédécesseur par prédécesseur.
//
// Parmi les prédécesseurs qui atteignent une case à son meilleur coût, on retient **le plus proche
// de la droite** qui joint le départ à l'arrivée — le produit vectoriel entier tient lieu de
// distance —, et à égalité celui d'indice de case le plus petit. Le chemin ne fait donc jamais de
// coude qu'un autre chemin de même coût aurait évité : c'est celui que la prévisualisation trace.
[[nodiscard]] Path rebuild(const std::vector<int>& costs, const Predecessors& predecessors,
                           GridPosition origin, GridPosition target, int width) {
    const auto indexOf = [width](GridPosition cell) {
        return (static_cast<std::size_t>(cell.row) * static_cast<std::size_t>(width)) +
               static_cast<std::size_t>(cell.column);
    };
    const long long axisColumn = target.column - origin.column;
    const long long axisRow = target.row - origin.row;
    const auto deviation = [&](GridPosition cell) {
        const long long column = cell.column - origin.column;
        const long long row = cell.row - origin.row;
        return std::llabs((axisColumn * row) - (axisRow * column));
    };

    Path path;
    path.cost = costs[indexOf(target)];
    GridPosition current = target;
    while (current != origin) {
        path.steps.push_back(current);
        const std::uint8_t mask = predecessors[indexOf(current)];
        std::optional<GridPosition> best;
        for (std::size_t rank = 0; rank < NEIGHBOURS.size(); ++rank) {
            if ((mask & (1U << rank)) == 0) {
                continue;
            }
            // Le bit dit d'où l'on vient : le prédécesseur est à l'opposé du pas relâché.
            const GridPosition candidate{.column = current.column - NEIGHBOURS[rank].first,
                                         .row = current.row - NEIGHBOURS[rank].second};
            if (!best.has_value() || deviation(candidate) < deviation(*best) ||
                (deviation(candidate) == deviation(*best) && indexOf(candidate) < indexOf(*best))) {
                best = candidate;
            }
        }
        if (!best.has_value()) {
            break;  // Une case sans prédécesseur est l'origine ; ne se produit pas ailleurs.
        }
        current = *best;
    }
    std::ranges::reverse(path.steps);
    return path;
}

}  // namespace

int movementBudget(float speedMeters) noexcept {
    if (!(speedMeters > 0.0F)) {
        return 0;
    }
    return static_cast<int>(std::floor(tilesFromMeters(speedMeters) + SPEED_EPSILON));
}

int movementBudget(const CharacterSheet& sheet) noexcept {
    return movementBudget(sheet.speedMeters);
}

int movementBudget(const Creature& creature, Locomotion locomotion) noexcept {
    switch (locomotion) {
        case Locomotion::Walk:
            return movementBudget(creature.speed.walk);
        case Locomotion::Fly:
            return movementBudget(creature.speed.fly.value_or(0.0F));
    }
    return 0;
}

ReachableArea::ReachableArea(const BattleGrid& grid, const Mover& mover, int budget)
    : _width(grid.width()),
      _height(grid.height()),
      _budget(std::max(budget, 0)),
      _costs(static_cast<std::size_t>(_width) * static_cast<std::size_t>(_height), UNREACHED),
      _predecessors(_costs.size(), 0),
      _endable(_costs.size(), false) {
    const std::optional<GridPosition> start = grid.positionOf(mover.combatant);
    if (!start.has_value()) {
        return;
    }
    _origin = *start;
    const StepRules rules{.grid = grid, .mover = mover, .side = grid.sideOf(mover.combatant)};

    MinQueue frontier;
    _costs[indexOf(_origin)] = 0;
    frontier.emplace(0, indexOf(_origin));
    while (!frontier.empty()) {
        const auto [cost, index] = frontier.top();
        frontier.pop();
        if (cost > _costs[index]) {
            continue;  // Entrée périmée : la case a été atteinte moins cher depuis.
        }
        const GridPosition current{.column = static_cast<int>(index) % _width,
                                   .row = static_cast<int>(index) / _width};
        for (std::size_t rank = 0; rank < NEIGHBOURS.size(); ++rank) {
            const auto [deltaColumn, deltaRow] = NEIGHBOURS[rank];
            const GridPosition next{.column = current.column + deltaColumn,
                                    .row = current.row + deltaRow};
            if (!grid.inBounds(next)) {
                continue;
            }
            const std::optional<int> step = rules.cost(current, next);
            // Il faut pouvoir payer l'entrée entière : une case difficile à une case du budget est
            // hors de portée, pas « à moitié » atteinte.
            if (!step.has_value() || cost + *step > _budget) {
                continue;
            }
            const std::size_t nextIndex = indexOf(next);
            if (relax(_costs, _predecessors, nextIndex, rank, cost + *step)) {
                frontier.emplace(_costs[nextIndex], nextIndex);
            }
        }
    }

    for (std::size_t index = 0; index < _costs.size(); ++index) {
        const GridPosition anchor{.column = static_cast<int>(index) % _width,
                                  .row = static_cast<int>(index) / _width};
        _endable[index] = _costs[index] != UNREACHED && anchor != _origin &&
                          grid.canStand(anchor, rules.side, mover.combatant, mover.locomotion);
    }
}

std::size_t ReachableArea::indexOf(GridPosition cell) const noexcept {
    return (static_cast<std::size_t>(cell.row) * static_cast<std::size_t>(_width)) +
           static_cast<std::size_t>(cell.column);
}

std::optional<int> ReachableArea::costTo(GridPosition anchor) const {
    if (anchor.column < 0 || anchor.row < 0 || anchor.column >= _width || anchor.row >= _height) {
        return std::nullopt;
    }
    const int cost = _costs[indexOf(anchor)];
    return cost == UNREACHED ? std::nullopt : std::optional<int>(cost);
}

bool ReachableArea::canEndAt(GridPosition anchor) const {
    if (anchor.column < 0 || anchor.row < 0 || anchor.column >= _width || anchor.row >= _height) {
        return false;
    }
    return _endable[indexOf(anchor)];
}

std::vector<GridPosition> ReachableArea::destinations() const {
    std::vector<GridPosition> cells;
    for (std::size_t index = 0; index < _endable.size(); ++index) {
        if (_endable[index]) {
            cells.push_back({.column = static_cast<int>(index) % _width,
                             .row = static_cast<int>(index) / _width});
        }
    }
    return cells;
}

std::optional<Path> ReachableArea::pathTo(GridPosition anchor) const {
    if (!canEndAt(anchor)) {
        return std::nullopt;
    }
    return rebuild(_costs, _predecessors, _origin, anchor, _width);
}

std::optional<Path> findPath(const BattleGrid& grid, const Mover& mover, GridPosition destination) {
    const std::optional<GridPosition> start = grid.positionOf(mover.combatant);
    const int side = grid.sideOf(mover.combatant);
    if (!start.has_value() ||
        !grid.canStand(destination, side, mover.combatant, mover.locomotion)) {
        return std::nullopt;
    }
    const int width = grid.width();
    const auto indexOf = [width](GridPosition cell) {
        return (static_cast<std::size_t>(cell.row) * static_cast<std::size_t>(width)) +
               static_cast<std::size_t>(cell.column);
    };
    // Distance de Tchebychev : chaque pas coûte au moins 1 et avance d'au plus une case sur chaque
    // axe. L'heuristique est donc cohérente — une case sortie de la file a son coût définitif.
    const auto estimate = [destination](GridPosition cell) {
        return std::max(std::abs(cell.column - destination.column),
                        std::abs(cell.row - destination.row));
    };
    const StepRules rules{.grid = grid, .mover = mover, .side = side};

    const std::size_t cellTotal =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(grid.height());
    std::vector<int> costs(cellTotal, UNREACHED);
    Predecessors predecessors(cellTotal, 0);
    const std::size_t target = indexOf(destination);

    MinQueue frontier;
    costs[indexOf(*start)] = 0;
    frontier.emplace(estimate(*start), indexOf(*start));
    while (!frontier.empty()) {
        const auto [priority, index] = frontier.top();
        // On ne s'arrête pas à la première sortie de la destination : tant qu'une case de même
        // estimation totale reste en file, elle peut être le prédécesseur de même coût que le
        // départage retiendra. S'arrêter plus tôt rendrait un chemin juste, mais pas toujours
        // celui de `ReachableArea::pathTo`.
        if (costs[target] != UNREACHED && priority > costs[target]) {
            break;
        }
        frontier.pop();
        const GridPosition current{.column = static_cast<int>(index) % width,
                                   .row = static_cast<int>(index) / width};
        if (priority - estimate(current) > costs[index]) {
            continue;  // Entrée périmée.
        }
        for (std::size_t rank = 0; rank < NEIGHBOURS.size(); ++rank) {
            const auto [deltaColumn, deltaRow] = NEIGHBOURS[rank];
            const GridPosition next{.column = current.column + deltaColumn,
                                    .row = current.row + deltaRow};
            if (!grid.inBounds(next)) {
                continue;
            }
            const std::optional<int> step = rules.cost(current, next);
            if (!step.has_value()) {
                continue;
            }
            const std::size_t nextIndex = indexOf(next);
            if (relax(costs, predecessors, nextIndex, rank, costs[index] + *step)) {
                frontier.emplace(costs[nextIndex] + estimate(next), nextIndex);
            }
        }
    }

    if (costs[target] == UNREACHED) {
        return std::nullopt;
    }
    return rebuild(costs, predecessors, *start, destination, width);
}

}  // namespace core
