// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/TacticalTerrain.h
 * @brief Une rencontre posée sur un terrain où l'on peut se battre — ou non (`LOT-11`,
 *        `EX-CBT-001`).
 */

#include <cstddef>
#include <string>
#include <vector>

#include "Core/Combat/Encounter.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileMap.h"

namespace core {

struct Bestiary;

/**
 * @brief Taille du groupe du joueur que la zone doit accueillir en plus de la rencontre.
 *
 * Quatre, le groupe pour lequel le Guide du Maître calibre ses rencontres. La carte ne sait pas qui
 * la traversera : l'auteur dimensionne pour un groupe complet, et un groupe plus petit n'en tiendra
 * que mieux. Décision nommée, à ajuster si le jeu fixe un autre effectif.
 */
inline constexpr int TACTICAL_PARTY_SIZE = 4;

/**
 * @brief Rayon de la zone de combat autour du déclencheur, en cases : un déplacement de 30 pieds.
 *
 * 9 m, la vitesse d'un combattant de taille M, font 6 cases (`core::movementBudget`) : la zone est
 * ce qu'un combattant ordinaire parcourt en **un tour** depuis le déclencheur. Plus large, une
 * clairière à trois tours de marche ferait passer un couloir pour un champ de bataille ; plus
 * étroite, le premier tour sortirait déjà de la zone vérifiée. Décision nommée, l'auteur peut la
 * régler.
 */
inline constexpr int TACTICAL_AREA_RADIUS = 6;

/**
 * @brief Cases libres exigées par combattant (rencontre + groupe).
 *
 * Une case pour se tenir, et de quoi manœuvrer — contourner, prendre en tenaille (`LOT-23`),
 * reculer. Quatre est un seuil d'auteur, pas une règle du Manuel : il laisse passer une salle de
 * 6 × 6 pour huit combattants et refuse un couloir d'une case de large. Décision nommée, à ajuster
 * quand les premières cartes l'éprouveront.
 */
inline constexpr int TACTICAL_CELLS_PER_COMBATANT = 4;

/// @brief Ce qui rend une rencontre injouable là où elle est posée. Des codes, jamais du texte
/// (`EX-NFR-011`) : c'est l'éditeur qui les dit à l'auteur.
enum class TacticalIssueCode {
    /// L'emprise d'un combattant sort de la carte.
    CombatantOutOfBounds,
    /// L'emprise d'un combattant touche un obstacle au sol.
    CombatantObstructed,
    /// L'emprise d'un combattant recouvre celle d'un combattant précédent.
    CombatantsOverlap,
    /// La zone atteignable autour du déclencheur ne loge pas tout le monde.
    AreaTooNarrow,
};

/// @brief Un problème relevé : quoi, qui (vide pour la zone), et où.
struct TacticalIssue {
    TacticalIssueCode code;
    std::string creatureId;
    GridPosition cell;

    bool operator==(const TacticalIssue&) const = default;
};

/// @brief Le verdict d'une rencontre posée sur la carte.
struct EncounterTerrain {
    /// Rang de l'entité dans la liste d'entités de la carte.
    std::size_t entityIndex = 0;
    std::string encounterId;
    GridPosition trigger;
    /// Les cases voulues par la formation (`core::placeCombatants`).
    std::vector<CombatantPlacement> placements;
    /// Cases libres atteignables depuis le déclencheur, déclencheur compris, triées (ligne,
    /// colonne).
    std::vector<GridPosition> area;
    /// `(combattants + TACTICAL_PARTY_SIZE) × TACTICAL_CELLS_PER_COMBATANT`.
    int requiredCells = 0;
    /// Dans l'ordre des combattants, puis la zone.
    std::vector<TacticalIssue> issues;

    /// @brief Vrai si aucun défaut n'a été relevé : le terrain accueille le combat tel quel.
    [[nodiscard]] bool valid() const {
        return issues.empty();
    }
};

/**
 * @brief Vérifie que chaque rencontre posée sur la carte tombe sur un terrain tactique valide.
 *
 * Le combat se joue **sur la carte d'exploration** : une rencontre posée dans un couloir d'une
 * case, ou dont la formation entre dans un mur, ne se découvrirait sinon qu'en jeu. Cette fonction
 * le dit à l'auteur au moment où il pose.
 *
 * ## Une seule règle pour « se tenir ici »
 *
 * Les combattants sont posés sur une `core::BattleGrid` construite depuis @p collision, par
 * `core::BattleGrid::place`, dans l'ordre de la formation — ce que fait le montage
 * (`core::mountEncounter`). Un avertissement de l'éditeur et un refus au montage ne peuvent donc
 * pas diverger. La zone est une `core::ReachableArea` : diagonales, coins de mur et terrain
 * difficile y suivent la règle du `LOT-19`, sans seconde implémentation.
 *
 * ## Ce qu'elle ne vérifie pas
 *
 * Une rencontre absente de @p encounters est **ignorée** : un autre validateur la signale, et la
 * signaler deux fois n'apprendrait rien. Une créature absente du bestiaire — ou un bestiaire
 * absent — compte pour une emprise de taille M. Le groupe du joueur n'est pas posé : ses cases ne
 * sont pas connues avant le jeu, et c'est la taille de la zone qui en répond.
 *
 * @param collision  La grille de collision de la carte.
 * @param entities   Les entités de la carte ; seules les `encounter` nommant une rencontre
 *                   comptent.
 * @param encounters Le catalogue des rencontres.
 * @param bestiary   Pour la taille des créatures, ou `nullptr`.
 * @return Un verdict par rencontre reconnue, dans l'ordre des entités.
 */
[[nodiscard]] std::vector<EncounterTerrain> analyzeEncounterTerrain(
    const TileMap& collision, const std::vector<MapEntity>& entities,
    const EncounterCatalog& encounters, const Bestiary* bestiary = nullptr);

}  // namespace core
