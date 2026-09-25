// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>

/**
 * @file HMI/Platform/ExecutableDirectory.h
 * @brief Localisation du dossier contenant l'exécutable en cours.
 */

namespace hmi {

/**
 * @brief Dossier contenant l'exécutable en cours, pour localiser les ressources copiées à côté
 *        (niveaux, catalogues de traduction, logs).
 * @return Le chemin du dossier de l'exécutable.
 */
[[nodiscard]] std::filesystem::path executableDirectory();

/**
 * @brief Le dossier du **contenu** du jeu : cartes, assets, monde, dialogues, rencontres.
 *
 * Celui de l'exécutable, sauf si `setDataDirectory` l'a remplacé — l'option `--data=<racine>`
 * d'un build de développement (`LOT-118`), qui joue une racine d'essai
 * (`Source/Test/Fixtures/GameData`) comme l'éditeur l'ouvre (`LevelEditor --data`). Les **règles**
 * (`Rpg/rules`, les fiches, les classes), les traductions du jeu et les journaux restent à côté de
 * l'exécutable : une racine de contenu n'a pas à les porter.
 */
[[nodiscard]] std::filesystem::path dataDirectory();

/// @brief Impose le dossier du contenu ; vide : celui de l'exécutable. À appeler avant tout modèle.
void setDataDirectory(std::filesystem::path directory);

}  // namespace hmi
