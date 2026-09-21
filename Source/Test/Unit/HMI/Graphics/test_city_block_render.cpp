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
 * \tetapes 1. Cadrer un ilot de 10 x 8 cases, puis un de 20 x 16, sur une carte de 48 x 40.<br/>
 * \tattendu Le point suivi est au centre de l'ilot, decale vers le haut par la hauteur des pieces ;
 * l'image du grand ilot est deux fois plus large que celle du petit, et plus haute que son seul
 * losange (LOT-96).
 * }
 */
TEST(CityBlockRenderTest, LeCadrageContientLIlot) {
    const core::IsoProjection projection(48, 40);
    const core::CityBlock petit{.name = "petit", .origin = {10, 10}, .columns = 10, .rows = 8};
    const core::CityBlock grand{.name = "grand", .origin = {10, 10}, .columns = 20, .rows = 16};

    const hmi::CityBlockFraming cadrePetit = hmi::cityBlockFraming(projection, petit);
    const hmi::CityBlockFraming cadreGrand = hmi::cityBlockFraming(projection, grand);

    EXPECT_GT(cadrePetit.pixelWidth, 0);
    EXPECT_EQ(cadreGrand.pixelWidth, 2 * cadrePetit.pixelWidth);
    const float losange = static_cast<float>(petit.columns + petit.rows) * projection.tileHeight() /
                          2.0F * hmi::Camera2D::PIXELS_PER_UNIT;
    EXPECT_GT(static_cast<float>(cadrePetit.pixelHeight), losange);

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
    const hmi::CityBlockFraming cadrage =
        hmi::cityBlockFraming(core::IsoProjection(instantane.columns, instantane.rows), *place);
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
