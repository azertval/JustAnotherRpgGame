// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_maquette_tokens.cpp
 * @brief Tests unitaires des jetons de maquette (LOT-128, décision D2) : leur chemin, leur lettre
 *        et l'image qu'ils peignent.
 */

#include <gtest/gtest.h>

#include "HMI/Graphics/MaquetteTokens.h"

/**
 * @brief La lettre d'un jeton est le premier caractère alphanumérique de son nom, en majuscule.
 * \castest{<b>La lettre d'un jeton est la premiere alphanumerique de son nom.</b><br/>
 * \tcat Unitaire · Jetons de maquette<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander la lettre de plusieurs noms, dont un vide et un sans lettre.<br/>
 * \tattendu La premiere alphanumerique en majuscule, et « ? » a defaut.
 * }
 */
TEST(MaquetteTokenTest, LaLettreEstLaPremiereAlphanumerique) {
    EXPECT_EQ(hmi::maquetteTokenLetter("market-mother"), 'M');
    EXPECT_EQ(hmi::maquetteTokenLetter("-- arenarea"), 'A');
    EXPECT_EQ(hmi::maquetteTokenLetter("2e-porte"), '2');
    EXPECT_EQ(hmi::maquetteTokenLetter(""), '?');
    EXPECT_EQ(hmi::maquetteTokenLetter("---"), '?');
}

/**
 * @brief Un jeton s'adresse par un chemin, et ce chemin se relit : c'est ce qui permet au rendu de
 *        le peindre au lieu de le charger.
 * \castest{<b>Le chemin d'un jeton se relit en sa nature et sa lettre.</b><br/>
 * \tcat Unitaire · Jetons de maquette<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ecrire puis relire le chemin de chaque nature.<br/>2. Relire des chemins qui n'en
 * sont pas.<br/>
 * \tattendu L'aller-retour rend la demande de depart ; les autres chemins sont refuses.
 * }
 */
TEST(MaquetteTokenTest, LeCheminSeRelit) {
    for (const hmi::MaquetteTokenKind kind :
         {hmi::MaquetteTokenKind::Player, hmi::MaquetteTokenKind::Talker,
          hmi::MaquetteTokenKind::Neutral, hmi::MaquetteTokenKind::Hostile,
          hmi::MaquetteTokenKind::Object, hmi::MaquetteTokenKind::Portal}) {
        const std::string path = hmi::maquetteTokenPath(kind, 'W');
        const auto relu = hmi::parseMaquetteTokenPath(path);
        ASSERT_TRUE(relu.has_value()) << path;
        EXPECT_EQ(relu->kind, kind);
        EXPECT_EQ(relu->letter, 'W');
    }
    EXPECT_FALSE(hmi::parseMaquetteTokenPath("Scene/coliseum/sand.png").has_value());
    EXPECT_FALSE(hmi::parseMaquetteTokenPath("Npc/anariel/idle.png").has_value());
    EXPECT_FALSE(hmi::parseMaquetteTokenPath("Token/inconnu/W.png").has_value());
    EXPECT_FALSE(hmi::parseMaquetteTokenPath("Token/player/MOT.png").has_value());
}

/**
 * @brief Le chemin d'un jeton porte toujours une lettre valide : un nom illisible donne `?`, pas
 *        un chemin cassé.
 * \castest{<b>Un nom illisible donne le chemin du point d'interrogation.</b><br/>
 * \tcat Unitaire · Jetons de maquette<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ecrire le chemin d'un jeton dont la lettre est un caractere de ponctuation.<br/>
 * \tattendu Le chemin porte « ? » et se relit.
 * }
 */
TEST(MaquetteTokenTest, UneLettreIllisibleDonneUnPointDInterrogation) {
    const std::string path = hmi::maquetteTokenPath(hmi::MaquetteTokenKind::Neutral, '-');
    const auto relu = hmi::parseMaquetteTokenPath(path);
    ASSERT_TRUE(relu.has_value());
    EXPECT_EQ(relu->letter, '?');
}

/**
 * @brief L'image d'un jeton est un disque plein cerné, sa lettre au centre, le reste transparent —
 *        et la même demande donne toujours la même image.
 * \castest{<b>L'image d'un jeton est un disque a lettre, deterministe.</b><br/>
 * \tcat Unitaire · Jetons de maquette<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Peindre deux fois le meme jeton.<br/>2. Examiner le centre, un coin, et deux
 * lettres differentes.<br/>
 * \tattendu Les deux images sont identiques ; les coins sont transparents ; deux lettres donnent
 * deux images differentes.
 * }
 */
TEST(MaquetteTokenTest, LImageEstUnDisqueALettreDeterministe) {
    const hmi::MaquetteTokenRequest demande{.kind = hmi::MaquetteTokenKind::Hostile, .letter = 'W'};
    const core::MarkerImage premiere = hmi::maquetteTokenImage(demande, 44);
    const core::MarkerImage seconde = hmi::maquetteTokenImage(demande, 44);

    ASSERT_FALSE(premiere.isEmpty());
    EXPECT_EQ(premiere.width, 44);
    EXPECT_EQ(premiere.height, 44);
    EXPECT_EQ(premiere.pixels, seconde.pixels);
    // Les coins sont hors du disque : transparents, pour que le sol se voie autour.
    EXPECT_EQ(premiere.at(0, 0).a, 0);
    EXPECT_EQ(premiere.at(43, 43).a, 0);
    // Le centre est plein.
    EXPECT_EQ(premiere.at(22, 22).a, 255);

    const core::MarkerImage autre = hmi::maquetteTokenImage(
        hmi::MaquetteTokenRequest{.kind = hmi::MaquetteTokenKind::Hostile, .letter = 'X'}, 44);
    EXPECT_NE(premiere.pixels, autre.pixels);
}

/**
 * @brief Une image de taille nulle, ou un chemin qui n'est pas celui d'un jeton, rend une image
 *        vide plutôt qu'une exception.
 * \castest{<b>Une demande impossible rend une image vide.</b><br/>
 * \tcat Unitaire · Jetons de maquette<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander un jeton de cote nul, puis l'image d'un chemin de planche.<br/>
 * \tattendu Deux images vides.
 * }
 */
TEST(MaquetteTokenTest, UneDemandeImpossibleRendUneImageVide) {
    EXPECT_TRUE(hmi::maquetteTokenImage(hmi::MaquetteTokenRequest{}, 0).isEmpty());
    EXPECT_TRUE(hmi::maquetteTokenImage("Scene/coliseum/sand.png", 44).isEmpty());
}

/**
 * @brief Chaque nature a sa teinte, et deux natures n'en partagent pas : c'est tout ce que la
 *        couleur d'un jeton doit garantir.
 * \castest{<b>Les six natures de jeton ont six teintes distinctes.</b><br/>
 * \tcat Unitaire · Jetons de maquette<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Comparer les teintes des six natures deux a deux.<br/>
 * \tattendu Aucune paire identique.
 * }
 */
TEST(MaquetteTokenTest, LesSixNaturesOntSixTeintes) {
    const std::vector<hmi::MaquetteTokenKind> natures = {
        hmi::MaquetteTokenKind::Player,  hmi::MaquetteTokenKind::Talker,
        hmi::MaquetteTokenKind::Neutral, hmi::MaquetteTokenKind::Hostile,
        hmi::MaquetteTokenKind::Object,  hmi::MaquetteTokenKind::Portal};
    for (std::size_t i = 0; i < natures.size(); ++i) {
        for (std::size_t j = i + 1; j < natures.size(); ++j) {
            EXPECT_FALSE(hmi::maquetteTokenColor(natures[i]) == hmi::maquetteTokenColor(natures[j]))
                << i << " et " << j;
            EXPECT_NE(hmi::maquetteTokenKindKey(natures[i]), hmi::maquetteTokenKindKey(natures[j]));
        }
    }
}
