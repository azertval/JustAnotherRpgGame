# HMI · Input

Tests unitaires — **5 cas** (1 bloquant, 3 majeurs, 1 mineur). [Retour à la synthèse](README.md).

## Ce que cette page couvre

| Fichier de test | Cas | Bloquant | Critique | Majeur | Mineur |
|---|---|---|---|---|---|
| [`test_qt_key_map.cpp`](#test-qt-key-mapcpp) | 5 | 1 | - | 3 | 1 |

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
