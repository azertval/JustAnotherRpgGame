// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/RunInGameDialog.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>

#include "Editor/Ui/WorldStateEditor.h"

namespace hmi {

std::optional<RunInGameChoice> askRunInGame(QWidget* parent, const QString& mapId,
                                            const std::vector<std::string>& knownFlags,
                                            const std::vector<core::QuestFlag>& declared,
                                            core::GridPosition bounds,
                                            const RunInGameChoice& current) {
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Run in game"));
    auto* const layout = new QVBoxLayout(&dialog);

    auto* const form = new QFormLayout;
    layout->addLayout(form);
    form->addRow(QStringLiteral("Map"),
                 new QLabel(mapId.isEmpty() ? QStringLiteral("(unsaved)") : mapId, &dialog));

    // D'ou l'on part : l'entree de la carte, ou une case. La case est bornee a la carte -- le jeu
    // refuserait d'y poser le heros, et le message arriverait trop tard.
    auto* const fromCell = new QCheckBox(QStringLiteral("Start from cell"), &dialog);
    auto* const column = new QSpinBox(&dialog);
    auto* const row = new QSpinBox(&dialog);
    column->setRange(0, std::max(0, bounds.column - 1));
    row->setRange(0, std::max(0, bounds.row - 1));
    fromCell->setChecked(current.cell.has_value());
    column->setValue(current.cell ? current.cell->column : 0);
    row->setValue(current.cell ? current.cell->row : 0);
    QObject::connect(fromCell, &QCheckBox::toggled, column, &QWidget::setEnabled);
    QObject::connect(fromCell, &QCheckBox::toggled, row, &QWidget::setEnabled);
    column->setEnabled(fromCell->isChecked());
    row->setEnabled(fromCell->isChecked());
    auto* const cellRow = new QHBoxLayout;
    cellRow->addWidget(fromCell);
    cellRow->addWidget(new QLabel(QStringLiteral("column"), &dialog));
    cellRow->addWidget(column);
    cellRow->addWidget(new QLabel(QStringLiteral("row"), &dialog));
    cellRow->addWidget(row);
    cellRow->addStretch();
    layout->addLayout(cellRow);

    // L'etat de partie : le meme selecteur que « World state… » (LOT-126).
    auto* const flagsBox = new QGroupBox(QStringLiteral("World state"), &dialog);
    auto* const flagsLayout = new QVBoxLayout(flagsBox);
    auto* const state = new WorldStateEditor(knownFlags, declared, current.flags, flagsBox);
    flagsLayout->addWidget(state);
    layout->addWidget(flagsBox);

    auto* const buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Run"));
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return std::nullopt;
    }
    RunInGameChoice choix;
    if (fromCell->isChecked()) {
        choix.cell = core::GridPosition{.column = column->value(), .row = row->value()};
    }
    choix.flags = state->entries();
    return choix;
}

}  // namespace hmi
