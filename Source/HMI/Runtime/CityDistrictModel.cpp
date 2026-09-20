// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/CityDistrictModel.h"

#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <string>

#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/World/CityBlock.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Runtime/RuleLabels.h"

namespace hmi {

namespace {

[[nodiscard]] core::LevelLoadResult lireCarte(const QString& mapId) {
    return core::LevelLoader::loadFromFile(executableDirectory() / "Levels" /
                                           (mapId.toStdString() + ".json"));
}

}  // namespace

CityDistrictModel::CityDistrictModel(QObject* parent) : QObject(parent) {}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static): Q_INVOKABLE.
QVariantMap CityDistrictModel::district(const QString& mapId) const {
    const core::LevelLoadResult lu = lireCarte(mapId);
    if (!lu.ok() || !lu.level.has_value()) {
        return {};
    }
    const core::Level& carte = *lu.level;
    const double colonnes = carte.tileMap().width();
    const double lignes = carte.tileMap().height();
    const std::string langue = activeLanguage();

    QVariantList ilots;
    for (const core::CityBlock& ilot : core::cityBlocksOf(carte)) {
        ilots.append(QVariantMap{
            {QStringLiteral("blockId"), QString::fromStdString(ilot.name)},
            {QStringLiteral("name"),
             QString::fromStdString(ruleLabel("city_block." + ilot.name, langue))},
            {QStringLiteral("column"), ilot.origin.column},
            {QStringLiteral("row"), ilot.origin.row},
            {QStringLiteral("width"), ilot.columns},
            {QStringLiteral("height"), ilot.rows},
            {QStringLiteral("x"), (ilot.origin.column + (ilot.columns / 2.0)) / colonnes},
            {QStringLiteral("y"), (ilot.origin.row + (ilot.rows / 2.0)) / lignes}});
    }
    return QVariantMap{{QStringLiteral("mapId"), mapId},
                       {QStringLiteral("columns"), carte.tileMap().width()},
                       {QStringLiteral("rows"), carte.tileMap().height()},
                       {QStringLiteral("blocks"), ilots}};
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static): Q_INVOKABLE.
QString CityDistrictModel::blockImage(const QString& mapId, const QString& blockId,
                                      const QString& figure, qreal heroColumn,
                                      qreal heroRow) const {
    // L'identifiant que `CityBlockImageProvider` lit : champs separes par `|`, encodes pour qu'une
    // barre oblique de la carte (`capital/martpart`) ne se confonde pas avec un chemin.
    const QString identifiant =
        QStringList{mapId, blockId, figure, QString::number(heroColumn), QString::number(heroRow)}
            .join(QLatin1Char('|'));
    return QStringLiteral("image://cityblock/") +
           QString::fromLatin1(QUrl::toPercentEncoding(identifiant));
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static): Q_INVOKABLE.
QString CityDistrictModel::blockAt(const QString& mapId, int column, int row) const {
    const core::LevelLoadResult lu = lireCarte(mapId);
    if (!lu.ok() || !lu.level.has_value()) {
        return {};
    }
    for (const core::CityBlock& ilot : core::cityBlocksOf(*lu.level)) {
        if (ilot.contains({.column = column, .row = row})) {
            return QString::fromStdString(ilot.name);
        }
    }
    return {};
}

}  // namespace hmi
