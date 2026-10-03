// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/World/WorldClock.h"

#include <array>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <variant>

namespace core {

namespace {

// Ramene des minutes dans [0, 1440[ ; une valeur non finie vaut minuit.
[[nodiscard]] float wrapped(float minutes) noexcept {
    if (!std::isfinite(minutes)) {
        return 0.0F;
    }
    const float inDay = std::fmod(minutes, WorldClock::MINUTES_PER_DAY);
    return inDay < 0.0F ? inDay + WorldClock::MINUTES_PER_DAY : inDay;
}

// L'entier ecrit par `text` tout entier, dans [0, limit[ ; rien sinon.
[[nodiscard]] std::optional<int> boundedInteger(std::string_view text, int limit) noexcept {
    if (text.empty() || text.size() > 2) {
        return std::nullopt;
    }
    int value = 0;
    const char* const end = text.data() + text.size();
    const std::from_chars_result parsed = std::from_chars(text.data(), end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end || value < 0 || value >= limit) {
        return std::nullopt;
    }
    return value;
}

}  // namespace

void WorldClock::setMinutes(float minutes) noexcept {
    _minutes = wrapped(minutes);
}

void WorldClock::advance(float seconds) noexcept {
    if (!_running || !std::isfinite(seconds) || seconds <= 0.0F) {
        return;
    }
    const float total = _minutes + (seconds * GAME_MINUTES_PER_SECOND);
    _day += static_cast<int>(std::floor(total / MINUTES_PER_DAY));
    _minutes = wrapped(total);
}

std::optional<float> parseClockTime(std::string_view text) noexcept {
    const std::size_t colon = text.find(':');
    const std::optional<int> hours = boundedInteger(text.substr(0, colon), 24);
    if (!hours) {
        return std::nullopt;
    }
    if (colon == std::string_view::npos) {
        return static_cast<float>(*hours * 60);
    }
    const std::string_view minutesText = text.substr(colon + 1);
    const std::optional<int> minutes =
        minutesText.size() == 2 ? boundedInteger(minutesText, 60) : std::nullopt;
    if (!minutes) {
        return std::nullopt;
    }
    return static_cast<float>((*hours * 60) + *minutes);
}

std::optional<float> mapFixedMinutes(const PropertyMap& properties) {
    const auto found = properties.find(std::string{MAP_HOUR_PROPERTY});
    if (found == properties.end()) {
        return std::nullopt;
    }
    const auto* const text = std::get_if<std::string>(&found->second);
    return text != nullptr ? parseClockTime(*text) : std::nullopt;
}

std::string formatClockTime(float minutes) {
    const int whole = static_cast<int>(wrapped(minutes));
    std::array<char, 8> text{};
    std::snprintf(text.data(), text.size(), "%02d:%02d", whole / 60, whole % 60);
    return std::string{text.data()};
}

}  // namespace core
