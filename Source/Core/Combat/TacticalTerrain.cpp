// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/TacticalTerrain.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>

#include "Core/Combat/BattleGrid.h"
#include "Core/Combat/CombatTransition.h"
#include "Core/Combat/Pathfinding.h"
#include "Core/Rpg/Bestiary.h"

namespace core {

namespace {

// Le combattant fictif qui arpente la zone : seul sur sa grille, son identifiant importe peu.
constexpr CombatantId ARPENTEUR{1};

[[nodiscard]] CreatureSize tailleDe(const std::string& creatureId, const Bestiary* bestiary) {
    if (bestiary == nullptr) {
        return CreatureSize::Medium;
    }
    const Creature* creature = bestiary->find(creatureId);
    return creature == nullptr ? CreatureSize::Medium : creature->size;
}

// Le refus de la grille, traduit en problème d'auteur. `InvalidCombatant` n'en est pas un : il
// ne survient pas pour un identifiant neuf et une taille du bestiaire.
[[nodiscard]] std::optional<TacticalIssueCode> problemeDe(PlacementResult result) {
    switch (result) {
        case PlacementResult::OutOfBounds:
            return TacticalIssueCode::CombatantOutOfBounds;
        case PlacementResult::Obstructed:
            return TacticalIssueCode::CombatantObstructed;
        case PlacementResult::Occupied:
            return TacticalIssueCode::CombatantsOverlap;
        case PlacementResult::Placed:
        case PlacementResult::InvalidCombatant:
            return std::nullopt;
    }
    return std::nullopt;
}

// Les cases où un marcheur de taille M, parti du déclencheur, peut finir un tour de 30 pieds —
// le déclencheur compris. Vide si le déclencheur lui-même ne se tient pas.
[[nodiscard]] std::vector<GridPosition> zoneAtteignable(const TileMap& collision,
                                                        GridPosition trigger) {
    BattleGrid grille(collision);
    if (grille.place(ARPENTEUR, trigger) != PlacementResult::Placed) {
        return {};
    }
    const ReachableArea aire(grille, Mover{.combatant = ARPENTEUR, .canPassThrough = {}},
                             TACTICAL_AREA_RADIUS);
    std::vector<GridPosition> zone = aire.destinations();
    zone.push_back(trigger);
    std::ranges::sort(zone, [](GridPosition a, GridPosition b) {
        return a.row != b.row ? a.row < b.row : a.column < b.column;
    });
    return zone;
}

}  // namespace

std::vector<EncounterTerrain> analyzeEncounterTerrain(const TileMap& collision,
                                                      const std::vector<MapEntity>& entities,
                                                      const EncounterCatalog& encounters,
                                                      const Bestiary* bestiary) {
    std::vector<EncounterTerrain> verdicts;
    for (std::size_t index = 0; index < entities.size(); ++index) {
        // Le nom de carte n'ouvre que la clé de drapeau, dont cette vérification n'a pas l'usage.
        const std::optional<EncounterTrigger> declencheur =
            encounterTriggerFor(entities[index], "");
        if (!declencheur.has_value()) {
            continue;
        }
        const Encounter* rencontre = encounters.find(declencheur->encounterId);
        if (rencontre == nullptr) {
            continue;
        }

        EncounterTerrain verdict;
        verdict.entityIndex = index;
        verdict.encounterId = declencheur->encounterId;
        verdict.trigger = declencheur->position;
        verdict.placements = placeCombatants(*rencontre, declencheur->position);

        // La même grille, la même pose que le montage : chaque combattant posé gêne les suivants,
        // et c'est donc le second de deux combattants superposés qui est signalé.
        BattleGrid grille(collision);
        std::uint32_t suivant = 1;
        for (const CombatantPlacement& placement : verdict.placements) {
            const PlacementResult pose =
                grille.place(CombatantId{suivant++}, placement.position,
                             footprintSide(tailleDe(placement.creatureId, bestiary)));
            if (const auto probleme = problemeDe(pose); probleme.has_value()) {
                verdict.issues.push_back({.code = *probleme,
                                          .creatureId = placement.creatureId,
                                          .cell = placement.position});
            }
        }

        verdict.area = zoneAtteignable(collision, verdict.trigger);
        verdict.requiredCells =
            (static_cast<int>(verdict.placements.size()) + TACTICAL_PARTY_SIZE) *
            TACTICAL_CELLS_PER_COMBATANT;
        if (std::cmp_less(verdict.area.size(), verdict.requiredCells)) {
            verdict.issues.push_back({.code = TacticalIssueCode::AreaTooNarrow,
                                      .creatureId = {},
                                      .cell = verdict.trigger});
        }
        verdicts.push_back(std::move(verdict));
    }
    return verdicts;
}

}  // namespace core
