// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <vector>

#include <DirectXMath.h>

#include "Core/Resources/MeshFile.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/TextureLoader.h"

class QRhi;
class QRhiBuffer;
class QRhiCommandBuffer;
class QRhiGraphicsPipeline;
class QRhiRenderPassDescriptor;
class QRhiRenderTarget;
class QRhiResourceUpdateBatch;
class QRhiSampler;
class QRhiShaderResourceBindings;

/**
 * @file HMI/Graphics/MeshBatch.h
 * @brief La **passe de maillages** sur QRhi (`LOT-1003`) : des volumes opaques, départagés par le
 *        tampon de profondeur.
 */

namespace hmi {

struct RhiContext;

/**
 * @brief Dessine des maillages texturés au travers de QRhi : sommets, normales, coordonnées de
 *        texture ; profondeur testée **et écrite** ; couleur de base seule, **sans éclairage**
 *        (la lumière est au `LOT-1007`).
 *
 * Le pendant de `hmi::SpriteBatch` pour ce qui a un volume, et le même découpage en deux temps que
 * QRhi impose : `draw` **enregistre** les maillages de l'image, `prepare` dépose leurs matrices
 * dans le lot de téléversements, `record` émet les appels de dessin dans la passe que l'appelant a
 * ouverte. Les maillages se dessinent **avant** les quads de la même passe : ils écrivent la
 * profondeur que les images testent ensuite.
 *
 * Un maillage chargé (`create`) possède ses tampons de sommets et d'indices, immuables, et la
 * texture de sa couleur de base. La normale est téléversée et transmise au shader, qui ne la lit
 * pas encore. Les faces ne sont pas triées par leur sens : aucune n'est écartée, la profondeur
 * décide — un modèle généré n'a pas toujours un sens de face fiable.
 *
 * L'opacité d'un dessin (`draw`) sert les calques grisés de l'éditeur ; à 1, le mélange
 * prémultiplié du pipeline ne change rien à un maillage opaque.
 */
class MeshBatch {
public:
    /// @param rhi Interface de rendu, non possédée ; doit survivre à l'objet.
    explicit MeshBatch(QRhi* rhi);
    ~MeshBatch();

    MeshBatch(const MeshBatch&) = delete;
    MeshBatch& operator=(const MeshBatch&) = delete;

    /**
     * @brief Charge un maillage sur le GPU : ses tampons, et la texture décodée de sa couleur de
     *        base (blanche s'il n'en a pas, ou si son image ne se décode pas).
     *
     * Les téléversements sont déposés dans le lot de l'image en cours (`context.updates`), que
     * l'appelant soumet avant sa passe.
     * @return L'identité du maillage, valable jusqu'à `clear` ; nulle si le maillage est vide ou
     *         si une ressource ne se crée pas.
     */
    [[nodiscard]] MeshHandle create(const RhiContext& context, const core::MeshData& mesh);

    /// Libère tous les maillages chargés ; leurs identités ne valent plus.
    void clear() noexcept;

    /// @return Les octets de mémoire graphique que tiennent les maillages chargés : tampons,
    ///         textures et leurs mipmaps.
    [[nodiscard]] std::size_t bytes() const noexcept {
        return _bytes;
    }

    /// Ouvre l'enregistrement d'une image : oublie les dessins de la précédente.
    void beginFrame();

    /**
     * @brief Enregistre un maillage à dessiner.
     * @param mesh    Le maillage (`create`) ; nul ou inconnu, rien n'est enregistré.
     * @param clip    Matrice maillage → clip (`hmi::PlaceCamera::meshMatrix`).
     * @param opacity Opacité, de 0 à 1.
     */
    void draw(MeshHandle mesh, const DirectX::XMFLOAT4X4& clip, float opacity = 1.0F);

    /// @return Le nombre de maillages enregistrés pour l'image en cours.
    [[nodiscard]] std::size_t drawCount() const noexcept {
        return _draws.size();
    }

    /**
     * @brief Dépose les matrices de l'image enregistrée dans un lot, **hors** de toute passe.
     * @param target  Cible de rendu de l'image, pourvue d'un tampon de profondeur.
     * @param updates Lot à compléter ; `nullptr` : un lot est pris au besoin.
     * @return Le lot à soumettre à l'ouverture de la passe.
     */
    [[nodiscard]] QRhiResourceUpdateBatch* prepare(QRhiRenderTarget* target,
                                                   QRhiResourceUpdateBatch* updates);

    /// @brief Émet les appels de dessin de l'image préparée, dans la passe ouverte par l'appelant.
    void record(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target);

private:
    /// Un maillage sur le GPU.
    struct GpuMesh {
        std::unique_ptr<QRhiBuffer> vertices;
        std::unique_ptr<QRhiBuffer> indices;
        LoadedTexture texture;
        std::size_t indexCount = 0;
        std::array<float, 4> baseColor{1.0F, 1.0F, 1.0F, 1.0F};
        /// Liaisons de ce maillage : son bloc uniforme à décalage dynamique, sa texture. Refaites
        /// quand le tampon uniforme change.
        std::unique_ptr<QRhiShaderResourceBindings> bindings;
    };

    /// Un dessin enregistré : un maillage, sa matrice, sa teinte.
    struct Draw {
        GpuMesh* mesh = nullptr;
        DirectX::XMFLOAT4X4 clip{};
        float opacity = 1.0F;
    };

    bool ensurePipeline(QRhiRenderTarget* target);
    bool ensureUniformCapacity(std::size_t drawCount);
    QRhiShaderResourceBindings* bindingsFor(GpuMesh& mesh);

    QRhi* _rhi;  // non possédé
    std::unique_ptr<QRhiSampler> _sampler;
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> _layoutBindings;
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;
    QRhiRenderPassDescriptor* _pipelinePass = nullptr;
    std::size_t _uniformSlots = 0;
    int _uniformStride = 0;

    std::vector<std::unique_ptr<GpuMesh>> _meshes;
    std::size_t _bytes = 0;
    std::vector<Draw> _draws;
    bool _drawable = false;
};

}  // namespace hmi
