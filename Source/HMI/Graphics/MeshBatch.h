// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <DirectXMath.h>

#include "Core/Resources/MeshFile.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/SceneLighting.h"
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
 *
 * ## Les maillages animés (`LOT-1005`)
 *
 * Un maillage lié à un squelette (`core::MeshData::skin`) porte un second tampon de sommets — les
 * quatre os et les quatre poids de chacun — et se dessine par un second pipeline, dont le shader
 * déforme chaque sommet par les matrices de ses os. La **pose** est donnée au dessin (`draw`,
 * `bones`) : seize flottants par os, tels que `core::poseSkeleton` les rend. Dessiné sans pose,
 * un maillage lié reste dans sa pose de liaison, par le pipeline des maillages fixes. Un maillage
 * fixe ne passe jamais par le second pipeline : son image est celle d'avant le lot.
 */
class LightingBlock;

/**
 * @brief Ce qu'un dessin donne à l'éclairage (`LOT-1007`). La valeur d'usine — des matrices
 *        nulles, sous le bloc d'éclairage neutre — est le dessin d'avant le lot.
 */
struct MeshLighting {
    /// Du repère du maillage à celui de la vue : la position et la normale éclairées.
    LightMatrix toView{};
    /// Du repère du maillage au clip de la carte d'ombres.
    LightMatrix toShadowClip{};
    /// Vrai : le maillage ne paraît pas à l'image, il n'y fait que porter son ombre — la boîte
    /// d'une pièce de décor en image (`hmi::WorldShadowBox`).
    bool castsOnly = false;
};

class MeshBatch {
public:
    /// @param rhi Interface de rendu, non possédée ; doit survivre à l'objet.
    /// Le plus d'os qu'un squelette dessiné peut porter : la taille du bloc d'os du shader
    /// (`mesh_skinned.vert`). Le squelette humanoïde en a 53.
    static constexpr std::size_t MAX_BONES = 64;

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

    /// @brief Libère le maillage @p mesh ; son identité ne vaut plus. Sans effet pour une identité
    ///        nulle ou inconnue. À appeler hors de toute image enregistrée.
    void destroy(MeshHandle mesh) noexcept;

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
     * @param bones   La pose d'un maillage animé : seize flottants par os (`core::poseSkeleton`).
     *                Vide, ou d'une taille qui n'est pas celle du squelette du maillage : il se
     *                dessine dans sa pose de liaison.
     * @param lighting Ce que le dessin donne à l'éclairage (`MeshLighting`) : sa pose dans la vue,
     *                sa projection dans la carte d'ombres. La valeur d'usine, sous le bloc
     *                d'éclairage neutre, est le dessin d'avant le `LOT-1007`.
     */
    void draw(MeshHandle mesh, const DirectX::XMFLOAT4X4& clip, float opacity = 1.0F,
              std::span<const float> bones = {}, const MeshLighting& lighting = {});

    /**
     * @brief Lie le bloc d'éclairage @p lighting aux pipelines (`LOT-1007`) ; `nullptr` : le bloc
     *        neutre du lot. Le bloc doit vivre aussi longtemps qu'il est lié.
     */
    void setLighting(LightingBlock* lighting);

    /// @return Le nombre de dessins **animés** enregistrés pour l'image en cours.
    [[nodiscard]] std::size_t skinnedDrawCount() const noexcept {
        return _poses.size() / (MAX_BONES * 16);
    }

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
    ///        Un dessin qui ne fait que porter une ombre (`MeshLighting::castsOnly`) n'y paraît
    ///        pas.
    void record(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target);

    /**
     * @brief Prépare la **passe d'ombres** (`LOT-1007`, `EX-REN-054`) de l'image enregistrée :
     *        ses pipelines,
     *        pour la cible @p target de la carte d'ombres. À appeler après `prepare`.
     * @return Vrai s'il y a quelque chose à y dessiner.
     */
    [[nodiscard]] bool prepareShadow(QRhiRenderTarget* target);

    /// @brief Émet les dessins de la passe d'ombres, dans la passe ouverte par l'appelant sur la
    ///        carte d'ombres : chaque maillage enregistré, vu du soleil, profondeur seule.
    void recordShadow(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target);

private:
    /// Un maillage sur le GPU.
    struct GpuMesh {
        std::unique_ptr<QRhiBuffer> vertices;
        std::unique_ptr<QRhiBuffer> indices;
        /// Les os et les poids de chaque sommet ; nul pour un maillage fixe.
        std::unique_ptr<QRhiBuffer> skin;
        /// Le nombre d'os du squelette ; 0 pour un maillage fixe.
        std::size_t boneCount = 0;
        LoadedTexture texture;
        std::size_t indexCount = 0;
        std::array<float, 4> baseColor{1.0F, 1.0F, 1.0F, 1.0F};
        /// Liaisons de ce maillage : son bloc uniforme à décalage dynamique, sa texture. Refaites
        /// quand le tampon uniforme change.
        std::unique_ptr<QRhiShaderResourceBindings> bindings;
        /// Les mêmes, plus le bloc d'os, pour le pipeline animé.
        std::unique_ptr<QRhiShaderResourceBindings> skinnedBindings;
    };

    /// Les quatre pipelines : fixe ou animé, à l'image ou à la carte d'ombres.
    struct Pipeline {
        std::unique_ptr<QRhiGraphicsPipeline> pipeline;
        /// La passe et le nombre d'échantillons de la cible pour laquelle il a été construit.
        QRhiRenderPassDescriptor* pass = nullptr;
        int samples = 1;
    };

    /// Un dessin enregistré : un maillage, sa matrice, sa teinte.
    struct Draw {
        GpuMesh* mesh = nullptr;
        DirectX::XMFLOAT4X4 clip{};
        float opacity = 1.0F;
        /// Le rang de sa pose parmi les dessins animés de l'image ; -1 : pose de liaison.
        int pose = -1;
        MeshLighting lighting{};
    };

    /// Garantit le pipeline @p kept pour la cible @p target : fixe ou animé (@p skinned), à
    /// l'image ou à la carte d'ombres (@p shadow).
    bool ensurePipeline(Pipeline& kept, QRhiRenderTarget* target, bool skinned, bool shadow);
    /// Garantit les liaisons de référence et celles de la passe d'ombres.
    bool ensureLayouts(bool skinned);
    /// Oublie toutes les liaisons : le tampon uniforme, le bloc d'os ou le bloc d'éclairage
    /// qu'elles citent a changé.
    void resetBindings(bool skinnedOnly);
    /// @return Le bloc d'éclairage lié : celui de l'appelant, sinon le bloc neutre du lot.
    [[nodiscard]] LightingBlock* lighting() const noexcept {
        return _lighting != nullptr ? _lighting : _ownLighting.get();
    }
    bool ensureUniformCapacity(std::size_t drawCount);
    bool ensureBoneCapacity(std::size_t poseCount);
    QRhiShaderResourceBindings* bindingsFor(GpuMesh& mesh);
    QRhiShaderResourceBindings* skinnedBindingsFor(GpuMesh& mesh);

    QRhi* _rhi;  // non possédé
    std::unique_ptr<QRhiSampler> _sampler;
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    /// Le bloc d'éclairage neutre du lot, lié tant que l'appelant n'en donne pas un autre.
    std::unique_ptr<LightingBlock> _ownLighting;
    /// Le bloc de l'appelant (`setLighting`), non possédé ; `nullptr` : le bloc neutre.
    LightingBlock* _lighting = nullptr;
    /// La révision du bloc lié pour laquelle les liaisons ont été faites.
    std::uint64_t _lightingRevision = 0;
    std::unique_ptr<QRhiShaderResourceBindings> _layoutBindings;
    Pipeline _pipeline;
    /// La passe d'ombres des maillages fixes : ses liaisons — le bloc du dessin, rien d'autre —
    /// valent pour tous les maillages.
    std::unique_ptr<QRhiShaderResourceBindings> _shadowBindings;
    Pipeline _shadowPipeline;
    std::size_t _uniformSlots = 0;
    int _uniformStride = 0;
    /// Le pipeline des maillages animés, son tampon d'os (un bloc de `MAX_BONES` matrices par
    /// dessin animé) et ses liaisons de référence.
    std::unique_ptr<QRhiBuffer> _boneBuffer;
    std::unique_ptr<QRhiShaderResourceBindings> _skinnedLayoutBindings;
    Pipeline _skinnedPipeline;
    /// La passe d'ombres des maillages animés : le bloc du dessin et le bloc d'os.
    std::unique_ptr<QRhiShaderResourceBindings> _skinnedShadowBindings;
    Pipeline _skinnedShadowPipeline;
    std::size_t _boneSlots = 0;
    int _boneStride = 0;
    /// Les poses de l'image : `MAX_BONES` matrices par dessin animé, l'identité au-delà du
    /// squelette.
    std::vector<float> _poses;

    std::vector<std::unique_ptr<GpuMesh>> _meshes;
    std::size_t _bytes = 0;
    std::vector<Draw> _draws;
    bool _drawable = false;
    /// La passe d'ombres a de quoi dessiner (`prepareShadow`, lu par `recordShadow`).
    bool _shadowDrawable = false;
};

}  // namespace hmi
