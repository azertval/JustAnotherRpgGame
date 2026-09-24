// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QtQmlIntegration>

#include "HMI/Presentation/QuestJournalScreen.h"
#include "HMI/Runtime/SheetRowModel.h"

/**
 * @file HMI/Runtime/QuestJournalModel.h
 * @brief La vue-modèle du journal de quêtes (`LOT-116`) : les quêtes de la partie, lues dans ses
 *        drapeaux, et la quête choisie au clavier.
 */

namespace hmi {

/**
 * @brief Ce que l'écran « Journal de quêtes » lit.
 *
 * Il ne garde **aucun** état de quête : tout se relit dans les drapeaux de la partie
 * (`hmi::WorldModel::current`) par `hmi::questJournalValues`, à l'ouverture et à chaque étape
 * atteinte. Sans partie (le designer), le journal est vide et le dit.
 */
class QuestJournalModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    /// Les quêtes commencées : rôles `rowId`, `label` (le titre), `value` (l'état).
    Q_PROPERTY(QAbstractItemModel* quests READ quests CONSTANT)
    /// L'entrée la plus récente de la quête choisie, ou « aucune quête ».
    Q_PROPERTY(QString detail READ detail NOTIFY changed)
    /// Les étapes atteintes de la quête choisie.
    Q_PROPERTY(QAbstractItemModel* objectives READ objectives CONSTANT)
    /// L'identifiant de la quête choisie, vide si le journal est vide.
    Q_PROPERTY(QString selected READ selected NOTIFY changed)

public:
    explicit QuestJournalModel(QObject* parent = nullptr);

    [[nodiscard]] QAbstractItemModel* quests() {
        return &_quests;
    }
    [[nodiscard]] QAbstractItemModel* objectives() {
        return &_objectives;
    }
    [[nodiscard]] QString detail() const {
        return QString::fromStdString(_values.detail);
    }
    [[nodiscard]] QString selected() const {
        return QString::fromStdString(_values.selected);
    }

    /// @brief Choisit la quête @p questId.
    Q_INVOKABLE void select(const QString& questId);
    /// @brief Choisit la quête suivante (@p step = 1) ou précédente (-1) : les flèches.
    Q_INVOKABLE void selectNeighbour(int step);
    /// @brief Relit le journal dans les drapeaux de la partie.
    Q_INVOKABLE void refresh();

signals:
    void changed();

private:
    QuestJournalValues _values;
    SheetRowModel _quests;
    SheetRowModel _objectives;
};

}  // namespace hmi
