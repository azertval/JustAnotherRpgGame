// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_level_draft_pieces.cpp
 * @brief Tests unitaires de la pose des pièces dans le brouillon, et de la collision qui suit les
 *        gestes (`LOT-EDITOR-03`, `EX-EDIT-064`, `EX-EDIT-065`).
 */

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileType.h"
#include "Core/Resources/ScenePieceManifest.h"

namespace {

using core::GridPosition;
using core::LevelDraft;
using core::TileType;

// Une petite planche : un pavé qui passe, un pilier qui arrête la vue, un étal 2 x 1 et une fosse
// 1 x 2 qui arrête le pas.
constexpr const char* PLANCHE = R"({
  "version": 1, "disposition": "essai",
  "textures": {
    "scene/essai/street": {"file": "street.png", "class": "floor", "footprint": [1, 1], "tactical": "open"},
    "scene/essai/pillar": {"file": "pillar.png", "class": "tall", "footprint": [1, 1]},
    "scene/essai/stall":  {"file": "stall.png",  "class": "wide", "footprint": [2, 1]},
    "scene/essai/pit":    {"file": "pit.png",    "class": "wide", "footprint": [1, 2], "tactical": "obstacle"}
  }
})";

// Carte v4 de 4 x 3 : un sol pavé partout, un décor vide, l'entrée en (0, 0).
constexpr const char* CARTE = R"({
  "version": 4, "name": "etal", "width": 4, "height": 3,
  "tiles": [ {"x": 0, "y": 0, "type": "entry"} ],
  "layers": [
    {"name": "sol", "kind": "ground", "tiles": [
      {"x": 0, "y": 0, "type": "dirt", "piece": "street"}, {"x": 1, "y": 0, "type": "dirt", "piece": "street"},
      {"x": 2, "y": 0, "type": "dirt", "piece": "street"}, {"x": 3, "y": 0, "type": "dirt", "piece": "street"},
      {"x": 0, "y": 1, "type": "dirt", "piece": "street"}, {"x": 1, "y": 1, "type": "dirt", "piece": "street"},
      {"x": 2, "y": 1, "type": "dirt", "piece": "street"}, {"x": 3, "y": 1, "type": "dirt", "piece": "street"},
      {"x": 0, "y": 2, "type": "dirt", "piece": "street"}, {"x": 1, "y": 2, "type": "dirt", "piece": "street"},
      {"x": 2, "y": 2, "type": "dirt", "piece": "street"}, {"x": 3, "y": 2, "type": "dirt", "piece": "street"}
    ]},
    {"name": "relief", "kind": "decor", "tiles": []}
  ]
})";

// Rangs des couches du brouillon : la collision est promue en tête.
constexpr std::size_t SOL = 1;
constexpr std::size_t DECOR = 2;

[[nodiscard]] LevelDraft brouillon() {
    core::LevelLoadResult loaded = core::LevelLoader::loadFromString(CARTE);
    EXPECT_TRUE(loaded.ok()) << loaded.error;
    LevelDraft draft = LevelDraft::fromLevel(*loaded.level);
    core::ScenePieceManifestResult planche = core::ScenePieceManifest::loadFromString(PLANCHE);
    EXPECT_TRUE(planche.ok()) << planche.message;
    draft.setPieceManifest(
        std::make_shared<const core::ScenePieceManifest>(std::move(planche.manifest)));
    return draft;
}

[[nodiscard]] TileType collision(const LevelDraft& draft, int column, int row) {
    return draft.tileMap().tile(column, row);
}

}  // namespace

/**
 * @brief Poser une pièce écrit, en un geste, sa couche, sa pièce, le type de sa case et la
 *        collision qu'elle donne.
 * \castest{<b>Poser une pièce écrit sa couche, sa pièce et sa collision.</b><br/>
 * \tcat Unitaire · Pièces du brouillon<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser un pilier en (2, 1) du décor.<br/>2. Défaire.<br/>
 * \tattendu Le décor nomme `pillar`, la case vaut `wall`, la collision `wall` ; un seul pas
 * d'annulation rend la carte d'avant.
 * }
 */
TEST(LevelDraftPiecesTest, PoserUnePieceEcritSaCoucheSaPieceEtSaCollision) {
    LevelDraft draft = brouillon();

    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 2, .row = 1}, "pillar", TileType::Wall));

    EXPECT_EQ(draft.layers()[DECOR].pieceAt(2, 1), "pillar");
    EXPECT_EQ(draft.layers()[DECOR].tiles.tile(2, 1), TileType::Wall);
    EXPECT_EQ(collision(draft, 2, 1), TileType::Wall);
    EXPECT_EQ(draft.undoDepth(), 1U);

    ASSERT_TRUE(draft.undo());
    EXPECT_TRUE(draft.layers()[DECOR].pieceAt(2, 1).empty());
    EXPECT_EQ(collision(draft, 2, 1), TileType::Empty);
}

/**
 * @brief Poser puis gommer un étal 2 × 1 occupe puis libère ses deux cases, collision comprise
 *        (acceptation du `LOT-EDITOR-03`).
 * \castest{<b>Un étal 2 × 1 occupe puis libère ses deux cases.</b><br/>
 * \tcat Unitaire · Pièces du brouillon<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser l'étal en (1, 1).<br/>2. Gommer sa deuxième case, (2, 1).<br/>
 * \tattendu Posé, il arrête la vue en (1, 1) et (2, 1) et seule son ancre le nomme ; gommé par sa
 * deuxième case, il part entier et les deux cases repassent.
 * }
 */
TEST(LevelDraftPiecesTest, PoserPuisGommerUnEtalOccupePuisLibereSesDeuxCases) {
    LevelDraft draft = brouillon();

    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 1, .row = 1}, "stall", TileType::Wall));
    EXPECT_EQ(collision(draft, 1, 1), TileType::Wall);
    EXPECT_EQ(collision(draft, 2, 1), TileType::Wall);
    EXPECT_EQ(collision(draft, 3, 1), TileType::Empty);
    EXPECT_TRUE(draft.layers()[DECOR].pieceAt(2, 1).empty());
    EXPECT_EQ(draft.pieceAnchorAt(DECOR, {.column = 2, .row = 1}),
              (GridPosition{.column = 1, .row = 1}));

    ASSERT_TRUE(draft.eraseLayerRegion(DECOR, {.column = 2, .row = 1}, {.column = 2, .row = 1}));
    EXPECT_TRUE(draft.layers()[DECOR].pieceAt(1, 1).empty());
    EXPECT_EQ(draft.layers()[DECOR].tiles.tile(1, 1), TileType::Empty);
    EXPECT_EQ(collision(draft, 1, 1), TileType::Empty);
    EXPECT_EQ(collision(draft, 2, 1), TileType::Empty);
}

/**
 * @brief Reposer la même pièce ne modifie pas la carte : ni pas d'annulation, ni révision neuve.
 * \castest{<b>Reposer la même pièce ne modifie pas la carte.</b><br/>
 * \tcat Unitaire · Pièces du brouillon<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Reposer `street` sur une case qui la porte déjà.<br/>
 * \tattendu Refusé sans effet ; révision et historique inchangés.
 * }
 */
TEST(LevelDraftPiecesTest, ReposerLaMemePieceNeModifiePasLaCarte) {
    LevelDraft draft = brouillon();
    const auto revision = draft.revision();

    EXPECT_FALSE(draft.placePiece(SOL, {.column = 3, .row = 2}, "street", TileType::Dirt));

    EXPECT_EQ(draft.revision(), revision);
    EXPECT_FALSE(draft.canUndo());
}

/**
 * @brief Sur une couche, deux emprises ne se recouvrent pas : poser une pièce retire, entières,
 *        celles qu'elle couvrirait.
 * \castest{<b>Une pièce retire celles qu'elle couvrirait.</b><br/>
 * \tcat Unitaire · Pièces du brouillon<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser un étal en (0, 1) et un pilier en (3, 1).<br/>2. Poser un étal en (1, 1).<br/>
 * \tattendu L'étal de (0, 1) part entier — (0, 1) repasse —, le pilier reste, le nouvel étal couvre
 * (1, 1) et (2, 1).
 * }
 */
TEST(LevelDraftPiecesTest, UnePieceRetireCellesQuElleCouvrirait) {
    LevelDraft draft = brouillon();
    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 0, .row = 1}, "stall", TileType::Wall));
    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 3, .row = 1}, "pillar", TileType::Wall));

    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 1, .row = 1}, "stall", TileType::Wall));

    EXPECT_TRUE(draft.layers()[DECOR].pieceAt(0, 1).empty());
    EXPECT_EQ(collision(draft, 0, 1), TileType::Empty);
    EXPECT_EQ(draft.layers()[DECOR].pieceAt(1, 1), "stall");
    EXPECT_EQ(draft.layers()[DECOR].pieceAt(3, 1), "pillar");
    EXPECT_EQ(collision(draft, 2, 1), TileType::Wall);
    EXPECT_EQ(collision(draft, 3, 1), TileType::Wall);
}

/**
 * @brief Une pièce dont l'emprise déborderait de la carte n'est pas posée.
 * \castest{<b>Une emprise qui déborde est refusée.</b><br/>
 * \tcat Unitaire · Pièces du brouillon<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Poser l'étal 2 × 1 sur la dernière colonne.<br/>
 * \tattendu Refusé ; rien d'empilé.
 * }
 */
TEST(LevelDraftPiecesTest, UneEmpriseQuiDebordeEstRefusee) {
    LevelDraft draft = brouillon();

    EXPECT_FALSE(draft.placePiece(DECOR, {.column = 3, .row = 0}, "stall", TileType::Wall));
    EXPECT_FALSE(draft.canUndo());
}

/**
 * @brief Le rectangle pave au pas de l'emprise, en un pas, et ne pose rien qui en déborderait.
 * \castest{<b>Le rectangle pave au pas de l'emprise.</b><br/>
 * \tcat Unitaire · Pièces du brouillon<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Paver d'étals le rectangle (0, 0)–(2, 1).<br/>
 * \tattendu Deux étals, ancrés en (0, 0) et (0, 1) ; la colonne 2 reste libre ; un pas
 * d'annulation.
 * }
 */
TEST(LevelDraftPiecesTest, LeRectanglePaveAuPasDeLEmprise) {
    LevelDraft draft = brouillon();

    ASSERT_TRUE(draft.placePieceRegion(DECOR, {.column = 0, .row = 0}, {.column = 2, .row = 1},
                                       "stall", TileType::Wall));

    EXPECT_EQ(draft.layers()[DECOR].pieceAt(0, 0), "stall");
    EXPECT_EQ(draft.layers()[DECOR].pieceAt(0, 1), "stall");
    EXPECT_TRUE(draft.layers()[DECOR].pieceAt(2, 0).empty());
    EXPECT_EQ(collision(draft, 2, 1), TileType::Empty);
    EXPECT_EQ(draft.undoDepth(), 1U);
    EXPECT_FALSE(draft.placePieceRegion(DECOR, {.column = 0, .row = 0}, {.column = 2, .row = 1},
                                        "stall", TileType::Wall));
}

/**
 * @brief Une pièce qui arrête le pas s'écrit `cliff` ; l'entrée n'est jamais recouverte par la
 *        déduction.
 * \castest{<b>Une fosse arrête le pas ; l'entrée reste l'entrée.</b><br/>
 * \tcat Unitaire · Pièces du brouillon<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser la fosse 1 × 2 en (0, 0), sur l'entrée.<br/>
 * \tattendu (0, 0) reste `entry`, (0, 1) vaut `cliff`.
 * }
 */
TEST(LevelDraftPiecesTest, UneFosseArreteLePasEtLEntreeResteLEntree) {
    LevelDraft draft = brouillon();

    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 0, .row = 0}, "pit", TileType::Cliff));

    EXPECT_EQ(collision(draft, 0, 0), TileType::Entry);
    EXPECT_EQ(collision(draft, 0, 1), TileType::Cliff);
}

/**
 * @brief Peindre un type sur une couche visuelle : la collision de la case suit, et celle d'une
 *        pièce large dont on repeint l'ancre aussi.
 * \castest{<b>Un type peint sur une couche : la collision suit.</b><br/>
 * \tcat Unitaire · Pièces du brouillon<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser un étal en (1, 1).<br/>2. Peindre `dirt` sur son ancre.<br/>3. Peindre `water`
 * profonde sur le sol en (3, 2).<br/>
 * \tattendu L'étal part, ses deux cases repassent ; l'eau profonde arrête le pas en (3, 2).
 * }
 */
TEST(LevelDraftPiecesTest, UnTypePeintSurUneCoucheLaCollisionSuit) {
    LevelDraft draft = brouillon();
    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 1, .row = 1}, "stall", TileType::Wall));

    ASSERT_TRUE(draft.paintLayerTile(DECOR, 1, 1, TileType::Dirt));
    EXPECT_EQ(collision(draft, 1, 1), TileType::Empty);
    EXPECT_EQ(collision(draft, 2, 1), TileType::Empty);

    ASSERT_TRUE(draft.paintLayerTile(SOL, 3, 2, TileType::DeepWater));
    EXPECT_EQ(collision(draft, 3, 2), TileType::Cliff);
}

/**
 * @brief Peindre la grille de collision force la case qui s'écarte de la déduction, et libère
 *        celle qui s'y accorde (`EX-EDIT-065`).
 * \castest{<b>Peindre la collision force ou libère la case.</b><br/>
 * \tcat Unitaire · Pièces du brouillon<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Peindre un mur de collision en (2, 2).<br/>2. Y repeindre du vide.<br/>
 * \tattendu Forcée après le mur ; libérée après le vide.
 * }
 */
TEST(LevelDraftPiecesTest, PeindreLaCollisionForceOuLibereLaCase) {
    LevelDraft draft = brouillon();
    const GridPosition cell{.column = 2, .row = 2};

    draft.paintTile(2, 2, TileType::Wall);
    EXPECT_TRUE(draft.isCollisionForced(cell));
    ASSERT_EQ(draft.forcedCollision().size(), 1U);

    draft.paintTile(2, 2, TileType::Empty);
    EXPECT_FALSE(draft.isCollisionForced(cell));
    EXPECT_TRUE(draft.forcedCollision().empty());
}

/**
 * @brief Une case forcée garde sa collision sous un geste de couche, jusqu'à ce qu'on la libère :
 *        elle reprend alors la déduction.
 * \castest{<b>Une case forcée tient, puis se libère.</b><br/>
 * \tcat Unitaire · Pièces du brouillon<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Forcer un mur de collision en (1, 1).<br/>2. Poser un étal en (0, 1).<br/>3. Libérer
 * la case.<br/>
 * \tattendu Le mur forcé tient sous l'étal comme sans lui ; libérée, la case prend ce que l'étal
 * donne — un mur —, et un second geste la rend au pavé quand l'étal part.
 * }
 */
TEST(LevelDraftPiecesTest, UneCaseForceeTientPuisSeLibere) {
    LevelDraft draft = brouillon();
    draft.paintTile(1, 1, TileType::Wall);
    ASSERT_TRUE(draft.isCollisionForced({.column = 1, .row = 1}));

    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 0, .row = 1}, "stall", TileType::Wall));
    EXPECT_TRUE(draft.isCollisionForced({.column = 1, .row = 1}));

    ASSERT_TRUE(draft.unforceCollision({{.column = 1, .row = 1}, {.column = 3, .row = 2}}));
    EXPECT_FALSE(draft.isCollisionForced({.column = 1, .row = 1}));
    EXPECT_EQ(collision(draft, 1, 1), TileType::Wall);

    ASSERT_TRUE(draft.eraseLayerRegion(DECOR, {.column = 0, .row = 1}, {.column = 0, .row = 1}));
    EXPECT_EQ(collision(draft, 1, 1), TileType::Empty);
    EXPECT_FALSE(draft.unforceCollision({{.column = 1, .row = 1}}));
}

/**
 * @brief Un geste ne touche la collision que des cases qu'il couvre : un écart laissé ailleurs,
 *        forcé ou non, reste tel quel.
 * \castest{<b>Un geste ne touche que ses cases.</b><br/>
 * \tcat Unitaire · Pièces du brouillon<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Charger une carte dont la collision s'écarte de la déduction en (3, 0), sans la
 * forcer.<br/>2. Poser un pilier en (1, 2).<br/>
 * \tattendu (1, 2) arrête la vue ; (3, 0) garde son mur.
 * }
 */
TEST(LevelDraftPiecesTest, UnGesteNeToucheQueSesCases) {
    std::string carte = CARTE;
    carte.replace(carte.find(R"({"x": 0, "y": 0, "type": "entry"} ])"),
                  std::string{R"({"x": 0, "y": 0, "type": "entry"} ])"}.size(),
                  R"({"x": 0, "y": 0, "type": "entry"}, {"x": 3, "y": 0, "type": "wall"} ])");
    core::LevelLoadResult loaded = core::LevelLoader::loadFromString(carte);
    ASSERT_TRUE(loaded.ok()) << loaded.error;
    LevelDraft draft = LevelDraft::fromLevel(*loaded.level);
    draft.setPieceManifest(std::make_shared<const core::ScenePieceManifest>(
        core::ScenePieceManifest::loadFromString(PLANCHE).manifest));

    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 1, .row = 2}, "pillar", TileType::Wall));

    EXPECT_EQ(collision(draft, 1, 2), TileType::Wall);
    EXPECT_EQ(collision(draft, 3, 0), TileType::Wall);
}

/**
 * @brief Remplacer une pièce renomme chacune de ses cases en un pas, et la collision suit ce que la
 *        nouvelle pièce oppose (`LOT-EDITOR-14`, `EX-EDIT-083`).
 * \castest{<b>Remplacer une pièce : un pas, la collision suit.</b><br/>
 * \tcat Unitaire · Pièces du brouillon<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser deux piliers.<br/>2. Remplacer `pillar` par `pit`, une fosse 1 × 2.<br/>
 * 3. Défaire.<br/>
 * \tattendu Les deux ancres nomment `pit`, leur type reste `wall`, la collision passe à `cliff` sur
 * l'emprise ; un seul pas d'annulation rend les piliers.
 * }
 */
TEST(LevelDraftPiecesTest, RemplacerUnePieceEnUnPasLaCollisionSuit) {
    LevelDraft draft = brouillon();
    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 1, .row = 0}, "pillar", TileType::Wall));
    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 3, .row = 0}, "pillar", TileType::Wall));
    const std::size_t depth = draft.undoDepth();

    ASSERT_TRUE(draft.replacePieces({{"pillar", "pit"}}));

    EXPECT_EQ(draft.layers()[DECOR].pieceAt(1, 0), "pit");
    EXPECT_EQ(draft.layers()[DECOR].pieceAt(3, 0), "pit");
    EXPECT_EQ(draft.layers()[DECOR].tiles.tile(1, 0), TileType::Wall);
    EXPECT_EQ(collision(draft, 1, 0), TileType::Cliff);
    EXPECT_EQ(collision(draft, 1, 1), TileType::Cliff);
    EXPECT_EQ(draft.undoDepth(), depth + 1);

    ASSERT_TRUE(draft.undo());
    EXPECT_EQ(draft.layers()[DECOR].pieceAt(1, 0), "pillar");
    EXPECT_EQ(collision(draft, 1, 1), TileType::Empty);
}

/**
 * @brief Un remplacement dont une emprise déborderait est refusé en entier, et un remplacement qui
 *        ne change rien n'empile rien.
 * \castest{<b>Un remplacement qui déborde est refusé en entier.</b><br/>
 * \tcat Unitaire · Pièces du brouillon<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser un pilier au milieu et un sur la dernière ligne.<br/>2. Remplacer `pillar` par
 * `pit` (1 × 2).<br/>3. Remplacer une pièce que la carte ne pose pas.<br/>
 * \tattendu Les deux refusés ; les piliers restent, aucun pas empilé.
 * }
 */
TEST(LevelDraftPiecesTest, UnRemplacementQuiDebordeEstRefuseEnEntier) {
    LevelDraft draft = brouillon();
    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 1, .row = 0}, "pillar", TileType::Wall));
    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 2, .row = 2}, "pillar", TileType::Wall));
    const std::size_t depth = draft.undoDepth();

    EXPECT_FALSE(draft.replacePieces({{"pillar", "pit"}}));
    EXPECT_FALSE(draft.replacePieces({{"stall", "pillar"}}));

    EXPECT_EQ(draft.layers()[DECOR].pieceAt(1, 0), "pillar");
    EXPECT_EQ(draft.undoDepth(), depth);
}

/**
 * @brief Changer de planche nomme le lieu, traduit les pièces par la table, et redéduit toute la
 *        collision par le nouveau manifeste, en un pas (`LOT-EDITOR-14`, `EX-EDIT-084`).
 * \castest{<b>Changer de planche : lieu, pièces et collision en un pas.</b><br/>
 * \tcat Unitaire · Pièces du brouillon<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser un pilier en (2, 1).<br/>2. Passer à une planche où le pavé s'appelle
 * `paving` et où le pilier, gardant son nom, se franchit.<br/>3. Défaire.<br/>
 * \tattendu Le sol nomme le lieu `autre` et pose `paving` ; le pilier garde son nom et sa case se
 * libère ; un pas d'annulation rend tout.
 * }
 */
TEST(LevelDraftPiecesTest, ChangerDePlancheTraduitLesPiecesEtRededuitLaCollision) {
    LevelDraft draft = brouillon();
    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 2, .row = 1}, "pillar", TileType::Wall));
    ASSERT_EQ(collision(draft, 2, 1), TileType::Wall);
    constexpr const char* AUTRE = R"({
      "version": 1, "disposition": "autre",
      "textures": {
        "scene/autre/paving": {"file": "paving.png", "class": "floor", "footprint": [1, 1], "tactical": "open"},
        "scene/autre/pillar": {"file": "pillar.png", "class": "tall", "footprint": [1, 1], "tactical": "open"}
      }
    })";
    auto manifest = std::make_shared<const core::ScenePieceManifest>(
        core::ScenePieceManifest::loadFromString(AUTRE).manifest);

    ASSERT_TRUE(draft.changeScene("autre", manifest, {{"street", "paving"}}));

    EXPECT_EQ(std::get<std::string>(draft.layers()[SOL].properties.at("scene")), "autre");
    EXPECT_EQ(draft.layers()[SOL].pieceAt(3, 2), "paving");
    EXPECT_EQ(draft.layers()[DECOR].pieceAt(2, 1), "pillar");
    EXPECT_EQ(collision(draft, 2, 1), TileType::Empty);

    ASSERT_TRUE(draft.undo());
    EXPECT_EQ(draft.layers()[SOL].properties.count("scene"), 0U);
    EXPECT_EQ(draft.layers()[SOL].pieceAt(3, 2), "street");
    EXPECT_EQ(collision(draft, 2, 1), TileType::Wall);
}

/**
 * @brief Un étage ne compte pas dans la collision : mettre une couche à l'étage libère ses cases,
 *        une pièce posée à l'étage n'arrête rien (`LOT-129`, `EX-LVL-025`).
 * \castest{<b>Un etage ne bloque aucune case.</b><br/>
 * \tcat Unitaire · Pieces du brouillon · Etages<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Poser un pilier en (2, 1) du decor, qui arrete la vue.<br/>
 *          2. Mettre le decor a l'etage 1, puis defaire.<br/>
 *          3. A l'etage 1, poser un pilier en (1, 1).<br/>
 * \tattendu A l'etage, la case (2, 1) redevient libre ; defaire rend le mur ; le pilier de l'etage
 *           laisse sa case libre.
 * }
 */
TEST(LevelDraftPiecesTest, UnEtageNeBloqueAucuneCase) {
    LevelDraft draft = brouillon();
    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 2, .row = 1}, "pillar", TileType::Wall));
    ASSERT_EQ(collision(draft, 2, 1), TileType::Wall);

    ASSERT_TRUE(draft.setLayerFloor(DECOR, 1));
    EXPECT_EQ(draft.layers()[DECOR].floor, 1);
    EXPECT_EQ(collision(draft, 2, 1), TileType::Empty);

    ASSERT_TRUE(draft.undo());
    EXPECT_EQ(draft.layers()[DECOR].floor, 0);
    EXPECT_EQ(collision(draft, 2, 1), TileType::Wall);

    ASSERT_TRUE(draft.setLayerFloor(DECOR, 1));
    ASSERT_TRUE(draft.placePiece(DECOR, {.column = 1, .row = 1}, "pillar", TileType::Wall));
    EXPECT_EQ(collision(draft, 1, 1), TileType::Empty);
}

/**
 * @brief Seule une couche de décor monte, et pas au-delà du dernier étage ; un étage inchangé
 *        n'empile rien.
 * \castest{<b>Seul un decor monte, de 1 au dernier etage.</b><br/>
 * \tcat Unitaire · Pieces du brouillon · Etages<br/>
 * \tcrit Majeur<br/>
 * \tetapes Demander l'etage 1 pour le sol, l'etage MAX_STOREY_FLOOR + 1 et l'etage -1 pour le
 *          decor, puis l'etage 0 qu'il a deja.<br/>
 * \tattendu Quatre refus, et aucun pas d'annulation.
 * }
 */
TEST(LevelDraftPiecesTest, SeulUnDecorMonteEtPasAuDelaDuDernierEtage) {
    LevelDraft draft = brouillon();
    EXPECT_FALSE(draft.setLayerFloor(SOL, 1));
    EXPECT_FALSE(draft.setLayerFloor(DECOR, core::MAX_STOREY_FLOOR + 1));
    EXPECT_FALSE(draft.setLayerFloor(DECOR, -1));
    EXPECT_FALSE(draft.setLayerFloor(DECOR, 0));
    EXPECT_EQ(draft.undoDepth(), 0U);
}
