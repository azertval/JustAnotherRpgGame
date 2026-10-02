// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include <DirectXMath.h>

#include "HMI/Graphics/Quad.h"
#include "HMI/Graphics/RenderLayer.h"

class QRhi;
class QRhiBuffer;
class QRhiCommandBuffer;
class QRhiGraphicsPipeline;
class QRhiRenderPassDescriptor;
class QRhiRenderTarget;
class QRhiResourceUpdateBatch;
class QRhiSampler;
class QRhiShaderResourceBindings;
class QRhiTexture;

/**
 * @file HMI/Graphics/SpriteBatch.h
 * @brief Pipeline de rendu 2D sur **QRhi** : accumule des quads texturés et les dessine par lots
 *        (`EX-REN-050`).
 *
 * Les primitives elles-mêmes (`hmi::SpriteQuad`, `hmi::LineQuad`) vivent dans
 * `HMI/Graphics/Quad.h`, sans dépendance graphique, pour que la **composition** du rendu puisse
 * les manipuler sans GPU (`EX-NFR-004`). Ce fichier les réexpose : le contrat public de
 * `SpriteBatch` vis-à-vis de ses appelants est inchangé.
 */

namespace hmi {

/**
 * @brief Dessine des quads texturés au travers de QRhi, avec transparence **prémultipliée** et un
 *        échantillonnage qui suit la nature de chaque texture : bilinéaire avec mipmaps pour l'art
 *        peint, au plus proche pour une image engendrée (`EX-ARCH-022`, `LOT-103`).
 *
 * Usage inchangé pour les appelants : `begin(projection, texture)`, un ou plusieurs `draw(quad)`,
 * puis `end()`. Ce qui change est **quand** le GPU voit ces quads.
 *
 * **Deux phases, contrairement à la version Direct3D 11.** QRhi interdit de téléverser un tampon
 * pendant une passe de rendu : impossible donc de réécrire le tampon de sommets entre deux
 * `drawIndexed`, comme le faisait `Map(WRITE_DISCARD)`. La classe **enregistre** donc toute l'image
 * côté CPU — sommets accumulés bout à bout, plus une liste de lots (texture, projection, plage de
 * quads) — et n'émet quoi que ce soit qu'au `submit()` final : un téléversement unique, puis une
 * passe contenant un appel de dessin par lot. Le découpage en lots, et donc le nombre d'appels,
 * est **strictement le même** qu'avant : c'est `hmi::ComposedScene` qui le décide, pas ce fichier.
 *
 * La projection est portée **par lot** et non par image : le HUD (`EX-REN-047`) se dessine en
 * coordonnées écran dans la même image que le monde. Elle transite par un tampon uniforme adressé
 * à décalage dynamique, un emplacement par lot.
 *
 * **La profondeur (`LOT-1003`).** Un sommet porte une troisième coordonnée, la profondeur de la
 * vue (`hmi::IsoView`), nulle par défaut. Elle n'est **testée** que si l'appelant le demande
 * (`setDepthTest`), c'est-à-dire quand l'image a des maillages à croiser : les quads sont alors
 * comparés au tampon de profondeur sans y écrire — leurs bords sont adoucis, et leur ordre reste
 * celui du peintre. Sans cela, le pipeline est celui d'avant le lot, sans test ni écriture.
 *
 * Pour partager une passe avec un autre pipeline (`hmi::MeshBatch`), `submit` se découpe en
 * `prepare` — les téléversements, hors de toute passe — et `record`, qui émet les appels de dessin
 * dans la passe que l'appelant a ouverte.
 */
class SpriteBatch {
public:
    /**
     * @brief Construit le pipeline (shaders, tampons, échantillonneur) sur une interface QRhi.
     * @param rhi Interface de rendu, non possédée ; doit survivre à l'objet.
     */
    explicit SpriteBatch(QRhi* rhi);

    ~SpriteBatch();

    SpriteBatch(const SpriteBatch&) = delete;
    SpriteBatch& operator=(const SpriteBatch&) = delete;

    /// @return L'interface de rendu servie par cette instance (comparaison de perte de contexte).
    [[nodiscard]] QRhi* rhi() const noexcept {
        return _rhi;
    }

    /// Ouvre l'enregistrement d'une image : vide les sommets et les lots de la précédente.
    void beginFrame();

    /**
     * @brief Démarre un lot : fixe la texture échantillonnée et la projection.
     * @param projection Matrice de projection monde → clip (fournie par la caméra).
     * @param texture    Texture à échantillonner (identité opaque, `hmi::TextureHandle`), non
     *                   possédée ; `nullptr` rend le lot muet.
     */
    void begin(const DirectX::XMFLOAT4X4& projection, TextureHandle texture);

    /**
     * @brief Ajoute un quad au lot courant.
     * @param quad    Quad à dessiner (unités monde, UV normalisées, teinte).
     * @param topDepth    Profondeur de ses deux sommets du haut (avant rotation).
     * @param bottomDepth Profondeur de ses deux sommets du bas.
     */
    void draw(const SpriteQuad& quad, float topDepth = 0.0F, float bottomDepth = 0.0F);

    /**
     * @brief Ajoute un segment épais (orienté librement) au lot courant.
     * @param line Segment à dessiner (unités monde, UV normalisées, teinte). Sans effet si les
     *             deux extrémités coïncident (segment dégénéré).
     * @param startDepth Profondeur de l'extrémité `a`.
     * @param endDepth   Profondeur de l'extrémité `b`.
     */
    void draw(const LineQuad& line, float startDepth = 0.0F, float endDepth = 0.0F);

    /**
     * @brief Ajoute un quadrilatère à quatre sommets libres au lot courant (`LOT-128`).
     * @param poly Quadrilatère à dessiner (unités monde, UV normalisées, teinte). Ses sommets sont
     *             repris tels quels : c'est à la composition de les donner dans l'ordre du
     *             pourtour.
     * @param depths La profondeur de chacun de ses quatre sommets.
     */
    void draw(const PolyQuad& poly, const std::array<float, 4>& depths = {});

    /// Termine le lot : fige la plage de quads enregistrée. Un lot vide n'émettra aucun dessin.
    void end();

    /**
     * @brief Téléverse l'image enregistrée et l'émet en une passe de rendu.
     *
     * À appeler **une fois par image**, hors de toute passe ouverte : la méthode ouvre et referme
     * la sienne. Le fond est effacé à @p clear, y compris si aucun lot n'a été enregistré — sans
     * quoi une image sans contenu afficherait le résidu de la précédente.
     * @param commandBuffer Tampon de commandes de l'image (fourni par `QRhiWidget::render`).
     * @param target        Cible de rendu de l'image.
     * @param updates       Lot de mises à jour à soumettre avec les sommets (téléversements de
     *                      textures accumulés pendant la composition) ; peut être `nullptr`.
     * @param clear         Couleur d'effacement : quatre composantes RGBA `[0, 1]`.
     */
    void submit(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target,
                QRhiResourceUpdateBatch* updates, const float* clear);

    /**
     * @brief Les quads de l'image testent-ils la profondeur (`LOT-1003`) ? Faux par défaut : le
     *        pipeline d'avant le lot. À régler avant `prepare` ; la cible doit alors avoir un
     *        tampon de profondeur.
     */
    void setDepthTest(bool enabled) noexcept {
        _depthTest = enabled;
    }

    /**
     * @brief Dépose dans un lot les téléversements de l'image enregistrée — sommets, projections,
     *        et le tampon d'indices à sa première image —, **hors** de toute passe.
     *
     * @param target  Cible de rendu de l'image : le pipeline se crée pour sa passe.
     * @param updates Lot à compléter ; `nullptr` : un lot est pris au besoin.
     * @return Le lot à soumettre à l'ouverture de la passe (`beginPass`), `nullptr` s'il n'y a
     *         rien à téléverser.
     */
    [[nodiscard]] QRhiResourceUpdateBatch* prepare(QRhiRenderTarget* target,
                                                   QRhiResourceUpdateBatch* updates);

    /**
     * @brief Émet les appels de dessin de l'image préparée, dans la passe **ouverte** par
     *        l'appelant. Sans effet si `prepare` n'a rien trouvé à dessiner.
     */
    void record(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target);

private:
    /// Sommet envoyé au GPU : position de la vue (x, y, profondeur), UV, couleur RVBA.
    struct Vertex {
        float x;
        float y;
        float z;
        float u;
        float v;
        float r;
        float g;
        float b;
        float a;
    };

    /// Un lot enregistré : une texture, une projection, une plage contiguë de quads.
    struct Batch {
        QRhiTexture* texture = nullptr;
        /// Décalage du bloc de projection de ce lot dans le tampon uniforme, en octets.
        int uniformOffset = 0;
        /// Rang du premier quad du lot dans le tampon de sommets.
        std::size_t firstQuad = 0;
        /// Nombre de quads du lot.
        std::size_t quadCount = 0;
    };

    /// Nombre de quads couverts par le tampon d'indices, et donc par un seul appel de dessin.
    static constexpr std::size_t MAXIMUM_QUADS = 65536 / 4;

    /// Crée (ou recrée) les pipelines pour la passe de rendu donnée. Idempotent.
    bool ensurePipeline(QRhiRenderTarget* target);
    /// Construit un pipeline pour @p pass, avec ou sans test de profondeur.
    [[nodiscard]] std::unique_ptr<QRhiGraphicsPipeline> createPipeline(
        QRhiRenderPassDescriptor* pass, bool depthTest);
    /// Redimensionne le tampon de sommets si l'image enregistrée n'y tient pas.
    bool ensureVertexCapacity(std::size_t quadCount);
    /// Redimensionne le tampon uniforme pour @p batchCount emplacements de projection.
    bool ensureUniformCapacity(std::size_t batchCount);
    /// @return Les liaisons de ressources associées à @p texture, créées à la première rencontre.
    QRhiShaderResourceBindings* bindingsFor(QRhiTexture* texture);
    /// Ferme le lot en cours d'enregistrement, s'il y en a un.
    void closeBatch();

    QRhi* _rhi;  // non possédé
    std::unique_ptr<QRhiBuffer> _vertexBuffer;
    std::unique_ptr<QRhiBuffer> _indexBuffer;
    std::unique_ptr<QRhiBuffer> _uniformBuffer;
    /// Au plus proche, sans mipmap : les images engendrées (`TextureFiltering::Sharp`).
    std::unique_ptr<QRhiSampler> _sharpSampler;
    /// Bilinéaire avec mipmaps : l'art peint (`TextureFiltering::Smooth`).
    std::unique_ptr<QRhiSampler> _smoothSampler;
    std::unique_ptr<QRhiGraphicsPipeline> _pipeline;
    /// Le même pipeline, qui teste la profondeur sans l'écrire (`setDepthTest`) ; créé à la
    /// première image qui le demande.
    std::unique_ptr<QRhiGraphicsPipeline> _depthPipeline;
    bool _depthTest = false;
    /// L'image préparée a de quoi dessiner (`prepare`, lu par `record`).
    bool _drawable = false;
    /// Descripteur de la passe pour laquelle `_pipeline` a été construit : un changement de cible
    /// (redimensionnement du widget, changement de fenêtre) impose de le reconstruire.
    QRhiRenderPassDescriptor* _pipelinePass = nullptr;
    /// Liaisons par texture, gardées d'une image à l'autre : les recréer à chaque lot allouerait
    /// des ressources GPU des centaines de fois par seconde.
    std::unordered_map<QRhiTexture*, std::unique_ptr<QRhiShaderResourceBindings>> _bindings;
    /// Liaisons de référence, construites une fois : QRhi exige que toutes les liaisons employées
    /// par un pipeline soient *layout-compatibles* avec celles fournies à sa création.
    std::unique_ptr<QRhiShaderResourceBindings> _layoutBindings;
    /// Téléversement du tampon d'indices, en attente de la première image : un tampon immuable se
    /// remplit une seule fois, et son lot ne peut être soumis qu'avec une image.
    QRhiResourceUpdateBatch* _pendingIndexUpload = nullptr;
    /// Capacité du tampon uniforme, en nombre d'emplacements de projection.
    std::size_t _uniformSlots = 0;
    /// Taille alignée d'un bloc de projection, imposée par le matériel.
    int _uniformStride = 0;

    std::vector<Vertex> _vertices;
    std::vector<Batch> _batches;
    /// Lot en cours d'enregistrement (entre `begin()` et `end()`), s'il y en a un.
    Batch _current{};
    bool _recording = false;
    /// Projections des lots, dans l'ordre d'enregistrement (téléversées d'un bloc au `submit`).
    std::vector<DirectX::XMFLOAT4X4> _projections;
};

}  // namespace hmi
