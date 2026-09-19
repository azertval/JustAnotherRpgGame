// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Combat/BattleGrid.h"  // core::CombatantId
#include "Core/Combat/TurnOrder.h"   // core::CombatSide
#include "Core/Levels/GridPosition.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/RenderLayer.h"
#include "HMI/Graphics/ScenePieces.h"

/**
 * @file HMI/Graphics/ArenaSceneComposer.h
 * @brief La scène de combat du Colisée composée en primitives, **sans GPU** : ce que dessinait
 *        `ArenaScene.ui.qml`/`ArenaTile.ui.qml` (`LOT-50`), prêt pour `hmi::SpriteBatch`.
 *
 * ## Ce qui va où
 *
 * | Pièce | Calque | Tri |
 * |---|---|---|
 * | sol (sable et variantes, pierre, seuil) | `RenderLayer::Tile` | texture, puis profondeur de
 * case | | enceinte (pan, angle, pilier, bannière, torche, arche) | `RenderLayer::Object` | pied de
 * la case |
 *
 * Le décor vient de l'atelier des textures (`LOT-92`, `Assets/Scene/coliseum/`) : **une** pièce par
 * case d'enceinte, qui porte son mur (une torche est un pan à torche), dressée contre l'arête du
 * fond parallèle au bord de la grille et posée par son ancre. Les figurines restent celles du
 * Colisée. | figurine d'un combattant | `RenderLayer::Player` | pied de son emprise |
 *
 * `Object` et `Player` partagent la bande de profondeur (`hmi::sortsByDepth`) : un mur plus bas à
 * l'écran passe devant une figurine, un mur plus haut derrière — ce que faisait le `z: c + r` de la
 * scène QML, sans que `hmi::ComposedScene::sort` change.
 *
 * ## La profondeur
 *
 * `sortOrder` = `depthSortOrder(pied) × ARENA_DEPTH_SLOTS + rang`. Le pied est le **sommet bas** du
 * losange de la case (de l'emprise, pour une figurine), pas le bord bas du quad : une bannière
 * posée plus haut que son mur doit rester devant lui. Le rang (`ArenaDepthSlot`) départage les
 * pièces d'une même case dans l'ordre où la brique QML les empilait ; laissé à égalité, le tri
 * trancherait par rang de texture, qui dépend de la première case composée.
 *
 * ## Les combattants
 *
 * - `Standing` : une figurine, à l'image que donne `hmi::ArenaAnimationState` ;
 * - `Down` : une figurine **quand même** — le combattant garde sa case et sa place dans l'ordre.
 *   Un allié montre la dernière image de sa bande `death.png`, un ennemi la dernière de `idle.png`,
 *   estompée (`ARENA_DOWN_ENEMY_ALPHA`), comme la brique QML ;
 * - `Withdrawn` : **aucun** quad — il a quitté la grille et l'ordre.
 *
 * Une créature de plus d'une case (`core::footprintSide`) a **une** figurine, centrée sur son
 * emprise et agrandie à sa taille, là où la scène QML en posait une par case tenue.
 *
 * ## Ce qui n'y est pas
 *
 * Les surbrillances de case (atteignable, au tour), la jauge et les points de vie : ce sont des
 * rectangles et du texte d'interface, pas des pièces de la planche, et le curseur de ciblage reste
 * en QML (`LOT-24`).
 *
 * ## Lecture seule, et par instantané
 *
 * La session n'est vue que par `const core::ArenaSession&` (`EX-ARCH-012`) : aucune méthode
 * mutante n'est appelable, et c'est le compilateur qui le garantit — `combat()` et `grid()` n'y
 * rendent que leur version `const`.
 *
 * Elle n'est lue qu'**une fois**, par `snapshotArenaScene`, qui en extrait ce que la composition
 * dessine — la grille et les figurines — **en valeurs** (`LOT-86` Phase 5). La composition ne lit
 * que cet instantané. C'est ce qui permet au rendu Qt Quick (`hmi::ArenaViewportItem`) de composer
 * sur le fil de rendu une session qui vit sur le fil graphique : l'instantané se prend dans
 * `synchronize()`, pendant que le fil graphique est bloqué, et ne garde aucun pointeur vers elle.
 * `core::CombatState` n'est pas copiable ; l'instantané ne copie que ce qui se dessine.
 */

namespace core {
class ArenaSession;
class IsoProjection;
}  // namespace core

namespace hmi {

class ArenaAppearanceCatalog;
struct ArenaAnimationState;

/// Largeur d'une image des bandes d'animation de la planche, en pixels (`manifest.json`, `frame`),
/// quand la bande n'en déclare pas d'autre (`ArenaTexture::frameWidth`). La géométrie des planches
/// de l'atelier est commune à l'arène et aux lieux qu'on parcourt : elle vit dans
/// `HMI/Graphics/ScenePieces.h` (`LOT-09`), et ces noms la désignent.
inline constexpr int ARENA_FIGURE_FRAME_WIDTH_PIXELS = FIGURE_FRAME_WIDTH_PIXELS;
/// Hauteur d'une image des bandes d'animation, en pixels.
inline constexpr int ARENA_FIGURE_FRAME_HEIGHT_PIXELS = FIGURE_FRAME_HEIGHT_PIXELS;
/// Agrandissement de la figurine par rapport à la planche (`ArenaTile.ui.qml`, `1.25`).
inline constexpr float ARENA_FIGURE_SCALE = FIGURE_SCALE;
/// Opacité d'un ennemi à terre (`ArenaTile.ui.qml`, `0.45`).
inline constexpr float ARENA_DOWN_ENEMY_ALPHA = 0.45f;
/// Largeur du losange des textures de scène de l'atelier (`LOT-92`), en pixels d'art : 68 × 42, le
/// rapport 0,62 d'`core::IsoProjection`. Une pièce se pose par son ancre, le sommet haut de son
/// emprise (`anchor` de `Assets/Scene/<lieu>/manifest.json`).
inline constexpr int ARENA_SCENE_TILE_WIDTH_PIXELS = SCENE_TILE_WIDTH_PIXELS;
/// Demi-largeur de ce losange : l'abscisse de l'ancre d'une pièce d'une case.
inline constexpr int ARENA_SCENE_HALF_TILE_WIDTH_PIXELS = SCENE_HALF_TILE_WIDTH_PIXELS;
/// Hauteur de ce losange, en pixels d'art : il occupe le bas d'une pièce d'une case.
inline constexpr int ARENA_SCENE_TILE_HEIGHT_PIXELS = SCENE_TILE_HEIGHT_PIXELS;

/**
 * @brief Rang d'une pièce à l'intérieur d'une même profondeur : l'ordre d'empilement de la brique
 *        `ArenaTile`, du sol vers le ciel.
 */
enum class ArenaDepthSlot : std::int32_t {
    Wall = 0,
    WallDecoration,
    Gate,
    Figure,
};

/// Nombre de rangs par profondeur : le multiplicateur de `depthSortOrder` dans le `sortOrder`.
inline constexpr std::int32_t ARENA_DEPTH_SLOTS = 4;

/**
 * @brief Ordre de tri d'une pièce de la bande de profondeur du Colisée.
 * @param footWorldY Ordonnée du sommet bas du losange (ou de l'emprise) qui porte la pièce.
 * @param slot       Rang de la pièce dans sa case.
 */
[[nodiscard]] std::int32_t arenaDepthSortOrder(float footWorldY, ArenaDepthSlot slot) noexcept;

/// @brief Une texture liable et ses dimensions en pixels (pour normaliser les UV).
struct ArenaTexture {
    TextureHandle texture = nullptr;
    int width = 0;
    int height = 0;
    /// Largeur d'une image si la texture est une bande d'animation (`frameWidth` de son
    /// `.anim.json`) ; 0 sinon, et la composition suppose `ARENA_FIGURE_FRAME_WIDTH_PIXELS`.
    int frameWidth = 0;
};

/**
 * @brief Les textures du Colisée, adressées par leur chemin relatif à
 *        `Source/Elements/Assets/Coliseum/` (`"../Scene/coliseum/sand.png"`,
 *        `"characters/bram/idle.png"`).
 *
 * Même rôle que `hmi::SceneTextures` pour l'exploration : la composition ne demande rien au GPU.
 * Un chemin absent se lie au damier `missing` (`EX-NFR-040`) ; si lui aussi manque, la pièce n'est
 * pas composée.
 */
struct ArenaSceneTextures {
    /// Textures chargées, par chemin. Comparateur transparent : la recherche se fait sans chaîne
    /// temporaire.
    std::map<std::string, ArenaTexture, std::less<>> byPath;
    /// Damier de repli.
    ArenaTexture missing;

    /// @return La texture de @p path, le damier si elle n'est pas chargée.
    [[nodiscard]] const ArenaTexture& resolve(std::string_view path) const {
        const auto found = byPath.find(path);
        return found != byPath.end() ? found->second : missing;
    }
};

/// @brief Ce qu'une figurine dessine d'un combattant, copié de la session.
struct ArenaFigureSnapshot {
    core::CombatantId id{};
    /// Nom du combattant : c'est lui qui choisit la planche (`ArenaAppearanceCatalog::figureFor`).
    std::string name;
    core::CombatSide side = core::CombatSide::Allies;
    /// À terre (`core::CombatantStatus::Down`) ; un combattant sorti n'a pas d'instantané.
    bool down = false;
    /// Coin haut-gauche de son emprise.
    core::GridPosition anchor{};
    /// Côté de son emprise, en cases (au moins 1).
    int footprint = 1;

    friend bool operator==(const ArenaFigureSnapshot&, const ArenaFigureSnapshot&) = default;
};

/**
 * @brief La scène de combat **en valeurs** : ce que la composition lit, et rien d'autre.
 *
 * Ne tient aucun pointeur, aucune référence : il survit à la session dont il est tiré, et traverse
 * sans risque la frontière entre le fil graphique et le fil de rendu.
 */
struct ArenaSceneSnapshot {
    int columns = 0;
    int rows = 0;
    /// Une entrée par case, ligne par ligne : vrai si la case obstrue le sol
    /// (`BattleGrid::isObstructed`, `Locomotion::Walk`) — ce que l'enceinte habille.
    std::vector<bool> obstructed;
    /// Les combattants sur la grille, dans l'ordre de `CombatState::combatants()`. Les sortis
    /// (`Withdrawn`) et ceux qui n'ont pas de case n'y sont pas.
    std::vector<ArenaFigureSnapshot> figures;

    /// @return Vrai si @p cell est dans la grille et obstrue le sol.
    [[nodiscard]] bool isObstructed(core::GridPosition cell) const noexcept;

    friend bool operator==(const ArenaSceneSnapshot&, const ArenaSceneSnapshot&) = default;
};

/**
 * @brief Tous les chemins de texture que la composition peut demander pour @p catalog : sols,
 *        pièces d'enceinte, dalles claires, bandes des figurines.
 *
 * Tenue ici, à côté des chemins que la composition écrit, pour que le rendu charge exactement ce
 * qu'elle résout — ni une pièce oubliée (qui tomberait sur le damier), ni une de trop.
 */
[[nodiscard]] std::vector<std::string> arenaTexturePaths(const ArenaAppearanceCatalog& catalog);

/// @brief Tire de @p session l'instantané que la composition dessine (lecture seule).
[[nodiscard]] ArenaSceneSnapshot snapshotArenaScene(const core::ArenaSession& session);

/**
 * @brief Compose la scène de combat dans un tampon réutilisé, depuis un instantané.
 *
 * Le tampon n'est **ni vidé ni trié** : même contrat que `hmi::composeWorldSprites`, l'appelant
 * enchaîne `clear()`, les compositions, puis `sort()`.
 * @param scene      Scène à remplir.
 * @param snapshot   La grille et les figurines, en valeurs.
 * @param catalog    Rôle des cases et figurines.
 * @param animation  Image courante de chaque figurine.
 * @param projection Projection isométrique de la grille.
 * @param textures   Textures liables.
 * @param scenery    Composer le décor historique ; faux quand la carte fournit le décor.
 */
void composeArenaScene(ComposedScene& scene, const ArenaSceneSnapshot& snapshot,
                       const ArenaAppearanceCatalog& catalog, const ArenaAnimationState& animation,
                       const core::IsoProjection& projection, const ArenaSceneTextures& textures,
                       bool scenery = true);

/**
 * @brief Compose la scène de combat dans un tampon réutilisé.
 *
 * Équivaut à composer `snapshotArenaScene(session)` : un seul chemin de composition.
 * @param scene      Scène à remplir.
 * @param session    La session d'arène, lue seulement.
 * @param catalog    Rôle des cases et figurines.
 * @param animation  Image courante de chaque figurine.
 * @param projection Projection isométrique de la grille (ses dimensions devraient être celles de
 *                   la grille ; la composition parcourt la grille).
 * @param textures   Textures liables.
 */
void composeArenaScene(ComposedScene& scene, const core::ArenaSession& session,
                       const ArenaAppearanceCatalog& catalog, const ArenaAnimationState& animation,
                       const core::IsoProjection& projection, const ArenaSceneTextures& textures);

/**
 * @brief Compose la scène de combat dans une scène neuve, **triée**.
 *
 * Commodité pour les tests et les captures ; le rendu garde un tampon et appelle la forme
 * ci-dessus.
 */
[[nodiscard]] ComposedScene composeArenaScene(const core::ArenaSession& session,
                                              const ArenaAppearanceCatalog& catalog,
                                              const ArenaAnimationState& animation,
                                              const core::IsoProjection& projection,
                                              const ArenaSceneTextures& textures);

}  // namespace hmi
