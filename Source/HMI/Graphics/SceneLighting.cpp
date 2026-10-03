// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/SceneLighting.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "Core/Rpg/Scale.h"

namespace hmi {

namespace {

using Vec3 = std::array<float, 3>;

// La hauteur, en metres, jusqu'ou la carte d'ombres couvre ce que la camera montre : un
// personnage debout, la tete d'une figurine a cheval. Plus haut, un soleil rasant etirerait la
// carte bien au-dela de l'image pour des recepteurs qui n'existent pas.
constexpr float RECEIVER_HEIGHT_METRES = 3.0F;
// Ce dont la carte d'ombres recule vers le soleil, en metres : un toit hors de l'image, du cote
// du soleil, y jette encore son ombre.
constexpr float CASTER_MARGIN_METRES = 80.0F;
// La marge derriere le recepteur le plus lointain, en metres.
constexpr float FAR_MARGIN_METRES = 4.0F;
// Le tremblement d'une flamme : ce qu'elle perd au plus de son intensite, et ses deux battements.
constexpr float FLICKER_DEPTH = 0.16F;
constexpr float FLICKER_SLOW_HERTZ = 2.3F;
constexpr float FLICKER_FAST_HERTZ = 5.9F;
// En dessous, une source ne vaut pas une lumiere du shader.
constexpr float MINIMUM_LIGHT_POWER = 0.01F;

[[nodiscard]] float dot(const Vec3& a, const Vec3& b) noexcept {
    return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}

[[nodiscard]] Vec3 cross(const Vec3& a, const Vec3& b) noexcept {
    return {(a[1] * b[2]) - (a[2] * b[1]), (a[2] * b[0]) - (a[0] * b[2]),
            (a[0] * b[1]) - (a[1] * b[0])};
}

[[nodiscard]] Vec3 normalized(const Vec3& v, const Vec3& fallback) noexcept {
    const float length = std::sqrt(dot(v, v));
    return length > 1e-6F ? Vec3{v[0] / length, v[1] / length, v[2] / length} : fallback;
}

// Le point `m` x (x, y, z, 1).
[[nodiscard]] Vec3 transformed(const LightMatrix& m, const Vec3& p) noexcept {
    return {(m[0] * p[0]) + (m[4] * p[1]) + (m[8] * p[2]) + m[12],
            (m[1] * p[0]) + (m[5] * p[1]) + (m[9] * p[2]) + m[13],
            (m[2] * p[0]) + (m[6] * p[1]) + (m[10] * p[2]) + m[14]};
}

// La direction `m` x (x, y, z, 0) : sans la translation.
[[nodiscard]] Vec3 rotated(const LightMatrix& m, const Vec3& d) noexcept {
    return {(m[0] * d[0]) + (m[4] * d[1]) + (m[8] * d[2]),
            (m[1] * d[0]) + (m[5] * d[1]) + (m[9] * d[2]),
            (m[2] * d[0]) + (m[6] * d[1]) + (m[10] * d[2])};
}

constexpr LightMatrix IDENTITY{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
                               0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};

// Ce qu'une flamme garde de son intensite a l'instant `seconds` : deux battements dephases par
// sa place, pour que deux braseros voisins ne tremblent pas ensemble.
[[nodiscard]] float flicker(const core::LightSource& source, float seconds) noexcept {
    const float phase = (source.column * 12.9898F) + (source.row * 78.233F);
    const float slow = std::sin((seconds * FLICKER_SLOW_HERTZ * 6.2831853F) + phase);
    const float fast = std::sin((seconds * FLICKER_FAST_HERTZ * 6.2831853F) + (phase * 1.7F));
    return 1.0F - (FLICKER_DEPTH * (0.5F + (0.3F * slow) + (0.2F * fast)));
}

// Une source retenue pour l'image : sa place dans la vue, sa portee, sa couleur finale.
struct ViewLight {
    Vec3 position{};
    float radius = 0.0F;
    Vec3 color{};
    float distance = 0.0F;
};

}  // namespace

LightMatrix toLightMatrix(const ViewTransform& t) noexcept {
    return {t[0], t[4], t[8],  0.0F, t[1], t[5], t[9],  0.0F,
            t[2], t[6], t[10], 0.0F, t[3], t[7], t[11], 1.0F};
}

LightMatrix metresToView(const IsoView& view) noexcept {
    // La pose d'un maillage dont l'origine est le coin de grille (0, 0), au sol : son repere est
    // celui du lieu, en metres.
    return toLightMatrix(view.meshTransform({0.0F, 0.0F}, 0.0F));
}

LightMatrix multiplied(const LightMatrix& left, const LightMatrix& right) noexcept {
    LightMatrix out{};
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            float sum = 0.0F;
            for (std::size_t k = 0; k < 4; ++k) {
                sum += left[(k * 4) + row] * right[(column * 4) + k];
            }
            out[(column * 4) + row] = sum;
        }
    }
    return out;
}

LightMatrix invertedAffine(const LightMatrix& m) noexcept {
    // La partie lineaire (3 x 3) par sa comatrice, puis la translation ramenee.
    const float a = m[0];
    const float b = m[4];
    const float c = m[8];
    const float d = m[1];
    const float e = m[5];
    const float f = m[9];
    const float g = m[2];
    const float h = m[6];
    const float i = m[10];
    const float determinant =
        (a * ((e * i) - (f * h))) - (b * ((d * i) - (f * g))) + (c * ((d * h) - (e * g)));
    if (std::fabs(determinant) < 1e-12F) {
        return IDENTITY;
    }
    const float inverse = 1.0F / determinant;
    LightMatrix out = IDENTITY;
    out[0] = ((e * i) - (f * h)) * inverse;
    out[4] = ((c * h) - (b * i)) * inverse;
    out[8] = ((b * f) - (c * e)) * inverse;
    out[1] = ((f * g) - (d * i)) * inverse;
    out[5] = ((a * i) - (c * g)) * inverse;
    out[9] = ((c * d) - (a * f)) * inverse;
    out[2] = ((d * h) - (e * g)) * inverse;
    out[6] = ((b * g) - (a * h)) * inverse;
    out[10] = ((a * e) - (b * d)) * inverse;
    const Vec3 translation = rotated(out, {m[12], m[13], m[14]});
    out[12] = -translation[0];
    out[13] = -translation[1];
    out[14] = -translation[2];
    return out;
}

SceneLightFrame buildSceneLighting(const IsoView& view, const core::Rect& visible,
                                   const WorldLighting& lighting,
                                   std::span<const core::LightSource> sources,
                                   bool flipShadowRows) {
    SceneLightFrame frame;
    LightingUniforms& uniforms = frame.uniforms;
    const core::DayLight& light = lighting.light;
    const LightMatrix toView = metresToView(view);
    const LightMatrix toMetres = invertedAffine(toView);
    const float unitsPerMetre = view.unitsPerMetre();

    uniforms.tint = {light.tint.r, light.tint.g, light.tint.b, 1.0F};
    uniforms.ambient = {light.ambient.r, light.ambient.g, light.ambient.b,
                        std::clamp(light.shadow, 0.0F, 1.0F)};
    const Vec3 toSun =
        normalized({light.toSun[0], light.toSun[1], light.toSun[2]}, {0.0F, 1.0F, 0.0F});
    const Vec3 sunInView = normalized(rotated(toView, toSun), {0.0F, -1.0F, 0.0F});
    const Vec3 upInView = normalized(rotated(toView, {0.0F, 1.0F, 0.0F}), {0.0F, -1.0F, 0.0F});
    uniforms.toSun = {sunInView[0], sunInView[1], sunInView[2], 0.0F};
    uniforms.up = {upInView[0], upInView[1], upInView[2], 0.0F};

    // --- Les lumieres de nuit : allumees, a portee de l'image, les plus proches du centre.
    const Vec3 centre{visible.position.x + (visible.size.x / 2.0F),
                      visible.position.y + (visible.size.y / 2.0F), 0.0F};
    std::vector<ViewLight> kept;
    for (const core::LightSource& source : sources) {
        const core::LightEmission& emission = source.emission;
        float power =
            emission.intensity * (emission.always ? 1.0F : std::clamp(light.lamps, 0.0F, 1.0F));
        if (emission.flicker) {
            power *= flicker(source, lighting.seconds);
        }
        if (power < MINIMUM_LIGHT_POWER || emission.radius <= 0.0F) {
            continue;
        }
        const Vec3 position =
            transformed(toView, {source.column * core::METERS_PER_TILE, emission.height,
                                 source.row * core::METERS_PER_TILE});
        const float radius = emission.radius * unitsPerMetre;
        // Hors de l'image, portee comprise : elle n'eclaire aucun pixel.
        if (position[0] + radius < visible.position.x ||
            position[0] - radius > visible.position.x + visible.size.x ||
            position[1] + radius < visible.position.y ||
            position[1] - radius > visible.position.y + visible.size.y) {
            continue;
        }
        kept.push_back(ViewLight{
            .position = position,
            .radius = radius,
            .color = {emission.color.r * power, emission.color.g * power, emission.color.b * power},
            .distance = std::hypot(position[0] - centre[0], position[1] - centre[1]) - radius});
    }
    std::ranges::stable_sort(kept, {}, &ViewLight::distance);
    const std::size_t count = std::min(kept.size(), MAXIMUM_SCENE_LIGHTS);
    for (std::size_t index = 0; index < count; ++index) {
        const ViewLight& one = kept[index];
        uniforms.lightPosition[index] = {one.position[0], one.position[1], one.position[2],
                                         one.radius};
        uniforms.lightColor[index] = {one.color[0], one.color[1], one.color[2], 0.0F};
    }
    uniforms.up[3] = static_cast<float>(count);

    // --- La lumiere dirigee et sa carte d'ombres.
    uniforms.sun = {light.sun.r, light.sun.g, light.sun.b, 0.0F};
    const bool lit = light.sun.r > 0.0F || light.sun.g > 0.0F || light.sun.b > 0.0F;
    if (!lighting.shadows || !lit || lighting.shadowSize < 16 || visible.size.x <= 0.0F ||
        visible.size.y <= 0.0F) {
        return frame;
    }
    // Le repere du soleil, en metres : `right` et `above` tendent la carte, `toSun` est sa
    // profondeur. Un soleil au zenith n'a pas de droite a lui : celle des colonnes.
    const Vec3 right = normalized(cross({0.0F, 1.0F, 0.0F}, toSun), {1.0F, 0.0F, 0.0F});
    const Vec3 above = cross(toSun, right);

    // Ce que la camera montre du sol, et la meme chose a hauteur d'homme.
    const float left = visible.position.x;
    const float top = visible.position.y;
    const std::array<core::Vector2, 4> corners{
        core::Vector2{left, top}, core::Vector2{left + visible.size.x, top},
        core::Vector2{left + visible.size.x, top + visible.size.y},
        core::Vector2{left, top + visible.size.y}};
    const Vec3 middle = transformed(toMetres, {centre[0], centre[1], view.groundDepth(centre[1])});
    const float middleX = dot(middle, right);
    const float middleY = dot(middle, above);
    float extent = 0.0F;
    float nearest = dot(middle, toSun);
    float farthest = nearest;
    for (const core::Vector2& corner : corners) {
        const Vec3 ground = transformed(toMetres, {corner.x, corner.y, view.groundDepth(corner.y)});
        for (const float height : {0.0F, RECEIVER_HEIGHT_METRES}) {
            const Vec3 point{ground[0], ground[1] + height, ground[2]};
            extent = std::max({extent, std::fabs(dot(point, right) - middleX),
                               std::fabs(dot(point, above) - middleY)});
            const float along = dot(point, toSun);
            nearest = std::max(nearest, along);    // vers le soleil
            farthest = std::min(farthest, along);  // loin de lui
        }
    }
    if (extent <= 0.0F) {
        return frame;
    }
    // Le centre cale sur un texel : la carte glisse par pas entiers, ses bords ne fremissent pas.
    // La carte s'elargit d'un texel de chaque cote : le calage a pu decaler le centre d'autant. Le
    // texel se mesure sur la carte elargie -- c'est son pas, exactement.
    const auto size = static_cast<float>(lighting.shadowSize);
    const float half = extent * (1.0F + (2.0F / size));
    const float texel = (2.0F * half) / size;
    const float centreX = std::floor(middleX / texel) * texel;
    const float centreY = std::floor(middleY / texel) * texel;
    const float start = nearest + CASTER_MARGIN_METRES;
    const float depth = (start - farthest) + FAR_MARGIN_METRES;

    // Metres -> clip, a la convention d'OpenGL que QRhi impose a la source : x, y et la
    // profondeur dans [-1, 1], de -1 (cote soleil) a 1. `QRhi::clipSpaceCorrMatrix` la ramene a
    // celle de l'interface au moment de dessiner.
    LightMatrix& clip = frame.metresToShadowClip;
    clip = IDENTITY;
    for (std::size_t axis = 0; axis < 3; ++axis) {
        clip[(axis * 4) + 0] = right[axis] / half;
        clip[(axis * 4) + 1] = above[axis] / half;
        clip[(axis * 4) + 2] = -2.0F * toSun[axis] / depth;
    }
    clip[12] = -centreX / half;
    clip[13] = -centreY / half;
    clip[14] = (2.0F * start / depth) - 1.0F;

    // Clip -> texture : (u, v) et la profondeur dans [0, 1] -- ce que la carte a garde --, la
    // premiere ligne en haut du clip ou en bas selon l'interface.
    LightMatrix bias = IDENTITY;
    bias[0] = 0.5F;
    bias[12] = 0.5F;
    bias[5] = flipShadowRows ? -0.5F : 0.5F;
    bias[13] = 0.5F;
    bias[10] = 0.5F;
    bias[14] = 0.5F;
    uniforms.viewToShadow = multiplied(bias, multiplied(clip, toMetres));
    uniforms.sun[3] = 1.0F;
    uniforms.toSun[3] = 1.0F / size;
    frame.shadows = true;
    return frame;
}

}  // namespace hmi
