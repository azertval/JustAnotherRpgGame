// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QColor>
#include <QImage>
#include <QSize>
#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>

#include "HMI/Graphics/WorldSceneRenderer.h"

class QRhi;
class QRhiRenderPassDescriptor;
class QRhiTexture;
class QRhiTextureRenderTarget;

/**
 * @file HMI/Graphics/OffscreenRender.h
 * @brief Le rendu d'un lieu **hors écran**, en image (`LOT-1002`) : `LevelEditor --render`, les
 *        vignettes de l'éditeur, les mesures et les tests.
 *
 * Le canevas de l'éditeur dessine par `hmi::WorldSceneRenderer` dans sa fenêtre ; ce qui n'a pas de
 * fenêtre passe par le même rendu, sur un `QRhi` sans surface. Il n'y a donc qu'un rendu d'un lieu,
 * et une image hors écran est, au pixel, celle que le jeu dessinerait avec le même cadrage.
 *
 * Sans carte graphique, Direct3D rend par WARP : c'est ce qui fait tourner `--render` en CI, comme
 * les tests de rendu du jeu.
 */

namespace hmi {

/// Le plus grand côté d'une cible de rendu hors écran, en pixels : une image plus grande se rend
/// par tuiles, ce qui borne la mémoire graphique d'un `--render` de 8 192 pixels de côté.
inline constexpr int OFFSCREEN_TILE_SIDE = 4096;

/// Le plafond des textures qu'un rendu **gardé** conserve d'une image à l'autre, en octets de
/// mémoire graphique : au-delà, il les rend avant de resservir (`OffscreenRhi::renderer`). C'est le
/// budget que tenait le cache d'images du canevas (`LOT-125`).
inline constexpr std::size_t OFFSCREEN_TEXTURE_BUDGET_BYTES = std::size_t{256} * 1024 * 1024;

/**
 * @brief Une interface `QRhi` sans fenêtre, la cible où elle dessine, et les rendus qu'elle garde.
 *
 * Elle ne se partage qu'entre objets du **même fil**. Tout `hmi::WorldSceneRenderer` qui a dessiné
 * par elle doit être libéré (`release`) ou détruit **avant** elle : ses ressources lui
 * appartiennent. Les rendus qu'elle garde (`renderer`) meurent avec elle, dans cet ordre.
 */
class OffscreenRhi {
public:
    ~OffscreenRhi();
    OffscreenRhi(const OffscreenRhi&) = delete;
    OffscreenRhi& operator=(const OffscreenRhi&) = delete;

    /**
     * @brief L'interface partagée : la même pour tous ceux qui la tiennent, recréée quand plus
     *        personne ne la tient.
     * @return L'interface, ou `nullptr` si la machine n'en offre aucune (`EX-NFR-040`).
     */
    [[nodiscard]] static std::shared_ptr<OffscreenRhi> shared();

    [[nodiscard]] QRhi* rhi() const noexcept {
        return _rhi.get();
    }

    /**
     * @brief Le rendu **gardé** du dossier d'assets @p assetsDirectory, ressources créées.
     *
     * Ses textures restent d'une image à l'autre tant que l'interface vit : vingt vignettes d'un
     * même kit ne décodent ses pièces qu'une fois. Elles sont bornées : un rendu qui tient plus de
     * `OFFSCREEN_TEXTURE_BUDGET_BYTES` est vidé avant d'être resservi, et recharge ce que la
     * prochaine carte demande. L'appelant règle tout ce dont son image dépend — carte, figurines,
     * opacité, options de composition : le rendu garde les réglages de l'appel d'avant.
     *
     * @return Le rendu, ou `nullptr` si ses ressources ne se créent pas.
     */
    [[nodiscard]] WorldSceneRenderer* renderer(const std::filesystem::path& assetsDirectory);

    /**
     * @brief Rend une image de @p size pixels du lieu de @p renderer, cadrée par @p framing.
     *
     * Le cadrage est celui de l'image **entière** ; une image plus grande que @p tileSide se rend
     * tuile par tuile, chacune cadrée sur sa part. Le cadrage imposé de @p renderer est rétabli
     * après le rendu.
     *
     * @param renderer Le rendu, sa carte déjà donnée ; ses ressources sont créées au besoin.
     * @param size     La taille de l'image, en pixels.
     * @param framing  Le centre de l'image et son échelle.
     * @param clear    Le fond ; transparent, l'image garde l'alpha de ce qui est dessiné.
     * @param tileSide Le plus grand côté d'une tuile, en pixels.
     * @return L'image, prémultipliée (`QImage::Format_RGBA8888_Premultiplied`), nulle en cas
     *         d'échec.
     */
    [[nodiscard]] QImage render(WorldSceneRenderer& renderer, QSize size,
                                const WorldFraming& framing, const QColor& clear,
                                int tileSide = OFFSCREEN_TILE_SIDE);

private:
    explicit OffscreenRhi(std::unique_ptr<QRhi> rhi);

    /// Garantit une cible de @p size pixels ; la précédente est réutilisée si elle convient.
    [[nodiscard]] bool ensureTarget(QSize size);

    // La cible meurt avant l'interface : l'ordre de déclaration est l'ordre inverse de libération.
    std::unique_ptr<QRhi> _rhi;
    std::unique_ptr<QRhiTexture> _texture;
    std::unique_ptr<QRhiRenderPassDescriptor> _pass;
    std::unique_ptr<QRhiTextureRenderTarget> _target;
    /// Les rendus gardés, par dossier d'assets.
    std::map<std::filesystem::path, std::unique_ptr<WorldSceneRenderer>> _renderers;
};

}  // namespace hmi
