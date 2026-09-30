// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Core/Combat/EncounterDifficulty.h"
#include "Core/Combat/PartyDeployment.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileMap.h"
#include "Core/World/CombatZone.h"
#include "Editor/Logic/EntityReferences.h"

/**
 * @file Editor/Logic/EntityVerdicts.h
 * @brief Ce que le canevas écrit **à côté d'une entité** : le verdict d'une zone de combat, pour
 *        un groupe de quatre (`LOT-EDITOR-05`, `LOT-143`), et le budget d'une rencontre pour un
 *        groupe de niveau donné (`LOT-139`).
 *
 * Le canevas, l'inspecteur et `--apply` ne connaissent qu'un `EntityVerdict` : des lignes, des
 * cases marquées d'un rôle, un « bon ou pas ». Aucun d'eux ne regarde le type d'une entité — les
 * familles qui ont un verdict se décident ici, une fois (règle « aucun code par famille dans
 * l'éditeur »).
 */

namespace hmi {

/// @brief Ce qu'une case marquée par un verdict représente, donc sa couleur.
enum class VerdictCellRole {
    /// Une case libre de la zone.
    Free,
    /// Une case pleine de la zone.
    Blocked,
    /// Une entrée d'arène que le combat voit.
    EntryInside,
    /// Une entrée d'arène qu'il ne voit pas.
    EntryOutside,
    /// Une place du groupe.
    Party,
    /// La case voulue d'un adversaire, sur le terrain.
    Foe,
    /// La case voulue d'un adversaire, hors du terrain.
    FoeOutside,
};

/// @brief Une case marquée par un verdict.
struct VerdictCell {
    core::GridPosition cell;
    VerdictCellRole role = VerdictCellRole::Free;

    [[nodiscard]] bool operator==(const VerdictCell&) const = default;
};

/// @brief Le verdict d'une entité, en anglais comme tout l'éditeur (`LOT-EDITOR-01`).
struct EntityVerdict {
    std::size_t entityIndex = 0;
    /// La première ligne s'écrit à côté de l'entité ; toutes, quand elle est sélectionnée.
    std::vector<std::string> lines;
    /// Faux : le verdict s'écrit en rouge.
    bool ok = true;
    /// Les cases que le canevas marque quand l'entité est sélectionnée.
    std::vector<VerdictCell> cells;
};

/// @brief Ce qu'il faut pour juger : les catalogues, et le niveau du groupe du budget.
struct VerdictContext {
    const core::EncounterCatalog* encounters = nullptr;
    const core::Bestiary* bestiary = nullptr;
    const core::EncounterDifficultyRules* difficulty = nullptr;
    /// Le niveau des quatre membres du groupe, de 1 à 20.
    int partyLevel = 1;
};

/// @return Le contexte de @p references, au niveau @p partyLevel.
[[nodiscard]] VerdictContext verdictContext(const EditorReferences& references, int partyLevel);

/**
 * @brief Le budget d'une rencontre en une ligne.
 *
 * Par exemple : `arene-bandits: 300 XP adjusted (6 foes, 150 XP x 2), difficile for 4 of level 1
 * (facile 100, moyenne 200, difficile 300, mortelle 400).` Les catégories se nomment comme la
 * donnée les déclare.
 */
[[nodiscard]] std::string encounterBudgetSummary(std::string_view encounterId,
                                                 const core::EncounterBudget& budget,
                                                 const core::EncounterDifficultyRules& rules,
                                                 int partySize, int partyLevel);

/**
 * @brief Le déploiement d'une rencontre en une ligne : l'effectif, les places du groupe, les cases.
 *
 * Par exemple : `arene-bandits: 4 vs 6, party places 4/4, 290 free cells reached (40 required).`
 */
[[nodiscard]] std::string deploymentSummary(const core::PartyDeployment& deployment);

/**
 * @brief Le verdict de chaque entité qui en a un, dans l'ordre des entités : les zones de combat
 *        (leur terrain, et chaque rencontre qui s'y joue face au groupe), les rencontres (leur
 *        budget, et leur déploiement).
 */
[[nodiscard]] std::vector<EntityVerdict> entityVerdicts(const core::TileMap& collision,
                                                        const std::vector<core::MapEntity>& entities,
                                                        const VerdictContext& context);

}  // namespace hmi
