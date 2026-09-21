// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <string>
#include <vector>

/**
 * @file Editor/Logic/GameLaunch.h
 * @brief L'essai **complet** (`LOT-EDITOR-10`, `EX-EDIT-095`) : ce qu'il faut poser sur le disque,
 * et où trouver le jeu, pour lancer `JustAnotherRpgGame` sur la carte ouverte.
 *
 * L'essai immédiat du canevas (`hmi::EditorViewport::startPlaytest`) reste ce qu'il est : la
 * marche et les portails, sans quitter la fenêtre. Ici, c'est le **vrai jeu** qui s'ouvre, avec
 * ses dialogues, ses écrans et son rendu — et il lit des fichiers, pas la mémoire de l'éditeur.
 * D'où ce dossier temporaire : les brouillons ouverts y sont écrits, et le jeu les sert avant les
 * cartes de son propre dossier (`core::WorldTravel::directoriesLoader`).
 *
 * Hors du dépôt, et hors de la copie de construction : un essai n'enregistre rien. Ce qu'on joue
 * n'est pas ce qu'on livre tant qu'on n'a pas enregistré.
 */

namespace hmi {

/// @brief Une carte ouverte, telle que l'essai la jouera : son identifiant et son brouillon.
struct DraftMap {
    /// L'identifiant de la carte (`capital/martpart`), qui donne son chemin sous le dossier.
    std::string mapId;
    /// Le JSON du brouillon (`hmi::EditorViewport::draftJson`).
    std::string json;
};

/// @return Le dossier temporaire de l'essai complet : `<temp>/JustAnotherRpgGame-playtest`.
[[nodiscard]] std::filesystem::path playtestDirectory();

/**
 * @brief Écrit @p drafts sous @p directory, un fichier par carte (`<mapId>.json`).
 *
 * Le dossier est **vidé** d'abord : une carte fermée depuis le dernier essai n'a pas à rester
 * devant celle du jeu, où elle serait jouée sans que rien ne le dise.
 *
 * @return Ce qui a empêché l'écriture, vide si tout est écrit (`EX-NFR-040` : la fenêtre le dit,
 *         elle ne plante pas).
 */
[[nodiscard]] std::string writeDraftMaps(const std::filesystem::path& directory,
                                         const std::vector<DraftMap>& drafts);

/**
 * @brief Le jeu, cherché **à côté de l'éditeur** : les deux binaires sortent du même `bin/`.
 * @return Son chemin, vide s'il n'y est pas — un éditeur peut vivre sans jeu construit.
 */
[[nodiscard]] std::filesystem::path gameExecutable(const std::filesystem::path& editorDirectory);

}  // namespace hmi
