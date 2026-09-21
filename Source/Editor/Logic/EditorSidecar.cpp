// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/EditorSidecar.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <system_error>
#include <tuple>
#include <utility>

namespace hmi {

namespace {

constexpr std::string_view SIDECAR_SUFFIX = ".editor.json";

[[nodiscard]] bool byRow(const AuthorNote& left, const AuthorNote& right) {
    return std::tie(left.cell.row, left.cell.column) < std::tie(right.cell.row, right.cell.column);
}

[[nodiscard]] bool isBlank(std::string_view text) {
    return std::ranges::all_of(text, [](char character) {
        return character == ' ' || character == '\t' || character == '\n' || character == '\r';
    });
}

}  // namespace

std::string_view mapStateKey(MapState state) noexcept {
    switch (state) {
        case MapState::Generated:
            return "generated";
        case MapState::Blockout:
            return "blockout";
        case MapState::Retouched:
            return "retouched";
        case MapState::Finished:
            return "finished";
        case MapState::Unset:
            break;
    }
    return {};
}

MapState mapStateFromKey(std::string_view key) noexcept {
    for (const MapState state : knownMapStates()) {
        if (mapStateKey(state) == key) {
            return state;
        }
    }
    return MapState::Unset;
}

std::string_view mapStateLabel(MapState state) noexcept {
    switch (state) {
        case MapState::Generated:
            return "Generated";
        case MapState::Blockout:
            return "Blockout";
        case MapState::Retouched:
            return "Retouched";
        case MapState::Finished:
            return "Finished";
        case MapState::Unset:
            break;
    }
    return "not stated";
}

const std::vector<MapState>& knownMapStates() {
    static const std::vector<MapState> states{MapState::Generated, MapState::Blockout,
                                              MapState::Retouched, MapState::Finished};
    return states;
}

std::filesystem::path sidecarPath(const std::filesystem::path& levelPath) {
    std::filesystem::path path = levelPath;
    path.replace_extension();
    path += std::string{SIDECAR_SUFFIX};
    return path;
}

bool isSidecarFile(const std::filesystem::path& path) {
    return path.filename().string().ends_with(SIDECAR_SUFFIX);
}

SidecarReadResult parseSidecar(std::string_view json) {
    SidecarReadResult result;
    const nlohmann::json root = nlohmann::json::parse(json, nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        result.warning = "The editor file is not a JSON object; it was ignored.";
        return result;
    }
    for (const auto& [key, value] : root.items()) {
        if (key == "version") {
            continue;
        }
        if (key == "state") {
            // Un mot inconnu vaut « rien dit » : l'annexe d'un editeur plus recent ne fait pas
            // echouer celui-ci, mais son mot n'est pas garde -- l'etat est une valeur, pas un
            // texte libre.
            result.sidecar.state =
                value.is_string() ? mapStateFromKey(value.get<std::string>()) : MapState::Unset;
            continue;
        }
        if (key != "notes") {
            result.sidecar.unknown[key] = value;
            continue;
        }
        if (!value.is_array()) {
            result.warning = "\"notes\" is not a list; the notes were ignored.";
            continue;
        }
        for (const nlohmann::json& note : value) {
            const bool wellFormed = note.is_object() && note.contains("column") &&
                                    note["column"].is_number_integer() && note.contains("row") &&
                                    note["row"].is_number_integer() && note.contains("text") &&
                                    note["text"].is_string();
            if (!wellFormed) {
                result.warning = "A malformed note was ignored.";
                continue;
            }
            setNote(result.sidecar,
                    core::GridPosition{.column = note["column"].get<int>(),
                                       .row = note["row"].get<int>()},
                    note["text"].get<std::string>());
        }
    }
    return result;
}

SidecarReadResult readSidecar(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    const std::string text{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    return parseSidecar(text);
}

std::string sidecarJson(const EditorSidecar& sidecar) {
    // nlohmann::json trie les clés d'un objet : l'écriture est canonique, un diff git reste
    // lisible.
    nlohmann::json root = sidecar.unknown.is_object() ? sidecar.unknown : nlohmann::json::object();
    root["version"] = EditorSidecar::FORMAT_VERSION;
    nlohmann::json notes = nlohmann::json::array();
    for (const AuthorNote& note : sidecar.notes) {
        notes.push_back(
            {{"column", note.cell.column}, {"row", note.cell.row}, {"text", note.text}});
    }
    root["notes"] = std::move(notes);
    if (sidecar.state != MapState::Unset) {
        root["state"] = std::string{mapStateKey(sidecar.state)};
    } else {
        root.erase("state");
    }
    return root.dump(2) + "\n";
}

bool writeSidecar(const std::filesystem::path& path, const EditorSidecar& sidecar) {
    std::error_code error;
    if (sidecar.empty()) {
        std::filesystem::remove(path, error);
        return !error;
    }
    std::filesystem::create_directories(path.parent_path(), error);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return false;
    }
    const std::string text = sidecarJson(sidecar);
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    return file.good();
}

const AuthorNote* noteAt(const EditorSidecar& sidecar, core::GridPosition cell) {
    const auto found = std::ranges::find(sidecar.notes, cell, &AuthorNote::cell);
    return found != sidecar.notes.end() ? &*found : nullptr;
}

bool setNote(EditorSidecar& sidecar, core::GridPosition cell, std::string text) {
    const auto found = std::ranges::find(sidecar.notes, cell, &AuthorNote::cell);
    if (isBlank(text)) {
        if (found == sidecar.notes.end()) {
            return false;
        }
        sidecar.notes.erase(found);
        return true;
    }
    if (found != sidecar.notes.end()) {
        if (found->text == text) {
            return false;
        }
        found->text = std::move(text);
        return true;
    }
    sidecar.notes.push_back(AuthorNote{.cell = cell, .text = std::move(text)});
    std::ranges::sort(sidecar.notes, byRow);
    return true;
}

}  // namespace hmi
