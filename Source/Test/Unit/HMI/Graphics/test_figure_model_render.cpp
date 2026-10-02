// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_figure_model_render.cpp
 * @brief Une figurine en modèle, animée par ses os, rendue hors écran (`LOT-1005`).
 *
 * Le pantin de la carte d'essai (`Fixtures/Meshes`) — trois blocs verts, trois os, six clips —
 * posé sur la carte d'essai en maillages et rendu par le rendu du jeu, sur un `QRhi` sans fenêtre.
 * Rien d'autre n'y est vert : ce que l'image montre de lui se compte et se mesure, pixel par pixel.
 */

#include <QColor>
#include <QImage>
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <memory>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "HMI/Graphics/OffscreenRender.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/Graphics/WorldSceneRenderer.h"

namespace {

const QColor BACKGROUND(24, 26, 30);
const QSize SIZE(640, 480);
constexpr const char* MODEL = "Npc/pantin/pantin.glb";

std::filesystem::path assets() {
    return std::filesystem::path(JADG_MESH_FIXTURE_DIR) / "Assets";
}

/// La carte d'essai, avec les figurines @p figures et sans celles de ses PNJ.
hmi::WorldSceneSnapshot ilot(std::vector<hmi::WorldFigureSnapshot> figures) {
    const core::LevelLoadResult map = core::LevelLoader::loadFromFile(
        std::filesystem::path(JADG_MESH_FIXTURE_DIR) / "Levels" / "ilot.json");
    EXPECT_TRUE(map.ok()) << map.error;
    const hmi::PlaceAppearanceResult appearance =
        hmi::PlaceAppearance::loadForPlace(assets(), "ilot");
    EXPECT_TRUE(appearance.ok()) << appearance.message;
    if (!map.ok() || !appearance.ok()) {
        return {};
    }
    return hmi::snapshotWorldScene(*map.level, appearance.appearance, std::move(figures));
}

/// Le cadrage qui centre l'îlot, une case à 64 pixels.
hmi::WorldFraming framing(const hmi::WorldSceneSnapshot& snapshot) {
    const core::IsoProjection projection{snapshot.columns, snapshot.rows,
                                         core::ARENA_TILE_WIDTH_UNITS, snapshot.diamondRatio};
    return hmi::WorldFraming{.center = projection.gridToWorld({5.5F, 3.5F}),
                             .pixelsPerUnit = 64.0F / projection.tileWidth()};
}

/// Le pantin au centre de la case (@p column, @p row), à l'instant @p seconds de @p clip.
hmi::WorldFigureSnapshot puppet(int column, int row, std::string clip, float seconds) {
    return hmi::WorldFigureSnapshot{
        .figure = "Npc/pantin",
        .clip = std::move(clip),
        .point = {static_cast<float>(column) + 0.5F, static_cast<float>(row) + 0.5F},
        .seconds = seconds,
        .model = MODEL};
}

/// Le vert feuillage du pantin : plus de vert que de rouge et de bleu.
bool isPuppet(const QColor& color) {
    return color.green() > color.red() + 25 && color.green() > color.blue() + 25;
}

/// Ce que l'image montre du pantin : son nombre de pixels, et la boîte qui les contient.
struct Seen {
    std::size_t pixels = 0;
    int top = 0;
    int bottom = 0;
    int left = 0;
    int right = 0;

    [[nodiscard]] int height() const {
        return pixels == 0 ? 0 : bottom - top + 1;
    }
    [[nodiscard]] int width() const {
        return pixels == 0 ? 0 : right - left + 1;
    }
};

Seen seen(const QImage& image) {
    Seen out;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (!isPuppet(image.pixelColor(x, y))) {
                continue;
            }
            if (out.pixels++ == 0) {
                out.top = out.bottom = y;
                out.left = out.right = x;
            }
            out.top = std::min(out.top, y);
            out.bottom = std::max(out.bottom, y);
            out.left = std::min(out.left, x);
            out.right = std::max(out.right, x);
        }
    }
    return out;
}

void capture(const QImage& image, const char* name) {
    const std::filesystem::path captures(JADG_RENDER_CAPTURES_DIR);
    std::filesystem::create_directories(captures);
    EXPECT_TRUE(image.save(QString::fromStdWString((captures / name).wstring())));
}

/// Rend la carte d'essai avec @p figure, et dit ce que l'image montre du pantin.
Seen renderPuppet(hmi::OffscreenRhi& offscreen, hmi::WorldSceneRenderer& renderer,
                  hmi::WorldFigureSnapshot figure, const char* name = nullptr) {
    const hmi::WorldSceneSnapshot snapshot = ilot({std::move(figure)});
    renderer.setSnapshot(snapshot);
    const QImage image = offscreen.render(renderer, SIZE, framing(snapshot), BACKGROUND);
    EXPECT_EQ(image.size(), SIZE);
    if (name != nullptr) {
        capture(image, name);
    }
    return seen(image);
}

}  // namespace

/**
 * @brief Le modèle d'essai se charge avec son squelette et se dessine debout ; à l'image clé de
 *        son attaque il est penché, à la fin de sa chute il est couché et y reste.
 * \castest{<b>Le modele d'essai se dessine et joue ses clips par ses os.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Squelette<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Rendre la carte d'essai, le pantin au repos devant l'îlot.<br/>
 *          2. Le rendre à 0,4 s de son attaque : le buste penché de 60° vers la caméra.<br/>
 *          3. Le tourner vers la droite de l'écran et le rendre à la fin de sa chute, puis
 *             trente secondes plus tard.<br/>
 * \tattendu Le modèle est chargé avec son squelette et sa description, aucune bande n'est
 *           demandée pour lui. Au repos il fait 1,80 m de haut à l'écran (cosinus de l'élévation
 *           compris). En attaque son sommet est plus bas d'un cinquième au moins et ses pieds
 *           n'ont pas bougé. Couché, il s'étend vers la gauche de l'écran, deux fois plus large
 *           que haut, et l'image trente secondes plus tard est la même.
 * }
 */
TEST(FigureModelRenderTest, LeModeleDEssaiSeDessineEtJoueSesClips) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneRenderer renderer(assets());
    const Seen idle =
        renderPuppet(*offscreen, renderer, puppet(5, 6, "idle", 0.0F), "pantin-repos.png");

    const hmi::SceneFigureModel* const model = renderer.textures().findFigure(MODEL);
    ASSERT_NE(model, nullptr);
    ASSERT_NE(model->rig, nullptr);
    EXPECT_EQ(model->rig->joints.size(), 3U);
    ASSERT_NE(model->skeleton, nullptr);
    EXPECT_EQ(model->skeleton->silhouette, "pantin");
    for (const std::string& path : renderer.requested()) {
        EXPECT_FALSE(path.starts_with("Npc/pantin/") && path.ends_with(".png")) << path;
    }
    ASSERT_GT(idle.pixels, 300U);
    // 1,80 m sous une caméra élevée de asin 0,62 : 1,80 x cos = 1,41 m, à 64 px pour 2,12 m, plus
    // la profondeur du socle (0,4 m vus de dessus).
    EXPECT_GT(idle.height(), 40);
    EXPECT_LT(idle.height(), 62);

    const Seen strike =
        renderPuppet(*offscreen, renderer, puppet(5, 6, "attack", 0.4F), "pantin-attaque.png");
    ASSERT_GT(strike.pixels, 300U);
    EXPECT_GT(strike.top, idle.top + (idle.height() / 5)) << "le buste est penché";
    EXPECT_LE(std::abs(strike.left - idle.left), 1) << "les jambes n'ont pas bougé";

    // Tourné vers la droite de l'écran (les colonnes croissent, les lignes décroissent), il tombe
    // en arrière : vers la gauche.
    hmi::WorldFigureSnapshot falling = puppet(5, 6, "death", 0.8F);
    falling.heading = -std::numbers::pi_v<float> / 4.0F;
    hmi::WorldFigureSnapshot still = falling;
    still.seconds = 30.0F;
    const Seen fallen = renderPuppet(*offscreen, renderer, falling, "pantin-chute.png");
    const Seen later = renderPuppet(*offscreen, renderer, still);
    ASSERT_GT(fallen.pixels, 300U);
    EXPECT_GT(fallen.width(), fallen.height() * 2) << "couché en travers de l'image";
    EXPECT_LT(fallen.left, idle.left - 20) << "tombé vers la gauche";
    EXPECT_EQ(fallen.pixels, later.pixels) << "un mort ne se relève pas";
    EXPECT_EQ(fallen.left, later.left);

    std::cout << "pantin : " << idle.pixels << " px au repos (haut de " << idle.height() << "), "
              << strike.pixels << " en attaque, " << fallen.pixels << " couche (haut de "
              << fallen.height() << ")\n";
}

/**
 * @brief Un modèle se départage du décor par la profondeur : un mur le cache, et ni un sol en
 *        image ni un sol en maillage ne lui rogne les pieds.
 * \castest{<b>Un modele passe derriere un mur, et se tient entier sur un sol en image.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Squelette<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Rendre le pantin devant l'îlot, sur la cour en maillages (5, 5).<br/>
 *          2. Le rendre derrière l'îlot (5, 1).<br/>
 *          3. Le rendre sur le sol en images, hors de la cour (1, 6).<br/>
 * \tattendu Derrière, moins d'un quart de lui paraît : le haut de sa tête, qui dépasse la rive du
 *           toit sous cette caméra, et rien d'autre. Sur le sol en images, il montre autant de
 *           pixels que sur le sol en maillages, à cinq pour cent près : le sol ne passe pas
 *           devant ses pieds.
 * }
 */
TEST(FigureModelRenderTest, UnModeleSeDepartageParLaProfondeur) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneRenderer renderer(assets());
    const Seen front = renderPuppet(*offscreen, renderer, puppet(5, 5, "idle", 0.0F));
    const Seen behind =
        renderPuppet(*offscreen, renderer, puppet(5, 1, "idle", 0.0F), "pantin-derriere.png");
    const Seen paved =
        renderPuppet(*offscreen, renderer, puppet(1, 6, "idle", 0.0F), "pantin-sol-image.png");
    std::cout << "pantin : " << front.pixels << " px devant, " << behind.pixels << " derriere, "
              << paved.pixels << " sur le sol en images\n";
    ASSERT_GT(front.pixels, 300U);
    EXPECT_LT(behind.pixels, front.pixels / 4);
    EXPECT_LT(behind.height(), front.height() / 3) << "seule sa tête dépasse du toit";
    EXPECT_GT(paved.pixels, (front.pixels * 95) / 100);
    EXPECT_LT(paved.pixels, (front.pixels * 105) / 100);
    EXPECT_NEAR(paved.height(), front.height(), 1);
}

/**
 * @brief Une figurine en bandes et un modèle tiennent sur la même carte ; sans modèle, la carte se
 *        rend comme avant le lot.
 * \castest{<b>Bandes et modele cohabitent ; sans modele, l'image est celle d'avant.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Squelette<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Rendre la carte d'essai avec la figurine témoin, en bandes.<br/>
 *          2. La rendre avec la figurine témoin et le pantin.<br/>
 *          3. La rendre de nouveau avec la figurine témoin seule.<br/>
 * \tattendu Avec le pantin, l'image compte un maillage animé de plus et montre le pantin ; hors de
 *           lui, la figurine témoin se voit comme avant. La troisième image est la première, au
 *           pixel.
 * }
 */
TEST(FigureModelRenderTest, BandesEtModeleCohabitent) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    const hmi::WorldFigureSnapshot witness{.figure = "Npc/temoin", .point = {2.5F, 6.5F}};
    hmi::WorldSceneRenderer renderer(assets());

    const hmi::WorldSceneSnapshot alone = ilot({witness});
    renderer.setSnapshot(alone);
    const QImage before = offscreen->render(renderer, SIZE, framing(alone), BACKGROUND);
    const std::size_t meshesBefore = renderer.composed().meshes().size();
    EXPECT_EQ(seen(before).pixels, 0U);

    const hmi::WorldSceneSnapshot both = ilot({witness, puppet(7, 6, "walk", 0.125F)});
    renderer.setSnapshot(both);
    const QImage together = offscreen->render(renderer, SIZE, framing(both), BACKGROUND);
    capture(together, "pantin-et-temoin.png");
    EXPECT_EQ(renderer.composed().meshes().size(), meshesBefore + 1);
    EXPECT_GT(seen(together).pixels, 300U);

    renderer.setSnapshot(alone);
    const QImage after = offscreen->render(renderer, SIZE, framing(alone), BACKGROUND);
    ASSERT_EQ(before.size(), after.size());
    std::size_t differing = 0;
    for (int y = 0; y < before.height(); ++y) {
        for (int x = 0; x < before.width(); ++x) {
            differing += before.pixel(x, y) != after.pixel(x, y) ? 1U : 0U;
        }
    }
    EXPECT_EQ(differing, 0U);
}

/**
 * @brief Le mannequin d'essai — 53 os, lié par la chaîne de l'atelier — marche sur la carte
 *        d'essai : il se charge, se pose à chaque instant et se dessine.
 * \castest{<b>Le mannequin d'essai marche sur la carte d'essai.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Squelette<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Rendre la carte d'essai sans figurine.<br/>2. La rendre avec le mannequin d'essai
 *          a deux instants de sa marche, puis a la fin de sa chute.<br/>
 * \tattendu Le modele est charge avec 53 os ; chaque image montre le mannequin (plus de trois
 *           cents pixels changent par rapport a la carte nue) ; les deux instants de marche
 *           different entre eux. Les captures sont ecrites pour l'oeil.
 * }
 */
TEST(FigureModelRenderTest, LeMannequinDEssaiMarche) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    // Un chemin absolu : le rendu le lit tel quel, hors du dossier des assets de la carte.
    const std::string model = (std::filesystem::path(JADG_CHARACTER_FIXTURE_DIR) / "Assets" /
                               "Common" / "Characters" / "Mannequins" / "humanoid" / "humanoid.glb")
                                  .generic_string();
    const auto mannequin = [&model](std::string clip, float seconds) {
        hmi::WorldFigureSnapshot figure = puppet(5, 6, std::move(clip), seconds);
        figure.model = model;
        return figure;
    };
    const auto differing = [](const QImage& a, const QImage& b) {
        std::size_t count = 0;
        for (int y = 0; y < a.height(); ++y) {
            for (int x = 0; x < a.width(); ++x) {
                count += a.pixel(x, y) != b.pixel(x, y) ? 1U : 0U;
            }
        }
        return count;
    };
    hmi::WorldSceneRenderer renderer(assets());
    const auto render = [&](std::vector<hmi::WorldFigureSnapshot> figures, const char* name) {
        const hmi::WorldSceneSnapshot snapshot = ilot(std::move(figures));
        renderer.setSnapshot(snapshot);
        // Une case a 128 pixels : le mannequin y fait 85 pixels de haut.
        hmi::WorldFraming frame = framing(snapshot);
        frame.pixelsPerUnit *= 2.0F;
        const QImage image = offscreen->render(renderer, SIZE, frame, BACKGROUND);
        if (name != nullptr) {
            capture(image, name);
        }
        return image;
    };
    const QImage bare = render({}, nullptr);
    const QImage first = render({mannequin("walk", 0.0F)}, "mannequin-marche-0.png");
    const hmi::SceneFigureModel* const loaded = renderer.textures().findFigure(model);
    ASSERT_NE(loaded, nullptr);
    ASSERT_NE(loaded->rig, nullptr);
    EXPECT_EQ(loaded->rig->joints.size(), 53U);
    const QImage second = render({mannequin("walk", 0.25F)}, "mannequin-marche-1.png");
    const QImage fallen = render({mannequin("death", 5.0F)}, "mannequin-chute.png");
    EXPECT_GT(differing(bare, first), 300U);
    EXPECT_GT(differing(bare, second), 300U);
    EXPECT_GT(differing(bare, fallen), 300U);
    EXPECT_GT(differing(first, second), 100U) << "les appuis ont change";
}
