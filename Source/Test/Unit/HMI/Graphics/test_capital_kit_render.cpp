// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_capital_kit_render.cpp
 * @brief Le kit commun de la Capitale (`LOT-105`), rendu par le **moteur** : une rue de douze cases
 *        composée avec ses seules pièces.
 *
 * C'est le premier critère du lot : « une rue de douze cases se compose avec le seul kit, sans
 * pièce propre à un quartier ». Les aperçus de production (`Tools/AssetsHD/…/apercus/`) posent les
 * pièces dans un navigateur ; ce test les pose comme le jeu, par `hmi::WorldSceneRenderer` :
 * échelle du lieu, ancre de chaque pièce, mipmaps.
 *
 * Le moteur lit encore un lieu sous `Scene/<place>/` : la résolution dans l'arbre `Regions/` est à
 * venir. Le test copie donc le dossier installé `Regions/central-empire/capital/Common/Scene` sous
 * `Scene/capital/` d'une racine temporaire — une copie de lecture, jamais versionnée — et rend la
 * rue en 1920 × 1080. L'image est écrite pour l'œil de l'auteur ; ce que le test vérifie seul,
 * c'est que chaque pièce posée est au manifeste et que la rue occupe l'écran.
 */

#include <QImage>
#include <QString>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <rhi/qrhi.h>

#include "Core/Resources/ScenePieceManifest.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/Graphics/WorldSceneRenderer.h"

namespace {

constexpr int COLUMNS = 12;
constexpr int ROWS = 5;
constexpr std::array<float, 4> BACKGROUND = {0.13F, 0.15F, 0.17F, 1.0F};

std::filesystem::path kitScene() {
    return std::filesystem::path(JADG_ASSETS_DIR) / "Regions" / "central-empire" / "capital" /
           "Common" / "Scene";
}

std::unique_ptr<QRhi> createOffscreenRhi() {
#ifdef Q_OS_WIN
    QRhiD3D11InitParams params;
    if (QRhi* const rhi = QRhi::create(QRhi::D3D11, &params)) {
        return std::unique_ptr<QRhi>(rhi);
    }
#endif
    return nullptr;
}

/// Une pièce debout, à sa case d'ancrage (le coin nord de son emprise).
struct Placed {
    const char* piece;
    int column;
    int row;
};

/**
 * La rue : un front de façades au nord (rangée 0), la chaussée bordée de ses deux trottoirs
 * (rangées 1 à 3), puis une rangée de dallage où se tient le mobilier (rangée 4).
 */
const std::vector<Placed>& streetPieces() {
    static const std::vector<Placed> pieces = {
        {"wall-limestone-corner-outer", 0, 0},
        {"wall-limestone-u", 1, 0},
        {"wall-limestone-window-u", 3, 0},
        {"wall-limestone-u", 5, 0},
        {"wall-limestone-window-u", 7, 0},
        {"wall-limestone-u", 9, 0},
        {"plant-cypress", 11, 0},
        {"prop-lamppost", 0, 4},
        {"prop-bench-u", 2, 4},
        {"prop-planter", 4, 4},
        {"plant-flowerbed", 5, 4},
        {"prop-stall-empty", 7, 4},
        {"prop-crate", 8, 4},
        {"prop-barrel", 9, 4},
        {"prop-lamppost", 11, 4},
    };
    return pieces;
}

std::string streetFloor(int column, int row) {
    const std::string variant = "-0" + std::to_string((((column * 7) + (row * 3)) % 3) + 1);
    if (row == 1) {
        return "floor-paving-edge-ne";
    }
    if (row == 3) {
        return "floor-paving-edge-sw";
    }
    return (row == 4 ? "floor-flagstone" : "floor-paving") + variant;
}

hmi::WorldSceneSnapshot streetSnapshot(const core::ScenePieceManifest& manifest) {
    hmi::WorldSceneSnapshot snapshot;
    snapshot.place = "capital";
    snapshot.diamondRatio = 159.0F / 256.0F;
    snapshot.columns = COLUMNS;
    snapshot.rows = ROWS;
    const auto cells = static_cast<std::size_t>(COLUMNS) * ROWS;
    snapshot.floors.assign(cells, std::string{});
    snapshot.relief.assign(cells, std::string{});
    snapshot.types.assign(cells, core::TileType::Solid);
    snapshot.reliefTypes.assign(cells, core::TileType::Empty);
    for (int row = 0; row < ROWS; ++row) {
        for (int column = 0; column < COLUMNS; ++column) {
            snapshot.floors[(static_cast<std::size_t>(row) * COLUMNS) + column] =
                streetFloor(column, row);
        }
    }
    for (const Placed& placed : streetPieces()) {
        snapshot.relief[(static_cast<std::size_t>(placed.row) * COLUMNS) + placed.column] =
            placed.piece;
        if (const core::ScenePiece* const declared = manifest.find(placed.piece)) {
            snapshot.footprints.insert_or_assign(placed.piece, declared->footprint());
        }
    }
    return snapshot;
}

}  // namespace

/**
 * @brief Une rue de douze cases se compose avec le seul kit de la Capitale, et le moteur la rend.
 * \castest{<b>Le moteur rend une rue de douze cases composee du seul kit de la Capitale.</b><br/>
 * \tcat Unitaire · Rendu HD<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Copier le Scene/ installe du kit sous une racine temporaire.<br/>
 *          2. Composer douze cases sur cinq : facades, chaussee bordee, dallage et mobilier.<br/>
 *          3. La rendre hors ecran en 1920 x 1080 et ecrire l'image.<br/>
 * \tattendu Chaque piece posee est au manifeste du kit ; la rue couvre une part notable de
 * l'image. L'auteur juge l'image ecrite (critere du LOT-105).
 * }
 */
TEST(CapitalKitRender, AStreetOfTwelveCellsIsComposedWithTheKitAlone) {
    const core::ScenePieceManifestResult manifest =
        core::ScenePieceManifest::loadFromFile(kitScene() / "manifest.json");
    ASSERT_TRUE(manifest.ok()) << manifest.message;
    for (const Placed& placed : streetPieces()) {
        EXPECT_NE(manifest.manifest.find(placed.piece), nullptr) << placed.piece;
    }
    for (int row = 0; row < ROWS; ++row) {
        for (int column = 0; column < COLUMNS; ++column) {
            EXPECT_NE(manifest.manifest.find(streetFloor(column, row)), nullptr)
                << streetFloor(column, row);
        }
    }

    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "jadg-kit-capitale";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "Scene");
    std::filesystem::copy(kitScene(), root / "Scene" / "capital",
                          std::filesystem::copy_options::recursive);

    const QSize size(1920, 1080);
    const std::unique_ptr<QRhiTexture> texture(
        rhi->newTexture(QRhiTexture::RGBA8, size, 1,
                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    ASSERT_TRUE(texture->create());
    const std::unique_ptr<QRhiTextureRenderTarget> target(
        rhi->newTextureRenderTarget({{texture.get()}}));
    const std::unique_ptr<QRhiRenderPassDescriptor> pass(
        target->newCompatibleRenderPassDescriptor());
    target->setRenderPassDescriptor(pass.get());
    ASSERT_TRUE(target->create());

    QImage rendered;
    {
        hmi::WorldSceneRenderer renderer(root);
        ASSERT_TRUE(renderer.ensureResources(rhi.get()));
        renderer.setSnapshot(streetSnapshot(manifest.manifest));
        renderer.setFocus({COLUMNS / 2.0F, ROWS / 2.0F});
        QRhiCommandBuffer* commandBuffer = nullptr;
        ASSERT_EQ(rhi->beginOffscreenFrame(&commandBuffer), QRhi::FrameOpSuccess);
        renderer.render(commandBuffer, target.get(), BACKGROUND.data());
        QRhiReadbackResult readback;
        QRhiResourceUpdateBatch* const batch = rhi->nextResourceUpdateBatch();
        batch->readBackTexture({texture.get()}, &readback);
        commandBuffer->resourceUpdate(batch);
        ASSERT_EQ(rhi->endOffscreenFrame(), QRhi::FrameOpSuccess);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): QImage lit des `uchar`.
        rendered =
            QImage(reinterpret_cast<const uchar*>(readback.data.constData()),
                   readback.pixelSize.width(), readback.pixelSize.height(), QImage::Format_RGBA8888)
                .copy();
        renderer.release();
    }
    ASSERT_EQ(rendered.size(), size);

    const std::filesystem::path capture =
        std::filesystem::temp_directory_path() / "jadg-kit-capitale-rue.png";
    rendered.save(QString::fromStdWString(capture.wstring()));

    // Le fond est uni : ce qui s'en écarte est la rue. Soixante losanges de 100 × 62 px et le front
    // de façades couvrent un dixième de l'écran en 1080p (0,104 mesuré) ; une rue sans sol n'en
    // couvrirait pas le tiers.
    const QImage rgb = rendered.convertToFormat(QImage::Format_RGB888);
    const int blue = static_cast<int>(BACKGROUND[2] * 255.0F);
    std::size_t covered = 0;
    for (int y = 0; y < rgb.height(); ++y) {
        const uchar* const line = rgb.constScanLine(y);
        for (int x = 0; x < rgb.width(); ++x) {
            if (std::abs(line[(x * 3) + 2] - blue) > 12) {
                ++covered;
            }
        }
    }
    const double share =
        static_cast<double>(covered) / (static_cast<double>(size.width()) * size.height());
    EXPECT_GT(share, 0.08) << "rendu ecrit dans " << capture.string();
    std::filesystem::remove_all(root);
}
