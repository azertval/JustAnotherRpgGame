// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/SpriteRenderer.h"

#include <array>
#include <cstddef>
#include <vector>

namespace hmi {

namespace {

// Un rectangle dans la scene en volume (LOT-1003) : a plat, il suit le sol ; dresse, il est un plan
// vertical au-dessus de la ligne de son pied et le sol au-dessous -- deux rectangles quand cette
// ligne le traverse, la profondeur n'etant plus lineaire d'un bord a l'autre.
void drawSpriteInDepth(SpriteBatch& batch, const ComposedQuad& composed, const SceneDepth& depth) {
    const SpriteQuad& quad = composed.sprite;
    if (composed.stance == QuadStance::Overlay) {
        batch.draw(quad, depth.overlayDepth(), depth.overlayDepth());
        return;
    }
    const float bias = depth.view.imageBias();
    const float top = quad.y;
    const float bottom = quad.y + quad.height;
    if (composed.stance == QuadStance::Ground) {
        batch.draw(quad, depth.view.groundDepth(top) - bias, depth.view.groundDepth(bottom) - bias);
        return;
    }
    const float foot = composed.footY;
    // Un rectangle tourne ne se decoupe pas le long d'une horizontale : il reste d'un seul plan.
    if (foot <= top || foot >= bottom || quad.rotation != 0.0F || !(quad.height > 0.0F)) {
        batch.draw(quad, depth.view.standingDepth(foot, top) - bias,
                   depth.view.standingDepth(foot, bottom) - bias);
        return;
    }
    const float footDepth = depth.view.groundDepth(foot) - bias;
    const float split = (foot - top) / quad.height;
    SpriteQuad upper = quad;
    upper.height = foot - top;
    upper.v1 = quad.v0 + ((quad.v1 - quad.v0) * split);
    batch.draw(upper, depth.view.standingDepth(foot, top) - bias, footDepth);
    SpriteQuad lower = quad;
    lower.y = foot;
    lower.height = bottom - foot;
    lower.v0 = upper.v1;
    batch.draw(lower, footDepth, depth.view.groundDepth(bottom) - bias);
}

void drawInDepth(SpriteBatch& batch, const ComposedQuad& composed, const SceneDepth& depth) {
    const bool overlay = composed.stance == QuadStance::Overlay;
    const float bias = depth.view.imageBias();
    switch (composed.kind) {
        case QuadKind::Sprite:
            drawSpriteInDepth(batch, composed, depth);
            break;
        case QuadKind::Line:
            batch.draw(
                composed.line,
                overlay ? depth.overlayDepth() : depth.view.groundDepth(composed.line.ay) - bias,
                overlay ? depth.overlayDepth() : depth.view.groundDepth(composed.line.by) - bias);
            break;
        case QuadKind::Poly: {
            // Chaque sommet dit son elevation : le sol sous lui, puis ce dont il s'en eleve.
            std::array<float, 4> depths{};
            for (std::size_t i = 0; i < depths.size(); ++i) {
                const float rise = composed.poly.rise[i];
                depths[i] = overlay
                                ? depth.overlayDepth()
                                : depth.view.raisedDepth(composed.poly.y[i] + rise, rise) - bias;
            }
            batch.draw(composed.poly, depths);
            break;
        }
    }
}

}  // namespace

// Soumet une scene composee au pipeline de dessin, une passe par groupe de texture.
void submitComposedScene(SpriteBatch& batch, const DirectX::XMFLOAT4X4& projection,
                         const ComposedScene& scene, const SceneDepth* depth) {
    const std::vector<ComposedQuad>& quads = scene.quads();
    TextureHandle current = nullptr;
    bool open = false;

    for (const ComposedQuad& composed : quads) {
        if (composed.texture == nullptr) {
            continue;  // primitive sans texture liee : rien a dessiner (robustesse).
        }
        if (!open || composed.texture != current) {
            if (open) {
                batch.end();
            }
            // L'identite opaque traverse la chaine telle quelle : ni la composition ni la
            // soumission ne connaissent le type reel de la texture (cf. hmi::TextureHandle).
            batch.begin(projection, composed.texture);
            current = composed.texture;
            open = true;
        }
        if (depth != nullptr) {
            drawInDepth(batch, composed, *depth);
            continue;
        }
        switch (composed.kind) {
            case QuadKind::Sprite:
                batch.draw(composed.sprite);
                break;
            case QuadKind::Line:
                batch.draw(composed.line);
                break;
            case QuadKind::Poly:
                batch.draw(composed.poly);
                break;
        }
    }

    if (open) {
        batch.end();
    }
}

// Projection ecran -> clip, en pixels, origine haut-gauche.
DirectX::XMFLOAT4X4 screenProjectionMatrix(int viewportWidth, int viewportHeight) noexcept {
    const float width = viewportWidth > 0 ? static_cast<float>(viewportWidth) : 1.0F;
    const float height = viewportHeight > 0 ? static_cast<float>(viewportHeight) : 1.0F;

    // Lignes de la matrice (_11.._14, _21.._24, ...), les autres coefficients a zero.
    return {2.0F / width, 0.0F,           0.0F, 0.0F,  //
            0.0F,         -2.0F / height, 0.0F, 0.0F,  //
            0.0F,         0.0F,           1.0F, 0.0F,  //
            -1.0F,        1.0F,           0.0F, 1.0F};
}

}  // namespace hmi
