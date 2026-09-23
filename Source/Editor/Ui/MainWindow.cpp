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
#include <QProcess>
#include <QPushButton>
#include <QRect>
#include <QScreen>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStatusBar>
#include <QString>
#include <QTabWidget>
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
#include "Core/World/WorldGraph.h"
#include "Editor/Logic/Autosave.h"
#include "Editor/Logic/CityView.h"
#include "Editor/Logic/DataRoot.h"
#include "Editor/Logic/DiskGuard.h"
#include "Editor/Logic/EditorStatus.h"
#include "Editor/Logic/EntityReferences.h"
#include "Editor/Logic/GameLaunch.h"
#include "Editor/Logic/MapDocuments.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/MapRefactor.h"
#include "Editor/Logic/PieceCatalog.h"
#include "Editor/Logic/Stamps.h"
#include "Editor/Logic/WorldLinks.h"
#include "Editor/Ui/EditorActions.h"
#include "Editor/Ui/EditorViewport.h"
#include "Editor/Ui/EntityPanel.h"
#include "Editor/Ui/LayersPanel.h"
#include "Editor/Ui/LevelBrowserPanel.h"
#include "Editor/Ui/MapPropertiesDialog.h"
#include "Editor/Ui/MapRender.h"
#include "Editor/Ui/MiniMap.h"
#include "Editor/Ui/PalettePanel.h"
#include "Editor/Ui/ProblemsPanel.h"
#include "Editor/Ui/RefactorDialogs.h"
#include "Editor/Ui/RunInGameDialog.h"
#include "HMI/Game/LaunchOptions.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/CrashDump.h"
#include "HMI/Platform/ExecutableDirectory.h"

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
// Cote de la vignette d'un prefabrique, en pixels : celle de la palette (LOT-EDITOR-08).
constexpr int PREFAB_THUMBNAIL_SIDE = 72;

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
    : _tabs(new QTabWidget), _crashAfterAutosave(crashAfterAutosave) {
    // Le dossier des données dans le titre : on sait où l'enregistrement écrit (LOT-EDITOR-06).
    setWindowTitle(QStringLiteral("Just Another RPG Game — Editor — %1")
                       .arg(QString::fromStdWString(hmi::editorDataRoot().wstring())));
    setDockNestingEnabled(true);

    // Les cartes ouvertes en onglets (LOT-EDITOR-09) : un canevas par onglet, celui de l'onglet
    // actif étant `_viewport`. Le dernier onglet ne se ferme pas — la fenêtre a toujours un
    // canevas, et rien ici n'a de cas « aucune carte ouverte ».
    _tabs->setDocumentMode(true);
    _tabs->setMovable(true);
    _tabs->setTabsClosable(true);
    setCentralWidget(_tabs);
    connect(_tabs, &QTabWidget::currentChanged, this, &MainWindow::activateDocument);
    connect(_tabs, &QTabWidget::tabCloseRequested, this,
            [this](int index) { static_cast<void>(closeDocument(index)); });
    addDocument();

    buildUi();

    // La palette arme le pinceau du canevas actif : un type ou une pièce du lieu.
    connect(_palette, &PalettePanel::tileSelected, this, [this](core::TileType type) {
        _viewport->setActiveTile(type);
        refreshStatusHelp();
    });
    connect(_palette, &PalettePanel::pieceSelected, this, [this](const QString& piece, bool floor) {
        _viewport->setActivePiece(piece.toStdString(), floor);
        refreshStatusHelp();
    });
    connect(_palette, &PalettePanel::prefabSelected, this, &MainWindow::armPrefab);
    // Ouvrir une carte depuis le panneau : son onglet s'il est déjà ouvert, un onglet neuf sinon.
    connect(_levels, &LevelBrowserPanel::levelOpenRequested, this, [this](const QString& path) {
        static_cast<void>(openMap(std::filesystem::path(path.toStdString())));
    });
    connect(_levels, &LevelBrowserPanel::mapRenameRequested, this,
            [this](const QString& mapId) { renameMap(mapId.toStdString()); });
    // Tirer un lien entre deux cartes du graphe du monde (LOT-EDITOR-09).
    connect(_levels, &LevelBrowserPanel::mapLinkRequested, this,
            [this](const QString& from, const QString& to) {
                linkMaps(from.toStdString(), to.toStdString());
            });
    // Les constats : un nouveau contrôle à la demande, et le chemin vers chacun.
    connect(_problems, &ProblemsPanel::checkRequested, this, &MainWindow::runContentCheck);
    connect(_problems, &ProblemsPanel::findingActivated, this, &MainWindow::goToFinding);

    connectMapPanels();
    reloadEditorReferences();
    // La fenêtre est bâtie : le canevas du premier onglet s'y branche et les panneaux le montrent.
    bindViewport(_viewport);

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

// --- Les cartes ouvertes en onglets (LOT-EDITOR-09) ---------------------------------------------

EditorViewport* MainWindow::addDocument() {
    // Le premier onglet montre la carte de départ, comme l'éditeur l'a toujours fait ; un onglet
    // de plus naît vierge — il va recevoir une carte nommée (LOT-EDITOR-09).
    auto* const view =
        new EditorViewport(_tabs->count() == 0 ? EditorViewport::StartContent::StartMap
                                               : EditorViewport::StartContent::Blank);
    view->setMinimumSize(320, 240);
    view->setFocusPolicy(Qt::StrongFocus);
    view->setEditorReferences(_references.get());  // les mêmes catalogues pour tous les onglets
    const int index = _tabs->addTab(view, QString::fromStdString(documentLabel({}, false)));
    _tabs->setCurrentIndex(index);  // `currentChanged` branche le canevas neuf
    if (_viewport != view) {
        bindViewport(view);  // premier onglet : `currentChanged` n'a pas de précédent à quitter
    }
    // Un seul onglet ne se ferme pas : la fenêtre garde toujours un canevas.
    _tabs->setTabsClosable(_tabs->count() > 1);
    return view;
}

EditorViewport* MainWindow::documentAt(int index) const {
    return index >= 0 && index < _tabs->count()
               ? qobject_cast<EditorViewport*>(_tabs->widget(index))
               : nullptr;
}

std::vector<OpenDocument> MainWindow::openDocuments() const {
    std::vector<OpenDocument> documents;
    for (int index = 0; index < _tabs->count(); ++index) {
        const EditorViewport* const view = documentAt(index);
        if (view != nullptr) {
            documents.push_back(OpenDocument{.mapId = view->mapId(), .dirty = view->isDirty()});
        }
    }
    return documents;
}

void MainWindow::activateDocument(int index) {
    EditorViewport* const view = documentAt(index);
    if (view == nullptr || view == _viewport) {
        return;
    }
    bindViewport(view);
    // L'onglet revient : le disque a pu changer pendant qu'on éditait ailleurs.
    if (_autosave != nullptr) {
        static_cast<void>(checkDiskChange());
    }
    view->setFocus();
}

void MainWindow::refreshDocumentLabels() {
    const std::vector<OpenDocument> documents = openDocuments();
    for (std::size_t index = 0; index < documents.size(); ++index) {
        const OpenDocument& document = documents[index];
        const int tab = static_cast<int>(index);
        _tabs->setTabText(tab,
                          QString::fromStdString(documentLabel(document.mapId, document.dirty)));
        _tabs->setTabToolTip(tab, QString::fromStdString(document.mapId));
    }
    _tabs->setTabsClosable(_tabs->count() > 1);
}

bool MainWindow::askAboutChanges(EditorViewport* view) {
    if (view == nullptr || !view->isDirty()) {
        return true;
    }
    // L'onglet en question passe devant : on ne répond pas d'une carte qu'on ne voit pas.
    _tabs->setCurrentWidget(view);
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, QStringLiteral("Unsaved changes"),
        QStringLiteral("Map \"%1\" has unsaved changes. Save them before closing?")
            .arg(QString::fromStdString(view->mapId())),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Cancel) {
        return false;
    }
    if (answer == QMessageBox::Save) {
        // Enregistrement refusé ou impossible : le brouillon reste. Si la garde a relu le disque,
        // le brouillon d'avant est déjà mis de côté et il n'y a plus rien à enregistrer.
        const bool draftKept = checkDiskChange();
        if ((draftKept && !view->save()) || (!draftKept && view->isDirty())) {
            return false;
        }
    }
    return true;
}

bool MainWindow::closeDocument(int index) {
    EditorViewport* const view = documentAt(index);
    if (view == nullptr || _tabs->count() <= 1) {
        return false;  // le dernier onglet reste : la fenêtre a toujours un canevas.
    }
    if (!askAboutChanges(view)) {
        return false;
    }
    // Fermeture voulue : la reprise de ce brouillon n'a plus d'objet.
    const auto autosaved = _autosavedMapIds.find(view);
    if (autosaved != _autosavedMapIds.end()) {
        if (!autosaved->second.empty()) {
            _autosave->discard(autosaved->second);
        }
        _autosavedMapIds.erase(autosaved);
    }
    const std::optional<std::size_t> next = documentAfterClose(
        static_cast<std::size_t>(_tabs->count()), static_cast<std::size_t>(_tabs->indexOf(view)));
    if (view == _viewport) {
        unbindViewport();
    }
    _tabs->removeTab(_tabs->indexOf(view));
    view->deleteLater();
    if (next) {
        _tabs->setCurrentIndex(static_cast<int>(*next));
        bindViewport(documentAt(static_cast<int>(*next)));
    }
    watchLevelFile();
    refreshDocumentLabels();
    return true;
}

bool MainWindow::openMap(const std::filesystem::path& path, bool reuseCurrent) {
    const std::string mapId = core::mapIdOf(editorDataRoot() / "Levels", path);
    if (const std::optional<std::size_t> open = documentOf(openDocuments(), mapId)) {
        _tabs->setCurrentIndex(static_cast<int>(*open));
        return true;  // une carte n'est ouverte qu'une fois : deux brouillons se contrediraient.
    }
    // Un onglet vierge et intact reçoit la carte ; `reuseCurrent` (le `--map=` du démarrage) prend
    // aussi l'onglet courant tant qu'il n'a rien de modifié. Sinon, un onglet neuf.
    const bool blank = (_viewport->mapId().empty() || reuseCurrent) && !_viewport->isDirty();
    EditorViewport* const view = blank ? _viewport : addDocument();
    if (!view->openLevel(path)) {
        if (!blank) {
            static_cast<void>(closeDocument(_tabs->indexOf(view)));
        }
        return false;
    }
    watchLevelFile();
    refreshDocumentLabels();
    return true;
}

void MainWindow::unbindViewport() {
    for (const QMetaObject::Connection& connection : _viewportConnections) {
        QObject::disconnect(connection);
    }
    _viewportConnections.clear();
}

void MainWindow::bindViewport(EditorViewport* view) {
    if (view == nullptr) {
        return;
    }
    unbindViewport();
    _viewport = view;
    _editContext = view;
    if (_palette == nullptr) {
        // La fenêtre n'est pas encore bâtie (premier onglet, dans le constructeur) : elle
        // rappellera `bindViewport` une fois ses panneaux et ses actions en place.
        return;
    }
    const auto keep = [this](const QMetaObject::Connection& connection) {
        _viewportConnections.push_back(connection);
    };

    // La pipette a pris un pinceau : la palette le montre, sans le réémettre.
    keep(connect(view, &EditorViewport::brushPicked, this, [this](const CanvasBrush& brush) {
        if (brush.kind == BrushKind::Piece) {
            _palette->showPiece(QString::fromStdString(brush.piece), brush.floor);
        } else if (brush.kind == BrushKind::Type) {
            _palette->showTile(brush.type);
        }
        refreshStatusHelp();
    }));
    keep(connect(view, &EditorViewport::toolStateChanged, this, [this] { refreshStatusHelp(); }));
    // L'outil Note : le texte se saisit dans une boîte, la note s'écrit dans l'annexe de la carte.
    keep(connect(view, &EditorViewport::noteRequested, this, [this](core::GridPosition cell) {
        const AuthorNote* const note = noteAt(_viewport->sidecar(), cell);
        bool accepted = false;
        const QString text = QInputDialog::getMultiLineText(
            this, QStringLiteral("Author note"),
            QStringLiteral("Note on (%1, %2) — empty removes it:").arg(cell.column).arg(cell.row),
            note != nullptr ? QString::fromStdString(note->text) : QString{}, &accepted);
        if (accepted) {
            _viewport->setNote(cell, text.toStdString());
        }
    }));
    // Le catalogue suit la carte : son lieu, et les pièces qu'elle cite sans que la planche les
    // ait. Les panneaux, l'onglet et la barre d'état suivent le brouillon.
    keep(connect(view, &EditorViewport::draftChanged, this, [this] {
        refreshPalettePanel();
        refreshLayersPanel();
        refreshEntitiesPanel();
        refreshMiniMap();
        refreshDocumentLabels();
        refreshStatusHelp();
        scheduleAutosave();
    }));
    keep(connect(view, &EditorViewport::activeLayerChanged, this,
                 [this](hmi::LayerSlot) { refreshLayersPanel(); }));
    keep(connect(view, &EditorViewport::layerViewChanged, this, [this] { refreshLayersPanel(); }));
    keep(connect(view, &EditorViewport::entitySelectionChanged, this,
                 [this](std::optional<std::size_t>) { refreshEntitiesPanel(); }));
    keep(connect(view, &EditorViewport::framingChanged, this,
                 [this] { _miniMap->setVisibleCorners(_viewport->visibleGridCorners()); }));
    // Le canevas change d'outil de lui-même (une famille d'entité choisie arme l'outil Entité) :
    // la barre d'outils suit, sans reboucler (setActiveTool n'émet rien).
    keep(connect(view, &EditorViewport::toolChanged, _actions, &EditorActions::setActiveTool));
    keep(connect(view, &EditorViewport::toolChanged, this,
                 [this](hmi::EditorTool) { refreshStatusHelp(); }));
    keep(connect(view, &EditorViewport::toolChanged, this, &MainWindow::applyPanelFocus));
    // Les messages d'état du canevas (enregistrement, essai, erreurs) s'affichent en bas, puis
    // laissent la main à l'aide contextuelle.
    keep(connect(view, &EditorViewport::statusMessage, this,
                 [this](const QString& message) { showTransientStatusMessage(message, 5000); }));
    keep(connect(view, &EditorViewport::hoveredCellChanged, this,
                 [this](std::optional<core::GridPosition>) { refreshStatusHelp(); }));
    keep(connect(view, &EditorViewport::zoomChanged, this, [this](float) { refreshStatusHelp(); }));
    // Vue iso ou à plat (décision D1) ; l'action suit la vue si elle change autrement.
    keep(connect(view, &EditorViewport::canvasViewChanged, this, [this](CanvasView canvasView) {
        QAction* const action = _actions->action(EditorCommand::IsoView);
        const QSignalBlocker blocker(action);
        action->setChecked(canvasView == CanvasView::Iso);
        refreshStatusHelp();
    }));

    _actions->setActiveTool(view->activeTool());
    _actions->applyShortcuts(view->editorBindings());
    refreshPalettePanel();
    refreshLayersPanel();
    refreshEntitiesPanel();
    refreshMiniMap();
    refreshDocumentLabels();
    refreshStatusHelp();
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
    // Les cartes ouvertes en onglets (LOT-EDITOR-09) : le dernier onglet ne se ferme pas.
    QAction* const closeTab = fileMenu->addAction(QStringLiteral("Close tab"));
    closeTab->setShortcut(QKeySequence::Close);
    connect(closeTab, &QAction::triggered, this,
            [this] { static_cast<void>(closeDocument(_tabs->currentIndex())); });
    fileMenu->addSeparator();
    QAction* const quit = fileMenu->addAction(QStringLiteral("Quit"));
    connect(quit, &QAction::triggered, this, &MainWindow::close);

    QMenu* const editMenu = menuBar()->addMenu(QStringLiteral("&Edit"));
    editMenu->addAction(_actions->action(EditorCommand::Undo));
    editMenu->addAction(_actions->action(EditorCommand::Redo));
    editMenu->addSeparator();
    editMenu->addAction(_actions->action(EditorCommand::Copy));
    editMenu->addAction(_actions->action(EditorCommand::Paste));
    editMenu->addAction(_actions->action(EditorCommand::PasteMirrored));
    editMenu->addSeparator();
    editMenu->addAction(_actions->action(EditorCommand::SaveAsPrefab));

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
    // L'essai COMPLET : le vrai jeu, sur les brouillons ouverts (LOT-EDITOR-10).
    mapMenu->addAction(_actions->action(EditorCommand::RunInGame));
    mapMenu->addAction(_actions->action(EditorCommand::RunInGameHere));
    mapMenu->addAction(_actions->action(EditorCommand::RunInGameOptions));
    mapMenu->addSeparator();
    QAction* const checkAll = mapMenu->addAction(QStringLiteral("Check all maps"));
    connect(checkAll, &QAction::triggered, this, [this] {
        runContentCheck();
        _problemsDock->show();
        _problemsDock->raise();
    });
    // Le lieu, la région, l'ambiance et où en est la carte (LOT-EDITOR-09).
    QAction* const properties = mapMenu->addAction(QStringLiteral("Map properties…"));
    connect(properties, &QAction::triggered, this, &MainWindow::openMapPropertiesDialog);
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

void MainWindow::refreshLayersPanel() {
    _layers->refresh(_viewport->draft(), _viewport->activeLayer(), _viewport->layerView());
}

void MainWindow::refreshEntitiesPanel() {
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
}

void MainWindow::refreshMiniMap() {
    _miniMap->setDraft(_viewport->draft());
    _miniMap->setVisibleCorners(_viewport->visibleGridCorners());
}

void MainWindow::refreshPalettePanel() {
    _palette->setPieceCatalog(_viewport->pieceCatalog(), _viewport->placeDirectory());
    refreshPrefabs();
}

void MainWindow::connectMapPanels() {
    // Les panneaux parlent au canevas ACTIF : chaque commande passe par `_viewport`, si bien
    // qu'un changement d'onglet ne demande de rebrancher personne (LOT-EDITOR-09).

    // Couches : le panneau demande, le canevas applique -- l'historique pour la structure, une
    // simple aide d'édition pour la visibilité et l'opacité.
    connect(_layers, &LayersPanel::activeLayerRequested, this,
            [this](hmi::LayerSlot slot) { _viewport->setActiveLayer(slot); });
    connect(_layers, &LayersPanel::visibilityRequested, this,
            [this](hmi::LayerSlot slot, bool visible) {
                _viewport->setMapLayerVisible(slot, visible);
            });
    connect(_layers, &LayersPanel::opacityRequested, this,
            [this](hmi::LayerSlot slot, float opacity) {
                _viewport->setMapLayerOpacity(slot, opacity);
            });
    connect(_layers, &LayersPanel::dimRequested, this, [this](hmi::LayerSlot slot, bool dimmed) {
        _viewport->setMapLayerDimmed(slot, dimmed);
    });
    connect(_layers, &LayersPanel::lockRequested, this, [this](hmi::LayerSlot slot, bool locked) {
        _viewport->setMapLayerLocked(slot, locked);
    });
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
    connect(_layers, &LayersPanel::moveRequested, this,
            [this](std::size_t index, bool forward) { _viewport->moveMapLayer(index, forward); });
    connect(_layers, &LayersPanel::renameRequested, this,
            [this](std::size_t index, const QString& name) {
                _viewport->renameMapLayer(index, name.toStdString());
            });
    connect(_layers, &LayersPanel::floorRequested, this,
            [this](std::size_t index, int floor) { _viewport->setMapLayerFloor(index, floor); });

    // Mini-carte : l'image suit le brouillon, le cadre suit la vue, un clic ramène la vue.
    connect(_miniMap, &MiniMap::centerRequested, this,
            [this](core::Vector2 point) { _viewport->centerOnGridPoint(point); });

    // Entités : choisir une famille à poser arme l'outil Entité.
    connect(_entities, &EntityPanel::kindToPlaceChanged, this, [this](const QString& type) {
        _viewport->setEntityKindToPlace(type.toStdString());
        if (!type.isEmpty()) {
            _viewport->setTool(hmi::EditorTool::Entity);
        }
    });
    connect(_entities, &EntityPanel::entitySelected, this,
            [this](std::optional<std::size_t> index) { _viewport->selectEntity(index); });
    connect(_entities, &EntityPanel::entitiesSelected, this,
            [this](const std::vector<std::size_t>& indices, std::optional<std::size_t> primary) {
                _viewport->setEntitySelection(indices, primary);
            });
    connect(_entities, &EntityPanel::propertyChanged, this,
            [this](std::size_t index, const QString& key, const core::PropertyValue& value) {
                _viewport->setEntityProperty(index, key.toStdString(), value);
            });
    connect(_entities, &EntityPanel::removeRequested, this,
            [this] { _viewport->removeSelectedEntities(); });
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
    refreshDocumentLabels();
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
            QStringLiteral("Rename entity id"));
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
            QStringLiteral("Rename arrival point"));
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
                     QStringLiteral("Replace piece"));
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
    // Un renommage récrit des fichiers de carte : un brouillon non enregistré, dans n'importe quel
    // onglet, serait écrit par-dessus au premier `Ctrl+S` (LOT-EDITOR-09).
    if (dirtyDocuments(openDocuments()).empty()) {
        return true;
    }
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, QStringLiteral("Save first"),
        QStringLiteral("This rewrites map files, maybe some open ones. Save every open map "
                       "first?"),
        QMessageBox::Save | QMessageBox::Cancel);
    if (answer != QMessageBox::Save) {
        return false;
    }
    for (int index = 0; index < _tabs->count(); ++index) {
        EditorViewport* const view = documentAt(index);
        if (view == nullptr || !view->isDirty()) {
            continue;
        }
        _tabs->setCurrentIndex(index);
        if (!saveMap()) {
            return false;
        }
    }
    return true;
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
    carryOutPlan(planRenameMap(editorDataRoot(), mapId, newId.toStdString()),
                 QStringLiteral("Rename map"), mapId, newId.toStdString());
}

void MainWindow::carryOutPlan(const RefactorPlan& plan, const QString& title,
                              const std::string& renamedFrom, const std::string& renamedTo) {
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
    // Toute carte ouverte a pu être récrite, et celle que le plan déplace a changé de chemin :
    // chaque onglet se relit là où sa carte est, propre (LOT-EDITOR-09).
    for (int index = 0; index < _tabs->count(); ++index) {
        EditorViewport* const view = documentAt(index);
        if (view == nullptr || view->mapId().empty()) {
            continue;
        }
        const std::string mapId = view->mapId() == renamedFrom ? renamedTo : view->mapId();
        const std::filesystem::path reopened = root / "Levels" / (mapId + ".json");
        std::error_code missing;
        if (std::filesystem::exists(reopened, missing)) {
            static_cast<void>(view->openLevel(reopened));
        }
    }
    watchLevelFile();
    refreshDocumentLabels();
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

// --- Tampons et prefabriques (LOT-EDITOR-08)
// -----------------------------------------------

void MainWindow::refreshPrefabs(bool force) {
    const std::filesystem::path& root = editorDataRoot();
    const std::string place = _viewport->place();
    if (!force && place == _prefabPlace) {
        return;  // le brouillon change a chaque geste, pas la bibliotheque.
    }
    _prefabPlace = place;
    const std::vector<std::string> names = prefabNames(root, place);
    std::vector<PalettePanel::PrefabItem> items;
    for (const std::string& name : names) {
        std::string error;
        const std::optional<Stamp> stamp = readPrefab(root, place, name, error);
        if (!stamp) {
            HMI_LOG_WARNING("Prefabriques : " + error);
            continue;
        }
        std::string key = place;
        key.append("/").append(name);
        const auto cached = _prefabThumbnails.find(key);
        if (cached == _prefabThumbnails.end()) {
            const QImage image = renderStamp(*stamp, root, place, PREFAB_THUMBNAIL_SIDE);
            _prefabThumbnails[key] = QPixmap::fromImage(image);
        }
        items.push_back(
            PalettePanel::PrefabItem{.name = QString::fromStdString(name),
                                     .detail = QString::fromStdString(stampLabel(*stamp)),
                                     .thumbnail = _prefabThumbnails[key]});
    }
    _palette->setPrefabs(std::move(items));
}

void MainWindow::saveSelectionAsPrefab() {
    const Stamp stamp = _viewport->selectionStamp();
    if (stamp.empty()) {
        showTransientStatusMessage(
            QStringLiteral("Nothing to save: select a region with the Selection tool first."),
            5000);
        return;
    }
    bool accepted = false;
    const QString name =
        QInputDialog::getText(this, QStringLiteral("Save selection as prefab"),
                              QStringLiteral("Prefab name (lowercase letters, digits, - and _):"),
                              QLineEdit::Normal, QString{}, &accepted);
    if (!accepted || name.isEmpty()) {
        return;
    }
    const std::filesystem::path& root = editorDataRoot();
    const std::string place = _viewport->place();
    const std::string error = writePrefab(root, place, name.toStdString(), stamp);
    if (!error.empty()) {
        QMessageBox::warning(this, QStringLiteral("Save failed"), QString::fromStdString(error));
        return;
    }
    // La vignette du nom repris est a refaire : le tampon a change.
    _prefabThumbnails.erase(place + "/" + name.toStdString());
    refreshPrefabs(true);
    showTransientStatusMessage(QStringLiteral("Prefab \"%1\" saved (%2).")
                                   .arg(name, QString::fromStdString(stampLabel(stamp))),
                               5000);
}

void MainWindow::armPrefab(const QString& name) {
    std::string error;
    std::optional<Stamp> stamp =
        readPrefab(editorDataRoot(), _viewport->place(), name.toStdString(), error);
    if (!stamp) {
        QMessageBox::warning(this, QStringLiteral("Prefab"), QString::fromStdString(error));
        return;
    }
    _viewport->setClipboardStamp(std::move(*stamp));
}

// --- Le monde : liens du graphe et propriétés de carte (LOT-EDITOR-09) -------------------------

void MainWindow::linkMaps(const std::string& fromMap, const std::string& toMap) {
    // Le plan récrit deux cartes : elles peuvent être ouvertes, et leur brouillon les écraserait.
    if (!saveBeforeRefactor()) {
        return;
    }
    carryOutPlan(planLinkMaps(editorDataRoot(), fromMap, toMap), QStringLiteral("Link maps"));
}

void MainWindow::openMapPropertiesDialog() {
    const core::PropertyMap& properties = _viewport->draft().properties();
    const auto text = [&properties](std::string_view key) {
        const auto found = properties.find(std::string{key});
        if (found == properties.end()) {
            return std::string{};
        }
        const auto* const value = std::get_if<std::string>(&found->second);
        return value != nullptr ? *value : std::string{};
    };
    const MapPropertiesChoice current{.region = text(core::MAP_REGION_PROPERTY),
                                      .ambience = text(core::MAP_AMBIENCE_PROPERTY),
                                      .state = _viewport->sidecar().state};
    const std::optional<MapPropertiesChoice> chosen = askMapProperties(
        this, QString::fromStdString(_viewport->mapId()),
        QString::fromStdString(_viewport->place()), worldRegionIds(editorDataRoot()), current);
    if (!chosen || *chosen == current) {
        return;
    }
    // La région et l'ambiance sont dans la carte : un pas d'annulation, enregistré avec elle.
    _viewport->setMapProperties({{std::string{core::MAP_REGION_PROPERTY}, chosen->region},
                                 {std::string{core::MAP_AMBIENCE_PROPERTY}, chosen->ambience}});
    // Où en est la carte est une note d'auteur : l'annexe s'écrit tout de suite, et seulement
    // pour une carte qui a un fichier.
    if (!_viewport->mapId().empty() && chosen->state != current.state) {
        _viewport->setMapState(chosen->state);
        _levels->refresh();
    }
    showTransientStatusMessage(QStringLiteral("Map properties changed. Ctrl+S saves the map."),
                               5000);
}

void MainWindow::stopRunningGame() {
    if (_game == nullptr || _game->state() == QProcess::NotRunning) {
        return;
    }
    // Un second essai REMPLACE le premier : deux jeux sur les memes brouillons, c'est deux mondes
    // qui divergent, et l'on ne sait plus lequel montre la retouche qu'on vient de faire.
    _game->kill();
    _game->waitForFinished(2000);
}

void MainWindow::runInGame(std::optional<core::GridPosition> at) {
    if (_viewport->mapId().empty()) {
        showTransientStatusMessage(QStringLiteral("Save the map first: the game opens maps by id."),
                                   5000);
        return;
    }
    // Le brouillon doit etre une carte JOUABLE : le jeu la refuserait, et l'on chercherait la
    // raison dans son journal a lui.
    const core::LevelLoadResult valide = _viewport->draft().toLevel();
    if (!valide.ok()) {
        showTransientStatusMessage(
            QStringLiteral("Cannot run in game: %1").arg(QString::fromStdString(valide.error)),
            8000);
        return;
    }
    const std::filesystem::path jeu = gameExecutable(executableDirectory());
    if (jeu.empty()) {
        showTransientStatusMessage(
            QStringLiteral("Cannot run in game: JustAnotherRpgGame is not built next to the "
                           "editor."),
            8000);
        return;
    }
    // TOUS les onglets, pas seulement celui qu'on joue : un portail mene sur la carte d'a cote, et
    // c'est le brouillon de cette carte-la qu'on veut voir, pas son dernier enregistrement.
    std::vector<DraftMap> brouillons;
    for (int index = 0; index < _tabs->count(); ++index) {
        const EditorViewport* const view = documentAt(index);
        if (view != nullptr && !view->mapId().empty()) {
            brouillons.push_back(DraftMap{.mapId = view->mapId(), .json = view->draftJson()});
        }
    }
    const std::filesystem::path dossier = playtestDirectory();
    if (const std::string erreur = writeDraftMaps(dossier, brouillons); !erreur.empty()) {
        HMI_LOG_WARNING("Editeur : essai complet refuse, " + erreur);
        showTransientStatusMessage(
            QStringLiteral("Cannot run in game: %1").arg(QString::fromStdString(erreur)), 8000);
        return;
    }

    GameLaunchOptions options;
    options.mapId = _viewport->mapId();
    options.cell = at;
    options.flags = _runChoice.flags;
    // Les brouillons d'abord, puis les cartes du depot : une carte qu'aucun onglet ne porte reste
    // celle que l'editeur ouvrirait, et non la copie de construction.
    options.levelDirectories = {dossier, editorDataRoot() / "Levels"};
    QStringList arguments;
    for (const std::string& argument : gameLaunchArguments(options)) {
        arguments.push_back(QString::fromStdString(argument));
    }

    stopRunningGame();
    if (_game == nullptr) {
        _game = new QProcess(this);
        connect(_game, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
            showTransientStatusMessage(
                QStringLiteral("The game could not be started: %1").arg(_game->errorString()),
                8000);
        });
        connect(_game, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
            showTransientStatusMessage(QStringLiteral("Game closed (exit code %1).").arg(code),
                                       5000);
        });
    }
    _game->setProgram(QString::fromStdString(jeu.string()));
    _game->setArguments(arguments);
    _game->setWorkingDirectory(QString::fromStdString(jeu.parent_path().string()));
    _game->start();
    HMI_LOG_INFO("Editeur : essai complet, " + jeu.filename().string() + " " +
                 arguments.join(QLatin1Char(' ')).toStdString());
    showTransientStatusMessage(
        QStringLiteral("Running %1 in the game…").arg(QString::fromStdString(options.mapId)), 5000);
}

void MainWindow::openRunInGameDialog() {
    const core::GridPosition bornes{.column = _viewport->levelWidth(),
                                    .row = _viewport->levelHeight()};
    // La case survolee est la proposition la plus utile : on ouvre le dialogue depuis l'endroit
    // qu'on regarde.
    RunInGameChoice depart = _runChoice;
    if (!depart.cell) {
        depart.cell = _viewport->hoveredCell();
    }
    const std::optional<RunInGameChoice> choix = askRunInGame(
        this, QString::fromStdString(_viewport->mapId()),
        _references != nullptr ? _references->flags : std::vector<std::string>{}, bornes, depart);
    if (!choix) {
        return;
    }
    _runChoice = *choix;
    runInGame(_runChoice.cell);
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
        !openMap(editorDataRoot() / "Levels" / (finding.mapId + ".json"))) {
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
    // Tous les onglets citent les mêmes catalogues : une carte ouverte dans un second onglet
    // verrait sinon toutes ses références cassées (LOT-EDITOR-09).
    for (int index = 0; index < _tabs->count(); ++index) {
        if (EditorViewport* const view = documentAt(index)) {
            view->setEditorReferences(_references.get());
        }
    }
}

void MainWindow::connectToolActions() {
    for (std::size_t index = 0; index < EDITOR_COMMAND_COUNT; ++index) {
        const auto command = static_cast<EditorCommand>(index);
        const std::optional<hmi::EditorTool> tool = EditorActions::toolOf(command);
        if (!tool) {
            continue;
        }
        connect(_actions->action(command), &QAction::toggled, this, [this, tool = *tool](bool on) {
            if (on) {
                _viewport->setTool(tool);
            }
        });
    }
}

void MainWindow::connectEditorCommands() {
    connect(_actions->action(EditorCommand::Save), &QAction::triggered, this,
            [this] { saveMap(); });
    connect(_actions->action(EditorCommand::Playtest), &QAction::triggered, this,
            [this] { _viewport->startPlaytest(); });
    connect(_actions->action(EditorCommand::PlaytestHere), &QAction::triggered, this,
            [this] { _viewport->startPlaytestHere(); });
    // L'essai complet (LOT-EDITOR-10) : le jeu, de l'entree de la carte ou de la case survolee.
    connect(_actions->action(EditorCommand::RunInGame), &QAction::triggered, this,
            [this] { runInGame(_runChoice.cell); });
    connect(_actions->action(EditorCommand::RunInGameHere), &QAction::triggered, this,
            [this] { runInGame(_viewport->hoveredCell()); });
    connect(_actions->action(EditorCommand::RunInGameOptions), &QAction::triggered, this,
            [this] { openRunInGameDialog(); });
    connect(_actions->action(EditorCommand::Mirror), &QAction::toggled, this,
            [this](bool enabled) { _viewport->setMirror(enabled); });
    connect(_actions->action(EditorCommand::Undo), &QAction::triggered, this,
            [this] { _editContext->undo(); });
    connect(_actions->action(EditorCommand::Redo), &QAction::triggered, this,
            [this] { _editContext->redo(); });
    connect(_actions->action(EditorCommand::Copy), &QAction::triggered, this,
            [this] { _editContext->copy(); });
    connect(_actions->action(EditorCommand::Paste), &QAction::triggered, this,
            [this] { _editContext->paste(); });
    // Tampons et préfabriqués (LOT-EDITOR-08) : le reflet, et la bibliothèque du lieu.
    connect(_actions->action(EditorCommand::PasteMirrored), &QAction::triggered, this,
            [this] { _viewport->pasteMirroredClipboard(); });
    connect(_actions->action(EditorCommand::SaveAsPrefab), &QAction::triggered, this,
            [this] { saveSelectionAsPrefab(); });
    connect(_actions->action(EditorCommand::ToggleGrid), &QAction::triggered, this,
            [this] { _viewport->toggleGrid(); });
    connect(_actions->action(EditorCommand::ResetCamera), &QAction::triggered, this,
            [this] { _viewport->resetCamera(); });
    // Vue iso ou à plat (décision D1) ; l'action suit la vue si elle change autrement.
    connect(_actions->action(EditorCommand::IsoView), &QAction::toggled, this, [this](bool iso) {
        _viewport->setCanvasView(iso ? CanvasView::Iso : CanvasView::Flat);
    });
    connect(_actions->action(EditorCommand::SeeThroughRelief), &QAction::toggled, this,
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
    // Chaque onglet modifié demande quoi faire du sien : rien ne se perd, et rien ne s'enregistre
    // sans qu'on l'ait dit (LOT-EDITOR-09).
    for (int index = 0; index < _tabs->count(); ++index) {
        if (!askAboutChanges(documentAt(index))) {
            event->ignore();
            return;
        }
    }
    // Fermeture voulue : les brouillons sont enregistrés ou abandonnés, leur reprise n'a plus
    // d'objet.
    _autosaveTimer->stop();
    for (const auto& [view, mapId] : _autosavedMapIds) {
        if (!mapId.empty()) {
            _autosave->discard(mapId);
        }
    }
    _autosavedMapIds.clear();
    saveLayout();
    QMainWindow::closeEvent(event);
}

void MainWindow::setUpSafetyNet() {
    _autosave = std::make_unique<AutosaveStore>(autosaveDirectory());
    _autosaveTimer = new QTimer(this);
    _autosaveTimer->setSingleShot(true);
    connect(_autosaveTimer, &QTimer::timeout, this, &MainWindow::writeAutosave);
    // `bindViewport` relance le délai à chaque geste du canevas actif.

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
    if (_autosaveTimer == nullptr) {
        return;  // le filet de sécurité n'est pas encore posé (construction de la fenêtre).
    }
    watchLevelFile();  // les cartes ouvertes ont pu changer (ouverture, renommage).
    _autosaveTimer->start(AUTOSAVE_DELAY_MS);
}

void MainWindow::writeAutosave() {
    // Chaque onglet a son brouillon de reprise, sous l'identifiant de sa carte (LOT-EDITOR-09).
    for (int index = 0; index < _tabs->count(); ++index) {
        EditorViewport* const view = documentAt(index);
        if (view == nullptr) {
            continue;
        }
        std::string& autosaved = _autosavedMapIds[view];
        const std::string& mapId = view->mapId();
        // Le fichier de reprise d'une autre carte (renommée, ou quittée en abandonnant ses
        // modifications) n'a plus d'objet.
        if (!autosaved.empty() && (autosaved != mapId || !view->isDirty())) {
            _autosave->discard(autosaved);
            autosaved.clear();
        }
        if (!view->isDirty()) {
            continue;
        }
        const AutosaveRecord record{
            .mapId = mapId, .levelPath = view->levelPath(), .draftJson = view->draftJson()};
        if (!_autosave->write(record)) {
            HMI_LOG_WARNING("Editeur : sauvegarde automatique impossible dans " +
                            _autosave->directory().string());
            continue;
        }
        autosaved = mapId;
        if (_crashAfterAutosave) {
            hmi::triggerCrashForTest();
        }
    }
}

void MainWindow::offerRecovery() {
    for (const AutosaveRecord& record : _autosave->pending()) {
        offerRecoveryFor(record);
    }
}

void MainWindow::offerRecoveryFor(const AutosaveRecord& record) {
    const QString map = QString::fromStdString(record.mapId);
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
    const bool recover = box.clickedButton() == recoverButton;
    // Chaque brouillon repris prend son onglet (LOT-EDITOR-09) : plus besoin de les mettre de
    // côté faute de place, comme quand une seule carte s'ouvrait.
    if (recover) {
        const bool blank = _viewport->mapId().empty() && !_viewport->isDirty();
        EditorViewport* const view = blank ? _viewport : addDocument();
        if (view->restoreDraft(record.mapId, record.draftJson)) {
            _autosavedMapIds[view] = record.mapId;
            watchLevelFile();
            refreshDocumentLabels();
            return;
        }
        if (!blank) {
            static_cast<void>(closeDocument(_tabs->indexOf(view)));
        }
    }
    const std::optional<std::filesystem::path> kept =
        _autosave->keepAside(record.mapId, "draft", timestamp(), record.draftJson);
    if (!kept) {
        HMI_LOG_WARNING("Editeur : brouillon de reprise laisse en place : " + record.mapId);
        return;  // rien n'est retiré tant qu'il n'est pas à l'abri.
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

void MainWindow::watchLevelFile() {
    if (_watcher == nullptr) {
        return;  // avant `setUpSafetyNet` : il n'y a encore rien à surveiller.
    }
    // Les fichiers de toutes les cartes ouvertes : celle qu'on regarde, et celles qui attendent
    // dans leur onglet (LOT-EDITOR-09). Sous Windows, un fichier remplacé (écriture puis
    // renommage) quitte la surveillance : on la refait dès qu'il existe à nouveau.
    QStringList wanted;
    for (int index = 0; index < _tabs->count(); ++index) {
        const EditorViewport* const view = documentAt(index);
        if (view == nullptr) {
            continue;
        }
        const QString path = displayPath(view->levelPath());
        if (!path.isEmpty() && QFileInfo::exists(path) && !wanted.contains(path)) {
            wanted.append(path);
        }
    }
    QStringList watched = _watcher->files();
    watched.sort();
    QStringList sorted = wanted;
    sorted.sort();
    if (watched == sorted) {
        return;
    }
    if (!watched.isEmpty()) {
        _watcher->removePaths(watched);
    }
    if (!wanted.isEmpty()) {
        _watcher->addPaths(wanted);
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
