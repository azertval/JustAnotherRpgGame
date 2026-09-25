# HMI · Input

Tests unitaires — **13 cas** (1 bloquant, 3 critiques, 7 majeurs, 2 mineurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_button_repeat.cpp`](#test-button-repeatcpp) | 1 | - | 1 | - | - |
| [`test_gamepad_probe.cpp`](#test-gamepad-probecpp) | 3 | - | 2 | 1 | - |
| [`test_input_state.cpp`](#test-input-statecpp) | 4 | - | - | 3 | 1 |
| [`test_qt_key_map.cpp`](#test-qt-key-mapcpp) | 5 | 1 | - | 3 | 1 |

## test_button_repeat.cpp

### ButtonRepeatTest.UnAppuiPuisUneRepetitionReguliere

*Critique · Unitaire · IHM* — `Source/Test/Unit/HMI/Input/test_button_repeat.cpp:18`

La croix de la manette tenue deplace le curseur d'une case a l'appui, puis se repete apres le delai, a intervalle regulier ; relachee, elle s'arrete, et un nouvel appui repart du debut.

**Étapes**

1. Appuyer a t = 0, tenir jusqu'a 600 ms en sondant toutes les 16 ms.
2. Relacher, puis rappuyer.

**Résultat attendu**

- Vérifie que `pas` vaut `(std::vector<long long>{0, 352, 464, 576})`.
- Vérifie que `bouton.update(false, debut + 620ms)` est faux.
- Vérifie que `bouton.update(false, debut + 700ms)` est faux.
- Vérifie que `bouton.update(true, debut + 716ms)` est vrai.
- Vérifie que `bouton.update(true, debut + 732ms)` est faux.

## test_gamepad_probe.cpp

### GamepadProbeTest.ManetteConnecteeSondeeAChaqueAppel

*Critique · Unitaire · Manette* — `Source/Test/Unit/HMI/Input/test_gamepad_probe.cpp:19`

Une manette connectee est sondee a chaque appel.

**Étapes**

1. Demander la decision avec une manette connectee et un delai nul.

**Résultat attendu**

- Vérifie que `hmi::gamepadProbeDue(/*wasConnected=*/true, 0ms)` est vrai.
- Vérifie que `hmi::gamepadProbeDue(/*wasConnected=*/true, 1ms)` est vrai.

### GamepadProbeTest.ManetteAbsenteReSondeeApresLeDelai

*Critique · Unitaire · Manette* — `Source/Test/Unit/HMI/Input/test_gamepad_probe.cpp:34`

Une manette absente n'est re-sondee qu'apres le delai d'anti-saccade.

**Étapes**

1. Demander la decision juste avant le delai, puis juste apres.

**Résultat attendu**

- Vérifie que `hmi::gamepadProbeDue(/*wasConnected=*/false, 0ms)` est faux.
- Vérifie que `hmi::gamepadProbeDue(/*wasConnected=*/false, hmi::GAMEPAD_DISCONNECTED_PROBE_PERIOD - 1ms)` est faux.
- Vérifie que `hmi::gamepadProbeDue(/*wasConnected=*/false, hmi::GAMEPAD_DISCONNECTED_PROBE_PERIOD)` est vrai.

### GamepadProbeTest.DelaiIndependantDeLaCadenceDAppel

*Majeur · Unitaire · Manette* — `Source/Test/Unit/HMI/Input/test_gamepad_probe.cpp:53`

Le delai de detection ne depend pas de la cadence de l'appelant.

**Étapes**

1. Simuler des appels espaces de 500 ms et compter ceux qui precedent le premier sondage du.
2. Recommencer avec des appels espaces de 16 ms (cadence de rendu).

**Résultat attendu**

- Vérifie que `firstDueAfter(500ms)` est inférieur ou égal à `hmi::GAMEPAD_DISCONNECTED_PROBE_PERIOD + 500ms`.
- Vérifie que `firstDueAfter(16ms)` est inférieur ou égal à `hmi::GAMEPAD_DISCONNECTED_PROBE_PERIOD + 16ms`.

## test_input_state.cpp

### InputStateTest.GamepadConnecteReecrasable

*Mineur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:15`

`gamepadConnected` reflète le dernier `setGamepadConnected` appelé.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.gamepadConnected()` est faux.
- Vérifie que `input.gamepadConnected()` est vrai.
- Vérifie que `input.gamepadConnected()` est faux.

### InputStateTest.FrontMontantBoutonManetteBrut

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:36`

Un bouton manette est « pressé » exactement un relevé.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.gamepadButtonDown(hmi::GamepadButton::A)` est vrai.
- Vérifie que `input.gamepadButtonPressed(hmi::GamepadButton::A)` est vrai.
- Vérifie que `input.gamepadButtonDown(hmi::GamepadButton::A)` est vrai.
- Vérifie que `input.gamepadButtonPressed(hmi::GamepadButton::A)` est faux.
- Vérifie que `input.gamepadButtonDown(hmi::GamepadButton::A)` est faux.

### InputStateTest.FrontDescendantBoutonManetteBrut

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:63`

Un bouton manette est « relâché » exactement un relevé.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.gamepadButtonReleased(hmi::GamepadButton::X)` est faux.
- Vérifie que `input.gamepadButtonReleased(hmi::GamepadButton::X)` est vrai.
- Vérifie que `input.gamepadButtonReleased(hmi::GamepadButton::X)` est faux.

### InputStateTest.RelacheToutSansFront

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:89`

releaseAll relâche tout sans produire de front « relâché ».

**Étapes**

1. Maintenir deux boutons.
2. Appeler releaseAll et verifier qu'aucun n'est plus enfonce ni signale « relâché », a ce releve comme au suivant.

**Résultat attendu**

- Vérifie que `input.gamepadButtonDown(hmi::GamepadButton::A)` est faux.
- Vérifie que `input.gamepadButtonReleased(hmi::GamepadButton::A)` est faux.
- Vérifie que `input.gamepadButtonDown(hmi::GamepadButton::Right)` est faux.
- Vérifie que `input.gamepadButtonReleased(hmi::GamepadButton::Right)` est faux.
- Vérifie que `input.gamepadButtonReleased(hmi::GamepadButton::A)` est faux.
- Vérifie que `input.gamepadButtonPressed(hmi::GamepadButton::A)` est faux.

## test_qt_key_map.cpp

### QtKeyMapTest.AllerRetourExactSurToutesLesTouches

*Bloquant · Unitaire · HMI Input* — `Source/Test/Unit/HMI/Input/test_qt_key_map.cpp:45`

Aller-retour hmi::Key -> Qt -> hmi::Key exact sur toutes les touches nommées.

**Étapes**

1. Pour chaque touche de `hmi::Key`, appliquer `hmiKeyToQtKey` puis `qtKeyToHmiKey`.

**Résultat attendu**

- Vérifie que `roundTrip.has_value()` est vrai.
- Vérifie que `*roundTrip` vaut `key`.

### QtKeyMapTest.TouchesSpecialesTraduitesExplicitement

*Majeur · Unitaire · HMI Input* — `Source/Test/Unit/HMI/Input/test_qt_key_map.cpp:65`

Touches spéciales traduites explicitement.

**Étapes**

1. Traduire Échap, Espace, les quatre flèches et F10 dans les deux sens.

**Résultat attendu**

- Vérifie que `hmi::hmiKeyToQtKey(hmi::Key::Escape)` vaut `static_cast<int>(Qt::Key_Escape)`.
- Vérifie que `hmi::hmiKeyToQtKey(hmi::Key::Space)` vaut `static_cast<int>(Qt::Key_Space)`.
- Vérifie que `hmi::hmiKeyToQtKey(hmi::Key::Left)` vaut `static_cast<int>(Qt::Key_Left)`.
- Vérifie que `hmi::hmiKeyToQtKey(hmi::Key::Down)` vaut `static_cast<int>(Qt::Key_Down)`.
- Vérifie que `hmi::hmiKeyToQtKey(hmi::Key::F10)` vaut `static_cast<int>(Qt::Key_F10)`.
- Vérifie que `hmi::qtKeyToHmiKey(Qt::Key_Escape)` vaut `std::optional<hmi::Key>(hmi::Key::Escape)`.
- Vérifie que `hmi::qtKeyToHmiKey(Qt::Key_Up)` vaut `std::optional<hmi::Key>(hmi::Key::Up)`.
- Vérifie que `hmi::qtKeyToHmiKey(Qt::Key_F1)` vaut `std::optional<hmi::Key>(hmi::Key::F1)`.

### QtKeyMapTest.LettresEtChiffresConvertisDirectement

*Majeur · Unitaire · HMI Input* — `Source/Test/Unit/HMI/Input/test_qt_key_map.cpp:86`

Lettres et chiffres : conversion directe Qt &lt;-&gt; Win32.

**Étapes**

1. Traduire `Qt::Key_A`, `Qt::Key_Z`, `Qt::Key_0` et `Qt::Key_9`.

**Résultat attendu**

- Vérifie que `hmi::qtKeyToHmiKey(Qt::Key_A)` vaut `std::optional<hmi::Key>(hmi::Key::A)`.
- Vérifie que `hmi::qtKeyToHmiKey(Qt::Key_Z)` vaut `std::optional<hmi::Key>(hmi::Key::Z)`.
- Vérifie que `hmi::qtKeyToHmiKey(Qt::Key_0)` vaut `std::optional<hmi::Key>(hmi::Key::D0)`.
- Vérifie que `nine.has_value()` est vrai.
- Vérifie que `static_cast<int>(*nine)` vaut `static_cast<int>(Qt::Key_9)`.

### QtKeyMapTest.SynonymesRejoignentLaFormeCanonique

*Mineur · Unitaire · HMI Input* — `Source/Test/Unit/HMI/Input/test_qt_key_map.cpp:110`

Backtab et Enter du pavé rejoignent la forme canonique.

**Étapes**

1. Traduire `Qt::Key_Backtab` et `Qt::Key_Enter`.
2. Retraduire le résultat.

**Résultat attendu**

- Vérifie que `hmi::qtKeyToHmiKey(Qt::Key_Backtab)` vaut `std::optional<hmi::Key>(hmi::Key::Tab)`.
- Vérifie que `hmi::qtKeyToHmiKey(Qt::Key_Enter)` vaut `std::optional<hmi::Key>(hmi::Key::Enter)`.
- Vérifie que `hmi::hmiKeyToQtKey(hmi::Key::Tab)` vaut `static_cast<int>(Qt::Key_Tab)`.
- Vérifie que `hmi::hmiKeyToQtKey(hmi::Key::Enter)` vaut `static_cast<int>(Qt::Key_Return)`.

### QtKeyMapTest.ToucheNonSuivieRendNullopt

*Majeur · Unitaire · HMI Input* — `Source/Test/Unit/HMI/Input/test_qt_key_map.cpp:126`

Touche non suivie -> nullopt.

**Étapes**

1. Traduire `Qt::Key_F5`, `Qt::Key_Alt`, `Qt::Key_Home` et une valeur hors intervalle.

**Résultat attendu**

- Vérifie que `hmi::qtKeyToHmiKey(Qt::Key_F5).has_value()` est faux.
- Vérifie que `hmi::qtKeyToHmiKey(Qt::Key_Alt).has_value()` est faux.
- Vérifie que `hmi::qtKeyToHmiKey(Qt::Key_Home).has_value()` est faux.
- Vérifie que `hmi::qtKeyToHmiKey(-1).has_value()` est faux.
