// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

#include "Core/Combat/IsoProjection.h"
#include "Core/Math/Rect.h"
#include "Core/Math/Vector2.h"
#include "Core/Rpg/Scale.h"

/**
 * @file HMI/Graphics/IsoView.h
 * @brief La **vue du lieu en volume** (`LOT-1003`) : ce que la caméra orthographique tournée de
 *        45° et inclinée de asin 0,62 fait d'un point de l'espace, profondeur comprise.
 *
 * `core::IsoProjection` projette le **sol** : une case de la grille tombe sur son losange. C'est
 * exactement ce que voit une caméra orthographique élevée d'un angle θ au-dessus de l'horizon, où
 * sin θ est le rapport du losange (0,62, soit 38,3°). Cette classe en tire les deux choses que le
 * sol seul ne disait pas : où tombe un point **élevé**, et à quelle **profondeur** se trouve un
 * point — de quoi départager les volumes par le tampon de profondeur plutôt que par un tri.
 *
 * ## Le repère de la vue
 *
 * `x` et `y` sont le plan de l'image, en unités monde, tel qu'`IsoProjection` l'écrit depuis
 * toujours (y vers le bas). `z` s'y ajoute : la distance le long du regard, **croissante en
 * s'éloignant**. Pour un point du sol projeté en `y`, élevé de `h` (mesuré à l'écran, vers le
 * haut) :
 *
 *     y' = y − h          z' = −(y − y₀) · cot θ − h · tan θ
 *
 * où y₀ est l'ordonnée du coin de grille (0, 0). C'est une rotation : rien n'est déformé, et un
 * point du sol reste exactement où `IsoProjection` le met — le pointage d'une case ne change pas.
 *
 * ## Les images dans la scène
 *
 * Une image n'a pas de volume ; elle reçoit une profondeur par sommet, selon ce qu'elle montre :
 *
 * - **à plat** (un sol) : chaque sommet a la profondeur du sol sous lui ;
 * - **dressée** (un relief, une figurine) : un plan **vertical**, tourné vers la caméra, posé sur
 *   la ligne de son pied. Au-dessus du pied, le plan vertical ; au-dessous — le losange de sa
 *   case, les orteils d'une figurine —, le sol, sans quoi un sol en maillage la rognerait.
 *
 * Vertical, et non perpendiculaire au regard : un plan face au regard penche en arrière de
 * tan θ par unité de hauteur, et une figurine de 1,80 m à moins de 0,88 m devant un mur aurait
 * la tête dedans. Sous une caméra orthographique fixe, l'un comme l'autre occupe exactement les
 * pixels de l'image d'aujourd'hui : seule la profondeur diffère.
 *
 * Logique pure, sans GPU ni Qt.
 */

namespace hmi {

/**
 * @brief Une transformation affine du repère d'un **maillage** vers celui de la vue : trois lignes
 *        (x, y, z de la vue), quatre colonnes (X, Y, Z du maillage, puis la translation).
 *
 * Le repère d'un maillage est celui du standard 3D : le mètre, la hauteur vers +Y, l'origine au
 * sol sous le centre de l'emprise ; **+X suit les colonnes** de la grille, **+Z ses lignes**.
 */
using ViewTransform = std::array<float, 12>;

/// @brief L'étendue de profondeur d'une scène : ce que la caméra ramène entre ses deux plans.
struct DepthRange {
    float nearest = -1.0F;
    float farthest = 1.0F;

    [[nodiscard]] bool operator==(const DepthRange&) const = default;
};

/// @brief La vue en volume d'une grille projetée par @ref core::IsoProjection.
class IsoView {
public:
    /// Le plus grand sinus d'élévation admis : au-delà, la caméra regarderait à la verticale et
    /// la hauteur n'aurait plus d'image.
    static constexpr float MAXIMUM_SINE = 0.999F;

    /// Ce dont une image passe **devant** sa profondeur calculée, en largeurs de case : un sol en
    /// image et un sol en maillage sont dans le même plan, et se disputeraient chaque pixel.
    static constexpr float IMAGE_DEPTH_BIAS_TILES = 0.02F;

    /// Marge de l'étendue de profondeur autour de la scène, en largeurs de case : ce qui déborde
    /// de la carte (le canevas de l'éditeur, un relief très haut) y tient encore.
    static constexpr float DEPTH_MARGIN_TILES = 64.0F;

    explicit IsoView(const core::IsoProjection& projection) noexcept
        : _projection(projection),
          _sine(std::clamp(projection.tileHeight() / projection.tileWidth(), 0.001F, MAXIMUM_SINE)),
          _cosine(std::sqrt(1.0F - (_sine * _sine))),
          _groundY(projection.origin().y) {}

    /// @return Le sinus de l'élévation de la caméra : le rapport du losange.
    [[nodiscard]] float sine() const noexcept {
        return _sine;
    }

    [[nodiscard]] float cosine() const noexcept {
        return _cosine;
    }

    /// @return Les unités monde qu'occupe, à l'horizontale de l'image, un mètre du lieu : la
    ///         diagonale d'une case de 1,5 m fait la largeur du losange.
    [[nodiscard]] float unitsPerMetre() const noexcept {
        return _projection.tileWidth() / (core::METERS_PER_TILE * std::numbers::sqrt2_v<float>);
    }

    /// @return La profondeur du point du **sol** projeté à l'ordonnée @p y.
    [[nodiscard]] float groundDepth(float y) const noexcept {
        return -(y - _groundY) * _cosine / _sine;
    }

    /// @return La profondeur d'un point élevé de @p rise (mesuré à l'écran, en unités monde)
    ///         au-dessus du point du sol projeté en @p groundY.
    [[nodiscard]] float raisedDepth(float groundY, float rise) const noexcept {
        return groundDepth(groundY) - (rise * _sine / _cosine);
    }

    /**
     * @brief La profondeur, à l'ordonnée @p y, d'une image **dressée** sur la ligne @p footY :
     *        le plan vertical au-dessus du pied, le sol au-dessous.
     */
    [[nodiscard]] float standingDepth(float footY, float y) const noexcept {
        return y < footY ? raisedDepth(footY, footY - y) : groundDepth(y);
    }

    /// @return Ce dont une image passe devant sa profondeur calculée, en unités monde.
    [[nodiscard]] float imageBias() const noexcept {
        return IMAGE_DEPTH_BIAS_TILES * _projection.tileWidth();
    }

    /// @return La hauteur à l'écran, en unités monde, d'une élévation de @p metres.
    [[nodiscard]] float riseOf(float metres) const noexcept {
        return metres * unitsPerMetre() * _cosine;
    }

    /**
     * @brief Pose un maillage : son origine au point de grille @p gridPoint, élevée de @p rise
     *        (mesuré à l'écran, en unités monde — un étage).
     */
    [[nodiscard]] ViewTransform meshTransform(core::Vector2 gridPoint, float rise) const noexcept {
        const core::Vector2 origin = _projection.gridToWorld(gridPoint);
        // Un mètre le long d'une colonne ou d'une ligne : un tiers de demi-losange par 0,5 m.
        const float along = _projection.tileWidth() / (2.0F * core::METERS_PER_TILE);
        const float up = unitsPerMetre();
        return ViewTransform{along,
                             0.0F,
                             -along,
                             origin.x,
                             along * _sine,
                             -up * _cosine,
                             along * _sine,
                             origin.y - rise,
                             -along * _cosine,
                             -up * _sine,
                             -along * _cosine,
                             raisedDepth(origin.y, rise)};
    }

    /**
     * @brief Tourne un maillage posé autour de sa verticale (`LOT-1005`) : la pose @p transform,
     *        précédée d'une rotation de @p yaw radians qui amène l'axe +Z du maillage vers son
     *        axe +X. Un modèle regarde vers +Z : tourné de `yaw`, il regarde vers
     *        (sin yaw, 0, cos yaw).
     */
    [[nodiscard]] static ViewTransform turned(const ViewTransform& transform, float yaw) noexcept {
        const float cosine = std::cos(yaw);
        const float sine = std::sin(yaw);
        ViewTransform out = transform;
        for (std::size_t row = 0; row < 3; ++row) {
            const float alongX = transform[row * 4];
            const float alongZ = transform[(row * 4) + 2];
            out[row * 4] = (alongX * cosine) - (alongZ * sine);
            out[(row * 4) + 2] = (alongX * sine) + (alongZ * cosine);
        }
        return out;
    }

    /// @return Le point de la vue où @p transform met le point (@p x, @p y, @p z) du maillage.
    [[nodiscard]] static std::array<float, 3> apply(const ViewTransform& transform, float x,
                                                    float y, float z) noexcept {
        return {(transform[0] * x) + (transform[1] * y) + (transform[2] * z) + transform[3],
                (transform[4] * x) + (transform[5] * y) + (transform[6] * z) + transform[7],
                (transform[8] * x) + (transform[9] * y) + (transform[10] * z) + transform[11]};
    }

    /**
     * @brief Le rectangle de l'image qu'occupe la boîte [@p minimum, @p maximum] d'un maillage posé
     *        par @p transform : ce que le culling et le cadrage comparent.
     */
    [[nodiscard]] static core::Rect projectedBounds(const ViewTransform& transform,
                                                    const std::array<float, 3>& minimum,
                                                    const std::array<float, 3>& maximum) noexcept {
        float left = 0.0F;
        float top = 0.0F;
        float right = 0.0F;
        float bottom = 0.0F;
        for (int corner = 0; corner < 8; ++corner) {
            const std::array<float, 3> point =
                apply(transform, (corner & 1) != 0 ? maximum[0] : minimum[0],
                      (corner & 2) != 0 ? maximum[1] : minimum[1],
                      (corner & 4) != 0 ? maximum[2] : minimum[2]);
            if (corner == 0) {
                left = right = point[0];
                top = bottom = point[1];
                continue;
            }
            left = std::min(left, point[0]);
            right = std::max(right, point[0]);
            top = std::min(top, point[1]);
            bottom = std::max(bottom, point[1]);
        }
        return core::Rect{{left, top}, {right - left, bottom - top}};
    }

    /// @return L'étendue de profondeur de toute la scène, marge comprise.
    [[nodiscard]] DepthRange depthRange() const noexcept {
        const float margin = DEPTH_MARGIN_TILES * _projection.tileWidth();
        return DepthRange{.nearest = raisedDepth(_projection.sceneSize().y + margin, margin),
                          .farthest = groundDepth(-margin)};
    }

private:
    core::IsoProjection _projection;
    float _sine;
    float _cosine;
    /// L'ordonnée du coin de grille (0, 0) : le sol y a la profondeur zéro.
    float _groundY;
};

}  // namespace hmi
