// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file HMI/Input/ButtonRepeat.h
 * @brief La répétition d'un bouton tenu : un pas à l'appui, puis un pas régulier après un délai
 *        (`LOT-24`).
 *
 * Un curseur de ciblage qu'on déplace à la croix de la manette doit avancer d'une case par appui,
 * et traverser une salle si l'on tient la croix — comme une touche fléchée tenue au clavier, que
 * le système répète déjà. XInput ne répète rien : c'est ce compteur, sans horloge à lui, qui le
 * fait. Le temps lui est donné, ce qui le rend testable sans attendre.
 */

#include <chrono>

namespace hmi {

/// Délai avant la première répétition d'un bouton tenu.
inline constexpr std::chrono::milliseconds BUTTON_REPEAT_DELAY{350};
/// Intervalle entre deux répétitions.
inline constexpr std::chrono::milliseconds BUTTON_REPEAT_INTERVAL{110};

/**
 * @brief Compteur de répétition d'un bouton tenu : un pas à l'appui, puis un pas tous les
 *        `BUTTON_REPEAT_INTERVAL` une fois `BUTTON_REPEAT_DELAY` écoulé.
 *
 * Sans horloge à lui : l'instant est donné à `update`, ce qui le rend testable sans attendre.
 */
class ButtonRepeat {
public:
    /**
     * @brief L'état du bouton à l'instant @p now.
     * @return Vrai si le bouton produit un pas maintenant : à l'appui, puis à chaque intervalle une
     *         fois le délai passé.
     */
    [[nodiscard]] bool update(bool down, std::chrono::steady_clock::time_point now) noexcept {
        if (!down) {
            _held = false;
            return false;
        }
        if (!_held) {
            _held = true;
            _next = now + BUTTON_REPEAT_DELAY;
            return true;
        }
        if (now >= _next) {
            _next = now + BUTTON_REPEAT_INTERVAL;
            return true;
        }
        return false;
    }

private:
    bool _held = false;
    std::chrono::steady_clock::time_point _next{};
};

}  // namespace hmi
