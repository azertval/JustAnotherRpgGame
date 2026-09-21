// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_arena_scene_composer.cpp
 * @brief Tests unitaires du composeur de la scène de combat du Colisée (Phase 3 du LOT-86) : nombre
 *        de quads, ordre des calques, combattants à terre et sortis — sans GPU.
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/Arena.h"
#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/TileMap.h"
#include "HMI/Graphics/ArenaAnimationState.h"
#include "HMI/Graphics/ArenaAppearanceCatalog.h"
#include "HMI/Graphics/ArenaSceneComposer.h"

namespace {

using core::CombatantId;
using core::CombatSide;
using hmi::RenderLayer;

constexpr const char* CATALOG_JSON = R"({
  "heroes": ["kaelith_voss", "bram", "elira", "darin"],
  "gladiators": ["gladiator_sword_shield", "gladiator_lance", "retiarius", "archer"],
  "scene": "coliseum",
  "paleSlabs": ["01", "02", "03", "04", "05", "10", "11", "13", "14", "15"],
  "heroFrames": 5,
  "enemyFrames": 8
})";

hmi::ArenaAppearanceCatalog catalog() {
    hmi::ArenaAppearanceCatalogResult result =
        hmi::ArenaAppearanceCatalog::loadFromString(CATALOG_JSON);
    EXPECT_TRUE(result.ok()) << result.error;
    return result.ok() ? *result.catalog : hmi::ArenaAppearanceCatalog{};
}

/**
 * Une piste 5x4 ceinte de murs, une porte en (0, 2). Au compte :
 * - 20 sols ;
 * - 13 cases de mur, une piece chacune (LOT-92 : une piece porte son mur) : l'angle du fond (0, 0),
 *   trois piliers aux autres angles, huit pans et un pan a torche en (4, 2) (bord droit, ligne 2) ;
 *   aucune banniere (aucune colonne multiple de 5 hors des angles) ;
 * - 1 arche, sur la porte.
 * Soit 20 quads `Tile` et 14 quads `Object`.
 */
core::Level piste() {
    core::TileMap carte(5, 4);
    for (int x = 0; x < 5; ++x) {
        carte.setTile(x, 0, core::TileType::Wall);
        carte.setTile(x, 3, core::TileType::Wall);
    }
    for (int y = 0; y < 4; ++y) {
        carte.setTile(0, y, core::TileType::Wall);
        carte.setTile(4, y, core::TileType::Wall);
    }
    carte.setTile(0, 2, core::TileType::Empty);
    return core::Level(core::LevelData{.name = "piste",
                                       .tileMap = std::move(carte),
                                       .entities = {},
                                       .entry = {1, 1}});
}

constexpr int FLOOR_QUADS = 20;
constexpr int STRUCTURE_QUADS = 14;

core::ArenaContestant concurrent(const std::string& nom, CombatSide camp, int colonne, int ligne) {
    const core::CombatantProfile profil{
        .name = nom, .side = camp, .maximumHitPoints = 10, .currentHitPoints = 10, .movement = 6};
    return {.profile = profil,
            .attacks = {},
            .position = core::GridPosition{.column = colonne, .row = ligne},
            .markId = {}};
}

/// Trois allies, deux ennemis, tous sur les six cases interieures.
core::ArenaBout affrontement() {
    core::ArenaBout bout{.seed = 7, .lethal = false, .heroicMark = false};
    bout.contestants.push_back(concurrent("Bram", CombatSide::Allies, 1, 1));
    bout.contestants.push_back(concurrent("Cid", CombatSide::Allies, 2, 1));
    bout.contestants.push_back(concurrent("Eve", CombatSide::Allies, 3, 1));
    bout.contestants.push_back(concurrent("Orc", CombatSide::Enemies, 1, 2));
    bout.contestants.push_back(concurrent("Rat", CombatSide::Enemies, 2, 2));
    return bout;
}

/// Identite opaque : la composition ne dereference jamais une texture.
hmi::TextureHandle handle(std::uintptr_t value) {
    return reinterpret_cast<hmi::TextureHandle>(value);
}

/// Chemin d'une piece de scene de l'atelier (LOT-92), relatif au dossier du Colisee.
std::string scenePiece(const std::string& name) {
    return "../Scene/coliseum/" + name + ".png";
}

/// Hauteur d'une piece debout de l'atelier, en pixels d'art (manifeste du Colisee, classe `tall`).
constexpr int STANDING_HEIGHT = 100;

/// Toutes les pieces que la piste et ses combattants demandent, aux dimensions de l'atelier.
hmi::ArenaSceneTextures textures(const hmi::ArenaAppearanceCatalog& appearance) {
    hmi::ArenaSceneTextures result;
    std::uintptr_t next = 1;
    const auto add = [&](const std::string& path, int width, int height) {
        result.byPath[path] = hmi::ArenaTexture{handle(next++), width, height};
    };
    for (const char* floor : {"sand", "sand-2", "sand-3", "sand-blood", "stone-slab", "gate-threshold"}) {
        add(scenePiece(floor), 68, 42);
    }
    for (const char* piece : {"wall-corner", "pillar", "wall-left", "wall-right", "banner-left",
                              "banner-right", "torch-left", "torch-right", "arch-left", "arch-right"}) {
        add(scenePiece(piece), 68, STANDING_HEIGHT);
    }
    for (const std::string& hero : appearance.heroes()) {
        const std::string directory = appearance.sheetDirectory(hero, CombatSide::Allies);
        add(directory + "/idle.png", 240, 64);
        add(directory + "/death.png", 240, 64);
    }
    for (const std::string& gladiator : appearance.gladiators()) {
        add("enemies/" + gladiator + "/idle.png", 384, 64);
    }
    result.missing = hmi::ArenaTexture{handle(9999), 16, 16};
    return result;
}

hmi::TextureHandle figureTexture(const hmi::ArenaSceneTextures& all,
                                 const hmi::ArenaAppearanceCatalog& appearance,
                                 const std::string& name, CombatSide side, const char* strip) {
    const hmi::FigureAppearance figure = appearance.figureFor(name, side);
    return all.resolve(figure.directory + "/" + strip).texture;
}

std::vector<hmi::ComposedQuad> onLayer(const hmi::ComposedScene& scene, RenderLayer layer) {
    std::vector<hmi::ComposedQuad> result;
    std::copy_if(scene.quads().begin(), scene.quads().end(), std::back_inserter(result),
                 [layer](const hmi::ComposedQuad& quad) { return quad.layer == layer; });
    return result;
}

const hmi::ComposedQuad* withTexture(const hmi::ComposedScene& scene, hmi::TextureHandle texture) {
    const auto found =
        std::find_if(scene.quads().begin(), scene.quads().end(),
                     [texture](const hmi::ComposedQuad& quad) { return quad.texture == texture; });
    return found != scene.quads().end() ? &*found : nullptr;
}

CombatantId idOf(const core::ArenaSession& session, const std::string& name) {
    for (const CombatantId id : session.combat().combatants()) {
        if (session.combat().find(id)->profile.name == name) {
            return id;
        }
    }
    ADD_FAILURE() << "combattant introuvable : " << name;
    return CombatantId{};
}

class ArenaSceneComposerTest : public ::testing::Test {
protected:
    void SetUp() override {
        const core::ArenaMount mount = session.mount(affrontement());
        ASSERT_TRUE(mount.refusals.empty());
        ASSERT_EQ(mount.allies.size(), 3U);
        ASSERT_EQ(mount.enemies.size(), 2U);
    }

    /// La figurine posee sur @p cell : deux combattants peuvent partager une meme bande.
    [[nodiscard]] const hmi::ComposedQuad* figureAt(const hmi::ComposedScene& scene,
                                                    hmi::TextureHandle texture,
                                                    core::GridPosition cell) const {
        const float centerX = projection.tileToWorld(cell).x;
        const auto found = std::find_if(
            scene.quads().begin(), scene.quads().end(), [&](const hmi::ComposedQuad& quad) {
                return quad.layer == RenderLayer::Player && quad.texture == texture &&
                       std::abs(quad.sprite.x + quad.sprite.width / 2.0f - centerX) < 1e-3f;
            });
        return found != scene.quads().end() ? &*found : nullptr;
    }

    [[nodiscard]] hmi::ComposedScene compose() const {
        return hmi::composeArenaScene(session, appearance, animation, projection, sceneTextures);
    }

    core::ArenaSession session{piste()};
    hmi::ArenaAppearanceCatalog appearance = catalog();
    hmi::ArenaSceneTextures sceneTextures = textures(appearance);
    hmi::ArenaAnimationState animation;
    core::IsoProjection projection{5, 4};
};

}  // namespace

/**
 * @brief Chaque case donne un sol, chaque piece d'enceinte un quad, chaque combattant une figurine.
 * \castest{<b>La scene composee compte exactement les quads attendus, par calque.</b><br/>
 * \tcat Unitaire · Composeur de la scene de l'arene<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Monter cinq combattants sur la piste 5x4.<br/>
 *          2. Composer la scene.<br/>
 * \tattendu 20 quads Tile, 14 Object, 5 Player ; aucun sur un autre calque.
 * }
 */
TEST_F(ArenaSceneComposerTest, NombreDeQuadsParCalque) {
    const hmi::ComposedScene scene = compose();
    EXPECT_EQ(onLayer(scene, RenderLayer::Tile).size(), static_cast<std::size_t>(FLOOR_QUADS));
    EXPECT_EQ(onLayer(scene, RenderLayer::Object).size(),
              static_cast<std::size_t>(STRUCTURE_QUADS));
    EXPECT_EQ(onLayer(scene, RenderLayer::Player).size(), 5U);
    EXPECT_EQ(scene.size(), static_cast<std::size_t>(FLOOR_QUADS + STRUCTURE_QUADS + 5));
}

/**
 * @brief Apres tri, tout le sol passe avant l'enceinte et les figurines, et la bande de profondeur
 *        est rangee par pied croissant.
 * \castest{<b>Le sol est dessine sous tout ; la bande de profondeur est triee par pied.</b><br/>
 * \tcat Unitaire · Composeur de la scene de l'arene<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Composer la scene (triee).<br/>
 * \tattendu Les 20 premiers quads sont Tile ; les suivants sont Object ou Player, a sortOrder non
 *           decroissant.
 * }
 */
TEST_F(ArenaSceneComposerTest, OrdreDesCalques) {
    const hmi::ComposedScene scene = compose();
    const std::vector<hmi::ComposedQuad>& quads = scene.quads();
    ASSERT_EQ(quads.size(), static_cast<std::size_t>(FLOOR_QUADS + STRUCTURE_QUADS + 5));
    for (std::size_t index = 0; index < quads.size(); ++index) {
        if (index < static_cast<std::size_t>(FLOOR_QUADS)) {
            EXPECT_EQ(quads[index].layer, RenderLayer::Tile) << "quad " << index;
        } else {
            EXPECT_TRUE(hmi::sortsByDepth(quads[index].layer)) << "quad " << index;
            if (index > static_cast<std::size_t>(FLOOR_QUADS)) {
                EXPECT_LE(quads[index - 1].sortOrder, quads[index].sortOrder) << "quad " << index;
            }
        }
    }
}

/**
 * @brief Une figurine passe devant le mur du fond et derriere le mur de devant.
 * \castest{<b>La profondeur suit le pied de la case.</b><br/>
 * \tcat Unitaire · Composeur de la scene de l'arene<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Composer la scene.<br/>
 *          2. Reperer la figurine de Bram (1, 1), le pan (1, 0), le pan (1, 3) et le pan a torche
 *             (4, 2).<br/>
 * \tattendu pan (1, 0) < Bram < pan (1, 3) ; la case (4, 2) porte le pan a torche, une seule piece.
 * }
 */
TEST_F(ArenaSceneComposerTest, ProfondeurAuPiedDeLaCase) {
    const hmi::ComposedScene scene = compose();
    const std::vector<hmi::ComposedQuad>& quads = scene.quads();
    const auto indexOf = [&](const hmi::ComposedQuad* quad) {
        return static_cast<std::size_t>(quad - quads.data());
    };
    const auto footOf = [&](core::GridPosition cell) {
        const core::Rect bounds = projection.tileBounds(cell);
        return bounds.position.y + bounds.size.y;
    };
    const auto structureAt = [&](core::GridPosition cell, hmi::ArenaDepthSlot slot) {
        const std::int32_t order = hmi::arenaDepthSortOrder(footOf(cell), slot);
        const core::Rect bounds = projection.tileBounds(cell);
        const float centerX = bounds.position.x + bounds.size.x / 2.0f;
        const auto found =
            std::find_if(quads.begin(), quads.end(), [&](const hmi::ComposedQuad& quad) {
                return quad.layer == RenderLayer::Object && quad.sortOrder == order &&
                       std::abs(quad.sprite.x + quad.sprite.width / 2.0f - centerX) < 1e-3f;
            });
        EXPECT_NE(found, quads.end())
            << "piece introuvable en (" << cell.column << ", " << cell.row << ")";
        return found != quads.end() ? &*found : nullptr;
    };

    const hmi::ComposedQuad* bram = figureAt(
        scene, figureTexture(sceneTextures, appearance, "Bram", CombatSide::Allies, "idle.png"),
        {.column = 1, .row = 1});
    ASSERT_NE(bram, nullptr);
    const hmi::ComposedQuad* back = structureAt({.column = 1, .row = 0}, hmi::ArenaDepthSlot::Wall);
    const hmi::ComposedQuad* front =
        structureAt({.column = 1, .row = 3}, hmi::ArenaDepthSlot::Wall);
    const hmi::ComposedQuad* torch =
        structureAt({.column = 4, .row = 2}, hmi::ArenaDepthSlot::Wall);
    ASSERT_NE(back, nullptr);
    ASSERT_NE(front, nullptr);
    ASSERT_NE(torch, nullptr);

    EXPECT_EQ(torch->texture, sceneTextures.resolve(scenePiece("torch-left")).texture);
    EXPECT_LT(indexOf(back), indexOf(bram));
    EXPECT_LT(indexOf(bram), indexOf(front));
}

/**
 * @brief Chaque piece de l'atelier se pose par son ancre et se dresse contre l'arete du fond
 *        parallele a son bord : pans et arche orientes, angle du fond, piliers, seuil sous la porte.
 * \castest{<b>Le decor de l'atelier des textures (LOT-92) est pose par son ancre, dans le bon
 * sens.</b><br/>
 * \tcat Unitaire · Composeur de la scene de l'arene<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Composer la piste 5x4.<br/>
 *          2. Relever la piece et le sol de (1, 0), (0, 1), (0, 0), (4, 3), (0, 2).<br/>
 * \tattendu (1, 0) wall-right et (0, 1) wall-left, dont le sommet haut du losange tombe sur le
 * pixel (34, 58) de la texture ; (0, 0) wall-corner ; (4, 3) pillar ; (0, 2) arch-left sur
 * gate-threshold.
 * }
 */
TEST_F(ArenaSceneComposerTest, LeDecorSePoseParSonAncreDansLeBonSens) {
    const hmi::ComposedScene composed = compose();
    const float unitsPerArtPixel =
        projection.tileWidth() / static_cast<float>(hmi::ARENA_SCENE_TILE_WIDTH_PIXELS);
    const auto pieceAt = [&](core::GridPosition cell, RenderLayer layer) {
        const core::Vector2 top = projection.gridToWorld(
            {static_cast<float>(cell.column), static_cast<float>(cell.row)});
        const auto found = std::find_if(
            composed.quads().begin(), composed.quads().end(), [&](const hmi::ComposedQuad& quad) {
                return quad.layer == layer &&
                       std::abs(quad.sprite.x + quad.sprite.width / 2.0f - top.x) < 1e-3f &&
                       quad.sprite.y <= top.y + 1e-3f &&
                       quad.sprite.y + quad.sprite.height >= top.y - 1e-3f;
            });
        EXPECT_NE(found, composed.quads().end())
            << "rien en (" << cell.column << ", " << cell.row << ")";
        return found != composed.quads().end() ? &*found : nullptr;
    };
    const auto is = [&](const hmi::ComposedQuad* quad, const char* name) {
        return quad != nullptr && quad->texture == sceneTextures.resolve(scenePiece(name)).texture;
    };

    const hmi::ComposedQuad* back = pieceAt({.column = 1, .row = 0}, RenderLayer::Object);
    EXPECT_TRUE(is(back, "wall-right"));
    const hmi::ComposedQuad* side = pieceAt({.column = 0, .row = 1}, RenderLayer::Object);
    EXPECT_TRUE(is(side, "wall-left"));
    ASSERT_NE(side, nullptr);
    const core::Vector2 top = projection.gridToWorld({0.0f, 1.0f});
    EXPECT_NEAR(side->sprite.x, top.x - 34.0f * unitsPerArtPixel, 1e-3f);
    EXPECT_NEAR(side->sprite.y, top.y - (STANDING_HEIGHT - 42) * unitsPerArtPixel, 1e-3f);
    EXPECT_NEAR(side->sprite.width, 68.0f * unitsPerArtPixel, 1e-3f);

    EXPECT_TRUE(is(pieceAt({.column = 0, .row = 0}, RenderLayer::Object), "wall-corner"));
    EXPECT_TRUE(is(pieceAt({.column = 4, .row = 3}, RenderLayer::Object), "pillar"));
    EXPECT_TRUE(is(pieceAt({.column = 0, .row = 2}, RenderLayer::Object), "arch-left"));
    EXPECT_TRUE(is(pieceAt({.column = 0, .row = 2}, RenderLayer::Tile), "gate-threshold"));
}

/**
 * @brief A terre, un combattant garde sa figurine (derniere image ; bande de mort pour un allie,
 *        estompee pour un ennemi) ; sorti, il n'en a plus.
 * \castest{<b>Down compose une figurine, Withdrawn aucune.</b><br/>
 * \tcat Unitaire · Composeur de la scene de l'arene<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Demarrer le combat.<br/>
 *          2. Abattre Bram (allie) et Orc (ennemi), faire sortir Cid.<br/>
 *          3. Composer la scene.<br/>
 * \tattendu 4 figurines ; Bram sur death.png a l'image 4 ; Orc sur idle.png a l'image 7, alpha
 *           0,45 ; aucun quad pour Cid.
 * }
 */
TEST_F(ArenaSceneComposerTest, ATerreEtSorti) {
    ASSERT_TRUE(session.start());
    core::CombatState& combat = session.combat();
    combat.applyDamage(idOf(session, "Bram"), 100);
    combat.applyDamage(idOf(session, "Orc"), 100);
    ASSERT_EQ(combat.withdraw(idOf(session, "Cid")), core::WithdrawResult::Withdrawn);
    ASSERT_EQ(combat.find(idOf(session, "Bram"))->status, core::CombatantStatus::Down);
    ASSERT_EQ(combat.find(idOf(session, "Orc"))->status, core::CombatantStatus::Down);
    ASSERT_EQ(combat.find(idOf(session, "Cid"))->status, core::CombatantStatus::Withdrawn);
    ASSERT_NE(combat.phase(), core::CombatPhase::Ended);

    const hmi::ComposedScene scene = compose();
    EXPECT_EQ(onLayer(scene, RenderLayer::Player).size(), 4U);

    const hmi::ComposedQuad* bram = withTexture(
        scene, figureTexture(sceneTextures, appearance, "Bram", CombatSide::Allies, "death.png"));
    ASSERT_NE(bram, nullptr);
    EXPECT_FLOAT_EQ(bram->sprite.u0, 4.0f * 48.0f / 240.0f);
    EXPECT_FLOAT_EQ(bram->sprite.u1, 1.0f);
    EXPECT_FLOAT_EQ(bram->sprite.a, 1.0f);

    const hmi::ComposedQuad* orc = withTexture(
        scene, figureTexture(sceneTextures, appearance, "Orc", CombatSide::Enemies, "idle.png"));
    ASSERT_NE(orc, nullptr);
    EXPECT_FLOAT_EQ(orc->sprite.u0, 7.0f * 48.0f / 384.0f);
    EXPECT_FLOAT_EQ(orc->sprite.a, hmi::ARENA_DOWN_ENEMY_ALPHA);

    const hmi::TextureHandle cid =
        figureTexture(sceneTextures, appearance, "Cid", CombatSide::Allies, "idle.png");
    const hmi::TextureHandle eve =
        figureTexture(sceneTextures, appearance, "Eve", CombatSide::Allies, "idle.png");
    ASSERT_NE(cid, eve) << "Cid et Eve doivent avoir deux figurines distinctes pour ce test";
    EXPECT_EQ(withTexture(scene, cid), nullptr);
}

/**
 * @brief Debout, la figurine montre l'image de l'etat d'animation, ramenee dans sa bande.
 * \castest{<b>L'image courante vient de ArenaAnimationState, bornee a la bande.</b><br/>
 * \tcat Unitaire · Composeur de la scene de l'arene<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Donner l'image 3 a Eve et l'image 99 a Rat.<br/>
 *          2. Composer la scene.<br/>
 * \tattendu Eve echantillonne [144, 192[ de 240 px ; Rat sa derniere image (7) ; opacite pleine.
 * }
 */
TEST_F(ArenaSceneComposerTest, ImageCouranteBornee) {
    animation.figures[idOf(session, "Eve")].frame = 3;
    animation.figures[idOf(session, "Rat")].frame = 99;
    const hmi::ComposedScene scene = compose();

    const hmi::ComposedQuad* eve = figureAt(
        scene, figureTexture(sceneTextures, appearance, "Eve", CombatSide::Allies, "idle.png"),
        {.column = 3, .row = 1});
    ASSERT_NE(eve, nullptr);
    EXPECT_FLOAT_EQ(eve->sprite.u0, 144.0f / 240.0f);
    EXPECT_FLOAT_EQ(eve->sprite.u1, 192.0f / 240.0f);
    EXPECT_FLOAT_EQ(eve->sprite.v1, 1.0f);

    const hmi::ComposedQuad* rat = withTexture(
        scene, figureTexture(sceneTextures, appearance, "Rat", CombatSide::Enemies, "idle.png"));
    ASSERT_NE(rat, nullptr);
    EXPECT_FLOAT_EQ(rat->sprite.u0, 7.0f * 48.0f / 384.0f);
    EXPECT_FLOAT_EQ(rat->sprite.a, 1.0f);
}

/**
 * @brief Une piece non chargee se lie au damier ; sans damier, elle n'est pas composee.
 * \castest{<b>Repli sur le damier, puis rien.</b><br/>
 * \tcat Unitaire · Composeur de la scene de l'arene<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Composer sans aucune texture chargee, damier fourni.<br/>
 *          2. Composer sans aucune texture ni damier.<br/>
 * \tattendu 1. Les 39 quads, tous sur le damier. 2. Aucun quad.
 * }
 */
TEST_F(ArenaSceneComposerTest, RepliSurLeDamier) {
    hmi::ArenaSceneTextures onlyMissing;
    onlyMissing.missing = sceneTextures.missing;
    const hmi::ComposedScene fallback =
        hmi::composeArenaScene(session, appearance, animation, projection, onlyMissing);
    EXPECT_EQ(fallback.size(), static_cast<std::size_t>(FLOOR_QUADS + STRUCTURE_QUADS + 5));
    EXPECT_TRUE(std::all_of(fallback.quads().begin(), fallback.quads().end(),
                            [&](const hmi::ComposedQuad& quad) {
                                return quad.texture == onlyMissing.missing.texture;
                            }));

    const hmi::ComposedScene empty = hmi::composeArenaScene(session, appearance, animation,
                                                            projection, hmi::ArenaSceneTextures{});
    EXPECT_EQ(empty.size(), 0U);
}

/**
 * @brief Composer ne change rien a la session, et recomposer donne les memes quads.
 * \castest{<b>La composition est une lecture seule (EX-ARCH-012).</b><br/>
 * \tcat Unitaire · Composeur de la scene de l'arene<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Demarrer le combat, relever phase, tour actif, journal et positions.<br/>
 *          2. Composer deux fois.<br/>
 * \tattendu Session inchangee ; les deux scenes ont les memes quads, dans le meme ordre.
 * }
 */
TEST_F(ArenaSceneComposerTest, LectureSeule) {
    ASSERT_TRUE(session.start());
    const core::CombatPhase phase = session.combat().phase();
    const auto active = session.combat().activeCombatant();
    const std::size_t journal = session.journal().size();
    std::vector<std::optional<core::GridPosition>> positions;
    for (const CombatantId id : session.combat().combatants()) {
        positions.push_back(session.combat().grid().positionOf(id));
    }

    const hmi::ComposedScene first = compose();
    const hmi::ComposedScene second = compose();

    EXPECT_EQ(session.combat().phase(), phase);
    EXPECT_EQ(session.combat().activeCombatant(), active);
    EXPECT_EQ(session.journal().size(), journal);
    std::vector<std::optional<core::GridPosition>> after;
    for (const CombatantId id : session.combat().combatants()) {
        after.push_back(session.combat().grid().positionOf(id));
    }
    EXPECT_EQ(after, positions);

    ASSERT_EQ(first.size(), second.size());
    for (std::size_t index = 0; index < first.size(); ++index) {
        const hmi::ComposedQuad& a = first.quads()[index];
        const hmi::ComposedQuad& b = second.quads()[index];
        EXPECT_EQ(a.layer, b.layer);
        EXPECT_EQ(a.texture, b.texture);
        EXPECT_EQ(a.sortOrder, b.sortOrder);
        EXPECT_FLOAT_EQ(a.sprite.x, b.sprite.x);
        EXPECT_FLOAT_EQ(a.sprite.y, b.sprite.y);
    }
}

/**
 * @brief L'instantane porte tout ce que la composition dessine : compose apres la mort de la
 *        session, il donne exactement la scene de la session vivante.
 * \castest{<b>L'instantane en valeurs survit a sa session (LOT-86 Phase 5).</b><br/>
 * \tcat Unitaire · Composeur de la scene de l'arene<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Abattre Bram, faire sortir Cid, tirer l'instantane.<br/>
 *          2. Detruire une copie de la session, composer l'instantane.<br/>
 * \tattendu Grille 5x4, 13 cases obstruees, 4 figurines (Cid absent, Bram a terre) ; memes quads,
 * dans le meme ordre, que la composition depuis la session.
 * }
 */
TEST_F(ArenaSceneComposerTest, InstantaneSurvitALaSession) {
    ASSERT_TRUE(session.start());
    session.combat().applyDamage(idOf(session, "Bram"), 100);
    ASSERT_EQ(session.combat().withdraw(idOf(session, "Cid")), core::WithdrawResult::Withdrawn);

    hmi::ArenaSceneSnapshot snapshot;
    {
        const auto ephemeral = std::make_unique<core::ArenaSession>(piste());
        ASSERT_TRUE(ephemeral->mount(affrontement()).refusals.empty());
        ASSERT_TRUE(ephemeral->start());
        ephemeral->combat().applyDamage(idOf(*ephemeral, "Bram"), 100);
        ASSERT_EQ(ephemeral->combat().withdraw(idOf(*ephemeral, "Cid")),
                  core::WithdrawResult::Withdrawn);
        snapshot = hmi::snapshotArenaScene(*ephemeral);
        EXPECT_EQ(snapshot, hmi::snapshotArenaScene(session));
    }

    EXPECT_EQ(snapshot.columns, 5);
    EXPECT_EQ(snapshot.rows, 4);
    EXPECT_EQ(std::count(snapshot.obstructed.begin(), snapshot.obstructed.end(), true), 13);
    ASSERT_EQ(snapshot.figures.size(), 4U);
    EXPECT_TRUE(std::none_of(snapshot.figures.begin(), snapshot.figures.end(),
                             [](const hmi::ArenaFigureSnapshot& f) { return f.name == "Cid"; }));
    const auto bram =
        std::find_if(snapshot.figures.begin(), snapshot.figures.end(),
                     [](const hmi::ArenaFigureSnapshot& f) { return f.name == "Bram"; });
    ASSERT_NE(bram, snapshot.figures.end());
    EXPECT_TRUE(bram->down);
    EXPECT_EQ(bram->anchor, (core::GridPosition{.column = 1, .row = 1}));

    hmi::ComposedScene fromSnapshot;
    hmi::composeArenaScene(fromSnapshot, snapshot, appearance, animation, projection,
                           sceneTextures);
    fromSnapshot.sort();
    const hmi::ComposedScene fromSession = compose();
    ASSERT_EQ(fromSnapshot.size(), fromSession.size());
    for (std::size_t index = 0; index < fromSession.size(); ++index) {
        const hmi::ComposedQuad& a = fromSnapshot.quads()[index];
        const hmi::ComposedQuad& b = fromSession.quads()[index];
        EXPECT_EQ(a.texture, b.texture) << "quad " << index;
        EXPECT_EQ(a.sortOrder, b.sortOrder) << "quad " << index;
        EXPECT_FLOAT_EQ(a.sprite.x, b.sprite.x) << "quad " << index;
        EXPECT_FLOAT_EQ(a.sprite.u0, b.sprite.u0) << "quad " << index;
        EXPECT_FLOAT_EQ(a.sprite.a, b.sprite.a) << "quad " << index;
    }
}

/**
 * @brief Charger exactement `arenaTexturePaths` suffit : aucune piece composee ne tombe sur le
 *        damier.
 * \castest{<b>La liste des textures couvre tout ce que la composition demande.</b><br/>
 * \tcat Unitaire · Composeur de la scene de l'arene<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lier chaque chemin de arenaTexturePaths a une texture, et le damier a une autre.<br/>
 *          2. Composer la piste avec un allie a terre.<br/>
 * \tattendu Les 39 quads, aucun sur le damier ; la liste n'a pas de doublon.
 * }
 */
TEST_F(ArenaSceneComposerTest, ListeDesTexturesCouvreLaComposition) {
    const std::vector<std::string> paths = hmi::arenaTexturePaths(appearance);
    std::vector<std::string> sorted = paths;
    std::sort(sorted.begin(), sorted.end());
    EXPECT_EQ(std::adjacent_find(sorted.begin(), sorted.end()), sorted.end());

    hmi::ArenaSceneTextures listed;
    std::uintptr_t next = 1;
    for (const std::string& path : paths) {
        listed.byPath[path] = hmi::ArenaTexture{handle(next++), 48, 64};
    }
    listed.missing = hmi::ArenaTexture{handle(9999), 16, 16};

    ASSERT_TRUE(session.start());
    session.combat().applyDamage(idOf(session, "Bram"), 100);
    const hmi::ComposedScene scene =
        hmi::composeArenaScene(session, appearance, animation, projection, listed);
    EXPECT_EQ(scene.size(), static_cast<std::size_t>(FLOOR_QUADS + STRUCTURE_QUADS + 5));
    EXPECT_TRUE(std::none_of(
        scene.quads().begin(), scene.quads().end(),
        [&](const hmi::ComposedQuad& quad) { return quad.texture == listed.missing.texture; }));
}

/**
 * @brief Une figurine de remplacement (atelier des PNJ, LOT-91) se lit dans son propre dossier,
 *        avec la decoupe que ses bandes declarent : six images de 48 px au repos, six de 96 px a
 *        terre.
 * \castest{<b>Un heros remplace se dessine depuis `../Npc/<slug>` ; une bande large (96 px)
 * donne un quad deux fois plus large, centre au meme endroit, et sa derniere image a terre.</b>
 * <br/>
 * \tcat Unitaire · Composeur de la scene de l'arene<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Remplacer la figurine de Bram par `../Npc/anariel`, avec `idle.png` 288x64
 *          (frameWidth 48) et `death.png` 576x64 (frameWidth 96).<br/>
 *          2. Composer debout a l'image 5, puis a terre.<br/>
 * \tattendu Debout : la texture de `../Npc/anariel/idle.png`, u dans [240, 288[ de 288, largeur
 * 48 x echelle. A terre : `death.png`, u dans [480, 576[ de 576, largeur doublee, meme centre.
 * }
 */
TEST_F(ArenaSceneComposerTest, UneFigurineDeRemplacementSuitSaPropreDecoupe) {
    const std::string sheet = appearance.figureFor("Bram", CombatSide::Allies).sheet;
    ASSERT_TRUE(appearance.replaceHero(sheet, "../Npc/anariel"));
    const hmi::TextureHandle idle = handle(7001);
    const hmi::TextureHandle death = handle(7002);
    sceneTextures.byPath["../Npc/anariel/idle.png"] = hmi::ArenaTexture{idle, 288, 64, 48};
    sceneTextures.byPath["../Npc/anariel/death.png"] = hmi::ArenaTexture{death, 576, 64, 96};

    ASSERT_TRUE(session.start());
    const CombatantId bram = idOf(session, "Bram");
    animation.figures[bram] = hmi::ArenaFigureAnimation{.frame = 5};
    const hmi::ComposedScene debout = compose();
    const hmi::ComposedQuad* repos = withTexture(debout, idle);
    ASSERT_NE(repos, nullptr);
    EXPECT_FLOAT_EQ(repos->sprite.u0, 5.0f * 48.0f / 288.0f);
    EXPECT_FLOAT_EQ(repos->sprite.u1, 1.0f);
    const float centreDebout = repos->sprite.x + repos->sprite.width / 2.0f;

    session.combat().applyDamage(bram, 100);
    ASSERT_EQ(session.combat().find(bram)->status, core::CombatantStatus::Down);
    // Un autre allie peut partager la figurine de Bram et rester debout : on ne verifie que la
    // bande de mort, qui n'appartient qu'a lui.
    const hmi::ComposedScene aTerre = compose();
    const hmi::ComposedQuad* mort = withTexture(aTerre, death);
    ASSERT_NE(mort, nullptr);
    EXPECT_FLOAT_EQ(mort->sprite.u0, 5.0f * 96.0f / 576.0f);
    EXPECT_FLOAT_EQ(mort->sprite.u1, 1.0f);
    EXPECT_FLOAT_EQ(mort->sprite.width, repos->sprite.width * 2.0f);
    EXPECT_FLOAT_EQ(mort->sprite.height, repos->sprite.height);
    EXPECT_NEAR(mort->sprite.x + mort->sprite.width / 2.0f, centreDebout, 1e-3f);
}
