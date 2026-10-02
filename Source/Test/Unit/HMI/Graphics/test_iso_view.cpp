// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_iso_view.cpp
 * @brief La vue du lieu en volume (`LOT-1003`) : la caméra orthographique tournée de 45° et
 *        inclinée de asin 0,62, vérifiée sans GPU.
 *
 * Trois choses doivent rester vraies : un point du **sol** tombe où `core::IsoProjection` le met
 * depuis toujours ; les mesures du standard 3D (120,7 px d'art par mètre, un corps de 1,80 m en
 * 170 px) en sortent sans réglage ; et la profondeur range les volumes et les images comme un œil
 * les rangerait.
 */

#include <array>
#include <cmath>
#include <numbers>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Rpg/Scale.h"
#include "HMI/Graphics/IsoView.h"

namespace {

constexpr float TOLERANCE = 1e-3F;

/// Une grille de 12 × 9 cases, au losange du jeu.
core::IsoProjection grid() {
    return core::IsoProjection{12, 9};
}

/// Le point de la vue où tombe le point (@p x, @p y, @p z) d'un maillage posé en @p gridPoint.
std::array<float, 3> placed(const hmi::IsoView& view, core::Vector2 gridPoint, float x, float y,
                            float z, float rise = 0.0F) {
    return hmi::IsoView::apply(view.meshTransform(gridPoint, rise), x, y, z);
}

}  // namespace

/**
 * @brief Le sol d'un maillage tombe sur la grille : son origine au point de grille, +X le long des
 *        colonnes, +Z le long des lignes, une case tous les 1,5 m.
 * \castest{<b>Le sol d'un maillage tombe sur la grille.</b><br/>
 * \tcat Unitaire · Vue en volume<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Poser un maillage au point de grille (4,5 ; 2,5).<br/>
 *          2. Relever où tombent son origine, le point à 1,5 m en +X et le point à 1,5 m en
 *             +Z.<br/>
 * \tattendu L'origine tombe où `IsoProjection::gridToWorld` met (4,5 ; 2,5), les deux autres où
 *           elle met (5,5 ; 2,5) et (4,5 ; 3,5) ; chacun a la profondeur du sol à son ordonnée.
 * }
 */
TEST(IsoViewTest, LeSolDUnMaillageTombeSurLaGrille) {
    const core::IsoProjection projection = grid();
    const hmi::IsoView view(projection);
    const core::Vector2 at{4.5F, 2.5F};

    const auto expect = [&](const std::array<float, 3>& point, core::Vector2 gridPoint) {
        const core::Vector2 world = projection.gridToWorld(gridPoint);
        EXPECT_NEAR(point[0], world.x, TOLERANCE);
        EXPECT_NEAR(point[1], world.y, TOLERANCE);
        EXPECT_NEAR(point[2], view.groundDepth(world.y), TOLERANCE);
    };
    expect(placed(view, at, 0.0F, 0.0F, 0.0F), at);
    expect(placed(view, at, core::METERS_PER_TILE, 0.0F, 0.0F), {5.5F, 2.5F});
    expect(placed(view, at, 0.0F, 0.0F, core::METERS_PER_TILE), {4.5F, 3.5F});
}

/**
 * @brief Les mesures du standard 3D sortent de la vue sans réglage.
 * \castest{<b>Les mesures du standard 3D sortent de la vue sans reglage.</b><br/>
 * \tcat Unitaire · Vue en volume<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Rapporter la vue au losange d'art de 256 pixels.<br/>
 *          2. Mesurer un mètre à l'horizontale, un corps de 1,80 m, un étage de 2,366 m.<br/>
 * \tattendu 120,7 px d'art par mètre ; 170 px pour le corps ; 224 px, soit 0,875 largeur de case,
 *           pour l'étage — les valeurs du standard (`style-3d.md`, §1).
 * }
 */
TEST(IsoViewTest, LesMesuresDuStandardSortentDeLaVue) {
    const core::IsoProjection projection = grid();
    const hmi::IsoView view(projection);
    const float artPixelsPerUnit = 256.0F / projection.tileWidth();

    EXPECT_NEAR(view.sine(), 0.62F, 1e-6F);
    EXPECT_NEAR(view.unitsPerMetre() * artPixelsPerUnit, 120.7F, 0.05F);
    EXPECT_NEAR(view.riseOf(1.80F) * artPixelsPerUnit, 170.0F, 0.5F);
    EXPECT_NEAR(view.riseOf(2.36573F) / projection.tileWidth(), 0.875F, 1e-4F);

    // Un point élevé monte à l'écran de sa hauteur raccourcie, sans bouger de côté.
    const core::Vector2 at{3.0F, 3.0F};
    const std::array<float, 3> ground = placed(view, at, 0.0F, 0.0F, 0.0F);
    const std::array<float, 3> head = placed(view, at, 0.0F, 1.80F, 0.0F);
    EXPECT_NEAR(head[0], ground[0], TOLERANCE);
    EXPECT_NEAR(ground[1] - head[1], view.riseOf(1.80F), TOLERANCE);
    EXPECT_LT(head[2], ground[2]) << "le haut d'un volume est plus près de la caméra que son pied";
}

/**
 * @brief La vue est une rotation : elle ne déforme rien.
 * \castest{<b>La vue est une rotation : les distances sont gardees.</b><br/>
 * \tcat Unitaire · Vue en volume<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Prendre des couples de points quelconques d'un maillage.<br/>
 *          2. Comparer leur distance dans la vue à leur distance en mètres.<br/>
 * \tattendu Le rapport est partout l'échelle de la vue (unités par mètre) : profondeur comprise,
 *           la vue ne fait que tourner et agrandir.
 * }
 */
TEST(IsoViewTest, LaVueEstUneRotation) {
    const hmi::IsoView view(grid());
    const core::Vector2 at{6.0F, 4.0F};
    const std::array<std::array<float, 3>, 4> points = {
        {{0.0F, 0.0F, 0.0F}, {1.0F, 2.0F, -0.5F}, {-3.0F, 0.7F, 2.2F}, {0.3F, 4.0F, 4.0F}}};
    for (const std::array<float, 3>& a : points) {
        for (const std::array<float, 3>& b : points) {
            const std::array<float, 3> va = placed(view, at, a[0], a[1], a[2]);
            const std::array<float, 3> vb = placed(view, at, b[0], b[1], b[2]);
            const float metres = std::hypot(a[0] - b[0], a[1] - b[1], a[2] - b[2]);
            const float units = std::hypot(va[0] - vb[0], va[1] - vb[1], va[2] - vb[2]);
            EXPECT_NEAR(units, metres * view.unitsPerMetre(), 1e-3F);
        }
    }
}

/**
 * @brief Un étage élève un maillage de sa hauteur à l'écran, à la profondeur d'un point élevé.
 * \castest{<b>Un etage eleve un maillage sans le deplacer au sol.</b><br/>
 * \tcat Unitaire · Vue en volume<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser le même maillage au rez, puis élevé d'un étage de 0,875 largeur de case.<br/>
 *          2. Comparer avec le sommet d'un mur haut d'un étage, posé au rez.<br/>
 * \tattendu L'origine du maillage élevé coïncide, profondeur comprise, avec le sommet du mur : un
 *           toit posé à l'étage repose sur les murs.
 * }
 */
TEST(IsoViewTest, UnEtageEleveUnMaillage) {
    const core::IsoProjection projection = grid();
    const hmi::IsoView view(projection);
    const core::Vector2 at{5.5F, 3.5F};
    const float storey = 0.875F * projection.tileWidth();

    const std::array<float, 3> raised = placed(view, at, 0.0F, 0.0F, 0.0F, storey);
    const std::array<float, 3> wallTop = placed(view, at, 0.0F, 2.36573F, 0.0F);
    EXPECT_NEAR(raised[0], wallTop[0], TOLERANCE);
    EXPECT_NEAR(raised[1], wallTop[1], TOLERANCE);
    EXPECT_NEAR(raised[2], wallTop[2], TOLERANCE);
}

/**
 * @brief Une image dressée est un plan vertical au-dessus de son pied, le sol au-dessous : elle se
 *        range devant le mur qui est derrière elle et derrière celui qui est devant, à toute
 *        hauteur.
 * \castest{<b>Une image dressee se range comme un plan vertical.</b><br/>
 * \tcat Unitaire · Vue en volume<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Dresser une image sur une ligne de pied.<br/>
 *          2. Comparer sa profondeur, de son pied à 2 m de haut, à celle d'un flanc de mur vertical
 *             placé 0,3 m derrière elle, puis 0,3 m devant.<br/>
 *          3. Comparer, sous son pied, sa profondeur à celle du sol.<br/>
 * \tattendu À chaque hauteur l'image est plus proche que le mur de derrière et plus loin que celui
 *           de devant — ce qu'un plan face au regard, penché en arrière, ne tiendrait pas ; sous le
 *           pied, elle a la profondeur du sol.
 * }
 */
TEST(IsoViewTest, UneImageDresseeSeRangeCommeUnPlanVertical) {
    const core::IsoProjection projection = grid();
    const hmi::IsoView view(projection);
    const core::Vector2 at{5.0F, 5.0F};
    const float footY = projection.gridToWorld(at).y;

    // Le flanc d'un mur vertical decale de `metres` vers la camera (positif) ou vers le fond : un
    // point de ce flanc a la hauteur `height`, et l'ordonnee ou il tombe.
    const auto wallPoint = [&](float metres, float height) {
        const float along = metres / std::numbers::sqrt2_v<float>;
        return placed(view, at, along, height, along);
    };
    for (const float height : {0.0F, 0.5F, 1.0F, 1.8F, 2.0F}) {
        const std::array<float, 3> behind = wallPoint(-0.3F, height);
        EXPECT_LT(view.standingDepth(footY, behind[1]), behind[2])
            << "devant le mur de derrière, à " << height << " m";
    }
    // Le mur de devant, des que son flanc monte au-dessus de la ligne du pied (au sol, les deux
    // sont le sol lui-meme).
    for (const float height : {0.5F, 1.0F, 1.8F, 2.0F}) {
        const std::array<float, 3> front = wallPoint(0.3F, height);
        ASSERT_LT(front[1], footY);
        EXPECT_GT(view.standingDepth(footY, front[1]), front[2])
            << "derrière le mur de devant, à " << height << " m";
    }
    // Un plan face au regard (profondeur constante, celle du pied) passerait derriere le mur de
    // derriere des la mi-hauteur : c'est ce que la tenue verticale evite.
    const std::array<float, 3> behindHead = wallPoint(-0.3F, 1.8F);
    EXPECT_GT(view.groundDepth(footY), behindHead[2]);

    // Sous le pied : le sol.
    EXPECT_FLOAT_EQ(view.standingDepth(footY, footY + 0.4F), view.groundDepth(footY + 0.4F));
    EXPECT_FLOAT_EQ(view.standingDepth(footY, footY), view.groundDepth(footY));
}

/**
 * @brief L'étendue de profondeur contient toute la scène, et la boîte projetée d'un maillage ce
 *        qu'il occupe à l'image.
 * \castest{<b>L'etendue de profondeur contient la scene.</b><br/>
 * \tcat Unitaire · Vue en volume<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander l'étendue de profondeur d'une grille.<br/>
 *          2. Y chercher le sol de ses quatre coins et le sommet d'un volume de 20 m.<br/>
 *          3. Projeter la boîte d'un mur d'une case.<br/>
 * \tattendu Tout est strictement entre le plan le plus proche et le plus lointain ; la boîte
 *           projetée fait une largeur de case, et s'étend du dessus du mur au sommet bas du
 *           losange.
 * }
 */
TEST(IsoViewTest, LEtendueDeProfondeurContientLaScene) {
    const core::IsoProjection projection = grid();
    const hmi::IsoView view(projection);
    const hmi::DepthRange range = view.depthRange();
    ASSERT_LT(range.nearest, range.farthest);
    for (const core::Vector2 corner : {core::Vector2{0.0F, 0.0F}, core::Vector2{12.0F, 0.0F},
                                       core::Vector2{0.0F, 9.0F}, core::Vector2{12.0F, 9.0F}}) {
        const std::array<float, 3> ground = placed(view, corner, 0.0F, 0.0F, 0.0F);
        const std::array<float, 3> top = placed(view, corner, 0.0F, 20.0F, 0.0F);
        EXPECT_GT(ground[2], range.nearest);
        EXPECT_LT(ground[2], range.farthest);
        EXPECT_GT(top[2], range.nearest);
        EXPECT_LT(top[2], range.farthest);
    }

    const core::Vector2 cell{4.5F, 2.5F};
    const hmi::ViewTransform transform = view.meshTransform(cell, 0.0F);
    const core::Rect bounds =
        hmi::IsoView::projectedBounds(transform, {-0.75F, 0.0F, -0.75F}, {0.75F, 2.0F, 0.75F});
    const core::Rect tile = projection.tileBounds({.column = 4, .row = 2});
    EXPECT_NEAR(bounds.position.x, tile.position.x, TOLERANCE);
    EXPECT_NEAR(bounds.size.x, tile.size.x, TOLERANCE);
    EXPECT_NEAR(bounds.position.y + bounds.size.y, tile.position.y + tile.size.y, TOLERANCE);
    EXPECT_NEAR(bounds.position.y, tile.position.y - view.riseOf(2.0F), TOLERANCE);
}
