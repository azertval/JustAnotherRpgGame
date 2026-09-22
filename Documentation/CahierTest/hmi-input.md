# HMI · Input

Tests unitaires — **25 cas** (1 bloquant, 3 critiques, 19 majeurs, 2 mineurs). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_button_repeat.cpp`](#test-button-repeatcpp) | 1 | - | 1 | - | - |
| [`test_gamepad_probe.cpp`](#test-gamepad-probecpp) | 3 | - | 2 | 1 | - |
| [`test_input_state.cpp`](#test-input-statecpp) | 16 | - | - | 15 | 1 |
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

### InputStateTest.FrontMontantClavier

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:15`

Une touche passée d'« absente » à « présente » est « pressée » exactement une frame.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.keyDown(hmi::Key::Enter)` est vrai.
- Vérifie que `input.keyPressed(hmi::Key::Enter)` est vrai.
- Vérifie que `input.keyReleased(hmi::Key::Enter)` est faux.
- Vérifie que `input.keyDown(hmi::Key::Enter)` est vrai.
- Vérifie que `input.keyPressed(hmi::Key::Enter)` est faux.

### InputStateTest.MaintienClavier

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:41`

Une touche restée enfoncée n'est « pressée » qu'à la première frame.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.keyPressed(hmi::Key::Down)` est vrai.
- Vérifie que `input.keyDown(hmi::Key::Down)` est vrai.
- Vérifie que `input.keyPressed(hmi::Key::Down)` est faux.
- Vérifie que `input.keyReleased(hmi::Key::Down)` est faux.

### InputStateTest.FrontDescendantClavier

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:66`

Le relâchement d'une touche est détecté « relâchée » pendant exactement une frame.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.keyDown(hmi::Key::Space)` est faux.
- Vérifie que `input.keyReleased(hmi::Key::Space)` est vrai.
- Vérifie que `input.keyReleased(hmi::Key::Space)` est faux.

### InputStateTest.TouchesIndependantes

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:93`

Les touches sont indépendantes : un front sur l'une n'affecte pas les autres.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.keyPressed(hmi::Key::Up)` est vrai.
- Vérifie que `input.keyDown(hmi::Key::Down)` est faux.
- Vérifie que `input.keyPressed(hmi::Key::Down)` est faux.

### InputStateTest.BoutonSouris

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:114`

Un bouton de souris suit la même logique pressé/cliqué/relâché que les touches.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.mouseButtonDown(hmi::MouseButton::Left)` est vrai.
- Vérifie que `input.mouseButtonPressed(hmi::MouseButton::Left)` est vrai.
- Vérifie que `input.mouseButtonDown(hmi::MouseButton::Left)` est vrai.
- Vérifie que `input.mouseButtonPressed(hmi::MouseButton::Left)` est faux.
- Vérifie que `input.mouseButtonDown(hmi::MouseButton::Left)` est faux.
- Vérifie que `input.mouseButtonReleased(hmi::MouseButton::Left)` est vrai.

### InputStateTest.PositionSouris

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:143`

La position de la souris reflète le dernier déplacement injecté.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.mouseX()` vaut `42`.
- Vérifie que `input.mouseY()` vaut `99`.
- Vérifie que `input.mouseX()` vaut `-3`.
- Vérifie que `input.mouseY()` vaut `7`.

### InputStateTest.MoletteAccumuleEtSeReinitialise

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:165`

Les incréments de molette d'une frame s'additionnent et repartent de zéro ensuite.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.wheelDelta()` vaut `0`.
- Vérifie que `input.wheelDelta()` vaut `80`.
- Vérifie que `input.wheelDelta()` vaut `0`.

### InputStateTest.CaracteresTapesAccumulesEtVides

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:189`

Les caractères tapés s'accumulent dans l'ordre puis sont vidés à la frame suivante.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.typedCharacters().empty()` est vrai.
- Vérifie que `input.typedCharacters().size()` vaut `3u`.
- Vérifie que `input.typedCharacters()[0]` vaut `L'N'`.
- Vérifie que `input.typedCharacters()[1]` vaut `L'1'`.
- Vérifie que `input.typedCharacters()[2]` vaut `L'\xE9'`.
- Vérifie que `input.typedCharacters().empty()` est vrai.

### InputStateTest.ManetteSeuleActiveLaTouche

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:218`

Un bouton manette seul rend `keyDown`/`keyPressed` vrais, comme au clavier.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.keyDown(hmi::Key::Enter)` est vrai.
- Vérifie que `input.keyPressed(hmi::Key::Enter)` est vrai.
- Vérifie que `input.keyDown(hmi::Key::Enter)` est vrai.
- Vérifie que `input.keyPressed(hmi::Key::Enter)` est faux.
- Vérifie que `input.keyDown(hmi::Key::Enter)` est faux.
- Vérifie que `input.keyReleased(hmi::Key::Enter)` est vrai.

### InputStateTest.ClavierEtManetteMemeToucheUnSeulFront

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:246`

Clavier et manette combinés sur la même touche ne produisent pas de double front.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.keyPressed(hmi::Key::Space)` est vrai.
- Vérifie que `input.keyPressed(hmi::Key::Space)` est faux.
- Vérifie que `input.keyDown(hmi::Key::Space)` est vrai.
- Vérifie que `input.keyReleased(hmi::Key::Space)` est faux.
- Vérifie que `input.keyDown(hmi::Key::Space)` est faux.
- Vérifie que `input.keyReleased(hmi::Key::Space)` est vrai.

### InputStateTest.ManetteRelacheeNeMasquePasLeClavier

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:283`

La manette relâchée ne masque jamais une touche clavier réellement maintenue.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.keyDown(hmi::Key::Left)` est vrai.
- Vérifie que `input.keyDown(hmi::Key::Left)` est vrai.
- Vérifie que `input.keyReleased(hmi::Key::Left)` est faux.

### InputStateTest.GamepadConnecteReecrasable

*Mineur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:311`

`gamepadConnected` reflète le dernier `setGamepadConnected` appelé.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.gamepadConnected()` est faux.
- Vérifie que `input.gamepadConnected()` est vrai.
- Vérifie que `input.gamepadConnected()` est faux.

### InputStateTest.FrontMontantBoutonManetteBrut

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:333`

Un bouton manette (piste brute) est « pressé » exactement une frame.

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

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:360`

Un bouton manette (piste brute) est « relâché » exactement une frame.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.gamepadButtonReleased(hmi::GamepadButton::X)` est faux.
- Vérifie que `input.gamepadButtonReleased(hmi::GamepadButton::X)` est vrai.
- Vérifie que `input.gamepadButtonReleased(hmi::GamepadButton::X)` est faux.

### InputStateTest.PisteBrutIndependanteDeLaFusionKey

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:386`

La piste manette brute est indépendante de la fusion clavier/manette sur Key.

**Étapes**

1. Mettre en place le contexte du test (arrangement).
2. Executer le scenario et verifier les assertions.

**Résultat attendu**

- Vérifie que `input.keyDown(hmi::Key::Enter)` est faux.
- Vérifie que `input.gamepadButtonDown(hmi::GamepadButton::B)` est faux.

### InputStateTest.RelacheToutSansFront

*Majeur · Unitaire · Input State* — `Source/Test/Unit/HMI/Input/test_input_state.cpp:409`

releaseAll relâche tout sans produire de front « relâchée ».

**Étapes**

1. Maintenir des entrées clavier/manette/souris.
2. Appeler releaseAll et verifier qu'aucune n'est plus enfoncee ni signalee « relâchée ».

**Résultat attendu**

- Vérifie que `input.keyDown(hmi::Key::Right)` est faux.
- Vérifie que `input.keyReleased(hmi::Key::Right)` est faux.
- Vérifie que `input.keyDown(hmi::Key::Space)` est faux.
- Vérifie que `input.keyReleased(hmi::Key::Space)` est faux.
- Vérifie que `input.gamepadButtonDown(hmi::GamepadButton::A)` est faux.
- Vérifie que `input.mouseButtonDown(hmi::MouseButton::Left)` est faux.
- Vérifie que `input.mouseButtonReleased(hmi::MouseButton::Left)` est faux.
- Vérifie que `input.keyReleased(hmi::Key::Right)` est faux.
- Vérifie que `input.keyPressed(hmi::Key::Right)` est faux.

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
