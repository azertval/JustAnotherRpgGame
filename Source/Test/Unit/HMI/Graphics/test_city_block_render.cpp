// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_city_block_render.cpp
 * @brief L'ilot vu sur le plan (LOT-96) : son cadrage, et son image hors ecran.
 */

#include <QColor>
#include <QImage>
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/World/CityBlock.h"
#include "HMI/Graphics/Camera2D.h"
#include "HMI/Graphics/CityBlockRender.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

const std::filesystem::path NIVEAUX{std::filesystem::path(JADG_TEST_DATA_DIR) / "Levels"};
const std::filesystem::path ASSETS{std::filesystem::path(JADG_TEST_DATA_DIR) / "Assets"};

}  // namespace

/**
 * @brief Le cadrage d'un ilot le contient, et grandit avec lui.
 * \castest{<b>Le cadrage d'un ilot couvre son losange englobant, plus la hauteur des
 * pieces.</b><br/>
 * \tcat Unitaire · Plan de la ville<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Cadrer un ilot de 10 x 8 cases, puis un de 20 x 16, sur une carte de 48 x 40, pour
 * un lieu dont la piece la plus haute s'eleve d'une case et demie.<br/>
 * \tattendu Le point suivi est au centre de l'ilot, decale vers le haut par la hauteur des pieces ;
 * l'image du grand ilot est deux fois plus large que celle du petit ; a 100 pixels par case, la
 * hauteur est celle du losange plus l'elevation lue dans le manifeste (LOT-96, LOT-103).
 * }
 */
TEST(CityBlockRenderTest, LeCadrageContientLIlot) {
    const core::IsoProjection projection(48, 40);
    const core::CityBlock petit{.name = "petit", .origin = {10, 10}, .columns = 10, .rows = 8};
    const core::CityBlock grand{.name = "grand", .origin = {10, 10}, .columns = 20, .rows = 16};

    // La piece la plus haute du lieu s'eleve d'une case et demie au-dessus de la sienne.
    constexpr float ELEVATION = 1.5F;
    const hmi::CityBlockFraming cadrePetit = hmi::cityBlockFraming(projection, petit, ELEVATION);
    const hmi::CityBlockFraming cadreGrand = hmi::cityBlockFraming(projection, grand, ELEVATION);

    EXPECT_GT(cadrePetit.pixelWidth, 0);
    EXPECT_EQ(cadreGrand.pixelWidth, 2 * cadrePetit.pixelWidth);
    // A 100 pixels par case (CITY_BLOCK_TILE_PIXELS), le losange de l'ilot, plus l'elevation.
    const float pixelsParUnite = hmi::CITY_BLOCK_TILE_PIXELS / projection.tileWidth();
    const float losange = static_cast<float>(petit.columns + petit.rows) * projection.tileHeight() /
                          2.0F * pixelsParUnite;
    EXPECT_NEAR(static_cast<float>(cadrePetit.pixelHeight),
                losange + (ELEVATION * hmi::CITY_BLOCK_TILE_PIXELS), 1.0F);
    EXPECT_NEAR(static_cast<float>(cadrePetit.pixelWidth),
                static_cast<float>(petit.columns + petit.rows) / 2.0F * hmi::CITY_BLOCK_TILE_PIXELS,
                1.0F);

    // Le centre de l'ilot est (15, 14) ; la hauteur des pieces remonte le point suivi.
    EXPECT_LT(cadrePetit.focus.x + cadrePetit.focus.y, 15.0F + 14.0F);
    EXPECT_NEAR(cadrePetit.focus.x - cadrePetit.focus.y, 15.0F - 14.0F, 0.01F);
}

/**
 * @brief L'ilot d'un lieu devient une image, dessinee par le rendu du lieu.
 * \castest{<b>Un ilot d'un lieu se dessine hors ecran, a la taille de son cadrage.</b><br/>
 * \tcat Unitaire · Plan de la ville<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Charger une carte de ville, sa table d'apparence et son plus grand ilot.<br/>
 * 2. Le dessiner hors ecran.<br/>
 * \tattendu Une image a la taille du cadrage, dont une part notable n'est pas le fond : l'ilot
 * est la carte telle que le jeu la dessine, pas une image a part (LOT-96).
 * }
 */
TEST(CityBlockRenderTest, UnIlotDevientUneImage) {
    const core::LevelLoadResult lu =
        core::LevelLoader::loadFromFile(NIVEAUX / "bourg" / "place.json");
    ASSERT_TRUE(lu.ok()) << lu.error;
    const hmi::PlaceAppearanceResult table =
        hmi::PlaceAppearance::loadFromFile(ASSETS / "Scene" / "bourg" / "appearance.json");
    ASSERT_TRUE(table.ok()) << table.message;
    const std::vector<core::CityBlock> ilots = core::cityBlocksOf(*lu.level);
    const auto place = std::ranges::find(ilots, std::string{"grand-place"}, &core::CityBlock::name);
    ASSERT_NE(place, ilots.end());

    const hmi::WorldSceneSnapshot instantane =
        hmi::snapshotWorldScene(*lu.level, table.appearance, {});
    const QImage image = hmi::renderCityBlock(ASSETS, instantane, *place);
    if (image.isNull()) {
        GTEST_SKIP() << "Aucune interface QRhi hors ecran sur cette machine.";
    }
    EXPECT_GT(instantane.maximumRise, 0.0F) << "l'elevation des pieces se lit dans le manifeste";
    const hmi::CityBlockFraming cadrage = hmi::cityBlockFraming(
        core::IsoProjection(instantane.columns, instantane.rows, core::ARENA_TILE_WIDTH_UNITS,
                            instantane.diamondRatio),
        *place, instantane.maximumRise);
    EXPECT_EQ(image.width(), cadrage.pixelWidth);
    EXPECT_EQ(image.height(), cadrage.pixelHeight);

    const QColor fond = image.pixelColor(0, 0);
    std::size_t peints = 0;
    for (int y = 0; y < image.height(); y += 4) {
        for (int x = 0; x < image.width(); x += 4) {
            peints += image.pixelColor(x, y) != fond ? 1U : 0U;
        }
    }
    const std::size_t echantillons = static_cast<std::size_t>((image.width() + 3) / 4) *
                                     static_cast<std::size_t>((image.height() + 3) / 4);
    EXPECT_GT(peints * 3, echantillons) << "l'ilot est presque vide";
}
