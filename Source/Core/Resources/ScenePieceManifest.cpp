// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Resources/ScenePieceManifest.h"

#include <algorithm>
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

/// Une paire d'entiers `[a, b]` du manifeste, @p fallback si le champ manque ou est mal formé.
[[nodiscard]] std::pair<int, int> intPair(const nlohmann::json& object, std::string_view field,
                                          std::pair<int, int> fallback) {
    const auto found = object.find(field);
    if (found == object.end() || !found->is_array() || found->size() != 2 ||
        !(*found)[0].is_number_integer() || !(*found)[1].is_number_integer()) {
        return fallback;
    }
    return {(*found)[0].get<int>(), (*found)[1].get<int>()};
}

[[nodiscard]] ScenePiece readPiece(const std::string& key, const nlohmann::json& value) {
    ScenePiece piece;
    piece.key = key;
    piece.name = std::string{scenePieceShortName(key)};
    piece.file = value["file"].get<std::string>();
    if (const auto found = value.find("class"); found != value.end() && found->is_string()) {
        piece.className = found->get<std::string>();
    }
    piece.pieceClass = parseScenePieceClass(piece.className);
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
        // Une entrée sans image est ignorée, pas fatale : les autres pièces restent utilisables.
        if (!value.is_object() || !value.contains("file") || !value["file"].is_string()) {
            continue;
        }
        result.manifest._pieces.push_back(readPiece(key, value));
    }
    return result;
}

}  // namespace core
