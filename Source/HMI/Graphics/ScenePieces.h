// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "Core/Levels/GridPosition.h"
#include "Core/Math/Rect.h"
#include "Core/Math/Vector2.h"
#include "HMI/Graphics/Quad.h"
#include "HMI/Graphics/RenderLayer.h"

/**
 * @file HMI/Graphics/ScenePieces.h
 * @brief La géométrie des **pièces de scène**, commune à l'arène et aux lieux qu'on parcourt.
 *
 * Un lieu livre ses pièces avec un manifeste (`LOT-104`) : chaque pièce est posée par son
 * **ancre**, le sommet haut du losange de son emprise, et le manifeste déclare le losange de sol
 * de l'art (`"tile": [256, 159]`, `EX-VIS-008`). C'est de ce losange que vient l'échelle d'une
 * pièce à l'écran : une pièce dont le losange fait 256 pixels d'art occupe exactement la largeur
 * d'une case de la projection. Aucune taille d'art n'est écrite ici (`LOT-103`) : un lieu livré
 * plus fin ou plus grossier se dessine à la bonne taille sans toucher au code.
 *
 * Ce que les fichiers voisins disent d'une image — découpe, échelle, ancre — se lit par
 * `hmi::readSceneTextureTraits` (`SceneTextureTraits.h`).
 */

namespace hmi {

/// @brief Une texture liable, ses dimensions en pixels et ce que ses fichiers voisins disent
/// d'elle.
struct SceneTexture {
    TextureHandle texture = nullptr;
    int width = 0;
    int height = 0;
    /// Largeur d'une image si la texture est une bande d'animation ; 0 sinon.
    int frameWidth = 0;
    /// Hauteur d'une image de la bande ; 0 : la hauteur de la texture.
    int frameHeight = 0;
    /// Origine explicite de la pièce, en pixels d'art du PNG.
    std::optional<core::Vector2> anchor{};
    /// Somme des coordonnees locales du pied utilise pour le tri, en cases.
    std::optional<float> depthOffset{};
    /// Le losange de sol de l'art, en pixels (`tile` du manifeste) ; (0, 0) s'il n'est pas déclaré.
    core::Vector2 artTile{};
    /// Ligne de sol d'une figurine, en pixels depuis le haut de sa cellule (`LOT-112`) ; sans elle,
    /// la figurine se pose par le bas de sa cellule.
    std::optional<float> groundLine{};
    /// Durée d'une image de la bande, en secondes (`.anim.json`) ; 0 si la bande n'en dit rien.
    float frameDuration = 0.0F;
    /// Hauteur d'un étage du lieu, en pixels d'art (`storey` du manifeste, `LOT-129`) : ce dont une
    /// pièce posée sur la couche d'étage `floor = n` s'élève, n fois.
    std::optional<float> storeyHeight{};
};

/**
 * @brief La largeur, en pixels d'art, d'une case de la projection : ce que le manifeste déclare.
 *
 * À défaut, l'image se mesure à elle-même, sans supposer aucune taille d'art : une pièce se
 * suppose **une case de large** (une pièce d'une case a la largeur de son losange), une bande
 * d'animation **une case de haut** — sa cellule est commune à toutes ses bandes, là où sa largeur
 * double pour une attaque, et elle vaut une case dans l'ancien standard (64 pour 68) comme dans le
 * nouveau (256 pour 256). Un lieu livré déclare toujours son losange
 * (`scripts/checks/check_hd_assets.py`).
 */
[[nodiscard]] inline float artTileWidth(const SceneTexture& texture) noexcept {
    if (texture.artTile.x > 0.0F) {
        return texture.artTile.x;
    }
    if (texture.frameHeight > 0) {
        return static_cast<float>(texture.frameHeight);
    }
    return static_cast<float>(
        std::max(1, texture.frameWidth > 0 ? texture.frameWidth : texture.width));
}

/// @return La hauteur, en pixels d'art, du losange d'une case ; à défaut, la largeur × @p ratio.
[[nodiscard]] inline float artTileHeight(const SceneTexture& texture, float ratio) noexcept {
    return texture.artTile.y > 0.0F ? texture.artTile.y : artTileWidth(texture) * ratio;
}

/// @return La largeur d'une image de la bande, la texture entière pour une image fixe.
[[nodiscard]] inline int frameWidthOf(const SceneTexture& texture) noexcept {
    return texture.frameWidth > 0 ? texture.frameWidth : texture.width;
}

/// @return La hauteur d'une image de la bande, la texture entière pour une image fixe.
[[nodiscard]] inline int frameHeightOf(const SceneTexture& texture) noexcept {
    return texture.frameHeight > 0 ? std::min(texture.frameHeight, texture.height) : texture.height;
}

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
 * @brief Pose une pièce **debout** par son ancre, à l'échelle de son lieu.
 *
 * @param texture   La pièce à poser, déjà chargée (sa taille donne le quad).
 * @param topVertex Sommet haut du losange de la case, en unités monde.
 * @param tileWidth Largeur du losange de la projection, en unités monde : le losange de l'art
 *                  (`hmi::artTileWidth`) s'y ramène.
 * @param ratio     Rapport hauteur / largeur du losange, pour l'ancre par défaut d'une pièce qui
 *                  n'en déclare pas (le bas de l'image est le losange de sa case).
 * @return Le quad à ajouter à la scène ; sa texture est nulle si la pièce ne se dessine pas.
 */
[[nodiscard]] inline SpriteQuad standingPieceQuad(const SceneTexture& texture,
                                                  core::Vector2 topVertex, float tileWidth,
                                                  float ratio) {
    SpriteQuad quad;
    const float art = artTileWidth(texture);
    const float unitsPerPixel = tileWidth / art;
    const auto height = static_cast<float>(texture.height);
    const core::Vector2 anchor =
        texture.anchor.value_or(core::Vector2{art / 2.0F, height - artTileHeight(texture, ratio)});
    quad.x = topVertex.x - anchor.x * unitsPerPixel;
    quad.y = topVertex.y - anchor.y * unitsPerPixel;
    quad.width = static_cast<float>(texture.width) * unitsPerPixel;
    quad.height = height * unitsPerPixel;
    return quad;
}

/**
 * @brief Débord d'une dalle de sol au-delà de son losange, de chaque côté, en largeurs de case.
 *
 * Une dalle HD a le bord adouci (alpha continu, `EX-VIS-008`) : deux losanges jointifs à l'arête
 * près laissent passer, là où chacun n'est qu'à demi opaque, le fond sous la couture — un treillis
 * sombre sur tout le sol. La maquette du `LOT-101` débordait d'un pixel d'art sur 256 ; le moteur
 * en fait autant, exprimé en fraction de case (`LOT-103`).
 */
inline constexpr float FLOOR_SEAM_OVERLAP = 1.0F / 256.0F;

/**
 * @brief Le quad d'une dalle de sol : la boîte de son losange, élargie de `FLOOR_SEAM_OVERLAP`.
 * @param bounds La boîte du losange de la case (`core::IsoProjection::tileBounds`).
 */
[[nodiscard]] inline SpriteQuad floorQuad(const core::Rect& bounds) noexcept {
    // Le meme debord relatif sur les deux axes : le losange grandit sans changer de rapport.
    const float grow = FLOOR_SEAM_OVERLAP * 2.0F;
    SpriteQuad quad;
    quad.width = bounds.size.x * (1.0F + grow);
    quad.height = bounds.size.y * (1.0F + grow);
    quad.x = bounds.position.x - ((quad.width - bounds.size.x) / 2.0F);
    quad.y = bounds.position.y - ((quad.height - bounds.size.y) / 2.0F);
    return quad;
}

/// @return Le nombre d'images de la bande @p texture (au moins 1).
[[nodiscard]] inline int frameCountOf(const SceneTexture& texture) noexcept {
    const int frameWidth = frameWidthOf(texture);
    return frameWidth > 0 ? std::max(1, texture.width / frameWidth) : 1;
}

/**
 * @brief Pose une **figurine** : l'image @p frame de sa bande, **entière**, à l'échelle de son art.
 *
 * La cellule se lit dans la bande (`frameWidth` et `frameHeight` de son `.anim.json`) : une
 * figurine de 192 × 256 et une créature de 384 × 384 s'affichent entières, sans hauteur supposée.
 * L'art est livré à sa taille finale (`EX-VIS-008`) : aucun agrandissement ne s'y ajoute.
 *
 * @param texture   La bande, déjà chargée.
 * @param frame     Rang de l'image dans la bande, déjà ramené dans `[0, frameCountOf(texture))`.
 * @param centerX   Abscisse du centre de la cellule, en unités monde.
 * @param bottomY   Ordonnée du bas de la cellule, en unités monde.
 * @param tileWidth Largeur du losange de la projection, en unités monde.
 */
[[nodiscard]] inline SpriteQuad figureQuad(const SceneTexture& texture, int frame, float centerX,
                                           float bottomY, float tileWidth) {
    const float unitsPerPixel = tileWidth / artTileWidth(texture);
    const int frameWidth = frameWidthOf(texture);
    const int frameHeight = frameHeightOf(texture);
    SpriteQuad quad;
    quad.width = static_cast<float>(frameWidth) * unitsPerPixel;
    quad.height = static_cast<float>(frameHeight) * unitsPerPixel;
    quad.x = centerX - (quad.width / 2.0F);
    quad.y = bottomY - quad.height;
    if (texture.width > 0 && texture.height > 0) {
        const auto width = static_cast<float>(texture.width);
        quad.u0 = static_cast<float>(frame * frameWidth) / width;
        quad.u1 = static_cast<float>((frame + 1) * frameWidth) / width;
        quad.v0 = 0.0F;
        quad.v1 = static_cast<float>(frameHeight) / static_cast<float>(texture.height);
    }
    return quad;
}

}  // namespace hmi
