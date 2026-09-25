// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_screen_flow.cpp
 * @brief Tests unitaires de la machine à états des écrans (EX-GP-041). Logique
 *        pure, sans Qt.
 */

#include <gtest/gtest.h>

#include "HMI/Presentation/ScreenFlow.h"

namespace {

using hmi::resolveTransition;
using hmi::ScreenEvent;
using hmi::ScreenId;
using hmi::ScreenState;

}  // namespace

/**
 * @brief Chaque transition autorisée mène à l'écran attendu.
 * \castest{<b>Chaque transition autorisée mène à l'écran attendu.</b><br/>
 * \tcat Unitaire · Machine à états des écrans<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Résoudre chacune des transitions autorisées listées.<br/>2. Vérifier l'écran
 * résultant.<br/>
 * \tattendu Chaque transition mène à l'écran attendu.
 * }
 */
TEST(ScreenFlowTest, TransitionsAutoriseesMenentALEcranAttendu) {
    const ScreenState menu{.screen = ScreenId::Menu, .optionsReturnTo = ScreenId::Menu};
    const ScreenState game{.screen = ScreenId::Game, .optionsReturnTo = ScreenId::Menu};
    const ScreenState pause{.screen = ScreenId::Pause, .optionsReturnTo = ScreenId::Menu};
    const ScreenState credits{.screen = ScreenId::Credits, .optionsReturnTo = ScreenId::Menu};
    const ScreenState optionsFromMenu{.screen = ScreenId::Options,
                                      .optionsReturnTo = ScreenId::Menu};
    const ScreenState optionsFromPause{.screen = ScreenId::Options,
                                       .optionsReturnTo = ScreenId::Pause};

    EXPECT_EQ(resolveTransition(menu, ScreenEvent::OpenGame)->screen, ScreenId::Game);
    EXPECT_EQ(resolveTransition(menu, ScreenEvent::OpenOptions)->screen, ScreenId::Options);
    EXPECT_EQ(resolveTransition(game, ScreenEvent::OpenMenu)->screen, ScreenId::Menu);
    EXPECT_EQ(resolveTransition(game, ScreenEvent::OpenPause)->screen, ScreenId::Pause);
    EXPECT_EQ(resolveTransition(pause, ScreenEvent::ResumePause)->screen, ScreenId::Game);
    EXPECT_EQ(resolveTransition(pause, ScreenEvent::QuitPauseToMenu)->screen, ScreenId::Menu);
    EXPECT_EQ(resolveTransition(pause, ScreenEvent::OpenOptions)->screen, ScreenId::Options);
    EXPECT_EQ(resolveTransition(optionsFromMenu, ScreenEvent::CloseOptions)->screen,
              ScreenId::Menu);
    EXPECT_EQ(resolveTransition(optionsFromPause, ScreenEvent::CloseOptions)->screen,
              ScreenId::Pause);
    EXPECT_EQ(resolveTransition(pause, ScreenEvent::QuitPauseToMenu)->screen, ScreenId::Menu);
    EXPECT_EQ(resolveTransition(menu, ScreenEvent::OpenCredits)->screen, ScreenId::Credits);
    EXPECT_EQ(resolveTransition(credits, ScreenEvent::CloseCredits)->screen, ScreenId::Menu);
    // Ecrans du RPG (LOT-68) : atteignables depuis le menu (echafaudage de « Nouvelle partie »),
    // depuis le jeu et depuis la pause.
    EXPECT_EQ(resolveTransition(menu, ScreenEvent::OpenRpgScreen)->screen, ScreenId::RpgScreen);
    EXPECT_EQ(resolveTransition(game, ScreenEvent::OpenRpgScreen)->screen, ScreenId::RpgScreen);
    EXPECT_EQ(resolveTransition(pause, ScreenEvent::OpenRpgScreen)->screen, ScreenId::RpgScreen);
}

/**
 * @brief Options revient vers l'écran d'où il a été ouvert : Menu si ouvert depuis Menu, Pause si
 *        ouvert depuis Pause -- porté par l'état, pas par une variable à part.
 * \castest{<b>Options revient vers son écran d'origine (Menu ou Pause).</b><br/>
 * \tcat Unitaire · Machine à états des écrans<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ouvrir Options depuis Menu, puis le fermer : vérifier le retour au Menu.<br/>2.
 * Ouvrir Options depuis Pause, puis le fermer : vérifier le retour à Pause.<br/>
 * \tattendu Chaque fermeture revient à l'écran d'origine respectif.
 * }
 */
TEST(ScreenFlowTest, OptionsRevientVersSonEcranDOrigine) {
    const ScreenState menu{.screen = ScreenId::Menu, .optionsReturnTo = ScreenId::Menu};
    const ScreenState pause{.screen = ScreenId::Pause, .optionsReturnTo = ScreenId::Menu};

    const std::optional<ScreenState> openedFromMenu =
        resolveTransition(menu, ScreenEvent::OpenOptions);
    ASSERT_TRUE(openedFromMenu.has_value());
    EXPECT_EQ(resolveTransition(*openedFromMenu, ScreenEvent::CloseOptions)->screen,
              ScreenId::Menu);

    const std::optional<ScreenState> openedFromPause =
        resolveTransition(pause, ScreenEvent::OpenOptions);
    ASSERT_TRUE(openedFromPause.has_value());
    EXPECT_EQ(resolveTransition(*openedFromPause, ScreenEvent::CloseOptions)->screen,
              ScreenId::Pause);
}

/**
 * @brief Une transition interdite est refusée (std::nullopt), notamment Menu -> Pause et
 *        Game -> Crédits.
 * \castest{<b>Une transition interdite est refusée.</b><br/>
 * \tcat Unitaire · Machine à états des écrans<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Tenter Menu -> Pause directement.<br/>2. Tenter Game -> Crédits.<br/>3. Tenter
 * Pause -> Colisée.<br/>
 * \tattendu Chaque tentative renvoie std::nullopt.
 * }
 */
TEST(ScreenFlowTest, TransitionInterditeEstRefusee) {
    const ScreenState menu{.screen = ScreenId::Menu, .optionsReturnTo = ScreenId::Menu};
    const ScreenState game{.screen = ScreenId::Game, .optionsReturnTo = ScreenId::Menu};
    const ScreenState pause{.screen = ScreenId::Pause, .optionsReturnTo = ScreenId::Menu};

    EXPECT_EQ(resolveTransition(menu, ScreenEvent::OpenPause), std::nullopt);
    EXPECT_EQ(resolveTransition(menu, ScreenEvent::ResumePause), std::nullopt);
    // Crédits : même règle, atteignable seulement depuis le menu.
    EXPECT_EQ(resolveTransition(game, ScreenEvent::OpenCredits), std::nullopt);
    // Ecrans du RPG (LOT-68) : on n'en referme pas un qui n'est pas ouvert.
    EXPECT_EQ(resolveTransition(menu, ScreenEvent::CloseRpgScreen), std::nullopt);
}

/**
 * @brief Un écran du RPG revient vers l'écran d'où il a été ouvert -- menu, jeu ou pause -- porté
 *        par l'état, comme la provenance d'Options (`LOT-68`, `EX-IHM-090`).
 * \castest{<b>Un ecran du RPG revient vers son ecran d'origine (Menu, Game ou Pause).</b><br/>
 * \tcat Unitaire · Machine à états des écrans<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ouvrir un ecran du RPG depuis le menu, le jeu puis la pause.<br/>2. Le fermer a
 * chaque fois et verifier l'ecran atteint.<br/>
 * \tattendu Chaque fermeture revient a l'ecran d'origine respectif.
 * }
 */
TEST(ScreenFlowTest, EcranDuRpgRevientVersSonEcranDOrigine) {
    const ScreenState menu{.screen = ScreenId::Menu, .optionsReturnTo = ScreenId::Menu};
    const ScreenState game{.screen = ScreenId::Game, .optionsReturnTo = ScreenId::Menu};
    const ScreenState pause{.screen = ScreenId::Pause, .optionsReturnTo = ScreenId::Menu};

    for (const auto& [origin, expected] :
         {std::pair{menu, ScreenId::Menu}, std::pair{game, ScreenId::Game},
          std::pair{pause, ScreenId::Pause}}) {
        const std::optional<ScreenState> opened =
            resolveTransition(origin, ScreenEvent::OpenRpgScreen);
        ASSERT_TRUE(opened.has_value());
        EXPECT_EQ(opened->rpgReturnTo, expected);
        EXPECT_EQ(resolveTransition(*opened, ScreenEvent::CloseRpgScreen)->screen, expected);
    }
}

/**
 * @brief Les écrans de fin (`LOT-119`) : la mort s'ouvre depuis le combat ou la carte, et n'en
 *        sort que pour recommencer ou rendre le menu ; la fin de la démo mène aux crédits ou au
 *        menu. Aucun des deux ne revient à la partie — leur règle de superposition est dans la
 *        table, la même d'où qu'ils s'ouvrent (`EX-IHM-091`).
 * \castest{<b>Les ecrans de mort et de fin de la demo ferment la partie.</b><br/>
 * \tcat Unitaire · Machine à états des écrans<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ouvrir la mort depuis un ecran du RPG (le HUD de combat) et depuis le jeu.<br/>
 * 2. En sortir par OpenGame, OpenMenu, puis tenter la pause, les options, la fermeture d'un
 * ecran du RPG.<br/>3. Ouvrir la fin de la demo depuis un ecran du RPG (le dialogue) et depuis le
 * jeu.<br/>4. En sortir par les credits et le menu, puis tenter OpenGame.<br/>5. Tenter les deux
 * depuis le menu.<br/>
 * \tattendu Mort : Game puis Menu ; pause, options et retour refuses. Fin : Credits puis Menu ;
 * OpenGame refuse. Depuis le menu : refuses.
 * }
 */
TEST(ScreenFlowTest, LesEcransDeFinFermentLaPartie) {
    const ScreenState menu{.screen = ScreenId::Menu, .optionsReturnTo = ScreenId::Menu};
    const ScreenState game{.screen = ScreenId::Game, .optionsReturnTo = ScreenId::Menu};
    const ScreenState combat{.screen = ScreenId::RpgScreen,
                             .optionsReturnTo = ScreenId::Menu,
                             .rpgReturnTo = ScreenId::Game};

    const auto mort = resolveTransition(combat, ScreenEvent::OpenDeath);
    ASSERT_TRUE(mort.has_value());
    EXPECT_EQ(mort->screen, ScreenId::Death);
    EXPECT_EQ(resolveTransition(game, ScreenEvent::OpenDeath)->screen, ScreenId::Death);
    EXPECT_EQ(resolveTransition(*mort, ScreenEvent::OpenGame)->screen, ScreenId::Game);
    EXPECT_EQ(resolveTransition(*mort, ScreenEvent::OpenMenu)->screen, ScreenId::Menu);
    EXPECT_FALSE(resolveTransition(*mort, ScreenEvent::OpenPause).has_value());
    EXPECT_FALSE(resolveTransition(*mort, ScreenEvent::OpenOptions).has_value());
    EXPECT_FALSE(resolveTransition(*mort, ScreenEvent::CloseRpgScreen).has_value());

    const auto fin = resolveTransition(combat, ScreenEvent::OpenDemoEnd);
    ASSERT_TRUE(fin.has_value());
    EXPECT_EQ(fin->screen, ScreenId::DemoEnd);
    EXPECT_EQ(resolveTransition(game, ScreenEvent::OpenDemoEnd)->screen, ScreenId::DemoEnd);
    const auto credits = resolveTransition(*fin, ScreenEvent::OpenCredits);
    ASSERT_TRUE(credits.has_value());
    EXPECT_EQ(credits->screen, ScreenId::Credits);
    EXPECT_EQ(resolveTransition(*credits, ScreenEvent::CloseCredits)->screen, ScreenId::Menu);
    EXPECT_EQ(resolveTransition(*fin, ScreenEvent::OpenMenu)->screen, ScreenId::Menu);
    EXPECT_FALSE(resolveTransition(*fin, ScreenEvent::OpenGame).has_value());

    EXPECT_FALSE(resolveTransition(menu, ScreenEvent::OpenDeath).has_value());
    EXPECT_FALSE(resolveTransition(menu, ScreenEvent::OpenDemoEnd).has_value());
}
