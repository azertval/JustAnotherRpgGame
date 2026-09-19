# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

# Le module éditeur (LOT-EDITOR-01, décision D5 de Documentation/Editeur/feuille-de-route.md).
#
#   Logic/  la logique pure de l'éditeur, sans Qt : bibliothèque `EditorLogic`, testée par
#           `UnitTests` (Source/Test/Unit/Editor) ;
#   Ui/     les widgets, construits en code (ni .ui ni charte) : fenêtre, canevas, panneaux --
#           l'exécutable `LevelEditor`.
#
# Le module dépend de Core et de HmiLib (composition, rendu, carte jouée par l'essai). Rien ne
# dépend de lui : ni le jeu, ni HmiLib. L'éditeur est un outil interne ; son point d'entrée reste
# dans Source/App/Editor, à côté de celui du jeu.

# --- EditorLogic : sans Qt, déclarée avant la garde Qt --------------------------------------------
#
# Même raison que HmiLib : sur un poste sans Qt (job sanitize, BUILD_EDITOR_QT=OFF), l'éditeur
# n'est pas construit, mais ses tests doivent l'être.
add_library(EditorLogic STATIC
    Logic/LevelNameValidation.cpp
    Logic/TileTaxonomy.cpp
    Logic/LevelFileOperations.cpp
    Logic/EditorStatus.cpp
    Logic/PanelFocus.cpp
    Logic/ThumbnailGeometry.cpp
    Logic/LayerView.cpp
    Logic/EntityGesture.cpp
    Logic/EntityShapes.cpp
    Logic/EntityReferences.cpp
    Logic/EditorDiagnostics.cpp
    Logic/WorldGraphLayout.cpp
    Logic/EditorKeyBindings.cpp
    # Sauvegarde automatique, reprise et garde du fichier modifié sur disque.
    Logic/Autosave.cpp
    Logic/DiskGuard.cpp
    # Le canevas qui montre le lieu (LOT-EDITOR-02) : pointage iso et à plat, logique pure.
    Logic/CanvasPicking.cpp
    Logic/CanvasScene.cpp
    # La garde du format de carte v4 (LOT-EDITOR-12) : --migrate et --check, sans fenêtre.
    Logic/MapFormat.cpp
    # Peindre avec les pièces du lieu (LOT-EDITOR-03) : catalogue et coups de pinceau.
    Logic/PieceCatalog.cpp
    Logic/BrushGesture.cpp
    Logic/PaintTools.cpp
    Logic/EditorSidecar.cpp
    # L'éditeur sans fenêtre (LOT-EDITOR-13) : --apply rejoue des gestes par les fonctions des outils.
    Logic/GestureScript.cpp
    # Les cartes se font dans l'éditeur (LOT-EDITOR-06) : la racine des données qu'il ouvre.
    Logic/DataRoot.cpp
    # Le contrôle du contenu (LOT-EDITOR-07) : références, terrain, atteignabilité, textes.
    Logic/ContentCheck.cpp
    Logic/MapTexts.cpp
)
target_include_directories(EditorLogic PUBLIC ${PROJECT_SOURCE_DIR}/Source)
target_link_libraries(EditorLogic
    # La composition d'un lieu, sans GPU (LOT-EDITOR-02) : ce que le canevas peint.
    PUBLIC SceneComposition
    PUBLIC HmiLib
    PRIVATE project_warnings project_options)
if(ENABLE_PCH)
    target_precompile_headers(EditorLogic PRIVATE ${PROJECT_SOURCE_DIR}/Source/pch.h)
endif()

if(NOT JADG_GAME_ENABLED)
    return()
endif()

# --- LevelEditor : l'outil d'auteur, en Qt Widgets ------------------------------------------------
#
# Deux applications depuis le LOT-86 : le jeu (Qt Quick, ne lie pas Qt6::Widgets) et l'éditeur. Elles
# ne partagent aucune technologie d'IHM : un widget ne peut pas réapparaître dans le jeu par
# inadvertance, l'édition de liens échouerait.
set(CMAKE_AUTOMOC ON)

set(APP_DIR ${PROJECT_SOURCE_DIR}/Source/App)
set(HMI_DIR ${PROJECT_SOURCE_DIR}/Source/HMI)

add_executable(LevelEditor
    ${APP_DIR}/Editor/Main.cpp
    ${APP_DIR}/Common/Bootstrap.cpp
    # Pont Qt -> Key : les raccourcis remappables de l'éditeur.
    ${HMI_DIR}/Input/QtKeyMap.cpp
    # Le canevas (LOT-EDITOR-02) : la composition du jeu, peinte par QPainter -- plus de QRhi ni de
    # GPU dans l'éditeur (décision D2). La vue à plat, les images des planches, la mini-carte.
    Ui/DraftRenderer.cpp
    Ui/SceneImages.cpp
    Ui/ScenePainter.cpp
    Ui/MiniMap.cpp
    # --render (LOT-EDITOR-13) : une carte rendue hors écran en PNG, par le peintre du canevas.
    Ui/MapRender.cpp
    # La fenêtre et ses actions.
    Ui/EditorActions.cpp
    Ui/MainWindow.cpp
    # Canevas et panneaux.
    Ui/EditorViewport.cpp
    Ui/PalettePanel.cpp
    Ui/LevelBrowserPanel.cpp
    # Le panneau « Problems » (LOT-EDITOR-07) : les constats du contrôle, sur toutes les cartes.
    Ui/ProblemsPanel.cpp
    Ui/LayersPanel.cpp
    Ui/EntityPanel.cpp
    Ui/WorldGraphView.cpp
)

target_link_libraries(LevelEditor PRIVATE
    EditorLogic
    HmiLib
    Qt6::Widgets
    Qt6::Gui
    project_warnings
    project_options
)

target_include_directories(LevelEditor PRIVATE ${PROJECT_SOURCE_DIR}/Source)
target_compile_definitions(LevelEditor PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
# L'arbre des sources qui a construit l'éditeur : il y ouvre les cartes, pas dans la copie que
# CopyGameData refait à chaque construction (LOT-EDITOR-06, hmi::resolveDataRoot).
target_compile_definitions(LevelEditor PRIVATE
    JADG_EDITOR_SOURCE_DATA="${PROJECT_SOURCE_DIR}/Source/Elements")
if(ENABLE_PCH)
    target_precompile_headers(LevelEditor PRIVATE ${PROJECT_SOURCE_DIR}/Source/pch.h)
endif()

# Données de jeu à côté de l'exécutable : la cible CopyGameData est déclarée dans Source/HMI.
add_dependencies(LevelEditor CopyGameData)

# Déploiement Qt : copie les DLL Qt (plateforme, styles) à côté de l'éditeur. Le jeu a le sien, dans
# Source/App : lui seul a des modules QML à embarquer.
get_target_property(_qmake_exe Qt6::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_exe}" DIRECTORY)
find_program(WINDEPLOYQT_EXE windeployqt HINTS "${_qt_bin_dir}")
if(WINDEPLOYQT_EXE)
    add_custom_command(TARGET LevelEditor POST_BUILD
        COMMAND "${WINDEPLOYQT_EXE}"
            --no-translations --compiler-runtime
            "$<TARGET_FILE:LevelEditor>"
        COMMENT "Déploiement des DLL Qt + runtime à côté de l'éditeur (windeployqt)")
    # Le déploiement du jeu vise le même `bin/` : jamais en même temps (Source/CMakeLists.txt).
    set_property(TARGET LevelEditor PROPERTY JOB_POOL_LINK deploiement_qt)
else()
    message(STATUS "LevelEditor : windeployqt introuvable, DLL Qt non déployées "
        "automatiquement (exécution possible depuis un environnement Qt).")
endif()
