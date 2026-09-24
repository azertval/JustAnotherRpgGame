// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/World/EntityKinds.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "Core/Combat/Arena.h"
#include "Core/Combat/BattleGrid.h"
#include "Core/Combat/CombatTransition.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/CityBlock.h"
#include "Core/World/CombatZone.h"
#include "Core/World/EntityPresence.h"

namespace core {

namespace {

// Proprietes lues par core::encounterTriggerFor (CombatTransition.cpp) : ecrites en dur la-bas, et
// nommees ici une seule fois pour la table.
constexpr std::string_view ENCOUNTER_ID_PROPERTY = "encounterId";
constexpr std::string_view ENCOUNTER_RESPAWNS_PROPERTY = "respawns";

[[nodiscard]] EntityPropertySpec choice(std::string_view key, EntityChoiceSource source,
                                        bool required) {
    return EntityPropertySpec{.key = key,
                              .kind = EntityPropertyKind::Choice,
                              .source = source,
                              .fixedChoices = {},
                              .required = required,
                              .defaultValue = std::string{}};
}

[[nodiscard]] EntityPropertySpec text(std::string_view key, bool required) {
    return EntityPropertySpec{.key = key,
                              .kind = EntityPropertyKind::Text,
                              .source = EntityChoiceSource::Fixed,
                              .fixedChoices = {},
                              .required = required,
                              .defaultValue = std::string{}};
}

[[nodiscard]] EntityPropertySpec boolean(std::string_view key) {
    return EntityPropertySpec{.key = key,
                              .kind = EntityPropertyKind::Boolean,
                              .source = EntityChoiceSource::Fixed,
                              .fixedChoices = {},
                              .required = false,
                              .defaultValue = false};
}

// Un entier requis, borne par le bas, qui vaut son minimum a la creation.
[[nodiscard]] EntityPropertySpec atLeast(std::string_view key, std::int64_t minimum,
                                         bool required = true) {
    return EntityPropertySpec{.key = key,
                              .kind = EntityPropertyKind::Integer,
                              .source = EntityChoiceSource::Fixed,
                              .fixedChoices = {},
                              .required = required,
                              .defaultValue = minimum,
                              .minimum = minimum};
}

// Vrai si @p value a le type qu'attend @p kind.
[[nodiscard]] bool hasExpectedType(const PropertyValue& value, EntityPropertyKind kind) {
    switch (kind) {
        case EntityPropertyKind::Text:
        case EntityPropertyKind::Choice:
            return std::holds_alternative<std::string>(value);
        case EntityPropertyKind::Integer:
            return std::holds_alternative<std::int64_t>(value);
        case EntityPropertyKind::Boolean:
            return std::holds_alternative<bool>(value);
    }
    return false;
}

[[nodiscard]] std::string textOf(const PropertyMap& properties, std::string_view key) {
    const auto found = properties.find(std::string{key});
    if (found == properties.end()) {
        return {};
    }
    const auto* const text = std::get_if<std::string>(&found->second);
    return text != nullptr ? *text : std::string{};
}

}  // namespace

const EntityPropertySpec* EntityKind::find(std::string_view key) const {
    const auto found = std::ranges::find(properties, key, &EntityPropertySpec::key);
    return found != properties.end() ? &*found : nullptr;
}

const std::vector<EntityKind>& knownEntityKinds() {
    // Construite une seule fois. L'ordre est celui de la liste de l'editeur : ce qu'on pose le plus
    // souvent d'abord.
    static const std::vector<EntityKind> familles = {
        // Coffre et panneau (LOT-10) : aucune propriete lue par le gameplay aujourd'hui. Le contenu
        // d'un coffre arrive avec le butin (LOT-26) ; la table ne l'invente pas avant.
        EntityKind{.type = "chest", .properties = {}},
        EntityKind{.type = "sign", .properties = {}},
        // PNJ (LOT-15) : un figurant sans dialogue est legal, d'ou `required = false`. Sa figurine
        // (LOT-91) : un PNJ sans figurine se parle et ne se dessine pas (LOT-09). La sentinelle
        // d'une porte gardee (LOT-96) nomme le quartier qu'elle ferme, par sa fiche d'atlas.
        EntityKind{.type = NPC_ENTITY_TYPE,
                   .properties = {choice(NPC_DIALOGUE_PROPERTY, EntityChoiceSource::Dialogues,
                                         /*required=*/false),
                                  choice(NPC_FIGURE_PROPERTY, EntityChoiceSource::Figures,
                                         /*required=*/false),
                                  choice(NPC_GUARDED_DISTRICT_PROPERTY,
                                         EntityChoiceSource::Locations, /*required=*/false)},
                   .labelProperty = NPC_DIALOGUE_PROPERTY,
                   .figureProperty = NPC_FIGURE_PROPERTY},
        // Rencontre (LOT-18) : sans rencontre nommee, le declencheur n'en est pas un.
        EntityKind{.type = ENCOUNTER_ENTITY_TYPE,
                   .properties = {choice(ENCOUNTER_ID_PROPERTY, EntityChoiceSource::Encounters,
                                         /*required=*/true),
                                  boolean(ENCOUNTER_RESPAWNS_PROPERTY)},
                   .labelProperty = ENCOUNTER_ID_PROPERTY},
        // Portail et point d'arrivee (LOT-11, traverses au LOT-09). Le drapeau exige n'est pas
        // requis : un portail ordinaire s'ouvre toujours.
        EntityKind{.type = PORTAL_ENTITY_TYPE,
                   .properties = {choice(PORTAL_TARGET_MAP_PROPERTY, EntityChoiceSource::Maps,
                                         /*required=*/true),
                                  choice(PORTAL_ARRIVAL_PROPERTY, EntityChoiceSource::ArrivalPoints,
                                         /*required=*/true),
                                  choice(PORTAL_REQUIRED_FLAG_PROPERTY, EntityChoiceSource::Flags,
                                         /*required=*/false)},
                   .labelProperty = PORTAL_TARGET_MAP_PROPERTY},
        EntityKind{.type = SPAWN_POINT_ENTITY_TYPE,
                   .properties = {text(SPAWN_POINT_NAME_PROPERTY, /*required=*/true)},
                   .labelProperty = SPAWN_POINT_NAME_PROPERTY},
        // Zone de combat (LOT-09) : le rectangle nomme ou l'on se bat, et lui seul (EX-LVL-018).
        EntityKind{.type = COMBAT_ZONE_ENTITY_TYPE,
                   .properties = {text(COMBAT_ZONE_NAME_PROPERTY, /*required=*/true),
                                  atLeast(COMBAT_ZONE_WIDTH_PROPERTY, 1),
                                  atLeast(COMBAT_ZONE_HEIGHT_PROPERTY, 1)},
                   .shape = EntityShape::Rectangle,
                   .labelProperty = COMBAT_ZONE_NAME_PROPERTY},
        // Ilot d'un quartier (LOT-96) : le rectangle nomme que le plan de la ville montre.
        EntityKind{.type = CITY_BLOCK_ENTITY_TYPE,
                   .properties = {text(CITY_BLOCK_NAME_PROPERTY, /*required=*/true),
                                  atLeast(CITY_BLOCK_WIDTH_PROPERTY, 1),
                                  atLeast(CITY_BLOCK_HEIGHT_PROPERTY, 1)},
                   .shape = EntityShape::Rectangle,
                   .labelProperty = CITY_BLOCK_NAME_PROPERTY},
        // Zone de regles (D13, lue par BattleGrid) : un rectangle ou des cases peintes. Sa taille
        // n'est pas requise -- une zone peinte n'en a pas. Le nom n'est lu par personne : il sert a
        // la reconnaitre dans la liste.
        EntityKind{.type = ZONE_ENTITY_TYPE,
                   .properties = {text(ZONE_NAME_PROPERTY, /*required=*/false),
                                  boolean(DIFFICULT_TERRAIN_PROPERTY),
                                  atLeast(ZONE_WIDTH_PROPERTY, 1, /*required=*/false),
                                  atLeast(ZONE_HEIGHT_PROPERTY, 1, /*required=*/false)},
                   .shape = EntityShape::Area,
                   .labelProperty = ZONE_NAME_PROPERTY},
        // Trajet (LOT-70, LOT-82) : ses horaires viendront avec l'horloge du LOT-70.
        EntityKind{.type = ROUTE_ENTITY_TYPE,
                   .properties = {text(ROUTE_NAME_PROPERTY, /*required=*/true),
                                  boolean(ROUTE_LOOP_PROPERTY)},
                   .shape = EntityShape::Path,
                   .labelProperty = ROUTE_NAME_PROPERTY},
        // Entree d'arene (LOT-50).
        EntityKind{.type = ARENA_ENTRY_ENTITY_TYPE,
                   .properties = {EntityPropertySpec{.key = ARENA_SIDE_PROPERTY,
                                                     .kind = EntityPropertyKind::Choice,
                                                     .source = EntityChoiceSource::Fixed,
                                                     .fixedChoices = {"allies", "enemies"},
                                                     .required = true,
                                                     .defaultValue = std::string{"allies"}},
                                  atLeast(ARENA_RANK_PROPERTY, 1)},
                   .labelProperty = ARENA_SIDE_PROPERTY},
    };
    return familles;
}

const EntityKind* findEntityKind(std::string_view type) {
    const auto found = std::ranges::find(knownEntityKinds(), type, &EntityKind::type);
    return found != knownEntityKinds().end() ? &*found : nullptr;
}

MapEntity makeEntity(const EntityKind& kind, GridPosition position) {
    MapEntity entity{.type = std::string{kind.type}, .position = position, .properties = {}};
    for (const EntityPropertySpec& spec : kind.properties) {
        entity.properties.emplace(std::string{spec.key}, spec.defaultValue);
    }
    return entity;
}

std::set<std::string, std::less<>> arrivalPointNames(const std::vector<MapEntity>& entities) {
    std::set<std::string, std::less<>> names;
    for (const MapEntity& entity : entities) {
        if (entity.type != SPAWN_POINT_ENTITY_TYPE) {
            continue;
        }
        if (std::string name = textOf(entity.properties, SPAWN_POINT_NAME_PROPERTY);
            !name.empty()) {
            names.insert(std::move(name));
        }
    }
    return names;
}

namespace {

// Defaut d'une propriete de choix non vide : la valeur n'est pas dans la liste que la source
// designe. Rien pour une valeur admise.
[[nodiscard]] std::optional<EntityIssueCode> choiceIssue(const MapEntity& entity,
                                                         const EntityPropertySpec& spec,
                                                         const std::string& text,
                                                         const EntityReferenceContext& context) {
    switch (spec.source) {
        case EntityChoiceSource::Fixed:
            if (std::ranges::find(spec.fixedChoices, std::string_view{text}) ==
                spec.fixedChoices.end()) {
                return EntityIssueCode::InvalidChoice;
            }
            break;
        case EntityChoiceSource::Dialogues:
            if (!context.dialogues.contains(text)) {
                return EntityIssueCode::UnknownDialogue;
            }
            break;
        case EntityChoiceSource::Encounters:
            if (!context.encounters.contains(text)) {
                return EntityIssueCode::UnknownEncounter;
            }
            break;
        case EntityChoiceSource::Maps:
            if (!context.arrivalPointsByMap.contains(text)) {
                return EntityIssueCode::UnknownTargetMap;
            }
            break;
        case EntityChoiceSource::ArrivalPoints: {
            // Un point d'arrivee ne se juge que dans une carte connue : une carte inconnue
            // est deja signalee, et la signaler deux fois n'apprendrait rien.
            const auto target = context.arrivalPointsByMap.find(
                textOf(entity.properties, PORTAL_TARGET_MAP_PROPERTY));
            if (target != context.arrivalPointsByMap.end() && !target->second.contains(text)) {
                return EntityIssueCode::UnknownArrivalPoint;
            }
            break;
        }
        case EntityChoiceSource::Figures:
            if (!context.figures.contains(text)) {
                return EntityIssueCode::UnknownFigure;
            }
            break;
        case EntityChoiceSource::Flags:
            if (!context.flags.contains(text)) {
                return EntityIssueCode::UnsetFlag;
            }
            break;
        case EntityChoiceSource::Locations:
            if (!context.locations.contains(text)) {
                return EntityIssueCode::UnknownLocation;
            }
            break;
        case EntityChoiceSource::Items:
            if (!context.items.contains(text)) {
                return EntityIssueCode::UnknownItem;
            }
            break;
        case EntityChoiceSource::EntityRefs:
            if (!context.entityRefs.contains(text)) {
                return EntityIssueCode::UnknownEntityRef;
            }
            break;
    }
    return std::nullopt;
}

// Defaut d'une propriete de l'entite au regard de sa specification, avec la valeur a citer.
// Rien pour une propriete conforme, ou facultative et absente.
[[nodiscard]] std::optional<std::pair<EntityIssueCode, std::string>> propertyIssue(
    const MapEntity& entity, const EntityPropertySpec& spec,
    const EntityReferenceContext& context) {
    const auto found = entity.properties.find(std::string{spec.key});
    if (found == entity.properties.end()) {
        if (spec.required) {
            return std::pair{EntityIssueCode::MissingProperty, std::string{}};
        }
        return std::nullopt;
    }
    if (!hasExpectedType(found->second, spec.kind)) {
        return std::pair{EntityIssueCode::WrongValueType, std::string{}};
    }
    if (const auto* const integer = std::get_if<std::int64_t>(&found->second)) {
        if (*integer < spec.minimum || *integer > spec.maximum) {
            return std::pair{EntityIssueCode::OutOfRange, std::to_string(*integer)};
        }
        return std::nullopt;
    }
    const auto* const text = std::get_if<std::string>(&found->second);
    if (text == nullptr) {
        return std::nullopt;  // booleen du bon type : rien d'autre a verifier.
    }
    if (text->empty()) {
        if (spec.required) {
            return std::pair{EntityIssueCode::MissingProperty, std::string{}};
        }
        return std::nullopt;
    }
    if (spec.kind != EntityPropertyKind::Choice) {
        return std::nullopt;
    }
    if (const std::optional<EntityIssueCode> code = choiceIssue(entity, spec, *text, context)) {
        return std::pair{*code, *text};
    }
    return std::nullopt;
}

}  // namespace

namespace {

// La condition de presence d'une entite : bien formee, et sur un drapeau qu'un dialogue ou une
// quete pose -- sans quoi le PNJ ne paraitrait (ou ne partirait) jamais.
template <class Report>
void presenceIssues(const MapEntity& entity, const EntityReferenceContext& context,
                    Report&& report) {
    const PresenceRead read = presenceConditionOf(entity);
    switch (read.issue) {
        case PresenceIssue::None:
            break;
        case PresenceIssue::WrongValueType:
            report(EntityIssueCode::WrongValueType, PRESENCE_FLAG_PROPERTY, std::string{});
            return;
        case PresenceIssue::UnknownTest:
            report(EntityIssueCode::InvalidPresence, PRESENCE_TEST_PROPERTY,
                   textOf(entity.properties, PRESENCE_TEST_PROPERTY));
            return;
        case PresenceIssue::MissingValue:
            report(EntityIssueCode::InvalidPresence, PRESENCE_VALUE_PROPERTY, std::string{});
            return;
        case PresenceIssue::MissingFlag:
            report(EntityIssueCode::InvalidPresence, PRESENCE_FLAG_PROPERTY, std::string{});
            return;
    }
    if (read.condition && !context.flags.contains(read.condition->flag)) {
        report(EntityIssueCode::UnsetFlag, PRESENCE_FLAG_PROPERTY, read.condition->flag);
    }
}

}  // namespace

std::vector<EntityIssue> validateMapEntities(const std::vector<MapEntity>& entities,
                                             const EntityReferenceContext& context) {
    std::vector<EntityIssue> issues;
    const auto report = [&issues](std::size_t index, EntityIssueCode code, std::string_view key,
                                  std::string value) {
        issues.push_back(EntityIssue{.entityIndex = index,
                                     .code = code,
                                     .key = std::string{key},
                                     .value = std::move(value)});
    };

    // Noms de points d'arrivee deja vus : le SECOND porteur d'un nom est signale, jamais le
    // premier.
    std::set<std::string, std::less<>> seenArrivals;

    for (std::size_t index = 0; index < entities.size(); ++index) {
        const MapEntity& entity = entities[index];
        // La condition de presence vaut pour TOUTE famille, connue ou non (LOT-116) : elle se
        // controle ici, une fois, et non propriete par propriete dans la table.
        presenceIssues(entity, context,
                       [&](EntityIssueCode code, std::string_view key, std::string value) {
                           report(index, code, key, std::move(value));
                       });
        const EntityKind* const kind = findEntityKind(entity.type);
        if (kind == nullptr) {
            report(index, EntityIssueCode::UnknownType, {}, entity.type);
            continue;
        }

        for (const EntityPropertySpec& spec : kind->properties) {
            if (auto issue = propertyIssue(entity, spec, context)) {
                report(index, issue->first, spec.key, std::move(issue->second));
            }
        }

        if (entity.type == SPAWN_POINT_ENTITY_TYPE) {
            const std::string name = textOf(entity.properties, SPAWN_POINT_NAME_PROPERTY);
            if (!name.empty() && !seenArrivals.insert(name).second) {
                report(index, EntityIssueCode::DuplicateArrivalPoint, SPAWN_POINT_NAME_PROPERTY,
                       name);
            }
        }
    }
    return issues;
}

}  // namespace core
