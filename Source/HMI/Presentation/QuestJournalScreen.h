// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Core/Gameplay/Quest.h"
#include "HMI/Presentation/DialogueScreen.h"

/**
 * @file HMI/Presentation/QuestJournalScreen.h
 * @brief Ce que le journal de quêtes montre, tiré des quêtes et des drapeaux de la partie, tout
 *        traduit (`LOT-116`, `EX-EXP-010`). Aucune dépendance à Qt : le test le vérifie sans
 *        fenêtre.
 */

namespace hmi {

/// @brief Une ligne du journal : une quête, ou une étape de la quête choisie.
struct QuestJournalRow {
    std::string id;
    std::string label;
    std::string value;

    [[nodiscard]] bool operator==(const QuestJournalRow&) const = default;
};

/// @brief L'écran du journal, tout traduit.
struct QuestJournalValues {
    /// Les quêtes commencées, dans l'ordre du catalogue ; la choisie porte la marque `›`.
    std::vector<QuestJournalRow> quests;
    /// L'identifiant de la quête choisie, vide si le journal est vide.
    std::string selected;
    /// L'entrée de l'étape la plus récente de la quête choisie — ou « aucune quête ».
    std::string detail;
    /// Les étapes atteintes de la quête choisie : `✓` pour les franchies, l'issue pour la dernière.
    std::vector<QuestJournalRow> objectives;
};

/// @brief `journal.status.<active|succeeded|failed>`.
[[nodiscard]] std::string questStatusKey(core::QuestStatus status);

/**
 * @brief Le journal tel que les drapeaux le disent.
 *
 * @param catalog  Les quêtes de la partie.
 * @param flags    Les drapeaux de la partie.
 * @param selected La quête que le joueur a choisie ; absente du journal, la première.
 * @param text     La traduction des clés.
 */
[[nodiscard]] QuestJournalValues questJournalValues(const core::QuestCatalog& catalog,
                                                    const core::WorldFlags& flags,
                                                    std::string_view selected,
                                                    const TextLookup& text);

/**
 * @brief La quête voisine de @p selected dans le journal : la suivante (@p step = 1) ou la
 *        précédente (-1), sans sortir de la liste.
 */
[[nodiscard]] std::string neighbourQuest(const QuestJournalValues& values, int step);

}  // namespace hmi
