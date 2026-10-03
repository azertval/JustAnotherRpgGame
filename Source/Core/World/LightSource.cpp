// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/World/LightSource.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <variant>

#include "Core/Rpg/Scale.h"

namespace core {

namespace {

// La valeur d'un chiffre hexadecimal ; -1 pour tout autre caractere.
[[nodiscard]] int hexDigit(char digit) noexcept {
    if (digit >= '0' && digit <= '9') {
        return digit - '0';
    }
    if (digit >= 'a' && digit <= 'f') {
        return digit - 'a' + 10;
    }
    if (digit >= 'A' && digit <= 'F') {
        return digit - 'A' + 10;
    }
    return -1;
}

// L'entier de la propriete `key`, ramene dans [low, high] ; `fallback` si elle manque ou n'en est
// pas un.
[[nodiscard]] int integerOf(const PropertyMap& properties, std::string_view key, int fallback,
                            int low, int high) {
    const auto found = properties.find(std::string{key});
    if (found == properties.end()) {
        return fallback;
    }
    const auto* const value = std::get_if<std::int64_t>(&found->second);
    return value != nullptr ? static_cast<int>(std::clamp<std::int64_t>(*value, low, high))
                            : fallback;
}

[[nodiscard]] bool flagOf(const PropertyMap& properties, std::string_view key) {
    const auto found = properties.find(std::string{key});
    if (found == properties.end()) {
        return false;
    }
    const auto* const value = std::get_if<bool>(&found->second);
    return value != nullptr && *value;
}

}  // namespace

std::optional<LightColor> parseLightColor(std::string_view text) noexcept {
    if (text.size() != 7 || text.front() != '#') {
        return std::nullopt;
    }
    std::array<float, 3> channels{};
    for (std::size_t channel = 0; channel < channels.size(); ++channel) {
        const int high = hexDigit(text[1 + (channel * 2)]);
        const int low = hexDigit(text[2 + (channel * 2)]);
        if (high < 0 || low < 0) {
            return std::nullopt;
        }
        channels[channel] = static_cast<float>((high * 16) + low) / 255.0F;
    }
    return LightColor{.r = channels[0], .g = channels[1], .b = channels[2]};
}

std::optional<LightSource> lightSourceOf(const MapEntity& entity) {
    if (entity.type != LIGHT_ENTITY_TYPE) {
        return std::nullopt;
    }
    LightEmission emission;
    if (const auto color = entity.properties.find(std::string{LIGHT_COLOR_PROPERTY});
        color != entity.properties.end()) {
        if (const auto* const text = std::get_if<std::string>(&color->second)) {
            emission.color = parseLightColor(*text).value_or(LightEmission::DEFAULT_COLOR);
        }
    }
    emission.radius =
        static_cast<float>(integerOf(entity.properties, LIGHT_RADIUS_PROPERTY,
                                     LIGHT_RADIUS_CELLS_DEFAULT, 1, LIGHT_RADIUS_CELLS_MAXIMUM)) *
        METERS_PER_TILE;
    emission.height = static_cast<float>(integerOf(entity.properties, LIGHT_HEIGHT_PROPERTY,
                                                   LIGHT_HEIGHT_DECIMETRES_DEFAULT, 0,
                                                   LIGHT_HEIGHT_DECIMETRES_MAXIMUM)) /
                      10.0F;
    emission.intensity =
        static_cast<float>(
            integerOf(entity.properties, LIGHT_INTENSITY_PROPERTY, LIGHT_INTENSITY_PERCENT_DEFAULT,
                      LIGHT_INTENSITY_PERCENT_MINIMUM, LIGHT_INTENSITY_PERCENT_MAXIMUM)) /
        100.0F;
    emission.flicker = flagOf(entity.properties, LIGHT_FLICKER_PROPERTY);
    emission.always = flagOf(entity.properties, LIGHT_ALWAYS_PROPERTY);
    return LightSource{.column = static_cast<float>(entity.position.column) + 0.5F,
                       .row = static_cast<float>(entity.position.row) + 0.5F,
                       .emission = emission};
}

}  // namespace core
