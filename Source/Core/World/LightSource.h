// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <optional>
#include <string_view>

#include "Core/Levels/MapEntity.h"

/**
 * @file Core/World/LightSource.h
 * @brief Les **sources de lumière** d'un lieu (`LOT-1007`, `EX-REN-055`, `EX-EDIT-106`) : ce
 *        qu'un lampadaire, un brasero, une lanterne émettent, et les deux façons de le dire.
 *
 * ## Deux façons de déclarer une source
 *
 * - **La pièce** : une entrée du manifeste d'un kit porte un champ `light`. Toute carte qui pose
 *   la pièce reçoit sa lumière, sans rien écrire — un lampadaire éclaire parce qu'il est un
 *   lampadaire.
 *
 *       "prop-lamppost": { "file": "props/prop-lamppost.png", …,
 *                          "light": { "color": "#ffbe7a", "radius": 6.0, "height": 3.2 } }
 *
 * - **L'entité** : la famille `light` (`core::knownEntityKinds`), posée dans l'éditeur comme un
 *   coffre. Pour ce qu'aucune pièce ne porte : la lueur d'une fenêtre, un feu, une lumière
 *   d'ambiance. Ses propriétés sont des scalaires (`core::PropertyValue` n'a pas de composite) :
 *   la couleur en `#rrggbb`, la portée en **cases**, la hauteur en **décimètres**, l'intensité en
 *   **pour cent**.
 *
 * Les deux aboutissent à la même valeur, `core::LightEmission`, que le rendu lit en mètres.
 *
 * ## Quand elle est allumée
 *
 * Une source s'allume avec `core::DayLight::lamps` — au crépuscule —, sauf si elle est
 * `always` : un sous-sol, une forge, un autel restent éclairés en plein jour.
 *
 * Logique pure, sans Qt ni GPU.
 */

namespace core {

/// @brief Une couleur de lumière : rouge, vert, bleu, de 0 à 1.
struct LightColor {
    float r = 1.0F;
    float g = 1.0F;
    float b = 1.0F;

    [[nodiscard]] bool operator==(const LightColor&) const = default;
};

/// @return La couleur écrite `#rrggbb` ; rien si le texte n'en est pas une.
[[nodiscard]] std::optional<LightColor> parseLightColor(std::string_view text) noexcept;

/// @brief Ce qu'une source émet, en mètres : la valeur commune à la pièce et à l'entité.
struct LightEmission {
    /// La couleur d'une flamme de lanterne : ce que vaut une source qui ne dit pas la sienne.
    static constexpr LightColor DEFAULT_COLOR{.r = 1.0F, .g = 0.745F, .b = 0.478F};
    static constexpr float DEFAULT_RADIUS_METRES = 6.0F;
    static constexpr float DEFAULT_HEIGHT_METRES = 2.2F;
    /// Bornes de la portée et de la hauteur, en mètres ; de l'intensité, sans unité.
    static constexpr float MAXIMUM_RADIUS_METRES = 24.0F;
    static constexpr float MAXIMUM_HEIGHT_METRES = 12.0F;
    static constexpr float MAXIMUM_INTENSITY = 3.0F;

    LightColor color = DEFAULT_COLOR;
    /// La distance, en mètres, au-delà de laquelle la source n'éclaire plus.
    float radius = DEFAULT_RADIUS_METRES;
    /// La hauteur de la source au-dessus du sol, en mètres.
    float height = DEFAULT_HEIGHT_METRES;
    /// Ce par quoi sa couleur est multipliée : 1 pour une lanterne.
    float intensity = 1.0F;
    /// Vrai pour une flamme : son intensité tremble.
    bool flicker = false;
    /// Vrai si elle reste allumée en plein jour.
    bool always = false;

    [[nodiscard]] bool operator==(const LightEmission&) const = default;
};

/// @brief Une source posée dans un lieu : où, et ce qu'elle émet.
struct LightSource {
    /// Le point de grille de la source, en cases continues : le centre de la case (4, 2) est
    /// (4,5 ; 2,5).
    float column = 0.0F;
    float row = 0.0F;
    LightEmission emission{};

    [[nodiscard]] bool operator==(const LightSource&) const = default;
};

/// @brief Type d'entité d'une source de lumière posée dans l'éditeur.
inline constexpr std::string_view LIGHT_ENTITY_TYPE = "light";
/// @brief Propriété d'une lumière : sa couleur, écrite `#rrggbb`.
inline constexpr std::string_view LIGHT_COLOR_PROPERTY = "color";
/// @brief Propriété d'une lumière : sa portée, en cases (1,5 m).
inline constexpr std::string_view LIGHT_RADIUS_PROPERTY = "radius";
/// @brief Propriété d'une lumière : sa hauteur au-dessus du sol, en décimètres.
inline constexpr std::string_view LIGHT_HEIGHT_PROPERTY = "height";
/// @brief Propriété d'une lumière : son intensité, en pour cent.
inline constexpr std::string_view LIGHT_INTENSITY_PROPERTY = "intensity";
/// @brief Propriété d'une lumière : vrai si elle tremble comme une flamme.
inline constexpr std::string_view LIGHT_FLICKER_PROPERTY = "flicker";
/// @brief Propriété d'une lumière : vrai si elle reste allumée en plein jour.
inline constexpr std::string_view LIGHT_ALWAYS_PROPERTY = "always";

/// Bornes des propriétés entières d'une lumière, et leurs valeurs à la création.
inline constexpr int LIGHT_RADIUS_CELLS_DEFAULT = 4;
inline constexpr int LIGHT_RADIUS_CELLS_MAXIMUM = 16;
inline constexpr int LIGHT_HEIGHT_DECIMETRES_DEFAULT = 22;
inline constexpr int LIGHT_HEIGHT_DECIMETRES_MAXIMUM = 120;
inline constexpr int LIGHT_INTENSITY_PERCENT_DEFAULT = 100;
inline constexpr int LIGHT_INTENSITY_PERCENT_MINIMUM = 10;
inline constexpr int LIGHT_INTENSITY_PERCENT_MAXIMUM = 300;
/// La couleur d'une lumière à sa création, telle qu'elle s'écrit.
inline constexpr std::string_view LIGHT_COLOR_DEFAULT = "#ffbe7a";

/**
 * @brief La source que pose l'entité @p entity, au centre de sa case.
 *
 * Une propriété absente, mal typée ou hors bornes vaut sa valeur de création, ramenée dans ses
 * bornes : une carte en cours d'édition éclaire quand même (`EX-NFR-040`).
 *
 * @return Rien si @p entity n'est pas une lumière.
 */
[[nodiscard]] std::optional<LightSource> lightSourceOf(const MapEntity& entity);

}  // namespace core
