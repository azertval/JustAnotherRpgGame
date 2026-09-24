// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/World/WorldTravel.h"

#include <iterator>
#include <set>
#include <utility>
#include <variant>

#include "Core/Gameplay/WorldFlags.h"
#include "Core/World/CombatZone.h"
#include "Core/World/EntityKinds.h"

namespace core {

namespace {

// Meme regle que le graphe : une propriete ne vaut que si elle est du texte. Un entier la ou l'on
// attend un nom de carte est une saisie fautive, et la traiter comme vide la fait signaler.
[[nodiscard]] std::string texteDe(const MapEntity& entite, std::string_view cle) {
    const auto trouvee = entite.properties.find(std::string{cle});
    if (trouvee == entite.properties.end()) {
        return {};
    }
    const std::string* texte = std::get_if<std::string>(&trouvee->second);
    return texte != nullptr ? *texte : std::string{};
}

// Le code de defaut correspondant au statut d'un portail. Un portail resolu n'en a pas.
[[nodiscard]] std::optional<WorldIssueCode> defautDe(PortalLinkStatus statut) {
    switch (statut) {
        case PortalLinkStatus::Resolved:
        case PortalLinkStatus::Sealed:
            return std::nullopt;
        case PortalLinkStatus::MissingTarget:
            return WorldIssueCode::MissingTargetMap;
        case PortalLinkStatus::UnknownMap:
            return WorldIssueCode::UnknownTargetMap;
        case PortalLinkStatus::MissingArrival:
            return WorldIssueCode::MissingArrivalPoint;
        case PortalLinkStatus::UnknownArrival:
            return WorldIssueCode::UnknownArrivalPoint;
        case PortalLinkStatus::TargetUnreadable:
            return WorldIssueCode::UnreadableTargetMap;
    }
    return std::nullopt;
}

// Ce qu'un defaut de portail doit citer pour que le message soit exploitable : la carte quand
// c'est elle qui manque, le point d'arrivee quand c'est lui.
[[nodiscard]] std::string valeurCitee(const WorldPortalLink& portail, WorldIssueCode code) {
    switch (code) {
        case WorldIssueCode::UnknownTargetMap:
        case WorldIssueCode::UnreadableTargetMap:
            return portail.toMap;
        case WorldIssueCode::UnknownArrivalPoint:
            return portail.arrival;
        case WorldIssueCode::MissingTargetMap:
        case WorldIssueCode::MissingArrivalPoint:
        case WorldIssueCode::UnreadableMap:
        case WorldIssueCode::DuplicateArrivalPoint:
        case WorldIssueCode::CombatZoneDegenerate:
        case WorldIssueCode::CombatZoneOutOfBounds:
        case WorldIssueCode::CombatZoneBlocked:
            return {};
    }
    return {};
}

}  // namespace

std::optional<PortalTarget> portalAt(const Level& level, GridPosition position) {
    for (const MapEntity& entite : level.entities()) {
        if (entite.type != PORTAL_ENTITY_TYPE || entite.position != position) {
            continue;
        }
        return PortalTarget{.map = texteDe(entite, PORTAL_TARGET_MAP_PROPERTY),
                            .arrival = texteDe(entite, PORTAL_ARRIVAL_PROPERTY),
                            .requiredFlag = texteDe(entite, PORTAL_REQUIRED_FLAG_PROPERTY),
                            .sealed = isSealedPortal(entite)};
    }
    return std::nullopt;
}

std::optional<GridPosition> arrivalPointAt(const Level& level, std::string_view name) {
    if (name.empty()) {
        return std::nullopt;
    }
    for (const MapEntity& entite : level.entities()) {
        if (entite.type == SPAWN_POINT_ENTITY_TYPE &&
            texteDe(entite, SPAWN_POINT_NAME_PROPERTY) == name) {
            return entite.position;
        }
    }
    return std::nullopt;
}

std::vector<WorldIssue> validateWorldGraph(const WorldGraph& graph) {
    std::vector<WorldIssue> defauts;
    for (const WorldMapNode& carte : graph.maps) {
        if (!carte.loadError.empty()) {
            defauts.push_back(WorldIssue{.mapId = carte.mapId,
                                         .position = {},
                                         .code = WorldIssueCode::UnreadableMap,
                                         .value = carte.loadError});
        }
    }
    for (const WorldPortalLink& portail : graph.portals) {
        const std::optional<WorldIssueCode> code = defautDe(portail.status);
        if (!code.has_value()) {
            continue;
        }
        defauts.push_back(WorldIssue{.mapId = portail.fromMap,
                                     .position = portail.position,
                                     .code = *code,
                                     .value = valeurCitee(portail, *code)});
    }
    return defauts;
}

std::vector<WorldIssue> validateWorldMap(std::string_view mapId, const Level& level) {
    std::vector<WorldIssue> defauts;
    std::set<std::string, std::less<>> vus;
    for (const MapEntity& entite : level.entities()) {
        if (entite.type != SPAWN_POINT_ENTITY_TYPE) {
            continue;
        }
        std::string nom = texteDe(entite, SPAWN_POINT_NAME_PROPERTY);
        if (nom.empty()) {
            continue;
        }
        const bool premiereFois = vus.insert(nom).second;
        if (premiereFois) {
            continue;
        }
        defauts.push_back(WorldIssue{.mapId = std::string{mapId},
                                     .position = entite.position,
                                     .code = WorldIssueCode::DuplicateArrivalPoint,
                                     .value = std::move(nom)});
    }
    // Les zones de combat de la carte (`EX-LVL-018`) : une zone qui deborde ou qui n'a aucune case
    // libre est un defaut de la CARTE, du meme ordre que le point d'arrivee en double.
    std::vector<WorldIssue> zones = validateCombatZones(mapId, level);
    defauts.insert(defauts.end(), std::make_move_iterator(zones.begin()),
                   std::make_move_iterator(zones.end()));
    return defauts;
}

WorldTravel::MapLoader WorldTravel::directoryLoader(std::filesystem::path levelsDir) {
    return [dossier = std::move(levelsDir)](std::string_view mapId) {
        return LevelLoader::loadFromFile(dossier / (std::string{mapId} + ".json"));
    };
}

WorldTravel::MapLoader WorldTravel::directoriesLoader(
    std::vector<std::filesystem::path> levelsDirs) {
    return [dossiers = std::move(levelsDirs)](std::string_view mapId) {
        // Valeur de depart : ce que rend une liste vide, et ce qui reste si aucun dossier ne
        // porte la carte.
        LevelLoadResult dernier;
        dernier.error = "Carte introuvable : " + std::string{mapId};
        dernier.errorCode = LevelValidationError::FileNotFound;
        for (const std::filesystem::path& dossier : dossiers) {
            dernier = LevelLoader::loadFromFile(dossier / (std::string{mapId} + ".json"));
            // Seule l'ABSENCE fait passer au dossier suivant : une carte presente mais illisible
            // est l'erreur qu'il faut montrer, pas une raison de jouer la version d'a cote.
            if (dernier.ok() || dernier.errorCode != LevelValidationError::FileNotFound) {
                return dernier;
            }
        }
        return dernier;
    };
}

WorldTravel::WorldTravel(MapLoader loader) : _loader(std::move(loader)) {}

const Level* WorldTravel::currentMap() const {
    const auto trouvee = _maps.find(_currentMapId);
    return trouvee != _maps.end() ? trouvee->second.get() : nullptr;
}

const Level* WorldTravel::mapFor(std::string_view mapId) {
    if (const auto deja = _maps.find(mapId); deja != _maps.end()) {
        return deja->second.get();
    }
    if (!_loader) {
        return nullptr;
    }
    LevelLoadResult lu = _loader(mapId);
    if (!lu.ok()) {
        _lastIssue = WorldIssue{.mapId = std::string{mapId},
                                .position = {},
                                .code = WorldIssueCode::UnreadableMap,
                                .value = std::move(lu.error)};
        return nullptr;
    }
    const auto posee =
        _maps.emplace(std::string{mapId}, std::make_shared<Level>(std::move(*lu.level))).first;
    return posee->second.get();
}

TravelResult WorldTravel::enter(std::string_view mapId, std::string_view arrival) {
    _lastIssue.reset();
    const Level* carte = mapFor(mapId);
    if (carte == nullptr) {
        return TravelResult::UnreadableMap;
    }
    // Une arrivee vide pose le personnage sur l'entree de la carte : c'est « Nouvelle partie »,
    // qui n'arrive par aucun portail.
    GridPosition ou = carte->entry();
    if (!arrival.empty()) {
        const std::optional<GridPosition> point = arrivalPointAt(*carte, arrival);
        if (!point.has_value()) {
            _lastIssue = WorldIssue{.mapId = std::string{mapId},
                                    .position = {},
                                    .code = WorldIssueCode::UnknownArrivalPoint,
                                    .value = std::string{arrival}};
            return TravelResult::UnknownArrival;
        }
        ou = *point;
    }
    _currentMapId = std::string{mapId};
    _position = ou;
    return TravelResult::Moved;
}

TravelResult WorldTravel::cross(GridPosition from, const WorldFlags& flags) {
    _lastIssue.reset();
    const Level* carte = currentMap();
    if (carte == nullptr) {
        return TravelResult::NoPortal;
    }
    const std::optional<PortalTarget> portail = portalAt(*carte, from);
    if (!portail.has_value()) {
        return TravelResult::NoPortal;
    }
    if (portail->sealed) {
        return TravelResult::Sealed;
    }
    // Le drapeau exige du monde, pas de la carte : la porte d'Arenarea s'ouvre quand la quete l'a
    // ouverte (LOT-16), et le portail se contente de le lire.
    if (!portail->requiredFlag.empty() && !flags.isSet(portail->requiredFlag)) {
        return TravelResult::Locked;
    }
    if (portail->map.empty()) {
        _lastIssue = WorldIssue{.mapId = _currentMapId,
                                .position = from,
                                .code = WorldIssueCode::MissingTargetMap,
                                .value = {}};
        return TravelResult::UnreadableMap;
    }
    return enter(portail->map, portail->arrival);
}

}  // namespace core
