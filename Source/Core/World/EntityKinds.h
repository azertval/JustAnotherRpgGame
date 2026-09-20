// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/LevelProperties.h"
#include "Core/Levels/MapEntity.h"

/**
 * @file Core/World/EntityKinds.h
 * @brief Les familles d'entités de carte que l'éditeur sait poser, et ce qu'une carte leur doit
 *        (`LOT-11`).
 */

namespace core {

/// @brief Type d'entité d'un portail : il mène à une autre carte (`LOT-09` le traversera).
inline constexpr std::string_view PORTAL_ENTITY_TYPE = "portal";
/// @brief Propriété d'un portail : la carte cible, nommée par son **identifiant** — le nom de son
///        fichier dans `Source/Elements/Levels/`, sans extension.
inline constexpr std::string_view PORTAL_TARGET_MAP_PROPERTY = "targetMap";
/// @brief Propriété d'un portail : le point d'arrivée **nommé** de la carte cible. Jamais des
///        coordonnées, qui se désynchroniseraient au premier redimensionnement de la cible.
inline constexpr std::string_view PORTAL_ARRIVAL_PROPERTY = "arrival";
/// @brief Propriété d'un portail : le drapeau de monde qu'il exige pour s'ouvrir (`LOT-16` le
///        pose ; le `LOT-09` le lit). Absent ou vide, le portail est toujours franchissable.
inline constexpr std::string_view PORTAL_REQUIRED_FLAG_PROPERTY = "requiresFlag";
/// @brief Type d'entité d'un point d'arrivée nommé : là où l'on apparaît en entrant par un portail.
inline constexpr std::string_view SPAWN_POINT_ENTITY_TYPE = "spawnPoint";
/// @brief Propriété d'un point d'arrivée : son nom, unique dans la carte.
inline constexpr std::string_view SPAWN_POINT_NAME_PROPERTY = "name";

/// @brief Nature d'une propriété d'entité, qui décide du contrôle que l'éditeur lui donne.
enum class EntityPropertyKind {
    Text,
    Integer,
    Boolean,
    /// Une valeur texte choisie dans une liste (fixe, ou tirée d'un catalogue).
    Choice,
};

/// @brief D'où viennent les choix d'une propriété `Choice`.
///
/// Une source est un **catalogue** : l'inspecteur de l'éditeur en tire sa liste de choix, et
/// `validateMapEntities` signale une valeur qui n'y est pas.
enum class EntityChoiceSource {
    /// Les valeurs de `EntityPropertySpec::fixedChoices`.
    Fixed,
    /// Les dialogues **acceptés** au chargement (`core::loadDialogues`).
    Dialogues,
    /// Les rencontres du catalogue (`core::loadEncounters`).
    Encounters,
    /// Les cartes du dossier des niveaux, par identifiant.
    Maps,
    /// Les points d'arrivée de la carte nommée par `targetMap` de la **même** entité.
    ArrivalPoints,
    /// Les figurines des ateliers (`LOT-91`, `LOT-93`) : un slug de PNJ (`anariel`), ou le dossier
    /// d'un monstre (`Monsters/lion`).
    Figures,
    /// Les drapeaux de monde qu'un dialogue pose (`LOT-15`, et les quêtes du `LOT-16`). Un drapeau
    /// que rien ne pose est signalé : le portail qui l'exige ne s'ouvrirait jamais.
    Flags,
    /// Les fiches de lieu de l'atlas (`World/locations`, `LOT-37`) : un quartier, une ville.
    Locations,
    /// Les objets du catalogue (`Rpg/items`) : le contenu d'un coffre, l'étal d'un marchand
    /// (`LOT-26`).
    Items,
    /// Une entité d'une carte, par `carte#id` (décision D8) : ce que quêtes et drapeaux citent.
    EntityRefs,
};

/**
 * @brief La forme d'une famille d'entités sur la carte : ce que le canevas de l'éditeur dessine et
 *        les poignées qu'il lui donne (`LOT-EDITOR-05`).
 *
 * C'est la forme, et non le type, que l'éditeur connaît : une famille nouvelle déclare la sienne
 * ici et reçoit poignées, tracé et déplacement sans une ligne de code d'éditeur (§5, règle 2).
 */
enum class EntityShape {
    /// Une case : un coffre, un PNJ, un portail.
    Point,
    /// Un rectangle : la case est son coin haut-gauche, `width` et `height` sa taille. Le jeu ne
    /// lit
    /// que cette forme (zone de combat, îlot).
    Rectangle,
    /// Une zone de règles (décision D13) : un rectangle, **ou** un ensemble de cases peint
    /// (`MapEntity::cells`) ; la case est alors une case de la zone.
    Area,
    /// Une ligne brisée : les points de passage dans `MapEntity::cells`, **dans l'ordre**, le
    /// premier sur la case de l'entité (un trajet de PNJ, une ronde).
    Path,
};

/// @brief Propriété d'une forme `Rectangle` ou `Area` : sa largeur, en cases.
inline constexpr std::string_view SHAPE_WIDTH_PROPERTY = "width";
/// @brief Propriété d'une forme `Rectangle` ou `Area` : sa hauteur, en cases.
inline constexpr std::string_view SHAPE_HEIGHT_PROPERTY = "height";

/// @brief Propriété d'une zone de règles (`core::ZONE_ENTITY_TYPE`) : son nom, que seul l'éditeur
///        montre.
inline constexpr std::string_view ZONE_NAME_PROPERTY = "name";

/// @brief Type d'entité d'un trajet : la ligne brisée qu'un PNJ parcourt (`LOT-70`, `LOT-82`).
inline constexpr std::string_view ROUTE_ENTITY_TYPE = "route";
/// @brief Propriété d'un trajet : son nom, unique dans la carte, que le PNJ citera.
inline constexpr std::string_view ROUTE_NAME_PROPERTY = "name";
/// @brief Propriété d'un trajet : vrai pour une ronde, qui revient à son premier point.
inline constexpr std::string_view ROUTE_LOOP_PROPERTY = "loop";

/// @brief Une propriété qu'une famille d'entités déclare.
struct EntityPropertySpec {
    std::string_view key;
    EntityPropertyKind kind = EntityPropertyKind::Text;
    EntityChoiceSource source = EntityChoiceSource::Fixed;
    std::vector<std::string_view> fixedChoices;
    /// Une propriété requise absente (ou texte vide) est signalée ; elle n'empêche pas
    /// d'enregistrer.
    bool required = false;
    /// Valeur posée à la création de l'entité.
    PropertyValue defaultValue;
    /// Bornes d'une propriété `Integer`, incluses ; une valeur hors bornes est signalée.
    std::int64_t minimum = (std::numeric_limits<std::int64_t>::min)();
    std::int64_t maximum = (std::numeric_limits<std::int64_t>::max)();
};

/// @brief Une famille d'entités : son type, les propriétés qu'elle déclare, et comment le canevas
///        de l'éditeur la montre.
struct EntityKind {
    std::string_view type;
    std::vector<EntityPropertySpec> properties{};
    /// Sa forme sur la carte.
    EntityShape shape = EntityShape::Point;
    /// La propriété que le canevas écrit à côté d'elle (la carte cible d'un portail) ; vide : rien.
    std::string_view labelProperty{};
    /// La propriété qui nomme sa figurine (source `Figures`) : le canevas dessine la figurine à la
    /// place du marqueur quand elle existe. Vide : la famille n'a pas de figurine.
    std::string_view figureProperty{};

    [[nodiscard]] const EntityPropertySpec* find(std::string_view key) const;
};

/**
 * @brief Les familles que l'éditeur sait poser, dans l'ordre de sa liste.
 *
 * Coffre, panneau, PNJ, rencontre, portail, point d'arrivée, zone de combat, îlot, zone de règles,
 * trajet, entrée d'arène. Les types et leurs propriétés sont ceux que le jeu lit déjà
 * (`core::knownInteractableKinds`, `core::dialogueTriggerFor`, `core::encounterTriggerFor`,
 * `core::arenaEntryPoints`, `core::BattleGrid`) : la table ne les invente pas, elle les rassemble.
 * Une seule exception, le **trajet**, que la feuille de route de l'éditeur demande avant que le jeu
 * ne le lise (`LOT-70`, `LOT-82`) : le format lui garde sa place, comme à la hauteur.
 *
 * Toute famille que le jeu lit **doit** y être : un test bloquant le vérifie (`EX-EDIT-073`). Un
 * type absent de la table reste **légal** sur une carte (`EX-NFR-040`) : l'éditeur le transporte et
 * en montre les propriétés brutes.
 */
[[nodiscard]] const std::vector<EntityKind>& knownEntityKinds();

/// @return La famille de @p type, ou `nullptr` si elle n'est pas dans la table.
[[nodiscard]] const EntityKind* findEntityKind(std::string_view type);

/// @brief Une entité neuve de la famille @p kind en @p position, ses propriétés à leur défaut.
[[nodiscard]] MapEntity makeEntity(const EntityKind& kind, GridPosition position);

/// @brief Ce qu'une carte peut référencer hors d'elle-même : les catalogues et les autres cartes.
struct EntityReferenceContext {
    std::set<std::string, std::less<>> dialogues;
    std::set<std::string, std::less<>> encounters;
    std::set<std::string, std::less<>> figures;
    /// Les drapeaux qu'un dialogue pose.
    std::set<std::string, std::less<>> flags;
    std::set<std::string, std::less<>> locations;
    std::set<std::string, std::less<>> items;
    /// `carte#id` de chaque entité des cartes, la carte éditée comprise.
    std::set<std::string, std::less<>> entityRefs;
    /// Identifiant de carte → noms de ses points d'arrivée. La carte **éditée** y figure aussi : un
    /// portail peut ramener ailleurs sur la même carte.
    std::map<std::string, std::set<std::string, std::less<>>, std::less<>> arrivalPointsByMap;
};

/// @brief Ce qui ne va pas dans une entité. `Core` n'écrit pas de texte (`EX-NFR-011`) : l'éditeur
///        traduit le code et ses arguments.
enum class EntityIssueCode {
    /// Le type n'est pas dans `knownEntityKinds` — toléré, signalé pour information.
    UnknownType,
    /// Une propriété requise manque ou est vide. `key` la nomme.
    MissingProperty,
    /// Une propriété a une valeur du mauvais type (un entier là où l'on attend un texte…).
    WrongValueType,
    /// Une valeur `Choice` fixe hors de sa liste. `key` et `value`.
    InvalidChoice,
    /// Le dialogue nommé n'est pas dans le catalogue, ou y a été refusé. `value`.
    UnknownDialogue,
    /// La rencontre nommée n'est pas dans le catalogue. `value`.
    UnknownEncounter,
    /// La carte cible d'un portail n'existe pas. `value`.
    UnknownTargetMap,
    /// Le point d'arrivée d'un portail n'existe pas dans sa carte cible. `value`.
    UnknownArrivalPoint,
    /// Deux points d'arrivée de la carte portent le même nom. `value`.
    DuplicateArrivalPoint,
    /// Un entier hors de ses bornes. `key` et `value`.
    OutOfRange,
    /// La figurine nommée n'est dans aucun atelier. `value`.
    UnknownFigure,
    /// Aucun dialogue ne pose ce drapeau. `value`.
    UnsetFlag,
    /// La fiche de lieu nommée n'est pas dans l'atlas. `value`.
    UnknownLocation,
    /// L'objet nommé n'est pas dans le catalogue. `value`.
    UnknownItem,
    /// Aucune carte n'a d'entité `carte#id`. `value`.
    UnknownEntityRef,
};

/// @brief Un problème relevé sur l'entité de rang `entityIndex`.
struct EntityIssue {
    std::size_t entityIndex = 0;
    EntityIssueCode code = EntityIssueCode::UnknownType;
    std::string key;
    std::string value;

    [[nodiscard]] bool operator==(const EntityIssue&) const = default;
};

/**
 * @brief Relève ce que les entités de @p entities référencent sans l'atteindre.
 *
 * **Avertit, ne refuse pas** : une carte s'écrit dans le désordre — le portail vers la forêt avant
 * la forêt —, et refuser de l'enregistrer tant que la cible n'existe pas imposerait un ordre de
 * travail que rien ne justifie. C'est au chargement du graphe du monde (`LOT-09`) qu'un portail
 * orphelin devient une erreur.
 *
 * @return Les problèmes, dans l'ordre des entités, puis des propriétés déclarées.
 */
[[nodiscard]] std::vector<EntityIssue> validateMapEntities(const std::vector<MapEntity>& entities,
                                                           const EntityReferenceContext& context);

/// @return Les noms des points d'arrivée de @p entities, sans doublon.
[[nodiscard]] std::set<std::string, std::less<>> arrivalPointNames(
    const std::vector<MapEntity>& entities);

}  // namespace core
