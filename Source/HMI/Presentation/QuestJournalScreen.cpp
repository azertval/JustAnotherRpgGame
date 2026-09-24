// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Presentation/QuestJournalScreen.h"

#include <algorithm>

#include "Core/Gameplay/WorldFlags.h"

namespace hmi {

namespace {

/// La marque de la quête choisie : le formulaire n'a pas de sélection à lui, la ligne la porte.
constexpr std::string_view MARQUE_CHOISIE = "› ";
constexpr std::string_view FRANCHIE = "✓";

}  // namespace

std::string questStatusKey(core::QuestStatus status) {
    switch (status) {
        case core::QuestStatus::Succeeded:
            return "journal.status.succeeded";
        case core::QuestStatus::Failed:
            return "journal.status.failed";
        case core::QuestStatus::NotStarted:
        case core::QuestStatus::Active:
            break;
    }
    return "journal.status.active";
}

QuestJournalValues questJournalValues(const core::QuestCatalog& catalog,
                                      const core::WorldFlags& flags, std::string_view selected,
                                      const TextLookup& text) {
    QuestJournalValues values;
    const core::Quest* choisie = nullptr;
    core::QuestProgress avancementChoisi;
    for (const core::Quest& quete : catalog.quests) {
        const core::QuestProgress avancement = core::questProgress(quete, flags);
        if (avancement.status == core::QuestStatus::NotStarted) {
            continue;
        }
        if (choisie == nullptr || quete.id == selected) {
            choisie = &quete;
            avancementChoisi = avancement;
        }
        values.quests.push_back({.id = quete.id,
                                 .label = text(core::questTitleKey(quete.id)),
                                 .value = text(questStatusKey(avancement.status))});
    }
    if (choisie == nullptr) {
        values.detail = text("journal.empty");
        return values;
    }
    values.selected = choisie->id;
    for (QuestJournalRow& ligne : values.quests) {
        if (ligne.id == values.selected) {
            ligne.label.insert(0, MARQUE_CHOISIE);
        }
    }
    const std::vector<std::string>& etapes = avancementChoisi.reachedSteps;
    values.detail = text(core::questStepKey(choisie->id, etapes.back()));
    for (std::size_t i = 0; i < etapes.size(); ++i) {
        const bool derniere = i + 1 == etapes.size();
        std::string valeur{FRANCHIE};
        if (derniere) {
            valeur = avancementChoisi.status == core::QuestStatus::Active
                         ? std::string()
                         : text(questStatusKey(avancementChoisi.status));
        }
        values.objectives.push_back({.id = etapes[i],
                                     .label = text(core::questStepKey(choisie->id, etapes[i])),
                                     .value = std::move(valeur)});
    }
    return values;
}

std::string neighbourQuest(const QuestJournalValues& values, int step) {
    if (values.quests.empty()) {
        return {};
    }
    const auto ici = std::ranges::find(values.quests, values.selected, &QuestJournalRow::id);
    const auto rang =
        static_cast<int>(ici == values.quests.end() ? 0 : ici - values.quests.begin());
    const int dernier = static_cast<int>(values.quests.size()) - 1;
    return values.quests[static_cast<std::size_t>(std::clamp(rang + step, 0, dernier))].id;
}

}  // namespace hmi
