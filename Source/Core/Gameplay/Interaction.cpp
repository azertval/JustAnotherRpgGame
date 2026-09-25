// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Gameplay/Interaction.h"

#include <cmath>
#include <limits>

#include "Core/Gameplay/WorldFlags.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"

namespace core {

namespace {

// Le centre d'une case, en unites monde ou une case vaut 1. Sert au departage : la distance se
// mesure au CENTRE de la case visee, pas a son coin, sinon deux candidats symetriques autour du
// centre departageraient sur un arrondi.
[[nodiscard]] Vector2 centreDe(GridPosition cellule) {
    return {static_cast<float>(cellule.column) + 0.5F, static_cast<float>(cellule.row) + 0.5F};
}

[[nodiscard]] float distanceCarree(Vector2 gauche, Vector2 droite) {
    const float dx = gauche.x - droite.x;
    const float dy = gauche.y - droite.y;
    return (dx * dx) + (dy * dy);
}

// Vrai si l'on peut tendre la main de la case @p from a la case voisine @p to sans traverser de
// matiere. La case cible doit etre traversable ; en diagonale, deux murs qui se touchent par le
// coin ferment le passage, comme ils ferment la marche (`core::ExplorationReach`).
[[nodiscard]] bool atteignable(const TileMap& map, GridPosition from, GridPosition to) {
    const int dc = to.column - from.column;
    const int dr = to.row - from.row;
    if (std::abs(dc) > 1 || std::abs(dr) > 1) {
        return false;
    }
    if (!map.inBounds(to.column, to.row) || map.isSolid(to.column, to.row)) {
        return false;
    }
    if (dc != 0 && dr != 0) {
        const bool coteColonne =
            !map.inBounds(from.column + dc, from.row) || map.isSolid(from.column + dc, from.row);
        const bool coteLigne =
            !map.inBounds(from.column, from.row + dr) || map.isSolid(from.column, from.row + dr);
        if (coteColonne && coteLigne) {
            return false;
        }
    }
    return true;
}

}  // namespace

GridPosition aimedCell(GridPosition from, Vector2 facing) {
    const float horizontal = std::abs(facing.x);
    const float vertical = std::abs(facing.y);
    if (horizontal <= 0.0F && vertical <= 0.0F) {
        // Orientation nulle : le personnage ne vise rien. Rendre une voisine arbitraire ferait
        // ouvrir un coffre qu'il ne regarde pas.
        return from;
    }
    // La direction DOMINANTE, jamais une diagonale : un personnage qui regarde a 30 degres vise la
    // case de droite. Viser en diagonale rendrait la cible imprevisible a la manette analogique.
    // A egalite exacte, l'horizontale l'emporte -- il faut un depart, et celui-la est ecrit.
    if (horizontal >= vertical) {
        return {.column = from.column + (facing.x >= 0.0F ? 1 : -1), .row = from.row};
    }
    return {.column = from.column, .row = from.row + (facing.y >= 0.0F ? 1 : -1)};
}

InteractionTarget findInteractionTarget(Vector2 from, Vector2 facing, const TileMap& map,
                                        const std::vector<InteractionCandidate>& candidates,
                                        const WorldFlags& flags) {
    const GridPosition ici{.column = static_cast<int>(std::floor(from.x)),
                           .row = static_cast<int>(std::floor(from.y))};
    InteractionTarget resultat;
    resultat.aimedCell = aimedCell(ici, facing);

    bool meilleureVisee = false;
    float meilleure = std::numeric_limits<float>::max();
    for (const InteractionCandidate& candidat : candidates) {
        if (candidat.interactable == nullptr) {
            continue;
        }
        const GridPosition ou = candidat.interactable->position;
        const float distance = distanceCarree(centreDe(ou), from);
        if (distance >= INTERACTION_REACH_CELLS * INTERACTION_REACH_CELLS) {
            continue;
        }
        if (!atteignable(map, ici, ou)) {
            continue;
        }
        if (candidat.interactable->isConsumable() &&
            flags.isSet(candidat.interactable->consumedFlag)) {
            // Un coffre vide n'est plus une cible : continuer a l'afficher comme telle promettrait
            // au joueur quelque chose qui n'arrivera pas.
            continue;
        }
        // Ce que le heros regarde passe avant ce qui est plus pres : a deux PNJ a portee, il parle
        // a celui vers lequel il s'est tourne. Puis le plus proche ; a egalite STRICTE, le premier
        // l'emporte, donc le plus petit indice -- sans ce depart, l'ordre de parcours de l'ECS,
        // qui n'est pas stable, deciderait.
        const bool visee = ou == resultat.aimedCell;
        const bool mieux = resultat.interactable == nullptr || (visee && !meilleureVisee) ||
                           (visee == meilleureVisee && distance < meilleure);
        if (mieux) {
            meilleureVisee = visee;
            meilleure = distance;
            resultat.interactable = candidat.interactable;
            resultat.index = candidat.index;
        }
    }
    return resultat;
}

InteractionOutcome interact(const InteractionTarget& target, WorldFlags& flags) {
    InteractionOutcome resultat;
    if (!target.found()) {
        return resultat;
    }
    resultat.happened = true;
    resultat.type = target.interactable->type;
    resultat.promptKey = target.interactable->promptKey;
    if (target.interactable->isConsumable()) {
        // `set` dit si le fait etait NEUF. Demander d'abord puis ecrire ensuite laisserait entre
        // les deux appels une fenetre ou un second appel donnerait le butin une seconde fois.
        resultat.consumed = flags.set(target.interactable->consumedFlag);
    }
    return resultat;
}

}  // namespace core
