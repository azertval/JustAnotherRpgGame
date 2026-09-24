// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/WorldGraphLayout.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>
#include <utility>

namespace hmi {

namespace {

/// Décalage perpendiculaire de deux flèches opposées (A→B et B→A), en pixels logiques.
constexpr float REVERSE_EDGE_OFFSET = 7.0F;

/// Rayon du cercle d'une boucle, en proportion du rayon d'un nœud.
constexpr float LOOP_RADIUS_RATIO = 0.55F;

/// @return Vrai si le portail nomme une carte absente du dossier (ou n'en nomme aucune).
[[nodiscard]] bool targetsGhost(core::PortalLinkStatus status) noexcept {
    return status == core::PortalLinkStatus::UnknownMap ||
           status == core::PortalLinkStatus::MissingTarget;
}

/// @return La distance de @p point au segment [@p a, @p b].
[[nodiscard]] float distanceToSegment(core::Vector2 point, core::Vector2 a, core::Vector2 b) {
    const core::Vector2 ab = b - a;
    const float lengthSquared = ab.lengthSquared();
    if (lengthSquared <= 0.0F) {
        return (point - a).length();
    }
    const float t = std::clamp((point - a).dot(ab) / lengthSquared, 0.0F, 1.0F);
    return (point - (a + ab * t)).length();
}

}  // namespace

float worldGraphCircleRadius(std::size_t nodeCount) noexcept {
    if (nodeCount < 2) {
        return 0.0F;
    }
    const float halfAngle = std::numbers::pi_v<float> / static_cast<float>(nodeCount);
    return std::max(WORLD_GRAPH_MIN_CIRCLE_RADIUS,
                    WORLD_GRAPH_NODE_SPACING / (2.0F * std::sin(halfAngle)));
}

WorldGraphLayout layoutWorldGraph(const core::WorldGraph& graph) {
    WorldGraphLayout layout;

    // Cartes réelles, triées par identifiant (le graphe l'est déjà : on ne s'en remet pas à lui).
    std::vector<const core::WorldMapNode*> maps;
    maps.reserve(graph.maps.size());
    for (const core::WorldMapNode& map : graph.maps) {
        maps.push_back(&map);
    }
    std::ranges::stable_sort(
        maps, [](const auto* lhs, const auto* rhs) { return lhs->mapId < rhs->mapId; });

    std::map<std::string, std::size_t> realIndex;
    for (const core::WorldMapNode* map : maps) {
        if (realIndex.contains(map->mapId)) {
            continue;  // identifiant en double : un seul nœud.
        }
        realIndex.emplace(map->mapId, layout.nodes.size());
        layout.nodes.push_back(WorldGraphLayoutNode{.mapId = map->mapId,
                                                    .name = map->name,
                                                    .ghost = false,
                                                    .unreadable = !map->loadError.empty(),
                                                    .loadError = map->loadError,
                                                    .center = {}});
    }

    // Fantômes : identifiants nommés par des portails sans carte. Triés, puis posés après les
    // cartes réelles — leur indice n'est connu qu'une fois tous réunis.
    std::map<std::string, std::size_t> ghostIndex;
    const auto noteGhost = [&](const std::string& mapId) { ghostIndex.emplace(mapId, 0); };
    for (const core::WorldPortalLink& portal : graph.portals) {
        if (!realIndex.contains(portal.fromMap)) {
            noteGhost(portal.fromMap);  // ne survient pas avec buildWorldGraph ; par robustesse.
        }
        if (targetsGhost(portal.status) || !realIndex.contains(portal.toMap)) {
            noteGhost(portal.toMap);
        }
    }
    for (auto& [mapId, index] : ghostIndex) {
        index = layout.nodes.size();
        layout.nodes.push_back(WorldGraphLayoutNode{.mapId = mapId,
                                                    .name = {},
                                                    .ghost = true,
                                                    .unreadable = false,
                                                    .loadError = {},
                                                    .center = {}});
    }

    // Cercle : le premier nœud en haut, puis sens horaire (Y vers le bas).
    const std::size_t count = layout.nodes.size();
    layout.circleRadius = worldGraphCircleRadius(count);
    for (std::size_t i = 0; i < count; ++i) {
        const float angle =
            (-std::numbers::pi_v<float> / 2.0F) +
            (2.0F * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(count));
        layout.nodes[i].center = core::Vector2(layout.circleRadius * std::cos(angle),
                                               layout.circleRadius * std::sin(angle));
    }

    const auto indexOf = [&](const std::string& mapId, bool preferGhost) {
        if (!preferGhost) {
            if (const auto real = realIndex.find(mapId); real != realIndex.end()) {
                return real->second;
            }
        }
        return ghostIndex.at(mapId);
    };

    // Flèches : une par paire ordonnée (source, cible), dans l'ordre de leur premier portail.
    std::map<std::pair<std::size_t, std::size_t>, std::size_t> edgeIndex;
    for (std::size_t p = 0; p < graph.portals.size(); ++p) {
        const core::WorldPortalLink& portal = graph.portals[p];
        const std::size_t from = indexOf(portal.fromMap, false);
        const std::size_t to = indexOf(portal.toMap, targetsGhost(portal.status));
        auto [it, inserted] = edgeIndex.try_emplace({from, to}, layout.edges.size());
        if (inserted) {
            layout.edges.push_back(WorldGraphLayoutEdge{
                .from = from, .to = to, .selfLoop = from == to, .portals = {}});
        }
        WorldGraphLayoutEdge& edge = layout.edges[it->second];
        edge.portals.push_back(p);
        if (portal.status == core::PortalLinkStatus::Sealed) {
            edge.sealed = true;
        } else if (portal.status != core::PortalLinkStatus::Resolved && !edge.broken) {
            edge.broken = true;
            edge.status = portal.status;
        }
    }
    return layout;
}

std::optional<std::size_t> nodeAt(const WorldGraphLayout& layout, core::Vector2 point,
                                  float radius) {
    std::optional<std::size_t> best;
    float bestDistance = radius;
    for (std::size_t i = 0; i < layout.nodes.size(); ++i) {
        const float distance = (point - layout.nodes[i].center).length();
        if (distance <= radius && (!best || distance < bestDistance)) {
            best = i;
            bestDistance = distance;
        }
    }
    return best;
}

WorldGraphEdgeGeometry worldGraphEdgeGeometry(const WorldGraphLayout& layout,
                                              std::size_t edgeIndex) {
    WorldGraphEdgeGeometry geometry;
    const WorldGraphLayoutEdge& edge = layout.edges.at(edgeIndex);
    const core::Vector2 from = layout.nodes.at(edge.from).center;
    const core::Vector2 to = layout.nodes.at(edge.to).center;

    if (edge.selfLoop) {
        // En diagonale montante, du côté extérieur : jamais dessous, où est l'étiquette.
        const core::Vector2 outward =
            core::Vector2(from.x < 0.0F ? -1.0F : 1.0F, -1.0F).normalized();
        geometry.loopRadius = WORLD_GRAPH_NODE_RADIUS * LOOP_RADIUS_RATIO;
        geometry.loopCenter = from + outward * WORLD_GRAPH_NODE_RADIUS;
        geometry.start = geometry.loopCenter;
        geometry.end = geometry.loopCenter;
        geometry.badge = geometry.loopCenter + outward * geometry.loopRadius;
        return geometry;
    }

    const core::Vector2 direction = (to - from).normalized();
    const bool hasReverse = std::ranges::any_of(layout.edges, [&](const WorldGraphLayoutEdge& e) {
        return e.from == edge.to && e.to == edge.from;
    });
    const core::Vector2 offset =
        hasReverse ? core::Vector2(-direction.y, direction.x) * REVERSE_EDGE_OFFSET
                   : core::Vector2();
    geometry.start = from + direction * WORLD_GRAPH_NODE_RADIUS + offset;
    geometry.end = to - direction * WORLD_GRAPH_NODE_RADIUS + offset;
    geometry.badge = (geometry.start + geometry.end) * 0.5F;
    return geometry;
}

std::optional<std::size_t> edgeAt(const WorldGraphLayout& layout, core::Vector2 point,
                                  float tolerance) {
    std::optional<std::size_t> best;
    float bestDistance = tolerance;
    for (std::size_t i = 0; i < layout.edges.size(); ++i) {
        const WorldGraphEdgeGeometry geometry = worldGraphEdgeGeometry(layout, i);
        const float distance =
            layout.edges[i].selfLoop
                ? std::abs((point - geometry.loopCenter).length() - geometry.loopRadius)
                : distanceToSegment(point, geometry.start, geometry.end);
        if (distance <= tolerance && (!best || distance < bestDistance)) {
            best = i;
            bestDistance = distance;
        }
    }
    return best;
}

}  // namespace hmi
