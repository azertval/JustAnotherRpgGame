// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Resources/SkeletonFile.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

#include "Core/Data/JsonDocument.h"

namespace core {

namespace {

using Json = nlohmann::json;

[[nodiscard]] SkeletonFileResult skeletonFailed(std::string message) {
    return SkeletonFileResult{.skeleton = {}, .message = std::move(message)};
}

[[nodiscard]] CharacterSheetFileResult sheetFailed(std::string message) {
    return CharacterSheetFileResult{.sheet = {}, .message = std::move(message)};
}

// Le texte du champ `key` de `object` ; rien s'il manque ou n'est pas une chaine.
[[nodiscard]] std::optional<std::string> text(const Json& object, const char* key) {
    const auto found = object.find(key);
    if (found == object.end() || !found->is_string()) {
        return std::nullopt;
    }
    return found->get<std::string>();
}

// Le nombre fini du champ `key` de `object` ; rien s'il manque ou n'en est pas un.
[[nodiscard]] std::optional<float> number(const Json& object, const char* key) {
    const auto found = object.find(key);
    if (found == object.end() || !found->is_number()) {
        return std::nullopt;
    }
    const auto value = found->get<float>();
    return std::isfinite(value) ? std::optional<float>{value} : std::nullopt;
}

// Un nom de fichier ou de silhouette : pas de chemin, pour qu'une fiche ne sorte pas de son
// dossier.
[[nodiscard]] bool plainName(std::string_view name) {
    return !name.empty() && name.find_first_of("/\\") == std::string_view::npos && name != "." &&
           name != "..";
}

[[nodiscard]] SkeletonFileResult skeletonFrom(const JsonDocument& document) {
    if (!document.ok()) {
        return skeletonFailed(document.message);
    }
    const Json& root = document.root;
    SkeletonDescription skeleton;
    skeleton.silhouette = text(root, "silhouette").value_or(std::string{});
    if (!plainName(skeleton.silhouette)) {
        return skeletonFailed("silhouette missing");
    }
    const auto bones = root.find("bones");
    if (bones == root.end() || !bones->is_array() || bones->empty()) {
        return skeletonFailed("bones missing");
    }
    std::set<std::string, std::less<>> known;
    for (const Json& entry : *bones) {
        const std::optional<std::string> name =
            entry.is_object() ? text(entry, "name") : std::nullopt;
        if (!name || name->empty()) {
            return skeletonFailed("bone without a name");
        }
        std::string parent = text(entry, "parent").value_or(std::string{});
        // Parents avant enfants : un parent encore inconnu est un os absent, ou un cycle.
        if (!parent.empty() && !known.contains(parent)) {
            return skeletonFailed("bone " + *name + " cites the unknown parent " + parent);
        }
        if (!known.insert(*name).second) {
            return skeletonFailed("bone " + *name + " declared twice");
        }
        skeleton.bones.push_back(SkeletonBone{.name = *name, .parent = std::move(parent)});
    }
    const auto clips = root.find("clips");
    if (clips == root.end() || !clips->is_array()) {
        return skeletonFailed("clips missing");
    }
    for (const Json& entry : *clips) {
        const std::optional<std::string> name =
            entry.is_object() ? text(entry, "name") : std::nullopt;
        if (!name || name->empty()) {
            return skeletonFailed("clip without a name");
        }
        if (skeleton.clip(*name) != nullptr) {
            return skeletonFailed("clip " + *name + " declared twice");
        }
        SkeletonClip clip;
        clip.name = *name;
        const std::optional<float> duration = number(entry, "duration");
        if (!duration || *duration <= 0.0F) {
            return skeletonFailed("clip " + *name + " must declare a positive duration");
        }
        clip.duration = *duration;
        if (const auto loop = entry.find("loop"); loop != entry.end()) {
            if (!loop->is_boolean()) {
                return skeletonFailed("clip " + *name + ": loop is not a boolean");
            }
            clip.loop = loop->get<bool>();
        }
        if (entry.contains("key")) {
            const std::optional<float> key = number(entry, "key");
            if (!key || *key < 0.0F || *key > clip.duration) {
                return skeletonFailed("clip " + *name + ": key must lie within the clip");
            }
            clip.key = key;
        }
        skeleton.clips.push_back(std::move(clip));
    }
    return SkeletonFileResult{.skeleton = std::move(skeleton), .message = {}};
}

[[nodiscard]] CharacterSheetFileResult sheetFrom(const JsonDocument& document) {
    if (!document.ok()) {
        return sheetFailed(document.message);
    }
    CharacterSheetFile sheet;
    sheet.model = text(document.root, "model").value_or(std::string{});
    sheet.skeleton = text(document.root, "skeleton").value_or(std::string{});
    if (!plainName(sheet.model)) {
        return sheetFailed("model must name a file of the character directory");
    }
    if (!plainName(sheet.skeleton)) {
        return sheetFailed("skeleton missing");
    }
    return CharacterSheetFileResult{.sheet = std::move(sheet), .message = {}};
}

}  // namespace

const SkeletonClip* SkeletonDescription::clip(std::string_view name) const noexcept {
    const auto found =
        std::ranges::find_if(clips, [name](const SkeletonClip& clip) { return clip.name == name; });
    return found != clips.end() ? &*found : nullptr;
}

SkeletonFileResult readSkeletonDescription(std::string_view json) {
    try {
        return skeletonFrom(readJsonObject(json, SKELETON_FORMAT_VERSION, "skeleton.json"));
    } catch (const std::exception& error) {
        return skeletonFailed(error.what());
    }
}

SkeletonFileResult readSkeletonFile(const std::filesystem::path& path) {
    try {
        SkeletonFileResult result =
            skeletonFrom(readJsonObjectFromFile(path, SKELETON_FORMAT_VERSION));
        if (!result.ok()) {
            result.message = path.string() + ": " + result.message;
        }
        return result;
    } catch (const std::exception& error) {
        return skeletonFailed(error.what());
    }
}

std::string skeletonFilePath(std::string_view silhouette) {
    std::string path{"Common/Characters/Skeletons/"};
    path.append(silhouette);
    path.append("/skeleton.json");
    return path;
}

CharacterSheetFileResult readCharacterSheet(std::string_view json) {
    try {
        return sheetFrom(readJsonObject(json, SKELETON_FORMAT_VERSION, CHARACTER_SHEET_FILE));
    } catch (const std::exception& error) {
        return sheetFailed(error.what());
    }
}

CharacterSheetFileResult readCharacterSheetFile(const std::filesystem::path& path) {
    try {
        CharacterSheetFileResult result =
            sheetFrom(readJsonObjectFromFile(path, SKELETON_FORMAT_VERSION));
        if (!result.ok()) {
            result.message = path.string() + ": " + result.message;
        }
        return result;
    } catch (const std::exception& error) {
        return sheetFailed(error.what());
    }
}

}  // namespace core
