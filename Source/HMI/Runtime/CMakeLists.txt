# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

# Jadg.Runtime : ce que le jeu SAIT DIRE, exposé au QML (LOT-87).
#
# Les vues-modèles (fiche, inventaire, réglages, navigation, données en attente) et la surface de
# rendu Qt Quick. C'est le seul module QML du projet qui contienne du C++, et c'est précisément
# pour cela qu'il est à part : Qt Design Studio ne charge aucun plugin C++ dans son marionnettiste
# (`qmlpuppet`), donc tout type C++ mêlé à un module de formulaires rend ce module irrésolvable dans
# l'atelier. Ici, les formulaires (`Jadg.Ui`) n'importent jamais ce module ; seuls les jumeaux de
# câblage (`Jadg.App`) le font, et l'atelier le remplace par des doublures (Source/Ui/Mocks/).
#
# Bibliothèque STATIQUE liée par le jeu. Les types s'enregistrent par le fichier engendré par
# `qmltyperegistrar` (les macros QML_ELEMENT / QML_SINGLETON des en-têtes) : rien n'est enregistré
# à la main, et ajouter un type se fait en écrivant la macro dans son en-tête.
#
# Les vues-modèles ne connaissent ni Quick ni Widgets (EX-IHM-101, `check_ui_layers.py`) ; seule
# `GameViewportItem`, `ArenaViewportItem` et `WorldViewportItem`, les surfaces de rendu, lient Quick -- ce sont des items de
# scène, pas des modèles.

qt_add_qml_module(JadgRuntime
    URI Jadg.Runtime
    VERSION 1.0
    STATIC
    SOURCES
        OptionsModel.h OptionsModel.cpp
        ScreenRouter.h ScreenRouter.cpp
        PendingData.h PendingData.cpp
        CharacterSheetModel.h CharacterSheetModel.cpp
        InventoryModel.h InventoryModel.cpp
        CreditsModel.h CreditsModel.cpp
        WorldMapModel.h WorldMapModel.cpp
        SheetRowModel.h SheetRowModel.cpp
        RuleLabels.h RuleLabels.cpp
        DemonstrationCharacter.h DemonstrationCharacter.cpp
        GameViewportItem.h GameViewportItem.cpp
        ArenaModel.h ArenaModel.cpp
        # La scene de combat du Colisee sur QRhi (LOT-86 Phase 5) : l'item, et le pipeline 2D qu'il
        # est le premier ecran du jeu a faire tourner -- jusqu'ici compile dans l'editeur seul.
        ArenaViewportItem.h ArenaViewportItem.cpp
        ../Graphics/ArenaSceneRenderer.cpp
        # Le lieu qu'on parcourt (LOT-09) : la vue-modele de l'exploration, sa surface de rendu, et
        # le rendu QRhi du lieu -- le meme pipeline 2D que l'arene, sur une carte au lieu d'une
        # grille de combat.
        WorldModel.h WorldModel.cpp
        WorldViewportItem.h WorldViewportItem.cpp
        ../Graphics/WorldSceneRenderer.cpp
        # Les deux niveaux du plan sous la ville (LOT-96) : le quartier et ses ilots, et l'image
        # d'un ilot, dessinee hors ecran par le rendu du lieu.
        CityDistrictModel.h CityDistrictModel.cpp
        CityBlockImageProvider.h CityBlockImageProvider.cpp
        ../Graphics/CityBlockRender.cpp
        # La galerie des assets : un outil de debug, pas un ecran du jeu (`--screen=AssetGallery`).
        AssetGalleryItem.h AssetGalleryItem.cpp
        ../Graphics/AssetGalleryRenderer.cpp
        ../Graphics/SceneResources.cpp
        ../Graphics/SpriteBatch.cpp
        ../Graphics/SpriteRenderer.cpp
        ../Graphics/TextureLoader.cpp
        ../Graphics/TextureAtlas.cpp
        ../Graphics/TextureCache.cpp
        DialogueModel.h DialogueModel.cpp
        # Le journal de quetes (LOT-116) : relu dans les drapeaux de la partie.
        QuestJournalModel.h QuestJournalModel.cpp
        # La manette du jeu Qt Quick (LOT-24) : le sondage XInput, jusqu'ici compile dans
        # l'editeur seul, devient un QObject qui emet des appuis nommes.
        GamepadNavigator.h GamepadNavigator.cpp
        ../Input/GamepadPoller.cpp
)

# Le fichier d'enregistrement des types est ENGENDRÉ et inclut les en-têtes par NOM DE BASE
# (`#if __has_include(<OptionsModel.h>)`), pas par le chemin « HMI/Runtime/… » qu'emploie le reste
# du projet. Sans ce répertoire dans les chemins d'inclusion, le `__has_include` échoue en silence
# et la compilation du fichier engendré échoue sur un identificateur que personne n'a écrit.
# Les credits (LOT-87, T3.3) sont EMBARQUES : des attributions que les licences exigent ne dependent
# pas d'un fichier pose a cote de l'executable, qu'un deploiement pourrait oublier.
qt_add_resources(JadgRuntime "JadgCredits"
    PREFIX "/jadg/credits"
    BASE "${PROJECT_SOURCE_DIR}/Source/Elements/Credits"
    FILES "${PROJECT_SOURCE_DIR}/Source/Elements/Credits/credits.json"
)

# Les shaders du pipeline 2D, embarques comme dans l'editeur : `hmi::SpriteBatch` les charge depuis
# les ressources (`:/shaders/...`), et le jeu n'en avait pas tant qu'aucun ecran ne dessinait par QRhi.
qt6_add_shaders(JadgRuntime "jadg_runtime_shaders"
    PREFIX "/shaders"
    BASE "${PROJECT_SOURCE_DIR}/Source/HMI/Graphics/Shaders"
    FILES
        ${PROJECT_SOURCE_DIR}/Source/HMI/Graphics/Shaders/sprite.vert
        ${PROJECT_SOURCE_DIR}/Source/HMI/Graphics/Shaders/sprite.frag
)

target_include_directories(JadgRuntime
    PUBLIC ${PROJECT_SOURCE_DIR}/Source
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(JadgRuntime PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)
target_link_libraries(JadgRuntime
    PUBLIC HmiLib Qt6::Qml Qt6::Quick Qt6::Gui Qt6::GuiPrivate
    PRIVATE xinput project_warnings project_options)
