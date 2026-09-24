// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QColor>
#include <QGraphicsView>
#include <QString>
#include <QTimer>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Core/Combat/IsoProjection.h"
#include "Core/Combat/TacticalTerrain.h"
#include "Core/Gameplay/WorldFlags.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelProperties.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileType.h"
#include "Core/Time/FixedTimestep.h"
#include "Core/World/CombatZone.h"
#include "Core/World/EntityKinds.h"
#include "Editor/Logic/BrushGesture.h"
#include "Editor/Logic/CanvasPicking.h"
#include "Editor/Logic/CanvasScene.h"
#include "Editor/Logic/DiskGuard.h"
#include "Editor/Logic/EditContextTarget.h"
#include "Editor/Logic/EditorDiagnostics.h"
#include "Editor/Logic/EditorKeyBindings.h"
#include "Editor/Logic/EditorSidecar.h"
#include "Editor/Logic/EditorTool.h"
#include "Editor/Logic/EntityGesture.h"
#include "Editor/Logic/LayerView.h"
#include "Editor/Logic/PaintTools.h"
#include "Editor/Logic/PieceCatalog.h"
#include "Editor/Logic/Stamps.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

class QGraphicsScene;
class QPainter;

/**
 * @file Editor/Ui/EditorViewport.h
 * @brief Le canevas de l'éditeur : le lieu qu'on édite, rendu comme dans le jeu, et l'essai
 *        immédiat (`LOT-EDITOR-02`, `EX-EDIT-059`).
 *
 * Une `QGraphicsView` et **un seul élément peint** (décision D2) : il parcourt la `ComposedScene`
 * que la composition du jeu produit — mêmes primitives, même ordre — et la peint par `QPainter`
 * (`hmi::paintComposedScene`), en ne touchant que la partie visible. Par-dessus, les aides
 * d'édition : quadrillage en losanges, case survolée, masque de collision, aperçu des outils,
 * entités (marqueur, ou figurine quand elle existe ; forme, étiquette et poignées), notes
 * d'auteur, axe du miroir.
 *
 * Les entités se dessinent et se manipulent **par leur forme** (`core::EntityKind::shape`), jamais
 * par leur type (`LOT-EDITOR-05`) : aucune famille n'a de code propre ici.
 *
 * Les outils du peintre (`LOT-EDITOR-04`) sont les fonctions pures de `Editor/Logic/PaintTools.h` :
 * le canevas ne fait que les appeler et en montrer l'aperçu. Un geste — du clic au relâchement —
 * est un pas d'annulation (`core::GestureScope`), et `Alt` + clic prend le pinceau sous la case
 * depuis n'importe quel outil.
 *
 * Deux vues, en bascule (décision D1) : **iso** par défaut, le lieu tel qu'on le jouera ; **à
 * plat**, une case par unité et les types en couleurs (`hmi::DraftRenderer`), pour lire types et
 * collision. Tout geste passe par le pointage (`Editor/Logic/CanvasPicking.h`) et parle en cases :
 * les outils ne savent pas quelle vue est affichée.
 *
 * Deux états, jamais mêlés. En **édition**, le brouillon est la seule source. En **essai**, la
 * carte est jouée par `hmi::WorldPlay` et composée comme dans le jeu (`EX-EDIT-055`) ; le même
 * peintre la dessine, cadrée sur le héros.
 */

namespace hmi {
class DraftRenderer;
class SceneImages;
class WorldPlay;
struct EditorReferences;
}  // namespace hmi

namespace hmi {

/// @brief Le canevas : le brouillon peint en iso ou à plat, les gestes des outils, l'essai.
class EditorViewport : public QGraphicsView, public EditContextTarget {
    Q_OBJECT

public:
    /// @brief Ce qu'un canevas neuf montre : la carte de départ, ou rien (`LOT-EDITOR-09`, un
    ///        onglet qui va recevoir une carte nommée).
    enum class StartContent {
        StartMap,
        Blank,
    };

    explicit EditorViewport(StartContent content = StartContent::StartMap,
                            QWidget* parent = nullptr);
    ~EditorViewport() override;
    EditorViewport(const EditorViewport&) = delete;
    EditorViewport& operator=(const EditorViewport&) = delete;

    // --- Pinceau (LOT-EDITOR-03) ---
    /// Arme le pinceau d'un type de tuile (palette des types) : il peint la couche active.
    void setActiveTile(core::TileType type);
    /**
     * @brief Arme le pinceau d'une pièce (palette des pièces) : elle va sur sa couche, qui devient
     *        la couche active (`hmi::pieceTargetLayer`).
     * @param piece Le nom court de la pièce.
     * @param floor La pièce est un sol.
     */
    void setActivePiece(const std::string& piece, bool floor);
    /**
     * @brief Le miroir (`LOT-EDITOR-04`) : actif, chaque geste se reflète de l'autre côté d'un axe
     *        vertical de l'écran, qui passe par la case survolée à l'activation (le centre de la
     *        carte sans case survolée).
     */
    void setMirror(bool enabled);
    [[nodiscard]] const std::optional<MirrorAxis>& mirror() const noexcept {
        return _mirror;
    }
    /// @return La mesure en cours (`hmi::measureLabel`), vide sans mesure.
    [[nodiscard]] std::string measureText() const;
    /// @return Le pinceau armé.
    [[nodiscard]] const CanvasBrush& brush() const noexcept {
        return _brush;
    }
    /// @return Le catalogue des pièces du lieu de la carte ouverte (`hmi::pieceCatalog`).
    [[nodiscard]] std::vector<PieceCatalogGroup> pieceCatalog() const;
    /// @return Le dossier auquel les fichiers du catalogue sont relatifs (`Assets/`, `LOT-124`),
    ///         vide sans lieu.
    [[nodiscard]] std::filesystem::path pieceImagesDirectory() const;
    /// @return Le lieu de la carte ouverte (`scene`), vide pour une carte sans lieu.
    [[nodiscard]] const std::string& place() const noexcept {
        return _appearancePlace;
    }
    /// @return Vrai si la collision de la case survolée est forcée à la main.
    [[nodiscard]] bool hoveredCellForced() const;
    void setTool(hmi::EditorTool tool);
    [[nodiscard]] hmi::EditorKeyBindings& editorBindings() noexcept {
        return _editorBindings;
    }

    /// Enregistre le brouillon sous `Levels/<nom>.json`, après validation (`EX-EDIT-007`).
    /// @return `true` si le fichier a été écrit.
    bool save();
    /// Ouvre @p path comme brouillon ; sort d'un éventuel essai.
    /// @return `true` si la carte s'est ouverte ; en cas d'échec, le brouillon reste intact.
    bool openLevel(const std::filesystem::path& path);
    /// Le brouillon porte des modifications : sa révision n'est plus celle de la dernière
    /// ouverture ou du dernier enregistrement (`core::LevelDraft::revision`). Défaire jusqu'à
    /// l'état enregistré rend un brouillon propre.
    [[nodiscard]] bool isDirty() const noexcept {
        return _draft.revision() != _savedRevision;
    }
    /// Renomme des pièces de la carte ouverte, en un pas d'annulation
    /// (`core::LevelDraft::replacePieces`, `LOT-EDITOR-14`).
    bool replacePieces(const core::PieceRenaming& renaming);
    /// Fait passer la carte ouverte à la planche du lieu @p place, pièces traduites par @p table,
    /// en un pas d'annulation (`core::LevelDraft::changeScene`, `LOT-EDITOR-14`).
    bool changeScene(const std::string& place, const core::PieceRenaming& table);

    // --- Sauvegarde automatique et garde du fichier (LOT-EDITOR-01) ---
    /// @return L'identifiant de la carte ouverte (`capital/martpart`).
    [[nodiscard]] const std::string& mapId() const noexcept {
        return _mapId;
    }
    /// @return Le fichier de la carte ouverte, qu'il existe ou non.
    [[nodiscard]] std::filesystem::path levelPath() const;
    /// @return Le brouillon en JSON non validé, pour la sauvegarde automatique.
    [[nodiscard]] std::string draftJson() const {
        return _draft.toJson();
    }
    /**
     * @brief Reprend un brouillon sauvegardé automatiquement : il remplace le brouillon courant et
     *        reste marqué modifié, jusqu'à ce qu'on l'enregistre.
     * @return `false` si le brouillon ne se relit pas (incomplet, format inconnu) : rien n'a
     *         changé, et le fichier de reprise reste en place.
     */
    bool restoreDraft(const std::string& mapId, const std::string& draftJson);
    /// @return Ce qui a changé sur disque depuis la dernière lecture ou écriture de la carte.
    [[nodiscard]] DiskChange diskChange() const;
    /// Prend l'état actuel du fichier pour référence : l'auteur a choisi de garder son brouillon,
    /// le prochain enregistrement écrasera la version du disque (mise de côté par l'appelant).
    void acceptDiskVersion();

    /**
     * @brief Joue le brouillon avec le moteur du jeu (`EX-EDIT-008`, `EX-EDIT-055`).
     * @param from La case d'où part le héros (`LOT-EDITOR-04`), l'entrée si rien. Une case hors
     *             de la carte ou qui arrête le pas est refusée.
     */
    void startPlaytest(std::optional<core::GridPosition> from = std::nullopt);
    /// Joue le brouillon depuis la case survolée ; depuis l'entrée s'il n'y en a pas.
    void startPlaytestHere();

    /**
     * @brief L'état de partie (`LOT-126`, `Editor/Logic/WorldState.h`) : l'essai en part toujours ;
     *        le canevas grise ce qu'il rend absent si @p preview.
     */
    void setWorldState(std::vector<std::string> entries, bool preview);

    // --- Notes d'auteur (LOT-EDITOR-04) ---
    /// @return L'annexe de la carte ouverte (`<carte>.editor.json`).
    [[nodiscard]] const EditorSidecar& sidecar() const noexcept {
        return _sidecar;
    }
    /// Écrit la note de @p cell, retirée si @p text est vide ; l'annexe s'écrit tout de suite.
    void setNote(core::GridPosition cell, const std::string& text);

    // --- Propriétés de carte et état (LOT-EDITOR-09) ---
    /**
     * @brief Assigne la propriété de carte @p key — sa région, son ambiance (`EX-EDIT-091`).
     *
     * C'est un pas d'annulation, comme toute mutation du brouillon ; une chaîne vide retire la
     * propriété.
     */
    void setMapProperty(const std::string& key, core::PropertyValue value);
    /// Assigne plusieurs propriétés de carte en **un** pas d'annulation : ce que valide le
    /// dialogue des propriétés.
    void setMapProperties(const std::vector<std::pair<std::string, core::PropertyValue>>& values);
    /// Écrit où en est la carte dans son annexe ; l'annexe s'écrit tout de suite, hors historique.
    void setMapState(MapState state);
    /// @return La note de la case survolée, sur une ligne, vide sans note.
    [[nodiscard]] std::string hoveredNote() const;
    [[nodiscard]] bool playtesting() const noexcept {
        return _play != nullptr;
    }

    void undo() override;
    void redo() override;
    [[nodiscard]] bool canUndo() const override {
        return _draft.canUndo();
    }
    [[nodiscard]] bool canRedo() const override {
        return _draft.canRedo();
    }
    void copy() override {
        copySelection();
    }
    void paste() override {
        pasteClipboard(false);
    }
    [[nodiscard]] bool canCopy() const override {
        return _selection.has_value();
    }
    [[nodiscard]] bool canPaste() const override {
        return !_clipboard.empty();
    }

    // --- Tampons et préfabriqués (LOT-EDITOR-08) ---
    /// Colle le tampon **reflété** : la jumelle de chaque pièce, le rectangle transposé.
    void pasteMirroredClipboard() {
        pasteClipboard(true);
    }
    /// Arme @p stamp comme tampon à poser : un préfabriqué choisi dans la bibliothèque.
    void setClipboardStamp(Stamp stamp);
    /// @return Le tampon courant : ce que `Ctrl+C` a pris, ou le préfabriqué armé.
    [[nodiscard]] const Stamp& clipboardStamp() const noexcept {
        return _clipboard;
    }
    /// @return Le tampon de la sélection courante, vide s'il n'y a pas de sélection.
    [[nodiscard]] Stamp selectionStamp() const;

    void toggleGrid() noexcept;
    /// Recadre la vue sur toute la carte.
    void resetCamera();

    // --- Vues du canevas (LOT-EDITOR-02) ---
    /// Bascule entre la vue iso (le lieu) et la vue à plat (les types), cadrage recalculé.
    void setCanvasView(CanvasView view);
    [[nodiscard]] CanvasView canvasView() const noexcept {
        return _view;
    }
    /// Reliefs en transparence : on voit ce qu'on pointe derrière un mur.
    void setSeeThroughRelief(bool enabled);
    [[nodiscard]] bool seeThroughRelief() const noexcept {
        return _seeThroughRelief;
    }
    /// @return La couleur d'un type de tuile, celle de la vue à plat (pour la mini-carte).
    [[nodiscard]] QColor tileColor(core::TileType type) const;
    /// @return Les pièces de la case survolée (`street · wall-left`), vide sinon.
    [[nodiscard]] std::string hoveredPieces() const;
    /**
     * @brief Les quatre coins de la partie visible, en coordonnées de grille continues (colonne,
     *        ligne) : un rectangle à plat, un losange en iso. Pour la mini-carte.
     */
    [[nodiscard]] std::array<core::Vector2, 4> visibleGridCorners() const;
    /// Centre la vue sur un point de grille continu (colonne, ligne).
    void centerOnGridPoint(core::Vector2 gridPoint);
    /**
     * @brief Centre la vue sur @p cell et la cerne, jusqu'à la carte suivante ou au prochain
     *        appel : la case d'un constat du panneau « Problems » (`LOT-EDITOR-07`).
     */
    void revealCell(core::GridPosition cell);

    void resizeLevel(int width, int height);
    [[nodiscard]] bool wouldResizeDrop(int width, int height) const;
    [[nodiscard]] int levelWidth() const;
    [[nodiscard]] int levelHeight() const;

    [[nodiscard]] const core::LevelDraft& draft() const noexcept {
        return _draft;
    }
    [[nodiscard]] std::optional<core::GridPosition> hoveredCell() const noexcept {
        return _hoverCell;
    }
    /// @return Le facteur d'agrandissement : 1 quand une unité monde fait 16 pixels.
    [[nodiscard]] float zoom() const noexcept;
    [[nodiscard]] EditorTool activeTool() const noexcept {
        return _tool;
    }

    // --- Couches et entités (LOT-11) ---
    void setActiveLayer(LayerSlot slot);
    [[nodiscard]] LayerSlot activeLayer() const noexcept {
        return _activeLayer;
    }
    [[nodiscard]] const LayerViewState& layerView() const noexcept {
        return _layerView;
    }
    void setMapLayerVisible(LayerSlot slot, bool visible);
    void setMapLayerOpacity(LayerSlot slot, float opacity);
    void setMapLayerDimmed(LayerSlot slot, bool dimmed);
    void setMapLayerLocked(LayerSlot slot, bool locked);
    void addMapLayer(core::LayerKind kind, const std::string& name);
    void removeMapLayer(std::size_t index);
    void moveMapLayer(std::size_t index, bool forward);
    void renameMapLayer(std::size_t index, const std::string& name);
    /// Met la couche de décor @p index à l'étage @p floor (`LOT-129`), par l'historique.
    void setMapLayerFloor(std::size_t index, int floor);

    /// Catalogues que les entités référencent (non possédés), relus par la fenêtre.
    void setEditorReferences(const EditorReferences* references);
    void setEntityKindToPlace(std::string type);
    /// Sélectionne la seule entité @p index, ou vide la sélection.
    void selectEntity(std::optional<std::size_t> index);
    /**
     * @brief Sélectionne les entités @p indices (`LOT-EDITOR-05`) ; @p primary, l'une d'elles, est
     *        celle que l'inspecteur montre (la dernière de la liste à défaut).
     */
    void setEntitySelection(std::vector<std::size_t> indices,
                            std::optional<std::size_t> primary = std::nullopt);
    /// @return L'entité que l'inspecteur montre : la dernière prise.
    [[nodiscard]] std::optional<std::size_t> selectedEntity() const noexcept {
        return _selectedEntity;
    }
    /// @return Toutes les entités sélectionnées, triées.
    [[nodiscard]] const std::vector<std::size_t>& selectedEntities() const noexcept {
        return _selectedEntities;
    }
    void setEntityProperty(std::size_t index, const std::string& key, core::PropertyValue value);
    void removeEntity(std::size_t index);
    /// Retire toutes les entités sélectionnées, en un pas.
    void removeSelectedEntities();
    /// @return Le verdict de chaque zone de combat du brouillon (`core::analyzeCombatZones`).
    [[nodiscard]] const std::vector<core::CombatZoneTerrain>& combatZones() const noexcept {
        return _zoneVerdicts;
    }
    [[nodiscard]] const std::vector<EditorDiagnostic>& diagnostics() const noexcept {
        return _diagnostics;
    }
    [[nodiscard]] const core::EntityReferenceContext& entityReferenceContext() const noexcept {
        return _referenceContext;
    }

    /// Peint le canevas : appelé par l'élément unique de la scène, @p exposed en unités monde.
    void paintCanvas(QPainter& painter, const QRectF& exposed);

signals:
    void activeLayerChanged(hmi::LayerSlot slot);
    void layerViewChanged();
    void entitySelectionChanged(std::optional<std::size_t> index);
    void statusMessage(const QString& message);
    void draftChanged();
    void toolChanged(hmi::EditorTool tool);
    void hoveredCellChanged(std::optional<core::GridPosition> cell);
    void zoomChanged(float zoom);
    /// Le cadrage a bougé (défilement, agrandissement, vue) : la mini-carte suit.
    void framingChanged();
    void canvasViewChanged(hmi::CanvasView view);
    /// La pipette a pris @p brush : la palette le montre.
    void brushPicked(const hmi::CanvasBrush& brush);
    /// L'outil Note a désigné @p cell : la fenêtre demande le texte.
    void noteRequested(core::GridPosition cell);
    /// La mesure, le miroir ou les notes ont changé : la barre d'état relit.
    void toolStateChanged();

protected:
    bool viewportEvent(QEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;

private:
    class CanvasItem;

    /// Avance l'essai des pas fixes dus, puis recompose sa scène.
    void stepPlaytest();
    /// Termine l'essai et rend la main à l'édition (brouillon intact).
    void stopPlaytest();
    /// Direction que composent les touches enfoncées, normalisée — la règle de `GameView.qml`.
    [[nodiscard]] core::Vector2 heldDirection() const;

    // --- Peinture ---
    /// Recompose la scène iso du brouillon si elle est périmée.
    void ensureIsoScene();
    void paintIso(QPainter& painter, const QRectF& exposed);
    void paintFlat(QPainter& painter, const QRectF& exposed);
    void paintPlaytest(QPainter& painter, const QRectF& exposed);
    void paintIsoOverlays(QPainter& painter, const CellRange& cells, const IsoBandOpacity& bands);
    /// Le masque de collision en losanges : une teinte par catégorie de règle.
    /// @param painter Le peintre du canevas.
    /// @param cells   Les cases visibles.
    /// @param opacity L'opacité de la bande de collision.
    void paintIsoCollisionMask(QPainter& painter, const CellRange& cells, float opacity);
    /// Le quadrillage en losanges, du premier au dernier bord visible.
    /// @param painter Le peintre du canevas.
    /// @param cells   Les cases visibles.
    void paintIsoGrid(QPainter& painter, const CellRange& cells);
    /// Le terrain de la rencontre sélectionnée : sa zone, puis la case de chaque combattant.
    /// @param painter Le peintre du canevas.
    void paintEncounterTerrain(QPainter& painter);
    /// Relit la table d'apparence et le manifeste du lieu @p place.
    /// @param place Le lieu que la carte nomme ; vide : rien à lire.
    void loadPlaceAssets(const std::string& place);
    /// @return Le rectangle du monde qu'occupe le contenu de la vue courante, marge comprise.
    [[nodiscard]] QRectF contentBounds() const;
    /// Recalcule l'étendue de la scène (vue, taille de carte, essai) et redemande une peinture.
    void refreshBounds();
    /// Invalide la scène iso et redemande une peinture.
    void invalidateScene();
    /// @return La projection iso du brouillon.
    [[nodiscard]] core::IsoProjection projection() const;
    [[nodiscard]] core::Vector2 worldPosition(const QMouseEvent* event) const;
    void emitZoomIfChanged();

    [[nodiscard]] std::optional<core::GridPosition> cellAt(const QMouseEvent* event) const;
    [[nodiscard]] core::GridPosition clampedCell(const QMouseEvent* event) const;
    /// Donne un coup de pinceau sur la case pointée ; @p continuing : le geste prolonge un glisser.
    void paintAt(const QMouseEvent* event, bool continuing);
    /// Le pinceau armé, son type recalculé d'après la table du lieu courant ; la gomme à l'outil
    /// Gomme.
    [[nodiscard]] CanvasBrush currentBrush() const;
    /// Ce qui accompagne un geste : le miroir et la table du lieu.
    [[nodiscard]] StrokeContext strokeContext() const;
    /// La pipette en @p cell : le pinceau pris, et sa couche rendue active.
    void pickAt(core::GridPosition cell);
    /// Ferme le geste en cours (glisser du pinceau ou de la gomme), s'il y en a un.
    void endPainting();
    /// Relit l'annexe de la carte ouverte.
    void reloadSidecar();
    /// Les notes d'auteur, en pastilles, en iso ou à plat.
    void paintNotes(QPainter& painter, const CellRange& cells, bool iso);
    /// L'axe du miroir, en iso ou à plat.
    void paintMirrorAxis(QPainter& painter, bool iso);
    /// L'aperçu de la ligne ou de la mesure tirée, en iso ou à plat.
    void paintDragPreview(QPainter& painter, bool iso);
    /// Applique le résultat d'un geste : brouillon redessiné, ou refus dit une fois par geste.
    void reportBrush(const BrushResult& result);
    /// Le masque des cases forcées, en losanges (iso) ou en carrés (à plat).
    void paintForcedMask(QPainter& painter, const CellRange& cells, bool iso);
    void applyRectangle(core::GridPosition a, core::GridPosition b);
    void copySelection();
    /// Pose le tampon au-dessus de la case survolée ; @p mirrored le reflète d'abord.
    void pasteClipboard(bool mirrored);
    [[nodiscard]] std::optional<std::pair<core::GridPosition, core::GridPosition>> highlight()
        const;
    /// Invalide le rendu du brouillon et notifie les panneaux (`draftChanged`) — après toute
    /// mutation de `_draft`.
    void markDraftMutated();
    void syncEditingState();
    void refreshDiagnostics();
    [[nodiscard]] const core::TileMap& activeLayerTiles() const;
    void handleEntityPress(const QMouseEvent* event);
    void handleEntityRelease(const QMouseEvent* event);
    /// L'outil Forme : peindre la zone sélectionnée, tracer le trajet sélectionné.
    void handleShapePress(const QMouseEvent* event);
    /// Peint ou gomme @p cell dans la zone sélectionnée, dans le geste ouvert à l'appui.
    void paintShapeAt(core::GridPosition cell);
    /// Écrit le résultat d'un glisser d'entités, en un pas.
    void applyEntityDrag(const EntityDragResult& result);
    /// @return Ce que le glisser en cours ferait, arrêté sur la case courante.
    [[nodiscard]] EntityDragResult pendingEntityDrag() const;
    /**
     * @brief Les entités du brouillon telles que le glisser en cours les laisserait.
     * @param withPlaced Ajouter en fin de liste l'entité qu'on tire, s'il y en a une.
     */
    [[nodiscard]] std::vector<core::MapEntity> previewEntities(bool withPlaced) const;
    /// Les entités par leur forme : zones, trajets, marqueurs, étiquettes, poignées et aperçu du
    /// glisser, en iso ou à plat.
    void paintEntities(QPainter& painter, const CellRange& cells, bool iso);
    /// Le verdict de la zone de combat sélectionnée : cases libres et pleines, entrées d'arène
    /// (`EX-EDIT-071`), recalculé sur l'aperçu pendant qu'on la tire.
    void paintZoneVerdict(QPainter& painter, bool iso);
    [[nodiscard]] bool hasVisualLayers() const;

    using Clock = std::chrono::steady_clock;

    QGraphicsScene* _canvasScene;
    CanvasItem* _item;
    /// Le cache d'images partagé avec les autres onglets et les vignettes (`LOT-125`).
    std::shared_ptr<SceneImages> _images;
    std::unique_ptr<DraftRenderer> _flat;
    hmi::EditorKeyBindings _editorBindings;

    core::LevelDraft _draft;
    CanvasView _view = CanvasView::Iso;
    bool _seeThroughRelief = false;
    /// Scène iso du brouillon, recomposée seulement quand il change.
    bool _isoSceneDirty = true;
    PlaceAppearance _appearance;
    std::string _appearancePlace;
    /// Vrai une fois le lieu lu : une carte sans lieu lit aussi les figurines du monde.
    bool _placeAssetsLoaded = false;
    /// Le manifeste des pièces du lieu : le brouillon en tire emprises et collision.
    std::shared_ptr<const core::ScenePieceManifest> _manifest;
    WorldSceneSnapshot _snapshot;
    ComposedScene _isoScene;
    /// Ce qu'occupe la scène composée, reliefs compris : le cadre du canevas (`LOT-125`).
    core::Rect _isoBounds;

    bool _rightDragging = false;
    QPoint _rightDragLast;
    CanvasBrush _brush;
    hmi::EditorTool _tool = hmi::EditorTool::Paint;
    /// L'outil du peintre auquel la pipette rend la main.
    hmi::EditorTool _paintTool = hmi::EditorTool::Paint;
    std::optional<MirrorAxis> _mirror;
    /// La mesure tirée (départ, arrivée), gardée jusqu'au prochain geste de l'outil Mesure.
    std::optional<std::pair<core::GridPosition, core::GridPosition>> _measure;
    EditorSidecar _sidecar;
    bool _painting = false;
    bool _dragging = false;
    core::GridPosition _dragStart{};
    core::GridPosition _dragCurrent{};
    std::optional<core::GridPosition> _hoverCell;
    /// La case d'un constat, cernée (`revealCell`).
    std::optional<core::GridPosition> _revealedCell;
    float _lastEmittedZoom = 0.0F;
    std::optional<std::pair<core::GridPosition, core::GridPosition>> _selection;
    /// Le tampon : ce que `Ctrl+C` a découpé, ou le préfabriqué armé (`LOT-EDITOR-08`).
    Stamp _clipboard;
    /// Identifiant de carte du brouillon (`capital/martpart`) : son chemin sous `Levels/`, sans
    /// extension ; son nom tant qu'il n'a jamais été ouvert ni enregistré.
    std::string _mapId;
    /// Révision du brouillon à la dernière ouverture ou au dernier enregistrement.
    std::uint64_t _savedRevision = 0;
    /// Empreinte du fichier de la carte à la dernière lecture ou écriture de l'éditeur.
    FileFingerprint _diskFingerprint;
    bool _showGrid = true;
    /// Le cadrage a été fixé pour ce contenu : un redimensionnement ne le refait pas.
    bool _framed = false;

    // --- Essai immédiat : la carte jouée par le moteur du jeu ---
    std::unique_ptr<WorldPlay> _play;
    QTimer _playTimer;
    /// Le cadrage d'édition, rendu à la fin de l'essai.
    QTransform _editTransform;
    QPointF _editCenter;
    Clock::time_point _previousFrame;
    core::FixedTimestep _timestep;
    WorldSceneSnapshot _playSnapshot;
    ComposedScene _playScene;
    /// Ce qu'occupe la scène de l'essai.
    core::Rect _playBounds;
    /// Touches de déplacement enfoncées (codes `Qt::Key`).
    std::set<int> _heldKeys;
    /// Interaction demandée depuis le dernier pas.
    bool _interactRequested = false;
    /// La scène de l'essai doit être recomposée avant la prochaine image.
    bool _playSceneDirty = true;

    // --- Couches et entités (LOT-11) ---
    LayerSlot _activeLayer;
    hmi::LayerViewState _layerView;
    bool _refusalReported = false;
    const EditorReferences* _references = nullptr;
    std::string _entityKindToPlace;
    std::optional<std::size_t> _selectedEntity;
    std::vector<std::size_t> _selectedEntities;
    /// Le glisser de l'outil Entité (ou d'un point de trajet de l'outil Forme), et sa case.
    std::optional<EntityDrag> _entityDrag;
    core::GridPosition _entityDragTo{};
    /// L'outil Forme peint (ou gomme, `Ctrl`) la zone sélectionnée, du clic au relâchement.
    bool _shapePainting = false;
    bool _shapeErasing = false;
    core::EntityReferenceContext _referenceContext;
    std::vector<core::EncounterTerrain> _terrains;
    std::vector<core::CombatZoneTerrain> _zoneVerdicts;
    std::vector<EditorDiagnostic> _diagnostics;

    // --- État de partie (LOT-126) ---
    std::vector<std::string> _stateEntries;
    bool _statePreview = false;
    /// Les drapeaux de l'état, déclarations des quêtes comprises ; relus quand l'état ou les
    /// catalogues changent.
    core::WorldFlags _stateFlags;
};

}  // namespace hmi
