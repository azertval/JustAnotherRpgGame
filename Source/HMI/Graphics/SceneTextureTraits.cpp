// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/SceneTextureTraits.h"

#include <cmath>
#include <string>
#include <system_error>

#include "HMI/Graphics/AnimationCatalog.h"

namespace hmi {

namespace {

/// Version la plus élevée des manifestes lus.
constexpr int MANIFEST_VERSION = 1;

/// L'entrée de manifeste qui cite @p filename, `nullptr` si aucune.
[[nodiscard]] const nlohmann::json* entryOf(const nlohmann::json& manifest,
                                            std::string_view filename) {
    const auto textures = manifest.find("textures");
    if (!manifest.is_object() || textures == manifest.end() || !textures->is_object()) {
        return nullptr;
    }
    for (const nlohmann::json& entry : *textures) {
        const auto file = entry.find("file");
        if (entry.is_object() && file != entry.end() && file->is_string() &&
            file->get<std::string>() == filename) {
            return &entry;
        }
    }
    return nullptr;
}

/// Le manifeste de @p directory, s'il se lit.
[[nodiscard]] std::optional<nlohmann::json> manifestOf(const std::filesystem::path& directory) {
    std::error_code error;
    const std::filesystem::path path = directory / "manifest.json";
    if (!std::filesystem::is_regular_file(path, error)) {
        return std::nullopt;
    }
    core::JsonDocument document = core::readJsonObjectFromFile(path, MANIFEST_VERSION);
    if (!document.ok()) {
        return std::nullopt;
    }
    return std::move(document.root);
}

/// Le nombre fini positif que @p manifest déclare sous @p key, rien sinon.
[[nodiscard]] std::optional<float> manifestLength(const nlohmann::json& manifest,
                                                  std::string_view key) {
    const auto found = manifest.is_object() ? manifest.find(key) : manifest.end();
    if (found == manifest.end() || !found->is_number()) {
        return std::nullopt;
    }
    const auto value = found->get<float>();
    return std::isfinite(value) && value > 0.0F ? std::optional<float>{value} : std::nullopt;
}

/// La ligne de sol que déclare @p manifest (`ground`), rien si elle n'est pas un nombre fini
/// positif.
[[nodiscard]] std::optional<float> manifestGroundLine(const nlohmann::json& manifest) {
    return manifestLength(manifest, "ground");
}

}  // namespace

core::Vector2 manifestArtTile(const nlohmann::json& manifest) {
    const auto tile = manifest.is_object() ? manifest.find("tile") : manifest.end();
    if (tile == manifest.end() || !tile->is_array() || tile->size() != 2 ||
        !(*tile)[0].is_number() || !(*tile)[1].is_number()) {
        return {};
    }
    const auto width = (*tile)[0].get<float>();
    const auto height = (*tile)[1].get<float>();
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0F || height <= 0.0F) {
        return {};
    }
    return {width, height};
}

std::optional<core::Vector2> scenePieceAnchor(const nlohmann::json& manifest,
                                              std::string_view filename) {
    const nlohmann::json* const entry = entryOf(manifest, filename);
    if (entry == nullptr) {
        return std::nullopt;
    }
    const auto anchor = entry->find("anchor");
    if (anchor == entry->end() || !anchor->is_array() || anchor->size() != 2 ||
        !(*anchor)[0].is_number() || !(*anchor)[1].is_number()) {
        return std::nullopt;
    }
    const auto x = (*anchor)[0].get<float>();
    const auto y = (*anchor)[1].get<float>();
    if (!std::isfinite(x) || !std::isfinite(y)) {
        return std::nullopt;
    }
    return core::Vector2{x, y};
}

std::optional<float> scenePieceDepthOffset(const nlohmann::json& manifest,
                                           std::string_view filename) {
    if (!scenePieceAnchor(manifest, filename)) {
        return std::nullopt;
    }
    const nlohmann::json& entry = *entryOf(manifest, filename);
    const auto offset = entry.find("depthOffset");
    if (offset == entry.end() || !offset->is_number()) {
        return std::nullopt;
    }
    const auto value = offset->get<float>();
    return std::isfinite(value) ? std::optional<float>{value} : std::nullopt;
}

SceneTextureTraits readSceneTextureTraits(const std::filesystem::path& assetsDirectory,
                                          std::string_view path) {
    SceneTextureTraits traits;
    const std::filesystem::path file = assetsDirectory / std::filesystem::path(path);

    // La découpe d'une bande : seules les figurines ont un `.anim.json`, et son absence n'est pas
    // une anomalie.
    std::filesystem::path description = file;
    description.replace_extension();
    description += ".anim.json";
    if (const AnimationDescriptionResult read = AnimationCatalog::loadFromFile(description);
        read.ok()) {
        traits.frameWidth = read.description->frameWidth;
        traits.frameHeight = read.description->frameHeight;
        if (read.description->clips.clipCount() > 0) {
            traits.frameDuration = read.description->clips.clipAt(0).frameDuration;
        }
    }

    // La pièce : son manifeste est dans son dossier. La figurine : plus haut, dans celui de
    // l'atelier qui la range (`Characters/`, `Npc/`) — un ou plusieurs dossiers au-dessus, car un
    // héros se range par classe (`Characters/Heroes/brawler/`).
    const std::string filename = file.filename().string();
    if (const std::optional<nlohmann::json> manifest = manifestOf(file.parent_path())) {
        traits.artTile = manifestArtTile(*manifest);
        traits.anchor = scenePieceAnchor(*manifest, filename);
        traits.depthOffset = scenePieceDepthOffset(*manifest, filename);
        traits.groundLine = manifestGroundLine(*manifest);
        // La hauteur d'un étage : une donnée du lieu, que son manifeste déclare (`LOT-129`).
        traits.storeyHeight = manifestLength(*manifest, "storey");
    }
    // Les ancêtres se remontent dans le chemin RELATIF : la lecture ne sort jamais de la racine.
    std::filesystem::path ancestor = std::filesystem::path(path).parent_path().parent_path();
    for (; traits.artTile.x <= 0.0F && !ancestor.empty(); ancestor = ancestor.parent_path()) {
        if (const std::optional<nlohmann::json> manifest = manifestOf(assetsDirectory / ancestor)) {
            traits.artTile = manifestArtTile(*manifest);
            if (!traits.groundLine) {
                traits.groundLine = manifestGroundLine(*manifest);
            }
        }
    }
    return traits;
}

void applySceneTextureTraits(SceneTexture& texture, const SceneTextureTraits& traits) {
    texture.frameWidth = traits.frameWidth;
    texture.frameHeight = traits.frameHeight;
    texture.artTile = traits.artTile;
    texture.anchor = traits.anchor;
    texture.depthOffset = traits.depthOffset;
    texture.groundLine = traits.groundLine;
    texture.frameDuration = traits.frameDuration;
    texture.storeyHeight = traits.storeyHeight;
}

}  // namespace hmi
