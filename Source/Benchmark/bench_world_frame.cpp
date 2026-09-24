// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file bench_world_frame.cpp
 * @brief Mesure d'une image d'un **grand lieu** : le quartier d'Arenarea (`LOT-109`, 128 × 88
 * cases, environ quatorze mille pièces sur sept couches).
 *
 * Le critère du lot est « 60 images par seconde à 1080p » : une image n'a donc que 16,7 ms, GPU
 * compris. Ces mesures isolent la part du processeur — l'instantané, la composition et le tri — sur
 * la carte **livrée**, lue de `Source/Elements`. Les textures sont des identités sans image, à la
 * taille que déclare le manifeste : ni disque ni GPU. Seuls les manifestes et la carte sont lus,
 * qui sont suivis par Git : la mesure tourne sans les kits d'images.
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
#include "HMI/Graphics/StaticWorldScene.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

/// Le lieu et la carte mesurés.
constexpr const char* PLACE = "central-empire/capital/arenarea";
constexpr const char* MAP = "central-empire/capital/arenarea.json";

/// Ce que les mesures partagent : la carte lue, sa table, des textures sans image.
struct Arenarea {
    core::LevelLoadResult map;
    hmi::PlaceAppearanceResult appearance;
    core::ScenePieceManifestResult manifest;
    std::vector<std::uint8_t> identities;
    hmi::ScenePieceTextures textures;

    Arenarea()
        : map(core::LevelLoader::loadFromFile(std::filesystem::path(JADG_ELEMENTS_DIR) / "Levels" /
                                              MAP)),
          appearance(hmi::PlaceAppearance::loadForPlace(
              std::filesystem::path(JADG_ELEMENTS_DIR) / "Assets", PLACE)),
          manifest(core::ScenePieceManifest::resolve(
              std::filesystem::path(JADG_ELEMENTS_DIR) / "Assets", PLACE)) {
        if (!ok()) {
            return;
        }
        identities.resize(manifest.manifest.pieces().size());
        for (std::size_t index = 0; index < identities.size(); ++index) {
            const core::ScenePiece& piece = manifest.manifest.pieces()[index];
            textures.byPath[piece.path()] = hmi::SceneTexture{
                .texture = &identities[index], .width = piece.width, .height = piece.height};
        }
    }

    [[nodiscard]] bool ok() const {
        return map.ok() && appearance.ok() && manifest.ok();
    }

    [[nodiscard]] hmi::WorldSceneSnapshot snapshot() const {
        return hmi::snapshotWorldScene(*map.level, appearance.appearance,
                                       hmi::npcFigures(map.level->entities(), 0));
    }
};

const Arenarea& arenarea() {
    static const Arenarea loaded;
    return loaded;
}

}  // namespace

/// L'instantané de la carte : ce que le jeu refaisait à chaque pas du héros.
static void ArenareaSnapshot(benchmark::State& state) {
    const Arenarea& data = arenarea();
    if (!data.ok()) {
        state.SkipWithError("carte d'Arenarea illisible");
        return;
    }
    for (auto _ : state) {
        const hmi::WorldSceneSnapshot snapshot = data.snapshot();
        benchmark::DoNotOptimize(snapshot.floors.data());
    }
}
BENCHMARK(ArenareaSnapshot)->Unit(benchmark::kMillisecond);

/// La composition triée de TOUTE la carte : ce que le jeu refaisait à chaque image.
static void ArenareaComposeWholeMap(benchmark::State& state) {
    const Arenarea& data = arenarea();
    if (!data.ok()) {
        state.SkipWithError("carte d'Arenarea illisible");
        return;
    }
    const hmi::WorldSceneSnapshot snapshot = data.snapshot();
    const core::IsoProjection projection(snapshot.columns, snapshot.rows,
                                         core::ARENA_TILE_WIDTH_UNITS, snapshot.diamondRatio);
    hmi::ComposedScene composed;
    for (auto _ : state) {
        composed.clear();
        hmi::composeWorldScene(composed, snapshot, projection, data.textures);
        composed.sort();
        benchmark::DoNotOptimize(composed.size());
    }
    state.counters["primitives"] = static_cast<double>(composed.size());
    state.counters["passes"] = static_cast<double>(composed.batchCount());
}
BENCHMARK(ArenareaComposeWholeMap)->Unit(benchmark::kMillisecond);

/// Les chemins de texture de la carte : ce que le rendu recalculait à chaque image.
static void ArenareaTexturePaths(benchmark::State& state) {
    const Arenarea& data = arenarea();
    if (!data.ok()) {
        state.SkipWithError("carte d'Arenarea illisible");
        return;
    }
    const hmi::WorldSceneSnapshot snapshot = data.snapshot();
    for (auto _ : state) {
        const std::vector<std::string> paths = hmi::worldTexturePaths(snapshot);
        benchmark::DoNotOptimize(paths.data());
    }
}
BENCHMARK(ArenareaTexturePaths)->Unit(benchmark::kMillisecond);

/// La partie fixe composée et indexée une fois : ce que coûte l'entrée dans la carte.
static void ArenareaBuildStaticScene(benchmark::State& state) {
    const Arenarea& data = arenarea();
    if (!data.ok()) {
        state.SkipWithError("carte d'Arenarea illisible");
        return;
    }
    const hmi::WorldSceneSnapshot snapshot = data.snapshot();
    const core::IsoProjection projection(snapshot.columns, snapshot.rows,
                                         core::ARENA_TILE_WIDTH_UNITS, snapshot.diamondRatio);
    hmi::StaticWorldScene statics;
    for (auto _ : state) {
        statics.build(snapshot, projection, data.textures);
        benchmark::DoNotOptimize(statics.size());
    }
    state.counters["primitives"] = static_cast<double>(statics.size());
}
BENCHMARK(ArenareaBuildStaticScene)->Unit(benchmark::kMillisecond);

/// Une image à 1080p, le héros au milieu de l'avenue : la partie fixe sous la caméra et le héros.
static void ArenareaFrame1080p(benchmark::State& state) {
    const Arenarea& data = arenarea();
    if (!data.ok()) {
        state.SkipWithError("carte d'Arenarea illisible");
        return;
    }
    const hmi::WorldSceneSnapshot snapshot = data.snapshot();
    const core::IsoProjection projection(snapshot.columns, snapshot.rows,
                                         core::ARENA_TILE_WIDTH_UNITS, snapshot.diamondRatio);
    hmi::StaticWorldScene statics;
    statics.build(snapshot, projection, data.textures);
    // Le cadrage du jeu : une case occupe la hauteur de la vue divisée par 10,8 (EX-REN-013), soit
    // 19,2 cases de large en 16:9.
    const float tile = projection.tileWidth();
    const core::Vector2 view{19.2F * tile, 10.8F * tile};
    const std::vector<hmi::WorldFigureSnapshot> hero{hmi::WorldFigureSnapshot{
        .figure = "Common/Characters/Heroes/brawler", .point = {60.5F, 42.5F}, .hero = true}};
    const core::Vector2 centre = projection.gridToWorld(hero.front().point);
    hmi::ComposedScene composed;
    for (auto _ : state) {
        composed.clear();
        composed.setVisibleBounds(
            core::Rect{{centre.x - (view.x / 2.0F), centre.y - (view.y / 2.0F)}, view});
        statics.compose(composed, hero, data.textures);
        benchmark::DoNotOptimize(composed.size());
    }
    state.counters["primitives"] = static_cast<double>(composed.size());
    state.counters["passes"] = static_cast<double>(composed.batchCount());
}
BENCHMARK(ArenareaFrame1080p)->Unit(benchmark::kMicrosecond);
