// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/TileType.h"
#include "Editor/Logic/BrushGesture.h"
#include "Editor/Logic/EditorSidecar.h"
#include "Editor/Logic/LayerView.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/PaintTools.h"
#include "Editor/Logic/Stamps.h"

namespace core {
class LevelDraft;
}

/**
 * @file Editor/Logic/GestureScript.h
 * @brief L'éditeur **sans fenêtre** : rejouer une liste de gestes sur une carte
 *        (`LevelEditor --apply gestes.json`, `LOT-EDITOR-13`, décision D9, `EX-EDIT-074`).
 *
 * Ce que les scripts des cartes apportaient — l'édition en masse, rejouable, relue en diff
 * (constat A11) — sans les scripts. Un fichier de gestes décrit ce que la main ferait dans la
 * fenêtre : un **outil**, un **appui** sur une case, un **glisser**, un **relâchement**, et entre
 * deux gestes ce qu'on arme (une pièce de la palette, une couche, le miroir, une famille
 * d'entité). L'exécuteur appelle **les fonctions mêmes** que le canevas appelle à la souris
 * (`hmi::applyStroke`, `hmi::applyRectangleStroke`, `hmi::applyBucket`, `hmi::pickBrush`,
 * `hmi::resolveEntityPress`, `hmi::dragEntities`, `hmi::applyEntityDrag`…), dans le même ordre
 * (règle 4 de la feuille de route) : un geste rejoué rend le fichier que le geste à la souris
 * aurait rendu, et chaque geste est un pas d'annulation, comme dans la fenêtre.
 *
 * ## Le fichier
 *
 * @code{.json}
 * {
 *   "format": "jadg-editor-gestures",
 *   "version": 1,
 *   "map": "capital/martpart",
 *   "gestures": [
 *     {"piece": "street-2", "tool": "paint", "path": [[1, 3], [4, 0]]},
 *     {"tool": "line", "piece": "wall-right", "from": [0, 2], "to": [12, 2]},
 *     {"tool": "entity", "kind": "chest", "at": [5, 4]},
 *     {"tool": "entity", "select": ["e3"], "set": {"name": "coffre-du-marchand"}}
 *   ]
 * }
 * @endcode
 *
 * Une case s'écrit `[colonne, ligne]`. Chaque geste peut d'abord **armer**, dans cet ordre :
 * `lock` et `unlock` (noms de couches), `layer` (la couche active, par son nom, ou `collision`),
 * `mirror` (`[c, r]` : l'axe qui passe par cette case ; `false` : plus de miroir), `type` (un type
 * de tuile) ou `piece` (une pièce de la planche du lieu, `floor` la dit sol si la planche ne la
 * connaît pas), `kind` (la famille d'entité à poser ; `""` : l'outil ne fait que sélectionner),
 * `select` (des identifiants d'entité), `prefab` (un préfabriqué de la bibliothèque du lieu, qui
 * devient le tampon à poser). Un geste sans `tool` ne fait qu'armer. Ce qui est armé le
 * reste pour les gestes suivants, comme dans la fenêtre.
 *
 * | `tool` | champs | ce que fait la main |
 * |---|---|---|
 * | `paint` | `path` | appui sur la première case, glisser par les suivantes |
 * | `eraser` | `path` | la gomme, de même, sans désarmer le pinceau |
 * | `rectangle`, `line` | `from`, `to` | tirer de `from` à `to` |
 * | `bucket` | `at` | un clic |
 * | `pipette` | `at` | un clic : le pinceau pris est armé |
 * | `selection` | `from`, `to`, `then` | tirer ; `then` : `copy` (`Ctrl+C`) ou `delete` (`Suppr`) |
 * | `paste` | `at`, `flip` | `Ctrl+V`, coin haut gauche sur la case ; `flip` : le tampon reflété |
 * | `entity` | `at`, `to`, `ctrl`, `shift`, `set`, `then` | appui, glisser jusqu'à `to` ; `set` :
 * propriétés de l'entité sélectionnée ; `then: "delete"` : `Suppr` | | `shape` | `path` ou `at`,
 * `to`, `ctrl` | l'outil Forme sur l'entité sélectionnée | | `measure` | `from`, `to` | la mesure,
 * écrite dans le compte rendu | | `note` | `at`, `text` | la note de la case (vide : retirée) |
 *
 * ## Un geste refusé
 *
 * Un geste que la fenêtre refuserait (couche verrouillée, case hors de la carte, pièce inconnue,
 * entité qui sortirait de la carte, identifiant qui n'existe pas…) arrête tout : l'exécution rend
 * une erreur lisible, qui nomme le geste, et **le fichier n'est pas touché**. Un geste qui ne
 * change rien n'est pas une erreur.
 */

namespace hmi {

/// @brief Nom du format d'un fichier de gestes (champ `format`).
inline constexpr std::string_view GESTURE_SCRIPT_FORMAT = "jadg-editor-gestures";

/// @brief Version du format d'un fichier de gestes (champ `version`).
inline constexpr int GESTURE_SCRIPT_VERSION = 1;

/**
 * @brief Ce que la fenêtre garde d'un geste à l'autre : le pinceau, la couche active, le miroir,
 *        la sélection… — l'état de l'éditeur que les gestes arment.
 */
struct GestureState {
    CanvasBrush brush;
    LayerSlot activeLayer;
    LayerViewState view;
    std::optional<MirrorAxis> mirror;
    /// La famille d'entité que l'outil Entité pose ; vide : il ne fait que sélectionner.
    std::string kindToPlace;
    /// Les entités sélectionnées (rangs triés) et la principale.
    std::vector<std::size_t> selectedEntities;
    std::optional<std::size_t> selectedEntity;
    /// La région de l'outil Sélection, coins triés.
    std::optional<std::pair<core::GridPosition, core::GridPosition>> selection;
    /// Ce que `Ctrl+C` a copié, ou le préfabriqué armé (`LOT-EDITOR-08`).
    Stamp clipboard;
};

/// @brief Ce qu'un fichier de gestes a fait.
struct GestureScriptResult {
    /// Le message d'échec, qui nomme le geste ; vide si tout a été rejoué.
    std::string error;
    /// Nombre de gestes rejoués.
    std::size_t gestures = 0;
    /// Nombre de pas d'annulation que les gestes ont empilés : un geste qui change la carte en est
    /// un.
    std::size_t steps = 0;
    /// Les notes d'auteur ont changé : l'annexe est à réécrire.
    bool notesChanged = false;
    /// Ce que les gestes ont à dire (mesures) : une ligne par constat.
    std::vector<std::string> log;

    [[nodiscard]] bool ok() const noexcept {
        return error.empty();
    }
};

/**
 * @brief Rejoue les gestes de @p script sur @p draft.
 *
 * Le brouillon doit porter le manifeste du lieu (`core::LevelDraft::setPieceManifest`) : la
 * collision suit chaque geste par la déduction, comme dans la fenêtre. En cas d'échec, @p draft
 * et @p sidecar sont dans l'état du geste refusé ; l'appelant les jette.
 *
 * @param script  Le fichier de gestes, lu.
 * @param draft   La carte à modifier.
 * @param sidecar Son annexe (les notes).
 * @param assets  Le manifeste et la table de son lieu.
 * @param dataRoot La racine des données, où vit la bibliothèque de préfabriqués ; vide, un geste
 *                 qui arme un `prefab` est refusé.
 */
[[nodiscard]] GestureScriptResult applyGestureScript(const nlohmann::json& script,
                                                     core::LevelDraft& draft,
                                                     EditorSidecar& sidecar,
                                                     const PlaceAssets& assets,
                                                     const std::filesystem::path& dataRoot = {});

/// @brief Ce que `--apply` a produit : le texte de la carte et celui de son annexe.
struct GestureFileResult {
    GestureScriptResult script;
    /// Le texte canonique de la carte modifiée, vide en cas d'échec.
    std::string mapText;
    /// L'annexe, si les notes ont changé : à écrire par `hmi::writeSidecar`, qui retire le
    /// fichier d'une annexe vidée.
    std::optional<EditorSidecar> sidecar;
    /// L'identifiant de la carte, lu dans le fichier de gestes ou donné par l'appelant.
    std::string mapId;
};

/**
 * @brief Lit le fichier de gestes @p scriptFile, charge la carte, rejoue les gestes et rend les
 *        textes à écrire — sans rien écrire.
 * @param scriptFile Le fichier de gestes.
 * @param map        La carte (identifiant ou chemin) ; vide : celle que nomme le champ `map`.
 * @param dataRoot   La racine des données.
 * @param mapFile    Reçoit le fichier de la carte.
 */
[[nodiscard]] GestureFileResult applyGestureFile(const std::filesystem::path& scriptFile,
                                                 std::string_view map,
                                                 const std::filesystem::path& dataRoot,
                                                 std::filesystem::path& mapFile);

}  // namespace hmi
