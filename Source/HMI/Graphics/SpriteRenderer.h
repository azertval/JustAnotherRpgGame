// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <DirectXMath.h>

#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/IsoView.h"
#include "HMI/Graphics/SpriteBatch.h"

/**
 * @file HMI/Graphics/SpriteRenderer.h
 * @brief Soumission d'une scène composée au pipeline de dessin.
 */

namespace hmi {

/**
 * @brief Ce qu'il faut pour donner sa **profondeur** à chaque image d'une scène qui a des
 *        maillages (`LOT-1003`) : la vue en volume, et l'étendue que la caméra en garde.
 */
struct SceneDepth {
    IsoView view;
    DepthRange range;

    /// @return La profondeur d'une marque (`QuadStance::Overlay`) : juste derrière le plan le
    ///         plus proche, donc devant tout volume.
    [[nodiscard]] float overlayDepth() const noexcept {
        return range.nearest + ((range.farthest - range.nearest) * OVERLAY_FRACTION);
    }

    /// La part de l'étendue qui sépare une marque du plan le plus proche : assez pour qu'un
    /// arrondi ne la fasse pas sortir du volume de la caméra.
    static constexpr float OVERLAY_FRACTION = 1.0e-4F;
};

/**
 * @brief Soumet une scène composée au pipeline de dessin, une passe par groupe de texture.
 *
 * Seul endroit du rendu qui reconvertit une `hmi::TextureHandle` en ressource Direct3D : c'est la
 * **frontière** entre la composition (pure, testable sans GPU) et la soumission. Émet un
 * `SpriteBatch::begin/end` par groupe **contigu** de même texture, dans l'ordre de la scène — donc
 * dans l'ordre des calques, que `ComposedScene::sort()` a rendu prioritaire (`EX-REN-043`). Le
 * contrat public de `hmi::SpriteBatch` est strictement inchangé.
 *
 * Sans @p depth, toutes les primitives sont à la profondeur zéro et aucune n'est découpée : les
 * sommets sont, au flottant près, ceux d'avant le `LOT-1003`. Avec, chaque primitive reçoit la
 * profondeur de sa tenue (`QuadStance`, `hmi::IsoView`) ; une image dressée que la ligne de son
 * pied traverse est soumise en **deux** rectangles, le plan vertical au-dessus, le sol au-dessous.
 * @param batch      Pipeline de quads texturés (non possédé).
 * @param projection Matrice de projection vue → clip (fournie par la caméra).
 * @param scene      Scène **déjà triée** (`ComposedScene::sort()`).
 * @param depth      La vue en volume d'une image qui a des maillages ; `nullptr` sinon.
 */
void submitComposedScene(SpriteBatch& batch, const DirectX::XMFLOAT4X4& projection,
                         const ComposedScene& scene, const SceneDepth* depth = nullptr);

/**
 * @brief Construit la projection **écran → clip**, indépendante de `PlaceCamera`.
 *
 * Pour ce qui se dessine en pixels d'écran plutôt qu'en unités monde (la galerie des assets) : la
 * projection ne dépend que des dimensions de la surface (origine haut-gauche, `Y` vers le bas —
 * même convention que le reste du rendu).
 * @param viewportWidth  Largeur de la surface de rendu, en pixels.
 * @param viewportHeight Hauteur de la surface de rendu, en pixels.
 */
[[nodiscard]] DirectX::XMFLOAT4X4 screenProjectionMatrix(int viewportWidth,
                                                         int viewportHeight) noexcept;

}  // namespace hmi
