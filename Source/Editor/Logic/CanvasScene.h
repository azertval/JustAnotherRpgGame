// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>
#include <string>
#include <vector>

#include "Core/Combat/TacticalTerrain.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/TileLayer.h"
#include "Editor/Logic/LayerView.h"
#include "HMI/Graphics/RenderLayer.h"
#include "HMI/Graphics/WorldSceneComposer.h"

/**
 * @file Editor/Logic/CanvasScene.h
 * @brief Ce que le canevas iso montre du brouillon : l'instantané que la composition du jeu
 *        dessine, et l'opacité de chacune de ses bandes (`LOT-EDITOR-02`).
 *
 * ## La même scène que le jeu
 *
 * Le canevas compose le brouillon par `hmi::snapshotWorldScene` et `hmi::composeWorldScene`, les
 * fonctions mêmes du jeu : `canvasSnapshot` n'ajoute que ce que l'éditeur choisit de montrer —
 * les PNJ par leur figurine, sans le héros. Un test compare sa liste de primitives à celle du jeu
 * sur Martpart (`test_canvas_scene.cpp`).
 *
 * ## Les couches, dans une scène fondue
 *
 * La composition fond les couches en bandes de dessin : le sol de la première couche « sol »
 * (`RenderLayer::Tile`), le relief de la première couche « décor » (`RenderLayer::Object`), les
 * figurines (`RenderLayer::Player`). Masquer, griser ou régler l'opacité d'une couche agit donc
 * sur sa bande. La collision n'a pas d'image dans le jeu : en iso, son masque ne se montre que
 * quand on la peint — ailleurs, il couvrirait le lieu qu'on vient voir (décision D1 : la vue à plat
 * sert à lire la collision).
 */

namespace core {
class LevelDraft;
class WorldFlags;
}  // namespace core

namespace hmi {

class PlaceAppearance;

/// @brief Opacité des reliefs **en transparence** : on voit à travers un mur ce qu'on pointe.
inline constexpr float SEE_THROUGH_RELIEF_OPACITY = 0.35F;

/// @brief Opacité du masque de collision en iso, relative à celle de la couche : la moitié de la
/// vue
///        à plat, pour que les murs gardent leur image sous la teinte.
inline constexpr float COLLISION_MASK_OPACITY = 0.3F;

/// @brief L'opacité de chaque bande de la scène iso ; 0 : la bande n'est pas peinte.
struct IsoBandOpacity {
    /// Le sol (`RenderLayer::Tile`), réglé par la première couche de sol.
    float floors = 1.0F;
    /// Le relief (`RenderLayer::Object`), réglé par la première couche de décor.
    float relief = 1.0F;
    /// Les figurines (`RenderLayer::Player`).
    float figures = 1.0F;
    /// Le masque de collision, par-dessus le lieu.
    float collision = 0.0F;
    /// Chaque étage, du premier au dernier, réglé par sa couche (`LOT-129`).
    std::array<float, core::MAX_STOREY_FLOOR> storeys = {1.0F, 1.0F, 1.0F, 1.0F};

    [[nodiscard]] bool operator==(const IsoBandOpacity&) const = default;
};
static_assert(core::MAX_STOREY_FLOOR == 4, "IsoBandOpacity::storeys s'initialise un étage par un");

/**
 * @brief Les opacités des bandes iso, d'après les réglages des couches.
 * @param layers           Les couches du brouillon.
 * @param view             Visibilité, opacité, grisé de chaque couche.
 * @param active           La couche qu'on peint.
 * @param seeThroughRelief Reliefs en transparence (`SEE_THROUGH_RELIEF_OPACITY`).
 */
[[nodiscard]] IsoBandOpacity isoBandOpacity(const std::vector<core::TileLayer>& layers,
                                            const LayerViewState& view, LayerSlot active,
                                            bool seeThroughRelief);

/// @return L'opacité d'une primitive du calque @p layer.
[[nodiscard]] float bandOpacity(const IsoBandOpacity& bands, RenderLayer layer) noexcept;

/// @return L'opacité de @p quad : celle de son étage s'il en a un (`LOT-129`), de son calque sinon.
[[nodiscard]] float bandOpacity(const IsoBandOpacity& bands, const ComposedQuad& quad) noexcept;

/**
 * @brief L'instantané que le canevas compose : celui du jeu, avec les PNJ et sans le héros.
 *
 * Le brouillon n'a pas à être valide : une carte en cours de tracé se montre telle qu'elle est.
 *
 * @param draft      Le brouillon.
 * @param appearance La table du lieu.
 * @param state      Un état de partie (`LOT-126`), ou `nullptr` : sous un état, les entités qu'il
 *                   rend absentes — un PNJ, un décor — ne se composent pas.
 */
[[nodiscard]] WorldSceneSnapshot canvasSnapshot(const core::LevelDraft& draft,
                                                const PlaceAppearance& appearance,
                                                const core::WorldFlags* state = nullptr);

/**
 * @brief Les figurines de la formation d'une rencontre (`LOT-EDITOR-05`) : chaque combattant sur sa
 *        case, par la figurine de l'atelier des monstres qui porte l'identifiant de sa créature
 *        (`Monsters/<créature>`, `LOT-93`, `EX-EDIT-071`).
 *
 * Un combattant sans figurine n'en reçoit pas : le canevas montre sa case, comme avant.
 *
 * @param terrain Le terrain de la rencontre (`core::analyzeEncounterTerrain`).
 * @param figures Les figurines connues (`hmi::EditorReferences::figures`), triées.
 */
[[nodiscard]] std::vector<WorldFigureSnapshot> formationFigures(
    const core::EncounterTerrain& terrain, const std::vector<std::string>& figures);

/**
 * @brief Les pièces d'une case, pour la barre d'état : `street · wall-left`, `street`, ou rien.
 */
[[nodiscard]] std::string cellPieces(const WorldSceneSnapshot& snapshot, core::GridPosition cell);

}  // namespace hmi
