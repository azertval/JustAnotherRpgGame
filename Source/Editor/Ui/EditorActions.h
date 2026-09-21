// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QObject>
#include <array>
#include <cstddef>
#include <optional>

#include "Editor/Logic/EditorTool.h"

class QAction;
class QActionGroup;
class QToolBar;

/**
 * @file Editor/Ui/EditorActions.h
 * @brief Actions Qt de l'éditeur : outils et commandes principales.
 */

namespace hmi {

class EditorKeyBindings;

/// Une commande de l'éditeur : les onze outils, puis les commandes.
enum class EditorCommand {
    ToolPaint,
    ToolRectangle,
    ToolLine,
    ToolBucket,
    ToolEraser,
    ToolPipette,
    ToolSelection,
    ToolEntity,
    /// Peindre une zone, tracer un trajet (LOT-EDITOR-05).
    ToolShape,
    ToolMeasure,
    ToolNote,
    Save,
    Playtest,
    /// L'essai qui part de la case survolée (LOT-EDITOR-04).
    PlaytestHere,
    /// L'essai **complet** : le vrai jeu, sur les brouillons ouverts (LOT-EDITOR-10).
    RunInGame,
    /// L'essai complet qui part de la case survolée (LOT-EDITOR-10).
    RunInGameHere,
    /// L'essai complet après avoir choisi la case et les drapeaux (LOT-EDITOR-10).
    RunInGameOptions,
    Undo,
    Redo,
    ToggleGrid,
    ResetCamera,
    /// Vue iso (cochée, par défaut) ou vue à plat (LOT-EDITOR-02, décision D1).
    IsoView,
    /// Reliefs en transparence (LOT-EDITOR-02, phase 3).
    SeeThroughRelief,
    /// Le miroir : chaque geste se reflète de l'autre côté d'un axe (LOT-EDITOR-04).
    Mirror,
    Copy,
    Paste,
    /// Coller le tampon **reflété** (`LOT-EDITOR-08`).
    PasteMirrored,
    /// Enregistrer la sélection comme préfabriqué du lieu (`LOT-EDITOR-08`).
    SaveAsPrefab,
    Rename,
    ShortcutsOverview,
};

/// Nombre de commandes, déclaré au plus près de l'énumération qu'il compte.
inline constexpr std::size_t EDITOR_COMMAND_COUNT = 30;

/**
 * @brief Construit et possède les `QAction` de l'éditeur : chaque outil et chaque commande
 *        n'existe qu'une fois, placée dans la barre d'outils, un menu et son raccourci.
 *
 * Outil interne (`LOT-EDITOR-01`) : libellés anglais écrits ici, icônes standard du style Qt quand
 * il en a une, texte sinon. La barre d'outils porte les outils et les commandes d'usage continu ;
 * le reste vit au menu. Chaque outil a sa touche (`LOT-EDITOR-04`) : B, R, L, G, E, I, S, O, D,
 * N ; `M` bascule le miroir.
 */
class EditorActions : public QObject {
    Q_OBJECT

public:
    explicit EditorActions(QObject* parent = nullptr);

    /// @return L'action de @p command.
    [[nodiscard]] QAction* action(EditorCommand command) const;
    /// @return L'action de l'outil @p tool.
    [[nodiscard]] QAction* toolAction(EditorTool tool) const;
    /// @return L'outil porté par @p command, si c'est un outil.
    [[nodiscard]] static std::optional<EditorTool> toolOf(EditorCommand command);
    /// @return Toutes les actions, dans l'ordre de l'énumération.
    [[nodiscard]] const std::array<QAction*, EDITOR_COMMAND_COUNT>& all() const noexcept {
        return _actions;
    }

    /// Ajoute à @p toolBar les outils, un séparateur, puis les commandes d'usage continu.
    void populateToolBar(QToolBar& toolBar) const;

    /// Coche l'action de l'outil actif **sans** émettre `triggered` : resynchronisation quand le
    /// canevas change d'outil de lui-même (choisir une famille d'entité arme l'outil Entité).
    void setActiveTool(EditorTool tool) const;

    /**
     * @brief Fait refléter @p bindings sur le raccourci effectif de chaque commande remappable,
     *        puis refait les infobulles (« libellé (raccourci) »).
     */
    void applyShortcuts(const EditorKeyBindings& bindings);

private:
    void refreshToolTips() const;

    std::array<QAction*, EDITOR_COMMAND_COUNT> _actions{};
    QActionGroup* _toolGroup;
};

}  // namespace hmi
