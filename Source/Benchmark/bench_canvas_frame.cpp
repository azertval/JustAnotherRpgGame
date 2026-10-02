// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file bench_canvas_frame.cpp
 * @brief Mesure de **l'image du canevas** de l'éditeur, par le rendu du jeu (`LOT-1002`).
 *
 * Le canevas ne peint plus la scène : il la fait dessiner par `hmi::WorldSceneRenderer`. La mesure
 * est donc celle d'une image de ce rendu, sur la carte d'Arenarea **livrée** et les images de son
 * kit, en 1920 × 1080, le long d'un **travelling** qui traverse la carte en diagonale — à deux
 * échelles : une case à 100 px, comme l'essai immédiat et le jeu à 1080p ; et **dézoomée**, une
 * case à 25 px, comme le canevas cadré sur une grande carte.
 *
 * Chaque image est dessinée hors écran puis **relue** (`hmi::OffscreenRhi::render`) : la relecture
 * attend que la carte graphique ait fini, si bien que le temps mesuré est celui de l'image entière,
 * processeur et carte graphique — et majore celui du canevas, qui ne relit rien. Les textures sont
 * chargées avant la mesure : on mesure l'image, pas le disque.
 *
 * Avant le lot, le même travelling peint par `QPainter` (`hmi::paintComposedScene`, retiré) coûtait
 * 7,3 ms par image à 100 px la case et 2,6 ms à 25 px, sur le poste de l'auteur le 2 octobre 2026.
 * Le même jour, par le rendu du jeu : 5,8 ms et 5,7 ms **relecture comprise**, pour un plancher
 * (`CanvasReadbackFloor`, effacer et relire une image vide) de 6,2 ms — la scène elle-même coûte
 * donc moins que le bruit de la mesure, quelques dixièmes de milliseconde.
 *
 * Une cible séparée des mesures sans Qt (`Benchmarks`) : elle lie Qt Gui et QRhi, et ne se
 * construit que là où Qt se trouve.
 */

#include <QColor>
#include <QImage>
#include <QSize>
#include <filesystem>
#include <memory>

#include <benchmark/benchmark.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/LevelLoader.h"
#include "HMI/Graphics/OffscreenRender.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/Graphics/WorldSceneRenderer.h"

namespace {

/// Le lieu et la carte mesurés.
constexpr const char* PLACE = "central-empire/capital/arenarea";
constexpr const char* MAP = "central-empire/capital/arenarea.json";

/// Le nombre d'arrêts du travelling, d'un coin de la carte à l'autre.
constexpr int TRAVEL_STEPS = 64;

/// Dessine la carte d'Arenarea en 1920 × 1080, une case à @p tilePixels pixels, la vue glissant le
/// long de la diagonale de la carte.
void travelArenarea(benchmark::State& state, float tilePixels) {
    const std::filesystem::path elements(JADG_ELEMENTS_DIR);
    const core::LevelLoadResult map = core::LevelLoader::loadFromFile(elements / "Levels" / MAP);
    const hmi::PlaceAppearanceResult appearance =
        hmi::PlaceAppearance::loadForPlace(elements / "Assets", PLACE);
    if (!map.ok() || !appearance.ok()) {
        state.SkipWithError("carte d'Arenarea illisible");
        return;
    }
    // L'interface d'abord, le rendu ensuite : ses ressources meurent avant elle.
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        state.SkipWithError("aucune interface QRhi sur cette machine");
        return;
    }
    hmi::WorldSceneSnapshot snapshot = hmi::snapshotWorldScene(
        *map.level, appearance.appearance, hmi::npcFigures(map.level->entities(), 0));
    const core::IsoProjection projection(snapshot.columns, snapshot.rows,
                                         core::ARENA_TILE_WIDTH_UNITS, snapshot.diamondRatio);
    const auto columns = static_cast<float>(snapshot.columns);
    const auto rows = static_cast<float>(snapshot.rows);

    hmi::WorldSceneRenderer renderer(elements / "Assets");
    if (!renderer.ensureResources(offscreen->rhi())) {
        state.SkipWithError("les ressources du rendu ne se creent pas");
        return;
    }
    renderer.setSnapshot(std::move(snapshot));

    const QSize size(1920, 1080);
    const QColor background(24, 26, 30);
    const auto framingAt = [&](int step) {
        const float t =
            static_cast<float>(step % TRAVEL_STEPS) / static_cast<float>(TRAVEL_STEPS - 1);
        return hmi::WorldFraming{.center = projection.gridToWorld(
                                     {columns * (0.1F + (0.8F * t)), rows * (0.1F + (0.8F * t))}),
                                 .pixelsPerUnit = tilePixels / projection.tileWidth()};
    };
    // Un premier passage chauffe : textures téléversées, mipmaps engendrées, pipeline créé.
    for (int step = 0; step < TRAVEL_STEPS; ++step) {
        benchmark::DoNotOptimize(offscreen->render(renderer, size, framingAt(step), background));
    }
    int step = 0;
    std::size_t primitives = 0;
    for (auto _ : state) {
        const QImage image = offscreen->render(renderer, size, framingAt(step++), background);
        benchmark::DoNotOptimize(image.constBits());
        primitives = renderer.composed().size();
    }
    state.counters["primitives"] = static_cast<double>(primitives);
    state.counters["carte"] = static_cast<double>(renderer.statics().size());
    renderer.release();
}

}  // namespace

/// Le **plancher** de la mesure : une carte vide, donc le seul effacement et la relecture de
/// l'image. Ce que le canevas, qui ne relit rien, ne paie pas — à retrancher des deux autres.
static void CanvasReadbackFloor(benchmark::State& state) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        state.SkipWithError("aucune interface QRhi sur cette machine");
        return;
    }
    hmi::WorldSceneRenderer renderer(std::filesystem::path(JADG_ELEMENTS_DIR) / "Assets");
    const QSize size(1920, 1080);
    const hmi::WorldFraming framing{.center = {0.0F, 0.0F}, .pixelsPerUnit = 16.0F};
    benchmark::DoNotOptimize(offscreen->render(renderer, size, framing, QColor(24, 26, 30)));
    for (auto _ : state) {
        const QImage image = offscreen->render(renderer, size, framing, QColor(24, 26, 30));
        benchmark::DoNotOptimize(image.constBits());
    }
    renderer.release();
}
BENCHMARK(CanvasReadbackFloor)->Unit(benchmark::kMillisecond);

/// Le travelling à 1080p, une case à 100 px (`hmi::worldTilePixels`).
static void CanvasTravelArenarea1080p(benchmark::State& state) {
    travelArenarea(state, hmi::worldTilePixels(1080));
}
BENCHMARK(CanvasTravelArenarea1080p)->Unit(benchmark::kMillisecond);

/// Le même, dézoomé : une case à 25 px, l'art lu sur ses mipmaps.
static void CanvasTravelArenareaZoomedOut(benchmark::State& state) {
    travelArenarea(state, 25.0F);
}
BENCHMARK(CanvasTravelArenareaZoomedOut)->Unit(benchmark::kMillisecond);
