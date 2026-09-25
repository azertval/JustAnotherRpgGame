// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_static_world_scene.cpp
 * @brief Le lieu composé une fois puis découpé à la vue (`hmi::StaticWorldScene`, audit de
 *        l'affichage d'un lieu) : une image rend **exactement** ce que rendait la composition
 *        complète, privée de ce que la caméra ne montre pas.
 *
 * Deux lieux, sans GPU : la place du bourg d'essai (pièces de planche, textures factices à la
 * taille du manifeste) et une maquette à étages (blocs extrudés, aplat), où le héros passe sous les
 * étages.
 */

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/StaticWorldScene.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

/// Un lieu prêt à composer : son instantané, sa projection, des textures sans image.
struct Place {
    hmi::WorldSceneSnapshot snapshot;
    hmi::ScenePieceTextures textures;
    /// Les identités des textures factices : leur adresse est la poignée.
    std::vector<std::uint8_t> identities = std::vector<std::uint8_t>(4096);

    [[nodiscard]] core::IsoProjection projection() const {
        return core::IsoProjection(snapshot.columns, snapshot.rows, core::ARENA_TILE_WIDTH_UNITS,
                                   snapshot.diamondRatio);
    }
};

/// La place du bourg d'essai, le héros au milieu.
[[nodiscard]] Place bourg() {
    const std::filesystem::path data{JADG_TEST_DATA_DIR};
    const core::LevelLoadResult map =
        core::LevelLoader::loadFromFile(data / "Levels" / "bourg" / "place.json");
    const hmi::PlaceAppearanceResult appearance =
        hmi::PlaceAppearance::loadForPlace(data / "Assets", "bourg");
    const core::ScenePieceManifestResult manifest =
        core::ScenePieceManifest::resolve(data / "Assets", "bourg");
    EXPECT_TRUE(map.ok() && appearance.ok() && manifest.ok());
    Place place;
    if (!map.ok() || !appearance.ok() || !manifest.ok()) {
        return place;
    }
    const float middle = static_cast<float>(map.level->tileMap().width()) / 2.0F;
    place.snapshot = hmi::snapshotWorldScene(
        *map.level, appearance.appearance,
        {hmi::WorldFigureSnapshot{.figure = "figurant", .point = {middle, middle}, .hero = true}});
    std::size_t next = 0;
    for (const core::ScenePiece& piece : manifest.manifest.pieces()) {
        place.textures.byPath[piece.path()] = hmi::SceneTexture{
            .texture = &place.identities[next++], .width = piece.width, .height = piece.height};
    }
    // Le damier tient lieu des figurines, sans image ici : elles se composent quand même.
    place.textures.missing =
        hmi::SceneTexture{.texture = &place.identities[next++], .width = 64, .height = 64};
    place.textures.solid =
        hmi::SceneTexture{.texture = &place.identities[next], .width = 1, .height = 1};
    return place;
}

/// Une maquette de 24 × 24 : à l'ouest, des îlots de murs montés d'un ou deux étages ; à l'est, la
/// rase campagne.
[[nodiscard]] Place maquetteAEtages(core::Vector2 hero) {
    constexpr int SIDE = 24;
    core::LevelData data{.name = "maquette", .tileMap = core::TileMap{SIDE, SIDE}};
    core::TileLayer ground{.name = "sol",
                           .kind = core::LayerKind::Ground,
                           .tiles = core::TileMap{SIDE, SIDE},
                           .properties = {},
                           .floor = 0};
    core::TileLayer relief{.name = "relief",
                           .kind = core::LayerKind::Decor,
                           .tiles = core::TileMap{SIDE, SIDE},
                           .properties = {},
                           .floor = 0};
    core::TileLayer first{.name = "etage1",
                          .kind = core::LayerKind::Decor,
                          .tiles = core::TileMap{SIDE, SIDE},
                          .properties = {},
                          .floor = 1};
    core::TileLayer second{.name = "etage2",
                           .kind = core::LayerKind::Decor,
                           .tiles = core::TileMap{SIDE, SIDE},
                           .properties = {},
                           .floor = 2};
    for (int row = 0; row < SIDE; ++row) {
        for (int column = 0; column < SIDE; ++column) {
            data.tileMap.setTile(column, row, core::TileType::Grass);
            ground.tiles.setTile(column, row, core::TileType::Grass);
            // Des îlots de murs, montés de un ou deux étages : de quoi masquer le héros.
            if (column < SIDE / 2 && (column % 6 == 2 || column % 6 == 3) && row % 5 != 0) {
                relief.tiles.setTile(column, row, core::TileType::Wall);
                first.tiles.setTile(column, row, core::TileType::Wall);
                if (row % 2 == 0) {
                    second.tiles.setTile(column, row, core::TileType::Wall);
                }
            }
        }
    }
    data.tileMap.setTile(0, 0, core::TileType::Entry);
    data.layers = {std::move(ground), std::move(relief), std::move(first), std::move(second)};
    Place place;
    place.snapshot = hmi::snapshotWorldScene(
        core::Level{std::move(data)}, hmi::PlaceAppearance{},
        {hmi::WorldFigureSnapshot{.figure = "figurant", .point = hero, .hero = true}});
    place.textures.missing =
        hmi::SceneTexture{.texture = &place.identities[0], .width = 64, .height = 64};
    place.textures.solid =
        hmi::SceneTexture{.texture = &place.identities[1], .width = 1, .height = 1};
    return place;
}

/// La composition complète triée : ce que le jeu dessinait avant la découpe.
[[nodiscard]] hmi::ComposedScene full(const Place& place) {
    return hmi::composeWorldScene(place.snapshot, place.projection(), place.textures);
}

/// L'image que compose la scène statique, sous @p view s'il est donné.
[[nodiscard]] hmi::ComposedScene image(const Place& place, const hmi::StaticWorldScene& statics,
                                       const std::optional<core::Rect>& view = std::nullopt) {
    hmi::ComposedScene out;
    if (view) {
        out.setVisibleBounds(*view);
    }
    statics.compose(out, place.snapshot.figures, place.textures);
    return out;
}

[[nodiscard]] bool same(const hmi::ComposedQuad& a, const hmi::ComposedQuad& b) {
    return a.layer == b.layer && a.texture == b.texture && a.textureRank == b.textureRank &&
           a.sortOrder == b.sortOrder && a.kind == b.kind && a.storey == b.storey &&
           a.sprite == b.sprite && a.line == b.line && a.poly == b.poly;
}

[[nodiscard]] core::Rect boundsOf(const hmi::ComposedQuad& quad) {
    switch (quad.kind) {
        case hmi::QuadKind::Sprite:
            return hmi::spriteQuadBounds(quad.sprite);
        case hmi::QuadKind::Line:
            return hmi::lineQuadBounds(quad.line);
        case hmi::QuadKind::Poly:
            return hmi::polyQuadBounds(quad.poly);
    }
    return {};
}

void expectSameSequence(const std::vector<hmi::ComposedQuad>& expected,
                        const std::vector<hmi::ComposedQuad>& actual) {
    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index) {
        ASSERT_TRUE(same(actual[index], expected[index])) << "primitive " << index;
    }
}

/// Un cadrage de @p columns × @p rows cases, centré sur la case @p centre.
[[nodiscard]] core::Rect viewAround(const Place& place, core::Vector2 centre, float columns,
                                    float rows) {
    const core::IsoProjection projection = place.projection();
    const core::Vector2 middle = projection.gridToWorld(centre);
    const core::Vector2 size{columns * projection.tileWidth(), rows * projection.tileWidth()};
    return core::Rect{{middle.x - (size.x / 2.0F), middle.y - (size.y / 2.0F)}, size};
}

[[nodiscard]] int fadedStoreys(const hmi::ComposedScene& scene) {
    int faded = 0;
    for (const hmi::ComposedQuad& quad : scene.quads()) {
        const float alpha = quad.kind == hmi::QuadKind::Sprite ? quad.sprite.a : quad.poly.a;
        if (quad.storey > 0 && alpha < 1.0F) {
            ++faded;
        }
    }
    return faded;
}

}  // namespace

/**
 * @brief Sans cadrage, l'image de la scène statique est la composition complète, primitive pour
 *        primitive.
 * \castest{<b>Composer la carte une fois ne change rien a ce qui se dessine.</b><br/>
 * \tcat Unitaire · Rendu d'un lieu<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Composer la place du bourg, puis une maquette a etages, par la composition complete
 * triee.<br/>2. Composer les memes lieux une fois par la scene statique, puis une image sans
 * cadrage, heros compris.<br/>
 * \tattendu Les deux listes sont identiques, dans le meme ordre : calque, texture et rang,
 * profondeur, geometrie, opacite (etages effaces devant le heros compris).
 * }
 */
TEST(StaticWorldSceneTest, SansCadrageLImageEstLaCompositionComplete) {
    for (const Place& place : {bourg(), maquetteAEtages({8.5F, 7.5F})}) {
        hmi::StaticWorldScene statics;
        statics.build(place.snapshot, place.projection(), place.textures);
        const hmi::ComposedScene expected = full(place);
        ASSERT_GT(expected.size(), 100U);
        expectSameSequence(expected.quads(), image(place, statics).quads());
    }
}

/**
 * @brief Un cadrage ne garde que ce qu'il montre, dans l'ordre de la composition complète.
 * \castest{<b>Une image ne compose que ce que la camera montre.</b><br/>
 * \tcat Unitaire · Rendu d'un lieu<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Composer la place du bourg une fois.<br/>2. Composer une image cadree sur dix cases
 * autour du heros, puis sur un coin de la carte.<br/>
 * \tattendu Chaque image est exactement la composition complete privee des primitives qui ne
 * touchent pas le cadrage (marge comprise) ; elle en ecarte une part, et les compteurs le disent.
 * }
 */
TEST(StaticWorldSceneTest, UnCadrageNeGardeQueCeQuIlMontre) {
    const Place place = bourg();
    hmi::StaticWorldScene statics;
    statics.build(place.snapshot, place.projection(), place.textures);
    const hmi::ComposedScene complete = full(place);
    const core::Vector2 hero = place.snapshot.figures.front().point;
    for (const core::Rect view :
         {viewAround(place, hero, 10.0F, 6.0F), viewAround(place, {2.0F, 2.0F}, 8.0F, 5.0F)}) {
        const hmi::ComposedScene framed = image(place, statics, view);
        hmi::ComposedScene reference;
        reference.setVisibleBounds(view);
        const core::Rect culling = reference.cullingBounds();
        std::vector<hmi::ComposedQuad> expected;
        for (const hmi::ComposedQuad& quad : complete.quads()) {
            if (boundsOf(quad).intersects(culling)) {
                expected.push_back(quad);
            }
        }
        expectSameSequence(expected, framed.quads());
        EXPECT_LT(framed.size(), complete.size());
        EXPECT_GT(framed.statistics().culled, 0);
    }
}

/**
 * @brief L'effacement des étages devant le héros se refait à chaque image, sans recomposer.
 * \castest{<b>Un etage s'efface devant le heros a chaque image.</b><br/>
 * \tcat Unitaire · Rendu d'un lieu · Etages<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Composer une fois une maquette a etages.<br/>2. Composer une image le heros au pied
 * d'un ilot de murs, puis une le heros en rase campagne, avec la meme scene statique.<br/>
 * \tattendu Au pied de l'ilot, des pieces d'etage sont effacees ; en rase campagne, aucune. La
 * scene statique, elle, n'a jamais d'etage efface.
 * }
 */
TEST(StaticWorldSceneTest, LEtageSEffaceDevantLeHerosAChaqueImage) {
    Place place = maquetteAEtages({2.5F, 3.5F});
    hmi::StaticWorldScene statics;
    statics.build(place.snapshot, place.projection(), place.textures);

    // Le héros derrière l'îlot de la colonne 2 : les étages dessinés après lui s'effacent.
    place.snapshot.figures.front().point = {2.2F, 2.5F};
    EXPECT_GT(fadedStoreys(image(place, statics)), 0);

    // En rase campagne, loin de tout mur : rien ne s'efface.
    place.snapshot.figures.front().point = {20.5F, 20.5F};
    EXPECT_EQ(fadedStoreys(image(place, statics)), 0);
    EXPECT_EQ(fadedStoreys(statics.scene()), 0);
}
