// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QString>
#include <optional>
#include <string>
#include <vector>

#include "Core/Gameplay/Quest.h"
#include "Core/Levels/GridPosition.h"

/**
 * @file Editor/Ui/RunInGameDialog.h
 * @brief Le dialogue **« Run in game… »** (`LOT-EDITOR-10`, `EX-EDIT-094`) : d'où l'on part, et
 *        avec quels drapeaux de monde.
 *
 * Lancer le jeu ne demande rien de tout cela — *Map* › *Run in game* part de l'entrée de la carte,
 * ou de la case survolée. Ce dialogue sert le second essai : la même carte **après** une quête.
 */

class QWidget;

namespace hmi {

/// @brief Ce que l'auteur a choisi pour l'essai.
struct RunInGameChoice {
    /// La case de départ, absente pour l'entrée de la carte (ou son point d'arrivée).
    std::optional<core::GridPosition> cell;
    /// Les drapeaux de monde posés avant le premier pas, dans l'ordre de la liste.
    std::vector<std::string> flags;

    [[nodiscard]] bool operator==(const RunInGameChoice&) const = default;
};

/**
 * @brief Demande où partir et quels drapeaux poser, puis rend le choix.
 *
 * @param parent     Fenêtre parente.
 * @param mapId      L'identifiant de la carte qui sera jouée, affiché.
 * @param knownFlags Les drapeaux que posent dialogues, quêtes et zones
 *                   (`EditorReferences::flags`) : la liste à cocher. Un drapeau hors liste se
 *                   saisit à la main — une quête peut précéder son dialogue.
 * @param declared   Les drapeaux à valeurs des quêtes : une valeur chacun (`LOT-126`).
 * @param bounds     La taille de la carte : la case se borne à elle.
 * @param current    Le choix de départ, celui du dernier essai.
 * @return Le choix, ou `std::nullopt` si l'auteur a renoncé.
 */
[[nodiscard]] std::optional<RunInGameChoice> askRunInGame(
    QWidget* parent, const QString& mapId, const std::vector<std::string>& knownFlags,
    const std::vector<core::QuestFlag>& declared, core::GridPosition bounds,
    const RunInGameChoice& current);

}  // namespace hmi
