// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/Flanking.h"

#include <algorithm>
#include <utility>

namespace core {
namespace {

// Vrai si le segment p-q touche le segment vertical x = @p x, y dans [@p y0, @p y1], extremites
// comprises. Les points sont en demi-cases ; tout reste entier.
[[nodiscard]] bool toucheLeBordVertical(GridPoint p, GridPoint q, int x, int y0, int y1) noexcept {
    if (q.x < p.x) {
        std::swap(p, q);
    }
    const long long dx = q.x - p.x;
    if (dx == 0) {
        // Parallele au bord : un centre de case n'est jamais sur un bord, la ligne ne le touche
        // pas.
        return false;
    }
    const long long n = x - p.x;
    if (n < 0 || n > dx) {
        return false;
    }
    const long long y = (static_cast<long long>(p.y) * dx) + (n * (q.y - p.y));
    return y >= static_cast<long long>(y0) * dx && y <= static_cast<long long>(y1) * dx;
}

[[nodiscard]] GridPoint transpose(GridPoint point) noexcept {
    return {.x = point.y, .y = point.x};
}

[[nodiscard]] int ecart(GridPosition a, int coteA, Footprint b) noexcept {
    const int dx = std::max(
        {0, b.anchor.column - (a.column + coteA - 1), a.column - (b.anchor.column + b.side - 1)});
    const int dy =
        std::max({0, b.anchor.row - (a.row + coteA - 1), a.row - (b.anchor.row + b.side - 1)});
    return std::max(dx, dy);
}

// Deux emprises alignees, par une case de chacune, sur deux cotes opposes de la cible.
[[nodiscard]] bool alignees(Footprint a, Footprint b, Footprint cible) noexcept {
    for (int ya = 0; ya < a.side; ++ya) {
        for (int xa = 0; xa < a.side; ++xa) {
            for (int yb = 0; yb < b.side; ++yb) {
                for (int xb = 0; xb < b.side; ++xb) {
                    if (crossesOppositeSides(
                            {.column = a.anchor.column + xa, .row = a.anchor.row + ya},
                            {.column = b.anchor.column + xb, .row = b.anchor.row + yb}, cible)) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

}  // namespace

bool crossesOppositeSides(GridPosition a, GridPosition b, Footprint target) noexcept {
    const GridPoint p = centerOf(a);
    const GridPoint q = centerOf(b);
    const int x0 = 2 * target.anchor.column;
    const int x1 = 2 * (target.anchor.column + target.side);
    const int y0 = 2 * target.anchor.row;
    const int y1 = 2 * (target.anchor.row + target.side);
    const bool gaucheEtDroite =
        toucheLeBordVertical(p, q, x0, y0, y1) && toucheLeBordVertical(p, q, x1, y0, y1);
    const bool hautEtBas = toucheLeBordVertical(transpose(p), transpose(q), y0, x0, x1) &&
                           toucheLeBordVertical(transpose(p), transpose(q), y1, x0, x1);
    return gaucheEtDroite || hautEtBas;
}

bool isFlankedFrom(const CombatState& combat, CombatantId attacker, GridPosition attackerAnchor,
                   CombatantId target) {
    const Combatant* assaillant = combat.find(attacker);
    const Combatant* cible = combat.find(target);
    const std::optional<GridPosition> ancreCible = combat.grid().positionOf(target);
    if (assaillant == nullptr || cible == nullptr || !ancreCible.has_value() ||
        attacker == target || assaillant->status != CombatantStatus::Standing) {
        return false;
    }
    const BattleGrid& grille = combat.grid();
    const Footprint emplacement{.anchor = *ancreCible, .side = footprintSide(cible->profile.size)};
    const Footprint moi{.anchor = attackerAnchor, .side = footprintSide(assaillant->profile.size)};
    if (ecart(moi.anchor, moi.side, emplacement) != 1 ||
        !hasLineOfSight(grille, moi, emplacement)) {
        return false;
    }
    const auto prendEnTenaille = [&](const CombatantId autre) {
        const Combatant* allie = combat.find(autre);
        const std::optional<GridPosition> ancre = grille.positionOf(autre);
        if (autre == attacker || autre == target || allie == nullptr || !ancre.has_value() ||
            allie->status != CombatantStatus::Standing ||
            allie->profile.side != assaillant->profile.side) {
            return false;
        }
        const Footprint lui{.anchor = *ancre, .side = footprintSide(allie->profile.size)};
        return ecart(lui.anchor, lui.side, emplacement) == 1 &&
               hasLineOfSight(grille, lui, emplacement) && alignees(moi, lui, emplacement);
    };
    return std::ranges::any_of(combat.combatants(), prendEnTenaille);
}

bool isFlanked(const CombatState& combat, CombatantId attacker, CombatantId target) {
    const std::optional<GridPosition> ancre = combat.grid().positionOf(attacker);
    return ancre.has_value() && isFlankedFrom(combat, attacker, *ancre, target);
}

}  // namespace core
