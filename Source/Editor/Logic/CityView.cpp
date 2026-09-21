// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/CityView.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <system_error>

#include "Core/World/Atlas.h"
#include "Core/World/CityPlan.h"

namespace hmi {

namespace {

/// Le dossier des villes jouables.
[[nodiscard]] std::filesystem::path citiesDirectory(const std::filesystem::path& dataRoot) {
    return dataRoot / "World" / "cities";
}

/// Le texte de @p path, vide s'il ne se lit pas (aucune lecture ne leve, `EX-NFR-040`).
[[nodiscard]] std::string readText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    return std::string{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

/// L'aire d'un cadre : ce qui departage deux quartiers qui se recouvrent.
[[nodiscard]] double area(const MapFrame& frame) {
    return frame.width * frame.height;
}

}  // namespace

std::vector<std::string> worldRegionIds(const std::filesystem::path& dataRoot) {
    std::vector<std::string> ids;
    const WorldMaps maps = readWorldMaps(readText(dataRoot / "Maps" / "world-maps.json"));
    for (const auto& [id, region] : maps.regions) {
        ids.push_back(id);
    }
    std::ranges::sort(ids);
    return ids;
}

std::vector<std::string> cityIds(const std::filesystem::path& dataRoot) {
    std::vector<std::string> ids;
    std::error_code error;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(citiesDirectory(dataRoot), error)) {
        if (entry.is_regular_file(error) && entry.path().extension() == ".json") {
            ids.push_back(entry.path().stem().string());
        }
    }
    std::ranges::sort(ids);
    return ids;
}

CityView buildCityView(const std::filesystem::path& dataRoot, std::string_view cityId) {
    CityView view;
    view.id = std::string{cityId};

    const core::CityPlanResult plan =
        core::loadCityPlan(citiesDirectory(dataRoot) / (std::string{cityId} + ".json"));
    if (!plan.ok()) {
        view.error = "City " + view.id + " cannot be read: " + plan.error;
        return view;
    }
    view.name = plan.plan.name;
    view.location = plan.plan.location;

    const WorldMaps maps = readWorldMaps(readText(dataRoot / "Maps" / "world-maps.json"));
    if (!maps.ok()) {
        view.error = "world-maps.json cannot be read: " + maps.error;
        return view;
    }
    const auto city = maps.cities.find(plan.plan.location);
    if (city == maps.cities.end()) {
        view.error = "world-maps.json has no plan for " + plan.plan.location + ".";
        return view;
    }
    view.image = city->second.image;

    // L'atlas ne sert qu'aux noms : une fiche absente laisse l'identifiant, elle n'empeche rien.
    const core::Atlas atlas = core::loadAtlas(dataRoot / "World");
    for (const core::CityDistrict& district : plan.plan.districts) {
        CityDistrictView entry{.id = district.id,
                               .name = district.id,
                               .frame = {},
                               .framed = false,
                               .mapId = district.map,
                               .guardMapId = district.guardMap,
                               .mapExists = false};
        if (const core::Location* const location = atlas.findLocation(district.id)) {
            entry.name = location->name;
        }
        if (const auto framed = city->second.districts.find(district.id);
            framed != city->second.districts.end()) {
            entry.frame = framed->second.frame;
            entry.framed = true;
        }
        if (!district.map.empty()) {
            std::error_code error;
            entry.mapExists =
                std::filesystem::exists(dataRoot / "Levels" / (district.map + ".json"), error);
        }
        view.districts.push_back(std::move(entry));
    }
    // Les quartiers ou l'on marche d'abord : ce sont ceux qu'on ouvre. L'ordre de la ville est
    // garde a l'interieur de chaque groupe.
    std::ranges::stable_partition(
        view.districts, [](const CityDistrictView& district) { return !district.mapId.empty(); });
    return view;
}

std::string cityOfMap(const std::filesystem::path& dataRoot, std::string_view mapId) {
    for (const std::string& id : cityIds(dataRoot)) {
        const core::CityPlanResult plan =
            core::loadCityPlan(citiesDirectory(dataRoot) / (id + ".json"));
        if (plan.ok() && plan.plan.districtOfMap(mapId) != nullptr) {
            return id;
        }
    }
    return {};
}

std::optional<std::size_t> districtAt(const CityView& city, MapPoint point) {
    std::optional<std::size_t> best;
    for (std::size_t index = 0; index < city.districts.size(); ++index) {
        const CityDistrictView& district = city.districts[index];
        if (!district.framed) {
            continue;
        }
        const MapFrame& frame = district.frame;
        const bool inside = point.x >= frame.x && point.x <= frame.x + frame.width &&
                            point.y >= frame.y && point.y <= frame.y + frame.height;
        if (!inside) {
            continue;
        }
        if (!best || area(frame) < area(city.districts[*best].frame)) {
            best = index;
        }
    }
    return best;
}

}  // namespace hmi
