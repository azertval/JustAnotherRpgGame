// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "Core/Combat/IsoProjection.h"
#include "Core/Math/Rect.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/WorldSceneComposer.h"

/**
 * @file HMI/Graphics/StaticWorldScene.h
 * @brief Un lieu composé **une fois**, puis découpé à la vue à chaque image (audit de l'affichage
 *        d'un lieu, `Planning/standards/audit-affichage-lieu.md`).
 *
 * Une carte ne change qu'en entrant, ou quand un drapeau fait paraître un PNJ ou fermer une porte ;
 * ses figurines, elles, bougent à chaque image. Composer et trier toute la carte à chaque image
 * coûtait autant qu'un quartier est grand, alors que l'écran n'en montre qu'une vingtaine de cases
 * sur onze.
 *
 * `build` compose donc ce qui ne dépend que de la carte (`hmi::composeWorldStatics`), le trie une
 * fois et range chaque primitive dans une **grille de seaux** en unités monde, selon sa boîte
 * englobante : une pièce haute ou large est dans tous les seaux qu'elle touche. `compose` ne prend
 * que les seaux sous le cadrage, garde les primitives qui le touchent vraiment, dans l'ordre déjà
 * trié, et y **fusionne** les figurines de l'image par le même comparateur
 * (`ComposedScene::drawsBefore`). Au passage, une pièce d'étage qui recouvre le héros et se dessine
 * après lui s'efface (`LOT-129`).
 *
 * Le résultat est, primitive pour primitive, celui d'une composition complète suivie d'un tri,
 * privé de ce qui est hors cadre : même ordre, mêmes rangs de texture. `hmi::composeWorldScene`
 * passe par ici, ce qui fait du jeu, de l'essai de l'éditeur, de son canevas et des tests un seul
 * chemin.
 *
 * Logique pure, sans GPU ni Qt. Une instance n'est pas partagée entre deux fils.
 */

namespace hmi {

/**
 * @brief La partie **fixe** d'une carte : composée une fois, triée, indexée par seaux, puis
 *        découpée au cadrage à chaque image, les figurines de l'instant insérées (`LOT-129`).
 *
 * `build` compose et indexe tout sauf les figurines ; `compose` extrait ce qui touche le cadrage
 * de la scène de sortie et y ajoute les figurines, dans l'ordre de dessin.
 */
class StaticWorldScene {
public:
    /// Côté d'un seau de la grille, en largeurs de case : quelques seaux par écran, peu de
    /// primitives par seau.
    static constexpr float BUCKET_TILES = 4.0F;

    /**
     * @brief Compose la partie fixe de @p snapshot (tout sauf ses figurines) et l'indexe.
     *
     * Les textures de @p textures doivent être celles du dessin : une pièce dont la texture
     * n'est pas encore chargée se compose sur le damier, comme dans `composeWorldScene`.
     */
    void build(const WorldSceneSnapshot& snapshot, const core::IsoProjection& projection,
               const ScenePieceTextures& textures, WorldComposeOptions options = {});

    /// Oublie la scène composée.
    void clear() noexcept;

    /**
     * @brief Compose une image dans @p out : la partie fixe sous le cadrage de @p out, et
     *        @p figures, dans l'ordre de dessin.
     *
     * Le cadrage est celui de @p out (`ComposedScene::setVisibleBounds`) ; sans cadrage, toute la
     * carte. @p out est **remplacée** si elle est vide, sinon la composition s'y ajoute et
     * l'appelant la retrie.
     *
     * @param out      La scène de l'image.
     * @param figures  Les figurines de l'image, héros compris ; leur dossier se lit dans
     *                 l'instantané passé à `build`.
     * @param textures Les textures du dessin (celles des figurines comprises).
     */
    void compose(ComposedScene& out, std::span<const WorldFigureSnapshot> figures,
                 const ScenePieceTextures& textures) const;

    /// @return La partie fixe de toute la carte, triée : ce que `compose` découpe.
    [[nodiscard]] const ComposedScene& scene() const noexcept {
        return _scene;
    }

    /// @return Le nombre de primitives fixes, toute la carte.
    [[nodiscard]] std::size_t size() const noexcept {
        return _scene.size();
    }

    /// @return Vrai si rien n'est composé.
    [[nodiscard]] bool empty() const noexcept {
        return _scene.size() == 0;
    }

    /// @return La projection de la dernière composition.
    [[nodiscard]] const core::IsoProjection& projection() const noexcept {
        return _projection;
    }

private:
    /// Construit la grille de seaux, à la première image cadrée.
    void index() const;
    /// Les indices, dans l'ordre trié, des primitives fixes qui touchent @p bounds.
    void visibleIndices(const core::Rect& bounds, std::vector<std::uint32_t>& indices) const;

    /// Ce que les figurines lisent de l'instantané : le lieu et le dossier de chaque figurine.
    WorldSceneSnapshot _figureContext;
    core::IsoProjection _projection{0, 0};
    /// Toute la partie fixe, triée une fois.
    ComposedScene _scene;
    /// L'index est bâti (`index`) : la boîte de chaque primitive fixe, dans le même ordre, et la
    /// grille — origine, côté d'un seau, dimensions, et pour chaque seau les indices des
    /// primitives qui le touchent (croissants, donc dans l'ordre de dessin).
    mutable bool _indexed = false;
    mutable std::vector<core::Rect> _bounds;
    mutable core::Vector2 _origin{};
    mutable float _bucketSize = 1.0F;
    mutable int _bucketColumns = 0;
    mutable int _bucketRows = 0;
    mutable std::vector<std::vector<std::uint32_t>> _buckets;
    /// Marque de la dernière image qui a retenu chaque primitive : une pièce présente dans
    /// plusieurs seaux n'est prise qu'une fois, sans ensemble à vider.
    mutable std::vector<std::uint32_t> _stamps;
    mutable std::uint32_t _frame = 0;
    /// Tampons réutilisés d'une image à l'autre.
    mutable std::vector<std::uint32_t> _visible;
    mutable std::vector<ComposedQuad> _merged;
};

/**
 * @brief Vrai si @p quad, pièce d'étage dessinée après le héros, recouvre son image (`LOT-129`) :
 *        elle doit s'effacer (`STOREY_SEE_THROUGH_OPACITY`).
 */
[[nodiscard]] bool hidesHero(const ComposedQuad& quad,
                             const std::optional<WorldHeroPlacement>& hero) noexcept;

/// @brief Efface, dans @p quads, les pièces d'étage qui recouvrent le héros (`hidesHero`).
void fadeStoreysOverHero(std::span<ComposedQuad> quads,
                         const std::optional<WorldHeroPlacement>& hero) noexcept;

}  // namespace hmi
