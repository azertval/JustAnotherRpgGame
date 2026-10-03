// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Resources/ScenePieceManifest.h"

#include <algorithm>
#include <cmath>
#include <system_error>
#include <tuple>
#include <utility>

#include "Core/Data/JsonDocument.h"

namespace core {

namespace {

[[nodiscard]] ScenePieceManifestError mapError(JsonReadError error) {
    switch (error) {
        case JsonReadError::None:
            return ScenePieceManifestError::None;
        case JsonReadError::FileNotFound:
            return ScenePieceManifestError::FileNotFound;
        case JsonReadError::ParseError:
            return ScenePieceManifestError::ParseError;
        case JsonReadError::UnsupportedVersion:
            return ScenePieceManifestError::UnsupportedVersion;
        case JsonReadError::MalformedStructure:
            return ScenePieceManifestError::MalformedStructure;
    }
    return ScenePieceManifestError::MalformedStructure;
}

// Une paire d'entiers `[a, b]` du manifeste, @p fallback si le champ manque ou est mal formé.
[[nodiscard]] std::pair<int, int> intPair(const nlohmann::json& object, std::string_view field,
                                          std::pair<int, int> fallback) {
    const auto found = object.find(field);
    if (found == object.end() || !found->is_array() || found->size() != 2 ||
        !(*found)[0].is_number_integer() || !(*found)[1].is_number_integer()) {
        return fallback;
    }
    return {(*found)[0].get<int>(), (*found)[1].get<int>()};
}

// Le nombre fini du champ `key`, ramene dans [low, high] ; `fallback` s'il manque.
[[nodiscard]] float boundedNumber(const nlohmann::json& object, const char* key, float fallback,
                                  float low, float high) {
    const auto found = object.find(key);
    if (found == object.end() || !found->is_number()) {
        return fallback;
    }
    const auto value = found->get<float>();
    return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
}

// La lumiere d'une piece (LOT-1007), en metres. Un champ absent ou faux vaut ce que vaut une
// lanterne : une piece mal ecrite eclaire quand meme, et `check_hd_assets` dit ce qui est faux.
[[nodiscard]] LightEmission readEmission(const nlohmann::json& value) {
    LightEmission emission;
    if (const auto color = value.find("color"); color != value.end() && color->is_string()) {
        emission.color =
            parseLightColor(color->get<std::string>()).value_or(LightEmission::DEFAULT_COLOR);
    }
    emission.radius = boundedNumber(value, "radius", LightEmission::DEFAULT_RADIUS_METRES, 0.5F,
                                    LightEmission::MAXIMUM_RADIUS_METRES);
    emission.height = boundedNumber(value, "height", LightEmission::DEFAULT_HEIGHT_METRES, 0.0F,
                                    LightEmission::MAXIMUM_HEIGHT_METRES);
    emission.intensity =
        boundedNumber(value, "intensity", 1.0F, 0.1F, LightEmission::MAXIMUM_INTENSITY);
    if (const auto flicker = value.find("flicker");
        flicker != value.end() && flicker->is_boolean()) {
        emission.flicker = flicker->get<bool>();
    }
    if (const auto always = value.find("always"); always != value.end() && always->is_boolean()) {
        emission.always = always->get<bool>();
    }
    return emission;
}

[[nodiscard]] ScenePiece readPiece(const std::string& key, const nlohmann::json& value) {
    ScenePiece piece;
    piece.key = key;
    piece.name = std::string{scenePieceShortName(key)};
    // Une clé de pièce cite une image (`file`) ou un maillage (`mesh`, LOT-1003).
    if (const auto file = value.find("file"); file != value.end() && file->is_string()) {
        piece.file = file->get<std::string>();
    }
    if (const auto mesh = value.find("mesh"); mesh != value.end() && mesh->is_string()) {
        piece.mesh = mesh->get<std::string>();
    }
    if (const auto found = value.find("class"); found != value.end() && found->is_string()) {
        piece.className = found->get<std::string>();
    }
    piece.pieceClass = parseScenePieceClass(piece.className);
    if (const auto family = value.find("family"); family != value.end() && family->is_string()) {
        piece.family = family->get<std::string>();
    }
    const auto [columns, rows] = intPair(value, "footprint", {1, 1});
    piece.footprintColumns = std::max(1, columns);
    piece.footprintRows = std::max(1, rows);
    std::tie(piece.width, piece.height) = intPair(value, "size", {0, 0});
    std::tie(piece.anchorX, piece.anchorY) = intPair(value, "anchor", {-1, -1});
    if (const auto mirror = value.find("mirrorOf"); mirror != value.end() && mirror->is_string()) {
        piece.mirrorOf = std::string{scenePieceShortName(mirror->get<std::string>())};
    }
    // Un sol passe, une pièce debout arrête la vue ; un nom tactique inconnu garde ce défaut
    // plutôt que de faire perdre la pièce.
    piece.tactical =
        piece.pieceClass == ScenePieceClass::Floor ? PieceTactical::Open : PieceTactical::Solid;
    if (const auto tactical = value.find("tactical");
        tactical != value.end() && tactical->is_string()) {
        piece.tactical = parsePieceTactical(tactical->get<std::string>()).value_or(piece.tactical);
    }
    if (const auto light = value.find("light"); light != value.end() && light->is_object()) {
        piece.light = readEmission(*light);
    }
    if (const auto glow = value.find("glow"); glow != value.end() && glow->is_number()) {
        piece.glow = std::clamp(glow->get<float>(), 0.0F, 1.0F);
    }
    if (const auto aliases = value.find("aliases"); aliases != value.end() && aliases->is_array()) {
        for (const nlohmann::json& alias : *aliases) {
            if (alias.is_string()) {
                piece.aliases.emplace_back(scenePieceShortName(alias.get<std::string>()));
            }
        }
    }
    return piece;
}

}  // namespace

const char* pieceTacticalName(PieceTactical tactical) noexcept {
    switch (tactical) {
        case PieceTactical::Open:
            return "open";
        case PieceTactical::Difficult:
            return "difficult";
        case PieceTactical::Cover:
            return "cover";
        case PieceTactical::Obstacle:
            return "obstacle";
        case PieceTactical::Solid:
            return "solid";
    }
    return "solid";
}

std::optional<PieceTactical> parsePieceTactical(std::string_view name) noexcept {
    for (const PieceTactical tactical :
         {PieceTactical::Open, PieceTactical::Difficult, PieceTactical::Cover,
          PieceTactical::Obstacle, PieceTactical::Solid}) {
        if (name == pieceTacticalName(tactical)) {
            return tactical;
        }
    }
    return std::nullopt;
}

ScenePieceClass parseScenePieceClass(std::string_view name) noexcept {
    if (name == "floor") {
        return ScenePieceClass::Floor;
    }
    if (name == "tall") {
        return ScenePieceClass::Tall;
    }
    if (name == "wide") {
        return ScenePieceClass::Wide;
    }
    return ScenePieceClass::Other;
}

std::string_view scenePieceShortName(std::string_view key) noexcept {
    const std::size_t slash = key.rfind('/');
    return slash == std::string_view::npos ? key : key.substr(slash + 1);
}

ScenePieceManifestResult ScenePieceManifest::loadFromString(std::string_view json) {
    return fromDocument(readJsonObject(json, FORMAT_VERSION, "manifest.json"));
}

ScenePieceManifestResult ScenePieceManifest::loadFromFile(const std::filesystem::path& path) {
    return fromDocument(readJsonObjectFromFile(path, FORMAT_VERSION));
}

ScenePieceManifestResult ScenePieceManifest::resolve(const std::filesystem::path& assetsDirectory,
                                                     std::string_view place) {
    ScenePieceManifestResult result;
    result.manifest._place = std::string{place};
    for (const SceneLevel& level : sceneLevelCandidates(place)) {
        const std::filesystem::path file =
            assetsDirectory / std::filesystem::path(level.directory) / "manifest.json";
        std::error_code error;
        if (!std::filesystem::is_regular_file(file, error)) {
            continue;
        }
        ScenePieceManifestResult read = loadFromFile(file);
        if (!read.ok()) {
            return ScenePieceManifestResult{
                .manifest = {},
                .error = read.error,
                .message = level.directory + "/manifest.json: " + read.message};
        }
        ScenePieceManifest& merged = result.manifest;
        merged._levels.push_back(level);
        if (merged._tileWidth == 0 && read.manifest._tileWidth > 0) {
            merged._tileWidth = read.manifest._tileWidth;
            merged._tileHeight = read.manifest._tileHeight;
        }
        for (ScenePiece& piece : read.manifest._pieces) {
            piece.directory = level.directory;
            piece.level = level.label;
            // Le plus propre gagne : une pièce commune de même nom est masquée, pas perdue.
            if (const auto owner = std::ranges::find(merged._pieces, piece.name, &ScenePiece::name);
                owner != merged._pieces.end()) {
                merged._masked.push_back(
                    MaskedScenePiece{.piece = std::move(piece), .by = owner->level});
                continue;
            }
            merged._pieces.push_back(std::move(piece));
        }
    }
    if (result.manifest._levels.empty()) {
        result.error = ScenePieceManifestError::FileNotFound;
        result.message = "no piece manifest for place \"" + std::string{place} + "\"";
    }
    return result;
}

const ScenePiece* ScenePieceManifest::find(std::string_view name) const noexcept {
    if (const auto found = std::ranges::find(_pieces, name, &ScenePiece::name);
        found != _pieces.end()) {
        return &*found;
    }
    // Un nom courant l'emporte toujours sur un ancien nom : une pièce renommée puis remplacée par
    // une nouvelle pièce du même nom ne détourne pas les cartes qui citent la nouvelle.
    const auto aliased = std::ranges::find_if(_pieces, [name](const ScenePiece& piece) {
        return std::ranges::find(piece.aliases, name) != piece.aliases.end();
    });
    return aliased == _pieces.end() ? nullptr : &*aliased;
}

ScenePieceManifestResult ScenePieceManifest::fromDocument(const JsonDocument& document) {
    ScenePieceManifestResult result;
    if (!document.ok()) {
        result.error = mapError(document.error);
        result.message = document.message;
        return result;
    }
    const auto textures = document.root.find("textures");
    if (textures == document.root.end() || !textures->is_object()) {
        result.error = ScenePieceManifestError::MalformedStructure;
        result.message = "textures manquant ou non objet";
        return result;
    }
    if (const auto disposition = document.root.find("disposition");
        disposition != document.root.end() && disposition->is_string()) {
        result.manifest._place = disposition->get<std::string>();
    }
    // L'échelle de l'art est une donnée du lieu (LOT-103) : un losange non positif ne dit rien.
    if (const auto [width, height] = intPair(document.root, "tile", {0, 0});
        width > 0 && height > 0) {
        result.manifest._tileWidth = width;
        result.manifest._tileHeight = height;
    }
    for (const auto& [key, value] : textures->items()) {
        // Une entrée sans image ni maillage est ignorée, pas fatale : les autres pièces restent
        // utilisables.
        const auto cites = [&value](const char* field) {
            const auto found = value.find(field);
            return found != value.end() && found->is_string() && !found->get<std::string>().empty();
        };
        if (!value.is_object() || (!cites("file") && !cites("mesh"))) {
            continue;
        }
        result.manifest._pieces.push_back(readPiece(key, value));
    }
    return result;
}

}  // namespace core
