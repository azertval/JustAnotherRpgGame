// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/LevelFileOperations.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <system_error>
#include <utility>
#include <vector>

#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelWriter.h"
#include "Core/World/WorldGraph.h"
#include "Editor/Logic/EditorSidecar.h"
#include "Editor/Logic/LevelNameValidation.h"
#include "Editor/Logic/MapRefactor.h"
#include "Editor/Logic/MapTexts.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace hmi {

namespace {

// Charge un niveau, en fait un brouillon renommé, valide, écrit à `target`. Facteur commun à
// rename/duplicate. Renvoie un FileOperationResult (jamais d'exception) ; @p previousName reçoit le
// nom qu'avait la carte.
[[nodiscard]] FileOperationResult writeRenamed(const std::filesystem::path& source,
                                               const std::string& newName,
                                               const std::filesystem::path& target,
                                               std::string& previousName) {
    core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(source);
    if (!loaded.ok()) {
        return FileOperationResult::failure("Niveau source illisible : " + loaded.error);
    }
    previousName = loaded.level->name();
    core::LevelDraft draft = core::LevelDraft::fromLevel(*loaded.level);
    draft.setName(newName);
    core::LevelLoadResult validated = draft.toLevel();
    if (!validated.ok()) {
        return FileOperationResult::failure("Niveau invalide : " + validated.error);
    }
    if (!core::LevelWriter::saveToFile(*validated.level, target)) {
        return FileOperationResult::failure("Échec de l'écriture du fichier.");
    }
    return FileOperationResult::success(target);
}

}  // namespace

LevelFileOperations::LevelFileOperations(std::filesystem::path levelsDir,
                                         std::filesystem::path levelsRoot)
    : _dir(std::move(levelsDir)), _root(levelsRoot.empty() ? _dir : std::move(levelsRoot)) {}

std::string LevelFileOperations::nameKeyFor(const std::filesystem::path& file) const {
    return mapNameKey(core::mapIdOf(_root, file));
}

void LevelFileOperations::addNameTranslation(const std::filesystem::path& file,
                                             const std::string& text,
                                             const std::string& copyFrom) const {
    // Un catalogue qui ne s'écrit pas n'empêche pas la carte d'exister : le contrôle le dira.
    static_cast<void>(addTranslation(localizationDirectory(_root.parent_path()), nameKeyFor(file),
                                     text, copyFrom));
}

std::filesystem::path LevelFileOperations::pathForName(const std::string& name) const {
    return _dir / (name + ".json");
}

std::vector<std::filesystem::path> LevelFileOperations::list() const {
    std::vector<std::filesystem::path> levels;
    std::error_code error;
    if (!std::filesystem::is_directory(_dir, error)) {
        return levels;  // dossier absent : liste vide (robustesse).
    }
    // Récursif : les quartiers de la Capitale vivent dans `capital/` (`LOT-96`).
    for (auto it = std::filesystem::recursive_directory_iterator(_dir, error);
         !error && it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
        const std::filesystem::directory_entry& entry = *it;
        // "sequence-" reste un préfixe réservé : un tel fichier n'est pas une carte, et
        // l'afficher comme ouvrable dans ce panneau mènerait à une ouverture en échec.
        // Pas plus l'annexe d'une carte (`<carte>.editor.json`, LOT-EDITOR-04), que le jeu ne lit
        // jamais.
        if (entry.is_regular_file(error) && entry.path().extension() == ".json" &&
            !entry.path().filename().string().starts_with("sequence-") &&
            !hmi::isSidecarFile(entry.path())) {
            levels.push_back(entry.path());
        }
    }
    std::ranges::sort(levels, [this](const auto& lhs, const auto& rhs) {
        return core::mapIdOf(_dir, lhs) < core::mapIdOf(_dir, rhs);
    });
    return levels;
}

FileOperationResult LevelFileOperations::create(const std::string& name, int width, int height,
                                                const std::string& place) const {
    const std::string trimmed = hmi::trimLevelName(name);
    if (!hmi::isValidLevelName(name)) {
        return FileOperationResult::failure("Nom de niveau invalide.");
    }
    if (width < 1 || height < 1) {
        return FileOperationResult::failure("Dimensions trop petites (une case au minimum).");
    }
    const std::filesystem::path target = pathForName(trimmed);
    std::error_code error;
    if (std::filesystem::exists(target, error)) {
        return FileOperationResult::failure("Un niveau porte déjà ce nom.");
    }
    // Niveau minimal valide : grille vide + une entrée (coin bas gauche).
    core::LevelDraft draft = core::LevelDraft::empty(nameKeyFor(target), width, height);
    if (!place.empty()) {
        // Les couches des cartes livrées. Le sol nomme le lieu ; la collision est celle que la
        // déduction donne à une carte vide : le vide arrête la vue (décision D10).
        const std::optional<std::size_t> ground = draft.addLayer(core::LayerKind::Ground, "sol");
        draft.setLayerProperty(*ground, std::string{SCENE_PLACE_PROPERTY}, place);
        draft.addLayer(core::LayerKind::Decor, "relief");
        draft.paintRegion(0, 0,
                          std::vector<std::vector<core::TileType>>(
                              static_cast<std::size_t>(height),
                              std::vector<core::TileType>(static_cast<std::size_t>(width),
                                                          core::TileType::Wall)));
        // L'entrée ne se tient pas dans le vide : sa case reçoit un sol de terre, que la table du
        // lieu traduit en pièce.
        draft.paintLayerTile(*ground, 0, height - 1, core::TileType::Dirt);
    }
    draft.setEntry(0, height - 1);
    core::LevelLoadResult validated = draft.toLevel();
    if (!validated.ok()) {
        return FileOperationResult::failure("Niveau invalide : " + validated.error);
    }
    if (!core::LevelWriter::saveToFile(*validated.level, target)) {
        return FileOperationResult::failure("Échec de l'écriture du fichier.");
    }
    addNameTranslation(target, trimmed);
    return FileOperationResult::success(target);
}

FileOperationResult LevelFileOperations::rename(const std::filesystem::path& source,
                                                const std::string& newName) const {
    std::error_code error;
    if (!std::filesystem::exists(source, error)) {
        return FileOperationResult::failure("Niveau introuvable.");
    }
    const std::string trimmed = hmi::trimLevelName(newName);
    if (!hmi::isValidLevelName(newName)) {
        return FileOperationResult::failure("Nom de niveau invalide.");
    }
    const std::filesystem::path target = source.parent_path() / (trimmed + ".json");
    const std::string oldId = core::mapIdOf(_root, source);
    const std::string newId = core::mapIdOf(_root, target);
    if (oldId == newId) {
        return FileOperationResult::success(source);
    }
    // Le renommage propagé (LOT-EDITOR-14) : portails, villes, clé du nom, annexe.
    const RefactorPlan plan = planRenameMap(_root.parent_path(), oldId, newId);
    if (!plan.ok()) {
        return FileOperationResult::failure(plan.error);
    }
    std::string written;
    if (!applyRefactorPlan(plan, written)) {
        return FileOperationResult::failure(written);
    }
    // Un catalogue qui n'avait pas l'ancienne clé reçoit la nouvelle, le nom tapé pour texte.
    addNameTranslation(target, trimmed);
    return FileOperationResult::success(target);
}

FileOperationResult LevelFileOperations::duplicate(const std::filesystem::path& source) const {
    std::error_code error;
    if (!std::filesystem::exists(source, error)) {
        return FileOperationResult::failure("Niveau introuvable.");
    }
    const std::string base = source.stem().string();
    // Cherche un nom de copie disponible : « base (copie) », « base (copie 2) », …
    std::string candidate = base + " (copie)";
    for (int index = 2; std::filesystem::exists(pathForName(candidate), error); ++index) {
        candidate = base + " (copie " + std::to_string(index) + ")";
    }
    std::string previousName;
    const FileOperationResult written = writeRenamed(source, nameKeyFor(pathForName(candidate)),
                                                     pathForName(candidate), previousName);
    if (written.ok()) {
        addNameTranslation(pathForName(candidate), candidate, previousName);
    }
    if (written.ok() && std::filesystem::exists(sidecarPath(source), error)) {
        std::filesystem::copy_file(sidecarPath(source), sidecarPath(pathForName(candidate)),
                                   error);  // la copie garde les notes, échec non bloquant.
    }
    return written;
}

FileOperationResult LevelFileOperations::remove(const std::filesystem::path& source) {
    std::error_code error;
    if (!std::filesystem::exists(source, error)) {
        return FileOperationResult::failure("Niveau introuvable.");
    }
    if (!std::filesystem::remove(source, error)) {
        return FileOperationResult::failure("Échec de la suppression.");
    }
    std::filesystem::remove(sidecarPath(source), error);  // ses notes partent avec elle.
    return FileOperationResult::success(source);
}

}  // namespace hmi
