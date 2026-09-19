// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QString>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "Editor/Logic/MapRefactor.h"

class QWidget;

/**
 * @file Editor/Ui/RefactorDialogs.h
 * @brief Les dialogues de « renommer et remplacer » (`LOT-EDITOR-14`) : qui cite ceci, ce qu'un
 *        renommage va récrire, quelle pièce remplace laquelle, quelle planche prendre.
 *
 * Des dialogues seulement : ce qu'ils décident, la fenêtre le fait par `hmi::planRenameMap` et
 * ses voisins, ou par le brouillon (`core::LevelDraft::replacePieces`, `changeScene`).
 */

namespace hmi {

/**
 * @brief Montre les citations de @p citations.
 * @return Celle qu'on double-clique — la fenêtre y va —, `std::nullopt` si on ferme.
 */
[[nodiscard]] std::optional<Citation> showCitations(QWidget* parent, const QString& title,
                                                    const std::vector<Citation>& citations,
                                                    const std::filesystem::path& dataRoot);

/// @return Vrai si l'auteur accepte ce que @p plan va récrire, qu'on lui montre.
[[nodiscard]] bool confirmPlan(QWidget* parent, const QString& title, const RefactorPlan& plan,
                               const std::filesystem::path& dataRoot);

/// @brief Ce que « Replace piece » a choisi.
struct PieceReplacementChoice {
    std::string from;
    std::string to;
    /// Sur toutes les cartes qui posent @p from ; sinon sur la carte ouverte, en un pas.
    bool allMaps = false;
};

/**
 * @brief Demande quelle pièce remplacer, et par laquelle.
 * @param cited Les pièces que la carte ouverte pose.
 * @param sheet La planche de la carte : on ne propose que des pièces de même classe (sol, ou
 *              pièce debout).
 */
[[nodiscard]] std::optional<PieceReplacementChoice> askPieceReplacement(
    QWidget* parent, const std::vector<std::string>& cited, const core::ScenePieceManifest& sheet);

/// @brief Ce que « Change sheet » a choisi : le lieu, et la table de correspondance.
struct SceneChangeChoice {
    std::string place;
    core::PieceRenaming table;
};

/**
 * @brief Demande la nouvelle planche, et pour chaque pièce que la carte pose, celle qui la
 *        remplace : la table proposée (`hmi::proposedPieceTable`) d'abord, qu'on corrige. On ne
 *        valide qu'une table sans trou.
 */
[[nodiscard]] std::optional<SceneChangeChoice> askSceneChange(
    QWidget* parent, const std::vector<core::TileLayer>& layers,
    const std::filesystem::path& dataRoot, const std::string& currentPlace);

}  // namespace hmi
