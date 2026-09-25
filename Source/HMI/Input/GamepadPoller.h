// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <chrono>

#include "HMI/Input/InputState.h"

/**
 * @file HMI/Input/GamepadPoller.h
 * @brief Sondage XInput de la manette, versé dans un `InputState` (réutilisable).
 */

namespace hmi {

/// Délai minimal entre deux sondages XInput d'un slot resté **déconnecté** (anti-saccade).
inline constexpr std::chrono::milliseconds GAMEPAD_DISCONNECTED_PROBE_PERIOD{2000};

/**
 * @brief Faut-il re-sonder XInput maintenant ?
 *
 * `XInputGetState` est notablement coûteux sur un slot vide (le pilote énumère les périphériques à
 * chaque appel) : sonder sans relâche une manette absente provoque des micro-saccades chez un
 * joueur clavier. L'espacement est donc décidé **en temps réel**, jamais en nombre
 * d'appels : le sondage est déclenché tantôt par la boucle de rendu (une fois par image), tantôt
 * par un temporisateur d'interface (150 ms pour la navigation de menu, 500 ms pour l'onglet des
 * options). Un compteur d'appels ferait dépendre le délai de détection de l'appelant, dans un
 * rapport de plus de cent entre le plus rapide et le plus lent : c'est la cadence en secondes qui
 * doit être garantie, pas un nombre de tours.
 *
 * Fonction **pure** (`EX-NFR-010`) : décidée ici, testable sans manette ni fenêtre.
 * @param wasConnected    État de connexion du dernier sondage effectif.
 * @param sinceLastProbe  Temps écoulé depuis ce sondage.
 * @return `true` si le sondage doit avoir lieu : toujours quand une manette est présente, sinon
 *         une fois par `hmi::GAMEPAD_DISCONNECTED_PROBE_PERIOD`.
 */
[[nodiscard]] constexpr bool gamepadProbeDue(
    bool wasConnected, std::chrono::steady_clock::duration sinceLastProbe) noexcept {
    return wasConnected || sinceLastProbe >= GAMEPAD_DISCONNECTED_PROBE_PERIOD;
}

/**
 * @brief Sonde la manette (XInput, joueur 0) et verse son état dans un `InputState`.
 *
 * Alimente l'état par `GamepadButton` (`onGamepadButtonDown`/`onGamepadButtonUp`, `EX-CTRL-002`) :
 * le D-pad et le stick gauche fusionnés en quatre directions, les quatre boutons et les deux
 * gâchettes hautes. C'est cet état que `hmi::GamepadNavigator` traduit en noms de boutons pour
 * les écrans.
 *
 * Objet **à état** (dernier état de connexion + horodatage du dernier sondage), à appeler **une
 * fois par frame** avant que la logique ne consomme les entrées. Le sondage est espacé tant que la
 * manette reste déconnectée (`hmi::gamepadProbeDue`).
 *
 * Extrait de l'ancienne fenêtre Win32 pour être utilisé par le viewport Qt, sans
 * dépendre d'aucune fenêtre.
 */
class GamepadPoller {
public:
    /// Sonde la manette et met à jour @p input (état par `GamepadButton` et connexion).
    void poll(InputState& input);

private:
    bool _wasConnected = false;  ///< Pour ne journaliser qu'un changement de connexion.
    /// Instant du dernier sondage XInput effectif ; l'époque tant qu'aucun n'a eu lieu, pour que
    /// le tout premier appel sonde immédiatement.
    std::chrono::steady_clock::time_point _lastProbe{};
};

}  // namespace hmi
