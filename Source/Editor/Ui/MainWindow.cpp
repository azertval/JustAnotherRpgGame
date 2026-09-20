// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFontMetrics>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QRect>
#include <QScreen>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QString>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <variant>

#include "Core/World/EntityKinds.h"
#include "Editor/Logic/Autosave.h"
#include "Editor/Logic/DataRoot.h"
#include "Editor/Logic/DiskGuard.h"
#include "Editor/Logic/EditorStatus.h"
#include "Editor/Logic/EntityReferences.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/MapRefactor.h"
#include "Editor/Logic/PieceCatalog.h"
#include "Editor/Ui/EditorActions.h"
#include "Editor/Ui/EditorViewport.h"
#include "Editor/Ui/EntityPanel.h"
#include "Editor/Ui/LayersPanel.h"
#include "Editor/Ui/LevelBrowserPanel.h"
#include "Editor/Ui/MiniMap.h"
#include "Editor/Ui/PalettePanel.h"
#include "Editor/Ui/ProblemsPanel.h"
#include "Editor/Ui/RefactorDialogs.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/CrashDump.h"

namespace hmi {

namespace {

// Version de la disposition sérialisée : à incrémenter si l'ensemble des docks change, pour
// invalider proprement une disposition sauvegardée devenue incompatible (`restoreState`).
constexpr int LAYOUT_VERSION = 13;  // 13 : le panneau « Problems » (LOT-EDITOR-07)

// Clés de persistance (portée application ; l'organisation/appli sont fixées dans `main`).
constexpr const char* GEOMETRY_KEY = "mainWindow/geometry";
constexpr const char* STATE_KEY = "mainWindow/state";
// Réglage de mise en avant automatique des panneaux.
constexpr const char* FOLLOW_ACTIVE_TOOL_KEY = "panels/followActiveTool";

// Taille maximale d'une carte dans la boîte « Resize ».
constexpr int MAXIMUM_MAP_SIDE = 100;

// Délai entre le dernier geste et la sauvegarde automatique : une rafale de coups de pinceau
// n'écrit qu'une fois, et un plantage ne perd au plus que ces deux secondes.
constexpr int AUTOSAVE_DELAY_MS = 2000;
// Délai avant de relire un fichier signalé changé : un script qui l'écrit en plusieurs fois a fini.
constexpr int DISK_CHECK_DELAY_MS = 300;
// Durée d'un message de la barre d'état après une commande de renommage ou de remplacement.
constexpr int REFACTOR_STATUS_TIMEOUT_MS = 5000;

// Dossier des brouillons de reprise, sur le poste et hors du dépôt :
// %LOCALAPPDATA%/JustAnotherRpgGame/Editor/autosave (organisation et application posées par main).
[[nodiscard]] std::filesystem::path autosaveDirectory() {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    return std::filesystem::path(base.toStdWString()) / "autosave";
}

// Horodatage des copies mises de côté.
[[nodiscard]] std::string timestamp() {
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")).toStdString();
}

[[nodiscard]] QString displayPath(const std::filesystem::path& path) {
    return QString::fromStdWString(path.wstring());
}

[[nodiscard]] std::optional<std::string> readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

}  // namespace

MainWindow::MainWindow(bool crashAfterAutosave)
    : _viewport(new EditorViewport()),
      _editContext(_viewport),
      _crashAfterAutosave(crashAfterAutosave) {
    // Le dossier des données dans le titre : on sait où l'enregistrement écrit (LOT-EDITOR-06).
    setWindowTitle(QStringLiteral("Just Another RPG Game — Editor — %1")
                       .arg(QString::fromStdWString(hmi::editorDataRoot().wstring())));
    setDockNestingEnabled(true);

    _viewport->setMinimumSize(320, 240);
    _viewport->setFocusPolicy(Qt::StrongFocus);
    setCentralWidget(_viewport);

    buildUi();

    // La palette arme le pinceau du canevas : un type ou une pièce du lieu.
    connect(_palette, &PalettePanel::tileSelected, this, [this](core::TileType type) {
        _viewport->setActiveTile(type);
        refreshStatusHelp();
    });
    connect(_palette, &PalettePanel::pieceSelected, this, [this](const QString& piece, bool floor) {
        _viewport->setActivePiece(piece.toStdString(), floor);
        refreshStatusHelp();
    });
    // La pipette a pris un pinceau : la palette le montre, sans le réémettre.
    connect(_viewport, &EditorViewport::brushPicked, this, [this](const CanvasBrush& brush) {
        if (brush.kind == BrushKind::Piece) {
            _palette->showPiece(QString::fromStdString(brush.piece), brush.floor);
        } else if (brush.kind == BrushKind::Type) {
            _palette->showTile(brush.type);
        }
        refreshStatusHelp();
    });
    connect(_viewport, &EditorViewport::toolStateChanged, this, [this] { refreshStatusHelp(); });
    // L'outil Note : le texte se saisit dans une boîte, la note s'écrit dans l'annexe de la carte.
    connect(_viewport, &EditorViewport::noteRequested, this, [this](core::GridPosition cell) {
        const AuthorNote* const note = noteAt(_viewport->sidecar(), cell);
        bool accepted = false;
        const QString text = QInputDialog::getMultiLineText(
            this, QStringLiteral("Author note"),
            QStringLiteral("Note on (%1, %2) — empty removes it:").arg(cell.column).arg(cell.row),
            note != nullptr ? QString::fromStdString(note->text) : QString{}, &accepted);
        if (accepted) {
            _viewport->setNote(cell, text.toStdString());
        }
    });
    // Le catalogue suit la carte : son lieu, et les pièces qu'elle cite sans que la planche les
    // ait.
    const auto refreshPalette = [this] {
        _palette->setPieceCatalog(_viewport->pieceCatalog(), _viewport->placeDirectory());
    };
    connect(_viewport, &EditorViewport::draftChanged, this, refreshPalette);
    refreshPalette();
    // Le canevas change d'outil de lui-même (une famille d'entité choisie arme l'outil Entité) :
    // la barre d'outils suit, sans reboucler (setActiveTool n'émet rien).
    connect(_viewport, &EditorViewport::toolChanged, _actions, &EditorActions::setActiveTool);
    // Les messages d'état du canevas (enregistrement, essai, erreurs) s'affichent en bas, puis
    // laissent la main à l'aide contextuelle.
    connect(_viewport, &EditorViewport::statusMessage, this,
            [this](const QString& message) { showTransientStatusMessage(message, 5000); });
    connect(_viewport, &EditorViewport::toolChanged, this,
            [this](hmi::EditorTool) { refreshStatusHelp(); });
    connect(_viewport, &EditorViewport::toolChanged, this, &MainWindow::applyPanelFocus);
    connect(_viewport, &EditorViewport::hoveredCellChanged, this,
            [this](std::optional<core::GridPosition>) { refreshStatusHelp(); });
    connect(_viewport, &EditorViewport::zoomChanged, this, [this](float) { refreshStatusHelp(); });
    connect(_viewport, &EditorViewport::draftChanged, this, [this] { refreshStatusHelp(); });
    // Ouvrir une carte depuis le panneau : garde-fou des modifications non enregistrées d'abord.
    connect(_levels, &LevelBrowserPanel::levelOpenRequested, this, [this](const QString& path) {
        openLevelGuarded(std::filesystem::path(path.toStdString()));
    });
    connect(_levels, &LevelBrowserPanel::mapRenameRequested, this,
            [this](const QString& mapId) { renameMap(mapId.toStdString()); });
    // Les constats : un nouveau contrôle à la demande, et le chemin vers chacun.
    connect(_problems, &ProblemsPanel::checkRequested, this, &MainWindow::runContentCheck);
    connect(_problems, &ProblemsPanel::findingActivated, this, &MainWindow::goToFinding);

    connectMapPanels();
    reloadEditorReferences();

    resize(1280, 720);
    // La palette des pièces est l'outil qu'on regarde le plus : elle prend la hauteur à gauche.
    resizeDocks({dockFor(PanelId::Palette), dockFor(PanelId::Layers)}, {440, 180}, Qt::Vertical);
    resizeDocks({dockFor(PanelId::Palette)}, {260}, Qt::Horizontal);
    refreshStatusHelp();

    // Capture la disposition par défaut (après création des docks, avant restauration d'une
    // éventuelle disposition sauvegardée) : sert de cible à « Reset layout ».
    _defaultState = saveState(LAYOUT_VERSION);
    restoreLayout();

    setUpSafetyNet();
    // Le bilan de toutes les cartes, dès que la fenêtre est montrée.
    QTimer::singleShot(0, this, &MainWindow::runContentCheck);
    // Le canevas prend le clavier au lancement : les raccourcis à une touche (P, F8, F9) marchent
    // tout de suite, au lieu d'aller à la recherche au clavier de la palette.
    _viewport->setFocus();
}

MainWindow::~MainWindow() = default;

QDockWidget* MainWindow::addPanel(const QString& objectName, const QString& title, QWidget* content,
                                  Qt::DockWidgetArea area) {
    auto* const dock = new QDockWidget(title, this);
    // objectName stable : c'est la clé de la disposition persistée (`saveState`).
    dock->setObjectName(objectName);
    dock->setWidget(content);
    addDockWidget(area, dock);
    return dock;
}

void MainWindow::buildUi() {
    // Outils et commandes : une action unique par commande, partagée entre la barre d'outils, le
    // menu et son raccourci.
    _actions = new EditorActions(this);
    _actions->applyShortcuts(_viewport->editorBindings());
    _toolBar = addToolBar(QStringLiteral("Tools"));
    _toolBar->setObjectName(QStringLiteral("EditorToolBar"));
    _toolBar->setMovable(false);
    _toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    _actions->populateToolBar(*_toolBar);

    _palette = new PalettePanel;
    _levels = new LevelBrowserPanel(hmi::editorDataRoot() / "Levels");
    _layers = new LayersPanel;
    _entities = new EntityPanel;
    _docks = {
        addPanel(QStringLiteral("PalettePanel"), QStringLiteral("Palette"), _palette,
                 Qt::LeftDockWidgetArea),
        addPanel(QStringLiteral("LevelsPanel"), QStringLiteral("Maps"), _levels,
                 Qt::RightDockWidgetArea),
        addPanel(QStringLiteral("LayersPanel"), QStringLiteral("Layers"), _layers,
                 Qt::LeftDockWidgetArea),
        addPanel(QStringLiteral("EntitiesPanel"), QStringLiteral("Entities"), _entities,
                 Qt::RightDockWidgetArea),
    };

    // La mini-carte : toute la carte, et le cadre de la vue (LOT-EDITOR-02, phase 3).
    _miniMap = new MiniMap([this](core::TileType type) { return _viewport->tileColor(type); });
    _miniMapDock = addPanel(QStringLiteral("MiniMapPanel"), QStringLiteral("Overview"), _miniMap,
                            Qt::LeftDockWidgetArea);

    // Les constats du contrôle, sous le canevas : une liste large, peu haute (LOT-EDITOR-07).
    _problems = new ProblemsPanel;
    _problemsDock = addPanel(QStringLiteral("ProblemsPanel"), QStringLiteral("Problems"), _problems,
                             Qt::BottomDockWidgetArea);

    // Cartes et Entités partagent une pile d'onglets par défaut ; chacun reste déplaçable,
    // détachable et refermable. Doit précéder la capture de _defaultState.
    QDockWidget* const levelsDock = dockFor(PanelId::Levels);
    QDockWidget* const entitiesDock = dockFor(PanelId::Entities);
    tabifyDockWidget(levelsDock, entitiesDock);
    // Un changement de visibilité non provoqué par notre propre code ne peut venir que d'un choix
    // explicite de l'utilisateur : cliquer un onglet, fermer ou détacher le panneau.
    for (QDockWidget* const dock : {levelsDock, entitiesDock}) {
        connect(dock, &QDockWidget::visibilityChanged, this, [this](bool) {
            if (!_suppressPanelFocusTracking) {
                _userPickedTab = true;
            }
        });
        connect(dock, &QDockWidget::topLevelChanged, this, [this](bool) { _userPickedTab = true; });
    }

    buildMenus();
    connectToolActions();
    connectEditorCommands();
    buildStatusBar();
}

void MainWindow::buildMenus() {
    // Menus PAR NATURE D'ACTION ; les commandes sont les mêmes QAction que la barre d'outils.
    QMenu* const fileMenu = menuBar()->addMenu(QStringLiteral("&File"));
    fileMenu->addAction(_actions->action(EditorCommand::Save));
    fileMenu->addAction(_actions->action(EditorCommand::Rename));
    _resizeAction = fileMenu->addAction(QStringLiteral("Resize…"));
    fileMenu->addSeparator();
    QAction* const quit = fileMenu->addAction(QStringLiteral("Quit"));
    connect(quit, &QAction::triggered, this, &MainWindow::close);

    QMenu* const editMenu = menuBar()->addMenu(QStringLiteral("&Edit"));
    editMenu->addAction(_actions->action(EditorCommand::Undo));
    editMenu->addAction(_actions->action(EditorCommand::Redo));
    editMenu->addSeparator();
    editMenu->addAction(_actions->action(EditorCommand::Copy));
    editMenu->addAction(_actions->action(EditorCommand::Paste));

    QMenu* const toolsMenu = menuBar()->addMenu(QStringLiteral("&Tools"));
    for (QAction* const act : _actions->all()) {
        if (act->isCheckable() && act->actionGroup() != nullptr) {
            toolsMenu->addAction(act);
        }
    }
    toolsMenu->addSeparator();
    toolsMenu->addAction(_actions->action(EditorCommand::Mirror));

    QMenu* const mapMenu = menuBar()->addMenu(QStringLiteral("&Map"));
    mapMenu->addAction(_actions->action(EditorCommand::Playtest));
    mapMenu->addAction(_actions->action(EditorCommand::PlaytestHere));
    mapMenu->addSeparator();
    QAction* const checkAll = mapMenu->addAction(QStringLiteral("Check all maps"));
    connect(checkAll, &QAction::triggered, this, [this] {
        runContentCheck();
        _problemsDock->show();
        _problemsDock->raise();
    });
    mapMenu->addSeparator();
    buildRefactorMenu(mapMenu);

    QMenu* const viewMenu = menuBar()->addMenu(QStringLiteral("&View"));
    viewMenu->addAction(_actions->action(EditorCommand::IsoView));
    viewMenu->addAction(_actions->action(EditorCommand::SeeThroughRelief));
    viewMenu->addAction(_actions->action(EditorCommand::ResetCamera));
    viewMenu->addAction(_actions->action(EditorCommand::ToggleGrid));
    viewMenu->addSeparator();
    QMenu* const panelsMenu = viewMenu->addMenu(QStringLiteral("Panels"));
    for (QDockWidget* const dock : _docks) {
        panelsMenu->addAction(dock->toggleViewAction());
    }
    panelsMenu->addAction(_miniMapDock->toggleViewAction());
    panelsMenu->addAction(_problemsDock->toggleViewAction());
    panelsMenu->addSeparator();
    // Mise en avant automatique du panneau de l'outil actif : persistée, active par défaut.
    _actFollowActiveTool = panelsMenu->addAction(QStringLiteral("Follow active tool"));
    _actFollowActiveTool->setCheckable(true);
    _actFollowActiveTool->setChecked(
        QSettings().value(QString::fromLatin1(FOLLOW_ACTIVE_TOOL_KEY), true).toBool());
    connect(_actFollowActiveTool, &QAction::toggled, this, [](bool enabled) {
        QSettings().setValue(QString::fromLatin1(FOLLOW_ACTIVE_TOOL_KEY), enabled);
    });
    _resetLayoutAction = panelsMenu->addAction(QStringLiteral("Reset layout"));

    QMenu* const helpMenu = menuBar()->addMenu(QStringLiteral("&Help"));
    helpMenu->addAction(_actions->action(EditorCommand::ShortcutsOverview));
}

void MainWindow::connectMapPanels() {
    const auto refreshLayers = [this] {
        _layers->refresh(_viewport->draft(), _viewport->activeLayer(), _viewport->layerView());
    };
    const auto refreshEntities = [this] {
        // Le verdict de la zone de combat principale, s'il y en a une (LOT-EDITOR-05).
        std::string verdict;
        if (const std::optional<std::size_t> selected = _viewport->selectedEntity()) {
            for (const core::CombatZoneTerrain& zone : _viewport->combatZones()) {
                if (zone.entityIndex == *selected) {
                    verdict = hmi::combatZoneSummary(zone);
                }
            }
        }
        _entities->refresh(_viewport->draft(), _viewport->selectedEntities(),
                           _viewport->selectedEntity(), _viewport->entityReferenceContext(),
                           _viewport->diagnostics(), verdict);
    };
    connect(_viewport, &EditorViewport::draftChanged, this, [refreshLayers, refreshEntities] {
        refreshLayers();
        refreshEntities();
    });
    connect(_viewport, &EditorViewport::activeLayerChanged, this,
            [refreshLayers](hmi::LayerSlot) { refreshLayers(); });
    connect(_viewport, &EditorViewport::layerViewChanged, this, refreshLayers);
    connect(_viewport, &EditorViewport::entitySelectionChanged, this,
            [refreshEntities](std::optional<std::size_t>) { refreshEntities(); });

    // Couches : le panneau demande, le canevas applique -- l'historique pour la structure, une
    // simple aide d'édition pour la visibilité et l'opacité.
    connect(_layers, &LayersPanel::activeLayerRequested, _viewport,
            &EditorViewport::setActiveLayer);
    connect(_layers, &LayersPanel::visibilityRequested, _viewport,
            &EditorViewport::setMapLayerVisible);
    connect(_layers, &LayersPanel::opacityRequested, _viewport,
            &EditorViewport::setMapLayerOpacity);
    connect(_layers, &LayersPanel::dimRequested, _viewport, &EditorViewport::setMapLayerDimmed);
    connect(_layers, &LayersPanel::lockRequested, _viewport, &EditorViewport::setMapLayerLocked);

    // Mini-carte : l'image suit le brouillon, le cadre suit la vue, un clic ramène la vue.
    const auto refreshMiniMapFrame = [this] {
        _miniMap->setVisibleCorners(_viewport->visibleGridCorners());
    };
    connect(_viewport, &EditorViewport::draftChanged, this, [this, refreshMiniMapFrame] {
        _miniMap->setDraft(_viewport->draft());
        refreshMiniMapFrame();
    });
    connect(_viewport, &EditorViewport::framingChanged, this, refreshMiniMapFrame);
    connect(_miniMap, &MiniMap::centerRequested, _viewport, &EditorViewport::centerOnGridPoint);
    _miniMap->setDraft(_viewport->draft());
    connect(_layers, &LayersPanel::addRequested, this, [this](core::LayerKind kind) {
        _viewport->addMapLayer(kind, kind == core::LayerKind::Decor ? "decor" : "ground");
    });
    connect(_layers, &LayersPanel::removeRequested, this, [this](std::size_t index) {
        if (index >= _viewport->draft().layers().size()) {
            return;
        }
        const QString name = QString::fromStdString(_viewport->draft().layers()[index].name);
        if (QMessageBox::question(
                this, QStringLiteral("Remove layer"),
                QStringLiteral("Remove layer \"%1\"? Ctrl+Z undoes it.").arg(name)) !=
            QMessageBox::Yes) {
            return;
        }
        _viewport->removeMapLayer(index);
    });
    connect(_layers, &LayersPanel::moveRequested, _viewport, &EditorViewport::moveMapLayer);
    connect(_layers, &LayersPanel::renameRequested, this,
            [this](std::size_t index, const QString& name) {
                _viewport->renameMapLayer(index, name.toStdString());
            });

    // Entités : choisir une famille à poser arme l'outil Entité.
    connect(_entities, &EntityPanel::kindToPlaceChanged, this, [this](const QString& type) {
        _viewport->setEntityKindToPlace(type.toStdString());
        if (!type.isEmpty()) {
            _viewport->setTool(hmi::EditorTool::Entity);
        }
    });
    connect(_entities, &EntityPanel::entitySelected, _viewport, &EditorViewport::selectEntity);
    connect(_entities, &EntityPanel::entitiesSelected, this,
            [this](const std::vector<std::size_t>& indices, std::optional<std::size_t> primary) {
                _viewport->setEntitySelection(indices, primary);
            });
    connect(_entities, &EntityPanel::propertyChanged, this,
            [this](std::size_t index, const QString& key, const core::PropertyValue& value) {
                _viewport->setEntityProperty(index, key.toStdString(), value);
            });
    connect(_entities, &EntityPanel::removeRequested, _viewport,
            &EditorViewport::removeSelectedEntities);

    refreshLayers();  // état initial (avant tout draftChanged).
    refreshEntities();
}

bool MainWindow::openLevelGuarded(const std::filesystem::path& path) {
    if (_viewport->isDirty()) {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this, QStringLiteral("Unsaved changes"),
            QStringLiteral("The current map has unsaved changes. Discard them and open the "
                           "other map?"));
        if (answer != QMessageBox::Yes) {
            return false;
        }
    }
    return _viewport->openLevel(path);
}

bool MainWindow::saveMap() {
    // La carte a pu changer sur disque : jamais d'écrasement en silence. Si l'auteur choisit la
    // version du disque, il n'y a plus rien à enregistrer.
    if (!checkDiskChange() || !_viewport->save()) {
        return false;
    }
    // Enregistrée : le fichier de reprise n'a plus d'objet, tout de suite.
    _autosaveTimer->stop();
    writeAutosave();
    // Une carte enregistrée peut avoir changé ses points d'arrivée ou son nom : les portails
    // des AUTRES cartes se valident contre le fichier, et le graphe du monde le montre.
    reloadEditorReferences();
    _levels->refreshWorldGraph();
    runContentCheck();  // les autres cartes peuvent dépendre de celle-ci (portails, retours).
    return true;
}

// --- Renommer et remplacer (LOT-EDITOR-14) ---------------------------------------------------

namespace {

// Le nom du point d'arrivée que @p entity déclare ; vide si ce n'en est pas un.
std::string arrivalNameOf(const core::MapEntity& entity) {
    const auto found = entity.properties.find(std::string{core::SPAWN_POINT_NAME_PROPERTY});
    const std::string* name =
        found != entity.properties.end() ? std::get_if<std::string>(&found->second) : nullptr;
    return entity.type == core::SPAWN_POINT_ENTITY_TYPE && name != nullptr ? *name : std::string{};
}

// Les pièces que la carte pose sur ses couches visuelles, sans doublon, triées.
std::vector<std::string> citedPiecesOf(const std::vector<core::TileLayer>& layers) {
    std::vector<std::string> cited;
    for (const core::TileLayer& layer : layers) {
        if (!core::isVisualLayerKind(layer.kind)) {
            continue;
        }
        for (const std::string& piece : layer.pieces) {
            if (!piece.empty() && std::ranges::find(cited, piece) == cited.end()) {
                cited.push_back(piece);
            }
        }
    }
    std::ranges::sort(cited);
    return cited;
}

}  // namespace

const core::MapEntity* MainWindow::selectedMapEntity() const {
    const std::optional<std::size_t> index = _viewport->selectedEntity();
    const std::vector<core::MapEntity>& entities = _viewport->draft().entities();
    return index && *index < entities.size() ? &entities[*index] : nullptr;
}

void MainWindow::showCitationsOf(const QString& title, const std::vector<Citation>& citations) {
    if (const std::optional<Citation> chosen =
            showCitations(this, title, citations, editorDataRoot())) {
        goToCitation(*chosen);
    }
}

void MainWindow::citeSelectedEntity() {
    const core::MapEntity* entity = selectedMapEntity();
    if (entity == nullptr) {
        showTransientStatusMessage(QStringLiteral("Select an entity first."),
                                   REFACTOR_STATUS_TIMEOUT_MS);
        return;
    }
    const std::filesystem::path& root = editorDataRoot();
    // Un point d'arrivée est cité par son nom ; toute entité, par `carte#id`.
    std::vector<Citation> citations = citationsOfEntity(root, _viewport->mapId(), entity->id);
    if (const std::string name = arrivalNameOf(*entity); !name.empty()) {
        const std::vector<Citation> byName = citationsOfArrival(root, _viewport->mapId(), name);
        citations.insert(citations.end(), byName.begin(), byName.end());
    }
    showCitationsOf(QStringLiteral("Who cites %1")
                        .arg(QString::fromStdString(entityRef(_viewport->mapId(), entity->id))),
                    citations);
}

void MainWindow::renameSelectedEntityId() {
    const core::MapEntity* entity = selectedMapEntity();
    if (entity == nullptr) {
        showTransientStatusMessage(QStringLiteral("Select an entity first."),
                                   REFACTOR_STATUS_TIMEOUT_MS);
        return;
    }
    const std::string oldId = entity->id;
    if (!saveBeforeRefactor()) {
        return;
    }
    bool accepted = false;
    const QString newId =
        QInputDialog::getText(this, QStringLiteral("Rename entity id"), QStringLiteral("New id:"),
                              QLineEdit::Normal, QString::fromStdString(oldId), &accepted);
    if (accepted && !newId.isEmpty()) {
        carryOutPlan(
            planRenameEntityId(editorDataRoot(), _viewport->mapId(), oldId, newId.toStdString()),
            QStringLiteral("Rename entity id"), _viewport->mapId());
    }
}

void MainWindow::renameSelectedArrival() {
    const core::MapEntity* entity = selectedMapEntity();
    const std::string oldName = entity != nullptr ? arrivalNameOf(*entity) : std::string{};
    if (oldName.empty()) {
        showTransientStatusMessage(QStringLiteral("Select an arrival point first."),
                                   REFACTOR_STATUS_TIMEOUT_MS);
        return;
    }
    if (!saveBeforeRefactor()) {
        return;
    }
    bool accepted = false;
    const QString newName = QInputDialog::getText(this, QStringLiteral("Rename arrival point"),
                                                  QStringLiteral("New name:"), QLineEdit::Normal,
                                                  QString::fromStdString(oldName), &accepted);
    if (accepted && !newName.isEmpty()) {
        carryOutPlan(
            planRenameArrival(editorDataRoot(), _viewport->mapId(), oldName, newName.toStdString()),
            QStringLiteral("Rename arrival point"), _viewport->mapId());
    }
}

void MainWindow::replacePieceOnMaps() {
    const core::ScenePieceManifest* sheet = _viewport->draft().pieceManifest();
    if (sheet == nullptr) {
        showTransientStatusMessage(QStringLiteral("This map has no sheet."),
                                   REFACTOR_STATUS_TIMEOUT_MS);
        return;
    }
    // Les pièces que la carte pose : celles de la palette, plus celles qui manquent.
    const std::optional<PieceReplacementChoice> choice =
        askPieceReplacement(this, citedPiecesOf(_viewport->draft().layers()), *sheet);
    if (!choice) {
        return;
    }
    if (!choice->allMaps) {
        if (!_viewport->replacePieces({{choice->from, choice->to}})) {
            showTransientStatusMessage(
                QStringLiteral("Nothing replaced: the piece would overflow the map."),
                REFACTOR_STATUS_TIMEOUT_MS);
        }
        return;
    }
    if (saveBeforeRefactor()) {
        carryOutPlan(planReplacePiece(editorDataRoot(), choice->from, choice->to, {}),
                     QStringLiteral("Replace piece"), _viewport->mapId());
    }
}

void MainWindow::buildRefactorMenu(QMenu* mapMenu) {
    connect(mapMenu->addAction(QStringLiteral("Who cites this map?")), &QAction::triggered, this,
            [this] {
                showCitationsOf(
                    QStringLiteral("Who cites %1").arg(QString::fromStdString(_viewport->mapId())),
                    citationsOfMap(editorDataRoot(), _viewport->mapId()));
            });
    connect(mapMenu->addAction(QStringLiteral("Who cites the selected entity?")),
            &QAction::triggered, this, [this] { citeSelectedEntity(); });
    connect(mapMenu->addAction(QStringLiteral("Rename entity id…")), &QAction::triggered, this,
            [this] { renameSelectedEntityId(); });
    connect(mapMenu->addAction(QStringLiteral("Rename arrival point…")), &QAction::triggered, this,
            [this] { renameSelectedArrival(); });
    mapMenu->addSeparator();
    connect(mapMenu->addAction(QStringLiteral("Replace piece…")), &QAction::triggered, this,
            [this] { replacePieceOnMaps(); });
    connect(mapMenu->addAction(QStringLiteral("Change sheet…")), &QAction::triggered, this, [this] {
        const std::optional<SceneChangeChoice> choice =
            askSceneChange(this, _viewport->draft().layers(), editorDataRoot(),
                           scenePlaceOf(_viewport->draft().layers()));
        if (choice && !_viewport->changeScene(choice->place, choice->table)) {
            showTransientStatusMessage(
                QStringLiteral("Sheet not changed (a variant changes it with --change-scene)."),
                REFACTOR_STATUS_TIMEOUT_MS);
        }
    });
}

bool MainWindow::saveBeforeRefactor() {
    if (!_viewport->isDirty()) {
        return true;
    }
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, QStringLiteral("Save first"),
        QStringLiteral("This rewrites map files, maybe the open one. Save the open map first?"),
        QMessageBox::Save | QMessageBox::Cancel);
    return answer == QMessageBox::Save && saveMap();
}

void MainWindow::renameMap(const std::string& mapId) {
    // Même pour une autre carte : un portail de la carte ouverte peut la citer, et le plan la
    // récrit puis la rouvre.
    if (!saveBeforeRefactor()) {
        return;
    }
    bool accepted = false;
    const QString newId =
        QInputDialog::getText(this, QStringLiteral("Rename map"),
                              QStringLiteral("New id (a folder is allowed: capital/market):"),
                              QLineEdit::Normal, QString::fromStdString(mapId), &accepted);
    if (!accepted || newId.isEmpty() || newId.toStdString() == mapId) {
        return;
    }
    const std::string openAfter =
        mapId == _viewport->mapId() ? newId.toStdString() : _viewport->mapId();
    carryOutPlan(planRenameMap(editorDataRoot(), mapId, newId.toStdString()),
                 QStringLiteral("Rename map"), openAfter);
}

void MainWindow::carryOutPlan(const RefactorPlan& plan, const QString& title,
                              const std::string& openAfter) {
    const std::filesystem::path& root = editorDataRoot();
    if (!plan.ok()) {
        QMessageBox::warning(this, title, QString::fromStdString(plan.error));
        return;
    }
    if (!confirmPlan(this, title, plan, root)) {
        return;
    }
    std::string error;
    const bool written = applyRefactorPlan(plan, error);
    HMI_LOG_INFO("Editeur : " + title.toStdString() + ", " + std::to_string(plan.edits.size()) +
                 " fichiers" + (written ? std::string{"."} : " : " + error));
    if (!written) {
        QMessageBox::warning(this, title, QString::fromStdString(error));
    }
    // La carte ouverte a pu être récrite ou déplacée : on la relit, propre, là où elle est.
    const std::filesystem::path reopened = root / "Levels" / (openAfter + ".json");
    std::error_code missing;
    if (std::filesystem::exists(reopened, missing)) {
        _viewport->openLevel(reopened);
        watchLevelFile();
    }
    _levels->refresh();
    reloadEditorReferences();
    runContentCheck();
}

void MainWindow::goToCitation(const Citation& citation) {
    if (citation.mapId.empty()) {
        showTransientStatusMessage(
            QString::fromStdString(formatCitation(citation, editorDataRoot())),
            REFACTOR_STATUS_TIMEOUT_MS);
        return;
    }
    goToFinding(MapCheckFinding{.severity = MapCheckSeverity::Warning,
                                .mapId = citation.mapId,
                                .cell = citation.cell,
                                .message = citation.what,
                                .entityId = citation.entityId});
}

void MainWindow::runContentCheck() {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const MapCheckReport report = checkAllMaps(editorDataRoot());
    QApplication::restoreOverrideCursor();
    _problems->setReport(report);
    HMI_LOG_INFO("Editeur : controle de " + std::to_string(report.maps) + " cartes, " +
                 std::to_string(report.count(MapCheckSeverity::Error)) + " erreurs.");
}

void MainWindow::goToFinding(const MapCheckFinding& finding) {
    if (finding.mapId != _viewport->mapId() &&
        !openLevelGuarded(editorDataRoot() / "Levels" / (finding.mapId + ".json"))) {
        return;
    }
    std::optional<core::GridPosition> cell = finding.cell;
    if (!finding.entityId.empty()) {
        const std::vector<core::MapEntity>& entities = _viewport->draft().entities();
        const auto entity = std::ranges::find(entities, finding.entityId, &core::MapEntity::id);
        if (entity != entities.end()) {
            _viewport->selectEntity(static_cast<std::size_t>(entity - entities.begin()));
            cell = cell.value_or(entity->position);
        }
    }
    if (cell) {
        _viewport->revealCell(*cell);
    }
    _viewport->setFocus();
}

void MainWindow::reloadEditorReferences() {
    _references =
        std::make_unique<EditorReferences>(hmi::loadEditorReferences(hmi::editorDataRoot()));
    _viewport->setEditorReferences(_references.get());
}

void MainWindow::connectToolActions() {
    for (std::size_t index = 0; index < EDITOR_COMMAND_COUNT; ++index) {
        const auto command = static_cast<EditorCommand>(index);
        const std::optional<hmi::EditorTool> tool = EditorActions::toolOf(command);
        if (!tool) {
            continue;
        }
        connect(_actions->action(command), &QAction::toggled, _viewport,
                [this, tool = *tool](bool on) {
                    if (on) {
                        _viewport->setTool(tool);
                    }
                });
    }
}

void MainWindow::connectEditorCommands() {
    connect(_actions->action(EditorCommand::Save), &QAction::triggered, this,
            [this] { saveMap(); });
    connect(_actions->action(EditorCommand::Playtest), &QAction::triggered, _viewport,
            [this] { _viewport->startPlaytest(); });
    connect(_actions->action(EditorCommand::PlaytestHere), &QAction::triggered, _viewport,
            [this] { _viewport->startPlaytestHere(); });
    connect(_actions->action(EditorCommand::Mirror), &QAction::toggled, _viewport,
            [this](bool enabled) { _viewport->setMirror(enabled); });
    connect(_actions->action(EditorCommand::Undo), &QAction::triggered, this,
            [this] { _editContext->undo(); });
    connect(_actions->action(EditorCommand::Redo), &QAction::triggered, this,
            [this] { _editContext->redo(); });
    connect(_actions->action(EditorCommand::Copy), &QAction::triggered, this,
            [this] { _editContext->copy(); });
    connect(_actions->action(EditorCommand::Paste), &QAction::triggered, this,
            [this] { _editContext->paste(); });
    connect(_actions->action(EditorCommand::ToggleGrid), &QAction::triggered, _viewport,
            [this] { _viewport->toggleGrid(); });
    connect(_actions->action(EditorCommand::ResetCamera), &QAction::triggered, _viewport,
            [this] { _viewport->resetCamera(); });
    // Vue iso ou à plat (décision D1) ; l'action suit la vue si elle change autrement.
    connect(
        _actions->action(EditorCommand::IsoView), &QAction::toggled, _viewport,
        [this](bool iso) { _viewport->setCanvasView(iso ? CanvasView::Iso : CanvasView::Flat); });
    connect(_viewport, &EditorViewport::canvasViewChanged, this, [this](CanvasView view) {
        QAction* const action = _actions->action(EditorCommand::IsoView);
        const QSignalBlocker blocker(action);
        action->setChecked(view == CanvasView::Iso);
        refreshStatusHelp();
    });
    connect(_actions->action(EditorCommand::SeeThroughRelief), &QAction::toggled, _viewport,
            [this](bool enabled) { _viewport->setSeeThroughRelief(enabled); });
    // Renommer la carte ouverte : le même renommage propagé que le navigateur de cartes.
    connect(_actions->action(EditorCommand::Rename), &QAction::triggered, this,
            [this] { renameMap(_viewport->mapId()); });
    connect(_actions->action(EditorCommand::ShortcutsOverview), &QAction::triggered, this,
            [this] { openShortcutsDialog(); });
    connect(_resizeAction, &QAction::triggered, this, [this] { openResizeDialog(); });
    connect(_resetLayoutAction, &QAction::triggered, this, [this] {
        _suppressPanelFocusTracking = true;
        restoreState(_defaultState, LAYOUT_VERSION);
        _suppressPanelFocusTracking = false;
        _userPickedTab = false;  // disposition remise à neuf : la mise en avant repart.
    });
}

void MainWindow::buildStatusBar() {
    // Barre d'état structurée : zones permanentes, jamais recouvertes par un message transitoire.
    // Largeur minimale sur les zones qui changent au survol (case, zoom) : sans elle, la barre
    // « saute » à chaque déplacement de souris.
    for (QLabel*& zone : _statusZones) {
        zone = new QLabel(this);
        statusBar()->addPermanentWidget(zone);
    }
    _statusZones[3]->setMinimumWidth(
        fontMetrics().horizontalAdvance(QStringLiteral("(999, 999) square · front-right")));
    // Une note longue ne doit pas pousser les autres zones hors de la barre.
    _statusZones[3]->setMaximumWidth(fontMetrics().horizontalAdvance(QStringLiteral("M")) * 70);
    _statusZones[4]->setMinimumWidth(
        fontMetrics().horizontalAdvance(QStringLiteral("Zoom: 999% · Flat")));
    _statusMessageTimer = new QTimer(this);
    _statusMessageTimer->setSingleShot(true);
    connect(_statusMessageTimer, &QTimer::timeout, this, &MainWindow::refreshStatusHelp);
}

void MainWindow::openResizeDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Resize map"));
    auto* const widthSpin = new QSpinBox(&dialog);
    auto* const heightSpin = new QSpinBox(&dialog);
    for (QSpinBox* const spin : {widthSpin, heightSpin}) {
        spin->setRange(1, MAXIMUM_MAP_SIDE);
    }
    widthSpin->setValue(_viewport->levelWidth());
    heightSpin->setValue(_viewport->levelHeight());
    auto* const buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    auto* const form = new QFormLayout(&dialog);
    form->addRow(QStringLiteral("Width (cells)"), widthSpin);
    form->addRow(QStringLiteral("Height (cells)"), heightSpin);
    form->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    const int width = widthSpin->value();
    const int height = heightSpin->value();
    // Confirmation si le redimensionnement supprimerait du contenu déjà posé (EX-EDIT-012).
    if (_viewport->wouldResizeDrop(width, height)) {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this, QStringLiteral("Resize map"),
            QStringLiteral(
                "Shrinking to %1 × %2 will remove content (entry or entities). Continue?")
                .arg(width)
                .arg(height));
        if (answer != QMessageBox::Yes) {
            return;
        }
    }
    _viewport->resizeLevel(width, height);
}

void MainWindow::openShortcutsDialog() {
    // Lit les raccourcis EFFECTIFS des actions à l'ouverture, jamais un texte figé (EX-EDIT-015).
    // Les commandes SANS raccourci sont omises -- une ligne vide n'apprendrait rien.
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Keyboard shortcuts"));
    auto* const table = new QTableWidget(0, 2, &dialog);
    table->setHorizontalHeaderLabels({QStringLiteral("Command"), QStringLiteral("Shortcut")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setAlternatingRowColors(true);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setShowGrid(false);
    for (QAction* const act : _actions->all()) {
        if (act->shortcut().isEmpty()) {
            continue;
        }
        const int row = table->rowCount();
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(act->text()));
        table->setItem(row, 1,
                       new QTableWidgetItem(act->shortcut().toString(QKeySequence::NativeText)));
    }
    table->resizeColumnsToContents();
    auto* const buttons = new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    auto* const layout = new QVBoxLayout(&dialog);
    layout->addWidget(table);
    layout->addWidget(buttons);
    dialog.exec();
}

void MainWindow::restoreLayout() {
    const QSettings settings;
    const QByteArray geometry = settings.value(QString::fromLatin1(GEOMETRY_KEY)).toByteArray();
    const QByteArray state = settings.value(QString::fromLatin1(STATE_KEY)).toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
        // La géométrie persistée n'a jamais été bornée à l'écran (EX-IHM-081) : on la ramène dans
        // la zone utile -- taille PUIS position, dans cet ordre : déplacer une fenêtre trop grande
        // ne la ferait pas tenir.
        const QScreen* const hostScreen = screen();
        if (hostScreen != nullptr) {
            const QRect available = hostScreen->availableGeometry();
            const QSize decorations = frameGeometry().size() - size();
            const QSize fittedFrame = frameGeometry().size().boundedTo(available.size());
            if (fittedFrame != frameGeometry().size()) {
                resize(fittedFrame - decorations);
            }
            QRect placed = frameGeometry();
            placed.moveLeft(
                std::clamp(placed.left(), available.left(),
                           std::max(available.left(), available.right() - placed.width() + 1)));
            placed.moveTop(
                std::clamp(placed.top(), available.top(),
                           std::max(available.top(), available.bottom() - placed.height() + 1)));
            if (placed.topLeft() != frameGeometry().topLeft()) {
                move(placed.topLeft());
            }
        }
    }
    if (!state.isEmpty()) {
        _suppressPanelFocusTracking = true;
        restoreState(state, LAYOUT_VERSION);
        _suppressPanelFocusTracking = false;
    }
}

void MainWindow::saveLayout() {
    QSettings settings;
    settings.setValue(QString::fromLatin1(GEOMETRY_KEY), saveGeometry());
    settings.setValue(QString::fromLatin1(STATE_KEY), saveState(LAYOUT_VERSION));
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (_viewport->isDirty()) {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this, QStringLiteral("Unsaved changes"),
            QStringLiteral("Map \"%1\" has unsaved changes. Save them before closing?")
                .arg(QString::fromStdString(_viewport->mapId())),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        if (answer == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
        // Enregistrement refusé ou impossible : la fenêtre reste, le brouillon aussi. Si la garde a
        // relu le disque, le brouillon d'avant est déjà mis de côté et il n'y a rien à enregistrer.
        if (answer == QMessageBox::Save) {
            const bool draftKept = checkDiskChange();
            if ((draftKept && !_viewport->save()) || (!draftKept && _viewport->isDirty())) {
                event->ignore();
                return;
            }
        }
    }
    // Fermeture voulue : le brouillon est enregistré ou abandonné, sa reprise n'a plus d'objet.
    _autosaveTimer->stop();
    if (!_autosavedMapId.empty()) {
        _autosave->discard(_autosavedMapId);
        _autosavedMapId.clear();
    }
    saveLayout();
    QMainWindow::closeEvent(event);
}

void MainWindow::setUpSafetyNet() {
    _autosave = std::make_unique<AutosaveStore>(autosaveDirectory());
    _autosaveTimer = new QTimer(this);
    _autosaveTimer->setSingleShot(true);
    connect(_autosaveTimer, &QTimer::timeout, this, &MainWindow::writeAutosave);
    connect(_viewport, &EditorViewport::draftChanged, this, &MainWindow::scheduleAutosave);

    _watcher = new QFileSystemWatcher(this);
    _diskCheckTimer = new QTimer(this);
    _diskCheckTimer->setSingleShot(true);
    connect(_diskCheckTimer, &QTimer::timeout, this, [this] { checkDiskChange(); });
    connect(_watcher, &QFileSystemWatcher::fileChanged, this,
            [this](const QString&) { _diskCheckTimer->start(DISK_CHECK_DELAY_MS); });
    // Un changement fait pendant que la fenêtre n'avait pas la main (script, git) se voit au
    // retour.
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
            [this](Qt::ApplicationState state) {
                if (state == Qt::ApplicationActive) {
                    _diskCheckTimer->start(0);
                }
            });
    watchLevelFile();

    if (_crashAfterAutosave) {
        HMI_LOG_WARNING(
            "--crash-test : plantage volontaire apres la premiere sauvegarde automatique.");
    }
    // Après l'affichage de la fenêtre : la question de reprise s'y rattache.
    QTimer::singleShot(0, this, &MainWindow::offerRecovery);
}

void MainWindow::scheduleAutosave() {
    watchLevelFile();  // la carte ouverte a pu changer (ouverture, renommage).
    _autosaveTimer->start(AUTOSAVE_DELAY_MS);
}

void MainWindow::writeAutosave() {
    const std::string& mapId = _viewport->mapId();
    // Le fichier de reprise d'une autre carte (renommée, ou quittée en abandonnant ses
    // modifications) n'a plus d'objet.
    if (!_autosavedMapId.empty() && (_autosavedMapId != mapId || !_viewport->isDirty())) {
        _autosave->discard(_autosavedMapId);
        _autosavedMapId.clear();
    }
    if (!_viewport->isDirty()) {
        return;
    }
    const AutosaveRecord record{
        .mapId = mapId, .levelPath = _viewport->levelPath(), .draftJson = _viewport->draftJson()};
    if (!_autosave->write(record)) {
        HMI_LOG_WARNING("Editeur : sauvegarde automatique impossible dans " +
                        _autosave->directory().string());
        return;
    }
    _autosavedMapId = mapId;
    if (_crashAfterAutosave) {
        hmi::triggerCrashForTest();
    }
}

void MainWindow::offerRecovery() {
    bool recovered = false;
    for (const AutosaveRecord& record : _autosave->pending()) {
        const QString map = QString::fromStdString(record.mapId);
        bool recover = false;
        // Une seule carte ouverte à la fois : les brouillons suivants sont mis de côté.
        if (!recovered) {
            QMessageBox box(QMessageBox::Warning, QStringLiteral("Recover unsaved draft"),
                            QStringLiteral("The editor did not close normally. An unsaved draft of "
                                           "map \"%1\" was found.\n\nRecover it? If you discard "
                                           "it, it is set aside, not deleted.")
                                .arg(map),
                            QMessageBox::NoButton, this);
            QPushButton* const recoverButton =
                box.addButton(QStringLiteral("Recover"), QMessageBox::AcceptRole);
            box.addButton(QStringLiteral("Discard"), QMessageBox::DestructiveRole);
            box.setDefaultButton(recoverButton);
            box.exec();
            recover = box.clickedButton() == recoverButton;
        }
        if (recover && _viewport->restoreDraft(record.mapId, record.draftJson)) {
            recovered = true;
            _autosavedMapId = record.mapId;
            watchLevelFile();
            continue;
        }
        const std::optional<std::filesystem::path> kept =
            _autosave->keepAside(record.mapId, "draft", timestamp(), record.draftJson);
        if (!kept) {
            HMI_LOG_WARNING("Editeur : brouillon de reprise laisse en place : " + record.mapId);
            continue;  // rien n'est retiré tant qu'il n'est pas à l'abri.
        }
        _autosave->discard(record.mapId);
        HMI_LOG_INFO("Editeur : brouillon de reprise mis de cote : " + kept->string());
        if (recover) {
            QMessageBox::warning(this, QStringLiteral("Recover unsaved draft"),
                                 QStringLiteral("The draft of map \"%1\" cannot be read as a map. "
                                                "It was set aside in:\n%2")
                                     .arg(map, displayPath(*kept)));
        } else {
            showTransientStatusMessage(
                QStringLiteral("Draft of %1 set aside: %2").arg(map, displayPath(*kept)), 8000);
        }
    }
}

void MainWindow::watchLevelFile() {
    const QString path = displayPath(_viewport->levelPath());
    const QStringList watched = _watcher->files();
    // Sous Windows, un fichier remplacé (écriture puis renommage) quitte la surveillance : on le
    // reprend dès qu'il existe à nouveau.
    if (watched.size() == 1 && watched.constFirst() == path) {
        return;
    }
    if (!watched.isEmpty()) {
        _watcher->removePaths(watched);
    }
    if (QFileInfo::exists(path)) {
        _watcher->addPath(path);
    }
}

QString MainWindow::keepAside(const char* label, const std::string& content) {
    const std::optional<std::filesystem::path> kept =
        _autosave->keepAside(_viewport->mapId(), label, timestamp(), content);
    return kept ? displayPath(*kept) : QString{};
}

bool MainWindow::checkDiskChange() {
    if (_checkingDisk) {
        return true;
    }
    watchLevelFile();
    const std::filesystem::path path = _viewport->levelPath();
    const QString map = QString::fromStdString(_viewport->mapId());
    switch (reactToDiskChange(_viewport->diskChange(), _viewport->isDirty())) {
        case DiskReaction::Ignore:
            return true;
        case DiskReaction::WarnDeleted:
            _viewport->acceptDiskVersion();
            showTransientStatusMessage(
                QStringLiteral("The file of %1 was deleted on disk; saving writes it again.")
                    .arg(map),
                8000);
            return true;
        case DiskReaction::ReloadQuietly: {
            // Rien à perdre de ce côté : on relit. Un fichier illisible est mis de côté, pour
            // qu'un enregistrement ne l'écrase pas sans trace.
            const std::optional<std::string> disk = readFile(path);
            if (_viewport->openLevel(path)) {
                showTransientStatusMessage(
                    QStringLiteral("%1 changed on disk and was reloaded.").arg(map), 8000);
                return false;
            }
            const QString kept = disk ? keepAside("disk", *disk) : QString{};
            _viewport->acceptDiskVersion();
            showTransientStatusMessage(
                QStringLiteral("%1 changed on disk but cannot be read; the disk version was set "
                               "aside in %2")
                    .arg(map, kept),
                10000);
            return true;
        }
        case DiskReaction::AskReloadOrKeep:
            break;
    }

    _checkingDisk = true;
    QMessageBox box(
        QMessageBox::Warning, QStringLiteral("Map changed on disk"),
        QStringLiteral("The file of map \"%1\" was changed outside the editor, and the "
                       "map has unsaved changes here.\n\nReload: open the disk version; "
                       "your changes are set aside first.\nKeep: keep your changes; "
                       "the disk version is set aside, and saving overwrites it.")
            .arg(map),
        QMessageBox::NoButton, this);
    QPushButton* const reloadButton =
        box.addButton(QStringLiteral("Reload from disk"), QMessageBox::DestructiveRole);
    QPushButton* const keepButton =
        box.addButton(QStringLiteral("Keep my version"), QMessageBox::RejectRole);
    box.setDefaultButton(keepButton);
    box.exec();
    _checkingDisk = false;

    if (box.clickedButton() == reloadButton) {
        const QString kept = keepAside("draft", _viewport->draftJson());
        if (kept.isEmpty() || !_viewport->openLevel(path)) {
            QMessageBox::warning(this, QStringLiteral("Map changed on disk"),
                                 QStringLiteral("The disk version could not be reloaded; your "
                                                "changes are kept."));
            return true;
        }
        showTransientStatusMessage(
            QStringLiteral("Reloaded from disk; your changes were set aside in %1").arg(kept),
            10000);
        return false;
    }
    const std::optional<std::string> disk = readFile(path);
    const QString kept = disk ? keepAside("disk", *disk) : QString{};
    if (kept.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Map changed on disk"),
                             QStringLiteral("The disk version could not be set aside; it is not "
                                            "overwritten. Try again, or save the map elsewhere."));
        return false;
    }
    _viewport->acceptDiskVersion();
    showTransientStatusMessage(
        QStringLiteral("Kept your changes; the disk version was set aside in %1").arg(kept), 10000);
    return true;
}

void MainWindow::refreshStatusHelp() {
    EditorStatusContext context;
    // L'essai n'édite rien : la barre d'état ne décrit alors aucun outil.
    if (!_viewport->playtesting()) {
        LevelStatusInfo level;
        // L'identifiant, pas le nom : celui-ci est une clé de traduction (LOT-EDITOR-07).
        level.name = _viewport->mapId();
        level.dirty = _viewport->isDirty();
        level.tool = _viewport->activeTool();
        level.hoveredCell = _viewport->hoveredCell();
        level.hoveredPieces = _viewport->hoveredPieces();
        level.hoveredForced = _viewport->hoveredCellForced();
        level.brush = brushLabel(_viewport->brush());
        level.collisionActive = !_viewport->activeLayer().has_value();
        level.mirror = _viewport->mirror().has_value();
        level.measure = _viewport->measureText();
        level.hoveredNote = _viewport->hoveredNote();
        level.zoom = _viewport->zoom();
        level.isoView = _viewport->canvasView() == CanvasView::Iso;
        context.level = level;
    }
    const EditorStatusLines lines = editorStatusLines(context);
    for (std::size_t index = 0; index < _statusZones.size(); ++index) {
        _statusZones[index]->setText(QString::fromStdString(lines.permanent[index]));
    }
    // Un message transitoire en cours garde la barre jusqu'à son expiration.
    if (!_statusMessageTimer->isActive()) {
        statusBar()->showMessage(QString::fromStdString(lines.help));
    }
}

void MainWindow::showTransientStatusMessage(const QString& message, int timeoutMs) {
    // Timeout laissé à 0 (défaut de showMessage) : le message reste affiché jusqu'à la
    // restauration explicite par _statusMessageTimer, seul maître de sa durée de vie.
    statusBar()->showMessage(message);
    _statusMessageTimer->start(timeoutMs);
}

QDockWidget* MainWindow::dockFor(PanelId panel) const {
    return _docks[static_cast<std::size_t>(panel)];
}

void MainWindow::applyPanelFocus(hmi::EditorTool tool) {
    if (!_actFollowActiveTool->isChecked() || _userPickedTab) {
        return;  // réglage désactivé, ou l'utilisateur a déjà imposé un onglet pour la session.
    }
    const std::optional<hmi::PanelId> panel = hmi::panelForTool(tool);
    if (!panel) {
        return;
    }
    // raise() met l'onglet au premier plan sans voler le focus clavier au canevas -- une
    // suggestion, jamais une confiscation.
    _suppressPanelFocusTracking = true;
    dockFor(*panel)->raise();
    _suppressPanelFocusTracking = false;
}

}  // namespace hmi
