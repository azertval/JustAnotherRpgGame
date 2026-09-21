// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "Core/Levels/GridPosition.h"
#include "Core/Math/Vector2.h"
#include "HMI/Graphics/Quad.h"
#include "HMI/Graphics/RenderLayer.h"

/**
 * @file HMI/Graphics/ScenePieces.h
 * @brief La géométrie des **pièces de l'atelier des textures** (`LOT-92`), commune au Colisée en
 *        combat et aux lieux qu'on parcourt (`LOT-09`).
 *
 * L'atelier livre, pour chaque lieu, une planche découpée en pièces : un losange de 68 × 42 pixels
 * d'art (le rapport 0,62 d'`core::IsoProjection`), chaque pièce posée par son **ancre**, le sommet
 * haut du losange de son emprise. Une pièce d'une case a donc la largeur du losange, dont les 42
 * pixels du bas ; ce qui dépasse monte au-dessus de la case.
 *
 * Ces constantes vivaient dans `hmi::ArenaSceneComposer`, qui fut le premier à les lire. Le monde
 * exploré lit les mêmes planches : les garder en deux copies ferait de leur égalité une
 * coïncidence.
 */

namespace hmi {

/// Largeur du losange des textures de scène de l'atelier, en pixels d'art.
inline constexpr int SCENE_TILE_WIDTH_PIXELS = 68;
/// Demi-largeur de ce losange : l'abscisse de l'ancre d'une pièce d'une case.
inline constexpr int SCENE_HALF_TILE_WIDTH_PIXELS = SCENE_TILE_WIDTH_PIXELS / 2;
/// Hauteur de ce losange : il occupe le bas d'une pièce d'une case.
inline constexpr int SCENE_TILE_HEIGHT_PIXELS = 42;

/// Largeur d'une image des bandes d'animation des figurines (`Assets/Npc/manifest.json`).
inline constexpr int FIGURE_FRAME_WIDTH_PIXELS = 48;
/// Hauteur d'une image de ces bandes.
inline constexpr int FIGURE_FRAME_HEIGHT_PIXELS = 64;
/// Agrandissement d'une figurine par rapport à la planche.
inline constexpr float FIGURE_SCALE = 1.25F;

/// @brief Une texture liable et ses dimensions en pixels (pour normaliser les UV).
struct SceneTexture {
    TextureHandle texture = nullptr;
    int width = 0;
    int height = 0;
    /// Largeur d'une image si la texture est une bande d'animation ; 0 sinon.
    int frameWidth = 0;
    /// Origine explicite des pieces modulaires, en pixels d'art du PNG.
    std::optional<core::Vector2> anchor{};
    /// Somme des coordonnees locales du pied utilise pour le tri, en cases.
    std::optional<float> depthOffset{};
};

/**
 * @brief Les textures d'un lieu, adressées par leur **chemin** tel que la composition l'écrit.
 *
 * La composition ne demande rien au GPU : elle résout un chemin en texture déjà chargée. Un chemin
 * absent tombe sur le damier (`EX-NFR-040`) ; si lui aussi manque, la pièce n'est pas composée.
 */
struct ScenePieceTextures {
    /// Comparateur transparent : la recherche se fait sans chaîne temporaire.
    std::map<std::string, SceneTexture, std::less<>> byPath;
    /// Damier de repli.
    SceneTexture missing;
    /// L'**aplat** : une texture blanche de 1 × 1, que les primitives de couleur du rendu de
    /// maquette lient pour n'être que leur teinte (`LOT-128`). Nulle : pas de maquette dessinée.
    SceneTexture solid;

    /// @return La texture de @p path, le damier si elle n'est pas chargée.
    [[nodiscard]] const SceneTexture& resolve(std::string_view path) const {
        const auto found = byPath.find(path);
        return found != byPath.end() ? found->second : missing;
    }

    /**
     * @brief La texture de @p path, **sans** repli sur le damier.
     *
     * Pour ce qui n'a de sens que dessiné juste : un jeton de maquette (`LOT-128`) est peint ou
     * n'est pas là — un damier à sa place ne dirait rien à personne, et se ferait passer pour une
     * pièce manquante.
     * @return La texture, ou `nullptr` si elle n'est pas chargée.
     */
    [[nodiscard]] const SceneTexture* find(std::string_view path) const {
        const auto found = byPath.find(path);
        return found != byPath.end() ? &found->second : nullptr;
    }
};

/**
 * @brief Pose une pièce **debout** d'une case par son ancre.
 *
 * @param texture La pièce à poser, déjà chargée (sa taille donne le quad).
 * @param topVertex Sommet haut du losange de la case, en unités monde.
 * @param unitsPerScenePixel Unités monde par pixel d'art (un losange de `SCENE_TILE_WIDTH_PIXELS`
 *                           occupe la largeur du losange de la projection).
 * @return Le quad à ajouter à la scène ; sa texture est nulle si la pièce ne se dessine pas.
 */
[[nodiscard]] inline SpriteQuad standingPieceQuad(const SceneTexture& texture,
                                                  core::Vector2 topVertex,
                                                  float unitsPerScenePixel) {
    SpriteQuad quad;
    const auto height = static_cast<float>(texture.height);
    const core::Vector2 anchor = texture.anchor.value_or(
        core::Vector2{static_cast<float>(SCENE_HALF_TILE_WIDTH_PIXELS),
                      height - static_cast<float>(SCENE_TILE_HEIGHT_PIXELS)});
    quad.x = topVertex.x - anchor.x * unitsPerScenePixel;
    quad.y = topVertex.y - anchor.y * unitsPerScenePixel;
    quad.width = static_cast<float>(texture.width) * unitsPerScenePixel;
    quad.height = height * unitsPerScenePixel;
    return quad;
}

}  // namespace hmi
