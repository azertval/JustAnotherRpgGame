// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <DirectXMath.h>

#include "Core/Math/Rect.h"
#include "Core/Math/Vector2.h"

/**
 * @file HMI/Graphics/Camera2D.h
 * @brief Caméra 2D : conversion entre unités monde et pixels écran.
 */

namespace hmi {

/**
 * @brief Caméra 2D orthographique : projette le monde vers l'écran pour le rendu.
 *
 * La caméra est centrée sur une position **en unités monde** et applique l'échelle
 * **16 pixels/unité** (`EX-ARCH-021`), avec l'origine écran en haut-gauche et l'axe Y vers
 * le bas (convention du projet). Un facteur de **zoom**, libre (`EX-ARCH-022`, `EX-REN-013`),
 * multiplie cette échelle. La caméra est un objet de
 * **présentation** : elle lit des positions monde mais ne modifie jamais l'ECS.
 */
class Camera2D {
public:
    /// Nombre de pixels par unité monde (`EX-ARCH-021`).
    static constexpr float PIXELS_PER_UNIT = 16.0f;

    /**
     * @brief Construit une caméra pour une surface de rendu donnée.
     * @param viewportWidth  Largeur de la surface de rendu, en pixels.
     * @param viewportHeight Hauteur de la surface de rendu, en pixels.
     */
    Camera2D(int viewportWidth, int viewportHeight);

    /**
     * @brief Met à jour les dimensions de la surface de rendu (redimensionnement).
     * @param viewportWidth  Nouvelle largeur, en pixels.
     * @param viewportHeight Nouvelle hauteur, en pixels.
     */
    void setViewportSize(int viewportWidth, int viewportHeight);

    /**
     * @brief Place le centre de la caméra.
     * @param worldCenter Position visée, au centre de l'écran, en unités monde.
     */
    void setCenter(const core::Vector2& worldCenter);

    /**
     * @brief Règle le facteur de zoom.
     * @param zoom Multiplicateur d'échelle (> 0).
     */
    void setZoom(float zoom);

    /// @return Le centre courant de la caméra, en unités monde.
    [[nodiscard]] const core::Vector2& center() const noexcept {
        return _center;
    }

    /// @return Le facteur de zoom courant.
    [[nodiscard]] float zoom() const noexcept {
        return _zoom;
    }

    /**
     * @brief Matrice de projection monde → clip, pour le vertex shader.
     * @return La matrice (ligne-major DirectXMath) transformant une position monde en clip.
     */
    [[nodiscard]] DirectX::XMFLOAT4X4 projectionMatrix() const;

    /**
     * @brief Convertit une position monde en pixels écran.
     * @param world Position en unités monde.
     * @return Position en pixels (origine haut-gauche, Y-bas).
     */
    [[nodiscard]] core::Vector2 worldToScreen(const core::Vector2& world) const;

    /**
     * @brief Convertit une position écran (pixels) en unités monde.
     * @param screen Position en pixels (origine haut-gauche, Y-bas).
     * @return Position en unités monde.
     */
    [[nodiscard]] core::Vector2 screenToWorld(const core::Vector2& screen) const;

    /**
     * @brief Rectangle du monde effectivement cadré par la caméra.
     *
     * Base du **culling** (`EX-NFR-005`) : une primitive dont la boîte englobante n'intersecte pas
     * ce rectangle (élargi d'une marge, cf. `hmi::ComposedScene::CULLING_MARGIN_UNITS`) n'a aucune
     * raison d'être soumise. Dérivé de `screenToWorld` : aucune notion de cadrage nouvelle n'est
     * introduite, la caméra reste la seule source de vérité.
     * @return Le rectangle visible, en unités monde (coin haut-gauche + dimensions).
     */
    [[nodiscard]] core::Rect visibleBounds() const;

    /**
     * @brief Facteur de zoom ajustant un contenu à une surface disponible, sans zone hors champ.
     *
     * Le facteur est celui qui fait tenir le contenu, **sans arrondi** (`EX-REN-013`,
     * `EX-EDIT-013`). Il s'arrondissait à l'entier au-dessus de 1 pour la netteté du pixel art ;
     * l'art de scène est désormais peint et filtré par mipmaps (`EX-ARCH-022`, `LOT-103`), et
     * l'arrondi ne faisait plus que laisser des bords vides.
     *
     * Fonction **pure**, partagée par le cadrage automatique de l'éditeur et celui du jeu (aucune
     * règle dupliquée entre les deux écrans, LOT-16).
     * @param availableWidth  Largeur disponible, en pixels (> 0).
     * @param availableHeight Hauteur disponible, en pixels (> 0).
     * @param contentWidth    Largeur du contenu à cadrer, en unités monde (> 0).
     * @param contentHeight   Hauteur du contenu à cadrer, en unités monde (> 0).
     * @param margin          Facteur multiplicatif appliqué avant l'arrondi (ex. `0.85` pour
     *                        laisser une marge visuelle) ; `1.0` par défaut (aucune marge).
     * @return Le facteur de zoom à appliquer via `setZoom`.
     */
    [[nodiscard]] static float fitZoom(float availableWidth, float availableHeight,
                                       float contentWidth, float contentHeight,
                                       float margin = 1.0f);

private:
    /// @return L'échelle effective, en pixels par unité monde (PIXELS_PER_UNIT × zoom).
    [[nodiscard]] float scale() const;

    int _viewportWidth;
    int _viewportHeight;
    core::Vector2 _center{};
    float _zoom = 1.0f;
};

}  // namespace hmi
