// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_entity_editing.cpp
 * @brief Tests unitaires de la logique d'édition des couches et des entités de l'éditeur
 *        (`LOT-11`) : lignes du panneau « Couches », réglages d'affichage, geste de l'outil
 *        « Entité », références proposées et avertissements.
 */

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/Arena.h"
#include "Core/Combat/TacticalTerrain.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileLayer.h"
#include "Core/World/EntityKinds.h"
#include "Editor/Logic/EditorDiagnostics.h"
#include "Editor/Logic/EntityGesture.h"
#include "Editor/Logic/EntityReferences.h"
#include "Editor/Logic/LayerView.h"

namespace {

constexpr const char* MAP = R"({
  "version": 3,
  "name": "Village",
  "width": 5,
  "height": 4,
  "tiles": [
    { "x": 0, "y": 0, "type": "entry" }
  ],
  "layers": [
    { "name": "sol", "kind": "ground", "tiles": [] },
    { "name": "arbres", "kind": "decor", "tiles": [] }
  ],
  "entities": [
    { "type": "chest", "x": 2, "y": 1 },
    { "type": "sign", "x": 2, "y": 1 }
  ]
})";

[[nodiscard]] core::LevelDraft draft() {
    const core::LevelLoadResult loaded = core::LevelLoader::loadFromString(MAP);
    EXPECT_TRUE(loaded.ok()) << loaded.error;
    return core::LevelDraft::fromLevel(*loaded.level);
}

[[nodiscard]] core::TileLayer layer(std::string name, core::LayerKind kind) {
    return core::TileLayer{
        .name = std::move(name), .kind = kind, .tiles = core::TileMap(2, 2), .properties = {}};
}

// La racine d'essai de l'editeur (LOT-123) : ce test lisait les catalogues et les cartes
// LIVRES, que la table rase du LOT-102 emporte. Voir Fixtures/GameData/README.md.
[[nodiscard]] std::filesystem::path elementsRoot() {
    return std::filesystem::path{JADG_TEST_DATA_DIR};
}

}  // namespace

/**
 * @brief La grille racine a une ligne, en tête, dont le rôle dit si elle est l'image ou la
 * collision ; les couches visuelles suivent dans l'ordre de dessin.
 * \castest{<b>Les lignes du panneau Couches suivent l'ordre de dessin.</b><br/>
 * \tcat Unitaire · Edition de couches<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Construire les lignes d'une carte sans couche visuelle.<br/>2. Puis d'une carte a
 * collision, sol et decor.<br/>
 * \tattendu Une ligne Legacy seule ; puis Collision, sol (rang 1), decor (rang 2).
 * }
 */
TEST(EditionEntitesTest, LignesDuPanneauCouchesSuiventLOrdreDeDessin) {
    EXPECT_EQ(hmi::layerRows({layer({}, core::LayerKind::Legacy)}),
              (std::vector<hmi::LayerRow>{
                  {.slot = std::nullopt, .kind = core::LayerKind::Legacy, .name = {}}}));

    const std::vector<core::TileLayer> layers = {layer({}, core::LayerKind::Collision),
                                                 layer("sol", core::LayerKind::Ground),
                                                 layer("arbres", core::LayerKind::Decor)};
    EXPECT_EQ(hmi::layerRows(layers),
              (std::vector<hmi::LayerRow>{
                  {.slot = std::nullopt, .kind = core::LayerKind::Collision, .name = {}},
                  {.slot = 1, .kind = core::LayerKind::Ground, .name = "sol"},
                  {.slot = 2, .kind = core::LayerKind::Decor, .name = "arbres"}}));
}

/**
 * @brief Une couche active qui n'existe plus, ou n'est plus visuelle, retombe sur la grille racine.
 * \castest{<b>La couche active invalide retombe sur la collision.</b><br/>
 * \tcat Unitaire · Edition de couches<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Valider le rang 2, puis 5, puis 0 (collision) sur trois couches.<br/>
 * \tattendu 2 est garde ; 5 et 0 retombent sur la grille racine.
 * }
 */
TEST(EditionEntitesTest, CoucheActiveInvalideRetombeSurLaRacine) {
    const std::vector<core::TileLayer> layers = {layer({}, core::LayerKind::Collision),
                                                 layer("sol", core::LayerKind::Ground),
                                                 layer("arbres", core::LayerKind::Decor)};
    EXPECT_EQ(hmi::validActiveLayer(layers, 2), hmi::LayerSlot{2});
    EXPECT_EQ(hmi::validActiveLayer(layers, 5), hmi::LayerSlot{});
    EXPECT_EQ(hmi::validActiveLayer(layers, 0), hmi::LayerSlot{});
}

/**
 * @brief La collision se montre à demi par-dessus des couches visuelles, pleine sur une carte à
 * grille unique ; les réglages se bornent et suivent un déplacement.
 * \castest{<b>Les reglages d'affichage des couches.</b><br/>
 * \tcat Unitaire · Edition de couches<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Lire l'opacite de la racine avec et sans couche visuelle.<br/>2. Regler une opacite
 * hors bornes et une non finie.<br/>3. Masquer le rang 1 puis l'echanger avec le rang 2.<br/>
 * \tattendu 0,55 puis 1 ; opacite bornee a 1, la non finie ignoree ; le masquage suit la couche.
 * }
 */
TEST(EditionEntitesTest, ReglagesDAffichageDesCouches) {
    hmi::LayerViewState view;
    view.sync(3);
    EXPECT_FLOAT_EQ(view.display(std::nullopt, true).opacity,
                    hmi::DEFAULT_COLLISION_OVERLAY_OPACITY);
    EXPECT_FLOAT_EQ(view.display(std::nullopt, false).opacity, 1.0F);

    view.setOpacity(1, 3.0F);
    EXPECT_FLOAT_EQ(view.display(1, true).opacity, 1.0F);
    view.setOpacity(1, 0.25F);
    view.setOpacity(1, std::numeric_limits<float>::quiet_NaN());
    EXPECT_FLOAT_EQ(view.display(1, true).opacity, 0.25F);

    view.setVisible(1, false);
    view.swap(1, 2);
    EXPECT_TRUE(view.display(1, true).visible);
    EXPECT_FALSE(view.display(2, true).visible);
    EXPECT_FLOAT_EQ(view.display(2, true).opacity, 0.25F);

    view.reset();
    EXPECT_EQ(view.display(2, true), hmi::LayerDisplay{});
}

/**
 * @brief L'appui de l'outil Entité : une case occupée prend l'entité du dessus, une case libre
 * pose, Ctrl pose sur une case occupée, Maj bascule dans la sélection, et sans famille une case
 * libre désélectionne.
 * \castest{<b>Le geste de l'outil Entite.</b><br/>
 * \tcat Unitaire · Edition d'entites<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Appuyer sur la case du coffre et du panneau, avec et sans Ctrl, avec Maj.<br/>2.
 * Appuyer sur une case libre, avec et sans famille choisie.<br/>3. Appuyer hors de la grille.<br/>
 * \tattendu Prise du panneau (rang 1) ; pose avec Ctrl ; bascule avec Maj ; pose ; deselection ;
 * rien.
 * }
 */
TEST(EditionEntitesTest, GesteDeLOutilEntite) {
    const core::LevelDraft map = draft();
    const core::GridPosition shared{.column = 2, .row = 1};
    const core::GridPosition libre{.column = 0, .row = 3};
    const std::vector<std::size_t> none;

    EXPECT_EQ(hmi::resolveEntityPress(map, shared, none, "npc", {}),
              (hmi::EntityGestureDecision{.action = hmi::EntityGestureAction::Grab,
                                          .entityIndex = 1,
                                          .cell = shared,
                                          .handle = std::nullopt}));
    EXPECT_EQ(
        hmi::resolveEntityPress(map, shared, none, "npc", {.force = true, .toggle = false}).action,
        hmi::EntityGestureAction::Place);
    EXPECT_EQ(hmi::resolveEntityPress(map, shared, none, "", {.force = false, .toggle = true}),
              (hmi::EntityGestureDecision{.action = hmi::EntityGestureAction::Toggle,
                                          .entityIndex = 1,
                                          .cell = shared,
                                          .handle = std::nullopt}));
    EXPECT_EQ(hmi::resolveEntityPress(map, libre, none, "npc", {}),
              (hmi::EntityGestureDecision{.action = hmi::EntityGestureAction::Place,
                                          .entityIndex = 0,
                                          .cell = libre,
                                          .handle = std::nullopt}));
    EXPECT_EQ(hmi::resolveEntityPress(map, libre, none, "", {}).action,
              hmi::EntityGestureAction::Deselect);
    EXPECT_EQ(hmi::resolveEntityPress(map, {.column = 9, .row = 9}, none, "npc", {}).action,
              hmi::EntityGestureAction::Ignore);
}

/**
 * @brief Un glisser déplace l'entité prise seulement si la case a changé, et un groupe bouge entier
 * ou pas du tout.
 * \castest{<b>Un glisser deplace l'entite saisie, ou le groupe.</b><br/>
 * \tcat Unitaire · Edition d'entites<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Glisser le coffre d'une case, puis sur place.<br/>2. Glisser le coffre et le
 * panneau ensemble, puis hors de la carte.<br/>
 * \tattendu Deplacement ; rien ; deux entites deplacees ; refus, rien ne bouge.
 * }
 */
TEST(EditionEntitesTest, GlisserDeplaceLEntiteSaisie) {
    const core::LevelDraft map = draft();
    const core::GridPosition from{.column = 2, .row = 1};
    const core::GridPosition to{.column = 3, .row = 1};
    const hmi::EntityDrag one{.mode = hmi::EntityDrag::Mode::Move,
                              .indices = {0},
                              .handle = std::nullopt,
                              .kind = {},
                              .from = from};
    const hmi::EntityDragResult moved = hmi::dragEntities(one, map.entities(), to, 5, 4);
    ASSERT_EQ(moved.replaced.size(), 1U);
    EXPECT_EQ(moved.replaced[0].first, 0U);
    EXPECT_EQ(moved.replaced[0].second.position, to);
    EXPECT_TRUE(hmi::dragEntities(one, map.entities(), from, 5, 4).empty());

    hmi::EntityDrag both = one;
    both.indices = {0, 1};
    EXPECT_EQ(hmi::dragEntities(both, map.entities(), to, 5, 4).replaced.size(), 2U);
    const hmi::EntityDragResult refused =
        hmi::dragEntities(both, map.entities(), {.column = 2, .row = 3}, 5, 3);
    EXPECT_TRUE(refused.refused);
    EXPECT_TRUE(refused.replaced.empty());
}

/**
 * @brief Les choix proposés : liste fixe, catalogues, cartes, et points d'arrivée de la carte
 * nommée par l'entité — ceux du brouillon pour la carte éditée.
 * \castest{<b>Les choix proposes par le panneau Entites.</b><br/>
 * \tcat Unitaire · Edition d'entites<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Construire un contexte : une carte « foret » au point « lisiere », la carte editee
 * « village » dont le brouillon pose le point « puits ».<br/>2. Demander les choix du camp d'une
 * entree d'arene, des cartes, et des points d'arrivee vers le village puis sans cible.<br/>
 * \tattendu allies/enemies ; foret et village ; puits ; aucun.
 * }
 */
TEST(EditionEntitesTest, ChoixProposesParLePanneau) {
    hmi::EditorReferences references;
    references.world = core::buildWorldGraph(
        {core::WorldMapInput{
             .mapId = "foret",
             .name = "Forêt",
             .entities = {core::MapEntity{.type = "spawnPoint",
                                          .position = {},
                                          .properties = {{"name", std::string{"lisiere"}}}}},
             .loadError = {}},
         core::WorldMapInput{
             .mapId = "village", .name = "Village", .entities = {}, .loadError = {}}});
    const std::vector<core::MapEntity> edited = {core::MapEntity{
        .type = "spawnPoint", .position = {}, .properties = {{"name", std::string{"puits"}}}}};
    const core::EntityReferenceContext context =
        hmi::referenceContext(references, "village", edited);

    const core::EntityKind* const arena = core::findEntityKind(core::ARENA_ENTRY_ENTITY_TYPE);
    const core::EntityKind* const portal = core::findEntityKind(core::PORTAL_ENTITY_TYPE);
    ASSERT_NE(arena, nullptr);
    ASSERT_NE(portal, nullptr);
    core::MapEntity gate = core::makeEntity(*portal, {});

    EXPECT_EQ(hmi::entityChoices(*arena->find("side"), gate, context),
              (std::vector<std::string>{"allies", "enemies"}));
    EXPECT_EQ(hmi::entityChoices(*portal->find("targetMap"), gate, context),
              (std::vector<std::string>{"foret", "village"}));
    EXPECT_TRUE(hmi::entityChoices(*portal->find("arrival"), gate, context).empty());
    gate.properties["targetMap"] = std::string{"village"};
    EXPECT_EQ(hmi::entityChoices(*portal->find("arrival"), gate, context),
              (std::vector<std::string>{"puits"}));
}

/**
 * @brief Les catalogues se lisent, et les deux cartes d essai ne lèvent aucun
 * avertissement : figurines, quartiers gardés, portails et points d'arrivée sont tous connus.
 * \castest{<b>Les catalogues alimentent l'editeur.</b><br/>
 * \tcat Unitaire · Edition d'entites<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire les references de la racine d essai.<br/>2. Valider les entites des deux
 * cartes.<br/>
 * \tattendu Le dialogue du garde, la rencontre du donjon, les figurines des deux ateliers, les
 * lieux de l'atlas, les objets et les cartes sont connus ; aucune entite n'est signalee.
 * }
 */
TEST(EditionEntitesTest, LesCataloguesAlimententLEditeur) {
    const hmi::EditorReferences references = hmi::loadEditorReferences(elementsRoot());
    EXPECT_NE(std::ranges::find(references.dialogues, "garde-du-bourg"),
              references.dialogues.end());
    EXPECT_NE(references.encounters.find("rats-du-donjon"), nullptr);
    ASSERT_NE(references.world.find("donjon"), nullptr);
    EXPECT_TRUE(std::ranges::binary_search(references.figures, "figurant"));
    EXPECT_TRUE(std::ranges::binary_search(references.figures, "Monsters/sentinelle"));
    EXPECT_FALSE(references.locations.empty());
    EXPECT_FALSE(references.items.empty());

    for (const char* const mapId : {"donjon", "bourg/place"}) {
        const core::LevelLoadResult map = core::LevelLoader::loadFromFile(
            elementsRoot() / "Levels" / (std::string{mapId} + ".json"));
        ASSERT_TRUE(map.ok()) << map.error;
        const core::EntityReferenceContext context =
            hmi::referenceContext(references, mapId, map.level->entities());
        EXPECT_TRUE(context.entityRefs.contains(std::string{mapId} + "#e1")) << mapId;
        for (const core::EntityIssue& issue :
             core::validateMapEntities(map.level->entities(), context)) {
            ADD_FAILURE() << mapId << " : entite " << issue.entityIndex << ", " << issue.key
                          << " = " << issue.value;
        }
    }
}

/**
 * @brief Les avertissements nomment l'entité, la propriété et la valeur, puis le terrain.
 * \castest{<b>Les avertissements de l'editeur.</b><br/>
 * \tcat Unitaire · Edition d'entites<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Traduire un dialogue inconnu, un combattant dans un mur et une zone trop
 * etroite.<br/>
 * \tattendu Trois lignes, references d'abord, avec leur message.
 * }
 */
TEST(EditionEntitesTest, AvertissementsDeLEditeur) {
    const std::vector<core::MapEntity> entities = {
        core::MapEntity{.type = "npc", .position = {.column = 1, .row = 1}, .properties = {}},
        core::MapEntity{
            .type = "encounter", .position = {.column = 3, .row = 2}, .properties = {}}};
    const std::vector<core::EntityIssue> issues = {
        core::EntityIssue{.entityIndex = 0,
                          .code = core::EntityIssueCode::UnknownDialogue,
                          .key = "dialogue",
                          .value = "absent"}};
    core::EncounterTerrain terrain;
    terrain.entityIndex = 1;
    terrain.encounterId = "colisee-fauves";
    terrain.area.resize(5);
    terrain.requiredCells = 24;
    terrain.issues = {core::TacticalIssue{.code = core::TacticalIssueCode::CombatantObstructed,
                                          .creatureId = "rat",
                                          .cell = {.column = 4, .row = 2}},
                      core::TacticalIssue{.code = core::TacticalIssueCode::AreaTooNarrow,
                                          .creatureId = {},
                                          .cell = {.column = 3, .row = 2}}};

    const std::vector<hmi::EditorDiagnostic> lines =
        hmi::editorDiagnostics(entities, issues, {terrain});
    ASSERT_EQ(lines.size(), 3U);
    EXPECT_EQ(lines[0], (hmi::EditorDiagnostic{.kind = hmi::EditorDiagnosticKind::Reference,
                                               .entityIndex = 0,
                                               .cell = {.column = 1, .row = 1},
                                               .message = "npc: dialogue \"absent\" does not "
                                                          "exist, or was rejected when loading."}));
    EXPECT_EQ(lines[1].message,
              "Encounter \"colisee-fauves\": \"rat\" would stand on an obstacle.");
    EXPECT_EQ(lines[1].cell, (core::GridPosition{.column = 4, .row = 2}));
    EXPECT_EQ(lines[2].message,
              "Encounter \"colisee-fauves\": area too narrow to fight in (5 free cells, 24 "
              "required).");
}
