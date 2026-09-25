# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

# Tests unitaires (GoogleTest).
include(GoogleTest)

add_executable(UnitTests
    # Les assertions de la CRT (Debug) vont sur stderr, jamais dans une boîte modale : sans ce
    # fichier, un test qui en lève une attend un clic -- sur le poste comme en CI.
    Support/CrtReports.cpp
    # L'arborescence des tests unitaires reflète celle des sources (Source/) :
    # le test de Source/<Module>/<X> vit sous Unit/<Module>/test_<x>.cpp.
    Unit/Core/test_core.cpp
    Unit/Core/Math/test_vector2.cpp
    Unit/Core/Math/test_rect.cpp
    Unit/Core/Math/test_math_utils.cpp
    Unit/Core/Math/test_deterministic_random.cpp
    Unit/Core/Ecs/test_entity_manager.cpp
    Unit/Core/Ecs/test_animation_clip.cpp
    Unit/Core/Ecs/test_component_pool.cpp
    Unit/Core/Ecs/test_view.cpp
    Unit/Core/Ecs/test_world.cpp
    Unit/Core/Ecs/test_sprite.cpp
    Unit/Core/Data/test_json_document.cpp
    Unit/Core/Levels/test_level.cpp
    Unit/Core/Levels/test_level_loader.cpp
    Unit/Core/Levels/test_level_writer.cpp
    Unit/Core/Levels/test_map_layers.cpp
    Unit/Core/Levels/test_level_draft.cpp
    Unit/Core/Levels/test_level_draft_editing.cpp
    Unit/Core/Levels/test_tile_type_name.cpp
    Unit/Core/Rpg/test_check.cpp
    Unit/Core/Rpg/test_dialogue.cpp
    Unit/Core/Rpg/test_dice.cpp
    Unit/Core/Rpg/test_equipment.cpp
    Unit/Core/Rpg/test_inventory.cpp
    Unit/Core/Combat/test_encounter.cpp
    # Une rencontre posee sur la carte (LOT-118) : la zone, la grille, les places.
    Unit/Core/Combat/test_map_encounter.cpp
    Unit/Core/Combat/test_battle_grid.cpp
    Unit/Core/Combat/test_pathfinding.cpp
    Unit/Core/Combat/test_turn_order.cpp
    Unit/Core/Combat/test_action_economy.cpp
    Unit/Core/Combat/test_combat_state.cpp
    Unit/Core/Combat/test_damage.cpp
    Unit/Core/Combat/test_attack.cpp
    Unit/Core/Combat/test_line_of_sight.cpp
    Unit/Core/Combat/test_area_of_effect.cpp
    Unit/Core/Combat/test_arena.cpp
    Unit/Core/Combat/test_enemy_ai.cpp
    Unit/Core/Combat/test_combat_preview.cpp
    Unit/Core/Combat/test_iso_projection.cpp
    Unit/Core/Combat/test_tactical_terrain.cpp
    Unit/Core/Resources/test_asset_keys.cpp
    Unit/Core/Resources/test_scene_piece_manifest.cpp
    Unit/Core/Resources/test_scene_place.cpp
    Unit/Core/Rpg/test_rpg_enums.cpp
    Unit/Core/Rpg/test_multiclassing.cpp
    Unit/Core/Rpg/test_bestiary.cpp
    Unit/Core/World/test_atlas.cpp
    Unit/Core/World/test_entity_kinds.cpp
    Unit/Core/Rpg/test_character_options.cpp
    Unit/Core/Rpg/test_character_sheet.cpp
    Unit/Core/Levels/test_rpg_terrain.cpp
    Unit/Core/Levels/test_layer_pieces.cpp
    Unit/Core/Levels/test_level_draft_pieces.cpp
    Unit/Core/Levels/test_format_v4.cpp
    Unit/Core/World/test_world_graph.cpp
    Unit/Core/World/test_world_travel.cpp
    Unit/Core/World/test_exploration_session.cpp
    Unit/Core/World/test_entity_presence.cpp
    Unit/Core/World/test_quest_map_features.cpp
    Unit/Core/World/test_city_plan.cpp
    Unit/Core/World/test_city_block.cpp
    Unit/Core/World/test_combat_zone.cpp
    Unit/Core/World/test_exploration_reach.cpp
    Unit/Core/Gameplay/test_interaction.cpp
    Unit/Core/Gameplay/test_quest.cpp
    Unit/Core/Time/test_fixed_timestep.cpp
    Unit/Core/Diagnostics/test_logger.cpp
    Unit/Core/Diagnostics/test_scoped_log_level.cpp
    Unit/Core/Diagnostics/test_sinks.cpp
    Unit/Core/Diagnostics/test_file_log_sink.cpp
    Unit/Core/Diagnostics/test_log_format.cpp
    Unit/Core/Diagnostics/test_assert.cpp
    Unit/Core/Diagnostics/test_log_level_parse.cpp
    Unit/HMI/Input/test_input_state.cpp
    Unit/HMI/Input/test_gamepad_probe.cpp
    Unit/HMI/Input/test_button_repeat.cpp
    Unit/HMI/Graphics/test_camera2d.cpp
    Unit/HMI/Graphics/test_procedural_atlas.cpp
    Unit/HMI/Graphics/test_tile_visuals.cpp
    Unit/HMI/Graphics/test_texture_atlas.cpp
    Unit/HMI/Graphics/test_animation_catalog.cpp
    Unit/HMI/Graphics/test_missing_texture.cpp
    Unit/HMI/Graphics/test_quad_recorder.cpp
    Unit/HMI/Graphics/test_depth_sort.cpp
    Unit/HMI/Graphics/test_render_culling.cpp
    Unit/HMI/Graphics/test_poly_quad.cpp
    Unit/HMI/Graphics/test_maquette_tokens.cpp
    Unit/HMI/Graphics/test_asset_gallery.cpp
    Unit/HMI/Graphics/test_world_scene_composer.cpp
    # Les etages de la scene (LOT-129) : elevation, tri, effacement devant le heros.
    Unit/HMI/Graphics/test_world_storeys.cpp
    Unit/HMI/Graphics/test_static_world_scene.cpp
    # Un kit range en sous-dossiers (LOT-129) : le rendu suit le fichier du manifeste.
    Unit/HMI/Graphics/test_scene_folders.cpp
    Unit/HMI/Graphics/test_cache_registry.cpp
    Unit/HMI/Graphics/test_entity_markers.cpp
    Unit/HMI/Localization/test_localization.cpp
    Unit/HMI/Platform/test_crash_dump.cpp
    Unit/HMI/Presentation/test_credits_catalog.cpp
    Unit/HMI/Presentation/test_inventory_screen.cpp
    Unit/HMI/Presentation/test_quest_journal_screen.cpp
    Unit/HMI/Presentation/test_world_maps.cpp
    Unit/HMI/Interface/test_screen_flow.cpp
    Unit/HMI/Interface/test_rpg_screens.cpp
    Unit/HMI/Interface/test_character_sheet_values.cpp
    Unit/HMI/Interface/test_dialogue_screen.cpp
    Unit/HMI/Interface/test_identity_scale.cpp
    # L'essai complet (LOT-EDITOR-10) : la ligne de commande que l'editeur ecrit, le jeu la relit.
    Unit/HMI/Game/test_launch_options.cpp
    # Les outils de debug : le catalogue suit les sources du jeu ; les cartes d'un dossier.
    Unit/HMI/Game/test_debug_commands.cpp
    Unit/HMI/Game/test_level_scan.cpp
    # Le combat sur la carte (LOT-118) : la file des mouvements ; les mannequins (LOT-145).
    Unit/HMI/Game/test_combat_cues.cpp
    Unit/HMI/Game/test_figure_resolver.cpp
    Unit/Editor/test_thumbnail_geometry.cpp
    Unit/Editor/test_level_name_validation.cpp
    Unit/Editor/test_tile_taxonomy.cpp
    Unit/Editor/test_level_file_operations.cpp
    Unit/Editor/test_editor_status.cpp
    Unit/Editor/test_panel_focus.cpp
    Unit/Editor/test_entity_editing.cpp
    Unit/Editor/test_world_graph_layout.cpp
    Unit/Editor/test_editor_key_bindings.cpp
    Unit/Editor/test_autosave.cpp
    Unit/Editor/test_disk_guard.cpp
    Unit/Editor/test_canvas_picking.cpp
    Unit/Editor/test_canvas_scene.cpp
    Unit/Editor/test_map_format.cpp
    Unit/Editor/test_content_check.cpp
    Unit/Editor/test_quest_map_editor.cpp
    Unit/Editor/test_map_refactor.cpp
    Unit/Editor/test_stamps.cpp
    # Les etages dans l'editeur (LOT-129) : pinceau, visibilite, prefabriques.
    Unit/Editor/test_storey_editing.cpp
    Unit/Editor/test_piece_catalog.cpp
    Unit/Editor/test_level_tree.cpp
    Unit/Editor/test_brush_gesture.cpp
    Unit/Editor/test_paint_tools.cpp
    Unit/Editor/test_editor_sidecar.cpp
    Unit/Editor/test_entity_shapes.cpp
    Unit/Editor/test_gesture_script.cpp
    Unit/Editor/test_shipped_maps.cpp
    # Le monde : onglets, liens du graphe, vue de ville (LOT-EDITOR-09).
    Unit/Editor/test_map_documents.cpp
    Unit/Editor/test_world_links.cpp
    Unit/Editor/test_city_view.cpp
    # L'essai complet dans le jeu (LOT-EDITOR-10) : les brouillons poses dans un dossier a eux.
    Unit/Editor/test_game_launch.cpp
)

# Encodage/decodage d'image : QImage (Qt) lit et ecrit le PNG, mais aucun device
# Direct3D n'est requis -- seul TextureLoader.cpp touche a Qt parmi les sources de UnitTests.
# Ajoute conditionnellement : sur un poste sans Qt, UnitTests doit continuer a se configurer et
# construire (meme garde que BUILD_EDITOR_QT dans Source/HMI/CMakeLists.txt, qui a deja resolu
# Qt6_DIR en cache si l'application a ete traitee avant ce dossier). Qt est toujours installe en
# CI (cf. ci.yml) : le test s'y execute donc systematiquement.
# GuiPrivate en plus de Gui : TextureLoader.cpp cree des
# QRhiTexture, et <rhi/qrhi.h> vit dans les en-tetes prives de QtGui. Le test, lui, ne touche que
# le decodage/encodage d'image -- aucune ressource GPU n'est creee sans QRhi.
find_package(Qt6 QUIET COMPONENTS Gui GuiPrivate)
if(Qt6_FOUND)
    target_sources(UnitTests PRIVATE
        Unit/HMI/Graphics/test_image_encode.cpp
        # Non-regression VISUELLE du pipeline QRhi : rendu hors ecran de motifs temoins, relus
        # pixel par pixel -- une image engendree reste nette, l'art peint se lisse et se
        # premultiplie. Se saute proprement si la machine n'offre aucune interface QRhi.
        Unit/HMI/Graphics/test_rhi_offscreen.cpp
        # La maquette du standard 2D HD (LOT-101), rendue par le moteur a 1080p et 2160p et
        # comparee a la maquette montee a la main (LOT-103).
        Unit/HMI/Graphics/test_hd_mockup_render.cpp
        # Le kit commun de la Capitale (LOT-105) : une rue de douze cases composee de ses seules
        # pieces, rendue par le moteur et ecrite pour l'oeil de l'auteur.
        Unit/HMI/Graphics/test_capital_kit_render.cpp
        # Le lieu qu'on parcourt, rendu hors ecran (LOT-09) : cycle de vie, cadrage qui suit le
        # heros, et une image du Colisee LIVRE -- la capture de reference du lot.
        Unit/HMI/Graphics/test_world_scene_renderer.cpp
        ../HMI/Graphics/WorldSceneRenderer.cpp
        # Le canevas de l'editeur peint le lieu comme le jeu (LOT-EDITOR-02) : Martpart et le Colisee
        # rendus par le GPU du jeu et par le peintre QPainter de l'editeur, compares pixel a pixel.
        Unit/Editor/test_scene_painter.cpp
        ../Editor/Ui/ScenePainter.cpp
        ../Editor/Ui/SceneImages.cpp
        # Le cache d'images du canevas (LOT-125) : partage entre onglets, borne, lu une fois.
        Unit/Editor/test_scene_images.cpp
        # Un ilot de murs du kit sur deux etages, coiffe d'un toit : le heros se voit a travers
        # (LOT-129).
        Unit/Editor/test_storey_render.cpp
        # LevelEditor --render (LOT-EDITOR-13) : une carte livree rendue en PNG, sans fenetre.
        Unit/Editor/test_map_render.cpp
        ../Editor/Ui/MapRender.cpp
        # L'ilot du plan (LOT-96) : son cadrage, et la place du marche LIVREE rendue hors ecran.
        Unit/HMI/Graphics/test_city_block_render.cpp
        ../HMI/Graphics/CityBlockRender.cpp
        # La galerie des assets hors ecran : chargement de ce qui est voulu, liberation du reste.
        Unit/HMI/Graphics/test_asset_gallery_renderer.cpp
        ../HMI/Graphics/AssetGalleryRenderer.cpp
        ../HMI/Graphics/SceneResources.cpp
        ../HMI/Graphics/SpriteRenderer.cpp
        ../HMI/Graphics/TextureAtlas.cpp
        # Table de correspondance Qt <-> Win32 : logique PURE, mais qui lit les
        # constantes de <qnamespace.h> -- elle depend des en-tetes Qt, jamais d'une QApplication.
        # Elle vit donc ici plutot qu'avec les autres sources HMI/Input compilees sans condition.
        Unit/HMI/Input/test_qt_key_map.cpp
        ../HMI/Input/QtKeyMap.cpp
        ../HMI/Graphics/TextureLoader.cpp
        ../HMI/Graphics/SpriteBatch.cpp)
    target_link_libraries(UnitTests PRIVATE Qt6::Gui Qt6::GuiPrivate)
    # Les shaders du pipeline sont embarques dans UnitTests comme dans l'executable : le test de
    # rendu hors ecran construit un vrai hmi::SpriteBatch, qui les charge depuis les ressources.
    qt6_add_shaders(UnitTests "unittests_shaders"
        PREFIX "/shaders"
        BASE "${PROJECT_SOURCE_DIR}/Source/HMI/Graphics/Shaders"
        FILES
            ${PROJECT_SOURCE_DIR}/Source/HMI/Graphics/Shaders/sprite.vert
            ${PROJECT_SOURCE_DIR}/Source/HMI/Graphics/Shaders/sprite.frag
    )
else()
    message(STATUS
        "UnitTests : Qt6 introuvable -- test d'encodage/decodage d'image ignore.")
endif()

# Le moteur audio enveloppe QSoundEffect (Qt Multimedia) : sa dependance a la
# bibliotheque n'est testable qu'avec le module present. Ajoute conditionnellement, meme garde que
# Qt6::Gui/TextureLoader.cpp ci-dessus -- sur un poste sans le module, UnitTests continue de se
# configurer et de construire (testable sans Qt6::Multimedia).
find_package(Qt6 QUIET COMPONENTS Multimedia)
if(Qt6_FOUND AND TARGET Qt6::Multimedia)
    target_sources(UnitTests PRIVATE
        Unit/HMI/Audio/test_audio_engine.cpp
        ../HMI/Audio/AudioEngine.cpp)
    target_link_libraries(UnitTests PRIVATE Qt6::Multimedia)
else()
    message(STATUS
        "UnitTests : Qt6::Multimedia introuvable -- test du moteur audio ignore.")
endif()

target_link_libraries(UnitTests PRIVATE
    Core
    # Couche de présentation sans Qt Widgets (LOT-02) : remplace les 79 chemins `../HMI/…` que
    # cette cible relistait pour les recompiler. HmiLib propage Core, la racine Source/ et
    # nlohmann_json.
    HmiLib
    # La logique pure du module éditeur (LOT-EDITOR-01) : ses tests vivent sous Unit/Editor.
    EditorLogic
    GTest::gtest_main
    project_warnings
    project_options
    # EditorKeyBindings.cpp (ci-dessus) est compile directement dans cette cible et persiste en
    # JSON. Core le propage depuis le LOT-79, mais la dependance reste declaree ici : ce fichier en
    # depend pour son compte.
    nlohmann_json::nlohmann_json
)

# Chemins (source) des assets, pour vérifier que le contenu livré (niveaux, catalogues) se charge.
target_compile_definitions(UnitTests PRIVATE
    # Dossier de fixtures parcouru par les tests parametres (LOT-79) : un catalogue se teste en
    # balayant un dossier, pas en ecrivant un test par entree.
    JADG_TEST_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Fixtures"
    # Racine de donnees d'ESSAI (LOT-123, elargie au LOT-102) : une racine complete, a la forme
    # de Source/Elements -- trois cartes reliees, une planche de lieu, un kit d'arene, des
    # figurines, une ville, un dialogue, une rencontre, leurs textes. Les tests de l'editeur ET du
    # jeu lisaient les cartes et les assets LIVRES ; la table rase du LOT-102 les vide, et ces
    # tests seraient tombes avec eux : ils lisent desormais cette racine, qui ne prouve pas du
    # contenu mais des MECANISMES. Voir Fixtures/GameData/README.md.
    JADG_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Fixtures/GameData"
    # La maquette du LOT-101 installee comme une zone livree, et ses deux images de reference :
    # le rendu HD du LOT-103 se juge contre elles (scripts/assetsGeneration/build_hd_mockup.py ecrit les deux).
    JADG_HD_MOCKUP_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Fixtures/HdMockup"
    JADG_MOCKUP_REFERENCE_DIR="${PROJECT_SOURCE_DIR}/Planning/versions/v0.1.0/v0.0.1-demo/maquettes"
    JADG_LEVELS_DIR="${PROJECT_SOURCE_DIR}/Source/Elements/Levels"
    JADG_LOCALIZATION_DIR="${PROJECT_SOURCE_DIR}/Source/Elements/Localization"
    # Schemas RPG (LOT-32) : le test d'enumerations lit la donnee LIVREE, pas une copie -- une
    # copie divergerait, et le test cesserait de prouver quoi que ce soit exactement quand il
    # compterait (EX-CNT-011).
    JADG_RPG_SCHEMA_DIR="${PROJECT_SOURCE_DIR}/Source/Elements/Rpg/schema"
    # Regles RPG livrees (LOT-43) : le test du multiclassage lit la table
    # d'emplacements REELLE, celle que le jeu chargera.
    JADG_RPG_RULES_DIR="${PROJECT_SOURCE_DIR}/Source/Elements/Rpg/rules"
    # Creatures livrees (LOT-33) : le test rejoue dix blocs contre des valeurs recopiees a la main
    # du PDF. Il lit les 94 fichiers REELS -- une fixture serait une copie, et une copie ne prouve
    # plus rien le jour ou la generation change.
    JADG_RPG_CREATURES_DIR="${PROJECT_SOURCE_DIR}/Source/Elements/Rpg/creatures"
    # Racine des catalogues RPG (LOT-36) : le test des options de personnage lit especes,
    # historiques et classes, ET balaie tout l'arbre pour verifier qu'aucune donnee definitive ne
    # reference une classe provisoire.
    JADG_RPG_DIR="${PROJECT_SOURCE_DIR}/Source/Elements/Rpg"
    # Credits livres (LOT-87, T3.3) : le test lit le fichier REEL que la ressource embarque.
    JADG_CREDITS_FILE="${PROJECT_SOURCE_DIR}/Source/Elements/Credits/credits.json"
    # Atlas livre (LOT-37) : le test des regions lit les 13 regions et 94 lieux REELS, ceux que le
    # jeu chargera. Une fixture serait une copie, et une copie ne prouve plus rien le jour ou
    # l'extraction change.
    JADG_WORLD_DIR="${PROJECT_SOURCE_DIR}/Source/Elements/World"
    # Positions relevees sur les cartes (LOT-94, LOT-95) : le test joint le fichier REEL a l'atlas.
    JADG_WORLD_MAPS="${PROJECT_SOURCE_DIR}/Source/Elements/Maps/world-maps.json"
    JADG_ASSETS_DIR="${PROJECT_SOURCE_DIR}/Source/Elements/Assets"
    # Sources du jeu (LOT-EDITOR-05) : le test du contrat d'extension y releve chaque famille
    # d'entite que le jeu lit, pour exiger qu'elle soit dans core::knownEntityKinds.
    JADG_SOURCE_DIR="${PROJECT_SOURCE_DIR}/Source"
    JADG_FR_LANG_PATH="${PROJECT_SOURCE_DIR}/Source/Elements/Localization/fr.lang"
    JADG_EN_LANG_PATH="${PROJECT_SOURCE_DIR}/Source/Elements/Localization/en.lang")
# La planche de la fiche (LOT-38) se lit desormais sous JADG_ASSETS_DIR : depuis la reprise, sa mise
# en page n'est plus un `.ui` mais la TABLE que l'ecran et le script de gravure partagent
# (Assets/UI/character-sheet-plate.json). Plus de chemin dedie, donc : c'est un asset livre.

# Tests des vues-modeles Qt Quick (LOT-24) : elles lisent leurs catalogues a cote de
# l'executable, comme le jeu ; l'executable de test est donc dans le meme `bin`, et attend que le
# jeu y ait deploye ses donnees.
# Ajoute conditionnellement, meme garde que Qt6::Gui/TextureLoader.cpp plus haut : JadgRuntime
# n'existe pas sous ENABLE_ASAN (job sanitize, sans Qt -- voir Source/CMakeLists.txt,
# JADG_GAME_ENABLED), et un lien vers une cible absente casserait ce build sans rapport avec le code.
if(TARGET JadgRuntime)
    add_executable(RuntimeTests
        # Le combat sur la carte (LOT-118) : la rencontre montee sur la zone, jouee, quittee.
        Unit/HMI/Runtime/test_encounter_model.cpp
    )

    target_link_libraries(RuntimeTests PRIVATE
        JadgRuntime
        GTest::gtest_main
        project_warnings
        project_options
    )
    # Le CONTENU que la vue-modele de l'arene ouvre : la racine d'essai (LOT-123), depuis que la
    # table rase du LOT-102 a emporte les arenes et leurs cartes. Ses catalogues de REGLES, eux,
    # restent ceux que le jeu deploie a cote de l'executable : ils n'ont pas bouge.
    target_compile_definitions(RuntimeTests PRIVATE
        JADG_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Fixtures/GameData")

    # Absent quand JustAnotherRpgGame n'est pas construit (BUILD_EDITOR_QT=OFF) : add_dependencies
    # exige que la cible existe deja, contrairement a target_link_libraries.
    if(TARGET JustAnotherRpgGame)
        add_dependencies(RuntimeTests JustAnotherRpgGame)
    endif()

    # Tests SYSTEME du jeu (LOT-120) : la demo jouee de bout en bout par les modeles du jeu, sans
    # fenetre, de « Nouvelle partie » a chacune de ses fins. La marche du heros est une horloge :
    # l'executable a son propre point d'entree, avec une application Qt sans interface.
    add_executable(SystemGameTests
        Systeme/SystemGameMain.cpp
        Systeme/test_demo_de_bout_en_bout.cpp
    )
    target_link_libraries(SystemGameTests PRIVATE
        JadgRuntime
        GTest::gtest
        project_warnings
        project_options
    )
    # Le contenu LIVRE : cartes de principe, plan de ville, quete, dialogues, rencontre. Les
    # regles et le heros restent lus a cote de l'executable, comme le jeu les lit.
    target_compile_definitions(SystemGameTests PRIVATE
        JADG_ELEMENTS_DIR="${PROJECT_SOURCE_DIR}/Source/Elements")
    if(TARGET JustAnotherRpgGame)
        add_dependencies(SystemGameTests JustAnotherRpgGame)
    endif()
    # La decouverte lance l'executable : une application Qt et ses DLL mettent plus que les cinq
    # secondes par defaut a demarrer sur un runner.
    gtest_discover_tests(SystemGameTests DISCOVERY_MODE PRE_TEST DISCOVERY_TIMEOUT 60
                         PROPERTIES LABELS systeme TIMEOUT 900)
endif()

# Tests Qt Quick de Jadg.Ui (refonte de la chaîne d'outillage, phase 4) : chaque formulaire et chaque
# brique se construit sans avertissement, les briques se comportent comme la galerie le suppose, et
# chaque écran ressemble à sa capture de référence (Qml/References). Détail : Qml/QmlTestSetup.cpp.
#
# Même garde que RuntimeTests : Jadg.Ui n'existe pas sans Qt (job sanitize). QuickTest fait partie de
# qtdeclarative, installé partout où Jadg.Ui se construit.
if(TARGET JadgUi)
    find_package(Qt6 QUIET COMPONENTS QuickTest QuickControls2)
endif()
if(TARGET JadgUi AND TARGET Qt6::QuickTest)
    qt_add_executable(QmlTests Qml/QmlTestSetup.cpp)
    set_target_properties(QmlTests PROPERTIES
        AUTOMOC ON
        # HORS de bin/ : windeployqt y dépose les DLL et les modules QML qu'utilise le JEU, sans
        # QtTest. Depuis son propre dossier, l'exécutable charge tout Qt depuis l'installation
        # (PATH ci-dessous), et ne court pas après le déploiement de l'application.
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qmltests)
    target_link_libraries(QmlTests PRIVATE
        JadgUi
        JadgUiplugin
        Qt6::QuickTest
        Qt6::QuickControls2
        Qt6::Quick
        Qt6::Qml
        project_warnings
        project_options
    )
    target_compile_definitions(QmlTests PRIVATE
        # Les tst_*.qml sont lus dans les sources : en ajouter un ne demande pas de reconstruire.
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Qml"
        JADG_QML_REFERENCES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Qml/References"
        JADG_QML_CAPTURES_DIR="${CMAKE_BINARY_DIR}/qml-captures"
        JADG_FONTS_DIR="${PROJECT_SOURCE_DIR}/Source/Elements/Assets/Fonts")
    # `offscreen` : aucune fenêtre visible, sur le poste comme sur le runner.
    add_test(NAME QmlTests COMMAND QmlTests -platform offscreen)
    set_tests_properties(QmlTests PROPERTIES
        LABELS unitaire
        TIMEOUT 300
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
endif()

# Tests d'intégration : plusieurs briques assemblées collaborant de bout en bout.
add_executable(IntegrationTests
    Support/CrtReports.cpp
    Integration/test_exploration_carte.cpp
    Integration/test_quete_trois_etapes.cpp
    Integration/test_quete_des_pommes.cpp
)

target_link_libraries(IntegrationTests PRIVATE
    HmiLib
    GTest::gtest_main
    project_warnings
    project_options
)

# Racine des données du test d'intégration « carte → scène » : la racine d'essai de l'éditeur
# (LOT-123). Il ouvrait les cartes LIVRÉES, que la table rase du LOT-102 emporte.
target_compile_definitions(IntegrationTests PRIVATE
    JADG_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Fixtures/GameData"
    # L'arborescence par niveaux (LOT-124) : une carte d'Arenarea qui cite une piece de chacun des
    # quatre niveaux, jouee comme le jeu la joue. Voir Fixtures/LevelTree/README.md.
    JADG_LEVEL_TREE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Fixtures/LevelTree"
    # Le contenu LIVRÉ, pour la quête de la démo (LOT-120) : ses cartes de principe, sa quête, ses
    # dialogues et sa rencontre se jouent sans fenêtre, tels que le jeu les joue.
    JADG_ELEMENTS_DIR="${PROJECT_SOURCE_DIR}/Source/Elements")

# Tests système : un parcours de jeu complet sur le contenu livré, sans la couche GPU.
add_executable(SystemTests
    Systeme/test_parcours_edition_rpg.cpp
)

target_link_libraries(SystemTests PRIVATE
    Core
    GTest::gtest_main
    project_warnings
    project_options
)

# Racine des données du parcours : la racine d'essai de l'éditeur (LOT-123). Le parcours validait
# ses entités contre les catalogues LIVRÉS, que la table rase du LOT-102 emporte.
target_compile_definitions(SystemTests PRIVATE
    JADG_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Fixtures/GameData")

# Enregistre chaque TEST() aupres de CTest.
#
# LABELS : l'etage d'appartenance, pour pouvoir ne rejouer qu'une famille --
# `ctest --preset vs -L unitaire` pendant une boucle de developpement, les trois en CI.
# Les proprietes de `gtest_discover_tests` s'appliquent a TOUTE la cible : un etiquetage plus fin
# (les quelques tests d'entrainement IA qui pesent des dizaines de secondes) se fait par nom, ces
# tests etant tous nommes d'apres leur sujet -- p. ex.
# `ctest --preset vs -E 'Trainer|Comparator|CrossLevel|NonRegression'`.
#
# TIMEOUT : une borne de TERMINAISON, pas un objectif de performance. Sans elle, un test parti en
# boucle bloque le job jusqu'au delai global du runner, sans dire lequel. Le seuil laisse une marge
# large au test le plus lourd connu (~42 s en Debug) : il n'attrape qu'un veritable emballement.
#
# DISCOVERY_MODE PRE_TEST : la decouverte a lieu au moment du `ctest`, pas a la fin du build.
#
# En mode POST_BUILD (le defaut), CMake execute l'executable de test des qu'il est lie, pour en
# lister les TEST(). Sous Ninja, qui parallelise agressivement, cette execution tombe dans la
# meme fenetre que le `windeployqt` de l'application : les deux visent `bin/`, l'un ecrit les DLL
# Qt pendant que l'autre les charge. Resultat, un STATUS_SHARING_VIOLATION (0xc0000043) au
# lancement -- « Error running test executable », sans le moindre rapport avec le code teste.
#
# La course preexistait ; le LOT-02 l'a rendue visible en decalant l'ordre d'edition de liens
# (UnitTests attend desormais HmiLib). Un simple reordonnancement ne ferait que la recacher :
# PRE_TEST la supprime, la decouverte n'ayant plus lieu pendant le build. Le generateur Visual
# Studio n'etait pas touche, ses cibles ne se recouvrant pas de la meme facon -- exactement le
# genre d'ecart entre generateurs que le job `build-ninja` existe pour attraper.
gtest_discover_tests(UnitTests DISCOVERY_MODE PRE_TEST PROPERTIES LABELS unitaire TIMEOUT 300)
if(TARGET RuntimeTests)
    gtest_discover_tests(RuntimeTests DISCOVERY_MODE PRE_TEST PROPERTIES LABELS unitaire TIMEOUT 300)
endif()
gtest_discover_tests(IntegrationTests DISCOVERY_MODE PRE_TEST
                     PROPERTIES LABELS integration TIMEOUT 300)
gtest_discover_tests(SystemTests DISCOVERY_MODE PRE_TEST PROPERTIES LABELS systeme TIMEOUT 900)
