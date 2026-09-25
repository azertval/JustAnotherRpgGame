// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/WorldMapModel.h"

#include <QRectF>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "Core/World/Atlas.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Presentation/WorldMaps.h"

namespace hmi {
namespace {

[[nodiscard]] QVariantList labelRows(const std::vector<MapLabel>& labels) {
    QVariantList rows;
    for (const MapLabel& label : labels) {
        rows.append(QVariantMap{{QStringLiteral("name"), QString::fromStdString(label.name)},
                                {QStringLiteral("kind"), QString::fromStdString(label.kind)},
                                {QStringLiteral("x"), label.at.x},
                                {QStringLiteral("y"), label.at.y}});
    }
    return rows;
}

[[nodiscard]] QVariantMap regionRow(const MapRegionView& region, int& placedCount) {
    QVariantList places;
    int placed = 0;
    for (const MapPlaceView& place : region.places) {
        placed += place.placed ? 1 : 0;
        places.append(QVariantMap{
            {QStringLiteral("placeId"), QString::fromStdString(place.id)},
            {QStringLiteral("name"), QString::fromStdString(place.name)},
            {QStringLiteral("description"), QString::fromStdString(place.description)},
            {QStringLiteral("placed"), place.placed},
            {QStringLiteral("x"), place.at.x},
            {QStringLiteral("y"), place.at.y},
            {QStringLiteral("hasCityMap"), place.hasCityMap},
            // Ce que `MapMarker` lit : une ville a plan s'ouvre, les autres se lisent.
            {QStringLiteral("kind"),
             place.hasCityMap ? QStringLiteral("city") : QStringLiteral("point-of-interest")},
            {QStringLiteral("number"), 0},
            {QStringLiteral("gateway"), place.hasCityMap}});
    }
    placedCount += placed;

    QVariantList grades;
    for (const int grade : region.grades) {
        grades.append(grade);
    }
    return QVariantMap{
        {QStringLiteral("regionId"), QString::fromStdString(region.id)},
        {QStringLiteral("name"), QString::fromStdString(region.name)},
        {QStringLiteral("image"), QString::fromStdString(region.image)},
        {QStringLiteral("x"), region.anchor.x},
        {QStringLiteral("y"), region.anchor.y},
        {QStringLiteral("kind"), QStringLiteral("city")},
        {QStringLiteral("number"), 0},
        {QStringLiteral("gateway"), true},
        {QStringLiteral("frame"), QVariantMap{{QStringLiteral("x"), region.frame.x},
                                              {QStringLiteral("y"), region.frame.y},
                                              {QStringLiteral("width"), region.frame.width},
                                              {QStringLiteral("height"), region.frame.height}}},
        {QStringLiteral("government"), QString::fromStdString(region.government)},
        {QStringLiteral("faction"), QString::fromStdString(region.faction)},
        {QStringLiteral("population"), region.population.value_or(0)},
        {QStringLiteral("grades"), grades},
        {QStringLiteral("places"), places},
        {QStringLiteral("placedCount"), placed},
        {QStringLiteral("labels"), labelRows(region.labels)}};
}

[[nodiscard]] QVariantMap cityRow(const MapCityView& city) {
    QVariantList points;
    for (const MapCityPointView& point : city.points) {
        QVariantMap row{{QStringLiteral("pointId"), QString::fromStdString(point.id)},
                        {QStringLiteral("number"), point.number},
                        {QStringLiteral("name"), QString::fromStdString(point.name)},
                        {QStringLiteral("description"), QString::fromStdString(point.description)},
                        {QStringLiteral("x"), point.at.x},
                        {QStringLiteral("y"), point.at.y},
                        {QStringLiteral("kind"), QStringLiteral("point-of-interest")},
                        {QStringLiteral("gateway"), false},
                        {QStringLiteral("hasDistrictView"), point.district.has_value()}};
        // La vue du quartier (LOT-96) : le cadre du plan qu'on agrandit, et sa carte peinte le jour
        // ou elle existe.
        if (point.district) {
            row.insert(QStringLiteral("frame"),
                       QRectF(point.district->frame.x, point.district->frame.y,
                              point.district->frame.width, point.district->frame.height));
            row.insert(QStringLiteral("districtImage"),
                       QString::fromStdString(point.district->image));
        }
        points.append(row);
    }
    return QVariantMap{{QStringLiteral("cityId"), QString::fromStdString(city.id)},
                       {QStringLiteral("name"), QString::fromStdString(city.name)},
                       {QStringLiteral("regionId"), QString::fromStdString(city.regionId)},
                       {QStringLiteral("image"), QString::fromStdString(city.image)},
                       {QStringLiteral("points"), points},
                       {QStringLiteral("labels"), labelRows(city.labels)}};
}

}  // namespace

WorldMapModel::WorldMapModel(QObject* parent) : QObject(parent) {}

void WorldMapModel::load() {
    const std::filesystem::path root = dataDirectory();
    const core::Atlas atlas = core::loadAtlas(root / "World");
    for (const std::string& error : atlas.errors) {
        HMI_LOG_WARNING("Atlas : " + error);
    }

    std::ifstream file(root / "Maps" / "world-maps.json", std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    const WorldMaps maps = readWorldMaps(buffer.str());
    if (!maps.ok()) {
        HMI_LOG_ERROR("Carte : " + maps.error);
    }

    std::vector<std::string> mismatches;
    const WorldMapViews views = joinWorldMaps(atlas, maps, mismatches);
    for (const std::string& mismatch : mismatches) {
        // Une region sans carte manque a l'ecran ; une position sans lieu ne designe rien. Les deux
        // se corrigent dans les donnees, et doivent donc se voir (EX-CNT-010).
        HMI_LOG_WARNING("Carte : " + mismatch + ".");
    }

    _worldImage = QString::fromStdString(views.worldImage);
    _regions.clear();
    _cities.clear();
    _placedCount = 0;
    for (const MapRegionView& region : views.regions) {
        _regions.append(regionRow(region, _placedCount));
    }
    for (const MapCityView& city : views.cities) {
        _cities.insert(QString::fromStdString(city.id), cityRow(city));
    }
    emit changed();
}

QVariantMap WorldMapModel::city(const QString& placeId) const {
    return _cities.value(placeId).toMap();
}

int WorldMapModel::regionIndex(const QString& regionId) const {
    for (int index = 0; index < _regions.size(); ++index) {
        if (_regions.at(index).toMap().value(QStringLiteral("regionId")).toString() == regionId) {
            return index;
        }
    }
    return -1;
}

}  // namespace hmi
