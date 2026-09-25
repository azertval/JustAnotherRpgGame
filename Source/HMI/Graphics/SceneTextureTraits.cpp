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

/// Le manifeste de @p directory, lu et indexé, s'il se lit.
[[nodiscard]] std::unique_ptr<const IndexedManifest> readManifest(
    const std::filesystem::path& directory) {
    std::error_code error;
    const std::filesystem::path path = directory / "manifest.json";
    if (!std::filesystem::is_regular_file(path, error)) {
        return nullptr;
    }
    core::JsonDocument document = core::readJsonObjectFromFile(path, MANIFEST_VERSION);
    if (!document.ok()) {
        return nullptr;
    }
    // Construit en place : l'index pointe dans le document, qui ne bougera plus.
    auto manifest = std::make_unique<IndexedManifest>();
    manifest->root = std::move(document.root);
    const auto textures = manifest->root.find("textures");
    if (textures != manifest->root.end() && textures->is_object()) {
        for (const nlohmann::json& entry : *textures) {
            const auto file = entry.is_object() ? entry.find("file") : entry.end();
            if (file != entry.end() && file->is_string()) {
                manifest->entries.try_emplace(file->get<std::string>(), &entry);
            }
        }
    }
    return manifest;
}

/// L'ancre d'une entrée de manifeste, rien si elle n'est pas numérique.
[[nodiscard]] std::optional<core::Vector2> anchorOf(const nlohmann::json& entry) {
    const auto anchor = entry.find("anchor");
    if (anchor == entry.end() || !anchor->is_array() || anchor->size() != 2 ||
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

/// Le `depthOffset` d'une entrée, rien sans ancre valide ou sans valeur finie.
[[nodiscard]] std::optional<float> depthOffsetOf(const nlohmann::json& entry) {
    if (!anchorOf(entry)) {
        return std::nullopt;
    }
    const auto offset = entry.find("depthOffset");
    if (offset == entry.end() || !offset->is_number()) {
        return std::nullopt;
    }
    const auto value = offset->get<float>();
    return std::isfinite(value) ? std::optional<float>{value} : std::nullopt;
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
    return entry != nullptr ? anchorOf(*entry) : std::nullopt;
}

std::optional<float> scenePieceDepthOffset(const nlohmann::json& manifest,
                                           std::string_view filename) {
    const nlohmann::json* const entry = entryOf(manifest, filename);
    return entry != nullptr ? depthOffsetOf(*entry) : std::nullopt;
}

const nlohmann::json* IndexedManifest::entry(std::string_view filename) const {
    const auto found = entries.find(std::string{filename});
    return found != entries.end() ? found->second : nullptr;
}

const IndexedManifest* ManifestCache::find(const std::filesystem::path& directory) {
    const auto found = _read.find(directory);
    if (found != _read.end()) {
        return found->second.get();
    }
    return _read.emplace(directory, readManifest(directory)).first->second.get();
}

SceneTextureTraits readSceneTextureTraits(const std::filesystem::path& assetsDirectory,
                                          std::string_view path, ManifestCache* manifests) {
    // Sans cache fourni, un cache le temps de l'appel : la lecture est la même.
    ManifestCache local;
    ManifestCache& cache = manifests != nullptr ? *manifests : local;

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
            const core::AnimationClip& clip = read.description->clips.clipAt(0);
            traits.frameDuration = clip.frameDuration;
            traits.loop = clip.endMode == core::ClipEndMode::Loop;
        }
    }

    // La pièce : son manifeste est dans son dossier. La figurine : plus haut, dans celui de
    // l'atelier qui la range (`Characters/`, `Npc/`) — un ou plusieurs dossiers au-dessus, car un
    // héros se range par classe (`Characters/Heroes/brawler/`).
    // Une pièce peut aussi être rangée dans un sous-dossier de son lieu (`roofs/l/d3/…`, LOT-129) :
    // son manifeste est alors plus haut, et la cite par son chemin relatif à lui. Le premier
    // manifeste qui la cite, en remontant, est le sien.
    const IndexedManifest* manifest = nullptr;
    const nlohmann::json* entry = nullptr;
    const std::filesystem::path relative{path};
    for (std::filesystem::path owner = relative.parent_path(); !owner.empty();
         owner = owner.parent_path()) {
        const IndexedManifest* const candidate = cache.find(assetsDirectory / owner);
        const std::string key = relative.lexically_relative(owner).generic_string();
        if (candidate != nullptr) {
            if (const nlohmann::json* const cited = candidate->entry(key); cited != nullptr) {
                manifest = candidate;
                entry = cited;
                break;
            }
        }
        if (owner == relative.parent_path() && candidate != nullptr) {
            manifest = candidate;  // le dossier de l'image, à défaut : figurines, sols.
        }
    }
    if (manifest != nullptr) {
        traits.artTile = manifestArtTile(manifest->root);
        if (entry != nullptr) {
            traits.anchor = anchorOf(*entry);
            traits.depthOffset = depthOffsetOf(*entry);
        }
        traits.groundLine = manifestGroundLine(manifest->root);
        // La hauteur d'un étage : une donnée du lieu, que son manifeste déclare (`LOT-129`).
        traits.storeyHeight = manifestLength(manifest->root, "storey");
    }
    // Les ancêtres se remontent dans le chemin RELATIF : la lecture ne sort jamais de la racine.
    std::filesystem::path ancestor = std::filesystem::path(path).parent_path().parent_path();
    for (; traits.artTile.x <= 0.0F && !ancestor.empty(); ancestor = ancestor.parent_path()) {
        if (const IndexedManifest* const above = cache.find(assetsDirectory / ancestor)) {
            traits.artTile = manifestArtTile(above->root);
            if (!traits.groundLine) {
                traits.groundLine = manifestGroundLine(above->root);
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
    texture.loop = traits.loop;
    texture.storeyHeight = traits.storeyHeight;
}

}  // namespace hmi
