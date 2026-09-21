// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Game/LaunchOptions.h"

#include <algorithm>
#include <charconv>
#include <string>

namespace hmi {

namespace {

/// Decoupe @p value sur @p separator ; les morceaux vides sont ecartes.
[[nodiscard]] std::vector<std::string_view> decouper(std::string_view value, char separator) {
    std::vector<std::string_view> morceaux;
    std::size_t debut = 0;
    while (debut <= value.size()) {
        const std::size_t fin = value.find(separator, debut);
        const std::string_view morceau = value.substr(
            debut, fin == std::string_view::npos ? std::string_view::npos : fin - debut);
        if (!morceau.empty()) {
            morceaux.push_back(morceau);
        }
        if (fin == std::string_view::npos) {
            break;
        }
        debut = fin + 1;
    }
    return morceaux;
}

/// @return L'entier de @p text, s'il est ecrit en entier et sans rien d'autre.
[[nodiscard]] std::optional<int> entier(std::string_view text) {
    int valeur = 0;
    const char* const fin = text.data() + text.size();
    const std::from_chars_result lu = std::from_chars(text.data(), fin, valeur);
    if (lu.ec != std::errc{} || lu.ptr != fin) {
        return std::nullopt;
    }
    return valeur;
}

}  // namespace

std::vector<std::string> gameLaunchArguments(const GameLaunchOptions& options) {
    std::vector<std::string> arguments;
    std::string carte = "--map=" + options.mapId;
    if (!options.arrival.empty()) {
        carte += '@' + options.arrival;
    }
    arguments.push_back(std::move(carte));
    if (options.cell) {
        arguments.push_back("--at=" + std::to_string(options.cell->column) + LAUNCH_LIST_SEPARATOR +
                            std::to_string(options.cell->row));
    }
    if (!options.flags.empty()) {
        std::string drapeaux = "--flags=";
        for (std::size_t index = 0; index < options.flags.size(); ++index) {
            if (index > 0) {
                drapeaux += LAUNCH_LIST_SEPARATOR;
            }
            drapeaux += options.flags[index];
        }
        arguments.push_back(std::move(drapeaux));
    }
    if (!options.levelDirectories.empty()) {
        std::string dossiers = "--levels=";
        for (std::size_t index = 0; index < options.levelDirectories.size(); ++index) {
            if (index > 0) {
                dossiers += LAUNCH_PATH_SEPARATOR;
            }
            dossiers += options.levelDirectories[index].string();
        }
        arguments.push_back(std::move(dossiers));
    }
    return arguments;
}

std::optional<core::GridPosition> parseStartCell(std::string_view value) {
    const std::vector<std::string_view> morceaux = decouper(value, LAUNCH_LIST_SEPARATOR);
    if (morceaux.size() != 2) {
        return std::nullopt;
    }
    const std::optional<int> colonne = entier(morceaux[0]);
    const std::optional<int> ligne = entier(morceaux[1]);
    if (!colonne || !ligne || *colonne < 0 || *ligne < 0) {
        return std::nullopt;
    }
    return core::GridPosition{.column = *colonne, .row = *ligne};
}

std::vector<std::string> parseWorldFlags(std::string_view value) {
    std::vector<std::string> drapeaux;
    for (const std::string_view morceau : decouper(value, LAUNCH_LIST_SEPARATOR)) {
        std::string drapeau{morceau};
        // Deux fois le meme drapeau ne veut rien dire de plus qu'une : `core::WorldFlags` est un
        // ensemble, et le relever ici garde la liste lisible dans le journal.
        if (std::ranges::find(drapeaux, drapeau) == drapeaux.end()) {
            drapeaux.push_back(std::move(drapeau));
        }
    }
    return drapeaux;
}

std::vector<std::filesystem::path> parseLevelDirectories(std::string_view value) {
    std::vector<std::filesystem::path> dossiers;
    for (const std::string_view morceau : decouper(value, LAUNCH_PATH_SEPARATOR)) {
        dossiers.emplace_back(morceau);
    }
    return dossiers;
}

}  // namespace hmi
