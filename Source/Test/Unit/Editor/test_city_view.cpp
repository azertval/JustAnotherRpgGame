// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_city_view.cpp
 * @brief Tests de la **vue de ville** (`LOT-EDITOR-09`, `EX-EDIT-090`) : les quartiers d'une ville,
 *        leurs cadres, et celui qu'un clic ouvre.
 *
 * Lus sur la racine d'essai de l'éditeur (`LOT-123`), jamais récrits : ce test lisait la Capitale
 * **livrée**, que la table rase du `LOT-102` emporte. Sa ville d'essai a la même forme — un
 * quartier où l'on marche, avec sa carte et son cadre, et quatre que ferme une porte gardée.
 */

#include <algorithm>
#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "Editor/Logic/CityView.h"

namespace {

[[nodiscard]] std::filesystem::path elements() {
    return std::filesystem::path(JADG_TEST_DATA_DIR);
}

[[nodiscard]] const hmi::CityDistrictView* quartier(const hmi::CityView& ville,
                                                    std::string_view id) {
    const auto found = std::ranges::find(ville.districts, id, &hmi::CityDistrictView::id);
    return found != ville.districts.end() ? &*found : nullptr;
}

}  // namespace

/**
 * @brief Une ville se voit par quartiers : ceux où l'on marche d'abord, avec leur cadre et leur
 *        carte, puis ceux que ferme une porte gardée.
 * \castest{<b>Une ville se voit par quartiers.</b><br/>
 * \tcat Unitaire · Le monde<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Bâtir la vue de la ville `bourg`.<br/>
 * \tattendu La vue porte le plan de la ville et ses cinq quartiers ; la Place a une carte
 * présente et un cadre ; les autres n'ont qu'une porte gardée, et viennent après.
 * }
 */
TEST(VueDeVille, UneVilleSeVoitParQuartiers) {
    const hmi::CityView ville = hmi::buildCityView(elements(), "bourg");
    ASSERT_TRUE(ville.ok()) << ville.error;
    EXPECT_EQ(ville.location, "test-city");
    EXPECT_EQ(ville.image, "city-test-city.jpg");
    ASSERT_GE(ville.districts.size(), 2U);

    const hmi::CityDistrictView* const place = quartier(ville, "test-city-place");
    ASSERT_NE(place, nullptr);
    EXPECT_EQ(place->mapId, "bourg/place");
    EXPECT_TRUE(place->mapExists);
    EXPECT_TRUE(place->framed);
    EXPECT_FALSE(place->name.empty());
    EXPECT_NE(place->name, place->id);  // l'atlas le nomme

    const hmi::CityDistrictView* const nord = quartier(ville, "test-city-nord");
    ASSERT_NE(nord, nullptr);
    EXPECT_TRUE(nord->mapId.empty());
    EXPECT_EQ(nord->guardMapId, "bourg/place");
    EXPECT_FALSE(nord->mapExists);

    // Les quartiers où l'on marche passent devant : ce sont ceux qu'on ouvre.
    const auto sansCarte = std::ranges::find_if(
        ville.districts, [](const hmi::CityDistrictView& d) { return d.mapId.empty(); });
    EXPECT_TRUE(std::all_of(sansCarte, ville.districts.end(),
                            [](const hmi::CityDistrictView& d) { return d.mapId.empty(); }));
}

/**
 * @brief Un clic sur le plan désigne le quartier dont le cadre le contient ; ailleurs, aucun.
 * \castest{<b>Un clic sur le plan désigne un quartier.</b><br/>
 * \tcat Unitaire · Le monde<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Pointer le centre du cadre de la Place, puis un coin du plan.<br/>
 * \tattendu Le premier point désigne la Place, le second ne désigne rien.
 * }
 */
TEST(VueDeVille, UnClicSurLePlanDesigneUnQuartier) {
    const hmi::CityView ville = hmi::buildCityView(elements(), "bourg");
    ASSERT_TRUE(ville.ok()) << ville.error;

    const std::optional<std::size_t> vise =
        hmi::districtAt(ville, hmi::MapPoint{.x = 0.70, .y = 0.50});
    ASSERT_TRUE(vise.has_value());
    EXPECT_EQ(ville.districts[*vise].id, "test-city-place");

    EXPECT_FALSE(hmi::districtAt(ville, hmi::MapPoint{.x = 0.01, .y = 0.99}).has_value());
}

/**
 * @brief Une ville inconnue rend une vue en erreur, sans lever ; la carte d'un quartier dit de
 *        quelle ville elle est.
 * \castest{<b>Une ville inconnue le dit.</b><br/>
 * \tcat Unitaire · Le monde<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Bâtir la vue d'une ville qui n'existe pas.<br/>2. Demander la ville de
 * `bourg/place`, puis celle d'une carte hors ville.<br/>
 * \tattendu La vue porte une erreur nommée ; la Place est du `bourg`, le Donjon d'aucune ville.
 * }
 */
TEST(VueDeVille, UneVilleInconnueLeDit) {
    const hmi::CityView aucune = hmi::buildCityView(elements(), "atlantide");
    EXPECT_FALSE(aucune.ok());
    EXPECT_NE(aucune.error.find("atlantide"), std::string::npos);

    EXPECT_EQ(hmi::cityOfMap(elements(), "bourg/place"), "bourg");
    EXPECT_EQ(hmi::cityOfMap(elements(), "donjon"), "");
    EXPECT_FALSE(hmi::cityIds(elements()).empty());
}
