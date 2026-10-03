// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstdint>
#include <memory>

#include "HMI/Graphics/SceneLighting.h"

class QRhi;
class QRhiBuffer;
class QRhiRenderPassDescriptor;
class QRhiResourceUpdateBatch;
class QRhiSampler;
class QRhiShaderResourceBinding;
class QRhiTexture;
class QRhiTextureRenderTarget;

/**
 * @file HMI/Graphics/LightingBlock.h
 * @brief Les ressources GPU de l'**éclairage d'une image** (`LOT-1007`) : le bloc uniforme que
 *        lisent les deux pipelines, et la carte d'ombres.
 */

namespace hmi {

/**
 * @brief Le bloc `Lighting` des shaders et la carte d'ombres, sur un `QRhi`.
 *
 * `hmi::SpriteBatch` et `hmi::MeshBatch` lient chacun un bloc aux points 3 et 4 de leurs shaders.
 * Chacun possède le sien, **neutre**, dès sa construction : un rendu qui ne règle pas d'éclairage
 * — la galerie des assets, un test — dessine comme avant le lot. Le rendu d'un lieu leur en donne
 * un commun (`setLighting`), qu'il remplit à chaque image.
 *
 * La carte d'ombres est une texture de profondeur lue par comparaison. Tant qu'aucune n'est
 * demandée (`ensureShadowMap`), une carte d'un texel tient sa place : les liaisons d'un pipeline
 * doivent être complètes, et le bloc neutre dit aux shaders de ne pas la lire.
 *
 * `revision()` avance quand la carte d'ombres change d'objet : les liaisons qui la citaient sont
 * alors à refaire.
 */
class LightingBlock {
public:
    /// Points de liaison du bloc et de la carte d'ombres dans les shaders.
    static constexpr int UNIFORM_BINDING = 3;
    static constexpr int SHADOW_BINDING = 4;
    /// Bornes du côté de la carte d'ombres, en texels.
    static constexpr int MINIMUM_SHADOW_SIZE = 256;
    static constexpr int MAXIMUM_SHADOW_SIZE = 8192;

    /// @throws std::runtime_error Si le tampon, la carte ou l'échantillonneur ne se créent pas.
    explicit LightingBlock(QRhi* rhi);
    ~LightingBlock();

    LightingBlock(const LightingBlock&) = delete;
    LightingBlock& operator=(const LightingBlock&) = delete;

    /// @return La liaison du bloc uniforme, au point `UNIFORM_BINDING` de l'étage de fragment.
    [[nodiscard]] QRhiShaderResourceBinding uniformBinding() const;
    /// @return La liaison de la carte d'ombres, au point `SHADOW_BINDING` de l'étage de fragment.
    [[nodiscard]] QRhiShaderResourceBinding shadowBinding() const;

    /// @return Un compte qui avance quand la carte d'ombres change d'objet.
    [[nodiscard]] std::uint64_t revision() const noexcept {
        return _revision;
    }

    /// @brief Dépose @p uniforms dans @p updates : le bloc de la prochaine passe.
    void upload(QRhiResourceUpdateBatch* updates, const LightingUniforms& uniforms);

    /**
     * @brief Le téléversement du bloc **neutre** posé à la construction, s'il attend encore.
     *
     * Un téléversement ne se soumet qu'avec une passe : qui lie le bloc le prend à sa première
     * image et le joint à son lot. `nullptr` une fois pris.
     */
    [[nodiscard]] QRhiResourceUpdateBatch* takePendingUpload() noexcept;

    /**
     * @brief Garantit une carte d'ombres de @p size texels de côté et sa cible de rendu.
     * @return La cible, ou `nullptr` si elle ne se crée pas : l'image se dessine alors sans ombres.
     */
    [[nodiscard]] QRhiTextureRenderTarget* ensureShadowMap(int size);

    /// @return La cible de la carte d'ombres courante ; `nullptr` tant qu'aucune n'est demandée.
    [[nodiscard]] QRhiTextureRenderTarget* shadowTarget() const noexcept {
        return _target.get();
    }

private:
    /// Crée la carte de @p size texels ; la précédente est gardée en cas d'échec.
    [[nodiscard]] bool createMap(int size, bool withTarget);

    QRhi* _rhi;  // non possédé
    std::unique_ptr<QRhiBuffer> _buffer;
    std::unique_ptr<QRhiSampler> _sampler;
    std::unique_ptr<QRhiTexture> _map;
    std::unique_ptr<QRhiRenderPassDescriptor> _pass;
    std::unique_ptr<QRhiTextureRenderTarget> _target;
    int _size = 0;
    std::uint64_t _revision = 0;
    /// Le téléversement du bloc neutre, tant qu'aucune passe ne l'a emporté.
    QRhiResourceUpdateBatch* _pendingUpload = nullptr;
};

}  // namespace hmi
