// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/SpriteBatch.h"

#include <QColor>
#include <QFile>
#include <QMatrix4x4>
#include <QSize>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

#include <rhi/qrhi.h>

#include "HMI/Graphics/GraphicsLog.h"
#include "HMI/Graphics/RhiShaders.h"

namespace hmi {

namespace {

// Deux matrices sont identiques quand leurs seize flottants le sont (comparaison membre a membre).
[[nodiscard]] bool sameMatrix(const DirectX::XMFLOAT4X4& a, const DirectX::XMFLOAT4X4& b) noexcept {
    for (std::size_t row = 0; row < 4; ++row) {
        for (std::size_t column = 0; column < 4; ++column) {
            if (a(row, column) != b(row, column)) {
                return false;
            }
        }
    }
    return true;
}

// Taille d'un bloc de projection : une matrice 4x4 de flottants.
constexpr int PROJECTION_BYTES = 16 * static_cast<int>(sizeof(float));

}  // namespace

// Construit le pipeline (shaders, tampons, echantillonneur) sur une interface QRhi.
SpriteBatch::SpriteBatch(QRhi* rhi) : _rhi(rhi) {
    if (_rhi == nullptr) {
        throw std::runtime_error("SpriteBatch : QRhi nul");
    }
    _vertices.reserve(MAXIMUM_QUADS * 4);

    // Tampon d'indices immuable : deux triangles par quad (0,1,2, 0,2,3). Indices 16 bits, d'ou le
    // plafond de MAXIMUM_QUADS quads par appel de dessin.
    std::vector<std::uint16_t> indices(MAXIMUM_QUADS * 6);
    for (std::size_t quad = 0; quad < MAXIMUM_QUADS; ++quad) {
        const auto base = static_cast<std::uint16_t>(quad * 4);
        const std::size_t offset = quad * 6;
        indices[offset + 0] = static_cast<std::uint16_t>(base + 0);
        indices[offset + 1] = static_cast<std::uint16_t>(base + 1);
        indices[offset + 2] = static_cast<std::uint16_t>(base + 2);
        indices[offset + 3] = static_cast<std::uint16_t>(base + 0);
        indices[offset + 4] = static_cast<std::uint16_t>(base + 2);
        indices[offset + 5] = static_cast<std::uint16_t>(base + 3);
    }
    _indexBuffer.reset(
        _rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer,
                        static_cast<quint32>(indices.size() * sizeof(std::uint16_t))));
    if (!_indexBuffer->create()) {
        throw std::runtime_error("SpriteBatch : echec de creation du tampon d'indices");
    }
    // Le contenu d'un tampon immuable se televerse une fois, dans son propre lot soumis a part :
    // ce tampon vit plus longtemps qu'une image, il ne dépend d'aucune passe.
    QRhiResourceUpdateBatch* const initial = _rhi->nextResourceUpdateBatch();
    initial->uploadStaticBuffer(_indexBuffer.get(), indices.data());
    _pendingIndexUpload = initial;

    // Deux echantillonneurs, selon la nature de l'image (EX-ARCH-022), bords fixes l'un et l'autre.
    // Au plus proche, sans mipmap : une image engendree dont chaque pixel est voulu (damier, aplat,
    // marqueur). Bilineaire entre les niveaux de mipmaps : l'art peint, toujours reduit a l'ecran,
    // qui scintillerait au plus proche (EX-VIS-008, LOT-103).
    _sharpSampler.reset(_rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest,
                                         QRhiSampler::None, QRhiSampler::ClampToEdge,
                                         QRhiSampler::ClampToEdge));
    _smoothSampler.reset(_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear,
                                          QRhiSampler::Linear, QRhiSampler::ClampToEdge,
                                          QRhiSampler::ClampToEdge));
    if (!_sharpSampler->create() || !_smoothSampler->create()) {
        throw std::runtime_error("SpriteBatch : echec de creation des echantillonneurs");
    }

    _uniformStride = _rhi->ubufAligned(PROJECTION_BYTES);
    if (!ensureUniformCapacity(64)) {
        throw std::runtime_error("SpriteBatch : echec de creation du tampon uniforme");
    }
    if (!ensureVertexCapacity(2048)) {
        throw std::runtime_error("SpriteBatch : echec de creation du tampon de sommets");
    }

    GRAPHICS_LOG_TRACE("SpriteBatch : pipeline 2D cree sur QRhi (" +
                       std::string(_rhi->backendName()) + ")");
}

SpriteBatch::~SpriteBatch() = default;

// Redimensionne le tampon uniforme pour `batchCount` emplacements de projection.
bool SpriteBatch::ensureUniformCapacity(std::size_t batchCount) {
    if (batchCount <= _uniformSlots && _uniformBuffer) {
        return true;
    }
    const std::size_t slotCount = batchCount * 2;  // marge : evite de recreer a chaque lot ajoute
    auto buffer = std::unique_ptr<QRhiBuffer>(_rhi->newBuffer(
        QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
        static_cast<quint32>(slotCount * static_cast<std::size_t>(_uniformStride))));
    if (!buffer->create()) {
        return false;
    }
    _uniformBuffer = std::move(buffer);
    _uniformSlots = slotCount;
    // Les liaisons referencent le tampon : elles deviennent caduques avec lui.
    _bindings.clear();
    _layoutBindings.reset();
    return true;
}

// Redimensionne le tampon de sommets si l'image enregistree n'y tient pas.
bool SpriteBatch::ensureVertexCapacity(std::size_t quadCount) {
    const auto needed = static_cast<quint32>(quadCount * 4 * sizeof(Vertex));
    if (_vertexBuffer && _vertexBuffer->size() >= needed) {
        return true;
    }
    auto buffer = std::unique_ptr<QRhiBuffer>(
        _rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, needed * 2));
    if (!buffer->create()) {
        return false;
    }
    _vertexBuffer = std::move(buffer);
    return true;
}

// Liaisons de ressources associees a une texture, creees a la premiere rencontre.
QRhiShaderResourceBindings* SpriteBatch::bindingsFor(QRhiTexture* texture) {
    const auto found = _bindings.find(texture);
    if (found != _bindings.end()) {
        return found->second.get();
    }
    // La texture dit sa nature : `hmi::createTexture` ne donne une chaine de mipmaps qu'a l'art
    // peint (`TextureFiltering::Smooth`).
    QRhiSampler* const sampler = texture->flags().testFlag(QRhiTexture::MipMapped)
                                     ? _smoothSampler.get()
                                     : _sharpSampler.get();
    auto bindings = std::unique_ptr<QRhiShaderResourceBindings>(_rhi->newShaderResourceBindings());
    bindings->setBindings({
        QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
            0, QRhiShaderResourceBinding::VertexStage, _uniformBuffer.get(), PROJECTION_BYTES),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                  texture, sampler),
    });
    if (!bindings->create()) {
        GRAPHICS_LOG_WARNING("SpriteBatch : echec de creation des liaisons de ressources");
        return nullptr;
    }
    QRhiShaderResourceBindings* const raw = bindings.get();
    _bindings.emplace(texture, std::move(bindings));
    return raw;
}

// Cree (ou recree) les pipelines pour la passe de rendu donnee : celui de toujours, et celui qui
// teste la profondeur si l'image le demande.
bool SpriteBatch::ensurePipeline(QRhiRenderTarget* target) {
    QRhiRenderPassDescriptor* const pass = target->renderPassDescriptor();
    // Les pipelines suivent aussi le nombre d'echantillons de la cible : une cible
    // multi-echantillonnee refuse un pipeline qui ne l'est pas autant qu'elle.
    const int samples = target->sampleCount();
    if (_pipelinePass != pass || _pipelineSamples != samples) {
        _pipeline.reset();
        _depthPipeline.reset();
    }
    if (_pipeline && (!_depthTest || _depthPipeline)) {
        return true;
    }

    // Liaisons de reference : jamais utilisees pour dessiner, seulement pour decrire la
    // disposition attendue par le pipeline (QRhi n'exige que la compatibilite de disposition).
    if (!_layoutBindings) {
        _layoutBindings.reset(_rhi->newShaderResourceBindings());
        _layoutBindings->setBindings({
            QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
                0, QRhiShaderResourceBinding::VertexStage, _uniformBuffer.get(), PROJECTION_BYTES),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                      nullptr, _sharpSampler.get()),
        });
        if (!_layoutBindings->create()) {
            GRAPHICS_LOG_WARNING("SpriteBatch : echec de creation des liaisons de reference");
            return false;
        }
    }

    if (!_pipeline) {
        _pipeline = createPipeline(pass, samples, false);
    }
    if (_depthTest && !_depthPipeline) {
        _depthPipeline = createPipeline(pass, samples, true);
    }
    _pipelinePass = pass;
    _pipelineSamples = samples;
    return _pipeline && (!_depthTest || _depthPipeline);
}

std::unique_ptr<QRhiGraphicsPipeline> SpriteBatch::createPipeline(QRhiRenderPassDescriptor* pass,
                                                                  int samples, bool depthTest) {
    auto pipeline = std::unique_ptr<QRhiGraphicsPipeline>(_rhi->newGraphicsPipeline());
    pipeline->setShaderStages({
        {QRhiShaderStage::Vertex, loadShader(":/shaders/sprite.vert.qsb")},
        {QRhiShaderStage::Fragment, loadShader(":/shaders/sprite.frag.qsb")},
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({{static_cast<quint32>(sizeof(Vertex))}});
    inputLayout.setAttributes({
        {0, 0, QRhiVertexInputAttribute::Float3, 0},
        {0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float)},
        {0, 2, QRhiVertexInputAttribute::Float4, 5 * sizeof(float)},
    });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(_layoutBindings.get());
    pipeline->setRenderPassDescriptor(pass);
    pipeline->setSampleCount(samples);
    pipeline->setTopology(QRhiGraphicsPipeline::Triangles);
    // Rendu 2D : les quads peuvent etre vus des deux cotes (un segment oriente peut « retourner »
    // son quadrilatere), et il n'y a ni profondeur ni pochoir a ecrire. Quand l'image a des
    // maillages (LOT-1003), les quads TESTENT la profondeur qu'ils ont ecrite, sans l'ecrire :
    // leurs bords sont adoucis, et entre eux l'ordre du peintre decide toujours.
    pipeline->setCullMode(QRhiGraphicsPipeline::None);
    pipeline->setDepthTest(depthTest);
    pipeline->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
    pipeline->setDepthWrite(false);

    // Fusion alpha PREMULTIPLIE (EX-VIS-008) : toute texture l'est au televersement, et le shader
    // premultiplie la teinte. Un bord adouci et ses mipmaps se melangent alors sans frange sombre.
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::One;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.opColor = QRhiGraphicsPipeline::Add;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.opAlpha = QRhiGraphicsPipeline::Add;
    pipeline->setTargetBlends({blend});

    if (!pipeline->create()) {
        GRAPHICS_LOG_WARNING("SpriteBatch : echec de creation du pipeline graphique");
        return nullptr;
    }
    return pipeline;
}

// Ouvre l'enregistrement d'une image.
void SpriteBatch::beginFrame() {
    _vertices.clear();
    _batches.clear();
    _projections.clear();
    _recording = false;
    _drawable = false;
}

// Demarre un lot : fixe la texture echantillonnee et la projection.
void SpriteBatch::begin(const DirectX::XMFLOAT4X4& projection, TextureHandle texture) {
    closeBatch();
    _current = Batch{};
    _current.texture = static_cast<QRhiTexture*>(texture);
    _current.firstQuad = _vertices.size() / 4;
    // Une projection par image, en pratique : une passe ne pousse la sienne que si elle differe de
    // la precedente. Des centaines de passes (une par texture de la bande de profondeur) ne
    // televersent plus chacune la meme matrice (audit de l'affichage, A7).
    if (_projections.empty() || !sameMatrix(_projections.back(), projection)) {
        _projections.push_back(projection);
    }
    _current.uniformOffset = static_cast<int>(_projections.size() - 1) * _uniformStride;
    _recording = true;
}

// Ferme le lot en cours d'enregistrement, s'il y en a un.
void SpriteBatch::closeBatch() {
    if (!_recording) {
        return;
    }
    _current.quadCount = (_vertices.size() / 4) - _current.firstQuad;
    if (_current.quadCount > 0 && _current.texture != nullptr) {
        _batches.push_back(_current);
    }
    _recording = false;
}

// Ajoute un quad au lot courant.
void SpriteBatch::draw(const SpriteQuad& quad, float topDepth, float bottomDepth) {
    const float halfWidth = quad.width * 0.5F;
    const float halfHeight = quad.height * 0.5F;
    const float centerX = quad.x + halfWidth;
    const float centerY = quad.y + halfHeight;
    const float cosR = std::cos(quad.rotation);
    const float sinR = std::sin(quad.rotation);

    // Coins relatifs au centre (haut-gauche, haut-droit, bas-droit, bas-gauche), tournes de
    // `rotation` radians autour du centre -- a rotation nulle (cosR=1, sinR=0), coincide avec le
    // rectangle aligne d'origine (meme formule que draw(LineQuad), coins pousses dans le meme
    // ordre attendu par le tampon d'indices).
    const std::array<float, 4> offsetsX = {-halfWidth, halfWidth, halfWidth, -halfWidth};
    const std::array<float, 4> offsetsY = {-halfHeight, -halfHeight, halfHeight, halfHeight};
    const std::array<float, 4> us = {quad.u0, quad.u1, quad.u1, quad.u0};
    const std::array<float, 4> vs = {quad.v0, quad.v0, quad.v1, quad.v1};
    const std::array<float, 4> depths = {topDepth, topDepth, bottomDepth, bottomDepth};
    for (std::size_t i = 0; i < 4; ++i) {
        const float x = centerX + (offsetsX[i] * cosR) - (offsetsY[i] * sinR);
        const float y = centerY + (offsetsX[i] * sinR) + (offsetsY[i] * cosR);
        _vertices.push_back(Vertex{.x = x,
                                   .y = y,
                                   .z = depths[i],
                                   .u = us[i],
                                   .v = vs[i],
                                   .r = quad.r,
                                   .g = quad.g,
                                   .b = quad.b,
                                   .a = quad.a});
    }
}

// Ajoute un segment epais (oriente librement) au lot courant.
void SpriteBatch::draw(const LineQuad& line, float startDepth, float endDepth) {
    const float dx = line.bx - line.ax;
    const float dy = line.by - line.ay;
    const float length = std::sqrt((dx * dx) + (dy * dy));
    if (length < 1e-6F) {
        return;  // segment degenere : rien a dessiner.
    }

    // Decalage perpendiculaire (normale unitaire x demi-epaisseur), de part et d'autre du segment.
    const float nx = -dy / length * (line.thickness * 0.5F);
    const float ny = dx / length * (line.thickness * 0.5F);

    // Quatre coins, meme ordre que draw(SpriteQuad) (le tampon d'indices attend un quadrilatere
    // convexe coherent, peu importe son orientation) : a+n, b+n, b-n, a-n.
    _vertices.push_back(Vertex{.x = line.ax + nx,
                               .y = line.ay + ny,
                               .z = startDepth,
                               .u = line.u0,
                               .v = line.v0,
                               .r = line.r,
                               .g = line.g,
                               .b = line.b,
                               .a = line.a});
    _vertices.push_back(Vertex{.x = line.bx + nx,
                               .y = line.by + ny,
                               .z = endDepth,
                               .u = line.u1,
                               .v = line.v0,
                               .r = line.r,
                               .g = line.g,
                               .b = line.b,
                               .a = line.a});
    _vertices.push_back(Vertex{.x = line.bx - nx,
                               .y = line.by - ny,
                               .z = endDepth,
                               .u = line.u1,
                               .v = line.v1,
                               .r = line.r,
                               .g = line.g,
                               .b = line.b,
                               .a = line.a});
    _vertices.push_back(Vertex{.x = line.ax - nx,
                               .y = line.ay - ny,
                               .z = startDepth,
                               .u = line.u0,
                               .v = line.v1,
                               .r = line.r,
                               .g = line.g,
                               .b = line.b,
                               .a = line.a});
}

// Ajoute un quadrilatere a quatre sommets libres au lot courant (LOT-128, decision D1).
//
// Rien de particulier a calculer : les quatre sommets sont deja la, et le tampon d'indices ne
// demande qu'un quadrilatere convexe donne dans l'ordre de son pourtour -- exactement ce que
// draw(LineQuad) lui fournit deja pour un segment oriente. Les UV suivent le meme tour que les
// coins d'un SpriteQuad (u0v0, u1v0, u1v1, u0v1) : avec l'aplat blanc, elles ne servent qu'a
// rester coherentes avec les deux autres primitives.
void SpriteBatch::draw(const PolyQuad& poly, const std::array<float, 4>& depths) {
    const std::array<float, 4> us = {poly.u0, poly.u1, poly.u1, poly.u0};
    const std::array<float, 4> vs = {poly.v0, poly.v0, poly.v1, poly.v1};
    for (std::size_t i = 0; i < 4; ++i) {
        _vertices.push_back(Vertex{.x = poly.x[i],
                                   .y = poly.y[i],
                                   .z = depths[i],
                                   .u = us[i],
                                   .v = vs[i],
                                   .r = poly.r,
                                   .g = poly.g,
                                   .b = poly.b,
                                   .a = poly.a});
    }
}

// Termine le lot : fige la plage de quads enregistree.
void SpriteBatch::end() {
    closeBatch();
}

// Televerse l'image enregistree et l'emet en une passe de rendu.
void SpriteBatch::submit(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target,
                         QRhiResourceUpdateBatch* updates, const float* clear) {
    updates = prepare(target, updates);
    const QColor clearColor = QColor::fromRgbF(clear[0], clear[1], clear[2], clear[3]);
    commandBuffer->beginPass(target, clearColor, {1.0F, 0}, updates);
    record(commandBuffer, target);
    commandBuffer->endPass();
}

// Depose les televersements de l'image enregistree dans un lot, hors de toute passe.
QRhiResourceUpdateBatch* SpriteBatch::prepare(QRhiRenderTarget* target,
                                              QRhiResourceUpdateBatch* updates) {
    closeBatch();

    const std::size_t quadCount = _vertices.size() / 4;

    // Le lot de televersement du tampon d'indices n'a pas encore ete soumis (premiere image) :
    // il doit l'etre avant tout dessin qui s'en sert.
    if (_pendingIndexUpload != nullptr) {
        if (updates != nullptr) {
            updates->merge(_pendingIndexUpload);
        } else {
            updates = _pendingIndexUpload;
        }
        _pendingIndexUpload = nullptr;
    }

    _drawable = quadCount > 0 && ensureVertexCapacity(quadCount) &&
                ensureUniformCapacity(_projections.size()) && ensurePipeline(target);
    if (_drawable) {
        if (updates == nullptr) {
            updates = _rhi->nextResourceUpdateBatch();
        }
        updates->updateDynamicBuffer(_vertexBuffer.get(), 0,
                                     static_cast<quint32>(_vertices.size() * sizeof(Vertex)),
                                     _vertices.data());
        for (std::size_t index = 0; index < _projections.size(); ++index) {
            const QMatrix4x4 clipMatrix = toClipMatrix(_rhi, _projections[index]);
            updates->updateDynamicBuffer(
                _uniformBuffer.get(),
                static_cast<quint32>(index * static_cast<std::size_t>(_uniformStride)),
                PROJECTION_BYTES, clipMatrix.constData());
        }
    }

    return updates;
}

// Emet les appels de dessin de l'image preparee, dans la passe ouverte par l'appelant.
void SpriteBatch::record(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target) {
    if (_drawable) {
        const QSize pixelSize = target->pixelSize();
        commandBuffer->setGraphicsPipeline(_depthTest ? _depthPipeline.get() : _pipeline.get());
        commandBuffer->setViewport({0.0F, 0.0F, static_cast<float>(pixelSize.width()),
                                    static_cast<float>(pixelSize.height())});
        for (const Batch& batch : _batches) {
            QRhiShaderResourceBindings* const bindings = bindingsFor(batch.texture);
            if (bindings == nullptr) {
                continue;
            }
            const QRhiCommandBuffer::DynamicOffset offset{
                0, static_cast<quint32>(batch.uniformOffset)};
            commandBuffer->setShaderResources(bindings, 1, &offset);
            // Un appel de dessin ne couvre pas plus de quads que n'en indexe le tampon d'indices
            // (16 bits) : un lot plus gros se dessine en plusieurs appels, chacun decale dans le
            // tampon de sommets. Il etait tronque en silence (audit de l'affichage, A8).
            for (std::size_t first = 0; first < batch.quadCount; first += MAXIMUM_QUADS) {
                const std::size_t drawnQuads = (std::min)(batch.quadCount - first, MAXIMUM_QUADS);
                const auto vertexOffset =
                    static_cast<quint32>((batch.firstQuad + first) * 4 * sizeof(Vertex));
                const QRhiCommandBuffer::VertexInput vertexInput(_vertexBuffer.get(), vertexOffset);
                commandBuffer->setVertexInput(0, 1, &vertexInput, _indexBuffer.get(), 0,
                                              QRhiCommandBuffer::IndexUInt16);
                commandBuffer->drawIndexed(static_cast<quint32>(drawnQuads * 6));
            }
        }
    }
}

}  // namespace hmi
