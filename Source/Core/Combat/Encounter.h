// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/Encounter.h
 * @brief Une **rencontre** : qui se dresse, et où, quand le combat s'engage (`LOT-18`,
 *        `EX-CBT-001`).
 */

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Levels/GridPosition.h"

namespace core {

/**
 * @brief Un combattant d'une rencontre : une créature du bestiaire, et sa case de départ.
 *
 * La position est **relative au déclencheur**, jamais absolue. Une rencontre est écrite une fois et
 * jouée partout où le déclencheur se trouve : deux gobelins « un pas devant, un pas à droite »
 * gardent leur formation quel que soit l'endroit de la carte, alors que des coordonnées absolues
 * feraient apparaître les mêmes gobelins au même endroit à chaque fois — ou hors de la carte.
 */
struct EncounterCombatant {
    /// Identifiant de créature du bestiaire (`LOT-33`).
    std::string creatureId;
    /// Décalage en cases depuis la case du déclencheur.
    int columnOffset = 0;
    int rowOffset = 0;
};

/**
 * @brief Une rencontre, telle que la donnée la décrit.
 *
 * Aucune position absolue, aucune carte nommée : une rencontre dit **qui** et **en quelle
 * formation**, pas **où**. C'est le déclencheur qui apporte le lieu.
 */
struct Encounter {
    std::string id;
    std::string name;
    std::string source;
    std::vector<EncounterCombatant> combatants;
    /**
     * @brief Vrai si le joueur peut fuir cette rencontre.
     *
     * Une rencontre dont on ne peut pas fuir est une décision de **contenu** — un combat de
     * scénario —, et elle est donc écrite dans la donnée plutôt que devinée d'un type d'ennemi.
     * Par défaut on fuit : l'inverse enfermerait le joueur dans toute rencontre qu'un auteur
     * aurait oublié de renseigner.
     */
    bool escapable = true;
};

/// @brief Le catalogue des rencontres, et ce qui n'a pas pu être lu.
struct EncounterCatalog {
    std::vector<Encounter> encounters;
    std::vector<std::string> errors;

    /// @brief La rencontre d'identifiant @p id, ou `nullptr` si elle est inconnue.
    [[nodiscard]] const Encounter* find(std::string_view id) const;
};

/// @brief Charge les rencontres depuis leur dossier (`Rpg/encounters/`).
[[nodiscard]] EncounterCatalog loadEncounters(const std::filesystem::path& encountersDir);

/// @brief Un combattant **placé** : sa créature, et sa case sur la carte.
struct CombatantPlacement {
    std::string creatureId;
    GridPosition position;
};

/**
 * @brief Place les combattants d'une rencontre autour de @p trigger.
 *
 * Fonction **pure** : aucune carte, aucun monde ECS, aucune collision. Ce qu'elle produit est une
 * liste de cases voulues — c'est au montage de la rencontre de refuser celles qui tombent dans un
 * mur, et il ne peut le faire que s'il les reçoit toutes.
 */
[[nodiscard]] std::vector<CombatantPlacement> placeCombatants(const Encounter& encounter,
                                                              GridPosition trigger);

}  // namespace core
