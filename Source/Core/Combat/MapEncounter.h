// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/MapEncounter.h
 * @brief Une rencontre **posée sur la carte** (`LOT-118`) : la zone de combat qui l'accueille, la
 *        carte réduite à cette zone, et où chacun se dresse dedans.
 *
 * ## Ce que ce fichier décide
 *
 * `core::beginEncounter` (`CombatTransition.h`) dit **qui** se bat et met l'exploration de côté ;
 * `core::cropLevelToZone` (`World/CombatZone.h`) découpe la carte. Entre les deux manque la
 * question du **lieu** : sur quelle zone de combat de la carte ce combat se joue, et comment les
 * cases voulues par la rencontre — écrites en cases de la carte — deviennent des cases de la grille
 * tactique, sans tomber dans un mur ni hors de la zone.
 *
 * La réponse est pure : une carte, une rencontre, deux cases (le déclencheur, le héros), et l'on
 * obtient une grille et des places. Aucune session n'est montée ici — c'est `hmi::EncounterModel`
 * qui le fait —, si bien que la règle se vérifie sans fenêtre (`EX-NFR-010`).
 */

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Combat/CombatTransition.h"
#include "Core/Combat/Encounter.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/Level.h"
#include "Core/World/CombatZone.h"

namespace core {

/**
 * @brief Une rencontre prête à monter sur la carte : la zone choisie, la grille, les places.
 *
 * Toutes les cases de cette structure sont en coordonnées de la **grille tactique** (l'origine de
 * la zone est la case (0, 0)) : c'est ce que la session de combat lit. Ce qui les dessine sur la
 * carte les ramène par `zone.origin`.
 */
struct MapEncounterSetup {
    /// La zone de combat de la carte, en cases de la carte.
    CombatZone zone;
    /// La carte réduite à la zone : la grille tactique.
    Level battlefield;
    /// Où le héros se dresse, en cases de la grille.
    GridPosition heroCell{};
    /// La rencontre engagée ; ses `placements` sont en cases de la grille.
    EncounterRun run;
    /// Ce qui a dû être déplacé pour tenir dans la zone, en clair : lu par le journal.
    std::vector<std::string> notes;
};

/// @brief Ce que la préparation a donné : un montage, ou la raison du refus.
struct MapEncounterResult {
    std::optional<MapEncounterSetup> setup;
    /// Vide si `setup` est là ; sinon, ce qui manque à la carte pour accueillir ce combat.
    std::string issue;

    [[nodiscard]] bool ok() const noexcept {
        return setup.has_value();
    }
};

/**
 * @brief Prépare @p encounter sur @p map, déclenchée en @p trigger, le héros en @p heroCell.
 *
 * ## La zone
 *
 * La zone de combat qui **contient le déclencheur** ; à défaut, celle qui contient le héros ; à
 * défaut, refus — un combat sur la carte se joue sur une zone que l'éditeur a posée et
 * contrôlée (`core::validateCombatZones`), jamais sur une fenêtre inventée autour du héros, qui
 * mettrait la moitié d'une maison dans la grille.
 *
 * ## Les places
 *
 * Le héros garde sa case si elle est dans la zone et libre ; sinon la case libre de la zone la
 * plus proche de lui. Chaque combattant de la rencontre garde la case que sa formation lui donne
 * autour du déclencheur (`core::placeCombatants`) si elle est dans la zone, libre et inoccupée ;
 * sinon la case libre la plus proche de celle qu'il voulait. Une place déplacée se note : le
 * journal du combat le dira, plutôt que de laisser croire que la formation a été respectée.
 *
 * @param map           La carte courante, entière.
 * @param mapId         Son identifiant : les clés de drapeau et les refus le citent.
 * @param encounter     La rencontre, telle que la donnée la décrit.
 * @param trigger       La case du déclencheur (le PNJ qui lance le combat, l'entité `encounter`).
 * @param heroCell      La case du héros au moment du déclenchement.
 * @param exploration   L'état d'exploration à retrouver ensuite.
 * @param defeatFlagKey Le drapeau qu'une victoire acquerra, ou une chaîne vide.
 */
[[nodiscard]] MapEncounterResult prepareMapEncounter(const Level& map, std::string_view mapId,
                                                     const Encounter& encounter,
                                                     GridPosition trigger, GridPosition heroCell,
                                                     const ExplorationSnapshot& exploration,
                                                     std::string defeatFlagKey);

/// @return @p cell, exprimée en cases de la carte depuis les cases de la grille de @p zone.
[[nodiscard]] GridPosition zoneToMap(const CombatZone& zone, GridPosition cell) noexcept;

/// @return @p cell, exprimée en cases de la grille de @p zone depuis les cases de la carte.
[[nodiscard]] GridPosition mapToZone(const CombatZone& zone, GridPosition cell) noexcept;

}  // namespace core
