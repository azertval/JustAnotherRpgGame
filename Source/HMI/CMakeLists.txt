# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

# Couche de présentation HMI : ce que le jeu (`JustAnotherRpgGame`, Qt Quick) et l'éditeur
# (`LevelEditor`, Qt Widgets) partagent. L'éditeur lui-même vit dans son module, Source/Editor
# (LOT-EDITOR-01) : il dépend de HMI, et rien ici ne dépend de lui. Le code est réparti par domaine :
#   Platform/     provisionnement bas niveau (répertoire exécutable, minidump) ;
#   Input/        entrées (état, manette, table Qt -> touche) ;
#   Graphics/     rendu 2D sur QRhi (sprites, caméra, lieu, arène) ;
#   Game/         la carte qu'on parcourt, partagée par le jeu et l'essai de l'éditeur ;
#   Presentation/ logique de présentation pure (table de transitions, échelle, valeurs de fiche) ;
#   Runtime/      les types C++ que les écrans du jeu voient -- module QML `Jadg.Runtime` (LOT-87) ;
#   Localization/ catalogue de traduction.

# --- SceneComposition : la composition d'un lieu, sans GPU ni Qt (LOT-EDITOR-02) ---
#
# Ce qu'un lieu met à l'écran, en primitives ordonnées : la liste (`ComposedScene`), la table du lieu
# (`PlaceAppearance`), la composition d'une carte (`WorldSceneComposer`) et la géométrie des pièces
# (`ScenePieces.h`, en-tête seul). Le jeu la soumet au GPU (`WorldSceneRenderer`), l'éditeur la peint
# avec QPainter : **une** composition, deux façons de la dessiner, et l'ordre de dessin est le même
# par construction (décision D2 de Planning/vision/archives/feuille-de-route-editeur.md, constat A8).
#
# Les fichiers restent sous HMI/Graphics, dans l'espace de noms `hmi` : ce qui sort de HmiLib est la
# frontière d'édition de liens, pas les chemins d'inclusion que quarante fichiers écrivent. HmiLib la
# lie en PUBLIC, rien ne change pour le jeu.
add_library(SceneComposition STATIC
    Graphics/ComposedScene.cpp
    Graphics/MaquetteTokens.cpp
    Graphics/PlaceAppearance.cpp
    Graphics/WorldSceneComposer.cpp
)
target_include_directories(SceneComposition PUBLIC ${PROJECT_SOURCE_DIR}/Source)
target_link_libraries(SceneComposition
    PUBLIC Core
    PRIVATE project_warnings project_options)
if(ENABLE_PCH)
    target_precompile_headers(SceneComposition PRIVATE ${PROJECT_SOURCE_DIR}/Source/pch.h)
endif()

# --- HmiLib : la couche de présentation **sans Qt Widgets** (LOT-02) ---
#
# Ces sources sont de la logique pure : géométrie de caméra, composition de scène (sans GPU),
# catalogues, entrées, persistance JSON, géométrie d'éditeur. Elles ne dérivent d'aucun QObject et
# n'incluent aucun en-tête Qt — c'est exactement ce que les tests unitaires consomment.
#
# Avant ce lot, `Source/Test/CMakeLists.txt` **relistait ces 79 chemins un par un** pour les
# recompiler dans `UnitTests` : chaque fichier ajouté à l'IHM devait l'être à deux endroits, et un
# oubli ne se voyait qu'à l'édition de liens. Une bibliothèque nomme cette frontière au lieu de la
# maintenir à la main dans un commentaire.
#
# Déclarée **avant** la garde `BUILD_EDITOR_QT`/Qt ci-dessus, et volontairement libre de Qt : sur un
# poste sans Qt, l'application est ignorée mais `UnitTests` doit continuer de se construire — c'est
# la raison d'être de l'option. AUTOMOC n'est pas encore actif à ce point du fichier, ce qui est
# correct : aucune de ces sources n'a de `Q_OBJECT`.
add_library(HmiLib STATIC
    # Graphismes : logique de rendu sans GPU (composition, catalogues, géométrie de caméra).
    Graphics/Camera2D.cpp
    Graphics/TileVisuals.cpp
    Graphics/ProceduralAtlas.cpp
    Graphics/TextureAtlasRegions.cpp
    Graphics/ArenaAppearanceCatalog.cpp
    Graphics/ArenaSceneComposer.cpp
    # Le lieu qu'on parcourt : sa table et sa composition vivent dans SceneComposition (ci-dessus).
    # La carte qu'on parcourt et sa mise en scene, partagees par le jeu et l'essai de l'editeur.
    Game/WorldPlay.cpp
    # L'essai complet (LOT-EDITOR-10) : la ligne de commande que l'editeur ecrit et que le jeu lit.
    Game/LaunchOptions.cpp
    Graphics/AssetGallery.cpp
    Graphics/ArenaAnimationDriver.cpp
    Graphics/AnimationCatalog.cpp
    Graphics/MissingTexture.cpp
    Graphics/QuadRecorder.cpp
    # Marqueurs d'entite de carte (LOT-11) : cle d'asset et pixels, sans GPU.
    Graphics/EntityMarkers.cpp
    # Ce que les fichiers voisins d'une image disent d'elle : decoupe, echelle, ancre (LOT-103).
    Graphics/SceneTextureTraits.cpp
    # Entrées : état, mapping, remappage persistant.
    Input/InputState.cpp
    # Localisation, journal de session.
    Localization/Localization.cpp
    # Interface : logique d'écrans, sans widget.
    Presentation/ScreenFlow.cpp
    Presentation/RpgScreens.cpp
    Presentation/CharacterSheetValues.cpp
    Presentation/InventoryValues.cpp
    Presentation/DialogueScreen.cpp
    Presentation/CreditsCatalog.cpp
    Presentation/InventoryScreen.cpp
    Presentation/WorldMaps.cpp
    Presentation/IdentityScale.cpp
    # Plateforme.
    Platform/ExecutableDirectory.cpp
    Platform/CrashDump.cpp
)

# `PUBLIC` sur la racine d'inclusion et sur Core : tout consommateur de HmiLib (l'application, les
# tests) écrit `#include "HMI/..."` et manipule des types de `core::`.
target_include_directories(HmiLib PUBLIC ${PROJECT_SOURCE_DIR}/Source)
target_link_libraries(HmiLib
    PUBLIC Core
    # La composition d'un lieu, sans GPU (LOT-EDITOR-02) : le jeu et l'éditeur la partagent.
    PUBLIC SceneComposition
    PRIVATE project_warnings project_options
    # MiniDumpWriteDump (Platform/CrashDump.cpp) : bibliothèque système, présente sur tout Windows.
    PRIVATE dbghelp
    # Les *KeyBindings et les catalogues persistent en JSON. Core le propage depuis le LOT-79,
    # mais HmiLib le redemande explicitement : ses en-tetes en dependent pour leur compte.
    PUBLIC nlohmann_json::nlohmann_json)

target_compile_definitions(HmiLib PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

if(ENABLE_PCH)
    target_precompile_headers(HmiLib PRIVATE ${PROJECT_SOURCE_DIR}/Source/pch.h)
endif()

# Tout ce qui suit demande Qt. La découverte de Qt et l'option BUILD_EDITOR_QT vivent dans
# Source/CMakeLists.txt depuis le LOT-87 : trois répertoires frères (HMI, Ui, App) en dépendent, et
# les cibles importées d'un find_package ne sont visibles que sous le répertoire qui l'a appelé.
if(NOT JADG_GAME_ENABLED)
    return()
endif()



# Traitement automatique des métacompilateurs Qt pour Runtime (moc). L'éditeur (LevelEditor, Qt
# Widgets) est déclaré dans Source/Editor depuis le LOT-EDITOR-01, le jeu dans Source/App.
set(CMAKE_AUTOMOC ON)

# Les types C++ que les écrans du JEU voient (module QML `Jadg.Runtime`, LOT-87) : après AUTOMOC,
# qu'ils exigent, et après HmiLib, qu'ils lient. Le jeu lui-même est dans Source/App.
add_subdirectory(Runtime)

# Copie des assets à côté des exécutables, pour que le jeu et l'éditeur chargent niveaux,
# traductions et catalogues relativement à leur binaire. Les deux cibles sortent dans le même
# répertoire : une seule copie les sert toutes les deux.
#
# Cible séparée (et non des POST_BUILD) : sous Ninja, un POST_BUILD est fondu dans l'arête de build
# de l'exécutable et n'est rejoué que si celle-ci l'est — c'est-à-dire quand un .obj change.
# Modifier uniquement un fichier sous Source/Elements/ n'en touche aucun, donc l'exécutable reste
# « à jour » aux yeux de Ninja et la copie est silencieusement sautée, même via un cmake --build
# complet. Une cible dédiée sans sortie déclarée est toujours considérée sale et s'exécute donc à
# chaque build.
add_custom_target(CopyGameData ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/Source/Elements/Localization
        $<TARGET_FILE_DIR:JustAnotherRpgGame>/Localization
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/Source/Elements/Levels
        $<TARGET_FILE_DIR:JustAnotherRpgGame>/Levels
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/Source/Elements/Assets
        $<TARGET_FILE_DIR:JustAnotherRpgGame>/Assets
    # Catalogues RPG (LOT-38) : espèces, classes, historiques, compétences, règles et fiches.
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/Source/Elements/Rpg
        $<TARGET_FILE_DIR:JustAnotherRpgGame>/Rpg
    # Atlas du monde (LOT-37) et positions relevees sur les cartes de l'auteur (LOT-94, LOT-95).
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/Source/Elements/World
        $<TARGET_FILE_DIR:JustAnotherRpgGame>/World
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/Source/Elements/Maps
        $<TARGET_FILE_DIR:JustAnotherRpgGame>/Maps
    COMMENT "Copie des données de jeu (traductions, cartes, assets, catalogues RPG)")
