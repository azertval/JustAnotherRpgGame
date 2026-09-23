// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_piece_catalog.cpp
 * @brief Tests du catalogue des pièces d'un lieu, que montre la palette (`LOT-EDITOR-03`,
 *        `EX-EDIT-063`).
 */

#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileType.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "Editor/Logic/PieceCatalog.h"
#include "HMI/Graphics/PlaceAppearance.h"

namespace {

constexpr const char* PLANCHE = R"({
  "version": 1, "disposition": "essai",
  "textures": {
    "scene/essai/street":   {"file": "street.png", "class": "floor", "footprint": [1, 1], "tactical": "open"},
    "scene/essai/wall":     {"file": "wall.png", "class": "tall", "footprint": [1, 1], "aliases": ["old-wall"]},
    "scene/essai/stall":    {"file": "stall.png", "class": "wide", "footprint": [2, 1]},
    "scene/essai/flag":     {"file": "flag.png", "class": "banner", "footprint": [1, 1]},
    "scene/essai/street-2": {"file": "street-2.png", "class": "floor", "footprint": [1, 1], "tactical": "open"}
  }
})";

// Une carte qui cite deux pièces que la planche n'a pas, une au sol, une au décor, et une par un
// ancien nom (qui, elle, n'est pas absente).
constexpr const char* CARTE = R"({
  "version": 4, "width": 3, "height": 1,
  "tiles": [ {"x": 0, "y": 0, "type": "entry"} ],
  "layers": [
    {"kind": "ground", "tiles": [ {"x": 0, "y": 0, "type": "dirt", "piece": "cobble"} ]},
    {"kind": "decor", "tiles": [ {"x": 1, "y": 0, "type": "wall", "piece": "fountain"},
                                 {"x": 2, "y": 0, "type": "wall", "piece": "old-wall"} ]}
  ]
})";

[[nodiscard]] core::ScenePieceManifest planche() {
    core::ScenePieceManifestResult read = core::ScenePieceManifest::loadFromString(PLANCHE);
    EXPECT_TRUE(read.ok()) << read.message;
    return read.manifest;
}

[[nodiscard]] std::vector<core::TileLayer> couches() {
    core::LevelLoadResult loaded = core::LevelLoader::loadFromString(CARTE);
    EXPECT_TRUE(loaded.ok()) << loaded.error;
    return loaded.level->layers();
}

[[nodiscard]] std::vector<std::string> noms(const hmi::PieceCatalogGroup& group) {
    std::vector<std::string> names;
    for (const hmi::PieceCatalogEntry& entry : group.pieces) {
        names.push_back(entry.name);
    }
    return names;
}

}  // namespace

/**
 * @brief Le catalogue range les pièces par classe, dans l'ordre du manifeste, puis liste à part
 *        celles que la carte cite et que la planche n'a pas.
 * \castest{<b>Le catalogue groupe par classe et garde les absentes.</b><br/>
 * \tcat Unitaire · Palette des pièces<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Construire le catalogue d'une planche de cinq pièces pour une carte qui cite deux
 * pièces inconnues.<br/>
 * \tattendu Groupes Floors, Standing, Wide, Other, puis « Missing from the sheet » avec `cobble`
 * (au sol) et `fountain` (au décor) ; `old-wall`, ancien nom de `wall`, n'y est pas.
 * }
 */
TEST(PieceCatalogTest, LeCatalogueGroupeParClasseEtGardeLesAbsentes) {
    const core::ScenePieceManifest manifest = planche();
    const std::vector<hmi::PieceCatalogGroup> catalog = hmi::pieceCatalog(&manifest, couches());

    ASSERT_EQ(catalog.size(), 5U);
    EXPECT_EQ(catalog[0].label, "Floors");
    EXPECT_EQ(noms(catalog[0]), (std::vector<std::string>{"street", "street-2"}));
    EXPECT_TRUE(catalog[0].pieces[0].floor);
    EXPECT_EQ(catalog[1].label, "Standing");
    EXPECT_EQ(catalog[2].label, "Wide");
    EXPECT_EQ(catalog[2].pieces[0].footprint, (core::PieceFootprint{.columns = 2, .rows = 1}));
    EXPECT_FALSE(catalog[2].pieces[0].floor);
    EXPECT_EQ(catalog[3].label, "Other");
    EXPECT_EQ(catalog[4].label, hmi::MISSING_PIECES_GROUP);
    EXPECT_EQ(noms(catalog[4]), (std::vector<std::string>{"cobble", "fountain"}));
    EXPECT_TRUE(catalog[4].pieces[0].missing);
    EXPECT_TRUE(catalog[4].pieces[0].floor);
    EXPECT_FALSE(catalog[4].pieces[1].floor);
}

/**
 * @brief Sans lieu, le catalogue ne garde que les pièces citées : elles restent visibles et
 *        posables, en damier.
 * \castest{<b>Sans lieu, seules les pièces citées restent.</b><br/>
 * \tcat Unitaire · Palette des pièces<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Construire le catalogue sans manifeste.<br/>
 * \tattendu Un seul groupe, les trois noms cités.
 * }
 */
TEST(PieceCatalogTest, SansLieuSeulesLesPiecesCiteesRestent) {
    const std::vector<hmi::PieceCatalogGroup> catalog = hmi::pieceCatalog(nullptr, couches());

    ASSERT_EQ(catalog.size(), 1U);
    EXPECT_EQ(noms(catalog[0]), (std::vector<std::string>{"cobble", "fountain", "old-wall"}));
    EXPECT_TRUE(hmi::pieceCatalog(nullptr, {}).empty());
}

/**
 * @brief La recherche garde les pièces dont le nom ou la classe contient le texte, sans égard à
 *        la casse, et retire les groupes vidés.
 * \castest{<b>La recherche filtre par nom et par classe.</b><br/>
 * \tcat Unitaire · Palette des pièces<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Chercher `STREET`, puis `wide`, puis rien.<br/>
 * \tattendu Les deux pavés ; l'étal ; tout.
 * }
 */
TEST(PieceCatalogTest, LaRechercheFiltreParNomEtParClasse) {
    const core::ScenePieceManifest manifest = planche();
    const std::vector<hmi::PieceCatalogGroup> catalog = hmi::pieceCatalog(&manifest, {});

    const std::vector<hmi::PieceCatalogGroup> streets = hmi::filterPieceCatalog(catalog, "STREET");
    ASSERT_EQ(streets.size(), 1U);
    EXPECT_EQ(noms(streets[0]), (std::vector<std::string>{"street", "street-2"}));

    const std::vector<hmi::PieceCatalogGroup> wide = hmi::filterPieceCatalog(catalog, "wide");
    ASSERT_EQ(wide.size(), 1U);
    EXPECT_EQ(noms(wide[0]), (std::vector<std::string>{"stall"}));

    EXPECT_EQ(hmi::filterPieceCatalog(catalog, ""), catalog);
    EXPECT_TRUE(hmi::filterPieceCatalog(catalog, "nothing like it").empty());
}

/**
 * @brief La bulle d'aide dit la classe, l'emprise et le type tactique d'une pièce.
 * \castest{<b>La bulle d'aide décrit la pièce.</b><br/>
 * \tcat Unitaire · Palette des pièces<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Décrire l'étal.<br/>
 * \tattendu `stall — wide, 2 × 1, solid`.
 * }
 */
TEST(PieceCatalogTest, LaBulleDAideDecritLaPiece) {
    const core::ScenePieceManifest manifest = planche();
    const std::vector<hmi::PieceCatalogGroup> catalog = hmi::pieceCatalog(&manifest, {});

    EXPECT_EQ(hmi::pieceDescription(catalog[2].pieces[0]), "stall — wide, 2 × 1, solid");
}

/**
 * @brief Une pièce vise sa couche : un sol la première couche de sol, le reste la première de
 *        décor.
 * \castest{<b>Une pièce vise sa couche.</b><br/>
 * \tcat Unitaire · Palette des pièces<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Chercher la couche d'un sol, puis d'une pièce debout, puis sans couche de décor.<br/>
 * \tattendu Rangs 1 et 2 (la collision est en tête) ; rien sans décor.
 * }
 */
TEST(PieceCatalogTest, UnePieceViseSaCouche) {
    const std::vector<core::TileLayer> layers = couches();

    EXPECT_EQ(hmi::pieceTargetLayer(layers, true), std::optional<std::size_t>{1});
    EXPECT_EQ(hmi::pieceTargetLayer(layers, false), std::optional<std::size_t>{2});
    const std::vector<core::TileLayer> solSeul(layers.begin(), layers.begin() + 2);
    EXPECT_EQ(hmi::pieceTargetLayer(solSeul, false), std::nullopt);
}

/**
 * @brief Le type d'une pièce posée est celui dont la table du lieu la tire ; à défaut, `wall` pour
 *        une pièce debout et le vide pour un sol.
 * \castest{<b>Le type d'une pièce vient de la table du lieu.</b><br/>
 * \tcat Unitaire · Palette des pièces<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander le type de `street-2`, de `stall`, d'un sol inconnu, et de `old-wall`
 * (ancien nom).<br/>
 * \tattendu `dirt` ; `wall` (défaut debout) ; vide ; `solid`, comme `wall`.
 * }
 */
TEST(PieceCatalogTest, LeTypeDUnePieceVientDeLaTableDuLieu) {
    hmi::PlaceAppearanceResult table = hmi::PlaceAppearance::loadFromString(R"({
      "version": 1, "place": "essai",
      "floors": {"dirt": ["street", "street-2"]},
      "relief": {"solid": ["wall"]}
    })");
    ASSERT_TRUE(table.ok()) << table.message;
    table.appearance.adoptManifest(planche());

    EXPECT_EQ(hmi::pieceCellType(&table.appearance, "street-2", true), core::TileType::Dirt);
    EXPECT_EQ(hmi::pieceCellType(&table.appearance, "stall", false), core::TileType::Wall);
    EXPECT_EQ(hmi::pieceCellType(&table.appearance, "cobble", true), core::TileType::Empty);
    EXPECT_EQ(hmi::pieceCellType(&table.appearance, "old-wall", false), core::TileType::Solid);
    EXPECT_EQ(hmi::pieceCellType(nullptr, "street", true), core::TileType::Empty);
}

/**
 * @brief Un kit rangé en sous-dossiers se groupe par dossier dans la palette ; ses pièces à plat
 *        gardent leur groupe de classe (`LOT-129`).
 * \castest{<b>La palette se groupe par dossier du kit.</b><br/>
 * \tcat Unitaire · Editeur · Palette<br/>
 * \tcrit Majeur<br/>
 * \tetapes Lire le catalogue d'un manifeste dont des toits et un sol sont ranges en sous-dossiers,
 *          un tonneau a plat.<br/>
 * \tattendu « Standing » pour le tonneau, puis « floors », « roofs/l/d3 », « roofs/t/d2 » dans
 *           l'ordre alphabetique, chacun avec ses pieces.
 * }
 */
TEST(PieceCatalogTest, UnKitRangeSeGroupeParDossier) {
    const core::ScenePieceManifestResult read = core::ScenePieceManifest::loadFromString(R"({
      "version": 1, "disposition": "ville",
      "textures": {
        "scene/ville/roof-t-d2-n-c0r0": {"file": "roofs/t/d2/roof-t-d2-n-c0r0.png", "class": "tall"},
        "scene/ville/floor-paving-01": {"file": "floors/floor-paving-01.png", "class": "floor"},
        "scene/ville/roof-l-d3-ne-c0r0": {"file": "roofs/l/d3/roof-l-d3-ne-c0r0.png", "class": "tall"},
        "scene/ville/roof-l-d3-ne-c1r0": {"file": "roofs/l/d3/roof-l-d3-ne-c1r0.png", "class": "tall"},
        "scene/ville/prop-barrel": {"file": "prop-barrel.png", "class": "tall"}
      }
    })");
    ASSERT_TRUE(read.ok()) << read.message;
    const std::vector<hmi::PieceCatalogGroup> catalog = hmi::pieceCatalog(&read.manifest, {});
    std::vector<std::string> labels;
    for (const hmi::PieceCatalogGroup& group : catalog) {
        labels.push_back(group.label);
    }
    EXPECT_EQ(labels, (std::vector<std::string>{"Standing", "floors", "roofs/l/d3", "roofs/t/d2"}));
    EXPECT_EQ(catalog[2].pieces.size(), 2U);
    EXPECT_TRUE(catalog[1].pieces.front().floor);
}
