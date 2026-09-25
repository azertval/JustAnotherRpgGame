// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/WorldSceneComposer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <set>
#include <utility>
#include <variant>

#include "Core/Combat/Arena.h"
#include "Core/Combat/CombatTransition.h"
#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileMap.h"
#include "Core/Resources/ScenePlace.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/CityBlock.h"
#include "Core/World/CombatZone.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/EntityPresence.h"
#include "HMI/Graphics/MaquettePalette.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/StaticWorldScene.h"

namespace hmi {

namespace {

// Le rang d'une piece d'etage dans sa profondeur ; le sol (etage 0) est au rang du relief, et un
// etage au-dela du dernier se range avec lui.
[[nodiscard]] WorldDepthSlot storeyDepthSlot(int storey) noexcept {
    switch (std::min(storey, core::MAX_STOREY_FLOOR)) {
        case 1:
            return WorldDepthSlot::Storey;
        case 2:
            return WorldDepthSlot::Storey2;
        case 3:
            return WorldDepthSlot::Storey3;
        case 4:
            return WorldDepthSlot::Storey4;
        default:
            return WorldDepthSlot::Relief;
    }
}

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

// Le chemin de l'image d'une piece, relatif au dossier des assets : celui que le catalogue du lieu
// lui donne sous son niveau (LOT-124), a defaut `<nom>.png` dans le dossier propre du lieu.
[[nodiscard]] std::string piecePath(const WorldSceneSnapshot& snapshot, std::string_view piece) {
    if (const auto found = snapshot.pieceFiles.find(piece); found != snapshot.pieceFiles.end()) {
        return found->second;
    }
    return core::fallbackScenePiecePath(snapshot.place, piece);
}

// La texture d'une piece : par son fichier, sans copier le chemin quand le catalogue le connait --
// ce qui se fait une fois par case et par couche (audit de l'affichage).
[[nodiscard]] const SceneTexture& pieceTexture(const WorldSceneSnapshot& snapshot,
                                               const ScenePieceTextures& textures,
                                               std::string_view piece) {
    if (const auto found = snapshot.pieceFiles.find(piece); found != snapshot.pieceFiles.end()) {
        return textures.resolve(found->second);
    }
    return textures.resolve(core::fallbackScenePiecePath(snapshot.place, piece));
}

// Le dossier d'une figurine, sous le niveau qui la range (LOT-124), a defaut elle-meme.
[[nodiscard]] std::string figureDirectoryIn(const WorldSceneSnapshot& snapshot,
                                            std::string_view figure) {
    if (const auto found = snapshot.figureDirectories.find(figure);
        found != snapshot.figureDirectories.end()) {
        return found->second;
    }
    return std::string{figure};
}

// Rend : La premiere couche de role `kind` au rez, ou `nullptr` si la carte n'en declare pas.
[[nodiscard]] const core::TileLayer* layerOf(const WorldSceneSource& source, core::LayerKind kind) {
    for (const core::TileLayer& couche : source.layers) {
        if (couche.kind == kind && couche.floor == 0) {
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

// Les quatre sommets du losange d'une case, dans l'ordre du pourtour : haut, droite, bas, gauche.
struct DiamondVertices {
    std::array<float, 4> x{};
    std::array<float, 4> y{};
};

[[nodiscard]] DiamondVertices diamondOf(const core::Rect& bounds) {
    const float halfWidth = bounds.size.x / 2.0F;
    const float halfHeight = bounds.size.y / 2.0F;
    const float left = bounds.position.x;
    const float top = bounds.position.y;
    return DiamondVertices{.x = {left + halfWidth, left + bounds.size.x, left + halfWidth, left},
                           .y = {top, top + halfHeight, top + bounds.size.y, top + halfHeight}};
}

// Eclairement des trois faces d'un bloc : le dessus prend la lumiere, la face gauche moins, la
// droite le moins. Sans cet ecart, trois quads de la MEME teinte ne se distinguent pas et le bloc
// redevient une tache plate -- exactement ce que l'extrusion doit eviter.
constexpr float BLOCK_TOP_LIGHT = 1.0F;
constexpr float BLOCK_LEFT_LIGHT = 0.74F;
constexpr float BLOCK_RIGHT_LIGHT = 0.54F;
// Le socle d'un bloc etroit (colonne, arbre, caisses), au sol autour de lui.
constexpr float BLOCK_BASE_LIGHT = 0.45F;

[[nodiscard]] PolyQuad tintedQuad(const MaquetteColor& tint, float light) {
    PolyQuad quad;
    quad.r = tint.r * light;
    quad.g = tint.g * light;
    quad.b = tint.b * light;
    return quad;
}

// Le losange plat d'une case, a la teinte de son type : le sol de maquette.
void composeMaquetteDiamond(ComposedScene& scene, const core::IsoProjection& projection,
                            const ScenePieceTextures& textures, core::GridPosition cell,
                            core::TileType type, float light) {
    const DiamondVertices diamond = diamondOf(projection.tileBounds(cell));
    PolyQuad quad = tintedQuad(maquetteColor(type), light);
    quad.x = diamond.x;
    quad.y = diamond.y;
    scene.addPoly(RenderLayer::Tile, textures.solid.texture, core::IsoProjection::depth(cell),
                  quad);
}

// Le losange d'une case, reduit autour de son centre a la fraction `footprint`.
[[nodiscard]] DiamondVertices shrunk(const DiamondVertices& diamond, float footprint) {
    const float centreX = diamond.x[0];
    const float centreY = diamond.y[1];
    DiamondVertices result;
    for (std::size_t i = 0; i < 4; ++i) {
        result.x[i] = centreX + ((diamond.x[i] - centreX) * footprint);
        result.y[i] = centreY + ((diamond.y[i] - centreY) * footprint);
    }
    return result;
}

// Le BLOC d'une case de matiere pleine ou de mobilier : trois faces, de la hauteur et de l'emprise
// que son type lui donne (`maquetteShape`) -- un mur fait une case de haut (decision D6), une
// colonne deux sur une base etroite, une palissade moins d'une demi-case.
//
// Sur le calque du DECOR, et trie au pied de la case comme une piece de relief : c'est ce qui le
// fait masquer ce qui est derriere lui, figurines comprises. Un bloc pose sur le calque des tuiles
// passerait sous le heros quel que soit leur ordre, et le mur cesserait d'etre un mur.
// L'élévation d'un étage de maquette : la hauteur d'un bloc de mur, pour que les blocs s'empilent.
[[nodiscard]] float maquetteStoreyHeight(const core::IsoProjection& projection) {
    return projection.tileHeight() * maquetteShape(core::TileType::Wall).height;
}

// Un bloc de maquette ; sur un etage (`storey` > 0), eleve de `storey` hauteurs de bloc, trie au
// rang de l'etage, jamais avant `minimumFootY`. Sur un etage, il porte ce qu'il masque : c'est au
// moment de dessiner l'image qu'on l'efface devant le heros (LOT-129, `StaticWorldScene`).
// Rend : le pied retenu pour le tri.
float composeMaquetteBlock(ComposedScene& scene, const core::IsoProjection& projection,
                           const ScenePieceTextures& textures, core::GridPosition cell,
                           core::TileType type, int storey = 0,
                           float minimumFootY = -std::numeric_limits<float>::infinity()) {
    const core::Rect bounds = projection.tileBounds(cell);
    const MaquetteShape shape = maquetteShape(type);
    const float elevation = static_cast<float>(storey) * maquetteStoreyHeight(projection);
    DiamondVertices base = shrunk(diamondOf(bounds), shape.footprint);
    for (float& y : base.y) {
        y -= elevation;
    }
    const float height = bounds.size.y * shape.height;  // en hauteurs de losange
    MaquetteColor tint = maquetteColor(type);
    const float footY = std::max(
        minimumFootY,
        projection
            .gridToWorld(gridPoint(static_cast<float>(cell.column), static_cast<float>(cell.row)))
            .y);
    const WorldDepthSlot slot = storeyDepthSlot(storey);
    const std::int32_t order = worldDepthSortOrder(footY, slot);
    const auto raised = [height](float y) { return y - height; };
    const core::Rect occlusion =
        storey > 0 ? core::Rect{{bounds.position.x, bounds.position.y - elevation - height},
                                {bounds.size.x, bounds.size.y + height}}
                   : core::Rect{};

    // Face gauche : arete gauche -> bas, puis les deux memes sommets remontes.
    PolyQuad leftFace = tintedQuad(tint, BLOCK_LEFT_LIGHT);
    leftFace.x = {base.x[3], base.x[2], base.x[2], base.x[3]};
    leftFace.y = {base.y[3], base.y[2], raised(base.y[2]), raised(base.y[3])};
    scene.addPoly(RenderLayer::Object, textures.solid.texture, order, leftFace, storey, occlusion);

    // Face droite : bas -> arete droite.
    PolyQuad rightFace = tintedQuad(tint, BLOCK_RIGHT_LIGHT);
    rightFace.x = {base.x[2], base.x[1], base.x[1], base.x[2]};
    rightFace.y = {base.y[2], base.y[1], raised(base.y[1]), raised(base.y[2])};
    scene.addPoly(RenderLayer::Object, textures.solid.texture, order, rightFace, storey, occlusion);

    // Dessus : le losange de la case, remonte d'une hauteur.
    PolyQuad topFace = tintedQuad(tint, BLOCK_TOP_LIGHT);
    topFace.x = base.x;
    for (std::size_t i = 0; i < 4; ++i) {
        topFace.y[i] = raised(base.y[i]);
    }
    scene.addPoly(RenderLayer::Object, textures.solid.texture, order, topFace, storey, occlusion);
    return footY;
}

// Epaisseur d'un trace de maquette, en hauteurs de losange : assez fin pour ne pas couvrir le sol,
// assez epais pour se voir a l'echelle ou l'on lit une carte entiere.
constexpr float TRACE_THICKNESS = 0.09F;

// Un jeton : l'image engendree du disque a lettre, posee au centre de sa case.
//
// Sur le calque de l'INTERFACE EN SCENE, comme les traces, et non dans la bande de profondeur ou
// vit la figurine qu'il remplace (decision de realisation D7). Un jeton n'est pas un objet du
// monde : c'est une marque sur un plan, et une marque a demi cachee par le mur d'en face ne dit
// plus ou est le PNJ -- ce qui est precisement son seul travail.
void composeToken(ComposedScene& scene, const core::IsoProjection& projection,
                  const ScenePieceTextures& textures, const MaquetteTokenSnapshot& token) {
    // Sans repli sur le damier : un jeton est peint ou n'est pas la. C'est aussi ce qui fait qu'un
    // rendu qui ignore les jetons -- l'arriere-plan de combat de l'arene -- n'en herite pas.
    const SceneTexture* const texture = textures.find(maquetteTokenPath(token.kind, token.letter));
    if (texture == nullptr || texture->texture == nullptr) {
        return;
    }
    const core::Vector2 centre = projection.gridToWorld(gridPoint(
        static_cast<float>(token.cell.column) + 0.5F, static_cast<float>(token.cell.row) + 0.5F));
    const float side = MAQUETTE_TOKEN_TILE_FRACTION * projection.tileWidth();
    const float footY = projection
                            .gridToWorld(gridPoint(static_cast<float>(token.cell.column),
                                                   static_cast<float>(token.cell.row)))
                            .y;
    const std::int32_t order = worldDepthSortOrder(footY, WorldDepthSlot::Figure);

    SpriteQuad quad;
    quad.x = centre.x - (side / 2.0F);
    // Le disque repose sur le centre de la case, legerement releve : il se tient dessus, il n'y
    // flotte pas.
    quad.y = centre.y - side + (projection.tileHeight() * 0.18F);
    quad.width = side;
    quad.height = side;
    scene.addSprite(RenderLayer::UI, texture->texture, order, quad);

    if (!token.arrow) {
        return;
    }
    // La fleche d'un portail : un fut et une pointe, au-dessus du jeton. La sortie se voit d'un
    // coup d'oeil, avant meme qu'on lise la lettre.
    const MaquetteColor gold = maquetteTokenColor(MaquetteTokenKind::Portal);
    const float top = quad.y - (side * 0.12F);
    const float headHeight = side * 0.38F;
    const float headHalf = side * 0.30F;
    const float shaftHalf = side * 0.10F;

    PolyQuad shaft = tintedQuad(gold, 1.0F);
    shaft.x = {centre.x - shaftHalf, centre.x + shaftHalf, centre.x + shaftHalf,
               centre.x - shaftHalf};
    shaft.y = {top - headHeight, top - headHeight, top, top};
    scene.addPoly(RenderLayer::UI, textures.solid.texture, order, shaft);

    // La pointe : un quad dont deux sommets coincident, donc un triangle.
    PolyQuad head = tintedQuad(gold, 1.0F);
    head.x = {centre.x, centre.x + headHalf, centre.x, centre.x - headHalf};
    head.y = {top - headHeight - headHeight, top - headHeight, top - headHeight, top - headHeight};
    scene.addPoly(RenderLayer::UI, textures.solid.texture, order, head);
}

// Un trace : le contour du losange de chaque case d'une zone, ou la ligne brisee d'un trajet.
void composeTrace(ComposedScene& scene, const core::IsoProjection& projection,
                  const ScenePieceTextures& textures, const MaquetteTraceSnapshot& trace) {
    if (textures.solid.texture == nullptr || trace.cells.empty()) {
        return;
    }
    const float thickness = projection.tileHeight() * TRACE_THICKNESS;
    const auto segment = [&](core::Vector2 from, core::Vector2 to, std::int32_t order) {
        LineQuad line;
        line.ax = from.x;
        line.ay = from.y;
        line.bx = to.x;
        line.by = to.y;
        line.thickness = thickness;
        line.r = trace.color.r;
        line.g = trace.color.g;
        line.b = trace.color.b;
        scene.addLine(RenderLayer::UI, textures.solid.texture, order, line);
    };

    if (trace.shape == MaquetteTraceShape::Path) {
        for (std::size_t i = 1; i < trace.cells.size(); ++i) {
            const auto centreOf = [&projection](core::GridPosition cell) {
                return projection.gridToWorld(gridPoint(static_cast<float>(cell.column) + 0.5F,
                                                        static_cast<float>(cell.row) + 0.5F));
            };
            segment(centreOf(trace.cells[i - 1]), centreOf(trace.cells[i]),
                    core::IsoProjection::depth(trace.cells[i]));
        }
        return;
    }
    for (const core::GridPosition cell : trace.cells) {
        const DiamondVertices diamond = diamondOf(projection.tileBounds(cell));
        const std::int32_t order = core::IsoProjection::depth(cell);
        for (std::size_t i = 0; i < 4; ++i) {
            const std::size_t next = (i + 1) % 4;
            segment(core::Vector2{diamond.x[i], diamond.y[i]},
                    core::Vector2{diamond.x[next], diamond.y[next]}, order);
        }
    }
}

// Le rendu de maquette d'une case (LOT-128). C'est ce qui se dessine quand AUCUNE piece n'est
// nommee -- carte sans lieu, ou type que la table du lieu ne couvre pas : les deux manques sont le
// meme cas.
void composeMaquetteCell(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                         const core::IsoProjection& projection, const ScenePieceTextures& textures,
                         core::GridPosition cell, bool flatBlocks) {
    if (textures.solid.texture == nullptr) {
        return;  // sans aplat, rien a teinter : on ne dessine pas plutot que de dessiner faux.
    }
    const core::TileType type = snapshot.typeAt(cell);
    if (type == core::TileType::Empty) {
        return;  // une case vide n'est pas du sol : elle ne se dessine pas, comme avant.
    }
    if (maquetteExtrudes(type) && !flatBlocks) {
        if (maquetteShape(type).footprint < 1.0F) {
            // Un bloc qui n'occupe pas toute sa case laisserait un trou autour de lui : son socle,
            // plus sombre, dit la case sans le confondre avec un sol voisin.
            composeMaquetteDiamond(scene, projection, textures, cell, type, BLOCK_BASE_LIGHT);
        }
        composeMaquetteBlock(scene, projection, textures, cell, type);
        return;
    }
    composeMaquetteDiamond(scene, projection, textures, cell, type, BLOCK_TOP_LIGHT);
}

void composeFloor(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                  const core::IsoProjection& projection, const ScenePieceTextures& textures,
                  core::GridPosition cell, bool flatBlocks) {
    const std::string_view piece = snapshot.floorAt(cell);
    if (piece.empty()) {
        composeMaquetteCell(scene, snapshot, projection, textures, cell, flatBlocks);
        return;
    }
    const SceneTexture& texture = pieceTexture(snapshot, textures, piece);
    if (texture.texture == nullptr) {
        return;
    }
    // Le sol est etire sur la boite du losange, un peu elargie pour couvrir la couture
    // (`floorQuad`, LOT-103).
    scene.addSprite(RenderLayer::Tile, texture.texture, core::IsoProjection::depth(cell),
                    floorQuad(projection.tileBounds(cell)));
}

// Pose une piece de relief a sa case ; sur une couche d'etage (`storey` > 0), elevee de `storey`
// hauteurs d'etage, triee au-dessus du rez de sa case, et portant ce qu'elle masque (son image)
// pour s'effacer devant le heros. Son pied ne passe jamais avant `minimumFootY` : le pied le plus
// avance de ce qui la porte.
// Rend : le pied retenu pour le tri, rien si la piece n'a pas de texture.
std::optional<float> composeStandingPiece(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                                          const core::IsoProjection& projection,
                                          const ScenePieceTextures& textures,
                                          core::GridPosition cell, std::string_view piece,
                                          int storey, float minimumFootY) {
    const SceneTexture& texture = pieceTexture(snapshot, textures, piece);
    if (texture.texture == nullptr) {
        return std::nullopt;
    }
    // Posee par son ancre, le sommet haut du losange de sa case ; triee au pied de son EMPRISE
    // (`core::footprintFootCorner`), pour qu'une piece plus haute ou plus large que sa case reste
    // derriere ce qui se tient devant n'importe laquelle de ses cases.
    core::Vector2 topVertex = projection.gridToWorld(
        gridPoint(static_cast<float>(cell.column), static_cast<float>(cell.row)));
    const auto emprise = snapshot.footprints.find(piece);
    const core::GridPosition pied = core::footprintFootCorner(
        cell, emprise == snapshot.footprints.end() ? core::PieceFootprint{} : emprise->second);
    const float footY = std::max(
        minimumFootY, texture.depthOffset
                          ? topVertex.y + ((*texture.depthOffset * projection.tileHeight()) / 2.0F)
                          : projection
                                .gridToWorld(gridPoint(static_cast<float>(pied.column),
                                                       static_cast<float>(pied.row)))
                                .y);
    // Un etage s'eleve de la hauteur que le manifeste de son lieu declare, a l'echelle de l'art ;
    // son pied, lui, reste celui de sa case : il se trie avec elle (LOT-129).
    if (storey > 0) {
        const float storeyWorld =
            texture.storeyHeight
                ? *texture.storeyHeight * projection.tileWidth() / artTileWidth(texture)
                : DEFAULT_STOREY_TILES * projection.tileWidth();
        topVertex.y -= static_cast<float>(storey) * storeyWorld;
    }
    // A l'echelle de son lieu : le losange que son manifeste declare occupe celui de la case.
    SpriteQuad quad = standingPieceQuad(texture, topVertex, projection.tileWidth(),
                                        projection.tileHeight() / projection.tileWidth());
    const WorldDepthSlot slot = storeyDepthSlot(storey);
    const std::int32_t sortOrder = worldDepthSortOrder(footY, slot);
    scene.addSprite(RenderLayer::Object, texture.texture, sortOrder, quad, storey,
                    storey > 0 ? spriteQuadBounds(quad) : core::Rect{});
    return footY;
}

// Le pied le plus avance qui porte chaque case : celui de la piece dont l'emprise la couvre. Un
// etage s'y trie au plus tot : un mur de deux cases se trie au pied de sa seconde, et le toit de sa
// premiere doit passer apres lui, sans quoi le mur en recouvre l'egout (LOT-129).
void coverCells(std::vector<float>& cover, const WorldSceneSnapshot& snapshot,
                core::GridPosition cell, std::string_view piece, float footY) {
    const auto emprise = snapshot.footprints.find(piece);
    const core::PieceFootprint footprint =
        emprise == snapshot.footprints.end() ? core::PieceFootprint{} : emprise->second;
    for (const core::GridPosition covered : core::footprintCells(cell, footprint)) {
        if (inGrid(covered, snapshot.columns, snapshot.rows)) {
            float& foot = cover[indexOf(covered, snapshot.columns)];
            foot = std::max(foot, footY);
        }
    }
}

std::optional<float> composeRelief(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                                   const core::IsoProjection& projection,
                                   const ScenePieceTextures& textures, core::GridPosition cell,
                                   bool flatBlocks) {
    const std::string_view piece = snapshot.reliefAt(cell);
    if (piece.empty()) {
        // Un mur se peint aussi souvent sur la couche decor que sur le sol : il doit s'y extruder
        // pareillement, sans quoi une carte maquettee a la maniere des modeles livres serait vide
        // (LOT-128). Le mobilier (caisses, etals, buissons) s'y extrude de meme ; un type de decor
        // plat n'a, lui, pas de forme a prendre.
        const core::TileType type = snapshot.reliefTypeAt(cell);
        if (textures.solid.texture != nullptr && maquetteExtrudes(type)) {
            if (!flatBlocks) {
                return composeMaquetteBlock(scene, projection, textures, cell, type);
            }
            // Le plan de principe couche le bloc : son losange, a la teinte du type, comme un
            // bloc pose sur le sol (LOT-146 : les murs d'une carte neuve sont sur `relief`).
            composeMaquetteDiamond(scene, projection, textures, cell, type, BLOCK_TOP_LIGHT);
        }
        return std::nullopt;
    }
    return composeStandingPiece(scene, snapshot, projection, textures, cell, piece, 0,
                                -std::numeric_limits<float>::infinity());
}

// Une figurine posee : sa texture, son image, son rang de dessin.
struct FigurePlacement {
    TextureHandle texture = nullptr;
    SpriteQuad quad{};
    std::int32_t sortOrder = 0;
};

[[nodiscard]] std::optional<FigurePlacement> placeFigure(const WorldSceneSnapshot& snapshot,
                                                         const core::IsoProjection& projection,
                                                         const ScenePieceTextures& textures,
                                                         const WorldFigureSnapshot& figure) {
    if (figure.figure.empty()) {
        return std::nullopt;
    }
    const SceneTexture& texture = textures.resolve(
        figureStripPath(figureDirectoryIn(snapshot, figure.figure), figure.clip, figure.facing));
    if (texture.texture == nullptr) {
        return std::nullopt;
    }
    // La bande dit sa propre decoupe : sa cellule, et avec sa largeur totale son nombre d'images.
    // Une image hors bande est ramenee dedans plutot que de lire a cote de la texture. Sa cadence
    // aussi, quand elle la dit et que l'instantane porte le temps.
    const int frameCount = frameCountOf(texture);
    const int rang = figure.seconds >= 0.0F && texture.frameDuration > 0.0F
                         ? static_cast<int>(figure.seconds / texture.frameDuration)
                         : figure.frame;
    // Une bande jouee une fois (attaque, touche, mort) se fige sur sa derniere image : un mort ne
    // se releve pas parce que le temps passe (`LOT-118`). Une bande qui boucle est ramenee dedans.
    const int frame = texture.loop ? ((rang % frameCount) + frameCount) % frameCount
                                   : std::clamp(rang, 0, frameCount - 1);

    const core::Vector2 center = projection.gridToWorld(figure.point);
    const float footY =
        projection.gridToWorld(gridPoint(figure.point.x + 0.5F, figure.point.y + 0.5F)).y;
    // Une figurine qui declare sa ligne de sol la pose au CENTRE du losange de sa position : c'est
    // la que la maquette du LOT-101 met les pieds. Sans elle, l'ancienne marge : le bas de la
    // cellule un peu au-dessus de la pointe sud.
    const float bottomY =
        texture.groundLine
            ? center.y + ((static_cast<float>(frameHeightOf(texture)) - *texture.groundLine) *
                          projection.tileWidth() / artTileWidth(texture))
            : footY - (projection.tileHeight() * WORLD_FIGURE_BOTTOM_MARGIN);
    return FigurePlacement{
        .texture = texture.texture,
        .quad = figureQuad(texture, frame, center.x, bottomY, projection.tileWidth()),
        .sortOrder = worldDepthSortOrder(footY, WorldDepthSlot::Figure)};
}

void composeFigure(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                   const core::IsoProjection& projection, const ScenePieceTextures& textures,
                   const WorldFigureSnapshot& figure) {
    if (const std::optional<FigurePlacement> placed =
            placeFigure(snapshot, projection, textures, figure)) {
        scene.addSprite(RenderLayer::Player, placed->texture, placed->sortOrder, placed->quad);
    }
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

core::TileType WorldSceneSnapshot::reliefTypeAt(core::GridPosition cell) const {
    if (!inGrid(cell, columns, rows)) {
        return core::TileType::Empty;
    }
    const std::size_t index = indexOf(cell, columns);
    return index < reliefTypes.size() ? reliefTypes[index] : core::TileType::Empty;
}

std::string scenePlaceOf(const core::Level& level) {
    return scenePlaceOf(level.layers());
}

namespace {

// La valeur texte d'une propriete d'entite, vide si elle manque ou n'est pas un texte.
[[nodiscard]] std::string_view textProperty(const core::MapEntity& entity, std::string_view key) {
    const auto found = entity.properties.find(std::string{key});
    if (found == entity.properties.end()) {
        return {};
    }
    const std::string* value = std::get_if<std::string>(&found->second);
    return value != nullptr ? std::string_view{*value} : std::string_view{};
}

// Le nom d'ou le jeton tire sa lettre : ce que le canevas ecrit deja a cote de l'entite -- la carte
// cible d'un portail, le dialogue d'un PNJ, le nom d'un point d'arrivee --, a defaut son TYPE.
//
// Surtout pas son identifiant : ils s'ecrivent tous `e<numero>`, et tous les jetons porteraient un
// `E`. Le type, lui, distingue au moins un coffre (`C`) d'un PNJ (`N`).
[[nodiscard]] std::string_view tokenName(const core::MapEntity& entity) {
    const core::EntityKind* const kind = core::findEntityKind(entity.type);
    if (kind != nullptr && !kind->labelProperty.empty()) {
        const std::string_view label = textProperty(entity, kind->labelProperty);
        if (!label.empty()) {
            return label;
        }
    }
    return entity.type;
}

// La nature du jeton d'une entite ponctuelle, ou rien si elle n'en merite pas (decision D3).
//
// Rien n'est ajoute au format pour cette table : `core::MapEntity` n'a aucune notion d'hostilite.
// Le jaune est le PNJ de quete : celui qui porte un dialogue, ou dont la presence depend d'un
// drapeau (LOT-116) -- l'enfant de la demo ne parle pas, mais la quete le fait paraitre.
[[nodiscard]] std::optional<MaquetteTokenKind> tokenKindOf(const core::MapEntity& entity,
                                                           bool figureDrawn) {
    if (entity.type == core::NPC_ENTITY_TYPE) {
        // Un PNJ qui porte deja sa figurine se dessine par elle : pas de jeton par-dessus. Un PNJ
        // qu'une figurine occupe -- un mannequin (LOT-145) -- non plus.
        if (figureDrawn || !textProperty(entity, core::NPC_FIGURE_PROPERTY).empty()) {
            return std::nullopt;
        }
        const bool quete = !textProperty(entity, core::NPC_DIALOGUE_PROPERTY).empty() ||
                           !textProperty(entity, core::PRESENCE_FLAG_PROPERTY).empty();
        return quete ? MaquetteTokenKind::Talker : MaquetteTokenKind::Neutral;
    }
    if (entity.type == core::ENCOUNTER_ENTITY_TYPE) {
        return MaquetteTokenKind::Hostile;
    }
    if (entity.type == core::ARENA_ENTRY_ENTITY_TYPE) {
        return textProperty(entity, core::ARENA_SIDE_PROPERTY) == "enemies"
                   ? MaquetteTokenKind::Hostile
                   : MaquetteTokenKind::Player;
    }
    if (entity.type == core::SPAWN_POINT_ENTITY_TYPE) {
        return MaquetteTokenKind::Player;
    }
    if (entity.type == core::PORTAL_ENTITY_TYPE) {
        return MaquetteTokenKind::Portal;
    }
    if (entity.type == "chest" || entity.type == "sign") {
        return MaquetteTokenKind::Object;
    }
    return std::nullopt;
}

// Les cases d'un rectangle nomme par `width` et `height` depuis la case de l'entite.
[[nodiscard]] std::vector<core::GridPosition> rectangleCells(const core::MapEntity& entity) {
    const auto extent = [&entity](std::string_view key) {
        const auto found = entity.properties.find(std::string{key});
        if (found == entity.properties.end()) {
            return 1;
        }
        const std::int64_t* value = std::get_if<std::int64_t>(&found->second);
        return value != nullptr ? static_cast<int>(std::max<std::int64_t>(1, *value)) : 1;
    };
    const int width = extent(core::SHAPE_WIDTH_PROPERTY);
    const int height = extent(core::SHAPE_HEIGHT_PROPERTY);
    std::vector<core::GridPosition> cells;
    cells.reserve(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
    for (int row = 0; row < height; ++row) {
        for (int column = 0; column < width; ++column) {
            cells.push_back(core::GridPosition{.column = entity.position.column + column,
                                               .row = entity.position.row + row});
        }
    }
    return cells;
}

}  // namespace

MaquetteMarks maquetteMarks(const std::vector<core::MapEntity>& entities, bool maquette,
                            std::span<const WorldFigureSnapshot> figures) {
    MaquetteMarks marks;
    const auto figureAt = [&figures](core::GridPosition cell) {
        return std::ranges::any_of(figures, [cell](const WorldFigureSnapshot& figure) {
            return !figure.figure.empty() && !figure.hero &&
                   core::GridPosition{.column = static_cast<int>(std::floor(figure.point.x)),
                                      .row = static_cast<int>(std::floor(figure.point.y))} == cell;
        });
    };
    for (const core::MapEntity& entity : entities) {
        if (const std::optional<MaquetteTokenKind> kind =
                tokenKindOf(entity, figureAt(entity.position))) {
            marks.tokens.push_back(
                MaquetteTokenSnapshot{.kind = *kind,
                                      .letter = maquetteTokenLetter(tokenName(entity)),
                                      .cell = entity.position,
                                      .arrow = maquette && *kind == MaquetteTokenKind::Portal});
            continue;
        }
        if (!maquette) {
            continue;  // une carte finie ne montre pas ses declencheurs
        }
        if (entity.type == core::COMBAT_ZONE_ENTITY_TYPE) {
            marks.traces.push_back(MaquetteTraceSnapshot{.shape = MaquetteTraceShape::Outline,
                                                         .color = maquetteColorOf(0xb33a3a),
                                                         .cells = rectangleCells(entity)});
        } else if (entity.type == core::CITY_BLOCK_ENTITY_TYPE) {
            marks.traces.push_back(MaquetteTraceSnapshot{.shape = MaquetteTraceShape::Outline,
                                                         .color = maquetteColorOf(0xd2ac62),
                                                         .cells = rectangleCells(entity)});
        } else if (entity.type == core::ZONE_ENTITY_TYPE) {
            marks.traces.push_back(MaquetteTraceSnapshot{.shape = MaquetteTraceShape::Outline,
                                                         .color = maquetteColorOf(0xefe6d2),
                                                         .cells = core::zoneCells(entity)});
        } else if (entity.type == core::ROUTE_ENTITY_TYPE) {
            std::vector<core::GridPosition> points = entity.cells;
            if (points.empty() || points.front() != entity.position) {
                points.insert(points.begin(), entity.position);
            }
            marks.traces.push_back(MaquetteTraceSnapshot{.shape = MaquetteTraceShape::Path,
                                                         .color = maquetteColorOf(0xefe6d2),
                                                         .cells = std::move(points)});
        }
    }
    return marks;
}

std::string placeholderFigureDirectory(std::string_view silhouette) {
    return "Common/Characters/Placeholders/" +
           std::string{silhouette.empty() ? DEFAULT_SILHOUETTE : silhouette};
}

std::vector<WorldFigureSnapshot> npcFigures(const std::vector<core::MapEntity>& entities, int frame,
                                            bool placeholders) {
    std::vector<WorldFigureSnapshot> figures;
    for (const core::MapEntity& entity : entities) {
        if (entity.type != core::NPC_ENTITY_TYPE) {
            continue;
        }
        std::string figure{textProperty(entity, core::NPC_FIGURE_PROPERTY)};
        if (figure.empty()) {
            if (!placeholders) {
                continue;  // Un PNJ sans figurine ne se dessine pas : il n'est pas encore dessiné.
            }
            // Le mannequin de sa silhouette tient la place (LOT-145) : un PNJ se voit et s'anime
            // avant que l'atelier ne l'ait dessine.
            figure = placeholderFigureDirectory(textProperty(entity, SILHOUETTE_PROPERTY));
        }
        figures.push_back(
            WorldFigureSnapshot{.figure = std::move(figure),
                                .clip = std::string{figure_clips::IDLE},
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

namespace {

// Une couche d'etage a la taille de la carte, ses pieces canoniques et leurs emprises relevees
// dans le cliche (LOT-129).
[[nodiscard]] WorldStoreySnapshot snapshotStorey(const core::TileLayer& couche,
                                                 const PlaceAppearance& appearance,
                                                 WorldSceneSnapshot& snapshot) {
    const auto cases =
        static_cast<std::size_t>(snapshot.columns) * static_cast<std::size_t>(snapshot.rows);
    WorldStoreySnapshot storey{.floor = couche.floor, .relief = {}, .types = {}};
    storey.relief.assign(cases, std::string{});
    storey.types.assign(cases, core::TileType::Empty);
    for (int row = 0; row < snapshot.rows; ++row) {
        for (int column = 0; column < snapshot.columns; ++column) {
            const core::GridPosition cell{.column = column, .row = row};
            const std::size_t index = indexOf(cell, snapshot.columns);
            // Un etage ne se deduit pas du type : seule une piece nommee s'y pose ; sans elle, le
            // type s'extrude en maquette.
            storey.types[index] = couche.tiles.tile(column, row);
            const std::string_view piece = couche.pieceAt(column, row);
            if (piece.empty()) {
                continue;
            }
            storey.relief[index] = std::string{appearance.canonicalPiece(piece)};
            const core::PieceFootprint emprise = appearance.pieceFootprint(storey.relief[index]);
            if (emprise != core::PieceFootprint{}) {
                snapshot.footprints.insert_or_assign(storey.relief[index], emprise);
            }
        }
    }
    return storey;
}

}  // namespace

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
    snapshot.maximumRise = appearance.maximumRise();
    snapshot.columns = std::max(0, grilleSol.width());
    snapshot.rows = std::max(0, grilleSol.height());
    snapshot.place = scenePlaceOf(source.layers);
    if (snapshot.place.empty()) {
        snapshot.place = appearance.place();
    }
    snapshot.figures = std::move(figures);
    // Une carte qui ne nomme aucun lieu est une maquette : ses declencheurs s'y voient. Un PNJ
    // qu'une figurine occupe n'a pas de jeton.
    snapshot.marks = maquetteMarks(source.entities, snapshot.place.empty(), snapshot.figures);

    const auto cases =
        static_cast<std::size_t>(snapshot.columns) * static_cast<std::size_t>(snapshot.rows);
    snapshot.floors.assign(cases, std::string{});
    snapshot.relief.assign(cases, std::string{});
    snapshot.types.assign(cases, core::TileType::Empty);
    snapshot.reliefTypes.assign(cases, core::TileType::Empty);

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
                snapshot.reliefTypes[index] = decor->tiles.tile(column, row);
                snapshot.relief[index] = pieceAt(*decor, cell, appearance, false);
                const core::PieceFootprint emprise =
                    appearance.pieceFootprint(snapshot.relief[index]);
                if (emprise != core::PieceFootprint{}) {
                    snapshot.footprints.insert_or_assign(snapshot.relief[index], emprise);
                }
            }
        }
    }
    // Les entites qui posent une piece (LOT-126) -- une porte close : sa piece se pose a sa case
    // comme une piece de decor, par-dessus celle de la couche. L'appelant n'a donne que les
    // entites presentes. Sans piece dessinable -- une maquette, une piece absente du lieu --, un
    // decor qui arrete le pas s'extrude en mur sur son emprise : on voit que le passage est clos.
    for (const core::MapEntity& entity : source.entities) {
        const core::EntityKind* const kind = core::findEntityKind(entity.type);
        if (kind == nullptr || kind->pieceProperty.empty()) {
            continue;
        }
        const std::string piece{textProperty(entity, kind->pieceProperty)};
        const core::GridPosition cell = entity.position;
        if (cell.column < 0 || cell.row < 0 || cell.column >= snapshot.columns ||
            cell.row >= snapshot.rows) {
            continue;
        }
        if (!snapshot.place.empty() && !piece.empty() && !appearance.pieceFile(piece).empty()) {
            const std::size_t index = indexOf(cell, snapshot.columns);
            snapshot.relief[index] = std::string{appearance.canonicalPiece(piece)};
            const core::PieceFootprint emprise = appearance.pieceFootprint(snapshot.relief[index]);
            if (emprise != core::PieceFootprint{}) {
                snapshot.footprints.insert_or_assign(snapshot.relief[index], emprise);
            }
            continue;
        }
        const auto blocks = entity.properties.find(std::string{core::PROP_BLOCKS_PROPERTY});
        const bool* const blocking =
            blocks != entity.properties.end() ? std::get_if<bool>(&blocks->second) : nullptr;
        if (blocking != nullptr && !*blocking) {
            continue;
        }
        for (const core::GridPosition covered : rectangleCells(entity)) {
            if (covered.column < snapshot.columns && covered.row < snapshot.rows) {
                snapshot.reliefTypes[indexOf(covered, snapshot.columns)] = core::TileType::Wall;
            }
        }
    }
    // Les couches d'etage : les decors d'etage 1 a MAX_STOREY_FLOOR, du plus bas au plus haut, a
    // la taille de la carte (LOT-129). Une autre valeur est gardee par le format, et ignoree ici.
    for (const core::TileLayer& couche : source.layers) {
        if (couche.kind != core::LayerKind::Decor || couche.floor < 1 ||
            couche.floor > core::MAX_STOREY_FLOOR || couche.tiles.width() != snapshot.columns ||
            couche.tiles.height() != snapshot.rows) {
            continue;
        }
        snapshot.storeys.push_back(snapshotStorey(couche, appearance, snapshot));
    }
    std::ranges::stable_sort(snapshot.storeys, {}, &WorldStoreySnapshot::floor);
    // Le fichier de chaque piece citee, sous le niveau qui la declare (LOT-124).
    const auto recordFile = [&snapshot, &appearance](const std::string& piece) {
        if (piece.empty() || snapshot.pieceFiles.contains(piece)) {
            return;
        }
        if (std::string file = appearance.pieceFile(piece); !file.empty()) {
            snapshot.pieceFiles.emplace(piece, std::move(file));
        }
    };
    // Le dossier de chaque figurine posee, sous le niveau qui la range (LOT-124).
    for (const WorldFigureSnapshot& figure : snapshot.figures) {
        if (!figure.figure.empty() && !snapshot.figureDirectories.contains(figure.figure)) {
            snapshot.figureDirectories.emplace(figure.figure,
                                               appearance.figureDirectory(figure.figure));
        }
    }
    std::ranges::for_each(snapshot.floors, recordFile);
    std::ranges::for_each(snapshot.relief, recordFile);
    for (const WorldStoreySnapshot& storey : snapshot.storeys) {
        std::ranges::for_each(storey.relief, recordFile);
    }
    return snapshot;
}

std::string_view figureFacingSuffix(FigureFacing facing) noexcept {
    switch (facing) {
        case FigureFacing::SouthEast:
            return "se";
        case FigureFacing::SouthWest:
            return "sw";
        case FigureFacing::NorthEast:
            return "ne";
        case FigureFacing::NorthWest:
            return "nw";
        case FigureFacing::None:
            break;
    }
    return {};
}

FigureFacing figureFacingFor(core::Vector2 move, FigureFacing previous) noexcept {
    const float enX = std::abs(move.x);
    const float enY = std::abs(move.y);
    if (enX == 0.0F && enY == 0.0F) {
        return previous;
    }
    // Colonne : sud-est en avancant, nord-ouest en reculant. Ligne : sud-ouest, nord-est.
    const FigureFacing parX = move.x > 0.0F ? FigureFacing::SouthEast : FigureFacing::NorthWest;
    const FigureFacing parY = move.y > 0.0F ? FigureFacing::SouthWest : FigureFacing::NorthEast;
    // Une egalite a l'arrondi pres : une diagonale normalisee n'a pas deux composantes
    // rigoureusement egales apres une division par sa longueur.
    constexpr float EGALITE = 1.0e-4F;
    if (std::abs(enX - enY) <= EGALITE * std::max(enX, enY)) {
        if (previous == parX || previous == parY) {
            return previous;
        }
        return std::min(parX, parY);
    }
    return enX > enY ? parX : parY;
}

std::string figureStripPath(std::string_view figure, std::string_view clip, FigureFacing facing) {
    // Un dossier depuis la racine des assets ; un slug seul se cherche a plat.
    std::string path = core::figureDirectory({}, figure);
    path.push_back('/');
    path.append(clip.empty() ? std::string_view{"idle"} : clip);
    if (facing != FigureFacing::None) {
        path.push_back('-');
        path.append(figureFacingSuffix(facing));
    }
    path.append(".png");
    return path;
}

std::string figureMarkerKey(std::string_view path) {
    // L'arborescence 2D HD range les figurines dans un dossier `Characters/`, a n'importe quel
    // niveau (`Common/`, une region, une zone) : la cle est ce qui suit, sans le fichier.
    constexpr std::string_view PERSONNAGES = "Characters/";
    for (std::size_t debut = path.find(PERSONNAGES); debut != std::string_view::npos;
         debut = path.find(PERSONNAGES, debut + 1)) {
        if (debut != 0 && path[debut - 1] != '/') {
            continue;
        }
        const std::string_view reste = path.substr(debut + PERSONNAGES.size());
        const std::size_t barre = reste.rfind('/');
        if (barre == 0 || barre == std::string_view::npos || barre + 1 == reste.size() ||
            reste.find("//") != std::string_view::npos) {
            return {};
        }
        std::string cle = "characters/";
        for (const char lettre : reste.substr(0, barre)) {
            cle.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(lettre))));
        }
        return cle;
    }
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

std::vector<std::string> worldFigureTexturePaths(const WorldSceneSnapshot& snapshot,
                                                 std::span<const WorldFigureSnapshot> figures) {
    std::set<std::string> uniques;
    for (const WorldFigureSnapshot& figure : figures) {
        if (figure.figure.empty()) {
            continue;
        }
        // Les deux bandes d'une figurine : elle marche et elle attend, et le rendu ne doit pas
        // charger une texture au milieu d'une image. Un combattant precharge ses six bandes : un
        // coup ne doit pas non plus attendre sa texture (LOT-118).
        const std::string directory = figureDirectoryIn(snapshot, figure.figure);
        uniques.insert(figureStripPath(directory, figure_clips::IDLE, figure.facing));
        uniques.insert(figureStripPath(directory, figure_clips::WALK, figure.facing));
        if (figure.combatant) {
            for (const std::string_view clip : figure_clips::ALL) {
                uniques.insert(figureStripPath(directory, clip, figure.facing));
            }
        }
        // La bande en cours, quelle qu'elle soit : elle se dessine a cette image.
        uniques.insert(figureStripPath(directory, figure.clip, figure.facing));
    }
    return {uniques.begin(), uniques.end()};
}

std::vector<std::string> worldTexturePaths(const WorldSceneSnapshot& snapshot) {
    std::set<std::string> uniques;
    std::vector<const std::vector<std::string>*> couches{&snapshot.floors, &snapshot.relief};
    for (const WorldStoreySnapshot& storey : snapshot.storeys) {
        couches.push_back(&storey.relief);
    }
    for (const std::vector<std::string>* couche : couches) {
        for (const std::string& piece : *couche) {
            if (!piece.empty()) {
                uniques.insert(piecePath(snapshot, piece));
            }
        }
    }
    for (std::string& path : worldFigureTexturePaths(snapshot, snapshot.figures)) {
        uniques.insert(std::move(path));
    }
    // Les jetons s'adressent comme des planches : un chemin de plus, que le rendu peindra au lieu
    // de le charger (LOT-128, decision D2).
    for (const MaquetteTokenSnapshot& token : snapshot.marks.tokens) {
        uniques.insert(maquetteTokenPath(token.kind, token.letter));
    }
    return {uniques.begin(), uniques.end()};
}

std::optional<WorldHeroPlacement> placeWorldHero(const WorldSceneSnapshot& snapshot,
                                                 std::span<const WorldFigureSnapshot> figures,
                                                 const core::IsoProjection& projection,
                                                 const ScenePieceTextures& textures) {
    std::optional<WorldHeroPlacement> hero;
    for (const WorldFigureSnapshot& figure : figures) {
        if (!figure.hero) {
            continue;
        }
        if (const std::optional<FigurePlacement> placed =
                placeFigure(snapshot, projection, textures, figure)) {
            hero = WorldHeroPlacement{.bounds = spriteQuadBounds(placed->quad),
                                      .sortOrder = placed->sortOrder};
        }
    }
    return hero;
}

namespace {

// Ce que toutes les cases d'un etage partagent.
struct StoreyContext {
    ComposedScene& scene;
    const WorldSceneSnapshot& snapshot;
    const core::IsoProjection& projection;
    const ScenePieceTextures& textures;
    WorldComposeOptions options;
};

// Une case d'etage : sa piece nommee, ou a defaut son type extrude en maquette. Elle se trie au
// plus tot au pied de ce qui la porte (`cover`) et releve le sien pour l'etage suivant (`next`).
void composeStoreyCell(const StoreyContext& context, const WorldStoreySnapshot& storey,
                       core::GridPosition cell, const std::vector<float>& cover,
                       std::vector<float>& next) {
    const std::size_t index = indexOf(cell, context.snapshot.columns);
    if (index >= storey.relief.size()) {
        return;
    }
    if (storey.relief[index].empty()) {
        // Sans pièce nommée, un mur d'étage s'extrude en maquette, comme au rez.
        const core::TileType type =
            index < storey.types.size() ? storey.types[index] : core::TileType::Empty;
        if (context.textures.solid.texture != nullptr && maquetteExtrudes(type) &&
            !context.options.flatBlocks) {
            float& foot = next[index];
            foot = std::max(
                foot, composeMaquetteBlock(context.scene, context.projection, context.textures,
                                           cell, type, storey.floor, cover[index]));
        }
        return;
    }
    if (const std::optional<float> foot = composeStandingPiece(
            context.scene, context.snapshot, context.projection, context.textures, cell,
            storey.relief[index], storey.floor, cover[index])) {
        coverCells(next, context.snapshot, cell, storey.relief[index], *foot);
    }
}

}  // namespace

void composeWorldStatics(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                         const core::IsoProjection& projection, const ScenePieceTextures& textures,
                         WorldComposeOptions options) {
    std::vector<float> cover(
        static_cast<std::size_t>(snapshot.columns) * static_cast<std::size_t>(snapshot.rows),
        -std::numeric_limits<float>::infinity());
    for (int row = 0; row < snapshot.rows; ++row) {
        for (int column = 0; column < snapshot.columns; ++column) {
            const core::GridPosition cell{.column = column, .row = row};
            composeFloor(scene, snapshot, projection, textures, cell, options.flatBlocks);
            if (const std::optional<float> foot = composeRelief(
                    scene, snapshot, projection, textures, cell, options.flatBlocks)) {
                coverCells(cover, snapshot, cell, snapshot.reliefAt(cell), *foot);
            }
        }
    }
    // Les etages, du plus bas au plus haut : chacun se trie au plus tot au pied de ce qui le porte.
    for (const WorldStoreySnapshot& storey : snapshot.storeys) {
        std::vector<float> next = cover;
        const StoreyContext context{.scene = scene,
                                    .snapshot = snapshot,
                                    .projection = projection,
                                    .textures = textures,
                                    .options = options};
        for (int row = 0; row < snapshot.rows; ++row) {
            for (int column = 0; column < snapshot.columns; ++column) {
                composeStoreyCell(context, storey, core::GridPosition{.column = column, .row = row},
                                  cover, next);
            }
        }
        cover = std::move(next);
    }
    // Les jetons et les traces : des marques sur le plan, d'une autre bande que les figurines ; ils
    // ne dependent que de la carte.
    for (const MaquetteTokenSnapshot& token : snapshot.marks.tokens) {
        composeToken(scene, projection, textures, token);
    }
    for (const MaquetteTraceSnapshot& trace : snapshot.marks.traces) {
        composeTrace(scene, projection, textures, trace);
    }
}

void composeWorldFigures(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                         std::span<const WorldFigureSnapshot> figures,
                         const core::IsoProjection& projection,
                         const ScenePieceTextures& textures) {
    for (const WorldFigureSnapshot& figure : figures) {
        composeFigure(scene, snapshot, projection, textures, figure);
    }
}

void composeWorldScene(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                       const core::IsoProjection& projection, const ScenePieceTextures& textures,
                       WorldComposeOptions options) {
    // Les memes briques que l'image du jeu (`StaticWorldScene`) : la carte, puis les figurines, et
    // l'effacement des etages devant le heros -- une regle de rang et de recouvrement, qui ne
    // depend pas de l'ordre de la liste. Le tri reste a l'appelant, comme toujours.
    composeWorldStatics(scene, snapshot, projection, textures, options);
    composeWorldFigures(scene, snapshot, snapshot.figures, projection, textures);
    std::vector<ComposedQuad> quads;
    scene.swapQuads(quads, 0, 0);
    fadeStoreysOverHero(quads, placeWorldHero(snapshot, snapshot.figures, projection, textures));
    scene.swapQuads(quads, 0, 0);
}

ComposedScene composeWorldScene(const WorldSceneSnapshot& snapshot,
                                const core::IsoProjection& projection,
                                const ScenePieceTextures& textures, WorldComposeOptions options) {
    ComposedScene scene;
    composeWorldScene(scene, snapshot, projection, textures, options);
    scene.sort();
    return scene;
}

}  // namespace hmi
