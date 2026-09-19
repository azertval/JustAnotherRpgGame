// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Core/Levels/Level.h"
#include "Editor/Logic/EntityReferences.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/MapTexts.h"

/**
 * @file Editor/Logic/ContentCheck.h
 * @brief Le **contrôle du contenu** : ce qu'une carte promet au joueur et ne tient pas
 *        (`LOT-EDITOR-07`).
 *
 * Le contrôle du format (`MapFormat.h`) dit si une carte est **bien écrite** ; celui-ci dit si elle
 * **se joue**. Il tourne sur toutes les cartes, dans `LevelEditor --check` comme dans le panneau
 * « Problems » de la fenêtre, et relève :
 *
 * - **les références** des entités : dialogues, rencontres, figurines, drapeaux de monde, lieux,
 *   objets, `carte#id`, propriétés requises et bornes (`core::validateMapEntities`) ;
 * - **le terrain** : une rencontre dont la formation ne tient pas (`core::analyzeEncounterTerrain`,
 *   le contrôle du `LOT-11` étendu à toutes les cartes), une zone de combat qui ne se joue pas, une
 *   entrée d'arène hors de toute zone ;
 * - **l'atteignabilité** : toute case utile — portail, point d'arrivée, PNJ, coffre, panneau,
 *   rencontre, zone — est joignable depuis l'entrée de la carte ou un point d'arrivée qu'on
 *   atteint d'ailleurs (`core::ExplorationReach`, la règle de marche du jeu) ;
 * - **le graphe** : un portail sans retour, un point d'arrivée que rien ne nomme ;
 * - **les textes** : le nom de la carte et celui de ses îlots sont des clés présentes dans chaque
 *   catalogue (`MapTexts.h`).
 *
 * Une **variante** (décision D12) se contrôle telle que le jeu la charge, c'est-à-dire sur les
 * cases de sa base : si la base change sous ses entités — un PNJ muré, un portail hors d'atteinte
 * —, c'est la variante qui le dit. Une entité que la base ne loge plus du tout rend la variante
 * illisible, ce que le contrôle du format signale déjà.
 *
 * Les portails vers une carte absente ou un point d'arrivée inconnu, et les zones de combat
 * dégénérées, sont dits une fois, par le contrôle du format (`core::validateWorldGraph`,
 * `core::validateWorldMap`) : ce fichier ne les répète pas.
 */

namespace hmi {

/// @brief Ce contre quoi toutes les cartes se contrôlent : lu une fois, pour toutes.
struct ContentContext {
    /// Les catalogues que les entités citent, et le graphe du monde.
    EditorReferences references;
    /// Les catalogues de traduction ; vides, les textes ne se contrôlent pas.
    TranslationCatalogs translations;
    /// Les points d'arrivée qu'on atteint d'ailleurs, `(carte, nom)` : ceux qu'un portail nomme, et
    /// la porte de départ de chaque ville (`World/cities`).
    std::set<std::pair<std::string, std::string>, std::less<>> namedArrivals;
};

/// @return Le contexte de contrôle des cartes de @p dataRoot.
[[nodiscard]] ContentContext loadContentContext(const std::filesystem::path& dataRoot);

/**
 * @brief Contrôle le contenu d'une carte lue.
 * @param mapId   Son identifiant (`capital/martpart`).
 * @param level   La carte, telle que le jeu la charge (une variante porte les cases de sa base).
 * @param context Le contexte de toutes les cartes.
 * @return Les constats, dans l'ordre : textes, références, terrain, atteignabilité, graphe.
 */
[[nodiscard]] std::vector<MapCheckFinding> checkMapContent(std::string_view mapId,
                                                           const core::Level& level,
                                                           const ContentContext& context);

}  // namespace hmi
