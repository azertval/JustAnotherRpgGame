// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/MapTexts.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <system_error>

#include "HMI/Localization/Localization.h"

namespace hmi {

namespace {

[[nodiscard]] std::string readText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Les fichiers `<langue>.lang` du dossier, tries.
[[nodiscard]] std::vector<std::filesystem::path> catalogFiles(
    const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    for (auto it = std::filesystem::directory_iterator(directory, error);
         !error && it != std::filesystem::directory_iterator(); it.increment(error)) {
        if (it->is_regular_file(error) && it->path().extension() == ".lang") {
            files.push_back(it->path());
        }
    }
    std::ranges::sort(files);
    return files;
}

}  // namespace

std::string mapNameKey(std::string_view mapId) {
    std::string key = "map.";
    key += mapId;
    std::ranges::replace(key, '/', '.');
    return key + ".name";
}

std::filesystem::path localizationDirectory(const std::filesystem::path& dataRoot) {
    return dataRoot / "Localization";
}

TranslationCatalogs loadTranslationCatalogs(const std::filesystem::path& directory) {
    TranslationCatalogs catalogs;
    for (const std::filesystem::path& file : catalogFiles(directory)) {
        catalogs.emplace(file.stem().string(), Localization::parseCatalog(readText(file)));
    }
    return catalogs;
}

std::vector<std::string> languagesMissing(const TranslationCatalogs& catalogs,
                                          std::string_view key) {
    std::vector<std::string> missing;
    for (const auto& [language, strings] : catalogs) {
        if (!strings.contains(std::string{key})) {
            missing.push_back(language);
        }
    }
    return missing;
}

bool addTranslation(const std::filesystem::path& directory, std::string_view key,
                    std::string_view text, std::string_view copyFrom) {
    bool written = true;
    for (const std::filesystem::path& file : catalogFiles(directory)) {
        const std::string content = readText(file);
        const std::unordered_map<std::string, std::string> strings =
            Localization::parseCatalog(content);
        if (strings.contains(std::string{key})) {
            continue;
        }
        const auto previous =
            copyFrom.empty() ? strings.end() : strings.find(std::string{copyFrom});
        std::ofstream stream(file, std::ios::binary | std::ios::app);
        if (!content.empty() && content.back() != '\n') {
            stream << '\n';
        }
        stream << key << " = " << (previous != strings.end() ? previous->second : std::string{text})
               << '\n';
        written = written && stream.good();
    }
    return written;
}

std::string nameMapInCatalogs(const std::filesystem::path& dataRoot, std::string_view mapId,
                              std::string_view text, std::string_view copyFrom) {
    std::string key = mapNameKey(mapId);
    return addTranslation(localizationDirectory(dataRoot), key, text, copyFrom) ? key
                                                                                : std::string{};
}

}  // namespace hmi
