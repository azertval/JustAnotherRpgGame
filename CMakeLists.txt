# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

cmake_minimum_required(VERSION 3.21)

# Format des informations de débogage MSVC piloté par CMAKE_MSVC_DEBUG_INFORMATION_FORMAT au lieu
# d'un /Zi écrit dans CMAKE_CXX_FLAGS_DEBUG (CMP0141, CMake 3.25). Sans effet par défaut : Debug
# reste en /Zi. Nécessaire au cache de compilation ci-dessous, et doit précéder project(), qui
# initialise les drapeaux.
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
endif()

# VERSION est la **source unique de vérité** du numéro de version : `core::Engine::version()` le
# reçoit par définition de compilation (Source/Core/CMakeLists.txt), et `scripts/docs/build_docs.py`
# l'injecte dans le `PROJECT_NUMBER` de Doxygen à la génération (le Doxyfile versionné ne le porte
# pas). Bumper ici, et nulle part ailleurs, avant de poser le tag `vX.Y.Z`.
project(JustAnotherRpgGame
    VERSION 0.0.1
    DESCRIPTION "RPG 2D en vue de dessus, exploration temps reel et combat tactique d20, en C++/Qt QRhi"
    LANGUAGES CXX)

# --- Standard C++ ---
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Kits d'assets hors de Git (LOT-108) ---
# Les images de Source/Elements/Assets/{Common,Regions,Maps,UI} ne sont pas suivies : elles viennent
# d'archives publiées, installées par scripts/fetch_assets.py, qui laisse un témoin par kit dans
# Source/Elements/Assets/.kits/. Sans eux, Source/Ui globerait un HUD vide et le jeu se construirait
# sans image, sans erreur visible : on refuse ici, avec la commande à lancer, plutôt que de
# télécharger en silence à la configuration.
option(SKIP_ASSET_KITS_CHECK "Désactiver la vérification des kits d'assets installés" OFF)
set(_jadg_assets "${PROJECT_SOURCE_DIR}/Source/Elements/Assets")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_jadg_assets}/kits.lock.json")
if(NOT SKIP_ASSET_KITS_CHECK AND EXISTS "${_jadg_assets}/kits.lock.json")
    file(READ "${_jadg_assets}/kits.lock.json" _jadg_lock)
    string(JSON _jadg_kit_count LENGTH "${_jadg_lock}" kits)
    set(_jadg_missing "")
    if(_jadg_kit_count GREATER 0)
        math(EXPR _jadg_last "${_jadg_kit_count} - 1")
        foreach(_i RANGE ${_jadg_last})
            string(JSON _jadg_path GET "${_jadg_lock}" kits ${_i} path)
            string(JSON _jadg_sha GET "${_jadg_lock}" kits ${_i} sha256)
            string(REPLACE "/" "-" _jadg_slug "${_jadg_path}")
            string(TOLOWER "${_jadg_slug}" _jadg_slug)
            set(_jadg_witness "${_jadg_assets}/.kits/${_jadg_slug}.json")
            set(_jadg_ok FALSE)
            if(EXISTS "${_jadg_witness}")
                file(READ "${_jadg_witness}" _jadg_witness_json)
                string(JSON _jadg_witness_sha ERROR_VARIABLE _jadg_err GET "${_jadg_witness_json}" sha256)
                if(_jadg_witness_sha STREQUAL _jadg_sha)
                    set(_jadg_ok TRUE)
                endif()
                set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_jadg_witness}")
            endif()
            if(NOT _jadg_ok)
                list(APPEND _jadg_missing "${_jadg_path}")
            endif()
        endforeach()
    endif()
    if(_jadg_missing)
        list(JOIN _jadg_missing "\n  " _jadg_missing_text)
        message(FATAL_ERROR
            "Kits d'assets absents ou périmés :\n  ${_jadg_missing_text}\n"
            "Les installer : python scripts/fetch_assets.py (scripts/build.ps1 le fait avant la "
            "configuration). Passer -DSKIP_ASSET_KITS_CHECK=ON pour outrepasser.")
    endif()
endif()

# --- Cohérence de l'environnement MSVC (générateurs hors Visual Studio) ---
# Les générateurs Ninja / Makefiles n'établissent pas l'environnement MSVC : ils héritent des
# variables LIB et INCLUDE du terminal appelant. Or une invite « Developer PowerShell/Command
# Prompt » démarre en **x86** par défaut. Le compilateur sélectionné par CMake est alors x64, mais
# l'éditeur de liens reçoit les bibliothèques x86 : le build échoue sur des centaines de « symbole
# externe non résolu » (LNK2001) que rien ne rattache à la cause réelle. On détecte l'incohérence
# à la configuration, seul endroit où le message peut être compris.
#
# Le générateur Visual Studio est exempt : MSBuild établit son propre environnement.
#
# Deux incohérences distinctes sont possibles, toutes deux silencieuses jusqu'à l'édition de liens :
#  - terminal x86 : CMake sélectionne le compilateur 32 bits et produit une build 32 bits
#    parfaitement cohérente… mais inutilisable, Qt (msvc*_64) et Direct3D 11 étant fournis en x64.
#    Symptôme observé : « Qt6 introuvable », donc aucun exécutable ;
#  - compilateur x64 imposé (cache d'un IDE) mais LIB héritée d'un terminal x86 : des centaines de
#    LNK2001 sans rapport apparent avec la cause.
option(SKIP_MSVC_ENV_CHECK "Désactiver la vérification d'architecture de l'environnement MSVC" OFF)
if(MSVC AND NOT SKIP_MSVC_ENV_CHECK AND NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    set(_msvc_env_hint
        "Corriger en construisant via le script fourni, qui établit lui-même l'environnement :\n"
        "  pwsh scripts/build.ps1\n"
        "Ou en ouvrant un terminal x64 avant CMake (vcvars64.bat, ou « x64 Native Tools Command "
        "Prompt » — la « Developer PowerShell » démarre en x86).\n"
        "Passer -DSKIP_MSVC_ENV_CHECK=ON pour outrepasser cette vérification.")

    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR
            "Environnement de compilation 32 bits — ce projet cible x64.\n"
            "  Compilateur : ${CMAKE_CXX_COMPILER}\n"
            "Le terminal appelant a été ouvert en x86, et les générateurs hors Visual Studio en "
            "héritent. La build serait cohérente mais inutilisable : Qt (msvc*_64) ne serait pas "
            "trouvé et aucun exécutable ne serait produit.\n"
            ${_msvc_env_hint})
    endif()

    # LIB est la variable que consulte l'éditeur de liens ; les chemins MSVC s'y terminent
    # invariablement par lib/<arch>.
    string(REPLACE "\\" "/" _msvc_lib_env "$ENV{LIB}")
    string(TOLOWER "${_msvc_lib_env}" _msvc_lib_env)
    if(NOT _msvc_lib_env MATCHES "/lib/x64(/|;|$)")
        message(FATAL_ERROR
            "Environnement MSVC incohérent : compilateur x64, bibliothèques non x64.\n"
            "  Compilateur  : ${CMAKE_CXX_COMPILER}\n"
            "  Variable LIB : $ENV{LIB}\n"
            "L'édition de liens échouerait sur des centaines de symboles externes non résolus "
            "(LNK2001) sans rapport apparent avec la cause.\n"
            ${_msvc_env_hint})
    endif()
endif()

# --- Dossiers de sortie ---
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# --- Options ---
option(BUILD_TESTING "Construire les tests (GoogleTest)" ON)
option(ENABLE_WARNINGS_AS_ERRORS "Traiter les avertissements comme des erreurs" ON)
option(ENABLE_ASAN "Activer AddressSanitizer" OFF)
option(ENABLE_PCH "Activer les en-têtes précompilés (Source/pch.h)" ON)
option(ENABLE_COMPILER_CACHE "Compiler à travers sccache s'il est trouvé (hors Visual Studio)" ON)
option(BUILD_FUZZERS "Construire les cibles libFuzzer de Source/Fuzz (clang-cl uniquement)" OFF)
option(BUILD_BENCHMARKS "Construire les mesures de performance de Source/Benchmark" OFF)

# --- Cache de compilation (refonte de la CI, phase 2) ---
# sccache rejoue le résultat d'une compilation déjà faite, sur le poste comme sur le runner : un
# changement de branche entre deux worktrees, ou un second push sur une PR, ne recompile que ce qui
# a changé. Mesuré sur le preset `ninja` : 487 s à froid, 247 s cache plein. Les fichiers compilés
# sous en-tête précompilé (/Yu, /Fp) ne sont pas cachables et restent compilés à chaque fois ; les
# autres (tests, dépendances, cibles sans PCH) le sont — désactiver les PCH n'y gagnerait rien à
# froid.
#
# Actif dès que `sccache` est dans le PATH (scripts/setup_dev.ps1 l'installe à la version de
# ci.yml). Le générateur Visual Studio n'est pas concerné : MSBuild ignore les lanceurs.
# Un lanceur imposé par l'appelant (variable ou environnement) est respecté.
if(ENABLE_COMPILER_CACHE
   AND NOT CMAKE_GENERATOR MATCHES "Visual Studio"
   AND NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER
   AND NOT DEFINED ENV{CMAKE_CXX_COMPILER_LAUNCHER})
    find_program(SCCACHE_PROGRAM sccache)
    # MSVC localisé : Ninja retrouve les en-têtes inclus en filtrant les lignes /showIncludes par leur
    # préfixe, détecté par CMake. En français, ce préfixe contient des espaces insécables
    # (« Remarque : inclusion du fichier : »). sccache 0.18 les SUPPRIME quand il réémet la sortie
    # d'une compilation cachable, et les laisse sur une compilation qu'il ne cache pas (sous en-tête
    # précompilé) : aucun préfixe ne filtre les deux, et Ninja perd des dépendances d'en-têtes —
    # modifier un .h ne recompile plus ce qui l'inclut, sans un message (constaté par `ninja -t deps`
    # le 15 septembre 2026). Avec un préfixe non ASCII, le cache n'est donc PAS activé. Les runners
    # de la CI ont un MSVC anglais (« Note: including file: ») et ne sont pas concernés.
    set(_jadg_cache_prefix_ok TRUE)
    if(SCCACHE_PROGRAM AND MSVC AND NOT CMAKE_CL_SHOWINCLUDES_PREFIX MATCHES "^[ -~]*$")
        set(_jadg_cache_prefix_ok FALSE)
        message(STATUS
            "Cache de compilation non activé : MSVC écrit /showIncludes dans une langue que sccache "
            "réécrit, et Ninja perdrait des dépendances d'en-têtes. Installer le module linguistique "
            "anglais de Visual Studio et définir VSLANG=1033 pour en profiter.")
    endif()
    if(SCCACHE_PROGRAM AND _jadg_cache_prefix_ok)
        set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
        if(MSVC)
            # /Z7 (informations dans chaque .obj) au lieu de /Zi : /Zi fait écrire tous les fichiers
            # d'une cible dans un même .pdb, ce qu'aucun cache ne sait rejouer. L'éditeur de liens
            # produit toujours le .pdb de l'exécutable.
            set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
        endif()
        message(STATUS "Cache de compilation : ${SCCACHE_PROGRAM} "
                       "(-DENABLE_COMPILER_CACHE=OFF pour s'en passer)")
    endif()
endif()

# --- Avertissements (appliqués uniquement à NOS cibles, pas aux dépendances) ---
add_library(project_warnings INTERFACE)
if(MSVC)
    # /external:* : les en-têtes inclus via <...> (STL, DirectX, GoogleTest) sont
    # traités comme externes et n'émettent pas d'avertissement — cohérent avec
    # notre règle « <...> = externe, "..." = projet ».
    target_compile_options(project_warnings INTERFACE
        /W4 /permissive- /external:anglebrackets /external:W0)
    if(ENABLE_WARNINGS_AS_ERRORS)
        target_compile_options(project_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(project_warnings INTERFACE -Wall -Wextra -Wpedantic)
    if(ENABLE_WARNINGS_AS_ERRORS)
        target_compile_options(project_warnings INTERFACE -Werror)
    endif()
endif()

# --- Compilation parallèle sous Visual Studio ---
# MSBuild compile les fichiers d'un projet UN PAR UN sans /MP : à code identique, le preset `vs`
# prenait deux fois le temps du preset `ninja`, qui parallélise lui-même. Portée globale (dépendances
# comprises) : l'option ne change que l'ordonnancement, jamais le code produit. Inutile sous Ninja.
# Pas de `jobs` dans les presets `vs` en plus : /m:N lancerait N projets ayant chacun autant de
# compilateurs que de cœurs, soit N × cœurs processus cl.exe — de quoi épuiser les 16 Gio du runner
# comme d'un poste. Les projets s'enchaînant surtout en série (Core, puis HMI, puis le reste), /MP
# seul occupe déjà tous les cœurs.
if(MSVC AND CMAKE_GENERATOR MATCHES "Visual Studio")
    add_compile_options(/MP)
endif()

# --- Options de compilation transverses (sanitizers) ---
add_library(project_options INTERFACE)
if(ENABLE_ASAN)
    # Portée GLOBALE (add_compile_options/add_link_options), pas seulement sur project_options :
    # sous MSVC, /fsanitize=address instrumente les conteneurs STL (annotate_string,
    # annotate_vector, annotate_optional...), et l'éditeur de liens exige que TOUS les objets liés
    # dans un même binaire partagent la même instrumentation (LNK2038 sinon). GoogleTest est
    # récupéré par FetchContent (add_subdirectory(External) ci-dessous) et ne lie pas
    # project_options : sans portée globale, gtest.lib serait compilé sans le sanitizer et
    # l'édition de liens de tout exécutable de test échouerait (« discordance détectée »).
    if(MSVC)
        add_compile_options(/fsanitize=address)
        target_compile_options(project_options INTERFACE /fsanitize=address)
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
        target_compile_options(project_options INTERFACE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(project_options INTERFACE -fsanitize=address)
    endif()
endif()

# --- Fuzzing des lecteurs de données (refonte de la CI, phase 3) ---
# libFuzzer avec le compilateur MSVC lui-même (/fsanitize=fuzzer, Visual Studio 2022 17.0+), et
# l'AddressSanitizer de MSVC que le job `sanitize` de ci.yml éprouve déjà. clang-cl a été essayé et
# écarté : sous son ASan, le `catch` d'une exception C++ recevait un objet invalide (lu dans une
# mémoire déjà libérée), si bien que chaque lecteur JSON « plantait » sur l'entrée vide.
#
# Toutes nos cibles sont instrumentées pour la couverture (/fsanitize-coverage) : le fuzzer ne mute
# bien que ce dont il voit les branches, et le lecteur de niveaux vit dans Core, pas dans le harnais.
# ASan globalement, pour la même raison que ENABLE_ASAN ci-dessus. Configuration attendue : Ninja,
# RelWithDebInfo, sans PCH ni cache de compilation — voir le job `fuzz` de nightly.yml.
if(BUILD_FUZZERS)
    if(NOT MSVC OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR
            "BUILD_FUZZERS exige le compilateur MSVC (cl.exe), pas ${CMAKE_CXX_COMPILER_ID}.\n"
            "  cmake -S . -B build/fuzz -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_FUZZERS=ON "
            "-DBUILD_TESTING=OFF -DBUILD_EDITOR_QT=OFF -DENABLE_PCH=OFF -DENABLE_COMPILER_CACHE=OFF")
    endif()
    if(ENABLE_ASAN)
        message(FATAL_ERROR "BUILD_FUZZERS active déjà AddressSanitizer : retirer ENABLE_ASAN.")
    endif()
    add_compile_options(/fsanitize=address
        /fsanitize-coverage=inline-8bit-counters /fsanitize-coverage=edge
        /fsanitize-coverage=trace-cmp /fsanitize-coverage=trace-div)
endif()

# --- Dépendances tierces (External/) ---
add_subdirectory(External)

# --- Code source ---
add_subdirectory(Source)

# --- Tests ---
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(Source/Test)
endif()

if(BUILD_FUZZERS)
    add_subdirectory(Source/Fuzz)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(Source/Benchmark)
endif()
