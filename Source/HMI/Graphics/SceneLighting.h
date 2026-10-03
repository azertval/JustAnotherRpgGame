// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>
#include <cstddef>
#include <span>

#include "Core/Math/Rect.h"
#include "Core/World/DayLight.h"
#include "Core/World/LightSource.h"
#include "HMI/Graphics/IsoView.h"

/**
 * @file HMI/Graphics/SceneLighting.h
 * @brief L'**éclairage d'une image** (`LOT-1007`, `EX-REN-053` à `EX-REN-056`), sans GPU : ce
 *        que la lumière d'une heure, les sources d'un lieu et le cadrage de la caméra donnent aux
 *        shaders.
 *
 * ## Tout se calcule dans le repère de la vue
 *
 * Un sommet d'image porte déjà sa position dans la vue (x, y, profondeur — `hmi::IsoView`), et
 * celle d'un maillage s'obtient par sa pose. La vue est une **rotation** de l'espace du lieu, à
 * une échelle près : les angles et les rapports de distance y sont ceux du lieu. Le soleil, les
 * sources et la verticale y sont donc amenés une fois par image, ici, et les shaders n'ont rien à
 * inverser.
 *
 * ## Qui reçoit quoi
 *
 * | | Un maillage | Une image |
 * |---|---|---|
 * | Ambiance et soleil | selon sa normale | — |
 * | Teinte de l'heure | — | multipliée |
 * | Lumières de nuit | selon sa normale et la distance | selon la distance |
 * | Ombres portées | reçues et portées | reçues au sol ; portées par sa boîte (`WorldShadowBox`) |
 *
 * Une image n'a pas de normale : le soleil ne la modèle pas, elle garde la lumière qu'on lui a
 * peinte. C'est la limite du décor en images, levée à la `0.0.3` (D-43).
 *
 * ## La carte d'ombres
 *
 * Une seule, vue du soleil, en projection orthographique, couvrant ce que la caméra montre du
 * sol. Son centre est **calé sur la grille de ses texels** : quand la caméra suit le héros, les
 * bords d'ombre ne frémissent pas. Sa taille ne dépend que du cadrage et de la hauteur du soleil,
 * jamais de la position de la caméra.
 *
 * Logique pure, sans GPU ni Qt.
 */

namespace hmi {

/// Le plus grand nombre de lumières de nuit qu'une image éclaire. Au-delà, les plus proches du
/// centre de l'image sont gardées.
inline constexpr std::size_t MAXIMUM_SCENE_LIGHTS = 16;

/// Une matrice 4 × 4 rangée **par colonnes**, appliquée à un vecteur colonne : celle d'un shader.
using LightMatrix = std::array<float, 16>;

/**
 * @brief La boîte qu'une pièce de décor en **image** oppose au soleil (`LOT-1007`).
 *
 * Une image dressée n'a pas de volume ; pour qu'un mur jette une ombre, son emprise est élevée en
 * boîte, que seule la carte d'ombres voit. En cases et en mètres.
 */
struct WorldShadowBox {
    /// Le coin de la boîte aux plus petits indices, en cases continues.
    float column = 0.0F;
    float row = 0.0F;
    /// Son étendue le long des colonnes et des lignes, en cases.
    float columns = 1.0F;
    float rows = 1.0F;
    /// La hauteur de sa base au-dessus du sol, en mètres : un étage n'est pas au rez.
    float base = 0.0F;
    /// Sa hauteur, en mètres.
    float height = 0.0F;
    /// Ce que son sommet garde de l'étendue de sa base, de 0 (une pointe) à 1 (une boîte) : un
    /// mur monte droit, une fontaine, une statue ou un arbre se resserrent.
    float top = 1.0F;

    [[nodiscard]] bool operator==(const WorldShadowBox&) const = default;
};

/// @brief La lumière que l'appelant donne au rendu d'un lieu.
struct WorldLighting {
    /// La lumière de l'heure (`core::DayLightTable::sample`).
    core::DayLight light{};
    /// Faux : aucune ombre portée, aucune passe d'ombres.
    bool shadows = true;
    /// Le côté de la carte d'ombres, en texels.
    int shadowSize = 2048;
    /// Le temps, en secondes : ce qui fait trembler une flamme.
    float seconds = 0.0F;

    [[nodiscard]] bool operator==(const WorldLighting&) const = default;
};

/**
 * @brief Le bloc uniforme `Lighting` des shaders (`std140`), tel qu'il est téléversé.
 *
 * Un bloc **neutre** (`active` nul) rend un shader à ce qu'il faisait avant le lot, au bit près :
 * c'est celui de tout rendu qui ne règle pas d'éclairage — la galerie des assets, un test, le
 * canevas en plan.
 */
struct LightingUniforms {
    /// De la vue vers la carte d'ombres : (u, v, profondeur) dans [0, 1].
    LightMatrix viewToShadow{};
    /// La teinte des images ; `a` : 1 si l'éclairage est actif, 0 pour le bloc neutre.
    std::array<float, 4> tint{1.0F, 1.0F, 1.0F, 0.0F};
    /// L'ambiance des maillages ; `a` : ce qu'une ombre portée retire à une image.
    std::array<float, 4> ambient{1.0F, 1.0F, 1.0F, 0.0F};
    /// La lumière dirigée ; `a` : 1 si la carte d'ombres est à lire.
    std::array<float, 4> sun{0.0F, 0.0F, 0.0F, 0.0F};
    /// Le vecteur unitaire vers le soleil, dans la vue ; `w` : le côté d'un texel de la carte
    /// d'ombres, en coordonnées de texture.
    std::array<float, 4> toSun{0.0F, -1.0F, 0.0F, 0.0F};
    /// La verticale du lieu, dans la vue, unitaire ; `w` : le nombre de lumières de nuit.
    std::array<float, 4> up{0.0F, -1.0F, 0.0F, 0.0F};
    /// La position de chaque lumière dans la vue ; `w` : sa portée, en unités de la vue.
    std::array<std::array<float, 4>, MAXIMUM_SCENE_LIGHTS> lightPosition{};
    /// La couleur de chaque lumière, intensité et allumage compris.
    std::array<std::array<float, 4>, MAXIMUM_SCENE_LIGHTS> lightColor{};
};
static_assert(sizeof(LightingUniforms) == (16 + (5 * 4) + (2 * 4 * MAXIMUM_SCENE_LIGHTS)) * 4,
              "LightingUniforms doit rester le bloc std140 des shaders, sans bourrage");

/// @brief L'éclairage d'une image : le bloc des shaders, et la projection de la carte d'ombres.
struct SceneLightFrame {
    LightingUniforms uniforms{};
    /// Du repère du lieu en mètres (X colonnes, Y hauteur, Z lignes) vers le clip de la carte
    /// d'ombres, à la convention d'OpenGL que QRhi impose à la source : x, y et la profondeur
    /// dans [−1, 1]. Sans objet sans ombres.
    LightMatrix metresToShadowClip{};
    /// Vrai si l'image a une passe d'ombres à dessiner.
    bool shadows = false;
};

/// @return La matrice qui amène un point du lieu, en mètres, dans la vue de @p view.
[[nodiscard]] LightMatrix metresToView(const IsoView& view) noexcept;

/// @return L'inverse de la matrice **affine** @p matrix ; l'identité si elle ne s'inverse pas.
[[nodiscard]] LightMatrix invertedAffine(const LightMatrix& matrix) noexcept;

/// @return Le produit @p left × @p right.
[[nodiscard]] LightMatrix multiplied(const LightMatrix& left, const LightMatrix& right) noexcept;

/// @return La pose @p transform (trois lignes, quatre colonnes) en matrice 4 × 4.
[[nodiscard]] LightMatrix toLightMatrix(const ViewTransform& transform) noexcept;

/**
 * @brief L'éclairage d'une image.
 *
 * @param view           La vue en volume du lieu.
 * @param visible        Ce que la caméra montre, en unités monde (`PlaceCamera::visibleBounds`).
 * @param lighting       La lumière de l'heure et les réglages d'ombre.
 * @param sources        Les sources du lieu ; celles qui sont allumées et proches sont gardées.
 * @param flipShadowRows Vrai si la première ligne d'une texture rendue est le **haut** du clip
 *                       (Direct3D, Vulkan, Metal) ; faux sous OpenGL.
 */
[[nodiscard]] SceneLightFrame buildSceneLighting(const IsoView& view, const core::Rect& visible,
                                                 const WorldLighting& lighting,
                                                 std::span<const core::LightSource> sources,
                                                 bool flipShadowRows);

}  // namespace hmi
