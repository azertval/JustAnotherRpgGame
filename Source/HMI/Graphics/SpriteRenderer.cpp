// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/SpriteRenderer.h"

#include <vector>

namespace hmi {

// Soumet une scene composee au pipeline de dessin, une passe par groupe de texture.
void submitComposedScene(SpriteBatch& batch, const DirectX::XMFLOAT4X4& projection,
                         const ComposedScene& scene) {
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
