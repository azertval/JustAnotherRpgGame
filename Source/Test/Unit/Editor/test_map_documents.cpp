// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_map_documents.cpp
 * @brief Tests des **cartes ouvertes en onglets** (`LOT-EDITOR-09`, `EX-EDIT-088`) : le libellé
 *        d'un onglet, celui qu'une ouverture vise, celui qui revient après une fermeture.
 */

#include <gtest/gtest.h>

#include "Editor/Logic/MapDocuments.h"

/**
 * @brief Un onglet porte le dernier segment de l'identifiant, et une étoile s'il est modifié.
 * \castest{<b>Un onglet dit sa carte et ses modifications.</b><br/>
 * \tcat Unitaire · Le monde<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander le libellé d'une carte de sous-dossier, propre puis modifiée.<br/>
 * \tattendu `place`, puis `place *` ; une carte sans identifiant se dit `untitled`.
 * }
 */
TEST(CartesEnOnglets, UnOngletDitSaCarteEtSesModifications) {
    EXPECT_EQ(hmi::documentLabel("bourg/place", false), "place");
    EXPECT_EQ(hmi::documentLabel("bourg/place", true), "place *");
    EXPECT_EQ(hmi::documentLabel("donjon", false), "donjon");
    EXPECT_EQ(hmi::documentLabel("", true), "untitled *");
}

/**
 * @brief Ouvrir une carte déjà ouverte y revient ; une carte neuve n'est jamais « déjà ouverte ».
 * \castest{<b>Une carte déjà ouverte ne s'ouvre pas deux fois.</b><br/>
 * \tcat Unitaire · Le monde<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Chercher une carte ouverte, une carte absente, puis une carte sans
 * identifiant.<br/>
 * \tattendu Le rang de la première, rien pour les deux autres.
 * }
 */
TEST(CartesEnOnglets, UneCarteDejaOuverteNeSouvrePasDeuxFois) {
    const std::vector<hmi::OpenDocument> ouvertes{{.mapId = "donjon", .dirty = false},
                                                  {.mapId = "bourg/place", .dirty = true},
                                                  {.mapId = "", .dirty = true}};
    EXPECT_EQ(hmi::documentOf(ouvertes, "bourg/place"), 1U);
    EXPECT_FALSE(hmi::documentOf(ouvertes, "bourg/absente").has_value());
    EXPECT_FALSE(hmi::documentOf(ouvertes, "").has_value());
    EXPECT_EQ(hmi::dirtyDocuments(ouvertes), (std::vector<std::string>{"bourg/place", ""}));
}

/**
 * @brief Fermer un onglet donne la main à son voisin de droite, ou à celui de gauche pour le
 *        dernier.
 * \castest{<b>Fermer un onglet donne la main au voisin.</b><br/>
 * \tcat Unitaire · Le monde<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Fermer le premier, puis le dernier, puis le seul onglet de trois, trois, un.<br/>
 * \tattendu Le rang 0, le rang 1, puis rien.
 * }
 */
TEST(CartesEnOnglets, FermerUnOngletDonneLaMainAuVoisin) {
    EXPECT_EQ(hmi::documentAfterClose(3, 0), 0U);
    EXPECT_EQ(hmi::documentAfterClose(3, 1), 1U);
    EXPECT_EQ(hmi::documentAfterClose(3, 2), 1U);
    EXPECT_FALSE(hmi::documentAfterClose(1, 0).has_value());
    EXPECT_FALSE(hmi::documentAfterClose(3, 7).has_value());
}
