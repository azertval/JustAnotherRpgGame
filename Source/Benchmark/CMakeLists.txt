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
    # Une image d'un grand lieu, le quartier d'Arenarea (LOT-109) : ce qu'il faut tenir en 16,7 ms.
    bench_world_frame.cpp
)
target_link_libraries(Benchmarks PRIVATE
    Core SceneComposition benchmark::benchmark_main project_warnings)
target_compile_definitions(Benchmarks PRIVATE
    JADG_RPG_RULES_DIR="${PROJECT_SOURCE_DIR}/Source/Elements/Rpg/rules"
    # Racine de donnees des tests de l'editeur (LOT-123) : les deux mesures de l'editeur chargeaient
    # une carte LIVREE en dur, que la table rase du LOT-102 emporte. Elles mesurent desormais la
    # carte d'essai, qui ne bouge pas -- et une serie se compare d'une version a l'autre.
    JADG_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/Source/Test/Fixtures/GameData"
    # Les donnees livrees : la carte d'Arenarea et les manifestes de ses pieces, suivis par Git.
    JADG_ELEMENTS_DIR="${PROJECT_SOURCE_DIR}/Source/Elements"
    # Le modèle d'essai animé (LOT-1005) : le mannequin humanoïde réduit, lié aux 53 os.
    JADG_CHARACTER_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/Source/Test/Fixtures/Characters")

# L'image du canevas de l'éditeur, par le rendu du jeu (LOT-1002) : un travelling sur la carte
# d'Arenarea, hors écran. Elle lie Qt Gui et QRhi, que les mesures ci-dessus évitent, et lit les
# images du kit : construite seulement là où Qt se trouve ; le job `benchmarks` de la nuit installe
# Qt pour elle et publie sa série à part.
#   cmake --build build/bench --target CanvasBenchmarks
#   build/bench/bin/CanvasBenchmarks.exe
find_package(Qt6 QUIET COMPONENTS Gui GuiPrivate ShaderTools)
if(TARGET Qt6::Gui AND TARGET Qt6::GuiPrivate AND COMMAND qt6_add_shaders)
    set(_hmi_graphics ${PROJECT_SOURCE_DIR}/Source/HMI/Graphics)
    add_executable(CanvasBenchmarks
        bench_canvas_frame.cpp
        ${_hmi_graphics}/WorldSceneRenderer.cpp
        ${_hmi_graphics}/OffscreenRender.cpp
        ${_hmi_graphics}/SceneResources.cpp
        ${_hmi_graphics}/MeshBatch.cpp
        ${_hmi_graphics}/SpriteBatch.cpp
        ${_hmi_graphics}/SpriteRenderer.cpp
        ${_hmi_graphics}/TextureLoader.cpp
        ${_hmi_graphics}/TextureAtlas.cpp
    )
    target_link_libraries(CanvasBenchmarks PRIVATE
        HmiLib Qt6::Gui Qt6::GuiPrivate benchmark::benchmark_main project_warnings)
    target_compile_definitions(CanvasBenchmarks PRIVATE
        JADG_ELEMENTS_DIR="${PROJECT_SOURCE_DIR}/Source/Elements"
        JADG_CHARACTER_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/Source/Test/Fixtures/Characters")
    qt6_add_shaders(CanvasBenchmarks "canvas_benchmarks_shaders"
        PREFIX "/shaders"
        BASE "${_hmi_graphics}/Shaders"
        FILES
            ${_hmi_graphics}/Shaders/sprite.vert
            ${_hmi_graphics}/Shaders/sprite.frag
            ${_hmi_graphics}/Shaders/mesh.vert
            ${_hmi_graphics}/Shaders/mesh_skinned.vert
            ${_hmi_graphics}/Shaders/mesh.frag
    )
else()
    message(STATUS "CanvasBenchmarks : Qt6 (Gui, GuiPrivate, ShaderTools) introuvable -- "
        "mesure de l'image du canevas ignoree.")
endif()
