# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

# Bibliothèque Core : logique et moteur du jeu.
add_library(Core STATIC
    Core.cpp
    Data/JsonDocument.cpp
    Math/Vector2.cpp
    Math/Rect.cpp
    Ecs/EntityManager.cpp
    Ecs/World.cpp
    Ecs/AnimationClip.cpp
    World/Atlas.cpp
    World/CityBlock.cpp
    World/CityPlan.cpp
    World/EntityKinds.cpp
    World/WorldGraph.cpp
    World/WorldTravel.cpp
    World/ExplorationSession.cpp
    World/ExplorationReach.cpp
    World/CombatZone.cpp
    World/EntityPresence.cpp
    Gameplay/Interaction.cpp
    Gameplay/MapEntitySpawner.cpp
    Gameplay/WorldFlags.cpp
    Gameplay/FlagCondition.cpp
    Gameplay/Quest.cpp
    Rpg/Ability.cpp
    Rpg/Bestiary.cpp
    Rpg/CharacterOptions.cpp
    Rpg/CharacterSheet.cpp
    Rpg/Skill.cpp
    Rpg/Check.cpp
    Rpg/Dialogue.cpp
    Rpg/Dice.cpp
    Rpg/Equipment.cpp
    Rpg/Inventory.cpp
    Combat/Encounter.cpp
    Resources/AssetKey.cpp
    Resources/AssetMarker.cpp
    # Le manifeste des pièces d'un lieu (LOT-EDITOR-02, constat A9) : lu ici, sans Qt ni GPU.
    Resources/ScenePieceManifest.cpp
    # Les niveaux d'un lieu, du plus propre au monde (LOT-124).
    Resources/ScenePlace.cpp
    Combat/CombatTransition.cpp
    Combat/BattleGrid.cpp
    Combat/Pathfinding.cpp
    Combat/TurnOrder.cpp
    Combat/ActionEconomy.cpp
    Combat/CombatCounters.cpp
    Combat/CombatState.cpp
    Combat/Damage.cpp
    Combat/Attack.cpp
    Combat/LineOfSight.cpp
    Combat/AreaOfEffect.cpp
    Combat/Arena.cpp
    Combat/Flanking.cpp
    Combat/EnemyAi.cpp
    Combat/CombatPreview.cpp
    Combat/IsoProjection.cpp
    Combat/TacticalTerrain.cpp
    Rpg/RpgEnumNames.cpp
    Rpg/Multiclassing.cpp
    Levels/TileMap.cpp
    Levels/TileLayer.cpp
    Levels/MapEntity.cpp
    Levels/CollisionDerivation.cpp
    Levels/LevelVariant.cpp
    Levels/TileTypeName.cpp
    Levels/LevelLoader.cpp
    Levels/LevelWriter.cpp
    Levels/LevelDraft.cpp
    Time/FixedTimestep.cpp
    Diagnostics/LogFormat.cpp
    Diagnostics/Logger.cpp
    Diagnostics/ConsoleLogSink.cpp
    Diagnostics/FileLogSink.cpp
    Diagnostics/Assert.cpp
)

# Racine d'inclusion = Source/ : les en-têtes s'incluent par chemin complet
# depuis la racine (ex. "Core/Core.h"), jamais par nom seul.
target_include_directories(Core PUBLIC ${PROJECT_SOURCE_DIR}/Source)

# Numéro de version : injecté depuis `project(... VERSION ...)`, seul endroit où il est écrit.
# PRIVATE suffit — Core.cpp est l'unique consommateur, et un en-tête généré obligerait tous les
# dépendants à connaître un répertoire d'inclusion supplémentaire pour une seule chaîne.
target_compile_definitions(Core PRIVATE JADG_VERSION="${PROJECT_VERSION}")

# `nlohmann_json` est PUBLIC depuis le LOT-79 : `Core/Data/JsonDocument.h` expose l'arbre parse a
# ses appelants, donc la bibliotheque fait partie de l'interface de Core et non de son
# implementation. C'est le prix -- assume -- d'une brique de lecture unique : la seule facon de
# l'eviter serait de reecrire une facade typee par-dessus nlohmann, soit un second modele d'arbre
# a maintenir pour ne rien gagner d'observable.
target_link_libraries(Core PUBLIC nlohmann_json::nlohmann_json)
target_link_libraries(Core PRIVATE project_warnings project_options)

if(ENABLE_PCH)
    target_precompile_headers(Core PRIVATE ${PROJECT_SOURCE_DIR}/Source/pch.h)
endif()
