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
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/World/DayLight.h"
#include "Core/World/LightSource.h"
#include "HMI/Graphics/OffscreenRender.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/SceneLighting.h"
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

namespace {

/// Les modèles à mesurer : les huit premiers `.glb` du dossier que désigne la variable
/// d'environnement `JADG_FIGURE_MODELS` (les modèles de l'atelier, jamais suivis par Git), à défaut
/// le mannequin d'essai, huit fois.
std::vector<std::string> figureModels() {
    std::vector<std::string> models;
    char* value = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&value, &size, "JADG_FIGURE_MODELS") == 0 && value != nullptr) {
        std::error_code error;
        for (const auto& entry :
             std::filesystem::recursive_directory_iterator(std::filesystem::path(value), error)) {
            if (entry.is_regular_file(error) && entry.path().extension() == ".glb") {
                models.push_back(entry.path().generic_string());
            }
        }
        std::free(value);  // NOLINT(cppcoreguidelines-no-malloc) : allouee par _dupenv_s
        std::ranges::sort(models);
    }
    const std::string fixture =
        (std::filesystem::path(JADG_CHARACTER_FIXTURE_DIR) / "Assets" / "Common" / "Characters" /
         "Mannequins" / "humanoid" / "humanoid.glb")
            .generic_string();
    for (std::size_t rank = models.size(); rank < 8; ++rank) {
        models.push_back(models.empty() ? fixture : models[rank % models.size()]);
    }
    models.resize(8);
    return models;
}

/// Ce qu'une image mesurée ajoute à la scène : rien, ou la lumière du `LOT-1007`.
enum class FrameLight {
    /// Sans éclairage : le rendu d'avant le lot.
    None,
    /// Le crépuscule, sans ombres : la teinte, le soleil sur les modèles, huit lumières de nuit.
    Lamps,
    /// Le même, avec la carte d'ombres de 2048 texels.
    LampsAndShadows,
};

/// Une image du jeu à 1080p sur Arenarea, cadrée sur l'avenue, avec @p modelCount modèles animés
/// (0 : la figurine en bandes seule, la référence), sous la lumière @p light.
void worldFrame(benchmark::State& state, std::size_t modelCount,
                FrameLight light = FrameLight::None) {
    const std::filesystem::path elements(JADG_ELEMENTS_DIR);
    const core::LevelLoadResult map = core::LevelLoader::loadFromFile(elements / "Levels" / MAP);
    const hmi::PlaceAppearanceResult appearance =
        hmi::PlaceAppearance::loadForPlace(elements / "Assets", PLACE);
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!map.ok() || !appearance.ok() || !offscreen) {
        state.SkipWithError("carte d'Arenarea illisible, ou aucune interface QRhi");
        return;
    }
    const std::vector<std::string> models = figureModels();
    std::vector<hmi::WorldFigureSnapshot> figures;
    for (std::size_t rank = 0; rank < std::max<std::size_t>(modelCount, 1); ++rank) {
        hmi::WorldFigureSnapshot figure{.figure = "Common/Characters/Heroes/brawler",
                                        .clip = "walk",
                                        .point = {7.5F + (1.5F * static_cast<float>(rank % 4)),
                                                  4.5F + (2.0F * static_cast<float>(rank / 4))},
                                        .seconds = 0.0F,
                                        .hero = rank == 0};
        if (modelCount > 0) {
            // Un chemin absolu : le rendu le lit tel quel, hors du dossier des assets.
            figure.model = models[rank];
            figure.heading = 0.4F * static_cast<float>(rank);
        }
        figures.push_back(std::move(figure));
    }
    hmi::WorldSceneSnapshot snapshot =
        hmi::snapshotWorldScene(*map.level, appearance.appearance, figures);
    const core::IsoProjection projection(snapshot.columns, snapshot.rows,
                                         core::ARENA_TILE_WIDTH_UNITS, snapshot.diamondRatio);
    hmi::WorldSceneRenderer renderer(elements / "Assets");
    if (!renderer.ensureResources(offscreen->rhi())) {
        state.SkipWithError("les ressources du rendu ne se creent pas");
        return;
    }
    // Huit lumières de nuit autour du groupe (`LOT-1007`) : le critère du lot, toutes à l'écran.
    if (light != FrameLight::None) {
        for (int rank = 0; rank < 8; ++rank) {
            snapshot.lights.push_back(
                core::LightSource{.column = 5.5F + (2.0F * static_cast<float>(rank % 4)),
                                  .row = 3.5F + (4.0F * static_cast<float>(rank / 4)),
                                  .emission = {.color = core::LightEmission::DEFAULT_COLOR,
                                               .radius = 7.5F,
                                               .height = 2.9F,
                                               .intensity = 1.0F,
                                               .flicker = rank % 2 == 0,
                                               .always = true}});
        }
        renderer.setLighting(
            hmi::WorldLighting{.light = core::DayLightTable::factory().sample(19.0F * 60.0F),
                               .shadows = light == FrameLight::LampsAndShadows,
                               .shadowSize = 2048,
                               .seconds = 0.0F});
    }
    renderer.setSnapshot(std::move(snapshot));
    const QSize size(1920, 1080);
    const QColor background(24, 26, 30);
    // Le milieu de la place : la carte fait 24 × 13 cases depuis sa refonte.
    const hmi::WorldFraming framing{
        .center = projection.gridToWorld({9.5F, 5.5F}),
        .pixelsPerUnit = hmi::worldTilePixels(1080) / projection.tileWidth()};
    // Une première image chauffe : modèles lus, textures téléversées, pipelines créés.
    benchmark::DoNotOptimize(offscreen->render(renderer, size, framing, background));
    float seconds = 0.0F;
    for (auto _ : state) {
        seconds += 1.0F / 60.0F;
        for (std::size_t rank = 0; rank < figures.size(); ++rank) {
            figures[rank].seconds = seconds + (0.07F * static_cast<float>(rank));
        }
        renderer.setFigures(figures);
        const QImage image = offscreen->render(renderer, size, framing, background);
        benchmark::DoNotOptimize(image.constBits());
    }
    state.counters["primitives"] = static_cast<double>(renderer.composed().size());
    state.counters["modeles"] = static_cast<double>(renderer.textures().figures.size());
    state.counters["maillages"] = static_cast<double>(renderer.composed().meshes().size());
    state.counters["Mio"] = static_cast<double>(renderer.textureBytes()) / (1024.0 * 1024.0);
    renderer.release();
}

}  // namespace

/// L'image du jeu à 1080p, le héros en bandes : la référence d'avant le `LOT-1005`.
static void WorldFrameStrips1080p(benchmark::State& state) {
    worldFrame(state, 0);
}
BENCHMARK(WorldFrameStrips1080p)->Unit(benchmark::kMillisecond);

/// La même image avec un, puis huit modèles animés à l'écran (`LOT-1005`) : la pose de leurs os, et
/// leur dessin par la carte graphique. `JADG_FIGURE_MODELS` désigne les modèles de l'atelier pour
/// mesurer le budget d'un modèle livré ; sans elle, le mannequin d'essai.
static void WorldFrameOneModel1080p(benchmark::State& state) {
    worldFrame(state, 1);
}
BENCHMARK(WorldFrameOneModel1080p)->Unit(benchmark::kMillisecond);

static void WorldFrameEightModels1080p(benchmark::State& state) {
    worldFrame(state, 8);
}
BENCHMARK(WorldFrameEightModels1080p)->Unit(benchmark::kMillisecond);

/// La même image à huit modèles, **éclairée** (`LOT-1007`) : le crépuscule et huit lumières de
/// nuit à l'écran, sans puis avec la carte d'ombres — la passe d'ombres redessine les huit modèles
/// et les boîtes du décor, vus du soleil.
static void WorldFrameLitEightModels1080p(benchmark::State& state) {
    worldFrame(state, 8, FrameLight::Lamps);
}
BENCHMARK(WorldFrameLitEightModels1080p)->Unit(benchmark::kMillisecond);

static void WorldFrameShadowedEightModels1080p(benchmark::State& state) {
    worldFrame(state, 8, FrameLight::LampsAndShadows);
}
BENCHMARK(WorldFrameShadowedEightModels1080p)->Unit(benchmark::kMillisecond);
