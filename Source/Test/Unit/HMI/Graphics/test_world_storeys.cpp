// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_world_storeys.cpp
 * @brief Les étages de la scène (`LOT-129`) : une couche de décor à l'étage `n` se dessine élevée
 * de `n` hauteurs d'étage, triée au-dessus du rez de sa case, et s'efface devant le héros.
 *
 * La composition seule, sans GPU : des textures factices au losange du standard (256 × 159), une
 * hauteur d'étage déclarée comme le manifeste d'un lieu la déclare.
 */

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/ScenePieces.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

/// La hauteur d'étage du lieu d'essai, en pixels d'art : celle des murs du kit de la Capitale.
constexpr float STOREY_PIXELS = 196.0F;

[[nodiscard]] hmi::PlaceAppearance table() {
    hmi::PlaceAppearanceResult read = hmi::PlaceAppearance::loadFromString(
        R"({"version": 1, "place": "haut", "floors": {}, "relief": {}})");
    EXPECT_TRUE(read.ok()) << read.message;
    return std::move(read.appearance);
}

[[nodiscard]] core::TileLayer decor(std::string name, int floor, const std::string& piece,
                                    core::GridPosition cell) {
    core::TileLayer layer{.name = std::move(name),
                          .kind = core::LayerKind::Decor,
                          .tiles = core::TileMap{3, 3},
                          .properties = {},
                          .floor = floor};
    layer.tiles.setTile(cell.column, cell.row, core::TileType::Wall);
    layer.setPiece(cell.column, cell.row, piece);
    return layer;
}

/**
 * Une carte de 3 × 3 : un mur au rez en (1, 1), un étage de mur au-dessus, un toit au sommet. Les
 * couches sont déclarées dans le désordre : l'ordre des étages vient de `floor`, pas du fichier.
 */
[[nodiscard]] core::Level island() {
    core::LevelData data{.name = "ilot", .tileMap = core::TileMap{3, 3}};
    data.layers.push_back(core::TileLayer{.name = "sol",
                                          .kind = core::LayerKind::Ground,
                                          .tiles = core::TileMap{3, 3},
                                          .properties = {{"scene", std::string{"haut"}}}});
    data.layers.push_back(decor("toit", 2, "roof", {.column = 1, .row = 1}));
    data.layers.push_back(decor("etage", 1, "wall-upper", {.column = 1, .row = 1}));
    data.layers.push_back(decor("rez", 0, "wall", {.column = 1, .row = 1}));
    return core::Level{std::move(data)};
}

/// Une identité de texture factice : la composition ne lit jamais au travers.
[[nodiscard]] hmi::TextureHandle handle(std::uintptr_t rank) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
    return reinterpret_cast<hmi::TextureHandle>(rank);
}

/// Des textures factices au losange du standard ; @p storey : la hauteur d'étage déclarée.
[[nodiscard]] hmi::ScenePieceTextures textures(const std::vector<std::string>& paths,
                                               std::optional<float> storey) {
    hmi::ScenePieceTextures resolved;
    std::uintptr_t rank = 1;
    for (const std::string& path : paths) {
        const bool figure = path.find("Npc/") != std::string::npos;
        resolved.byPath.emplace(path,
                                hmi::SceneTexture{.texture = handle(rank++),
                                                  .width = figure ? 192 : 256,
                                                  .height = figure ? 256 : 400,
                                                  .frameWidth = figure ? 192 : 0,
                                                  .artTile = {256.0F, 159.0F},
                                                  .storeyHeight = figure ? std::nullopt : storey});
    }
    return resolved;
}

/// Les primitives de relief de la scène composée, par étage.
struct Composed {
    hmi::ComposedScene scene;
    std::vector<const hmi::ComposedQuad*> byStorey;  // rang = étage
    const hmi::ComposedQuad* hero = nullptr;
};

[[nodiscard]] Composed compose(const hmi::WorldSceneSnapshot& snapshot,
                               std::optional<float> storey = STOREY_PIXELS) {
    Composed result;
    result.scene = hmi::composeWorldScene(snapshot, core::IsoProjection{3, 3},
                                          textures(hmi::worldTexturePaths(snapshot), storey));
    result.byStorey.assign(3, nullptr);
    for (const hmi::ComposedQuad& quad : result.scene.quads()) {
        if (quad.layer == hmi::RenderLayer::Object && quad.storey >= 0 && quad.storey < 3) {
            result.byStorey[static_cast<std::size_t>(quad.storey)] = &quad;
        } else if (quad.layer == hmi::RenderLayer::Player) {
            result.hero = &quad;
        }
    }
    return result;
}

[[nodiscard]] hmi::WorldFigureSnapshot hero(core::Vector2 point) {
    return hmi::WorldFigureSnapshot{
        .figure = "brawler", .clip = "idle", .point = point, .frame = 0, .hero = true};
}

}  // namespace

/**
 * @brief L'instantané range les couches d'étage par étage, et le rez reste la couche au rez.
 * \castest{<b>Les couches d'etage entrent dans l'instantane, rangees par etage.</b><br/>
 * \tcat Unitaire · Lieu compose · Etages<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Batir une carte dont les couches de decor sont declarees toit, etage, rez.<br/>
 *          2. En tirer l'instantane.<br/>
 * \tattendu Le relief du rez est le mur ; deux etages, le premier puis le second, portent l'etage
 *           de mur et le toit a la case (1, 1).
 * }
 */
TEST(WorldStoreysTest, LesEtagesEntrentDansLInstantaneRangesParEtage) {
    const hmi::WorldSceneSnapshot snapshot = hmi::snapshotWorldScene(island(), table(), {});
    EXPECT_EQ(snapshot.reliefAt({.column = 1, .row = 1}), "wall");
    ASSERT_EQ(snapshot.storeys.size(), 2U);
    EXPECT_EQ(snapshot.storeys[0].floor, 1);
    EXPECT_EQ(snapshot.storeys[1].floor, 2);
    EXPECT_EQ(snapshot.storeys[0].relief[4], "wall-upper");
    EXPECT_EQ(snapshot.storeys[1].relief[4], "roof");
    const std::vector<std::string> paths = hmi::worldTexturePaths(snapshot);
    EXPECT_NE(std::ranges::find(paths, "Scene/haut/roof.png"), paths.end());
}

/**
 * @brief Un étage s'élève de la hauteur que déclare le manifeste de son lieu, sans réglage par
 * pièce.
 * \castest{<b>Un etage s'eleve de la hauteur declaree par son lieu.</b><br/>
 * \tcat Unitaire · Lieu compose · Etages<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Composer l'ilot avec une hauteur d'etage de 196 pixels d'art, puis sans.<br/>
 * \tattendu Chaque etage monte de 196 pixels d'art a l'echelle du lieu au-dessus du precedent ;
 * sans hauteur declaree, d'une largeur de case ; chaque piece porte son etage.
 * }
 */
TEST(WorldStoreysTest, UnEtageSEleveDeLaHauteurDeclareeParSonLieu) {
    const hmi::WorldSceneSnapshot snapshot = hmi::snapshotWorldScene(island(), table(), {});
    const core::IsoProjection projection{3, 3};
    const float step = STOREY_PIXELS * projection.tileWidth() / 256.0F;

    const Composed declared = compose(snapshot);
    for (int storey = 0; storey < 3; ++storey) {
        ASSERT_NE(declared.byStorey[static_cast<std::size_t>(storey)], nullptr) << storey;
        EXPECT_EQ(declared.byStorey[static_cast<std::size_t>(storey)]->storey, storey);
    }
    EXPECT_NEAR(declared.byStorey[0]->sprite.y - declared.byStorey[1]->sprite.y, step, 1e-3F);
    EXPECT_NEAR(declared.byStorey[1]->sprite.y - declared.byStorey[2]->sprite.y, step, 1e-3F);
    EXPECT_FLOAT_EQ(declared.byStorey[0]->sprite.x, declared.byStorey[2]->sprite.x);

    const Composed undeclared = compose(snapshot, std::nullopt);
    EXPECT_NEAR(undeclared.byStorey[0]->sprite.y - undeclared.byStorey[1]->sprite.y,
                hmi::DEFAULT_STOREY_TILES * projection.tileWidth(), 1e-3F);
}

/**
 * @brief Un étage se trie avec sa case : après le rez et la figurine qui s'y tient, avant ce qui
 * est devant.
 * \castest{<b>Un etage se trie au-dessus du rez de sa case.</b><br/>
 * \tcat Unitaire · Lieu compose · Etages<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Composer l'ilot, un heros sur la case devant le batiment.<br/>
 * \tattendu Rez, etage, toit dans cet ordre ; le heros, devant, passe apres les trois.
 * }
 */
TEST(WorldStoreysTest, UnEtageSeTrieAuDessusDuRezDeSaCase) {
    const hmi::WorldSceneSnapshot snapshot =
        hmi::snapshotWorldScene(island(), table(), {hero({2.5F, 2.5F})});
    const Composed composed = compose(snapshot);
    ASSERT_NE(composed.hero, nullptr);
    EXPECT_LT(composed.byStorey[0]->sortOrder, composed.byStorey[1]->sortOrder);
    EXPECT_LT(composed.byStorey[1]->sortOrder, composed.byStorey[2]->sortOrder);
    EXPECT_GT(composed.hero->sortOrder, composed.byStorey[2]->sortOrder);
}

/**
 * @brief Un étage qui masque le héros s'efface ; ni le rez, ni un étage qui ne le masque pas, ni un
 *        étage devant un PNJ ne s'effacent.
 * \castest{<b>Un etage qui masque le heros s'efface.</b><br/>
 * \tcat Unitaire · Lieu compose · Etages<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Poser le heros derriere le batiment, sur la case (1, 0).<br/>
 *          2. Le poser devant, sur la case (2, 2).<br/>
 *          3. Poser un PNJ derriere, sans heros.<br/>
 * \tattendu Derriere : l'etage et le toit prennent l'opacite STOREY_SEE_THROUGH_OPACITY, le rez
 *           reste opaque. Devant, ou pour un PNJ : tout reste opaque.
 * }
 */
TEST(WorldStoreysTest, UnEtageQuiMasqueLeHerosSEfface) {
    const Composed behind =
        compose(hmi::snapshotWorldScene(island(), table(), {hero({1.5F, 0.5F})}));
    EXPECT_FLOAT_EQ(behind.byStorey[0]->sprite.a, 1.0F);
    EXPECT_FLOAT_EQ(behind.byStorey[1]->sprite.a, hmi::STOREY_SEE_THROUGH_OPACITY);
    EXPECT_FLOAT_EQ(behind.byStorey[2]->sprite.a, hmi::STOREY_SEE_THROUGH_OPACITY);

    const Composed inFront =
        compose(hmi::snapshotWorldScene(island(), table(), {hero({2.5F, 2.5F})}));
    for (const hmi::ComposedQuad* quad : inFront.byStorey) {
        EXPECT_FLOAT_EQ(quad->sprite.a, 1.0F);
    }

    hmi::WorldFigureSnapshot npc = hero({1.5F, 0.5F});
    npc.hero = false;
    const Composed npcBehind = compose(hmi::snapshotWorldScene(island(), table(), {npc}));
    for (const hmi::ComposedQuad* quad : npcBehind.byStorey) {
        EXPECT_FLOAT_EQ(quad->sprite.a, 1.0F);
    }
}

/**
 * @brief Une couche d'étage hors bornes, ou un étage sur une couche de sol, est gardée par le
 * format mais n'est pas dessinée comme un étage.
 * \castest{<b>Un etage hors bornes n'est pas joue.</b><br/>
 * \tcat Unitaire · Lieu compose · Etages<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ajouter une couche de decor a l'etage 9 et une couche de sol a l'etage 1.<br/>
 * \tattendu L'instantane n'a que les deux etages joues.
 * }
 */
TEST(WorldStoreysTest, UnEtageHorsBornesNEstPasJoue) {
    core::LevelData data = island().data();
    data.layers.push_back(decor("trop-haut", 9, "roof", {.column = 0, .row = 0}));
    core::TileLayer groundUp{.name = "sol-haut",
                             .kind = core::LayerKind::Ground,
                             .tiles = core::TileMap{3, 3},
                             .properties = {},
                             .floor = 1};
    data.layers.push_back(std::move(groundUp));
    const hmi::WorldSceneSnapshot snapshot =
        hmi::snapshotWorldScene(core::Level{std::move(data)}, table(), {});
    EXPECT_EQ(snapshot.storeys.size(), 2U);
}
