// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file Core/Resources/ScenePlace.h
 * @brief Les **niveaux** d'un lieu : où chercher ses pièces, du plus propre au plus commun
 *        (`LOT-124`, règle 2 de l'arborescence des assets).
 *
 * Une carte nomme son lieu par un **chemin** (`central-empire/capital/arenarea/arena-of-fate`),
 * celui de son dossier sous `Assets/Regions/`. Ses pièces se cherchent dans ce dossier, puis dans
 * chacun de ses ancêtres, jusqu'au monde :
 *
 * | Niveau | Dossier de pièces (relatif à `Assets/`) |
 * |---|---|
 * | sous-zone, zone | `Regions/<chemin>/Scene` |
 * | ville | `Regions/<région>/<ville>/Common/Scene` |
 * | région | `Regions/<région>/Common/Scene` |
 * | monde | `Common/Terrain`, `Common/Nature`, `Common/Props` |
 *
 * À chaque préfixe du chemin, les deux dossiers sont candidats — le `Scene` propre d'abord, puis le
 * `Common/Scene` d'une ville : l'arbre dit ce qu'est chaque dossier, pas le code. Seuls comptent
 * ceux qui portent un manifeste (`core::ScenePieceManifest::resolve`).
 *
 * Un lieu **sans barre** (`bourg`) est un lieu d'essai à plat, sous `Assets/Scene/<lieu>/` : la
 * forme des racines d'essai, que l'arborescence livrée n'a plus. Il remonte au monde comme les
 * autres.
 *
 * ## Les figurines
 *
 * Un PNJ se range de même (règle 5 de l'arborescence) : un PNJ nommé dans le `Characters/` de sa
 * zone, un archétype dans le `Common/Characters/` de sa ville ou de sa région, un héros ou un
 * peuple générique dans `Common/Characters/` du monde. Une carte nomme la figurine par son
 * **slug**, relatif au dossier `Characters/` qui la range (`citizen`, `Heroes/brawler`) ;
 * `core::resolveFigures` la cherche du plus propre au monde, par les listes `npcs` des manifestes.
 *
 * Logique pure : ce fichier ne lit pas le disque, sauf `scenePlaces` et `resolveFigures`.
 */

namespace core {

/// @brief Un niveau de la résolution : un dossier de pièces, et ce qu'il sert.
struct SceneLevel {
    /// Le nom du niveau, tel que la palette le montre : « Arena of Fate », « Capital », « World ».
    std::string label;
    /// Le lieu que ce niveau couvre, préfixe du chemin d'une carte (`central-empire/capital`) ;
    /// vide pour le monde. Un préfabriqué fait de ses pièces se range sous ce chemin.
    std::string place;
    /// Le dossier des pièces, relatif à `Assets/`, en barres obliques
    /// (`Regions/central-empire/capital/Common/Scene`).
    std::string directory;

    [[nodiscard]] bool operator==(const SceneLevel&) const = default;
};

/// @return Vrai si @p place est un lieu utilisable : non vide, des segments non vides, ni `.` ni
///         `..`, ni antislash.
[[nodiscard]] bool isValidScenePlace(std::string_view place) noexcept;

/// @return Vrai si @p place est un lieu d'essai à plat (`bourg`), faux pour un chemin.
[[nodiscard]] bool isFlatScenePlace(std::string_view place) noexcept;

/**
 * @brief Les niveaux **candidats** de @p place, du plus propre au plus commun, monde compris.
 * @return Vide pour un lieu vide ou invalide.
 */
[[nodiscard]] std::vector<SceneLevel> sceneLevelCandidates(std::string_view place);

/// @return Le nom d'un segment de lieu, pour l'œil : `arena-of-fate` → « Arena of Fate ».
[[nodiscard]] std::string scenePlaceLabel(std::string_view segment);

/**
 * @brief Le dossier **propre** de @p place, relatif à `Assets/` : celui où naît une pièce, où vit
 *        sa table d'apparence. `Regions/<chemin>/Scene`, ou `Scene/<lieu>` à plat ; vide pour un
 *        lieu invalide.
 */
[[nodiscard]] std::string ownSceneDirectory(std::string_view place);

/**
 * @brief Le chemin, relatif à `Assets/`, d'une pièce que **aucun** manifeste ne cite :
 *        `<dossier propre>/<nom>.png`. Ce que le rendu essaie, faute de mieux.
 */
[[nodiscard]] std::string fallbackScenePiecePath(std::string_view place, std::string_view piece);

/// @return Vrai si le lieu @p place descend de @p ancestor (ou l'est) ; tout lieu descend du monde
///         (@p ancestor vide).
[[nodiscard]] bool scenePlaceDescendsFrom(std::string_view place, std::string_view ancestor);

/// @return Les préfixes de @p place, du plus propre au plus commun, le monde (vide) compris :
///         `a/b/c` donne `a/b/c`, `a/b`, `a` puis la chaîne vide ; un lieu à plat, lui-même puis
///         la chaîne vide.
[[nodiscard]] std::vector<std::string> scenePlaceAncestry(std::string_view place);

/**
 * @brief Les lieux où une carte peut se poser — l'**arbre des lieux** de « New map » : chaque
 *        dossier de `Assets/Regions/` dont le `Scene/` propre porte un manifeste (zones et
 *        sous-zones), et chaque lieu d'essai à plat de `Assets/Scene/`. Triés.
 * @param assetsDirectory Le dossier `Assets/`.
 */
[[nodiscard]] std::vector<std::string> scenePlaces(const std::filesystem::path& assetsDirectory);

/**
 * @brief Les niveaux **candidats** des figurines de @p place, du plus propre au monde
 *        (`LOT-124`) : `Regions/<préfixe>/Characters`, puis `Regions/<préfixe>/Common/Characters`,
 *        à chaque préfixe du chemin, puis `Common/Characters`. Un lieu à plat ou vide n'a que le
 *        monde.
 */
[[nodiscard]] std::vector<SceneLevel> characterLevelCandidates(std::string_view place);

/// @brief Le dossier de chaque figurine qu'un lieu peut poser : slug → dossier relatif à `Assets/`.
using FigureDirectories = std::map<std::string, std::string, std::less<>>;

/**
 * @brief Les figurines que le lieu @p place peut poser : le slug de chaque entrée `npcs` des
 *        manifestes de ses niveaux (`characterLevelCandidates`), vers son dossier. Le plus propre
 *        gagne : un PNJ de zone masque un archétype de même slug. Un manifeste absent ou illisible
 *        ne donne rien (`EX-NFR-040`) : la figurine se dessinera par son marqueur.
 * @param assetsDirectory Le dossier `Assets/`.
 * @param place           Le lieu ; vide, le monde seul.
 */
[[nodiscard]] FigureDirectories resolveFigures(const std::filesystem::path& assetsDirectory,
                                               std::string_view place);

/**
 * @brief Le dossier, relatif à `Assets/`, de la figurine @p figure : celui que @p figures lui
 *        donne ; à défaut, @p figure lui-même s'il contient une barre (un dossier nommé depuis
 *        `Assets/`, `Common/Characters/Heroes/brawler`) ; à défaut, `Npc/<slug>`, l'atelier à plat
 *        des racines d'essai. Vide pour une figurine vide.
 */
[[nodiscard]] std::string figureDirectory(const FigureDirectories& figures,
                                          std::string_view figure);

}  // namespace core
