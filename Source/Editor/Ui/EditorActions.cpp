// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/EditorActions.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QKeyCombination>
#include <QKeySequence>
#include <QSignalBlocker>
#include <QString>
#include <QStyle>
#include <QToolBar>

#include "Editor/Logic/EditorKeyBindings.h"
#include "HMI/Input/QtKeyMap.h"

namespace hmi {

namespace {

/// Description d'une commande : de quoi construire son `QAction`.
struct CommandSpec {
    EditorCommand command{};
    const char* label = nullptr;
    /// Raccourci par défaut (`QKeySequence`), vide si aucun. Celui d'une commande remappable est
    /// remplacé par `applyShortcuts`.
    const char* shortcut = nullptr;
    /// Icône standard du style, ou `QStyle::SP_CustomBase` pour un simple libellé.
    QStyle::StandardPixmap icon = QStyle::SP_CustomBase;
    /// Dans la barre d'outils (usage continu), ou au menu seul.
    bool onToolBar = false;
    /// Commande remappable qui la déclenche, s'il y en a une.
    std::optional<EditorAction> binding;
};

constexpr QStyle::StandardPixmap TEXT_ONLY = QStyle::SP_CustomBase;

const std::array<CommandSpec, EDITOR_COMMAND_COUNT>& commandSpecs() {
    static const std::array<CommandSpec, EDITOR_COMMAND_COUNT> specs{{
        // Une touche par outil (LOT-EDITOR-04) : la lettre de son nom anglais, sauf le seau (G,
        // l'usage des logiciels de dessin), les entités (O, objets), la forme (Z, zone) et la
        // mesure (D, distance).
        {.command = EditorCommand::ToolPaint,
         .label = "Brush",
         .shortcut = "B",
         .icon = TEXT_ONLY,
         .onToolBar = true,
         .binding = std::nullopt},
        {.command = EditorCommand::ToolRectangle,
         .label = "Rectangle",
         .shortcut = "R",
         .icon = TEXT_ONLY,
         .onToolBar = true,
         .binding = std::nullopt},
        {.command = EditorCommand::ToolLine,
         .label = "Line",
         .shortcut = "L",
         .icon = TEXT_ONLY,
         .onToolBar = true,
         .binding = std::nullopt},
        {.command = EditorCommand::ToolBucket,
         .label = "Bucket",
         .shortcut = "G",
         .icon = TEXT_ONLY,
         .onToolBar = true,
         .binding = std::nullopt},
        {.command = EditorCommand::ToolEraser,
         .label = "Eraser",
         .shortcut = "E",
         .icon = TEXT_ONLY,
         .onToolBar = true,
         .binding = std::nullopt},
        {.command = EditorCommand::ToolPipette,
         .label = "Pipette",
         .shortcut = "I",
         .icon = TEXT_ONLY,
         .onToolBar = true,
         .binding = std::nullopt},
        {.command = EditorCommand::ToolSelection,
         .label = "Selection",
         .shortcut = "S",
         .icon = TEXT_ONLY,
         .onToolBar = true,
         .binding = std::nullopt},
        {.command = EditorCommand::ToolEntity,
         .label = "Entity",
         .shortcut = "O",
         .icon = TEXT_ONLY,
         .onToolBar = true,
         .binding = std::nullopt},
        {.command = EditorCommand::ToolShape,
         .label = "Shape",
         .shortcut = "Z",
         .icon = TEXT_ONLY,
         .onToolBar = true,
         .binding = std::nullopt},
        {.command = EditorCommand::ToolMeasure,
         .label = "Measure",
         .shortcut = "D",
         .icon = TEXT_ONLY,
         .onToolBar = true,
         .binding = std::nullopt},
        {.command = EditorCommand::ToolNote,
         .label = "Note",
         .shortcut = "N",
         .icon = TEXT_ONLY,
         .onToolBar = true,
         .binding = std::nullopt},
        {.command = EditorCommand::Save,
         .label = "Save",
         .shortcut = "Ctrl+S",
         .icon = QStyle::SP_DialogSaveButton,
         .onToolBar = true,
         .binding = EditorAction::Save},
        {.command = EditorCommand::Playtest,
         .label = "Playtest",
         .shortcut = "P",
         .icon = QStyle::SP_MediaPlay,
         .onToolBar = true,
         .binding = EditorAction::Playtest},
        {.command = EditorCommand::PlaytestHere,
         .label = "Playtest from hovered cell",
         .shortcut = "Shift+P",
         .icon = TEXT_ONLY,
         .onToolBar = false,
         .binding = std::nullopt},
        {.command = EditorCommand::Undo,
         .label = "Undo",
         .shortcut = "Ctrl+Z",
         .icon = QStyle::SP_ArrowBack,
         .onToolBar = true,
         .binding = EditorAction::Undo},
        {.command = EditorCommand::Redo,
         .label = "Redo",
         .shortcut = "Ctrl+Y",
         .icon = QStyle::SP_ArrowForward,
         .onToolBar = true,
         .binding = EditorAction::Redo},
        {.command = EditorCommand::ToggleGrid,
         .label = "Grid",
         .shortcut = "F10",
         .icon = TEXT_ONLY,
         .onToolBar = false,
         .binding = EditorAction::ToggleGrid},
        {.command = EditorCommand::ResetCamera,
         .label = "Reset camera",
         .shortcut = "0",
         .icon = TEXT_ONLY,
         .onToolBar = false,
         .binding = std::nullopt},
        {.command = EditorCommand::IsoView,
         .label = "Iso view",
         .shortcut = "F9",
         .icon = TEXT_ONLY,
         .onToolBar = true,
         .binding = std::nullopt},
        {.command = EditorCommand::SeeThroughRelief,
         .label = "See-through relief",
         .shortcut = "F8",
         .icon = TEXT_ONLY,
         .onToolBar = false,
         .binding = std::nullopt},
        {.command = EditorCommand::Mirror,
         .label = "Mirror",
         .shortcut = "M",
         .icon = TEXT_ONLY,
         .onToolBar = true,
         .binding = std::nullopt},
        {.command = EditorCommand::Copy,
         .label = "Copy",
         .shortcut = "Ctrl+C",
         .icon = TEXT_ONLY,
         .onToolBar = false,
         .binding = EditorAction::Copy},
        {.command = EditorCommand::Paste,
         .label = "Paste",
         .shortcut = "Ctrl+V",
         .icon = TEXT_ONLY,
         .onToolBar = false,
         .binding = EditorAction::Paste},
        {.command = EditorCommand::Rename,
         .label = "Rename",
         .shortcut = "F2",
         .icon = TEXT_ONLY,
         .onToolBar = false,
         .binding = EditorAction::Rename},
        {.command = EditorCommand::ShortcutsOverview,
         .label = "Shortcuts overview",
         .shortcut = "F1",
         .icon = TEXT_ONLY,
         .onToolBar = false,
         .binding = EditorAction::ToggleHelp},
    }};
    return specs;
}

// Le modificateur Ctrl de Save/Undo/Redo/Copy/Paste reste câblé en dur (EditorKeyBindings.h) :
// seule la touche-lettre associée est remappable.
[[nodiscard]] bool carriesImplicitCtrl(EditorAction action) {
    switch (action) {
        case EditorAction::Save:
        case EditorAction::Undo:
        case EditorAction::Redo:
        case EditorAction::Copy:
        case EditorAction::Paste:
            return true;
        default:
            return false;
    }
}

[[nodiscard]] std::size_t indexOf(EditorCommand command) {
    return static_cast<std::size_t>(command);
}

}  // namespace

EditorActions::EditorActions(QObject* parent)
    : QObject(parent), _toolGroup(new QActionGroup(this)) {
    _toolGroup->setExclusive(true);
    for (const CommandSpec& spec : commandSpecs()) {
        auto* const act = new QAction(QString::fromUtf8(spec.label), this);
        if (spec.icon != TEXT_ONLY) {
            act->setIcon(QApplication::style()->standardIcon(spec.icon));
        }
        if (spec.shortcut[0] != '\0') {
            act->setShortcut(QKeySequence(QString::fromLatin1(spec.shortcut)));
        }
        if (toolOf(spec.command)) {
            act->setCheckable(true);
            act->setActionGroup(_toolGroup);
        }
        _actions[indexOf(spec.command)] = act;
    }
    // Pinceau actif par défaut (EX-EDIT-014).
    action(EditorCommand::ToolPaint)->setChecked(true);
    // Deux bascules d'affichage (LOT-EDITOR-02) : la vue iso est celle par défaut (décision D1).
    action(EditorCommand::IsoView)->setCheckable(true);
    action(EditorCommand::IsoView)->setChecked(true);
    action(EditorCommand::SeeThroughRelief)->setCheckable(true);
    action(EditorCommand::Mirror)->setCheckable(true);
    refreshToolTips();
}

QAction* EditorActions::action(EditorCommand command) const {
    return _actions[indexOf(command)];
}

std::optional<EditorTool> EditorActions::toolOf(EditorCommand command) {
    switch (command) {
        case EditorCommand::ToolPaint:
            return EditorTool::Paint;
        case EditorCommand::ToolRectangle:
            return EditorTool::Rectangle;
        case EditorCommand::ToolLine:
            return EditorTool::Line;
        case EditorCommand::ToolBucket:
            return EditorTool::Bucket;
        case EditorCommand::ToolEraser:
            return EditorTool::Eraser;
        case EditorCommand::ToolPipette:
            return EditorTool::Pipette;
        case EditorCommand::ToolSelection:
            return EditorTool::Selection;
        case EditorCommand::ToolEntity:
            return EditorTool::Entity;
        case EditorCommand::ToolShape:
            return EditorTool::Shape;
        case EditorCommand::ToolMeasure:
            return EditorTool::Measure;
        case EditorCommand::ToolNote:
            return EditorTool::Note;
        default:
            return std::nullopt;
    }
}

QAction* EditorActions::toolAction(EditorTool tool) const {
    for (const CommandSpec& spec : commandSpecs()) {
        if (toolOf(spec.command) == tool) {
            return action(spec.command);
        }
    }
    return nullptr;
}

void EditorActions::populateToolBar(QToolBar& toolBar) const {
    bool separatorInserted = false;
    for (const CommandSpec& spec : commandSpecs()) {
        if (!spec.onToolBar) {
            continue;
        }
        if (!separatorInserted && !toolOf(spec.command)) {
            toolBar.addSeparator();
            separatorInserted = true;
        }
        toolBar.addAction(action(spec.command));
    }
}

void EditorActions::setActiveTool(EditorTool tool) const {
    QAction* const act = toolAction(tool);
    if (act == nullptr || act->isChecked()) {
        return;
    }
    const QSignalBlocker blocker(act);
    act->setChecked(true);
}

void EditorActions::applyShortcuts(const EditorKeyBindings& bindings) {
    for (const CommandSpec& spec : commandSpecs()) {
        if (!spec.binding) {
            continue;
        }
        const auto qtKey = static_cast<Qt::Key>(hmiKeyToQtKey(bindings.key(*spec.binding)));
        const Qt::KeyboardModifiers modifiers =
            carriesImplicitCtrl(*spec.binding) ? Qt::ControlModifier : Qt::NoModifier;
        action(spec.command)->setShortcut(QKeySequence(QKeyCombination(modifiers, qtKey)));
    }
    refreshToolTips();
}

void EditorActions::refreshToolTips() const {
    // Infobulle = libellé + raccourci de l'action elle-même : un remappage ne peut pas la rendre
    // fausse.
    for (QAction* const act : _actions) {
        const QKeySequence shortcut = act->shortcut();
        act->setToolTip(shortcut.isEmpty() ? act->text()
                                           : act->text() + QStringLiteral(" (") +
                                                 shortcut.toString(QKeySequence::NativeText) +
                                                 QStringLiteral(")"));
    }
}

}  // namespace hmi
