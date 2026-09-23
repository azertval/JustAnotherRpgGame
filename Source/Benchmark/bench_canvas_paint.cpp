// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file bench_canvas_paint.cpp
 * @brief Mesure de la **peinture** du canevas de l'éditeur sur de l'art HD (`LOT-125`).
 *
 * `bench_canvas.cpp` ne mesure que la composition : ce que la HD change, c'est la peinture
 * `QPainter`, qui lisse désormais l'art et lit ses niveaux réduits (constat H7 de l'audit de
 * l'éditeur). L'entrée est la maquette du standard (`Source/Test/Fixtures/HdMockup`, huit cases sur
 * huit à 256 px de losange), peinte à deux échelles : **à 1080p**, une case à 100 px, comme l'essai
 * immédiat et le jeu en plein écran ; et **dézoomée**, une case à 25 px, comme le canevas cadré sur
 * une grande carte — là où servent les niveaux réduits. Le cache est chaud : on mesure la peinture,
 * pas le disque.
 *
 * Une cible séparée des mesures sans Qt (`Benchmarks`) : elle lie Qt Gui, et ne se construit que là
 * où Qt se trouve.
 */

#include <QColor>
#include <QImage>
#include <filesystem>

#include <benchmark/benchmark.h>

#include "Core/Combat/IsoProjection.h"
#include "Editor/Ui/SceneImages.h"
#include "Editor/Ui/ScenePainter.h"
#include "HMI/Graphics/Camera2D.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "Test/Support/HdMockupScene.h"

namespace {

/// Peint la maquette HD en 1920 × 1080, une case à @p tilePixels pixels.
void paintHdMockup(benchmark::State& state, float tilePixels) {
    const std::filesystem::path directory(JADG_HD_MOCKUP_DIR);
    const nlohmann::json scene = test_support::readHdMockupJson(directory / "scene.json");
    if (scene.is_discarded()) {
        state.SkipWithError("maquette HD illisible");
        return;
    }
    const hmi::WorldSceneSnapshot snapshot = test_support::hdMockupSnapshot(scene, directory);
    const core::IsoProjection projection(snapshot.columns, snapshot.rows,
                                         core::ARENA_TILE_WIDTH_UNITS, snapshot.diamondRatio);
    hmi::SceneImages images(directory);
    images.ensure(hmi::worldTexturePaths(snapshot));
    hmi::ComposedScene composed = hmi::composeWorldScene(snapshot, projection, images.textures());
    constexpr int width = 1920;
    constexpr int height = 1080;
    // Le cadrage du jeu (`hmi::worldCamera`, que le rendu GPU porte) : la case à @p tilePixels,
    // centrée sur le point suivi.
    hmi::Camera2D camera(width, height);
    camera.setZoom(tilePixels / (projection.tileWidth() * hmi::Camera2D::PIXELS_PER_UNIT));
    camera.setCenter(projection.gridToWorld(test_support::hdMockupFocus(scene)));
    // Une première image chauffe le cache : les niveaux réduits se calculent une fois.
    benchmark::DoNotOptimize(
        hmi::renderComposedScene(composed, camera, width, height, QColor(24, 26, 30)));
    for (auto _ : state) {
        const QImage image =
            hmi::renderComposedScene(composed, camera, width, height, QColor(24, 26, 30));
        benchmark::DoNotOptimize(image.constBits());
    }
    state.counters["primitives"] = static_cast<double>(composed.size());
    state.counters["Mio"] = static_cast<double>(images.residentBytes()) / (1024.0 * 1024.0);
}

}  // namespace

/// La maquette HD peinte à 1080p, une case à 100 px (`hmi::worldTilePixels`).
static void PaintHdMockup1080p(benchmark::State& state) {
    paintHdMockup(state, hmi::worldTilePixels(1080));
}
BENCHMARK(PaintHdMockup1080p)->Unit(benchmark::kMillisecond);

/// La même, dézoomée : une case à 25 px, l'art lu sur ses niveaux réduits.
static void PaintHdMockupZoomedOut(benchmark::State& state) {
    paintHdMockup(state, 25.0F);
}
BENCHMARK(PaintHdMockupZoomedOut)->Unit(benchmark::kMillisecond);
