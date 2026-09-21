// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/Level.h"
#include "Editor/Logic/MapRefactor.h"

/**
 * @file Editor/Logic/WorldLinks.h
 * @brief **Relier deux cartes** depuis le graphe du monde (`LOT-EDITOR-09`, `EX-EDIT-089`).
 *        Logique pure, sans fenêtre.
 *
 * Tirer un lien d'une carte à une autre pose **quatre entités** : sur chacune, le portail qui mène
 * à l'autre et le point d'arrivée où l'autre fait arriver. Un aller sans retour n'existe pas ici :
 * c'est précisément ce que le contrôle du `LOT-EDITOR-07` reproche à une carte.
 *
 * Comme les renommages (`MapRefactor.h`), l'opération se fait en deux temps : un
 * `hmi::RefactorPlan` calcule le texte de chaque carte sans rien écrire, puis
 * `hmi::applyRefactorPlan` écrit. La fenêtre et `LevelEditor --link-maps` appellent le même plan
 * (règle 4 de la feuille de route).
 */

namespace hmi {

/// @brief Ce qu'un lien pose sur une carte.
struct MapLinkEnd {
    std::string mapId;
    /// Case du portail qui mène à l'autre carte.
    core::GridPosition portalCell;
    /// Case du point d'arrivée où l'autre carte fait arriver.
    core::GridPosition arrivalCell;
    /// Nom de ce point d'arrivée, tel que le portail d'en face le cite.
    std::string arrivalName;
    /// Identifiants donnés aux deux entités posées.
    std::string portalId;
    std::string arrivalId;
};

/// @brief Le lien posé, des deux côtés.
struct MapLink {
    MapLinkEnd from;
    MapLinkEnd to;
};

/**
 * @brief Le nom d'un point d'arrivée qui accueille ceux qui viennent de @p fromMap :
 *        `from-<dernier segment>` (`capital/martpart` → `from-martpart`).
 * @param fromMap Carte d'où l'on vient.
 * @param taken   Les noms déjà pris sur la carte d'accueil.
 * @return Le nom, suffixé `-2`, `-3`… s'il est pris.
 */
[[nodiscard]] std::string arrivalNameFrom(std::string_view fromMap,
                                          const std::vector<std::string>& taken);

/**
 * @brief Les deux cases où poser le portail et le point d'arrivée d'une carte.
 *
 * **Règle** : les cases **libres** (ni solides, ni portant une entité, ni l'entrée) et
 * **atteignables** depuis l'entrée, prises au plus près de l'entrée — à distance égale, la plus
 * petite ligne, puis la plus petite colonne. Le portail prend la première, le point d'arrivée la
 * seconde : ce qu'on relie s'ouvre près de la porte, et l'outil « Entité » les déplace ensuite.
 * @return Faux si la carte n'offre pas deux cases pareilles.
 */
[[nodiscard]] bool linkCells(const core::Level& level, core::GridPosition& portalCell,
                             core::GridPosition& arrivalCell);

/**
 * @brief Plan du lien @p fromMap ↔ @p toMap : le portail et le point d'arrivée des deux côtés.
 * @param dataRoot Racine des données (`Source/Elements`).
 * @param fromMap  Carte d'où part le geste.
 * @param toMap    Carte où il arrive.
 * @param link     Rempli par ce que le plan pose, s'il est accepté (facultatif).
 *
 * Refusé, sans rien écrire : une carte inconnue ou illisible, une carte reliée à elle-même, une
 * carte qui n'offre pas deux cases libres atteignables, ou une variante (elle ne porte pas ses
 * propres cases).
 */
[[nodiscard]] RefactorPlan planLinkMaps(const std::filesystem::path& dataRoot,
                                        std::string_view fromMap, std::string_view toMap,
                                        MapLink* link = nullptr);

}  // namespace hmi
