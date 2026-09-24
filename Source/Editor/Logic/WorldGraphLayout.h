// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Core/Math/Vector2.h"
#include "Core/World/WorldGraph.h"

/**
 * @file Editor/Logic/WorldGraphLayout.h
 * @brief Disposition de la vue « graphe du monde » du navigateur de cartes (`LOT-11`) : où poser
 *        chaque carte, quelles flèches tirer, et ce qui est sous le pointeur. Pur et testable —
 *        `hmi::WorldGraphView` ne fait que peindre ce qui est calculé ici.
 *
 * Les coordonnées sont en **pixels logiques à l'échelle 1**, centrées sur l'origine : la vue
 * translate et réduit l'ensemble pour le faire tenir dans sa surface.
 */

namespace hmi {

/// Rayon du disque d'une carte, en pixels logiques.
inline constexpr float WORLD_GRAPH_NODE_RADIUS = 26.0f;

/// Distance minimale entre les centres de deux cartes voisines sur le cercle : la largeur d'une
/// étiquette (nom et identifiant, posés sous le disque) plus une marge.
inline constexpr float WORLD_GRAPH_NODE_SPACING = 160.0f;

/// Rayon plancher du cercle dès qu'il porte deux cartes ou plus.
inline constexpr float WORLD_GRAPH_MIN_CIRCLE_RADIUS = 120.0f;

/// @brief Une carte posée sur le cercle — réelle, ou « fantôme ».
struct WorldGraphLayoutNode {
    /// Identifiant de la carte ; pour un fantôme, la carte nommée par les portails (vide si les
    /// portails n'en nomment aucune).
    std::string mapId;
    /// Nom de la carte ; vide pour un fantôme ou une carte illisible.
    std::string name;
    /// Vrai si la carte n'existe pas dans le dossier : seuls des portails la nomment.
    bool ghost = false;
    /// Vrai si la carte existe mais n'a pas pu être lue (`core::WorldMapNode::loadError`).
    bool unreadable = false;
    /// Message technique de lecture, vide sauf si `unreadable`.
    std::string loadError;
    /// Centre du disque.
    core::Vector2 center;
};

/// @brief Une flèche : tous les portails d'une carte vers une autre, dessinés une seule fois.
struct WorldGraphLayoutEdge {
    /// Indice du nœud source dans `WorldGraphLayout::nodes`.
    std::size_t from = 0;
    /// Indice du nœud cible dans `WorldGraphLayout::nodes`.
    std::size_t to = 0;
    /// Statut affiché : le premier statut non résolu des portails, dans l'ordre de `portals`, ou
    /// `Resolved` si tous le sont.
    core::PortalLinkStatus status = core::PortalLinkStatus::Resolved;
    /// Vrai si au moins un portail de la flèche n'est pas résolu — un portail condamné excepté.
    bool broken = false;
    /// Vrai si au moins un portail de la flèche est condamné (`LOT-126`) : pointillé, sans erreur.
    bool sealed = false;
    /// Vrai si la flèche part d'une carte et y revient (portail vers sa propre carte).
    bool selfLoop = false;
    /// Indices des portails regroupés, dans `core::WorldGraph::portals`, dans leur ordre.
    std::vector<std::size_t> portals;

    /// @return Le nombre de portails regroupés (affiché en pastille s'il dépasse 1).
    [[nodiscard]] std::size_t count() const noexcept {
        return portals.size();
    }
};

/// @brief La disposition complète du graphe.
struct WorldGraphLayout {
    /// Les cartes réelles triées par identifiant, puis les fantômes triés par identifiant.
    std::vector<WorldGraphLayoutNode> nodes;
    /// Les flèches, dans l'ordre de leur premier portail.
    std::vector<WorldGraphLayoutEdge> edges;
    /// Rayon du cercle portant les nœuds (0 pour zéro ou un nœud).
    float circleRadius = 0.0f;
};

/**
 * @brief Rayon du cercle pour @p nodeCount nœuds.
 *
 * **Règle** : deux voisins sur le cercle sont séparés d'une corde `2·R·sin(π/n)` ; on prend le
 * plus petit `R` qui la porte à `WORLD_GRAPH_NODE_SPACING`, sans descendre sous
 * `WORLD_GRAPH_MIN_CIRCLE_RADIUS`. Zéro ou un nœud : rayon nul (le nœud unique est au centre).
 */
[[nodiscard]] float worldGraphCircleRadius(std::size_t nodeCount) noexcept;

/**
 * @brief Dispose @p graph : nœuds sur un cercle, fantômes et flèches regroupées.
 *
 * Les nœuds sont posés dans l'ordre de `WorldGraphLayout::nodes`, le premier en haut, puis dans le
 * sens horaire (Y vers le bas). Un portail `UnknownMap` ou `MissingTarget` pointe sur un fantôme
 * portant l'identifiant manquant (vide pour `MissingTarget`) — un seul fantôme par identifiant,
 * quel que soit le nombre de portails qui le nomment. Les portails d'une même paire **ordonnée**
 * (source, cible) forment une seule flèche. Le résultat ne dépend que de @p graph.
 */
[[nodiscard]] WorldGraphLayout layoutWorldGraph(const core::WorldGraph& graph);

/**
 * @brief Le nœud sous @p point.
 * @param layout Disposition dans laquelle chercher.
 * @param point Position à tester, dans le même repère que @p layout.
 * @param radius Distance maximale au centre (en général `WORLD_GRAPH_NODE_RADIUS`).
 * @return L'indice du nœud le plus proche à moins de @p radius (bord inclus) ; à distance égale,
 *         le plus petit indice. `std::nullopt` si aucun.
 */
[[nodiscard]] std::optional<std::size_t> nodeAt(const WorldGraphLayout& layout, core::Vector2 point,
                                                float radius);

/// @brief Le tracé d'une flèche, prêt à peindre.
struct WorldGraphEdgeGeometry {
    /// Départ du trait, au bord du disque source (hors boucle).
    core::Vector2 start;
    /// Arrivée du trait (pointe), au bord du disque cible (hors boucle).
    core::Vector2 end;
    /// Pour une boucle : centre et rayon du cercle dessiné contre le nœud.
    core::Vector2 loopCenter;
    float loopRadius = 0.0f;
    /// Position de la pastille de compte (milieu du trait, ou sommet extérieur de la boucle).
    core::Vector2 badge;
};

/**
 * @brief Tracé de la flèche @p edgeIndex.
 *
 * Quand la flèche inverse existe (A→B et B→A), chacune est décalée perpendiculairement sur sa
 * droite, pour ne pas se superposer. Une boucle est un cercle posé contre le nœud, en diagonale
 * montante du côté extérieur (à droite pour un nœud au centre ou dans la moitié droite, à gauche
 * sinon) : jamais dessous, où l'étiquette est écrite.
 */
[[nodiscard]] WorldGraphEdgeGeometry worldGraphEdgeGeometry(const WorldGraphLayout& layout,
                                                            std::size_t edgeIndex);

/**
 * @brief La flèche sous @p point : distance au trait (ou au cercle d'une boucle) au plus
 *        @p tolerance.
 * @return L'indice de la flèche la plus proche ; à distance égale, le plus petit indice.
 */
[[nodiscard]] std::optional<std::size_t> edgeAt(const WorldGraphLayout& layout, core::Vector2 point,
                                                float tolerance);

}  // namespace hmi
