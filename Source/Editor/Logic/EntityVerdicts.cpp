// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/EntityVerdicts.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string_view>

#include "Editor/Logic/EditorDiagnostics.h"

namespace hmi {

namespace {

// `2`, `1.5`, `2.5` : un multiplicateur tel que le Guide l'ecrit.
[[nodiscard]] std::string multiplierText(double multiplier) {
    const long tenths = std::lround(multiplier * 10.0);
    std::string text = std::to_string(tenths / 10);
    if (tenths % 10 != 0) {
        text += "." + std::to_string(tenths % 10);
    }
    return text;
}

// Les cases que le deploiement marque : la formation (sur le terrain ou hors de lui), le groupe.
void markDeployment(const core::PartyDeployment& deployment,
                    const std::vector<core::MapEntity>& entities, std::vector<VerdictCell>& cells) {
    const std::optional<core::CombatZone> zone =
        deployment.zoneIndex && *deployment.zoneIndex < entities.size()
            ? std::make_optional(core::combatZoneOf(entities[*deployment.zoneIndex]))
            : std::nullopt;
    for (const core::CombatantPlacement& foe : deployment.formation) {
        const bool inside = zone && zone->contains(foe.position);
        cells.push_back(
            VerdictCell{.cell = foe.position,
                        .role = inside ? VerdictCellRole::Foe : VerdictCellRole::FoeOutside});
    }
    for (const core::GridPosition place : deployment.partyPlaces) {
        cells.push_back(VerdictCell{.cell = place, .role = VerdictCellRole::Party});
    }
}

void addIssueLines(const core::PartyDeployment& deployment,
                   const std::vector<core::MapEntity>& entities, std::vector<std::string>& lines) {
    for (const core::DeploymentIssue& issue : deployment.issues) {
        lines.push_back(deploymentIssueText(deployment, issue, entities));
    }
}

[[nodiscard]] EntityVerdict zoneVerdict(const core::CombatZoneTerrain& zone,
                                        const std::vector<core::PartyDeployment>& deployments,
                                        const std::vector<core::MapEntity>& entities) {
    EntityVerdict verdict{.entityIndex = zone.entityIndex,
                          .lines = {combatZoneSummary(zone)},
                          .ok = !zone.issue.has_value(),
                          .cells = {}};
    for (const core::GridPosition cell : zone.freeCells) {
        verdict.cells.push_back(VerdictCell{.cell = cell, .role = VerdictCellRole::Free});
    }
    for (const core::GridPosition cell : zone.blockedCells) {
        verdict.cells.push_back(VerdictCell{.cell = cell, .role = VerdictCellRole::Blocked});
    }
    const auto entries = [&](const std::vector<std::size_t>& indices, VerdictCellRole role) {
        for (const std::size_t entry : indices) {
            if (entry < entities.size()) {
                verdict.cells.push_back(
                    VerdictCell{.cell = entities[entry].position, .role = role});
            }
        }
    };
    entries(zone.entriesInside, VerdictCellRole::EntryInside);
    entries(zone.entriesOutside, VerdictCellRole::EntryOutside);
    for (const core::PartyDeployment& deployment : deployments) {
        if (deployment.zoneIndex != zone.entityIndex) {
            continue;
        }
        verdict.lines.push_back(deploymentSummary(deployment));
        addIssueLines(deployment, entities, verdict.lines);
        verdict.ok = verdict.ok && deployment.valid();
        markDeployment(deployment, entities, verdict.cells);
    }
    return verdict;
}

[[nodiscard]] EntityVerdict encounterVerdict(const core::PartyDeployment& deployment,
                                             const std::vector<core::MapEntity>& entities,
                                             const VerdictContext& context) {
    EntityVerdict verdict{.entityIndex = deployment.encounterIndex,
                          .lines = {},
                          .ok = deployment.valid(),
                          .cells = {}};
    const core::Encounter* const encounter =
        context.encounters != nullptr ? context.encounters->find(deployment.encounterId) : nullptr;
    if (encounter != nullptr && context.difficulty != nullptr && context.bestiary != nullptr &&
        context.difficulty->ok()) {
        const std::vector<int> levels(static_cast<std::size_t>(deployment.partySize),
                                      context.partyLevel);
        verdict.lines.push_back(encounterBudgetSummary(
            deployment.encounterId,
            core::rateEncounter(*context.difficulty, *encounter, *context.bestiary, levels),
            *context.difficulty, deployment.partySize, context.partyLevel));
    } else {
        verdict.lines.push_back(deployment.encounterId + ": no difficulty rules to rate it.");
    }
    verdict.lines.push_back(deploymentSummary(deployment));
    addIssueLines(deployment, entities, verdict.lines);
    markDeployment(deployment, entities, verdict.cells);
    return verdict;
}

}  // namespace

VerdictContext verdictContext(const EditorReferences& references, int partyLevel) {
    return VerdictContext{.encounters = &references.encounters,
                          .bestiary = &references.bestiary,
                          .difficulty = &references.difficulty,
                          .partyLevel = std::clamp(partyLevel, 1, 20)};
}

std::string encounterBudgetSummary(std::string_view encounterId,
                                   const core::EncounterBudget& budget,
                                   const core::EncounterDifficultyRules& rules, int partySize,
                                   int partyLevel) {
    std::string text = std::string{encounterId} + ": " + std::to_string(budget.adjustedExperience) +
                       " XP adjusted (" + std::to_string(budget.monsters) + " foes, " +
                       std::to_string(budget.monsterExperience) + " XP x " +
                       multiplierText(budget.multiplier) + "), " +
                       (budget.category.empty()
                            ? "below " + (rules.categories.empty() ? std::string{"any category"}
                                                                   : rules.categories.front())
                            : budget.category) +
                       " for " + std::to_string(partySize) + " of level " +
                       std::to_string(partyLevel) + " (";
    bool first = true;
    for (const std::string& category : rules.categories) {
        const auto threshold = budget.thresholds.find(category);
        if (threshold == budget.thresholds.end()) {
            continue;
        }
        text += (first ? "" : ", ") + category + " " + std::to_string(threshold->second);
        first = false;
    }
    text += ")";
    if (!budget.unknownCreatures.empty()) {
        text += ", unknown:";
        for (const std::string& creature : budget.unknownCreatures) {
            text += " " + creature;
        }
    }
    return text + ".";
}

std::string deploymentSummary(const core::PartyDeployment& deployment) {
    return deployment.encounterId + ": " + std::to_string(deployment.partySize) + " vs " +
           std::to_string(deployment.formation.size()) + ", party places " +
           std::to_string(deployment.partyPlaces.size()) + "/" +
           std::to_string(deployment.partySize) + ", " + std::to_string(deployment.reachableCells) +
           " free cells reached (" + std::to_string(deployment.requiredCells) + " required).";
}

std::vector<EntityVerdict> entityVerdicts(const core::TileMap& collision,
                                          const std::vector<core::MapEntity>& entities,
                                          const VerdictContext& context) {
    static const core::EncounterCatalog noEncounters;
    const std::vector<core::CombatZoneTerrain> zones =
        core::analyzeCombatZones(collision, entities);
    const std::vector<core::PartyDeployment> deployments = core::analyzePartyDeployment(
        collision, entities, context.encounters != nullptr ? *context.encounters : noEncounters,
        context.bestiary);

    std::vector<EntityVerdict> verdicts;
    verdicts.reserve(zones.size() + deployments.size());
    for (const core::CombatZoneTerrain& zone : zones) {
        verdicts.push_back(zoneVerdict(zone, deployments, entities));
    }
    for (const core::PartyDeployment& deployment : deployments) {
        verdicts.push_back(encounterVerdict(deployment, entities, context));
    }
    std::ranges::stable_sort(verdicts, {}, &EntityVerdict::entityIndex);
    return verdicts;
}

}  // namespace hmi
