// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_paint_tools.cpp
 * @brief Tests des outils du peintre — ligne, seau, pipette, miroir, mesure — et de
 *        l'acceptation du `LOT-EDITOR-04` : une maison de la carte d'essai en moins de dix gestes.
 */

#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/CollisionDerivation.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileType.h"
#include "Editor/Logic/BrushGesture.h"
#include "Editor/Logic/LayerView.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/PaintTools.h"
#include "Editor/Logic/PieceCatalog.h"

namespace {

using core::GridPosition;
using core::LevelDraft;
using core::TileType;
using hmi::BrushKind;
using hmi::CanvasBrush;

// La racine d'essai de l'editeur (LOT-123) : ces gestes se jouaient sur une carte LIVREE, que
// la table rase du LOT-102 emporte. La carte d'essai a la meme geometrie.
[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path(JADG_TEST_DATA_DIR);
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
    std::size_t sol = 0;
    std::size_t decor = 0;

    [[nodiscard]] hmi::StrokeContext contexte(std::optional<hmi::MirrorAxis> miroir = {}) const {
        return hmi::StrokeContext{.mirror = miroir, .appearance = &*lieu.appearance};
    }
    [[nodiscard]] CanvasBrush piece(const std::string& nom, bool floor) const {
        return CanvasBrush{.kind = BrushKind::Piece,
                           .type = hmi::pieceCellType(&*lieu.appearance, nom, floor),
                           .piece = nom,
                           .floor = floor};
    }
    [[nodiscard]] std::string_view pieceEn(std::size_t couche, int column, int row) const {
        return draft.layers()[couche].pieceAt(column, row);
    }
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
    carte.sol = *hmi::pieceTargetLayer(carte.draft.layers(), true);
    carte.decor = *hmi::pieceTargetLayer(carte.draft.layers(), false);
    return carte;
}

const CanvasBrush GOMME{.kind = BrushKind::Eraser, .type = {}, .piece = {}, .floor = false};

/// Vrai si la collision écrite de chaque case égale la déduction, hors entrée et cases forcées.
[[nodiscard]] bool collisionSuitLesPieces(const LevelDraft& draft) {
    const core::CollisionDerivation derived = core::deriveCollision(
        draft.layers(), draft.tileMap().width(), draft.tileMap().height(), draft.pieceManifest());
    for (int row = 0; row < draft.tileMap().height(); ++row) {
        for (int column = 0; column < draft.tileMap().width(); ++column) {
            const GridPosition cell{.column = column, .row = row};
            if (draft.entry() == cell || draft.isCollisionForced(cell)) {
                continue;
            }
            if (core::canonicalCollisionTile(draft.tileMap().tile(column, row)) !=
                derived.collision.tile(column, row)) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace

/**
 * @brief La ligne suit Bresenham : une case par pas sur l'axe dominant, bornes incluses.
 * \castest{<b>La ligne pose une case par pas.</b><br/>
 * \tcat Unitaire · Outils du peintre<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Tracer une ligne droite, une diagonale, une pente raide, un point.<br/>
 * \tattendu Les cases attendues, dans l'ordre du tracé.
 * }
 */
TEST(PaintToolsTest, LaLignePoseUneCaseParPas) {
    using Cells = std::vector<GridPosition>;
    EXPECT_EQ(hmi::lineCells({.column = 1, .row = 2}, {.column = 4, .row = 2}),
              (Cells{{1, 2}, {2, 2}, {3, 2}, {4, 2}}));
    EXPECT_EQ(hmi::lineCells({.column = 3, .row = 3}, {.column = 1, .row = 1}),
              (Cells{{3, 3}, {2, 2}, {1, 1}}));
    EXPECT_EQ(hmi::lineCells({.column = 0, .row = 0}, {.column = 1, .row = 3}),
              (Cells{{0, 0}, {0, 1}, {1, 2}, {1, 3}}));
    EXPECT_EQ(hmi::lineCells({.column = 5, .row = 5}, {.column = 5, .row = 5}), (Cells{{5, 5}}));
}

/**
 * @brief Un trait de plusieurs cases, puis une ligne, se défont chacun en un pas.
 * \castest{<b>Un trait est un seul pas d'annulation.</b><br/>
 * \tcat Unitaire · Outils du peintre<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Sur La carte d'essai, tracer une ligne de `light` sur six cases de rue.<br/>2.
 * Annuler.<br/>
 * \tattendu Six lanternes, un pas d'annulation ; annulé, le fichier livré revient.
 * }
 */
TEST(PaintToolsTest, UnTraitEstUnSeulPas) {
    CarteDEssai carte = carteDEssai();
    const std::vector<GridPosition> ligne =
        hmi::lineCells({.column = 20, .row = 3}, {.column = 25, .row = 3});
    const hmi::BrushResult result =
        hmi::applyStroke(carte.draft, carte.piece("light", false), std::nullopt,
                         hmi::LayerViewState{}, ligne, false, carte.contexte());
    ASSERT_TRUE(result.changed);
    for (const GridPosition cell : ligne) {
        EXPECT_EQ(carte.pieceEn(carte.decor, cell.column, cell.row), "light");
    }
    EXPECT_EQ(carte.draft.undoDepth(), 1U);
    ASSERT_TRUE(carte.draft.undo());
    EXPECT_EQ(carte.draft.toJson(), carte.fichier);
}

/**
 * @brief Le seau remplit les cases reliées de même contenu, et seulement elles, en un pas.
 * \castest{<b>Le seau remplit une région, en un pas.</b><br/>
 * \tcat Unitaire · Outils du peintre<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Sur La carte d'essai, verser `square` sur une case de la place, le sol actif… puis
 * `street-3` sur la place.<br/>2. Annuler.<br/>
 * \tattendu Toute la place (et rien d'autre) devient `street-3`, collision suivie ; un pas ;
 * annulé, le fichier livré revient.
 * }
 */
TEST(PaintToolsTest, LeSeauRemplitUneRegionEnUnPas) {
    CarteDEssai carte = carteDEssai();
    const GridPosition graine{.column = 20, .row = 16};
    ASSERT_EQ(carte.pieceEn(carte.sol, graine.column, graine.row), "square");
    const std::vector<GridPosition> place = hmi::floodRegion(carte.draft, carte.sol, graine);
    int carres = 0;
    for (int row = 0; row < carte.draft.tileMap().height(); ++row) {
        for (int column = 0; column < carte.draft.tileMap().width(); ++column) {
            carres += carte.pieceEn(carte.sol, column, row) == "square" ? 1 : 0;
        }
    }
    EXPECT_EQ(static_cast<int>(place.size()), carres);  // la place est d'un seul tenant.

    const hmi::BrushResult result =
        hmi::applyBucket(carte.draft, carte.piece("street-3", true), carte.sol,
                         hmi::LayerViewState{}, graine, carte.contexte());
    ASSERT_TRUE(result.changed);
    for (const GridPosition cell : place) {
        EXPECT_EQ(carte.pieceEn(carte.sol, cell.column, cell.row), "street-3");
    }
    EXPECT_EQ(carte.pieceEn(carte.sol, 0, 3), "street");  // hors de la place : intact.
    EXPECT_TRUE(collisionSuitLesPieces(carte.draft));
    EXPECT_EQ(carte.draft.undoDepth(), 1U);
    ASSERT_TRUE(carte.draft.undo());
    EXPECT_EQ(carte.draft.toJson(), carte.fichier);
}

/**
 * @brief Le seau refuse une pièce large, et une case couverte par une pièce large n'est égale
 *        qu'aux cases de cette pièce.
 * \castest{<b>Le seau et les pièces larges.</b><br/>
 * \tcat Unitaire · Outils du peintre<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Verser `feature-1` (2 × 1).<br/>2. Poser un étal, chercher la région de sa case.<br/>
 * \tattendu Refus nommé, rien d'empilé ; la région de l'étal est ses deux cases.
 * }
 */
TEST(PaintToolsTest, LeSeauEtLesPiecesLarges) {
    CarteDEssai carte = carteDEssai();
    const hmi::BrushResult refus =
        hmi::applyBucket(carte.draft, carte.piece("feature-1", false), std::nullopt,
                         hmi::LayerViewState{}, {.column = 20, .row = 16}, carte.contexte());
    EXPECT_FALSE(refus.changed);
    EXPECT_NE(refus.refusal.find("single-cell pieces"), std::string::npos);
    EXPECT_FALSE(carte.draft.canUndo());

    ASSERT_TRUE(carte.draft.placePiece(carte.decor, {.column = 20, .row = 16}, "feature-1",
                                       TileType::Wall));
    EXPECT_EQ(hmi::floodRegion(carte.draft, carte.decor, {.column = 21, .row = 16}),
              (std::vector<GridPosition>{{20, 16}, {21, 16}}));
}

/**
 * @brief La pipette prend ce qu'on voit : la pièce de la couche active, sinon celle de devant,
 *        l'ancre d'une pièce large, ou le type de la collision.
 * \castest{<b>La pipette prend ce qu'on voit.</b><br/>
 * \tcat Unitaire · Outils du peintre<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Sur La carte d'essai, piquer un mur, la collision active puis le sol actif, et une
 * case vide.<br/>2. Poser un étal, piquer sa deuxième case.<br/>
 * \tattendu `wall` sur la collision ; `street` sur le sol ; `wall-right` depuis le décor (ou une
 * autre couche), couche rendue ; rien sur le vide ; `feature-1` pour l'étal.
 * }
 */
TEST(PaintToolsTest, LaPipettePrendCeQuOnVoit) {
    CarteDEssai carte = carteDEssai();
    const GridPosition mur{.column = 0, .row = 2};  // wall-right, au-dessus de la rue
    const GridPosition rue{.column = 0, .row = 3};
    const hmi::PlaceAppearance* const table = &*carte.lieu.appearance;

    const auto collision = hmi::pickBrush(carte.draft, std::nullopt, mur, table);
    ASSERT_TRUE(collision);
    EXPECT_EQ(collision->brush.kind, BrushKind::Type);
    EXPECT_EQ(collision->brush.type, TileType::Wall);
    EXPECT_FALSE(collision->layer);

    const auto surLeSol = hmi::pickBrush(carte.draft, carte.sol, rue, table);
    ASSERT_TRUE(surLeSol);
    EXPECT_EQ(surLeSol->brush, carte.piece("street", true));
    EXPECT_EQ(surLeSol->layer, carte.sol);

    const auto devant = hmi::pickBrush(carte.draft, carte.sol, mur, table);
    ASSERT_TRUE(devant);
    EXPECT_EQ(devant->brush, carte.piece("wall-right", false));
    EXPECT_EQ(devant->layer, carte.decor);

    EXPECT_FALSE(hmi::pickBrush(carte.draft, carte.decor, {.column = 30, .row = 0}, table));
    EXPECT_FALSE(hmi::pickBrush(carte.draft, carte.decor, {.column = -1, .row = 0}, table));

    ASSERT_TRUE(carte.draft.placePiece(carte.decor, {.column = 20, .row = 16}, "feature-1",
                                       TileType::Wall));
    const auto etal = hmi::pickBrush(carte.draft, carte.decor, {.column = 21, .row = 16}, table);
    ASSERT_TRUE(etal);
    EXPECT_EQ(etal->brush.piece, "feature-1");
}

/**
 * @brief Le miroir est l'axe vertical de l'écran iso : il transpose les cases et pose la jumelle.
 * \castest{<b>Le miroir pose la jumelle, de l'autre côté de l'axe.</b><br/>
 * \tcat Unitaire · Outils du peintre<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Refléter des cases, nommer les jumelles.<br/>2. Tracer une ligne de `wall-right`,
 * le miroir actif.<br/>3. Poser un `wall-left` sur l'axe, une façade qui le traverse.<br/>
 * \tattendu (c, r) → (r + k, c − k) ; `wall-left` ↔ `wall-right`, `front-left` ↔
 * `front-right`, `light` ↔ `light` ; la ligne a son reflet en `wall-left` ; sur l'axe, et pour
 * la façade qui le chevauche, aucun reflet.
 * }
 */
TEST(PaintToolsTest, LeMiroirPoseLaJumelle) {
    CarteDEssai carte = carteDEssai();
    const hmi::MirrorAxis axe = hmi::mirrorAxisThrough({.column = 30, .row = 5});
    EXPECT_EQ(axe.offset, 25);
    EXPECT_EQ(hmi::mirrorCell(axe, {.column = 26, .row = 5}), (GridPosition{30, 1}));
    EXPECT_EQ(hmi::mirrorCell(axe, {.column = 30, .row = 5}), (GridPosition{30, 5}));

    const core::ScenePieceManifest* const manifeste = carte.draft.pieceManifest();
    EXPECT_EQ(hmi::mirrorPieceName(manifeste, "wall-left"), "wall-right");
    EXPECT_EQ(hmi::mirrorPieceName(manifeste, "wall-right"), "wall-left");
    EXPECT_EQ(hmi::mirrorPieceName(manifeste, "front-left"), "front-right");
    EXPECT_EQ(hmi::mirrorPieceName(manifeste, "light"), "light");
    EXPECT_EQ(hmi::mirrorPieceName(nullptr, "wall-left"), "wall-left");

    const hmi::LayerViewState vue;
    ASSERT_TRUE(hmi::applyStroke(carte.draft, carte.piece("wall-right", false), std::nullopt, vue,
                                 hmi::lineCells({.column = 26, .row = 5}, {.column = 28, .row = 5}),
                                 false, carte.contexte(axe))
                    .changed);
    for (int column = 26; column <= 28; ++column) {
        EXPECT_EQ(carte.pieceEn(carte.decor, column, 5), "wall-right");
        EXPECT_EQ(carte.pieceEn(carte.decor, 30, column - 25), "wall-left");
    }
    EXPECT_EQ(carte.draft.undoDepth(), 1U);

    ASSERT_TRUE(hmi::applyStroke(carte.draft, carte.piece("wall-corner", false), std::nullopt, vue,
                                 {{.column = 30, .row = 5}}, false, carte.contexte(axe))
                    .changed);
    EXPECT_EQ(carte.pieceEn(carte.decor, 30, 5), "wall-corner");

    // front-left (1 × 2) ancré en (24, 0) couvre (24, 0) et (24, 1) ; l'axe par (24, 1) le coupe.
    const hmi::MirrorAxis coupe = hmi::mirrorAxisThrough({.column = 24, .row = 1});
    ASSERT_TRUE(hmi::applyStroke(carte.draft, carte.piece("front-left", false), std::nullopt, vue,
                                 {{.column = 24, .row = 0}}, false, carte.contexte(coupe))
                    .changed);
    EXPECT_EQ(carte.pieceEn(carte.decor, 24, 0), "front-left");
    EXPECT_EQ(carte.pieceEn(carte.decor, 23, 1), "");  // pas de front-right sur la façade
}

/**
 * @brief La mesure compte une diagonale pour une case, 5 pieds la case.
 * \castest{<b>La mesure en cases et en pieds.</b><br/>
 * \tcat Unitaire · Outils du peintre<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Mesurer de (2, 1) à (8, 4), puis une case sur elle-même.<br/>
 * \tattendu 6 × 3 d'écart, 6 cases = 30 ft ; `7 × 4 · 6 cells = 30 ft` ; `1 × 1 · 0 cells = 0 ft`.
 * }
 */
TEST(PaintToolsTest, LaMesureEnCasesEtEnPieds) {
    const hmi::Measure mesure =
        hmi::measureBetween({.column = 2, .row = 1}, {.column = 8, .row = 4});
    EXPECT_EQ(mesure, (hmi::Measure{.columns = 6, .rows = 3, .cells = 6, .feet = 30}));
    EXPECT_EQ(hmi::measureLabel(mesure), "7 × 4 · 6 cells = 30 ft");
    EXPECT_EQ(
        hmi::measureLabel(hmi::measureBetween({.column = 3, .row = 3}, {.column = 3, .row = 3})),
        "1 × 1 · 0 cells = 0 ft");
}

/**
 * @brief Tracer une maison de la carte d'essai — sol, murs, porte, seuil — prend moins de dix
 * gestes, sans ouvrir la couche collision ni un formulaire (acceptation du `LOT-EDITOR-04`).
 * \castest{<b>Une maison de la carte d'essai en six gestes.</b><br/>
 * \tcat Unitaire · Outils du peintre<br/>
 * \tcrit Critique<br/>
 * \tetapes Sur un terrain vide de la carte d'essai, le décor actif, le miroir par l'angle (30, 5)
 * :<br/>
 * 1. rectangle de `street` (25, 0)–(29, 4) ;<br/>2. ligne de `wall-right` (25, 5)–(29, 5) ;<br/>
 * 3. pinceau `wall-corner` sur l'angle ;<br/>4. pinceau `door-right` en (27, 5) ;<br/>5. pinceau
 * `doorstep` en (27, 4) ;<br/>6. annuler puis refaire le dernier geste.<br/>
 * \tattendu Deux façades (la seconde par le miroir, en `wall-left`), deux portes, deux seuils,
 * l'angle ; un pas d'annulation par geste ; la collision écrite suit les pièces, aucune case
 * forcée, et la couche active n'a jamais été la collision.
 * }
 */
TEST(PaintToolsTest, UneMaisonEnMoinsDeDixGestes) {
    CarteDEssai carte = carteDEssai();
    const hmi::LayerViewState vue;
    const hmi::StrokeContext miroir =
        carte.contexte(hmi::mirrorAxisThrough({.column = 30, .row = 5}));
    const hmi::LayerSlot active = carte.decor;  // jamais la collision
    int gestes = 0;
    const auto pinceau = [&](const CanvasBrush& brush, GridPosition cell) {
        ++gestes;
        return hmi::applyStroke(carte.draft, brush, active, vue, {cell}, false, miroir).changed;
    };
    for (int row = 0; row <= 5; ++row) {
        for (int column = 24; column <= 31; ++column) {
            ASSERT_EQ(carte.pieceEn(carte.sol, column, row), "") << column << ", " << row;
            ASSERT_EQ(carte.pieceEn(carte.decor, column, row), "") << column << ", " << row;
        }
    }

    ++gestes;
    ASSERT_TRUE(hmi::applyRectangleStroke(carte.draft, carte.piece("street", true), active, vue,
                                          {.column = 25, .row = 0}, {.column = 29, .row = 4},
                                          miroir)
                    .changed);
    ++gestes;
    ASSERT_TRUE(hmi::applyStroke(carte.draft, carte.piece("wall-right", false), active, vue,
                                 hmi::lineCells({.column = 25, .row = 5}, {.column = 29, .row = 5}),
                                 false, miroir)
                    .changed);
    ASSERT_TRUE(pinceau(carte.piece("wall-corner", false), {.column = 30, .row = 5}));
    ASSERT_TRUE(pinceau(carte.piece("door-right", false), {.column = 27, .row = 5}));
    ASSERT_TRUE(pinceau(carte.piece("doorstep", true), {.column = 27, .row = 4}));
    EXPECT_LT(gestes, 10);
    EXPECT_EQ(carte.draft.undoDepth(), static_cast<std::size_t>(gestes));

    for (int row = 0; row <= 4; ++row) {
        for (int column = 25; column <= 29; ++column) {
            const bool seuil = (column == 27 && row == 4) || (column == 29 && row == 2);
            EXPECT_EQ(carte.pieceEn(carte.sol, column, row), seuil ? "doorstep" : "street");
        }
    }
    for (int i = 25; i <= 29; ++i) {
        const bool porte = i == 27;
        EXPECT_EQ(carte.pieceEn(carte.decor, i, 5), porte ? "door-right" : "wall-right");
        EXPECT_EQ(carte.pieceEn(carte.decor, 30, i - 25), porte ? "door-left" : "wall-left");
    }
    EXPECT_EQ(carte.pieceEn(carte.decor, 30, 5), "wall-corner");
    EXPECT_EQ(carte.draft.tileMap().tile(27, 5), TileType::Wall);  // une porte arrête la vue
    EXPECT_EQ(carte.draft.tileMap().tile(27, 4), TileType::Empty);
    EXPECT_TRUE(carte.draft.forcedCollision().empty());
    EXPECT_TRUE(collisionSuitLesPieces(carte.draft));

    ASSERT_TRUE(carte.draft.undo());
    EXPECT_EQ(carte.pieceEn(carte.sol, 27, 4), "street");
    EXPECT_EQ(carte.pieceEn(carte.sol, 29, 2), "street");
    ASSERT_TRUE(carte.draft.redo());
    EXPECT_EQ(carte.pieceEn(carte.sol, 29, 2), "doorstep");
}
