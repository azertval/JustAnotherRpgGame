// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_entity_shapes.cpp
 * @brief Tests unitaires des entités à forme sur le canevas (`LOT-EDITOR-05`) : rectangles et
 *        poignées, zones peintes, trajets, choix sous le curseur, glisser de groupe, liste
 *        filtrable, et l'acceptation sur la zone de combat du Colisée.
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/Arena.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/MapEntity.h"
#include "Core/World/CombatZone.h"
#include "Core/World/EntityKinds.h"
#include "Editor/Logic/CanvasScene.h"
#include "Editor/Logic/EditorDiagnostics.h"
#include "Editor/Logic/EntityGesture.h"
#include "Editor/Logic/EntityShapes.h"

namespace {

using core::GridPosition;

[[nodiscard]] core::MapEntity rectangleEntity(std::string type, GridPosition origin, int width,
                                              int height) {
    return core::MapEntity{.type = std::move(type),
                           .position = origin,
                           .properties = {{"name", std::string{"z"}},
                                          {"width", std::int64_t{width}},
                                          {"height", std::int64_t{height}}}};
}

[[nodiscard]] core::MapEntity route(std::vector<GridPosition> points) {
    core::MapEntity entity{.type = std::string{core::ROUTE_ENTITY_TYPE},
                           .position = points.front(),
                           .properties = {{"name", std::string{"ronde"}}}};
    entity.cells = std::move(points);
    return entity;
}

[[nodiscard]] std::size_t countIssues(const std::vector<hmi::EditorDiagnostic>& lines,
                                      const std::string& fragment) {
    return static_cast<std::size_t>(
        std::ranges::count_if(lines, [&fragment](const hmi::EditorDiagnostic& line) {
            return line.message.find(fragment) != std::string::npos;
        }));
}

}  // namespace

/**
 * @brief La forme se lit dans la table des familles, jamais dans le type : un rectangle a huit
 * poignées, un trajet une par point, un point aucune.
 * \castest{<b>Les formes et leurs poignees viennent de la table.</b><br/>
 * \tcat Unitaire · Entites a forme<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire la forme d'une zone de combat, d'un ilot, d'une zone, d'un trajet, d'un coffre
 * et d'une famille inconnue.<br/>2. Lister les poignees d'un rectangle 5 x 4, d'un rectangle
 * 2 x 1, d'un trajet a trois points, d'un coffre.<br/>
 * \tattendu Rectangle, rectangle, zone, trajet, point, point ; 8, 4 (coins seuls), 3 et 0
 * poignees.
 * }
 */
TEST(EntitesAFormeTest, LaFormeVientDeLaTable) {
    const auto shape = [](std::string type) {
        return hmi::entityShape(core::MapEntity{.type = std::move(type), .position = {}});
    };
    EXPECT_EQ(shape("combatZone"), core::EntityShape::Rectangle);
    EXPECT_EQ(shape("cityBlock"), core::EntityShape::Rectangle);
    EXPECT_EQ(shape("zone"), core::EntityShape::Area);
    EXPECT_EQ(shape("route"), core::EntityShape::Path);
    EXPECT_EQ(shape("chest"), core::EntityShape::Point);
    EXPECT_EQ(shape("dragon"), core::EntityShape::Point);

    const core::MapEntity big = rectangleEntity("combatZone", {.column = 2, .row = 3}, 5, 4);
    const std::vector<hmi::EntityHandle> handles = hmi::entityHandles(big);
    EXPECT_EQ(handles.size(), 8U);
    EXPECT_EQ(hmi::handleAt(big, {.column = 6, .row = 6})->kind, hmi::HandleKind::SouthEast);
    EXPECT_EQ(hmi::handleAt(big, {.column = 4, .row = 3})->kind, hmi::HandleKind::North);
    EXPECT_FALSE(hmi::handleAt(big, {.column = 4, .row = 4}));
    EXPECT_EQ(hmi::entityHandles(rectangleEntity("cityBlock", {}, 2, 1)).size(), 4U);
    EXPECT_EQ(
        hmi::entityHandles(
            route({{.column = 1, .row = 1}, {.column = 4, .row = 1}, {.column = 4, .row = 5}}))
            .size(),
        3U);
    EXPECT_TRUE(hmi::entityHandles(core::MapEntity{.type = "chest", .position = {}}).empty());
}

/**
 * @brief Tirer une poignée déplace son côté, pas le côté opposé ; au-delà, le rectangle se
 * retourne et garde une case.
 * \castest{<b>Redimensionner un rectangle par ses poignees.</b><br/>
 * \tcat Unitaire · Entites a forme<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Tirer le coin bas-droit d'un rectangle (2, 3) 5 x 4 en (8, 9).<br/>2. Tirer le cote
 * nord en (4, 1).<br/>3. Tirer le cote ouest au-dela du cote est, en (9, 4).<br/>
 * \tattendu (2, 3) 7 x 7 ; (2, 1) 5 x 6 ; (6, 3) 4 x 4.
 * }
 */
TEST(EntitesAFormeTest, PoigneesDUnRectangle) {
    const hmi::CellRect rect{.origin = {.column = 2, .row = 3}, .columns = 5, .rows = 4};
    EXPECT_EQ(hmi::resizeRectangle(rect, hmi::HandleKind::SouthEast, {.column = 8, .row = 9}),
              (hmi::CellRect{.origin = {.column = 2, .row = 3}, .columns = 7, .rows = 7}));
    EXPECT_EQ(hmi::resizeRectangle(rect, hmi::HandleKind::North, {.column = 4, .row = 1}),
              (hmi::CellRect{.origin = {.column = 2, .row = 1}, .columns = 5, .rows = 6}));
    EXPECT_EQ(hmi::resizeRectangle(rect, hmi::HandleKind::West, {.column = 9, .row = 4}),
              (hmi::CellRect{.origin = {.column = 6, .row = 3}, .columns = 4, .rows = 4}));
    EXPECT_EQ(hmi::rectangleBetween({.column = 5, .row = 1}, {.column = 2, .row = 4}),
              (hmi::CellRect{.origin = {.column = 2, .row = 1}, .columns = 4, .rows = 4}));
}

/**
 * @brief Une zone rectangle devient peinte à sa première retouche ; ses cases restent triées, sa
 * case reste une case de la zone, et la gomme ne la vide jamais.
 * \castest{<b>Peindre une zone de regles case par case.</b><br/>
 * \tcat Unitaire · Entites a forme<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ajouter la case (3, 0) a une zone rectangle (1, 0) 2 x 1.<br/>2. En retirer (1, 0),
 * sa case.<br/>3. Retirer toutes ses cases.<br/>
 * \tattendu Trois cases triees, sans largeur ni hauteur ; deux cases, la case de l'entite passe a
 * (2, 0) ; la zone reste telle quelle.
 * }
 */
TEST(EntitesAFormeTest, UneZoneSePeint) {
    core::MapEntity zone{.type = std::string{core::ZONE_ENTITY_TYPE},
                         .position = {.column = 1, .row = 0},
                         .properties = {{"width", std::int64_t{2}}, {"height", std::int64_t{1}}}};
    zone = hmi::paintArea(zone, {{.column = 3, .row = 0}}, /*add=*/true);
    EXPECT_EQ(zone.cells,
              (std::vector<GridPosition>{
                  {.column = 1, .row = 0}, {.column = 2, .row = 0}, {.column = 3, .row = 0}}));
    EXPECT_FALSE(zone.properties.contains("width"));
    EXPECT_FALSE(zone.properties.contains("height"));
    EXPECT_FALSE(hmi::entityRectangle(zone));

    zone = hmi::paintArea(zone, {{.column = 1, .row = 0}}, /*add=*/false);
    EXPECT_EQ(zone.cells.size(), 2U);
    EXPECT_EQ(zone.position, (GridPosition{.column = 2, .row = 0}));
    EXPECT_EQ(core::zoneCells(zone), zone.cells);  // le jeu lit la meme forme.

    const core::MapEntity kept = hmi::paintArea(zone, zone.cells, /*add=*/false);
    EXPECT_EQ(kept.cells, zone.cells);
}

/**
 * @brief Un trajet se prolonge, un point se déplace (le premier emporte la case de l'entité), un
 * point se retire mais jamais le dernier.
 * \castest{<b>Tracer un trajet point par point.</b><br/>
 * \tcat Unitaire · Entites a forme<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Prolonger un trajet d'un point, puis du meme.<br/>2. Deplacer son premier point.<br/>
 * 3. Retirer des points jusqu'au dernier.<br/>
 * \tattendu Trois points, pas de doublon ; la case de l'entite suit ; un point reste.
 * }
 */
TEST(EntitesAFormeTest, UnTrajetSeTrace) {
    core::MapEntity path = route({{.column = 1, .row = 1}, {.column = 4, .row = 1}});
    path = hmi::withWaypointAdded(path, {.column = 4, .row = 5});
    path = hmi::withWaypointAdded(path, {.column = 4, .row = 5});
    EXPECT_EQ(path.cells.size(), 3U);

    path = hmi::withWaypointMoved(path, 0, {.column = 0, .row = 2});
    EXPECT_EQ(path.position, (GridPosition{.column = 0, .row = 2}));
    EXPECT_EQ(path.cells.front(), path.position);

    path = hmi::withWaypointRemoved(path, 0);
    EXPECT_EQ(path.position, (GridPosition{.column = 4, .row = 1}));
    path = hmi::withWaypointRemoved(path, 1);
    path = hmi::withWaypointRemoved(path, 0);
    EXPECT_EQ(path.cells, (std::vector<GridPosition>{{.column = 4, .row = 1}}));
}

/**
 * @brief Sous le curseur : une poignée de la sélection, puis une entité par sa case, puis le corps
 * de la plus petite forme.
 * \castest{<b>L'entite sous le curseur.</b><br/>
 * \tcat Unitaire · Entites a forme<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser un ilot 10 x 10, une zone 3 x 3 dedans, un coffre dans la zone.<br/>2.
 * Designer la case du coffre, une case de la zone, une case de l'ilot seul, le coin de la zone
 * selectionnee.<br/>
 * \tattendu Le coffre ; la zone (corps) ; l'ilot (corps) ; la poignee nord-ouest de la zone.
 * }
 */
TEST(EntitesAFormeTest, LEntiteSousLeCurseur) {
    const std::vector<core::MapEntity> entities = {
        rectangleEntity("cityBlock", {.column = 0, .row = 0}, 10, 10),
        rectangleEntity("zone", {.column = 4, .row = 4}, 3, 3),
        core::MapEntity{.type = "chest", .position = {.column = 5, .row = 5}}};
    EXPECT_EQ(hmi::pickEntity(entities, {.column = 5, .row = 5}, {}),
              (hmi::EntityPick{.index = 2, .handle = std::nullopt, .body = false}));
    EXPECT_EQ(hmi::pickEntity(entities, {.column = 6, .row = 6}, {}),
              (hmi::EntityPick{.index = 1, .handle = std::nullopt, .body = true}));
    EXPECT_EQ(hmi::pickEntity(entities, {.column = 8, .row = 8}, {}),
              (hmi::EntityPick{.index = 0, .handle = std::nullopt, .body = true}));
    const std::optional<hmi::EntityPick> corner =
        hmi::pickEntity(entities, {.column = 4, .row = 4}, {1});
    ASSERT_TRUE(corner);
    EXPECT_EQ(corner->index, 1U);
    ASSERT_TRUE(corner->handle);
    EXPECT_EQ(corner->handle->kind, hmi::HandleKind::NorthWest);
    EXPECT_FALSE(hmi::pickEntity(entities, {.column = 12, .row = 12}, {}));
}

/**
 * @brief Une famille à forme se tire au lieu de se poser : un rectangle entre deux coins, un trajet
 * à deux points ; poser dans le corps d'une zone reste possible.
 * \castest{<b>Tirer une zone, un trajet.</b><br/>
 * \tcat Unitaire · Entites a forme<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Appuyer avec la famille zone de combat, puis tirer de (1, 1) a (3, 2).<br/>2. Tirer
 * un trajet de (0, 0) a (4, 3), puis sans bouger.<br/>3. Appuyer dans le corps d'une zone avec la
 * famille coffre.<br/>
 * \tattendu Draw ; une zone de combat (1, 1) 3 x 2 ; un trajet de deux points, puis d'un ; Place.
 * }
 */
TEST(EntitesAFormeTest, TirerUneZoneOuUnTrajet) {
    core::LevelDraft map = core::LevelDraft::empty("tirer", 8, 6);
    EXPECT_EQ(map.placeEntity(rectangleEntity("zone", {.column = 4, .row = 2}, 3, 3)), 0U);
    EXPECT_EQ(hmi::resolveEntityPress(map, {.column = 1, .row = 1}, {}, "combatZone", {}).action,
              hmi::EntityGestureAction::Draw);
    EXPECT_EQ(hmi::resolveEntityPress(map, {.column = 5, .row = 3}, {}, "chest", {}).action,
              hmi::EntityGestureAction::Place);

    const hmi::EntityDrag drawZone{.mode = hmi::EntityDrag::Mode::Draw,
                                   .indices = {},
                                   .handle = std::nullopt,
                                   .kind = "combatZone",
                                   .from = {.column = 1, .row = 1}};
    const hmi::EntityDragResult zone =
        hmi::dragEntities(drawZone, map.entities(), {.column = 3, .row = 2}, 8, 6);
    ASSERT_TRUE(zone.placed);
    EXPECT_EQ(hmi::entityRectangle(*zone.placed),
              (hmi::CellRect{.origin = {.column = 1, .row = 1}, .columns = 3, .rows = 2}));

    hmi::EntityDrag drawRoute = drawZone;
    drawRoute.kind = "route";
    drawRoute.from = {.column = 0, .row = 0};
    EXPECT_EQ(hmi::dragEntities(drawRoute, map.entities(), {.column = 4, .row = 3}, 8, 6)
                  .placed->cells.size(),
              2U);
    EXPECT_TRUE(hmi::dragEntities(drawRoute, map.entities(), {.column = 0, .row = 0}, 8, 6)
                    .placed->cells.empty());
}

/**
 * @brief Le brouillon remplace une entité en un pas, sans toucher à son type ni à son
 * identifiant, et refuse une forme qui sort de la carte.
 * \castest{<b>Remplacer une entite garde son identifiant.</b><br/>
 * \tcat Unitaire · Entites a forme<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser un trajet, le remplacer par une valeur au type et a l'identifiant changes.<br/>
 * 2. Le remplacer par la meme valeur ; par une valeur dont un point sort de la carte.<br/>3.
 * Annuler.<br/>
 * \tattendu Remplace, type et identifiant d'origine ; rien ; refuse ; le trajet d'origine.
 * }
 */
TEST(EntitesAFormeTest, RemplacerGardeLIdentifiant) {
    core::LevelDraft map = core::LevelDraft::empty("remplacer", 6, 6);
    const core::MapEntity original = route({{.column = 1, .row = 1}, {.column = 3, .row = 1}});
    ASSERT_EQ(map.placeEntity(original), 0U);
    const std::string id = map.entities()[0].id;

    core::MapEntity changed = hmi::withWaypointAdded(map.entities()[0], {.column = 3, .row = 4});
    changed.type = "chest";
    changed.id = "e99";
    EXPECT_TRUE(map.replaceEntity(0, changed));
    EXPECT_EQ(map.entities()[0].type, core::ROUTE_ENTITY_TYPE);
    EXPECT_EQ(map.entities()[0].id, id);
    EXPECT_EQ(map.entities()[0].cells.size(), 3U);

    EXPECT_FALSE(map.replaceEntity(0, map.entities()[0]));
    EXPECT_FALSE(
        map.replaceEntity(0, hmi::withWaypointAdded(map.entities()[0], {.column = 9, .row = 1})));
    map.undo();
    EXPECT_EQ(map.entities()[0].cells, original.cells);
}

/**
 * @brief La liste se filtre sur le type, l'identifiant et les valeurs ; la sélection bascule.
 * \castest{<b>La liste filtrable des entites.</b><br/>
 * \tcat Unitaire · Entites a forme<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Filtrer trois entites par « PORTAL », par « place », par « e2 », par rien.<br/>
 * 2. Basculer le rang 2 dans une selection {0}, puis l'en retirer.<br/>
 * \tattendu {1} ; {1} ; {1} ; tout ; {0, 2} puis {0}.
 * }
 */
TEST(EntitesAFormeTest, ListeFiltrableEtSelection) {
    std::vector<core::MapEntity> entities = {
        core::MapEntity{.type = "chest", .position = {}},
        core::MapEntity{.type = "portal",
                        .position = {},
                        .properties = {{"targetMap", std::string{"bourg/place"}}}},
        core::MapEntity{.type = "npc", .position = {}}};
    entities[0].id = "e1";
    entities[1].id = "e2";
    entities[2].id = "e3";
    EXPECT_EQ(hmi::filterEntities(entities, "PORTAL"), (std::vector<std::size_t>{1}));
    EXPECT_EQ(hmi::filterEntities(entities, "place"), (std::vector<std::size_t>{1}));
    EXPECT_EQ(hmi::filterEntities(entities, "e2"), (std::vector<std::size_t>{1}));
    EXPECT_EQ(hmi::filterEntities(entities, "").size(), 3U);
    EXPECT_EQ(hmi::entityLabel(entities[1]), "bourg/place");

    EXPECT_EQ(hmi::toggledSelection({0}, 2), (std::vector<std::size_t>{0, 2}));
    EXPECT_EQ(hmi::toggledSelection({0, 2}, 2), (std::vector<std::size_t>{0}));
}

/**
 * @brief L'outil Forme peint une zone, trace un trajet, et laisse les rectangles du jeu à leurs
 * poignées.
 * \castest{<b>Le geste de l'outil Forme.</b><br/>
 * \tcat Unitaire · Entites a forme<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Appuyer sur une zone, avec et sans Ctrl.<br/>2. Sur un trajet : sur un point, avec
 * Ctrl, ailleurs.<br/>3. Sur une zone de combat.<br/>
 * \tattendu Peindre, gommer ; prendre le point 1, le retirer, prolonger ; rien.
 * }
 */
TEST(EntitesAFormeTest, GesteDeLOutilForme) {
    const core::MapEntity zone{.type = "zone", .position = {}};
    EXPECT_EQ(hmi::resolveShapePress(zone, {}, false).action, hmi::ShapeGestureAction::PaintCells);
    EXPECT_EQ(hmi::resolveShapePress(zone, {}, true).action, hmi::ShapeGestureAction::EraseCells);

    const core::MapEntity path = route({{.column = 1, .row = 1}, {.column = 3, .row = 1}});
    EXPECT_EQ(hmi::resolveShapePress(path, {.column = 3, .row = 1}, false),
              (hmi::ShapeGestureDecision{.action = hmi::ShapeGestureAction::GrabWaypoint,
                                         .waypoint = 1}));
    EXPECT_EQ(hmi::resolveShapePress(path, {.column = 3, .row = 1}, true).action,
              hmi::ShapeGestureAction::RemoveWaypoint);
    EXPECT_EQ(hmi::resolveShapePress(path, {.column = 5, .row = 5}, false).action,
              hmi::ShapeGestureAction::AppendWaypoint);
    EXPECT_EQ(hmi::resolveShapePress(rectangleEntity("combatZone", {}, 3, 3), {}, false).action,
              hmi::ShapeGestureAction::Ignore);
}

/**
 * @brief La formation d'une rencontre se montre par les figurines de l'atelier des monstres,
 * quand elles existent.
 * \castest{<b>La formation par ses figurines.</b><br/>
 * \tcat Unitaire · Entites a forme<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Une formation d'un lion et d'un rat ; l'atelier n'a que le lion.<br/>
 * \tattendu Une figurine, Monsters/lion, au centre de la case du lion.
 * }
 */
TEST(EntitesAFormeTest, LaFormationParSesFigurines) {
    core::EncounterTerrain terrain;
    terrain.placements = {
        core::CombatantPlacement{.creatureId = "lion", .position = {.column = 2, .row = 3}},
        core::CombatantPlacement{.creatureId = "rat", .position = {.column = 3, .row = 3}}};
    const std::vector<hmi::WorldFigureSnapshot> figures =
        hmi::formationFigures(terrain, {"Monsters/lion", "anariel"});
    ASSERT_EQ(figures.size(), 1U);
    EXPECT_EQ(figures[0].figure, "Monsters/lion");
    EXPECT_FLOAT_EQ(figures[0].point.x, 2.5F);
    EXPECT_FLOAT_EQ(figures[0].point.y, 3.5F);
}

/**
 * @brief **Acceptation du lot** : redimensionner la zone de combat du Colisée à la souris met à
 * jour son verdict tactique.
 * \castest{<b>Tirer la zone du Colisee change son verdict.</b><br/>
 * \tcat Unitaire · Entites a forme<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Ouvrir le Colisee livre ; lire le verdict de sa zone « sable ».<br/>2. Prendre sa
 * poignee est et la tirer de la colonne 29 a la colonne 20 ; ecrire le glisser.<br/>3. Relire le
 * verdict et les avertissements ; annuler.<br/>
 * \tattendu Avant : 20 x 14, huit entrees dedans, aucun avertissement de zone. Apres, en un pas :
 * 11 x 14, les quatre entrees ennemies dehors, quatre avertissements. Annuler rend la zone.
 * }
 */
TEST(EntitesAFormeTest, AcceptationRedimensionnerLaZoneDuColisee) {
    const core::LevelLoadResult loaded =
        core::LevelLoader::loadFromFile(std::filesystem::path{JADG_EDITOR_DATA_DIR} / "Levels" /
                                        "donjon.json");
    ASSERT_TRUE(loaded.ok()) << loaded.error;
    core::LevelDraft map = core::LevelDraft::fromLevel(*loaded.level);
    const auto zoneIndex = static_cast<std::size_t>(
        std::ranges::find(map.entities(), std::string{core::COMBAT_ZONE_ENTITY_TYPE},
                          &core::MapEntity::type) -
        map.entities().begin());
    ASSERT_LT(zoneIndex, map.entities().size());

    const auto verdict = [&map, zoneIndex] {
        const std::vector<core::CombatZoneTerrain> zones =
            core::analyzeCombatZones(map.tileMap(), map.entities());
        return *std::ranges::find(zones, zoneIndex, &core::CombatZoneTerrain::entityIndex);
    };
    const core::CombatZoneTerrain before = verdict();
    EXPECT_FALSE(before.issue);
    EXPECT_EQ(before.zone.columns, 20);
    EXPECT_EQ(before.entriesInside.size(), 8U);
    EXPECT_TRUE(before.entriesOutside.empty());
    EXPECT_EQ(countIssues(hmi::editorDiagnostics(map.entities(), {}, {}, {before}), "outside"), 0U);

    // La poignee est de la zone (colonne 29, milieu de ses 14 lignes), tiree jusqu'a la colonne 20.
    const GridPosition eastHandle{.column = 29, .row = 16};
    const hmi::EntityGestureDecision press =
        hmi::resolveEntityPress(map, eastHandle, {zoneIndex}, "", {});
    ASSERT_EQ(press.action, hmi::EntityGestureAction::Grab);
    ASSERT_TRUE(press.handle);
    EXPECT_EQ(press.handle->kind, hmi::HandleKind::East);
    const hmi::EntityDrag drag{.mode = hmi::EntityDrag::Mode::Reshape,
                               .indices = {zoneIndex},
                               .handle = press.handle,
                               .kind = {},
                               .from = eastHandle};
    const hmi::EntityDragResult result =
        hmi::dragEntities(drag, map.entities(), {.column = 20, .row = 16}, map.tileMap().width(),
                          map.tileMap().height());
    ASSERT_EQ(result.replaced.size(), 1U);
    const std::size_t steps = map.undoDepth();
    {
        const core::GestureScope gesture(map);
        for (const auto& [index, entity] : result.replaced) {
            EXPECT_TRUE(map.replaceEntity(index, entity));
        }
    }
    EXPECT_EQ(map.undoDepth(), steps + 1);

    const core::CombatZoneTerrain after = verdict();
    EXPECT_FALSE(after.issue);
    EXPECT_EQ(after.zone.columns, 11);
    EXPECT_EQ(after.zone.rows, 14);
    EXPECT_EQ(after.entriesInside.size(), 4U);
    EXPECT_EQ(after.entriesOutside.size(), 4U);
    const std::vector<hmi::EditorDiagnostic> lines =
        hmi::editorDiagnostics(map.entities(), {}, {}, {after});
    EXPECT_EQ(countIssues(lines, "outside every combat zone"), 4U);
    EXPECT_NE(hmi::combatZoneSummary(after).find("salle: 11 x 14"), std::string::npos);

    map.undo();
    EXPECT_EQ(verdict().zone.columns, 20);
}
