// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_offscreen_render.cpp
 * @brief Ce que l'éditeur demande au rendu du jeu (`LOT-1002`) : un cadrage imposé, une opacité par
 *        primitive, la carte préparée avant l'image, et une image hors écran rendue par tuiles.
 *
 * Le canevas de l'éditeur, `LevelEditor --render` et les vignettes ne peignent plus la scène : ils
 * la font dessiner par `hmi::WorldSceneRenderer`. Ces tests tiennent les trois réglages que
 * l'éditeur y ajoute, sur le donjon d'essai, par un `QRhi` Direct3D 11 sans fenêtre.
 */

#include <QColor>
#include <QImage>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileMap.h"
#include "HMI/Graphics/MaquetteTokens.h"
#include "HMI/Graphics/OffscreenRender.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/Graphics/WorldSceneRenderer.h"

namespace {

const QColor BACKGROUND(24, 26, 30);

std::filesystem::path assets() {
    return std::filesystem::path(JADG_TEST_DATA_DIR) / "Assets";
}

/// Le donjon d'essai, ses sentinelles comprises, sans héros : ce que le canevas montre.
hmi::WorldSceneSnapshot donjon() {
    const core::LevelLoadResult carte = core::LevelLoader::loadFromFile(
        std::filesystem::path(JADG_TEST_DATA_DIR) / "Levels" / "donjon.json");
    EXPECT_TRUE(carte.ok()) << carte.error;
    const hmi::PlaceAppearanceResult table =
        hmi::PlaceAppearance::loadFromFile(assets() / "Scene" / "bourg" / "appearance.json");
    EXPECT_TRUE(table.ok()) << table.message;
    if (!carte.ok() || !table.ok()) {
        return {};
    }
    return hmi::snapshotWorldScene(*carte.level, table.appearance,
                                   hmi::npcFigures(carte.level->entities(), 0));
}

core::IsoProjection projectionOf(const hmi::WorldSceneSnapshot& snapshot) {
    return {snapshot.columns, snapshot.rows, core::ARENA_TILE_WIDTH_UNITS, snapshot.diamondRatio};
}

/// Le cadrage qui montre le milieu du donjon, une case à 40 pixels.
hmi::WorldFraming milieu(const hmi::WorldSceneSnapshot& snapshot) {
    const core::IsoProjection projection = projectionOf(snapshot);
    return hmi::WorldFraming{
        .center = {projection.sceneSize().x / 2.0F, projection.sceneSize().y / 2.0F},
        .pixelsPerUnit = 40.0F / projection.tileWidth()};
}

std::size_t differingPixels(const QImage& a, const QImage& b) {
    if (a.size() != b.size()) {
        return static_cast<std::size_t>(-1);
    }
    std::size_t differing = 0;
    for (int y = 0; y < a.height(); ++y) {
        for (int x = 0; x < a.width(); ++x) {
            differing += a.pixel(x, y) != b.pixel(x, y) ? 1U : 0U;
        }
    }
    return differing;
}

}  // namespace

/**
 * @brief Un cadrage imposé remplace la caméra qui suit le héros, et la rend quand on le retire.
 * \castest{<b>Un cadrage impose remplace la camera qui suit le heros.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Editeur<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Construire la camera d'un cadrage : un centre, une echelle.<br/>
 *          2. Rendre le donjon cadre sur le heros, puis par un cadrage impose, puis de nouveau
 *             sans.<br/>
 * \tattendu La camera du cadrage a son centre et son echelle, quelle que soit la cible ; l'image
 *           cadree differe de l'image qui suit le heros ; le cadrage retire, l'image d'avant
 *           revient au pixel.
 * }
 */
TEST(OffscreenRenderTest, UnCadrageImposeRemplaceLaCameraQuiSuitLeHeros) {
    const hmi::WorldFraming framing{.center = {12.5F, -3.0F}, .pixelsPerUnit = 24.0F};
    const hmi::PlaceCamera camera = hmi::framedCamera(framing, 800, 600);
    EXPECT_FLOAT_EQ(camera.center().x, 12.5F);
    EXPECT_FLOAT_EQ(camera.center().y, -3.0F);
    EXPECT_FLOAT_EQ(camera.zoom() * hmi::PlaceCamera::PIXELS_PER_UNIT, 24.0F);
    // Le centre du monde tombe au centre de la cible.
    EXPECT_FLOAT_EQ(camera.worldToScreen(framing.center).x, 400.0F);
    EXPECT_FLOAT_EQ(camera.worldToScreen(framing.center).y, 300.0F);

    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    EXPECT_EQ(hmi::OffscreenRhi::shared(), offscreen) << "une seule interface tant qu'on la tient";

    hmi::WorldSceneRenderer renderer(assets());
    const hmi::WorldSceneSnapshot snapshot = donjon();
    renderer.setSnapshot(snapshot);
    const QSize size(480, 360);
    const QImage framed = offscreen->render(renderer, size, milieu(snapshot), BACKGROUND);
    ASSERT_EQ(framed.size(), size);
    EXPECT_FALSE(renderer.framing().has_value()) << "le cadrage du rendu est retabli";

    // Un autre cadrage : une autre image, et la premiere revient avec le premier cadrage.
    hmi::WorldFraming shifted = milieu(snapshot);
    shifted.center.x += 2.0F * projectionOf(snapshot).tileWidth();
    const QImage moved = offscreen->render(renderer, size, shifted, BACKGROUND);
    EXPECT_GT(differingPixels(framed, moved), 1000U);
    const QImage again = offscreen->render(renderer, size, milieu(snapshot), BACKGROUND);
    EXPECT_EQ(differingPixels(framed, again), 0U);
}

/**
 * @brief L'opacité des calques de l'éditeur est un paramètre du rendu : une bande éteinte ne se
 *        dessine pas, une bande entière ne change rien.
 * \castest{<b>L'opacite des calques est un parametre du rendu.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Editeur<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Rendre le donjon sans reglage.<br/>
 *          2. Le rendre avec une opacite de 1 pour toute primitive.<br/>
 *          3. Le rendre le relief eteint, puis le relief a demi.<br/>
 * \tattendu L'opacite de 1 rend la meme image au pixel ; le relief eteint, plus aucune primitive
 *           du calque Object n'est composee et l'image change ; a demi, les primitives du relief
 *           portent un alpha de moitie et l'image differe des deux autres.
 * }
 */
TEST(OffscreenRenderTest, LOpaciteDesCalquesEstUnParametreDuRendu) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneRenderer renderer(assets());
    const hmi::WorldSceneSnapshot snapshot = donjon();
    renderer.setSnapshot(snapshot);
    const QSize size(480, 360);
    const hmi::WorldFraming framing = milieu(snapshot);

    const QImage plain = offscreen->render(renderer, size, framing, BACKGROUND);
    const auto relief = [](const hmi::WorldSceneRenderer& rendu) {
        std::size_t count = 0;
        for (const hmi::ComposedQuad& quad : rendu.composed().quads()) {
            count += quad.layer == hmi::RenderLayer::Object ? 1U : 0U;
        }
        return count;
    };
    ASSERT_GT(relief(renderer), 0U) << "le donjon a des murs sous ce cadrage";

    renderer.setQuadOpacity([](const hmi::ComposedQuad&) { return 1.0F; });
    EXPECT_EQ(differingPixels(plain, offscreen->render(renderer, size, framing, BACKGROUND)), 0U);

    renderer.setQuadOpacity([](const hmi::ComposedQuad& quad) {
        return quad.layer == hmi::RenderLayer::Object ? 0.0F : 1.0F;
    });
    const QImage hidden = offscreen->render(renderer, size, framing, BACKGROUND);
    EXPECT_EQ(relief(renderer), 0U);
    EXPECT_GT(differingPixels(plain, hidden), 1000U);

    renderer.setQuadOpacity([](const hmi::ComposedQuad& quad) {
        return quad.layer == hmi::RenderLayer::Object ? 0.5F : 1.0F;
    });
    const QImage half = offscreen->render(renderer, size, framing, BACKGROUND);
    for (const hmi::ComposedQuad& quad : renderer.composed().quads()) {
        if (quad.layer == hmi::RenderLayer::Object && quad.kind == hmi::QuadKind::Sprite) {
            EXPECT_LE(quad.sprite.a, 0.5F);
        }
    }
    EXPECT_GT(differingPixels(plain, half), 1000U);
    EXPECT_GT(differingPixels(hidden, half), 1000U);

    // Le réglage retiré, l'image d'origine revient : la carte composée n'a pas été touchée.
    renderer.setQuadOpacity({});
    EXPECT_EQ(differingPixels(plain, offscreen->render(renderer, size, framing, BACKGROUND)), 0U);
}

/**
 * @brief La carte se prépare et se mesure avant la première image : le cadre d'un rendu se calcule
 *        sur ce qui sera dessiné.
 * \castest{<b>La carte se prepare et se mesure avant la premiere image.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Editeur<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander ce qu'occupe le donjon a un rendu sans ressources.<br/>
 *          2. Creer les ressources, redemander, sans dessiner.<br/>
 *          3. Dessiner.<br/>
 * \tattendu Sans ressources, le rectangle de base revient tel quel ; avec, les textures de la carte
 *           sont chargees et la carte composee sans qu'aucune image n'ait ete dessinee, et le
 *           rectangle mesure contient la base ; l'image dessinee ensuite est peinte.
 * }
 */
TEST(OffscreenRenderTest, LaCarteSePrepareEtSeMesureAvantLaPremiereImage) {
    const hmi::WorldSceneSnapshot snapshot = donjon();
    const core::Rect base{{0.0F, 0.0F}, projectionOf(snapshot).sceneSize()};

    {
        hmi::WorldSceneRenderer bare(assets());
        bare.setSnapshot(snapshot);
        EXPECT_FALSE(bare.prepare());
        const core::Rect without = bare.paintedBounds(base);
        EXPECT_FLOAT_EQ(without.size.x, base.size.x);
        EXPECT_FLOAT_EQ(without.size.y, base.size.y);
    }

    // L'interface d'abord, le rendu ensuite : ses ressources meurent avant elle.
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneRenderer renderer(assets());
    renderer.setSnapshot(snapshot);
    ASSERT_TRUE(renderer.ensureResources(offscreen->rhi()));
    const core::Rect painted = renderer.paintedBounds(base);
    EXPECT_FALSE(renderer.textures().byPath.empty()) << "les pieces se chargent a la preparation";
    EXPECT_GT(renderer.statics().size(), 700U);
    EXPECT_LE(painted.position.y, base.position.y);
    EXPECT_LE(painted.position.x, base.position.x);
    EXPECT_GE(painted.position.x + painted.size.x, base.position.x + base.size.x);
    EXPECT_GE(painted.position.y + painted.size.y, base.position.y + base.size.y);

    // Les televersements attendaient : la premiere image les soumet, et elle est peinte.
    const QImage image = offscreen->render(renderer, QSize(480, 360), milieu(snapshot), BACKGROUND);
    ASSERT_FALSE(image.isNull());
    std::size_t paintedPixels = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            paintedPixels += image.pixelColor(x, y) != BACKGROUND ? 1U : 0U;
        }
    }
    EXPECT_GT(paintedPixels, static_cast<std::size_t>(image.width() * image.height() / 4));
}

/**
 * @brief Une image plus grande qu'une tuile se rend tuile par tuile, et reste la même image.
 * \castest{<b>Une image rendue par tuiles est la meme image.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu · Editeur<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Rendre le donjon en 700 x 500 d'un seul coup.<br/>
 *          2. Le rendre par tuiles de 256 pixels : trois colonnes, deux rangees, les dernieres
 *             debordant de l'image.<br/>
 * \tattendu Deux images de meme taille ; aucune couture : moins d'un pixel sur cent differe, de
 *           deux niveaux par canal au plus (l'arrondi du centre de chaque tuile), et pas plus d'un
 *           pixel sur cent mille ne s'en ecarte davantage -- un bord de piece dont la couverture
 *           bascule. Mesure le 2 octobre 2026 : 215 pixels sur la carte graphique du poste, tous a
 *           deux niveaux au plus ; 1 132 sous le rendu logiciel de la CI (WARP), dont un seul
 *           au-dela.
 * }
 */
TEST(OffscreenRenderTest, UneImageRendueParTuilesEstLaMemeImage) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::WorldSceneRenderer renderer(assets());
    const hmi::WorldSceneSnapshot snapshot = donjon();
    renderer.setSnapshot(snapshot);
    const QSize size(700, 500);
    const hmi::WorldFraming framing = milieu(snapshot);

    const QImage whole = offscreen->render(renderer, size, framing, BACKGROUND);
    const QImage tiled = offscreen->render(renderer, size, framing, BACKGROUND, 256);
    ASSERT_EQ(whole.size(), size);
    ASSERT_EQ(tiled.size(), size);

    std::size_t differing = 0;
    std::size_t beyondRounding = 0;
    int worst = 0;
    for (int y = 0; y < size.height(); ++y) {
        for (int x = 0; x < size.width(); ++x) {
            const QRgb a = whole.pixel(x, y);
            const QRgb b = tiled.pixel(x, y);
            if (a == b) {
                continue;
            }
            ++differing;
            const int gap = std::max({std::abs(qRed(a) - qRed(b)), std::abs(qGreen(a) - qGreen(b)),
                                      std::abs(qBlue(a) - qBlue(b))});
            beyondRounding += gap > 2 ? 1U : 0U;
            worst = std::max(worst, gap);
        }
    }
    std::cout << differing << " pixels different entre l'image entiere et ses tuiles, de " << worst
              << " niveaux au plus, dont " << beyondRounding << " de plus de deux niveaux\n";
    // Etalonne sur les deux rendus (voir l'attendu) : une couture ferait differer des lignes
    // entieres, soit des centaines de pixels au-dela de l'arrondi.
    const auto pixels = static_cast<std::size_t>(size.width() * size.height());
    EXPECT_LT(differing, pixels / 100);
    EXPECT_LE(beyondRounding, pixels / 100000);
}

namespace {

/// Une carte de maquette batie **en memoire** : aucun lieu, aucune piece, aucun fichier d'image.
/// Des sols, de l'eau, une enceinte de murs, et une entite de chaque couleur de jeton.
hmi::WorldSceneSnapshot mockUpMap() {
    constexpr int WIDTH = 12;
    constexpr int HEIGHT = 9;
    core::TileMap tiles{WIDTH, HEIGHT};
    for (int row = 0; row < HEIGHT; ++row) {
        for (int column = 0; column < WIDTH; ++column) {
            const bool border = column == 0 || row == 0 || column == WIDTH - 1 || row == HEIGHT - 1;
            core::TileType type = border ? core::TileType::Wall : core::TileType::Grass;
            if (!border && column >= 3 && column <= 5 && row >= 3 && row <= 5) {
                type = core::TileType::Water;
            }
            if (!border && row == 7) {
                type = core::TileType::Dirt;
            }
            tiles.setTile(column, row, type);
        }
    }
    core::LevelData data{.name = "maquette", .tileMap = std::move(tiles)};
    data.entities = {
        core::MapEntity{.type = "npc",
                        .position = {.column = 2, .row = 2},
                        .properties = {{"dialogue", std::string{"market-mother"}}}},
        core::MapEntity{.type = "encounter",
                        .position = {.column = 8, .row = 2},
                        .properties = {{"encounterId", std::string{"wolves"}}}},
        core::MapEntity{.type = "spawnPoint",
                        .position = {.column = 2, .row = 7},
                        .properties = {{"name", std::string{"gate"}}}},
        core::MapEntity{.type = "portal",
                        .position = {.column = 8, .row = 7},
                        .properties = {{"targetMap", std::string{"arenarea"}},
                                       {"arrival", std::string{"gate"}}}},
    };
    const core::Level level{std::move(data)};
    return hmi::snapshotWorldScene(level, hmi::PlaceAppearance{},
                                   hmi::npcFigures(level.entities(), 0));
}

}  // namespace

/**
 * @brief Une carte **sans un seul fichier d'image** se voit (`EX-EXP-005`, `LOT-128`) : le rendu
 *        de maquette est dans la composition, que le jeu, le canevas et `--render` partagent.
 * \castest{<b>Une carte sans aucun fichier d'image se voit.</b><br/>
 * \tcat Unitaire · Rendu de maquette<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Batir en memoire une carte sans lieu : sols, eau, enceinte de murs, quatre
 * entites.<br/>2. La rendre hors ecran par le rendu du jeu, cadree sur son milieu.<br/>
 * \tattendu Les seules textures demandees sont celles des jetons, aucune n'est un fichier ;
 * l'image est peinte sur plus de la moitie de sa surface -- rien n'est reste vide.
 * }
 */
TEST(OffscreenRenderTest, UneCarteSansAucuneImageSeVoit) {
    const std::shared_ptr<hmi::OffscreenRhi> offscreen = hmi::OffscreenRhi::shared();
    if (!offscreen) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    const hmi::WorldSceneSnapshot maquette = mockUpMap();
    ASSERT_TRUE(maquette.place.empty());
    // Aucune planche : les seules textures demandees sont celles des jetons.
    for (const std::string& path : hmi::worldTexturePaths(maquette)) {
        EXPECT_TRUE(hmi::parseMaquetteTokenPath(path).has_value()) << path;
    }
    const core::IsoProjection projection = projectionOf(maquette);
    hmi::WorldSceneRenderer renderer(assets());
    renderer.setSnapshot(maquette);
    const hmi::WorldFraming framing{.center = projection.gridToWorld({6.0F, 4.5F}),
                                    .pixelsPerUnit = 64.0F / projection.tileWidth()};
    const QImage image = offscreen->render(renderer, QSize(480, 360), framing, BACKGROUND);
    ASSERT_FALSE(image.isNull());
    std::size_t painted = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            painted += image.pixelColor(x, y) != BACKGROUND ? 1U : 0U;
        }
    }
    EXPECT_GT(painted, static_cast<std::size_t>(image.width() * image.height() / 2));
}
