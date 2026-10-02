// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <string>
#include <string_view>

/**
 * @file Editor/Logic/Sha256.h
 * @brief L'empreinte SHA-256 d'un fichier, en hexadécimal (`LOT-1008`).
 *
 * Les manifestes des personnages inscrivent l'empreinte du modèle installé et celle de sa source
 * (`models`, `sources`) : c'est ce qui dit, sans rouvrir l'atelier, de quoi un asset est fait.
 * L'atelier des assets les calcule ici, sans Qt — la logique de l'éditeur se construit sans lui.
 */

namespace hmi {

/// @return L'empreinte SHA-256 de @p bytes, en 64 chiffres hexadécimaux minuscules.
[[nodiscard]] std::string sha256Hex(std::string_view bytes);

}  // namespace hmi
