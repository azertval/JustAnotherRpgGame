// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_lit_render.cpp
 * @brief Le lieu **éclairé** (`LOT-1007`), sur la carte d'essai : la teinte de l'heure, le soleil
 *        sur les maillages, les ombres portées et les lumières de nuit.
 *
 * La carte d'essai (`Fixtures/Meshes`) — un îlot de murs et un toit en maillages, un sol en
 * images — rendue hors écran par le rendu du jeu aux quatre heures du lot : l'aube, midi, le
 * crépuscule, la nuit. Le mannequin d'essai s'y tient, et une lumière posée à côté de lui.
 */

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QRect>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/World/DayLight.h"
#include "Core/World/LightSource.h"
#include "Core/World/WorldClock.h"
#include "HMI/Graphics/OffscreenRender.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/SceneLighting.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/Graphics/WorldSceneRenderer.h"

namespace {

const QColor BACKGROUND(24, 26, 30);
const QSize SIZE(960, 640);

std::filesystem::path assets() {
    return std::filesystem::path(JADG_MESH_FIXTURE_DIR) / "Assets";
}

/// La carte d'essai, le mannequin debout devant l'îlot, case (5, 6).
hmi::WorldSceneSnapshot ilot(bool withMannequin = true) {
    const core::LevelLoadResult map = core::LevelLoader::loadFromFile(
        std::filesystem::path(JADG_MESH_FIXTURE_DIR) / "Levels" / "ilot.json");
    EXPECT_TRUE(map.ok()) << map.error;
    const hmi::PlaceAppearanceResult appearance =
        hmi::PlaceAppearance::loadForPlace(assets(), "ilot");
    EXPECT_TRUE(appearance.ok()) << appearance.message;
    if (!map.ok() || !appearance.ok()) {
        return {};
    }
    std::vector<hmi::WorldFigureSnapshot> figures;
    if (withMannequin) {
        figures.push_back(hmi::WorldFigureSnapshot{
            .figure = "Npc/pantin",
            .clip = "idle",
            .point = {5.5F, 6.5F},
            .seconds = 0.0F,
            // Un chemin absolu : le rendu le lit tel quel, hors du dossier des assets de la carte.
            .model = (std::filesystem::path(JADG_CHARACTER_FIXTURE_DIR) / "Assets" / "Common" /
                      "Characters" / "Mannequins" / "humanoid" / "humanoid.glb")
                         .generic_string()});
    }
    return hmi::snapshotWorldScene(*map.level, appearance.appearance, std::move(figures));
}

/// Le cadrage qui centre l'îlot, une case à 96 pixels.
hmi::WorldFraming framing(const hmi::WorldSceneSnapshot& snapshot) {
    const core::IsoProjection projection{snapshot.columns, snapshot.rows,
                                         core::ARENA_TILE_WIDTH_UNITS, snapshot.diamondRatio};
    return hmi::WorldFraming{.center = projection.gridToWorld({5.5F, 4.5F}),
                             .pixelsPerUnit = 96.0F / projection.tileWidth()};
}

/// La lumière de l'heure @p time (`HH:MM`) de la table d'usine.
hmi::WorldLighting lightingAt(const char* time, bool shadows = true) {
    return hmi::WorldLighting{
        .light = core::DayLightTable::factory().sample(core::parseClockTime(time).value_or(0.0F)),
        .shadows = shadows,
        .shadowSize = 2048,
        .seconds = 0.0F};
}

/// La luminance moyenne des pixels de @p image que @p matches reconnaît, de 0 à 255 ; −1 si
/// aucun ne l'est.
template <class Predicate>
double meanLuminance(const QImage& image, Predicate matches) {
    double sum = 0.0;
    std::size_t count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (matches(x, y, color)) {
                sum += (0.2126 * color.red()) + (0.7152 * color.green()) + (0.0722 * color.blue());
                ++count;
            }
        }
    }
    return count == 0 ? -1.0 : sum / static_cast<double>(count);
}

bool everywhere(int /*x*/, int /*y*/, const QColor& color) {
    return color != BACKGROUND;
}

void capture(const QImage& image, const char* name) {
    const std::filesystem::path captures(JADG_RENDER_CAPTURES_DIR);
    std::filesystem::create_directories(captures);
    EXPECT_TRUE(image.save(QString::fromStdWString((captures / name).wstring())));
}

}  // namespace

/**
 * @brief La carte d'essai se rend aux quatre heures du lot, et chacune a sa lumière.
 * \castest{<b>La carte d'essai se rend a l'aube, a midi, au crepuscule et la nuit.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Lumière<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Rendre la carte d'essai et son mannequin sans éclairage.<br/>
 *          2. La rendre à 06:30, 12:00, 19:00 et 00:00, avec les ombres.<br/>
 * \tattendu Les cinq images ont la taille demandée. La nuit est la plus sombre et tire sur le
 *           bleu ; l'aube et le crépuscule sont plus sombres que midi et tirent sur le rouge. La
 *           nuit reste lisible : sa luminance moyenne dépasse le tiers de celle de midi. Les
 *           captures sont écrites pour l'œil.
 * }
 */
TEST(LitRenderTest, LaCarteDEssaiSeRendAuxQuatreHeures) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneRenderer renderer(assets());
    const hmi::WorldSceneSnapshot snapshot = ilot();
    renderer.setSnapshot(snapshot);
    const auto render = [&](std::optional<hmi::WorldLighting> lighting, const char* name) {
        renderer.setLighting(std::move(lighting));
        const QImage image = offscreen->render(renderer, SIZE, framing(snapshot), BACKGROUND);
        EXPECT_EQ(image.size(), SIZE) << name;
        capture(image, name);
        return image;
    };
    const QImage unlit = render(std::nullopt, "lumiere-sans.png");
    const QImage dawn = render(lightingAt("06:30"), "lumiere-0630-aube.png");
    const QImage noon = render(lightingAt("12:00"), "lumiere-1200-midi.png");
    const QImage dusk = render(lightingAt("19:00"), "lumiere-1900-crepuscule.png");
    const QImage night = render(lightingAt("00:00"), "lumiere-0000-nuit.png");
    ASSERT_FALSE(unlit.isNull());

    const double noonLight = meanLuminance(noon, everywhere);
    const double nightLight = meanLuminance(night, everywhere);
    EXPECT_LT(meanLuminance(dawn, everywhere), noonLight);
    EXPECT_LT(meanLuminance(dusk, everywhere), noonLight);
    EXPECT_LT(nightLight, meanLuminance(dawn, everywhere));
    EXPECT_LT(nightLight, meanLuminance(dusk, everywhere));
    EXPECT_GT(nightLight, noonLight / 3.0) << "la nuit reste lisible";

    // La couleur moyenne d'une image : la nuit tire sur le bleu, le crepuscule sur le rouge.
    const auto mean = [](const QImage& image) {
        double red = 0.0;
        double blue = 0.0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                red += image.pixelColor(x, y).red();
                blue += image.pixelColor(x, y).blue();
            }
        }
        return std::pair<double, double>{red, blue};
    };
    const auto [noonRed, noonBlue] = mean(noon);
    const auto [nightRed, nightBlue] = mean(night);
    const auto [duskRed, duskBlue] = mean(dusk);
    EXPECT_GT(nightBlue / nightRed, noonBlue / noonRed);
    EXPECT_GT(duskRed / duskBlue, noonRed / noonBlue);
}

/**
 * @brief À midi, la lumière ne change pas la facture : les images sont telles que peintes, et un
 *        maillage garde ses teintes sous une lumière qui le modèle.
 * \castest{<b>A midi, la lumiere ne change pas la facture.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Lumière<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Rendre la carte d'essai, sans mannequin, sans éclairage.<br/>
 *          2. La rendre à midi, sans ombres.<br/>
 * \tattendu Le fond et le sol en images sont les mêmes, pixel pour pixel : la teinte de midi est
 *           blanche, et plus de la moitié de l'image ne change pas. Le toit en maillage garde sa
 *           teinte — le rapport de son rouge à son bleu ne bouge pas d'un dixième — et sa
 *           luminance moyenne reste à moins d'un cinquième de celle du rendu sans lumière : la
 *           lumière le modèle, elle ne le repeint pas.
 * }
 */
TEST(LitRenderTest, AMidiLaLumiereNeChangePasLaFacture) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneRenderer renderer(assets());
    const hmi::WorldSceneSnapshot snapshot = ilot(false);
    renderer.setSnapshot(snapshot);
    const QImage unlit = offscreen->render(renderer, SIZE, framing(snapshot), BACKGROUND);
    renderer.setLighting(lightingAt("12:00", false));
    const QImage noon = offscreen->render(renderer, SIZE, framing(snapshot), BACKGROUND);
    ASSERT_EQ(unlit.size(), SIZE);
    ASSERT_EQ(noon.size(), SIZE);

    std::size_t same = 0;
    for (int y = 0; y < SIZE.height(); ++y) {
        for (int x = 0; x < SIZE.width(); ++x) {
            same += unlit.pixel(x, y) == noon.pixel(x, y) ? 1U : 0U;
        }
    }
    // Le fond et le sol en images ne changent pas ; les maillages -- l'îlot et son dallage --
    // tiennent le reste.
    EXPECT_GT(same, static_cast<std::size_t>(SIZE.width() * SIZE.height()) / 2U);

    // Le bourgogne des tuiles du toit, dans l'une et l'autre image.
    const auto isRoof = [](int /*x*/, int /*y*/, const QColor& color) {
        return color.red() > 90 && color.green() < 80 && color.blue() < 100 &&
               color.red() > color.blue() + 40;
    };
    const double unlitRoof = meanLuminance(unlit, isRoof);
    const double noonRoof = meanLuminance(noon, isRoof);
    ASSERT_GT(unlitRoof, 0.0);
    ASSERT_GT(noonRoof, 0.0);
    EXPECT_NEAR(noonRoof / unlitRoof, 1.0, 0.2);
    const auto redToBlue = [&isRoof](const QImage& image) {
        double red = 0.0;
        double blue = 0.0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor color = image.pixelColor(x, y);
                if (isRoof(x, y, color)) {
                    red += color.red();
                    blue += color.blue();
                }
            }
        }
        return red / blue;
    };
    EXPECT_NEAR(redToBlue(noon) / redToBlue(unlit), 1.0, 0.1);
}

/**
 * @brief Le mannequin et l'îlot jettent une ombre sur le sol, que le réglage des ombres retire.
 * \castest{<b>Les maillages jettent une ombre, que le reglage retire.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Lumière<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Rendre la carte d'essai et son mannequin à 16:00, ombres éteintes.<br/>
 *          2. La rendre à la même heure, ombres allumées, à 2048 puis à 512 texels.<br/>
 *          3. La rendre de nouveau ombres éteintes.<br/>
 * \tattendu Avec les ombres, plus de deux mille pixels sont plus sombres qu'à l'étape 1, et
 *           aucun n'est plus clair : une ombre ne fait qu'assombrir. La carte de 512 texels ombre
 *           la même surface, à un cinquième près. Ombres éteintes, l'image est celle de l'étape
 *           1, pixel pour pixel.
 * }
 */
TEST(LitRenderTest, LesMaillagesJettentUneOmbreQueLeReglageRetire) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneRenderer renderer(assets());
    const hmi::WorldSceneSnapshot snapshot = ilot();
    renderer.setSnapshot(snapshot);
    const auto render = [&](bool shadows, int size) {
        hmi::WorldLighting lighting = lightingAt("16:00", shadows);
        lighting.shadowSize = size;
        renderer.setLighting(lighting);
        return offscreen->render(renderer, SIZE, framing(snapshot), BACKGROUND);
    };
    const QImage flat = render(false, 2048);
    const QImage shaded = render(true, 2048);
    const QImage coarse = render(true, 512);
    const QImage again = render(false, 2048);
    ASSERT_EQ(flat.size(), SIZE);
    capture(shaded, "lumiere-1600-ombres.png");

    const auto darker = [&flat](const QImage& image, std::size_t& brighter) {
        std::size_t count = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const int before = qGray(flat.pixel(x, y));
                const int after = qGray(image.pixel(x, y));
                count += after < before - 8 ? 1U : 0U;
                brighter += after > before + 2 ? 1U : 0U;
            }
        }
        return count;
    };
    std::size_t brighter = 0;
    const std::size_t fine = darker(shaded, brighter);
    EXPECT_GT(fine, 2000U) << "l'îlot et le mannequin jettent une ombre";
    EXPECT_EQ(brighter, 0U) << "une ombre ne fait qu'assombrir";
    std::size_t ignored = 0;
    const std::size_t rough = darker(coarse, ignored);
    EXPECT_NEAR(static_cast<double>(rough), static_cast<double>(fine),
                static_cast<double>(fine) / 5.0);
    EXPECT_EQ(again, flat);
}

/**
 * @brief Une lumière de nuit éclaire autour d'elle, la nuit seulement — sauf si elle est
 *        toujours allumée.
 * \castest{<b>Une lumiere de nuit eclaire autour d'elle, la nuit.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Lumière<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Rendre la carte d'essai à minuit, sans source.<br/>
 *          2. Y poser une lanterne en (2, 6), de 4,5 m de portée, et la rendre à minuit.<br/>
 *          3. La rendre à midi ; puis à midi, la lanterne toujours allumée.<br/>
 * \tattendu À minuit, le sol sous la lanterne est nettement plus clair qu'à l'étape 1, et plus
 *           chaud ; à l'autre bout de la carte, hors de sa portée, rien n'a changé. À midi, la
 *           lanterne est éteinte : l'image est celle de midi sans source. Toujours allumée, elle
 *           éclaire en plein jour.
 * }
 */
TEST(LitRenderTest, UneLumiereDeNuitEclaireAutourDElle) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneRenderer renderer(assets());
    const hmi::WorldSceneSnapshot bare = ilot(false);
    hmi::WorldSceneSnapshot lit = bare;
    lit.lights.push_back(core::LightSource{.column = 2.5F,
                                           .row = 6.5F,
                                           .emission = {.color = {1.0F, 0.75F, 0.48F},
                                                        .radius = 4.5F,
                                                        .height = 2.2F,
                                                        .intensity = 1.0F,
                                                        .flicker = false,
                                                        .always = false}});
    hmi::WorldSceneSnapshot always = lit;
    always.lights.front().emission.always = true;
    const auto render = [&](const hmi::WorldSceneSnapshot& snapshot, const char* time) {
        renderer.setSnapshot(snapshot);
        renderer.setLighting(lightingAt(time, false));
        return offscreen->render(renderer, SIZE, framing(snapshot), BACKGROUND);
    };
    const QImage dark = render(bare, "00:00");
    const QImage lamp = render(lit, "00:00");
    const QImage noon = render(bare, "12:00");
    const QImage lampAtNoon = render(lit, "12:00");
    const QImage alwaysAtNoon = render(always, "12:00");
    ASSERT_EQ(lamp.size(), SIZE);
    capture(lamp, "lumiere-0000-lanterne.png");

    // Le pixel du sol sous la lanterne : sa case, projetée et cadrée comme l'image.
    const core::IsoProjection projection{bare.columns, bare.rows, core::ARENA_TILE_WIDTH_UNITS,
                                         bare.diamondRatio};
    const hmi::WorldFraming frame = framing(bare);
    const auto pixelOf = [&projection, &frame](core::Vector2 gridPoint) {
        const core::Vector2 world = projection.gridToWorld(gridPoint);
        return QPoint(static_cast<int>(((world.x - frame.center.x) * frame.pixelsPerUnit) +
                                       (static_cast<float>(SIZE.width()) / 2.0F)),
                      static_cast<int>(((world.y - frame.center.y) * frame.pixelsPerUnit) +
                                       (static_cast<float>(SIZE.height()) / 2.0F)));
    };
    const QPoint under = pixelOf({2.5F, 6.5F});
    ASSERT_TRUE(QRect(QPoint(0, 0), SIZE).contains(under));
    const auto around = [under](int x, int y, const QColor& /*color*/) {
        return std::abs(x - under.x()) <= 12 && std::abs(y - under.y()) <= 8;
    };
    EXPECT_GT(meanLuminance(lamp, around), meanLuminance(dark, around) * 1.4);
    const QColor warm = lamp.pixelColor(under);
    const QColor cold = dark.pixelColor(under);
    EXPECT_GT(warm.red() - warm.blue(), cold.red() - cold.blue())
        << "la lanterne réchauffe la nuit bleue";

    // Loin d'elle, rien n'a changé : l'autre bout de la carte, à plus de sa portée.
    const QPoint away = pixelOf({9.5F, 0.5F});
    ASSERT_TRUE(QRect(QPoint(0, 0), SIZE).contains(away));
    EXPECT_EQ(lamp.pixel(away), dark.pixel(away));

    EXPECT_EQ(lampAtNoon, noon) << "à midi, une lanterne est éteinte";
    EXPECT_NE(alwaysAtNoon, noon) << "toujours allumée, elle éclaire en plein jour";
}
