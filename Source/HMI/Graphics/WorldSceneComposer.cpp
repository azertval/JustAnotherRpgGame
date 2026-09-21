// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/WorldSceneComposer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <set>
#include <utility>
#include <variant>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileMap.h"
#include "Core/Rpg/Dialogue.h"
#include "HMI/Graphics/MaquettePalette.h"
#include "HMI/Graphics/PlaceAppearance.h"

namespace hmi {

namespace {

// Les planches de l'atelier, telles que le rendu les adresse : relatif au dossier des assets.
constexpr std::string_view SCENE_ROOT = "Scene/";
constexpr std::string_view FIGURE_ROOT = "Npc/";
// Les dossiers de figurines qu'un marqueur peut remplacer : les PNJ (LOT-91), les monstres
// (LOT-93).
constexpr std::array<std::string_view, 2> FIGURE_DIRECTORIES = {"Npc/", "Monsters/"};

[[nodiscard]] std::size_t indexOf(core::GridPosition cell, int columns) {
    return (static_cast<std::size_t>(cell.row) * static_cast<std::size_t>(columns)) +
           static_cast<std::size_t>(cell.column);
}

[[nodiscard]] bool inGrid(core::GridPosition cell, int columns, int rows) {
    return cell.column >= 0 && cell.row >= 0 && cell.column < columns && cell.row < rows;
}

[[nodiscard]] std::string piecePath(std::string_view place, std::string_view piece) {
    std::string path{SCENE_ROOT};
    path.append(place);
    path.push_back('/');
    path.append(piece);
    path.append(".png");
    return path;
}

/// @return La premiere couche de role @p kind, ou `nullptr` si la carte n'en declare pas.
[[nodiscard]] const core::TileLayer* layerOf(const WorldSceneSource& source, core::LayerKind kind) {
    for (const core::TileLayer& couche : source.layers) {
        if (couche.kind == kind) {
            return &couche;
        }
    }
    return nullptr;
}

// La piece d'une case : celle que la couche nomme, sous son nom courant, a defaut celle que la
// table du lieu donne a son type.
[[nodiscard]] std::string pieceAt(const core::TileLayer& couche, core::GridPosition cell,
                                  const PlaceAppearance& appearance, bool floor) {
    const std::string_view nommee = couche.pieceAt(cell.column, cell.row);
    if (!nommee.empty()) {
        return std::string{appearance.canonicalPiece(nommee)};
    }
    const core::TileType type = couche.tiles.tile(cell.column, cell.row);
    return std::string{floor ? appearance.floorPiece(type, cell)
                             : appearance.reliefPiece(type, cell)};
}

[[nodiscard]] core::Vector2 gridPoint(float column, float row) {
    return {column, row};
}

// Le losange de maquette d'une case : les quatre sommets du losange de sa boite, a la teinte de son
// type (LOT-128). C'est ce qui se dessine quand AUCUNE piece n'est nommee -- carte sans lieu, ou
// type que la table du lieu ne couvre pas : les deux manques sont le meme cas.
void composeMaquetteFloor(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                          const core::IsoProjection& projection,
                          const ScenePieceTextures& textures, core::GridPosition cell) {
    if (textures.solid.texture == nullptr) {
        return;  // sans aplat, rien a teinter : on ne dessine pas plutot que de dessiner faux.
    }
    const core::TileType type = snapshot.typeAt(cell);
    if (type == core::TileType::Empty) {
        return;  // une case vide n'est pas du sol : elle ne se dessine pas, comme avant.
    }
    const core::Rect bounds = projection.tileBounds(cell);
    const float halfWidth = bounds.size.x / 2.0F;
    const float halfHeight = bounds.size.y / 2.0F;
    const float left = bounds.position.x;
    const float top = bounds.position.y;
    const MaquetteColor tint = maquetteColor(type);

    // Sommets dans l'ordre du pourtour : haut, droite, bas, gauche.
    PolyQuad quad;
    quad.x = {left + halfWidth, left + bounds.size.x, left + halfWidth, left};
    quad.y = {top, top + halfHeight, top + bounds.size.y, top + halfHeight};
    quad.r = tint.r;
    quad.g = tint.g;
    quad.b = tint.b;
    scene.addPoly(RenderLayer::Tile, textures.solid.texture, core::IsoProjection::depth(cell),
                  quad);
}

void composeFloor(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                  const core::IsoProjection& projection, const ScenePieceTextures& textures,
                  core::GridPosition cell) {
    const std::string_view piece = snapshot.floorAt(cell);
    if (piece.empty()) {
        composeMaquetteFloor(scene, snapshot, projection, textures, cell);
        return;
    }
    const SceneTexture& texture = textures.resolve(piecePath(snapshot.place, piece));
    if (texture.texture == nullptr) {
        return;
    }
    // Le sol est etire sur la boite du losange : c'est la pièce du lieu, dessinee a sa taille.
    const core::Rect bounds = projection.tileBounds(cell);
    SpriteQuad quad;
    quad.x = bounds.position.x;
    quad.y = bounds.position.y;
    quad.width = bounds.size.x;
    quad.height = bounds.size.y;
    scene.addSprite(RenderLayer::Tile, texture.texture, core::IsoProjection::depth(cell), quad);
}

void composeRelief(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                   const core::IsoProjection& projection, const ScenePieceTextures& textures,
                   core::GridPosition cell, float unitsPerScenePixel) {
    const std::string_view piece = snapshot.reliefAt(cell);
    if (piece.empty()) {
        return;
    }
    const SceneTexture& texture = textures.resolve(piecePath(snapshot.place, piece));
    if (texture.texture == nullptr) {
        return;
    }
    // Posee par son ancre, le sommet haut du losange de sa case ; triee au pied de son EMPRISE
    // (`core::footprintFootCorner`), pour qu'une piece plus haute ou plus large que sa case reste
    // derriere ce qui se tient devant n'importe laquelle de ses cases.
    const core::Vector2 topVertex = projection.gridToWorld(
        gridPoint(static_cast<float>(cell.column), static_cast<float>(cell.row)));
    const auto emprise = snapshot.footprints.find(piece);
    const core::GridPosition pied = core::footprintFootCorner(
        cell, emprise == snapshot.footprints.end() ? core::PieceFootprint{} : emprise->second);
    const float footY =
        texture.depthOffset
            ? topVertex.y + ((*texture.depthOffset * projection.tileHeight()) / 2.0F)
            : projection
                  .gridToWorld(
                      gridPoint(static_cast<float>(pied.column), static_cast<float>(pied.row)))
                  .y;
    const SpriteQuad quad = standingPieceQuad(texture, topVertex, unitsPerScenePixel);
    scene.addSprite(RenderLayer::Object, texture.texture,
                    worldDepthSortOrder(footY, WorldDepthSlot::Relief), quad);
}

void composeFigure(ComposedScene& scene, const core::IsoProjection& projection,
                   const ScenePieceTextures& textures, const WorldFigureSnapshot& figure,
                   float unitsPerPixel) {
    if (figure.figure.empty()) {
        return;
    }
    const SceneTexture& texture = textures.resolve(figureStripPath(figure.figure, figure.clip));
    if (texture.texture == nullptr) {
        return;
    }
    // La bande dit sa propre decoupe : sa largeur d'image, et avec sa largeur totale son nombre
    // d'images. Une image hors bande est ramenee dedans plutot que de lire a cote de la texture.
    const int frameWidthPixels =
        texture.frameWidth > 0 ? texture.frameWidth : FIGURE_FRAME_WIDTH_PIXELS;
    const int frameCount = texture.width > 0 ? std::max(1, texture.width / frameWidthPixels) : 1;
    const int frame = ((figure.frame % frameCount) + frameCount) % frameCount;

    const core::Vector2 center = projection.gridToWorld(figure.point);
    const float footY =
        projection.gridToWorld(gridPoint(figure.point.x + 0.5F, figure.point.y + 0.5F)).y;
    const float scale = unitsPerPixel * FIGURE_SCALE;

    SpriteQuad quad;
    quad.width = static_cast<float>(frameWidthPixels) * scale;
    quad.height = static_cast<float>(FIGURE_FRAME_HEIGHT_PIXELS) * scale;
    quad.x = center.x - (quad.width / 2.0F);
    quad.y = footY - (projection.tileHeight() * WORLD_FIGURE_BOTTOM_MARGIN) - quad.height;
    if (texture.width > 0 && texture.height > 0) {
        const auto frameWidth = static_cast<float>(frameWidthPixels);
        quad.u0 = static_cast<float>(frame) * frameWidth / static_cast<float>(texture.width);
        quad.u1 = static_cast<float>(frame + 1) * frameWidth / static_cast<float>(texture.width);
        quad.v0 = 0.0F;
        quad.v1 = std::min(1.0F, static_cast<float>(FIGURE_FRAME_HEIGHT_PIXELS) /
                                     static_cast<float>(texture.height));
    }
    scene.addSprite(RenderLayer::Player, texture.texture,
                    worldDepthSortOrder(footY, WorldDepthSlot::Figure), quad);
}

}  // namespace

std::int32_t worldDepthSortOrder(float footWorldY, WorldDepthSlot slot) noexcept {
    return (depthSortOrder(footWorldY) * WORLD_DEPTH_SLOTS) + static_cast<std::int32_t>(slot);
}

std::string_view WorldSceneSnapshot::floorAt(core::GridPosition cell) const {
    if (!inGrid(cell, columns, rows)) {
        return {};
    }
    const std::size_t index = indexOf(cell, columns);
    return index < floors.size() ? std::string_view{floors[index]} : std::string_view{};
}

std::string_view WorldSceneSnapshot::reliefAt(core::GridPosition cell) const {
    if (!inGrid(cell, columns, rows)) {
        return {};
    }
    const std::size_t index = indexOf(cell, columns);
    return index < relief.size() ? std::string_view{relief[index]} : std::string_view{};
}

core::TileType WorldSceneSnapshot::typeAt(core::GridPosition cell) const {
    if (!inGrid(cell, columns, rows)) {
        return core::TileType::Empty;
    }
    const std::size_t index = indexOf(cell, columns);
    return index < types.size() ? types[index] : core::TileType::Empty;
}

std::string scenePlaceOf(const core::Level& level) {
    return scenePlaceOf(level.layers());
}

std::vector<WorldFigureSnapshot> npcFigures(const std::vector<core::MapEntity>& entities,
                                            int frame) {
    std::vector<WorldFigureSnapshot> figures;
    for (const core::MapEntity& entity : entities) {
        if (entity.type != core::NPC_ENTITY_TYPE) {
            continue;
        }
        const auto found = entity.properties.find(std::string{core::NPC_FIGURE_PROPERTY});
        const std::string* figure =
            found != entity.properties.end() ? std::get_if<std::string>(&found->second) : nullptr;
        if (figure == nullptr || figure->empty()) {
            continue;  // Un PNJ sans figurine ne se dessine pas : il n'est pas encore dessiné.
        }
        figures.push_back(
            WorldFigureSnapshot{.figure = *figure,
                                .clip = "idle",
                                .point = {static_cast<float>(entity.position.column) + 0.5F,
                                          static_cast<float>(entity.position.row) + 0.5F},
                                .frame = frame});
    }
    return figures;
}

std::string scenePlaceOf(const std::vector<core::TileLayer>& layers) {
    for (const core::TileLayer& couche : layers) {
        const auto trouvee = couche.properties.find(std::string{SCENE_PLACE_PROPERTY});
        if (trouvee == couche.properties.end()) {
            continue;
        }
        if (const std::string* nom = std::get_if<std::string>(&trouvee->second);
            nom != nullptr && !nom->empty()) {
            return *nom;
        }
    }
    return {};
}

WorldSceneSnapshot snapshotWorldScene(const core::Level& level, const PlaceAppearance& appearance,
                                      std::vector<WorldFigureSnapshot> figures) {
    return snapshotWorldScene(worldSceneSource(level), appearance, std::move(figures));
}

WorldSceneSnapshot snapshotWorldScene(const WorldSceneSource& source,
                                      const PlaceAppearance& appearance,
                                      std::vector<WorldFigureSnapshot> figures) {
    // Le sol se lit sur la premiere couche de sol, a defaut sur la grille racine (une carte sans
    // couche visuelle reste jouable -- `EX-NFR-040`).
    const core::TileLayer* sol = layerOf(source, core::LayerKind::Ground);
    const core::TileLayer* decor = layerOf(source, core::LayerKind::Decor);
    const core::TileMap& grilleSol = sol != nullptr ? sol->tiles : source.root;

    WorldSceneSnapshot snapshot;
    snapshot.diamondRatio = appearance.diamondRatio();
    snapshot.columns = std::max(0, grilleSol.width());
    snapshot.rows = std::max(0, grilleSol.height());
    snapshot.place = scenePlaceOf(source.layers);
    if (snapshot.place.empty()) {
        snapshot.place = appearance.place();
    }
    snapshot.figures = std::move(figures);

    const auto cases =
        static_cast<std::size_t>(snapshot.columns) * static_cast<std::size_t>(snapshot.rows);
    snapshot.floors.assign(cases, std::string{});
    snapshot.relief.assign(cases, std::string{});
    snapshot.types.assign(cases, core::TileType::Empty);

    for (int row = 0; row < snapshot.rows; ++row) {
        for (int column = 0; column < snapshot.columns; ++column) {
            const core::GridPosition cell{.column = column, .row = row};
            const std::size_t index = indexOf(cell, snapshot.columns);
            // Le type de la case vient de la MEME grille que son sol : ce qui se dessine en
            // maquette est ce que la couche de sol dit, jamais une autre.
            snapshot.types[index] = grilleSol.tile(column, row);
            snapshot.floors[index] =
                sol != nullptr
                    ? pieceAt(*sol, cell, appearance, true)
                    : std::string{appearance.floorPiece(grilleSol.tile(column, row), cell)};
            if (decor != nullptr && decor->tiles.inBounds(column, row)) {
                snapshot.relief[index] = pieceAt(*decor, cell, appearance, false);
                const core::PieceFootprint emprise =
                    appearance.pieceFootprint(snapshot.relief[index]);
                if (emprise != core::PieceFootprint{}) {
                    snapshot.footprints.insert_or_assign(snapshot.relief[index], emprise);
                }
            }
        }
    }
    return snapshot;
}

std::string figureStripPath(std::string_view figure, std::string_view clip) {
    // Un nom sans barre est un PNJ de l'atelier ; avec, un dossier depuis la racine des assets.
    std::string path =
        figure.find('/') == std::string_view::npos ? std::string{FIGURE_ROOT} : std::string{};
    path.append(figure);
    path.push_back('/');
    path.append(clip.empty() ? std::string_view{"idle"} : clip);
    path.append(".png");
    return path;
}

std::string figureMarkerKey(std::string_view path) {
    for (const std::string_view dossier : FIGURE_DIRECTORIES) {
        if (!path.starts_with(dossier)) {
            continue;
        }
        const std::string_view reste = path.substr(dossier.size());
        const std::size_t barre = reste.find('/');
        if (barre == 0 || barre == std::string_view::npos) {
            return {};
        }
        std::string cle;
        for (const char lettre : dossier.substr(0, dossier.size() - 1)) {
            cle.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(lettre))));
        }
        cle.push_back('/');
        cle.append(reste.substr(0, barre));
        return cle;
    }
    return {};
}

std::vector<std::string> worldTexturePaths(const WorldSceneSnapshot& snapshot) {
    std::set<std::string> uniques;
    for (const std::vector<std::string>* couche : {&snapshot.floors, &snapshot.relief}) {
        for (const std::string& piece : *couche) {
            if (!piece.empty()) {
                uniques.insert(piecePath(snapshot.place, piece));
            }
        }
    }
    for (const WorldFigureSnapshot& figure : snapshot.figures) {
        if (figure.figure.empty()) {
            continue;
        }
        // Les deux bandes d'une figurine : elle marche et elle attend, et le rendu ne doit pas
        // charger une texture au milieu d'une image.
        uniques.insert(figureStripPath(figure.figure, "idle"));
        uniques.insert(figureStripPath(figure.figure, "walk"));
    }
    return {uniques.begin(), uniques.end()};
}

void composeWorldScene(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                       const core::IsoProjection& projection, const ScenePieceTextures& textures) {
    const float unitsPerPixel = projection.tileWidth() / core::ARENA_SHEET_TILE_WIDTH_PIXELS;
    const float unitsPerScenePixel =
        projection.tileWidth() / static_cast<float>(SCENE_TILE_WIDTH_PIXELS);

    for (int row = 0; row < snapshot.rows; ++row) {
        for (int column = 0; column < snapshot.columns; ++column) {
            const core::GridPosition cell{.column = column, .row = row};
            composeFloor(scene, snapshot, projection, textures, cell);
            composeRelief(scene, snapshot, projection, textures, cell, unitsPerScenePixel);
        }
    }
    for (const WorldFigureSnapshot& figure : snapshot.figures) {
        composeFigure(scene, projection, textures, figure, unitsPerPixel);
    }
}

ComposedScene composeWorldScene(const WorldSceneSnapshot& snapshot,
                                const core::IsoProjection& projection,
                                const ScenePieceTextures& textures) {
    ComposedScene scene;
    composeWorldScene(scene, snapshot, projection, textures);
    scene.sort();
    return scene;
}

}  // namespace hmi
