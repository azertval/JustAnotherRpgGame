// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <optional>

#include "HMI/Input/Key.h"

/**
 * @file HMI/Input/QtKeyMap.h
 * @brief Traduction d'un code de touche Qt en `hmi::Key` (code virtuel Win32).
 */

namespace hmi {

/**
 * @brief Traduit un code `Qt::Key` en `hmi::Key`.
 *
 * Les lettres/chiffres/espace partagent déjà la même valeur entre Qt et Win32
 * (`Qt::Key_A == VK 'A' == 0x41`) ; seules les touches spéciales (flèches, Échap, Tab, Maj, Ctrl,
 * F1/F2/F10) passent par une correspondance explicite. `nullopt` pour une touche non suivie.
 */
[[nodiscard]] std::optional<hmi::Key> qtKeyToHmiKey(int qtKey);

/**
 * @brief Traduit un `hmi::Key` en code `Qt::Key`, inverse de `qtKeyToHmiKey`.
 *
 * Sert à faire refléter un remappage (`EditorKeyBindings`) sur le raccourci
 * effectif d'un `QAction` (`hmi::EditorActions::applyShortcuts`). Comme l'aller, les lettres et
 * chiffres partagent déjà la même valeur entre Qt et Win32 — conversion directe pour tout code non
 * explicitement nommé.
 * @param key Touche à traduire.
 * @return Le code `Qt::Key` correspondant.
 */
[[nodiscard]] int hmiKeyToQtKey(hmi::Key key);

}  // namespace hmi
