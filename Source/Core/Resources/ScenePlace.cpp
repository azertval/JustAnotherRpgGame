// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Resources/ScenePlace.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <system_error>

#include "Core/Data/JsonDocument.h"

namespace core {

namespace {

// Les dossiers de pièces du monde, dans l'ordre où l'on y cherche : ce qu'on voit partout.
constexpr std::array<std::string_view, 3> WORLD_DIRECTORIES = {"Common/Terrain", "Common/Nature",
                                                               "Common/Props"};
constexpr std::string_view WORLD_LABEL = "World";
constexpr std::string_view REGIONS = "Regions";
constexpr std::string_view FLAT_ROOT = "Scene";
constexpr std::string_view OWN_SCENE = "Scene";
constexpr std::string_view COMMON_SCENE = "Common/Scene";
constexpr std::string_view OWN_CHARACTERS = "Characters";
constexpr std::string_view COMMON_CHARACTERS = "Common/Characters";
// L'atelier des PNJ a plat, celui des racines d'essai (LOT-91) : le repli d'un slug introuvable.
constexpr std::string_view FLAT_FIGURES = "Npc/";

// Les petits mots qu'un nom de lieu garde en minuscules, sauf en tête.
constexpr std::array<std::string_view, 5> SMALL_WORDS = {"of", "the", "and", "de", "du"};

[[nodiscard]] std::vector<std::string_view> segmentsOf(std::string_view place) {
    std::vector<std::string_view> segments;
    std::size_t start = 0;
    while (start <= place.size()) {
        const std::size_t slash = place.find('/', start);
        const std::size_t end = slash == std::string_view::npos ? place.size() : slash;
        segments.push_back(place.substr(start, end - start));
        if (slash == std::string_view::npos) {
            break;
        }
        start = slash + 1;
    }
    return segments;
}

[[nodiscard]] std::string joined(const std::vector<std::string_view>& segments, std::size_t count) {
    std::string path;
    for (std::size_t index = 0; index < count; ++index) {
        if (index > 0) {
            path.push_back('/');
        }
        path.append(segments[index]);
    }
    return path;
}

void appendWorld(std::vector<SceneLevel>& levels) {
    for (const std::string_view directory : WORLD_DIRECTORIES) {
        levels.push_back(SceneLevel{
            .label = std::string{WORLD_LABEL}, .place = {}, .directory = std::string{directory}});
    }
}

}  // namespace

bool isValidScenePlace(std::string_view place) noexcept {
    if (place.empty() || place.find('\\') != std::string_view::npos) {
        return false;
    }
    std::size_t start = 0;
    while (true) {
        const std::size_t slash = place.find('/', start);
        const std::string_view segment = place.substr(
            start, slash == std::string_view::npos ? std::string_view::npos : slash - start);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        if (slash == std::string_view::npos) {
            return true;
        }
        start = slash + 1;
    }
}

bool isFlatScenePlace(std::string_view place) noexcept {
    return place.find('/') == std::string_view::npos;
}

std::string scenePlaceLabel(std::string_view segment) {
    std::string label;
    std::size_t start = 0;
    while (start <= segment.size()) {
        const std::size_t dash = segment.find('-', start);
        const std::string_view word = segment.substr(
            start, dash == std::string_view::npos ? std::string_view::npos : dash - start);
        if (!word.empty()) {
            std::string written{word};
            const bool small = std::ranges::find(SMALL_WORDS, word) != SMALL_WORDS.end();
            if (label.empty() || !small) {
                written.front() =
                    static_cast<char>(std::toupper(static_cast<unsigned char>(written.front())));
            }
            if (!label.empty()) {
                label.push_back(' ');
            }
            label += written;
        }
        if (dash == std::string_view::npos) {
            break;
        }
        start = dash + 1;
    }
    return label;
}

std::vector<SceneLevel> sceneLevelCandidates(std::string_view place) {
    std::vector<SceneLevel> levels;
    if (!isValidScenePlace(place)) {
        return levels;
    }
    if (isFlatScenePlace(place)) {
        levels.push_back(
            SceneLevel{.label = scenePlaceLabel(place),
                       .place = std::string{place},
                       .directory = std::string{FLAT_ROOT} + "/" + std::string{place}});
        appendWorld(levels);
        return levels;
    }
    const std::vector<std::string_view> segments = segmentsOf(place);
    for (std::size_t count = segments.size(); count >= 1; --count) {
        const std::string prefix = joined(segments, count);
        const std::string label = scenePlaceLabel(segments[count - 1]);
        const std::string base = std::string{REGIONS} + "/" + prefix + "/";
        // Une région n'a qu'un commun : ses zones hors ville ont leur propre dossier.
        if (count > 1) {
            levels.push_back(SceneLevel{
                .label = label, .place = prefix, .directory = base + std::string{OWN_SCENE}});
        }
        levels.push_back(SceneLevel{
            .label = label, .place = prefix, .directory = base + std::string{COMMON_SCENE}});
    }
    appendWorld(levels);
    return levels;
}

std::string ownSceneDirectory(std::string_view place) {
    if (!isValidScenePlace(place)) {
        return {};
    }
    if (isFlatScenePlace(place)) {
        return std::string{FLAT_ROOT} + "/" + std::string{place};
    }
    return std::string{REGIONS} + "/" + std::string{place} + "/" + std::string{OWN_SCENE};
}

std::string fallbackScenePiecePath(std::string_view place, std::string_view piece) {
    const std::string directory = ownSceneDirectory(place);
    if (directory.empty() || piece.empty()) {
        return {};
    }
    return directory + "/" + std::string{piece} + ".png";
}

bool scenePlaceDescendsFrom(std::string_view place, std::string_view ancestor) {
    if (ancestor.empty()) {
        return true;
    }
    return place == ancestor || (place.size() > ancestor.size() && place.starts_with(ancestor) &&
                                 place[ancestor.size()] == '/');
}

std::vector<std::string> scenePlaceAncestry(std::string_view place) {
    std::vector<std::string> ancestry;
    if (isValidScenePlace(place)) {
        const std::vector<std::string_view> segments = segmentsOf(place);
        for (std::size_t count = segments.size(); count >= 1; --count) {
            ancestry.push_back(joined(segments, count));
        }
    }
    ancestry.emplace_back();
    return ancestry;
}

std::vector<std::string> scenePlaces(const std::filesystem::path& assetsDirectory) {
    std::vector<std::string> places;
    std::error_code error;
    for (auto it = std::filesystem::directory_iterator(assetsDirectory / FLAT_ROOT, error);
         !error && it != std::filesystem::directory_iterator(); it.increment(error)) {
        if (it->is_directory(error) &&
            std::filesystem::is_regular_file(it->path() / "manifest.json", error)) {
            places.push_back(it->path().filename().string());
        }
    }
    error.clear();
    const std::filesystem::path regions = assetsDirectory / REGIONS;
    for (auto it = std::filesystem::recursive_directory_iterator(regions, error);
         !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
        // Un lieu est un dossier dont le `Scene/` PROPRE porte un manifeste ; un `Common/` n'en est
        // pas un, et ce qu'il y a sous un `Scene/` non plus.
        const std::string name = it->path().filename().string();
        if (name == OWN_SCENE || name == "Common" || name == "Characters" || name == "Map") {
            it.disable_recursion_pending();
            continue;
        }
        if (it->is_directory(error) &&
            std::filesystem::is_regular_file(it->path() / OWN_SCENE / "manifest.json", error)) {
            places.push_back(it->path().lexically_relative(regions).generic_string());
        }
    }
    std::ranges::sort(places);
    return places;
}

std::vector<SceneLevel> characterLevelCandidates(std::string_view place) {
    std::vector<SceneLevel> levels;
    if (isValidScenePlace(place) && !isFlatScenePlace(place)) {
        const std::vector<std::string_view> segments = segmentsOf(place);
        for (std::size_t count = segments.size(); count >= 1; --count) {
            const std::string prefix = joined(segments, count);
            const std::string label = scenePlaceLabel(segments[count - 1]);
            const std::string base = std::string{REGIONS} + "/" + prefix + "/";
            if (count > 1) {
                levels.push_back(SceneLevel{.label = label,
                                            .place = prefix,
                                            .directory = base + std::string{OWN_CHARACTERS}});
            }
            levels.push_back(SceneLevel{.label = label,
                                        .place = prefix,
                                        .directory = base + std::string{COMMON_CHARACTERS}});
        }
    }
    levels.push_back(SceneLevel{.label = std::string{WORLD_LABEL},
                                .place = {},
                                .directory = std::string{COMMON_CHARACTERS}});
    return levels;
}

FigureDirectories resolveFigures(const std::filesystem::path& assetsDirectory,
                                 std::string_view place) {
    FigureDirectories figures;
    for (const SceneLevel& level : characterLevelCandidates(place)) {
        const std::filesystem::path file =
            assetsDirectory / std::filesystem::path(level.directory) / "manifest.json";
        std::error_code error;
        if (!std::filesystem::is_regular_file(file, error)) {
            continue;
        }
        const JsonDocument document = readJsonObjectFromFile(file, 1);
        const auto npcs = document.ok() ? document.root.find("npcs") : document.root.end();
        if (!document.ok() || npcs == document.root.end() || !npcs->is_array()) {
            continue;
        }
        for (const nlohmann::json& slug : *npcs) {
            if (slug.is_string() && !slug.get<std::string>().empty()) {
                // Le plus propre gagne : un niveau plus commun ne remplace pas un slug déjà vu.
                figures.try_emplace(slug.get<std::string>(),
                                    level.directory + "/" + slug.get<std::string>());
            }
        }
    }
    return figures;
}

std::string figureDirectory(const FigureDirectories& figures, std::string_view figure) {
    if (figure.empty()) {
        return {};
    }
    if (const auto found = figures.find(figure); found != figures.end()) {
        return found->second;
    }
    if (figure.find('/') != std::string_view::npos) {
        return std::string{figure};
    }
    return std::string{FLAT_FIGURES} + std::string{figure};
}

}  // namespace core
