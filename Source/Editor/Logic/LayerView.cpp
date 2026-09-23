// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/LayerView.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace hmi {

std::vector<LayerRow> layerRows(const std::vector<core::TileLayer>& layers) {
    const bool hasVisual = std::ranges::any_of(
        layers, [](const core::TileLayer& layer) { return core::isVisualLayerKind(layer.kind); });
    std::vector<LayerRow> rows;
    rows.push_back(
        LayerRow{.slot = std::nullopt,
                 .kind = hasVisual ? core::LayerKind::Collision : core::LayerKind::Legacy,
                 .name = {},
                 .floor = 0});
    for (std::size_t index = 0; index < layers.size(); ++index) {
        if (core::isVisualLayerKind(layers[index].kind)) {
            rows.push_back(LayerRow{.slot = index,
                                    .kind = layers[index].kind,
                                    .name = layers[index].name,
                                    .floor = layers[index].floor});
        }
    }
    return rows;
}

LayerSlot validActiveLayer(const std::vector<core::TileLayer>& layers, LayerSlot active) {
    if (active && *active < layers.size() && core::isVisualLayerKind(layers[*active].kind)) {
        return active;
    }
    return std::nullopt;
}

void LayerViewState::reset() noexcept {
    _layers.clear();
    _rootVisible = true;
    _rootDimmed = false;
    _rootLocked = false;
    _rootOpacity.reset();
}

void LayerViewState::sync(std::size_t layerCount) {
    _layers.resize(layerCount);
}

LayerDisplay LayerViewState::display(LayerSlot slot, bool hasVisualLayers) const {
    if (!slot) {
        const float fallback = hasVisualLayers ? DEFAULT_COLLISION_OVERLAY_OPACITY : 1.0F;
        return LayerDisplay{.visible = _rootVisible,
                            .opacity = _rootOpacity.value_or(fallback),
                            .dimmed = _rootDimmed,
                            .locked = _rootLocked};
    }
    return *slot < _layers.size() ? _layers[*slot] : LayerDisplay{};
}

void LayerViewState::setVisible(LayerSlot slot, bool visible) {
    if (!slot) {
        _rootVisible = visible;
        return;
    }
    at(*slot).visible = visible;
}

void LayerViewState::setOpacity(LayerSlot slot, float opacity) {
    if (!std::isfinite(opacity)) {
        return;
    }
    const float bounded = std::clamp(opacity, 0.0F, 1.0F);
    if (!slot) {
        _rootOpacity = bounded;
        return;
    }
    at(*slot).opacity = bounded;
}

void LayerViewState::setDimmed(LayerSlot slot, bool dimmed) {
    if (!slot) {
        _rootDimmed = dimmed;
        return;
    }
    at(*slot).dimmed = dimmed;
}

void LayerViewState::setLocked(LayerSlot slot, bool locked) {
    if (!slot) {
        _rootLocked = locked;
        return;
    }
    at(*slot).locked = locked;
}

void LayerViewState::swap(std::size_t a, std::size_t b) {
    std::swap(at(a), at(b));
}

LayerDisplay& LayerViewState::at(std::size_t index) {
    if (index >= _layers.size()) {
        _layers.resize(index + 1);
    }
    return _layers[index];
}

}  // namespace hmi
