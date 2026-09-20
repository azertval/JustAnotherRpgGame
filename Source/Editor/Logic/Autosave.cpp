// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/Autosave.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

namespace hmi {

namespace {

constexpr std::string_view SUFFIX = ".autosave.json";
constexpr std::string_view CONFLICTS = "conflicts";

// Écrit @p content dans @p path en passant par un fichier voisin : le renommage remplace d'un coup,
// et un plantage pendant l'écriture ne laisse que le temporaire.
[[nodiscard]] bool writeReplacing(const std::filesystem::path& path, std::string_view content) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    std::filesystem::path temporary = path;
    temporary += ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file) {
            return false;
        }
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!file.good()) {
            return false;
        }
    }
    std::filesystem::rename(temporary, path, error);
    if (error) {
        std::filesystem::remove(temporary, error);
        return false;
    }
    return true;
}

[[nodiscard]] std::optional<std::string> readAll(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

}  // namespace

std::string autosaveFileName(std::string_view mapId) {
    std::string name = mapId.empty() ? std::string{"untitled"} : std::string{mapId};
    // Un identifiant porte le sous-dossier de la carte (`capital/martpart`) : le dossier de
    // reprise, lui, est plat. `~` n'est pas un caractère de nom de carte (`isValidLevelName`).
    std::ranges::replace(name, '/', '~');
    std::ranges::replace(name, '\\', '~');
    name += SUFFIX;
    return name;
}

std::string serializeAutosave(const AutosaveRecord& record) {
    // Le chemin en UTF-8, en `std::string` : un `std::u8string` passerait dans le JSON comme un
    // tableau d'entiers, et le fichier ne se relirait plus.
    const std::u8string path = record.levelPath.generic_u8string();
    const nlohmann::json root = {
        {"format", AUTOSAVE_FORMAT},
        {"mapId", record.mapId},
        {"levelPath", std::string(path.begin(), path.end())},
        {"draft", record.draftJson},
    };
    return root.dump(2) + "\n";
}

std::optional<AutosaveRecord> parseAutosave(std::string_view text) {
    const nlohmann::json root = nlohmann::json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (!root.is_object() || root.value("format", 0) != AUTOSAVE_FORMAT) {
        return std::nullopt;
    }
    const auto field = [&root](const char* name) -> const nlohmann::json* {
        const auto found = root.find(name);
        return found != root.end() && found->is_string() ? &*found : nullptr;
    };
    const nlohmann::json* const mapId = field("mapId");
    const nlohmann::json* const levelPath = field("levelPath");
    const nlohmann::json* const draft = field("draft");
    if (mapId == nullptr || levelPath == nullptr || draft == nullptr) {
        return std::nullopt;
    }
    const std::string path = levelPath->get<std::string>();
    return AutosaveRecord{
        .mapId = mapId->get<std::string>(),
        .levelPath = std::filesystem::path(std::u8string(path.begin(), path.end())),
        .draftJson = draft->get<std::string>(),
    };
}

AutosaveStore::AutosaveStore(std::filesystem::path directory) : _directory(std::move(directory)) {}

std::filesystem::path AutosaveStore::pathFor(std::string_view mapId) const {
    return _directory / autosaveFileName(mapId);
}

bool AutosaveStore::write(const AutosaveRecord& record) const {
    return writeReplacing(pathFor(record.mapId), serializeAutosave(record));
}

std::vector<AutosaveRecord> AutosaveStore::pending() const {
    std::vector<AutosaveRecord> records;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(_directory, error)) {
        const std::string name = entry.path().filename().string();
        if (!entry.is_regular_file(error) || !name.ends_with(SUFFIX)) {
            continue;
        }
        if (const std::optional<std::string> text = readAll(entry.path())) {
            if (std::optional<AutosaveRecord> record = parseAutosave(*text)) {
                records.push_back(std::move(*record));
            }
        }
    }
    std::ranges::sort(records, {}, &AutosaveRecord::mapId);
    return records;
}

void AutosaveStore::discard(std::string_view mapId) const {
    std::error_code error;
    std::filesystem::remove(pathFor(mapId), error);
}

std::optional<std::filesystem::path> AutosaveStore::keepAside(std::string_view mapId,
                                                              std::string_view label,
                                                              std::string_view stamp,
                                                              std::string_view content) const {
    std::string name = autosaveFileName(mapId);
    name.resize(name.size() - SUFFIX.size());
    name += '.';
    name += label;
    name += '.';
    name += stamp;
    name += ".json";
    std::filesystem::path path = _directory / CONFLICTS / name;
    if (!writeReplacing(path, content)) {
        return std::nullopt;
    }
    return path;
}

}  // namespace hmi
