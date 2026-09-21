// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_parcours_edition_rpg.cpp
 * @brief Parcours système du `LOT-11` : produire une carte du RPG sans écrire de JSON — trois
 *        couches, un PNJ, un coffre, un portail, une rencontre — l'enregistrer, la recharger et la
 *        peupler comme le fait l'essai immédiat, sans la couche GPU.
 */

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/Encounter.h"
#include "Core/Combat/TacticalTerrain.h"
#include "Core/Ecs/Components/Interactable.h"
#include "Core/Ecs/World.h"
#include "Core/Gameplay/MapEntitySpawner.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelWriter.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileType.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/WorldGraph.h"

namespace {

// La racine d essai de l editeur (LOT-123) : ce parcours validait ses entites contre les
// catalogues LIVRES, que la table rase du LOT-102 emporte. Les catalogues d essai portent ce
// qu il lui faut -- un dialogue, une rencontre a deux combattants.
[[nodiscard]] std::filesystem::path elementsRoot() {
    return std::filesystem::path{JADG_EDITOR_DATA_DIR};
}

// Pose une entite neuve de la famille @p type, ses proprietes a leur defaut, puis @p properties.
std::size_t place(core::LevelDraft& draft, std::string_view type, int column, int row,
                  const core::PropertyMap& properties = {}) {
    const core::EntityKind* const kind = core::findEntityKind(type);
    EXPECT_NE(kind, nullptr) << type;
    const std::optional<std::size_t> index =
        draft.placeEntity(core::makeEntity(*kind, {.column = column, .row = row}));
    EXPECT_TRUE(index.has_value());
    for (const auto& [key, value] : properties) {
        EXPECT_TRUE(draft.setEntityProperty(*index, key, value)) << key;
    }
    return *index;
}

}  // namespace

/**
 * @brief Parcours complet d'auteur du `LOT-11` : une carte à trois couches et quatre familles
 * d'entités, validée contre les catalogues d essai, enregistrée, rechargée, peuplée.
 * \castest{<b>Produire une carte du RPG dans l'editeur, sans JSON ecrit a la main.</b><br/>
 * \tcat Système · Éditeur de niveaux<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Creer une carte, poser l'entree et un couloir de murs dans la collision.<br/>2.
 * Ajouter un sol (qui reprend l'image) puis un decor, peindre l'un et l'autre.<br/>3. Poser un PNJ
 * au dialogue du garde, un coffre, un point d'arrivee et un portail qui y mene, une rencontre en
 * terrain ouvert et une dans le couloir.<br/>4. Valider contre les catalogues d essai et analyser
 * le terrain.<br/>5. Enregistrer, recharger, construire le graphe du monde et peupler le monde
 * ECS.<br/>6. Tout annuler.<br/>
 * \tattendu Aucune reference cassee ; seule la rencontre du couloir est signalee ; la carte relue
 * porte ses trois couches et ses six entites ; le portail se resout ; le coffre et le PNJ sont
 * interactifs ; l'annulation rend la carte vierge d'entites et de couches.
 * }
 */
TEST(ParcoursEditionSysteme, ProduitUneCarteDuRpgSansEcrireDeJson) {
    // 1. Collision : l'entree, et un couloir d'une case de large ferme par des murs.
    core::LevelDraft draft = core::LevelDraft::empty("parcours-lot-11", 20, 12);
    draft.setEntry(1, 1);
    for (int column = 12; column < 20; ++column) {
        draft.paintTile(column, 5, core::TileType::Wall);
        draft.paintTile(column, 7, core::TileType::Wall);
    }
    draft.paintTile(19, 6, core::TileType::Wall);

    // 2. Couches visuelles : le sol reprend l'image de la grille, le decor nait vide.
    const std::optional<std::size_t> ground = draft.addLayer(core::LayerKind::Ground, "sol");
    const std::optional<std::size_t> decor = draft.addLayer(core::LayerKind::Decor, "decor");
    ASSERT_TRUE(ground && decor);
    EXPECT_EQ(draft.layers()[*ground].tiles.tile(12, 5), core::TileType::Wall);
    EXPECT_TRUE(
        draft.paintLayerRegion(*ground, 0, 0,
                               std::vector<std::vector<core::TileType>>(
                                   4, std::vector<core::TileType>(6, core::TileType::Grass))));
    EXPECT_TRUE(draft.paintLayerTile(*decor, 3, 3, core::TileType::Water));
    EXPECT_FALSE(draft.paintLayerTile(*decor, 3, 4, core::TileType::Entry));

    // 3. Entites.
    place(draft, core::NPC_ENTITY_TYPE, 4, 2, {{"dialogue", std::string{"garde-du-bourg"}}});
    place(draft, "chest", 6, 2);
    place(draft, core::SPAWN_POINT_ENTITY_TYPE, 2, 9, {{"name", std::string{"puits"}}});
    place(draft, core::PORTAL_ENTITY_TYPE, 9, 9,
          {{"targetMap", std::string{"parcours-lot-11"}}, {"arrival", std::string{"puits"}}});
    const std::size_t open =
        place(draft, "encounter", 5, 6, {{"encounterId", std::string{"rats-du-donjon"}}});
    const std::size_t corridor =
        place(draft, "encounter", 15, 6, {{"encounterId", std::string{"rats-du-donjon"}}});

    // 4. Validation contre les catalogues d essai, comme le fait le panneau Entites.
    core::EntityReferenceContext context;
    for (const core::DialogueGraph& graph :
         core::loadDialogues(elementsRoot() / "World" / "dialogues").dialogues) {
        context.dialogues.insert(graph.id);
    }
    const core::EncounterCatalog encounters =
        core::loadEncounters(elementsRoot() / "Rpg" / "encounters");
    for (const core::Encounter& encounter : encounters.encounters) {
        context.encounters.insert(encounter.id);
    }
    context.arrivalPointsByMap[draft.name()] = core::arrivalPointNames(draft.entities());
    EXPECT_TRUE(core::validateMapEntities(draft.entities(), context).empty());

    const std::vector<core::EncounterTerrain> terrains =
        core::analyzeEncounterTerrain(draft.tileMap(), draft.entities(), encounters);
    ASSERT_EQ(terrains.size(), 2U);
    EXPECT_EQ(terrains[0].entityIndex, open);
    EXPECT_TRUE(terrains[0].valid());
    EXPECT_EQ(terrains[1].entityIndex, corridor);
    EXPECT_FALSE(terrains[1].valid());

    // 5. Enregistrer, recharger, relier, peupler.
    const core::LevelLoadResult validated = draft.toLevel();
    ASSERT_TRUE(validated.ok()) << validated.error;
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "jadg_systeme_parcours_lot_11";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    ASSERT_TRUE(
        core::LevelWriter::saveToFile(*validated.level, directory / "parcours-lot-11.json"));

    const core::LevelLoadResult reloaded =
        core::LevelLoader::loadFromFile(directory / "parcours-lot-11.json");
    const core::WorldGraph world = core::loadWorldGraph(directory);
    std::filesystem::remove_all(directory);
    ASSERT_TRUE(reloaded.ok()) << reloaded.error;
    EXPECT_EQ(reloaded.level->layers().size(), 3U);
    EXPECT_EQ(reloaded.level->layers()[*decor].tiles.tile(3, 3), core::TileType::Water);
    ASSERT_EQ(reloaded.level->entities().size(), 6U);
    ASSERT_EQ(world.portals.size(), 1U);
    EXPECT_EQ(world.portals.front().status, core::PortalLinkStatus::Resolved);

    core::World ecs;
    EXPECT_EQ(core::spawnMapEntities(ecs, *reloaded.level, reloaded.level->name()), 6U);
    std::vector<std::string> interactables;
    for (auto [entity, interactable] : ecs.view<core::Interactable>()) {
        interactables.push_back(interactable.type);
    }
    std::ranges::sort(interactables);
    EXPECT_EQ(interactables, (std::vector<std::string>{"chest", "npc"}));

    // 6. Tout est annulable : l'historique ramene la carte a sa collision seule.
    while (draft.canUndo()) {
        ASSERT_TRUE(draft.undo());
    }
    EXPECT_TRUE(draft.entities().empty());
    EXPECT_TRUE(draft.layers().empty());
}
