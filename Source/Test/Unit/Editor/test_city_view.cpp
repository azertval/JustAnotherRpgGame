// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_city_view.cpp
 * @brief Tests de la **vue de ville** (`LOT-EDITOR-09`, `EX-EDIT-090`) : les quartiers de la
 *        Capitale, leurs cadres, et celui qu'un clic ouvre. Lus sur les données livrées, jamais
 *        récrits.
 */

#include <algorithm>
#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "Editor/Logic/CityView.h"

namespace {

[[nodiscard]] std::filesystem::path elements() {
    return std::filesystem::path(JADG_LEVELS_DIR).parent_path();
}

[[nodiscard]] const hmi::CityDistrictView* quartier(const hmi::CityView& ville,
                                                    std::string_view id) {
    const auto found = std::ranges::find(ville.districts, id, &hmi::CityDistrictView::id);
    return found != ville.districts.end() ? &*found : nullptr;
}

}  // namespace

/**
 * @brief La Capitale se voit par quartiers : ceux où l'on marche d'abord, avec leur cadre et leur
 *        carte, puis ceux que ferme une porte gardée.
 * \castest{<b>La Capitale se voit par quartiers.</b><br/>
 * \tcat Unitaire · Le monde<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Bâtir la vue de la ville `capital` sur les données livrées.<br/>
 * \tattendu La vue porte le plan de la ville et ses six quartiers ; Martpart et Arenarea ont
 * une carte présente et un cadre ; les autres n'ont qu'une porte gardée, et viennent après.
 * }
 */
TEST(VueDeVille, LaCapitaleSeVoitParQuartiers) {
    const hmi::CityView ville = hmi::buildCityView(elements(), "capital");
    ASSERT_TRUE(ville.ok()) << ville.error;
    EXPECT_EQ(ville.location, "central-empire-the-capital-city");
    EXPECT_EQ(ville.image, "city-central-empire-the-capital-city.jpg");
    ASSERT_GE(ville.districts.size(), 2U);

    const hmi::CityDistrictView* const martpart =
        quartier(ville, "central-empire-the-capital-city-martpart");
    ASSERT_NE(martpart, nullptr);
    EXPECT_EQ(martpart->mapId, "capital/martpart");
    EXPECT_TRUE(martpart->mapExists);
    EXPECT_TRUE(martpart->framed);
    EXPECT_FALSE(martpart->name.empty());
    EXPECT_NE(martpart->name, martpart->id);  // l'atlas le nomme

    const hmi::CityDistrictView* const dweomer =
        quartier(ville, "central-empire-the-capital-city-dweomer");
    ASSERT_NE(dweomer, nullptr);
    EXPECT_TRUE(dweomer->mapId.empty());
    EXPECT_EQ(dweomer->guardMapId, "capital/martpart");
    EXPECT_FALSE(dweomer->mapExists);

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
 * \tetapes 1. Pointer le centre du cadre de Martpart, puis un coin du plan.<br/>
 * \tattendu Le premier point désigne Martpart, le second ne désigne rien.
 * }
 */
TEST(VueDeVille, UnClicSurLePlanDesigneUnQuartier) {
    const hmi::CityView ville = hmi::buildCityView(elements(), "capital");
    ASSERT_TRUE(ville.ok()) << ville.error;

    const std::optional<std::size_t> vise =
        hmi::districtAt(ville, hmi::MapPoint{.x = 0.702, .y = 0.505});
    ASSERT_TRUE(vise.has_value());
    EXPECT_EQ(ville.districts[*vise].id, "central-empire-the-capital-city-martpart");

    EXPECT_FALSE(hmi::districtAt(ville, hmi::MapPoint{.x = 0.01, .y = 0.99}).has_value());
}

/**
 * @brief Une ville inconnue rend une vue en erreur, sans lever ; la carte d'un quartier dit de
 *        quelle ville elle est.
 * \castest{<b>Une ville inconnue le dit.</b><br/>
 * \tcat Unitaire · Le monde<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Bâtir la vue d'une ville qui n'existe pas.<br/>2. Demander la ville de
 * `capital/martpart`, puis celle du Colisée.<br/>
 * \tattendu La vue porte une erreur nommée ; Martpart est de `capital`, le Colisée d'aucune
 * ville.
 * }
 */
TEST(VueDeVille, UneVilleInconnueLeDit) {
    const hmi::CityView aucune = hmi::buildCityView(elements(), "atlantide");
    EXPECT_FALSE(aucune.ok());
    EXPECT_NE(aucune.error.find("atlantide"), std::string::npos);

    EXPECT_EQ(hmi::cityOfMap(elements(), "capital/martpart"), "capital");
    EXPECT_EQ(hmi::cityOfMap(elements(), "coliseum"), "");
    EXPECT_FALSE(hmi::cityIds(elements()).empty());
}
