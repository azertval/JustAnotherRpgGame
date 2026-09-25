# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

# Jadg.Ui : les écrans du jeu, en QML déclaratif -- le territoire de la conception (LOT-87).
#
# Ce module ne contient PAS UNE LIGNE DE C++, et ce n'est pas un état de fait mais sa raison d'être.
# Qt Design Studio dessine avec son propre Qt (6.8.7, embarqué) et un marionnettiste (`qmlpuppet`)
# qui ne charge aucun plugin C++ du projet. Un module qui mêle formulaires et types C++ est donc
# résolvable par le jeu et pas par l'atelier ; un module de QML pur l'est par les deux, tel quel.
#
# Ce fichier vit DANS Source/Ui, à côté des fichiers qu'il liste. `qt_add_qml_module` calcule le
# chemin de ressource de chaque fichier relativement à ce répertoire : listé d'ailleurs, chaque
# chemin aurait dû être réécrit un par un (par un alias de ressource), et c'est cette plomberie qui
# a coûté 125 commits à la branche abandonnée. Aucun alias ici, ni nulle part.
#
# DESIGNER_SUPPORTED : le mot que Qt Design Studio exige dans le `qmldir` pour traiter un module
# comme une bibliothèque de conception. Nécessaire, et pas suffisant : la bibliothèque de
# composants de l'atelier reste « (vide) » avec ou sans lui (constat de la phase 1 du LOT-87, deux
# dispositions essayées) ; les briques se posent par la galerie DesignStudio/Main.ui.qml et par le
# code, en attendant que la phase 2 instruise ce point avec ses nouvelles briques.

# Contrôles : les briques que la conception réutilise d'un écran à l'autre. Écrans : un FORMULAIRE
# `.ui.qml` par écran ; son jumeau de câblage `.qml` vit dans Source/App/Game/Qml/Screens/, module
# `Jadg.App`. Ajouter un écran = ajouter son formulaire ici et son jumeau là-bas.
set(JADG_UI_QML_FILES
    Theme/Tokens.qml
    Controls/DiagnosticsOverlay.ui.qml
    # Charte v2 (LOT-87, T2.7) : les briques que la phase 3 transcrit dans les écrans, et les deux
    # porteurs d'image qu'elles partagent. `Theme/Artwork.qml` dit quelles pièces sont livrées.
    Theme/Artwork.qml
    Controls/NinePatchArt.ui.qml
    Controls/FixedArt.ui.qml
    Controls/PanelFrame.ui.qml
    Controls/TitlePlate.ui.qml
    Controls/SectionBanner.ui.qml
    Controls/OrnateButton.ui.qml
    Controls/OrnateTab.ui.qml
    Controls/OrnateCheck.ui.qml
    Controls/OrnateSlider.ui.qml
    Controls/OrnateScrollBar.ui.qml
    Controls/OrnateCombo.ui.qml
    Controls/StatMedallion.ui.qml
    Controls/PortraitFrame.ui.qml
    Controls/ItemSlot.ui.qml
    Controls/Gauge.ui.qml
    Controls/GoldDivider.ui.qml
    # Phase 3 (T3.1) : les pièces du menu principal qui n'avaient pas encore de brique.
    Controls/CoverArt.ui.qml
    Controls/LogoPlate.ui.qml
    Controls/QuotePlate.ui.qml
    Controls/FocusMark.ui.qml
    Controls/CreditSection.ui.qml
    Controls/FieldRow.ui.qml
    Controls/SkillRow.ui.qml
    Controls/MapMarker.ui.qml
    Controls/MapCanvas.ui.qml
    Controls/MapHud.ui.qml
    Controls/MapSidePanel.ui.qml
    Controls/LedgerList.ui.qml
    Controls/SpellEntry.ui.qml
    Controls/ScreenPage.ui.qml
    Controls/ActionSlot.ui.qml
    Controls/OrnateRoundButton.ui.qml
    Controls/HudFrame.ui.qml
    # Le Colisee (LOT-50) : la marque de ciblage. La scene elle-meme (LOT-86 Phase 7) est rendue par QRhi.
    Controls/ArenaMark.ui.qml
    # Le calque tactique (LOT-118) : cases, curseur, chemin et combattants, au Colisee comme sur la carte.
    Controls/TacticalLayer.ui.qml
    Screens/CharacterSheetForm.ui.qml
    Screens/InventoryForm.ui.qml
    Screens/JournalForm.ui.qml
    Screens/WorldMapForm.ui.qml
    Screens/RegionMapForm.ui.qml
    Screens/CityMapForm.ui.qml
    # Sous la ville (LOT-96) : le quartier et ses ilots, puis l'ilot.
    Screens/DistrictMapForm.ui.qml
    Screens/BlockMapForm.ui.qml
    Screens/DialogueForm.ui.qml
    Screens/MerchantForm.ui.qml
    Screens/CombatHudForm.ui.qml
    Screens/CompanyForm.ui.qml
    Screens/SkillsForm.ui.qml
    Screens/MainMenuForm.ui.qml
    Screens/OptionsForm.ui.qml
    Screens/PauseForm.ui.qml
    Screens/CreditsForm.ui.qml
    Screens/GameViewForm.ui.qml
    # Les ecrans de fin (LOT-119) : la mort, et la fin de la demo.
    Screens/DeathForm.ui.qml
    Screens/DemoEndForm.ui.qml
    # La galerie de l'atelier : ce que Qt Design Studio ouvre en premier (mainFile du .qmlproject).
    DesignStudio/Main.ui.qml
)

# Les singletons doivent être DÉCLARÉS : CMake ne déduit pas `pragma Singleton` du contenu du
# fichier. Non déclaré, le type se charge quand même, mais chaque `import` en construirait une
# instance neuve, et le facteur d'agrandissement posé par la fenêtre ne serait vu par aucun écran.
set(JADG_UI_QML_SINGLETONS
    Theme/Tokens.qml
    Theme/Artwork.qml
)
set_source_files_properties(${JADG_UI_QML_SINGLETONS} PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# Pas un seul Q_OBJECT ici, mais qt_add_qml_module extrait les métatypes de toute cible de module,
# et cette extraction exige AUTOMOC : sans lui, la configuration échoue sur « Metatype generation
# requires ... AUTOMOC », pour un module qui n'a rien à mocquer.
set(CMAKE_AUTOMOC ON)

qt_add_qml_module(JadgUi
    URI Jadg.Ui
    VERSION 1.0
    STATIC
    DESIGNER_SUPPORTED
    QML_FILES ${JADG_UI_QML_FILES}
)

# Illustrations PRODUITES de la charte v2 (LOT-87, EX-IHM-076), que les briques désignent par un
# chemin RELATIF À LEUR SOURCE (`../../Elements/Assets/UI/<famille>/<pièce>.png`). Le même chemin
# doit fonctionner dans l'atelier (qui lit Source/), depuis les sources (JADG_QML_FROM_SOURCE=1) et
# depuis la ressource : l'image est donc embarquée à l'endroit exact où ce chemin relatif la
# cherche, c'est-à-dire `/qt/qml/Jadg/Elements/Assets/UI/`. `BASE` est le mécanisme prévu par
# qt_add_resources pour cela -- pas un alias par fichier.
#
# Rangées par famille sous `UI/<famille>/` par scripts/assetsGeneration/receive_ui_assets.py, elles sont ramassées
# par motif et non listées : la liste qui fait foi est le manifeste (`illustrations.json`), et
# `check_ui_assets.py` refuse déjà tout fichier qu'il ne déclare pas -- une seconde liste ici serait
# une troisième copie à tenir. `CONFIGURE_DEPENDS` : une livraison est vue à la construction
# suivante, sans reconfigurer. Aucune image extraite du corpus n'est plus embarquée (LOT-94).
file(GLOB_RECURSE _jadg_produced_art CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/Source/Elements/Assets/UI/*/*.png")
if(_jadg_produced_art)
    qt_add_resources(JadgUi "JadgUiIllustrations"
        PREFIX "/qt/qml/Jadg/Elements/Assets/UI"
        BASE "${PROJECT_SOURCE_DIR}/Source/Elements/Assets/UI"
        FILES ${_jadg_produced_art}
    )
endif()

# Les cartes de l'ecran « Carte » (LOT-94, LOT-95) : le monde, les treize regions et les plans de
# ville, PEINTS PAR L'AUTEUR (aucune image du corpus), sous Source/Elements/Assets/Maps/. Embarquees
# par le meme mecanisme, a l'endroit ou `../../Elements/Assets/Maps/` les cherche depuis
# Source/Ui/Controls/. Ramassees par motif : la liste qui fait foi est leur manifeste
# (`Maps/manifest.json`), que `check_map_assets.py` recoupe avec le dossier.
file(GLOB _jadg_map_art CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/Source/Elements/Assets/Maps/*.jpg")
qt_add_resources(JadgUi "JadgUiMaps"
    PREFIX "/qt/qml/Jadg/Elements/Assets/Maps"
    BASE "${PROJECT_SOURCE_DIR}/Source/Elements/Assets/Maps"
    FILES ${_jadg_map_art}
)

# Les cartes RENDUES des zones de l'onglet « Carte » (LOT-121) : `LevelEditor --render --canvas`,
# rangees avec leur zone (`Regions/<...>/Map/*.jpg`, dans son kit) et nommees par world-maps.json
# d'un chemin relatif a Assets/. Embarquees de meme, la ou `../../Elements/Assets/Regions/` les
# cherche.
file(GLOB_RECURSE _jadg_zone_maps CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/Source/Elements/Assets/Regions/*.jpg")
list(FILTER _jadg_zone_maps INCLUDE REGEX "/Map/[^/]+\\.jpg$")
if(_jadg_zone_maps)
    qt_add_resources(JadgUi "JadgUiZoneMaps"
        PREFIX "/qt/qml/Jadg/Elements/Assets/Regions"
        BASE "${PROJECT_SOURCE_DIR}/Source/Elements/Assets/Regions"
        FILES ${_jadg_zone_maps}
    )
endif()

# La table rase du LOT-102 a emporte les pieces du Colisee, et avec elles la ressource
# « JadgUiColiseum » qui les embarquait : plus aucun QML ne les lisait depuis que le rendu de
# l'arene passe par QRhi (LOT-86, phase 7), qui charge ses pieces depuis le disque. Le kit d'une
# arene HD reviendra par la nouvelle arborescence (LOT-106), pas par une ressource Qt.

# --- Éditer un écran sans rien reconstruire (EX-IHM-100) --------------------------------------
#
# La ressource embarque les .qml : la moindre retouche de mise en page demanderait sinon de
# repasser par le build. Un second `qmldir` -- engendré depuis LA MÊME liste que la ressource,
# jamais tenu à la main -- désigne les fichiers SOURCES. Lancer le jeu avec JADG_QML_FROM_SOURCE=1
# place Source/Ui en tête des chemins d'import : le programme lit alors ces fichiers directement.
#
# Écrit SOUS Source/Ui (ignoré par git), pas dans le répertoire de construction : c'est ce qui
# permet au `.qmlproject` de l'artiste de déclarer `importPaths: ["."]` et de résoudre `Jadg.Ui`
# sans connaître le build. `designersupported` y figure pour la même raison que ci-dessus.
set(JADG_QML_DEV_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(_dev_module_dir ${CMAKE_CURRENT_SOURCE_DIR}/Jadg/Ui)
file(MAKE_DIRECTORY ${_dev_module_dir})
set(_dev_qmldir "module Jadg.Ui
designersupported
")
foreach(_qml ${JADG_UI_QML_FILES})
    get_filename_component(_qml_type ${_qml} NAME_WE)
    file(RELATIVE_PATH _qml_rel ${_dev_module_dir} ${CMAKE_CURRENT_SOURCE_DIR}/${_qml})
    if(${_qml} IN_LIST JADG_UI_QML_SINGLETONS)
        string(APPEND _dev_qmldir "singleton ${_qml_type} 1.0 ${_qml_rel}
")
    else()
        string(APPEND _dev_qmldir "${_qml_type} 1.0 ${_qml_rel}
")
    endif()
endforeach()
file(WRITE ${_dev_module_dir}/qmldir "${_dev_qmldir}")
