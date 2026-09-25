// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "HMI/Input/GamepadButton.h"

/**
 * @file HMI/Input/InputState.h
 * @brief État de la manette échantillonné une fois par relevé, et la touche `hmi::Key` de
 *        l'éditeur.
 */

namespace hmi {

/**
 * @brief Touche du clavier, identifiée par son **code virtuel Win32**.
 *
 * Les valeurs coïncident volontairement avec les codes `VK_*` de Win32 : c'est le code brut que
 * le fichier des raccourcis de l'éditeur enregistre (`hmi::EditorKeyBindings`), et que
 * `hmi::qtKeyToHmiKey` / `hmi::hmiKeyToQtKey` traduisent depuis et vers `Qt::Key`. Seules les
 * touches utiles sont nommées ; ajouter une touche revient à ajouter un énumérateur. Le jeu Qt
 * Quick, lui, lit ses touches par les événements de ses éléments (`Keys.onPressed`) et ne passe
 * pas par cette énumération.
 */
enum class Key : std::uint16_t {
    Backspace = 0x08,
    Tab = 0x09,
    Enter = 0x0D,
    Shift = 0x10,    // Maj : modificateur (Maj+clic de l'éditeur)
    Control = 0x11,  // Ctrl : raccourcis d'édition (annuler/refaire, éditeur)
    Escape = 0x1B,
    Space = 0x20,
    Left = 0x25,
    Up = 0x26,
    Right = 0x27,
    Down = 0x28,
    D0 = 0x30,  // « 0 » : réinitialiser la caméra de l'éditeur
    A = 0x41,   // touches lettres (codes VK_*) pour les schémas ZQSD / WASD
    C = 0x43,   // Ctrl+C : copier une zone (éditeur)
    D = 0x44,
    E = 0x45,  // Interagir, touche clavier par défaut (jeu)
    P = 0x50,  // Essai immédiat de la carte en cours d'édition (éditeur)
    Q = 0x51,
    R = 0x52,  // Ctrl+R : redimensionner par saisie directe (éditeur)
    S = 0x53,  // Ctrl+S : enregistrer (éditeur)
    T = 0x54,
    V = 0x56,  // Ctrl+V : coller une zone (éditeur)
    W = 0x57,
    Y = 0x59,    // Ctrl+Y : refaire (éditeur)
    Z = 0x5A,    // Ctrl+Z : annuler (éditeur)
    F1 = 0x70,   // Aide des raccourcis (éditeur)
    F2 = 0x71,   // Renommer le niveau en cours d'édition (éditeur)
    F10 = 0x79,  // Bascule la grille de repère (éditeur)
};

/**
 * @brief État des boutons de la manette à un relevé, avec détection des fronts (pressé / relâché).
 *
 * L'objet distingue, pour chaque bouton, l'état **courant** (enfoncé ou non) et l'état du
 * **relevé précédent**, ce qui permet de déduire les fronts : *pressé* (« vient d'être enfoncé »,
 * `EX-CTRL-011`) et *relâché*. Le cycle d'un relevé est :
 * 1. `beginFrame()` recopie l'état courant vers l'état précédent ;
 * 2. `hmi::GamepadPoller::poll` applique le sondage XInput via `onGamepadButtonDown`/`Up` ;
 * 3. la logique (`hmi::GamepadNavigator`) lit `gamepadButtonDown`/`gamepadButtonPressed`/
 *    `gamepadButtonReleased`.
 *
 * Le clavier et la souris du jeu Qt Quick arrivent par les événements de ses éléments et ne
 * passent plus par ici : l'état est celui de la **manette** (`EX-CTRL-002`), seule source qu'il
 * faille sonder.
 *
 * L'`InputState` est **indépendant de toute fenêtre** (aucune dépendance `<Windows.h>`, ni
 * `<Xinput.h>`) : les boutons peuvent être injectés directement, ce qui le rend testable en
 * isolation (`EX-NFR-010`), sans manette réelle.
 */
class InputState {
public:
    /// Construit un état vide (aucun bouton enfoncé, manette déconnectée).
    InputState() = default;

    /**
     * @brief Ouvre un nouveau relevé : recopie l'état courant vers l'état précédent.
     *
     * À appeler une fois par relevé, avant d'appliquer le sondage. Les fronts
     * (`gamepadButtonPressed`, `gamepadButtonReleased`) se calculent ensuite par comparaison
     * courant/précédent.
     */
    void beginFrame() noexcept;

    /**
     * @brief Relâche **tous** les boutons maintenus, sans front.
     *
     * Remet à zéro l'état courant **et** l'état précédent, si bien qu'aucun front (pressé/relâché)
     * n'est produit. À appeler quand un lecteur cesse d'écouter (`hmi::GamepadNavigator`
     * désactivé) : sans cela, un bouton tenu à cet instant resterait « collé ».
     */
    void releaseAll() noexcept;

    /// Marque @p button comme enfoncé dans l'état courant.
    void onGamepadButtonDown(GamepadButton button) noexcept;

    /// Marque @p button comme relâché dans l'état courant.
    void onGamepadButtonUp(GamepadButton button) noexcept;

    /**
     * @brief Déclare l'état de connexion de la manette pour ce relevé (`EX-CTRL-002`).
     * @param connected Vrai si `XInputGetState` a réussi ce pas-ci (voir `hmi::GamepadPoller`).
     */
    void setGamepadConnected(bool connected) noexcept;

    /// @return true si une manette était connectée au dernier relevé sondé.
    [[nodiscard]] bool gamepadConnected() const noexcept;

    /// @return true si @p button est enfoncé à ce relevé (maintenu ou vient d'être pressé).
    [[nodiscard]] bool gamepadButtonDown(GamepadButton button) const noexcept;

    /// @return true si @p button **vient d'être enfoncé** à ce relevé (front montant).
    [[nodiscard]] bool gamepadButtonPressed(GamepadButton button) const noexcept;

    /// @return true si @p button **vient d'être relâché** à ce relevé (front descendant).
    [[nodiscard]] bool gamepadButtonReleased(GamepadButton button) const noexcept;

private:
    std::array<bool, GAMEPAD_BUTTON_COUNT> _gamepadButtonsCurrent{};
    std::array<bool, GAMEPAD_BUTTON_COUNT> _gamepadButtonsPrevious{};
    bool _gamepadConnected = false;
};

}  // namespace hmi
