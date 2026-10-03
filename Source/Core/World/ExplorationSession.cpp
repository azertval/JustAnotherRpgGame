// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/World/ExplorationSession.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <optional>
#include <utility>

#include "Core/Combat/CombatTransition.h"
#include "Core/Gameplay/Interaction.h"
#include "Core/Gameplay/MapEntitySpawner.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/EntityPresence.h"

namespace core {

GridPosition cellOf(CellPoint point) noexcept {
    return {.column = static_cast<int>(std::floor(point.column)),
            .row = static_cast<int>(std::floor(point.row))};
}

CellPoint cellCenter(GridPosition cell) noexcept {
    return {.column = static_cast<float>(cell.column) + 0.5F,
            .row = static_cast<float>(cell.row) + 0.5F};
}

ExplorationSession::ExplorationSession(WorldTravel::MapLoader loader)
    : _travel(std::move(loader)) {}

bool ExplorationSession::start(std::string_view mapId, std::string_view arrival) {
    if (_travel.enter(mapId, arrival) != TravelResult::Moved) {
        return false;
    }
    rebuildInteractables();
    _hero = cellCenter(_travel.position());
    _lastCell = _travel.position();
    resetZones();
    lineUpFollowers();
    return true;
}

void ExplorationSession::placeHero(CellPoint point) noexcept {
    _hero = point;
    _lastCell = cellOf(point);
    resetZones();
    lineUpFollowers();
}

void ExplorationSession::setFollowers(std::size_t count) {
    _followers = count;
    // La trace garde de quoi poser le dernier suiveur, et un peu plus : un point de trop coute
    // moins qu'un suiveur qui attend au bout d'une trace trop courte.
    _trail.keep((static_cast<float>(count) + 1.0F) * FollowTrail::SPACING_CELLS);
    lineUpFollowers();
}

void ExplorationSession::lineUpFollowers() {
    std::vector<TrailPoint> points{TrailPoint{_hero.column, _hero.row}};
    if (_followers > 0 && map() != nullptr) {
        // Dans le dos du heros, par petits pas, tant que son gabarit y tiendrait : ce qui est
        // ainsi pose est un chemin qu'il aurait pu faire, et les suiveurs n'y sont jamais dans le
        // plein. Un mur derriere lui arrete la file ; les derniers attendent sur le meme point,
        // et se deplieront des qu'il marchera.
        constexpr float PAS = 0.1F;
        const float longueur = static_cast<float>(_followers) * FollowTrail::SPACING_CELLS;
        const Vector2 dos =
            (_facing.x == 0.0F && _facing.y == 0.0F) ? Vector2{0.0F, -1.0F} : -_facing.normalized();
        // La distance s'accumule pas a pas, en flottant : c'est elle qui situe chaque point.
        const float limite = longueur + (PAS / 2.0F);
        float parcouru = PAS;
        while (parcouru <= limite) {
            const CellPoint essai{.column = _hero.column + (dos.x * parcouru),
                                  .row = _hero.row + (dos.y * parcouru)};
            if (!fits(essai)) {
                break;
            }
            points.emplace_back(essai.column, essai.row);
            parcouru += PAS;
        }
    }
    _trail.reset(points);
}

CellPoint ExplorationSession::followerPoint(std::size_t rank) const {
    const TrailPoint point =
        _trail.pointBehind(static_cast<float>(rank + 1) * FollowTrail::SPACING_CELLS);
    return {.column = point.x, .row = point.y};
}

Vector2 ExplorationSession::followerFacing(std::size_t rank) const {
    return _trail.directionAt(static_cast<float>(rank + 1) * FollowTrail::SPACING_CELLS);
}

GridPosition ExplorationSession::aimedCell() const {
    return core::aimedCell(heroCell(), _facing);
}

void ExplorationSession::rebuildInteractables() {
    _interactables.clear();
    _blocked.clear();
    const Level* carte = map();
    if (carte == nullptr) {
        _seenRevision = _flags.revision();
        return;
    }
    // Meme table et meme fabrique de cle que le peuplement ECS (`spawnMapEntities`) : la session du
    // jeu n'a pas d'ECS, mais elle ne peut pas avoir sa PROPRE idee de ce qui est interactif.
    _seenRevision = _flags.revision();
    for (const MapEntity& objet : carte->entities()) {
        if (!isEntityPresent(objet, _flags)) {
            continue;
        }
        if (objet.type == PROP_ENTITY_TYPE) {
            // Un decor present arrete le pas sur son emprise, sauf s'il dit le contraire.
            const auto arrete = objet.properties.find(std::string{PROP_BLOCKS_PROPERTY});
            const bool* const valeur =
                arrete != objet.properties.end() ? std::get_if<bool>(&arrete->second) : nullptr;
            if (valeur == nullptr || *valeur) {
                const std::vector<GridPosition> emprise = zoneCells(objet);
                _blocked.insert(_blocked.end(), emprise.begin(), emprise.end());
            }
            continue;
        }
        const auto famille =
            std::ranges::find(knownInteractableKinds(), objet.type, &InteractableKind::type);
        if (famille == knownInteractableKinds().end()) {
            continue;
        }
        Interactable interactif;
        interactif.type = objet.type;
        interactif.position = objet.position;
        interactif.promptKey = std::string{famille->promptKey};
        if (famille->consumable) {
            interactif.consumedFlag =
                keyForEntity(mapId(), objet.type, objet.position.column, objet.position.row);
        }
        _interactables.push_back(std::move(interactif));
    }
}

bool ExplorationSession::fits(CellPoint point) const {
    const Level* carte = map();
    if (carte == nullptr) {
        return false;
    }
    const TileMap& collision = carte->tileMap();
    // Les quatre coins du gabarit : un heros qui tient dans un couloir d'une case ne doit pas
    // pouvoir couper l'angle d'un mur par sa moitie de case.
    const std::array<float, 2> cotes = {-HERO_HALF_SIZE_CELLS, HERO_HALF_SIZE_CELLS};
    for (const float dx : cotes) {
        for (const float dy : cotes) {
            const GridPosition coin =
                cellOf(CellPoint{.column = point.column + dx, .row = point.row + dy});
            if (!collision.inBounds(coin.column, coin.row) ||
                collision.isSolid(coin.column, coin.row) ||
                std::ranges::find(_blocked, coin) != _blocked.end()) {
                return false;
            }
        }
    }
    return true;
}

void ExplorationSession::walk(Vector2 move, float seconds) {
    if (move.x == 0.0F && move.y == 0.0F) {
        return;
    }
    _facing = move;
    const float pas = WALK_SPEED_CELLS_PER_SECOND * seconds;
    // Axe par axe : un mur pris en biais fait glisser le long au lieu d'arreter net, ce qui est la
    // difference entre un couloir jouable et un couloir ou l'on s'accroche.
    const CellPoint enX{.column = _hero.column + (move.x * pas), .row = _hero.row};
    if (fits(enX)) {
        _hero = enX;
    }
    const CellPoint enY{.column = _hero.column, .row = _hero.row + (move.y * pas)};
    if (fits(enY)) {
        _hero = enY;
    }
}

void ExplorationSession::crossPortal(std::vector<ExplorationEvent>& events) {
    const GridPosition ici = heroCell();
    if (ici == _lastCell) {
        return;
    }
    _lastCell = ici;
    const Level* carte = map();
    if (carte == nullptr) {
        return;
    }
    const std::optional<PortalTarget> portail = portalAt(*carte, ici);
    if (!portail.has_value()) {
        return;
    }
    switch (_travel.cross(ici, _flags)) {
        case TravelResult::Moved:
            arrived(events);
            break;
        case TravelResult::Sealed:
            events.push_back(ExplorationEvent{
                .kind = ExplorationEventKind::PortalSealed, .value = portail->map, .cell = ici});
            break;
        case TravelResult::Locked:
            events.push_back(ExplorationEvent{.kind = ExplorationEventKind::PortalLocked,
                                              .value = portail->requiredFlag,
                                              .cell = ici});
            break;
        case TravelResult::UnreadableMap:
        case TravelResult::UnknownArrival:
            events.push_back(ExplorationEvent{
                .kind = ExplorationEventKind::PortalBroken, .value = portail->map, .cell = ici});
            break;
        case TravelResult::NoPortal:
            break;
    }
}

void ExplorationSession::arrived(std::vector<ExplorationEvent>& events) {
    rebuildInteractables();
    _hero = cellCenter(_travel.position());
    _lastCell = _travel.position();
    resetZones();
    lineUpFollowers();
    events.push_back(ExplorationEvent{
        .kind = ExplorationEventKind::MapEntered, .value = mapId(), .cell = _travel.position()});
}

namespace {

// Vrai si la zone @p entity declenche quelque chose a l'entree (LOT-126).
[[nodiscard]] bool hasTrigger(const MapEntity& entity) {
    if (entity.type != ZONE_ENTITY_TYPE) {
        return false;
    }
    return std::ranges::any_of(std::array{ZONE_TRIGGER_DIALOGUE_PROPERTY,
                                          ZONE_TRIGGER_FLAG_PROPERTY, ZONE_TRIGGER_MAP_PROPERTY},
                               [&entity](std::string_view key) {
                                   const auto found = entity.properties.find(std::string{key});
                                   const std::string* const text =
                                       found != entity.properties.end()
                                           ? std::get_if<std::string>(&found->second)
                                           : nullptr;
                                   return text != nullptr && !text->empty();
                               });
}

[[nodiscard]] std::string textOf(const MapEntity& entity, std::string_view key) {
    const auto found = entity.properties.find(std::string{key});
    const std::string* const text =
        found != entity.properties.end() ? std::get_if<std::string>(&found->second) : nullptr;
    return text != nullptr ? *text : std::string{};
}

// Les rangs des zones a declencheur, presentes, qui couvrent @p cell.
[[nodiscard]] std::vector<std::size_t> triggerZonesAt(const Level& map, GridPosition cell,
                                                      const WorldFlags& flags) {
    std::vector<std::size_t> zones;
    const std::vector<MapEntity>& entities = map.entities();
    for (std::size_t index = 0; index < entities.size(); ++index) {
        const MapEntity& entity = entities[index];
        if (!hasTrigger(entity) || !isEntityPresent(entity, flags)) {
            continue;
        }
        const std::vector<GridPosition> cells = zoneCells(entity);
        if (std::ranges::find(cells, cell) != cells.end()) {
            zones.push_back(index);
        }
    }
    return zones;
}

}  // namespace

void ExplorationSession::resetZones() {
    const Level* carte = map();
    _insideZones =
        carte != nullptr ? triggerZonesAt(*carte, heroCell(), _flags) : std::vector<std::size_t>{};
}

void ExplorationSession::enterZones(std::vector<ExplorationEvent>& events) {
    const Level* carte = map();
    if (carte == nullptr) {
        return;
    }
    const std::vector<std::size_t> ici = triggerZonesAt(*carte, heroCell(), _flags);
    std::vector<std::size_t> entrees;
    std::ranges::set_difference(ici, _insideZones, std::back_inserter(entrees));
    _insideZones = ici;
    for (const std::size_t rang : entrees) {
        // Copie : un transfert change la carte courante, et l'entite avec elle.
        const MapEntity zone = carte->entities()[rang];
        // Une zone « une fois » garde sa trace dans un fait fabrique, comme un coffre ouvert.
        const auto once = zone.properties.find(std::string{ZONE_TRIGGER_ONCE_PROPERTY});
        const bool* const unique =
            once != zone.properties.end() ? std::get_if<bool>(&once->second) : nullptr;
        if (unique != nullptr && *unique) {
            const std::string fait =
                keyForEntity(mapId(), zone.type, zone.position.column, zone.position.row);
            if (_flags.isSet(fait)) {
                continue;
            }
            _flags.set(fait);
        }
        // Le drapeau d'abord : le dialogue qui s'ouvre, ou la carte d'arrivee, le voient deja.
        if (const std::string drapeau = textOf(zone, ZONE_TRIGGER_FLAG_PROPERTY);
            !drapeau.empty()) {
            const std::string valeur = textOf(zone, ZONE_TRIGGER_VALUE_PROPERTY);
            if (valeur.empty()) {
                _flags.set(drapeau);
            } else {
                _flags.setValue(drapeau, valeur);
            }
        }
        if (const std::string dialogue = textOf(zone, ZONE_TRIGGER_DIALOGUE_PROPERTY);
            !dialogue.empty()) {
            events.push_back(ExplorationEvent{
                .kind = ExplorationEventKind::Dialogue, .value = dialogue, .cell = heroCell()});
        }
        if (const std::string cible = textOf(zone, ZONE_TRIGGER_MAP_PROPERTY); !cible.empty()) {
            const GridPosition depart = heroCell();
            if (_travel.enter(cible, textOf(zone, ZONE_TRIGGER_ARRIVAL_PROPERTY)) ==
                TravelResult::Moved) {
                arrived(events);
            } else {
                events.push_back(ExplorationEvent{
                    .kind = ExplorationEventKind::PortalBroken, .value = cible, .cell = depart});
            }
            return;  // ailleurs : les autres zones de la case sont restees derriere.
        }
    }
}

void ExplorationSession::resolveInteraction(std::vector<ExplorationEvent>& events) {
    const Level* carte = map();
    if (carte == nullptr) {
        return;
    }
    std::vector<InteractionCandidate> candidats;
    candidats.reserve(_interactables.size());
    for (std::size_t rang = 0; rang < _interactables.size(); ++rang) {
        candidats.push_back(
            InteractionCandidate{.interactable = &_interactables[rang], .index = rang});
    }
    const InteractionTarget cible = findInteractionTarget({_hero.column, _hero.row}, _facing,
                                                          carte->tileMap(), candidats, _flags);
    if (!cible.found()) {
        return;
    }
    const InteractionOutcome issue = interact(cible, _flags);
    if (!issue.happened) {
        return;
    }
    // La cible est designee sur la liste des interactifs, qui suit l'ordre des entites de la
    // carte ; l'entite d'origine porte les proprietes (dialogue, rencontre) que la liste ne copie
    // pas. On la retrouve par sa case et son type, l'identite meme d'une entite de carte.
    const GridPosition ou = _interactables[cible.index].position;
    for (const MapEntity& objet : carte->entities()) {
        // Une entite absente peut partager la case d'une presente : l'enfant rendu a sa mere.
        if (objet.position != ou || objet.type != issue.type || !isEntityPresent(objet, _flags)) {
            continue;
        }
        if (const std::optional<DialogueTrigger> parole = dialogueTriggerFor(objet);
            parole.has_value()) {
            events.push_back(ExplorationEvent{
                .kind = ExplorationEventKind::Dialogue, .value = parole->dialogueId, .cell = ou});
            return;
        }
        if (const std::optional<EncounterTrigger> combat = encounterTriggerFor(objet, mapId());
            combat.has_value()) {
            events.push_back(ExplorationEvent{
                .kind = ExplorationEventKind::Encounter, .value = combat->encounterId, .cell = ou});
            return;
        }
        break;
    }
    events.push_back(ExplorationEvent{
        .kind = ExplorationEventKind::Interacted, .value = issue.type, .cell = ou});
}

void ExplorationSession::setQuests(QuestCatalog quests) {
    _quests = std::move(quests);
    declareQuestFlags(_quests, _flags);
    advanceQuests(_quests, _flags);
    rebuildInteractables();
}

std::vector<ExplorationEvent> ExplorationSession::refreshFromFlags() {
    std::vector<ExplorationEvent> events;
    if (_flags.revision() == _seenRevision) {
        return events;
    }
    for (const QuestEvent& etape : advanceQuests(_quests, _flags)) {
        events.push_back(ExplorationEvent{.kind = ExplorationEventKind::QuestAdvanced,
                                          .value = etape.quest + "/" + etape.step,
                                          .cell = heroCell()});
    }
    rebuildInteractables();
    return events;
}

bool ExplorationSession::isPresent(const MapEntity& entity) const {
    return isEntityPresent(entity, _flags);
}

float ExplorationSession::shownMinutes() const {
    const Level* const current = map();
    const std::optional<float> fixed =
        current != nullptr ? mapFixedMinutes(current->properties()) : std::nullopt;
    return fixed.value_or(_clock.minutes());
}

std::vector<ExplorationEvent> ExplorationSession::update(const ExplorationIntent& intent,
                                                         float seconds) {
    // Avant le gel : un dialogue ouvert a pu poser un drapeau, et la quete doit avancer meme si
    // la carte attend que la conversation se referme.
    std::vector<ExplorationEvent> events = refreshFromFlags();
    if (_frozen || map() == nullptr || seconds <= 0.0F) {
        return events;
    }
    // L'heure du monde passe avec la marche (LOT-1007) : gelee comme elle pendant un dialogue ou
    // un combat.
    _clock.advance(seconds);
    walk(intent.move, seconds);
    // Les suiveurs mettent leurs pas dans ceux du heros : la trace s'allonge de ce qu'il vient de
    // faire, avant qu'un portail ne la remette a zero sur la carte d'arrivee.
    _trail.record(TrailPoint{_hero.column, _hero.row});
    crossPortal(events);
    enterZones(events);
    if (intent.interact) {
        resolveInteraction(events);
    }
    // Une interaction a pu poser un drapeau (un coffre ouvert) : ses consequences, dans le meme
    // pas.
    std::vector<ExplorationEvent> suite = refreshFromFlags();
    events.insert(events.end(), suite.begin(), suite.end());
    return events;
}

}  // namespace core
