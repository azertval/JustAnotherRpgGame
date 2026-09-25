// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

class QRhi;
class QRhiResourceUpdateBatch;

/**
 * @file HMI/Graphics/RhiContext.h
 * @brief Accès partagé au `QRhi` courant et au lot de mises à jour de ressources de l'image en
 *        cours (`EX-REN-050`).
 */

namespace hmi {

/**
 * @brief Le `QRhi` courant et le lot de téléversements de l'image en cours.
 *
 * Deux contraintes de QRhi rendent cette indirection nécessaire, là où Direct3D 11 se contentait
 * d'un `ID3D11Device*` passé une fois :
 * - **Un téléversement ne se déclare pas n'importe quand.** Créer une texture ne suffit pas : ses
 *   pixels transitent par un `QRhiResourceUpdateBatch`, qui ne peut être soumis qu'**en dehors**
 *   d'une passe de rendu. Or les textures se chargent paresseusement, en pleine composition
 *   (les bibliothèques de textures des rendus de scène, `hmi::WorldSceneRenderer::textures()`).
 *   Le lot courant est donc exposé ici, rempli au fil de la composition, et soumis d'un bloc
 *   avant l'ouverture de la passe.
 * - **Le `QRhi` peut changer.** `QRhiWidget` recrée ses ressources quand le widget change de
 *   fenêtre de haut niveau. Les propriétaires de textures comparent donc le `QRhi` qu'ils ont
 *   servi à celui du contexte, plutôt que de supposer qu'il ne bouge jamais.
 *
 * L'instance est **possédée par le viewport** et référencée (jamais copiée) par les objets qui
 * créent des textures : `hmi::TextureAtlas` et les rendus de scène (`hmi::WorldSceneRenderer`).
 */
struct RhiContext {
    /// Interface de rendu courante, ou `nullptr` avant la première initialisation.
    QRhi* rhi = nullptr;
    /// Lot de mises à jour de l'image en cours, ou `nullptr` hors d'une image.
    QRhiResourceUpdateBatch* updates = nullptr;

    /// @return `true` si une texture peut être créée et téléversée maintenant.
    [[nodiscard]] bool ready() const noexcept {
        return rhi != nullptr && updates != nullptr;
    }
};

}  // namespace hmi
