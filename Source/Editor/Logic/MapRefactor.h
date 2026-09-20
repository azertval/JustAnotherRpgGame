// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Resources/ScenePieceManifest.h"

/**
 * @file Editor/Logic/MapRefactor.h
 * @brief **Renommer et remplacer** d'un bout à l'autre du projet (`LOT-EDITOR-14`, constat A7,
 *        règle 3 de la feuille de route). Logique pure, sans fenêtre.
 *
 * L'identifiant d'une carte est son chemin (`capital/martpart`), et d'autres fichiers le citent :
 * les portails des autres cartes, les variantes (`base`), les villes (`World/cities`), la clé de
 * son nom dans les catalogues (`map.capital.martpart.name`). Un point d'arrivée est cité par les
 * portails qui y mènent et par la porte de départ d'une ville ; une entité par `carte#id`. Une
 * pièce, enfin, est citée case par case.
 *
 * Chaque opération se fait en deux temps. Un **plan** lit tout le projet et calcule chaque fichier
 * à récrire, sans rien écrire : une opération refusée (nom pris, carte illisible, pièce absente de
 * la planche) ne touche aucun fichier. Puis `applyRefactorPlan` écrit. La fenêtre et `LevelEditor
 * --rename-map …` appellent les mêmes plans (règle 4).
 *
 * Ce qui cite quoi ne s'écrit pas famille par famille : une propriété d'entité dont la source est
 * `Maps`, `ArrivalPoints` ou `EntityRefs` dans `core::knownEntityKinds` est suivie sans code
 * (règle 2). Les cartes se récrivent par l'écrivain canonique, les villes en JSON indenté de deux
 * espaces, les catalogues ligne par ligne : un renommage donne un diff git lisible.
 */

namespace hmi {

/// @brief Une écriture d'un plan : le texte d'un fichier, ou son retrait.
struct ProjectEdit {
    std::filesystem::path file;
    /// Le nouveau texte ; `std::nullopt` : le fichier est retiré.
    std::optional<std::string> text;
};

/// @brief Un endroit où une chose est nommée : « qui cite ceci ? ».
struct Citation {
    /// Le fichier qui la nomme.
    std::filesystem::path file{};
    /// La carte qui la nomme, vide hors carte (une ville, un catalogue).
    std::string mapId{};
    /// L'entité qui la nomme, par son `id` ; vide sinon.
    std::string entityId{};
    std::optional<core::GridPosition> cell{};
    /// Ce qui la nomme : `portal e2: targetMap`, `district …: map`, `map.capital.martpart.name`.
    std::string what{};

    [[nodiscard]] bool operator==(const Citation&) const = default;
};

/// @return La citation en une ligne : `capital/arenarea (47, 35): portal e2: targetMap`, ou
///         `World/cities/capital.json: district …: map`.
[[nodiscard]] std::string formatCitation(const Citation& citation,
                                         const std::filesystem::path& dataRoot);

/// @brief Ce qu'une opération récrira, calculé sans rien écrire.
struct RefactorPlan {
    /// Le refus, vide si l'opération est possible.
    std::string error;
    std::vector<ProjectEdit> edits;
    /// Ce qui change, citation par citation.
    std::vector<Citation> changes;

    [[nodiscard]] bool ok() const noexcept {
        return error.empty();
    }
};

/**
 * @brief Écrit le plan : les textes d'abord (dossiers créés au besoin), les retraits ensuite ; un
 *        dossier de cartes vidé par un déplacement est retiré.
 * @return Faux, avec @p error, si un fichier n'a pas pu s'écrire.
 */
[[nodiscard]] bool applyRefactorPlan(const RefactorPlan& plan, std::string& error);

/// @name Qui cite ceci ?
/// @{

/// @return Ce qui nomme la carte @p mapId : portails, variantes, villes, clé de son nom.
[[nodiscard]] std::vector<Citation> citationsOfMap(const std::filesystem::path& dataRoot,
                                                   std::string_view mapId);

/// @return Ce qui nomme le point d'arrivée @p arrival de @p mapId : portails, départ d'une ville.
[[nodiscard]] std::vector<Citation> citationsOfArrival(const std::filesystem::path& dataRoot,
                                                       std::string_view mapId,
                                                       std::string_view arrival);

/// @return Ce qui nomme l'entité `mapId#entityId`.
[[nodiscard]] std::vector<Citation> citationsOfEntity(const std::filesystem::path& dataRoot,
                                                      std::string_view mapId,
                                                      std::string_view entityId);

/// @return Les couches qui posent @p piece, une citation par carte et par couche, sur la première
///         case ; @p what dit combien de cases.
[[nodiscard]] std::vector<Citation> citationsOfPiece(const std::filesystem::path& dataRoot,
                                                     std::string_view piece);

/// @}

/// @name Renommer
/// @{

/**
 * @brief Renomme la carte @p oldId en @p newId — un autre dossier est permis
 *        (`coliseum` → `capital/coliseum`).
 *
 * Le fichier et son annexe (`<carte>.editor.json`) changent de chemin ; son nom suit s'il est la
 * clé de l'ancien identifiant, et la clé change dans chaque catalogue, texte gardé ; portails,
 * variantes et villes citent le nouvel identifiant.
 */
[[nodiscard]] RefactorPlan planRenameMap(const std::filesystem::path& dataRoot,
                                         std::string_view oldId, std::string_view newId);

/// @brief Renomme le point d'arrivée @p oldName de @p mapId, et ce qui le cite.
[[nodiscard]] RefactorPlan planRenameArrival(const std::filesystem::path& dataRoot,
                                             std::string_view mapId, std::string_view oldName,
                                             std::string_view newName);

/**
 * @brief Renomme l'entité `mapId#oldId` en @p newId, et ce qui la cite.
 *
 * Refusé : un identifiant vide, pris, qui contient `#`, `/` ou une espace, ou de la forme
 * `e<n>` avec `n` au moins `nextEntityId` — l'éditeur pourrait le redonner (décision D8).
 */
[[nodiscard]] RefactorPlan planRenameEntityId(const std::filesystem::path& dataRoot,
                                              std::string_view mapId, std::string_view oldId,
                                              std::string_view newId);

/// @}

/// @name Remplacer, changer de planche
/// @{

/**
 * @brief Remplace la pièce @p from par @p to, sur les cartes @p maps (toutes celles qui la posent
 *        si la liste est vide), par `core::LevelDraft::replacePieces`.
 *
 * Refusé si @p to n'est pas sur la planche d'une de ces cartes, si l'une est un sol et l'autre
 * non, ou si une nouvelle emprise déborde.
 */
[[nodiscard]] RefactorPlan planReplacePiece(const std::filesystem::path& dataRoot,
                                            std::string_view from, std::string_view to,
                                            const std::vector<std::string>& maps);

/**
 * @brief La table que l'éditeur propose pour passer @p layers à la planche @p target : chaque
 *        pièce citée va à la pièce de même nom — ou dont elle est un ancien nom (`aliases`),
 *        sous son nom courant. Une pièce sans correspondant n'est pas dans la table.
 */
[[nodiscard]] core::PieceRenaming proposedPieceTable(const std::vector<core::TileLayer>& layers,
                                                     const core::ScenePieceManifest& target);

/// @return Les pièces citées par @p layers qui, renommées par @p table, manquent à @p target —
///          triées, sans doublon.
[[nodiscard]] std::vector<std::string> piecesMissingFrom(const std::vector<core::TileLayer>& layers,
                                                         const core::PieceRenaming& table,
                                                         const core::ScenePieceManifest& target);

/**
 * @brief Fait passer la carte @p mapId à la planche du lieu @p place, par
 *        `core::LevelDraft::changeScene` : la table proposée, que @p table complète ou corrige.
 *
 * Refusé si une pièce citée n'a pas de correspondant sur la nouvelle planche : la carte ne se
 * repeint pas, elle se traduit. Une variante ne change que sa propre planche (`scene`), et ce sont
 * les pièces de sa base qui doivent exister sur la nouvelle.
 *
 * Un lieu dont l'atelier vient d'installer la planche n'a pas encore de table d'apparence
 * (`appearance.json`, que lisent le rendu des îlots et les cases sans pièce) : le plan la crée
 * depuis celle de l'ancien lieu, pièces traduites par la même table.
 */
[[nodiscard]] RefactorPlan planChangeScene(const std::filesystem::path& dataRoot,
                                           std::string_view mapId, std::string_view place,
                                           const core::PieceRenaming& table);

/// @brief Une table de correspondance lue d'un fichier.
struct PieceTableResult {
    core::PieceRenaming table;
    std::string error;

    [[nodiscard]] bool ok() const noexcept {
        return error.empty();
    }
};

/**
 * @brief Lit une table de correspondance :
 *        `{"format": "jadg-piece-table", "version": 1, "pieces": {"ancien": "nouveau"}}`.
 */
[[nodiscard]] PieceTableResult readPieceTable(const std::filesystem::path& file);

/// @}

/**
 * @brief L'entrée sans fenêtre (décision D9) :
 *
 * - `--who-cites map <carte>`, `--who-cites arrival <carte> <point>`,
 *   `--who-cites entity <carte> <id>`, `--who-cites piece <pièce>` ;
 * - `--rename-map <ancien> <nouveau>`, `--rename-arrival <carte> <ancien> <nouveau>`,
 *   `--rename-id <carte> <ancien> <nouveau>` ;
 * - `--replace-piece <ancienne> <nouvelle> [carte…]` ;
 * - `--change-scene <carte> <lieu> [--table <table.json>]`.
 *
 * Chaque commande écrit ce qu'elle change, et rien si elle est refusée (sortie 1). Suivie de
 * `--check`, elle contrôle ensuite toutes les cartes.
 *
 * @return Le code de sortie, ou `std::nullopt` si aucune de ces commandes n'est demandée.
 */
[[nodiscard]] std::optional<int> runRefactorCommand(const std::vector<std::string>& arguments,
                                                    const std::filesystem::path& dataRoot,
                                                    std::string& output);

}  // namespace hmi
