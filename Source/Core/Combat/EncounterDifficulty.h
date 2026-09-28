// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/EncounterDifficulty.h
 * @brief Le **budget d'une rencontre** (`LOT-139`) : ce que pèse une rencontre à plusieurs
 *        adversaires pour un groupe de niveau donné, d'après le *Guide du Maître*.
 *
 * La méthode est celle de « La difficulté d'une rencontre de combat » (p. 82-83) : les seuils de
 * PX de chaque personnage, sommés par catégorie ; la somme des PX des monstres, multipliée selon
 * leur nombre ; le seuil inférieur le plus proche donne la catégorie. Les PX d'un monstre viennent
 * de son indice de dangerosité (p. 274). Tout est donnée (`Rpg/rules/encounter-difficulty.json`) :
 * aucun seuil n'est écrit ici (`EX-REG-021`).
 *
 * Ce que ce fichier ne fait pas : choisir les monstres. Il **juge** une rencontre écrite, pour
 * l'auteur de contenu (l'éditeur, `LOT-143`) et pour les tests qui vérifient qu'une rencontre
 * livrée est à la hauteur du groupe.
 */

#include <filesystem>
#include <map>
#include <span>
#include <string>
#include <vector>

#include "Core/Combat/Encounter.h"
#include "Core/Rpg/Bestiary.h"

namespace core {

/// @brief Les seuils de PX d'un personnage d'un niveau, par catégorie de difficulté.
struct DifficultyThresholds {
    int level = 1;
    /// Par catégorie, dans l'ordre de `EncounterDifficultyRules::categories`.
    std::map<std::string, int> experience;
};

/// @brief Le multiplicateur qui s'applique à partir de `minMonsters` monstres.
struct EncounterMultiplier {
    int minMonsters = 1;
    double multiplier = 1.0;
};

/// @brief Les PX que vaut un indice de dangerosité.
struct ChallengeExperience {
    float challengeRating = 0.0F;
    int experience = 0;
};

/**
 * @brief Les règles du budget, telles que `rules/encounter-difficulty.json` les déclare.
 */
struct EncounterDifficultyRules {
    /// Les catégories, de la plus facile à la plus dure (« facile », « moyenne », « difficile »,
    /// « mortelle »).
    std::vector<std::string> categories;
    /// Les seuils par niveau, du niveau 1 au niveau 20.
    std::vector<DifficultyThresholds> thresholds;
    /// Les multiplicateurs, croissants par nombre de monstres.
    std::vector<EncounterMultiplier> multipliers;
    /// Au-delà du dernier multiplicateur, pour un groupe de moins de trois.
    double smallPartyMultiplier = 1.0;
    /// En deçà du premier, pour un groupe de six ou plus.
    double largePartyMultiplier = 1.0;
    /// Les PX par indice.
    std::vector<ChallengeExperience> experienceByChallenge;
    /// Ce qui a manqué à la lecture ; vide si tout est là.
    std::vector<std::string> errors;

    /// @return Vrai si les règles portent au moins une catégorie et un seuil.
    [[nodiscard]] bool ok() const noexcept {
        return !categories.empty() && !thresholds.empty();
    }
};

/// @brief Charge `rules/encounter-difficulty.json` ; une règle illisible a ses `errors`.
[[nodiscard]] EncounterDifficultyRules loadEncounterDifficultyRules(
    const std::filesystem::path& path);

/// @return Les PX d'un monstre d'indice @p challengeRating ; 0 si la table ne le connaît pas.
[[nodiscard]] int experienceForChallenge(const EncounterDifficultyRules& rules,
                                         float challengeRating);

/**
 * @return Le multiplicateur d'une rencontre de @p monsters monstres pour un groupe de
 *         @p partySize personnages (p. 83) : celui de son nombre ; la catégorie supérieure pour
 *         moins de trois personnages, l'inférieure pour six ou plus.
 */
[[nodiscard]] double encounterMultiplierFor(const EncounterDifficultyRules& rules, int monsters,
                                            int partySize);

/// @return Les seuils du **groupe** : la somme, par catégorie, des seuils de chaque niveau de
///         @p partyLevels (p. 82, étape 2). Un niveau hors table compte pour le plus proche.
[[nodiscard]] std::map<std::string, int> partyThresholds(const EncounterDifficultyRules& rules,
                                                         std::span<const int> partyLevels);

/// @brief Le verdict sur une rencontre, pour un groupe.
struct EncounterBudget {
    /// La somme des PX des monstres, telle que le profil de chacun la donne.
    int monsterExperience = 0;
    /// Le nombre de monstres comptés (ceux que le bestiaire connaît).
    int monsters = 0;
    /// Le multiplicateur appliqué.
    double multiplier = 1.0;
    /// La valeur modifiée, comparée aux seuils.
    int adjustedExperience = 0;
    /// Les seuils du groupe, par catégorie.
    std::map<std::string, int> thresholds;
    /// La catégorie atteinte : la dernière dont le seuil est atteint ; vide si même le premier ne
    /// l'est pas — une rencontre en deçà de « facile ».
    std::string category;
    /// Les créatures que le bestiaire ne connaît pas : comptées pour 0, et dites.
    std::vector<std::string> unknownCreatures;
};

/**
 * @brief Juge @p encounter pour un groupe de niveaux @p partyLevels (p. 82-83, les cinq étapes).
 * @param rules       Les règles du budget.
 * @param encounter   La rencontre.
 * @param bestiary    Le bestiaire, pour l'indice de chaque créature.
 * @param partyLevels Le niveau de chaque membre du groupe ; vide : aucun seuil, catégorie vide.
 */
[[nodiscard]] EncounterBudget rateEncounter(const EncounterDifficultyRules& rules,
                                            const Encounter& encounter, const Bestiary& bestiary,
                                            std::span<const int> partyLevels);

}  // namespace core
