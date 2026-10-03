// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/World/DayLight.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

#include "Core/Data/JsonDocument.h"
#include "Core/World/WorldClock.h"

namespace core {

namespace {

using Json = nlohmann::json;

// L'azimut 0 est la gauche de l'ecran, (-X, +Z) : 135 degres dans le plan (X, Z).
constexpr float AZIMUTH_ORIGIN_DEGREES = 135.0F;

[[nodiscard]] float radians(float degrees) noexcept {
    return degrees * std::numbers::pi_v<float> / 180.0F;
}

[[nodiscard]] float mix(float from, float to, float t) noexcept {
    return from + ((to - from) * t);
}

[[nodiscard]] LightColor mix(const LightColor& from, const LightColor& to, float t) noexcept {
    return LightColor{
        .r = mix(from.r, to.r, t), .g = mix(from.g, to.g, t), .b = mix(from.b, to.b, t)};
}

[[nodiscard]] DayLightTableResult failed(std::string message) {
    return DayLightTableResult{.table = {}, .message = std::move(message)};
}

// Le nombre fini du champ `key`, dans [low, high] ; rien s'il manque ou sort de l'intervalle.
[[nodiscard]] std::optional<float> boundedNumber(const Json& object, const char* key, float low,
                                                 float high) {
    const auto found = object.find(key);
    if (found == object.end() || !found->is_number()) {
        return std::nullopt;
    }
    const auto value = found->get<float>();
    return std::isfinite(value) && value >= low && value <= high ? std::optional<float>{value}
                                                                 : std::nullopt;
}

// La couleur `#rrggbb` du champ `key` ; rien s'il manque ou n'en est pas une.
[[nodiscard]] std::optional<LightColor> color(const Json& object, const char* key) {
    const auto found = object.find(key);
    if (found == object.end() || !found->is_string()) {
        return std::nullopt;
    }
    return parseLightColor(found->get<std::string>());
}

[[nodiscard]] DayLightTableResult tableFrom(const JsonDocument& document) {
    if (!document.ok()) {
        return failed(document.message);
    }
    const auto keys = document.root.find("keys");
    if (keys == document.root.end() || !keys->is_array() || keys->empty()) {
        return failed("keys missing");
    }
    std::vector<DayLightKey> read;
    read.reserve(keys->size());
    for (const Json& entry : *keys) {
        if (!entry.is_object()) {
            return failed("a key is not an object");
        }
        const auto time = entry.find("time");
        const std::optional<float> minutes = time != entry.end() && time->is_string()
                                                 ? parseClockTime(time->get<std::string>())
                                                 : std::nullopt;
        if (!minutes) {
            return failed("a key has no time written HH:MM");
        }
        const std::string at = formatClockTime(*minutes);
        const std::optional<LightColor> tint = color(entry, "tint");
        const std::optional<LightColor> ambient = color(entry, "ambient");
        const std::optional<LightColor> sun = color(entry, "sun");
        if (!tint || !ambient || !sun) {
            return failed("key " + at + ": tint, ambient and sun are colours written #rrggbb");
        }
        const std::optional<float> azimuth = boundedNumber(entry, "azimuth", -360.0F, 360.0F);
        const std::optional<float> elevation = boundedNumber(entry, "elevation", 1.0F, 90.0F);
        if (!azimuth || !elevation) {
            return failed("key " + at + ": azimuth in [-360, 360], elevation in [1, 90]");
        }
        const std::optional<float> shadow = boundedNumber(entry, "shadow", 0.0F, 1.0F);
        const std::optional<float> lamps = boundedNumber(entry, "lamps", 0.0F, 1.0F);
        if (!shadow || !lamps) {
            return failed("key " + at + ": shadow and lamps in [0, 1]");
        }
        if (std::ranges::any_of(
                read, [&minutes](const DayLightKey& known) { return known.minutes == *minutes; })) {
            return failed("key " + at + " declared twice");
        }
        read.push_back(DayLightKey{.minutes = *minutes,
                                   .tint = *tint,
                                   .ambient = *ambient,
                                   .sun = *sun,
                                   .azimuth = *azimuth,
                                   .elevation = *elevation,
                                   .shadow = *shadow,
                                   .lamps = *lamps});
    }
    return DayLightTableResult{.table = DayLightTable{std::move(read)}, .message = {}};
}

// Une cle d'usine, ecrite comme dans le fichier : l'heure en heures et minutes, les couleurs en
// `0xRRGGBB`.
[[nodiscard]] DayLightKey key(int hours, int minutes, unsigned tint, unsigned ambient, unsigned sun,
                              float azimuth, float elevation, float shadow, float lamps) {
    const auto unpack = [](unsigned packed) {
        return LightColor{.r = static_cast<float>((packed >> 16U) & 0xFFU) / 255.0F,
                          .g = static_cast<float>((packed >> 8U) & 0xFFU) / 255.0F,
                          .b = static_cast<float>(packed & 0xFFU) / 255.0F};
    };
    return DayLightKey{.minutes = static_cast<float>((hours * 60) + minutes),
                       .tint = unpack(tint),
                       .ambient = unpack(ambient),
                       .sun = unpack(sun),
                       .azimuth = azimuth,
                       .elevation = elevation,
                       .shadow = shadow,
                       .lamps = lamps};
}

}  // namespace

std::array<float, 3> sunDirection(float azimuth, float elevation) noexcept {
    const float around = radians(AZIMUTH_ORIGIN_DEGREES + azimuth);
    const float above = radians(elevation);
    const float flat = std::cos(above);
    return {flat * std::cos(around), std::sin(above), flat * std::sin(around)};
}

DayLightTable::DayLightTable(std::vector<DayLightKey> keys) : _keys(std::move(keys)) {
    std::ranges::sort(_keys, {}, &DayLightKey::minutes);
}

const DayLightTable& DayLightTable::factory() {
    // Les memes cles que `Assets/Common/Lighting/daylight.json` : un test les compare. La lumiere
    // dirigee est le soleil de 05:31 a 20:30, la lune le reste du temps ; elle est noire aux deux
    // bascules, qui ne se voient donc pas.
    static const DayLightTable table{{
        key(0, 0, 0x7684BE, 0x5B699E, 0x1F212C, -15.0F, 50.0F, 0.20F, 1.0F),
        key(4, 30, 0x7684BE, 0x606EA5, 0x1F212C, 20.0F, 35.0F, 0.20F, 1.0F),
        key(5, 30, 0x8E90BA, 0x8C90BA, 0x000000, 30.0F, 25.0F, 0.00F, 1.0F),
        key(5, 31, 0x8E90BA, 0x8C90BA, 0x000000, -165.0F, 4.0F, 0.00F, 1.0F),
        key(6, 30, 0xF4CDB4, 0xC4A99B, 0x735C4B, -160.0F, 12.0F, 0.22F, 0.5F),
        key(8, 0, 0xFFF2E0, 0xAAA8A5, 0x867968, -120.0F, 30.0F, 0.32F, 0.0F),
        key(10, 0, 0xFFFAF0, 0x999DA2, 0x867D70, -60.0F, 42.0F, 0.35F, 0.0F),
        key(12, 0, 0xFFFFFF, 0x9199A5, 0x868077, 0.0F, 48.0F, 0.36F, 0.0F),
        key(16, 0, 0xFFF6E6, 0x9C9D9D, 0x867B6B, 50.0F, 40.0F, 0.34F, 0.0F),
        key(18, 30, 0xFFD9A8, 0xC1A989, 0x866C4E, 80.0F, 16.0F, 0.30F, 0.2F),
        key(19, 45, 0xD8A6A4, 0xBC9498, 0x4F3A35, 95.0F, 6.0F, 0.14F, 0.8F),
        key(20, 30, 0x9A92BC, 0x9892BC, 0x000000, 100.0F, 3.0F, 0.00F, 1.0F),
        key(20, 31, 0x9A92BC, 0x9892BC, 0x000000, -40.0F, 25.0F, 0.00F, 1.0F),
        key(22, 0, 0x7684BE, 0x5D6BA2, 0x1F212C, -30.0F, 42.0F, 0.20F, 1.0F),
    }};
    return table;
}

DayLight DayLightTable::sample(float minutes) const noexcept {
    if (_keys.empty()) {
        return DayLight{};
    }
    WorldClock clock;
    clock.setMinutes(minutes);
    const float now = clock.minutes();
    // La cle qui suit l'instant, et celle qui le precede ; passe minuit, la derniere et la
    // premiere s'encadrent d'un jour sur l'autre.
    const auto after = std::ranges::upper_bound(_keys, now, {}, &DayLightKey::minutes);
    const DayLightKey& next = after == _keys.end() ? _keys.front() : *after;
    const DayLightKey& previous = after == _keys.begin() ? _keys.back() : *(after - 1);
    float span = next.minutes - previous.minutes;
    float elapsed = now - previous.minutes;
    if (span <= 0.0F) {
        span += WorldClock::MINUTES_PER_DAY;
    }
    if (elapsed < 0.0F) {
        elapsed += WorldClock::MINUTES_PER_DAY;
    }
    const float t = _keys.size() == 1 ? 0.0F : std::clamp(elapsed / span, 0.0F, 1.0F);
    return DayLight{.tint = mix(previous.tint, next.tint, t),
                    .ambient = mix(previous.ambient, next.ambient, t),
                    .sun = mix(previous.sun, next.sun, t),
                    .toSun = sunDirection(mix(previous.azimuth, next.azimuth, t),
                                          mix(previous.elevation, next.elevation, t)),
                    .shadow = mix(previous.shadow, next.shadow, t),
                    .lamps = mix(previous.lamps, next.lamps, t)};
}

DayLightTableResult readDayLightTable(std::string_view json) {
    return tableFrom(readJsonObject(json, DAYLIGHT_FORMAT_VERSION, "daylight.json"));
}

DayLightTableResult readDayLightTableFile(const std::filesystem::path& path) {
    return tableFrom(readJsonObjectFromFile(path, DAYLIGHT_FORMAT_VERSION));
}

}  // namespace core
