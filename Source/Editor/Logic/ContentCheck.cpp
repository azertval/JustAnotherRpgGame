// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/ContentCheck.h"

#include <algorithm>
#include <optional>
#include <system_error>
#include <variant>

#include "Core/Combat/CombatTransition.h"
#include "Core/Combat/TacticalTerrain.h"
#include "Core/Gameplay/MapEntitySpawner.h"
#include "Core/Gameplay/Quest.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/CityBlock.h"
#include "Core/World/CityPlan.h"
#include "Core/World/CombatZone.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/ExplorationReach.h"
#include "Editor/Logic/EditorDiagnostics.h"
#include "Editor/Logic/EntityShapes.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace hmi {

namespace {

// Accumule les constats d'une carte.
class ContentFindings {
public:
    explicit ContentFindings(std::string_view mapId) : _mapId(mapId) {}

    void add(MapCheckSeverity severity, std::string message,
             std::optional<core::GridPosition> cell = std::nullopt, std::string entityId = {}) {
        _findings.push_back(MapCheckFinding{.severity = severity,
                                            .mapId = _mapId,
                                            .cell = cell,
                                            .message = std::move(message),
                                            .entityId = std::move(entityId)});
    }

    [[nodiscard]] std::vector<MapCheckFinding> take() {
        return std::move(_findings);
    }

private:
    std::string _mapId;
    std::vector<MapCheckFinding> _findings;
};

[[nodiscard]] std::string joined(const std::vector<std::string>& words) {
    std::string text;
    for (const std::string& word : words) {
        text += (text.empty() ? "" : ", ") + word + ".lang";
    }
    return text;
}

// `npc e3 (garde)` : de quoi retrouver l'entite dans la liste.
[[nodiscard]] std::string describe(const core::MapEntity& entity) {
    std::string text = entity.type;
    if (!entity.id.empty()) {
        text += " " + entity.id;
    }
    if (const std::string label = entityLabel(entity); !label.empty()) {
        text += " (" + label + ")";
    }
    return text;
}

[[nodiscard]] std::string textOf(const core::MapEntity& entity, std::string_view key) {
    const auto found = entity.properties.find(std::string{key});
    if (found == entity.properties.end()) {
        return {};
    }
    const std::string* const text = std::get_if<std::string>(&found->second);
    return text != nullptr ? *text : std::string{};
}

// --- Textes ----------------------------------------------------------------------------------

void checkTexts(std::string_view mapId, const core::Level& level, const ContentContext& context,
                ContentFindings& findings) {
    if (context.translations.empty()) {
        findings.add(MapCheckSeverity::Warning,
                     "no translation catalog under Localization/: map texts not checked");
        return;
    }
    if (const std::vector<std::string> missing =
            languagesMissing(context.translations, level.name());
        !missing.empty()) {
        findings.add(MapCheckSeverity::Error,
                     "map name \"" + level.name() + "\" is not a translation key of " +
                         joined(missing) + " (expected " + mapNameKey(mapId) + ")");
    }
    for (const core::MapEntity& entity : level.entities()) {
        if (entity.type != core::CITY_BLOCK_ENTITY_TYPE) {
            continue;
        }
        const std::string key =
            std::string{CITY_BLOCK_KEY_PREFIX} + textOf(entity, core::CITY_BLOCK_NAME_PROPERTY);
        if (const std::vector<std::string> missing = languagesMissing(context.translations, key);
            !missing.empty()) {
            findings.add(MapCheckSeverity::Error,
                         describe(entity) + ": key " + key + " is missing from " + joined(missing),
                         entity.position, entity.id);
        }
    }
}

// --- Références et terrain -------------------------------------------------------------------

// Dit une fois ailleurs : par le graphe du monde (portails) et par la carte (points en double).
[[nodiscard]] bool saidByTheFormatCheck(core::EntityIssueCode code) {
    return code == core::EntityIssueCode::UnknownTargetMap ||
           code == core::EntityIssueCode::UnknownArrivalPoint ||
           code == core::EntityIssueCode::DuplicateArrivalPoint;
}

void addDiagnostics(const std::vector<EditorDiagnostic>& diagnostics,
                    const std::vector<core::MapEntity>& entities, MapCheckSeverity severity,
                    ContentFindings& findings) {
    for (const EditorDiagnostic& diagnostic : diagnostics) {
        const std::string id =
            diagnostic.entityIndex < entities.size() ? entities[diagnostic.entityIndex].id : "";
        findings.add(severity, diagnostic.message, diagnostic.cell, id);
    }
}

// L'emprise d'une entite qui pose une piece (`EntityKind::pieceProperty`, LOT-126) est celle de sa
// piece au manifeste : sinon la porte n'arrete pas le pas la ou elle se dessine.
void checkPieceFootprints(const core::Level& level, const core::ScenePieceManifest* manifest,
                          ContentFindings& findings) {
    if (manifest == nullptr) {
        return;
    }
    for (const core::MapEntity& entity : level.entities()) {
        const core::EntityKind* const kind = core::findEntityKind(entity.type);
        if (kind == nullptr || kind->pieceProperty.empty() ||
            kind->shape != core::EntityShape::Rectangle) {
            continue;
        }
        const core::ScenePiece* const piece = manifest->find(textOf(entity, kind->pieceProperty));
        if (piece == nullptr) {
            continue;  // piece inconnue : dit par les references.
        }
        const std::optional<CellRect> rect = entityRectangle(entity);
        if (rect &&
            (rect->columns != piece->footprintColumns || rect->rows != piece->footprintRows)) {
            findings.add(MapCheckSeverity::Warning,
                         describe(entity) + ": its extent is " + std::to_string(rect->columns) +
                             " x " + std::to_string(rect->rows) + ", its piece's is " +
                             std::to_string(piece->footprintColumns) + " x " +
                             std::to_string(piece->footprintRows),
                         entity.position, entity.id);
        }
    }
}

void checkEntities(std::string_view mapId, const core::Level& level, const ContentContext& context,
                   ContentFindings& findings) {
    const std::vector<core::MapEntity>& entities = level.entities();
    const EditorReferences& references = context.references;
    std::vector<core::EntityIssue> errors;
    std::vector<core::EntityIssue> unknownKinds;
    // Le catalogue du lieu : ce qu'un decor peut nommer, et l'emprise de chaque piece (LOT-126).
    const std::string place = scenePlaceOf(level.layers());
    std::optional<core::ScenePieceManifest> manifest;
    if (!place.empty() && !references.assets.empty()) {
        if (core::ScenePieceManifestResult read =
                core::ScenePieceManifest::resolve(references.assets, place);
            read.ok()) {
            manifest = std::move(read.manifest);
        }
    }
    const core::ScenePieceManifest* const pieces = manifest ? &*manifest : nullptr;
    checkPieceFootprints(level, pieces, findings);
    for (core::EntityIssue& issue : core::validateMapEntities(
             entities, referenceContext(references, mapId, entities, place, pieces))) {
        if (issue.code == core::EntityIssueCode::UnknownType) {
            unknownKinds.push_back(std::move(issue));  // legal, transporte : une information.
        } else if (!saidByTheFormatCheck(issue.code)) {
            errors.push_back(std::move(issue));
        }
    }
    addDiagnostics(editorDiagnostics(entities, errors, {}), entities, MapCheckSeverity::Error,
                   findings);
    addDiagnostics(editorDiagnostics(entities, unknownKinds, {}), entities,
                   MapCheckSeverity::Warning, findings);
    addDiagnostics(editorDiagnostics(
                       entities, {},
                       core::analyzeEncounterTerrain(level.tileMap(), entities,
                                                     references.encounters, &references.bestiary)),
                   entities, MapCheckSeverity::Error, findings);
    // Les zones fautives sont dites par le controle du format ; reste l'entree d'arene qu'aucune
    // zone ne voit.
    std::vector<core::CombatZoneTerrain> zones =
        core::analyzeCombatZones(level.tileMap(), entities);
    for (core::CombatZoneTerrain& zone : zones) {
        zone.issue.reset();
    }
    addDiagnostics(editorDiagnostics(entities, {}, {}, zones), entities, MapCheckSeverity::Warning,
                   findings);
}

// --- Atteignabilité --------------------------------------------------------------------------

// Les familles qu'on aborde sur leur case : le heros y marche (portail, arrivee, rencontre) ou
// l'y vise (coffre, panneau, PNJ), ce qui revient au meme (`ExplorationReach.h`).
[[nodiscard]] bool standsOnItsCell(std::string_view type) {
    return type == core::PORTAL_ENTITY_TYPE || type == core::SPAWN_POINT_ENTITY_TYPE ||
           type == core::ENCOUNTER_ENTITY_TYPE ||
           std::ranges::find(core::knownInteractableKinds(), type, &core::InteractableKind::type) !=
               core::knownInteractableKinds().end();
}

// Les zones : il suffit d'une case atteinte pour y entrer.
[[nodiscard]] bool isZone(std::string_view type) {
    return type == core::COMBAT_ZONE_ENTITY_TYPE || type == core::ZONE_ENTITY_TYPE;
}

void checkReach(std::string_view mapId, const core::Level& level, const ContentContext& context,
                ContentFindings& findings) {
    const core::TileMap& collision = level.tileMap();
    std::vector<core::GridPosition> starts = {level.entry()};
    for (const core::MapEntity& entity : level.entities()) {
        if (entity.type != core::SPAWN_POINT_ENTITY_TYPE ||
            !context.namedArrivals.contains(std::pair<std::string, std::string>{
                mapId, textOf(entity, core::SPAWN_POINT_NAME_PROPERTY)})) {
            continue;
        }
        starts.push_back(entity.position);
    }
    const core::ExplorationReach reach(collision, starts);
    for (const core::MapEntity& entity : level.entities()) {
        if (standsOnItsCell(entity.type)) {
            const core::GridPosition cell = entity.position;
            if (collision.inBounds(cell.column, cell.row) &&
                collision.isSolid(cell.column, cell.row)) {
                findings.add(MapCheckSeverity::Error,
                             describe(entity) + " stands on a blocked cell", cell, entity.id);
            } else if (!reach.reaches(cell)) {
                findings.add(
                    MapCheckSeverity::Error,
                    describe(entity) + " cannot be reached from the entry or an arrival point",
                    cell, entity.id);
            }
        } else if (isZone(entity.type)) {
            const std::vector<core::GridPosition> cells = entityCells(entity);
            if (!cells.empty() && std::ranges::none_of(cells, [&reach](core::GridPosition cell) {
                    return reach.reaches(cell);
                })) {
                findings.add(
                    MapCheckSeverity::Error,
                    describe(entity) +
                        ": no cell of it can be reached from the entry or an arrival point",
                    entity.position, entity.id);
            }
        }
    }
}

// --- Graphe ----------------------------------------------------------------------------------

void checkGraph(std::string_view mapId, const core::Level& level, const ContentContext& context,
                ContentFindings& findings) {
    const core::WorldGraph& world = context.references.world;
    for (const core::MapEntity& entity : level.entities()) {
        if (entity.type == core::SPAWN_POINT_ENTITY_TYPE) {
            const std::string name = textOf(entity, core::SPAWN_POINT_NAME_PROPERTY);
            if (!name.empty() &&
                !context.namedArrivals.contains(std::pair<std::string, std::string>{mapId, name})) {
                findings.add(MapCheckSeverity::Warning,
                             describe(entity) + " is named by no portal and no city start",
                             entity.position, entity.id);
            }
        } else if (entity.type == core::PORTAL_ENTITY_TYPE) {
            const std::string target = textOf(entity, core::PORTAL_TARGET_MAP_PROPERTY);
            if (core::isSealedPortal(entity)) {
                continue;  // condamne : on n'en revient pas, puisqu'on n'y passe pas (LOT-126).
            }
            if (target.empty() || target == mapId || world.find(target) == nullptr) {
                continue;  // pas de cible, ou cible inconnue : dit par le controle du format.
            }
            const std::vector<const core::WorldPortalLink*> back = world.portalsFrom(target);
            if (std::ranges::none_of(back, [mapId](const core::WorldPortalLink* link) {
                    return link->toMap == mapId;
                })) {
                findings.add(MapCheckSeverity::Warning,
                             describe(entity) + " has no way back: no portal of \"" + target +
                                 "\" leads here",
                             entity.position, entity.id);
            }
        }
    }
}

}  // namespace

ContentContext loadContentContext(const std::filesystem::path& dataRoot) {
    ContentContext context{.references = loadEditorReferences(dataRoot),
                           .translations = loadTranslationCatalogs(localizationDirectory(dataRoot)),
                           .namedArrivals = {}};
    for (const core::WorldPortalLink& link : context.references.world.portals) {
        if (!link.toMap.empty() && !link.arrival.empty()) {
            context.namedArrivals.emplace(link.toMap, link.arrival);
        }
    }
    // « Nouvelle partie » entre par la porte de depart d'une ville.
    std::error_code error;
    for (auto it = std::filesystem::directory_iterator(dataRoot / "World" / "cities", error);
         !error && it != std::filesystem::directory_iterator(); it.increment(error)) {
        if (it->path().extension() != ".json") {
            continue;
        }
        const core::CityPlanResult city = core::loadCityPlan(it->path());
        if (city.ok() && !city.plan.startMap().empty()) {
            context.namedArrivals.emplace(city.plan.startMap(), city.plan.startArrival);
        }
    }
    return context;
}

std::vector<MapCheckFinding> checkMapContent(std::string_view mapId, const core::Level& level,
                                             const ContentContext& context) {
    ContentFindings findings(mapId);
    checkTexts(mapId, level, context, findings);
    checkEntities(mapId, level, context, findings);
    checkReach(mapId, level, context, findings);
    checkGraph(mapId, level, context, findings);
    return findings.take();
}

std::vector<MapCheckFinding> checkStoryContent(const std::filesystem::path& dataRoot) {
    std::vector<MapCheckFinding> findings;
    const auto error = [&findings](std::string message) {
        findings.push_back(MapCheckFinding{.severity = MapCheckSeverity::Error,
                                           .mapId = "World",
                                           .cell = std::nullopt,
                                           .message = std::move(message),
                                           .entityId = {}});
    };
    const core::QuestCatalog quests = core::loadQuests(dataRoot / "World" / "quests");
    const core::DialogueCatalog dialogues = core::loadDialogues(dataRoot / "World" / "dialogues");
    // Les messages du chargement viennent de `core`, en francais : ce sont ceux que l'auteur des
    // donnees lit aussi au demarrage du jeu, et ils nomment fichier et ligne.
    for (const std::string& rejected : quests.errors) {
        error("quest rejected: " + rejected);
    }
    // Un dialogue refuse au chargement -- un jet sans branche d'echec (LOT-117), une cible
    // inconnue, une impasse -- ne se jouerait pas : le PNJ resterait muet en jeu.
    // Un projet sans dossier de dialogues n'en a simplement pas : ce n'est pas un refus.
    if (std::filesystem::is_directory(dataRoot / "World" / "dialogues")) {
        for (const std::string& rejected : dialogues.errors) {
            error("dialogue rejected: " + rejected);
        }
    }
    for (const std::string& misuse : core::validateFlagUses(quests, dialogues)) {
        error("flag misuse: " + misuse);
    }
    std::set<std::string, std::less<>> written = core::flagsWrittenBy(quests, dialogues);
    // Les zones des cartes posent aussi des drapeaux (LOT-126).
    for (const core::WorldMapNode& map : core::loadWorldGraph(dataRoot / "Levels").maps) {
        written.insert(map.triggerFlags.begin(), map.triggerFlags.end());
    }
    for (const core::FlagRead& read : core::flagsReadBy(quests, dialogues)) {
        if (!written.contains(read.flag)) {
            error(read.where + ": flag \"" + read.flag +
                  "\" is read but no dialogue, quest or zone sets it");
        }
    }
    return findings;
}

}  // namespace hmi
