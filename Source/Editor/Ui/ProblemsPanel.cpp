// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/ProblemsPanel.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <algorithm>

namespace hmi {

namespace {

// Les colonnes de la liste.
constexpr int SEVERITY_COLUMN = 0;
constexpr int MAP_COLUMN = 1;
constexpr int CELL_COLUMN = 2;
constexpr int MESSAGE_COLUMN = 3;
constexpr int COLUMN_COUNT = 4;

// Le rang du constat dans le bilan, porté par sa ligne.
constexpr int FINDING_ROLE = Qt::UserRole + 1;

}  // namespace

ProblemsPanel::ProblemsPanel(QWidget* parent)
    : QWidget(parent),
      _summary(new QLabel(QStringLiteral("Not checked yet."), this)),
      _errorsOnly(new QCheckBox(QStringLiteral("Errors only"), this)),
      _tree(new QTreeWidget(this)) {
    auto* const check = new QPushButton(QStringLiteral("Check all maps"), this);
    check->setToolTip(
        QStringLiteral("Check every map as saved on disk: what LevelEditor --check runs in CI."));
    connect(check, &QPushButton::clicked, this, &ProblemsPanel::checkRequested);
    connect(_errorsOnly, &QCheckBox::toggled, this, [this](bool) { rebuild(); });

    _tree->setColumnCount(COLUMN_COUNT);
    _tree->setHeaderLabels({QStringLiteral("Severity"), QStringLiteral("Map"),
                            QStringLiteral("Cell"), QStringLiteral("Problem")});
    _tree->setRootIsDecorated(false);
    _tree->setUniformRowHeights(true);
    _tree->setAlternatingRowColors(true);
    // Croissant : l'en-tête part d'un tri descendant, qui mettrait les avertissements devant.
    _tree->sortByColumn(SEVERITY_COLUMN, Qt::AscendingOrder);
    _tree->setSortingEnabled(true);
    _tree->header()->setStretchLastSection(true);
    _tree->setToolTip(QStringLiteral("Double-click a problem to go to its cell."));
    connect(_tree, &QTreeWidget::itemActivated, this,
            [this](QTreeWidgetItem* item, int) { onActivated(item); });

    auto* const bar = new QHBoxLayout;
    bar->addWidget(check);
    bar->addWidget(_errorsOnly);
    bar->addWidget(_summary, 1);
    auto* const layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->addLayout(bar);
    layout->addWidget(_tree, 1);
}

void ProblemsPanel::setReport(const MapCheckReport& report) {
    _report = report;
    // Erreurs d'abord ; à gravité égale, l'ordre du contrôle (carte, puis sorte de constat).
    std::ranges::stable_sort(_report.findings, {}, [](const MapCheckFinding& finding) {
        return finding.severity == MapCheckSeverity::Error ? 0 : 1;
    });
    rebuild();
}

void ProblemsPanel::rebuild() {
    const std::size_t errors = _report.count(MapCheckSeverity::Error);
    const std::size_t warnings = _report.count(MapCheckSeverity::Warning);
    _summary->setText(QStringLiteral("%1 maps checked as saved: %2 errors, %3 warnings")
                          .arg(_report.maps)
                          .arg(errors)
                          .arg(warnings));

    _tree->setSortingEnabled(false);
    _tree->clear();
    const QIcon errorIcon = style()->standardIcon(QStyle::SP_MessageBoxCritical);
    const QIcon warningIcon = style()->standardIcon(QStyle::SP_MessageBoxWarning);
    for (std::size_t index = 0; index < _report.findings.size(); ++index) {
        const MapCheckFinding& finding = _report.findings[index];
        const bool error = finding.severity == MapCheckSeverity::Error;
        if (_errorsOnly->isChecked() && !error) {
            continue;
        }
        auto* const item = new QTreeWidgetItem(_tree);
        // « error » précède « warning » dans l'alphabet : le tri par gravité garde les erreurs
        // devant.
        item->setIcon(SEVERITY_COLUMN, error ? errorIcon : warningIcon);
        item->setText(SEVERITY_COLUMN, error ? QStringLiteral("error") : QStringLiteral("warning"));
        item->setText(MAP_COLUMN, QString::fromStdString(finding.mapId));
        if (finding.cell) {
            item->setText(
                CELL_COLUMN,
                QStringLiteral("%1, %2").arg(finding.cell->column).arg(finding.cell->row));
        }
        item->setText(MESSAGE_COLUMN, QString::fromStdString(finding.message));
        item->setToolTip(MESSAGE_COLUMN, QString::fromStdString(finding.message));
        item->setData(SEVERITY_COLUMN, FINDING_ROLE, static_cast<qulonglong>(index));
    }
    _tree->setSortingEnabled(true);
    for (int column = 0; column < MESSAGE_COLUMN; ++column) {
        _tree->resizeColumnToContents(column);
    }
}

void ProblemsPanel::onActivated(QTreeWidgetItem* item) {
    if (item == nullptr) {
        return;
    }
    const auto index =
        static_cast<std::size_t>(item->data(SEVERITY_COLUMN, FINDING_ROLE).toULongLong());
    if (index < _report.findings.size()) {
        emit findingActivated(_report.findings[index]);
    }
}

}  // namespace hmi
