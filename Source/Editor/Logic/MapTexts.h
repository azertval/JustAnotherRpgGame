// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

/**
 * @file Editor/Logic/MapTexts.h
 * @brief Les textes qu'une carte cite : des **clés de traduction**, jamais du texte
 *        (`LOT-EDITOR-07`).
 *
 * Une carte ne porte que deux textes que le jeu affiche : son **nom** (`name`), que le bandeau du
 * jeu montre, et le nom de ses **îlots** (`cityBlock`), que le plan de la ville montre sous la clé
 * `city_block.<nom>`. Le nom d'une carte est lui-même une clé, `map.<identifiant>.name`, les barres
 * obliques de l'identifiant devenant des points (`map.capital.martpart.name`) : *décision de
 * l'auteur, 19 septembre 2026*. Le jeu la traduit (`hmi::ruleLabel`), et un nom qui n'est pas une
 * clé s'affiche tel quel.
 *
 * Les catalogues sont les fichiers `<langue>.lang` de `<racine>/Localization`, lus par
 * `hmi::Localization::parseCatalog` : l'éditeur ne connaît pas d'autre format.
 */

namespace hmi {

/// @brief Le préfixe de clé du nom d'un îlot, suivi de son nom (`city_block.parvis`).
inline constexpr std::string_view CITY_BLOCK_KEY_PREFIX = "city_block.";

/// @return La clé du nom de la carte @p mapId : `map.capital.martpart.name`.
[[nodiscard]] std::string mapNameKey(std::string_view mapId);

/// @return Le dossier des catalogues de traduction de @p dataRoot.
[[nodiscard]] std::filesystem::path localizationDirectory(const std::filesystem::path& dataRoot);

/// @brief Les catalogues d'un dossier : langue (nom du fichier sans `.lang`) → clé → texte.
using TranslationCatalogs =
    std::map<std::string, std::unordered_map<std::string, std::string>, std::less<>>;

/// @return Les catalogues `<langue>.lang` de @p directory ; vide si le dossier n'en a pas.
[[nodiscard]] TranslationCatalogs loadTranslationCatalogs(const std::filesystem::path& directory);

/// @return Les langues de @p catalogs qui n'ont pas @p key, triées.
[[nodiscard]] std::vector<std::string> languagesMissing(const TranslationCatalogs& catalogs,
                                                        std::string_view key);

/**
 * @brief Ajoute `key = …` à la fin de chaque catalogue de @p directory qui n'a pas @p key.
 *
 * Chaque catalogue reçoit son texte de @p copyFrom s'il a cette clé — une carte renommée garde ses
 * traductions —, sinon @p text, le même dans toutes les langues : c'est un nom propre que l'auteur
 * traduira ensuite. Un catalogue qui a déjà la clé n'est pas touché ; l'ancienne clé reste.
 *
 * @return Faux si un catalogue n'a pas pu s'écrire.
 */
[[nodiscard]] bool addTranslation(const std::filesystem::path& directory, std::string_view key,
                                  std::string_view text, std::string_view copyFrom = {});

/**
 * @brief La clé du nom de la carte @p mapId, ajoutée aux catalogues de @p dataRoot.
 * @param dataRoot La racine des données, dont les catalogues de traduction s'écrivent.
 * @param mapId    L'identifiant de la carte, qui forme la clé `map.<id>.name`.
 * @param text    Le texte des catalogues qui n'ont ni la clé, ni @p copyFrom.
 * @param copyFrom L'ancien nom de la carte, dont les traductions se reprennent.
 * @return La clé ; vide si un catalogue n'a pas pu s'écrire.
 */
[[nodiscard]] std::string nameMapInCatalogs(const std::filesystem::path& dataRoot,
                                            std::string_view mapId, std::string_view text,
                                            std::string_view copyFrom = {});

}  // namespace hmi
