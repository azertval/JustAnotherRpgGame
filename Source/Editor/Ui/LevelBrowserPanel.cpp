// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/LevelBrowserPanel.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QVBoxLayout>
#include <utility>

#include "Core/World/WorldGraph.h"
#include "Editor/Logic/LevelFileOperations.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Ui/WorldGraphView.h"
#include "HMI/HmiLog.h"

namespace hmi {

namespace {

// Rôle portant le chemin absolu du fichier d'un item de la liste.
constexpr int PATH_ROLE = Qt::UserRole + 1;

// Taille proposée pour une nouvelle carte, et son plafond (celui du redimensionnement).
constexpr int NEW_MAP_WIDTH = 24;
constexpr int NEW_MAP_HEIGHT = 14;
constexpr int NEW_MAP_MAXIMUM_SIDE = 100;

// Texte localisé d'une clé (repli sur la clé si aucun catalogue — ne survient pas en pratique).
// Signale l'échec éventuel d'une opération à l'utilisateur (jamais silencieux) et le journalise.
void reportIfError(QWidget* parent, const QString& title, const FileOperationResult& result) {
    if (!result.ok()) {
        HMI_LOG_WARNING("Niveaux : operation fichier echouee : " + result.error);
        QMessageBox::warning(parent, title, QString::fromStdString(result.error));
    }
}

}  // namespace

/// Les widgets du panneau : un onglet « List » (recherche, liste, boutons de gestion) et un onglet
/// « Graph » (le graphe du monde).
struct LevelBrowserPanel::Widgets {
    QTabWidget* viewTabs;
    QLineEdit* searchField;
    QListView* levelList;
    QPushButton* newButton;
    QPushButton* renameButton;
    QPushButton* duplicateButton;
    QPushButton* deleteButton;
    WorldGraphView* worldGraph;

    explicit Widgets(QWidget* panel)
        : viewTabs(new QTabWidget(panel)),
          searchField(new QLineEdit),
          levelList(new QListView),
          newButton(new QPushButton(QStringLiteral("New"))),
          renameButton(new QPushButton(QStringLiteral("Rename"))),
          duplicateButton(new QPushButton(QStringLiteral("Duplicate"))),
          deleteButton(new QPushButton(QStringLiteral("Delete"))),
          worldGraph(new WorldGraphView) {
        searchField->setPlaceholderText(QStringLiteral("Search…"));
        searchField->setClearButtonEnabled(true);

        auto* const listTab = new QWidget;
        auto* const buttonRow = new QHBoxLayout;
        for (QPushButton* const button : {newButton, renameButton, duplicateButton, deleteButton}) {
            buttonRow->addWidget(button);
        }
        auto* const listLayout = new QVBoxLayout(listTab);
        listLayout->addWidget(searchField);
        listLayout->addWidget(levelList);
        listLayout->addLayout(buttonRow);
        auto* const graphTab = new QWidget;
        auto* const graphLayout = new QVBoxLayout(graphTab);
        graphLayout->addWidget(worldGraph);
        viewTabs->addTab(listTab, QStringLiteral("List"));
        viewTabs->addTab(graphTab, QStringLiteral("Graph"));

        auto* const layout = new QVBoxLayout(panel);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(viewTabs);
    }
};

LevelBrowserPanel::LevelBrowserPanel(std::filesystem::path levelsDir, QWidget* parent)
    : QWidget(parent),
      _ui(std::make_unique<Widgets>(this)),
      _dir(std::move(levelsDir)),
      _model(new QStandardItemModel(this)),
      _proxy(new QSortFilterProxyModel(this)) {
    // Filtre de recherche (modèle -> proxy) et branchement de la liste.
    _proxy->setSourceModel(_model);
    _proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    _ui->levelList->setModel(_proxy);
    _ui->levelList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _ui->levelList->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(_ui->searchField, &QLineEdit::textChanged, _proxy,
            &QSortFilterProxyModel::setFilterFixedString);
    connect(_ui->levelList, &QListView::doubleClicked, this, &LevelBrowserPanel::onActivated);
    connect(_ui->newButton, &QPushButton::clicked, this, &LevelBrowserPanel::onNew);
    connect(_ui->renameButton, &QPushButton::clicked, this, &LevelBrowserPanel::onRename);
    connect(_ui->duplicateButton, &QPushButton::clicked, this, &LevelBrowserPanel::onDuplicate);
    connect(_ui->deleteButton, &QPushButton::clicked, this, &LevelBrowserPanel::onDelete);
    // Le graphe ouvre une carte par le même signal que la liste : MainWindow n'a rien à brancher.
    connect(_ui->worldGraph, &WorldGraphView::levelOpenRequested, this,
            &LevelBrowserPanel::levelOpenRequested);

    refresh();
}

LevelBrowserPanel::~LevelBrowserPanel() = default;

void LevelBrowserPanel::refresh() {
    _model->clear();
    const LevelFileOperations ops(_dir);
    for (const std::filesystem::path& path : ops.list()) {
        // L'identifiant de carte, sous-dossier compris : `capital/martpart` (`LOT-96`).
        auto* const item = new QStandardItem(QString::fromStdString(core::mapIdOf(_dir, path)));
        item->setEditable(false);
        item->setData(QString::fromStdString(path.string()), PATH_ROLE);
        _model->appendRow(item);
    }
    _model->sort(0);
    refreshWorldGraph();
}

void LevelBrowserPanel::refreshWorldGraph() {
    _ui->worldGraph->setGraph(core::loadWorldGraph(_dir), _dir);
}

std::filesystem::path LevelBrowserPanel::selectedPath() const {
    const QModelIndex proxyIndex = _ui->levelList->currentIndex();
    if (!proxyIndex.isValid()) {
        return {};
    }
    const QVariant pathData = _proxy->mapToSource(proxyIndex).data(PATH_ROLE);
    return {pathData.toString().toStdString()};
}

void LevelBrowserPanel::onNew() {
    // Nom, taille et lieu (LOT-EDITOR-06) : sans lieu, la palette n'aurait que les types en
    // couleurs, et aucune pièce à poser.
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("New map"));
    auto* const nameEdit = new QLineEdit(&dialog);
    auto* const widthSpin = new QSpinBox(&dialog);
    auto* const heightSpin = new QSpinBox(&dialog);
    for (QSpinBox* const spin : {widthSpin, heightSpin}) {
        spin->setRange(1, NEW_MAP_MAXIMUM_SIDE);
    }
    widthSpin->setValue(NEW_MAP_WIDTH);
    heightSpin->setValue(NEW_MAP_HEIGHT);
    auto* const placeCombo = new QComboBox(&dialog);
    for (const std::string& place : scenePlaces(_dir.parent_path())) {
        placeCombo->addItem(QString::fromStdString(place), QString::fromStdString(place));
    }
    placeCombo->addItem(QStringLiteral("(none: colored tile types)"), QString());
    auto* const buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    auto* const form = new QFormLayout(&dialog);
    form->addRow(QStringLiteral("Map name"), nameEdit);
    form->addRow(QStringLiteral("Width (cells)"), widthSpin);
    form->addRow(QStringLiteral("Height (cells)"), heightSpin);
    form->addRow(QStringLiteral("Place (piece sheet)"), placeCombo);
    form->addRow(buttons);
    if (dialog.exec() != QDialog::Accepted || nameEdit->text().isEmpty()) {
        return;
    }
    const QString name = nameEdit->text();
    const LevelFileOperations ops(_dir);
    const FileOperationResult result =
        ops.create(name.toStdString(), widthSpin->value(), heightSpin->value(),
                   placeCombo->currentData().toString().toStdString());
    if (result.ok()) {
        HMI_LOG_INFO("Niveaux : cree « " + name.toStdString() + " ».");
    }
    reportIfError(this, QStringLiteral("Operation failed"), result);
    refresh();
}

void LevelBrowserPanel::onRename() {
    const std::filesystem::path path = selectedPath();
    if (!path.empty()) {
        emit mapRenameRequested(QString::fromStdString(core::mapIdOf(_dir, path)));
    }
}

void LevelBrowserPanel::onDuplicate() {
    const std::filesystem::path path = selectedPath();
    if (path.empty()) {
        return;
    }
    const LevelFileOperations ops(_dir);
    HMI_LOG_INFO("Niveaux : duplication de « " + path.stem().string() + " ».");
    reportIfError(this, QStringLiteral("Operation failed"), ops.duplicate(path));
    refresh();
}

void LevelBrowserPanel::onDelete() {
    const std::filesystem::path path = selectedPath();
    if (path.empty()) {
        return;
    }
    const QMessageBox::StandardButton answer =
        QMessageBox::question(this, QStringLiteral("Delete"),
                              QStringLiteral("Permanently delete “%1”?")
                                  .arg(QString::fromStdString(path.stem().string())));
    if (answer != QMessageBox::Yes) {
        return;
    }
    const LevelFileOperations ops(_dir);
    HMI_LOG_INFO("Niveaux : suppression de « " + path.stem().string() + " ».");
    reportIfError(this, QStringLiteral("Operation failed"), hmi::LevelFileOperations::remove(path));
    refresh();
}

void LevelBrowserPanel::onActivated(const QModelIndex& index) {
    const QVariant pathData = _proxy->mapToSource(index).data(PATH_ROLE);
    if (pathData.isValid()) {
        emit levelOpenRequested(pathData.toString());
    }
}

}  // namespace hmi
