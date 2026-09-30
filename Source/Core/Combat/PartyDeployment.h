// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/PartyDeployment.h
 * @brief Une rencontre et le **groupe de quatre** sur leur zone de combat (`LOT-143`) : la
 *        formation adverse y tient-elle, le groupe y trouve-t-il ses places, et reste-t-il de quoi
 *        manœuvrer ?
 *
 * Le verdict de la zone (`core::analyzeCombatZones`, `LOT-EDITOR-05`) a été écrit pour un duel ;
 * celui du terrain (`core::analyzeEncounterTerrain`) regarde la formation sur la **carte**, sans la
 * borner à la zone que le combat découpe (`core::cropLevelToZone`). Depuis le `LOT-139`, le combat
 * se joue à quatre contre plusieurs, **sur la zone seule** : c'est là qu'il faut compter tout le
 * monde.
 *
 * ## La zone d'une rencontre
 *
 * Celle qui contient son marqueur, comme au jeu (`core::prepareMapEncounter`) ; à défaut, la plus
 * proche du marqueur — celle où se tiendra le groupe qui l'engage —, et le marqueur hors zone est
 * un défaut : sa formation, écrite autour de lui, tomberait hors du terrain.
 *
 * ## Les places du groupe
 *
 * Au jeu, le groupe entre là où il marche (`LOT-139`) : l'éditeur ne peut pas le savoir. Le
 * verdict lui cherche donc **quatre places** sur la zone, hors de la formation et atteignables
 * depuis elle : les entrées d'arène alliées de la zone d'abord (`core::ARENA_ENTRY_ENTITY_TYPE`),
 * puis, décision nommée, **le front opposé** au marqueur — la case libre la plus éloignée de lui,
 * et ses plus proches voisines. Un groupe qui y tient tient ailleurs sur la zone ; un groupe qui
 * n'y tient pas ne tient nulle part.
 *
 * ## Ce qu'elle ne redit pas
 *
 * Une zone dégénérée, débordante ou pleine est dite par `core::validateCombatZones` ; un
 * combattant dans un mur ou sur un autre, par `core::analyzeEncounterTerrain` ; une rencontre
 * inconnue, par la validation des entités. Le verdict ne le répète pas.
 */

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Core/Combat/Encounter.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileMap.h"

namespace core {

struct Bestiary;

/// @brief Ce qui empêche le groupe de se déployer face à une rencontre. Des codes, jamais du
///        texte (`EX-NFR-011`).
enum class DeploymentIssueCode {
    /// La carte ne déclare aucune zone de combat : la rencontre ne peut pas se jouer.
    NoCombatZone,
    /// Le marqueur de la rencontre est hors de sa zone.
    TriggerOutsideZone,
    /// L'emprise d'un adversaire sort de la zone.
    CombatantOutsideZone,
    /// Moins de places que de membres du groupe, hors de la formation et atteignables.
    PartyCannotDeploy,
    /// Les cases libres atteignables de la zone ne logent pas tout le monde.
    ZoneTooNarrow,
};

/// @brief Un défaut relevé : quoi, qui (vide pour le groupe ou la zone), et où.
struct DeploymentIssue {
    DeploymentIssueCode code;
    std::string creatureId;
    GridPosition cell;

    bool operator==(const DeploymentIssue&) const = default;
};

/// @brief Le verdict d'une rencontre et du groupe sur leur zone de combat.
struct PartyDeployment {
    /// Rang de l'entité `encounter` dans la liste d'entités de la carte.
    std::size_t encounterIndex = 0;
    /// Rang de l'entité `combatZone` où la rencontre se joue ; vide sans zone sur la carte.
    std::optional<std::size_t> zoneIndex;
    std::string encounterId;
    GridPosition trigger;
    /// Les cases voulues par la formation (`core::placeCombatants`), en cases de la carte.
    std::vector<CombatantPlacement> formation;
    /// Les places du groupe trouvées, en cases de la carte : au plus `partySize`.
    std::vector<GridPosition> partyPlaces;
    /// La taille du groupe comptée (`core::TACTICAL_PARTY_SIZE`).
    int partySize = 0;
    /// Les cases libres de la zone atteignables depuis le marqueur (ou la case libre la plus
    /// proche de lui).
    int reachableCells = 0;
    /// `(adversaires + groupe) × TACTICAL_CELLS_PER_COMBATANT`.
    int requiredCells = 0;
    /// Dans l'ordre : marqueur, adversaires, groupe, zone.
    std::vector<DeploymentIssue> issues;

    /// @return Vrai si le groupe se déploie face à toute la rencontre.
    [[nodiscard]] bool valid() const {
        return issues.empty();
    }
};

/**
 * @brief Le verdict de chaque rencontre de @p entities face au groupe de quatre, sur sa zone de
 *        combat, dans l'ordre des entités.
 *
 * Fonction **pure**, qui prend une grille et des entités comme `core::analyzeCombatZones` :
 * l'éditeur la calcule sur l'aperçu pendant qu'on tire la zone. Une rencontre absente de
 * @p encounters est ignorée ; une zone qui a son propre défaut ne donne ni places ni défaut.
 *
 * @param collision  La grille de collision de la carte.
 * @param entities   Les entités de la carte.
 * @param encounters Le catalogue des rencontres.
 * @param bestiary   Pour la taille des créatures, ou `nullptr` (taille M).
 */
[[nodiscard]] std::vector<PartyDeployment> analyzePartyDeployment(
    const TileMap& collision, const std::vector<MapEntity>& entities,
    const EncounterCatalog& encounters, const Bestiary* bestiary = nullptr);

}  // namespace core
