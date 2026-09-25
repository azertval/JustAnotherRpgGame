// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/LevelBrowserPanel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QModelIndex>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QVBoxLayout>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "Core/Levels/LevelLoader.h"
#include "Core/Resources/ScenePlace.h"
#include "Core/World/WorldGraph.h"
#include "Editor/Logic/CityView.h"
#include "Editor/Logic/EditorSidecar.h"
#include "Editor/Logic/LevelFileOperations.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/Stamps.h"
#include "Editor/Ui/CityMapView.h"
#include "Editor/Ui/MapRender.h"
#include "Editor/Ui/WorldGraphView.h"
#include "HMI/HmiLog.h"

namespace hmi {

namespace {

// Rôle portant le chemin absolu du fichier d'un item de la liste.
constexpr int PATH_ROLE = Qt::UserRole + 1;

// Côté d'une vignette de carte, en pixels, et échelle de son rendu (LOT-EDITOR-09).
constexpr int THUMBNAIL_SIDE = 112;
constexpr double THUMBNAIL_SCALE = 0.25;
// Marge de la grille d'icônes autour d'une vignette : de quoi loger le nom dessous.
constexpr int THUMBNAIL_MARGIN_X = 24;
constexpr int THUMBNAIL_MARGIN_Y = 36;

// Le choix « tous les états » du filtre.
constexpr const char* EVERY_STATE = "Any state";

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
    QComboBox* stateFilter;
    QCheckBox* thumbnails;
    QListView* levelList;
    QPushButton* newButton;
    QPushButton* renameButton;
    QPushButton* duplicateButton;
    QPushButton* deleteButton;
    WorldGraphView* worldGraph;
    QComboBox* cityChooser;
    CityMapView* city;

    explicit Widgets(QWidget* panel)
        : viewTabs(new QTabWidget(panel)),
          searchField(new QLineEdit),
          stateFilter(new QComboBox),
          thumbnails(new QCheckBox(QStringLiteral("Thumbnails"))),
          levelList(new QListView),
          newButton(new QPushButton(QStringLiteral("New"))),
          renameButton(new QPushButton(QStringLiteral("Rename"))),
          duplicateButton(new QPushButton(QStringLiteral("Duplicate"))),
          deleteButton(new QPushButton(QStringLiteral("Delete"))),
          worldGraph(new WorldGraphView),
          cityChooser(new QComboBox),
          city(new CityMapView) {
        searchField->setPlaceholderText(QStringLiteral("Search…"));
        searchField->setClearButtonEnabled(true);
        // Où en est chaque carte (LOT-EDITOR-09) : le filtre du tableau de bord.
        stateFilter->addItem(QString::fromLatin1(EVERY_STATE), -1);
        for (const MapState state : knownMapStates()) {
            stateFilter->addItem(QString::fromUtf8(mapStateLabel(state).data(),
                                                   static_cast<int>(mapStateLabel(state).size())),
                                 static_cast<int>(state));
        }
        stateFilter->addItem(QStringLiteral("Not stated"), static_cast<int>(MapState::Unset));

        auto* const listTab = new QWidget;
        auto* const filterRow = new QHBoxLayout;
        filterRow->addWidget(stateFilter);
        filterRow->addWidget(thumbnails);
        auto* const buttonRow = new QHBoxLayout;
        for (QPushButton* const button : {newButton, renameButton, duplicateButton, deleteButton}) {
            buttonRow->addWidget(button);
        }
        auto* const listLayout = new QVBoxLayout(listTab);
        listLayout->addWidget(searchField);
        listLayout->addLayout(filterRow);
        listLayout->addWidget(levelList);
        listLayout->addLayout(buttonRow);
        auto* const graphTab = new QWidget;
        auto* const graphLayout = new QVBoxLayout(graphTab);
        graphLayout->addWidget(worldGraph);
        auto* const cityTab = new QWidget;
        auto* const cityLayout = new QVBoxLayout(cityTab);
        cityLayout->addWidget(cityChooser);
        cityLayout->addWidget(city);
        viewTabs->addTab(listTab, QStringLiteral("List"));
        viewTabs->addTab(graphTab, QStringLiteral("Graph"));
        viewTabs->addTab(cityTab, QStringLiteral("City"));

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
    // Le graphe et la ville ouvrent une carte par le même signal que la liste : MainWindow n'a
    // rien de plus à brancher.
    connect(_ui->worldGraph, &WorldGraphView::levelOpenRequested, this,
            &LevelBrowserPanel::levelOpenRequested);
    connect(_ui->city, &CityMapView::levelOpenRequested, this,
            &LevelBrowserPanel::levelOpenRequested);
    // Tirer un lien entre deux cartes du graphe (LOT-EDITOR-09) : la fenêtre mène le plan.
    connect(_ui->worldGraph, &WorldGraphView::linkRequested, this,
            &LevelBrowserPanel::mapLinkRequested);
    connect(_ui->stateFilter, &QComboBox::currentIndexChanged, this, [this](int) { refresh(); });
    connect(_ui->thumbnails, &QCheckBox::toggled, this, [this](bool) {
        applyThumbnailMode();
        refresh();
    });
    connect(_ui->cityChooser, &QComboBox::currentIndexChanged, this,
            [this](int) { refreshCity(); });
    applyThumbnailMode();

    refresh();
}

LevelBrowserPanel::~LevelBrowserPanel() = default;

void LevelBrowserPanel::refresh() {
    _model->clear();
    const int wantedState = _ui->stateFilter->currentData().toInt();
    const LevelFileOperations ops(_dir);
    for (const std::filesystem::path& path : ops.list()) {
        // L'identifiant de carte, sous-dossier compris : `capital/martpart` (`LOT-96`).
        const std::string mapId = core::mapIdOf(_dir, path);
        // Où en est la carte (LOT-EDITOR-09) : une note d'auteur, dans son annexe.
        const MapState state = readSidecar(sidecarPath(path)).sidecar.state;
        if (wantedState >= 0 && static_cast<int>(state) != wantedState) {
            continue;
        }
        auto* const item = new QStandardItem(QString::fromStdString(mapId));
        item->setEditable(false);
        item->setData(QString::fromStdString(path.string()), PATH_ROLE);
        item->setToolTip(
            QStringLiteral("%1\nState: %2")
                .arg(QString::fromStdString(mapId),
                     QString::fromUtf8(mapStateLabel(state).data(),
                                       static_cast<int>(mapStateLabel(state).size()))));
        if (state != MapState::Unset) {
            item->setText(QStringLiteral("%1 — %2").arg(
                QString::fromStdString(mapId),
                QString::fromUtf8(mapStateLabel(state).data(),
                                  static_cast<int>(mapStateLabel(state).size()))));
        }
        if (_ui->thumbnails->isChecked()) {
            item->setIcon(QIcon(thumbnailFor(path)));
        }
        _model->appendRow(item);
    }
    _model->sort(0);
    refreshWorldGraph();
    refreshCity();
}

void LevelBrowserPanel::applyThumbnailMode() {
    const bool icons = _ui->thumbnails->isChecked();
    _ui->levelList->setViewMode(icons ? QListView::IconMode : QListView::ListMode);
    _ui->levelList->setIconSize(icons ? QSize(THUMBNAIL_SIDE, THUMBNAIL_SIDE) : QSize());
    const QSize cell(THUMBNAIL_SIDE + THUMBNAIL_MARGIN_X, THUMBNAIL_SIDE + THUMBNAIL_MARGIN_Y);
    _ui->levelList->setGridSize(icons ? cell : QSize());
    _ui->levelList->setResizeMode(QListView::Adjust);
    _ui->levelList->setMovement(QListView::Static);
    _ui->levelList->setWordWrap(icons);
}

QPixmap LevelBrowserPanel::thumbnailFor(const std::filesystem::path& path) {
    // La clé porte l'horodatage du fichier : une carte récrite perd sa vignette, les autres la
    // gardent (le rendu d'une grande carte coûte plus qu'un aller-retour au disque).
    std::error_code error;
    const auto written = std::filesystem::last_write_time(path, error);
    const std::string key =
        path.string() + "@" + std::to_string(error ? 0 : written.time_since_epoch().count());
    if (const auto found = _thumbnails.find(key); found != _thumbnails.end()) {
        return found->second;
    }
    QPixmap thumbnail;
    const core::LevelLoadResult level = core::LevelLoader::loadFromFile(path);
    if (level.ok()) {
        const QImage rendered = renderMap(*level.level, _dir.parent_path(),
                                          MapRenderOptions{.bands = {},
                                                           .scale = THUMBNAIL_SCALE,
                                                           .maxSide = 2 * THUMBNAIL_SIDE,
                                                           .canvas = std::nullopt});
        if (!rendered.isNull()) {
            thumbnail = QPixmap::fromImage(rendered.scaled(
                THUMBNAIL_SIDE, THUMBNAIL_SIDE, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
    _thumbnails.emplace(key, thumbnail);
    return thumbnail;
}

void LevelBrowserPanel::refreshCity() {
    // Les villes jouables du projet : celle qu'on regarde est celle du choix, la première sinon.
    const std::filesystem::path dataRoot = _dir.parent_path();
    const std::vector<std::string> cities = cityIds(dataRoot);
    if (_ui->cityChooser->count() != static_cast<int>(cities.size())) {
        const QSignalBlocker blocker(_ui->cityChooser);
        _ui->cityChooser->clear();
        for (const std::string& city : cities) {
            _ui->cityChooser->addItem(QString::fromStdString(city));
        }
    }
    const int chosen = _ui->cityChooser->currentIndex();
    if (chosen < 0 || std::cmp_greater_equal(chosen, cities.size())) {
        _ui->city->setCity(CityView{}, dataRoot);
        return;
    }
    _ui->city->setCity(buildCityView(dataRoot, cities[static_cast<std::size_t>(chosen)]), dataRoot);
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

namespace {

// Le chemin d'un lieu pour l'œil : « Central Empire › Capital › Arenarea » (LOT-124).
[[nodiscard]] std::string placeTreeLabel(std::string_view place) {
    std::string label;
    std::size_t start = 0;
    while (start <= place.size()) {
        const std::size_t slash = place.find('/', start);
        const std::string_view segment = place.substr(
            start, slash == std::string_view::npos ? std::string_view::npos : slash - start);
        if (!label.empty()) {
            label += " › ";
        }
        label += core::scenePlaceLabel(segment);
        if (slash == std::string_view::npos) {
            break;
        }
        start = slash + 1;
    }
    return label;
}

}  // namespace

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
    // L'arbre des lieux (LOT-124) : chaque zone et sous-zone, sous son chemin lisible
    // (« Central Empire › Capital › Arenarea »). La carte se range sous le même chemin.
    auto* const placeCombo = new QComboBox(&dialog);
    for (const std::string& place : scenePlaces(_dir.parent_path())) {
        placeCombo->addItem(QString::fromStdString(placeTreeLabel(place)),
                            QString::fromStdString(place));
        placeCombo->setItemData(placeCombo->count() - 1, QString::fromStdString(place),
                                Qt::ToolTipRole);
    }
    placeCombo->addItem(QStringLiteral("(none: colored tile types)"), QString());
    auto* const whereLabel = new QLabel(&dialog);
    // Le modèle (LOT-EDITOR-08) : ses couches, son tampon, son entrée. Le choisir reprend sa
    // taille ; l'auteur peut encore l'agrandir, et ce que le modèle ne couvre pas reste plein. Les
    // modèles proposés sont ceux du lieu et de ses niveaux communs (LOT-124).
    std::vector<MapTemplate> models;
    auto* const templateCombo = new QComboBox(&dialog);
    const auto fillTemplates = [&] {
        const std::string place = placeCombo->currentData().toString().toStdString();
        models = mapTemplates(_dir.parent_path(), place);
        const QSignalBlocker blocker(templateCombo);
        templateCombo->clear();
        templateCombo->addItem(QStringLiteral("(none: an empty map)"), -1);
        for (std::size_t index = 0; index < models.size(); ++index) {
            templateCombo->addItem(QString::fromStdString(models[index].label),
                                   static_cast<int>(index));
            templateCombo->setItemData(templateCombo->count() - 1,
                                       QString::fromStdString(models[index].description),
                                       Qt::ToolTipRole);
        }
        // Où ira la carte : sous le chemin de son lieu, nommée comme lui par défaut.
        const std::string folder = levelFolderOf(place);
        if (!nameEdit->isModified()) {
            nameEdit->setText(QString::fromStdString(core::isFlatScenePlace(place)
                                                         ? std::string{}
                                                         : place.substr(place.rfind('/') + 1)));
        }
        whereLabel->setText(
            QStringLiteral("Levels/%1")
                .arg(QString::fromStdString(folder.empty() ? std::string{} : folder + "/")));
    };
    fillTemplates();
    connect(placeCombo, &QComboBox::currentIndexChanged, &dialog, [&](int) { fillTemplates(); });
    connect(templateCombo, &QComboBox::currentIndexChanged, &dialog, [&](int) {
        const int chosen = templateCombo->currentData().toInt();
        if (chosen >= 0 && std::cmp_less(chosen, models.size())) {
            widthSpin->setValue(models[static_cast<std::size_t>(chosen)].width);
            heightSpin->setValue(models[static_cast<std::size_t>(chosen)].height);
        }
    });
    auto* const buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    auto* const form = new QFormLayout(&dialog);
    form->addRow(QStringLiteral("Map name"), nameEdit);
    form->addRow(QStringLiteral("Width (cells)"), widthSpin);
    form->addRow(QStringLiteral("Height (cells)"), heightSpin);
    form->addRow(QStringLiteral("Place (piece sheet)"), placeCombo);
    form->addRow(QStringLiteral("Saved under"), whereLabel);
    form->addRow(QStringLiteral("Template"), templateCombo);
    form->addRow(buttons);
    if (dialog.exec() != QDialog::Accepted || nameEdit->text().isEmpty()) {
        return;
    }
    const QString name = nameEdit->text();
    const LevelFileOperations ops(_dir);
    const int chosen = templateCombo->currentData().toInt();
    const MapTemplate* const model = chosen >= 0 && std::cmp_less(chosen, models.size())
                                         ? &models[static_cast<std::size_t>(chosen)]
                                         : nullptr;
    const FileOperationResult result =
        ops.create(name.toStdString(), widthSpin->value(), heightSpin->value(),
                   placeCombo->currentData().toString().toStdString(), model);
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
