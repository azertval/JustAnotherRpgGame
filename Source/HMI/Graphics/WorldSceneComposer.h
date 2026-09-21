// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/PieceFootprint.h"
#include "Core/Levels/TileType.h"
#include "Core/Math/Vector2.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/ScenePieces.h"

/**
 * @file HMI/Graphics/WorldSceneComposer.h
 * @brief Un **lieu qu'on parcourt** composé en primitives, sans GPU : le jumeau
 *        d'`hmi::ArenaSceneComposer` pour la carte du jeu (`LOT-09`).
 *
 * Le lieu et l'arène se dessinent par le **même** code : même projection isométrique
 * (`core::IsoProjection`), mêmes planches de l'atelier des textures (`LOT-92`, `ScenePieces.h`),
 * même tri par profondeur. Ce qui change est la **source** : l'arène lit une grille de combat, le
 * lieu lit une carte (`core::Level`) — ses couches, les pièces qu'elles nomment, ses entités.
 *
 * ## Ce qui va où
 *
 * | Pièce | Calque | Tri |
 * |---|---|---|
 * | sol (type de tuile de la couche « sol » → table du lieu) | `RenderLayer::Tile` | profondeur de
 * case | | relief (pièce nommée à la case, ou type de la couche « décor ») | `RenderLayer::Object`
 * | pied de la case | | figurine (héros, PNJ) | `RenderLayer::Player` | pied de sa case |
 *
 * ## Par instantané, comme l'arène
 *
 * La composition ne lit qu'un `hmi::WorldSceneSnapshot` — des **valeurs**, aucun pointeur vers la
 * carte ni vers la session. C'est ce qui lui permet de tourner sur le fil de rendu de Qt Quick,
 * l'instantané étant pris dans `synchronize()` pendant que le fil graphique est bloqué.
 */

namespace core {
class IsoProjection;
class Level;
class TileMap;
struct MapEntity;
struct TileLayer;
}  // namespace core

namespace hmi {

class PlaceAppearance;

/// @brief Rang d'une pièce dans une même profondeur : le relief, puis la figurine posée dessus.
enum class WorldDepthSlot : std::int32_t {
    Relief = 0,
    Figure,
};

/// Nombre de rangs par profondeur.
inline constexpr std::int32_t WORLD_DEPTH_SLOTS = 2;

/// Hauteur d'écran pour laquelle l'art est dessiné : l'agrandissement est 1 jusque-là, 2 au double
/// (`hmi::worldCamera`). Ici plutôt qu'avec le rendu GPU : l'essai de l'éditeur cadre comme le jeu.
inline constexpr int WORLD_ART_HEIGHT_PIXELS = 720;

/// Marge basse d'une figurine, en hauteurs de losange — la même que dans l'arène.
inline constexpr float WORLD_FIGURE_BOTTOM_MARGIN = 0.42F;

/// @brief Propriété de couche qui nomme le **lieu** dont la carte porte les planches.
inline constexpr std::string_view SCENE_PLACE_PROPERTY = "scene";

/// @brief Ordre de tri d'une pièce du lieu.
[[nodiscard]] std::int32_t worldDepthSortOrder(float footWorldY, WorldDepthSlot slot) noexcept;

/// @brief Une figurine à dessiner sur la carte : sa planche, son image, où elle est.
struct WorldFigureSnapshot {
    /// Figurine : un slug de l'atelier des PNJ (`anariel`, `jade`…, dossier `Assets/Npc/<slug>`),
    /// ou, s'il contient une barre, un dossier relatif à `Assets/` — une figurine de l'atelier des
    /// monstres (`Monsters/ironhand-soldier`, `LOT-93`).
    std::string figure;
    /// Bande d'animation : `idle`, `walk`.
    std::string clip = "idle";
    /// Position **continue**, en cases : `{1.5, 2.5}` est le centre de la case (1, 2).
    core::Vector2 point{};
    /// Image de la bande, ramenée dans la bande par la composition.
    int frame = 0;

    [[nodiscard]] bool operator==(const WorldFigureSnapshot&) const = default;
};

/**
 * @brief Le lieu **en valeurs** : ce que la composition lit, et rien d'autre.
 *
 * `floors` et `relief` portent une entrée par case, ligne par ligne : le **nom** de la pièce de la
 * planche du lieu, vide si la case ne dessine rien. `footprints` donne l'emprise des pièces de
 * relief plus grandes qu'une case : la composition les trie au pied de leur emprise.
 */
struct WorldSceneSnapshot {
    float diamondRatio = core::ARENA_DIAMOND_RATIO;
    int columns = 0;
    int rows = 0;
    /// Le lieu, qui nomme le dossier de planches : `Assets/Scene/<place>/`.
    std::string place;
    std::vector<std::string> floors;
    std::vector<std::string> relief;
    /// Le **type** de chaque case, une entrée par case, ligne par ligne : ce que le rendu de
    /// maquette dessine là où aucune pièce n'est nommée (`LOT-128`).
    std::vector<core::TileType> types;
    std::map<std::string, core::PieceFootprint, std::less<>> footprints;
    std::vector<WorldFigureSnapshot> figures;

    /// @return Le nom de la pièce de sol de @p cell, vide hors grille ou sans pièce.
    [[nodiscard]] std::string_view floorAt(core::GridPosition cell) const;
    /// @return Le nom de la pièce de relief de @p cell, vide hors grille ou sans relief.
    [[nodiscard]] std::string_view reliefAt(core::GridPosition cell) const;
    /// @return Le type de @p cell, `core::TileType::Empty` hors grille.
    [[nodiscard]] core::TileType typeAt(core::GridPosition cell) const;

    [[nodiscard]] bool operator==(const WorldSceneSnapshot&) const = default;
};

/**
 * @brief Ce que la composition lit d'une carte : sa grille racine, ses couches et ses entités.
 *
 * Le jeu compose une `core::Level` validée ; l'éditeur compose son brouillon (`core::LevelDraft`),
 * qui n'est pas toujours valide — une carte en cours de tracé l'est rarement. Les deux exposent les
 * mêmes accesseurs : `worldSceneSource` les prend chez l'un comme chez l'autre, et la composition
 * n'a qu'un chemin (`LOT-EDITOR-02`, acceptation : « la même liste de primitives que dans le jeu
 * »). Des références seulement : la source ne vit pas plus longtemps que la carte.
 */
struct WorldSceneSource {
    /// La grille racine : la collision, et le sol d'une carte sans couche visuelle.
    const core::TileMap& root;
    /// Les couches : la première de sol donne le sol, la première de décor le relief ; la pièce
    /// qu'une case nomme l'emporte sur la table du lieu.
    const std::vector<core::TileLayer>& layers;
    /// Les entités : leurs figurines, pour qui en compose (`npcFigures`).
    const std::vector<core::MapEntity>& entities;
};

/// @return La source de composition de @p map (`core::Level` ou `core::LevelDraft`).
template <class Map>
[[nodiscard]] WorldSceneSource worldSceneSource(const Map& map) {
    return WorldSceneSource{
        .root = map.tileMap(), .layers = map.layers(), .entities = map.entities()};
}

/// @return Le lieu que déclarent @p layers (propriété de couche `scene`), vide sinon.
[[nodiscard]] std::string scenePlaceOf(const std::vector<core::TileLayer>& layers);

/// @return Le lieu que déclare @p level (propriété de couche `scene`), vide s'il n'en déclare pas.
[[nodiscard]] std::string scenePlaceOf(const core::Level& level);

/**
 * @brief Les figurines des PNJ d'une carte, dans l'ordre des entités.
 *
 * Un PNJ sans propriété `figure` ne se dessine pas : il n'est pas encore dessiné. Le jeu y ajoute
 * le héros (`hmi::WorldPlay::figures`) ; l'éditeur les montre telles quelles.
 * @param entities Les entités de la carte.
 * @param frame    L'image des bandes (0 pour une image fixe).
 */
[[nodiscard]] std::vector<WorldFigureSnapshot> npcFigures(
    const std::vector<core::MapEntity>& entities, int frame);

/**
 * @brief Tire de @p level l'instantané que la composition dessine.
 *
 * Le sol vient de la couche **visuelle de sol** (`core::LayerKind::Ground`, à défaut la grille
 * racine), le relief de la couche **décor** : la pièce que la case nomme (sous son nom courant,
 * `hmi::PlaceAppearance::canonicalPiece`), à défaut celle que la table du lieu donne à son type.
 *
 * @param source     La carte, lue seulement.
 * @param appearance La table du lieu.
 * @param figures    Les figurines à poser, dans l'ordre où l'appelant les veut.
 */
[[nodiscard]] WorldSceneSnapshot snapshotWorldScene(const WorldSceneSource& source,
                                                    const PlaceAppearance& appearance,
                                                    std::vector<WorldFigureSnapshot> figures);

/// @brief Comme ci-dessus, pour une carte validée.
[[nodiscard]] WorldSceneSnapshot snapshotWorldScene(const core::Level& level,
                                                    const PlaceAppearance& appearance,
                                                    std::vector<WorldFigureSnapshot> figures);

/**
 * @brief La clé du marqueur d'une figurine qui n'a pas d'image (`LOT-39`, `LOT-96`).
 *
 * Une figurine nommée par une carte avant que son atelier (`LOT-91`, `LOT-93`) ne l'ait
 * produite se dessine par son **marqueur**, comme toute clé d'asset sans image
 * (`EX-CNT-041`) : on la voit, on lui parle, et on ne la prend pas pour une illustration.
 *
 * @param path Un chemin de bande de figurine, tel que la composition l'écrit
 * (`Npc/<slug>/idle.png`, `Monsters/<slug>/idle.png`).
 * @return `npc/<slug>` ou `monsters/<slug>`, ou une chaîne vide si @p path n'est pas celui d'une
 *         figurine.
 */
[[nodiscard]] std::string figureMarkerKey(std::string_view path);

/**
 * @brief Le chemin d'une bande de figurine, relatif au dossier des assets.
 *
 * @param figure Le nom que la carte donne (`WorldFigureSnapshot::figure`).
 * @param clip   La bande (`idle`, `walk`) ; vide : `idle`.
 * @return `Npc/<figure>/<clip>.png` pour un slug, `<figure>/<clip>.png` pour un dossier.
 */
[[nodiscard]] std::string figureStripPath(std::string_view figure, std::string_view clip);

/// @return Tous les chemins de texture que @p snapshot demandera, sans doublon, triés.
[[nodiscard]] std::vector<std::string> worldTexturePaths(const WorldSceneSnapshot& snapshot);

/**
 * @brief Compose le lieu dans un tampon réutilisé.
 *
 * Le tampon n'est **ni vidé ni trié** : même contrat qu'`hmi::composeArenaScene`, l'appelant
 * enchaîne `clear()`, les compositions, puis `sort()`.
 */
void composeWorldScene(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                       const core::IsoProjection& projection, const ScenePieceTextures& textures);

/// @brief Compose le lieu dans une scène neuve, **triée** — commodité des tests et des captures.
[[nodiscard]] ComposedScene composeWorldScene(const WorldSceneSnapshot& snapshot,
                                              const core::IsoProjection& projection,
                                              const ScenePieceTextures& textures);

}  // namespace hmi
