// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_canvas_picking.cpp
 * @brief Le pointage du canevas de l'éditeur (`LOT-EDITOR-02`) : juste aux quatre coins de
 *        La carte d'essai, juste sous un mur haut, et la hauteur réservée en paramètre.
 */

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "Editor/Logic/CanvasPicking.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

// La racine d'essai de l'éditeur (`LOT-123`) : ce test pointait une carte LIVRÉE, que la
// table rase du `LOT-102` emporte.
[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path(JADG_TEST_DATA_DIR);
}

[[nodiscard]] std::filesystem::path assets() {
    return dataRoot() / "Assets";
}

/// La carte d'essai, telle qu'elle est sur disque.
[[nodiscard]] core::Level carteDEssai() {
    core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(
        dataRoot() / "Levels" / "bourg" / "place.json");
    if (!loaded.ok()) {
        // Pas de carte vide à rendre : gtest compte l'exception comme un échec du test.
        throw std::runtime_error("place.json : " + loaded.error);
    }
    return std::move(*loaded.level);
}

/// Un point à une fraction @p t du chemin de @p from vers @p to.
[[nodiscard]] core::Vector2 toward(core::Vector2 from, core::Vector2 to, float t) {
    return {from.x + ((to.x - from.x) * t), from.y + ((to.y - from.y) * t)};
}

[[nodiscard]] bool contains(const core::Rect& rect, core::Vector2 point) {
    return point.x >= rect.position.x && point.y >= rect.position.y &&
           point.x < rect.position.x + rect.size.x && point.y < rect.position.y + rect.size.y;
}

}  // namespace

/**
 * @brief Aux quatre coins de la carte d'essai, le centre et les sommets d'une case la désignent.
 * \castest{<b>Le pointage iso est juste aux quatre coins de la carte.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Projeter la carte d'essai (48 x 40 cases).<br/>
 *          2. Pointer le centre des quatre cases de coin, puis un point pres de chacun de leurs
 *             sommets, a l'interieur du losange.<br/>
 *          3. Pointer juste au-dela du sommet exterieur de chaque coin.<br/>
 * \tattendu Chaque point interieur designe sa case ; au-dela du sommet exterieur, aucune case
 *           n'est designee, et le pointage borne ramene au coin.
 * }
 */
TEST(CanvasPickingTest, LePointageEstJusteAuxQuatreCoins) {
    const core::Level map = carteDEssai();
    const core::IsoProjection projection(map.tileMap().width(), map.tileMap().height());
    ASSERT_EQ(projection.columns(), 48);
    ASSERT_EQ(projection.rows(), 40);

    // Le sommet extérieur de chaque coin : haut, droit, bas, gauche du losange de la case.
    const struct {
        core::GridPosition cell;
        std::size_t outerVertex;
    } corners[] = {
        {{.column = 0, .row = 0}, 0},
        {{.column = 47, .row = 0}, 1},
        {{.column = 47, .row = 39}, 2},
        {{.column = 0, .row = 39}, 3},
    };
    for (const auto& corner : corners) {
        const core::Vector2 center = projection.tileToWorld(corner.cell);
        EXPECT_EQ(hmi::pickIsoCell(projection, center), corner.cell);
        const std::array<core::Vector2, 4> diamond = hmi::isoCellDiamond(projection, corner.cell);
        for (const core::Vector2& vertex : diamond) {
            EXPECT_EQ(hmi::pickIsoCell(projection, toward(vertex, center, 0.05F)), corner.cell)
                << "pres d'un sommet de (" << corner.cell.column << ", " << corner.cell.row << ")";
        }
        const core::Vector2 outside = toward(center, diamond[corner.outerVertex], 1.05F);
        EXPECT_FALSE(hmi::pickIsoCell(projection, outside).has_value());
        EXPECT_EQ(hmi::clampedIsoCell(projection, outside), corner.cell);
    }
}

/**
 * @brief Sous un mur haut de la carte d'essai, on pointe la case dont le losange est sous le pointeur.
 * \castest{<b>Sous un mur haut, le pointage designe la case par son pied.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Composer la carte d'essai avec la taille des pieces de son manifeste.<br/>
 *          2. Prendre une piece de relief plus haute que deux losanges, et la case juste derriere
 *             elle (colonne - 1, ligne - 1).<br/>
 *          3. Pointer le centre du losange de cette case.<br/>
 * \tattendu Le mur couvre ce point a l'ecran, et le pointage designe pourtant la case derriere,
 *           pas celle du mur.
 * }
 */
TEST(CanvasPickingTest, SousUnMurHautOnPointeLaCaseDerriere) {
    const core::Level map = carteDEssai();
    const hmi::PlaceAppearanceResult appearance =
        hmi::PlaceAppearance::loadFromFile(assets() / "Scene" / "bourg" / "appearance.json");
    ASSERT_TRUE(appearance.ok()) << appearance.message;
    const core::ScenePieceManifestResult manifest =
        core::ScenePieceManifest::loadFromFile(assets() / "Scene" / "bourg" / "manifest.json");
    ASSERT_TRUE(manifest.ok()) << manifest.message;

    // Des textures sans GPU : une identité par pièce, la taille que déclare le manifeste.
    hmi::ScenePieceTextures textures;
    std::vector<std::uint8_t> identities(manifest.manifest.pieces().size());
    for (std::size_t index = 0; index < identities.size(); ++index) {
        const core::ScenePiece& piece = manifest.manifest.pieces()[index];
        textures.byPath["Scene/bourg/" + piece.file] = hmi::SceneTexture{
            .texture = &identities[index], .width = piece.width, .height = piece.height};
    }
    const hmi::WorldSceneSnapshot snapshot =
        hmi::snapshotWorldScene(map, appearance.appearance, {});
    const core::IsoProjection projection(snapshot.columns, snapshot.rows);
    const hmi::ComposedScene scene = hmi::composeWorldScene(snapshot, projection, textures);

    bool checked = false;
    for (int row = 1; row < snapshot.rows && !checked; ++row) {
        for (int column = 1; column < snapshot.columns && !checked; ++column) {
            const core::GridPosition wall{.column = column, .row = row};
            if (snapshot.reliefAt(wall).empty()) {
                continue;
            }
            const core::GridPosition behind{.column = column - 1, .row = row - 1};
            const core::Vector2 point = projection.tileToWorld(behind);
            for (const hmi::ComposedQuad& quad : scene.quads()) {
                if (quad.layer != hmi::RenderLayer::Object ||
                    quad.sprite.height < 2.0F * projection.tileHeight()) {
                    continue;
                }
                const core::Rect bounds = hmi::spriteQuadBounds(quad.sprite);
                // Le quad de ce mur-ci : son bas est celui du losange de sa case.
                const core::Rect cellBox = projection.tileBounds(wall);
                if (std::abs((bounds.position.y + bounds.size.y) -
                             (cellBox.position.y + cellBox.size.y)) > 0.05F ||
                    !contains(bounds, projection.tileToWorld(wall))) {
                    continue;
                }
                EXPECT_TRUE(contains(bounds, point)) << "le mur couvre la case derriere lui";
                EXPECT_EQ(hmi::pickIsoCell(projection, point), behind);
                checked = true;
                break;
            }
        }
    }
    EXPECT_TRUE(checked) << "La carte d'essai a au moins un relief haut devant une case";
}

/**
 * @brief La hauteur réservée (D11) se prend en paramètre : un losange élevé se pointe plus haut.
 * \castest{<b>Le pointage prend la hauteur en parametre.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Elever d'un niveau le losange d'une case.<br/>
 *          2. Pointer son centre avec la hauteur 1, puis avec la hauteur 0.<br/>
 * \tattendu Avec la hauteur 1, la case est designee ; avec 0, c'est la case dont le losange au
 *           sol est a cet endroit, celle de derriere.
 * }
 */
TEST(CanvasPickingTest, LaHauteurSePrendEnParametre) {
    const core::IsoProjection projection(10, 10);
    const core::GridPosition cell{.column = 5, .row = 5};
    const std::array<core::Vector2, 4> raised = hmi::isoCellDiamond(projection, cell, 1);
    const core::Vector2 center = toward(raised[0], raised[2], 0.5F);
    EXPECT_EQ(hmi::pickIsoCell(projection, center, 1), cell);
    EXPECT_EQ(hmi::pickIsoCell(projection, center, 0), (core::GridPosition{.column = 4, .row = 4}));
    EXPECT_EQ(hmi::isoCellDiamond(projection, cell, 0)[0], projection.gridToWorld({5.0F, 5.0F}));
}

/**
 * @brief Les cases à dessiner pour un cadrage : toutes pour la scène entière, peu pour un détail.
 * \castest{<b>Le canevas ne parcourt que les cases visibles.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander les cases couvertes par la scene entiere de la carte d'essai.<br/>
 *          2. Demander celles d'un petit rectangle autour du centre d'une case.<br/>
 *          3. Faire de meme en vue a plat.<br/>
 * \tattendu La scene entiere couvre toute la grille ; le petit rectangle couvre la case et au
 *           plus ses voisines ; a plat, un rectangle hors de la grille ne couvre rien.
 * }
 */
TEST(CanvasPickingTest, OnNeParcourtQueLesCasesVisibles) {
    const core::IsoProjection projection(48, 40);
    const core::Vector2 size = projection.sceneSize();
    const hmi::CellRange all =
        hmi::isoCellsCovering(projection, core::Rect{{0.0F, 0.0F}, {size.x, size.y}});
    EXPECT_EQ(all,
              (hmi::CellRange{.firstColumn = 0, .firstRow = 0, .lastColumn = 47, .lastRow = 39}));

    const core::GridPosition cell{.column = 20, .row = 13};
    const core::Vector2 center = projection.tileToWorld(cell);
    const hmi::CellRange detail = hmi::isoCellsCovering(
        projection, core::Rect{{center.x - 0.5F, center.y - 0.5F}, {1.0F, 1.0F}});
    EXPECT_TRUE(detail.contains(cell));
    EXPECT_LE(detail.lastColumn - detail.firstColumn, 2);
    EXPECT_LE(detail.lastRow - detail.firstRow, 2);

    EXPECT_EQ(hmi::flatCellsCovering(core::Rect{{2.5F, 3.2F}, {2.0F, 1.0F}}, 48, 40),
              (hmi::CellRange{.firstColumn = 2, .firstRow = 3, .lastColumn = 4, .lastRow = 4}));
    EXPECT_TRUE(hmi::flatCellsCovering(core::Rect{{60.0F, 3.0F}, {2.0F, 1.0F}}, 48, 40).empty());
}

/**
 * @brief En vue à plat, une case par unité ; hors de la grille, rien, ou le bord le plus proche.
 * \castest{<b>Le pointage a plat designe la case sous le point.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Pointer dans la grille, puis juste a gauche de la colonne 0.<br/>
 * \tattendu Dans la grille, la case du point ; a gauche, aucune case, et le pointage borne rend
 *           la colonne 0.
 * }
 */
TEST(CanvasPickingTest, EnVueAPlatUneCaseParUnite) {
    EXPECT_EQ(hmi::pickFlatCell({3.7F, 1.2F}, 10, 5), (core::GridPosition{.column = 3, .row = 1}));
    EXPECT_FALSE(hmi::pickFlatCell({-0.2F, 1.0F}, 10, 5).has_value());
    EXPECT_EQ(hmi::clampedFlatCell({-0.2F, 1.0F}, 10, 5),
              (core::GridPosition{.column = 0, .row = 1}));
    EXPECT_EQ(hmi::clampedFlatCell({42.0F, 9.0F}, 10, 5),
              (core::GridPosition{.column = 9, .row = 4}));
}
