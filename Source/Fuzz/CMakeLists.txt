# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

# Cibles libFuzzer des lecteurs de données (refonte de la CI, phase 3).
#
# Chaque exécutable passe à un lecteur des octets arbitraires, des millions de fois, en mutant ceux
# qui ouvrent de la couverture nouvelle. Les 600 JSON du dépôt sont lus par du C++ qui promet de ne
# jamais lever ; le fuzzer cherche le fichier qui fait mentir cette promesse : exception qui
# s'échappe, lecture hors bornes (AddressSanitizer), allocation démesurée. Exécutées la nuit par le
# job `fuzz` de .github/workflows/nightly.yml.
#
# Construites uniquement avec -DBUILD_FUZZERS=ON sous MSVC (garde du CMakeLists racine, qui
# instrumente aussi Core et HmiLib).

# L'exécutable a besoin de la DLL d'ASan de MSVC, qui vit à côté de cl.exe. Le job `sanitize` de
# ci.yml l'obtient par le PATH de vcvars64.bat, dans la même étape ; ici elle est copiée à côté de
# chaque exécutable, qui se lance alors depuis n'importe quel shell.
get_filename_component(_jadg_cl_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
find_file(JADG_ASAN_DLL
    NAMES clang_rt.asan_dynamic-x86_64.dll
    HINTS "${_jadg_cl_dir}" NO_DEFAULT_PATH)
if(NOT JADG_ASAN_DLL)
    message(FATAL_ERROR "clang_rt.asan_dynamic-x86_64.dll introuvable à côté de ${CMAKE_CXX_COMPILER}.")
endif()

# Données d'amorce : le fuzzer part de fichiers réels plutôt que de rien, et atteint ainsi dès les
# premières secondes le code qui suit l'enveloppe JSON. Le job de nuit les lit à côté du corpus
# qu'il conserve d'une nuit à l'autre.
set(JADG_FUZZ_SEEDS_json "${PROJECT_SOURCE_DIR}/Source/Test/Fixtures/Json;${PROJECT_SOURCE_DIR}/Source/Elements/Rpg/rules")
# Les cartes v4 de la racine d'essai, et une carte de chaque version passée : le chargeur lit les deux
# formats (LOT-EDITOR-12), et une amorce v3 seule laisserait la v4 à la chance. Le jeu ne livre plus
# de carte depuis la table rase (LOT-102). libFuzzer parcourt les sous-dossiers de chaque amorce, et
# refuse de démarrer si l'une manque.
set(JADG_FUZZ_SEEDS_level "${PROJECT_SOURCE_DIR}/Source/Test/Fixtures/GameData/Levels;${PROJECT_SOURCE_DIR}/Source/Test/Fixtures/Levels")
set(JADG_FUZZ_SEEDS_dialogue "${PROJECT_SOURCE_DIR}/Source/Elements/World/dialogues")
set(JADG_FUZZ_SEEDS_localization "${PROJECT_SOURCE_DIR}/Source/Elements/Localization")

function(jadg_add_fuzzer name)
    add_executable(fuzz_${name} fuzz_${name}.cpp)
    target_link_libraries(fuzz_${name} PRIVATE ${ARGN} project_warnings)
    # Le harnais fournit LLVMFuzzerTestOneInput ; /fsanitize=fuzzer fait lier libFuzzer, qui fournit
    # main (MSVC nomme lui-même ses bibliothèques dans l'objet).
    target_compile_options(fuzz_${name} PRIVATE /fsanitize=fuzzer)
    add_custom_command(TARGET fuzz_${name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${JADG_ASAN_DLL}" "$<TARGET_FILE_DIR:fuzz_${name}>")
    foreach(_jadg_seed IN LISTS JADG_FUZZ_SEEDS_${name})
        if(NOT IS_DIRECTORY "${_jadg_seed}")
            message(FATAL_ERROR "Amorce de fuzz_${name} introuvable : ${_jadg_seed}")
        endif()
    endforeach()
    string(REPLACE ";" "\n" _jadg_seeds "${JADG_FUZZ_SEEDS_${name}}")
    file(WRITE "${CMAKE_BINARY_DIR}/fuzz/${name}.seeds" "${_jadg_seeds}\n")
endfunction()

jadg_add_fuzzer(json Core)
jadg_add_fuzzer(level Core)
jadg_add_fuzzer(dialogue Core)
jadg_add_fuzzer(localization HmiLib)
