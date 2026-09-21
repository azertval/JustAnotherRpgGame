// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file bench_canvas.cpp
 * @brief Mesure de la composition d'un lieu : ce que le canevas de l'éditeur refait à chaque geste
 *        et le jeu à chaque image (`LOT-EDITOR-02`, feuille de route de l'éditeur, §5 règle 5).
 *
 * La carte d'essai `bourg/place` (48 × 40 cases, quelques centaines de pièces de relief), lue de la
 * racine de données des tests de l'éditeur (`Source/Test/Fixtures/EditorData`). Jusqu'au `LOT-123`
 * c'était une carte **livrée**, chargée en dur : la table rase du `LOT-102` l'emporte, et la mesure
 * serait partie avec elle — une série ne se compare d'une version à l'autre que si son entrée ne
 * bouge pas. Les textures sont des identités sans image, à la taille que déclare le manifeste : on
 * mesure l'instantané et la composition triée, pas le disque ni le GPU.
 */

#include <cstdint>
#include <filesystem>
#include <vector>

#include <benchmark/benchmark.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

/// Le lieu de la carte d'essai, et le dossier de sa planche.
constexpr const char* PLACE = "bourg";

}  // namespace

/// Instantané puis composition triée de la carte d'essai, comme le canevas après un coup de
/// pinceau.
static void ComposeTestMap(benchmark::State& state) {
    const std::filesystem::path dataRoot(JADG_EDITOR_DATA_DIR);
    const std::filesystem::path scene = dataRoot / "Assets" / "Scene" / PLACE;
    const core::LevelLoadResult map =
        core::LevelLoader::loadFromFile(dataRoot / "Levels" / PLACE / "place.json");
    const hmi::PlaceAppearanceResult appearance =
        hmi::PlaceAppearance::loadFromFile(scene / "appearance.json");
    const core::ScenePieceManifestResult manifest =
        core::ScenePieceManifest::loadFromFile(scene / "manifest.json");
    if (!map.ok() || !appearance.ok() || !manifest.ok()) {
        state.SkipWithError("carte d'essai illisible");
        return;
    }
    hmi::ScenePieceTextures textures;
    std::vector<std::uint8_t> identities(manifest.manifest.pieces().size());
    for (std::size_t index = 0; index < identities.size(); ++index) {
        const core::ScenePiece& piece = manifest.manifest.pieces()[index];
        textures.byPath[std::string{"Scene/"} + PLACE + "/" + piece.file] = hmi::SceneTexture{
            .texture = &identities[index], .width = piece.width, .height = piece.height};
    }
    hmi::ComposedScene composed;
    for (auto _ : state) {
        const hmi::WorldSceneSnapshot snapshot = hmi::snapshotWorldScene(
            *map.level, appearance.appearance, hmi::npcFigures(map.level->entities(), 0));
        composed.clear();
        hmi::composeWorldScene(composed, snapshot,
                               core::IsoProjection(snapshot.columns, snapshot.rows), textures);
        composed.sort();
        benchmark::DoNotOptimize(composed.size());
    }
    state.counters["primitives"] = static_cast<double>(composed.size());
}
BENCHMARK(ComposeTestMap)->Unit(benchmark::kMicrosecond);
