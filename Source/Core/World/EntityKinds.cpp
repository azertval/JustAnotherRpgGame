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

[[nodiscard]] EntityPropertySpec boolean(std::string_view key, bool byDefault = false) {
    return EntityPropertySpec{.key = key,
                              .kind = EntityPropertyKind::Boolean,
                              .source = EntityChoiceSource::Fixed,
                              .fixedChoices = {},
                              .required = false,
                              .defaultValue = byDefault};
}

// Un choix qui depend d'une autre propriete de l'entite : la carte d'un point d'arrivee, le
// drapeau de ses valeurs.
[[nodiscard]] EntityPropertySpec related(std::string_view key, EntityChoiceSource source,
                                         std::string_view relatedKey, bool required = false) {
    EntityPropertySpec spec = choice(key, source, required);
    spec.relatedKey = relatedKey;
    return spec;
}

// Requise, sauf quand le booleen @p waivedBy est vrai.
[[nodiscard]] EntityPropertySpec waived(EntityPropertySpec spec, std::string_view waivedBy) {
    spec.waivedBy = waivedBy;
    return spec;
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
        // requis : un portail ordinaire s'ouvre toujours. Un portail condamne (LOT-126) n'a ni
        // cible ni arrivee a nommer.
        EntityKind{.type = PORTAL_ENTITY_TYPE,
                   .properties = {waived(choice(PORTAL_TARGET_MAP_PROPERTY,
                                                EntityChoiceSource::Maps, /*required=*/true),
                                         PORTAL_SEALED_PROPERTY),
                                  waived(related(PORTAL_ARRIVAL_PROPERTY,
                                                 EntityChoiceSource::ArrivalPoints,
                                                 PORTAL_TARGET_MAP_PROPERTY, /*required=*/true),
                                         PORTAL_SEALED_PROPERTY),
                                  choice(PORTAL_REQUIRED_FLAG_PROPERTY, EntityChoiceSource::Flags,
                                         /*required=*/false),
                                  boolean(PORTAL_SEALED_PROPERTY)},
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
        // Ses declencheurs (LOT-126) : ce qu'elle fait quand le heros y entre -- un dialogue, un
        // drapeau pose, un transfert. Aucun n'est requis : une zone de regles ne declenche rien.
        EntityKind{
            .type = ZONE_ENTITY_TYPE,
            .properties = {text(ZONE_NAME_PROPERTY, /*required=*/false),
                           boolean(DIFFICULT_TERRAIN_PROPERTY),
                           atLeast(ZONE_WIDTH_PROPERTY, 1, /*required=*/false),
                           atLeast(ZONE_HEIGHT_PROPERTY, 1, /*required=*/false),
                           choice(ZONE_TRIGGER_DIALOGUE_PROPERTY, EntityChoiceSource::Dialogues,
                                  /*required=*/false),
                           choice(ZONE_TRIGGER_FLAG_PROPERTY, EntityChoiceSource::WrittenFlags,
                                  /*required=*/false),
                           [] {
                               EntityPropertySpec value = related(ZONE_TRIGGER_VALUE_PROPERTY,
                                                                  EntityChoiceSource::FlagValues,
                                                                  ZONE_TRIGGER_FLAG_PROPERTY);
                               value.writesFlag = true;
                               return value;
                           }(),
                           choice(ZONE_TRIGGER_MAP_PROPERTY, EntityChoiceSource::Maps,
                                  /*required=*/false),
                           related(ZONE_TRIGGER_ARRIVAL_PROPERTY, EntityChoiceSource::ArrivalPoints,
                                   ZONE_TRIGGER_MAP_PROPERTY),
                           boolean(ZONE_TRIGGER_ONCE_PROPERTY)},
            .shape = EntityShape::Area,
            .labelProperty = ZONE_NAME_PROPERTY},
        // Decor qui change (LOT-126) : une piece du lieu, son emprise en rectangle -- prise au
        // manifeste quand on choisit la piece --, et le pas qu'elle arrete tant qu'elle est la.
        EntityKind{
            .type = PROP_ENTITY_TYPE,
            .properties = {choice(PROP_PIECE_PROPERTY, EntityChoiceSource::Pieces,
                                  /*required=*/true),
                           boolean(PROP_BLOCKS_PROPERTY, /*byDefault=*/true),
                           atLeast(SHAPE_WIDTH_PROPERTY, 1), atLeast(SHAPE_HEIGHT_PROPERTY, 1)},
            .shape = EntityShape::Rectangle,
            .labelProperty = PROP_PIECE_PROPERTY,
            .pieceProperty = PROP_PIECE_PROPERTY},
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

const std::vector<EntityPropertySpec>& commonEntityProperties() {
    static const std::vector<EntityPropertySpec> communes = {
        choice(PRESENCE_FLAG_PROPERTY, EntityChoiceSource::Flags, /*required=*/false),
        EntityPropertySpec{.key = PRESENCE_TEST_PROPERTY,
                           .kind = EntityPropertyKind::Choice,
                           .source = EntityChoiceSource::Fixed,
                           .fixedChoices = {"set", "unset", "equals", "notEquals"},
                           .required = false,
                           .defaultValue = std::string{}},
        related(PRESENCE_VALUE_PROPERTY, EntityChoiceSource::FlagValues, PRESENCE_FLAG_PROPERTY),
    };
    return communes;
}

std::vector<const EntityPropertySpec*> inspectedProperties(const EntityKind& kind) {
    std::vector<const EntityPropertySpec*> specs;
    specs.reserve(kind.properties.size() + commonEntityProperties().size());
    for (const EntityPropertySpec& spec : kind.properties) {
        specs.push_back(&spec);
    }
    for (const EntityPropertySpec& spec : commonEntityProperties()) {
        if (kind.find(spec.key) == nullptr) {
            specs.push_back(&spec);
        }
    }
    return specs;
}

const EntityPropertySpec* findInspectedProperty(const EntityKind& kind, std::string_view key) {
    if (const EntityPropertySpec* const own = kind.find(key)) {
        return own;
    }
    const auto found = std::ranges::find(commonEntityProperties(), key, &EntityPropertySpec::key);
    return found != commonEntityProperties().end() ? &*found : nullptr;
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

std::set<std::string, std::less<>> flagsSetByEntities(const std::vector<MapEntity>& entities) {
    std::set<std::string, std::less<>> flags;
    for (const MapEntity& entity : entities) {
        if (entity.type != ZONE_ENTITY_TYPE) {
            continue;
        }
        if (std::string flag = textOf(entity.properties, ZONE_TRIGGER_FLAG_PROPERTY);
            !flag.empty()) {
            flags.insert(std::move(flag));
        }
    }
    return flags;
}

bool isSealedPortal(const MapEntity& entity) {
    if (entity.type != PORTAL_ENTITY_TYPE) {
        return false;
    }
    const auto found = entity.properties.find(std::string{PORTAL_SEALED_PROPERTY});
    const bool* const sealed =
        found != entity.properties.end() ? std::get_if<bool>(&found->second) : nullptr;
    return sealed != nullptr && *sealed;
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

// Vrai si une quete declare pour @p flag chacune des valeurs de @p text (`a|b`) -- une seule
// valeur, telle quelle, si l'entite la POSE : on ne donne pas deux valeurs a un drapeau.
[[nodiscard]] bool declaresValues(const EntityReferenceContext& context, std::string_view flag,
                                  std::string_view text, bool single) {
    const auto declared = context.flagValues.find(flag);
    if (declared == context.flagValues.end()) {
        return false;  // aucune quete ne declare ce drapeau : il n'a pas de valeurs.
    }
    if (single) {
        return declared->second.contains(text);
    }
    const std::vector<std::string> values = splitFlagValues(text);
    return !values.empty() && std::ranges::all_of(values, [&declared](const std::string& value) {
        return declared->second.contains(value);
    });
}

// Le defaut @p code si @p text manque a @p ensemble, rien sinon.
template <class Ensemble>
[[nodiscard]] std::optional<EntityIssueCode> absentDe(const Ensemble& ensemble,
                                                      const std::string& text,
                                                      EntityIssueCode code) {
    return ensemble.contains(text) ? std::nullopt : std::optional<EntityIssueCode>{code};
}

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
            return absentDe(context.dialogues, text, EntityIssueCode::UnknownDialogue);
        case EntityChoiceSource::Encounters:
            return absentDe(context.encounters, text, EntityIssueCode::UnknownEncounter);
        case EntityChoiceSource::Maps:
            return absentDe(context.arrivalPointsByMap, text, EntityIssueCode::UnknownTargetMap);
        case EntityChoiceSource::ArrivalPoints: {
            // Un point d'arrivee ne se juge que dans une carte connue : une carte inconnue
            // est deja signalee, et la signaler deux fois n'apprendrait rien.
            const auto target = context.arrivalPointsByMap.find(
                textOf(entity.properties,
                       spec.relatedKey.empty() ? PORTAL_TARGET_MAP_PROPERTY : spec.relatedKey));
            if (target != context.arrivalPointsByMap.end() && !target->second.contains(text)) {
                return EntityIssueCode::UnknownArrivalPoint;
            }
            break;
        }
        case EntityChoiceSource::Figures:
            return absentDe(context.figures, text, EntityIssueCode::UnknownFigure);
        case EntityChoiceSource::Flags:
            return absentDe(context.flags, text, EntityIssueCode::UnsetFlag);
        case EntityChoiceSource::Locations:
            return absentDe(context.locations, text, EntityIssueCode::UnknownLocation);
        case EntityChoiceSource::Items:
            return absentDe(context.items, text, EntityIssueCode::UnknownItem);
        case EntityChoiceSource::EntityRefs:
            return absentDe(context.entityRefs, text, EntityIssueCode::UnknownEntityRef);
        case EntityChoiceSource::FlagValues:
            if (!declaresValues(context, textOf(entity.properties, spec.relatedKey), text,
                                spec.writesFlag)) {
                return EntityIssueCode::UndeclaredFlagValue;
            }
            break;
        case EntityChoiceSource::WrittenFlags:
            break;  // l'entite le pose : il n'a pas a l'etre ailleurs.
        case EntityChoiceSource::Pieces:
            // Un lieu inconnu ne dit rien de ses pieces : c'est le lieu qui manque, pas elles.
            if (!context.pieces.empty() && !context.pieces.contains(text)) {
                return EntityIssueCode::UnknownPiece;
            }
            break;
    }
    return std::nullopt;
}

// Vrai si @p spec est requise sur @p entity : par sa declaration, sauf si son booleen de dispense
// est vrai ; ou parce qu'elle donne sa valeur a un drapeau qu'une quete declare.
[[nodiscard]] bool isRequired(const MapEntity& entity, const EntityPropertySpec& spec,
                              const EntityReferenceContext& context) {
    if (spec.writesFlag &&
        context.flagValues.contains(textOf(entity.properties, spec.relatedKey))) {
        return true;
    }
    if (!spec.required) {
        return false;
    }
    if (spec.waivedBy.empty()) {
        return true;
    }
    const auto waiver = entity.properties.find(std::string{spec.waivedBy});
    const bool* const waived =
        waiver != entity.properties.end() ? std::get_if<bool>(&waiver->second) : nullptr;
    return waived == nullptr || !*waived;
}

// Defaut d'une propriete de l'entite au regard de sa specification, avec la valeur a citer.
// Rien pour une propriete conforme, ou facultative et absente.
[[nodiscard]] std::optional<std::pair<EntityIssueCode, std::string>> propertyIssue(
    const MapEntity& entity, const EntityPropertySpec& spec,
    const EntityReferenceContext& context) {
    const auto found = entity.properties.find(std::string{spec.key});
    const bool required = isRequired(entity, spec, context);
    if (found == entity.properties.end()) {
        if (required) {
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
        if (required) {
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
                    const Report& report) {
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
    if (!read.condition) {
        return;
    }
    if (!context.flags.contains(read.condition->flag)) {
        report(EntityIssueCode::UnsetFlag, PRESENCE_FLAG_PROPERTY, read.condition->flag);
    }
    // Une valeur qu'aucune quete ne declare pour ce drapeau (LOT-126) : `equals` ne tiendrait
    // jamais, `notEquals` toujours -- une faute de frappe dans les deux cas.
    const auto declared = context.flagValues.find(read.condition->flag);
    for (const std::string& value : read.condition->values) {
        if (declared == context.flagValues.end() || !declared->second.contains(value)) {
            report(EntityIssueCode::UndeclaredFlagValue, PRESENCE_VALUE_PROPERTY, value);
        }
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
