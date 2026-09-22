// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_poly_quad.cpp
 * @brief Tests unitaires de la primitive à quatre sommets libres (LOT-128, décision D1) : sa boîte
 *        englobante, son culling, et sa place dans le tri commun.
 */

#include <gtest/gtest.h>

#include "HMI/Graphics/ComposedScene.h"

namespace {

// Textures factices : la composition ne fait que comparer des identites.
int solidStorage = 0;
int otherStorage = 0;
hmi::TextureHandle solid = &solidStorage;
hmi::TextureHandle other = &otherStorage;

/// Un losange isometrique pose sur la case (x, y), large de @p width et haut de @p height.
/// Sommets dans l'ordre du pourtour : haut, droite, bas, gauche.
hmi::PolyQuad diamond(float x, float y, float width, float height) {
    const float halfWidth = width / 2.0f;
    const float halfHeight = height / 2.0f;
    hmi::PolyQuad quad;
    quad.x = {x + halfWidth, x + width, x + halfWidth, x};
    quad.y = {y, y + halfHeight, y + height, y + halfHeight};
    return quad;
}

}  // namespace

/**
 * @brief La boîte englobante d'un losange est celle de ses quatre sommets — et non celle de ses
 *        côtés, qui la sous-estimerait.
 * \castest{<b>La boite englobante d'un losange est celle de ses quatre sommets.</b><br/>
 * \tcat Unitaire · PolyQuad<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Borner un losange de 4 x 2 pose en (10, 20).<br/>
 * \tattendu La boite vaut exactement (10, 20, 4, 2).
 * }
 */
TEST(PolyQuadTest, BoiteEnglobanteDesSommets) {
    const core::Rect bounds = hmi::polyQuadBounds(diamond(10.0f, 20.0f, 4.0f, 2.0f));

    EXPECT_FLOAT_EQ(bounds.position.x, 10.0f);
    EXPECT_FLOAT_EQ(bounds.position.y, 20.0f);
    EXPECT_FLOAT_EQ(bounds.size.x, 4.0f);
    EXPECT_FLOAT_EQ(bounds.size.y, 2.0f);
}

/**
 * @brief Un losange hors du cadrage est écarté comme n'importe quelle autre primitive : la
 *        primitive nouvelle n'échappe pas au culling.
 * \castest{<b>Un losange hors cadrage est ecarte, celui du cadrage est conserve.</b><br/>
 * \tcat Unitaire · PolyQuad<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Fixer un cadrage.<br/>2. Composer un losange dedans, puis un losange loin
 * dehors.<br/>
 * \tattendu Seul le losange interieur est conserve, et les compteurs le refletent.
 * }
 */
TEST(PolyQuadTest, CullingCommeLesAutres) {
    hmi::ComposedScene scene;
    scene.setVisibleBounds(core::Rect{core::Vector2{10.0f, 10.0f}, core::Vector2{20.0f, 10.0f}});

    EXPECT_TRUE(scene.addPoly(hmi::RenderLayer::Tile, solid, 0, diamond(15.0f, 15.0f, 2.0f, 1.0f)));
    EXPECT_FALSE(
        scene.addPoly(hmi::RenderLayer::Tile, solid, 0, diamond(800.0f, 15.0f, 2.0f, 1.0f)));

    EXPECT_EQ(scene.statistics().considered, 2);
    EXPECT_EQ(scene.statistics().culled, 1);
    EXPECT_EQ(scene.statistics().submitted, 1);
}

/**
 * @brief La primitive conservée est bien un `QuadKind::Poly`, et ses sommets sont repris tels
 *        quels : la composition ne les réordonne pas.
 * \castest{<b>Le quad compose porte bien la nature Poly et ses sommets intacts.</b><br/>
 * \tcat Unitaire · PolyQuad<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Composer un losange sans cadrage.<br/>
 * \tattendu Le quad capture est de nature Poly et porte les quatre sommets donnes.
 * }
 */
TEST(PolyQuadTest, NatureEtSommetsPreserves) {
    hmi::ComposedScene scene;
    const hmi::PolyQuad given = diamond(0.0f, 0.0f, 2.0f, 1.0f);

    ASSERT_TRUE(scene.addPoly(hmi::RenderLayer::Tile, solid, 7, given));

    const hmi::ComposedQuad& composed = scene.quads().front();
    EXPECT_EQ(composed.kind, hmi::QuadKind::Poly);
    EXPECT_EQ(composed.sortOrder, 7);
    for (std::size_t i = 0; i < 4; ++i) {
        EXPECT_FLOAT_EQ(composed.poly.x[i], given.x[i]);
        EXPECT_FLOAT_EQ(composed.poly.y[i], given.y[i]);
    }
}

/**
 * @brief Le tri ne connaît pas la nature des primitives : un losange de sol reste sous un sprite
 *        de décor, et se regroupe par texture comme les autres.
 * \castest{<b>Un losange se trie avec les autres primitives, par calque puis par texture.</b><br/>
 * \tcat Unitaire · PolyQuad<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Composer un sprite de decor, puis un losange de sol, puis un second losange de
 * sol.<br/>2. Trier.<br/>
 * \tattendu Les deux losanges (calque Tile) passent devant le sprite de decor (calque Object), et
 * le decor se dessine en dernier.
 * }
 */
TEST(PolyQuadTest, TrieAvecLesAutresPrimitives) {
    hmi::ComposedScene scene;
    hmi::SpriteQuad decor;
    decor.width = 1.0f;
    decor.height = 1.0f;

    scene.addSprite(hmi::RenderLayer::Object, other, 0, decor);
    scene.addPoly(hmi::RenderLayer::Tile, solid, 0, diamond(0.0f, 0.0f, 2.0f, 1.0f));
    scene.addPoly(hmi::RenderLayer::Tile, solid, 1, diamond(2.0f, 1.0f, 2.0f, 1.0f));
    scene.sort();

    ASSERT_EQ(scene.size(), 3U);
    EXPECT_EQ(scene.quads()[0].layer, hmi::RenderLayer::Tile);
    EXPECT_EQ(scene.quads()[1].layer, hmi::RenderLayer::Tile);
    EXPECT_EQ(scene.quads()[2].layer, hmi::RenderLayer::Object);
    // Les deux losanges partagent l'aplat : une seule passe pour eux, une pour le decor.
    EXPECT_EQ(scene.batchCount(), 2);
}
