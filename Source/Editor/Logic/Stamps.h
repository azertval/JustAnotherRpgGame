// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileType.h"
#include "Editor/Logic/LayerView.h"

/**
 * @file Editor/Logic/Stamps.h
 * @brief Le **tampon** : un morceau de carte qu'on découpe, qu'on repose ailleurs et qu'on garde
 *        comme **préfabriqué** (`LOT-EDITOR-08`, `EX-EDIT-085`, `EX-EDIT-086`).
 *
 * Un tampon (`hmi::Stamp`) est un rectangle de carte **complet** : les types de chaque couche
 * visuelle, les pièces qui y sont ancrées, les entités qui s'y tiennent, et les cases dont la
 * collision est forcée à la main. `Ctrl+C` le découpe (`hmi::cutStamp`), `Ctrl+V` le repose
 * (`hmi::pasteStamp`), et il s'écrit dans la bibliothèque du lieu pour revenir plus tard
 * (`hmi::writePrefab`). Logique **pure** : le canevas l'appelle à la souris, l'éditeur sans
 * fenêtre la rejoue (règle 4 de la feuille de route).
 *
 * ## Ce que le découpage prend
 *
 * - **Une pièce est prise entière ou pas du tout** : elle l'est si sa case d'**ancrage** est dans
 *   le rectangle. Le rectangle du tampon s'agrandit alors jusqu'à contenir toute son emprise —
 *   sans quoi un étal 2 × 1 choisi au bord perdrait sa moitié droite. Une pièce qui couvre le
 *   rectangle mais est ancrée dehors n'est pas prise : elle appartient à ce qu'on laisse.
 * - **Une entité est prise si sa case est dans le rectangle**, avec sa forme entière (`cells`),
 *   qui peut déborder : une zone est à elle-même sa propre étendue, et le rectangle ne s'agrandit
 *   pas pour elle. Son **identifiant ne suit pas** : le tampon ne porte que ce qu'elle est, et
 *   chaque pose en donne un neuf (décision D8).
 * - **Les cases forcées** gardent leur valeur de collision. L'**entrée** n'est jamais prise : elle
 *   est unique dans une carte, et deux poses la déplaceraient sans qu'on l'ait demandé.
 *
 * ## Ce que la pose écrit
 *
 * Un tampon **remplace** ce qu'il couvre, il ne s'y ajoute pas : chaque case de son rectangle
 * reçoit le type qu'il porte, vide compris. C'est ce qu'on attend d'un tampon, et c'est la seule
 * règle qui rende une pose prévisible. Toute la pose est **un geste** : un `Ctrl+Z` la défait
 * (`core::GestureScope`, `EX-EDIT-066`).
 *
 * Une couche du tampon va à la couche **de même nom** de la carte ; à défaut, à la première couche
 * visuelle de même rôle ; à défaut, la pose est refusée. Une couche verrouillée la refuse aussi,
 * comme pour le pinceau. Ce qui déborde de la carte est découpé aux bords ; un tampon qui
 * tomberait entièrement dehors est refusé.
 *
 * ## Le miroir
 *
 * `hmi::mirrorStamp` reflète le tampon dans **son propre** miroir — la diagonale de son coin haut
 * gauche, verticale à l'écran iso, celle que connaissent les planches (`hmi::MirrorAxis`) : la
 * case (c, r) passe en (r, c), un tampon de w × h devient h × w, et chaque pièce prend sa jumelle
 * (`hmi::mirrorPieceName`). Le miroir **armé** du canevas, lui, ne s'applique pas à une pose : un
 * tampon est déjà une composition, et la refléter au loin la poserait ailleurs qu'au curseur.
 */

namespace core {
class LevelDraft;
class ScenePieceManifest;
}  // namespace core

namespace hmi {

/// @brief Une pièce du tampon, ancrée en coordonnées **relatives** à son coin haut gauche.
struct StampPiece {
    core::GridPosition anchor{};
    std::string piece;
    /// Le type qu'écrit sa case d'ancrage (`hmi::pieceCellType`), tel qu'il était sur la carte.
    core::TileType type = core::TileType::Empty;

    [[nodiscard]] bool operator==(const StampPiece&) const = default;
};

/// @brief Une couche visuelle du tampon.
struct StampLayer {
    /// Le nom de la couche d'où elle vient : la pose cherche ce nom d'abord.
    std::string name;
    core::LayerKind kind = core::LayerKind::Ground;
    /// L'étage de la couche (`LOT-129`) : un toit reste un toit une fois posé.
    int floor = 0;
    /// Les types, denses, ligne par ligne (`width` × `height`).
    std::vector<core::TileType> types;
    /// Les pièces ancrées dans le tampon, dans l'ordre où la carte les range.
    std::vector<StampPiece> pieces;

    [[nodiscard]] bool operator==(const StampLayer&) const = default;
};

/// @brief Une case dont la collision est forcée à la main, et la valeur qu'elle force.
struct StampForcedCell {
    core::GridPosition cell{};
    core::TileType type = core::TileType::Solid;

    [[nodiscard]] bool operator==(const StampForcedCell&) const = default;
};

/// @brief Un morceau de carte : le rectangle, ses couches, ses entités, ses cases forcées.
struct Stamp {
    int width = 0;
    int height = 0;
    /// Le lieu dont il tient ses pièces (`scene`), vide pour un tampon sans pièce.
    std::string place;
    std::vector<StampLayer> layers;
    /// Les entités, en coordonnées relatives, **sans identifiant**.
    std::vector<core::MapEntity> entities;
    std::vector<StampForcedCell> forced;

    /// @return Vrai si le tampon n'a rien à poser.
    [[nodiscard]] bool empty() const noexcept {
        return width <= 0 || height <= 0 || (layers.empty() && entities.empty());
    }

    [[nodiscard]] bool operator==(const Stamp&) const = default;
};

/**
 * @brief Découpe le rectangle [@p first, @p last] (bornes incluses, dans n'importe quel ordre) de
 *        @p draft, agrandi jusqu'à contenir l'emprise de chaque pièce prise (voir l'en-tête).
 * @return Le tampon ; vide si le rectangle ne rencontre pas la carte.
 */
[[nodiscard]] Stamp cutStamp(const core::LevelDraft& draft, core::GridPosition first,
                             core::GridPosition last);

/// @return Le reflet de @p stamp dans son propre miroir ; @p manifest donne les jumelles des
///         pièces (`nullptr` : chaque pièce garde son nom).
[[nodiscard]] Stamp mirrorStamp(const Stamp& stamp, const core::ScenePieceManifest* manifest);

/// @brief Ce qu'une pose a fait.
struct StampPasteResult {
    /// La carte a changé (un pas d'annulation de plus).
    bool changed = false;
    /// Le refus, en anglais, pour la barre d'état ; vide sinon.
    std::string refusal;
    /// Les rangs des entités posées, dans l'ordre : ce que la fenêtre sélectionne ensuite.
    std::vector<std::size_t> entities;

    [[nodiscard]] bool operator==(const StampPasteResult&) const = default;
};

/**
 * @brief Pose @p stamp, son coin haut gauche en @p at, en **un** geste (voir l'en-tête).
 * @param draft La carte.
 * @param stamp Le tampon.
 * @param at    La case du coin haut gauche.
 * @param view  Les réglages des couches (verrous).
 */
StampPasteResult pasteStamp(core::LevelDraft& draft, const Stamp& stamp, core::GridPosition at,
                            const LayerViewState& view);

/// @return Ce que la barre d'état dit d'un tampon : `3 × 2 · 2 pieces · 1 entity`.
[[nodiscard]] std::string stampLabel(const Stamp& stamp);

// --- La bibliothèque de préfabriqués ------------------------------------------------------------

/// @brief Nom du format d'un préfabriqué (champ `format`).
inline constexpr std::string_view PREFAB_FORMAT = "jadg-editor-prefab";

/// @brief Version du format d'un préfabriqué (champ `version`).
inline constexpr int PREFAB_VERSION = 1;

/// @brief Nom du format d'un modèle de carte (champ `format`).
inline constexpr std::string_view MAP_TEMPLATE_FORMAT = "jadg-editor-map-template";

/// @brief Version du format d'un modèle de carte (champ `version`).
inline constexpr int MAP_TEMPLATE_VERSION = 1;

/// @return Le tampon en JSON, tel que la bibliothèque l'écrit.
[[nodiscard]] nlohmann::json stampToJson(const Stamp& stamp);

/// @brief Relit un tampon écrit par `hmi::stampToJson`.
/// @param json  Le tampon, lu.
/// @param error Reçoit le motif du refus ; vide si la lecture a réussi.
[[nodiscard]] std::optional<Stamp> stampFromJson(const nlohmann::json& json, std::string& error);

/// @return Le dossier des préfabriqués du lieu @p place :
///         `<dataRoot>/Editor/Prefabs/<place>/`.
[[nodiscard]] std::filesystem::path prefabsDir(const std::filesystem::path& dataRoot,
                                               std::string_view place);

/// @return Les noms des préfabriqués du lieu @p place, triés (vide si le dossier n'existe pas).
[[nodiscard]] std::vector<std::string> prefabNames(const std::filesystem::path& dataRoot,
                                                   std::string_view place);

/// @brief Écrit @p stamp comme préfabriqué @p name du lieu @p place, dossier créé au besoin.
/// @return Le motif du refus (nom vide ou invalide, tampon vide, écriture impossible) ; vide si
///         le fichier a été écrit.
[[nodiscard]] std::string writePrefab(const std::filesystem::path& dataRoot, std::string_view place,
                                      std::string_view name, const Stamp& stamp);

/// @brief Relit le préfabriqué @p name du lieu @p place.
/// @param dataRoot La racine des données.
/// @param place    Le lieu dont il vient.
/// @param name     Son nom, sans extension.
/// @param error    Reçoit le motif du refus ; vide si la lecture a réussi.
[[nodiscard]] std::optional<Stamp> readPrefab(const std::filesystem::path& dataRoot,
                                              std::string_view place, std::string_view name,
                                              std::string& error);

/// @return Vrai si @p name peut nommer un préfabriqué : non vide, et fait de lettres ASCII
///         minuscules, de chiffres, de tirets et de tirets bas — ce qui en fait un nom de fichier
///         sûr sur tout poste.
[[nodiscard]] bool isValidPrefabName(std::string_view name);

// --- Les modèles de carte -----------------------------------------------------------------------

/// @brief Une couche que le modèle donne à la carte neuve.
struct MapTemplateLayer {
    std::string name;
    core::LayerKind kind = core::LayerKind::Ground;
    /// Cette couche porte la propriété `scene` : c'est elle qui nomme le lieu.
    bool scene = false;

    [[nodiscard]] bool operator==(const MapTemplateLayer&) const = default;
};

/**
 * @brief Un **modèle de carte** : ce dont part une carte neuve (`EX-EDIT-087`).
 *
 * Un modèle ne nomme **aucune pièce** : il sert à tous les lieux, et une pièce n'existe que dans
 * la planche d'un lieu. Son tampon ne pose donc que des types et, s'il en a, des entités.
 */
struct MapTemplate {
    /// L'identifiant, celui du fichier : `street`.
    std::string id;
    /// Le libellé montré au choix, en anglais : `Street`.
    std::string label;
    /// Ce qu'il fait, en une ligne.
    std::string description;
    int width = 24;
    int height = 24;
    std::vector<MapTemplateLayer> layers;
    /// La case d'entrée de la carte neuve ; par défaut le coin bas gauche. Un modèle qui dresse un
    /// mur sur ce coin doit en nommer une autre : l'entrée ne se pose pas dans un mur.
    std::optional<core::GridPosition> entry;
    /// Ce qu'il pose, coin haut gauche sur la carte ; vide s'il ne pose rien.
    Stamp stamp;

    [[nodiscard]] bool operator==(const MapTemplate&) const = default;
};

/// @return Les modèles de `<dataRoot>/Editor/Templates/*.json`, triés par identifiant ; les
///         fichiers illisibles sont passés (`--check` les nomme).
[[nodiscard]] std::vector<MapTemplate> mapTemplates(const std::filesystem::path& dataRoot);

/// @brief Relit un modèle écrit dans le format `jadg-editor-map-template`.
/// @param json  Le modèle, lu.
/// @param error Reçoit le motif du refus ; vide si la lecture a réussi.
[[nodiscard]] std::optional<MapTemplate> mapTemplateFromJson(const nlohmann::json& json,
                                                             std::string& error);

/// @brief Un constat sur la bibliothèque : un préfabriqué ou un modèle qui ne se lit pas.
struct LibraryFinding {
    std::filesystem::path file;
    std::string message;

    [[nodiscard]] bool operator==(const LibraryFinding&) const = default;
};

/// @return Les fichiers de `<dataRoot>/Editor/` que l'éditeur ne sait pas relire — ce que
///         `LevelEditor --check` ajoute au contrôle des cartes (`EX-EDIT-086`).
[[nodiscard]] std::vector<LibraryFinding> checkEditorLibrary(const std::filesystem::path& dataRoot);

/**
 * @brief La bibliothèque **sans fenêtre** (`LOT-EDITOR-08`, règle 4 de la feuille de route) : les
 *        mêmes fonctions que la fenêtre, appelées en ligne de commande.
 *
 * - `--list-prefabs [lieu…]` : les préfabriqués d'un lieu, tous les lieux à défaut ;
 * - `--save-prefab <carte> <nom> --from <c,r> --to <c,r>` : découpe le rectangle de la carte et
 *   l'écrit comme préfabriqué du lieu de cette carte. C'est ce que fait « Save selection as
 *   prefab… », et c'est ainsi que se fabrique un préfabriqué livré.
 *
 * @return Le code de sortie (0, 1 en cas d'échec, 2 si la ligne de commande est fausse), ou
 *         `std::nullopt` si aucune de ces commandes n'est demandée.
 */
[[nodiscard]] std::optional<int> runPrefabCommand(const std::vector<std::string>& arguments,
                                                  const std::filesystem::path& dataRoot,
                                                  std::string& output);

}  // namespace hmi
