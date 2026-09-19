// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Editor/Logic/FileOperationResult.h"

/**
 * @file Editor/Logic/LevelFileOperations.h
 * @brief Opérations fichiers sur les niveaux (créer/renommer/dupliquer/supprimer), sans Qt.
 */

namespace hmi {

/**
 * @brief Opérations sur les fichiers de niveaux d'un dossier, **sans dépendance Qt/GPU**.
 *
 * Logique pure et testable (`EX-NFR-010`, `EX-IHM-021`) : réutilise `hmi::isValidLevelName`
 * (validation de nom, `EX-EDIT-009`), `core::LevelLoader`/`LevelWriter` (format) et
 * `core::LevelDraft` (aucune règle de niveau dupliquée, `EX-EDIT-010`). Chaque opération renvoie un
 * `FileOperationResult` — jamais d'exception vers l'appelant (`EX-NFR-040`).
 */
class LevelFileOperations {
public:
    /**
     * @param levelsDir  Le dossier des cartes qu'on gère (`Levels`, ou un sous-dossier).
     * @param levelsRoot Le dossier `Levels` lui-même, d'où se tirent les identifiants de carte
     *                   (`capital/martpart`) ; à défaut, @p levelsDir. Les catalogues de traduction
     *                   sont à côté de lui, dans `Localization`.
     */
    explicit LevelFileOperations(std::filesystem::path levelsDir,
                                 std::filesystem::path levelsRoot = {});

    /// @return Les fichiers `.json` du dossier, triés par nom (vide si le dossier n'existe pas).
    [[nodiscard]] std::vector<std::filesystem::path> list() const;

    /**
     * @brief Crée un niveau minimal valide nommé @p name, d'une case au moins, entrée au coin bas
     *        gauche.
     *
     * Sans lieu, la carte est une grille unique vide. Avec un lieu (`LOT-EDITOR-06`), elle naît
     * comme les cartes livrées : une couche de sol `sol` qui prend ses pièces dans la planche de
     * @p place (propriété `scene`), une couche de décor `relief` au-dessus, et la collision
     * déduite — tout est vide, donc tout arrête la vue, sauf la case d'entrée, qui reçoit un sol.
     *
     * Son nom est la clé `map.<identifiant>.name` (`LOT-EDITOR-07`), que chaque catalogue de
     * traduction reçoit avec @p name pour texte : la carte passe le contrôle telle quelle.
     */
    [[nodiscard]] FileOperationResult create(const std::string& name, int width, int height,
                                             const std::string& place = {}) const;

    /// Renomme le niveau @p source en @p newName : son nom devient la clé du nouvel identifiant,
    /// dont les catalogues reprennent les traductions de l'ancienne (`LOT-EDITOR-07`).
    [[nodiscard]] FileOperationResult rename(const std::filesystem::path& source,
                                             const std::string& newName) const;

    /// Duplique le niveau @p source sous un nom unique (« … (copie) », « … (copie 2) », …) ; le
    /// nom de la copie est sa propre clé, aux traductions de l'original.
    [[nodiscard]] FileOperationResult duplicate(const std::filesystem::path& source) const;

    /// Supprime le fichier de niveau @p source.
    [[nodiscard]] static FileOperationResult remove(const std::filesystem::path& source);

private:
    /// Chemin du fichier `.json` correspondant à un nom de niveau, dans le dossier géré.
    [[nodiscard]] std::filesystem::path pathForName(const std::string& name) const;

    /// @return La clé du nom de la carte écrite en @p file.
    [[nodiscard]] std::string nameKeyFor(const std::filesystem::path& file) const;

    /// @brief Ajoute aux catalogues la clé du nom de @p file (voir `hmi::addTranslation`).
    void addNameTranslation(const std::filesystem::path& file, const std::string& text,
                            const std::string& copyFrom = {}) const;

    std::filesystem::path _dir;
    std::filesystem::path _root;
};

}  // namespace hmi
