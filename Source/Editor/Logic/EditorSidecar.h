// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "Core/Levels/GridPosition.h"

/**
 * @file Editor/Logic/EditorSidecar.h
 * @brief Le fichier annexe d'une carte, `<carte>.editor.json` : ce que l'éditeur garde pour
 *        l'auteur et que le jeu ne lit jamais (`LOT-EDITOR-04`, `EX-EDIT-068`).
 *
 * Il vit à côté de la carte (`capital/martpart.json` → `capital/martpart.editor.json`). Ce lot y
 * range les **notes d'auteur**, épinglées à une case ; la feuille de route y prévoit ensuite les
 * régions verrouillées et la dernière vue. Une clé que ce lecteur ne connaît pas est gardée telle
 * quelle et réécrite : un éditeur plus ancien ne perd pas ce qu'un plus récent a écrit.
 *
 * Les notes ne font pas partie de la carte : elles n'entrent ni dans l'historique d'annulation ni
 * dans l'indicateur de modification, et s'écrivent dès qu'on les change. Logique pure, sans Qt.
 */

namespace hmi {

/**
 * @brief Où en est une carte (`LOT-EDITOR-09`, `EX-EDIT-092`) : ce que le navigateur montre, et le
 *        tableau de bord du monde le jour où il y aura cent cartes.
 *
 * L'état est une note d'auteur, pas une donnée de jeu : il vit dans l'annexe, jamais dans la carte.
 */
enum class MapState {
    /// Rien n'est dit : une carte qu'on vient de créer.
    Unset,
    /// Sortie du générateur, ou posée par un modèle : personne ne l'a encore regardée.
    Generated,
    /// **Maquettée** (`LOT-128`) : sa physique est tracée — où l'on marche, ce qui bloque, qui est
    /// où, par où l'on sort — et elle se joue telle quelle, sans une seule pièce. L'état de départ
    /// normal d'une carte, avant qu'un lieu ne l'habille.
    Blockout,
    /// Retouchée à la main, mais pas terminée.
    Retouched,
    /// Finie : elle se joue telle quelle.
    Finished,
};

/// @return Le mot que l'annexe écrit pour @p state (`generated`) ; vide pour `Unset`.
[[nodiscard]] std::string_view mapStateKey(MapState state) noexcept;

/// @return L'état que nomme @p key ; `MapState::Unset` pour un mot inconnu.
[[nodiscard]] MapState mapStateFromKey(std::string_view key) noexcept;

/// @return Le libellé anglais de @p state, tel que la fenêtre l'affiche (`Generated`) ;
///         `not stated` pour `Unset`.
[[nodiscard]] std::string_view mapStateLabel(MapState state) noexcept;

/// Les états, dans l'ordre d'avancement : ce que les menus et les filtres listent.
[[nodiscard]] const std::vector<MapState>& knownMapStates();

/// @brief Une note d'auteur, épinglée à une case.
struct AuthorNote {
    core::GridPosition cell;
    std::string text;

    [[nodiscard]] bool operator==(const AuthorNote&) const = default;
};

/// @brief Le contenu d'un fichier annexe.
struct EditorSidecar {
    /// Version du format de l'annexe.
    static constexpr int FORMAT_VERSION = 1;

    /// Les notes, une au plus par case, triées ligne par ligne.
    std::vector<AuthorNote> notes;
    /// Où en est la carte (`LOT-EDITOR-09`) : `MapState::Unset` tant que l'auteur ne l'a pas dit.
    MapState state = MapState::Unset;
    /// Les clés que ce lecteur ne connaît pas, gardées pour la réécriture.
    nlohmann::json unknown = nlohmann::json::object();

    [[nodiscard]] bool operator==(const EditorSidecar&) const = default;

    /// @return Vrai si l'annexe n'a rien à garder : son fichier n'a pas lieu d'être.
    [[nodiscard]] bool empty() const noexcept {
        return notes.empty() && unknown.empty() && state == MapState::Unset;
    }
};

/// @return Le fichier annexe de la carte @p levelPath (`martpart.json` → `martpart.editor.json`).
[[nodiscard]] std::filesystem::path sidecarPath(const std::filesystem::path& levelPath);

/// @return Vrai si @p path est un fichier annexe d'éditeur, pas une carte.
[[nodiscard]] bool isSidecarFile(const std::filesystem::path& path);

/// @brief Ce que rend une lecture : l'annexe, et un avertissement si le fichier est illisible.
struct SidecarReadResult {
    EditorSidecar sidecar;
    /// Vide si le fichier se lit ou n'existe pas ; sinon, pourquoi il a été ignoré (en anglais).
    std::string warning;
};

/**
 * @brief Lit l'annexe depuis son texte JSON.
 *
 * Tolérant : une note mal formée est ignorée, les autres restent ; un texte qui n'est pas un objet
 * JSON rend une annexe vide et un avertissement. Jamais d'exception (`EX-NFR-040`).
 */
[[nodiscard]] SidecarReadResult parseSidecar(std::string_view json);

/// @brief Lit l'annexe de @p path ; un fichier absent rend une annexe vide, sans avertissement.
[[nodiscard]] SidecarReadResult readSidecar(const std::filesystem::path& path);

/// @return Le texte canonique de l'annexe : clés triées, deux espaces, fin de ligne finale.
[[nodiscard]] std::string sidecarJson(const EditorSidecar& sidecar);

/**
 * @brief Écrit l'annexe dans @p path, ou retire le fichier si elle est vide.
 * @return Faux si l'écriture ou le retrait a échoué.
 */
bool writeSidecar(const std::filesystem::path& path, const EditorSidecar& sidecar);

/// @return La note de @p cell, s'il y en a une.
[[nodiscard]] const AuthorNote* noteAt(const EditorSidecar& sidecar, core::GridPosition cell);

/**
 * @brief Écrit la note de @p cell ; un texte vide (ou fait d'espaces) la retire.
 * @return Vrai si l'annexe a changé.
 */
bool setNote(EditorSidecar& sidecar, core::GridPosition cell, std::string text);

}  // namespace hmi
