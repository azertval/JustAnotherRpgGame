// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Presentation/WorldMaps.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <nlohmann/json.hpp>

#include "Core/Data/JsonDocument.h"

namespace hmi {
namespace {

constexpr int FORMAT_VERSION = 1;
constexpr std::string_view ORIGIN = "world-maps.json";

/// Échec de lecture : porte le message, remonté tel quel dans `WorldMaps::error`.
class ReadFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[nodiscard]] ReadFailure failure(const std::string& where, const std::string& what) {
    return ReadFailure{std::string(ORIGIN) + " : " + where + " — " + what};
}

[[nodiscard]] bool isFraction(const nlohmann::json& value) {
    return value.is_number() && value.get<double>() >= 0.0 && value.get<double>() <= 1.0;
}

/// Chemin de lecture `where / part`, pour situer une erreur sans chaîner des concaténations.
[[nodiscard]] std::string within(std::string where, std::string_view part) {
    where += " / ";
    where += part;
    return where;
}

[[nodiscard]] MapPoint readPoint(const nlohmann::json& value, const std::string& where) {
    if (!value.is_array() || value.size() != 2 || !isFraction(value[0]) || !isFraction(value[1])) {
        throw failure(where, "la position n'est pas une paire de fractions entre 0 et 1.");
    }
    return MapPoint{.x = value[0].get<double>(), .y = value[1].get<double>()};
}

// Une paire de nombres quelconques : un vecteur de grille, une case.
[[nodiscard]] MapPoint readPair(const nlohmann::json& value, const std::string& where) {
    if (!value.is_array() || value.size() != 2 || !value[0].is_number() || !value[1].is_number()) {
        throw failure(where, "il faut une paire de nombres.");
    }
    return MapPoint{.x = value[0].get<double>(), .y = value[1].get<double>()};
}

// La grille d'une carte rendue (LOT-121) : trois paires, `origin`, `column`, `row`.
[[nodiscard]] std::optional<MapGrid> readGrid(const nlohmann::json& object,
                                              const std::string& where) {
    const auto found = object.find("grid");
    if (found == object.end()) {
        return std::nullopt;
    }
    if (!found->is_object() || !found->contains("origin") || !found->contains("column") ||
        !found->contains("row")) {
        throw failure(where, "la grille (« grid ») veut « origin », « column » et « row ».");
    }
    return MapGrid{.origin = readPair((*found)["origin"], within(where, "grid / origin")),
                   .column = readPair((*found)["column"], within(where, "grid / column")),
                   .row = readPair((*found)["row"], within(where, "grid / row"))};
}

[[nodiscard]] MapFrame readFrame(const nlohmann::json& value, const std::string& where) {
    constexpr std::size_t FRAME_SIZE = 4;
    const bool shaped =
        value.is_array() && value.size() == FRAME_SIZE && std::ranges::all_of(value, isFraction);
    if (!shaped) {
        throw failure(where, "le cadre n'est pas [x, y, largeur, hauteur] en fractions.");
    }
    const MapFrame frame{.x = value[0].get<double>(),
                         .y = value[1].get<double>(),
                         .width = value[2].get<double>(),
                         .height = value[3].get<double>()};
    constexpr double EPSILON = 1e-9;
    if (frame.width <= 0.0 || frame.height <= 0.0 || frame.x + frame.width > 1.0 + EPSILON ||
        frame.y + frame.height > 1.0 + EPSILON) {
        throw failure(where, "le cadre est vide ou sort de la carte.");
    }
    return frame;
}

[[nodiscard]] std::string readText(const nlohmann::json& object, const char* field,
                                   const std::string& where, bool required) {
    const auto found = object.find(field);
    if (found == object.end()) {
        if (required) {
            throw failure(where, std::string("le champ « ") + field + " » manque.");
        }
        return {};
    }
    if (!found->is_string()) {
        throw failure(where, std::string("le champ « ") + field + " » n'est pas un texte.");
    }
    return found->get<std::string>();
}

[[nodiscard]] std::map<std::string, MapPoint> readPlaces(const nlohmann::json& object,
                                                         const std::string& where) {
    std::map<std::string, MapPoint> places;
    const auto found = object.find("places");
    if (found == object.end()) {
        return places;
    }
    if (!found->is_object()) {
        throw failure(where, "le champ « places » n'est pas un objet.");
    }
    for (const auto& [id, position] : found->items()) {
        std::string sousChemin = where;
        sousChemin += " / ";
        sousChemin += id;
        places.emplace(id, readPoint(position, sousChemin));
    }
    return places;
}

[[nodiscard]] std::vector<MapLabel> readLabels(const nlohmann::json& object,
                                               const std::string& where) {
    std::vector<MapLabel> labels;
    const auto found = object.find("labels");
    if (found == object.end()) {
        return labels;
    }
    if (!found->is_array()) {
        throw failure(where, "le champ « labels » n'est pas une liste.");
    }
    for (const nlohmann::json& entry : *found) {
        if (!entry.is_object() || !entry.contains("at")) {
            throw failure(where, "un nom de géographie n'a pas de position.");
        }
        MapLabel label{.name = readText(entry, "name", where, true),
                       .kind = readText(entry, "kind", where, false),
                       .at = {}};
        label.at = readPoint(entry["at"], where + " / " + label.name);
        labels.push_back(std::move(label));
    }
    return labels;
}

[[nodiscard]] std::vector<MapSite> readSites(const nlohmann::json& object,
                                             const std::string& where) {
    std::vector<MapSite> sites;
    const auto found = object.find("sites");
    if (found == object.end()) {
        return sites;
    }
    if (!found->is_array()) {
        throw failure(where, "le champ « sites » n'est pas une liste.");
    }
    for (const nlohmann::json& entry : *found) {
        if (!entry.is_object() || !entry.contains("at")) {
            throw failure(where, "un lieu numéroté n'a pas de position.");
        }
        MapSite site{.number = 0,
                     .name = readText(entry, "name", where, true),
                     .note = readText(entry, "note", where, false),
                     .at = {}};
        if (const auto number = entry.find("number");
            number != entry.end() && number->is_number_integer()) {
            site.number = number->get<int>();
        }
        site.at = readPoint(entry["at"], where + " / " + site.name);
        sites.push_back(std::move(site));
    }
    return sites;
}

[[nodiscard]] RegionMap readRegion(const nlohmann::json& object, const std::string& where) {
    if (!object.is_object() || !object.contains("anchor") || !object.contains("frame")) {
        throw failure(where, "il faut une image, un repère (« anchor ») et un cadre (« frame »).");
    }
    RegionMap region;
    region.image = readText(object, "image", where, true);
    region.anchor = readPoint(object["anchor"], where + " / anchor");
    region.frame = readFrame(object["frame"], where + " / frame");
    region.places = readPlaces(object, where);
    region.labels = readLabels(object, where);
    if (const auto omitted = object.find("omitted"); omitted != object.end()) {
        if (!omitted->is_array() || !std::ranges::all_of(*omitted, [](const nlohmann::json& id) {
                return id.is_string();
            })) {
            throw failure(where, "le champ « omitted » n'est pas une liste d'identifiants.");
        }
        region.omitted = omitted->get<std::vector<std::string>>();
    }
    return region;
}

// Les sous-zones d'un quartier (LOT-121) : `zones: { <dossier>: {name, entrance, image, grid} }`.
[[nodiscard]] std::map<std::string, MapZone> readZones(const nlohmann::json& object,
                                                       const std::string& where) {
    std::map<std::string, MapZone> zones;
    const auto found = object.find("zones");
    if (found == object.end()) {
        return zones;
    }
    if (!found->is_object()) {
        throw failure(where, "le champ « zones » n'est pas un objet.");
    }
    for (const auto& [id, entry] : found->items()) {
        const std::string here = within(within(where, "zones"), id);
        if (!entry.is_object() || !entry.contains("entrance")) {
            throw failure(here, "il faut une entrée (« entrance »), en cases.");
        }
        MapZone zone{.name = readText(entry, "name", here, true),
                     .entrance = readPair(entry["entrance"], within(here, "entrance")),
                     .image = readText(entry, "image", here, false),
                     .grid = readGrid(entry, here)};
        if (!zone.image.empty() && !zone.grid) {
            throw failure(here, "une carte rendue veut sa grille (« grid »).");
        }
        zones.emplace(id, std::move(zone));
    }
    return zones;
}

[[nodiscard]] std::map<std::string, MapDistrict> readDistricts(const nlohmann::json& object,
                                                               const std::string& where) {
    std::map<std::string, MapDistrict> districts;
    const auto found = object.find("districts");
    if (found == object.end()) {
        return districts;
    }
    if (!found->is_object()) {
        throw failure(where, "le champ « districts » n'est pas un objet.");
    }
    for (const auto& [id, entry] : found->items()) {
        if (!entry.is_object() || !entry.contains("frame")) {
            throw failure(within(within(where, "districts"), id), "il faut un cadre (« frame »).");
        }
        const std::string here = within(within(where, "districts"), id);
        MapDistrict district{.frame = readFrame(entry["frame"], within(where, id)),
                             .image = readText(entry, "image", where, false),
                             .grid = readGrid(entry, here),
                             .zones = readZones(entry, here)};
        if (!district.image.empty() && district.image.find('/') != std::string::npos &&
            !district.grid) {
            throw failure(here, "une carte rendue (un chemin) veut sa grille (« grid »).");
        }
        districts.emplace(id, std::move(district));
    }
    return districts;
}

[[nodiscard]] CityMap readCity(const nlohmann::json& object, const std::string& where) {
    if (!object.is_object()) {
        throw failure(where, "le plan n'est pas un objet.");
    }
    CityMap city;
    city.image = readText(object, "image", where, true);
    city.places = readPlaces(object, where);
    city.sites = readSites(object, where);
    city.labels = readLabels(object, where);
    city.districts = readDistricts(object, where);
    for (const auto& [id, district] : city.districts) {
        if (!city.places.contains(id)) {
            throw failure(within(within(where, "districts"), id),
                          "ce quartier n'est pas placé sur le plan.");
        }
    }
    return city;
}

void readInto(WorldMaps& maps, const nlohmann::json& root) {
    const auto world = root.find("world");
    if (world == root.end() || !world->is_object()) {
        throw failure("world", "le champ manque ou n'est pas un objet.");
    }
    maps.worldImage = readText(*world, "image", "world", true);

    const auto regions = root.find("regions");
    if (regions == root.end() || !regions->is_object()) {
        throw failure("regions", "le champ manque ou n'est pas un objet.");
    }
    for (const auto& [id, region] : regions->items()) {
        maps.regions.emplace(id, readRegion(region, "regions / " + id));
    }

    if (const auto cities = root.find("cities"); cities != root.end()) {
        if (!cities->is_object()) {
            throw failure("cities", "le champ n'est pas un objet.");
        }
        for (const auto& [id, city] : cities->items()) {
            maps.cities.emplace(id, readCity(city, "cities / " + id));
        }
    }
}

[[nodiscard]] MapRegionView regionView(const core::Atlas& atlas, const core::Region& region,
                                       const RegionMap& map, const WorldMaps& maps,
                                       const std::set<std::string>& cityPlaces) {
    MapRegionView view{.id = region.id,
                       .name = region.name,
                       .image = map.image,
                       .anchor = map.anchor,
                       .frame = map.frame,
                       .government = region.government,
                       .faction = region.faction,
                       .population = region.population.total,
                       .grades = {},
                       .places = {},
                       .labels = map.labels};
    for (std::size_t axis = 0; axis < core::kRegionAxisCount; ++axis) {
        view.grades.at(axis) = static_cast<int>(region.statistics.at(axis).grade());
    }
    for (const std::string& locationId : region.locations) {
        const core::Location* const location = atlas.findLocation(locationId);
        const bool omitted = std::ranges::find(map.omitted, locationId) != map.omitted.end();
        if (location == nullptr || omitted || cityPlaces.contains(locationId)) {
            continue;
        }
        const auto position = map.places.find(locationId);
        view.places.push_back(
            MapPlaceView{.id = location->id,
                         .name = location->name,
                         .description = location->description,
                         .placed = position != map.places.end(),
                         .at = position != map.places.end() ? position->second : MapPoint{},
                         .hasCityMap = maps.cities.contains(locationId)});
    }
    // Les lieux posés d'abord : ce sont eux qu'on parcourt sur la carte ; l'ordre de l'atlas
    // départage.
    std::ranges::stable_partition(view.places,
                                  [](const MapPlaceView& place) { return place.placed; });
    return view;
}

[[nodiscard]] MapCityView cityView(const core::Atlas& atlas, const core::Location& location,
                                   const CityMap& map, std::vector<std::string>& mismatches) {
    MapCityView view{.id = location.id,
                     .name = location.name,
                     .regionId = location.region,
                     .image = map.image,
                     .points = {},
                     .labels = map.labels};
    // Les quartiers dans l'ordre où la RÉGION les liste — celui du livre (1 - Sloghood,
    // 2 - Uptown…), que l'ordre des fichiers de l'atlas, alphabétique, ne garde pas —, puis les
    // lieux numérotés dans l'ordre du fichier.
    int number = 0;
    const core::Region* const region = atlas.findRegion(location.region);
    for (const std::string& candidateId :
         region != nullptr ? region->locations : std::vector<std::string>{}) {
        const auto position = map.places.find(candidateId);
        const core::Location* const candidate = atlas.findLocation(candidateId);
        if (position == map.places.end() || candidate == nullptr) {
            continue;
        }
        const auto district = map.districts.find(candidateId);
        view.points.push_back(MapCityPointView{.id = candidate->id,
                                               .number = ++number,
                                               .name = candidate->name,
                                               .description = candidate->description,
                                               .at = position->second,
                                               .district = district != map.districts.end()
                                                               ? std::optional{district->second}
                                                               : std::nullopt});
    }
    for (const auto& [id, position] : map.places) {
        if (atlas.findLocation(id) == nullptr) {
            mismatches.push_back("plan de « " + location.id + " » : le quartier « " + id +
                                 " » n'est pas un lieu de l'atlas");
        }
    }
    for (const MapSite& site : map.sites) {
        view.points.push_back(
            MapCityPointView{.id = location.id + "#" + std::to_string(site.number),
                             .number = site.number,
                             .name = site.name,
                             .description = site.note,
                             .at = site.at,
                             .district = std::nullopt});
    }
    return view;
}

}  // namespace

WorldMaps readWorldMaps(std::string_view json) {
    WorldMaps maps;
    const core::JsonDocument document = core::readJsonObject(json, FORMAT_VERSION, ORIGIN);
    if (!document.ok()) {
        maps.error = document.message;
        return maps;
    }
    try {
        readInto(maps, document.root);
    } catch (const ReadFailure& error) {
        maps = WorldMaps{};
        maps.error = error.what();
    }
    return maps;
}

WorldMapViews joinWorldMaps(const core::Atlas& atlas, const WorldMaps& maps,
                            std::vector<std::string>& mismatches) {
    WorldMapViews views;
    views.worldImage = maps.worldImage;

    // Un quartier posé sur un plan de ville ne se répète pas dans la liste de sa région.
    std::set<std::string> cityPlaces;
    for (const auto& [cityId, city] : maps.cities) {
        for (const auto& [placeId, position] : city.places) {
            cityPlaces.insert(placeId);
        }
    }

    for (const core::Region& region : atlas.regions) {
        const auto map = maps.regions.find(region.id);
        if (map == maps.regions.end()) {
            mismatches.push_back("la région « " + region.id + " » n'a pas de carte");
            continue;
        }
        for (const auto& [placeId, position] : map->second.places) {
            if (std::ranges::find(region.locations, placeId) == region.locations.end()) {
                mismatches.push_back("carte de « " + region.id + " » : « " + placeId +
                                     " » n'est pas un lieu de cette région");
            }
        }
        for (const std::string& omitted : map->second.omitted) {
            if (std::ranges::find(region.locations, omitted) == region.locations.end()) {
                mismatches.push_back("carte de « " + region.id + " » : l'entrée écartée « " +
                                     omitted + " » n'est pas un lieu de cette région");
            }
        }
        views.regions.push_back(regionView(atlas, region, map->second, maps, cityPlaces));
    }
    for (const auto& [id, map] : maps.regions) {
        if (atlas.findRegion(id) == nullptr) {
            mismatches.push_back("la carte « " + id + " » ne désigne aucune région de l'atlas");
        }
    }

    for (const auto& [id, map] : maps.cities) {
        const core::Location* const location = atlas.findLocation(id);
        if (location == nullptr) {
            mismatches.push_back("le plan « " + id + " » ne désigne aucun lieu de l'atlas");
            continue;
        }
        views.cities.push_back(cityView(atlas, *location, map, mismatches));
    }
    return views;
}

}  // namespace hmi
