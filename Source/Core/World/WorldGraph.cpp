// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/World/WorldGraph.h"

#include <algorithm>
#include <functional>
#include <set>
#include <system_error>
#include <utility>
#include <variant>

#include "Core/Levels/LevelLoader.h"
#include "Core/World/EntityKinds.h"

namespace core {

namespace {

// Une propriete ne vaut que si elle est du texte : un entier la ou l'on attend un nom de carte est
// une saisie fautive, et la traiter comme vide la fait signaler plutot que deviner.
[[nodiscard]] std::string texteDe(const MapEntity& entite, std::string_view cle) {
    const auto trouvee = entite.properties.find(std::string{cle});
    if (trouvee == entite.properties.end()) {
        return {};
    }
    const std::string* texte = std::get_if<std::string>(&trouvee->second);
    return texte != nullptr ? *texte : std::string{};
}

// Le premier cas qui s'applique l'emporte : une cible illisible passe avant une arrivee vide, car
// on ne peut rien dire des points d'arrivee d'une carte qu'on n'a pas lue.
[[nodiscard]] PortalLinkStatus statutDu(const WorldGraph& graphe, const WorldPortalLink& portail) {
    if (portail.toMap.empty()) {
        return PortalLinkStatus::MissingTarget;
    }
    const WorldMapNode* cible = graphe.find(portail.toMap);
    if (cible == nullptr) {
        return PortalLinkStatus::UnknownMap;
    }
    if (!cible->loadError.empty()) {
        return PortalLinkStatus::TargetUnreadable;
    }
    if (portail.arrival.empty()) {
        return PortalLinkStatus::MissingArrival;
    }
    if (!std::ranges::binary_search(cible->arrivalPoints, portail.arrival)) {
        return PortalLinkStatus::UnknownArrival;
    }
    return PortalLinkStatus::Resolved;
}

}  // namespace

const WorldMapNode* WorldGraph::find(std::string_view mapId) const {
    const auto trouvee = std::ranges::lower_bound(
        maps, mapId, std::less<>{},
        [](const WorldMapNode& carte) -> std::string_view { return carte.mapId; });
    return trouvee != maps.end() && trouvee->mapId == mapId ? &*trouvee : nullptr;
}

std::vector<const WorldPortalLink*> WorldGraph::portalsFrom(std::string_view mapId) const {
    std::vector<const WorldPortalLink*> trouves;
    for (const WorldPortalLink& portail : portals) {
        if (portail.fromMap == mapId) {
            trouves.push_back(&portail);
        }
    }
    return trouves;
}

std::vector<const WorldPortalLink*> WorldGraph::portalsTo(std::string_view mapId) const {
    std::vector<const WorldPortalLink*> trouves;
    for (const WorldPortalLink& portail : portals) {
        if (portail.toMap == mapId) {
            trouves.push_back(&portail);
        }
    }
    return trouves;
}

std::vector<std::string> WorldGraph::unreachableFrom(std::string_view fromMapId) const {
    std::set<std::string, std::less<>> vues;
    if (find(fromMapId) != nullptr) {
        std::vector<std::string> pile{std::string{fromMapId}};
        vues.insert(std::string{fromMapId});
        while (!pile.empty()) {
            const std::string courante = std::move(pile.back());
            pile.pop_back();
            for (const WorldPortalLink* portail : portalsFrom(courante)) {
                // Seul un portail resolu mene quelque part : un portail casse ne rend pas sa cible
                // joignable, meme si elle existe.
                if (portail->status == PortalLinkStatus::Resolved &&
                    vues.insert(portail->toMap).second) {
                    pile.push_back(portail->toMap);
                }
            }
        }
    }
    std::vector<std::string> isolees;
    for (const WorldMapNode& carte : maps) {
        if (!vues.contains(carte.mapId)) {
            isolees.push_back(carte.mapId);
        }
    }
    return isolees;
}

WorldGraph buildWorldGraph(std::vector<WorldMapInput> maps) {
    // Tri stable : l'ordre de lecture d'un dossier n'est pas garanti, celui du graphe doit l'etre.
    std::ranges::stable_sort(maps, std::less<>{}, &WorldMapInput::mapId);

    WorldGraph graphe;
    graphe.maps.reserve(maps.size());
    for (const WorldMapInput& entree : maps) {
        std::set<std::string, std::less<>> points;
        std::set<std::string, std::less<>> identifiants;
        const std::set<std::string, std::less<>> drapeaux = flagsSetByEntities(entree.entities);
        for (const MapEntity& entite : entree.entities) {
            if (!entite.id.empty()) {
                identifiants.insert(entite.id);
            }
            if (entite.type != SPAWN_POINT_ENTITY_TYPE) {
                continue;
            }
            std::string nom = texteDe(entite, SPAWN_POINT_NAME_PROPERTY);
            if (!nom.empty()) {
                points.insert(std::move(nom));
            }
        }
        graphe.maps.push_back(WorldMapNode{
            .mapId = entree.mapId,
            .name = entree.name,
            .arrivalPoints = std::vector<std::string>(points.begin(), points.end()),
            .loadError = entree.loadError,
            .entityIds = std::vector<std::string>(identifiants.begin(), identifiants.end()),
            .triggerFlags = std::vector<std::string>(drapeaux.begin(), drapeaux.end())});
    }

    // Deux passes : un portail peut viser une carte triee apres la sienne.
    for (const WorldMapInput& entree : maps) {
        for (const MapEntity& entite : entree.entities) {
            if (entite.type == PORTAL_ENTITY_TYPE) {
                WorldPortalLink portail{.fromMap = entree.mapId,
                                        .position = entite.position,
                                        .toMap = texteDe(entite, PORTAL_TARGET_MAP_PROPERTY),
                                        .arrival = texteDe(entite, PORTAL_ARRIVAL_PROPERTY)};
                // Condamne, il est voulu tel : ni erreur, ni chemin (LOT-126).
                portail.status =
                    isSealedPortal(entite) ? PortalLinkStatus::Sealed : statutDu(graphe, portail);
                graphe.portals.push_back(std::move(portail));
            } else if (entite.type == ZONE_ENTITY_TYPE) {
                WorldPortalLink transfert{.fromMap = entree.mapId,
                                          .position = entite.position,
                                          .toMap = texteDe(entite, ZONE_TRIGGER_MAP_PROPERTY),
                                          .arrival = texteDe(entite, ZONE_TRIGGER_ARRIVAL_PROPERTY),
                                          .kind = WorldLinkKind::Transfer};
                if (transfert.toMap.empty()) {
                    continue;  // une zone sans transfert n'est pas une arete.
                }
                transfert.status = statutDu(graphe, transfert);
                graphe.portals.push_back(std::move(transfert));
            }
        }
    }
    return graphe;
}

std::string mapIdOf(const std::filesystem::path& levelsDir, const std::filesystem::path& file) {
    std::filesystem::path relatif = file.lexically_relative(levelsDir);
    relatif.replace_extension();
    return relatif.generic_string();
}

WorldGraph loadWorldGraph(const std::filesystem::path& levelsDir) {
    std::vector<WorldMapInput> cartes;
    std::error_code code;
    if (!std::filesystem::is_directory(levelsDir, code)) {
        return buildWorldGraph(std::move(cartes));
    }
    // Recursif : les quartiers de la Capitale vivent dans `capital/` (`LOT-96`), et leur
    // identifiant de carte est leur chemin relatif, sans extension, en barres obliques --
    // `capital/martpart`, ce que le chargeur du jeu (`WorldTravel::directoryLoader`) resout.
    for (auto iterateur = std::filesystem::recursive_directory_iterator(levelsDir, code);
         !code && iterateur != std::filesystem::recursive_directory_iterator();
         iterateur.increment(code)) {
        const std::filesystem::directory_entry& fichier = *iterateur;
        const std::filesystem::path& chemin = fichier.path();
        // Memes exclusions que le navigateur de cartes (`hmi::LevelFileOperations::list`) : les
        // scripts `sequence-*.json` et les annexes de l'editeur (`*.editor.json`) vivent a cote
        // des niveaux sans en etre.
        if (!fichier.is_regular_file(code) || chemin.extension() != ".json" ||
            chemin.filename().string().starts_with("sequence-") ||
            chemin.filename().string().ends_with(".editor.json")) {
            continue;
        }
        WorldMapInput carte{
            .mapId = mapIdOf(levelsDir, chemin), .name = {}, .entities = {}, .loadError = {}};
        LevelLoadResult lu = LevelLoader::loadFromFile(chemin);
        if (lu.ok()) {
            carte.name = lu.level->name();
            carte.entities = lu.level->entities();
        } else {
            carte.loadError = std::move(lu.error);
        }
        cartes.push_back(std::move(carte));
    }
    return buildWorldGraph(std::move(cartes));
}

}  // namespace core
