// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/PendingData.h"

#include <QVector>
#include <algorithm>
#include <utility>

namespace hmi {
namespace {

// Le même signe que le châssis pose sur un champ sans source (`hmi::CharacterSheetModel`).
constexpr const char* EMPTY_MARK = "—";

// Garde-fou de bon sens : une liste d'attente ne décrit jamais mille lignes, et une valeur folle
// venue d'une liaison QML mal écrite ne doit pas allouer sans fin.
constexpr int MAXIMUM_ROWS = 64;

}  // namespace

PendingData::PendingData(QObject* parent) : QObject(parent) {}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static) : Q_INVOKABLE, appelée par QML.
QString PendingData::value(const QString& /*key*/) const {
    return QString::fromUtf8(EMPTY_MARK);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static) : Q_INVOKABLE, appelée par QML.
QUrl PendingData::image(const QString& /*key*/) const {
    return {};
}

QAbstractItemModel* PendingData::rows(const QString& key, int count) {
    const int wanted = std::clamp(count, 0, MAXIMUM_ROWS);
    // Mis en cache par clé ET par nombre : un même écran peut demander deux listes de tailles
    // différentes, et les servir toutes deux depuis la même entrée donnerait la mauvaise hauteur
    // à l'une des deux.
    const QString cacheKey = key + QLatin1Char('#') + QString::number(wanted);
    if (SheetRowModel* cached = _models.value(cacheKey, nullptr); cached != nullptr) {
        return cached;
    }

    auto* model = new SheetRowModel(this);
    QVector<SheetRow> placeholder;
    placeholder.reserve(wanted);
    for (int rank = 0; rank < wanted; ++rank) {
        placeholder.append(
            SheetRow{.id = key, .label = QString::fromUtf8(EMPTY_MARK), .value = QString()});
    }
    model->setRows(std::move(placeholder));
    _models.insert(cacheKey, model);
    return model;
}

}  // namespace hmi
