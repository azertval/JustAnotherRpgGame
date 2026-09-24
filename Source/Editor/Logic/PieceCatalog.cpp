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

[[nodiscard]] PieceCatalogEntry entryOf(const core::ScenePiece& piece) {
    return PieceCatalogEntry{.name = piece.name,
                             .file = piece.path(),
                             .pieceClass = piece.pieceClass,
                             .footprint = piece.footprint(),
                             .tactical = piece.tactical,
                             .missing = false,
                             .floor = piece.pieceClass == core::ScenePieceClass::Floor,
                             .masks = {},
                             .maskedBy = {}};
}

/// @return Le dossier de @p file relatif à son niveau (`roofs/l/d3`), vide pour un fichier à plat.
[[nodiscard]] std::string folderOf(std::string_view file) {
    const std::size_t slash = file.rfind('/');
    return slash == std::string_view::npos ? std::string{} : std::string{file.substr(0, slash)};
}

// Une pièce d'un niveau, telle que la palette la range : son entrée, et son fichier relatif au
// niveau, qui dit son sous-dossier.
struct LevelPiece {
    PieceCatalogEntry entry;
    std::string file;
};

// Les groupes des pièces d'un niveau. Un kit rangé en sous-dossiers se groupe par dossier, dans
// l'ordre alphabétique (`floors`, `roofs/l/d2`…) : c'est l'arborescence que l'auteur a choisie, et
// six cents toits n'ont pas de sens dans un seul groupe (LOT-129). Les pièces à plat, elles, se
// groupent par classe, un groupe vide n'étant pas rendu.
void appendLevelGroups(const std::string& level, const std::vector<LevelPiece>& pieces,
                       std::vector<PieceCatalogGroup>& groups) {
    std::map<std::string, PieceCatalogGroup> folders;
    for (const LevelPiece& piece : pieces) {
        if (std::string folder = folderOf(piece.file); !folder.empty()) {
            PieceCatalogGroup& group = folders[folder];
            group.label = folder;
            group.level = level;
            group.pieces.push_back(piece.entry);
        }
    }
    for (const auto& [pieceClass, label] : CLASS_GROUPS) {
        PieceCatalogGroup group{.label = std::string{label}, .level = level, .pieces = {}};
        for (const LevelPiece& piece : pieces) {
            if (piece.entry.pieceClass == pieceClass && folderOf(piece.file).empty()) {
                group.pieces.push_back(piece.entry);
            }
        }
        if (!group.pieces.empty()) {
            groups.push_back(std::move(group));
        }
    }
    for (auto& [folder, group] : folders) {
        groups.push_back(std::move(group));
    }
}

// Les groupes du catalogue, niveau par niveau (LOT-124). Deux dossiers d'un même niveau (le monde
// en a trois) se rangent sous le même titre.
std::vector<PieceCatalogGroup> manifestGroups(const core::ScenePieceManifest& manifest) {
    std::vector<std::string> levels;
    for (const core::SceneLevel& level : manifest.levels()) {
        if (std::ranges::find(levels, level.label) == levels.end()) {
            levels.push_back(level.label);
        }
    }
    if (levels.empty()) {
        levels.emplace_back();  // un manifeste lu seul : un seul niveau, sans nom.
    }
    // Le niveau commun que masque chaque pièce propre.
    std::map<std::string, std::string, std::less<>> masks;
    for (const core::MaskedScenePiece& masked : manifest.masked()) {
        masks.try_emplace(masked.piece.name, masked.piece.level);
    }
    std::vector<PieceCatalogGroup> groups;
    for (const std::string& level : levels) {
        std::vector<LevelPiece> pieces;
        for (const core::ScenePiece& piece : manifest.pieces()) {
            if (piece.level == level) {
                PieceCatalogEntry entry = entryOf(piece);
                if (const auto found = masks.find(piece.name); found != masks.end()) {
                    entry.masks = found->second;
                }
                pieces.push_back(LevelPiece{.entry = std::move(entry), .file = piece.file});
            }
        }
        for (const core::MaskedScenePiece& masked : manifest.masked()) {
            if (masked.piece.level == level) {
                PieceCatalogEntry entry = entryOf(masked.piece);
                entry.maskedBy = masked.by;
                pieces.push_back(LevelPiece{.entry = std::move(entry), .file = masked.piece.file});
            }
        }
        appendLevelGroups(level, pieces, groups);
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
        PieceCatalogGroup group{
            .label = std::string{MISSING_PIECES_GROUP}, .level = {}, .pieces = {}};
        for (const auto& [name, floor] : missing) {
            group.pieces.push_back(PieceCatalogEntry{.name = name,
                                                     .file = {},
                                                     .pieceClass = core::ScenePieceClass::Other,
                                                     .footprint = {},
                                                     .tactical = core::PieceTactical::Open,
                                                     .missing = true,
                                                     .floor = floor,
                                                     .masks = {},
                                                     .maskedBy = {}});
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
        PieceCatalogGroup kept{.label = group.label, .level = group.level, .pieces = {}};
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
    std::string description = entry.name + " — " + classLabel(entry.pieceClass) + ", " +
                              std::to_string(entry.footprint.columns) + " × " +
                              std::to_string(entry.footprint.rows) + ", " +
                              core::pieceTacticalName(entry.tactical);
    if (!entry.maskedBy.empty()) {
        description += " — masked by the " + entry.maskedBy + " piece of the same name";
    } else if (!entry.masks.empty()) {
        description += " — masks the " + entry.masks + " piece of the same name";
    }
    return description;
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
