// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Resources/SkeletonPose.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace core {

namespace {

constexpr std::size_t MATRIX_FLOATS = 16;

// a x b, en colonnes : m[colonne * 4 + ligne]. La derniere ligne d'une pose est (0, 0, 0, 1).
[[nodiscard]] MeshMatrix multiply(const float* a, const float* b) noexcept {
    MeshMatrix out{};
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            float sum = 0.0F;
            for (std::size_t k = 0; k < 4; ++k) {
                sum += a[(k * 4) + row] * b[(column * 4) + k];
            }
            out[(column * 4) + row] = sum;
        }
    }
    return out;
}

// translation x rotation x echelle.
[[nodiscard]] MeshMatrix compose(const std::array<float, 3>& t, const std::array<float, 4>& q,
                                 const std::array<float, 3>& s) noexcept {
    const float x = q[0];
    const float y = q[1];
    const float z = q[2];
    const float w = q[3];
    return MeshMatrix{(1 - (2 * ((y * y) + (z * z)))) * s[0],
                      (2 * ((x * y) + (z * w))) * s[0],
                      (2 * ((x * z) - (y * w))) * s[0],
                      0,
                      (2 * ((x * y) - (z * w))) * s[1],
                      (1 - (2 * ((x * x) + (z * z)))) * s[1],
                      (2 * ((y * z) + (x * w))) * s[1],
                      0,
                      (2 * ((x * z) + (y * w))) * s[2],
                      (2 * ((y * z) - (x * w))) * s[2],
                      (1 - (2 * ((x * x) + (y * y)))) * s[2],
                      0,
                      t[0],
                      t[1],
                      t[2],
                      1};
}

// Les deux cles qui encadrent `seconds`, et la part de la seconde ; la cle du bord hors du clip.
struct KeySpan {
    std::size_t before = 0;
    std::size_t after = 0;
    float fraction = 0.0F;
};

[[nodiscard]] KeySpan keysAround(const MeshChannel& channel, float seconds) noexcept {
    const std::vector<float>& times = channel.times;
    const auto next = std::ranges::upper_bound(times, seconds);
    if (next == times.begin()) {
        return KeySpan{};
    }
    const auto after = static_cast<std::size_t>(next - times.begin());
    if (next == times.end()) {
        return KeySpan{.before = after - 1, .after = after - 1, .fraction = 0.0F};
    }
    const float span = times[after] - times[after - 1];
    const float fraction =
        channel.step || span <= 0.0F ? 0.0F : (seconds - times[after - 1]) / span;
    return KeySpan{.before = after - 1, .after = after, .fraction = fraction};
}

[[nodiscard]] std::array<float, 3> sampleTranslation(const MeshChannel& channel, float seconds,
                                                     const std::array<float, 3>& rest) noexcept {
    if (channel.empty() || channel.values.size() < channel.times.size() * 3) {
        return rest;
    }
    const KeySpan keys = keysAround(channel, seconds);
    std::array<float, 3> out{};
    for (std::size_t part = 0; part < 3; ++part) {
        const float a = channel.values[(keys.before * 3) + part];
        const float b = channel.values[(keys.after * 3) + part];
        out[part] = a + ((b - a) * keys.fraction);
    }
    return out;
}

[[nodiscard]] std::array<float, 4> normalized(std::array<float, 4> q) noexcept {
    const float length = std::sqrt((q[0] * q[0]) + (q[1] * q[1]) + (q[2] * q[2]) + (q[3] * q[3]));
    if (!(length > 0.0F) || !std::isfinite(length)) {
        return {0.0F, 0.0F, 0.0F, 1.0F};
    }
    for (float& part : q) {
        part /= length;
    }
    return q;
}

// La rotation entre deux cles : interpolation lineaire normalisee, par le plus court chemin. Les
// cles d'un clip echantillonne a pas fixe sont proches : l'ecart a l'interpolation spherique ne
// se voit pas.
[[nodiscard]] std::array<float, 4> sampleRotation(const MeshChannel& channel, float seconds,
                                                  const std::array<float, 4>& rest) noexcept {
    if (channel.empty() || channel.values.size() < channel.times.size() * 4) {
        return normalized(rest);
    }
    const KeySpan keys = keysAround(channel, seconds);
    const float* const a = &channel.values[keys.before * 4];
    const float* const b = &channel.values[keys.after * 4];
    const float dot = (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]) + (a[3] * b[3]);
    const float sign = dot < 0.0F ? -1.0F : 1.0F;
    std::array<float, 4> out{};
    for (std::size_t part = 0; part < 4; ++part) {
        out[part] = a[part] + (((b[part] * sign) - a[part]) * keys.fraction);
    }
    return normalized(out);
}

}  // namespace

const MeshClip* findClip(const MeshRig& rig, std::string_view name) noexcept {
    const auto found =
        std::ranges::find_if(rig.clips, [name](const MeshClip& clip) { return clip.name == name; });
    return found != rig.clips.end() ? &*found : nullptr;
}

float clipTime(float seconds, float duration, bool loop) noexcept {
    if (!(duration > 0.0F) || !std::isfinite(seconds)) {
        return 0.0F;
    }
    if (!loop) {
        return std::clamp(seconds, 0.0F, duration);
    }
    const float wrapped = std::fmod(seconds, duration);
    return wrapped < 0.0F ? wrapped + duration : wrapped;
}

void poseSkeleton(const MeshRig& rig, const MeshClip* clip, float seconds, std::span<float> out) {
    const std::size_t count = rig.joints.size();
    if (out.size() < count * MATRIX_FLOATS || rig.order.size() != count) {
        return;
    }
    const bool animated = clip != nullptr && clip->tracks.size() == count;
    // Premier temps : la transformation de chaque os dans le repere du maillage, parents d'abord.
    for (const std::uint16_t rank : rig.order) {
        const MeshJoint& joint = rig.joints[rank];
        std::array<float, 3> translation = joint.translation;
        std::array<float, 4> rotation = joint.rotation;
        if (animated) {
            const MeshJointTrack& track = clip->tracks[rank];
            translation = sampleTranslation(track.translation, seconds, joint.translation);
            rotation = sampleRotation(track.rotation, seconds, joint.rotation);
        }
        const MeshMatrix local = compose(translation, rotation, joint.scale);
        const float* const parent =
            joint.parent >= 0 && std::cmp_less(joint.parent, count)
                ? &out[static_cast<std::size_t>(joint.parent) * MATRIX_FLOATS]
                : joint.anchor.data();
        const MeshMatrix global = multiply(parent, local.data());
        std::ranges::copy(global, out.begin() + static_cast<std::ptrdiff_t>(rank * MATRIX_FLOATS));
    }
    // Second temps : chaque os compose avec sa matrice de liaison inverse.
    for (std::size_t rank = 0; rank < count; ++rank) {
        float* const matrix = &out[rank * MATRIX_FLOATS];
        const MeshMatrix skin = multiply(matrix, rig.joints[rank].inverseBind.data());
        std::ranges::copy(skin, matrix);
    }
}

std::array<float, 3> skinnedPosition(std::span<const float> matrices, const MeshSkinVertex& skin,
                                     const std::array<float, 3>& position) noexcept {
    std::array<float, 3> out{};
    for (std::size_t influence = 0; influence < 4; ++influence) {
        const float weight = skin.weights[influence];
        const std::size_t base = static_cast<std::size_t>(skin.joints[influence]) * MATRIX_FLOATS;
        if (weight <= 0.0F || base + MATRIX_FLOATS > matrices.size()) {
            continue;
        }
        const float* const m = &matrices[base];
        for (std::size_t row = 0; row < 3; ++row) {
            out[row] += weight * ((m[row] * position[0]) + (m[4 + row] * position[1]) +
                                  (m[8 + row] * position[2]) + m[12 + row]);
        }
    }
    return out;
}

}  // namespace core
