// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/EditorDiagnostics.h"

#include <algorithm>

namespace hmi {

namespace {

// Remplace `%1`, `%2`, `%3` de @p pattern par les arguments, dans l'ordre.
[[nodiscard]] std::string format(std::string pattern, const std::vector<std::string>& args) {
    for (std::size_t index = 0; index < args.size(); ++index) {
        const std::string marker = "%" + std::to_string(index + 1);
        for (std::size_t at = pattern.find(marker); at != std::string::npos;
             at = pattern.find(marker, at + args[index].size())) {
            pattern.replace(at, marker.size(), args[index]);
        }
    }
    return pattern;
}

}  // namespace

const char* entityIssueTemplate(core::EntityIssueCode code) noexcept {
    switch (code) {
        case core::EntityIssueCode::UnknownType:
            return R"(Entity kind "%1" is unknown to the editor.)";
        case core::EntityIssueCode::MissingProperty:
            return R"(%1: property "%2" must be filled in.)";
        case core::EntityIssueCode::WrongValueType:
            return R"(%1: property "%2" has the wrong value type.)";
        case core::EntityIssueCode::InvalidChoice:
            return R"(%1: "%3" is not an allowed value for "%2".)";
        case core::EntityIssueCode::UnknownDialogue:
            return R"(%1: dialogue "%3" does not exist, or was rejected when loading.)";
        case core::EntityIssueCode::UnknownEncounter:
            return R"(%1: encounter "%3" does not exist.)";
        case core::EntityIssueCode::UnknownTargetMap:
            return R"(%1: map "%3" does not exist.)";
        case core::EntityIssueCode::UnknownArrivalPoint:
            return R"(%1: the target map has no arrival point "%3".)";
        case core::EntityIssueCode::DuplicateArrivalPoint:
            return R"(Two arrival points are named "%3".)";
        case core::EntityIssueCode::OutOfRange:
            return R"(%1: %3 is out of range for "%2".)";
        case core::EntityIssueCode::UnknownFigure:
            return R"(%1: figure "%3" is in no workshop.)";
        case core::EntityIssueCode::UnsetFlag:
            return R"(%1: no dialogue sets flag "%3".)";
        case core::EntityIssueCode::UnknownLocation:
            return R"(%1: location "%3" is not in the atlas.)";
        case core::EntityIssueCode::UnknownItem:
            return R"(%1: item "%3" does not exist.)";
        case core::EntityIssueCode::UnknownEntityRef:
            return R"(%1: no map has an entity "%3".)";
    }
    return R"(Entity kind "%1" is unknown to the editor.)";
}

const char* tacticalIssueTemplate(core::TacticalIssueCode code) noexcept {
    switch (code) {
        case core::TacticalIssueCode::CombatantOutOfBounds:
            return R"(Encounter "%1": "%2" would stand off the map.)";
        case core::TacticalIssueCode::CombatantObstructed:
            return R"(Encounter "%1": "%2" would stand on an obstacle.)";
        case core::TacticalIssueCode::CombatantsOverlap:
            return R"(Encounter "%1": "%2" would overlap another combatant.)";
        case core::TacticalIssueCode::AreaTooNarrow:
            return R"(Encounter "%1": area too narrow to fight in (%2 free cells, %3 required).)";
    }
    return R"(Encounter "%1": area too narrow to fight in (%2 free cells, %3 required).)";
}

namespace {

[[nodiscard]] const char* combatZoneTemplate(core::WorldIssueCode code) noexcept {
    switch (code) {
        case core::WorldIssueCode::CombatZoneDegenerate:
            return R"(Combat zone "%1" has no width or no height.)";
        case core::WorldIssueCode::CombatZoneOutOfBounds:
            return R"(Combat zone "%1" overflows the map.)";
        case core::WorldIssueCode::CombatZoneBlocked:
            return R"(Combat zone "%1" has no free cell: nobody can stand in it.)";
        case core::WorldIssueCode::UnreadableMap:
        case core::WorldIssueCode::MissingTargetMap:
        case core::WorldIssueCode::UnknownTargetMap:
        case core::WorldIssueCode::UnreadableTargetMap:
        case core::WorldIssueCode::MissingArrivalPoint:
        case core::WorldIssueCode::UnknownArrivalPoint:
        case core::WorldIssueCode::DuplicateArrivalPoint:
            break;
    }
    return R"(Combat zone "%1" cannot be played.)";
}

}  // namespace

std::string combatZoneSummary(const core::CombatZoneTerrain& zone) {
    if (zone.issue) {
        return format(combatZoneTemplate(*zone.issue), {zone.zone.name});
    }
    return format(
        "%1: %2 x %3, %4 free cells of %5, %6 arena entries inside, %7 outside.",
        {zone.zone.name, std::to_string(zone.zone.columns), std::to_string(zone.zone.rows),
         std::to_string(zone.freeCells.size()),
         std::to_string(zone.freeCells.size() + zone.blockedCells.size()),
         std::to_string(zone.entriesInside.size()), std::to_string(zone.entriesOutside.size())});
}

std::vector<EditorDiagnostic> editorDiagnostics(const std::vector<core::MapEntity>& entities,
                                                const std::vector<core::EntityIssue>& issues,
                                                const std::vector<core::EncounterTerrain>& terrains,
                                                const std::vector<core::CombatZoneTerrain>& zones) {
    std::vector<EditorDiagnostic> lines;
    for (const core::EntityIssue& issue : issues) {
        if (issue.entityIndex >= entities.size()) {
            continue;  // rapport perime : l'entite a disparu entre la validation et l'affichage.
        }
        const core::MapEntity& entity = entities[issue.entityIndex];
        lines.push_back(EditorDiagnostic{.kind = EditorDiagnosticKind::Reference,
                                         .entityIndex = issue.entityIndex,
                                         .cell = entity.position,
                                         .message = format(entityIssueTemplate(issue.code),
                                                           {entity.type, issue.key, issue.value})});
    }
    for (const core::EncounterTerrain& terrain : terrains) {
        for (const core::TacticalIssue& issue : terrain.issues) {
            const std::vector<std::string> args =
                issue.code == core::TacticalIssueCode::AreaTooNarrow
                    ? std::vector<std::string>{terrain.encounterId,
                                               std::to_string(terrain.area.size()),
                                               std::to_string(terrain.requiredCells)}
                    : std::vector<std::string>{terrain.encounterId, issue.creatureId};
            lines.push_back(
                EditorDiagnostic{.kind = EditorDiagnosticKind::Terrain,
                                 .entityIndex = terrain.entityIndex,
                                 .cell = issue.cell,
                                 .message = format(tacticalIssueTemplate(issue.code), args)});
        }
    }
    for (const core::CombatZoneTerrain& zone : zones) {
        if (zone.issue) {
            lines.push_back(EditorDiagnostic{.kind = EditorDiagnosticKind::Terrain,
                                             .entityIndex = zone.entityIndex,
                                             .cell = zone.zone.origin,
                                             .message = combatZoneSummary(zone)});
        }
    }
    // Une entree d'arene hors de TOUTE zone : aucun combat de cette carte ne la verra.
    if (!zones.empty()) {
        for (const std::size_t entry : zones.front().entriesOutside) {
            const bool outsideAll = std::ranges::all_of(zones, [entry](const auto& zone) {
                return std::ranges::find(zone.entriesOutside, entry) != zone.entriesOutside.end();
            });
            if (outsideAll && entry < entities.size()) {
                lines.push_back(EditorDiagnostic{
                    .kind = EditorDiagnosticKind::Terrain,
                    .entityIndex = entry,
                    .cell = entities[entry].position,
                    .message = "Arena entry stands outside every combat zone: the fight will not "
                               "see it."});
            }
        }
    }
    return lines;
}

}  // namespace hmi
