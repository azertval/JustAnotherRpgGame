// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/RefactorDialogs.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cstddef>

#include "Editor/Logic/MapFormat.h"

namespace hmi {

namespace {

/// Largeur des dialogues de liste : une citation tient sur une ligne.
constexpr int LIST_DIALOG_WIDTH = 640;
constexpr int LIST_DIALOG_HEIGHT = 360;

/// Le choix « pas de correspondant » d'une ligne de la table.
constexpr const char* NO_MATCH = "(no match)";

[[nodiscard]] bool isFloor(const core::ScenePiece* piece) {
    return piece != nullptr && piece->pieceClass == core::ScenePieceClass::Floor;
}

}  // namespace

std::optional<Citation> showCitations(QWidget* parent, const QString& title,
                                      const std::vector<Citation>& citations,
                                      const std::filesystem::path& dataRoot) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.resize(LIST_DIALOG_WIDTH, LIST_DIALOG_HEIGHT);
    auto* const layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(
        citations.empty()
            ? QStringLiteral("Nothing cites it.")
            : QStringLiteral("%1 citation(s). Double-click one to go to it.").arg(citations.size()),
        &dialog));
    auto* const list = new QListWidget(&dialog);
    for (const Citation& citation : citations) {
        list->addItem(QString::fromStdString(formatCitation(citation, dataRoot)));
    }
    layout->addWidget(list);
    auto* const buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    std::optional<Citation> chosen;
    QObject::connect(list, &QListWidget::itemActivated, &dialog, [&](QListWidgetItem* item) {
        chosen = citations[static_cast<std::size_t>(list->row(item))];
        dialog.accept();
    });
    dialog.exec();
    return chosen;
}

bool confirmPlan(QWidget* parent, const QString& title, const RefactorPlan& plan,
                 const std::filesystem::path& dataRoot) {
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.resize(LIST_DIALOG_WIDTH, LIST_DIALOG_HEIGHT);
    auto* const layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(QStringLiteral("%1 file(s) will be written or removed. What "
                                                "changes:")
                                     .arg(plan.edits.size()),
                                 &dialog));
    auto* const list = new QListWidget(&dialog);
    for (const Citation& change : plan.changes) {
        list->addItem(QString::fromStdString(formatCitation(change, dataRoot)));
    }
    layout->addWidget(list);
    auto* const buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    return dialog.exec() == QDialog::Accepted;
}

std::optional<PieceReplacementChoice> askPieceReplacement(QWidget* parent,
                                                          const std::vector<std::string>& cited,
                                                          const core::ScenePieceManifest& sheet) {
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Replace piece"));
    auto* const fromCombo = new QComboBox(&dialog);
    for (const std::string& piece : cited) {
        fromCombo->addItem(QString::fromStdString(piece));
    }
    auto* const toCombo = new QComboBox(&dialog);
    // On ne propose que des pièces de la même classe : un sol pour un sol.
    const auto fillTargets = [&] {
        toCombo->clear();
        const bool floor = isFloor(sheet.find(fromCombo->currentText().toStdString()));
        for (const core::ScenePiece& piece : sheet.pieces()) {
            if (isFloor(&piece) == floor && piece.name != fromCombo->currentText().toStdString()) {
                toCombo->addItem(QString::fromStdString(piece.name));
            }
        }
    };
    fillTargets();
    QObject::connect(fromCombo, &QComboBox::currentTextChanged, &dialog, fillTargets);
    auto* const thisMap = new QRadioButton(QStringLiteral("On this map (undoable)"), &dialog);
    auto* const allMaps =
        new QRadioButton(QStringLiteral("On every map that places it (rewrites files)"), &dialog);
    thisMap->setChecked(true);
    auto* const buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    auto* const form = new QFormLayout(&dialog);
    form->addRow(QStringLiteral("Replace"), fromCombo);
    form->addRow(QStringLiteral("With"), toCombo);
    form->addRow(thisMap);
    form->addRow(allMaps);
    form->addRow(buttons);
    if (dialog.exec() != QDialog::Accepted || fromCombo->currentText().isEmpty() ||
        toCombo->currentText().isEmpty()) {
        return std::nullopt;
    }
    return PieceReplacementChoice{.from = fromCombo->currentText().toStdString(),
                                  .to = toCombo->currentText().toStdString(),
                                  .allMaps = allMaps->isChecked()};
}

std::optional<SceneChangeChoice> askSceneChange(QWidget* parent,
                                                const std::vector<core::TileLayer>& layers,
                                                const std::filesystem::path& dataRoot,
                                                const std::string& currentPlace) {
    QDialog dialog(parent);
    dialog.setWindowTitle(QStringLiteral("Change sheet"));
    dialog.resize(LIST_DIALOG_WIDTH, LIST_DIALOG_HEIGHT);
    auto* const placeCombo = new QComboBox(&dialog);
    for (const std::string& place : scenePlaces(dataRoot)) {
        if (place != currentPlace) {
            placeCombo->addItem(QString::fromStdString(place));
        }
    }
    auto* const table = new QTableWidget(&dialog);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(
        {QStringLiteral("Piece on this map"), QStringLiteral("Piece on the new sheet")});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    auto* const hint = new QLabel(&dialog);
    auto* const buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // Les pièces que la carte pose, une ligne chacune.
    std::vector<std::string> cited;
    for (const core::TileLayer& layer : layers) {
        if (core::isVisualLayerKind(layer.kind)) {
            for (const std::string& piece : layer.pieces) {
                if (!piece.empty() && std::ranges::find(cited, piece) == cited.end()) {
                    cited.push_back(piece);
                }
            }
        }
    }
    std::ranges::sort(cited);

    const auto refreshOk = [&] {
        int holes = 0;
        for (int row = 0; row < table->rowCount(); ++row) {
            const auto* combo = qobject_cast<QComboBox*>(table->cellWidget(row, 1));
            holes += combo == nullptr || combo->currentIndex() == 0 ? 1 : 0;
        }
        buttons->button(QDialogButtonBox::Ok)->setEnabled(placeCombo->count() > 0 && holes == 0);
        hint->setText(holes == 0 ? QStringLiteral("Every piece has a match: the map is kept.")
                                 : QStringLiteral("%1 piece(s) without a match.").arg(holes));
    };
    const auto rebuild = [&] {
        table->setRowCount(0);
        const PlaceAssets assets =
            loadPlaceAssets(dataRoot, placeCombo->currentText().toStdString());
        if (!assets.manifest) {
            refreshOk();
            return;
        }
        const core::PieceRenaming proposed = proposedPieceTable(layers, *assets.manifest);
        table->setRowCount(static_cast<int>(cited.size()));
        for (std::size_t index = 0; index < cited.size(); ++index) {
            const int row = static_cast<int>(index);
            auto* const name = new QTableWidgetItem(QString::fromStdString(cited[index]));
            name->setFlags(Qt::ItemIsEnabled);
            table->setItem(row, 0, name);
            auto* const combo = new QComboBox(table);
            combo->addItem(QString::fromLatin1(NO_MATCH));
            for (const core::ScenePiece& piece : assets.manifest->pieces()) {
                combo->addItem(QString::fromStdString(piece.name));
            }
            if (const auto found = proposed.find(cited[index]); found != proposed.end()) {
                combo->setCurrentText(QString::fromStdString(found->second));
            }
            QObject::connect(combo, &QComboBox::currentIndexChanged, &dialog, refreshOk);
            table->setCellWidget(row, 1, combo);
        }
        refreshOk();
    };
    QObject::connect(placeCombo, &QComboBox::currentTextChanged, &dialog, rebuild);
    rebuild();

    auto* const layout = new QVBoxLayout(&dialog);
    auto* const form = new QFormLayout;
    form->addRow(QStringLiteral("New sheet"), placeCombo);
    layout->addLayout(form);
    layout->addWidget(table);
    layout->addWidget(hint);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) {
        return std::nullopt;
    }
    SceneChangeChoice choice{.place = placeCombo->currentText().toStdString(), .table = {}};
    for (int row = 0; row < table->rowCount(); ++row) {
        const auto* combo = qobject_cast<QComboBox*>(table->cellWidget(row, 1));
        choice.table.emplace(table->item(row, 0)->text().toStdString(),
                             combo->currentText().toStdString());
    }
    return choice;
}

}  // namespace hmi
