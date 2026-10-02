// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_render_culling.cpp
 * @brief Tests unitaires du culling par cadrage caméra (EX-NFR-005).
 */

#include <gtest/gtest.h>

#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/PlaceCamera.h"
#include "HMI/Graphics/QuadRecorder.h"

namespace {

// Texture factice : la composition ne fait que comparer des identites (cf. test_quad_recorder).
int textureStorage = 0;
hmi::TextureHandle texture = &textureStorage;

/// Cadrage de reference des tests : la caméra voit le monde de (10, 10) a (30, 20).
const core::Rect VISIBLE_BOUNDS{core::Vector2{10.0f, 10.0f}, core::Vector2{20.0f, 10.0f}};

/// Rectangle d'une case a une position donnee.
hmi::SpriteQuad tileQuad(float x, float y) {
    hmi::SpriteQuad quad;
    quad.x = x;
    quad.y = y;
    quad.width = 1.0f;
    quad.height = 1.0f;
    return quad;
}

}  // namespace

/**
 * @brief Sans cadrage fixé, aucune primitive n'est écartée : le culling est une décision
 *        explicite de l'appelant.
 * \castest{<b>Sans cadrage fixe, aucune primitive n'est ecartee.</b><br/>
 * \tcat Unitaire · Render Culling<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Composer un quad tres eloigne dans une scene sans cadrage.<br/>
 * \tattendu Le quad est conserve.
 * }
 */
TEST(RenderCullingTest, SansCadrageAucunRejet) {
    hmi::ComposedScene scene;

    EXPECT_FALSE(scene.isCullingEnabled());
    EXPECT_TRUE(scene.addSprite(hmi::RenderLayer::Tile, texture, 0, tileQuad(9999.0f, 9999.0f)));
    EXPECT_EQ(scene.statistics().culled, 0);
}

/**
 * @brief Une primitive hors du cadrage est écartée ; la même primitive dans le cadrage est
 *        conservée.
 * \castest{<b>Une primitive hors cadrage est ecartee, la meme dans le cadrage est
 * conservee.</b><br/>
 * \tcat Unitaire · Render Culling<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Fixer le cadrage de reference.<br/>2. Composer un quad dedans, puis un quad
 * loin dehors.<br/>
 * \tattendu Seul le quad interieur est conserve, et les compteurs le refletent.
 * }
 */
TEST(RenderCullingTest, HorsCadrageEcarte) {
    hmi::ComposedScene scene;
    scene.setVisibleBounds(VISIBLE_BOUNDS);

    EXPECT_TRUE(scene.addSprite(hmi::RenderLayer::Tile, texture, 0, tileQuad(15.0f, 15.0f)));
    EXPECT_FALSE(scene.addSprite(hmi::RenderLayer::Tile, texture, 0, tileQuad(80.0f, 15.0f)));

    hmi::QuadRecorder recorder;
    recorder.record(scene);

    EXPECT_EQ(recorder.size(), 1u);
    EXPECT_TRUE(recorder.containsSpriteAt(15.0f, 15.0f)) << recorder.describe();
    EXPECT_EQ(recorder.statistics().considered, 2);
    EXPECT_EQ(recorder.statistics().culled, 1);
    EXPECT_EQ(recorder.statistics().submitted, 1);
}

/**
 * @brief Une primitive à cheval sur la frontière du cadrage est conservée : la marge évite qu'une
 *        entité partiellement visible disparaisse prématurément.
 * \castest{<b>Une primitive a cheval sur la frontiere du cadrage est conservee.</b><br/>
 * \tcat Unitaire · Render Culling<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Fixer le cadrage de reference.<br/>2. Composer un quad chevauchant le bord
 * droit, puis un quad entierement dans la marge.<br/>
 * \tattendu Les deux quads sont conserves.
 * }
 */
TEST(RenderCullingTest, AChevalSurLaFrontiereConserve) {
    hmi::ComposedScene scene;
    scene.setVisibleBounds(VISIBLE_BOUNDS);

    // Chevauche le bord droit du cadrage (x = 30).
    EXPECT_TRUE(scene.addSprite(hmi::RenderLayer::Tile, texture, 0, tileQuad(29.5f, 15.0f)));
    // Entierement hors du cadrage visible, mais dans la marge d'une case.
    EXPECT_TRUE(scene.addSprite(hmi::RenderLayer::Tile, texture, 0, tileQuad(30.2f, 15.0f)));
    // Au-dela de la marge : ecarte.
    EXPECT_FALSE(scene.addSprite(hmi::RenderLayer::Tile, texture, 0, tileQuad(31.5f, 15.0f)));
}

/**
 * @brief Un segment horizontal (boîte d'aire nulle sans son épaisseur) traversant le cadrage est
 *        conservé : la boîte englobante tient compte de l'épaisseur.
 * \castest{<b>Un segment horizontal traversant le cadrage est conserve.</b><br/>
 * \tcat Unitaire · Render Culling<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Fixer le cadrage de reference.<br/>2. Composer un segment horizontal a
 * l'interieur, puis un segment horizontal tres au-dessus.<br/>
 * \tattendu Seul le segment interieur est conserve.
 * }
 */
TEST(RenderCullingTest, SegmentHorizontalConserve) {
    hmi::LineQuad inside;
    inside.ax = 0.0f;
    inside.ay = 15.0f;
    inside.bx = 100.0f;
    inside.by = 15.0f;
    inside.thickness = 0.05f;

    hmi::LineQuad outside = inside;
    outside.ay = -50.0f;
    outside.by = -50.0f;

    hmi::ComposedScene scene;
    scene.setVisibleBounds(VISIBLE_BOUNDS);

    EXPECT_TRUE(scene.addLine(hmi::RenderLayer::EditorOverlay, texture, 0, inside));
    EXPECT_FALSE(scene.addLine(hmi::RenderLayer::EditorOverlay, texture, 0, outside));
}

/**
 * @brief La boîte englobante réelle est testée, pas la position d'ancrage : une primitive très
 *        étirée dont le coin est hors cadrage reste soumise (cas du fond de niveau).
 * \castest{<b>Une primitive etiree dont l'ancrage est hors cadrage reste soumise.</b><br/>
 * \tcat Unitaire · Render Culling<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Fixer le cadrage de reference.<br/>2. Composer un quad ancre en (0, 0) et large
 * de 200 unites.<br/>
 * \tattendu Le quad est conserve.
 * }
 */
TEST(RenderCullingTest, PrimitiveEtireeConservee) {
    hmi::SpriteQuad background;
    background.x = 0.0f;
    background.y = 0.0f;
    background.width = 200.0f;
    background.height = 100.0f;

    hmi::ComposedScene scene;
    scene.setVisibleBounds(VISIBLE_BOUNDS);

    EXPECT_TRUE(scene.addSprite(hmi::RenderLayer::Background, texture, 0, background));
}

/**
 * @brief Un quad texturé sans rotation a la même boîte englobante que le rectangle brut
 * (`x`, `y`, `width`, `height`) : la rotation nulle ne coûte rien et ne change rien.
 * \castest{<b>spriteQuadBounds sans rotation vaut le rectangle brut.</b><br/>
 * \tcat Unitaire · Render Culling<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Calculer la boite englobante d'un quad de rotation nulle.<br/>
 * \tattendu La boite correspond exactement a (x, y, width, height).
 * }
 */
TEST(RenderCullingTest, SpriteQuadBoundsSansRotationEstLeRectangleBrut) {
    hmi::SpriteQuad quad;
    quad.x = 5.0f;
    quad.y = 3.0f;
    quad.width = 4.0f;
    quad.height = 2.0f;

    const core::Rect bounds = hmi::spriteQuadBounds(quad);

    EXPECT_FLOAT_EQ(bounds.position.x, 5.0f);
    EXPECT_FLOAT_EQ(bounds.position.y, 3.0f);
    EXPECT_FLOAT_EQ(bounds.size.x, 4.0f);
    EXPECT_FLOAT_EQ(bounds.size.y, 2.0f);
}

/**
 * @brief Un quad texturé tourné d'un quart de tour (90°) a une boîte englobante correspondant à ses
 * dimensions permutées (large devient haut), centrée au même endroit que le rectangle non tourné —
 * le culling doit juger ce rectangle-là, jamais (x, y, width, height) brut.
 * \castest{<b>spriteQuadBounds a 90 degres permute largeur et hauteur.</b><br/>
 * \tcat Unitaire · Render Culling<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Calculer la boite englobante d'un quad large tourne de 90 degres.<br/>
 * \tattendu La boite est haute (largeur/hauteur permutees), centree au meme endroit.
 * }
 */
TEST(RenderCullingTest, SpriteQuadBoundsA90DegresPermuteLargeurEtHauteur) {
    hmi::SpriteQuad quad;
    quad.x = 0.0f;
    quad.y = 0.0f;
    quad.width = 4.0f;
    quad.height = 2.0f;
    quad.rotation = 1.57079632679f;  // pi/2

    const core::Rect bounds = hmi::spriteQuadBounds(quad);

    EXPECT_NEAR(bounds.size.x, 2.0f, 1e-3f);
    EXPECT_NEAR(bounds.size.y, 4.0f, 1e-3f);
    // Meme centre que le rectangle non tourne : (2, 1).
    EXPECT_NEAR(bounds.position.x + bounds.size.x * 0.5f, 2.0f, 1e-3f);
    EXPECT_NEAR(bounds.position.y + bounds.size.y * 0.5f, 1.0f, 1e-3f);
}

/**
 * @brief Un décor tourné près du bord du cadrage, dont le rectangle non tourné serait hors champ
 * mais dont la boîte englobante réelle (élargie par la rotation) chevauche le cadrage, reste
 * soumis : un culling naïf sur (x, y, width, height) l'écarterait à tort.
 * \castest{<b>Un quad tourne dont la boite reelle chevauche le cadrage reste soumis.</b><br/>
 * \tcat Unitaire · Render Culling<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Fixer un cadrage de reference.<br/>2. Composer un quad tourne de 45 degres juste a
 * l'exterieur du rectangle non tourne, mais dont la diagonale rentre dans le cadrage.<br/>
 * \tattendu Le quad est conserve.
 * }
 */
TEST(RenderCullingTest, QuadTourneChevauchantLeCadrageResteSoumis) {
    hmi::ComposedScene scene;
    scene.setVisibleBounds(VISIBLE_BOUNDS);  // (10, 10) a (30, 20) ; marge d'une unite -> bord
                                             // droit du cadrage de culling a x = 31.

    // Rectangle NON tourne (31.2, 14.0, 2x2) : bord gauche a x=31.2, entierement au-dela du bord
    // droit du cadrage de culling (31.0) -- un culling naif l'ecarterait. Tourne de 45 degres, sa
    // demi-diagonale (~1.414) ramene son bord gauche reel a x~30.79, dans le cadrage.
    hmi::SpriteQuad quad;
    quad.width = 2.0f;
    quad.height = 2.0f;
    quad.x = 31.2f;
    quad.y = 14.0f;
    quad.rotation = 0.78539816339f;  // pi/4

    EXPECT_TRUE(scene.addSprite(hmi::RenderLayer::EditorOverlay, texture, 0, quad));
}

/**
 * @brief Le cadrage utilisé pour le culling est le cadrage visible élargi de la marge, sur les
 *        quatre côtés.
 * \castest{<b>Le cadrage de culling est le cadrage visible elargi de la marge.</b><br/>
 * \tcat Unitaire · Render Culling<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Fixer le cadrage de reference.<br/>2. Lire le rectangle de culling.<br/>
 * \tattendu Il deborde d'une marge de chaque cote.
 * }
 */
TEST(RenderCullingTest, MargeAppliqueeSurLesQuatreCotes) {
    hmi::ComposedScene scene;
    scene.setVisibleBounds(VISIBLE_BOUNDS);
    const core::Rect bounds = scene.cullingBounds();
    constexpr float MARGIN = hmi::ComposedScene::CULLING_MARGIN_UNITS;

    EXPECT_FLOAT_EQ(bounds.left(), VISIBLE_BOUNDS.left() - MARGIN);
    EXPECT_FLOAT_EQ(bounds.top(), VISIBLE_BOUNDS.top() - MARGIN);
    EXPECT_FLOAT_EQ(bounds.right(), VISIBLE_BOUNDS.right() + MARGIN);
    EXPECT_FLOAT_EQ(bounds.bottom(), VISIBLE_BOUNDS.bottom() + MARGIN);
}

/**
 * @brief La caméra fournit le rectangle monde qu'elle cadre, base du culling.
 * \castest{<b>La camera fournit le rectangle monde qu'elle cadre.</b><br/>
 * \tcat Unitaire · Render Culling<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Centrer une camera 320x160 sur (10, 5) au zoom 1.<br/>2. Lire son cadrage.<br/>
 * \tattendu Le cadrage couvre 20x10 unites a partir de l'origine.
 * }
 */
TEST(RenderCullingTest, CadrageDeLaCamera) {
    hmi::PlaceCamera camera(320, 160);
    camera.setCenter(core::Vector2{10.0f, 5.0f});
    camera.setZoom(1.0f);

    const core::Rect bounds = camera.visibleBounds();

    EXPECT_FLOAT_EQ(bounds.left(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.top(), 0.0f);
    EXPECT_FLOAT_EQ(bounds.size.x, 20.0f);
    EXPECT_FLOAT_EQ(bounds.size.y, 10.0f);

    // Le zoom retrecit le cadrage d'autant.
    camera.setZoom(2.0f);
    EXPECT_FLOAT_EQ(camera.visibleBounds().size.x, 10.0f);
}
