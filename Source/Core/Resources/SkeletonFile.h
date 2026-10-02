// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file Core/Resources/SkeletonFile.h
 * @brief Ce qu'un personnage en modèle **déclare en données** (`LOT-1005`) : la description de son
 *        squelette (`skeleton.json`) et sa fiche (`character.json`).
 *
 * ## Le squelette d'une silhouette
 *
 * Un squelette est commun à tous les personnages d'une silhouette (`humanoid`) : ses os, et les
 * **clips** que tous jouent. `Common/Characters/Skeletons/<silhouette>/skeleton.json` les
 * déclare :
 *
 *     { "version": 1, "silhouette": "humanoid",
 *       "bones": [ { "name": "Root", "parent": "" }, { "name": "pelvis", "parent": "Root" } ],
 *       "clips": [ { "name": "walk", "duration": 0.5, "loop": true },
 *                  { "name": "attack", "duration": 0.875, "loop": false, "key": 0.4 } ] }
 *
 * Les os sont écrits parents avant enfants. Un clip dit sa **durée**, s'il **boucle**, et son
 * **image clé** (`key`, en secondes) : l'instant où le coup porte, où le sort part — c'est à lui
 * que le combat accroche le touché de la cible (`hmi::CombatCueTrack`). Les courbes elles-mêmes
 * sont dans le `.glb` de chaque personnage, posées sur ses propres articulations.
 *
 * ## La fiche d'un personnage
 *
 * Le dossier d'un personnage en modèle porte `character.json` : son modèle (un `.glb` du même
 * dossier) et le squelette auquel il est lié.
 *
 *     { "version": 1, "model": "brawler.glb", "skeleton": "humanoid" }
 *
 * Un dossier sans fiche est une figurine en bandes, jusqu'au `LOT-1006`.
 *
 * Logique pure, sans Qt ni GPU ; aucune lecture ne lève (`EX-NFR-040`).
 */

namespace core {

/// Version des deux formats écrite dans les fichiers, et plus élevée qui soit lue.
inline constexpr int SKELETON_FORMAT_VERSION = 1;

/// @brief Le fichier de la fiche d'un personnage, dans son dossier.
inline constexpr std::string_view CHARACTER_SHEET_FILE = "character.json";

/// @brief Un os déclaré : son nom, celui de son parent (vide pour la racine).
struct SkeletonBone {
    std::string name;
    std::string parent;

    [[nodiscard]] bool operator==(const SkeletonBone&) const = default;
};

/// @brief Un clip déclaré : sa durée, sa boucle, son image clé.
struct SkeletonClip {
    std::string name;
    /// Durée du clip, en secondes (> 0).
    float duration = 0.0F;
    /// Vrai s'il boucle (repos, marche) ; faux s'il se joue une fois et se fige sur sa fin.
    bool loop = false;
    /// L'instant de l'impact, en secondes depuis le début ; absent pour un clip qui n'en a pas.
    std::optional<float> key;

    [[nodiscard]] bool operator==(const SkeletonClip&) const = default;
};

/// @brief Le squelette d'une silhouette, tel que son fichier le déclare.
struct SkeletonDescription {
    std::string silhouette;
    std::vector<SkeletonBone> bones;
    std::vector<SkeletonClip> clips;

    /// @return Le clip @p name, `nullptr` s'il n'est pas déclaré.
    [[nodiscard]] const SkeletonClip* clip(std::string_view name) const noexcept;

    [[nodiscard]] bool operator==(const SkeletonDescription&) const = default;
};

/// @brief Résultat d'une lecture de squelette : la description, ou ce qui a échoué.
struct SkeletonFileResult {
    SkeletonDescription skeleton;
    /// Message technique, vide en cas de succès.
    std::string message;

    [[nodiscard]] bool ok() const noexcept {
        return message.empty();
    }
};

/// @brief Lit la description d'un squelette depuis le texte de son fichier.
[[nodiscard]] SkeletonFileResult readSkeletonDescription(std::string_view json);

/// @brief Lit `skeleton.json` ; un fichier absent est une erreur du résultat.
[[nodiscard]] SkeletonFileResult readSkeletonFile(const std::filesystem::path& path);

/// @return Le chemin, relatif au dossier des assets, de la description du squelette de
///         @p silhouette : `Common/Characters/Skeletons/<silhouette>/skeleton.json`.
[[nodiscard]] std::string skeletonFilePath(std::string_view silhouette);

/// @brief La fiche d'un personnage : son modèle et son squelette.
struct CharacterSheetFile {
    /// Le fichier du modèle, relatif au dossier de la fiche (`brawler.glb`).
    std::string model;
    /// La silhouette du squelette auquel le modèle est lié (`humanoid`).
    std::string skeleton;

    [[nodiscard]] bool operator==(const CharacterSheetFile&) const = default;
};

/// @brief Résultat d'une lecture de fiche : la fiche, ou ce qui a échoué.
struct CharacterSheetFileResult {
    CharacterSheetFile sheet;
    std::string message;

    [[nodiscard]] bool ok() const noexcept {
        return message.empty();
    }
};

/// @brief Lit la fiche d'un personnage depuis le texte de son fichier.
[[nodiscard]] CharacterSheetFileResult readCharacterSheet(std::string_view json);

/// @brief Lit `character.json` ; un fichier absent est une erreur du résultat.
[[nodiscard]] CharacterSheetFileResult readCharacterSheetFile(const std::filesystem::path& path);

}  // namespace core
