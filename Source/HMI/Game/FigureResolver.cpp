// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Game/FigureResolver.h"

#include <system_error>
#include <utility>

#include "HMI/Graphics/WorldSceneComposer.h"

namespace hmi {

FigureResolver::FigureResolver(std::filesystem::path assetsDirectory)
    : _assetsDirectory(std::move(assetsDirectory)) {}

bool FigureResolver::hasIdleStrip(std::string_view directory, bool& oriented) const {
    if (directory.empty()) {
        return false;
    }
    // Une figurine est orientee si sa bande de repos vers le sud-est existe : c'est la premiere
    // que l'atelier produit, et `check_hd_assets.py` exige les quatre des qu'il y en a une.
    std::error_code erreur;
    if (std::filesystem::is_regular_file(
            _assetsDirectory /
                figureStripPath(directory, figure_clips::IDLE, FigureFacing::SouthEast),
            erreur)) {
        oriented = true;
        return true;
    }
    if (std::filesystem::is_regular_file(
            _assetsDirectory / figureStripPath(directory, figure_clips::IDLE), erreur)) {
        oriented = false;
        return true;
    }
    return false;
}

const ResolvedFigure& FigureResolver::resolve(std::string_view figure, std::string_view silhouette,
                                              const PlaceAppearance& appearance) {
    const std::string key = std::string{figure} + "|" + std::string{silhouette};
    if (const auto found = _found.find(key); found != _found.end()) {
        return found->second;
    }
    ResolvedFigure resolved;
    // 1. La figurine nommee, par la table du lieu.
    const std::string named = appearance.figureDirectory(figure);
    bool oriented = false;
    if (hasIdleStrip(named, oriented)) {
        resolved = ResolvedFigure{.directory = named, .oriented = oriented, .placeholder = false};
        return _found.emplace(key, std::move(resolved)).first->second;
    }
    // 2. Le mannequin de sa silhouette ; 3. l'humanoide.
    for (const std::string_view candidate : {silhouette, DEFAULT_SILHOUETTE}) {
        const std::string placeholder = placeholderFigureDirectory(candidate);
        if (hasIdleStrip(placeholder, oriented)) {
            resolved =
                ResolvedFigure{.directory = placeholder, .oriented = oriented, .placeholder = true};
            return _found.emplace(key, std::move(resolved)).first->second;
        }
    }
    // Rien d'installe : la figurine nommee telle quelle, et son marqueur au rendu.
    resolved = ResolvedFigure{.directory = named, .oriented = false, .placeholder = false};
    return _found.emplace(key, std::move(resolved)).first->second;
}

}  // namespace hmi
