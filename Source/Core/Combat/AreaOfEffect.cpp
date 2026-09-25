// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/AreaOfEffect.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <set>

#include "Core/Combat/CombatState.h"
#include "Core/Rpg/Scale.h"

namespace core {
namespace {

// « Au moins la moitie » : une case couverte a exactement 50 % est dans la zone. La tolerance
// absorbe l'arrondi d'une racine de 2, jamais une vraie difference de surface.
constexpr double MOITIE = 0.5 - 1e-9;

struct Point {
    double x = 0.0;
    double y = 0.0;
};

using Polygone = std::vector<Point>;

// Decoupe un polygone convexe par le demi-plan ou `garder` est vrai (Sutherland-Hodgman).
template <typename Garder, typename Couper>
[[nodiscard]] Polygone decouper(const Polygone& polygone, Garder garder, Couper couper) {
    Polygone resultat;
    for (std::size_t i = 0; i < polygone.size(); ++i) {
        const Point& a = polygone[i];
        const Point& b = polygone[(i + 1) % polygone.size()];
        const bool dedansA = garder(a);
        const bool dedansB = garder(b);
        if (dedansA) {
            resultat.push_back(a);
        }
        if (dedansA != dedansB) {
            resultat.push_back(couper(a, b));
        }
    }
    return resultat;
}

// Surface d'un polygone convexe dans la case [x0, x0 + 1] x [y0, y0 + 1].
[[nodiscard]] double surfaceDansCase(Polygone polygone, double x0, double y0) {
    const auto surX = [](double bord) {
        return [bord](const Point& a, const Point& b) {
            const double t = (bord - a.x) / (b.x - a.x);
            return Point{.x = bord, .y = a.y + (t * (b.y - a.y))};
        };
    };
    const auto surY = [](double bord) {
        return [bord](const Point& a, const Point& b) {
            const double t = (bord - a.y) / (b.y - a.y);
            return Point{.x = a.x + (t * (b.x - a.x)), .y = bord};
        };
    };
    polygone = decouper(polygone, [x0](const Point& p) { return p.x >= x0; }, surX(x0));
    polygone = decouper(polygone, [x0](const Point& p) { return p.x <= x0 + 1.0; }, surX(x0 + 1.0));
    polygone = decouper(polygone, [y0](const Point& p) { return p.y >= y0; }, surY(y0));
    polygone = decouper(polygone, [y0](const Point& p) { return p.y <= y0 + 1.0; }, surY(y0 + 1.0));
    double doubleSurface = 0.0;
    for (std::size_t i = 0; i < polygone.size(); ++i) {
        const Point& a = polygone[i];
        const Point& b = polygone[(i + 1) % polygone.size()];
        doubleSurface += (a.x * b.y) - (b.x * a.y);
    }
    return std::abs(doubleSurface) / 2.0;
}

// Primitive de sqrt(r^2 - x^2).
[[nodiscard]] double primitiveDemiDisque(double x, double rayon) {
    const double borne = std::clamp(x / rayon, -1.0, 1.0);
    return 0.5 * ((x * std::sqrt(std::max(0.0, (rayon * rayon) - (x * x)))) +
                  (rayon * rayon * std::asin(borne)));
}

// Surface du disque de centre (0, 0) et de rayon @p rayon dans le rectangle [x0, x1] x [y0, y1].
//
// L'integrale sur x de la hauteur du disque tronquee au rectangle. Entre deux abscisses ou le
// cercle croise y0 ou y1, chaque borne est soit la droite, soit l'arc, et se primitive exactement.
[[nodiscard]] double surfaceDisqueRectangle(double rayon, double x0, double y0, double x1,
                                            double y1) {
    const double a = std::max(x0, -rayon);
    const double b = std::min(x1, rayon);
    if (a >= b) {
        return 0.0;
    }
    std::vector<double> bornes{a, b};
    for (const double y : {y0, y1}) {
        if (std::abs(y) < rayon) {
            const double q = std::sqrt((rayon * rayon) - (y * y));
            for (const double x : {-q, q}) {
                if (x > a && x < b) {
                    bornes.push_back(x);
                }
            }
        }
    }
    std::ranges::sort(bornes);
    double surface = 0.0;
    for (std::size_t i = 0; i + 1 < bornes.size(); ++i) {
        const double p = bornes[i];
        const double q = bornes[i + 1];
        if (q <= p) {
            continue;
        }
        const double milieu = (p + q) / 2.0;
        const double s = std::sqrt(std::max(0.0, (rayon * rayon) - (milieu * milieu)));
        const bool hautSurArc = s < y1;
        const bool basSurArc = -s > y0;
        if ((hautSurArc ? s : y1) <= (basSurArc ? -s : y0)) {
            continue;
        }
        const double arc = primitiveDemiDisque(q, rayon) - primitiveDemiDisque(p, rayon);
        surface += (hautSurArc ? arc : y1 * (q - p)) - (basSurArc ? -arc : y0 * (q - p));
    }
    return surface;
}

// La forme en polygone (cone, cube, ligne), en cases ; vide si elle n'a pas de direction.
[[nodiscard]] Polygone polygoneDe(const AreaOfEffect& zone) {
    const auto dx = static_cast<double>(zone.toward.x - zone.origin.x);
    const auto dy = static_cast<double>(zone.toward.y - zone.origin.y);
    const double norme = std::hypot(dx, dy);
    if (norme == 0.0 || zone.size <= 0) {
        return {};
    }
    const Point o{.x = zone.origin.x / 2.0, .y = zone.origin.y / 2.0};
    const Point u{.x = dx / norme, .y = dy / norme};
    const Point n{.x = -u.y, .y = u.x};
    const double longueur = zone.size;
    // Demi-largeur au bout de la forme : L/2 pour un cone (largeur = distance), L/2 pour un cube,
    // W/2 pour une ligne.
    const double demiLargeur =
        zone.shape == AreaShape::Line ? std::max(0, zone.width) / 2.0 : longueur / 2.0;
    if (demiLargeur <= 0.0) {
        return {};
    }
    const auto point = [&](double leLong, double enTravers) {
        return Point{.x = o.x + (leLong * u.x) + (enTravers * n.x),
                     .y = o.y + (leLong * u.y) + (enTravers * n.y)};
    };
    if (zone.shape == AreaShape::Cone) {
        return {o, point(longueur, demiLargeur), point(longueur, -demiLargeur)};
    }
    return {point(0.0, -demiLargeur), point(0.0, demiLargeur), point(longueur, demiLargeur),
            point(longueur, -demiLargeur)};
}

[[nodiscard]] bool estCirculaire(AreaShape forme) noexcept {
    return forme == AreaShape::Sphere || forme == AreaShape::Cylinder;
}

}  // namespace

std::optional<int> areaTilesFromMeters(float meters) {
    if (!(meters > 0.0F)) {
        return std::nullopt;
    }
    // 6 m / 1,5 = 4 exactement, mais 4,5 / 1,5 peut tomber a 2,9999 : la marge evite de perdre une
    // case sur un arrondi.
    const int cases = static_cast<int>(std::floor(tilesFromMeters(meters) + 1e-4F));
    return cases > 0 ? std::optional<int>(cases) : std::nullopt;
}

std::vector<GridPosition> areaTemplate(const AreaOfEffect& area, int columns, int rows) {
    std::vector<GridPosition> cases;
    if (area.size <= 0 || columns <= 0 || rows <= 0) {
        return cases;
    }
    const Polygone polygone = estCirculaire(area.shape) ? Polygone{} : polygoneDe(area);
    if (!estCirculaire(area.shape) && polygone.empty()) {
        return cases;
    }

    // La boite englobante de la forme, en cases.
    double minX = 0.0;
    double minY = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
    const double cx = area.origin.x / 2.0;
    const double cy = area.origin.y / 2.0;
    if (estCirculaire(area.shape)) {
        minX = cx - area.size;
        maxX = cx + area.size;
        minY = cy - area.size;
        maxY = cy + area.size;
    } else {
        minX = maxX = polygone.front().x;
        minY = maxY = polygone.front().y;
        for (const Point& p : polygone) {
            minX = std::min(minX, p.x);
            maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y);
            maxY = std::max(maxY, p.y);
        }
    }
    const int colonneMin = std::max(0, static_cast<int>(std::floor(minX)));
    const int colonneMax = std::min(columns - 1, static_cast<int>(std::ceil(maxX)));
    const int ligneMin = std::max(0, static_cast<int>(std::floor(minY)));
    const int ligneMax = std::min(rows - 1, static_cast<int>(std::ceil(maxY)));

    for (int ligne = ligneMin; ligne <= ligneMax; ++ligne) {
        for (int colonne = colonneMin; colonne <= colonneMax; ++colonne) {
            const double surface = estCirculaire(area.shape)
                                       ? surfaceDisqueRectangle(area.size, colonne - cx, ligne - cy,
                                                                colonne + 1 - cx, ligne + 1 - cy)
                                       : surfaceDansCase(polygone, colonne, ligne);
            if (surface >= MOITIE) {
                cases.push_back({.column = colonne, .row = ligne});
            }
        }
    }
    return cases;
}

std::vector<GridPosition> affectedCells(const BattleGrid& grid, const AreaOfEffect& area) {
    std::vector<GridPosition> atteintes;
    for (const GridPosition cellule : areaTemplate(area, grid.width(), grid.height())) {
        if (grid.blocksSight(cellule)) {
            continue;
        }
        const GridPoint coin = cornerOf(cellule);
        const std::array<GridPoint, 4> coins{{coin,
                                              {.x = coin.x + 2, .y = coin.y},
                                              {.x = coin.x, .y = coin.y + 2},
                                              {.x = coin.x + 2, .y = coin.y + 2}}};
        if (std::ranges::any_of(coins,
                                [&](GridPoint p) { return isSightClear(grid, area.origin, p); })) {
            atteintes.push_back(cellule);
        }
    }
    return atteintes;
}

std::vector<CombatantId> combatantsInArea(const CombatState& combat, const AreaOfEffect& area) {
    std::set<CombatantId> touches;
    for (const GridPosition cellule : affectedCells(combat.grid(), area)) {
        if (const std::optional<CombatantId> occupant = combat.grid().occupantAt(cellule)) {
            touches.insert(*occupant);
        }
    }
    return {touches.begin(), touches.end()};
}

}  // namespace core
