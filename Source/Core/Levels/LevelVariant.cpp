// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Levels/LevelVariant.h"

#include <string>
#include <system_error>
#include <utility>

namespace core {

Level applyVariant(const Level& base, LevelData variant) {
    LevelData played = base.data();
    played.name = std::move(variant.name);
    played.entities = std::move(variant.entities);
    played.nextEntityId = variant.nextEntityId;
    if (!variant.scene.empty()) {
        // La planche se nomme sur les couches qui la déclarent : ce sont elles qui en changent.
        for (TileLayer& layer : played.layers) {
            const auto found = layer.properties.find(std::string{SCENE_LAYER_PROPERTY});
            if (found != layer.properties.end()) {
                found->second = variant.scene;
            }
        }
    }
    played.base = std::move(variant.base);
    played.scene = std::move(variant.scene);
    return Level(std::move(played));
}

std::optional<std::filesystem::path> findVariantBase(const std::filesystem::path& variantPath,
                                                     std::string_view baseId) {
    if (baseId.empty()) {
        return std::nullopt;
    }
    const std::filesystem::path relative = std::filesystem::path{std::string{baseId} + ".json"};
    std::error_code error;
    std::filesystem::path directory = std::filesystem::absolute(variantPath, error).parent_path();
    while (!directory.empty()) {
        std::filesystem::path candidate = directory / relative;
        if (std::filesystem::is_regular_file(candidate, error) &&
            !std::filesystem::equivalent(candidate, variantPath, error)) {
            return candidate;
        }
        const std::filesystem::path parent = directory.parent_path();
        if (parent == directory) {
            break;
        }
        directory = parent;
    }
    return std::nullopt;
}

}  // namespace core
