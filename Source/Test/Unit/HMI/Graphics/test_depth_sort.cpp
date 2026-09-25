// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_depth_sort.cpp
 * @brief Tests unitaires du tri par profondeur en vue de dessus (`EX-REN-018`, LOT-07).
 *        Composition pure, sans GPU (`EX-NFR-004`).
 */

#include <vector>

#include <gtest/gtest.h>

#include "Core/Math/Vector2.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/QuadRecorder.h"
#include "HMI/Graphics/RenderLayer.h"

namespace {

// Deux identites de texture distinctes : la composition ne fait que les comparer, jamais les
// dereferencer (EX-NFR-004). C'est le point du lot -- personnage et decor n'ont PAS la meme.
int firstTextureStorage = 0;
int secondTextureStorage = 0;
hmi::TextureHandle textureA = &firstTextureStorage;
hmi::TextureHandle textureB = &secondTextureStorage;

// Rectangle d'une case, dont le PIED (bord bas) est a @p footY.
hmi::SpriteQuad quadWithFoot(float x, float footY) {
    hmi::SpriteQuad quad;
    quad.x = x;
    quad.y = footY - 1.0f;
    quad.width = 1.0f;
    quad.height = 1.0f;
    return quad;
}

}  // namespace

/**
 * @brief Trois primitives de la bande de profondeur, à Y croissants, sortent dans l'ordre de leur
 * pied — **même quand leurs textures diffèrent** (`EX-REN-018`).
 *
 * C'est le cœur du lot : avant lui, le regroupement par texture tranchait avant le tri fin, et
 * l'ordre de profondeur ne survivait pas à deux textures.
 * \castest{<b>Trois primitives a Y croissants sortent dans l'ordre de leur pied.</b><br/>
 * \tcat Unitaire · Tri par profondeur<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Composer trois primitives de la bande de profondeur, dans le desordre, avec deux
 * textures differentes.<br/>2. Trier la scene.<br/>
 * \tattendu Les trois sortent par pied croissant, quelle que soit leur texture.
 * }
 */
TEST(TriParProfondeurTest, TroisPrimitivesSortentParPiedCroissant) {
    hmi::ComposedScene scene;
    scene.addSprite(hmi::RenderLayer::Object, textureA, hmi::depthSortOrder(6.0f),
                    quadWithFoot(0.0f, 6.0f));
    scene.addSprite(hmi::RenderLayer::Player, textureB, hmi::depthSortOrder(2.0f),
                    quadWithFoot(1.0f, 2.0f));
    scene.addSprite(hmi::RenderLayer::Object, textureB, hmi::depthSortOrder(4.0f),
                    quadWithFoot(2.0f, 4.0f));
    scene.sort();

    ASSERT_EQ(scene.size(), 3u);
    EXPECT_FLOAT_EQ(scene.quads()[0].sprite.x, 1.0f);  // pied 2
    EXPECT_FLOAT_EQ(scene.quads()[1].sprite.x, 2.0f);  // pied 4
    EXPECT_FLOAT_EQ(scene.quads()[2].sprite.x, 0.0f);  // pied 6
}

/**
 * @brief Le personnage passe **derrière** un objet plus bas et **devant** un objet plus haut : le
 * rang de calque ne tranche plus à l'intérieur de la bande (`EX-REN-018`).
 * \castest{<b>Le personnage passe derriere un objet plus bas et devant un objet plus haut.</b><br/>
 * \tcat Unitaire · Tri par profondeur<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Composer un arbre au-dessus du personnage, le personnage, puis un arbre en
 * dessous.<br/>2. Trier.<br/>
 * \tattendu L'arbre du haut sort avant le personnage, celui du bas apres.
 * }
 */
TEST(TriParProfondeurTest, PersonnageEntreDeuxObjets) {
    hmi::ComposedScene scene;
    scene.addSprite(hmi::RenderLayer::Object, textureA, hmi::depthSortOrder(3.0f),
                    quadWithFoot(0.0f, 3.0f));  // arbre plus haut a l'ecran
    scene.addSprite(hmi::RenderLayer::Player, textureB, hmi::depthSortOrder(5.0f),
                    quadWithFoot(1.0f, 5.0f));  // personnage
    scene.addSprite(hmi::RenderLayer::Object, textureA, hmi::depthSortOrder(7.0f),
                    quadWithFoot(2.0f, 7.0f));  // arbre plus bas a l'ecran
    scene.sort();

    ASSERT_EQ(scene.size(), 3u);
    EXPECT_EQ(scene.quads()[0].layer, hmi::RenderLayer::Object);
    EXPECT_EQ(scene.quads()[1].layer, hmi::RenderLayer::Player);
    EXPECT_EQ(scene.quads()[2].layer, hmi::RenderLayer::Object);
    EXPECT_FLOAT_EQ(scene.quads()[2].sprite.x, 2.0f);
}

/**
 * @brief À pied égal, l'ordre reste celui de la composition : deux sprites ne peuvent pas
 * scintiller d'une image à l'autre (`EX-REN-018`).
 * \castest{<b>A pied egal, l'ordre de composition est preserve : aucun scintillement.</b><br/>
 * \tcat Unitaire · Tri par profondeur<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Composer trois primitives de meme pied, a des positions differentes.<br/>2.
 * Trier.<br/>3. Recomposer et retrier a l'identique.<br/>
 * \tattendu Les deux tris rendent exactement le meme ordre.
 * }
 */
TEST(TriParProfondeurTest, PiedEgalConserveLOrdreDeComposition) {
    const auto composer = [] {
        hmi::ComposedScene scene;
        scene.addSprite(hmi::RenderLayer::Object, textureA, hmi::depthSortOrder(4.0f),
                        quadWithFoot(3.0f, 4.0f));
        scene.addSprite(hmi::RenderLayer::Player, textureB, hmi::depthSortOrder(4.0f),
                        quadWithFoot(1.0f, 4.0f));
        scene.addSprite(hmi::RenderLayer::Object, textureA, hmi::depthSortOrder(4.0f),
                        quadWithFoot(2.0f, 4.0f));
        scene.sort();
        std::vector<float> order;
        for (const hmi::ComposedQuad& quad : scene.quads()) {
            order.push_back(quad.sprite.x);
        }
        return order;
    };

    const std::vector<float> first = composer();
    const std::vector<float> second = composer();
    EXPECT_EQ(first, second);
    // Et l'ordre est bien celui de la composition, texture par texture regroupee ensuite.
    ASSERT_EQ(first.size(), 3u);
    EXPECT_FLOAT_EQ(first[0], 3.0f);
}

/**
 * @brief Un écart de moins d'un pixel ne départage pas deux profondeurs : la quantification au
 * pixel évite qu'un arrondi flottant fasse permuter deux sprites (`EX-REN-018`).
 * \castest{<b>Un ecart inferieur au pixel ne departage pas deux profondeurs.</b><br/>
 * \tcat Unitaire · Tri par profondeur<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Calculer l'ordre de deux pieds distants d'un centieme d'unite.<br/>2. Le comparer a
 * celui de deux pieds distants d'une demi-unite.<br/>
 * \tattendu Le premier couple partage le meme ordre, le second non.
 * }
 */
TEST(TriParProfondeurTest, QuantificationAuPixel) {
    EXPECT_EQ(hmi::depthSortOrder(4.0f), hmi::depthSortOrder(4.01f));
    EXPECT_LT(hmi::depthSortOrder(4.0f), hmi::depthSortOrder(4.5f));
    // Un Y plus grand (plus bas a l'ecran) se dessine plus tard, donc au-dessus.
    EXPECT_LT(hmi::depthSortOrder(1.0f), hmi::depthSortOrder(9.0f));
}

/**
 * @brief Les bandes restent hiérarchisées : la profondeur ne fait jamais passer un objet devant
 * l'interface, ni sous les tuiles (`EX-REN-014`).
 * \castest{<b>La profondeur ne deborde pas de sa bande.</b><br/>
 * \tcat Unitaire · Tri par profondeur<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Composer une tuile, un objet tres bas, un element d'interface tres haut.<br/>2.
 * Trier.<br/>
 * \tattendu La tuile sort en premier, l'objet ensuite, l'interface en dernier.
 * }
 */
TEST(TriParProfondeurTest, LaProfondeurNeDebordePasDeSaBande) {
    hmi::ComposedScene scene;
    scene.addSprite(hmi::RenderLayer::UI, textureA, hmi::depthSortOrder(0.0f),
                    quadWithFoot(0.0f, 0.0f));
    scene.addSprite(hmi::RenderLayer::Object, textureA, hmi::depthSortOrder(99.0f),
                    quadWithFoot(1.0f, 99.0f));
    scene.addSprite(hmi::RenderLayer::Tile, textureA, 0, quadWithFoot(2.0f, 50.0f));
    scene.sort();

    hmi::QuadRecorder recorder;
    recorder.record(scene);

    ASSERT_EQ(scene.size(), 3u);
    EXPECT_EQ(scene.quads()[0].layer, hmi::RenderLayer::Tile);
    EXPECT_EQ(scene.quads()[1].layer, hmi::RenderLayer::Object);
    EXPECT_EQ(scene.quads()[2].layer, hmi::RenderLayer::UI);
    EXPECT_TRUE(recorder.isLayerOrderRespected()) << recorder.describe();
}
