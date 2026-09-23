// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/Camera2D.h"

#include <algorithm>
#include <cmath>

namespace hmi {

// Construit une caméra pour une surface de rendu donnée.
Camera2D::Camera2D(int viewportWidth, int viewportHeight)
    : _viewportWidth(viewportWidth), _viewportHeight(viewportHeight) {}

// Met à jour les dimensions de la surface de rendu (redimensionnement).
void Camera2D::setViewportSize(int viewportWidth, int viewportHeight) {
    _viewportWidth = viewportWidth;
    _viewportHeight = viewportHeight;
}

// Place le centre de la caméra.
void Camera2D::setCenter(const core::Vector2& worldCenter) {
    _center = worldCenter;
}

// Règle le facteur de zoom.
void Camera2D::setZoom(float zoom) {
    _zoom = zoom;
}

// L'échelle effective, en pixels par unité monde (PIXELS_PER_UNIT × zoom).
float Camera2D::scale() const {
    return PIXELS_PER_UNIT * _zoom;
}

// Matrice de projection monde → clip, pour le vertex shader.
// La matrice (ligne-major DirectXMath) transformant une position monde en clip.
DirectX::XMFLOAT4X4 Camera2D::projectionMatrix() const {
    // Échelle monde → clip sur chaque axe : la moitié d'écran (viewport/2 pixels) doit
    // couvrir 1 en NDC. L'axe Y est inversé (monde Y-bas, NDC Y-haut).
    const float scaleX = scale() * 2.0F / static_cast<float>(_viewportWidth);
    const float scaleY = scale() * 2.0F / static_cast<float>(_viewportHeight);
    const float translateX = -_center.x * scaleX;
    const float translateY = _center.y * scaleY;

    // Ligne-major, appliquée en `position * matrice` (convention DirectXMath) :
    // clip.x =  scaleX * (x - cx) ; clip.y = -scaleY * (y - cy) ; clip.z = 0 ; clip.w = 1.
    return {scaleX,     0.0F,       0.0F, 0.0F,  //
            0.0F,       -scaleY,    0.0F, 0.0F,  //
            0.0F,       0.0F,       1.0F, 0.0F,  //
            translateX, translateY, 0.0F, 1.0F};
}

// Convertit une position monde en pixels écran.
// Position en pixels (origine haut-gauche, Y-bas).
core::Vector2 Camera2D::worldToScreen(const core::Vector2& world) const {
    const float halfWidth = static_cast<float>(_viewportWidth) * 0.5F;
    const float halfHeight = static_cast<float>(_viewportHeight) * 0.5F;
    return core::Vector2{((world.x - _center.x) * scale()) + halfWidth,
                         ((world.y - _center.y) * scale()) + halfHeight};
}

// Convertit une position écran (pixels) en unités monde.
// Position en unités monde.
core::Vector2 Camera2D::screenToWorld(const core::Vector2& screen) const {
    const float halfWidth = static_cast<float>(_viewportWidth) * 0.5F;
    const float halfHeight = static_cast<float>(_viewportHeight) * 0.5F;
    return core::Vector2{((screen.x - halfWidth) / scale()) + _center.x,
                         ((screen.y - halfHeight) / scale()) + _center.y};
}

// Rectangle du monde effectivement cadre par la camera (base du culling, EX-NFR-005).
// Le rectangle visible, en unites monde (coin haut-gauche + dimensions).
core::Rect Camera2D::visibleBounds() const {
    const core::Vector2 topLeft = screenToWorld(core::Vector2{0.0F, 0.0F});
    const core::Vector2 size{static_cast<float>(_viewportWidth) / scale(),
                             static_cast<float>(_viewportHeight) / scale()};
    return core::Rect{topLeft, size};
}

// Facteur de zoom ajustant un contenu a une surface disponible, sans zone hors champ (LOT-16).
// Libre depuis le LOT-103 : l'arrondi a l'entier ne protegeait que la grille du pixel art.
float Camera2D::fitZoom(float availableWidth, float availableHeight, float contentWidth,
                        float contentHeight, float margin) {
    const float fitX = availableWidth / (contentWidth * PIXELS_PER_UNIT);
    const float fitY = availableHeight / (contentHeight * PIXELS_PER_UNIT);
    return (std::min)(fitX, fitY) * margin;
}

}  // namespace hmi
