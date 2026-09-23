// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_storey_render.cpp
 * @brief Un îlot de murs du kit de la Capitale sur deux étages, coiffé d'un toit, **rendu** : on
 *        voit le héros à travers ce qui le masque (`LOT-129`).
 *
 * Les murs sont ceux du kit installé (`LOT-105`), copiés sous `Scene/capital/` d'une racine
 * temporaire comme le fait `test_capital_kit_render.cpp` — le moteur ne résout pas encore l'arbre
 * `Regions/`. Leur manifeste déclare la hauteur d'étage (`storey`). Le toit est **provisoire** :
 * une pyramide bourgogne peinte par le test, en attendant la toiture de la Capitale que produit le
 * lot. Le rendu passe par le peintre de l'éditeur, que le `LOT-125` rend conforme au jeu ; l'image
 * est écrite dans `editor-captures/` pour l'œil de l'auteur.
 */

#include <QColor>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QPolygonF>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/LevelDraft.h"
#include "Editor/Ui/SceneImages.h"
#include "Editor/Ui/ScenePainter.h"
#include "HMI/Graphics/Camera2D.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

constexpr int MAP = 5;

[[nodiscard]] std::filesystem::path kitScene() {
    return std::filesystem::path(JADG_ASSETS_DIR) / "Regions" / "central-empire" / "capital" /
           "Common" / "Scene";
}

/// Le toit provisoire : une pyramide bourgogne sur une emprise de 3 × 3, au losange du standard.
void writeProvisionalRoof(const std::filesystem::path& scene) {
    constexpr double width = 3 * 256.0;
    constexpr double diamond = 3 * 159.0;
    constexpr double rise = 260.0;
    QImage roof(static_cast<int>(width), static_cast<int>(diamond + rise), QImage::Format_ARGB32);
    roof.fill(Qt::transparent);
    QPainter painter(&roof);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    const QPointF top(width / 2, rise);
    const QPointF right(width, rise + (diamond / 2));
    const QPointF bottom(width / 2, rise + diamond);
    const QPointF left(0, rise + (diamond / 2));
    const QPointF apex(width / 2, rise + (diamond / 2) - rise);
    painter.setBrush(QColor(0x6e, 0x1a, 0x28));
    painter.drawPolygon(QPolygonF{apex, top, right});
    painter.drawPolygon(QPolygonF{apex, left, top});
    painter.setBrush(QColor(0x8e, 0x23, 0x35));
    painter.drawPolygon(QPolygonF{apex, left, bottom});
    painter.setBrush(QColor(0x74, 0x1d, 0x2c));
    painter.drawPolygon(QPolygonF{apex, bottom, right});
    painter.end();
    ASSERT_TRUE(roof.save(QString::fromStdWString((scene / "roof-test.png").wstring())));
}

/// Copie le kit sous @p root/Assets/Scene/capital, et y ajoute le toit provisoire.
[[nodiscard]] std::filesystem::path installPlace(const std::filesystem::path& root) {
    const std::filesystem::path scene = root / "Assets" / "Scene" / "capital";
    std::filesystem::create_directories(scene);
    // Le kit est rangé en sous-dossiers (LOT-129) : il se copie entier.
    std::filesystem::copy(kitScene(), scene,
                          std::filesystem::copy_options::recursive |
                              std::filesystem::copy_options::overwrite_existing);
    writeProvisionalRoof(scene);
    std::ifstream in(scene / "manifest.json");
    nlohmann::json manifest = nlohmann::json::parse(in);
    in.close();
    manifest["textures"]["scene/capital/roof-test"] = {
        {"file", "roof-test.png"}, {"class", "wide"},      {"footprint", {3, 3}},
        {"size", {768, 737}},      {"anchor", {384, 260}}, {"tactical", "open"}};
    std::ofstream(scene / "manifest.json") << manifest.dump(2);
    std::ofstream(scene / "appearance.json")
        << R"({"version": 1, "place": "capital", "floors": {}, "relief": {}})";
    return root / "Assets";
}

/// Les deux façades vues d'un bâtiment de 3 × 3 aux cases (1..3, 1..3) : la rangée de devant, la
/// colonne de droite, et l'angle sortant qui les joint, comme la carte de validation du kit.
void placeWalls(core::LevelDraft& draft, std::size_t layer) {
    struct Wall {
        const char* piece;
        int column;
        int row;
    };
    const std::vector<Wall> walls = {
        {"wall-limestone-window-u", 1, 3},
        {"wall-limestone-window-v", 3, 1},
        {"wall-limestone-corner-outer", 3, 3},
    };
    for (const Wall& wall : walls) {
        ASSERT_TRUE(draft.placePiece(layer, {.column = wall.column, .row = wall.row}, wall.piece,
                                     core::TileType::Wall))
            << wall.piece;
    }
}

}  // namespace

/**
 * @brief L'îlot de murs sur deux étages, coiffé de son toit : le héros qui passe derrière reste
 *        visible à travers l'étage et le toit.
 * \castest{<b>On voit le heros a travers l'etage et le toit.</b><br/>
 * \tcat Unitaire · Editeur · Etages<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Batir un batiment de 3 x 3 en murs du kit, au rez et a l'etage 1, coiffe d'un toit a
 *             l'etage 2 ; manifeste a 224 pixels d'etage.<br/>
 *          2. Rendre la scene avec le heros juste derriere, puis sans lui.<br/>
 * \tattendu Les deux images different la ou le heros se tient : il se voit a travers ce qui le
 *           masque. La collision ne dit que le rez.
 * }
 */
TEST(StoreyRenderTest, OnVoitLeHerosATraversLEtageEtLeToit) {
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jadg-lot129-ilot";
    std::filesystem::remove_all(root);
    const std::filesystem::path assets = installPlace(root);

    const hmi::PlaceAppearanceResult appearance =
        hmi::PlaceAppearance::loadFromFile(assets / "Scene" / "capital" / "appearance.json");
    ASSERT_TRUE(appearance.ok()) << appearance.message;
    const core::ScenePieceManifestResult manifest =
        core::ScenePieceManifest::loadFromFile(assets / "Scene" / "capital" / "manifest.json");
    ASSERT_TRUE(manifest.ok()) << manifest.message;

    core::LevelDraft draft = core::LevelDraft::empty("ilot", MAP, MAP);
    draft.setPieceManifest(std::make_shared<const core::ScenePieceManifest>(manifest.manifest));
    const std::size_t ground = *draft.addLayer(core::LayerKind::Ground, "sol");
    ASSERT_TRUE(draft.setLayerProperty(ground, std::string{hmi::SCENE_PLACE_PROPERTY}, "capital"));
    for (int row = 0; row < MAP; ++row) {
        for (int column = 0; column < MAP; ++column) {
            ASSERT_TRUE(
                draft.placePiece(ground, {.column = column, .row = row},
                                 "floor-paving-0" + std::to_string(((row + column) % 3) + 1),
                                 core::TileType::Pavement));
        }
    }
    const std::size_t rez = *draft.addLayer(core::LayerKind::Decor, "rez");
    const std::size_t upper = *draft.addLayer(core::LayerKind::Decor, "etage");
    const std::size_t roof = *draft.addLayer(core::LayerKind::Decor, "toit");
    ASSERT_TRUE(draft.setLayerFloor(upper, 1));
    ASSERT_TRUE(draft.setLayerFloor(roof, 2));
    placeWalls(draft, rez);
    placeWalls(draft, upper);
    ASSERT_TRUE(draft.placePiece(roof, {.column = 1, .row = 1}, "roof-test", core::TileType::Wall));
    // Le rez arrête la vue ; l'étage et le toit n'ajoutent rien à la collision.
    EXPECT_EQ(draft.tileMap().tile(2, 3), core::TileType::Wall);
    EXPECT_NE(draft.tileMap().tile(2, 2), core::TileType::Wall);

    // Une figurine juste derrière le bâtiment : le héros, un PNJ, ou personne.
    enum class Behind { Nobody, Hero, Npc };
    const auto render = [&](Behind who) {
        std::vector<hmi::WorldFigureSnapshot> figures;
        if (who != Behind::Nobody) {
            figures.push_back(hmi::WorldFigureSnapshot{.figure = "heros-essai",
                                                       .clip = "idle",
                                                       .point = {2.5F, 0.5F},
                                                       .hero = who == Behind::Hero});
        }
        const hmi::WorldSceneSnapshot snapshot = hmi::snapshotWorldScene(
            hmi::worldSceneSource(draft), appearance.appearance, std::move(figures));
        const core::IsoProjection projection(snapshot.columns, snapshot.rows,
                                             core::ARENA_TILE_WIDTH_UNITS, snapshot.diamondRatio);
        hmi::SceneImages images(assets);
        images.ensure(hmi::worldTexturePaths(snapshot));
        const hmi::ComposedScene scene =
            hmi::composeWorldScene(snapshot, projection, images.textures());
        hmi::Camera2D camera(960, 720);
        camera.setZoom(hmi::worldTilePixels(1080) /
                       (projection.tileWidth() * hmi::Camera2D::PIXELS_PER_UNIT));
        camera.setCenter(projection.gridToWorld({2.5F, 1.5F}));
        return hmi::renderComposedScene(scene, camera, 960, 720, QColor(24, 26, 30));
    };
    const QImage nobody = render(Behind::Nobody);
    const QImage hero = render(Behind::Hero);
    const QImage npc = render(Behind::Npc);

    const QDir captures(QDir::current().filePath(QStringLiteral("editor-captures")));
    QDir().mkpath(captures.path());
    hero.save(captures.filePath(QStringLiteral("etages-heros-derriere.png")));
    npc.save(captures.filePath(QStringLiteral("etages-pnj-derriere.png")));

    const auto visible = [&nobody](const QImage& image) {
        std::size_t differing = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                differing += image.pixel(x, y) != nobody.pixel(x, y) ? 1U : 0U;
            }
        }
        return differing;
    };
    // Le PNJ, lui, disparaît derrière l'étage et le toit : c'est l'effacement qui montre le héros.
    const std::size_t heroPixels = visible(hero);
    const std::size_t npcPixels = visible(npc);
    std::cout << "heros visible sur " << heroPixels << " pixels, PNJ sur " << npcPixels << "\n";
    EXPECT_GT(heroPixels, 2 * npcPixels) << "le heros ne se voit pas a travers l'etage et le toit";
    EXPECT_GT(heroPixels, 1000U);
    std::filesystem::remove_all(root);
}
