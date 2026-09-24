// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file HMI/Game/GameQuests.h
 * @brief Les quêtes de la partie, lues et validées au démarrage avec les dialogues qui en posent
 *        les drapeaux (`LOT-116`).
 */

#include <filesystem>
#include <string>
#include <vector>

#include "Core/Gameplay/Quest.h"

namespace hmi {

/// @brief Les quêtes acceptées, et tout ce qui a été refusé ou relevé.
struct GameQuests {
    core::QuestCatalog catalog;
    /// Les quêtes refusées (`fichier:ligne : …`), puis les usages de drapeaux que dialogues et
    /// quêtes font à tort (`core::validateFlagUses`).
    std::vector<std::string> errors;
};

/**
 * @brief Lit `World/quests` sous @p root et confronte les quêtes aux dialogues de
 *        `World/dialogues`.
 *
 * Un usage fautif ne retire rien : la partie reste jouable (`EX-NFR-040`), le journal dit ce qui
 * ne va pas, et le test des données livrées exige qu'il n'y ait rien à dire.
 *
 * @param root La racine des éléments déployés (le dossier de l'exécutable, ou une racine d'essai).
 */
[[nodiscard]] GameQuests loadGameQuests(const std::filesystem::path& root);

}  // namespace hmi
