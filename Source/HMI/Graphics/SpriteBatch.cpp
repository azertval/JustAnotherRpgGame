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

namespace hmi {

namespace {

// Charge un shader precompile (.qsb) depuis les ressources de l'executable. Un shader absent est
// une erreur de BUILD, pas un etat d'execution recuperable : la ressource est embarquee par
// qt6_add_shaders, elle ne peut manquer que si la compilation n'a pas eu lieu.
QShader loadShader(const char* resourcePath) {
    QFile file(QString::fromLatin1(resourcePath));
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error(std::string("Shader introuvable dans les ressources : ") +
                                 resourcePath);
    }
    const QShader shader = QShader::fromSerialized(file.readAll());
    if (!shader.isValid()) {
        throw std::runtime_error(std::string("Shader illisible : ") + resourcePath);
    }
    return shader;
}

// Taille d'un bloc de projection : une matrice 4x4 de flottants.
constexpr int PROJECTION_BYTES = 16 * static_cast<int>(sizeof(float));

// Convertit la matrice ligne-major de DirectXMath (convention `position * matrice`) en QMatrix4x4
// (convention `matrice * position`, comme GLSL) : c'est exactement sa transposee. La correction
// d'espace de clip de QRhi est appliquee par-dessus -- le shader ecrit en convention OpenGL, QRhi
// la ramene a celle du backend (Direct3D 11 sous Windows).
QMatrix4x4 toClipMatrix(QRhi* rhi, const DirectX::XMFLOAT4X4& projection) {
    const QMatrix4x4 columnMajor(
        projection(0, 0), projection(1, 0), projection(2, 0), projection(3, 0),  //
        projection(0, 1), projection(1, 1), projection(2, 1), projection(3, 1),  //
        projection(0, 2), projection(1, 2), projection(2, 2), projection(3, 2),  //
        projection(0, 3), projection(1, 3), projection(2, 3), projection(3, 3));
    return rhi->clipSpaceCorrMatrix() * columnMajor;
}

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

// Cree (ou recree) le pipeline pour la passe de rendu donnee.
bool SpriteBatch::ensurePipeline(QRhiRenderTarget* target) {
    QRhiRenderPassDescriptor* const pass = target->renderPassDescriptor();
    if (_pipeline && _pipelinePass == pass) {
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

    auto pipeline = std::unique_ptr<QRhiGraphicsPipeline>(_rhi->newGraphicsPipeline());
    pipeline->setShaderStages({
        {QRhiShaderStage::Vertex, loadShader(":/shaders/sprite.vert.qsb")},
        {QRhiShaderStage::Fragment, loadShader(":/shaders/sprite.frag.qsb")},
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({{static_cast<quint32>(sizeof(Vertex))}});
    inputLayout.setAttributes({
        {0, 0, QRhiVertexInputAttribute::Float2, 0},
        {0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float)},
        {0, 2, QRhiVertexInputAttribute::Float4, 4 * sizeof(float)},
    });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(_layoutBindings.get());
    pipeline->setRenderPassDescriptor(pass);
    pipeline->setTopology(QRhiGraphicsPipeline::Triangles);
    // Rendu 2D : les quads peuvent etre vus des deux cotes (un segment oriente peut « retourner »
    // son quadrilatere), et il n'y a ni profondeur ni pochoir a ecrire.
    pipeline->setCullMode(QRhiGraphicsPipeline::None);
    pipeline->setDepthTest(false);
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
        return false;
    }
    _pipeline = std::move(pipeline);
    _pipelinePass = pass;
    return true;
}

// Ouvre l'enregistrement d'une image.
void SpriteBatch::beginFrame() {
    _vertices.clear();
    _batches.clear();
    _projections.clear();
    _recording = false;
}

// Demarre un lot : fixe la texture echantillonnee et la projection.
void SpriteBatch::begin(const DirectX::XMFLOAT4X4& projection, TextureHandle texture) {
    closeBatch();
    _current = Batch{};
    _current.texture = static_cast<QRhiTexture*>(texture);
    _current.firstQuad = _vertices.size() / 4;
    _current.uniformOffset = static_cast<int>(_projections.size()) * _uniformStride;
    _projections.push_back(projection);
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
void SpriteBatch::draw(const SpriteQuad& quad) {
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
    for (std::size_t i = 0; i < 4; ++i) {
        const float x = centerX + (offsetsX[i] * cosR) - (offsetsY[i] * sinR);
        const float y = centerY + (offsetsX[i] * sinR) + (offsetsY[i] * cosR);
        _vertices.push_back(Vertex{.x = x,
                                   .y = y,
                                   .u = us[i],
                                   .v = vs[i],
                                   .r = quad.r,
                                   .g = quad.g,
                                   .b = quad.b,
                                   .a = quad.a});
    }
}

// Ajoute un segment epais (oriente librement) au lot courant.
void SpriteBatch::draw(const LineQuad& line) {
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
                               .u = line.u0,
                               .v = line.v0,
                               .r = line.r,
                               .g = line.g,
                               .b = line.b,
                               .a = line.a});
    _vertices.push_back(Vertex{.x = line.bx + nx,
                               .y = line.by + ny,
                               .u = line.u1,
                               .v = line.v0,
                               .r = line.r,
                               .g = line.g,
                               .b = line.b,
                               .a = line.a});
    _vertices.push_back(Vertex{.x = line.bx - nx,
                               .y = line.by - ny,
                               .u = line.u1,
                               .v = line.v1,
                               .r = line.r,
                               .g = line.g,
                               .b = line.b,
                               .a = line.a});
    _vertices.push_back(Vertex{.x = line.ax - nx,
                               .y = line.ay - ny,
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
void SpriteBatch::draw(const PolyQuad& poly) {
    const std::array<float, 4> us = {poly.u0, poly.u1, poly.u1, poly.u0};
    const std::array<float, 4> vs = {poly.v0, poly.v0, poly.v1, poly.v1};
    for (std::size_t i = 0; i < 4; ++i) {
        _vertices.push_back(Vertex{.x = poly.x[i],
                                   .y = poly.y[i],
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
    closeBatch();

    const QColor clearColor = QColor::fromRgbF(clear[0], clear[1], clear[2], clear[3]);
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

    const bool drawable = quadCount > 0 && ensureVertexCapacity(quadCount) &&
                          ensureUniformCapacity(_projections.size()) && ensurePipeline(target);
    if (drawable) {
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

    commandBuffer->beginPass(target, clearColor, {1.0F, 0}, updates);
    if (drawable) {
        const QSize pixelSize = target->pixelSize();
        commandBuffer->setGraphicsPipeline(_pipeline.get());
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
            const auto vertexOffset = static_cast<quint32>(batch.firstQuad * 4 * sizeof(Vertex));
            const QRhiCommandBuffer::VertexInput vertexInput(_vertexBuffer.get(), vertexOffset);
            commandBuffer->setVertexInput(0, 1, &vertexInput, _indexBuffer.get(), 0,
                                          QRhiCommandBuffer::IndexUInt16);
            // Un appel de dessin ne peut couvrir plus de quads que n'en indexe le tampon
            // d'indices (16 bits) : un lot plus gros est tronque plutot que de lire hors bornes.
            const std::size_t drawnQuads = (std::min)(batch.quadCount, MAXIMUM_QUADS);
            commandBuffer->drawIndexed(static_cast<quint32>(drawnQuads * 6));
        }
    }
    commandBuffer->endPass();
}

}  // namespace hmi
