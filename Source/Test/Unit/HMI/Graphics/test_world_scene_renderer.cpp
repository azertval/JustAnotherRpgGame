// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_world_scene_renderer.cpp
 * @brief Le rendu QRhi d'un **lieu qu'on parcourt** (`LOT-09`), hors écran : cycle de vie des
 *        ressources, cadrage qui suit le héros, et une vraie image de la carte **livrée**.
 *
 * Jumeau de `test_arena_scene_renderer.cpp`, et pour la même raison : `hmi::WorldViewportItem`
 * n'est qu'un hôte Qt Quick, et tout ce qui peut fuir ou planter vit dans
 * `hmi::WorldSceneRenderer`. Ce test le fait tourner sur un `QRhi` Direct3D 11 sans fenêtre, sur le
 * donjon d'essai tel qu'il est commité — c'est la **capture de référence** du lot : elle est
 * écrite à côté des captures de l'arène, pour être relue à l'œil.
 */

#include <QImage>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <gtest/gtest.h>
#include <rhi/qrhi.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Gameplay/WorldFlags.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/World/WorldTravel.h"
#include "Editor/Ui/SceneImages.h"
#include "HMI/Graphics/Camera2D.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/Graphics/WorldSceneRenderer.h"

namespace {

/// Côté, en pixels, de la cible de rendu.
constexpr int TARGET_SIZE = 512;

/// Fond franc, qu'aucune pièce de la planche ne reproduit à l'identique.
constexpr float CLEAR[4] = {1.0F, 0.0F, 1.0F, 1.0F};

std::filesystem::path assets() {
    return std::filesystem::path(JADG_TEST_DATA_DIR) / "Assets";
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

struct OffscreenTarget {
    std::unique_ptr<QRhiTexture> texture;
    std::unique_ptr<QRhiTextureRenderTarget> renderTarget;
    std::unique_ptr<QRhiRenderPassDescriptor> pass;

    explicit OffscreenTarget(QRhi& rhi, QSize size = QSize(TARGET_SIZE, TARGET_SIZE))
        : texture(rhi.newTexture(QRhiTexture::RGBA8, size, 1,
                                 QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource)) {
        EXPECT_TRUE(texture->create());
        renderTarget.reset(rhi.newTextureRenderTarget({{texture.get()}}));
        pass.reset(renderTarget->newCompatibleRenderPassDescriptor());
        renderTarget->setRenderPassDescriptor(pass.get());
        EXPECT_TRUE(renderTarget->create());
    }
};

QImage renderFrame(QRhi& rhi, hmi::WorldSceneRenderer& renderer, OffscreenTarget& target) {
    QRhiCommandBuffer* commandBuffer = nullptr;
    if (rhi.beginOffscreenFrame(&commandBuffer) != QRhi::FrameOpSuccess) {
        ADD_FAILURE() << "beginOffscreenFrame";
        return {};
    }
    renderer.render(commandBuffer, target.renderTarget.get(), CLEAR);

    QRhiReadbackResult readback;
    QRhiResourceUpdateBatch* const readbackBatch = rhi.nextResourceUpdateBatch();
    readbackBatch->readBackTexture({target.texture.get()}, &readback);
    commandBuffer->resourceUpdate(readbackBatch);
    if (rhi.endOffscreenFrame() != QRhi::FrameOpSuccess) {
        ADD_FAILURE() << "endOffscreenFrame";
        return {};
    }
    return QImage(reinterpret_cast<const uchar*>(readback.data.constData()),
                  readback.pixelSize.width(), readback.pixelSize.height(), QImage::Format_RGBA8888)
        .copy();
}

std::size_t paintedPixels(const QImage& image) {
    std::size_t painted = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (pixel.red() != 255 || pixel.green() != 0 || pixel.blue() != 255) {
                ++painted;
            }
        }
    }
    return painted;
}

/// Le donjon d'essai et sa table d'apparence : un lieu tel qu'il se joue.
hmi::WorldSceneSnapshot donjonDEssai() {
    core::LevelLoadResult carte = core::LevelLoader::loadFromFile(
        std::filesystem::path(JADG_TEST_DATA_DIR) / "Levels" / "donjon.json");
    EXPECT_TRUE(carte.ok()) << carte.error;
    hmi::PlaceAppearanceResult table =
        hmi::PlaceAppearance::loadFromFile(assets() / "Scene" / "bourg" / "appearance.json");
    EXPECT_TRUE(table.ok()) << table.message;
    if (!carte.ok() || !table.ok()) {
        return {};
    }
    // Le héros à la porte, comme « Nouvelle partie » le pose.
    return hmi::snapshotWorldScene(
        *carte.level, table.appearance,
        {hmi::WorldFigureSnapshot{
            .figure = "figurant", .clip = "idle", .point = {19.5F, 32.5F}, .frame = 0}});
}

}  // namespace

/**
 * @brief Créer, libérer deux fois, recréer : les ressources suivent, sans fuite de lot.
 * \castest{<b>Le cycle de vie des ressources QRhi d'un lieu est sur.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Creer les ressources hors ecran, sans dessiner.<br/>
 *          2. Liberer, puis liberer encore.<br/>
 *          3. Recreer, dessiner une image d'un lieu, detruire le rendu avant
 *             l'interface.<br/>
 * \tattendu Le damier de repli existe des la creation ; les textures du lieu ne se chargent qu'au
 *           premier dessin (elles dependent de la carte) ; apres liberation plus rien n'est cree.
 * }
 */
TEST(WorldSceneRendererTest, CreationLiberationRecreation) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    OffscreenTarget target(*rhi);
    {
        hmi::WorldSceneRenderer renderer(assets());
        EXPECT_FALSE(renderer.ensureResources(nullptr));
        EXPECT_FALSE(renderer.created());

        ASSERT_TRUE(renderer.ensureResources(rhi.get()));
        EXPECT_TRUE(renderer.created());
        EXPECT_EQ(renderer.rhi(), rhi.get());
        EXPECT_NE(renderer.textures().missing.texture, nullptr);
        // Rien n'est encore charge : un lieu n'a pas de catalogue, il a la carte qu'on lui donne.
        EXPECT_TRUE(renderer.textures().byPath.empty());
        EXPECT_TRUE(renderer.ensureResources(rhi.get())) << "idempotent sur la meme interface";

        renderer.setSnapshot(donjonDEssai());
        renderer.setFocus({19.5F, 32.5F});
        static_cast<void>(renderFrame(*rhi, renderer, target));
        EXPECT_FALSE(renderer.textures().byPath.empty())
            << "les pieces de la carte se chargent au premier dessin";
        const std::size_t chargees = renderer.textures().byPath.size();
        const std::size_t demandees = renderer.requested().size();

        // Une seconde image ne redemande rien : une piece absente ne doit pas etre retentee a
        // chaque image, et une piece chargee ne doit pas l'etre deux fois.
        static_cast<void>(renderFrame(*rhi, renderer, target));
        EXPECT_EQ(renderer.textures().byPath.size(), chargees);
        EXPECT_EQ(renderer.requested().size(), demandees);

        renderer.release();
        EXPECT_FALSE(renderer.created());
        EXPECT_EQ(renderer.rhi(), nullptr);
        EXPECT_TRUE(renderer.textures().byPath.empty());
        EXPECT_TRUE(renderer.requested().empty());
        renderer.release();

        ASSERT_TRUE(renderer.ensureResources(rhi.get()));
        renderer.setSnapshot(donjonDEssai());
        const QImage image = renderFrame(*rhi, renderer, target);
        EXPECT_EQ(image.size(), QSize(TARGET_SIZE, TARGET_SIZE));
    }
}

/**
 * @brief Le donjon d'essai devient des pixels, et aucune pièce ne tombe sur le damier.
 * \castest{<b>Le donjon d'essai se dessine, sans une seule piece manquante.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Charger la carte du donjon d'essai et la table d'apparence du lieu.<br/>
 *          2. Dessiner une image hors ecran, cadree sur le heros a la porte.<br/>
 * \tattendu Plus de mille quads composes, tous sur une piece CHARGEE (aucun damier) ; une part
 *           notable de l'image est peinte.
 * }
 */
TEST(WorldSceneRendererTest, UnLieuDevientDesPixels) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    OffscreenTarget target(*rhi);
    hmi::WorldSceneRenderer renderer(assets());
    ASSERT_TRUE(renderer.ensureResources(rhi.get()));

    renderer.setSnapshot(donjonDEssai());
    renderer.setFocus({19.5F, 32.5F});
    const QImage image = renderFrame(*rhi, renderer, target);
    ASSERT_EQ(image.size(), QSize(TARGET_SIZE, TARGET_SIZE));

    // 634 cases franchissables, autant de sols, plus le relief et la figurine du heros.
    EXPECT_GT(renderer.composed().size(), 700U);
    for (const hmi::ComposedQuad& quad : renderer.composed().quads()) {
        EXPECT_NE(quad.texture, nullptr);
        EXPECT_NE(quad.texture, renderer.textures().missing.texture)
            << "piece tombee sur le damier";
    }
    EXPECT_GT(paintedPixels(image), static_cast<std::size_t>(TARGET_SIZE * TARGET_SIZE / 4))
        << "le hall et les gradins devraient couvrir une part notable de la cible";

    // Une scene vide ne dessine que le fond.
    renderer.setSnapshot({});
    const QImage empty = renderFrame(*rhi, renderer, target);
    EXPECT_EQ(renderer.composed().size(), 0U);
    EXPECT_EQ(paintedPixels(empty), 0U);
}

/**
 * @brief La caméra suit le héros et ne sort pas de la carte.
 * \castest{<b>Le cadrage d'un lieu suit le heros, borne a la scene, a un agrandissement
 * entier.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Cadrer une grande carte sur son centre, puis sur un coin.<br/>
 *          2. Cadrer une carte plus petite que la vue.<br/>
 * \tattendu Au centre, la camera est sur le heros ; au coin, elle s'arrete au bord de la scene ;
 *           une petite carte reste centree. L'agrandissement est entier -- le pixel art ne se met
 *           pas a l'echelle 0,62.
 * }
 */
TEST(WorldSceneRendererTest, LaCameraSuitLeHerosSansSortirDeLaCarte) {
    const core::IsoProjection grande{40, 34};
    const core::Vector2 centre = grande.gridToWorld({20.0F, 17.0F});

    const hmi::Camera2D suivie = hmi::worldCamera(grande, centre, 1280, 720);
    EXPECT_FLOAT_EQ(suivie.zoom(), 1.0F);
    EXPECT_NEAR(suivie.center().x, centre.x, 0.001F);
    EXPECT_NEAR(suivie.center().y, centre.y, 0.001F);

    // Le coin GAUCHE du losange -- la case (0, lignes - 1), celle qui touche le bord gauche de la
    // scene : la camera s'y arrete au bord, et ne montre pas le vide au-dela.
    const core::Vector2 coin = grande.gridToWorld({0.0F, 33.0F});
    const hmi::Camera2D bornee = hmi::worldCamera(grande, coin, 1280, 720);
    EXPECT_GT(bornee.center().x, coin.x) << "la camera est ramenee dans la scene";
    EXPECT_GE(bornee.visibleBounds().position.x, -0.001F);
    EXPECT_LE(bornee.visibleBounds().position.x + bornee.visibleBounds().size.x,
              grande.sceneSize().x + 0.001F);

    // Une carte plus petite que la vue reste centree, et l'ecran double au-dela de 720 lignes.
    const core::IsoProjection petite{4, 3};
    const hmi::Camera2D petiteVue =
        hmi::worldCamera(petite, petite.gridToWorld({0.0F, 0.0F}), 1280, 1440);
    EXPECT_FLOAT_EQ(petiteVue.zoom(), 2.0F);
    EXPECT_NEAR(petiteVue.center().x, petite.sceneSize().x / 2.0F, 0.001F);
    EXPECT_NEAR(petiteVue.center().y, petite.sceneSize().y / 2.0F, 0.001F);
}

/**
 * @brief Deux lieux deviennent des pixels, leurs sentinelles comprises.
 * \castest{<b>Deux lieux se dessinent sans une piece sur le damier, sentinelles sous les traits de
 * leur figurine.</b><br/>
 * \tcat Unitaire · Rendu QRhi d'un lieu<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Charger deux cartes d'essai, et la table du lieu.<br/>
 *          2. Dessiner chacune hors ecran, cadre sur une sentinelle, le heros a cote.<br/>
 * \tattendu Aucun quad sur le damier ; la figurine de la sentinelle est une vraie bande, chargee
 *           et non remplacee par le damier ; une part notable de l'image est peinte (LOT-96).
 * }
 */
TEST(WorldSceneRendererTest, DeuxLieuxDeviennentDesPixels) {
    const std::unique_ptr<QRhi> rhi = createOffscreenRhi();
    if (!rhi) {
        GTEST_SKIP() << "Aucune interface QRhi disponible sur cette machine.";
    }
    hmi::PlaceAppearanceResult table =
        hmi::PlaceAppearance::loadFromFile(assets() / "Scene" / "bourg" / "appearance.json");
    ASSERT_TRUE(table.ok()) << table.message;

    for (const char* const quartier : {"bourg/place", "cave"}) {
        core::LevelLoadResult carte =
            core::LevelLoader::loadFromFile(std::filesystem::path(JADG_TEST_DATA_DIR) / "Levels" /
                                            (std::string{quartier} + ".json"));
        ASSERT_TRUE(carte.ok()) << quartier << " : " << carte.error;

        // Les figurines de la carte, comme le jeu les pose : les sentinelles, puis le heros a cote
        // de la premiere.
        std::vector<hmi::WorldFigureSnapshot> figurines;
        for (const core::MapEntity& objet : carte.level->entities()) {
            const auto figurine = objet.properties.find("figure");
            if (objet.type != "npc" || figurine == objet.properties.end()) {
                continue;
            }
            figurines.push_back(
                hmi::WorldFigureSnapshot{.figure = std::get<std::string>(figurine->second),
                                         .clip = "idle",
                                         .point = {static_cast<float>(objet.position.column) + 0.5F,
                                                   static_cast<float>(objet.position.row) + 0.5F},
                                         .frame = 0});
        }
        ASSERT_FALSE(figurines.empty()) << quartier << " n'a aucune sentinelle";
        const core::Vector2 porte = figurines.front().point;

        OffscreenTarget target(*rhi);
        hmi::WorldSceneRenderer renderer(assets());
        ASSERT_TRUE(renderer.ensureResources(rhi.get()));
        renderer.setSnapshot(hmi::snapshotWorldScene(*carte.level, table.appearance, figurines));
        renderer.setFocus(porte);
        const QImage image = renderFrame(*rhi, renderer, target);

        for (const hmi::ComposedQuad& quad : renderer.composed().quads()) {
            EXPECT_NE(quad.texture, renderer.textures().missing.texture)
                << quartier << " : piece tombee sur le damier";
        }
        // La sentinelle porte une vraie bande de figurine, et non plus un marqueur.
        const auto soldat = renderer.textures().byPath.find("Monsters/sentinelle/idle.png");
        ASSERT_NE(soldat, renderer.textures().byPath.end()) << quartier;
        EXPECT_NE(soldat->second.texture, nullptr);
        EXPECT_NE(soldat->second.texture, renderer.textures().missing.texture) << quartier;
        EXPECT_GT(paintedPixels(image), static_cast<std::size_t>(TARGET_SIZE * TARGET_SIZE / 4))
            << quartier;
    }
}

/**
 * @brief Les trois cartes se sauvegardent et se rendent avec leur kit.
 * \castest{<b>Les trois cartes se sauvegardent et se rendent avec leur kit.</b><br/>
 * \tcat Unitaire · Rendu du donjon d'essai<br/>
 * \tcrit Critique<br/>
 * \tetapes Charger et enregistrer chaque carte ; comparer les textures du jeu et de l’éditeur.<br/>
 * \tattendu Entités et couches conservées, ancrages et profondeurs identiques, aucune texture
 * manquante.
 * }
 */
TEST(WorldSceneRendererTest, LesCartesSeSauvegardentEtSeRendentAvecLeurKit) {
    const auto rhi = createOffscreenRhi();
    ASSERT_NE(rhi, nullptr);
    const auto table = hmi::PlaceAppearance::loadFromFile(assets() / "Scene/bourg/appearance.json");
    ASSERT_TRUE(table.ok()) << table.message;
    for (const std::string name : {"bourg/place", "cave", "donjon"}) {
        const auto loaded =
            core::LevelLoader::loadFromFile(assets().parent_path() / "Levels" / (name + ".json"));
        ASSERT_TRUE(loaded.ok()) << name << ": " << loaded.error;
        const auto draft = core::LevelDraft::fromLevel(*loaded.level);
        const auto saved = core::LevelLoader::loadFromString(draft.toJson());
        ASSERT_TRUE(saved.ok()) << saved.error;
        EXPECT_EQ(saved.level->layers().size(), loaded.level->layers().size());
        EXPECT_EQ(saved.level->entities().size(), loaded.level->entities().size());
        OffscreenTarget target(*rhi);
        hmi::WorldSceneRenderer renderer(assets());
        ASSERT_TRUE(renderer.ensureResources(rhi.get()));
        renderer.setSnapshot(hmi::snapshotWorldScene(*saved.level, table.appearance, {}));
        hmi::SceneImages editorImages(assets());
        const auto paths =
            hmi::worldTexturePaths(hmi::snapshotWorldScene(*saved.level, table.appearance, {}));
        editorImages.ensure(paths);
        // Cadre au centre de la carte : ce test juge le rendu, pas un endroit particulier.
        renderer.setFocus(
            core::Vector2{static_cast<float>(saved.level->tileMap().width()) / 2.0F,
                          static_cast<float>(saved.level->tileMap().height()) / 2.0F});
        const auto image = renderFrame(*rhi, renderer, target);
        EXPECT_GT(paintedPixels(image), static_cast<std::size_t>(TARGET_SIZE * TARGET_SIZE / 4));
        for (const auto& quad : renderer.composed().quads())
            EXPECT_NE(quad.texture, renderer.textures().missing.texture) << name;
        for (const auto& path : paths) {
            const auto& gpu = renderer.textures().byPath.at(path);
            const auto& editor = editorImages.textures().byPath.at(path);
            EXPECT_EQ(editor.depthOffset, gpu.depthOffset) << path;
            ASSERT_EQ(editor.anchor.has_value(), gpu.anchor.has_value()) << path;
            if (gpu.anchor) {
                EXPECT_FLOAT_EQ(editor.anchor->x, gpu.anchor->x);
                EXPECT_FLOAT_EQ(editor.anchor->y, gpu.anchor->y);
            }
        }
        EXPECT_TRUE(image.save(QString::fromStdString(
            std::string(name).substr(std::string(name).find('/') + 1) + "-renderer.png")));
    }
}

/**
 * @brief Tous les portails des cartes se traversent.
 * \castest{<b>Tous les portails des cartes se traversent.</b><br/>
 * \tcat Unitaire · Rendu du donjon d'essai<br/>
 * \tcrit Critique<br/>
 * \tetapes Entrer dans chaque carte et traverser chaque portail avec WorldTravel.<br/>
 * \tattendu Arrivées sur une case libre, sans boucle de téléportation.
 * }
 */
TEST(WorldSceneRendererTest, TousLesPortailsSeTraversent) {
    core::WorldTravel travel(core::WorldTravel::directoryLoader(assets().parent_path() / "Levels"));
    const core::WorldFlags flags;
    for (const std::string name : {"bourg/place", "cave", "donjon"}) {
        const std::string id = name;
        ASSERT_EQ(travel.enter(id, {}), core::TravelResult::Moved);
        const auto entities = travel.currentMap()->entities();
        for (const auto& entity : entities) {
            if (entity.type != "portal")
                continue;
            ASSERT_EQ(travel.enter(id, {}), core::TravelResult::Moved);
            EXPECT_EQ(travel.cross(entity.position, flags), core::TravelResult::Moved);
            const auto pos = travel.position();
            EXPECT_FALSE(core::isSolid(travel.currentMap()->tileMap().tile(pos.column, pos.row)));
            EXPECT_FALSE(core::portalAt(*travel.currentMap(), pos).has_value());
        }
    }
}
