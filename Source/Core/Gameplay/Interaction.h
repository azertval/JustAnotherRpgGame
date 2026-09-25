// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Gameplay/Interaction.h
 * @brief Désignation et résolution de la cible d'interaction (`LOT-10`).
 */

#include <string>
#include <string_view>
#include <vector>

#include "Core/Ecs/Components/Interactable.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Math/Vector2.h"

namespace core {

class TileMap;
class WorldFlags;

/**
 * @brief Une cible candidate : son composant et l'entité qui la porte.
 *
 * L'indice plutôt que `core::Entity` : la désignation est une fonction **pure** sur une liste, et
 * lui passer le monde entier la rendrait intestable sans ECS. L'appelant retrouve son entité.
 */
struct InteractionCandidate {
    const Interactable* interactable = nullptr;
    /// Indice libre, rendu tel quel dans le résultat. L'appelant y met ce qu'il veut.
    std::size_t index = 0;
};

/// @brief Ce que la désignation a trouvé.
struct InteractionTarget {
    const Interactable* interactable = nullptr;
    std::size_t index = 0;
    /// La case visée, que la cible existe ou non — l'invite visuelle en a besoin.
    GridPosition aimedCell;
    /// Vrai si une cible a été désignée.
    [[nodiscard]] bool found() const {
        return interactable != nullptr;
    }
};

/// @brief Portée de l'interaction, en cases, mesurée de la position du personnage au centre de la
///        case de la cible (strictement moins).
inline constexpr float INTERACTION_REACH_CELLS = 1.5F;

/**
 * @brief La case que vise un personnage, d'après son orientation.
 *
 * L'orientation est un vecteur libre (`core::Actor::facing`) et la case visée est **la voisine**
 * dans la direction dominante — jamais une diagonale. Un personnage qui regarde à 30° vise la case
 * de droite, pas celle du coin : viser en diagonale rendrait la cible imprévisible sur une manette
 * analogique, alors que le joueur doit savoir ce qu'il désigne avant d'appuyer.
 *
 * Une orientation nulle ne vise rien et renvoie la case du personnage lui-même : c'est le cas d'un
 * `facing` non initialisé, et rendre une voisine arbitraire ferait ouvrir un coffre que le joueur
 * ne regarde pas.
 */
[[nodiscard]] GridPosition aimedCell(GridPosition from, Vector2 facing);

/**
 * @brief Désigne la cible d'interaction d'un personnage.
 *
 * Quatre règles :
 *
 * 1. **La cible est à portée** : le centre de sa case est à moins de
 *    `INTERACTION_REACH_CELLS` (1,5 case) de la position **continue** du personnage. Le héros ne
 *    marche pas de case en case ; exiger qu'il regarde exactement la case voisine rendait
 *    l'abord d'un PNJ tatillon (retour de l'auteur, démo 0.0.1). À 1,5 case, les huit voisines
 *    sont à portée, diagonales comprises, et rien au-delà.
 * 2. **L'interaction ne traverse pas un mur** : la case de la cible doit être traversable, et en
 *    diagonale deux murs qui se touchent par le coin ferment le passage, comme pour la marche.
 *    Un coffre derrière un mur se voit et ne s'ouvre pas.
 * 3. **Ce que le personnage regarde passe d'abord** : une cible sur la case visée
 *    (`core::aimedCell`) l'emporte sur une cible plus proche ailleurs. À deux PNJ à portée, on
 *    parle à celui vers lequel on s'est tourné.
 * 4. **Le reste est déterministe** : puis la plus proche du personnage, et à distance égale le
 *    plus petit indice. Sans départage, deux coffres empilés donneraient tantôt l'un tantôt
 *    l'autre selon l'ordre de parcours de l'ECS, qui n'est pas stable.
 *
 * Une cible dont le drapeau de consommation est déjà levé n'est **pas** retenue : un coffre vidé
 * n'est plus une cible, et continuer à l'afficher comme telle promettrait au joueur quelque chose
 * qui n'arrivera pas.
 *
 * @param from Position continue du personnage, en cases (le centre d'une case est à +0,5).
 * @param facing Orientation du personnage.
 * @param map La carte, pour la règle du mur.
 * @param candidates Les entités interactives de la carte.
 * @param flags Les drapeaux de monde, pour écarter ce qui est déjà consommé.
 */
[[nodiscard]] InteractionTarget findInteractionTarget(
    Vector2 from, Vector2 facing, const TileMap& map,
    const std::vector<InteractionCandidate>& candidates, const WorldFlags& flags);

/// @brief Ce qu'une interaction a produit.
struct InteractionOutcome {
    /// Vrai si l'interaction a eu lieu.
    bool happened = false;
    /// Vrai si elle vient d'être consommée — c'est la première et la dernière fois.
    bool consumed = false;
    /// Type de l'entité sollicitée, pour que l'appelant sache quoi jouer.
    std::string type;
    /// Clé de localisation à afficher, si l'entité en porte une.
    std::string promptKey;
};

/**
 * @brief Résout une interaction sur une cible désignée, en levant son drapeau s'il y a lieu.
 *
 * **C'est ici que « deux fois ne donne qu'une fois » se joue.** Le drapeau est levé et sa valeur
 * de retour dit s'il était neuf ; l'appelant n'a pas à demander d'abord puis à écrire ensuite,
 * séquence dans laquelle un second appel pourrait se glisser.
 */
InteractionOutcome interact(const InteractionTarget& target, WorldFlags& flags);

}  // namespace core
