// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file bench_levels.cpp
 * @brief Mesure du chargement d'un niveau, du texte JSON au niveau validé.
 *
 * La carte d'essai `donjon`, lue de la racine de données des tests de l'éditeur
 * (`Source/Test/Fixtures/EditorData`). Jusqu'au `LOT-123` c'était une carte **livrée** : la table
 * rase du `LOT-102` l'emporte, et une série ne se compare d'une version à l'autre que si son entrée
 * ne bouge pas. Le fichier est lu une fois hors de la boucle : c'est l'analyse et la validation
 * qu'on mesure, pas le disque du runner.
 */

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include <benchmark/benchmark.h>

#include "Core/Levels/LevelLoader.h"

/// Analyse et validation de la carte d'essai `donjon` (48 × 40 cases).
static void LoadTestLevel(benchmark::State& state) {
    std::ifstream fichier(std::filesystem::path(JADG_EDITOR_DATA_DIR) / "Levels" / "donjon.json",
                          std::ios::binary);
    const std::string texte{std::istreambuf_iterator<char>(fichier),
                            std::istreambuf_iterator<char>()};
    if (texte.empty()) {
        state.SkipWithError("donjon.json illisible");
        return;
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(core::LevelLoader::loadFromString(texte));
    }
    state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(texte.size()));
}
BENCHMARK(LoadTestLevel);
