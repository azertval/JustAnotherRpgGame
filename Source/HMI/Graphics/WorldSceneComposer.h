// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <numbers>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/PieceFootprint.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileType.h"
#include "Core/Math/Vector2.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/MaquettePalette.h"
#include "HMI/Graphics/MaquetteTokens.h"
#include "HMI/Graphics/ScenePieces.h"

/**
 * @file HMI/Graphics/WorldSceneComposer.h
 * @brief Un **lieu qu'on parcourt** composé en primitives, sans GPU (`LOT-09`) ; depuis le
 *        `LOT-118`, la scène du combat aussi.
 *
 * Le lieu et le combat se dessinent par le **même** code : même projection isométrique
 * (`core::IsoProjection`), mêmes planches de l'atelier des textures (`LOT-92`, `ScenePieces.h`),
 * même tri par profondeur. La **source** est une carte (`core::Level`) — ses couches, les pièces
 * qu'elles nomment, ses entités — et, en combat, les combattants posés dessus (`snapshot.figures`).
 *
 * ## Ce qui va où
 *
 * | Pièce | Calque | Tri |
 * |---|---|---|
 * | sol (type de tuile de la couche « sol » → table du lieu) | `RenderLayer::Tile` | profondeur de
 * case | | relief (pièce nommée à la case, ou type de la couche « décor ») | `RenderLayer::Object`
 * | pied de la case | | figurine (héros, PNJ) | `RenderLayer::Player` | pied de sa case |
 *
 * ## Les pièces en maillage (`LOT-1003`)
 *
 * Une pièce dont le manifeste cite un maillage (`"mesh"`) ne produit pas de primitive : elle
 * s'ajoute à la liste des **maillages placés** de la scène (`ComposedScene::meshes`), posée par
 * `hmi::IsoView` au centre de son emprise, sans rang de tri — le tampon de profondeur la départage.
 * Les images gardent leur liste et leur ordre, et portent en plus la ligne où elles se dressent
 * (`ComposedQuad::footY`), lue seulement quand l'image a des maillages à croiser.
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

/**
 * @brief Rang d'une pièce dans une même profondeur : le relief, la figurine posée dessus, puis les
 *        étages de la case, du premier au dernier (`LOT-129`) — le rang de l'étage `n` est
 *        `Storey + n - 1`.
 */
enum class WorldDepthSlot : std::int32_t {
    Relief = 0,
    Figure,
    Storey,
    Storey2,
    Storey3,
    Storey4,
};
static_assert(static_cast<std::int32_t>(WorldDepthSlot::Storey4) ==
                  static_cast<std::int32_t>(WorldDepthSlot::Storey) + core::MAX_STOREY_FLOOR - 1,
              "WorldDepthSlot nomme un rang par etage");

/// Nombre de rangs par profondeur : le relief, la figurine, et un rang par étage.
inline constexpr std::int32_t WORLD_DEPTH_SLOTS =
    static_cast<std::int32_t>(WorldDepthSlot::Storey) + core::MAX_STOREY_FLOOR;

/// Opacité d'une pièce d'étage qui masque le héros (`LOT-129`) : on le voit à travers le toit.
inline constexpr float STOREY_SEE_THROUGH_OPACITY = 0.35F;

/// Hauteur d'étage d'un lieu dont le manifeste n'en déclare pas, en largeurs de case (`LOT-129`).
inline constexpr float DEFAULT_STOREY_TILES = 1.0F;

/// Hauteur de la vue, en largeurs de case (`EX-REN-013`) : une case occupe à l'écran la hauteur de
/// la fenêtre divisée par 10,8 — 100 px à 1080p, 200 px à 2160p. Toutes les définitions cadrent
/// donc la même étendue de monde : un écran plus fin montre le même jeu, plus finement.
inline constexpr float WORLD_VIEW_HEIGHT_IN_TILES = 10.8F;

/**
 * @brief La largeur d'une case à l'écran, en pixels, pour une vue de @p pixelHeight de haut.
 *
 * Ici plutôt qu'avec le rendu GPU : l'essai de l'éditeur cadre comme le jeu.
 */
[[nodiscard]] inline float worldTilePixels(int pixelHeight) noexcept {
    return static_cast<float>(pixelHeight > 0 ? pixelHeight : 1) / WORLD_VIEW_HEIGHT_IN_TILES;
}

/// Marge basse d'une figurine, en hauteurs de losange — la même que dans l'arène.
inline constexpr float WORLD_FIGURE_BOTTOM_MARGIN = 0.42F;

/// @brief Propriété de couche qui nomme le **lieu** dont la carte porte les planches.
inline constexpr std::string_view SCENE_PLACE_PROPERTY = "scene";

/// @brief Ordre de tri d'une pièce du lieu.
[[nodiscard]] std::int32_t worldDepthSortOrder(float footWorldY, WorldDepthSlot slot) noexcept;

/**
 * @brief Le **cap** d'une figurine (`LOT-1005`) : l'angle, en radians, de la direction où elle
 *        regarde dans le plan de la grille — 0 vers les colonnes croissantes (sud-est à l'écran),
 *        π/2 vers les lignes croissantes (sud-ouest). Un modèle s'oriente librement : il n'a pas
 *        de table de quatre orientations.
 */

/// @brief Le cap d'une figurine qui fait **face à la caméra** : le bas de l'écran.
inline constexpr float FIGURE_HEADING_FRONT = std::numbers::pi_v<float> / 4.0F;

/// @return Le cap d'une figurine qui se déplace de @p move, en cases ; @p previous si le
///         déplacement est nul.
[[nodiscard]] float figureHeadingFor(core::Vector2 move, float previous) noexcept;

/**
 * @brief Les clips d'une figurine : ce que le squelette d'un personnage sait jouer (standard des
 *        personnages, §7), et le tir du `LOT-136`.
 *
 * Le repos et la marche **bouclent** ; les autres se jouent une fois et se figent sur leur fin —
 * c'est le squelette qui le déclare (`core::SkeletonClip::loop`).
 */
namespace figure_clips {
/// Le repos, en boucle.
inline constexpr std::string_view IDLE = "idle";
/// La marche, en boucle.
inline constexpr std::string_view WALK = "walk";
/// L'attaque au contact, jouée une fois.
inline constexpr std::string_view ATTACK = "attack";
/// L'incantation, jouée une fois.
inline constexpr std::string_view CAST = "cast";
/// Le tir, joué une fois (`LOT-136`) ; une figurine qui n'en a pas joue son `attack`.
inline constexpr std::string_view RANGED = "ranged";
/// Le coup encaissé, joué une fois.
inline constexpr std::string_view HIT = "hit";
/// La chute, jouée une fois ; la dernière image reste.
inline constexpr std::string_view DEATH = "death";
/// Toutes, dans l'ordre du standard.
inline constexpr std::array<std::string_view, 7> ALL = {IDLE, WALK, ATTACK, RANGED,
                                                        CAST, HIT,  DEATH};

/// @brief Le clip qu'une figurine joue à défaut de @p clip : le tir sans clip de tir est une
///        attaque ; les autres n'ont pas de repli (vide).
[[nodiscard]] constexpr std::string_view fallbackOf(std::string_view clip) noexcept {
    return clip == RANGED ? ATTACK : std::string_view{};
}
}  // namespace figure_clips

/// @brief Le dossier du **mannequin** d'une silhouette (`LOT-145`, en modèle depuis le
///        `LOT-1005`) : un maillage neutre lié au squelette de la silhouette, et sa fiche — ce que
///        dessine un personnage sans modèle.
[[nodiscard]] std::string mannequinFigureDirectory(std::string_view silhouette);

/// @brief Le dossier des effets, depuis la racine des assets : une bande par effet (`LOT-136`).
inline constexpr std::string_view FX_DIRECTORY = "Common/Fx";

/// @brief La silhouette par défaut d'un personnage qui n'en déclare pas.
inline constexpr std::string_view DEFAULT_SILHOUETTE = "humanoid";

/// @brief Propriété d'entité `npc` et de fiche de créature qui nomme la silhouette (`LOT-145`).
inline constexpr std::string_view SILHOUETTE_PROPERTY = "silhouette";

/**
 * @brief Une figurine à dessiner sur la carte : son modèle, son clip, où elle est et où elle
 *        regarde — ou un **effet** (`effect`), qui reste une bande d'images.
 *
 * Depuis le `LOT-1006` une figurine est un **modèle** (`EX-REN-051`) : celui que nomme @ref model, à défaut celui
 * que déclare la fiche de son dossier (`character.json`, lue par le rendu). Sans l'un ni l'autre,
 * elle se dessine par son **marqueur** (`figureMarkerPath`) : on la voit, on lui parle, et on ne
 * la prend pas pour une illustration.
 */
struct WorldFigureSnapshot {
    /// Figurine : un slug, cherché dans les `Characters/` du lieu et de ses niveaux communs
    /// (`citizen`, `Heroes/brawler`, `LOT-124`) ; à défaut un dossier relatif à `Assets/`
    /// (`Common/Characters/Heroes/brawler`), ou un PNJ de l'atelier à plat (`Npc/<slug>`).
    /// Pour un effet, `FX_DIRECTORY`.
    std::string figure;
    /// Le clip joué : l'un de `figure_clips` (`idle`, `walk`, `attack`, `cast`, `hit`, `death`).
    /// Pour un effet, sa bande (`fire-bolt`, `impact`).
    std::string clip = "idle";
    /// Position **continue**, en cases : `{1.5, 2.5}` est le centre de la case (1, 2).
    core::Vector2 point{};
    /// Rang d'image, pour qui ne porte pas le temps (@ref seconds négatif) : une image fixe.
    int frame = 0;
    /// Temps écoulé depuis le début du clip, en secondes ; négatif : inconnu, @ref frame vaut.
    float seconds = -1.0F;
    /// Le héros : un étage qui le masque s'efface (`LOT-129`).
    bool hero = false;
    /// Un combattant (`LOT-118`).
    bool combatant = false;
    /// Le **modèle** de la figurine, quand l'appelant l'a déjà résolu : le chemin de son `.glb`,
    /// relatif au dossier des assets (`hmi::ResolvedFigure::model`). Vide : le rendu lit la fiche
    /// du dossier de la figurine.
    std::string model;
    /// Le cap de la figurine (`figureHeadingFor`) ; par défaut, face à la caméra.
    float heading = FIGURE_HEADING_FRONT;
    /// Vrai pour un **effet** (`LOT-136`) : la bande @ref clip de `Common/Fx/`, posée à plat sur
    /// @ref point. Un effet n'a ni modèle ni marqueur.
    bool effect = false;

    [[nodiscard]] bool operator==(const WorldFigureSnapshot&) const = default;
};

/**
 * @brief Un **jeton** posé sur une case : ce qui tient lieu de figurine tant qu'il n'y en a pas
 *        (`LOT-128`, décision D3).
 */
struct MaquetteTokenSnapshot {
    MaquetteTokenKind kind = MaquetteTokenKind::Neutral;
    /// La lettre du jeton, déjà choisie (`hmi::maquetteTokenLetter`).
    char letter = '?';
    core::GridPosition cell{};
    /// Un portail porte en plus sa flèche : la sortie se voit avant qu'on lise sa lettre.
    bool arrow = false;

    [[nodiscard]] bool operator==(const MaquetteTokenSnapshot&) const = default;
};

/// @brief Ce qu'un tracé de maquette dessine.
enum class MaquetteTraceShape {
    /// Le contour du losange de **chaque** case citée : une zone, quelle que soit sa forme.
    Outline,
    /// Une ligne brisée reliant les centres des cases citées, dans l'ordre : un trajet.
    Path,
};

/// @brief Un **tracé** de maquette : le contour d'une zone, ou le trajet d'un PNJ.
struct MaquetteTraceSnapshot {
    MaquetteTraceShape shape = MaquetteTraceShape::Outline;
    MaquetteColor color{};
    std::vector<core::GridPosition> cells;

    [[nodiscard]] bool operator==(const MaquetteTraceSnapshot&) const = default;
};

/**
 * @brief Les marques de maquette d'une carte : ses jetons et ses tracés.
 */
struct MaquetteMarks {
    std::vector<MaquetteTokenSnapshot> tokens;
    std::vector<MaquetteTraceSnapshot> traces;

    [[nodiscard]] bool operator==(const MaquetteMarks&) const = default;
};

/**
 * @brief Les marques que @p entities méritent.
 *
 * Les **jetons** se posent toujours : une entité sans figurine est invisible autrement, sur une
 * carte habillée comme sur une maquette. Les **tracés** — contours de zone, trajets — et les
 * flèches de portail ne paraissent qu'en maquette : une carte finie ne montre pas ses
 * déclencheurs.
 *
 * Un PNJ dont une **figurine** occupe la case n'a pas de jeton (`LOT-145`) : le jeton d'un
 * personnage tenait lieu de figurine, et il y en a une — un mannequin ou la vraie.
 *
 * @param entities Les entités de la carte.
 * @param maquette Vrai si la carte ne nomme aucun lieu.
 * @param figures  Les figurines posées sur la carte, dont les PNJ dessinés.
 */
[[nodiscard]] MaquetteMarks maquetteMarks(const std::vector<core::MapEntity>& entities,
                                          bool maquette,
                                          std::span<const WorldFigureSnapshot> figures = {});

/**
 * @brief Le lieu **en valeurs** : ce que la composition lit, et rien d'autre.
 *
 * `floors` et `relief` portent une entrée par case, ligne par ligne : le **nom** de la pièce de la
 * planche du lieu, vide si la case ne dessine rien. `footprints` donne l'emprise des pièces de
 * relief plus grandes qu'une case : la composition les trie au pied de leur emprise.
 */
/**
 * @brief Une couche d'**étage** (`LOT-129`, `EX-LVL-025`) : les pièces d'une couche de décor à
 *        l'étage `floor`, une entrée par case, ligne par ligne.
 */
struct WorldStoreySnapshot {
    int floor = 1;
    std::vector<std::string> relief;
    /// Le type de chaque case de la couche : sans pièce nommée, un mur s'y extrude en maquette,
    /// comme au rez (`LOT-128`).
    std::vector<core::TileType> types;

    [[nodiscard]] bool operator==(const WorldStoreySnapshot&) const = default;
};

/**
 * @brief Une carte **en valeurs**, telle que la composition la lit : sa grille, ses pièces par
 *        case, ses étages, ses figurines et où trouver leurs fichiers.
 *
 * C'est ce que `hmi::WorldPlay::snapshot` produit et que `composeWorldScene` et
 * `StaticWorldScene::build` consomment : aucune référence à la session ni au disque, donc une
 * valeur partageable entre le fil graphique et le fil de rendu, et comparable dans un test.
 */
struct WorldSceneSnapshot {
    float diamondRatio = core::ARENA_DIAMOND_RATIO;
    int columns = 0;
    int rows = 0;
    /// Le lieu (`central-empire/capital/arenarea`) : ses niveaux disent où sont ses pièces
    /// (`core::sceneLevelCandidates`).
    std::string place;
    /// La plus haute élévation d'une pièce du lieu au-dessus du losange de sa case, en largeurs de
    /// case (`hmi::PlaceAppearance::maximumRise`) : ce qu'un cadrage doit réserver au-dessus.
    float maximumRise = 0.0F;
    std::vector<std::string> floors;
    std::vector<std::string> relief;
    /// Le **type** de chaque case, une entrée par case, ligne par ligne : ce que le rendu de
    /// maquette dessine là où aucune pièce n'est nommée (`LOT-128`).
    std::vector<core::TileType> types;
    /// Le type de chaque case de la couche **décor**, même disposition. Un mur s'y peint aussi
    /// souvent que sur le sol, et il doit s'y extruder pareillement.
    std::vector<core::TileType> reliefTypes;
    std::map<std::string, core::PieceFootprint, std::less<>> footprints;
    /// Les couches d'étage, de la plus basse à la plus haute (`LOT-129`).
    std::vector<WorldStoreySnapshot> storeys;
    /// Le fichier, relatif au dossier des assets, de chaque pièce citée, sous le niveau qui la
    /// déclare (`hmi::PlaceAppearance::pieceFile`, `LOT-124`). Une pièce absente de la table se
    /// cherche en `<nom>.png` dans le dossier propre du lieu (`core::fallbackScenePiecePath`).
    std::map<std::string, std::string, std::less<>> pieceFiles;
    /// Le dossier, relatif au dossier des assets, de chaque figurine posée, sous le niveau qui la
    /// range (`hmi::PlaceAppearance::figureDirectory`, `LOT-124`). Une figurine absente se cherche
    /// par `core::figureDirectory`.
    std::map<std::string, std::string, std::less<>> figureDirectories;
    std::vector<WorldFigureSnapshot> figures;
    /// Les jetons et les tracés de maquette (`LOT-128`), déjà choisis par `maquetteMarks`.
    MaquetteMarks marks;

    /// @return Le nom de la pièce de sol de @p cell, vide hors grille ou sans pièce.
    [[nodiscard]] std::string_view floorAt(core::GridPosition cell) const;
    /// @return Le nom de la pièce de relief de @p cell, vide hors grille ou sans relief.
    [[nodiscard]] std::string_view reliefAt(core::GridPosition cell) const;
    /// @return Le type de @p cell, `core::TileType::Empty` hors grille.
    [[nodiscard]] core::TileType typeAt(core::GridPosition cell) const;
    /// @return Le type de @p cell sur la couche décor, `core::TileType::Empty` hors grille.
    [[nodiscard]] core::TileType reliefTypeAt(core::GridPosition cell) const;

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
 * Un PNJ sans propriété `figure` ne se dessine pas, sauf si @p placeholders est vrai : il prend
 * alors le mannequin de sa silhouette (`mannequinFigureDirectory`, `LOT-145`), et c'est le
 * résolveur de figurines du jeu qui dira ensuite si ce mannequin existe. Le jeu y ajoute le héros
 * (`hmi::WorldPlay::figures`) ; l'éditeur les montre telles quelles, jetons compris.
 * @param entities     Les entités de la carte.
 * @param frame        Le rang d'image des clips (0 pour une image fixe).
 * @param placeholders Vrai pour donner un mannequin aux PNJ sans figurine.
 */
[[nodiscard]] std::vector<WorldFigureSnapshot> npcFigures(
    const std::vector<core::MapEntity>& entities, int frame, bool placeholders = false);

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
 * @brief La clé du marqueur d'une figurine qui n'a pas de modèle (`LOT-39`, `LOT-96`).
 *
 * Une figurine nommée par une carte avant que son atelier ne l'ait produite se dessine par son
 * **marqueur**, comme toute clé d'asset sans image (`EX-CNT-041`).
 *
 * @param path Le chemin du marqueur d'une figurine (`figureMarkerPath`).
 * @return `npc/<slug>`, `monsters/<slug>`, `characters/<dossier>` (`characters/heroes/brawler`),
 *         ou une chaîne vide si @p path n'est pas celui d'un marqueur de figurine.
 */
[[nodiscard]] std::string figureMarkerKey(std::string_view path);

/**
 * @brief Le chemin sous lequel le rendu range le **marqueur** de la figurine @p figure : un nom,
 *        pas un fichier — le marqueur se peint (`<dossier>/@marker`).
 * @param figure Le dossier de la figurine, relatif à `Assets/`, ou un slug seul, cherché à plat.
 */
[[nodiscard]] std::string figureMarkerPath(std::string_view figure);

/// @return Le chemin de la bande de l'effet @p effect, relatif au dossier des assets :
///         `Common/Fx/<effet>.png`.
[[nodiscard]] std::string effectStripPath(std::string_view effect);

/**
 * @return Les chemins de texture que les figurines @p figures demandent, sans doublon, triés : la
 *         bande d'un effet, le marqueur d'une figurine qui ne nomme pas son modèle — le rendu ne
 *         le peint que si la fiche de son dossier n'en donne pas non plus.
 */
[[nodiscard]] std::vector<std::string> worldFigureTexturePaths(
    const WorldSceneSnapshot& snapshot, std::span<const WorldFigureSnapshot> figures);

/**
 * @brief Le dossier de la figurine @p figure, relatif au dossier des assets : celui que
 *        @p snapshot lui donne (`figureDirectories`), à défaut elle-même.
 */
[[nodiscard]] std::string worldFigureDirectory(const WorldSceneSnapshot& snapshot,
                                               std::string_view figure);

/// @return Les fichiers de modèle (`.glb`) que les figurines @p figures nomment elles-mêmes, sans
///         doublon, triés (`LOT-1005`).
[[nodiscard]] std::vector<std::string> worldFigureModelPaths(
    std::span<const WorldFigureSnapshot> figures);

/// @return Tous les chemins de texture que @p snapshot demandera, sans doublon, triés. Une pièce
///         en maillage n'y est pas : son fichier se charge à part (`worldMeshPaths`).
[[nodiscard]] std::vector<std::string> worldTexturePaths(const WorldSceneSnapshot& snapshot);

/// @return Les fichiers de maillage (`.glb`) des pièces que @p snapshot pose, sans doublon, triés
///         (`LOT-1003`) ; vide pour une carte dont toutes les pièces sont des images.
[[nodiscard]] std::vector<std::string> worldMeshPaths(const WorldSceneSnapshot& snapshot);

/// @brief Ce que l'appelant peut changer à la composition — rien, par défaut.
struct WorldComposeOptions {
    /**
     * @brief Les blocs de maquette se dessinent **à plat** : le vocabulaire des plans de principe
     *        du planning (`LevelEditor --render --plan`, `LOT-128`).
     *
     * Un plan dit ce que la carte contient et comment on y circule ; l'extrusion, qui sert à
     * *jouer*, y cacherait justement ce qu'on vient lire — ce qui se trouve derrière un mur.
     */
    bool flatBlocks = false;

    [[nodiscard]] bool operator==(const WorldComposeOptions&) const = default;
};

/**
 * @brief Compose le lieu dans un tampon réutilisé, figurines comprises (`snapshot.figures`).
 *
 * Les briques de l'image du jeu (`hmi::StaticWorldScene`) : la carte (`composeWorldStatics`), les
 * figurines (`composeWorldFigures`), puis l'effacement des étages devant le héros. Le tampon n'est
 * **ni vidé ni trié** : l'appelant enchaîne `clear()`,
 * les compositions, puis `sort()`. Un cadrage de @p scene (`ComposedScene::setVisibleBounds`)
 * écarte ce qu'il ne montre pas.
 */
void composeWorldScene(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                       const core::IsoProjection& projection, const ScenePieceTextures& textures,
                       WorldComposeOptions options = {});

/// @brief Ce qui masque le héros : son image, et son rang de dessin (`LOT-129`).
struct WorldHeroPlacement {
    core::Rect bounds;
    std::int32_t sortOrder = 0;
};

/**
 * @brief Le héros parmi @p figures, placé et mesuré ; rien s'il n'y en a pas, ou s'il n'a pas
 *        d'image.
 *
 * Une pièce d'étage dessinée après lui et qui recouvre son image s'efface
 * (`STOREY_SEE_THROUGH_OPACITY`) : c'est ce qu'on compare à `ComposedQuad::occlusion`.
 */
[[nodiscard]] std::optional<WorldHeroPlacement> placeWorldHero(
    const WorldSceneSnapshot& snapshot, std::span<const WorldFigureSnapshot> figures,
    const core::IsoProjection& projection, const ScenePieceTextures& textures);

/**
 * @brief Compose ce qui ne dépend **que de la carte** : sols, reliefs, étages, jetons et tracés,
 *        dans l'ordre de composition, sans trier.
 *
 * Aucune figurine, aucun effacement devant le héros : les pièces d'étage portent seulement ce
 * qu'elles masquent (`ComposedQuad::occlusion`). C'est la part qu'une image n'a pas à refaire.
 */
void composeWorldStatics(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                         const core::IsoProjection& projection, const ScenePieceTextures& textures,
                         WorldComposeOptions options = {});

/**
 * @brief Compose les figurines @p figures, sans trier ; leur dossier se lit dans @p snapshot.
 */
void composeWorldFigures(ComposedScene& scene, const WorldSceneSnapshot& snapshot,
                         std::span<const WorldFigureSnapshot> figures,
                         const core::IsoProjection& projection, const ScenePieceTextures& textures);

/// @brief Compose le lieu dans une scène neuve, **triée** — commodité des tests et des captures.
[[nodiscard]] ComposedScene composeWorldScene(const WorldSceneSnapshot& snapshot,
                                              const core::IsoProjection& projection,
                                              const ScenePieceTextures& textures,
                                              WorldComposeOptions options = {});

}  // namespace hmi
