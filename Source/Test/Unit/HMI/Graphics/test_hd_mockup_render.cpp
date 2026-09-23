// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_hd_mockup_render.cpp
 * @brief La maquette du standard 2D HD (`LOT-101`), rendue par le **moteur** et comparée à la
 *        maquette montée à la main (`LOT-103`).
 *
 * Le `LOT-101` a monté huit cases sur huit d'Arenarea à l'échelle du standard et les a cadrées aux
 * deux définitions où le jeu se joue ; ses deux images sans texte sont les références de ce test.
 * Les pièces qu'il a découpées sont installées dans `Fixtures/HdMockup` comme la chaîne HD les
 * installe — une image par pièce, un manifeste qui déclare le losange de l'art, l'emprise et
 * l'ancre — avec la disposition de la scène (`scene.json`) : `scripts/build_hd_mockup.py` écrit
 * les deux d'un même geste, et `--check` garde l'un et l'autre à jour.
 *
 * Ce que le test prouve est tout ce que le lot change : une pièce se met à l'échelle de son lieu
 * (et non à celle d'une constante), se pose par son ancre, se filtre sans crénelage, et la caméra
 * cadre une case à la hauteur de la fenêtre divisée par 10,8 (`EX-REN-013`). Une seule de ces
 * choses de travers, et l'image s'écarte de sa référence de plusieurs dizaines de niveaux.
 *
 * Ce qu'il ne prouve pas : l'égalité au pixel. La maquette a réduit l'art en Lanczos, le GPU le
 * réduit par mipmaps. L'écart moyen se mesure donc à un seuil, écrit ci-dessous avec ce qu'il
 * attrape.
 */

#include <QImage>
#include <QString>
#include <QtGlobal>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <rhi/qrhi.h>

#include "Core/Resources/ScenePieceManifest.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/Graphics/WorldSceneRenderer.h"

namespace {

/**
 * Côté des blocs sur lesquels les deux images se moyennent avant d'être comparées, en pixels.
 *
 * Pixel à pixel, l'écart est dominé par le filtrage — Lanczos d'un côté, mipmaps de l'autre, sur
 * le fin quadrillage du pavage — et un défaut réel s'y noie : les coutures entre dalles ne
 * donnaient que 8,2 contre 6,8. Moyennées par blocs de 4 × 4, les deux images ne gardent que ce que
 * l'œil juge à la taille du jeu — où est chaque pièce, de quelle taille, et si le sol est continu.
 */
constexpr int COMPARISON_BLOCK = 4;

/**
 * Écart moyen admis entre les deux images moyennées, en niveaux (0-255) par canal.
 *
 * Mesuré à la mise en place (`LOT-103`, Direct3D 11) : **2,3** à 1080p, **3,1** dans la fenêtre
 * 2160p. Chaque hypothèse de travers le dépasse sur au moins une vue, mesurée en la provoquant :
 *
 * | Défaut provoqué | 1080p | 2160p |
 * |---|---:|---:|
 * | l'échelle de 68 px au lieu du losange du lieu | 16,9 | 34,6 |
 * | les ancres des pièces ignorées | 4,3 | 11,6 |
 * | les dalles jointives, sans débord (`FLOOR_SEAM_OVERLAP` nul) | 4,2 | 5,1 |
 * | une échelle fausse de 10 % | 3,1 | 6,9 |
 */
constexpr double MEAN_ERROR_THRESHOLD = 4.0;

std::filesystem::path mockupDirectory() {
    return std::filesystem::path(JADG_HD_MOCKUP_DIR);
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

nlohmann::json readJson(const std::filesystem::path& path) {
    std::ifstream stream(path);
    return nlohmann::json::parse(stream, nullptr, false);
}

/// La scène de `scene.json`, en instantané : les sols par la légende, les pièces à leur case.
hmi::WorldSceneSnapshot mockupSnapshot(const nlohmann::json& scene) {
    hmi::WorldSceneSnapshot snapshot;
    snapshot.place = scene["place"].get<std::string>();
    snapshot.diamondRatio = scene["diamondRatio"].get<float>();
    snapshot.columns = scene["columns"].get<int>();
    snapshot.rows = scene["rows"].get<int>();
    const auto cells =
        static_cast<std::size_t>(snapshot.columns) * static_cast<std::size_t>(snapshot.rows);
    snapshot.floors.assign(cells, std::string{});
    snapshot.relief.assign(cells, std::string{});
    snapshot.types.assign(cells, core::TileType::Solid);
    snapshot.reliefTypes.assign(cells, core::TileType::Empty);

    const nlohmann::json& legend = scene["legend"];
    const nlohmann::json& floors = scene["floors"];
    for (int row = 0; row < snapshot.rows; ++row) {
        const std::string line = floors[static_cast<std::size_t>(row)].get<std::string>();
        for (int column = 0; column < snapshot.columns; ++column) {
            const std::string symbol(1, line[static_cast<std::size_t>(column)]);
            snapshot.floors[(static_cast<std::size_t>(row) * snapshot.columns) + column] =
                legend[symbol].get<std::string>();
        }
    }

    const core::ScenePieceManifestResult manifest = core::ScenePieceManifest::loadFromFile(
        mockupDirectory() / "Scene" / snapshot.place / "manifest.json");
    EXPECT_TRUE(manifest.ok()) << manifest.message;
    for (const nlohmann::json& piece : scene["pieces"]) {
        const std::string name = piece["piece"].get<std::string>();
        const int column = piece["column"].get<int>();
        const int row = piece["row"].get<int>();
        snapshot.relief[(static_cast<std::size_t>(row) * snapshot.columns) + column] = name;
        if (const core::ScenePiece* const declared = manifest.manifest.find(name)) {
            snapshot.footprints.insert_or_assign(name, declared->footprint());
        }
    }
    return snapshot;
}

/// Des images du moteur à la définition @p size, une par point suivi de @p focuses (en cases).
std::vector<QImage> renderViews(QRhi& rhi, const nlohmann::json& scene, QSize size,
                                const std::vector<core::Vector2>& focuses) {
    const std::unique_ptr<QRhiTexture> texture(
        rhi.newTexture(QRhiTexture::RGBA8, size, 1,
                       QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    EXPECT_TRUE(texture->create());
    const std::unique_ptr<QRhiTextureRenderTarget> target(
        rhi.newTextureRenderTarget({{texture.get()}}));
    const std::unique_ptr<QRhiRenderPassDescriptor> pass(
        target->newCompatibleRenderPassDescriptor());
    target->setRenderPassDescriptor(pass.get());
    EXPECT_TRUE(target->create());

    const nlohmann::json& background = scene["background"];
    const float clear[4] = {background[0].get<float>() / 255.0F,
                            background[1].get<float>() / 255.0F,
                            background[2].get<float>() / 255.0F, 1.0F};

    std::vector<QImage> images;
    {
        hmi::WorldSceneRenderer renderer(mockupDirectory());
        EXPECT_TRUE(renderer.ensureResources(&rhi));
        renderer.setSnapshot(mockupSnapshot(scene));
        for (const core::Vector2 focus : focuses) {
            renderer.setFocus(focus);
            QRhiCommandBuffer* commandBuffer = nullptr;
            if (rhi.beginOffscreenFrame(&commandBuffer) != QRhi::FrameOpSuccess) {
                ADD_FAILURE() << "beginOffscreenFrame";
                return images;
            }
            renderer.render(commandBuffer, target.get(), clear);
            QRhiReadbackResult readback;
            QRhiResourceUpdateBatch* const batch = rhi.nextResourceUpdateBatch();
            batch->readBackTexture({texture.get()}, &readback);
            commandBuffer->resourceUpdate(batch);
            if (rhi.endOffscreenFrame() != QRhi::FrameOpSuccess) {
                ADD_FAILURE() << "endOffscreenFrame";
                return images;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): QImage lit des `uchar`.
            images.push_back(QImage(reinterpret_cast<const uchar*>(readback.data.constData()),
                                    readback.pixelSize.width(), readback.pixelSize.height(),
                                    QImage::Format_RGBA8888)
                                 .copy());
        }
        renderer.release();
    }
    return images;
}

/// Le point suivi de la scène, en cases.
core::Vector2 sceneFocus(const nlohmann::json& scene) {
    return {scene["focus"][0].get<float>(), scene["focus"][1].get<float>()};
}

/// Une image du moteur, à la définition @p size, cadrée sur le point suivi de la scène.
QImage renderView(QRhi& rhi, const nlohmann::json& scene, QSize size) {
    std::vector<QImage> images = renderViews(rhi, scene, size, {sceneFocus(scene)});
    return images.empty() ? QImage{} : images.front();
}

/// Les moyennes de @p image par blocs de `COMPARISON_BLOCK` pixels, trois canaux par bloc.
std::vector<double> blockMeans(const QImage& image) {
    const QImage rgb = image.convertToFormat(QImage::Format_RGB888);
    const int columns = rgb.width() / COMPARISON_BLOCK;
    const int rows = rgb.height() / COMPARISON_BLOCK;
    std::vector<double> means(static_cast<std::size_t>(columns) * rows * 3, 0.0);
    for (int y = 0; y < rows * COMPARISON_BLOCK; ++y) {
        const uchar* const line = rgb.constScanLine(y);
        for (int x = 0; x < columns * COMPARISON_BLOCK; ++x) {
            const std::size_t block =
                (static_cast<std::size_t>(y / COMPARISON_BLOCK) * columns) + (x / COMPARISON_BLOCK);
            for (int channel = 0; channel < 3; ++channel) {
                means[(block * 3) + channel] += line[(x * 3) + channel];
            }
        }
    }
    for (double& mean : means) {
        mean /= static_cast<double>(COMPARISON_BLOCK * COMPARISON_BLOCK);
    }
    return means;
}

/// L'écart moyen, par canal de couleur, entre deux images de même taille moyennées par blocs.
double meanError(const QImage& rendered, const QImage& reference) {
    const std::vector<double> left = blockMeans(rendered);
    const std::vector<double> right = blockMeans(reference);
    double total = 0.0;
    for (std::size_t index = 0; index < left.size(); ++index) {
        total += std::abs(left[index] - right[index]);
    }
    return left.empty() ? 0.0 : total / static_cast<double>(left.size());
}

/// Rend la vue @p name de la maquette et la compare à sa référence ; écrit le rendu pour l'œil.
void expectViewMatchesReference(const std::string& name) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    const nlohmann::json scene = readJson(mockupDirectory() / "scene.json");
    ASSERT_FALSE(scene.is_discarded());
    const nlohmann::json& view = scene["views"][name];
    const QSize size(view["size"][0].get<int>(), view["size"][1].get<int>());

    QImage rendered = renderView(*rhi, scene, size);
    ASSERT_EQ(rendered.size(), size);
    if (view.contains("window")) {
        // La référence 2160p est une fenêtre au pixel près, prise au centre de la vue.
        const QSize window(view["window"][0].get<int>(), view["window"][1].get<int>());
        rendered =
            rendered.copy((size.width() - window.width()) / 2,
                          (size.height() - window.height()) / 2, window.width(), window.height());
    }

    const std::filesystem::path referencePath =
        std::filesystem::path(JADG_MOCKUP_REFERENCE_DIR) / view["reference"].get<std::string>();
    const QImage reference(QString::fromStdWString(referencePath.wstring()));
    ASSERT_FALSE(reference.isNull()) << referencePath.string();
    ASSERT_EQ(rendered.size(), reference.size());

    const std::filesystem::path capture =
        std::filesystem::temp_directory_path() / ("jadg-maquette-hd-" + name + ".png");
    rendered.save(QString::fromStdWString(capture.wstring()));

    const double error = meanError(rendered, reference);
    ::testing::Test::RecordProperty("meanError", std::to_string(error));
    EXPECT_LT(error, MEAN_ERROR_THRESHOLD) << "vue " << name << " : ecart moyen " << error
                                           << ", rendu ecrit dans " << capture.string();
}

}  // namespace

/**
 * @brief La maquette rendue par le moteur à 1080p est conforme à la maquette montée à la main.
 * \castest{<b>Le moteur rend la maquette du standard 2D HD a 1080p.</b><br/>
 * \tcat Unitaire · Rendu HD<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire la scene et les pieces installees de Fixtures/HdMockup.<br/>
 *          2. La rendre hors ecran en 1920 x 1080, cadree sur le milieu de la place.<br/>
 *          3. La comparer a maquette-2d-hd-1080.png.<br/>
 * \tattendu L'ecart moyen par canal reste sous le seuil ecrit dans le test.
 * }
 */
TEST(HdMockupRender, MatchesTheHandMadeMockupAt1080p) {
    expectViewMatchesReference("1080");
}

/**
 * @brief À 2160p, le moteur cadre la même étendue deux fois plus fine : la fenêtre centrale est
 *        conforme à celle de la maquette.
 * \castest{<b>Le moteur rend la maquette du standard 2D HD a 2160p.</b><br/>
 * \tcat Unitaire · Rendu HD<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Rendre la meme scene en 3840 x 2160.<br/>
 *          2. En extraire la fenetre centrale de 1920 x 1080.<br/>
 *          3. La comparer a maquette-2d-hd-2160.png.<br/>
 * \tattendu L'ecart moyen par canal reste sous le seuil ecrit dans le test.
 * }
 */
TEST(HdMockupRender, MatchesTheHandMadeMockupAt2160p) {
    expectViewMatchesReference("2160");
}

/**
 * @brief Le **travelling lent** du critère du `LOT-103`, écrit image par image pour l'œil de
 *        l'auteur : le scintillement ne se mesure pas, il se regarde.
 *
 * Sauté sauf si la variable d'environnement `JADG_TRAVELLING_DIR` nomme un dossier : il y écrit
 * `travelling-000.png` et suivantes, en 1920 × 1080, la caméra glissant le long d'une rangée d'un
 * quart de largeur de pixel d'écran par image — un pas plus fin que le pixel, où le plus proche
 * voisin, lui, scintillait. `scripts/build_hd_mockup.py --travelling <dossier>` en fait une
 * animation.
 * \castest{<b>Le travelling de la maquette s'ecrit pour le controle visuel.</b><br/>
 * \tcat Unitaire · Rendu HD<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Nommer un dossier dans JADG_TRAVELLING_DIR.<br/>
 * 2. Rendre 96 images de la maquette, la camera glissant d'un quart de pixel par image.<br/>
 * \tattendu Les 96 images sont ecrites ; l'auteur juge le scintillement a l'oeil (critere du
 * LOT-103).
 * }
 */
TEST(HdMockupRender, WritesASlowTravellingForTheAuthor) {
    const QString directory = qEnvironmentVariable("JADG_TRAVELLING_DIR");
    if (directory.isEmpty()) {
        GTEST_SKIP() << "JADG_TRAVELLING_DIR n'est pas posee : pas de travelling a ecrire.";
    }
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    const nlohmann::json scene = readJson(mockupDirectory() / "scene.json");
    ASSERT_FALSE(scene.is_discarded());

    // Un quart de pixel d'ecran par image, en largeur : a 1080p une case vaut 100 pixels, et un pas
    // d'une colonne de grille n'en avance que la moitie a l'ecran -- d'ou 0,005 colonne.
    constexpr int FRAMES = 96;
    constexpr float STEP_CELLS = 0.25F / 50.0F;
    const core::Vector2 start = sceneFocus(scene);
    std::vector<core::Vector2> focuses;
    for (int frame = 0; frame < FRAMES; ++frame) {
        const float offset = STEP_CELLS * static_cast<float>(frame);
        focuses.push_back({start.x + offset, start.y});
    }
    const std::vector<QImage> images = renderViews(*rhi, scene, QSize(1920, 1080), focuses);
    ASSERT_EQ(images.size(), focuses.size());

    const std::filesystem::path output(directory.toStdWString());
    std::filesystem::create_directories(output);
    for (std::size_t index = 0; index < images.size(); ++index) {
        char name[32];
        std::snprintf(name, sizeof(name), "travelling-%03zu.png", index);
        EXPECT_TRUE(images[index].save(QString::fromStdWString((output / name).wstring())));
    }
}
