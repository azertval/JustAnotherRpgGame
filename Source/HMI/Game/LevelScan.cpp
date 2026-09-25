// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Game/LevelScan.h"

#include <algorithm>
#include <string_view>
#include <system_error>

namespace hmi {

namespace {

constexpr std::string_view EXTENSION = ".json";
// Les notes de l'éditeur, posées à côté de la carte (`LOT-EDITOR-04`) : pas une carte.
constexpr std::string_view EDITOR_SIDECAR = ".editor.json";

[[nodiscard]] bool finitPar(std::string_view texte, std::string_view suffixe) {
    return texte.size() >= suffixe.size() &&
           texte.compare(texte.size() - suffixe.size(), suffixe.size(), suffixe) == 0;
}

// Rend : L'identifiant de `file` sous `directory` : le chemin relatif, `/` partout, sans
//         `.json`.
[[nodiscard]] std::string identifiant(const std::filesystem::path& file,
                                      const std::filesystem::path& directory) {
    std::string relatif = file.lexically_relative(directory).generic_string();
    relatif.resize(relatif.size() - EXTENSION.size());
    return relatif;
}

}  // namespace

std::vector<LevelEntry> scanLevelDirectories(
    const std::vector<std::filesystem::path>& directories) {
    std::vector<LevelEntry> cartes;
    for (const std::filesystem::path& dossier : directories) {
        std::error_code erreur;
        for (auto it = std::filesystem::recursive_directory_iterator(dossier, erreur);
             !erreur && it != std::filesystem::recursive_directory_iterator();
             it.increment(erreur)) {
            if (!it->is_regular_file(erreur)) {
                continue;
            }
            const std::string nom = it->path().filename().generic_string();
            if (!finitPar(nom, EXTENSION) || finitPar(nom, EDITOR_SIDECAR)) {
                continue;
            }
            std::string id = identifiant(it->path(), dossier);
            // Le premier dossier l'emporte : la carte du brouillon cache celle du binaire, comme
            // au chargement.
            const bool dejaVue = std::ranges::any_of(
                cartes, [&id](const LevelEntry& carte) { return carte.mapId == id; });
            if (dejaVue) {
                continue;
            }
            cartes.push_back(
                LevelEntry{.mapId = std::move(id), .file = it->path(), .directory = dossier});
        }
    }
    std::ranges::sort(cartes, {}, &LevelEntry::mapId);
    return cartes;
}

}  // namespace hmi
