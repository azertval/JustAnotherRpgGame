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
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>

namespace hmi {

namespace {

/// Les drapeaux saisis a la main, separes par des virgules : ceux qu'aucun dialogue ne pose encore.
[[nodiscard]] std::vector<std::string> extraFlags(const QString& text) {
    std::vector<std::string> saisis;
    for (const QString& morceau : text.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        saisis.push_back(morceau.trimmed().toStdString());
    }
    return saisis;
}

}  // namespace

std::optional<RunInGameChoice> askRunInGame(QWidget* parent, const QString& mapId,
                                            const std::vector<std::string>& knownFlags,
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

    auto* const flagsBox = new QGroupBox(QStringLiteral("World flags"), &dialog);
    auto* const flagsLayout = new QVBoxLayout(flagsBox);
    auto* const list = new QListWidget(flagsBox);
    for (const std::string& flag : knownFlags) {
        auto* const item = new QListWidgetItem(QString::fromStdString(flag), list);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(std::ranges::find(current.flags, flag) == current.flags.end()
                                ? Qt::Unchecked
                                : Qt::Checked);
    }
    flagsLayout->addWidget(list);
    // Ce que la liste ne propose pas : les drapeaux du dernier essai qu'aucun dialogue ne pose.
    QStringList libres;
    for (const std::string& flag : current.flags) {
        if (std::ranges::find(knownFlags, flag) == knownFlags.end()) {
            libres.push_back(QString::fromStdString(flag));
        }
    }
    auto* const extra = new QLineEdit(libres.join(QStringLiteral(", ")), flagsBox);
    extra->setPlaceholderText(QStringLiteral("other flags, comma separated"));
    flagsLayout->addWidget(extra);
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
    for (int index = 0; index < list->count(); ++index) {
        const QListWidgetItem* const item = list->item(index);
        if (item->checkState() == Qt::Checked) {
            choix.flags.push_back(item->text().toStdString());
        }
    }
    for (std::string& flag : extraFlags(extra->text())) {
        if (!flag.empty() && std::ranges::find(choix.flags, flag) == choix.flags.end()) {
            choix.flags.push_back(std::move(flag));
        }
    }
    return choix;
}

}  // namespace hmi
