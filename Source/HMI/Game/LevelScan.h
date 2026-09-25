// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file HMI/Game/LevelScan.h
 * @brief Les cartes qu'un dossier `Levels/` contient, pour le lanceur de cartes (outil de debug).
 *
 * Le jeu ne **liste** jamais ses cartes : il en charge une par son identifiant
 * (`core::WorldTravel::directoryLoader`), et l'identifiant est le chemin du fichier sous
 * `Levels/`, sans `.json` (`LOT-124`). Le lanceur de cartes fait l'inverse — montrer ce qu'on
 * pourrait ouvrir — et c'est ici que le chemin redevient identifiant, par la même règle, pour
 * qu'une carte listée soit exactement une carte que `--map=` ouvre.
 *
 * Sans Qt : un parcours de dossiers, testable sur un dossier d'essai.
 */

#include <filesystem>
#include <string>
#include <vector>

namespace hmi {

/// @brief Une carte trouvée : son identifiant, son fichier, le dossier `Levels/` d'où elle vient.
struct LevelEntry {
    /// L'identifiant que `--map=` attend : `capital/martpart`, avec des `/` quel que soit l'OS.
    std::string mapId;
    std::filesystem::path file;
    std::filesystem::path directory;

    friend bool operator==(const LevelEntry&, const LevelEntry&) = default;
};

/**
 * @brief Toutes les cartes de @p directories, dans l'ordre des identifiants.
 *
 * Les dossiers sont lus **dans l'ordre** : quand deux portent la même carte, la première l'emporte
 * — la règle de `core::WorldTravel::directoriesLoader`, celle que le jeu applique quand
 * `--levels=` pose les brouillons devant les cartes du binaire. Les fichiers annexes de l'éditeur
 * (`<carte>.editor.json`) ne sont pas des cartes et sont écartés. Un dossier absent n'est pas une
 * erreur : il ne contient rien.
 */
[[nodiscard]] std::vector<LevelEntry> scanLevelDirectories(
    const std::vector<std::filesystem::path>& directories);

}  // namespace hmi
