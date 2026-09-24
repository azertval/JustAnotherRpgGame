// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/QuestJournalModel.h"

#include <QVector>

#include "Core/Gameplay/WorldFlags.h"
#include "HMI/Runtime/RuleLabels.h"
#include "HMI/Runtime/WorldModel.h"

namespace hmi {

namespace {

[[nodiscard]] QVector<SheetRow> lignes(const std::vector<QuestJournalRow>& rows) {
    QVector<SheetRow> resultat;
    resultat.reserve(static_cast<qsizetype>(rows.size()));
    for (const QuestJournalRow& ligne : rows) {
        resultat.push_back({.id = QString::fromStdString(ligne.id),
                            .label = QString::fromStdString(ligne.label),
                            .value = QString::fromStdString(ligne.value)});
    }
    return resultat;
}

}  // namespace

QuestJournalModel::QuestJournalModel(QObject* parent) : QObject(parent) {
    if (WorldModel* const partie = WorldModel::current()) {
        connect(partie, &WorldModel::questAdvanced, this, &QuestJournalModel::refresh);
    }
    refresh();
}

void QuestJournalModel::select(const QString& questId) {
    _values.selected = questId.toStdString();
    refresh();
}

void QuestJournalModel::selectNeighbour(int step) {
    _values.selected = neighbourQuest(_values, step);
    refresh();
}

void QuestJournalModel::refresh() {
    const TextLookup texte = [](std::string_view key) { return ruleLabel(key, activeLanguage()); };
    if (WorldModel* const partie = WorldModel::current()) {
        _values = questJournalValues(partie->quests(), partie->flags(), _values.selected, texte);
    } else {
        _values = questJournalValues(core::QuestCatalog{}, core::WorldFlags{}, {}, texte);
    }
    _quests.setRows(lignes(_values.quests));
    _objectives.setRows(lignes(_values.objectives));
    emit changed();
}

}  // namespace hmi
