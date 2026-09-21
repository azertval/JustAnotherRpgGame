// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QByteArray>
#include <QMainWindow>
#include <QMetaObject>
#include <QPixmap>
#include <array>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Editor/Logic/EditContextTarget.h"
#include "Editor/Logic/EditorTool.h"
#include "Editor/Logic/PanelFocus.h"

class QAction;
class QDockWidget;
class QFileSystemWatcher;
class QLabel;
class QMenu;
class QTabWidget;
class QTimer;
class QToolBar;

/**
 * @file Editor/Ui/MainWindow.h
 * @brief Fenêtre de l'éditeur de cartes (`LevelEditor`) : le canevas au centre, les panneaux
 *        d'édition en docks.
 */

namespace core {
struct MapEntity;
}  // namespace core

namespace hmi {

class AutosaveStore;
class EditorActions;
class EditorViewport;
class PalettePanel;
class LevelBrowserPanel;
class LayersPanel;
class EntityPanel;
class MiniMap;
class ProblemsPanel;
struct OpenDocument;
struct EditorReferences;
struct MapCheckFinding;
struct Citation;
struct RefactorPlan;

/**
 * @brief Fenêtre principale de l'éditeur (Qt Widgets, style Fusion, textes anglais).
 *
 * Le canevas (`hmi::EditorViewport`) est le widget central ; la palette, le navigateur de cartes,
 * les couches et les entités sont des docks détachables dont la disposition est persistée
 * (`EX-IHM-011`). Tout est construit en code (`LOT-EDITOR-01`). La fenêtre ne possède aucune donnée
 * d'édition : le canevas est le seul propriétaire du brouillon, les panneaux demandent et il
 * applique.
 *
 * Elle garde aussi le travail de l'auteur (`LOT-EDITOR-01`) : un brouillon modifié est sauvegardé
 * automatiquement hors du dépôt et proposé à la reprise après un plantage ; une carte changée sur
 * disque pendant qu'elle est ouverte n'est jamais écrasée en silence ; fermer avec des
 * modifications demande quoi en faire.
 */
class MainWindow : public QMainWindow {
public:
    /// @param crashAfterAutosave `--crash-test` : planter juste après la première sauvegarde
    ///        automatique, pour éprouver la reprise.
    explicit MainWindow(bool crashAfterAutosave = false);
    ~MainWindow() override;

    /**
     * @brief Ouvre la carte @p path dans un onglet (`LOT-EDITOR-09`).
     *
     * Une carte déjà ouverte revient au premier plan ; sinon un onglet vierge la reçoit, à défaut
     * un onglet neuf.
     * @param path         Le fichier de la carte.
     * @param reuseCurrent Vrai : l'onglet courant la reçoit s'il n'a rien de modifié — c'est le
     *                     `--map=` du démarrage, qui remplace la carte de départ au lieu de
     *                     s'ouvrir à côté d'elle.
     * @return `false` si la carte ne s'ouvre pas.
     */
    bool openMap(const std::filesystem::path& path, bool reuseCurrent = false);
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildUi();

    // --- Les cartes ouvertes en onglets (LOT-EDITOR-09) ---
    /// Crée un onglet, son canevas, et le rend actif. @return le canevas créé.
    EditorViewport* addDocument();
    /// Branche le canevas @p view sur la fenêtre et ses panneaux ; l'ancien est débranché.
    void bindViewport(EditorViewport* view);
    /// Débranche le canevas actif : les liaisons prises par `bindViewport` sont défaites.
    void unbindViewport();
    /// Le canevas de l'onglet @p index, `nullptr` hors bornes.
    [[nodiscard]] EditorViewport* documentAt(int index) const;
    /// Les cartes ouvertes, dans l'ordre des onglets.
    [[nodiscard]] std::vector<OpenDocument> openDocuments() const;
    /// L'onglet @p index devient actif : la fenêtre s'y rebranche, les panneaux le montrent.
    void activateDocument(int index);
    /// Réécrit le titre de chaque onglet (nom court, étoile si modifié) et celui de la fenêtre.
    void refreshDocumentLabels();
    /**
     * @brief Ferme l'onglet @p index, après avoir demandé quoi faire de ses modifications.
     * @return `false` si l'auteur a renoncé.
     */
    bool closeDocument(int index);
    /// Demande quoi faire des modifications de @p view. @return `false` si l'auteur renonce.
    bool askAboutChanges(EditorViewport* view);

    // --- Le monde : graphe, ville, propriétés (LOT-EDITOR-09) ---
    /// Relie deux cartes depuis le graphe : le plan, montré, puis écrit (`hmi::planLinkMaps`).
    void linkMaps(const std::string& fromMap, const std::string& toMap);
    /// « Map properties… » : le lieu, la région et l'ambiance de la carte ouverte.
    void openMapPropertiesDialog();
    /// « Map properties… » écrit aussi où en est la carte : son état va dans l'annexe.
    [[nodiscard]] QDockWidget* addPanel(const QString& objectName, const QString& title,
                                        QWidget* content, Qt::DockWidgetArea area);
    void buildMenus();
    void connectMapPanels();
    /// Les panneaux relisent le canevas actif : couches, entités, mini-carte, palette du lieu.
    void refreshLayersPanel();
    void refreshEntitiesPanel();
    void refreshMiniMap();
    void refreshPalettePanel();
    void connectToolActions();
    void connectEditorCommands();
    void buildStatusBar();
    void reloadEditorReferences();
    void restoreLayout();
    void saveLayout();
    void openResizeDialog();
    void openShortcutsDialog();
    void refreshStatusHelp();
    void showTransientStatusMessage(const QString& message, int timeoutMs);
    void applyPanelFocus(hmi::EditorTool tool);
    [[nodiscard]] QDockWidget* dockFor(PanelId panel) const;

    // --- Contrôle du contenu (LOT-EDITOR-07) ---
    /// Contrôle toutes les cartes enregistrées et montre le bilan dans « Problems ».
    void runContentCheck();
    /// Ouvre la carte du constat (garde-fou des modifications d'abord), sélectionne son entité et
    /// cerne sa case.
    void goToFinding(const MapCheckFinding& finding);
    /// Enregistre la carte ouverte (garde du fichier modifié sur disque d'abord), puis relit ce
    /// qui en dépend : références, graphe du monde, contrôle.
    bool saveMap();

    // --- Tampons et préfabriqués (LOT-EDITOR-08) ---
    /// Montre la bibliothèque du lieu ouvert dans la palette ; les vignettes déjà rendues sont
    /// gardées. Le dossier n'est relu que si le lieu a changé, ou si @p force le demande (après
    /// un enregistrement) : le brouillon change à chaque geste, pas la bibliothèque.
    void refreshPrefabs(bool force = false);
    /// « Save selection as prefab… » : demande un nom, écrit le tampon de la sélection.
    void saveSelectionAsPrefab();
    /// Arme le préfabriqué @p name du lieu ouvert comme tampon du canevas.
    void armPrefab(const QString& name);

    // --- Renommer et remplacer (LOT-EDITOR-14) ---
    /// Les commandes du menu *Map* : qui cite ceci, renommer une entité ou un point d'arrivée,
    /// remplacer une pièce, changer de planche.
    void buildRefactorMenu(QMenu* mapMenu);
    /// @return L'entité sélectionnée du brouillon, `nullptr` s'il n'y en a pas.
    [[nodiscard]] const core::MapEntity* selectedMapEntity() const;
    /// Montre @p citations sous @p title et va à celle que l'auteur choisit.
    /// @param title     Le titre de la fenêtre.
    /// @param citations Ce qui cite.
    void showCitationsOf(const QString& title, const std::vector<Citation>& citations);
    /// « Qui cite l'entité sélectionnée ? »
    void citeSelectedEntity();
    /// « Renommer l'identifiant de l'entité sélectionnée. »
    void renameSelectedEntityId();
    /// « Renommer le point d'arrivée sélectionné. »
    void renameSelectedArrival();
    /// « Remplacer une pièce », sur cette carte ou sur toutes.
    void replacePieceOnMaps();
    /// Renomme la carte @p mapId par le renommage propagé ; la carte ouverte suit.
    void renameMap(const std::string& mapId);
    /// Un renommage récrit des fichiers : **toutes** les cartes ouvertes doivent être
    /// enregistrées. @return `false` si l'auteur renonce.
    bool saveBeforeRefactor();
    /**
     * @brief Montre ce que @p plan récrit, l'écrit si l'auteur accepte, puis relit chaque onglet.
     * @param plan        Ce qui sera récrit.
     * @param title       Le titre de la fenêtre de confirmation.
     * @param renamedFrom La carte que le plan déplace, vide si aucune.
     * @param renamedTo   Son nouvel identifiant : l'onglet qui la portait l'y suit.
     */
    void carryOutPlan(const RefactorPlan& plan, const QString& title,
                      const std::string& renamedFrom = {}, const std::string& renamedTo = {});
    /// Ouvre la carte de la citation et y va, comme pour un constat du contrôle.
    void goToCitation(const Citation& citation);

    // --- Sauvegarde automatique, reprise, garde du fichier (LOT-EDITOR-01) ---
    void setUpSafetyNet();
    /// Relance le délai de sauvegarde automatique : une rafale de gestes n'écrit qu'une fois.
    void scheduleAutosave();
    /// Écrit le brouillon modifié, ou retire le fichier de reprise d'un brouillon redevenu propre.
    void writeAutosave();
    /// Au démarrage : propose de reprendre les brouillons laissés par une session interrompue.
    void offerRecovery();
    /// Surveille les fichiers de **toutes** les cartes ouvertes (à refaire après un remplacement
    /// de fichier, une ouverture ou un renommage).
    void watchLevelFile();
    /**
     * @brief Compare la carte sur disque à celle que l'éditeur a lue ou écrite, et réagit.
     * @return `false` si le brouillon a été remplacé par la version du disque (un enregistrement
     *         en cours doit alors s'arrêter).
     */
    bool checkDiskChange();
    /// Met @p content de côté pour la carte ouverte ; @return le chemin écrit, vide en cas d'échec.
    [[nodiscard]] QString keepAside(const char* label, const std::string& content);

    /// Les onglets : un canevas par carte ouverte, le widget central (`LOT-EDITOR-09`).
    QTabWidget* _tabs = nullptr;
    EditorViewport* _viewport = nullptr;  ///< Canevas de l'onglet actif (jamais nul).
    EditContextTarget* _editContext = nullptr;
    /// Les liaisons prises sur le canevas actif, défaites quand il quitte la scène.
    std::vector<QMetaObject::Connection> _viewportConnections;
    PalettePanel* _palette = nullptr;
    LevelBrowserPanel* _levels = nullptr;
    LayersPanel* _layers = nullptr;
    EntityPanel* _entities = nullptr;
    std::array<QDockWidget*, PANEL_COUNT> _docks{};  ///< Dans l'ordre de `PanelId`.
    /// La mini-carte (LOT-EDITOR-02) : un dock hors de la mise en avant par outil.
    MiniMap* _miniMap = nullptr;
    QDockWidget* _miniMapDock = nullptr;
    /// Les constats du contrôle, sur toutes les cartes (LOT-EDITOR-07) : hors de la mise en avant.
    ProblemsPanel* _problems = nullptr;
    QDockWidget* _problemsDock = nullptr;
    std::unique_ptr<EditorReferences> _references;
    EditorActions* _actions = nullptr;
    QToolBar* _toolBar = nullptr;
    QAction* _resizeAction = nullptr;
    QAction* _resetLayoutAction = nullptr;
    QByteArray _defaultState;                 ///< Disposition par défaut (pour « Reset layout »).
    QAction* _actFollowActiveTool = nullptr;  ///< Réglage persisté (menu View).
    /// L'utilisateur a choisi un onglet lui-même : la mise en avant automatique s'efface.
    bool _userPickedTab = false;
    bool _suppressPanelFocusTracking = false;
    std::array<QLabel*, 5> _statusZones{};
    QTimer* _statusMessageTimer = nullptr;

    /// La bibliothèque montrée : le lieu dont elle vient — relue quand il change — et les
    /// vignettes déjà rendues, par `lieu/nom`.
    std::string _prefabPlace;
    std::map<std::string, QPixmap> _prefabThumbnails;

    std::unique_ptr<AutosaveStore> _autosave;
    QTimer* _autosaveTimer = nullptr;
    /// Par canevas ouvert, la carte dont un fichier de reprise existe, écrit par cette session.
    std::map<EditorViewport*, std::string> _autosavedMapIds;
    QFileSystemWatcher* _watcher = nullptr;
    QTimer* _diskCheckTimer = nullptr;
    /// Une question sur le disque est déjà posée : ne pas en ouvrir une seconde.
    bool _checkingDisk = false;
    bool _crashAfterAutosave = false;
};

}  // namespace hmi
