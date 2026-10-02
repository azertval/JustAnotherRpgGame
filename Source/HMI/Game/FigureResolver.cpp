// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Game/FigureResolver.h"

#include <system_error>
#include <utility>

#include "HMI/Graphics/WorldSceneComposer.h"

namespace hmi {

FigureResolver::FigureResolver(std::filesystem::path assetsDirectory)
    : _assetsDirectory(std::move(assetsDirectory)) {}

bool FigureResolver::hasModel(std::string_view directory, ResolvedFigure& resolved) {
    if (directory.empty()) {
        return false;
    }
    const std::filesystem::path folder = _assetsDirectory / std::filesystem::path(directory);
    std::error_code erreur;
    if (!std::filesystem::is_regular_file(folder / core::CHARACTER_SHEET_FILE, erreur)) {
        return false;
    }
    const core::CharacterSheetFileResult sheet =
        core::readCharacterSheetFile(folder / core::CHARACTER_SHEET_FILE);
    if (!sheet.ok() || !std::filesystem::is_regular_file(folder / sheet.sheet.model, erreur)) {
        return false;
    }
    auto known = _skeletons.find(sheet.sheet.skeleton);
    if (known == _skeletons.end()) {
        core::SkeletonFileResult skeleton =
            core::readSkeletonFile(_assetsDirectory / core::skeletonFilePath(sheet.sheet.skeleton));
        known = _skeletons
                    .emplace(sheet.sheet.skeleton,
                             skeleton.ok() ? std::make_shared<const core::SkeletonDescription>(
                                                 std::move(skeleton.skeleton))
                                           : nullptr)
                    .first;
    }
    resolved.model = std::string{directory} + "/" + sheet.sheet.model;
    resolved.skeleton = known->second;
    return true;
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
    if (hasModel(named, resolved)) {
        resolved.directory = named;
        return _found.emplace(key, std::move(resolved)).first->second;
    }
    // 2. Le mannequin de sa silhouette ; 3. l'humanoide.
    for (const std::string_view candidate : {silhouette, DEFAULT_SILHOUETTE}) {
        if (const std::string mannequin = mannequinFigureDirectory(candidate);
            hasModel(mannequin, resolved)) {
            resolved.directory = mannequin;
            resolved.placeholder = true;
            return _found.emplace(key, std::move(resolved)).first->second;
        }
    }
    // Rien d'installe : la figurine nommee telle quelle, et son marqueur au rendu.
    resolved = ResolvedFigure{.directory = named, .placeholder = false, .model = {}, .skeleton = {}};
    return _found.emplace(key, std::move(resolved)).first->second;
}

}  // namespace hmi
