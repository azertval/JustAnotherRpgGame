// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>
#include <cstddef>
#include <filesystem>

#include "HMI/Input/Key.h"

/**
 * @file Editor/Logic/EditorKeyBindings.h
 * @brief Association remappable action d'éditeur -> touche clavier, avec persistance
 *        (`EX-CTRL-012`).
 */

namespace hmi {

/**
 * @brief Action d'éditeur logique, remappable indépendamment de la touche physique.
 *
 * Sous-ensemble **significatif** des raccourcis de l'éditeur (`hmi::EditorViewport`),
 * pas exhaustif (décision de cadrage) : navigation de menu, redimensionnement par flèches,
 * `Ctrl+R`, `"0"`, `Tab`, Maj+clic restent câblés en dur. Le modificateur `Ctrl` de
 * Save/Undo/Redo/Copy/Paste reste lui-même câblé en dur ; seule la touche-lettre associée est ici
 * remappable.
 */
enum class EditorAction {
    Save,
    Undo,
    Redo,
    Copy,
    Paste,
    Playtest,
    ToggleGrid,
    ToggleHelp,
    Rename,
};

/// Nombre d'actions d'éditeur remappables (`EditorAction`).
constexpr int EDITOR_ACTION_COUNT = 9;

/**
 * @brief Association action d'éditeur -> touche, avec persistance JSON (`EX-CTRL-012`).
 *
 * Échange sur conflit ; persistance dans la section `"editeur"` de `keybindings.json`, fusionnée
 * plutôt qu'écrasée à chaque sauvegarde : les autres sections du fichier sont préservées.
 */
class EditorKeyBindings {
public:
    /// Construit avec les valeurs par défaut (S/Z/Y/C/V/P/F10/F1/F2, cf. l'éditeur
    /// `hmi::EditorViewport`).
    EditorKeyBindings();

    /// @return La touche actuellement liée à @p action.
    [[nodiscard]] Key key(EditorAction action) const noexcept;

    /**
     * @brief Lie @p action à @p newKey.
     *
     * Si @p newKey était déjà liée à une autre action de ce jeu de bindings, les deux échangent
     * leurs touches (jamais deux actions sur la même touche à l'issue de l'appel).
     */
    void setKey(EditorAction action, Key newKey) noexcept;

    /// Restaure les neuf actions à leurs touches par défaut (`defaultKey`).
    void resetToDefaults() noexcept;

    /// @return La touche par défaut de @p action, indépendante de l'état courant.
    [[nodiscard]] static Key defaultKey(EditorAction action) noexcept;

    /**
     * @brief Sauvegarde dans la section `"editeur"` de @p path, en préservant les autres
     *        sections déjà présentes.
     * @return Faux si le fichier n'a pas pu être écrit (dossier non créable, etc.).
     */
    bool save(const std::filesystem::path& path) const;

    /**
     * @brief Charge la section `"editeur"` de @p path.
     * @return Les bindings lus ; valeurs par défaut pour toute entrée absente, invalide, ou si le
     *         fichier est absent/corrompu (récupérable, jamais bloquant, `EX-NFR-040`).
     */
    [[nodiscard]] static EditorKeyBindings load(const std::filesystem::path& path);

private:
    std::array<Key, EDITOR_ACTION_COUNT> _keys{};
};

}  // namespace hmi
