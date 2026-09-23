// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/CanvasScene.h"

#include <algorithm>
#include <optional>

#include "Core/Levels/LevelDraft.h"
#include "HMI/Graphics/PlaceAppearance.h"

namespace hmi {

namespace {

/// @return Le rang de la première couche de sorte @p kind : celle que la composition lit.
[[nodiscard]] LayerSlot firstLayerOf(const std::vector<core::TileLayer>& layers,
                                     core::LayerKind kind) {
    const auto found = std::ranges::find(layers, kind, &core::TileLayer::kind);
    return found == layers.end()
               ? LayerSlot{}
               : LayerSlot{static_cast<std::size_t>(std::distance(layers.begin(), found))};
}

}  // namespace

IsoBandOpacity isoBandOpacity(const std::vector<core::TileLayer>& layers,
                              const LayerViewState& view, LayerSlot active, bool seeThroughRelief) {
    const bool hasVisual = std::ranges::any_of(
        layers, [](const core::TileLayer& layer) { return core::isVisualLayerKind(layer.kind); });
    IsoBandOpacity bands;
    const LayerDisplay root = view.display(std::nullopt, hasVisual);
    if (!hasVisual) {
        // Grille unique : elle est l'image, et son sol est celui que la composition lit.
        bands.floors = root.effectiveOpacity();
    } else {
        const LayerSlot ground = firstLayerOf(layers, core::LayerKind::Ground);
        bands.floors = ground ? view.display(ground, true).effectiveOpacity() : 1.0F;
        // La collision se voit quand on la peint, et seulement alors.
        bands.collision = !active ? root.effectiveOpacity() * COLLISION_MASK_OPACITY : 0.0F;
    }
    const LayerSlot decor = firstLayerOf(layers, core::LayerKind::Decor);
    bands.relief = decor ? view.display(decor, hasVisual).effectiveOpacity() : 1.0F;
    // Chaque etage, par sa premiere couche (LOT-129) : la cacher cache l'etage.
    std::array<bool, core::MAX_STOREY_FLOOR> seen{};
    for (std::size_t index = 0; index < layers.size(); ++index) {
        const core::TileLayer& layer = layers[index];
        if (layer.kind != core::LayerKind::Decor || layer.floor < 1 ||
            layer.floor > core::MAX_STOREY_FLOOR) {
            continue;
        }
        const auto rank = static_cast<std::size_t>(layer.floor - 1);
        if (!seen[rank]) {
            seen[rank] = true;
            bands.storeys[rank] = view.display(LayerSlot{index}, hasVisual).effectiveOpacity();
        }
    }
    if (seeThroughRelief) {
        bands.relief *= SEE_THROUGH_RELIEF_OPACITY;
        for (float& storey : bands.storeys) {
            storey *= SEE_THROUGH_RELIEF_OPACITY;
        }
    }
    return bands;
}

float bandOpacity(const IsoBandOpacity& bands, const ComposedQuad& quad) noexcept {
    if (quad.storey >= 1 && quad.storey <= core::MAX_STOREY_FLOOR) {
        return bands.storeys[static_cast<std::size_t>(quad.storey - 1)];
    }
    return bandOpacity(bands, quad.layer);
}

float bandOpacity(const IsoBandOpacity& bands, RenderLayer layer) noexcept {
    switch (layer) {
        case RenderLayer::Tile:
            return bands.floors;
        case RenderLayer::Object:
            return bands.relief;
        case RenderLayer::Player:
            return bands.figures;
        default:
            return 1.0F;
    }
}

WorldSceneSnapshot canvasSnapshot(const core::LevelDraft& draft,
                                  const PlaceAppearance& appearance) {
    // Une image fixe des bandes : le canevas n'anime pas les figurines.
    return snapshotWorldScene(worldSceneSource(draft), appearance, npcFigures(draft.entities(), 0));
}

std::vector<WorldFigureSnapshot> formationFigures(const core::EncounterTerrain& terrain,
                                                  const std::vector<std::string>& figures) {
    std::vector<WorldFigureSnapshot> formation;
    for (const core::CombatantPlacement& placement : terrain.placements) {
        const std::string figure = "Monsters/" + placement.creatureId;
        if (!std::ranges::binary_search(figures, figure)) {
            continue;
        }
        formation.push_back(
            WorldFigureSnapshot{.figure = figure,
                                .clip = "idle",
                                .point = {static_cast<float>(placement.position.column) + 0.5F,
                                          static_cast<float>(placement.position.row) + 0.5F},
                                .frame = 0});
    }
    return formation;
}

std::string cellPieces(const WorldSceneSnapshot& snapshot, core::GridPosition cell) {
    std::string pieces{snapshot.floorAt(cell)};
    const std::string_view relief = snapshot.reliefAt(cell);
    if (!relief.empty()) {
        if (!pieces.empty()) {
            pieces += " · ";
        }
        pieces += relief;
    }
    return pieces;
}

}  // namespace hmi
