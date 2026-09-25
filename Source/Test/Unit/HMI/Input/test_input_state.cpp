// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_input_state.cpp
 * @brief Tests unitaires de l'état de la manette : fronts des boutons, connexion, relâchement.
 */

#include <gtest/gtest.h>

#include "HMI/Input/InputState.h"

/**
 * @brief `gamepadConnected` reflète le dernier `setGamepadConnected` appelé.
 * \castest{<b>`gamepadConnected` reflète le dernier `setGamepadConnected` appelé.</b><br/>
 * \tcat Unitaire · Input State<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu `gamepadConnected` reflète le dernier `setGamepadConnected` appelé.
 * }
 */
TEST(InputStateTest, GamepadConnecteReecrasable) {
    hmi::InputState input;
    EXPECT_FALSE(input.gamepadConnected());

    input.setGamepadConnected(true);
    EXPECT_TRUE(input.gamepadConnected());

    input.setGamepadConnected(false);
    EXPECT_FALSE(input.gamepadConnected());
}

/**
 * @brief Un bouton manette passé d'« absent » à « présent » est « pressé » exactement un relevé.
 * \castest{<b>Un bouton manette est « pressé » exactement un relevé.</b><br/>
 * \tcat Unitaire · Input State<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Un bouton manette est « pressé » exactement un relevé.
 * }
 */
TEST(InputStateTest, FrontMontantBoutonManetteBrut) {
    hmi::InputState input;

    input.beginFrame();
    input.onGamepadButtonDown(hmi::GamepadButton::A);
    EXPECT_TRUE(input.gamepadButtonDown(hmi::GamepadButton::A));
    EXPECT_TRUE(input.gamepadButtonPressed(hmi::GamepadButton::A));

    input.beginFrame();
    EXPECT_TRUE(input.gamepadButtonDown(hmi::GamepadButton::A));
    EXPECT_FALSE(input.gamepadButtonPressed(hmi::GamepadButton::A));

    input.onGamepadButtonUp(hmi::GamepadButton::A);
    EXPECT_FALSE(input.gamepadButtonDown(hmi::GamepadButton::A));
}

/**
 * @brief Un bouton manette relâché après avoir été enfoncé est « relâché » exactement un relevé
 *        (action Interagir).
 * \castest{<b>Un bouton manette est « relâché » exactement un relevé.</b><br/>
 * \tcat Unitaire · Input State<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu `gamepadButtonReleased` est vrai la frame où le bouton passe d'enfoncé à relâché,
 * faux avant et après.
 * }
 */
TEST(InputStateTest, FrontDescendantBoutonManetteBrut) {
    hmi::InputState input;

    input.onGamepadButtonDown(hmi::GamepadButton::X);
    input.beginFrame();
    EXPECT_FALSE(input.gamepadButtonReleased(hmi::GamepadButton::X));

    input.onGamepadButtonUp(hmi::GamepadButton::X);
    EXPECT_TRUE(input.gamepadButtonReleased(hmi::GamepadButton::X));

    input.beginFrame();
    EXPECT_FALSE(input.gamepadButtonReleased(hmi::GamepadButton::X));
}

/**
 * @brief `releaseAll()` relâche tous les boutons maintenus sans produire de front « relâché » :
 *        un écran qui cesse d'écouter la manette ne déclenche pas d'action fantôme.
 * \castest{<b>releaseAll relâche tout sans produire de front « relâché ».</b><br/>
 * \tcat Unitaire · Input State<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Maintenir deux boutons.<br/>2. Appeler releaseAll et verifier qu'aucun n'est plus
 * enfonce ni signale « relâché », a ce releve comme au suivant.<br/>
 * \tattendu Tous les boutons sont relaches, sans aucun front.
 * }
 */
TEST(InputStateTest, RelacheToutSansFront) {
    hmi::InputState input;

    input.beginFrame();
    input.onGamepadButtonDown(hmi::GamepadButton::A);
    input.onGamepadButtonDown(hmi::GamepadButton::Right);

    // Le lecteur cesse d'ecouter : tout est relâché immédiatement, sans front « relâché »
    // (courant ET précédent remis à zéro), pour ne pas déclencher une action fantôme.
    input.releaseAll();
    EXPECT_FALSE(input.gamepadButtonDown(hmi::GamepadButton::A));
    EXPECT_FALSE(input.gamepadButtonReleased(hmi::GamepadButton::A));
    EXPECT_FALSE(input.gamepadButtonDown(hmi::GamepadButton::Right));
    EXPECT_FALSE(input.gamepadButtonReleased(hmi::GamepadButton::Right));

    // Le relevé suivant ne fait pas non plus réapparaître de front.
    input.beginFrame();
    EXPECT_FALSE(input.gamepadButtonReleased(hmi::GamepadButton::A));
    EXPECT_FALSE(input.gamepadButtonPressed(hmi::GamepadButton::A));
}
