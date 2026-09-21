// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_brush_gesture.cpp
 * @brief Tests des coups de pinceau — type, pièce, gomme — sur le brouillon (`LOT-EDITOR-03`,
 *        `EX-EDIT-064`, `EX-EDIT-065`), dont l'acceptation du lot sur la carte d'essai.
 */

#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileType.h"
#include "Editor/Logic/BrushGesture.h"
#include "Editor/Logic/LayerView.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/PieceCatalog.h"

namespace {

using core::GridPosition;
using core::LevelDraft;
using core::TileType;
using hmi::BrushKind;
using hmi::CanvasBrush;

// La racine d'essai de l'éditeur (`LOT-123`) : ces gestes se jouaient sur une carte LIVRÉE,
// que la table rase du `LOT-102` emporte. La carte d'essai a la même géométrie.
[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path(JADG_EDITOR_DATA_DIR);
}

[[nodiscard]] std::string lire(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream text;
    text << file.rdbuf();
    return text.str();
}

/// La carte d'essai, telle que livrée, avec le manifeste et la table de son lieu.
struct CarteDEssai {
    std::string fichier;
    hmi::PlaceAssets lieu;
    LevelDraft draft;
};

[[nodiscard]] CarteDEssai carteDEssai() {
    const std::filesystem::path path = dataRoot() / "Levels" / "bourg" / "place.json";
    core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(path);
    EXPECT_TRUE(loaded.ok()) << loaded.error;
    CarteDEssai carte{.fichier = lire(path),
                      .lieu = hmi::loadPlaceAssets(dataRoot(), "bourg"),
                      .draft = LevelDraft::fromLevel(*loaded.level)};
    EXPECT_TRUE(carte.lieu.manifest.has_value());
    EXPECT_TRUE(carte.lieu.appearance.has_value());
    carte.draft.setPieceManifest(
        std::make_shared<const core::ScenePieceManifest>(*carte.lieu.manifest));
    return carte;
}

// Le pinceau de la pièce @p piece, son type tiré de la table du lieu comme au canevas.
[[nodiscard]] CanvasBrush pinceau(const CarteDEssai& carte, const std::string& piece, bool floor) {
    return CanvasBrush{.kind = BrushKind::Piece,
                       .type = hmi::pieceCellType(&*carte.lieu.appearance, piece, floor),
                       .piece = piece,
                       .floor = floor};
}

// La première case (ligne par ligne) de la couche @p index qui nomme @p piece.
[[nodiscard]] std::optional<GridPosition> premiereCase(const LevelDraft& draft, std::size_t index,
                                                       std::string_view piece) {
    const core::TileLayer& layer = draft.layers()[index];
    for (int row = 0; row < layer.tiles.height(); ++row) {
        for (int column = 0; column < layer.tiles.width(); ++column) {
            if (layer.pieceAt(column, row) == piece) {
                return GridPosition{.column = column, .row = row};
            }
        }
    }
    return std::nullopt;
}

const CanvasBrush GOMME{.kind = BrushKind::Eraser, .type = {}, .piece = {}, .floor = false};

}  // namespace

/**
 * @brief Repeindre une rue et une façade de la carte d'essai, sans toucher à la collision, rend un
 *        fichier identique à l'original (acceptation du `LOT-EDITOR-03`).
 * \castest{<b>Repeindre une rue et une façade rend le même fichier.</b><br/>
 * \tcat Unitaire · Pinceau<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ouvrir la carte d essai.<br/>2. Gommer une case de rue (`street-2`) et un pan de façade
 * (`window-left`), puis les reposer du pinceau de pièce, le décor actif.<br/>3. Écrire le
 * brouillon.<br/>
 * \tattendu Gommées, la rue devient un mur (rien sous les pieds) et la façade libère sa case ;
 * reposées, le texte écrit est celui du fichier livré, octet pour octet.
 * }
 */
TEST(BrushGestureTest, RepeindreUneRueEtUneFacadeRendLeMemeFichier) {
    CarteDEssai carte = carteDEssai();
    const std::size_t sol = *hmi::pieceTargetLayer(carte.draft.layers(), true);
    const std::size_t decor = *hmi::pieceTargetLayer(carte.draft.layers(), false);
    const std::optional<GridPosition> rue = premiereCase(carte.draft, sol, "street-2");
    const std::optional<GridPosition> facade = premiereCase(carte.draft, decor, "window-left");
    ASSERT_TRUE(rue && facade);
    ASSERT_EQ(carte.draft.toJson(), carte.fichier);
    const hmi::LayerViewState vue;

    EXPECT_TRUE(hmi::applyBrush(carte.draft, GOMME, sol, vue, *rue, *rue).changed);
    EXPECT_TRUE(hmi::applyBrush(carte.draft, GOMME, decor, vue, *facade, *facade).changed);
    EXPECT_EQ(carte.draft.tileMap().tile(rue->column, rue->row), TileType::Wall);
    EXPECT_NE(carte.draft.toJson(), carte.fichier);

    EXPECT_TRUE(
        hmi::applyBrush(carte.draft, pinceau(carte, "street-2", true), decor, vue, *rue, *rue)
            .changed);
    EXPECT_TRUE(hmi::applyBrush(carte.draft, pinceau(carte, "window-left", false), decor, vue,
                                *facade, *facade)
                    .changed);
    EXPECT_EQ(carte.draft.toJson(), carte.fichier);
}

/**
 * @brief Poser puis gommer un étal 2 × 1 sur la place de la carte d'essai occupe puis libère ses deux
 *        cases, collision comprise (acceptation du `LOT-EDITOR-03`).
 * \castest{<b>Un étal 2 × 1 posé puis gommé sur la carte d'essai.</b><br/>
 * \tcat Unitaire · Pinceau<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ouvrir la carte d essai.<br/>2. Poser `feature-1` (2 × 1) sur deux cases libres de la
 * place.<br/>3. Le gommer par sa deuxième case.<br/>
 * \tattendu Posé, ses deux cases arrêtent la vue ; gommé, elles repassent, et le brouillon écrit
 * le fichier livré.
 * }
 */
TEST(BrushGestureTest, PoserPuisGommerUnEtalSurLaCarteDEssai) {
    CarteDEssai carte = carteDEssai();
    const std::size_t sol = *hmi::pieceTargetLayer(carte.draft.layers(), true);
    const std::size_t decor = *hmi::pieceTargetLayer(carte.draft.layers(), false);
    // Deux cases voisines de la place, sans relief.
    std::optional<GridPosition> place;
    const core::TileLayer& couche = carte.draft.layers()[sol];
    for (int row = 0; row < couche.tiles.height() && !place; ++row) {
        for (int column = 0; column + 1 < couche.tiles.width() && !place; ++column) {
            const bool libres =
                couche.pieceAt(column, row) == "square" &&
                couche.pieceAt(column + 1, row) == "square" &&
                !carte.draft.pieceAnchorAt(decor, {.column = column, .row = row}) &&
                !carte.draft.pieceAnchorAt(decor, {.column = column + 1, .row = row});
            if (libres) {
                place = GridPosition{.column = column, .row = row};
            }
        }
    }
    ASSERT_TRUE(place);
    const GridPosition seconde{.column = place->column + 1, .row = place->row};
    const hmi::LayerViewState vue;

    ASSERT_TRUE(hmi::applyBrush(carte.draft, pinceau(carte, "feature-1", false), std::nullopt, vue,
                                *place, *place)
                    .changed);
    EXPECT_EQ(carte.draft.layers()[decor].pieceAt(place->column, place->row), "feature-1");
    EXPECT_EQ(carte.draft.tileMap().tile(place->column, place->row), TileType::Wall);
    EXPECT_EQ(carte.draft.tileMap().tile(seconde.column, seconde.row), TileType::Wall);

    ASSERT_TRUE(hmi::applyBrush(carte.draft, GOMME, decor, vue, seconde, seconde).changed);
    EXPECT_EQ(carte.draft.tileMap().tile(place->column, place->row), TileType::Empty);
    EXPECT_EQ(carte.draft.tileMap().tile(seconde.column, seconde.row), TileType::Empty);
    EXPECT_EQ(carte.draft.toJson(), carte.fichier);
}

/**
 * @brief Glisser une pièce large ne la décale pas d'une case à chaque pas : une case que la même
 *        pièce couvre déjà ne la reçoit pas une seconde fois.
 * \castest{<b>Glisser un étal ne le décale pas.</b><br/>
 * \tcat Unitaire · Pinceau<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser `feature-1` au clic.<br/>2. Prolonger le glisser sur sa deuxième case.<br/>
 * \tattendu Le second pas ne change rien ; l'étal reste à son ancre.
 * }
 */
TEST(BrushGestureTest, GlisserUnEtalNeLeDecalePas) {
    CarteDEssai carte = carteDEssai();
    const std::size_t decor = *hmi::pieceTargetLayer(carte.draft.layers(), false);
    const hmi::LayerViewState vue;
    const GridPosition ancre{.column = 20, .row = 20};
    const GridPosition seconde{.column = 21, .row = 20};

    ASSERT_TRUE(
        hmi::applyBrush(carte.draft, pinceau(carte, "feature-1", false), decor, vue, ancre, ancre)
            .changed);
    EXPECT_FALSE(hmi::applyBrush(carte.draft, pinceau(carte, "feature-1", false), decor, vue,
                                 seconde, seconde, true)
                     .changed);
    EXPECT_EQ(carte.draft.layers()[decor].pieceAt(ancre.column, ancre.row), "feature-1");
}

/**
 * @brief Une couche verrouillée refuse le pinceau, et le refus se dit ; une pièce sans couche où
 *        aller aussi.
 * \castest{<b>Couche verrouillée ou absente : le geste est refusé.</b><br/>
 * \tcat Unitaire · Pinceau<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Verrouiller le décor, poser une pièce debout.<br/>2. Poser une pièce debout sur une
 * carte sans décor.<br/>
 * \tattendu Deux refus, chacun avec sa raison ; rien d'empilé.
 * }
 */
TEST(BrushGestureTest, CoucheVerrouilleeOuAbsenteLeGesteEstRefuse) {
    CarteDEssai carte = carteDEssai();
    const std::size_t decor = *hmi::pieceTargetLayer(carte.draft.layers(), false);
    hmi::LayerViewState vue;
    vue.sync(carte.draft.layers().size());
    vue.setLocked(decor, true);

    const hmi::BrushResult verrou =
        hmi::applyBrush(carte.draft, pinceau(carte, "light", false), std::nullopt, vue,
                        {.column = 20, .row = 20}, {.column = 20, .row = 20});
    EXPECT_FALSE(verrou.changed);
    EXPECT_EQ(verrou.refusal, "The decor layer is locked.");

    LevelDraft plate = LevelDraft::empty("plate", 4, 4);
    const hmi::BrushResult absente =
        hmi::applyBrush(plate, pinceau(carte, "light", false), std::nullopt, hmi::LayerViewState{},
                        {.column = 1, .row = 1}, {.column = 1, .row = 1});
    EXPECT_FALSE(absente.changed);
    EXPECT_NE(absente.refusal.find("no decor layer"), std::string::npos);
    EXPECT_FALSE(carte.draft.canUndo());
}

/**
 * @brief Sur la collision, peindre force la case ; la gomme la libère et la rend à la déduction.
 * \castest{<b>Sur la collision, le pinceau force et la gomme libère.</b><br/>
 * \tcat Unitaire · Pinceau<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Peindre un mur sur la collision d'une case de rue.<br/>2. La gommer, la collision
 * active.<br/>
 * \tattendu Forcée puis libérée ; le fichier écrit redevient celui livré.
 * }
 */
TEST(BrushGestureTest, SurLaCollisionLePinceauForceEtLaGommeLibere) {
    CarteDEssai carte = carteDEssai();
    const std::size_t sol = *hmi::pieceTargetLayer(carte.draft.layers(), true);
    const GridPosition rue = *premiereCase(carte.draft, sol, "street");
    const hmi::LayerViewState vue;
    const CanvasBrush mur{
        .kind = BrushKind::Type, .type = TileType::Wall, .piece = {}, .floor = false};

    ASSERT_TRUE(hmi::applyBrush(carte.draft, mur, std::nullopt, vue, rue, rue).changed);
    EXPECT_TRUE(carte.draft.isCollisionForced(rue));

    ASSERT_TRUE(hmi::applyBrush(carte.draft, GOMME, std::nullopt, vue, rue, rue).changed);
    EXPECT_FALSE(carte.draft.isCollisionForced(rue));
    EXPECT_EQ(carte.draft.toJson(), carte.fichier);
}

/**
 * @brief Un type se peint sur la couche active ; l'entrée ne se peint pas sur une couche visuelle.
 * \castest{<b>Un type va sur la couche active, jamais l'entrée.</b><br/>
 * \tcat Unitaire · Pinceau<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Peindre `entry` sur le sol de la carte d'essai.<br/>
 * \tattendu Refusé : l'entrée vit dans la grille de collision.
 * }
 */
TEST(BrushGestureTest, UnTypeVaSurLaCoucheActiveJamaisLEntree) {
    CarteDEssai carte = carteDEssai();
    const std::size_t sol = *hmi::pieceTargetLayer(carte.draft.layers(), true);
    const CanvasBrush entree{
        .kind = BrushKind::Type, .type = TileType::Entry, .piece = {}, .floor = false};

    const hmi::BrushResult result =
        hmi::applyBrush(carte.draft, entree, sol, hmi::LayerViewState{}, {.column = 5, .row = 5},
                        {.column = 5, .row = 5});

    EXPECT_FALSE(result.changed);
    EXPECT_NE(result.refusal.find("entry lives in the collision grid"), std::string::npos);
}

/**
 * @brief La barre d'état nomme le pinceau armé.
 * \castest{<b>Le pinceau se nomme.</b><br/>
 * \tcat Unitaire · Pinceau<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Nommer un pinceau de type, de pièce, la gomme.<br/>
 * \tattendu `grass`, `wall-left`, `Eraser`.
 * }
 */
TEST(BrushGestureTest, LePinceauSeNomme) {
    EXPECT_EQ(hmi::brushLabel(
                  {.kind = BrushKind::Type, .type = TileType::Grass, .piece = {}, .floor = false}),
              "grass");
    EXPECT_EQ(hmi::brushLabel({.kind = BrushKind::Piece,
                               .type = TileType::Wall,
                               .piece = "wall-left",
                               .floor = false}),
              "wall-left");
    EXPECT_EQ(hmi::brushLabel(GOMME), "Eraser");
}
