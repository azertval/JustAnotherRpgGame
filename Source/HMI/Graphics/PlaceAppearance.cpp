// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/PlaceAppearance.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <set>
#include <utility>

#include "Core/Data/JsonDocument.h"
#include "Core/Levels/TileTypeName.h"
#include "Core/Resources/ScenePieceManifest.h"

namespace hmi {

namespace {

constexpr std::string_view FIELD_PLACE = "place";
constexpr std::string_view FIELD_FLOORS = "floors";
constexpr std::string_view FIELD_RELIEF = "relief";

[[nodiscard]] PlaceAppearanceError mapError(core::JsonReadError error) {
    switch (error) {
        case core::JsonReadError::None:
            return PlaceAppearanceError::None;
        case core::JsonReadError::FileNotFound:
            return PlaceAppearanceError::FileNotFound;
        case core::JsonReadError::ParseError:
            return PlaceAppearanceError::ParseError;
        case core::JsonReadError::UnsupportedVersion:
            return PlaceAppearanceError::UnsupportedVersion;
        case core::JsonReadError::MalformedStructure:
            return PlaceAppearanceError::MalformedStructure;
    }
    return PlaceAppearanceError::MalformedStructure;
}

[[nodiscard]] PlaceAppearanceResult failure(std::string message, PlaceAppearanceError error) {
    return PlaceAppearanceResult{.appearance = {}, .error = error, .message = std::move(message)};
}

// Lit un objet « type de tuile -> liste de pieces ». Un type inconnu est une donnee fautive, pas
// un type a deviner : le signaler vaut mieux que dessiner du sable sous un mur.
[[nodiscard]] bool readTable(const nlohmann::json& root, std::string_view field,
                             std::map<core::TileType, std::vector<std::string>>& table,
                             std::string& error) {
    const auto found = root.find(field);
    if (found == root.end()) {
        return true;  // Une table sans sol, ou sans relief, est legitime.
    }
    if (!found->is_object()) {
        error = std::string{field} + " doit etre un objet";
        return false;
    }
    for (const auto& [name, pieces] : found->items()) {
        const std::optional<core::TileType> type = core::parseTileType(name);
        if (!type.has_value()) {
            error = std::string{field} + " : type de tuile inconnu « " + name + " »";
            return false;
        }
        if (!pieces.is_array() || pieces.empty()) {
            error = std::string{field} + " / " + name + " doit etre une liste non vide";
            return false;
        }
        std::vector<std::string> noms;
        for (const nlohmann::json& piece : pieces) {
            if (!piece.is_string() || piece.get<std::string>().empty()) {
                error = std::string{field} + " / " + name + " : piece vide";
                return false;
            }
            noms.push_back(piece.get<std::string>());
        }
        table.emplace(*type, std::move(noms));
    }
    return true;
}

// La variante d'une case : toujours la meme pour la meme case, et sans rapport avec l'ordre de
// parcours. Les deux facteurs sont premiers entre eux et avec les petits nombres de variantes, si
// bien que les voisines ne tombent pas toutes sur la meme.
[[nodiscard]] std::size_t variantOf(core::GridPosition cell, std::size_t count) {
    const long long melange =
        (static_cast<long long>(cell.column) * 7) + (static_cast<long long>(cell.row) * 13);
    const long long positif = melange < 0 ? -melange : melange;
    return count == 0 ? 0 : static_cast<std::size_t>(positif) % count;
}

[[nodiscard]] std::string_view pieceOf(
    const std::map<core::TileType, std::vector<std::string>>& table, core::TileType type,
    core::GridPosition cell) {
    const auto found = table.find(type);
    if (found == table.end() || found->second.empty()) {
        return {};
    }
    return found->second[variantOf(cell, found->second.size())];
}

}  // namespace

PlaceAppearanceResult PlaceAppearance::loadFromString(std::string_view json) {
    return fromDocument(core::readJsonObject(json, FORMAT_VERSION, "appearance.json"));
}

PlaceAppearanceResult PlaceAppearance::loadFromFile(const std::filesystem::path& path) {
    PlaceAppearanceResult result = fromDocument(core::readJsonObjectFromFile(path, FORMAT_VERSION));
    if (result.ok()) {
        // Le manifeste voisin est facultatif : un lieu sans planche livree n'en a pas, et sa table
        // reste lisible.
        const core::ScenePieceManifestResult manifest =
            core::ScenePieceManifest::loadFromFile(path.parent_path() / "manifest.json");
        if (manifest.ok()) {
            result.appearance.adoptManifest(manifest.manifest);
        }
    }
    return result;
}

void PlaceAppearance::adoptManifest(const core::ScenePieceManifest& manifest) {
    for (const core::ScenePiece& piece : manifest.pieces()) {
        for (const std::string& alias : piece.aliases) {
            _aliases.emplace(alias, piece.name);
        }
        if (piece.footprintColumns > 1 || piece.footprintRows > 1) {
            _footprints.insert_or_assign(piece.name, piece.footprint());
        }
        // Une pièce rangée ailleurs qu'à plat : le rendu doit la chercher sous son vrai chemin.
        if (!piece.file.empty() && piece.file != piece.name + ".png") {
            _files.insert_or_assign(piece.name, piece.file);
        }
        // Ce qui monte au-dessus du sommet haut de l'emprise : l'ancre, a defaut le haut de
        // l'image au-dessus du losange de sa case. En largeurs de case, au losange du lieu -- a
        // defaut la largeur de la piece sur son emprise, comme le rendu le suppose.
        if (piece.height > 0) {
            const float tileWidth =
                manifest.tileWidth() > 0
                    ? static_cast<float>(manifest.tileWidth())
                    : static_cast<float>(std::max(1, piece.width)) /
                          static_cast<float>(std::max(1, piece.footprintColumns));
            const float tileHeight = manifest.tileHeight() > 0
                                         ? static_cast<float>(manifest.tileHeight())
                                         : tileWidth * _diamondRatio;
            const float above = piece.anchorY >= 0 ? static_cast<float>(piece.anchorY)
                                                   : static_cast<float>(piece.height) - tileHeight;
            _maximumRise = std::max(_maximumRise, above / tileWidth);
        }
    }
    // Un nom courant n'est jamais un alias : il designe la piece qui le porte aujourd'hui.
    for (const core::ScenePiece& piece : manifest.pieces()) {
        _aliases.erase(piece.name);
    }
}

std::string_view PlaceAppearance::canonicalPiece(std::string_view name) const {
    const auto found = _aliases.find(name);
    return found == _aliases.end() ? name : std::string_view{found->second};
}

core::PieceFootprint PlaceAppearance::pieceFootprint(std::string_view name) const {
    const auto found = _footprints.find(name);
    return found == _footprints.end() ? core::PieceFootprint{} : found->second;
}

std::string_view PlaceAppearance::pieceFile(std::string_view name) const {
    const auto found = _files.find(name);
    return found == _files.end() ? std::string_view{} : std::string_view{found->second};
}

PlaceAppearanceResult PlaceAppearance::fromDocument(const core::JsonDocument& document) {
    if (!document.ok()) {
        return failure(document.message, mapError(document.error));
    }
    PlaceAppearance appearance;
    const auto place = document.root.find(FIELD_PLACE);
    if (place == document.root.end() || !place->is_string()) {
        return failure("place manquant ou non textuel", PlaceAppearanceError::MalformedStructure);
    }
    appearance._place = place->get<std::string>();
    if (document.root.contains("diamondRatio")) {
        const auto& ratio = document.root["diamondRatio"];
        if (!ratio.is_number()) {
            return failure("diamondRatio doit etre numerique",
                           PlaceAppearanceError::MalformedStructure);
        }
        const float value = ratio.get<float>();
        if (!std::isfinite(value) || value <= 0.0F || value > 2.0F) {
            return failure("diamondRatio hors limites", PlaceAppearanceError::MalformedStructure);
        }
        appearance._diamondRatio = value;
    }

    std::string error;
    if (!readTable(document.root, FIELD_FLOORS, appearance._floors, error) ||
        !readTable(document.root, FIELD_RELIEF, appearance._relief, error)) {
        return failure(std::move(error), PlaceAppearanceError::MalformedStructure);
    }
    return {
        .appearance = std::move(appearance), .error = PlaceAppearanceError::None, .message = {}};
}

std::string_view PlaceAppearance::floorPiece(core::TileType type, core::GridPosition cell) const {
    return pieceOf(_floors, type, cell);
}

std::string_view PlaceAppearance::reliefPiece(core::TileType type, core::GridPosition cell) const {
    return pieceOf(_relief, type, cell);
}

std::optional<core::TileType> PlaceAppearance::typeOfPiece(std::string_view piece,
                                                           bool floor) const {
    const std::string_view name = canonicalPiece(piece);
    for (const auto& [type, noms] : floor ? _floors : _relief) {
        if (std::ranges::find(noms, name) != noms.end()) {
            return type;
        }
    }
    return std::nullopt;
}

std::vector<std::string> PlaceAppearance::pieces() const {
    std::set<std::string> uniques;
    for (const auto* table : {&_floors, &_relief}) {
        for (const auto& [type, noms] : *table) {
            uniques.insert(noms.begin(), noms.end());
        }
    }
    return {uniques.begin(), uniques.end()};
}

}  // namespace hmi
