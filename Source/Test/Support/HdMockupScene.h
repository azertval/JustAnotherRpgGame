// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Test/Support/HdMockupScene.h
 * @brief La maquette du standard 2D HD (`Fixtures/HdMockup`, `LOT-101`) lue en instantané de scène.
 *
 * Deux tests la rendent : le rendu GPU du jeu contre les images de la maquette
 * (`test_hd_mockup_render.cpp`, `LOT-103`), et le peintre de l'éditeur contre le rendu GPU
 * (`test_scene_painter.cpp`, `LOT-125`) ; le banc de peinture du canevas la peint aussi
 * (`bench_canvas_paint.cpp`). La lecture de `scene.json` est ici, une fois — sans GoogleTest, que
 * le banc ne lie pas.
 */

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

#include <nlohmann/json.hpp>

#include "Core/Levels/TileType.h"
#include "Core/Math/Vector2.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace test_support {

/// @return Le JSON de @p path, « discarded » s'il ne se lit pas.
inline nlohmann::json readHdMockupJson(const std::filesystem::path& path) {
    std::ifstream stream(path);
    return nlohmann::json::parse(stream, nullptr, false);
}

/**
 * @brief La scène de `scene.json`, en instantané : les sols par la légende, les pièces à leur
 *        case.
 * @param scene     Le contenu de `scene.json`.
 * @param directory Le dossier de la maquette (celui de `scene.json` et de `Scene/`).
 */
inline hmi::WorldSceneSnapshot hdMockupSnapshot(const nlohmann::json& scene,
                                                const std::filesystem::path& directory) {
    hmi::WorldSceneSnapshot snapshot;
    snapshot.place = scene["place"].get<std::string>();
    snapshot.diamondRatio = scene["diamondRatio"].get<float>();
    snapshot.columns = scene["columns"].get<int>();
    snapshot.rows = scene["rows"].get<int>();
    const auto cells =
        static_cast<std::size_t>(snapshot.columns) * static_cast<std::size_t>(snapshot.rows);
    snapshot.floors.assign(cells, std::string{});
    snapshot.relief.assign(cells, std::string{});
    snapshot.types.assign(cells, core::TileType::Solid);
    snapshot.reliefTypes.assign(cells, core::TileType::Empty);

    const nlohmann::json& legend = scene["legend"];
    const nlohmann::json& floors = scene["floors"];
    for (int row = 0; row < snapshot.rows; ++row) {
        const std::string line = floors[static_cast<std::size_t>(row)].get<std::string>();
        for (int column = 0; column < snapshot.columns; ++column) {
            const std::string symbol(1, line[static_cast<std::size_t>(column)]);
            snapshot.floors[(static_cast<std::size_t>(row) * snapshot.columns) + column] =
                legend[symbol].get<std::string>();
        }
    }

    const core::ScenePieceManifestResult manifest = core::ScenePieceManifest::loadFromFile(
        directory / "Scene" / snapshot.place / "manifest.json");
    // Un manifeste illisible laisse les pièces sans emprise : la comparaison d'images le verra.
    for (const nlohmann::json& piece : scene["pieces"]) {
        const std::string name = piece["piece"].get<std::string>();
        const int column = piece["column"].get<int>();
        const int row = piece["row"].get<int>();
        snapshot.relief[(static_cast<std::size_t>(row) * snapshot.columns) + column] = name;
        if (const core::ScenePiece* const declared =
                manifest.ok() ? manifest.manifest.find(name) : nullptr) {
            snapshot.footprints.insert_or_assign(name, declared->footprint());
        }
    }
    return snapshot;
}

/// @return Le point suivi de la scène, en cases.
inline core::Vector2 hdMockupFocus(const nlohmann::json& scene) {
    return {scene["focus"][0].get<float>(), scene["focus"][1].get<float>()};
}

}  // namespace test_support
