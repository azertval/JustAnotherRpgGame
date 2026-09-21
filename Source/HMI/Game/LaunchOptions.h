// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file HMI/Game/LaunchOptions.h
 * @brief Ce que l'éditeur demande au jeu quand il le lance sur la carte ouverte
 *        (`LOT-EDITOR-10`).
 *
 * Les deux moitiés de l'essai complet vivent ici : l'éditeur **écrit** la ligne de commande
 * (`gameLaunchArguments`), le jeu la **relit** (`parseStartCell`, `parseWorldFlags`,
 * `parseLevelDirectories`). Les écrire chacune de son côté ferait diverger la virgule de `--at=`
 * du point-virgule de `--levels=` au premier ajout, et le défaut ne se verrait qu'à l'essai
 * suivant — c'est-à-dire au moment où l'on regarde autre chose.
 *
 * Sans Qt : les deux applications n'ont en commun que `HmiLib`, et l'analyse d'une chaîne n'a
 * besoin de rien de plus.
 */

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Levels/GridPosition.h"

namespace hmi {

/// @brief L'essai complet demandé : la carte, l'endroit, l'état de la partie, et d'où les cartes
///        se lisent.
struct GameLaunchOptions {
    /// Identifiant de la carte (`capital/martpart`), tel que le jeu la charge.
    std::string mapId;
    /// Point d'arrivée nommé, vide pour l'entrée de la carte.
    std::string arrival;
    /// La case où poser le héros, à la place de l'entrée ou du point d'arrivée.
    std::optional<core::GridPosition> cell;
    /// Les drapeaux de monde posés avant le premier pas : la carte après une quête.
    std::vector<std::string> flags;
    /// Les dossiers où chercher les cartes, **dans l'ordre** ; le premier est celui des
    /// brouillons.
    std::vector<std::filesystem::path> levelDirectories;
};

/// Séparateur des drapeaux de `--flags=`, et des coordonnées de `--at=`.
inline constexpr char LAUNCH_LIST_SEPARATOR = ',';
/// Séparateur des dossiers de `--levels=` : une virgule couperait un chemin qui en porte une, et
/// le deux-points est pris par la lettre de lecteur sous Windows.
inline constexpr char LAUNCH_PATH_SEPARATOR = ';';

/**
 * @brief La ligne de commande du jeu pour @p options, sans le programme.
 *
 * Seul ce qui est renseigné paraît : une carte sans case ne produit pas de `--at=` vide, que le
 * jeu aurait à distinguer d'une absence.
 */
[[nodiscard]] std::vector<std::string> gameLaunchArguments(const GameLaunchOptions& options);

/// @return La case de `--at=<colonne>,<ligne>`, ou `std::nullopt` si la valeur n'est pas deux
///         entiers positifs séparés par une virgule (`EX-NFR-040` : le jeu s'ouvre quand même).
[[nodiscard]] std::optional<core::GridPosition> parseStartCell(std::string_view value);

/// @return Les drapeaux de `--flags=a,b,c`, dans l'ordre, sans les vides ni les doublons.
[[nodiscard]] std::vector<std::string> parseWorldFlags(std::string_view value);

/// @return Les dossiers de `--levels=<dossier>;<dossier>`, dans l'ordre, sans les vides.
[[nodiscard]] std::vector<std::filesystem::path> parseLevelDirectories(std::string_view value);

}  // namespace hmi
