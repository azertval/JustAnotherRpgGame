// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_city_plan.cpp
 * @brief Le graphe d'une ville jouable (LOT-96) : la lecture, ses refus, et la Capitale livree.
 */

#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "Core/World/CityPlan.h"
#include "Core/World/WorldTravel.h"

namespace {

const std::filesystem::path MONDE{std::filesystem::path(JADG_TEST_DATA_DIR) / "World"};
const std::filesystem::path NIVEAUX{std::filesystem::path(JADG_TEST_DATA_DIR) / "Levels"};

// Fournit un dossier temporaire vierge par test (cree/supprime automatiquement).
class CityPlanFileTest : public ::testing::Test {
protected:
    std::filesystem::path dir;

    void SetUp() override {
        dir = std::filesystem::temp_directory_path() /
              ("jadg_city_plan_" +
               std::string{::testing::UnitTest::GetInstance()->current_test_info()->name()});
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);
    }

    void TearDown() override {
        std::filesystem::remove_all(dir);
    }

    [[nodiscard]] std::filesystem::path ecrire(const std::string& contenu) const {
        const std::filesystem::path chemin = dir / "ville.json";
        std::ofstream(chemin, std::ios::binary) << contenu;
        return chemin;
    }
};

}  // namespace

/**
 * @brief La ville d'essai se lit : six quartiers, deux cartes, quatre portes gardees.
 * \castest{<b>Une ville se lit, et sa porte de depart s'ouvre.</b><br/>
 * \tcat Unitaire · Graphe de la ville<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire la ville de la racine d'essai.<br/>
 * 2. Entrer a sa porte de depart par le chargeur du jeu.<br/>
 * \tattendu Six quartiers, dont deux avec leur carte et quatre fermes ; le depart est la porte de
 * l'Est du premier, et la carte s'y ouvre.
 * }
 */
TEST(CityPlanTest, UneVilleSeLit) {
    const core::CityPlanResult lue = core::loadCityPlan(MONDE / "cities" / "bourg.json");
    ASSERT_TRUE(lue.ok()) << lue.error;
    const core::CityPlan& capitale = lue.plan;

    EXPECT_EQ(capitale.location, "test-city");
    ASSERT_EQ(capitale.districts.size(), 6U);
    int cartes = 0;
    int gardees = 0;
    for (const core::CityDistrict& quartier : capitale.districts) {
        (quartier.hasMap() ? cartes : gardees) += 1;
    }
    EXPECT_EQ(cartes, 2);
    EXPECT_EQ(gardees, 4);

    EXPECT_EQ(capitale.startMap(), "bourg/place");
    EXPECT_EQ(capitale.startArrival, "porte-est");
    const core::CityDistrict* const cave = capitale.districtOfMap("cave");
    ASSERT_NE(cave, nullptr);
    EXPECT_EQ(cave->id, "test-city-cave");
    EXPECT_EQ(capitale.districtOfMap("donjon"), nullptr);

    core::WorldTravel voyage{core::WorldTravel::directoryLoader(NIVEAUX)};
    EXPECT_EQ(voyage.enter(capitale.startMap(), capitale.startArrival), core::TravelResult::Moved);
}

/**
 * @brief Une sous-zone est dans son quartier : sa carte est rangee sous la sienne (D-16).
 * \castest{<b>La carte d'une sous-zone designe son quartier.</b><br/>
 * \tcat Unitaire · Graphe de la ville<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire la ville d'essai.<br/>
 * 2. Chercher le quartier de `bourg/place/crypte`, de `bourg/place/crypte/-1`, puis de
 * `bourg/placette` et `bourg/plac`.<br/>
 * \tattendu Les deux premieres sont dans le quartier de `bourg/place` (LOT-121) ; les deux
 * dernieres, qui ne font que commencer comme lui, ne sont dans aucun.
 * }
 */
TEST(CityPlanTest, UneSousZoneEstDansSonQuartier) {
    const core::CityPlanResult lue = core::loadCityPlan(MONDE / "cities" / "bourg.json");
    ASSERT_TRUE(lue.ok()) << lue.error;
    const core::CityDistrict* const place = lue.plan.districtOfMap("bourg/place");
    ASSERT_NE(place, nullptr);

    EXPECT_EQ(lue.plan.districtOfMap("bourg/place/crypte"), place);
    EXPECT_EQ(lue.plan.districtOfMap("bourg/place/crypte/-1"), place);
    EXPECT_EQ(lue.plan.districtOfMap("bourg/placette"), nullptr);
    EXPECT_EQ(lue.plan.districtOfMap("bourg/plac"), nullptr);
}

/**
 * @brief Un quartier qui a a la fois une carte et une porte gardee est refuse.
 * \castest{<b>Un quartier doit avoir soit une carte, soit une porte gardee.</b><br/>
 * \tcat Unitaire · Graphe de la ville<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire une ville dont un quartier porte `map` et `guard`, puis une autre dont un
 * quartier ne porte ni l'un ni l'autre.<br/>
 * \tattendu Les deux lectures echouent, en nommant le quartier.
 * }
 */
TEST_F(CityPlanFileTest, UnQuartierDoitAvoirSoitUneCarteSoitUnePorteGardee) {
    const core::CityPlanResult lesDeux = core::loadCityPlan(ecrire(R"({
        "id": "v", "name": "V", "location": "v",
        "start": {"district": "a", "arrival": "porte"},
        "districts": [{"id": "a", "map": "v/a", "guard": {"map": "v/a"}}]})"));
    EXPECT_FALSE(lesDeux.ok());
    EXPECT_NE(lesDeux.error.find("« a »"), std::string::npos) << lesDeux.error;

    const core::CityPlanResult aucun = core::loadCityPlan(ecrire(R"({
        "id": "v", "name": "V", "location": "v",
        "start": {"district": "a", "arrival": "porte"},
        "districts": [{"id": "a", "map": "v/a"}, {"id": "b"}]})"));
    EXPECT_FALSE(aucun.ok());
    EXPECT_NE(aucun.error.find("« b »"), std::string::npos) << aucun.error;
}

/**
 * @brief Un depart dans un quartier sans carte est refuse.
 * \castest{<b>« Nouvelle partie » doit pouvoir poser le heros : le depart a une carte.</b><br/>
 * \tcat Unitaire · Graphe de la ville<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire une ville dont le quartier de depart est ferme par une porte gardee.<br/>
 * 2. Lire un fichier absent.<br/>
 * \tattendu Les deux lectures echouent sans lever (EX-NFR-040).
 * }
 */
TEST_F(CityPlanFileTest, UnDepartDansUnQuartierSansCarteEstRefuse) {
    const core::CityPlanResult ferme = core::loadCityPlan(ecrire(R"({
        "id": "v", "name": "V", "location": "v",
        "start": {"district": "b", "arrival": "porte"},
        "districts": [{"id": "a", "map": "v/a"}, {"id": "b", "guard": {"map": "v/a"}}]})"));
    EXPECT_FALSE(ferme.ok());
    EXPECT_TRUE(ferme.plan.startMap().empty());

    core::CityPlanResult absent;
    EXPECT_NO_THROW(absent = core::loadCityPlan(dir / "inexistant.json"));
    EXPECT_FALSE(absent.ok());
}
