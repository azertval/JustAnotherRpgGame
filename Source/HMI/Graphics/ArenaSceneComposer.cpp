// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/ArenaSceneComposer.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <vector>

#include "Core/Combat/Arena.h"
#include "Core/Combat/IsoProjection.h"
#include "HMI/Graphics/ArenaAnimationState.h"
#include "HMI/Graphics/ArenaAppearanceCatalog.h"

namespace hmi {

namespace {

// Les textures de scene du lieu, a cote du dossier du kit d'arene (comme ../Npc). Le lieu vient
// du manifeste du kit (`scene`), jamais du code : une arene n'est pas forcement le Colisee.
constexpr std::string_view SCENE_ROOT = "../Scene/";

// Les sols : le sable, ses variantes semees la ou la brique QML posait une dalle claire, la pierre
// sous l'enceinte, le seuil sous une porte.
constexpr std::string_view SAND = "sand";
constexpr std::array<std::string_view, 3> SAND_VARIANTS{"sand-2", "sand-3", "sand-blood"};
constexpr std::string_view STONE = "stone-slab";
constexpr std::string_view THRESHOLD = "gate-threshold";

// Les pieces d'enceinte : un pan, une banniere, une torche et une arche existent pour chaque arete
// du fond (suffixe -left ou -right) ; l'angle et le pilier n'en ont pas.
constexpr std::string_view WALL = "wall";
constexpr std::string_view BANNER = "banner";
constexpr std::string_view TORCH = "torch";
constexpr std::string_view ARCH = "arch";
constexpr std::string_view CORNER = "wall-corner";
constexpr std::string_view PILLAR = "pillar";

// Marge basse d'une figurine, en hauteurs de losange (anchors.bottomMargin de ArenaTile.ui.qml).
constexpr float FIGURE_BOTTOM_MARGIN = 0.42F;

// Le chemin d'une piece de scene : `../Scene/<lieu du kit>/<nom><suffixe>.png`.
void scenePath(const ArenaAppearanceCatalog& catalog, std::string& path, std::string_view name,
               std::string_view suffix = {}) {
    path.assign(SCENE_ROOT);
    path.append(catalog.scene());
    path.push_back('/');
    path.append(name);
    path.append(suffix);
    path.append(".png");
}

// L'arete du fond contre laquelle se dresse une piece du bord de la grille.
//
// Le bord de la ligne 0 (et de la derniere ligne) court comme l'arete droite d'une case, du sommet
// haut au sommet droit ; celui de la colonne 0 (et de la derniere), comme l'arete gauche. Une piece
// du bord de devant se dresse contre l'arete du fond de sa case, parallele au bord : l'atelier ne
// dessine que des pieces du fond.
[[nodiscard]] std::string_view edgeSuffix(core::GridPosition cell, int rows) {
    return cell.row == 0 || cell.row == rows - 1 ? "-right" : "-left";
}

// Ce qui ne change pas d'une piece a l'autre : la projection, les textures et le tampon.
struct Composer {
    ComposedScene& scene;
    const core::IsoProjection& projection;
    const ArenaSceneTextures& textures;

    // Une piece de scene d'une case, posee par son ancre et a l'echelle de son lieu : le
    //        losange que son manifeste declare occupe celui de la case (`hmi::standingPieceQuad`).
    void addStanding(RenderLayer layer, std::string_view path, core::Vector2 topVertex,
                     std::int32_t sortOrder) const {
        const ArenaTexture& texture = textures.resolve(path);
        if (texture.texture == nullptr) {
            return;
        }
        scene.addSprite(layer, texture.texture, sortOrder,
                        standingPieceQuad(texture, topVertex, projection.tileWidth(),
                                          projection.tileHeight() / projection.tileWidth()));
    }
};

[[nodiscard]] core::Vector2 gridPoint(int column, int row) {
    return {static_cast<float>(column), static_cast<float>(row)};
}

void composeTile(const Composer& composer, const ArenaAppearanceCatalog& catalog,
                 const ArenaSceneSnapshot& snapshot, core::GridPosition cell, std::string& path) {
    const ArenaTileAppearance appearance =
        catalog.tileAppearance(cell, snapshot.columns, snapshot.rows, snapshot.isObstructed(cell));
    const core::Rect bounds = composer.projection.tileBounds(cell);
    const float tileHeight = bounds.size.y;
    const float bottomY = bounds.position.y + tileHeight;
    const core::Vector2 topVertex =
        composer.projection.gridToWorld(gridPoint(cell.column, cell.row));

    // --- Le sol : etire sur la boite du losange, comme l'Image en anchors.fill ---------------
    if (appearance.wall) {
        scenePath(catalog, path, STONE);
    } else if (appearance.gateSpot) {
        scenePath(catalog, path, THRESHOLD);
    } else if (appearance.slab) {
        scenePath(
            catalog, path,
            SAND_VARIANTS[static_cast<std::size_t>(appearance.slabVariant) % SAND_VARIANTS.size()]);
    } else {
        scenePath(catalog, path, SAND);
    }
    if (const ArenaTexture& floor = composer.textures.resolve(path); floor.texture != nullptr) {
        composer.scene.addSprite(RenderLayer::Tile, floor.texture, core::IsoProjection::depth(cell),
                                 floorQuad(bounds));
    }

    // --- L'enceinte : une piece debout par case, triee au pied de la case --------------------
    // Une piece de l'atelier porte son mur : une torche, une banniere, une arche sont un pan
    // decore, pas une decoration posee sur un pan.
    const std::string_view edge = edgeSuffix(cell, snapshot.rows);
    if (appearance.gateSpot) {
        scenePath(catalog, path, ARCH, edge);
    } else {
        switch (appearance.wallFeature) {
            case WallFeature::None:
                return;
            case WallFeature::Corner:
                // L'angle du fond ferme les deux murs ; les trois autres angles sont des piliers.
                scenePath(catalog, path, cell.column == 0 && cell.row == 0 ? CORNER : PILLAR);
                break;
            case WallFeature::Plain:
                scenePath(catalog, path, WALL, edge);
                break;
            case WallFeature::BannerSpot:
                scenePath(catalog, path, BANNER, edge);
                break;
            case WallFeature::TorchSpot:
                scenePath(catalog, path, TORCH, edge);
                break;
        }
    }
    composer.addStanding(RenderLayer::Object, path, topVertex,
                         arenaDepthSortOrder(bottomY, appearance.gateSpot ? ArenaDepthSlot::Gate
                                                                          : ArenaDepthSlot::Wall));
}

void composeFigure(const Composer& composer, const ArenaAppearanceCatalog& catalog,
                   const ArenaAnimationState& animation, const ArenaFigureSnapshot& combatant,
                   std::string& path) {
    const core::CombatSide side = combatant.side;
    const FigureAppearance figure = catalog.figureFor(combatant.name, side);
    if (figure.sheet.empty() || figure.frameCount <= 0) {
        return;
    }
    const bool down = combatant.down;
    const bool ally = side == core::CombatSide::Allies;

    path.assign(figure.directory);
    path.append(down && ally ? "/death.png" : "/idle.png");
    const ArenaTexture& texture = composer.textures.resolve(path);
    if (texture.texture == nullptr) {
        return;
    }

    // La bande dit sa propre decoupe : sa cellule (`.anim.json`) et, avec sa largeur totale, son
    // nombre d'images. Une bande sans description se decoupe au compte du manifeste.
    ArenaTexture band = texture;
    if (band.frameWidth <= 0 && figure.frameCount > 1 && band.width > 0) {
        band.frameWidth = std::max(1, band.width / figure.frameCount);
    }
    const int frameCount = frameCountOf(band);

    // A terre, la figurine s'arrete sur la derniere image ; debout, elle suit l'animation, ramenee
    // dans sa bande.
    const int frame =
        down ? frameCount - 1 : std::clamp(animation.frameOf(combatant.id), 0, frameCount - 1);

    // Une emprise de n cases : une figurine centree dessus, au pied de l'emprise. Elle n'est plus
    // agrandie n fois : une grande creature est livree a sa taille (384 x 384, EX-VIS-008).
    const core::GridPosition anchor = combatant.anchor;
    const int footprint = std::max(1, combatant.footprint);
    const auto extent = static_cast<float>(footprint);
    const core::Vector2 center =
        composer.projection.gridToWorld({static_cast<float>(anchor.column) + (extent / 2.0F),
                                         static_cast<float>(anchor.row) + (extent / 2.0F)});
    const float footY =
        composer.projection
            .gridToWorld(gridPoint(anchor.column + footprint, anchor.row + footprint))
            .y;
    const float tileHeight = composer.projection.tileHeight();

    SpriteQuad quad =
        figureQuad(band, frame, center.x, footY - (tileHeight * FIGURE_BOTTOM_MARGIN * extent),
                   composer.projection.tileWidth());
    if (down && !ally) {
        quad.a = ARENA_DOWN_ENEMY_ALPHA;
    }
    composer.scene.addSprite(RenderLayer::Player, texture.texture,
                             arenaDepthSortOrder(footY, ArenaDepthSlot::Figure), quad);
}

}  // namespace

std::int32_t arenaDepthSortOrder(float footWorldY, ArenaDepthSlot slot) noexcept {
    return (depthSortOrder(footWorldY) * ARENA_DEPTH_SLOTS) + static_cast<std::int32_t>(slot);
}

std::vector<std::string> arenaTexturePaths(const ArenaAppearanceCatalog& catalog) {
    std::vector<std::string> paths;
    std::string path;
    const auto add = [&](std::string_view name, std::string_view suffix = {}) {
        scenePath(catalog, path, name, suffix);
        paths.push_back(path);
    };
    add(SAND);
    for (const std::string_view variant : SAND_VARIANTS) {
        add(variant);
    }
    add(STONE);
    add(THRESHOLD);
    add(CORNER);
    add(PILLAR);
    for (const std::string_view piece : {WALL, BANNER, TORCH, ARCH}) {
        add(piece, "-left");
        add(piece, "-right");
    }
    // Un allie a terre montre sa bande de mort ; un ennemi, sa bande de repos estompee.
    for (const std::string& hero : catalog.heroes()) {
        const std::string directory = catalog.sheetDirectory(hero, core::CombatSide::Allies);
        paths.push_back(directory + "/idle.png");
        paths.push_back(directory + "/death.png");
    }
    for (const std::string& gladiator : catalog.gladiators()) {
        paths.push_back(catalog.sheetDirectory(gladiator, core::CombatSide::Enemies) + "/idle.png");
    }
    return paths;
}

bool ArenaSceneSnapshot::isObstructed(core::GridPosition cell) const noexcept {
    if (cell.column < 0 || cell.row < 0 || cell.column >= columns || cell.row >= rows) {
        return false;
    }
    const std::size_t index =
        (static_cast<std::size_t>(cell.row) * static_cast<std::size_t>(columns)) +
        static_cast<std::size_t>(cell.column);
    return index < obstructed.size() && obstructed[index];
}

ArenaSceneSnapshot snapshotArenaScene(const core::ArenaSession& session) {
    const core::CombatState& combat = session.combat();
    const core::BattleGrid& grid = combat.grid();

    ArenaSceneSnapshot snapshot;
    snapshot.columns = std::max(0, grid.width());
    snapshot.rows = std::max(0, grid.height());
    snapshot.obstructed.reserve(static_cast<std::size_t>(snapshot.columns) *
                                static_cast<std::size_t>(snapshot.rows));
    for (int row = 0; row < snapshot.rows; ++row) {
        for (int column = 0; column < snapshot.columns; ++column) {
            snapshot.obstructed.push_back(
                grid.isObstructed({.column = column, .row = row}, core::Locomotion::Walk));
        }
    }

    for (const core::CombatantId id : combat.combatants()) {
        const core::Combatant* const combatant = combat.find(id);
        // Sorti : il a quitte la grille et l'ordre, il n'a plus de figurine.
        if (combatant == nullptr || combatant->status == core::CombatantStatus::Withdrawn) {
            continue;
        }
        const std::optional<core::GridPosition> anchor = grid.positionOf(id);
        if (!anchor.has_value()) {
            continue;
        }
        snapshot.figures.push_back(
            ArenaFigureSnapshot{.id = id,
                                .name = combatant->profile.name,
                                .side = combatant->profile.side,
                                .down = combatant->status == core::CombatantStatus::Down,
                                .anchor = *anchor,
                                .footprint = std::max(1, grid.sideOf(id))});
    }
    return snapshot;
}

void composeArenaScene(ComposedScene& scene, const ArenaSceneSnapshot& snapshot,
                       const ArenaAppearanceCatalog& catalog, const ArenaAnimationState& animation,
                       const core::IsoProjection& projection, const ArenaSceneTextures& textures,
                       bool scenery) {
    const Composer composer{.scene = scene, .projection = projection, .textures = textures};
    // Un seul tampon de chemin pour toute la scene : apres la premiere image, plus d'allocation.
    std::string path;

    for (int row = 0; scenery && row < snapshot.rows; ++row) {
        for (int column = 0; column < snapshot.columns; ++column) {
            composeTile(composer, catalog, snapshot, {.column = column, .row = row}, path);
        }
    }
    for (const ArenaFigureSnapshot& figure : snapshot.figures) {
        composeFigure(composer, catalog, animation, figure, path);
    }
}

void composeArenaScene(ComposedScene& scene, const core::ArenaSession& session,
                       const ArenaAppearanceCatalog& catalog, const ArenaAnimationState& animation,
                       const core::IsoProjection& projection, const ArenaSceneTextures& textures) {
    composeArenaScene(scene, snapshotArenaScene(session), catalog, animation, projection, textures);
}

ComposedScene composeArenaScene(const core::ArenaSession& session,
                                const ArenaAppearanceCatalog& catalog,
                                const ArenaAnimationState& animation,
                                const core::IsoProjection& projection,
                                const ArenaSceneTextures& textures) {
    ComposedScene scene;
    composeArenaScene(scene, session, catalog, animation, projection, textures);
    scene.sort();
    return scene;
}

}  // namespace hmi
