// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/World/FollowTrail.h"

#include <cstddef>

namespace core {

void FollowTrail::reset(const std::vector<TrailPoint>& points) {
    _points.assign(points.begin(), points.end());
}

void FollowTrail::record(TrailPoint leader) {
    if (!_points.empty() && (leader - _points.front()).length() < MIN_STEP_CELLS) {
        return;
    }
    _points.push_front(leader);
    if (_kept <= 0.0F) {
        return;
    }
    // On ne jette un point que si ce qui reste couvre encore la longueur gardee : le dernier
    // suiveur doit toujours trouver sa place sur la trace.
    float parcouru = 0.0F;
    for (std::size_t rang = 1; rang < _points.size(); ++rang) {
        parcouru += (_points[rang] - _points[rang - 1]).length();
        if (parcouru >= _kept) {
            _points.resize(rang + 1);
            return;
        }
    }
}

void FollowTrail::keep(float length) {
    _kept = length;
}

float FollowTrail::length() const noexcept {
    float total = 0.0F;
    for (std::size_t rang = 1; rang < _points.size(); ++rang) {
        total += (_points[rang] - _points[rang - 1]).length();
    }
    return total;
}

TrailPoint FollowTrail::pointBehind(float distance) const {
    if (_points.empty()) {
        return {};
    }
    float reste = distance;
    for (std::size_t rang = 1; rang < _points.size(); ++rang) {
        const Vector2 segment = _points[rang] - _points[rang - 1];
        const float longueur = segment.length();
        if (reste <= longueur) {
            return longueur > 0.0F ? _points[rang - 1] + (segment * (reste / longueur))
                                   : _points[rang - 1];
        }
        reste -= longueur;
    }
    return _points.back();
}

Vector2 FollowTrail::directionAt(float distance) const {
    if (_points.size() < 2) {
        return {};
    }
    float reste = distance;
    for (std::size_t rang = 1; rang < _points.size(); ++rang) {
        const Vector2 segment = _points[rang - 1] - _points[rang];
        const float longueur = segment.length();
        if (reste <= longueur) {
            return segment;
        }
        reste -= longueur;
    }
    // Au-dela de la trace : le suiveur attend au bout, tourne comme le dernier segment.
    return _points[_points.size() - 2] - _points.back();
}

}  // namespace core
