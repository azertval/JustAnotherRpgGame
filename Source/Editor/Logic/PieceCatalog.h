// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Levels/PieceFootprint.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileType.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "Editor/Logic/LayerView.h"

/**
 * @file Editor/Logic/PieceCatalog.h
 * @brief Le **catalogue des pièces** d'un lieu : ce que la palette montre, groupé par classe, avec
 *        les pièces que la carte cite et que la planche n'a plus (`LOT-EDITOR-03`, `EX-EDIT-063`).
 *
 * La palette est la planche du lieu : chaque pièce de son manifeste, sous le nom court que les
 * cartes écrivent, rangée par classe (sols, pièces debout, pièces larges). Une pièce qu'une couche
 * nomme mais que le manifeste ignore — planche réextraite, pièce renommée sans alias — n'est pas
 * perdue : elle reste dans la carte, se dessine en damier, et la palette la liste à part pour
 * qu'on la voie et qu'on puisse encore la poser (règle 3 de la feuille de route).
 *
 * Logique pure, sans Qt : la palette n'en fait que l'image.
 */

namespace hmi {

class PlaceAppearance;

/// @brief Une pièce du catalogue.
struct PieceCatalogEntry {
    /// Nom court, celui que la carte écrit (`wall-left`).
    std::string name;
    /// Fichier image, relatif au dossier du lieu ; vide pour une pièce absente de la planche.
    std::string file;
    core::ScenePieceClass pieceClass = core::ScenePieceClass::Other;
    core::PieceFootprint footprint;
    core::PieceTactical tactical = core::PieceTactical::Solid;
    /// Citée par la carte, absente du manifeste : montrée en damier.
    bool missing = false;
    /// Se pose sur la couche de sol (un sol) ; sinon sur la couche de décor.
    bool floor = false;

    [[nodiscard]] bool operator==(const PieceCatalogEntry&) const = default;
};

/// @brief Un groupe de la palette : un titre, ses pièces dans l'ordre du manifeste.
struct PieceCatalogGroup {
    std::string label;
    std::vector<PieceCatalogEntry> pieces;

    [[nodiscard]] bool operator==(const PieceCatalogGroup&) const = default;
};

/// Titre du groupe des pièces que la carte cite et que la planche n'a pas.
inline constexpr std::string_view MISSING_PIECES_GROUP = "Missing from the sheet";

/**
 * @brief Le catalogue d'un lieu.
 *
 * Groupes, dans cet ordre et seulement s'ils ont une pièce : « Floors », « Standing », « Wide »,
 * « Other » (une classe que l'éditeur ne connaît pas), puis `MISSING_PIECES_GROUP` — les noms
 * que @p layers citent et que @p manifest ne connaît ni par leur nom ni par un alias, triés.
 * @param manifest Le manifeste du lieu, `nullptr` sans lieu : seules restent les pièces absentes.
 * @param layers   Les couches de la carte.
 */
[[nodiscard]] std::vector<PieceCatalogGroup> pieceCatalog(
    const core::ScenePieceManifest* manifest, const std::vector<core::TileLayer>& layers);

/**
 * @brief Le catalogue réduit aux pièces dont le nom ou la classe contient @p query, sans égard à
 *        la casse ; les groupes vidés disparaissent. Une recherche vide rend tout.
 */
[[nodiscard]] std::vector<PieceCatalogGroup> filterPieceCatalog(
    const std::vector<PieceCatalogGroup>& catalog, std::string_view query);

/// @return La bulle d'aide d'une pièce : `front-left — wide, 2 × 1, solid`.
[[nodiscard]] std::string pieceDescription(const PieceCatalogEntry& entry);

/**
 * @return La couche que vise une pièce : la première couche de sol au rez pour un sol (@p floor) ;
 *         pour un relief, la couche de décor @p active si c'en est une — un étage se peint comme
 *         le rez (`LOT-129`) —, la première couche de décor au rez sinon. Ce sont celles que la
 *         composition du jeu lit. `std::nullopt` si la carte n'en a pas.
 */
[[nodiscard]] std::optional<std::size_t> pieceTargetLayer(
    const std::vector<core::TileLayer>& layers, bool floor, LayerSlot active = {});

/**
 * @brief Le type qu'écrit la case d'ancrage d'une pièce posée à la main (décision D3 : le type
 *        garde le sens de règle de la case).
 *
 * Dans l'ordre : le type dont la table du lieu tire cette pièce (`PlaceAppearance::typeOfPiece`) ;
 * à défaut, `wall` pour une pièce debout — ce que portent les reliefs des cartes livrées, et
 * qu'une pièce venue à manquer arrête encore la vue — et le vide pour un sol, que la pièce seule
 * habille.
 * @param appearance La table du lieu, `nullptr` sans lieu.
 * @param piece      Le nom court de la pièce.
 * @param floor      La pièce est un sol.
 */
[[nodiscard]] core::TileType pieceCellType(const PlaceAppearance* appearance,
                                           std::string_view piece, bool floor);

}  // namespace hmi
