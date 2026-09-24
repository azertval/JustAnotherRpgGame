// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/WorldStateEditor.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>
#include <algorithm>

#include "Editor/Logic/WorldState.h"

namespace hmi {

namespace {

/// Les faits d'une etape de quete (`quest/<quete>/step/<etape>`) ne se cochent pas : c'est la
/// quete qui les pose, d'apres les drapeaux qu'on regle.
[[nodiscard]] bool isStepFact(const std::string& flag) {
    return flag.starts_with("quest/");
}

}  // namespace

WorldStateEditor::WorldStateEditor(const std::vector<std::string>& knownFlags,
                                   const std::vector<core::QuestFlag>& declared,
                                   const std::vector<std::string>& entries, QWidget* parent)
    : QWidget(parent) {
    auto* const layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Un choix par drapeau declare : sa valeur initiale d'abord, celle d'une partie neuve.
    if (!declared.empty()) {
        auto* const valuesBox = new QGroupBox(QStringLiteral("Quest flags"), this);
        auto* const form = new QFormLayout(valuesBox);
        for (const core::QuestFlag& flag : declared) {
            auto* const combo = new QComboBox(valuesBox);
            for (const std::string& value : flag.values) {
                const QString text = QString::fromStdString(value);
                combo->addItem(value == flag.initial ? text + QStringLiteral(" (initial)") : text,
                               text);
            }
            const std::string current = worldStateValue(entries, flag.id).value_or(flag.initial);
            combo->setCurrentIndex(std::max(0, combo->findData(QString::fromStdString(current))));
            connect(combo, &QComboBox::currentIndexChanged, this, &WorldStateEditor::changed);
            form->addRow(QString::fromStdString(flag.id), combo);
            _values.emplace_back(flag, combo);
        }
        layout->addWidget(valuesBox);
    }

    // Les faits : tout drapeau connu qu'aucune quete ne declare.
    auto* const factsBox = new QGroupBox(QStringLiteral("Facts"), this);
    auto* const factsLayout = new QVBoxLayout(factsBox);
    _facts = new QListWidget(factsBox);
    std::vector<std::string> listed;
    for (const std::string& flag : knownFlags) {
        if (isStepFact(flag) || std::ranges::any_of(declared, [&flag](const core::QuestFlag& d) {
                return d.id == flag;
            })) {
            continue;
        }
        listed.push_back(flag);
        auto* const item = new QListWidgetItem(QString::fromStdString(flag), _facts);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(std::ranges::find(entries, flag) == entries.end() ? Qt::Unchecked
                                                                              : Qt::Checked);
    }
    connect(_facts, &QListWidget::itemChanged, this, &WorldStateEditor::changed);
    factsLayout->addWidget(_facts);
    // Ce que la liste ne propose pas : un fait qu'aucun dialogue ne pose encore.
    QStringList others;
    for (const std::string& entry : entries) {
        const WorldStateEntry read = parseWorldStateEntry(entry);
        const bool declaredValue = read.value.has_value();
        if (!declaredValue && std::ranges::find(listed, entry) == listed.end()) {
            others.push_back(QString::fromStdString(entry));
        }
    }
    _extra = new QLineEdit(others.join(QStringLiteral(", ")), factsBox);
    _extra->setPlaceholderText(QStringLiteral("other flags, comma separated"));
    connect(_extra, &QLineEdit::editingFinished, this, &WorldStateEditor::changed);
    factsLayout->addWidget(_extra);
    layout->addWidget(factsBox);
}

std::vector<std::string> WorldStateEditor::entries() const {
    std::vector<std::string> state;
    for (const auto& [flag, combo] : _values) {
        const std::string value = combo->currentData().toString().toStdString();
        if (value != flag.initial) {
            state.push_back(flag.id + "=" + value);
        }
    }
    for (int index = 0; index < _facts->count(); ++index) {
        const QListWidgetItem* const item = _facts->item(index);
        if (item->checkState() == Qt::Checked) {
            state.push_back(item->text().toStdString());
        }
    }
    for (const QString& piece : _extra->text().split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        std::string flag = piece.trimmed().toStdString();
        if (!flag.empty() && std::ranges::find(state, flag) == state.end()) {
            state.push_back(std::move(flag));
        }
    }
    return state;
}

std::optional<WorldStateChoice> askWorldState(QWidget* parent,
                                              const std::vector<std::string>& knownFlags,
                                              const std::vector<core::QuestFlag>& declared,
                                              const WorldStateChoice& current) {
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("World state"));
    auto* const layout = new QVBoxLayout(&dialog);
    auto* const help = new QLabel(QStringLiteral("The state the playtest (P) and Run in game (F5) "
                                                 "start from. Shown on the canvas, entities it "
                                                 "leaves absent are greyed."),
                                  &dialog);
    help->setWordWrap(true);
    layout->addWidget(help);
    auto* const preview = new QCheckBox(QStringLiteral("Show the map under this state"), &dialog);
    preview->setChecked(current.preview);
    layout->addWidget(preview);
    auto* const editor = new WorldStateEditor(knownFlags, declared, current.entries, &dialog);
    layout->addWidget(editor);
    auto* const buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) {
        return std::nullopt;
    }
    return WorldStateChoice{.entries = editor->entries(), .preview = preview->isChecked()};
}

}  // namespace hmi
