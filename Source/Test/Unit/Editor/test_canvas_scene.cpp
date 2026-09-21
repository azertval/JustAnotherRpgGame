// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_canvas_scene.cpp
 * @brief Ce que le canevas iso montre (`LOT-EDITOR-02`) : la même liste de primitives que le jeu
 *        sur la carte d'essai, et l'effet des réglages de couche sur les bandes de la scène.
 */

#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/World/WorldTravel.h"
#include "Editor/Logic/CanvasScene.h"
#include "Editor/Logic/LayerView.h"
#include "HMI/Game/WorldPlay.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

// La racine d'essai de l'éditeur (`LOT-123`) : ce test comparait le canevas au jeu sur une
// carte LIVRÉE, que la table rase du `LOT-102` emporte.
[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path(JADG_EDITOR_DATA_DIR);
}

[[nodiscard]] std::filesystem::path assets() {
    return dataRoot() / "Assets";
}

[[nodiscard]] core::LevelDraft draftOf(const std::string& relativePath) {
    core::LevelLoadResult loaded =
        core::LevelLoader::loadFromFile(dataRoot() / "Levels" / relativePath);
    if (!loaded.ok()) {
        throw std::runtime_error(relativePath + " : " + loaded.error);
    }
    return core::LevelDraft::fromLevel(*loaded.level);
}

[[nodiscard]] hmi::PlaceAppearance appearanceOf(const std::string& place) {
    hmi::PlaceAppearanceResult read =
        hmi::PlaceAppearance::loadFromFile(assets() / "Scene" / place / "appearance.json");
    if (!read.ok()) {
        throw std::runtime_error(place + " : " + read.message);
    }
    return std::move(read.appearance);
}

/// Des textures sans GPU, une identité distincte par chemin : ce que les deux compositions
/// comparées partagent, pour que seules les primitives puissent différer.
struct FakeTextures {
    hmi::ScenePieceTextures textures;
    std::vector<std::uint8_t> identities;

    explicit FakeTextures(const std::vector<std::string>& paths) : identities(paths.size() + 1) {
        for (std::size_t index = 0; index < paths.size(); ++index) {
            textures.byPath[paths[index]] = hmi::SceneTexture{
                .texture = &identities[index], .width = 68, .height = 96, .frameWidth = 48};
        }
        textures.missing = hmi::SceneTexture{
            .texture = &identities.back(), .width = 16, .height = 16, .frameWidth = 0};
    }
};

[[nodiscard]] bool sameQuad(const hmi::ComposedQuad& a, const hmi::ComposedQuad& b) {
    return a.layer == b.layer && a.texture == b.texture && a.sortOrder == b.sortOrder &&
           a.kind == b.kind && a.sprite.x == b.sprite.x && a.sprite.y == b.sprite.y &&
           a.sprite.width == b.sprite.width && a.sprite.height == b.sprite.height &&
           a.sprite.u0 == b.sprite.u0 && a.sprite.u1 == b.sprite.u1 && a.sprite.v0 == b.sprite.v0 &&
           a.sprite.v1 == b.sprite.v1 && a.sprite.a == b.sprite.a;
}

[[nodiscard]] core::TileLayer layer(std::string name, core::LayerKind kind) {
    return core::TileLayer{
        .name = std::move(name), .kind = kind, .tiles = core::TileMap(4, 4), .properties = {}};
}

}  // namespace

/**
 * @brief La carte d'essai ouverte dans l'éditeur produit la même liste de primitives que dans le jeu.
 * \castest{<b>Le canevas iso compose une carte comme le jeu.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Ouvrir la carte d'essai comme brouillon d'editeur et prendre l'instantane du
 *             canevas.<br/>
 *          2. Entrer dans la carte d'essai avec le moteur du jeu (`hmi::WorldPlay`) et prendre son
 *             instantane.<br/>
 *          3. Composer les deux avec les memes textures.<br/>
 * \tattendu Les instantanes sont egaux, heros mis a part ; les listes de primitives sont egales,
 *           dans le meme ordre, a la figurine du heros pres.
 * }
 */
TEST(CanvasSceneTest, LaCarteDEssaiSeComposeCommeDansLeJeu) {
    const core::LevelDraft draft = draftOf("bourg/place.json");
    const hmi::WorldSceneSnapshot editor = hmi::canvasSnapshot(draft, appearanceOf("bourg"));

    hmi::WorldPlay play(core::WorldTravel::directoryLoader(dataRoot() / "Levels"), assets());
    ASSERT_TRUE(play.enter("bourg/place", {}));
    hmi::WorldSceneSnapshot game = play.snapshot();
    ASSERT_FALSE(game.figures.empty());
    const hmi::WorldFigureSnapshot hero = game.figures.back();
    game.figures.pop_back();  // le héros : le jeu le pose, l'éditeur non.

    EXPECT_EQ(editor.place, "bourg");
    EXPECT_EQ(editor, game);
    EXPECT_FALSE(editor.figures.empty()) << "les sentinelles Ironhand ont leur figurine";

    // La même liste, dans le même ordre : le jeu n'a en plus que la figurine du héros.
    std::vector<std::string> paths = hmi::worldTexturePaths(editor);
    for (const char* clip : {"idle", "walk"}) {
        paths.push_back(hmi::figureStripPath(hero.figure, clip));
    }
    const FakeTextures fake(paths);
    const core::IsoProjection projection(editor.columns, editor.rows);
    const hmi::ComposedScene fromEditor = hmi::composeWorldScene(editor, projection, fake.textures);
    game.figures.push_back(hero);
    const hmi::ComposedScene fromGame = hmi::composeWorldScene(game, projection, fake.textures);

    ASSERT_EQ(fromGame.size(), fromEditor.size() + 1);
    std::size_t matched = 0;
    for (const hmi::ComposedQuad& quad : fromGame.quads()) {
        if (matched < fromEditor.size() && sameQuad(quad, fromEditor.quads()[matched])) {
            ++matched;
        }
    }
    EXPECT_EQ(matched, fromEditor.size()) << "l'ordre du jeu, primitive pour primitive";
    EXPECT_GT(fromEditor.size(), 1000U) << "La carte d'essai : sol et relief de 48 x 40 cases";
}

/**
 * @brief Les réglages d'une couche agissent sur sa bande de la scène iso.
 * \castest{<b>Masquer, griser, voir a travers : chaque reglage agit sur sa bande.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Une carte a couche de sol et couche de decor, reglages par defaut.<br/>
 *          2. Masquer le decor, griser le sol, activer les reliefs en transparence.<br/>
 *          3. Rendre la collision active.<br/>
 * \tattendu Par defaut tout est plein et la collision cachee ; masque, le relief tombe a 0 ;
 *           grise, le sol tombe a `DIMMED_LAYER_OPACITY` ; en transparence, le relief a
 *           `SEE_THROUGH_RELIEF_OPACITY` ; la collision ne se montre que quand on la peint.
 * }
 */
TEST(CanvasSceneTest, LesReglagesDeCoucheAgissentSurLeurBande) {
    const std::vector<core::TileLayer> layers = {layer("sol", core::LayerKind::Ground),
                                                 layer("relief", core::LayerKind::Decor)};
    hmi::LayerViewState view;
    view.sync(layers.size());

    const hmi::IsoBandOpacity defaults = hmi::isoBandOpacity(layers, view, 0U, false);
    EXPECT_EQ(defaults, (hmi::IsoBandOpacity{
                            .floors = 1.0F, .relief = 1.0F, .figures = 1.0F, .collision = 0.0F}));

    view.setVisible(1U, false);
    view.setDimmed(0U, true);
    const hmi::IsoBandOpacity changed = hmi::isoBandOpacity(layers, view, 0U, false);
    EXPECT_FLOAT_EQ(changed.relief, 0.0F);
    EXPECT_FLOAT_EQ(changed.floors, hmi::DIMMED_LAYER_OPACITY);
    EXPECT_FLOAT_EQ(hmi::bandOpacity(changed, hmi::RenderLayer::Object), 0.0F);
    EXPECT_FLOAT_EQ(hmi::bandOpacity(changed, hmi::RenderLayer::Tile), hmi::DIMMED_LAYER_OPACITY);

    view.setVisible(1U, true);
    EXPECT_FLOAT_EQ(hmi::isoBandOpacity(layers, view, 0U, true).relief,
                    hmi::SEE_THROUGH_RELIEF_OPACITY);

    const hmi::IsoBandOpacity collision = hmi::isoBandOpacity(layers, view, std::nullopt, false);
    EXPECT_FLOAT_EQ(collision.collision,
                    hmi::DEFAULT_COLLISION_OVERLAY_OPACITY * hmi::COLLISION_MASK_OPACITY);
}

/**
 * @brief Une carte à grille unique : la grille est l'image, sans masque de collision.
 * \castest{<b>Une grille unique est l'image du canevas iso.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Une carte sans couche visuelle ; masquer puis montrer sa grille.<br/>
 * \tattendu Le sol suit la grille racine ; aucun masque de collision n'est montre.
 * }
 */
TEST(CanvasSceneTest, UneGrilleUniqueEstLImage) {
    hmi::LayerViewState view;
    EXPECT_FLOAT_EQ(hmi::isoBandOpacity({}, view, std::nullopt, false).floors, 1.0F);
    EXPECT_FLOAT_EQ(hmi::isoBandOpacity({}, view, std::nullopt, false).collision, 0.0F);
    view.setVisible(std::nullopt, false);
    EXPECT_FLOAT_EQ(hmi::isoBandOpacity({}, view, std::nullopt, false).floors, 0.0F);
}

/**
 * @brief Griser et verrouiller sont des réglages de l'éditeur, oubliés à l'ouverture d'une carte.
 * \castest{<b>Griser et verrouiller une couche, puis tout oublier.</b><br/>
 * \tcat Unitaire · Editeur · Couches<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Verrouiller la collision, griser une couche visuelle.<br/>
 *          2. Remettre les reglages a zero.<br/>
 * \tattendu Les reglages se lisent sur chaque couche ; l'opacite effective d'une couche grisee
 *           est reduite ; apres remise a zero, plus rien n'est grise ni verrouille.
 * }
 */
TEST(CanvasSceneTest, GriserEtVerrouillerSontDesReglagesDEditeur) {
    hmi::LayerViewState view;
    view.sync(1);
    view.setLocked(std::nullopt, true);
    view.setDimmed(0U, true);
    EXPECT_TRUE(view.display(std::nullopt, true).locked);
    EXPECT_TRUE(view.display(0U, true).dimmed);
    EXPECT_FLOAT_EQ(view.display(0U, true).effectiveOpacity(), hmi::DIMMED_LAYER_OPACITY);

    view.reset();
    view.sync(1);
    EXPECT_FALSE(view.display(std::nullopt, true).locked);
    EXPECT_FALSE(view.display(0U, true).dimmed);
}

/**
 * @brief La barre d'état nomme les pièces de la case survolée.
 * \castest{<b>Les pieces d'une case se lisent dans la barre d'etat.</b><br/>
 * \tcat Unitaire · Editeur · Canevas<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Un instantane dont une case a un sol et un relief, une autre un sol seul.<br/>
 * \tattendu `sol · relief`, puis `sol` ; rien hors de la grille.
 * }
 */
TEST(CanvasSceneTest, LesPiecesDUneCaseSeLisent) {
    hmi::WorldSceneSnapshot snapshot;
    snapshot.columns = 2;
    snapshot.rows = 1;
    snapshot.floors = {"street", "square"};
    snapshot.relief = {"wall-left", ""};
    EXPECT_EQ(hmi::cellPieces(snapshot, {.column = 0, .row = 0}), "street · wall-left");
    EXPECT_EQ(hmi::cellPieces(snapshot, {.column = 1, .row = 0}), "square");
    EXPECT_EQ(hmi::cellPieces(snapshot, {.column = 5, .row = 0}), "");
}
