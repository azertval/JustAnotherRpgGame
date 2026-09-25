// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_iso_projection.cpp
 * @brief Tests de la projection isométrique du Colisée : positions connues, orientation des axes,
 *        allers-retours case ↔ monde, et conformité aux formules de la scène QML qu'elle remplace.
 *
 * Les valeurs attendues sont calculées à la main dans les commentaires, pas par la fonction
 * testée : une erreur de signe ou d'échelle ici casserait en silence tout le rendu de l'arène.
 */

#include <algorithm>
#include <optional>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Math/Vector2.h"

namespace {

constexpr float TOLERANCE = 1e-4f;

/// Losange de 10 unités de large : H = 6,2, murs = 8,5 — des nombres qui se vérifient de tête.
constexpr float LARGEUR = 10.0f;

void expectPoint(const core::Vector2& obtenu, float x, float y) {
    EXPECT_NEAR(obtenu.x, x, TOLERANCE);
    EXPECT_NEAR(obtenu.y, y, TOLERANCE);
}

void expectCase(const std::optional<core::GridPosition>& obtenue, int column, int row) {
    ASSERT_TRUE(obtenue.has_value());
    EXPECT_EQ(obtenue->column, column);
    EXPECT_EQ(obtenue->row, row);
}

/// Transcription littérale de l'écran QML du Colisée (LOT-50, retiré) : la position en pixels
/// d'élément de la brique `ArenaTile` de la case (c, r), pour un élément de `largeur` × `hauteur`.
struct SceneQml {
    float largeur;
    float hauteur;
    int colonnes;
    int lignes;

    static constexpr float diamondRatio = 0.62f;
    static constexpr float wallRise = 0.85f;

    [[nodiscard]] float diagonals() const {
        return static_cast<float>(std::max(1, colonnes + lignes));
    }
    [[nodiscard]] float tileWidth() const {
        return std::max(8.0f, std::min(largeur / (diagonals() / 2),
                                       hauteur / (diagonals() / 2 * diamondRatio + wallRise)));
    }
    [[nodiscard]] float tileHeight() const {
        return tileWidth() * diamondRatio;
    }
    [[nodiscard]] float sceneWidth() const {
        return diagonals() / 2 * tileWidth();
    }
    [[nodiscard]] float sceneHeight() const {
        return diagonals() / 2 * tileHeight() + wallRise * tileWidth();
    }
    [[nodiscard]] float originX() const {
        return (largeur - sceneWidth()) / 2 + static_cast<float>(lignes - 1) * tileWidth() / 2;
    }
    [[nodiscard]] float originY() const {
        return (hauteur - sceneHeight()) / 2 + wallRise * tileWidth();
    }
    [[nodiscard]] float x(int c, int r) const {
        return originX() + static_cast<float>(c - r) * tileWidth() / 2;
    }
    [[nodiscard]] float y(int c, int r) const {
        return originY() + static_cast<float>(c + r) * tileHeight() / 2;
    }
};

}  // namespace

/**
 * @brief Les dimensions de la scène et du losange.
 * \castest{<b>Une grille 4 × 3 à losange de 10 unités : H = 6,2, murs 8,5, scène 35 × 30,2,
 * origine (10 ; 8,5).</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Construire la projection d'une grille 4 × 3, losange de 10 unités.<br/>2. Lire les
 * dimensions.<br/>
 * \tattendu diagonales 7 ; scène 7/2 × 10 = 35 de large, 7/2 × 6,2 + 8,5 = 30,2 de haut ; origine
 * ((3 − 1) × 5 ; 8,5).
 * }
 */
TEST(IsoProjectionTest, DimensionsDeLaScene) {
    const core::IsoProjection projection(4, 3, LARGEUR);

    EXPECT_NEAR(projection.tileWidth(), 10.0f, TOLERANCE);
    EXPECT_NEAR(projection.tileHeight(), 6.2f, TOLERANCE);
    EXPECT_NEAR(projection.wallHeight(), 8.5f, TOLERANCE);
    EXPECT_EQ(projection.diagonals(), 7);
    expectPoint(projection.sceneSize(), 35.0f, 30.2f);
    expectPoint(projection.origin(), 10.0f, 8.5f);
}

/**
 * @brief Les quatre cases d'angle touchent les bords de la scène.
 * \castest{<b>Sur une grille 4 × 3, la case (0, 2) touche le bord gauche, (3, 0) le bord droit,
 * (3, 2) le bord bas, et (0, 0) est posée sous la bande des murs.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire la boîte du losange des quatre cases d'angle.<br/>
 * \tattendu (0, 0) : (10 ; 8,5) ; (0, 2) : (0 ; 14,7) ; (3, 0) : (25 ; 17,8), bord droit 35 ;
 * (3, 2) : (15 ; 24), bord bas 30,2.
 * }
 */
TEST(IsoProjectionTest, LesCasesDAngleTouchentLesBords) {
    const core::IsoProjection projection(4, 3, LARGEUR);

    const core::Rect hautGauche = projection.tileBounds({0, 0});
    expectPoint(hautGauche.position, 10.0f, 8.5f);
    expectPoint(hautGauche.size, 10.0f, 6.2f);

    const core::Rect gauche = projection.tileBounds({0, 2});
    EXPECT_NEAR(gauche.left(), 0.0f, TOLERANCE);
    EXPECT_NEAR(gauche.top(), 14.7f, TOLERANCE);

    const core::Rect droite = projection.tileBounds({3, 0});
    EXPECT_NEAR(droite.left(), 25.0f, TOLERANCE);
    EXPECT_NEAR(droite.top(), 17.8f, TOLERANCE);
    EXPECT_NEAR(droite.right(), projection.sceneSize().x, TOLERANCE);

    const core::Rect bas = projection.tileBounds({3, 2});
    EXPECT_NEAR(bas.left(), 15.0f, TOLERANCE);
    EXPECT_NEAR(bas.top(), 24.0f, TOLERANCE);
    EXPECT_NEAR(bas.bottom(), projection.sceneSize().y, TOLERANCE);

    // Le point le plus haut du plateau est le sommet haut de (0, 0), juste sous la bande des murs.
    expectPoint(projection.gridToWorld({0.0f, 0.0f}), 15.0f, projection.wallHeight());
}

/**
 * @brief La case centrale d'une grille carrée est au centre du plateau.
 * \castest{<b>Sur une grille 5 × 5, le centre de la case (2, 2) est au milieu horizontal de la
 * scène et au milieu vertical du plateau (sous la bande des murs).</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Construire la projection 5 × 5, losange de 10.<br/>2. Lire le centre de (2, 2).<br/>
 * \tattendu (25 ; 8,5 + 31 / 2 = 24).
 * }
 */
TEST(IsoProjectionTest, LaCaseCentraleEstAuCentre) {
    const core::IsoProjection projection(5, 5, LARGEUR);

    expectPoint(projection.sceneSize(), 50.0f, 39.5f);
    expectPoint(projection.tileToWorld({2, 2}), 25.0f, 24.0f);
}

/**
 * @brief Le sens des axes : une colonne descend à droite, une ligne descend à gauche.
 * \castest{<b>Garde contre l'erreur de signe : un pas de colonne déplace le centre de (+L/2 ;
 * +H/2), un pas de ligne de (−L/2 ; +H/2).</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Comparer les centres de (1, 1), (2, 1) et (1, 2) sur une grille 4 × 3.<br/>2. Lire
 * les quatre sommets du losange de (1, 1) en coordonnées de grille continues.<br/>
 * \tattendu Colonne : (+5 ; +3,1) ; ligne : (−5 ; +3,1) ; sommets haut, droit, bas, gauche aux
 * milieux et coins de la boîte du losange.
 * }
 */
TEST(IsoProjectionTest, LeSensDesAxes) {
    const core::IsoProjection projection(4, 3, LARGEUR);
    const core::Vector2 centre = projection.tileToWorld({1, 1});

    const core::Vector2 colonneSuivante = projection.tileToWorld({2, 1});
    expectPoint(colonneSuivante - centre, 5.0f, 3.1f);

    const core::Vector2 ligneSuivante = projection.tileToWorld({1, 2});
    expectPoint(ligneSuivante - centre, -5.0f, 3.1f);

    const core::Rect boite = projection.tileBounds({1, 1});
    expectPoint(projection.gridToWorld({1.0f, 1.0f}), boite.left() + 5.0f, boite.top());   // haut
    expectPoint(projection.gridToWorld({2.0f, 1.0f}), boite.right(), boite.top() + 3.1f);  // droit
    expectPoint(projection.gridToWorld({2.0f, 2.0f}), boite.left() + 5.0f, boite.bottom());  // bas
    expectPoint(projection.gridToWorld({1.0f, 2.0f}), boite.left(), boite.top() + 3.1f);  // gauche
    expectPoint(projection.gridToWorld({1.5f, 1.5f}), boite.left() + 5.0f, boite.top() + 3.1f);
}

/**
 * @brief Aller-retour case → monde → case, sur toute une grille et près des sommets.
 * \castest{<b>Chaque case d'une grille 12 × 9, projetée à son centre puis relue, redonne la même
 * case ; de même pour quatre points pris juste à l'intérieur de ses sommets.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Pour chaque case : centre → `worldToTile`.<br/>2. Pour chaque case : les points de
 * grille (c + 0,02 ; r + 0,02), (c + 0,98 ; r + 0,02), (c + 0,02 ; r + 0,98), (c + 0,98 ;
 * r + 0,98) → monde → `worldToTile`.<br/>
 * \tattendu Toujours la case de départ.
 * }
 */
TEST(IsoProjectionTest, AllerRetourCaseMondeCase) {
    const core::IsoProjection projection(12, 9);

    for (int r = 0; r < projection.rows(); ++r) {
        for (int c = 0; c < projection.columns(); ++c) {
            expectCase(projection.worldToTile(projection.tileToWorld({c, r})), c, r);
            for (const float dc : {0.02f, 0.98f}) {
                for (const float dr : {0.02f, 0.98f}) {
                    const core::Vector2 point = projection.gridToWorld(
                        {static_cast<float>(c) + dc, static_cast<float>(r) + dr});
                    expectCase(projection.worldToTile(point), c, r);
                }
            }
        }
    }
}

/**
 * @brief Aller-retour continu grille → monde → grille.
 * \castest{<b>`worldToGrid` est l'inverse de `gridToWorld`, y compris hors de la grille.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Parcourir un quadrillage de points continus de −3 à 15 par pas de 0,37 sur une grille
 * 12 × 9, losange de 7,3.<br/>2. Projeter, puis relire.<br/>
 * \tattendu Le point de départ, à 1e-3 près.
 * }
 */
TEST(IsoProjectionTest, AllerRetourContinu) {
    const core::IsoProjection projection(12, 9, 7.3f);

    constexpr int PAS = 49;  // 49 × 0,37 ≈ 18 : de −3 à 15.
    for (int i = 0; i < PAS; ++i) {
        for (int j = 0; j < PAS; ++j) {
            const float gc = -3.0f + 0.37f * static_cast<float>(i);
            const float gr = -3.0f + 0.37f * static_cast<float>(j);
            const core::Vector2 relu = projection.worldToGrid(projection.gridToWorld({gc, gr}));
            EXPECT_NEAR(relu.x, gc, 1e-3f);
            EXPECT_NEAR(relu.y, gr, 1e-3f);
        }
    }
}

/**
 * @brief La relecture suit le losange, pas sa boîte, et s'arrête aux bords de la grille.
 * \castest{<b>Un point dans un coin de la boîte d'une case, hors de son losange, appartient à la
 * voisine ; les arêtes appartiennent à la case de plus grand indice ; hors grille, rien.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Grille 5 × 5, losange 4, rapport 0,5, sans murs (valeurs exactes en binaire).<br/>
 * 2. Lire les quatre coins de la boîte de (2, 2), rentrés d'un centième.<br/>3. Lire les sommets
 * exacts de (2, 2).<br/>4. Lire le centre projeté de (−1, 0), (5, 2), (2, 5) et un point au-dessus
 * de la scène.<br/>
 * \tattendu Coins : (1, 2), (2, 1), (2, 3), (3, 2) ; sommet haut → (2, 2), sommet droit → (3, 2),
 * sommet gauche → (2, 3), sommet bas → (3, 3) ; hors grille : rien.
 * }
 */
TEST(IsoProjectionTest, LaRelectureSuitLeLosange) {
    const core::IsoProjection projection(5, 5, 4.0f, 0.5f, 0.0f);
    const core::Rect boite = projection.tileBounds({2, 2});
    constexpr float RENTRE = 0.01f;

    expectCase(projection.worldToTile({boite.left() + RENTRE, boite.top() + RENTRE}), 1, 2);
    expectCase(projection.worldToTile({boite.right() - RENTRE, boite.top() + RENTRE}), 2, 1);
    expectCase(projection.worldToTile({boite.left() + RENTRE, boite.bottom() - RENTRE}), 2, 3);
    expectCase(projection.worldToTile({boite.right() - RENTRE, boite.bottom() - RENTRE}), 3, 2);

    expectCase(projection.worldToTile(projection.gridToWorld({2.0f, 2.0f})), 2, 2);
    expectCase(projection.worldToTile(projection.gridToWorld({3.0f, 2.0f})), 3, 2);
    expectCase(projection.worldToTile(projection.gridToWorld({2.0f, 3.0f})), 2, 3);
    expectCase(projection.worldToTile(projection.gridToWorld({3.0f, 3.0f})), 3, 3);

    EXPECT_FALSE(projection.worldToTile(projection.tileToWorld({-1, 0})).has_value());
    EXPECT_FALSE(projection.worldToTile(projection.tileToWorld({5, 2})).has_value());
    EXPECT_FALSE(projection.worldToTile(projection.tileToWorld({2, 5})).has_value());
    EXPECT_FALSE(projection.worldToTile({10.0f, -1.0f}).has_value());
    EXPECT_FALSE(projection.contains({0, 5}));
    EXPECT_TRUE(projection.contains({4, 4}));
}

/**
 * @brief Conformité à la scène QML du LOT-50 : même dessin, à une homothétie et une translation
 *        près.
 * \castest{<b>Pour plusieurs tailles d'élément et de grille, la position QML de chaque brique
 * `ArenaTile` vaut décalage + échelle × position monde, où l'échelle est le rapport des largeurs de
 * losange et le décalage celui qui centre la scène.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Transcrire littéralement les formules de l'écran QML du Colisée (LOT-50).<br/>2.
 * Pour des éléments 1280 × 720, 800 × 900, 640 × 360 et des grilles 16 × 12, 9 × 14, 1 × 1 :
 * comparer la position de chaque case.<br/>
 * \tattendu Égalité à 1e-2 pixel près, pour chaque case.
 * }
 */
TEST(IsoProjectionTest, ConformeALaSceneQml) {
    struct Cas {
        float largeur;
        float hauteur;
        int colonnes;
        int lignes;
    };
    for (const Cas& cas : {Cas{1280, 720, 16, 12}, Cas{800, 900, 9, 14}, Cas{640, 360, 1, 1},
                           Cas{1280, 720, 9, 14}}) {
        const SceneQml qml{cas.largeur, cas.hauteur, cas.colonnes, cas.lignes};
        const core::IsoProjection projection(cas.colonnes, cas.lignes);

        const float echelle = qml.tileWidth() / projection.tileWidth();
        const core::Vector2 scene = projection.sceneSize() * echelle;
        EXPECT_NEAR(scene.x, qml.sceneWidth(), 1e-2f);
        EXPECT_NEAR(scene.y, qml.sceneHeight(), 1e-2f);
        const core::Vector2 decalage{(cas.largeur - scene.x) / 2, (cas.hauteur - scene.y) / 2};

        for (int r = 0; r < cas.lignes; ++r) {
            for (int c = 0; c < cas.colonnes; ++c) {
                const core::Vector2 pixel =
                    decalage + projection.tileBounds({c, r}).position * echelle;
                EXPECT_NEAR(pixel.x, qml.x(c, r), 1e-2f) << "case (" << c << ", " << r << ")";
                EXPECT_NEAR(pixel.y, qml.y(c, r), 1e-2f) << "case (" << c << ", " << r << ")";
            }
        }
    }
}

/**
 * @brief Valeurs par défaut, profondeur et entrées dégénérées.
 * \castest{<b>Le losange par défaut fait 86 / 16 unités (la tuile de la planche à sa taille native
 * au zoom 1) ; la profondeur est colonne + ligne ; une taille négative devient nulle, une largeur
 * nulle devient celle par défaut, une grille vide garde une diagonale.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Construire une projection par défaut, puis des projections dégénérées.<br/>
 * \tattendu L = 5,375 ; H = 3,3325 ; profondeur (3, 4) = 7 ; grille (−2, −1) : 0 × 0, diagonales
 * 1 ; largeur 0 → 5,375.
 * }
 */
TEST(IsoProjectionTest, ValeursParDefautEtEntreesDegenerees) {
    const core::IsoProjection defaut(8, 6);
    EXPECT_NEAR(defaut.tileWidth(), 5.375f, TOLERANCE);
    EXPECT_NEAR(defaut.tileHeight(), 3.3325f, TOLERANCE);
    EXPECT_EQ(core::IsoProjection::depth({3, 4}), 7);

    const core::IsoProjection vide(-2, -1, 0.0f);
    EXPECT_EQ(vide.columns(), 0);
    EXPECT_EQ(vide.rows(), 0);
    EXPECT_EQ(vide.diagonals(), 1);
    EXPECT_NEAR(vide.tileWidth(), core::ARENA_TILE_WIDTH_UNITS, TOLERANCE);
    EXPECT_FALSE(vide.worldToTile(vide.tileToWorld({0, 0})).has_value());
}
