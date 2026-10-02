// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_qt_key_map.cpp
 * @brief Tests unitaires de `hmi::qtKeyToHmiKey` / `hmi::hmiKeyToQtKey`.
 *
 * Table de correspondance pure : aucune `QApplication`, aucune fenêtre. `Qt::Key_*` n'est qu'un
 * jeu de constantes d'énumération.
 */

#include <optional>
#include <vector>

#include <gtest/gtest.h>
#include <qnamespace.h>

#include "HMI/Input/Key.h"
#include "HMI/Input/QtKeyMap.h"

namespace {

/// Toutes les touches nommées de `hmi::Key`, y compris celles qui passent par la conversion
/// directe (lettres et chiffres) : c'est l'ensemble sur lequel l'aller-retour doit être exact.
const std::vector<hmi::Key>& allKeys() {
    static const std::vector<hmi::Key> keys{
        hmi::Key::Backspace, hmi::Key::Tab,   hmi::Key::Enter, hmi::Key::Shift, hmi::Key::Control,
        hmi::Key::Escape,    hmi::Key::Space, hmi::Key::Left,  hmi::Key::Up,    hmi::Key::Right,
        hmi::Key::Down,      hmi::Key::D0,    hmi::Key::A,     hmi::Key::C,     hmi::Key::D,
        hmi::Key::E,         hmi::Key::P,     hmi::Key::Q,     hmi::Key::R,     hmi::Key::S,
        hmi::Key::T,         hmi::Key::V,     hmi::Key::W,     hmi::Key::Y,     hmi::Key::Z,
        hmi::Key::F1,        hmi::Key::F2,    hmi::Key::F10};
    return keys;
}

}  // namespace

/**
 * @brief L'aller-retour `hmi::Key` -> `Qt::Key` -> `hmi::Key` rend exactement la touche de départ,
 * pour **toutes** les touches nommées.
 *
 * C'est l'invariant dont dépend le remappage : une touche choisie par l'utilisateur traverse un
 * raccourci de `QAction` puis revient. Une seule touche qui ne reviendrait pas identique rendrait
 * son remappage silencieusement inopérant.
 * \castest{<b>Aller-retour hmi::Key -> Qt -> hmi::Key exact sur toutes les touches
 * nommées.</b><br/>
 * \tcat Unitaire · HMI Input<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Pour chaque touche de `hmi::Key`, appliquer `hmiKeyToQtKey` puis
 * `qtKeyToHmiKey`.<br/>
 * \tattendu La touche obtenue est identique à la touche de départ, pour les 28 touches.}
 */
TEST(QtKeyMapTest, AllerRetourExactSurToutesLesTouches) {
    for (const hmi::Key key : allKeys()) {
        const std::optional<hmi::Key> roundTrip = hmi::qtKeyToHmiKey(hmi::hmiKeyToQtKey(key));
        ASSERT_TRUE(roundTrip.has_value())
            << "touche 0x" << std::hex << static_cast<int>(key) << " non retraduite";
        EXPECT_EQ(*roundTrip, key) << "touche 0x" << std::hex << static_cast<int>(key);
    }
}

/**
 * @brief Les touches spéciales sont traduites vers le bon code Qt, et non par la conversion
 * directe qui vaut pour les lettres et les chiffres.
 * \castest{<b>Touches spéciales traduites explicitement.</b><br/>
 * \tcat Unitaire · HMI Input<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Traduire Échap, Espace, les quatre flèches et F10 dans les deux sens.<br/>
 * \tattendu Chaque touche correspond au code `Qt::Key_*` attendu, dans les deux sens.}
 */
TEST(QtKeyMapTest, TouchesSpecialesTraduitesExplicitement) {
    EXPECT_EQ(hmi::hmiKeyToQtKey(hmi::Key::Escape), static_cast<int>(Qt::Key_Escape));
    EXPECT_EQ(hmi::hmiKeyToQtKey(hmi::Key::Space), static_cast<int>(Qt::Key_Space));
    EXPECT_EQ(hmi::hmiKeyToQtKey(hmi::Key::Left), static_cast<int>(Qt::Key_Left));
    EXPECT_EQ(hmi::hmiKeyToQtKey(hmi::Key::Down), static_cast<int>(Qt::Key_Down));
    EXPECT_EQ(hmi::hmiKeyToQtKey(hmi::Key::F10), static_cast<int>(Qt::Key_F10));

    EXPECT_EQ(hmi::qtKeyToHmiKey(Qt::Key_Escape), std::optional<hmi::Key>(hmi::Key::Escape));
    EXPECT_EQ(hmi::qtKeyToHmiKey(Qt::Key_Up), std::optional<hmi::Key>(hmi::Key::Up));
    EXPECT_EQ(hmi::qtKeyToHmiKey(Qt::Key_F1), std::optional<hmi::Key>(hmi::Key::F1));
}

/**
 * @brief Lettres et chiffres partagent la même valeur entre Qt et Win32 : la conversion est
 * directe, sans table.
 * \castest{<b>Lettres et chiffres : conversion directe Qt &lt;-&gt; Win32.</b><br/>
 * \tcat Unitaire · HMI Input<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Traduire `Qt::Key_A`, `Qt::Key_Z`, `Qt::Key_0` et `Qt::Key_9`.<br/>
 * \tattendu Les valeurs numériques coïncident (`Qt::Key_A == 0x41 == hmi::Key::A`).}
 */
TEST(QtKeyMapTest, LettresEtChiffresConvertisDirectement) {
    EXPECT_EQ(hmi::qtKeyToHmiKey(Qt::Key_A), std::optional<hmi::Key>(hmi::Key::A));
    EXPECT_EQ(hmi::qtKeyToHmiKey(Qt::Key_Z), std::optional<hmi::Key>(hmi::Key::Z));
    EXPECT_EQ(hmi::qtKeyToHmiKey(Qt::Key_0), std::optional<hmi::Key>(hmi::Key::D0));
    // Bornes de l'intervalle chiffres : Key_9 est suivi, meme sans enumerateur hmi::Key dedie.
    // Resultat nomme plutot que deux appels imbriques : le cahier de test reprend le texte des
    // assertions, et Doxygen 1.9.8 echoue a resoudre un appel de fonction cite dans un code span.
    const std::optional<hmi::Key> nine = hmi::qtKeyToHmiKey(Qt::Key_9);
    EXPECT_TRUE(nine.has_value());
    EXPECT_EQ(static_cast<int>(*nine), static_cast<int>(Qt::Key_9));
}

/**
 * @brief `Qt::Key_Backtab` et `Qt::Key_Enter` sont des **synonymes d'entrée** : ils rejoignent
 * `Tab` et `Enter`, dont le retour est la forme canonique (`Key_Tab`, `Key_Return`).
 *
 * L'aller-retour depuis Qt n'est donc volontairement pas l'identité pour ces deux codes — le
 * remappage, lui, part toujours d'un `hmi::Key`, où l'aller-retour reste exact.
 * \castest{<b>Backtab et Enter du pavé rejoignent la forme canonique.</b><br/>
 * \tcat Unitaire · HMI Input<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Traduire `Qt::Key_Backtab` et `Qt::Key_Enter`.<br/>2. Retraduire le résultat.<br/>
 * \tattendu Les deux rejoignent `Tab`/`Enter`, dont le retour vaut `Key_Tab`/`Key_Return`.}
 */
TEST(QtKeyMapTest, SynonymesRejoignentLaFormeCanonique) {
    EXPECT_EQ(hmi::qtKeyToHmiKey(Qt::Key_Backtab), std::optional<hmi::Key>(hmi::Key::Tab));
    EXPECT_EQ(hmi::qtKeyToHmiKey(Qt::Key_Enter), std::optional<hmi::Key>(hmi::Key::Enter));

    EXPECT_EQ(hmi::hmiKeyToQtKey(hmi::Key::Tab), static_cast<int>(Qt::Key_Tab));
    EXPECT_EQ(hmi::hmiKeyToQtKey(hmi::Key::Enter), static_cast<int>(Qt::Key_Return));
}

/**
 * @brief Une touche non suivie rend `nullopt` plutôt qu'une valeur inventée.
 * \castest{<b>Touche non suivie -> nullopt.</b><br/>
 * \tcat Unitaire · HMI Input<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Traduire `Qt::Key_F5`, `Qt::Key_Alt`, `Qt::Key_Home` et une valeur hors
 * intervalle.<br/>
 * \tattendu Aucune n'a de valeur : la table refuse plutôt que de deviner.}
 */
TEST(QtKeyMapTest, ToucheNonSuivieRendNullopt) {
    EXPECT_FALSE(hmi::qtKeyToHmiKey(Qt::Key_F5).has_value());
    EXPECT_FALSE(hmi::qtKeyToHmiKey(Qt::Key_Alt).has_value());
    EXPECT_FALSE(hmi::qtKeyToHmiKey(Qt::Key_Home).has_value());
    EXPECT_FALSE(hmi::qtKeyToHmiKey(-1).has_value());
}
