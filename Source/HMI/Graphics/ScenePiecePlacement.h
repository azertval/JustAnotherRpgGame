// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
#pragma once

#include <cmath>
#include <optional>
#include <string_view>

#include "Core/Data/JsonDocument.h"
#include "Core/Math/Vector2.h"

namespace hmi {

/// Opt-in : les manifestes historiques ne modifient pas le placement des cartes livrees.
[[nodiscard]] inline std::optional<core::Vector2> scenePieceAnchor(const nlohmann::json& root,
                                                                   std::string_view filename) {
    if (!root.is_object() || !root.contains("placementVersion") || root["placementVersion"] != 1 ||
        !root.contains("textures") || !root["textures"].is_object()) {
        return std::nullopt;
    }
    for (const auto& entry : root["textures"]) {
        if (!entry.is_object() || !entry.contains("file") || !entry["file"].is_string() ||
            entry["file"].get<std::string>() != filename || !entry.contains("anchor")) {
            continue;
        }
        const auto& anchor = entry["anchor"];
        if (!anchor.is_array() || anchor.size() != 2 || !anchor[0].is_number() ||
            !anchor[1].is_number()) {
            return std::nullopt;
        }
        const float x = anchor[0].get<float>();
        const float y = anchor[1].get<float>();
        if (!std::isfinite(x) || !std::isfinite(y)) {
            return std::nullopt;
        }
        return core::Vector2{x, y};
    }
    return std::nullopt;
}

[[nodiscard]] inline std::optional<float> scenePieceDepthOffset(const nlohmann::json& root,
                                                                std::string_view filename) {
    if (!scenePieceAnchor(root, filename))
        return std::nullopt;
    for (const auto& entry : root["textures"]) {
        if (entry.is_object() && entry.contains("file") && entry["file"].is_string() &&
            entry["file"].get<std::string>() == filename && entry.contains("depthOffset") &&
            entry["depthOffset"].is_number()) {
            const float value = entry["depthOffset"].get<float>();
            if (std::isfinite(value))
                return value;
        }
    }
    return std::nullopt;
}

}  // namespace hmi
