// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

/**
 * @file Core/Data/JsonDocument.h
 * @brief Brique de lecture JSON partagée par tous les catalogues (`EX-CNT-012`).
 */

namespace core {

/**
 * @brief Catégorie d'échec de lecture d'un document JSON.
 *
 * Les cinq catégories que les six lecteurs du dépôt avaient chacun redéfinies pour leur compte
 * (`hmi::AnimationCatalogError`, `hmi::PlaceAppearanceError`…). Elles sont
 * ici **une seule fois** : deux définitions de « version non gérée » finissent par diverger sur
 * ce qu'elles recouvrent.
 */
enum class JsonReadError {
    None,                ///< Pas d'erreur.
    FileNotFound,        ///< Fichier absent ou illisible.
    ParseError,          ///< JSON malformé, ou racine qui n'est pas un objet.
    UnsupportedVersion,  ///< Numéro de version supérieur à celui que le lecteur sait lire.
    MalformedStructure,  ///< Structure inattendue (champ absent, type incorrect…).
};

/**
 * @brief Position d'un octet dans un texte, en **ligne et colonne** comptées à partir de 1.
 *
 * `nlohmann` ne rapporte qu'un décalage en octets, inexploitable pour un humain devant un
 * catalogue de mille lignes. `EX-CNT-010` exige que l'échec nomme le fichier **et la ligne**.
 */
struct TextPosition {
    int line = 0;    ///< Ligne, à partir de 1. `0` si inconnue.
    int column = 0;  ///< Colonne, à partir de 1. `0` si inconnue.
};

/**
 * @brief Résultat de la lecture de l'enveloppe commune d'un document JSON.
 *
 * Contient l'arbre parsé **ou** la description de l'échec, jamais les deux. Aucune lecture ne
 * lève d'exception vers l'appelant (`EX-NFR-040`).
 */
struct JsonDocument {
    nlohmann::json root;                        ///< L'arbre parsé. Vide si `error != None`.
    JsonReadError error = JsonReadError::None;  ///< Catégorie d'échec.
    std::string message;                        ///< Description lisible, vide si succès.
    TextPosition position;                      ///< Position de l'erreur, si connue.
    int version = 0;                            ///< Version lue dans le document.

    /// @brief Vrai si la lecture a abouti.
    [[nodiscard]] bool ok() const {
        return error == JsonReadError::None;
    }
};

/**
 * @brief Convertit un décalage en octets en ligne et colonne.
 * @param text Le texte d'origine.
 * @param byteOffset Décalage, tel que rapporté par `nlohmann::json::parse_error::byte`.
 * @return La position, ou `{0, 0}` si le décalage sort du texte.
 */
[[nodiscard]] TextPosition positionOf(std::string_view text, std::size_t byteOffset);

/**
 * @brief La position où commence, dans le texte, la valeur que désigne un pointeur JSON.
 *
 * nlohmann/json (3.11) ne garde pas les positions de l'arbre qu'il construit : une erreur de
 * **sens** (une étape sans condition, une valeur hors de sa liste) ne pourrait nommer que son
 * chemin. Ce repérage relit le texte, en suivant le chemin, pour que le message nomme aussi la
 * **ligne** (`LOT-116`, `EX-CNT-010`). Le texte est supposé bien formé : il vient d'être lu.
 *
 * @param text    Le texte du document.
 * @param pointer Le chemin de la valeur, par exemple `/steps/2/when`.
 * @return La position, ou `{0, 0}` si le chemin ne mène à rien.
 */
[[nodiscard]] TextPosition positionOfPointer(std::string_view text,
                                             const nlohmann::json::json_pointer& pointer);

/**
 * @brief Lit l'enveloppe commune d'un catalogue : JSON bien formé, racine objet, garde de version.
 *
 * C'est la routine que les six lecteurs du dépôt réimplémentaient à l'identique. Le champ de
 * version **absent vaut 1** — un catalogue écrit avant que le format ne se versionne reste
 * lisible ; une version **supérieure** à `supportedVersion` est refusée explicitement, jamais
 * lue au mieux : un fichier plus récent que le jeu contient par définition ce que le jeu ne sait
 * pas interpréter.
 *
 * @param json Contenu JSON.
 * @param supportedVersion Version la plus élevée que le lecteur sait lire. **`0` désactive la
 *        garde** : l'appelant porte alors sa propre catégorie d'échec de version, ce qui est le
 *        cas du chargeur de niveaux (`core::LevelValidationError::UnsupportedFormatVersion`).
 * @param origin Nom à faire figurer dans les messages (« manifest.json »). Facultatif.
 * @param versionField Nom du champ de version. Vaut `"version"` par défaut.
 * @return Le document, ou l'échec décrit. Ne lève jamais.
 */
[[nodiscard]] JsonDocument readJsonObject(std::string_view json, int supportedVersion,
                                          std::string_view origin = {},
                                          std::string_view versionField = "version");

/**
 * @brief Comme `readJsonObject`, en lisant d'abord le fichier.
 *
 * Un fichier **absent** produit `FileNotFound` plutôt qu'un document vide : c'est à l'appelant de
 * décider qu'un catalogue absent est un état de départ légitime, et cette décision ne peut pas
 * être prise ici.
 *
 * @param path Chemin du fichier.
 * @param supportedVersion Version la plus élevée que le lecteur sait lire.
 * @param versionField Nom du champ de version.
 * @return Le document, ou l'échec décrit. Ne lève jamais.
 */
[[nodiscard]] JsonDocument readJsonObjectFromFile(const std::filesystem::path& path,
                                                  int supportedVersion,
                                                  std::string_view versionField = "version");

}  // namespace core
