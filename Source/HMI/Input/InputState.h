// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "HMI/Input/GamepadButton.h"

/**
 * @file HMI/Input/InputState.h
 * @brief État des entrées clavier/souris/manette échantillonné une fois par frame.
 */

namespace hmi {

/**
 * @brief Touche du clavier, identifiée par son **code virtuel Win32**.
 *
 * Les valeurs coïncident volontairement avec les codes `VK_*` de Win32, ce qui permet à la
 * couche de capture (`Window`) de convertir directement un `WPARAM` en `Key` par un simple
 * `static_cast`, sans table de correspondance. Seules les touches utiles au menu et à la
 * navigation sont nommées ici ; l'`InputState` stocke néanmoins n'importe quel code, si bien
 * qu'ajouter une touche revient à ajouter un énumérateur (aucune autre modification).
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
 * @brief Bouton de la souris suivi par l'`InputState`.
 *
 * `Count` n'est pas un bouton : c'est la sentinelle donnant le nombre de boutons suivis
 * (dimension des tableaux d'état).
 */
enum class MouseButton : std::uint8_t {
    Left = 0,
    Right = 1,
    Middle = 2,
    Count = 3,
};

/**
 * @brief État des entrées d'une frame, avec détection des fronts (pressée / relâchée).
 *
 * L'objet distingue, pour chaque touche et chaque bouton, l'état **courant** (enfoncé ou non)
 * et l'état de la **frame précédente**, ce qui permet de déduire les fronts :
 * *pressée* (« vient d'être enfoncée », `EX-CTRL-011`) et *relâchée*. Le cycle d'une frame est :
 * 1. `beginFrame()` recopie l'état courant vers l'état précédent ;
 * 2. la couche de capture (`Window`) applique les événements via `onKeyDown`/`onKeyUp`/… ;
 * 3. la logique lit `keyPressed`/`keyReleased`/`keyDown` et les équivalents souris.
 *
 * **Manette (`EX-CTRL-002`)** : une seconde source, indépendante du clavier, alimentée par
 * `onGamepadKeyDown`/`onGamepadKeyUp` — le même `Key` peut être enfoncé par le clavier, la
 * manette, ou les deux à la fois. `keyDown`/`keyPressed`/`keyReleased` **combinent** les deux
 * sources (OU logique) ; la manette ne s'écrit **jamais** dans les tableaux clavier, pour ne
 * jamais effacer une touche clavier réellement maintenue quand la manette relâche le bouton
 * correspondant (voir `hmi::GamepadPoller`). Cette fusion en lecture, plutôt qu'une
 * table d'actions logiques séparée, satisfait déjà `EX-CTRL-010` (action dissociée de la touche
 * physique) : le `Key` **est** l'action logique, quelle que soit sa source.
 *
 * L'`InputState` est **indépendant de toute fenêtre** (aucune dépendance `<Windows.h>`, ni
 * `<Xinput.h>`) : les événements peuvent être injectés directement, ce qui le rend testable en
 * isolation (`EX-NFR-010`) — y compris la fusion manette, sans manette réelle. Il est échantillonné
 * une fois par frame, en amont de la logique.
 */
class InputState {
public:
    /// Construit un état vide (aucune touche ni bouton enfoncé, souris à l'origine).
    InputState() = default;

    /**
     * @brief Ouvre une nouvelle frame : recopie l'état courant vers l'état précédent.
     *
     * À appeler une fois par frame, avant d'appliquer les événements de la frame. Les fronts
     * (`keyPressed`, `keyReleased`, …) se calculent ensuite par comparaison courant/précédent.
     */
    void beginFrame() noexcept;

    /**
     * @brief Relâche **toutes** les entrées maintenues (clavier, manette, souris), sans front.
     *
     * Remet à zéro l'état courant **et** l'état précédent des touches/boutons, si bien qu'aucun
     * front (pressée/relâchée) n'est produit. À appeler quand la fenêtre perd le focus
     * (`WM_KILLFOCUS`) : sans cela, une touche maintenue au moment d'un `Alt+Tab` ne reçoit jamais
     * son `WM_KEYUP` et resterait « collée » (le personnage continuerait d'avancer au retour).
     */
    void releaseAll() noexcept;

    /// Marque @p key comme enfoncée dans l'état courant (source **clavier**).
    void onKeyDown(Key key) noexcept;

    /// Marque @p key comme relâchée dans l'état courant (source **clavier**).
    void onKeyUp(Key key) noexcept;

    /**
     * @brief Marque @p key comme enfoncée dans l'état courant, source **manette**.
     *
     * Écrit dans un tableau **distinct** du clavier (voir en-tête) : n'affecte jamais
     * `onKeyDown`/`onKeyUp`. `keyDown`/`keyPressed`/`keyReleased` combinent les deux sources.
     */
    void onGamepadKeyDown(Key key) noexcept;

    /// Marque @p key comme relâchée dans l'état courant, source **manette** (voir
    /// `onGamepadKeyDown`).
    void onGamepadKeyUp(Key key) noexcept;

    /**
     * @brief Marque @p button comme enfoncé dans l'état courant (piste manette **brute**).
     *
     * Indépendante de `onGamepadKeyDown`/`Up` : celle-ci fusionne la manette dans l'espace des
     * touches clavier (`Key`, utilisée par la navigation de menu, jamais remappable) ; celle-ci
     * expose l'état de chaque bouton physique (`GamepadButton`) tel quel, pour les actions de jeu
     * remappables via `GamepadBindings` (`EX-CTRL-002`).
     */
    void onGamepadButtonDown(GamepadButton button) noexcept;

    /// Marque @p button comme relâché dans l'état courant (voir `onGamepadButtonDown`).
    void onGamepadButtonUp(GamepadButton button) noexcept;

    /**
     * @brief Met à jour la position de la souris.
     * @param x Abscisse en pixels de la zone client.
     * @param y Ordonnée en pixels de la zone client.
     */
    void onMouseMove(int x, int y) noexcept;

    /// Marque @p button comme enfoncé dans l'état courant.
    void onMouseButtonDown(MouseButton button) noexcept;

    /// Marque @p button comme relâché dans l'état courant.
    void onMouseButtonUp(MouseButton button) noexcept;

    /**
     * @brief Accumule un incrément de molette pour la frame courante.
     * @param delta Incrément signé (positif = molette vers l'avant), en unités natives Win32
     *              (`WHEEL_DELTA` = 120 par cran) ; l'appelant convertit selon son besoin.
     */
    void onMouseWheel(int delta) noexcept;

    /**
     * @brief Enregistre un caractère tapé pour la frame courante (`WM_CHAR`).
     * @param character Caractère déjà traduit selon la disposition clavier active.
     */
    void onCharTyped(wchar_t character);

    /**
     * @brief Déclare l'état de connexion de la manette pour cette frame (`EX-CTRL-002`).
     * @param connected Vrai si `XInputGetState` a réussi ce pas-ci (voir `hmi::GamepadPoller`).
     */
    void setGamepadConnected(bool connected) noexcept;

    /// @return true si une manette était connectée à la dernière frame sondée.
    [[nodiscard]] bool gamepadConnected() const noexcept;

    /// @return true si @p key est enfoncée à cette frame (maintenue ou vient d'être pressée).
    [[nodiscard]] bool keyDown(Key key) const noexcept;

    /// @return true si @p key **vient d'être pressée** cette frame (front montant).
    [[nodiscard]] bool keyPressed(Key key) const noexcept;

    /// @return true si @p key **vient d'être relâchée** cette frame (front descendant).
    [[nodiscard]] bool keyReleased(Key key) const noexcept;

    /// @return true si @p button (piste manette brute) est enfoncé à cette frame.
    [[nodiscard]] bool gamepadButtonDown(GamepadButton button) const noexcept;

    /// @return true si @p button (piste manette brute) **vient d'être enfoncé** cette frame.
    [[nodiscard]] bool gamepadButtonPressed(GamepadButton button) const noexcept;

    /// @return true si @p button (piste manette brute) **vient d'être relâché** cette frame.
    [[nodiscard]] bool gamepadButtonReleased(GamepadButton button) const noexcept;

    /// @return Abscisse de la souris, en pixels de la zone client.
    [[nodiscard]] int mouseX() const noexcept;

    /// @return Ordonnée de la souris, en pixels de la zone client.
    [[nodiscard]] int mouseY() const noexcept;

    /// @return true si @p button est enfoncé à cette frame.
    [[nodiscard]] bool mouseButtonDown(MouseButton button) const noexcept;

    /// @return true si @p button **vient d'être cliqué** cette frame (front montant).
    [[nodiscard]] bool mouseButtonPressed(MouseButton button) const noexcept;

    /// @return true si @p button **vient d'être relâché** cette frame (front descendant).
    [[nodiscard]] bool mouseButtonReleased(MouseButton button) const noexcept;

    /**
     * @brief Somme des incréments de molette accumulés depuis le dernier `beginFrame()`.
     * @return Positif si la molette a tourné vers l'avant depuis la frame précédente, négatif
     *         vers l'arrière, zéro si elle n'a pas bougé.
     */
    [[nodiscard]] int wheelDelta() const noexcept;

    /**
     * @brief Caractères tapés depuis le dernier `beginFrame()`, dans leur ordre de saisie.
     * @return Une file vide si rien n'a été tapé depuis la frame précédente.
     */
    [[nodiscard]] const std::vector<wchar_t>& typedCharacters() const noexcept;

private:
    /// Nombre de codes de touches suivis (couvre l'ensemble des codes virtuels Win32).
    static constexpr std::size_t KEY_COUNT = 256;

    /// Nombre de boutons de souris suivis.
    static constexpr std::size_t BUTTON_COUNT = static_cast<std::size_t>(MouseButton::Count);

    std::array<bool, KEY_COUNT> _keysCurrent{};
    std::array<bool, KEY_COUNT> _keysPrevious{};
    std::array<bool, KEY_COUNT> _gamepadCurrent{};  ///< Source manette, distincte du clavier.
    std::array<bool, KEY_COUNT> _gamepadPrevious{};
    /// Piste manette brute (`GamepadButton`), indépendante de la fusion `Key` ci-dessus.
    std::array<bool, GAMEPAD_BUTTON_COUNT> _gamepadButtonsCurrent{};
    std::array<bool, GAMEPAD_BUTTON_COUNT> _gamepadButtonsPrevious{};
    std::array<bool, BUTTON_COUNT> _buttonsCurrent{};
    std::array<bool, BUTTON_COUNT> _buttonsPrevious{};
    int _mouseX = 0;
    int _mouseY = 0;
    int _wheelDelta = 0;                    ///< Remis à zéro à chaque `beginFrame()`.
    std::vector<wchar_t> _typedCharacters;  ///< Vidée à chaque `beginFrame()`.
    bool _gamepadConnected = false;
};

}  // namespace hmi
