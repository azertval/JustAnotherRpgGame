// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <DirectXMath.h>

#include "Core/Math/Rect.h"
#include "Core/Math/Vector2.h"
#include "HMI/Graphics/IsoView.h"

/**
 * @file HMI/Graphics/PlaceCamera.h
 * @brief La **caméra du lieu** (`LOT-1003`) : orthographique, elle cadre la vue en volume
 *        (`hmi::IsoView`) et ramène sa profondeur entre deux plans.
 */

namespace hmi {

/**
 * @brief Caméra orthographique d'un lieu : du repère de la vue (`hmi::IsoView`) à l'écran.
 *
 * L'orientation est fixe et n'est pas ici : tournée de 45°, élevée de asin 0,62 (38,3°), c'est
 * `hmi::IsoView` qui la porte, et `core::IsoProjection` pour le sol. La caméra ne fait que
 * **cadrer** : elle est centrée sur un point du plan de l'image, **en unités monde**, à l'échelle
 * de **16 pixels par unité** (`EX-ARCH-021`) que multiplie un facteur de **zoom** libre
 * (`EX-ARCH-022`, `EX-REN-013`) ; l'origine de l'écran est en haut à gauche, l'axe Y vers le bas.
 *
 * Elle remplace la caméra 2D d'avant le lot, dont elle garde le cadrage au flottant près : la
 * matrice des deux premiers axes est la même, et une image posée dans le plan occupe les mêmes
 * pixels. Ce qu'elle y ajoute est le troisième axe — la profondeur de la vue, ramenée de
 * `depthRange()` à l'étendue du tampon de profondeur — et la matrice d'un maillage posé
 * (`meshMatrix`).
 *
 * Objet de **présentation** : elle lit des positions, elle ne modifie jamais la simulation.
 */
class PlaceCamera {
public:
    /// Nombre de pixels par unité monde (`EX-ARCH-021`).
    static constexpr float PIXELS_PER_UNIT = 16.0f;

    /**
     * @brief Construit une caméra pour une surface de rendu donnée.
     * @param viewportWidth  Largeur de la surface de rendu, en pixels.
     * @param viewportHeight Hauteur de la surface de rendu, en pixels.
     */
    PlaceCamera(int viewportWidth, int viewportHeight);

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

    /**
     * @brief Fixe l'étendue de profondeur que la caméra ramène entre ses deux plans
     *        (`hmi::IsoView::depthRange`).
     *
     * Sans effet sur le plan de l'image. L'étendue par défaut, [-1, 1], laisse la profondeur telle
     * quelle : une scène sans volume, dont toutes les primitives sont à la profondeur zéro, se
     * dessine comme avant le lot.
     */
    void setDepthRange(const DepthRange& range);

    /// @return Le centre courant de la caméra, en unités monde.
    [[nodiscard]] const core::Vector2& center() const noexcept {
        return _center;
    }

    /// @return Le facteur de zoom courant.
    [[nodiscard]] float zoom() const noexcept {
        return _zoom;
    }

    [[nodiscard]] const DepthRange& depthRange() const noexcept {
        return _depth;
    }

    /**
     * @brief Matrice de projection vue → clip, pour le vertex shader.
     * @return La matrice (ligne-major DirectXMath) transformant une position de la vue en clip :
     *         le plan de l'image sur les deux premiers axes, la profondeur sur le troisième
     *         (convention OpenGL, de -1 au plus près à 1 au plus loin ; QRhi la ramène à celle du
     *         backend).
     */
    [[nodiscard]] DirectX::XMFLOAT4X4 projectionMatrix() const;

    /**
     * @brief Matrice maillage → clip d'un maillage posé par @p transform
     *        (`hmi::IsoView::meshTransform`) : la projection, composée avec la pose.
     */
    [[nodiscard]] DirectX::XMFLOAT4X4 meshMatrix(const ViewTransform& transform) const;

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
     * `EX-EDIT-013`) : l'art de scène est filtré par mipmaps (`EX-ARCH-022`, `LOT-103`), aucune
     * grille de pixels n'est à protéger.
     *
     * Fonction **pure**, partagée par le cadrage automatique de l'éditeur et celui du jeu (aucune
     * règle dupliquée entre les deux écrans, LOT-16).
     * @param availableWidth  Largeur disponible, en pixels (> 0).
     * @param availableHeight Hauteur disponible, en pixels (> 0).
     * @param contentWidth    Largeur du contenu à cadrer, en unités monde (> 0).
     * @param contentHeight   Hauteur du contenu à cadrer, en unités monde (> 0).
     * @param margin          Facteur multiplicatif (ex. `0.85` pour laisser une marge visuelle) ;
     *                        `1.0` par défaut (aucune marge).
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
    DepthRange _depth{};
};

}  // namespace hmi
