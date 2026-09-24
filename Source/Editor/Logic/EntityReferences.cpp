// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/EntityReferences.h"

#include <algorithm>
#include <fstream>
#include <set>
#include <system_error>
#include <variant>

#include <nlohmann/json.hpp>

#include "Core/Resources/ScenePlace.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/Rpg/Inventory.h"

namespace hmi {

namespace {

// Vrai si @p directory existe : les chargeurs du Core tolerent un dossier absent, mais certains le
// journalisent comme une erreur -- l'editeur deploye sans catalogue n'en est pas une.
[[nodiscard]] bool isDirectory(const std::filesystem::path& directory) {
    std::error_code error;
    return std::filesystem::is_directory(directory, error);
}

// Le manifeste JSON @p path, ou `null` s'il manque ou ne se lit pas : une figurine absente se
// signale a l'entite qui la nomme, pas au chargement de l'editeur.
[[nodiscard]] nlohmann::json readManifest(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        return nullptr;
    }
    return nlohmann::json::parse(file, nullptr, /*allow_exceptions=*/false);
}

// Les figurines des deux ateliers : un PNJ par son slug, un monstre par son dossier.
[[nodiscard]] std::vector<std::string> loadFigures(const std::filesystem::path& assets) {
    std::vector<std::string> figures;
    const nlohmann::json npcs = readManifest(assets / "Npc" / "manifest.json");
    if (npcs.is_object() && npcs.contains("npcs") && npcs.at("npcs").is_array()) {
        for (const nlohmann::json& slug : npcs.at("npcs")) {
            if (slug.is_string()) {
                figures.push_back(slug.get<std::string>());
            }
        }
    }
    const nlohmann::json monsters = readManifest(assets / "Monsters" / "manifest.json");
    if (monsters.is_object() && monsters.contains("monsters") &&
        monsters.at("monsters").is_array()) {
        for (const nlohmann::json& monster : monsters.at("monsters")) {
            if (monster.is_object() && monster.contains("slug") && monster.at("slug").is_string()) {
                figures.push_back("Monsters/" + monster.at("slug").get<std::string>());
            }
        }
    }
    // Les figurines du monde (`Common/Characters`, LOT-124) : celles que toute carte peut poser.
    for (const auto& [slug, directory] : core::resolveFigures(assets, {})) {
        figures.push_back(slug);
    }
    std::ranges::sort(figures);
    figures.erase(std::ranges::unique(figures).begin(), figures.end());
    return figures;
}

// Les drapeaux que posent les dialogues : un portail qui en exige un autre ne s'ouvrirait jamais.
[[nodiscard]] std::vector<std::string> flagsSetBy(const core::DialogueCatalog& catalog) {
    std::set<std::string, std::less<>> flags;
    for (const core::DialogueGraph& graph : catalog.dialogues) {
        for (const core::DialogueNode& node : graph.nodes) {
            for (const core::DialogueAction& action : node.actions) {
                if (action.kind == core::DialogueActionKind::SetFlag) {
                    flags.insert(action.target);
                } else if (action.kind == core::DialogueActionKind::StartQuest) {
                    flags.insert(core::questStartedFlag(action.target));
                }
            }
        }
    }
    return {flags.begin(), flags.end()};
}

// Les fiches d'un dossier de l'atlas, par le nom de leur fichier.
[[nodiscard]] std::vector<std::string> fileStems(const std::filesystem::path& directory) {
    std::vector<std::string> stems;
    std::error_code error;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(directory, error)) {
        if (entry.path().extension() == ".json") {
            stems.push_back(entry.path().stem().string());
        }
    }
    std::ranges::sort(stems);
    return stems;
}

}  // namespace

std::string entityRef(std::string_view mapId, std::string_view entityId) {
    return std::string{mapId} + "#" + std::string{entityId};
}

EditorReferences loadEditorReferences(const std::filesystem::path& root) {
    EditorReferences references;
    if (const std::filesystem::path dialogues = root / "World" / "dialogues";
        isDirectory(dialogues)) {
        const core::DialogueCatalog loaded = core::loadDialogues(dialogues);
        for (const core::DialogueGraph& graph : loaded.dialogues) {
            references.dialogues.push_back(graph.id);
        }
        std::ranges::sort(references.dialogues);
        references.flags = flagsSetBy(loaded);
    }
    if (const std::filesystem::path locations = root / "World" / "locations";
        isDirectory(locations)) {
        references.locations = fileStems(locations);
    }
    if (const std::filesystem::path items = root / "Rpg" / "items"; isDirectory(items)) {
        const auto loadedItems = core::loadItems(items);
        for (const core::Item& item : loadedItems.items) {
            references.items.push_back(item.id);
        }
        std::ranges::sort(references.items);
    }
    references.assets = root / "Assets";
    references.figures = loadFigures(references.assets);
    if (const std::filesystem::path encounters = root / "Rpg" / "encounters";
        isDirectory(encounters)) {
        references.encounters = core::loadEncounters(encounters);
    }
    if (const std::filesystem::path creatures = root / "Rpg" / "creatures";
        isDirectory(creatures)) {
        references.bestiary = core::loadBestiary(creatures);
    }
    references.world = core::loadWorldGraph(root / "Levels");
    return references;
}

core::EntityReferenceContext referenceContext(const EditorReferences& references,
                                              std::string_view editedMapId,
                                              const std::vector<core::MapEntity>& editedEntities,
                                              std::string_view place) {
    core::EntityReferenceContext context;
    context.dialogues.insert(references.dialogues.begin(), references.dialogues.end());
    for (const core::Encounter& encounter : references.encounters.encounters) {
        context.encounters.insert(encounter.id);
    }
    context.figures.insert(references.figures.begin(), references.figures.end());
    // Les figurines du lieu et de ses niveaux communs (LOT-124) : un PNJ nommé d'une zone ne se
    // propose qu'aux cartes qui en descendent.
    if (!place.empty() && !references.assets.empty()) {
        for (const auto& [slug, directory] : core::resolveFigures(references.assets, place)) {
            context.figures.insert(slug);
        }
    }
    context.flags.insert(references.flags.begin(), references.flags.end());
    context.locations.insert(references.locations.begin(), references.locations.end());
    context.items.insert(references.items.begin(), references.items.end());
    for (const core::WorldMapNode& map : references.world.maps) {
        context.arrivalPointsByMap[map.mapId].insert(map.arrivalPoints.begin(),
                                                     map.arrivalPoints.end());
        // La carte editee se lit dans le brouillon, pas dans son fichier : un identifiant retire
        // depuis l'ouverture n'est plus citable.
        if (map.mapId == editedMapId) {
            continue;
        }
        for (const std::string& id : map.entityIds) {
            context.entityRefs.insert(entityRef(map.mapId, id));
        }
    }
    if (!editedMapId.empty()) {
        context.arrivalPointsByMap[std::string{editedMapId}] =
            core::arrivalPointNames(editedEntities);
        for (const core::MapEntity& entity : editedEntities) {
            if (!entity.id.empty()) {
                context.entityRefs.insert(entityRef(editedMapId, entity.id));
            }
        }
    }
    return context;
}

std::vector<std::string> entityChoices(const core::EntityPropertySpec& spec,
                                       const core::MapEntity& entity,
                                       const core::EntityReferenceContext& context) {
    std::vector<std::string> choices;
    if (spec.kind != core::EntityPropertyKind::Choice) {
        return choices;
    }
    switch (spec.source) {
        case core::EntityChoiceSource::Fixed:
            for (const std::string_view choice : spec.fixedChoices) {
                choices.emplace_back(choice);
            }
            break;
        case core::EntityChoiceSource::Dialogues:
            choices.assign(context.dialogues.begin(), context.dialogues.end());
            break;
        case core::EntityChoiceSource::Encounters:
            choices.assign(context.encounters.begin(), context.encounters.end());
            break;
        case core::EntityChoiceSource::Maps:
            for (const auto& [mapId, points] : context.arrivalPointsByMap) {
                static_cast<void>(points);
                choices.push_back(mapId);
            }
            break;
        case core::EntityChoiceSource::Figures:
            choices.assign(context.figures.begin(), context.figures.end());
            break;
        case core::EntityChoiceSource::Flags:
            choices.assign(context.flags.begin(), context.flags.end());
            break;
        case core::EntityChoiceSource::Locations:
            choices.assign(context.locations.begin(), context.locations.end());
            break;
        case core::EntityChoiceSource::Items:
            choices.assign(context.items.begin(), context.items.end());
            break;
        case core::EntityChoiceSource::EntityRefs:
            choices.assign(context.entityRefs.begin(), context.entityRefs.end());
            break;
        case core::EntityChoiceSource::ArrivalPoints: {
            const auto target =
                entity.properties.find(std::string{core::PORTAL_TARGET_MAP_PROPERTY});
            if (target == entity.properties.end()) {
                break;
            }
            const auto* const mapId = std::get_if<std::string>(&target->second);
            if (mapId == nullptr) {
                break;
            }
            if (const auto points = context.arrivalPointsByMap.find(*mapId);
                points != context.arrivalPointsByMap.end()) {
                choices.assign(points->second.begin(), points->second.end());
            }
            break;
        }
    }
    std::ranges::sort(choices);
    return choices;
}

}  // namespace hmi
