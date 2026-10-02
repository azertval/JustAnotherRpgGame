// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/PlaceCamera.h"

#include <algorithm>
#include <cmath>

namespace hmi {

namespace {

// Les trois coefficients d'un axe du plan de l'image : ce qui multiplie la coordonnee, et ce qui
// s'y ajoute.
struct AxisScale {
    float scale;
    float translate;
};

// L'axe de la profondeur : [nearest, farthest] ramene a [-1, 1]. Une etendue vide ou a l'envers
// laisse la profondeur telle quelle plutot que de diviser par zero.
[[nodiscard]] AxisScale depthAxis(const DepthRange& range) noexcept {
    const float span = range.farthest - range.nearest;
    if (!(span > 0.0F)) {
        return AxisScale{.scale = 1.0F, .translate = 0.0F};
    }
    // `0 - x` et non `-x` : l'etendue par defaut rend un zero positif, comme la matrice d'avant.
    return AxisScale{.scale = 2.0F / span,
                     .translate = 0.0F - ((range.farthest + range.nearest) / span)};
}

}  // namespace

// Construit une caméra pour une surface de rendu donnée.
PlaceCamera::PlaceCamera(int viewportWidth, int viewportHeight)
    : _viewportWidth(viewportWidth), _viewportHeight(viewportHeight) {}

// Place le centre de la caméra.
void PlaceCamera::setCenter(const core::Vector2& worldCenter) {
    _center = worldCenter;
}

// Règle le facteur de zoom.
void PlaceCamera::setZoom(float zoom) {
    _zoom = zoom;
}

void PlaceCamera::setDepthRange(const DepthRange& range) {
    _depth = range;
}

// L'échelle effective, en pixels par unité monde (PIXELS_PER_UNIT × zoom).
float PlaceCamera::scale() const {
    return PIXELS_PER_UNIT * _zoom;
}

// Matrice de projection vue → clip, pour le vertex shader.
// La matrice (ligne-major DirectXMath) transformant une position de la vue en clip.
DirectX::XMFLOAT4X4 PlaceCamera::projectionMatrix() const {
    // Échelle monde → clip sur chaque axe : la moitié d'écran (viewport/2 pixels) doit
    // couvrir 1 en NDC. L'axe Y est inversé (monde Y-bas, NDC Y-haut).
    const float scaleX = scale() * 2.0F / static_cast<float>(_viewportWidth);
    const float scaleY = scale() * 2.0F / static_cast<float>(_viewportHeight);
    const float translateX = -_center.x * scaleX;
    const float translateY = _center.y * scaleY;
    const AxisScale depth = depthAxis(_depth);

    // Ligne-major, appliquée en `position * matrice` (convention DirectXMath) :
    // clip.x =  scaleX * (x - cx) ; clip.y = -scaleY * (y - cy) ; clip.z = profondeur ramenee ;
    // clip.w = 1. Les deux premiers axes sont ceux de la camera 2D d'avant le LOT-1003, au
    // flottant pres : une image posee dans le plan occupe les memes pixels.
    return {scaleX,     0.0F,       0.0F,
            0.0F,  //
            0.0F,       -scaleY,    0.0F,
            0.0F,  //
            0.0F,       0.0F,       depth.scale,
            0.0F,  //
            translateX, translateY, depth.translate,
            1.0F};
}

// Matrice maillage -> clip : la pose du maillage dans la vue, puis la projection.
DirectX::XMFLOAT4X4 PlaceCamera::meshMatrix(const ViewTransform& transform) const {
    const DirectX::XMFLOAT4X4 projection = projectionMatrix();
    const float scaleX = projection(0, 0);
    const float scaleY = projection(1, 1);
    const float scaleZ = projection(2, 2);
    // Chaque colonne de la matrice est un axe du clip ; chaque ligne, un axe du maillage (puis la
    // translation).
    return {scaleX * transform[0],
            scaleY * transform[4],
            scaleZ * transform[8],
            0.0F,  //
            scaleX * transform[1],
            scaleY * transform[5],
            scaleZ * transform[9],
            0.0F,  //
            scaleX * transform[2],
            scaleY * transform[6],
            scaleZ * transform[10],
            0.0F,  //
            (scaleX * transform[3]) + projection(3, 0),
            (scaleY * transform[7]) + projection(3, 1),
            (scaleZ * transform[11]) + projection(3, 2),
            1.0F};
}

// Convertit une position monde en pixels écran.
// Position en pixels (origine haut-gauche, Y-bas).
core::Vector2 PlaceCamera::worldToScreen(const core::Vector2& world) const {
    const float halfWidth = static_cast<float>(_viewportWidth) * 0.5F;
    const float halfHeight = static_cast<float>(_viewportHeight) * 0.5F;
    return core::Vector2{((world.x - _center.x) * scale()) + halfWidth,
                         ((world.y - _center.y) * scale()) + halfHeight};
}

// Convertit une position écran (pixels) en unités monde.
// Position en unités monde.
core::Vector2 PlaceCamera::screenToWorld(const core::Vector2& screen) const {
    const float halfWidth = static_cast<float>(_viewportWidth) * 0.5F;
    const float halfHeight = static_cast<float>(_viewportHeight) * 0.5F;
    return core::Vector2{((screen.x - halfWidth) / scale()) + _center.x,
                         ((screen.y - halfHeight) / scale()) + _center.y};
}

// Rectangle du monde effectivement cadre par la camera (base du culling, EX-NFR-005).
// Le rectangle visible, en unites monde (coin haut-gauche + dimensions).
core::Rect PlaceCamera::visibleBounds() const {
    const core::Vector2 topLeft = screenToWorld(core::Vector2{0.0F, 0.0F});
    const core::Vector2 size{static_cast<float>(_viewportWidth) / scale(),
                             static_cast<float>(_viewportHeight) / scale()};
    return core::Rect{topLeft, size};
}

// Facteur de zoom ajustant un contenu a une surface disponible, sans zone hors champ (LOT-16).
// Libre depuis le LOT-103 : l'arrondi a l'entier ne protegeait que la grille du pixel art.
float PlaceCamera::fitZoom(float availableWidth, float availableHeight, float contentWidth,
                           float contentHeight, float margin) {
    const float fitX = availableWidth / (contentWidth * PIXELS_PER_UNIT);
    const float fitY = availableHeight / (contentHeight * PIXELS_PER_UNIT);
    return (std::min)(fitX, fitY) * margin;
}

}  // namespace hmi
