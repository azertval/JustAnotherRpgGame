// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Input/InputState.h"

namespace hmi {

namespace {
// Indice de tableau associé à un bouton manette.
[[nodiscard]] std::size_t gamepadButtonIndex(GamepadButton button) noexcept {
    return static_cast<std::size_t>(button);
}
}  // namespace

// Ouvre un nouveau relevé : recopie l'état courant vers l'état précédent.
//
// Seul l'état *précédent* est réécrit : l'état courant (boutons encore enfoncés) persiste d'un
// relevé à l'autre jusqu'au prochain relâchement que le sondage écrit.
void InputState::beginFrame() noexcept {
    _gamepadButtonsPrevious = _gamepadButtonsCurrent;
}

// Relâche tous les boutons maintenus (courant ET précédent), sans produire de front.
//
// Remettre à zéro les deux états (et pas seulement le courant) évite qu'un front « relâché »
// n'apparaisse pour chaque bouton tenu au moment où le lecteur cesse d'écouter : après cet appel,
// courant == précédent == relâché pour tout, donc aucun front.
void InputState::releaseAll() noexcept {
    _gamepadButtonsCurrent.fill(false);
    _gamepadButtonsPrevious.fill(false);
}

// Marque button comme enfoncé dans l'état courant.
void InputState::onGamepadButtonDown(GamepadButton button) noexcept {
    _gamepadButtonsCurrent[gamepadButtonIndex(button)] = true;
}

void InputState::onGamepadButtonUp(GamepadButton button) noexcept {
    _gamepadButtonsCurrent[gamepadButtonIndex(button)] = false;
}

// Declare l'etat de connexion de la manette pour ce releve.
void InputState::setGamepadConnected(bool connected) noexcept {
    _gamepadConnected = connected;
}

// true si une manette etait connectee au dernier releve sonde.
bool InputState::gamepadConnected() const noexcept {
    return _gamepadConnected;
}

// true si button est enfoncé à ce relevé.
bool InputState::gamepadButtonDown(GamepadButton button) const noexcept {
    return _gamepadButtonsCurrent[gamepadButtonIndex(button)];
}

// Indique si button vient d'être enfoncé à ce relevé (front montant).
bool InputState::gamepadButtonPressed(GamepadButton button) const noexcept {
    const std::size_t index = gamepadButtonIndex(button);
    return _gamepadButtonsCurrent[index] && !_gamepadButtonsPrevious[index];
}

// Indique si button vient d'être relâché à ce relevé (front descendant).
bool InputState::gamepadButtonReleased(GamepadButton button) const noexcept {
    const std::size_t index = gamepadButtonIndex(button);
    return !_gamepadButtonsCurrent[index] && _gamepadButtonsPrevious[index];
}

}  // namespace hmi
