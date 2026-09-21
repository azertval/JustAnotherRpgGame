// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Core/Combat/TacticalTerrain.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Math/Rect.h"
#include "Editor/Logic/LayerView.h"
#include "HMI/Graphics/ComposedScene.h"

/**
 * @file Editor/Ui/DraftRenderer.h
 * @brief Rendu d'un brouillon d'édition (`core::LevelDraft`) dans le canevas de l'éditeur.
 */

namespace core {
class LevelDraft;
struct MapEntity;
class TileMap;
}  // namespace core

namespace hmi {

/**
 * @brief Les textures que la vue à plat nomme, en identités opaques : la composition ne sait pas
 *        comment elles seront peintes (`LOT-EDITOR-02` : par `QPainter`,
 * `hmi::paintComposedScene`).
 */
struct DraftTextures {
    /// L'atlas procédural des types de tuile, et sa taille en pixels (pour les UV).
    TextureHandle atlas = nullptr;
    int atlasWidth = 1;
    int atlasHeight = 1;
    /// Une teinte unie : les aides d'édition (grille, masques, cadres) se peignent en aplat.
    TextureHandle solid = nullptr;
    /// Le marqueur d'une clé d'asset d'entité (`LOT-39`), `nullptr` si elle est refusée.
    std::function<TextureHandle(const std::string& key)> marker;
};

/**
 * @brief Ce que le canevas montre des entités de carte (`LOT-11`), fourni à chaque rendu.
 */
struct DraftEntityOverlay {
    /// Entité sélectionnée : cernée, et sa rencontre montrée sur le terrain.
    std::optional<std::size_t> selectedEntity;
    /// Verdicts de terrain tactique des rencontres de la carte (non possédés), ou `nullptr`.
    const std::vector<core::EncounterTerrain>* terrains = nullptr;
    /// Montrer la zone et la formation de la rencontre sélectionnée — outil « Entité » actif.
    bool showTerrain = false;
};

/**
 * @brief Compose un `core::LevelDraft` en cours d'édition, **à plat** : la vue qui lit les types
 *        et la collision (décision D1 de la feuille de route de l'éditeur).
 *
 * Une couleur par type de tuile (`hmi::maquetteColor`, la palette de maquette), les couches visuelles
 * dans leur ordre, la collision en masque teinté par catégorie, puis les entités par leur marqueur
 * de famille (`LOT-39`). Cette vue ne cherche pas à ressembler au jeu : elle montre ce qu'on
 * édite — le type de chaque case. La vue iso, par défaut, montre le lieu (`hmi::EditorViewport`).
 *
 * Toutes les primitives sont composées dans une seule `hmi::ComposedScene`, en unités de case :
 * les aides d'édition portent le calque `RenderLayer::EditorOverlay`, qui les place au-dessus du
 * reste par construction. La liste est inspectable sans GPU (`EX-NFR-004`) ; le canevas la peint.
 */
class DraftRenderer {
public:
    explicit DraftRenderer(DraftTextures textures);

    /**
     * @brief Compose le brouillon.
     *
     * @param draft         Brouillon de carte à dessiner.
     * @param visible       Rectangle visible, en cases : ce qui est hors cadre n'est pas composé.
     * @param showGrid      Superpose la grille des cases (`EX-EDIT-023`).
     * @param highlight     Zone à voiler (bornes incluses) : l'aperçu des outils
     *                      Rectangle/Sélection.
     * @param entityOverlay Entité sélectionnée et terrains de rencontre à superposer (rien par
     *                      défaut).
     * @return La scène composée, triée ; valide jusqu'au prochain appel.
     */
    const ComposedScene& compose(
        const core::LevelDraft& draft, const std::optional<core::Rect>& visible, bool showGrid,
        const std::optional<std::pair<core::GridPosition, core::GridPosition>>& highlight,
        const DraftEntityOverlay& entityOverlay = {});

    void setLayerView(const LayerViewState& view);

    /// @return La scène composée au dernier appel de `compose`.
    [[nodiscard]] const ComposedScene& lastScene() const noexcept {
        return _scene;
    }

private:
    /// Compose les tuiles d'une grille, de rang @p order dans le calque @p layer.
    void composeTiles(const core::TileMap& tiles, RenderLayer layer, std::int32_t order,
                      float opacity);
    /// Compose la grille des cases sur le calque d'édition.
    void composeGrid(const core::LevelDraft& draft);
    /// Compose le voile d'aperçu d'une zone (outil Rectangle/Sélection) sur le calque d'édition.
    void composeHighlight(const core::GridPosition& minimum, const core::GridPosition& maximum);
    /// Compose le masque de collision d'une carte à couches (voiles par catégorie, `LOT-11`).
    void composeCollisionMask(const core::LevelDraft& draft);
    /// Compose les entités, la sélection, et le terrain de la rencontre sélectionnée.
    void composeEntities(const core::LevelDraft& draft, const DraftEntityOverlay& overlay);
    /// Compose le terrain d'une rencontre : sa zone, puis la case voulue de chaque combattant.
    void composeEncounterTerrain(const core::EncounterTerrain& terrain);
    /// Compose le marqueur d'une entité et, si @p selected, son cadre de sélection.
    void composeEntityMarker(const core::MapEntity& entity, bool selected);
    /// Ajoute un quad uni teinté @p (x, y, w, h) au calque d'édition, rang @p order.
    void addOverlayRect(float x, float y, float width, float height, float r, float g, float b,
                        float a, std::int32_t order);

    LayerViewState _layerView;
    DraftTextures _textures;
    ComposedScene _scene;
};

}  // namespace hmi
