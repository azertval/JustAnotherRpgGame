// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_world_maps.cpp
 * @brief Tests unitaires des cartes de l'écran « Carte » : monde, régions, villes (`LOT-94`,
 * `LOT-95`) — trois niveaux reliés par des repères (`EX-IHM-106`), positions tenues à part de
 * l'atlas et jamais inventées (`EX-IHM-107`).
 */

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/World/Atlas.h"
#include "HMI/Presentation/WorldMaps.h"

namespace {

[[nodiscard]] std::string readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

constexpr const char* MINIMAL = R"({
  "version": 1,
  "world": {"image": "world.jpg"},
  "regions": {
    "a": {"image": "region-a.jpg", "anchor": [0.25, 0.5], "frame": [0.1, 0.2, 0.3, 0.3],
          "places": {"a-town": [0.4, 0.6]}, "omitted": ["a-rule"],
          "labels": [{"name": "Lake", "kind": "lake", "at": [0.7, 0.1]}]}
  },
  "cities": {
    "a-town": {"image": "city-a-town.jpg", "places": {"a-town-market": [0.5, 0.5]},
               "sites": [{"number": 3, "name": "Harbour", "note": "Piers.", "at": [0.2, 0.8]}]}
  }
})";

[[nodiscard]] core::Atlas smallAtlas() {
    core::Atlas atlas;
    core::Region region;
    region.id = "a";
    region.name = "Region A";
    region.locations = {"a-rule", "a-hidden", "a-town", "a-town-market"};
    atlas.regions.push_back(region);
    for (const char* id : {"a-rule", "a-hidden", "a-town", "a-town-market"}) {
        core::Location location;
        location.id = id;
        location.name = id;
        location.region = "a";
        atlas.locations.push_back(location);
    }
    return atlas;
}

}  // namespace

/**
 * @brief Un fichier valide se lit en entier ; une position ou un cadre hors de la carte fait
 *        échouer toute la lecture, en nommant l'entrée.
 * \castest{<b>Les cartes se lisent, et une position hors de la carte est refusée.</b><br/>
 * \tcat Unitaire · Carte<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire un fichier à une région et une ville.<br/>
 * 2. Lire une position d'abscisse 1,2, puis un cadre qui déborde.<br/>
 * \tattendu Les valeurs lues ; puis deux échecs qui nomment l'entrée, sans carte rendue.
 * }
 */
TEST(WorldMapsTest, FichierLuEtBorne) {
    const hmi::WorldMaps maps = hmi::readWorldMaps(MINIMAL);
    ASSERT_TRUE(maps.ok()) << maps.error;
    EXPECT_EQ(maps.worldImage, "world.jpg");
    ASSERT_EQ(maps.regions.size(), 1U);
    const hmi::RegionMap& region = maps.regions.at("a");
    EXPECT_DOUBLE_EQ(region.anchor.x, 0.25);
    EXPECT_DOUBLE_EQ(region.frame.width, 0.3);
    EXPECT_DOUBLE_EQ(region.places.at("a-town").y, 0.6);
    ASSERT_EQ(region.labels.size(), 1U);
    EXPECT_EQ(region.labels.front().kind, "lake");
    ASSERT_EQ(maps.cities.at("a-town").sites.size(), 1U);
    EXPECT_EQ(maps.cities.at("a-town").sites.front().number, 3);

    const hmi::WorldMaps outside = hmi::readWorldMaps(
        R"({"version":1,"world":{"image":"w.jpg"},"regions":{"c":{"image":"c.jpg",
            "anchor":[1.2,0.5],"frame":[0,0,1,1]}}})");
    EXPECT_FALSE(outside.ok());
    EXPECT_TRUE(outside.regions.empty());
    EXPECT_NE(outside.error.find("c"), std::string::npos);

    const hmi::WorldMaps overflowing = hmi::readWorldMaps(
        R"({"version":1,"world":{"image":"w.jpg"},"regions":{"d":{"image":"d.jpg",
            "anchor":[0.5,0.5],"frame":[0.8,0.1,0.4,0.4]}}})");
    EXPECT_FALSE(overflowing.ok());
    EXPECT_NE(overflowing.error.find("cadre"), std::string::npos);
}

/**
 * @brief Un quartier rendu porte sa grille et ses sous-zones ; une carte rendue sans grille, ou une
 *        sous-zone sans entrée, est refusée (`LOT-121`).
 * \castest{<b>La carte rendue d'un quartier se lit avec sa grille et ses sous-zones.</b><br/>
 * \tcat Unitaire · Carte<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire un plan dont le quartier nomme une carte rendue, sa grille et une sous-zone.<br/>
 * 2. Lire le même quartier sans grille, puis une sous-zone sans entrée.<br/>
 * \tattendu La grille place le point (2, 1) en origine + 2 colonnes + 1 ligne ; la sous-zone a
 * son nom, son entrée et sa carte ; puis deux échecs qui disent ce qui manque.
 * }
 */
TEST(WorldMapsTest, UnQuartierRenduPorteSaGrilleEtSesSousZones) {
    const std::string head = R"({"version":1,"world":{"image":"w.jpg"},"regions":{},"cities":{
        "t":{"image":"t.jpg","places":{"t-a":[0.5,0.5]},"districts":{"t-a":{
            "frame":[0.4,0.4,0.2,0.2],"image":"Regions/r/t/a/Map/a.jpg")";
    const std::string grid =
        R"(,"grid":{"origin":[0.5,0.1],"column":[0.02,0.025],"row":[-0.02,0.025]})";
    const std::string zones = R"(,"zones":{"crypte":{"name":"La crypte","entrance":[23,6],
            "image":"Regions/r/t/a/crypte/Map/crypte.jpg",
            "grid":{"origin":[0.4,0.05],"column":[0.01,0.01],"row":[-0.01,0.01]}}})";
    const hmi::WorldMaps maps = hmi::readWorldMaps(head + grid + zones + "}}}}}");
    ASSERT_TRUE(maps.ok()) << maps.error;
    const hmi::MapDistrict& district = maps.cities.at("t").districts.at("t-a");
    EXPECT_EQ(district.image, "Regions/r/t/a/Map/a.jpg");
    ASSERT_TRUE(district.grid.has_value());
    const hmi::MapPoint point = district.grid->at(2.0, 1.0);
    EXPECT_DOUBLE_EQ(point.x, 0.5 + (2 * 0.02) - 0.02);
    EXPECT_DOUBLE_EQ(point.y, 0.1 + (2 * 0.025) + 0.025);
    ASSERT_EQ(district.zones.size(), 1U);
    const hmi::MapZone& zone = district.zones.at("crypte");
    EXPECT_EQ(zone.name, "La crypte");
    EXPECT_DOUBLE_EQ(zone.entrance.x, 23.0);
    EXPECT_DOUBLE_EQ(zone.entrance.y, 6.0);
    EXPECT_TRUE(zone.grid.has_value());

    const hmi::WorldMaps gridless = hmi::readWorldMaps(head + "}}}}}");
    EXPECT_FALSE(gridless.ok());
    EXPECT_NE(gridless.error.find("grid"), std::string::npos) << gridless.error;

    const hmi::WorldMaps doorless = hmi::readWorldMaps(
        head + grid + R"(,"zones":{"crypte":{"name":"La crypte"}}}}}}})");
    EXPECT_FALSE(doorless.ok());
    EXPECT_NE(doorless.error.find("entrance"), std::string::npos) << doorless.error;
}

/**
 * @brief La jointure range les lieux : posés d'abord, écartés absents, quartiers dans leur ville.
 * \castest{<b>Une région montre ses lieux posés puis les autres, sans les entrées écartées ni les
 * quartiers d'une ville.</b><br/>
 * \tcat Unitaire · Carte<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Joindre un atlas de quatre lieux (une règle, un lieu sans position, une ville à
 * plan, son quartier) au fichier minimal.<br/>
 * \tattendu Deux lieux dans la région — la ville, posée et ouvrable, puis le lieu sans position ;
 * le plan de la ville porte son quartier numéroté 1 puis le lieu numéroté 3 ; aucun écart.
 * }
 */
TEST(WorldMapsTest, JointureRangeLesLieux) {
    const hmi::WorldMaps maps = hmi::readWorldMaps(MINIMAL);
    ASSERT_TRUE(maps.ok()) << maps.error;
    std::vector<std::string> mismatches;
    const hmi::WorldMapViews views = hmi::joinWorldMaps(smallAtlas(), maps, mismatches);
    EXPECT_TRUE(mismatches.empty()) << (mismatches.empty() ? "" : mismatches.front());

    ASSERT_EQ(views.regions.size(), 1U);
    const std::vector<hmi::MapPlaceView>& places = views.regions.front().places;
    ASSERT_EQ(places.size(), 2U);
    EXPECT_EQ(places[0].id, "a-town");
    EXPECT_TRUE(places[0].placed);
    EXPECT_TRUE(places[0].hasCityMap);
    EXPECT_EQ(places[1].id, "a-hidden");
    EXPECT_FALSE(places[1].placed);

    ASSERT_EQ(views.cities.size(), 1U);
    const std::vector<hmi::MapCityPointView>& points = views.cities.front().points;
    ASSERT_EQ(points.size(), 2U);
    EXPECT_EQ(points[0].id, "a-town-market");
    EXPECT_EQ(points[0].number, 1);
    EXPECT_EQ(points[1].name, "Harbour");
    EXPECT_EQ(points[1].number, 3);
}

/**
 * @brief Chaque écart entre l'atlas et les cartes est nommé.
 * \castest{<b>Une carte sans région, une région sans carte et une position étrangère sont
 * signalées.</b><br/>
 * \tcat Unitaire · Carte<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Joindre au fichier minimal, qui ne connaît que la région `a`, un atlas dont la seule
 * région est `b`.<br/>
 * \tattendu Trois écarts au moins : `b` sans carte, la carte `a` sans région, le plan `a-town` sans
 * lieu de l'atlas.
 * }
 */
TEST(WorldMapsTest, EcartsNommes) {
    core::Atlas atlas;
    core::Region region;
    region.id = "b";
    atlas.regions.push_back(region);

    std::vector<std::string> mismatches;
    const hmi::WorldMapViews views =
        hmi::joinWorldMaps(atlas, hmi::readWorldMaps(MINIMAL), mismatches);
    EXPECT_TRUE(views.regions.empty());
    EXPECT_TRUE(views.cities.empty());
    EXPECT_GE(mismatches.size(), 3U);
}

/**
 * @brief L'atlas livré et les cartes livrées se recouvrent exactement.
 * \castest{<b>Les treize régions de l'atlas ont leur carte, et chaque position désigne un lieu de
 * sa région.</b><br/>
 * \tcat Unitaire · Carte<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger l'atlas livré et `Source/Elements/Maps/world-maps.json`.<br/>
 * 2. Joindre les deux.<br/>
 * 3. Chercher l'image de chaque carte sous `Source/Elements/Assets/Maps/`.<br/>
 * \tattendu Aucun écart ; treize régions ; la Capitale impériale et Fisherman's Wharf ont un plan,
 * celui de la Capitale porte ses douze quartiers ; chaque image existe.
 * }
 */
TEST(WorldMapsTest, AtlasLivreEntierementCartographie) {
    const core::Atlas atlas = core::loadAtlas(JADG_WORLD_DIR);
    ASSERT_TRUE(atlas.errors.empty());
    const hmi::WorldMaps maps = hmi::readWorldMaps(readFile(JADG_WORLD_MAPS));
    ASSERT_TRUE(maps.ok()) << maps.error;

    std::vector<std::string> mismatches;
    const hmi::WorldMapViews views = hmi::joinWorldMaps(atlas, maps, mismatches);
    for (const std::string& mismatch : mismatches) {
        ADD_FAILURE() << mismatch;
    }
    EXPECT_EQ(views.regions.size(), atlas.regions.size());

    const std::filesystem::path images = std::filesystem::path(JADG_ASSETS_DIR) / "Maps";
    EXPECT_TRUE(std::filesystem::is_regular_file(images / views.worldImage)) << views.worldImage;
    for (const hmi::MapRegionView& region : views.regions) {
        EXPECT_TRUE(std::filesystem::is_regular_file(images / region.image)) << region.image;
    }

    ASSERT_EQ(views.cities.size(), 2U);
    for (const hmi::MapCityView& city : views.cities) {
        EXPECT_TRUE(std::filesystem::is_regular_file(images / city.image)) << city.image;
        EXPECT_EQ(city.points.size(), 12U) << city.id;
        // Une ville a plan doit etre posee sur la carte de sa region : c'est par son repere qu'on
        // y entre.
        const auto region = std::ranges::find_if(
            views.regions, [&city](const hmi::MapRegionView& r) { return r.id == city.regionId; });
        ASSERT_NE(region, views.regions.end()) << city.id;
        const auto place = std::ranges::find_if(
            region->places, [&city](const hmi::MapPlaceView& p) { return p.id == city.id; });
        ASSERT_NE(place, region->places.end()) << city.id;
        EXPECT_TRUE(place->placed) << city.id;
        EXPECT_TRUE(place->hasCityMap) << city.id;
        // La carte d'un quartier et de ses sous-zones : peinte dans Maps/, ou rendue et rangee
        // avec sa zone, relative a Assets/ (LOT-121).
        const auto imageOf = [&images](const std::string& image) {
            return image.find('/') != std::string::npos
                       ? std::filesystem::path(JADG_ASSETS_DIR) / image
                       : images / image;
        };
        for (const hmi::MapCityPointView& point : city.points) {
            if (!point.district || point.district->image.empty()) {
                continue;
            }
            EXPECT_TRUE(std::filesystem::is_regular_file(imageOf(point.district->image)))
                << point.district->image;
            for (const auto& [id, zone] : point.district->zones) {
                EXPECT_TRUE(zone.image.empty() ||
                            std::filesystem::is_regular_file(imageOf(zone.image)))
                    << id << " : " << zone.image;
            }
        }
    }
}
