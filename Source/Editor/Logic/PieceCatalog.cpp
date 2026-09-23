// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/PieceCatalog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <iterator>
#include <map>
#include <utility>

#include "HMI/Graphics/PlaceAppearance.h"

namespace hmi {

namespace {

/// Les classes connues, dans l'ordre de la palette, et le titre de leur groupe.
constexpr std::array<std::pair<core::ScenePieceClass, std::string_view>, 4> CLASS_GROUPS{{
    {core::ScenePieceClass::Floor, "Floors"},
    {core::ScenePieceClass::Tall, "Standing"},
    {core::ScenePieceClass::Wide, "Wide"},
    {core::ScenePieceClass::Other, "Other"},
}};

[[nodiscard]] std::string lowered(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    std::ranges::transform(text, std::back_inserter(result), [](char letter) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(letter)));
    });
    return result;
}

[[nodiscard]] const char* classLabel(core::ScenePieceClass pieceClass) {
    switch (pieceClass) {
        case core::ScenePieceClass::Floor:
            return "floor";
        case core::ScenePieceClass::Tall:
            return "standing";
        case core::ScenePieceClass::Wide:
            return "wide";
        case core::ScenePieceClass::Other:
            return "other";
    }
    return "other";
}

// Les groupes des pièces du manifeste, une par classe ; un groupe vide n'est pas rendu.
std::vector<PieceCatalogGroup> manifestGroups(const core::ScenePieceManifest& manifest) {
    std::vector<PieceCatalogGroup> groups;
    for (const auto& [pieceClass, label] : CLASS_GROUPS) {
        PieceCatalogGroup group{.label = std::string{label}, .pieces = {}};
        for (const core::ScenePiece& piece : manifest.pieces()) {
            if (piece.pieceClass != pieceClass) {
                continue;
            }
            group.pieces.push_back(
                PieceCatalogEntry{.name = piece.name,
                                  .file = piece.file,
                                  .pieceClass = piece.pieceClass,
                                  .footprint = piece.footprint(),
                                  .tactical = piece.tactical,
                                  .missing = false,
                                  .floor = piece.pieceClass == core::ScenePieceClass::Floor});
        }
        if (!group.pieces.empty()) {
            groups.push_back(std::move(group));
        }
    }
    return groups;
}

}  // namespace

std::vector<PieceCatalogGroup> pieceCatalog(const core::ScenePieceManifest* manifest,
                                            const std::vector<core::TileLayer>& layers) {
    std::vector<PieceCatalogGroup> catalog;
    if (manifest != nullptr) {
        catalog = manifestGroups(*manifest);
    }

    // Les noms cités que le manifeste ignore ; un nom vu d'abord sur une couche de sol s'y repose.
    std::map<std::string, bool, std::less<>> missing;
    for (const core::TileLayer& layer : layers) {
        if (!core::isVisualLayerKind(layer.kind)) {
            continue;
        }
        for (const std::string& name : layer.pieces) {
            if (name.empty() || (manifest != nullptr && manifest->find(name) != nullptr)) {
                continue;
            }
            missing.try_emplace(name, layer.kind == core::LayerKind::Ground);
        }
    }
    if (!missing.empty()) {
        PieceCatalogGroup group{.label = std::string{MISSING_PIECES_GROUP}, .pieces = {}};
        for (const auto& [name, floor] : missing) {
            group.pieces.push_back(PieceCatalogEntry{.name = name,
                                                     .file = {},
                                                     .pieceClass = core::ScenePieceClass::Other,
                                                     .footprint = {},
                                                     .tactical = core::PieceTactical::Open,
                                                     .missing = true,
                                                     .floor = floor});
        }
        catalog.push_back(std::move(group));
    }
    return catalog;
}

std::vector<PieceCatalogGroup> filterPieceCatalog(const std::vector<PieceCatalogGroup>& catalog,
                                                  std::string_view query) {
    const std::string needle = lowered(query);
    if (needle.empty()) {
        return catalog;
    }
    std::vector<PieceCatalogGroup> filtered;
    for (const PieceCatalogGroup& group : catalog) {
        PieceCatalogGroup kept{.label = group.label, .pieces = {}};
        for (const PieceCatalogEntry& entry : group.pieces) {
            if (lowered(entry.name).find(needle) != std::string::npos ||
                std::string_view{classLabel(entry.pieceClass)}.find(needle) !=
                    std::string_view::npos) {
                kept.pieces.push_back(entry);
            }
        }
        if (!kept.pieces.empty()) {
            filtered.push_back(std::move(kept));
        }
    }
    return filtered;
}

std::string pieceDescription(const PieceCatalogEntry& entry) {
    if (entry.missing) {
        return entry.name + " — missing from the sheet: kept in the map, drawn as a checkerboard";
    }
    return entry.name + " — " + classLabel(entry.pieceClass) + ", " +
           std::to_string(entry.footprint.columns) + " × " + std::to_string(entry.footprint.rows) +
           ", " + core::pieceTacticalName(entry.tactical);
}

std::optional<std::size_t> pieceTargetLayer(const std::vector<core::TileLayer>& layers, bool floor,
                                            LayerSlot active) {
    const core::LayerKind wanted = floor ? core::LayerKind::Ground : core::LayerKind::Decor;
    // La couche de decor qu'on peint, quand c'en est une : un etage se peint comme le rez
    // (LOT-129).
    if (!floor && active && *active < layers.size() && layers[*active].kind == wanted) {
        return *active;
    }
    const auto found = std::ranges::find_if(layers, [wanted](const core::TileLayer& layer) {
        return layer.kind == wanted && layer.floor == 0;
    });
    if (found == layers.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(layers.begin(), found));
}

core::TileType pieceCellType(const PlaceAppearance* appearance, std::string_view piece,
                             bool floor) {
    if (appearance != nullptr) {
        if (const std::optional<core::TileType> type = appearance->typeOfPiece(piece, floor)) {
            return *type;
        }
    }
    return floor ? core::TileType::Empty : core::TileType::Wall;
}

}  // namespace hmi
