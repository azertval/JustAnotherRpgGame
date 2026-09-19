// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_coliseum_map.cpp
 * @brief La carte du Colisee, telle qu'elle est LIVREE (LOT-09) : elle se charge, elle se tient,
 *        et tout ce qu'elle nomme existe.
 *
 * Ce ne sont pas des tests de moteur mais des tests de CONTENU : la carte est la premiere du jeu,
 * et un portail orphelin, une piece de planche absente ou une salle inatteignable ne se voient pas
 * dans un test unitaire de code -- ils se voient ici, ou nulle part avant la manette en main.
 */

#include <deque>
#include <filesystem>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/Arena.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileMap.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/ExplorationSession.h"
#include "Core/World/WorldGraph.h"
#include "Core/World/WorldTravel.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

const std::filesystem::path NIVEAUX{JADG_LEVELS_DIR};
const std::filesystem::path MONDE{JADG_WORLD_DIR};
const std::filesystem::path ASSETS{JADG_ASSETS_DIR};

[[nodiscard]] core::Level colisee() {
    core::LevelLoadResult lu = core::LevelLoader::loadFromFile(NIVEAUX / "coliseum.json");
    EXPECT_TRUE(lu.ok()) << lu.error;
    EXPECT_TRUE(lu.level.has_value());
    return lu.level.has_value() ? std::move(*lu.level)
                                : core::Level{core::LevelData{.tileMap = core::TileMap{1, 1}}};
}

[[nodiscard]] std::string texteDe(const core::MapEntity& entite, std::string_view cle) {
    const auto trouvee = entite.properties.find(std::string{cle});
    if (trouvee == entite.properties.end()) {
        return {};
    }
    const std::string* texte = std::get_if<std::string>(&trouvee->second);
    return texte != nullptr ? *texte : std::string{};
}

}  // namespace

/**
 * @brief La carte livree se charge, et c'est un lieu, pas une grille nue.
 * \castest{<b>coliseum.json se charge : 40 x 34 cases, un sol, un relief, et un lieu
 * nomme.</b><br/>
 * \tcat Unitaire · Carte du Colisee<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger `Source/Elements/Levels/coliseum.json`.<br/>
 * \tattendu Aucun defaut de chargement ; une couche de sol qui declare le lieu « coliseum », une
 * couche de relief, et le sable du LOT-50 (20 x 14) au centre.
 * }
 */
TEST(ColiseumMapTest, LaCarteLivreeSeCharge) {
    const core::Level carte = colisee();

    // Une cle de traduction, que le bandeau du jeu traduit (LOT-EDITOR-07).
    EXPECT_EQ(carte.name(), "map.coliseum.name");
    EXPECT_EQ(carte.tileMap().width(), 40);
    EXPECT_EQ(carte.tileMap().height(), 34);
    EXPECT_EQ(hmi::scenePlaceOf(carte), "coliseum");

    bool sol = false;
    bool relief = false;
    for (const core::TileLayer& couche : carte.layers()) {
        sol = sol || couche.kind == core::LayerKind::Ground;
        relief = relief || couche.kind == core::LayerKind::Decor;
    }
    EXPECT_TRUE(sol);
    EXPECT_TRUE(relief);

    // Le sable : vingt colonnes sur quatorze lignes, la grille du LOT-50, franchissable partout.
    for (int ligne = 10; ligne <= 23; ++ligne) {
        for (int colonne = 10; colonne <= 29; ++colonne) {
            EXPECT_FALSE(carte.tileMap().isSolid(colonne, ligne))
                << "le sable doit etre franchissable en (" << colonne << ", " << ligne << ")";
        }
    }
}

/**
 * @brief Le graphe du dossier des niveaux livre ne porte aucun defaut.
 * \castest{<b>Aucun portail orphelin, aucun point d'arrivee en double dans les cartes
 * livrees.</b><br/>
 * \tcat Unitaire · Carte du Colisee<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire le graphe du dossier des niveaux livre.<br/>
 * 2. Le valider, puis valider la carte du Colisee elle-meme.<br/>
 * \tattendu Aucun defaut : c'est le controle que le LOT-09 exige au chargement (EX-NFR-040),
 * applique au contenu commite.
 * }
 */
TEST(ColiseumMapTest, LeGrapheLivreNePorteAucunDefaut) {
    const core::WorldGraph graphe = core::loadWorldGraph(NIVEAUX);
    const std::vector<core::WorldIssue> defauts = core::validateWorldGraph(graphe);
    for (const core::WorldIssue& defaut : defauts) {
        ADD_FAILURE() << defaut.mapId << " (" << defaut.position.column << ", "
                      << defaut.position.row << ") : code " << static_cast<int>(defaut.code) << " «"
                      << defaut.value << "»";
    }
    EXPECT_TRUE(core::validateWorldMap("coliseum", colisee()).empty());

    // Les deux points d'arrivee que le LOT-96 visera depuis Arenarea.
    const core::WorldMapNode* const noeud = graphe.find("coliseum");
    ASSERT_NE(noeud, nullptr);
    EXPECT_EQ(noeud->arrivalPoints, (std::vector<std::string>{"porte", "sable"}));
}

/**
 * @brief Chaque PNJ de la carte nomme un dialogue qui existe et une figurine livree.
 * \castest{<b>Les cinq PNJ du Colisee ont un dialogue du catalogue et une figurine de
 * l'atelier.</b><br/>
 * \tcat Unitaire · Carte du Colisee<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Relever les entites `npc` de la carte.<br/>
 * 2. Verifier chaque dialogue contre le catalogue, chaque figurine contre `Assets/Npc/`.<br/>
 * \tattendu Cinq PNJ, cinq dialogues acceptes, cinq dossiers de figurine avec leur bande de repos.
 * }
 */
TEST(ColiseumMapTest, ChaquePnjNommeUnDialogueEtUneFigurineQuiExistent) {
    const core::Level carte = colisee();
    const core::DialogueCatalog catalogue = core::loadDialogues(MONDE / "dialogues");
    EXPECT_TRUE(catalogue.errors.empty()) << catalogue.errors.front();

    int pnj = 0;
    for (const core::MapEntity& entite : carte.entities()) {
        if (entite.type != core::NPC_ENTITY_TYPE) {
            continue;
        }
        ++pnj;
        const std::string dialogue = texteDe(entite, core::NPC_DIALOGUE_PROPERTY);
        const std::string figurine = texteDe(entite, core::NPC_FIGURE_PROPERTY);
        EXPECT_NE(catalogue.find(dialogue), nullptr) << "dialogue inconnu : " << dialogue;
        EXPECT_FALSE(figurine.empty());
        EXPECT_TRUE(std::filesystem::exists(ASSETS / "Npc" / figurine / "idle.png"))
            << "figurine inconnue : " << figurine;
        EXPECT_FALSE(carte.tileMap().isSolid(entite.position.column, entite.position.row))
            << "un PNJ pose dans un mur : " << dialogue;
    }
    EXPECT_EQ(pnj, 5);
}

/**
 * @brief Chaque piece nommee par la carte existe sur la planche du lieu.
 * \castest{<b>Toute piece assignee a une case existe dans Assets/Scene/coliseum/.</b><br/>
 * \tcat Unitaire · Carte du Colisee<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Tirer l'instantane du Colisee avec la table d'apparence livree.<br/>
 * 2. Verifier que chaque chemin de texture qu'il demande existe sur le disque.<br/>
 * \tattendu Aucun chemin manquant : une piece absente tomberait sur le damier, ce qui se voit a
 * l'ecran et ne casse aucun test de code.
 * }
 */
TEST(ColiseumMapTest, ChaquePieceNommeeExisteSurLaPlanche) {
    hmi::PlaceAppearanceResult table =
        hmi::PlaceAppearance::loadFromFile(ASSETS / "Scene" / "coliseum" / "appearance.json");
    ASSERT_TRUE(table.ok()) << table.message;
    EXPECT_EQ(table.appearance.place(), "coliseum");

    const hmi::WorldSceneSnapshot instantane = hmi::snapshotWorldScene(
        colisee(), table.appearance,
        {hmi::WorldFigureSnapshot{.figure = "jade", .clip = "idle", .point = {}, .frame = 0}});
    const std::vector<std::string> chemins = hmi::worldTexturePaths(instantane);
    EXPECT_GT(chemins.size(), 20U);
    for (const std::string& chemin : chemins) {
        EXPECT_TRUE(std::filesystem::exists(ASSETS / chemin)) << "piece absente : " << chemin;
    }
}

/**
 * @brief Tout le Colisee se parcourt depuis sa porte.
 * \castest{<b>Depuis la porte, on atteint le sable, les deux vestiaires, les tribunes et la
 * loge.</b><br/>
 * \tcat Unitaire · Carte du Colisee<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Remplir la carte depuis la case d'entree, en ne passant que par le
 * franchissable.<br/>
 * 2. Verifier qu'une case de chaque lieu est atteinte, et qu'aucune case atteinte n'est hors du
 * batiment.<br/>
 * \tattendu Les six lieux du Colisee sont relies ; une salle inatteignable serait du contenu
 * invisible.
 * }
 */
TEST(ColiseumMapTest, ToutLeColiseeSeParcourtDepuisSaPorte) {
    const core::Level carte = colisee();
    const core::TileMap& collision = carte.tileMap();

    std::set<std::pair<int, int>> vues;
    std::deque<std::pair<int, int>> file{{19, 32}};
    vues.insert({19, 32});
    while (!file.empty()) {
        const auto [colonne, ligne] = file.front();
        file.pop_front();
        const std::pair<int, int> voisins[4] = {
            {colonne + 1, ligne}, {colonne - 1, ligne}, {colonne, ligne + 1}, {colonne, ligne - 1}};
        for (const auto& [x, y] : voisins) {
            if (!collision.inBounds(x, y) || collision.isSolid(x, y) || vues.contains({x, y})) {
                continue;
            }
            vues.insert({x, y});
            file.push_back({x, y});
        }
    }

    // Une case temoin par lieu : le hall, le sable, les deux vestiaires, les deux tribunes, la
    // loge.
    const std::pair<int, int> temoins[7] = {
        {19, 31},  // le hall
        {20, 16},  // le sable
        {4, 16},   // le vestiaire de l'ouest
        {35, 16},  // le vestiaire de l'est
        {13, 7},   // les tribunes du nord
        {26, 26},  // les tribunes du sud
        {19, 6},   // la loge imperiale
    };
    for (const auto& [colonne, ligne] : temoins) {
        EXPECT_TRUE(vues.contains({colonne, ligne}))
            << "inatteignable depuis la porte : (" << colonne << ", " << ligne << ")";
    }
    // Et l'on ne sort pas du batiment : chaque case atteinte porte une matiere de sol.
    hmi::PlaceAppearanceResult table =
        hmi::PlaceAppearance::loadFromFile(ASSETS / "Scene" / "coliseum" / "appearance.json");
    ASSERT_TRUE(table.ok());
    const hmi::WorldSceneSnapshot instantane = hmi::snapshotWorldScene(carte, table.appearance, {});
    for (const auto& [colonne, ligne] : vues) {
        EXPECT_FALSE(instantane.floorAt({colonne, ligne}).empty())
            << "case franchissable sans sol : (" << colonne << ", " << ligne << ")";
    }
}

/**
 * @brief Les points d'entree des deux camps sont dans le sable.
 * \castest{<b>Les huit points d'entree d'arene sont sur le sable, quatre par camp.</b><br/>
 * \tcat Unitaire · Carte du Colisee<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Relever les entites `arenaEntry` de la carte.<br/>
 * \tattendu Quatre par camp, toutes dans le rectangle du sable : la session d'arene jouera sur
 * cette zone (LOT-09, phase 4), et un point d'entree hors zone lui serait inconnu.
 * }
 */
TEST(ColiseumMapTest, LesPointsDEntreeDesDeuxCampsSontDansLeSable) {
    const core::Level carte = colisee();
    int allies = 0;
    int ennemis = 0;
    for (const core::MapEntity& entite : carte.entities()) {
        if (entite.type != core::ARENA_ENTRY_ENTITY_TYPE) {
            continue;
        }
        const std::string camp = texteDe(entite, core::ARENA_SIDE_PROPERTY);
        if (camp == "allies") {
            ++allies;
        } else if (camp == "enemies") {
            ++ennemis;
        }
        EXPECT_GE(entite.position.column, 10);
        EXPECT_LE(entite.position.column, 29);
        EXPECT_GE(entite.position.row, 10);
        EXPECT_LE(entite.position.row, 23);
    }
    EXPECT_EQ(allies, 4);
    EXPECT_EQ(ennemis, 4);
}
