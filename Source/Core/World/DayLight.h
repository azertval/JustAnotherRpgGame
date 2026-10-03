// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/World/LightSource.h"

/**
 * @file Core/World/DayLight.h
 * @brief La **lumière du jour** (`LOT-1007`, `EX-REN-052`) : ce que l'heure du monde fait de la
 *        lumière, lu dans une table en données.
 *
 * ## La table
 *
 * `Assets/Common/Lighting/daylight.json` porte une liste de **clés**, une par heure remarquable ;
 * entre deux clés, tout s'interpole en ligne droite, et la dernière clé du jour rejoint la
 * première du lendemain. Une nuit trop sombre, un couchant trop rouge se règlent là, sans
 * recompiler — la lisibilité du combat la nuit est un critère du lot, et c'est la table qui la
 * tient.
 *
 *     { "version": 1,
 *       "keys": [ { "time": "12:00", "tint": "#ffffff", "ambient": "#999999", "sun": "#808080",
 *                   "azimuth": 50, "elevation": 58, "shadow": 0.35, "lamps": 0.0 } ] }
 *
 * | Champ | Ce qu'il règle |
 * |---|---|
 * | `tint` | ce par quoi une **image** est multipliée : le décor peint, les effets |
 * | `ambient` | la lumière que reçoit toute face d'un **maillage**, d'où qu'elle regarde |
 * | `sun` | la lumière **dirigée** — le soleil le jour, la lune la nuit — sur un maillage |
 * | `azimuth`, `elevation` | d'où elle vient, en degrés (voir ci-dessous) |
 * | `shadow` | ce qu'une ombre portée retire à une image, de 0 (rien) à 1 (le noir) |
 * | `lamps` | l'allumage des lumières de nuit, de 0 (éteintes) à 1 |
 *
 * ## D'où vient la lumière
 *
 * Le repère est celui d'un maillage posé (`hmi::IsoView`) : +X suit les colonnes de la grille, +Z
 * ses lignes, +Y monte. La caméra regarde depuis les grands indices : à l'écran, la **gauche** est
 * (−X, +Z) et le **fond** (−X, −Z).
 *
 * L'azimut tourne dans le plan du sol, de la gauche de l'écran vers sa droite en passant par le
 * fond : 0° à gauche, 45° en haut à gauche (−X), 90° au fond, 180° à droite ; négatif, il passe
 * par le devant (−90° : du côté de la caméra). L'élévation est l'angle au-dessus de l'horizon.
 *
 * Le décor peint porte une lumière « du haut à gauche » : sa face de gauche est éclairée, sa face
 * de droite à l'ombre. C'est un soleil à **0°** — à gauche de l'écran, au-dessus —, et c'est là
 * qu'est celui de midi : la lumière du moteur et celle des images ne se contredisent pas à
 * l'heure où on les compare. Le matin il vient du devant, le soir il passe derrière le lieu et
 * les ombres s'allongent vers la caméra.
 *
 * ## Un sol en maillage et un sol en image se raccordent
 *
 * Une face tournée vers le haut reçoit `ambient + sun × (sin(elevation) + 0,3) / 1,3`. La table
 * livrée tient cette somme égale à `tint`, à toute heure : un dallage modelé et un dallage peint
 * posés côte à côte ont la même lumière.
 *
 * Logique pure, sans Qt ni GPU ; aucune lecture ne lève (`EX-NFR-040`).
 */

namespace core {

/// Version du format de la table écrite dans le fichier, et plus élevée qui soit lue.
inline constexpr int DAYLIGHT_FORMAT_VERSION = 1;

/// @brief Le fichier de la table, relatif au dossier des assets.
inline constexpr std::string_view DAYLIGHT_FILE = "Common/Lighting/daylight.json";

/// @brief Une clé de la table : la lumière d'une heure remarquable.
struct DayLightKey {
    /// L'heure de la clé, en minutes depuis minuit, dans [0, 1440[.
    float minutes = 0.0F;
    LightColor tint{};
    LightColor ambient{};
    LightColor sun{};
    /// D'où vient la lumière dirigée, en degrés (voir le fichier).
    float azimuth = 45.0F;
    float elevation = 60.0F;
    /// Ce qu'une ombre portée retire à une image, de 0 à 1.
    float shadow = 0.0F;
    /// L'allumage des lumières de nuit, de 0 à 1.
    float lamps = 0.0F;

    [[nodiscard]] bool operator==(const DayLightKey&) const = default;
};

/// @brief La lumière d'un instant : ce que le rendu reçoit.
struct DayLight {
    LightColor tint{};
    LightColor ambient{};
    LightColor sun{};
    /// Le vecteur unitaire **vers** la source de la lumière dirigée, dans le repère d'un maillage
    /// posé (X colonnes, Y hauteur, Z lignes).
    std::array<float, 3> toSun{-0.5F, 0.7071F, 0.0F};
    float shadow = 0.0F;
    float lamps = 0.0F;

    [[nodiscard]] bool operator==(const DayLight&) const = default;
};

/// @return Le vecteur unitaire vers la source placée à @p azimuth et @p elevation degrés.
[[nodiscard]] std::array<float, 3> sunDirection(float azimuth, float elevation) noexcept;

/// @brief La table des clés d'un jour, triée par heure.
class DayLightTable {
public:
    DayLightTable() = default;
    /// @param keys Les clés, dans un ordre quelconque : elles sont triées par heure.
    explicit DayLightTable(std::vector<DayLightKey> keys);

    /// @return La table d'usine : celle du jeu si son fichier manque. Les mêmes clés que le
    ///         fichier livré.
    [[nodiscard]] static const DayLightTable& factory();

    [[nodiscard]] const std::vector<DayLightKey>& keys() const noexcept {
        return _keys;
    }

    /**
     * @brief La lumière à @p minutes depuis minuit, interpolée entre les deux clés qui
     *        l'encadrent ; la dernière du jour rejoint la première du lendemain.
     * @return La lumière neutre (teinte blanche, ni ombre ni lampe) d'une table vide.
     */
    [[nodiscard]] DayLight sample(float minutes) const noexcept;

    [[nodiscard]] bool operator==(const DayLightTable&) const = default;

private:
    std::vector<DayLightKey> _keys;
};

/// @brief Résultat d'une lecture de table : la table, ou ce qui a échoué.
struct DayLightTableResult {
    DayLightTable table;
    /// Message technique, vide en cas de succès.
    std::string message;

    [[nodiscard]] bool ok() const noexcept {
        return message.empty();
    }
};

/// @brief Lit la table depuis le texte de son fichier.
[[nodiscard]] DayLightTableResult readDayLightTable(std::string_view json);

/// @brief Lit `daylight.json` ; un fichier absent est une erreur du résultat.
[[nodiscard]] DayLightTableResult readDayLightTableFile(const std::filesystem::path& path);

}  // namespace core
