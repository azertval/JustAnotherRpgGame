// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/LineOfSight.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "Core/Combat/CombatState.h"

namespace core {
namespace {

// Un parametre t du segment, en fraction exacte : num / den, den > 0.
struct Fraction {
    std::int64_t num = 0;
    std::int64_t den = 1;
};

[[nodiscard]] bool inferieur(Fraction a, Fraction b) noexcept {
    return a.num * b.den < b.num * a.den;
}

[[nodiscard]] bool egal(Fraction a, Fraction b) noexcept {
    return a.num * b.den == b.num * a.den;
}

// Releve la borne basse de l'intervalle des t si @p entree la depasse ; a egalite, la borne
// n'est stricte que si les deux l'etaient.
void resserrerBas(Fraction& bas, bool& basStrict, Fraction entree, bool fermee) noexcept {
    if (inferieur(bas, entree)) {
        bas = entree;
        basStrict = !fermee;
    } else if (egal(bas, entree)) {
        basStrict = basStrict || !fermee;
    }
}

// Abaisse la borne haute de l'intervalle des t si @p sortie passe dessous ; meme regle d'egalite.
void resserrerHaut(Fraction& haut, bool& hautStrict, Fraction sortie, bool fermee) noexcept {
    if (inferieur(sortie, haut)) {
        haut = sortie;
        hautStrict = !fermee;
    } else if (egal(sortie, haut)) {
        hautStrict = hautStrict || !fermee;
    }
}

// Vrai si le segment ]a, b[ rencontre la boite [x0, x1] x [y0, y1] (fermee) ou ]x0, x1[ x ]y0, y1[
// (ouverte). Les extremites du segment ne comptent jamais : un tir part d'un coin de sa propre
// case, et ce coin touche souvent un mur sans que le mur soit sur le chemin.
//
// Arithmetique entiere : chaque axe donne l'intervalle des t ou la coordonnee est dans la boite,
// et le segment la rencontre si l'intersection de ces intervalles avec ]0, 1[ n'est pas vide. Rien
// ne depend du sens de a vers b.
[[nodiscard]] bool rencontreBoite(GridPoint a, GridPoint b, int x0, int y0, int x1, int y1,
                                  bool fermee) noexcept {
    if (a == b) {
        return fermee ? (a.x >= x0 && a.x <= x1 && a.y >= y0 && a.y <= y1)
                      : (a.x > x0 && a.x < x1 && a.y > y0 && a.y < y1);
    }
    Fraction bas{.num = 0, .den = 1};
    Fraction haut{.num = 1, .den = 1};
    bool basStrict = true;
    bool hautStrict = true;
    const std::array<std::array<int, 4>, 2> axes{
        {{a.x, b.x - a.x, x0, x1}, {a.y, b.y - a.y, y0, y1}}};
    for (const auto& [p, d, lo, hi] : axes) {
        if (d == 0) {
            const bool dedans = fermee ? (p >= lo && p <= hi) : (p > lo && p < hi);
            if (!dedans) {
                return false;
            }
            continue;
        }
        const Fraction entree =
            d > 0 ? Fraction{.num = lo - p, .den = d} : Fraction{.num = p - hi, .den = -d};
        const Fraction sortie =
            d > 0 ? Fraction{.num = hi - p, .den = d} : Fraction{.num = p - lo, .den = -d};
        resserrerBas(bas, basStrict, entree, fermee);
        resserrerHaut(haut, hautStrict, sortie, fermee);
    }
    if (inferieur(bas, haut)) {
        return true;
    }
    return egal(bas, haut) && !basStrict && !hautStrict;
}

// Division entiere arrondie vers le bas, negatifs compris.
[[nodiscard]] constexpr int divBas(int a, int b) noexcept {
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

// Division entiere arrondie vers le haut.
[[nodiscard]] constexpr int divHaut(int a, int b) noexcept {
    return -divBas(-a, b);
}

// Les cases de la grille dont la boite fermee peut toucher le segment : la case c couvre
// [2c, 2c + 2], et touche [min, max] si 2c <= max et 2c + 2 >= min.
template <typename Visiteur>
bool pourChaqueCaseTouchee(const BattleGrid& grille, GridPoint a, GridPoint b,
                           const Visiteur& visiter) {
    const int colonneMin = std::max(0, divHaut(std::min(a.x, b.x) - 2, 2));
    const int colonneMax = std::min(grille.width() - 1, divBas(std::max(a.x, b.x), 2));
    const int ligneMin = std::max(0, divHaut(std::min(a.y, b.y) - 2, 2));
    const int ligneMax = std::min(grille.height() - 1, divBas(std::max(a.y, b.y), 2));
    for (int ligne = ligneMin; ligne <= ligneMax; ++ligne) {
        for (int colonne = colonneMin; colonne <= colonneMax; ++colonne) {
            if (visiter(GridPosition{.column = colonne, .row = ligne})) {
                return true;
            }
        }
    }
    return false;
}

// Tous les points de grille d'une emprise, bords et interieur.
[[nodiscard]] std::vector<GridPoint> pointsDe(Footprint emprise) {
    std::vector<GridPoint> points;
    const int cote = std::max(1, emprise.side);
    points.reserve(static_cast<std::size_t>(cote + 1) * static_cast<std::size_t>(cote + 1));
    for (int j = 0; j <= cote; ++j) {
        for (int i = 0; i <= cote; ++i) {
            points.push_back(
                {.x = 2 * (emprise.anchor.column + i), .y = 2 * (emprise.anchor.row + j)});
        }
    }
    return points;
}

// Les obstacles d'une famille : ce qui arrete la vue, ou un masque de cases qui font corps.
class Famille {
public:
    Famille(const BattleGrid& grille, Cover plafond) : _grille(grille), _plafond(plafond) {
        if (plafond != Cover::Total) {
            _masque.assign(static_cast<std::size_t>(grille.width()) *
                               static_cast<std::size_t>(grille.height()),
                           false);
        }
    }

    void marquer(GridPosition cellule) {
        if (_grille.inBounds(cellule)) {
            _masque[indice(cellule)] = true;
        }
    }

    [[nodiscard]] bool vide() const {
        return _plafond != Cover::Total && std::ranges::find(_masque, true) == _masque.end();
    }

    [[nodiscard]] Cover plafond() const noexcept {
        return _plafond;
    }

    [[nodiscard]] bool coupe(GridPoint a, GridPoint b) const {
        if (_plafond == Cover::Total) {
            return !isSightClear(_grille, a, b);
        }
        return pourChaqueCaseTouchee(_grille, a, b, [&](GridPosition cellule) {
            return _masque[indice(cellule)] &&
                   rencontreBoite(a, b, 2 * cellule.column, 2 * cellule.row,
                                  (2 * cellule.column) + 2, (2 * cellule.row) + 2, false);
        });
    }

private:
    [[nodiscard]] std::size_t indice(GridPosition cellule) const noexcept {
        return (static_cast<std::size_t>(cellule.row) * static_cast<std::size_t>(_grille.width())) +
               static_cast<std::size_t>(cellule.column);
    }

    const BattleGrid& _grille;
    Cover _plafond;
    std::vector<bool> _masque;
};

// Guide du Maitre : une ou deux lignes coupees, abri partiel ; trois, important ; quatre, total.
[[nodiscard]] constexpr Cover abriPourLignesCoupees(int coupees) noexcept {
    if (coupees <= 0) {
        return Cover::None;
    }
    if (coupees <= 2) {
        return Cover::Half;
    }
    return coupees == 3 ? Cover::ThreeQuarters : Cover::Total;
}

// Marque les cases dont l'objet abrite, chacune dans la famille de son niveau.
void marquerObjets(const BattleGrid& grille, Famille& important, Famille& partiel) {
    for (int ligne = 0; ligne < grille.height(); ++ligne) {
        for (int colonne = 0; colonne < grille.width(); ++colonne) {
            const GridPosition cellule{.column = colonne, .row = ligne};
            switch (grille.objectCoverAt(cellule)) {
                case Cover::ThreeQuarters:
                    important.marquer(cellule);
                    break;
                case Cover::Half:
                    partiel.marquer(cellule);
                    break;
                case Cover::None:
                case Cover::Total:
                    break;
            }
        }
    }
}

// Marque les cases des corps interposes : un abri partiel.
void marquerCorps(std::span<const Footprint> interposes, Famille& partiel) {
    for (const Footprint& corps : interposes) {
        for (int j = 0; j < corps.side; ++j) {
            for (int i = 0; i < corps.side; ++i) {
                partiel.marquer({.column = corps.anchor.column + i, .row = corps.anchor.row + j});
            }
        }
    }
}

// L'abri qu'une famille donne pour @p coupees lignes coupees sur quatre.
[[nodiscard]] Cover abriDeFamille(const Famille& famille, int coupees) {
    // Un mur abrite selon les lignes qu'il coupe ; un corps declare abrite de son
    // niveau des qu'il en coupe une.
    if (famille.plafond() == Cover::Total) {
        return abriPourLignesCoupees(coupees);
    }
    return coupees > 0 ? famille.plafond() : Cover::None;
}

// L'abri de la case dont le coin haut-gauche est (@p x, @p y), vue depuis @p origine : le
// meilleur abri des familles, jamais leur somme.
[[nodiscard]] Cover abriVersCase(const std::vector<const Famille*>& familles, GridPoint origine,
                                 int x, int y) {
    const std::array<GridPoint, 4> coins{
        {{.x = x, .y = y}, {.x = x + 2, .y = y}, {.x = x, .y = y + 2}, {.x = x + 2, .y = y + 2}}};
    Cover ici = Cover::None;
    for (const Famille* famille : familles) {
        int coupees = 0;
        for (const GridPoint coin : coins) {
            coupees += famille->coupe(origine, coin) ? 1 : 0;
        }
        ici = std::max(ici, abriDeFamille(*famille, coupees));
    }
    return ici;
}

[[nodiscard]] Cover abriDepuis(const BattleGrid& grille, const std::vector<GridPoint>& origines,
                               Footprint cible, std::span<const Footprint> interposes) {
    Famille vue(grille, Cover::Total);
    Famille important(grille, Cover::ThreeQuarters);
    Famille partiel(grille, Cover::Half);
    marquerObjets(grille, important, partiel);
    marquerCorps(interposes, partiel);
    std::vector<const Famille*> familles{&vue};
    for (const Famille* famille : {&important, &partiel}) {
        if (!famille->vide()) {
            familles.push_back(famille);
        }
    }

    // L'attaquant choisit le point et la case de la cible qui l'arrangent : le plus petit abri.
    // Pour chaque choix, le meilleur abri des familles compte, jamais leur somme.
    Cover meilleur = Cover::Total;
    const int cote = std::max(1, cible.side);
    for (const GridPoint origine : origines) {
        for (int j = 0; j < cote; ++j) {
            for (int i = 0; i < cote; ++i) {
                const Cover ici = abriVersCase(familles, origine, 2 * (cible.anchor.column + i),
                                               2 * (cible.anchor.row + j));
                meilleur = std::min(meilleur, ici);
                if (meilleur == Cover::None) {
                    return meilleur;
                }
            }
        }
    }
    return meilleur;
}

[[nodiscard]] std::optional<Footprint> empriseDe(const BattleGrid& grille, CombatantId combattant) {
    const std::optional<GridPosition> ancre = grille.positionOf(combattant);
    if (!ancre.has_value()) {
        return std::nullopt;
    }
    return Footprint{.anchor = *ancre, .side = grille.sideOf(combattant)};
}

}  // namespace

std::string_view coverLabel(Cover cover) noexcept {
    switch (cover) {
        case Cover::None:
            return "sans abri";
        case Cover::Half:
            return "abri partiel";
        case Cover::ThreeQuarters:
            return "abri important";
        case Cover::Total:
            return "abri total";
    }
    return "?";
}

bool isSightClear(const BattleGrid& grid, GridPoint a, GridPoint b) {
    // Une extremite posee sur un coin de cases, d'ou le segment part en diagonale entre deux cases
    // qui arretent la vue : le regard se glisserait par le coin commun de deux murs. Le test des
    // boites ignore les extremites, et c'est ici qu'on le rattrape. La condition ne depend que de
    // l'extremite et de la direction qui s'en eloigne : symetrique.
    for (const auto& [p, q] : {std::pair{a, b}, std::pair{b, a}}) {
        if (p.x % 2 != 0 || p.y % 2 != 0 || q.x == p.x || q.y == p.y) {
            continue;
        }
        const int colonneVers = q.x > p.x ? p.x / 2 : (p.x / 2) - 1;
        const int colonneAutre = q.x > p.x ? (p.x / 2) - 1 : p.x / 2;
        const int ligneVers = q.y > p.y ? p.y / 2 : (p.y / 2) - 1;
        const int ligneAutre = q.y > p.y ? (p.y / 2) - 1 : p.y / 2;
        if (grid.blocksSight({.column = colonneVers, .row = ligneAutre}) &&
            grid.blocksSight({.column = colonneAutre, .row = ligneVers})) {
            return false;
        }
    }
    return !pourChaqueCaseTouchee(grid, a, b, [&](GridPosition cellule) {
        return grid.blocksSight(cellule) &&
               rencontreBoite(a, b, 2 * cellule.column, 2 * cellule.row, (2 * cellule.column) + 2,
                              (2 * cellule.row) + 2, true);
    });
}

bool hasLineOfSight(const BattleGrid& grid, Footprint a, Footprint b) {
    const std::vector<GridPoint> depuis = pointsDe(a);
    const std::vector<GridPoint> vers = pointsDe(b);
    return std::ranges::any_of(depuis, [&](GridPoint p) {
        return std::ranges::any_of(vers, [&](GridPoint q) { return isSightClear(grid, p, q); });
    });
}

bool hasLineOfSight(const CombatState& combat, CombatantId a, CombatantId b) {
    const std::optional<Footprint> empriseA = empriseDe(combat.grid(), a);
    const std::optional<Footprint> empriseB = empriseDe(combat.grid(), b);
    return empriseA.has_value() && empriseB.has_value() &&
           hasLineOfSight(combat.grid(), *empriseA, *empriseB);
}

Cover coverFrom(const BattleGrid& grid, Footprint attacker, Footprint target,
                std::span<const Footprint> interposed) {
    return abriDepuis(grid, pointsDe(attacker), target, interposed);
}

Cover coverFromPoint(const BattleGrid& grid, GridPoint origin, Footprint target,
                     std::span<const Footprint> interposed) {
    return abriDepuis(grid, {origin}, target, interposed);
}

Cover coverBetween(const CombatState& combat, CombatantId attacker, CombatantId target) {
    const BattleGrid& grille = combat.grid();
    const std::optional<Footprint> tireur = empriseDe(grille, attacker);
    const std::optional<Footprint> cible = empriseDe(grille, target);
    if (!tireur.has_value() || !cible.has_value()) {
        return Cover::Total;
    }
    std::vector<Footprint> interposes;
    for (const CombatantId autre : grille.combatants()) {
        if (autre != attacker && autre != target) {
            if (const std::optional<Footprint> corps = empriseDe(grille, autre)) {
                interposes.push_back(*corps);
            }
        }
    }
    return coverFrom(grille, *tireur, *cible, interposes);
}

}  // namespace core
