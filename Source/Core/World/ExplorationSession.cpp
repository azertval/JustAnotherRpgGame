// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/World/ExplorationSession.h"

#include <algorithm>
#include <array>
#include <cmath>
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
    return true;
}

void ExplorationSession::placeHero(CellPoint point) noexcept {
    _hero = point;
    _lastCell = cellOf(point);
}

GridPosition ExplorationSession::aimedCell() const {
    return core::aimedCell(heroCell(), _facing);
}

void ExplorationSession::rebuildInteractables() {
    _interactables.clear();
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
                collision.isSolid(coin.column, coin.row)) {
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
            rebuildInteractables();
            _hero = cellCenter(_travel.position());
            _lastCell = _travel.position();
            events.push_back(ExplorationEvent{.kind = ExplorationEventKind::MapEntered,
                                              .value = mapId(),
                                              .cell = _travel.position()});
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
    const InteractionTarget cible =
        findInteractionTarget(heroCell(), _facing, carte->tileMap(), candidats, _flags);
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

std::vector<ExplorationEvent> ExplorationSession::update(const ExplorationIntent& intent,
                                                         float seconds) {
    // Avant le gel : un dialogue ouvert a pu poser un drapeau, et la quete doit avancer meme si
    // la carte attend que la conversation se referme.
    std::vector<ExplorationEvent> events = refreshFromFlags();
    if (_frozen || map() == nullptr || seconds <= 0.0F) {
        return events;
    }
    walk(intent.move, seconds);
    crossPortal(events);
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
