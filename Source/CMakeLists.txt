# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

# Modules de code source
#
# Core     : logique / moteur (bibliothèque), sans Qt
# HMI      : la couche de présentation -- HmiLib (sans Qt), l'éditeur de niveaux (Qt Widgets) et
#            Runtime/, les types C++ que les écrans du jeu voient (module QML `Jadg.Runtime`)
# Ui       : les écrans du jeu, en QML déclaratif (module `Jadg.Ui`) -- le territoire de la
#            conception, ouvert par Qt Design Studio, sans une ligne de C++
# App      : les exécutables -- le jeu (Qt Quick, module `Jadg.App` : fenêtre, câblage, jumeaux)
#            et l'éditeur (dont les sources sont listées depuis HMI)
# Elements : assets statiques non compilés (niveaux, localisation, UI Qt, thèmes, textures…)
#
# Trois modules QML, chacun dans le répertoire de ses fichiers (LOT-87). Ce n'est pas un choix de
# rangement : `qt_add_qml_module` calcule le chemin de ressource de chaque fichier RELATIVEMENT au
# répertoire du CMakeLists.txt qui l'appelle. Un module déclaré depuis un autre répertoire oblige à
# réécrire ces chemins un par un (QT_RESOURCE_ALIAS), et c'est cette plomberie qui a coûté 125
# commits à une branche abandonnée. Aucun alias dans ce dépôt, et c'est vérifié.

add_subdirectory(Core)

# La cible reste **optionnelle** : sur un poste sans Qt (ex. CI n'exécutant que les tests unitaires,
# qui compilent les sources pures directement), la configuration n'échoue pas — l'application est
# simplement ignorée. Fournir Qt via CMAKE_PREFIX_PATH (ou l'action CI install-qt-action) pour
# l'activer, ou passer -DBUILD_EDITOR_QT=OFF pour l'ignorer explicitement.
option(BUILD_EDITOR_QT "Construire l'éditeur Qt (JustAnotherRpgGame). Nécessite Qt6." ON)
# Vrai quand Qt est trouvé ET voulu : les trois répertoires Qt (HMI après HmiLib, Ui, App) le lisent.
set(JADG_GAME_ENABLED ON)
if(NOT BUILD_EDITOR_QT)
    message(STATUS "JustAnotherRpgGame : désactivé (BUILD_EDITOR_QT=OFF).")
    set(JADG_GAME_ENABLED OFF)
endif()

# --- Version de référence (EX-BUILD-010) ---
# Seul endroit où la version de Qt est écrite dans le CMake : la CI (`.github/workflows/ci.yml`,
# `release.yml`) l'installe explicitement à cette même valeur (`env.QT_VERSION`), vérifié
# automatiquement par `scripts/ci/check_qt_version_pin.py` (job `lint-exigences`) pour que les deux
# ne divergent jamais silencieusement.
set(QT_VERSION_MINIMUM "6.11.2")

# --- Découverte de Qt sans configuration manuelle ---
# find_package() ne consulte que CMAKE_PREFIX_PATH. Or l'installateur officiel Qt place ses
# versions dans <racine>/<version>/<compilateur> — un emplacement que CMake ne connaît pas. Sur un
# poste neuf, la configuration ne trouvait donc rien et la cible était ignorée *en silence* : le
# build « réussissait » sans jamais produire d'exécutable. On complète la recherche par les
# emplacements conventionnels, en les ajoutant **en fin** de liste pour qu'un choix explicite
# (-DCMAKE_PREFIX_PATH, -DQt6_DIR) garde toujours la priorité.
set(_qt_hints "")

# Variables d'environnement usuelles. QT_ROOT_DIR est exporté par install-qt-action (CI) ;
# QTDIR est la convention historique de l'installateur.
foreach(_qt_var QT_ROOT_DIR Qt6_ROOT QTDIR)
    if(DEFINED ENV{${_qt_var}})
        list(APPEND _qt_hints "$ENV{${_qt_var}}")
    endif()
endforeach()

# Arborescences de l'installateur officiel. Le tri décroissant naturel place la version la plus
# récente en tête (6.11 avant 6.9, ce qu'un tri lexicographique ferait à l'envers).
if(WIN32)
    file(GLOB _qt_installs
        "$ENV{SystemDrive}/Qt/*/msvc*_64"
        "C:/Qt/*/msvc*_64"
        "D:/Qt/*/msvc*_64")
else()
    file(GLOB _qt_installs
        "$ENV{HOME}/Qt/*/gcc_64"
        "/opt/Qt/*/gcc_64")
endif()
list(SORT _qt_installs COMPARE NATURAL ORDER DESCENDING)
list(APPEND _qt_hints ${_qt_installs})

if(_qt_hints)
    list(REMOVE_DUPLICATES _qt_hints)
    list(APPEND CMAKE_PREFIX_PATH ${_qt_hints})
endif()

# Un seul `windeployqt` à la fois. Le jeu (Source/App) et l'éditeur (Source/HMI) déploient chacun,
# en POST_BUILD, les DLL Qt **et le runtime MSVC** dans le même `bin/`. Sous Ninja, les deux
# éditions de liens tournent en parallèle, et l'une ouvre `MSVCP140D.dll` pendant que l'autre la
# copie : « The process cannot access the file because it is being used by another process », le
# lien du jeu échoue, `build-ninja` rougit sans rapport avec le code (vu aux LOT-15 et LOT-22). Sous
# Ninja, le POST_BUILD fait partie de l'arête de lien : une réserve de lien de taille 1 sérialise
# donc les deux déploiements, et rien d'autre. Le générateur Visual Studio ignore la propriété.
set_property(GLOBAL APPEND PROPERTY JOB_POOLS deploiement_qt=1)

find_package(Qt6 ${QT_VERSION_MINIMUM} QUIET COMPONENTS Widgets Gui GuiPrivate Multimedia ShaderTools Quick Qml QuickControls2)
if(NOT Qt6_FOUND)
    # Deuxième essai, sans contrainte de version, seulement pour distinguer dans le message
    # « rien d'installé » de « une version installée, mais trop ancienne » — sinon les deux
    # produisent le même NOT Qt6_FOUND et le second cas serait diagnostiqué à tort comme le premier.
    find_package(Qt6 QUIET COMPONENTS Widgets Gui GuiPrivate Multimedia ShaderTools Quick Qml QuickControls2)
    # Avertissement et non simple statut : sans Qt il n'y a **aucun exécutable**, seulement les
    # tests. Passer inaperçu est précisément ce qui rendait l'absence d'exe difficile à diagnostiquer.
    if(Qt6_FOUND)
        message(WARNING
            "JustAnotherRpgGame : Qt6 ${Qt6_VERSION} trouvé (${Qt6_DIR}), mais inférieur à la version "
            "minimale requise ${QT_VERSION_MINIMUM} — cible ignorée, AUCUN EXÉCUTABLE ne sera produit.\n"
            "Installer au moins Qt ${QT_VERSION_MINIMUM} (ou désigner une autre installation via "
            "-DCMAKE_PREFIX_PATH), ou passer -DBUILD_EDITOR_QT=OFF pour ne construire que les tests.")
    else()
        # Troisième cas, distinct des deux précédents : Qt est bien installé, mais il lui manque un
        # MODULE (typiquement Multimedia, un composant additionnel que l'installateur officiel ne
        # coche pas par défaut). find_package échoue alors en bloc, et annoncer « Qt6 introuvable »
        # envoie chercher au mauvais endroit — le cas a réellement coûté une session de diagnostic.
        find_package(Qt6 QUIET COMPONENTS Widgets Gui)
        if(Qt6_FOUND)
            set(_qt_missing "")
            foreach(_qt_component Widgets Gui GuiPrivate Multimedia ShaderTools Quick Qml QuickControls2)
                find_package(Qt6${_qt_component} QUIET)
                if(NOT Qt6${_qt_component}_FOUND)
                    list(APPEND _qt_missing ${_qt_component})
                endif()
            endforeach()
            message(WARNING
                "JustAnotherRpgGame : Qt6 ${Qt6_VERSION} est bien installé (${Qt6_DIR}), mais il manque "
                "le(s) module(s) : ${_qt_missing} — cible ignorée, AUCUN EXÉCUTABLE ne sera produit.\n"
                "ShaderTools (l'outil `qsb`) et GuiPrivate (les en-têtes QRhi) sont exigés : le rendu "
                "passe par QRhi, plus par Direct3D 11 en direct.\n"
                "Quick, Qml et QuickControls2 le sont depuis le LOT-86 : les écrans du jeu sont "
                "du QML, et Widgets ne sert plus qu'à l'éditeur de niveaux.
"
                "Ajouter le(s) module(s) manquant(s) avec l'outil de maintenance Qt "
                "(MaintenanceTool.exe, à la racine de l'installation), puis reconfigurer.\n"
                "Passer -DBUILD_EDITOR_QT=OFF pour ne construire que les tests, sans cet avertissement.")
        else()
            message(WARNING
                "JustAnotherRpgGame : Qt6 introuvable, cible ignorée — AUCUN EXÉCUTABLE ne sera produit.\n"
                "Version minimale requise : ${QT_VERSION_MINIMUM}.\n"
                "Emplacements examinés : ${_qt_hints}\n"
                "Corriger en installant Qt via l'installateur officiel, ou en désignant l'installation :\n"
                "  cmake --preset ninja -DCMAKE_PREFIX_PATH=<chemin>/msvc2022_64\n"
                "Passer -DBUILD_EDITOR_QT=OFF pour ne construire que les tests, sans cet avertissement.")
        endif()
    endif()
    set(JADG_GAME_ENABLED OFF)
endif()

if(JADG_GAME_ENABLED AND NOT Qt6_VERSION VERSION_EQUAL QT_VERSION_MINIMUM)
    # Avertissement, pas erreur : interdire de construire sur une version voisine
    # coûterait plus qu'il ne rapporterait. Le but est qu'un écart soit visible, pas interdit.
    message(WARNING
        "JustAnotherRpgGame : Qt6 ${Qt6_VERSION} trouvé (${Qt6_DIR}), différent de la version de "
        "référence ${QT_VERSION_MINIMUM} (celle validée par la CI) — le build peut diverger de la "
        "CI sans que rien d'autre ne le signale.")
endif()

if(JADG_GAME_ENABLED)
    message(STATUS "JustAnotherRpgGame : Qt6 ${Qt6_VERSION} trouvé (${Qt6_DIR}).")
endif()

if(JADG_GAME_ENABLED)
    # Politiques de Qt sur les modules QML, prises telles que Qt 6.8+ les recommande, et posées ici
    # pour les trois modules : le préfixe de ressource `/qt/qml` (QTP0001, là où l'engine cherche)
    # et un `qmldir` engendré par sous-répertoire (QTP0004, ce qui rend `Screens/` et `Controls/`
    # visibles entre eux sans les aplatir).
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)

    # Un seul répertoire de sortie pour les trois modules : chaque `qmldir` engendré atterrit sous
    # build/<preset>/qml/Jadg/<Module>/. C'est ce qui donne à l'outillage (qmllint, qmlls) UN chemin
    # d'import où il résout les trois modules à la fois -- sans lui, la cible `all_qmllint` ne voit
    # que le module qu'elle vérifie, et signale chaque écran en erreur sur `import Jadg.Ui`. Qt le
    # recommande pour tout projet à plusieurs modules ; le jeu, lui, lit la ressource embarquée.
    set(QT_QML_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml)
endif()

add_subdirectory(HMI)
# Le module éditeur (LOT-EDITOR-01) : dépend de HMI, et rien ne dépend de lui.
add_subdirectory(Editor)
if(JADG_GAME_ENABLED)
    add_subdirectory(Ui)
    add_subdirectory(App)
endif()
