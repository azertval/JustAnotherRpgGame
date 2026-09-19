// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/PieceFootprint.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Core/Resources/ScenePieceManifest.h"

/**
 * @file Core/Levels/LevelDraft.h
 * @brief Représentation mutable d'un niveau en cours d'édition.
 */

namespace core {

/// @brief Ancien nom de pièce → nouveau : un remplacement, ou la table de correspondance d'un
///        changement de planche (`LOT-EDITOR-14`).
using PieceRenaming = std::map<std::string, std::string, std::less<>>;

/**
 * @brief Carte **mutable** en cours d'édition, distincte de `Level` (immuable).
 *
 * `LevelDraft` porte toute la mutabilité nécessaire à l'éditeur (`EX-EDIT-002` à `EX-EDIT-005`) :
 * peindre une tuile, déplacer l'entrée, éditer les couches et les entités, redimensionner la
 * grille. Il ne duplique **aucune règle de validation** : `toLevel()` reconstruit le niveau en
 * repassant par `LevelLoader::loadFromString` (même chemin que le chargement d'un fichier),
 * garantissant que le niveau produit satisfait exactement les mêmes règles (`EX-LVL-004`,
 * `EX-EDIT-010`).
 *
 * Invariant maintenu par tous les mutateurs : la grille de tuiles reste la **source de vérité**
 * de la position d'entrée (comme pour `Level`) ; `entry()` n'est qu'un accès en cache, toujours
 * synchronisé avec le contenu de `tileMap()`.
 *
 * ## La collision suit les gestes (`LOT-EDITOR-03`, `EX-EDIT-064`, `EX-EDIT-065`)
 *
 * Dès qu'une carte a une couche visuelle, sa collision se **déduit** de ce qu'elle montre
 * (`core::deriveCollision`, décision D10) et s'écrit, hors des cases **forcées** à la main. Le
 * brouillon tient cet accord geste par geste, sans rien toucher d'autre :
 *
 * - un geste sur une couche visuelle (type, pièce, gomme) redéduit la collision des **seules**
 *   cases qu'il a touchées — emprises comprises —, hors cases forcées et hors entrée ;
 * - peindre la grille de collision elle-même **force** la case si la valeur peinte s'écarte de la
 *   déduction, et la **libère** si elle s'y accorde ;
 * - `unforceCollision` rend des cases forcées à la déduction.
 *
 * La déduction lit le manifeste des pièces du lieu (`setPieceManifest`) ; sans manifeste, toute
 * pièce compte pour inconnue et la case suit la règle de son type, comme au contrôle.
 *
 * Logique **pure**, sans dépendance rendu ni fenêtre — testable sans GPU (`EX-NFR-010`).
 */
class LevelDraft {
public:
    /**
     * @brief Nombre maximal de pas d'annulation gardés (`EX-EDIT-058`).
     *
     * Chaque pas est un instantané complet du brouillon : sans plafond, une longue séance sur
     * Martpart (48 × 40, trois couches) accumulait des centaines d'instantanés jamais rendus.
     * Au-delà, le pas le plus ancien est oublié.
     */
    static constexpr std::size_t UNDO_HISTORY_LIMIT = 200;

    /**
     * @brief Crée un brouillon vierge (grille entièrement `Empty`), sans entrée.
     * @param name   Nom du niveau.
     * @param width  Largeur de la grille, en cases (> 0).
     * @param height Hauteur de la grille, en cases (> 0).
     */
    [[nodiscard]] static LevelDraft empty(std::string name, int width, int height);

    /**
     * @brief Crée un brouillon à partir d'un niveau déjà chargé (édition d'un fichier existant).
     * @param level Niveau source.
     */
    [[nodiscard]] static LevelDraft fromLevel(const Level& level);

    /**
     * @brief Peint le type de tuile @p type en (column, row).
     *
     * Cas particulier : peindre `Entry` délègue à `setEntry` (unicité). Peindre un autre type sur
     * la case de l'entrée l'invalide, et peindre un type différent retire la pièce assignée à la
     * case, pour ne jamais laisser d'incohérence entre la grille et ces caches. Sur une carte à
     * couches visuelles, la case est forcée si la valeur peinte s'écarte de la déduction, libérée
     * sinon (voir l'en-tête).
     * @param column Colonne visée (doit être dans les bornes).
     * @param row    Ligne visée (doit être dans les bornes).
     * @param type   Type de tuile à poser.
     */
    void paintTile(int column, int row, TileType type);

    /**
     * @brief Applique un bloc rectangulaire de types de tuiles à partir de (@p originColumn,
     *        @p originRow).
     *
     * Repasse par la même sémantique cellule-par-cellule que `paintTile` (déplacement de
     * l'entrée, retrait des pièces assignées) pour chaque case du bloc, mais ne pousse **qu'un
     * seul** snapshot undo pour toute l'opération — sert au remplissage rectangulaire et au
     * collage (`EX-EDIT-014`), sans dupliquer de règle de niveau (`EX-EDIT-010`). Les cases du
     * bloc hors des bornes de la grille sont silencieusement ignorées (découpe aux bords, même
     * principe que `resize`).
     * @param originColumn Colonne de la case (0,0) du bloc.
     * @param originRow    Ligne de la case (0,0) du bloc.
     * @param block        Bloc de types, indexé `[ligne][colonne]` ; sans effet si vide.
     */
    void paintRegion(int originColumn, int originRow,
                     const std::vector<std::vector<TileType>>& block);

    /**
     * @brief Place l'entrée en (column, row) ; déplace l'occurrence existante s'il y en avait
     *        une (unicité, `EX-EDIT-004`).
     */
    void setEntry(int column, int row);

    /**
     * @name Couches de tuiles (`LOT-04`, `LOT-11`)
     *
     * Un rang désigne une entrée de `layers()`. Seules les couches **visuelles** — sol et décor
     * (`core::isVisualLayerKind`) — se créent, se retirent, se renomment, se déplacent et se
     * peignent ici : la grille de collision est `tileMap()`, que peignent `paintTile` et
     * `paintRegion`, et l'entrée `Collision` ou `Legacy` de `layers()` n'en est que le reflet.
     *
     * Un rang hors bornes, une couche non visuelle ou un type de tuile refusé
     * (`core::isVisualLayerTileType`) ne fait **rien** et n'empile **rien**.
     * @{
     */

    /**
     * @brief Ajoute une couche visuelle en fin de liste (la plus en avant).
     *
     * **Promotion d'une carte à grille unique.** Tant qu'une carte n'a aucune couche visuelle, sa
     * grille racine vaut à la fois image et collision (`LayerKind::Legacy`) ; dès qu'elle en a une,
     * la grille racine n'est plus dessinée. Ajouter la **première**
     * couche visuelle y recopie donc la grille racine, types refusés mis à part : ce qu'on voyait
     * reste ce qu'on voit, et l'auteur retire ensuite ce qui ne relève que de la collision. Une
     * couche ajoutée à une carte qui en a déjà naît vide.
     *
     * @param kind `Ground` ou `Decor`.
     * @param name Nom affiché par l'éditeur.
     * @return Le rang de la couche créée, ou `std::nullopt` si @p kind n'est pas visuel.
     */
    std::optional<std::size_t> addLayer(LayerKind kind, std::string name);

    /// Retire la couche visuelle au rang @p index.
    /// @return `false` si @p index est hors bornes ou désigne une couche non visuelle.
    bool removeLayer(std::size_t index);

    /// Renomme la couche visuelle au rang @p index. Un nom identique n'empile rien.
    bool renameLayer(std::size_t index, std::string name);

    /// Change le rôle (sol ↔ décor) de la couche visuelle au rang @p index.
    bool setLayerKind(std::size_t index, LayerKind kind);

    /// Assigne la propriété @p key de la couche visuelle au rang @p index — par exemple le lieu
    /// dont elle prend les pièces (`scene`, `LOT-EDITOR-06`). Une valeur identique n'empile rien.
    bool setLayerProperty(std::size_t index, const std::string& key, PropertyValue value);

    /**
     * @brief Échange la couche visuelle au rang @p index avec sa voisine visuelle, en avant
     *        (@p forward) ou en arrière. L'entrée de collision ne se franchit pas : elle n'a pas
     *        de rang de dessin.
     * @return Le nouveau rang ; @p index inchangé si la couche est déjà au bout ;
     *         `std::nullopt` si @p index est hors bornes ou non visuel.
     */
    std::optional<std::size_t> moveLayer(std::size_t index, bool forward);

    /// Peint @p type en (@p column, @p row) de la couche visuelle @p index ; la collision de la
    /// case suit.
    /// @return `false` (rien d'empilé) si le rang, la case ou le type est refusé, ou si la case
    ///         porte déjà ce type.
    bool paintLayerTile(std::size_t index, int column, int row, TileType type);

    /// Applique @p block (indexé `[ligne][colonne]`) sur la couche visuelle @p index, découpé aux
    /// bords, en **un** pas d'annulation. Un bloc contenant un type refusé est refusé en entier.
    bool paintLayerRegion(std::size_t index, int originColumn, int originRow,
                          const std::vector<std::vector<TileType>>& block);

    /** @} */

    /**
     * @name Pièces de couche (`LOT-EDITOR-03`)
     *
     * Poser une pièce écrit, **en un pas d'annulation**, sa couche, sa pièce et la collision
     * qu'elle donne (`EX-EDIT-064`). Une pièce est **ancrée** sur une case et occupe son emprise
     * (`core::footprintCells`) ; l'emprise d'une pièce que le manifeste ne connaît pas vaut 1 × 1.
     * Sur une même couche, deux emprises ne se recouvrent pas : poser une pièce retire celles
     * qu'elle couvrirait.
     * @{
     */

    /// @brief Le manifeste des pièces du lieu : emprises et types tactiques. Ni peint ni défait.
    void setPieceManifest(std::shared_ptr<const ScenePieceManifest> manifest) noexcept {
        _manifest = std::move(manifest);
    }

    /// @return Le manifeste des pièces du lieu, `nullptr` sans lieu.
    [[nodiscard]] const ScenePieceManifest* pieceManifest() const noexcept {
        return _manifest.get();
    }

    /// @return L'emprise de @p piece selon le manifeste, 1 × 1 si elle y est inconnue.
    [[nodiscard]] PieceFootprint pieceFootprint(std::string_view piece) const noexcept;

    /**
     * @return La case d'ancrage de la pièce de la couche @p index dont l'emprise couvre @p cell
     *         (elle-même si la case nomme sa pièce), ou `std::nullopt` si aucune ne la couvre.
     */
    [[nodiscard]] std::optional<GridPosition> pieceAnchorAt(std::size_t index,
                                                            GridPosition cell) const;

    /**
     * @brief Pose @p piece ancrée en @p anchor sur la couche visuelle @p index, la case d'ancrage
     *        prenant le type @p type ; les pièces que son emprise couvrirait sont retirées, type
     *        compris. La collision des cases touchées suit.
     * @return `false` (rien d'empilé) si le rang, la case ou le nom est refusé, si l'emprise
     *         déborde de la carte, ou si la case porte déjà cette pièce et ce type.
     */
    bool placePiece(std::size_t index, GridPosition anchor, const std::string& piece,
                    TileType type);

    /**
     * @brief Pave de @p piece le rectangle [@p first, @p last] (bornes incluses) de la couche
     *        @p index, au pas de son emprise, en **un** pas d'annulation. Une pièce dont
     *        l'emprise déborderait du rectangle n'est pas posée.
     * @return `false` si rien n'a changé.
     */
    bool placePieceRegion(std::size_t index, GridPosition first, GridPosition last,
                          const std::string& piece, TileType type);

    /**
     * @brief Gomme les cases du rectangle [@p first, @p last] de la couche visuelle @p index : une
     *        case couverte par une pièce retire la pièce **entière** (type de sa case d'ancrage
     *        compris), une case sans pièce perd son type. La collision suit, en un pas.
     * @return `false` si rien n'a changé.
     */
    bool eraseLayerRegion(std::size_t index, GridPosition first, GridPosition last);

    /**
     * @brief Rend les cases forcées de @p cells à la déduction (`EX-EDIT-065`), en un pas.
     * @return `false` si aucune n'était forcée.
     */
    bool unforceCollision(const std::vector<GridPosition>& cells);

    /// @return Vrai si la collision de @p cell est forcée à la main.
    [[nodiscard]] bool isCollisionForced(GridPosition cell) const noexcept;

    /**
     * @brief Renomme, sur toutes les couches visuelles, chaque pièce que @p renaming nomme
     *        (`LOT-EDITOR-14`, `EX-EDIT-083`), en **un** pas d'annulation.
     *
     * La pièce garde sa case d'ancrage et son type (décision D3) ; la collision des cases que
     * couvrent l'ancienne et la nouvelle emprise suit.
     * @return `false` (rien d'empilé) si rien ne change, ou si une nouvelle emprise déborderait de
     *         la carte.
     */
    bool replacePieces(const PieceRenaming& renaming);

    /**
     * @brief Fait passer la carte à la planche du lieu @p place (`LOT-EDITOR-14`), en **un** pas :
     *        la propriété `scene` des couches qui la portent (la première couche de sol, si aucune
     *        ne la porte), le manifeste @p manifest, et les pièces renommées par @p renaming.
     *
     * Une pièce que @p renaming ne nomme pas garde son nom. Toute la collision est redéduite par
     * le nouveau manifeste, hors cases forcées et hors entrée.
     * @return `false` si rien ne change, si la carte n'a pas de couche visuelle, ou si une emprise
     *         déborderait de la carte.
     */
    bool changeScene(const std::string& place, std::shared_ptr<const ScenePieceManifest> manifest,
                     const PieceRenaming& renaming);

    /** @} */

    /**
     * @name Entités de carte (`LOT-04`, `LOT-11`)
     *
     * Un rang désigne une entrée de `entities()`. Plusieurs entités peuvent partager une case ;
     * `entityAt` rend la **dernière** posée, celle qu'on voit au-dessus. Chaque mutateur empile un
     * pas d'annulation, sauf s'il est refusé ou sans effet.
     * @{
     */

    /// Pose @p entity en fin de liste. Une entité sans identifiant en reçoit un neuf
    /// (`core::entityIdFor`), jamais donné auparavant dans cette carte (décision D8).
    /// @return Son rang, ou `std::nullopt` si sa case est hors de la grille (`EX-LVL-017`).
    std::optional<std::size_t> placeEntity(MapEntity entity);

    /// Déplace l'entité @p index en @p position. Refusé hors bornes ; sans effet sur place.
    bool moveEntity(std::size_t index, GridPosition position);

    /**
     * @brief Remplace l'entité @p index par @p entity : sa case, sa forme (`cells`), ses
     *        propriétés, en un pas (`LOT-EDITOR-05`).
     *
     * Le type et l'identifiant restent ceux de l'entité en place : ceux de @p entity sont ignorés.
     * Un identifiant ne change jamais (décision D8), et changer de famille n'est pas un geste de
     * l'éditeur. Refusé si la case ou une case de `cells` sort de la grille ; sans effet si rien ne
     * change.
     */
    bool replaceEntity(std::size_t index, MapEntity entity);

    /// Retire l'entité @p index.
    bool removeEntity(std::size_t index);

    /// Assigne la propriété @p key de l'entité @p index. Une valeur identique n'empile rien.
    bool setEntityProperty(std::size_t index, const std::string& key, PropertyValue value);

    /// Retire la propriété @p key de l'entité @p index, si elle l'a.
    bool removeEntityProperty(std::size_t index, const std::string& key);

    /// @return Le rang de la dernière entité posée en @p position, s'il y en a une.
    [[nodiscard]] std::optional<std::size_t> entityAt(GridPosition position) const;

    /** @} */

    /**
     * @brief Redimensionne la grille (`EX-EDIT-005`).
     *
     * Agrandir complète les nouvelles cases en `Empty` ; réduire **tronque** silencieusement le
     * contenu hors des nouvelles bornes (entrée, entités ou pièces assignées perdues sont
     * invalidées en conséquence). L'avertissement de perte revient à l'appelant `HMI`, avant
     * d'invoquer `resize`.
     * @param width  Nouvelle largeur, en cases (> 0).
     * @param height Nouvelle hauteur, en cases (> 0).
     */
    void resize(int width, int height);

    /**
     * @brief Indique si redimensionner à (@p width, @p height) supprimerait du contenu déjà posé.
     *
     * Requête **pure** (n'altère rien) : vraie si les nouvelles bornes excluraient l'entrée, une
     * entité ou une pièce assignée actuellement posées. Permet à
     * `HMI` d'avertir avant d'appeler `resize` (`EX-EDIT-012`), sans dupliquer la logique de
     * troncature déjà portée par `resize`.
     * @param width  Largeur envisagée, en cases (> 0).
     * @param height Hauteur envisagée, en cases (> 0).
     * @return `true` si l'entrée, une entité ou une pièce assignée serait perdue.
     */
    [[nodiscard]] bool wouldResizeDropContent(int width, int height) const noexcept;

    /**
     * @name Gestes (`LOT-EDITOR-04`)
     *
     * Un geste de l'auteur — un glisser du pinceau, une ligne, un seau, un trait et son reflet —
     * enchaîne plusieurs mutations ; il se défait pourtant **en un pas**. Entre `beginGesture` et
     * `endGesture`, seule la première mutation qui change la carte empile un pas d'annulation ; un
     * geste sans effet n'en empile aucun. Les appels s'imbriquent : seul le plus extérieur ferme le
     * geste. La révision change à chaque mutation, comme hors geste.
     * @{
     */

    /// Ouvre un geste.
    void beginGesture() noexcept;

    /// Ferme le geste ouvert ; sans effet s'il n'y en a pas.
    void endGesture() noexcept;

    /// @return Vrai si un geste est ouvert.
    [[nodiscard]] bool inGesture() const noexcept {
        return _gestureDepth > 0;
    }

    /** @} */

    /**
     * @brief Annule la dernière mutation (`EX-EDIT-005`).
     * @return `true` si une mutation a été annulée, `false` si l'historique était vide.
     */
    bool undo();

    /**
     * @brief Refait la dernière mutation annulée.
     *
     * Toute nouvelle mutation après un `undo()` invalide la branche de refaire (historique
     * linéaire classique) : `redo()` redevient sans effet tant qu'aucun nouvel `undo()` n'a eu
     * lieu depuis.
     * @return `true` si une mutation a été refaite, `false` si rien n'était à refaire.
     */
    bool redo();

    /// @return Le nombre de pas que `undo()` peut défaire (au plus `UNDO_HISTORY_LIMIT`).
    [[nodiscard]] std::size_t undoDepth() const noexcept {
        return _undoHistory.size();
    }

    /**
     * @brief Identifie l'état du contenu : change à chaque mutation, et revient à sa valeur
     *        d'avant quand `undo()` défait la mutation (et inversement pour `redo()`,
     * `EX-EDIT-058`).
     *
     * L'éditeur compare cette révision à celle de la dernière ouverture ou du dernier
     * enregistrement pour dire si le brouillon porte des modifications : défaire jusqu'à l'état
     * enregistré rend un brouillon propre, et rien ne le marque modifié sans mutation réelle.
     */
    [[nodiscard]] std::uint64_t revision() const noexcept {
        return _revision;
    }

    /// @return `true` si `undo()` aurait un effet.
    [[nodiscard]] bool canUndo() const noexcept {
        return !_undoHistory.empty();
    }

    /// @return `true` si `redo()` aurait un effet.
    [[nodiscard]] bool canRedo() const noexcept {
        return !_redoHistory.empty();
    }

    /// Renomme le niveau.
    void setName(std::string name) {
        _name = std::move(name);
    }

    /// @return Le nom courant du niveau.
    [[nodiscard]] const std::string& name() const noexcept {
        return _name;
    }

    /// @return La grille de tuiles courante.
    [[nodiscard]] const TileMap& tileMap() const noexcept {
        return _tileMap;
    }

    /// @return La position d'entrée, si elle est posée.
    [[nodiscard]] std::optional<GridPosition> entry() const noexcept {
        return _entry;
    }

    /// @return Les cases de collision forcées à la main (`core::LevelData::forcedCollision`).
    [[nodiscard]] const std::vector<GridPosition>& forcedCollision() const noexcept {
        return _forcedCollision;
    }

    /// @return Le prochain identifiant d'entité que `placeEntity` donnera.
    [[nodiscard]] int nextEntityId() const noexcept {
        return _nextEntityId;
    }

    /// @return Les couches de tuiles du niveau (`LOT-04`), dans leur ordre de superposition.
    ///
    /// Une entrée `Collision` ou `Legacy` y reflète la grille racine telle que le fichier l'a
    /// promue ; c'est `tileMap()` qui fait foi, et `toLevel()` l'y recopie. Les couches visuelles
    /// s'éditent par les mutateurs de couches (`LOT-11`).
    [[nodiscard]] const std::vector<TileLayer>& layers() const noexcept {
        return _layers;
    }

    /// @return Les entités placées sur la carte (`LOT-04`), éditées par les mutateurs d'entités.
    [[nodiscard]] const std::vector<MapEntity>& entities() const noexcept {
        return _entities;
    }

    /**
     * @brief Convertit le brouillon en `Level` **validé** (`EX-EDIT-007`), en repassant par la
     *        même validation que `LevelLoader` (sérialise puis recharge : aucune règle
     *        dupliquée, `EX-EDIT-010`).
     * @return Un résultat récupérable : niveau valide, ou message d'erreur exploitable si le
     *         brouillon est incomplet (pas d'entrée, …).
     */
    [[nodiscard]] LevelLoadResult toLevel() const;

    /**
     * @brief Le brouillon en JSON, **sans validation** : ce que `toLevel()` relit.
     *
     * Sert à la sauvegarde automatique (`LOT-EDITOR-01`), qui doit garder un brouillon même
     * incomplet (sans entrée, par exemple).
     */
    [[nodiscard]] std::string toJson() const;

private:
    LevelDraft(std::string name, TileMap tileMap);

    /// Logique de `paintTile`, sans `pushUndo()` : réutilisée cellule par cellule par
    /// `paintRegion` pour n'empiler qu'un seul snapshot par opération de bloc.
    void paintTileInternal(int column, int row, TileType type);

    /// Vrai si @p index désigne une couche visuelle de `_layers`.
    [[nodiscard]] bool isVisualLayerIndex(std::size_t index) const noexcept;

    /// Vrai si @p index désigne une entité de `_entities`.
    [[nodiscard]] bool isEntityIndex(std::size_t index) const noexcept {
        return index < _entities.size();
    }

    /// Logique de `setEntry`, sans `pushUndo()`.
    void setEntryInternal(int column, int row);

    /// État complet du brouillon, hors historique (utilisé pour les snapshots undo/redo).
    struct State {
        std::string name;
        TileMap tileMap;
        std::optional<GridPosition> entry;
        std::vector<TileLayer> layers;
        std::vector<MapEntity> entities;
        std::vector<GridPosition> forcedCollision;
        std::uint64_t revision = 0;
    };

    /// Capture l'état courant (pour empiler dans l'historique undo/redo).
    [[nodiscard]] State snapshot() const;

    /// Restitue un état capturé précédemment.
    void restore(State state);

    /// Empile l'état courant sur la pile d'annulation ; à appeler avant toute mutation
    /// undoable. Une nouvelle mutation invalide toujours la branche de refaire, oublie le pas le
    /// plus ancien au-delà de `UNDO_HISTORY_LIMIT` et donne au contenu une révision neuve.
    void pushUndo();

    /// Vrai si peindre @p type en (@p column, @p row) changerait la carte.
    [[nodiscard]] bool paintChanges(int column, int row, TileType type) const;

    /// Vrai si la carte a une couche visuelle : sa collision se déduit alors de ses couches.
    [[nodiscard]] bool derivesCollision() const noexcept;

    /// Redéduit la collision de @p cells, hors cases forcées et hors entrée (voir l'en-tête).
    void followCollision(const std::vector<GridPosition>& cells);

    /// Après une peinture de la grille de collision : force chaque case de @p cells qui s'écarte
    /// de la déduction, libère celle qui s'y accorde.
    void updateForcing(const std::vector<GridPosition>& cells);

    /// Retire de la couche @p index la pièce ancrée en @p anchor, type de l'ancre compris ; ajoute
    /// ses cases à @p touched.
    void removePieceInternal(std::size_t index, GridPosition anchor,
                             std::vector<GridPosition>& touched);

    /// Logique de `placePiece`, sans `pushUndo()` ; ajoute les cases touchées à @p touched.
    void placePieceInternal(std::size_t index, GridPosition anchor, const std::string& piece,
                            TileType type, std::vector<GridPosition>& touched);

    /// Vrai si l'emprise de @p piece ancrée en @p anchor tient dans la carte.
    [[nodiscard]] bool pieceFits(GridPosition anchor, std::string_view piece) const noexcept;

    std::string _name;
    TileMap _tileMap;
    std::optional<GridPosition> _entry;
    std::vector<TileLayer> _layers;
    std::vector<MapEntity> _entities;
    std::vector<GridPosition> _forcedCollision;
    /// Compteur d'identifiants : hors des instantanés, il ne recule jamais (décision D8).
    int _nextEntityId = 1;
    /// Base et planche d'une variante (décision D12) : ni peintes ni défaites, recopiées.
    std::string _base;
    std::string _scene;
    /// Manifeste des pièces du lieu, partagé : copier un brouillon ne le recopie pas.
    std::shared_ptr<const ScenePieceManifest> _manifest;
    std::vector<State> _undoHistory;
    std::vector<State> _redoHistory;
    std::uint64_t _revision = 0;
    /// Dernière révision donnée : jamais réemployée, même après un `undo()`.
    std::uint64_t _lastRevision = 0;
    /// Profondeur des gestes ouverts (`beginGesture`).
    int _gestureDepth = 0;
    /// Le geste ouvert a déjà empilé son pas d'annulation.
    bool _gesturePushed = false;
};

/// @brief Un geste ouvert le temps d'une portée (`LevelDraft::beginGesture`).
class GestureScope {
public:
    explicit GestureScope(LevelDraft& draft) noexcept : _draft(draft) {
        _draft.beginGesture();
    }
    ~GestureScope() {
        _draft.endGesture();
    }
    GestureScope(const GestureScope&) = delete;
    GestureScope& operator=(const GestureScope&) = delete;
    GestureScope(GestureScope&&) = delete;
    GestureScope& operator=(GestureScope&&) = delete;

private:
    LevelDraft& _draft;
};

}  // namespace core
