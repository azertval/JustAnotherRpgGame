// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/MapEntity.h"

/**
 * @file Core/World/WorldGraph.h
 * @brief Le graphe **statique** du monde : les cartes du dossier des niveaux, et les portails qui
 *        les relient (`LOT-11`, étendu par `LOT-09`).
 *
 * Ce fichier ne porte que la **lecture** du graphe — ce que montre la vue « graphe du monde » du
 * navigateur de cartes. Le `LOT-09` y ajoutera le chargement de carte à chaud et la traversée d'un
 * portail : il partira de `WorldGraph` tel qu'il est ici, sans en changer le contrat. Un portail
 * dont le statut n'est pas `PortalLinkStatus::Resolved` est précisément l'erreur de chargement
 * explicite que le `LOT-09` exige (`EX-NFR-040`).
 */

namespace core {

/// @brief Une carte telle que `buildWorldGraph` la reçoit : déjà lue, ou en échec de lecture.
struct WorldMapInput {
    /// Identifiant : le nom du fichier de la carte, sans extension.
    std::string mapId;
    /// Nom de la carte (`core::Level::name`). Vide si la carte est illisible.
    std::string name;
    std::vector<MapEntity> entities;
    /// Message technique du chargeur, vide si la carte a été lue.
    std::string loadError;
};

/// @brief Un nœud du graphe : une carte, et les points d'arrivée qu'elle offre.
struct WorldMapNode {
    std::string mapId;
    std::string name;
    /// Noms des points d'arrivée de la carte, triés, sans doublon.
    std::vector<std::string> arrivalPoints;
    /// Message technique du chargeur, vide si la carte a été lue. Une carte illisible reste un
    /// nœud : la retirer ferait passer ses portails entrants pour des cibles inconnues.
    std::string loadError;
    /// Identifiants des entités de la carte (décision D8), triés : ce qu'un `carte#id` peut citer.
    std::vector<std::string> entityIds;
    /// Les drapeaux que les zones de la carte posent à l'entrée (`LOT-126`), triés : ils comptent
    /// parmi ce qu'un PNJ, un portail ou un dialogue peut attendre.
    std::vector<std::string> triggerFlags{};
};

/// @brief Ce que vaut un portail. `Core` n'écrit pas de texte (`EX-NFR-011`) : l'éditeur traduit.
enum class PortalLinkStatus {
    /// La carte cible et son point d'arrivée existent.
    Resolved,
    /// Le portail ne nomme aucune carte cible.
    MissingTarget,
    /// La carte cible n'est pas dans le dossier.
    UnknownMap,
    /// Le portail ne nomme aucun point d'arrivée.
    MissingArrival,
    /// La carte cible n'offre pas ce point d'arrivée.
    UnknownArrival,
    /// La carte cible existe mais n'a pas pu être lue : ses points d'arrivée sont inconnaissables.
    TargetUnreadable,
    /// Le portail est **condamné** (`core::PORTAL_SEALED_PROPERTY`, `LOT-126`) : voulu, il ne mène
    /// nulle part — ni une erreur, ni un chemin.
    Sealed,
};

/// @brief Ce qui fait passer d'une carte à l'autre.
enum class WorldLinkKind {
    /// Un portail : on marche dessus.
    Portal,
    /// Le transfert d'une zone (`core::ZONE_TRIGGER_MAP_PROPERTY`, `LOT-126`) : on y entre, et
    /// l'on est ailleurs. Un chemin comme un autre pour l'atteignabilité.
    Transfer,
};

/// @brief Une arête du graphe : un portail d'une carte, et ce qu'il atteint.
struct WorldPortalLink {
    std::string fromMap;
    /// Case du portail sur sa carte source.
    GridPosition position;
    /// Carte cible, telle que le portail la nomme (éventuellement vide ou inconnue).
    std::string toMap;
    /// Point d'arrivée, tel que le portail le nomme.
    std::string arrival;
    PortalLinkStatus status = PortalLinkStatus::Resolved;
    WorldLinkKind kind = WorldLinkKind::Portal;

    [[nodiscard]] bool operator==(const WorldPortalLink&) const = default;
};

/**
 * @brief Les cartes et leurs portails.
 *
 * L'ordre est **déterministe** — cartes par identifiant, portails par carte source puis dans
 * l'ordre des entités — pour que deux lectures du même dossier donnent le même graphe, et la même
 * vue.
 */
struct WorldGraph {
    /// Les cartes, triées par identifiant.
    std::vector<WorldMapNode> maps;
    /// Les portails, par carte source, puis dans l'ordre des entités de cette carte.
    std::vector<WorldPortalLink> portals;

    /// @return La carte portant cet identifiant, ou `nullptr`.
    [[nodiscard]] const WorldMapNode* find(std::string_view mapId) const;

    /// @return Les portails posés sur @p mapId, dans l'ordre de `portals`.
    [[nodiscard]] std::vector<const WorldPortalLink*> portalsFrom(std::string_view mapId) const;

    /// @return Les portails qui nomment @p mapId comme cible, quel que soit leur statut, dans
    ///         l'ordre de `portals`.
    [[nodiscard]] std::vector<const WorldPortalLink*> portalsTo(std::string_view mapId) const;

    /**
     * @brief Cartes qu'aucun chemin de portails résolus ne relie à @p fromMapId (hors elle-même),
     *        triées.
     *
     * Les portails se suivent dans leur **sens** : une carte d'où l'on peut partir mais où rien ne
     * mène est injoignable. Si @p fromMapId n'est pas une carte du graphe, toutes le sont.
     */
    [[nodiscard]] std::vector<std::string> unreachableFrom(std::string_view fromMapId) const;
};

/**
 * @brief Construit le graphe à partir de cartes déjà lues.
 *
 * Un portail est une entité de type `core::PORTAL_ENTITY_TYPE` ; ses propriétés ne comptent que
 * si elles sont du texte (toute autre valeur vaut « vide »). Un point d'arrivée est une entité de
 * type `core::SPAWN_POINT_ENTITY_TYPE` au nom texte non vide. Un portail vers sa propre carte est
 * légal.
 *
 * Le statut se décide dans cet ordre : portail condamné, cible vide, carte inconnue, carte
 * illisible, arrivée vide, arrivée inconnue — le premier qui s'applique l'emporte.
 *
 * Une zone qui nomme une carte de transfert (`core::ZONE_TRIGGER_MAP_PROPERTY`) donne une arête
 * `WorldLinkKind::Transfer`, jugée comme un portail (`LOT-126`).
 */
[[nodiscard]] WorldGraph buildWorldGraph(std::vector<WorldMapInput> maps);

/**
 * @brief L'identifiant de carte d'un fichier de niveau : son chemin relatif à @p levelsDir, sans
 *        extension, en barres obliques (`capital/martpart`, `LOT-96`).
 */
[[nodiscard]] std::string mapIdOf(const std::filesystem::path& levelsDir,
                                  const std::filesystem::path& file);

/**
 * @brief Lit chaque `*.json` de @p levelsDir **et de ses sous-dossiers** (identifiant :
 *        `core::mapIdOf`) ; une carte illisible reste un nœud, avec son erreur.
 *
 * Les fichiers `sequence-*.json` sont écartés, comme le fait le navigateur de cartes : ce ne sont
 * pas des cartes. Un dossier absent donne un graphe vide, sans lever (`EX-NFR-040`).
 */
[[nodiscard]] WorldGraph loadWorldGraph(const std::filesystem::path& levelsDir);

}  // namespace core
