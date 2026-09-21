# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

# Mesures de performance (refonte de la CI, phase 3).
#
# Le déplacement, la ligne de vue et l'IA tactique sont des algorithmes dont le coût peut doubler
# sans qu'aucun test ne rougisse. Le job `benchmarks` de .github/workflows/nightly.yml exécute ces
# mesures en Release chaque nuit et en conserve l'historique ; une régression au-delà du seuil y est
# signalée. Construites uniquement avec -DBUILD_BENCHMARKS=ON, sans Qt.
#
# Rejouer sur le poste (Release, sinon les chiffres ne disent rien) :
#   cmake -S . -B build/bench -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON
#         -DBUILD_TESTING=OFF -DBUILD_EDITOR_QT=OFF
#   cmake --build build/bench --target Benchmarks
#   build/bench/bin/Benchmarks.exe

add_executable(Benchmarks
    bench_combat.cpp
    bench_levels.cpp
    # La composition d'un lieu, que le canevas de l'éditeur refait à chaque geste (LOT-EDITOR-02).
    bench_canvas.cpp
)
target_link_libraries(Benchmarks PRIVATE
    Core SceneComposition benchmark::benchmark_main project_warnings)
target_compile_definitions(Benchmarks PRIVATE
    JADG_RPG_RULES_DIR="${PROJECT_SOURCE_DIR}/Source/Elements/Rpg/rules"
    # Racine de donnees des tests de l'editeur (LOT-123) : les deux mesures de l'editeur chargeaient
    # une carte LIVREE en dur, que la table rase du LOT-102 emporte. Elles mesurent desormais la
    # carte d'essai, qui ne bouge pas -- et une serie se compare d'une version a l'autre.
    JADG_EDITOR_DATA_DIR="${PROJECT_SOURCE_DIR}/Source/Test/Fixtures/EditorData")
