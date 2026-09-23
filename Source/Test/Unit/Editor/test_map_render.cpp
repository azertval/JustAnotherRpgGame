// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_map_render.cpp
 * @brief Tests de `LevelEditor --render` (`LOT-EDITOR-13`) : une carte livrée rendue hors écran,
 *        sans fenêtre, par le peintre du canevas.
 */

#include <QColor>
#include <QImage>
#include <QRect>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

#include <gtest/gtest.h>

#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Editor/Logic/CanvasScene.h"
#include "Editor/Ui/MapRender.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

// La racine d'essai de l'éditeur (`LOT-123`) : ce test rendait une carte LIVRÉE, que la
// table rase du `LOT-102` emporte. Voir `Fixtures/GameData/README.md`.
[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path(JADG_TEST_DATA_DIR);
}

/// La part des pixels de @p image qui ne sont pas le fond @p background.
[[nodiscard]] double peinte(const QImage& image, const QColor& background) {
    const QImage pixels = image.convertToFormat(QImage::Format_RGB32);
    long long painted = 0;
    for (int y = 0; y < pixels.height(); ++y) {
        for (int x = 0; x < pixels.width(); ++x) {
            if (pixels.pixelColor(x, y) != background) {
                ++painted;
            }
        }
    }
    return static_cast<double>(painted) /
           (static_cast<double>(pixels.width()) * static_cast<double>(pixels.height()));
}

}  // namespace

/**
 * @brief La carte d'essai se rend en PNG, sans fenêtre ; la collision se peint par-dessus quand
 *        demande.
 * \castest{<b>--render peint une carte hors écran.</b><br/>
 * \tcat Unitaire · Editeur · Sans fenetre<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Rendre la Place au quart de l'échelle, bandes par défaut.<br/>
 *          2. La rendre avec la collision en plus.<br/>
 * \tattendu Une image de la taille du cadre (1113 × 716), peinte sur plus du cinquième de sa
 *           surface (le losange de la carte en couvre la moitié, moins les îlots vides) ; la
 *           collision change l'image.
 * }
 */
TEST(MapRenderTest, UneCarteSeRendSansFenetre) {
    const core::LevelLoadResult carte =
        core::LevelLoader::loadFromFile(dataRoot() / "Levels" / "bourg" / "place.json");
    ASSERT_TRUE(carte.ok()) << carte.error;

    hmi::MapRenderOptions options;
    options.scale = 0.25;
    const QImage lieu = hmi::renderMap(*carte.level, dataRoot(), options);
    EXPECT_EQ(lieu.width(), 1113);
    EXPECT_EQ(lieu.height(), 716);
    EXPECT_GT(peinte(lieu, options.background), 1.0 / 5.0);

    options.bands.collision = 1.0F;
    const QImage collision = hmi::renderMap(*carte.level, dataRoot(), options);
    EXPECT_NE(collision, lieu);
}

namespace {

/// Un lieu HD d'une seule pièce, une tour de quatre cases de haut, écrit sous @p root.
void writeTallPiecePlace(const std::filesystem::path& root) {
    const std::filesystem::path place = root / "Assets" / "Scene" / "haut";
    std::filesystem::create_directories(place);
    // Quatre largeurs de case de haut : bien plus que la marge d'un losange d'avant le LOT-125.
    constexpr int tileWidth = 256;
    constexpr int towerHeight = 4 * tileWidth;
    QImage tower(tileWidth, towerHeight, QImage::Format_ARGB32);
    tower.fill(QColor(220, 20, 20));
    ASSERT_TRUE(tower.save(QString::fromStdWString((place / "tower.png").wstring())));
    std::ofstream(place / "manifest.json") << R"({
  "version": 1,
  "disposition": "haut",
  "tile": [256, 159],
  "textures": {
    "scene/haut/tower": {
      "file": "tower.png", "class": "tall", "footprint": [1, 1],
      "size": [256, 1024], "anchor": [128, 944]
    }
  }
})";
    std::ofstream(place / "appearance.json") << R"({
  "version": 1,
  "place": "haut",
  "floors": {},
  "relief": {"wall": ["tower"]}
})";
}

}  // namespace

/**
 * @brief Une pièce de quatre cases de haut, posée sur la case la plus haute de la carte, n'est
 *        rognée par `--render` (`LOT-125`) — ni en vignette, qui passe par le même cadre
 *        (`hmi::renderStamp`, la liste des cartes).
 * \castest{<b>Le cadre de --render tient les pièces hautes.</b><br/>
 * \tcat Unitaire · Editeur · Sans fenetre<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Écrire un lieu HD d'une seule pièce, une tour rouge de 256 × 1024.<br/>
 *          2. La poser sur la case (0, 0) d'une carte de 3 × 3, la plus haute à l'écran.<br/>
 *          3. Rendre la carte au demi.<br/>
 * \tattendu La tour se voit entière : autant de pixels rouges que son aire à l'échelle, et aucun
 *           sur le bord de l'image.
 * }
 */
TEST(MapRenderTest, UnePieceHauteNEstPasRognee) {
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "jadg-lot125-piece-haute";
    std::filesystem::remove_all(root);
    writeTallPiecePlace(root);

    core::LevelDraft draft = core::LevelDraft::empty("haut", 3, 3);
    const std::optional<std::size_t> ground = draft.addLayer(core::LayerKind::Ground, "ground");
    const std::optional<std::size_t> decor = draft.addLayer(core::LayerKind::Decor, "decor");
    ASSERT_TRUE(ground && decor);
    ASSERT_TRUE(draft.setLayerProperty(*ground, std::string{hmi::SCENE_PLACE_PROPERTY}, "haut"));
    ASSERT_TRUE(draft.placePiece(*decor, {.column = 0, .row = 0}, "tower", core::TileType::Wall));
    draft.setEntry(2, 2);
    const core::LevelLoadResult level = draft.toLevel();
    ASSERT_TRUE(level.ok()) << level.error;

    hmi::MapRenderOptions options;
    options.scale = 0.5;
    options.background = QColor(0, 0, 0);
    const QImage image = hmi::renderMap(*level.level, root, options);
    ASSERT_FALSE(image.isNull());

    const auto red = [](QRgb pixel) {
        return qRed(pixel) > 150 && qGreen(pixel) < 80 && qBlue(pixel) < 80;
    };
    std::size_t tower = 0;
    std::size_t onEdge = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (!red(image.pixel(x, y))) {
                continue;
            }
            ++tower;
            if (x == 0 || y == 0 || x == image.width() - 1 || y == image.height() - 1) {
                ++onEdge;
            }
        }
    }
    // Une case de 256 px d'art fait 50 px à l'échelle 0,5 : la tour, 50 × 200.
    const double expected = 0.5 * 100.0 * 0.5 * 400.0;
    EXPECT_GT(static_cast<double>(tower), 0.9 * expected);
    EXPECT_LT(static_cast<double>(tower), 1.1 * expected);
    EXPECT_EQ(onEdge, 0U) << "la tour touche le bord : elle est rognée";
    std::filesystem::remove_all(root);
}

/**
 * @brief Les bandes de `--layers` se lisent par leur nom ; un nom inconnu est refusé.
 * \castest{<b>--layers lit les bandes du canevas.</b><br/>
 * \tcat Unitaire · Editeur · Sans fenetre<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Lire `floors,collision`, puis `floors,toit`.<br/>
 * \tattendu Le sol et la collision seuls ; puis un refus.
 * }
 */
TEST(MapRenderTest, LesBandesSeLisentParLeurNom) {
    const std::optional<hmi::IsoBandOpacity> bands = hmi::parseRenderLayers("floors,collision");
    ASSERT_TRUE(bands.has_value());
    EXPECT_EQ(*bands, (hmi::IsoBandOpacity{
                          .floors = 1.0F, .relief = 0.0F, .figures = 0.0F, .collision = 1.0F}));
    EXPECT_FALSE(hmi::parseRenderLayers("floors,toit").has_value());
}

/**
 * @brief `--render` écrit une image par carte, nommée d'après son identifiant.
 * \castest{<b>--render écrit une image par carte.</b><br/>
 * \tcat Unitaire · Editeur · Sans fenetre<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. `--render bourg/place donjon --scale 0.125 --output <dossier>`.<br/>
 * \tattendu Code 0 ; `bourg-place.png` et `donjon.png` dans le dossier.
 * }
 */
TEST(MapRenderTest, RenderEcritUneImageParCarte) {
    const std::filesystem::path dossier =
        std::filesystem::temp_directory_path() / ("jadg-render-" + std::to_string(std::rand()));
    std::string sortie;
    const std::optional<int> code =
        hmi::runRenderCommand({"--render", "bourg/place", "donjon", "--scale", "0.125", "--data",
                               dataRoot().string(), "--output", dossier.string()},
                              {}, sortie);
    ASSERT_TRUE(code.has_value());
    EXPECT_EQ(*code, 0) << sortie;
    EXPECT_TRUE(std::filesystem::exists(dossier / "bourg-place.png")) << sortie;
    EXPECT_TRUE(std::filesystem::exists(dossier / "donjon.png")) << sortie;
    EXPECT_FALSE(hmi::runRenderCommand({"--check"}, {}, sortie).has_value());

    std::error_code ignore;
    std::filesystem::remove_all(dossier, ignore);
}

/**
 * @brief `--plan` rend la carte au vocabulaire des plans de principe : blocs couchés à plat et
 *        légende, ce qui donne une image différente du rendu ordinaire (`LOT-128`).
 * \castest{<b>--plan couche les blocs et ajoute une legende.</b><br/>
 * \tcat Unitaire · Editeur · Sans fenetre<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Rendre une carte de maquette, une fois ordinairement, une fois en plan.<br/>
 * \tattendu Deux images de meme taille mais differentes ; le coin haut gauche, vide dans le rendu
 * ordinaire, porte la legende du plan.
 * }
 */
TEST(MapRenderTest, LePlanCoucheLesBlocsEtLegende) {
    const core::LevelLoadResult carte =
        core::LevelLoader::loadFromFile(dataRoot() / "Levels" / "donjon.json");
    ASSERT_TRUE(carte.ok()) << carte.error;

    hmi::MapRenderOptions options;
    options.scale = 0.5;
    const QImage ordinaire = hmi::renderMap(*carte.level, dataRoot(), options);
    options.plan = true;
    const QImage plan = hmi::renderMap(*carte.level, dataRoot(), options);

    ASSERT_FALSE(plan.isNull());
    EXPECT_EQ(plan.size(), ordinaire.size());
    EXPECT_NE(plan, ordinaire);
    // La legende occupe le coin haut gauche, que le losange de la carte laisse vide.
    const QRect coin(0, 0, plan.width() / 4, plan.height() / 4);
    EXPECT_GT(peinte(plan.copy(coin), options.background),
              peinte(ordinaire.copy(coin), options.background));
}
