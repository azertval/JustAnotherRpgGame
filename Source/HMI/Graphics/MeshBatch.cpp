// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/MeshBatch.h"

#include <QImage>
#include <QSize>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>

#include <rhi/qrhi.h>

#include "HMI/Graphics/GraphicsLog.h"
#include "HMI/Graphics/RhiContext.h"
#include "HMI/Graphics/RhiShaders.h"

namespace hmi {

namespace {

// Le bloc uniforme d'un dessin (std140) : la matrice maillage -> clip, puis la teinte RVBA.
constexpr int MATRIX_BYTES = 16 * static_cast<int>(sizeof(float));
constexpr int TINT_BYTES = 4 * static_cast<int>(sizeof(float));
constexpr int DRAW_BYTES = MATRIX_BYTES + TINT_BYTES;

// Le sommet televerse est celui du chargeur, tel quel : position, normale, coordonnees de texture.
static_assert(sizeof(core::MeshVertex) == 8 * sizeof(float),
              "core::MeshVertex doit rester huit flottants contigus : il est televerse tel quel");

// Ce que pese une texture `RGBA8` lissee : ses pixels, plus un tiers pour sa chaine de mipmaps.
[[nodiscard]] std::size_t textureWeight(int width, int height) noexcept {
    const std::size_t bytes =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
    return bytes + (bytes / 3U);
}

// L'image de la couleur de base, decodee en RGBA a alpha droit ; rien si elle ne se decode pas.
[[nodiscard]] std::optional<DecodedImage> decodeBaseColor(const core::MeshData& mesh) {
    if (mesh.image.empty()) {
        return std::nullopt;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): QImage lit des `uchar`.
    const QImage source = QImage::fromData(reinterpret_cast<const uchar*>(mesh.image.data()),
                                           static_cast<int>(mesh.image.size()));
    if (source.isNull()) {
        return std::nullopt;
    }
    const QImage image = source.convertToFormat(QImage::Format_RGBA8888);
    DecodedImage decoded;
    decoded.width = image.width();
    decoded.height = image.height();
    decoded.pixels.resize(static_cast<std::size_t>(decoded.width) *
                          static_cast<std::size_t>(decoded.height));
    const std::size_t rowBytes = static_cast<std::size_t>(decoded.width) * sizeof(std::uint32_t);
    for (int row = 0; row < decoded.height; ++row) {
        std::memcpy(decoded.pixels.data() + (static_cast<std::size_t>(row) * decoded.width),
                    image.constScanLine(row), rowBytes);
    }
    return decoded;
}

}  // namespace

MeshBatch::MeshBatch(QRhi* rhi) : _rhi(rhi) {
    if (_rhi == nullptr) {
        throw std::runtime_error("MeshBatch : QRhi nul");
    }
    // Bilineaire entre les niveaux de mipmaps, comme l'art peint ; en repetition, la convention de
    // glTF pour une texture qui n'en dit rien.
    _sampler.reset(_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
                                    QRhiSampler::Repeat, QRhiSampler::Repeat));
    if (!_sampler->create()) {
        throw std::runtime_error("MeshBatch : echec de creation de l'echantillonneur");
    }
    _uniformStride = _rhi->ubufAligned(DRAW_BYTES);
    if (!ensureUniformCapacity(16)) {
        throw std::runtime_error("MeshBatch : echec de creation du tampon uniforme");
    }
    GRAPHICS_LOG_TRACE("MeshBatch : passe de maillages creee sur QRhi (" +
                       std::string(_rhi->backendName()) + ")");
}

MeshBatch::~MeshBatch() = default;

MeshHandle MeshBatch::create(const RhiContext& context, const core::MeshData& mesh) {
    if (!context.ready() || mesh.vertices.empty() || mesh.indices.empty()) {
        return nullptr;
    }
    auto gpu = std::make_unique<GpuMesh>();
    const auto vertexBytes = static_cast<quint32>(mesh.vertices.size() * sizeof(core::MeshVertex));
    const auto indexBytes = static_cast<quint32>(mesh.indices.size() * sizeof(std::uint32_t));
    gpu->vertices.reset(
        _rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, vertexBytes));
    gpu->indices.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, indexBytes));
    if (!gpu->vertices->create() || !gpu->indices->create()) {
        GRAPHICS_LOG_WARNING("MeshBatch : echec de creation des tampons d'un maillage");
        return nullptr;
    }
    // Le lot copie les octets : les tampons du chargeur peuvent mourir apres l'appel.
    context.updates->uploadStaticBuffer(gpu->vertices.get(), mesh.vertices.data());
    context.updates->uploadStaticBuffer(gpu->indices.get(), mesh.indices.data());

    // La couleur de base : l'image du fichier, a defaut un pixel blanc que le facteur colore.
    std::optional<LoadedTexture> texture;
    if (const std::optional<DecodedImage> image = decodeBaseColor(mesh)) {
        texture = createTexture(context, image->width, image->height, image->pixels,
                                TextureFiltering::Smooth);
    } else if (!mesh.image.empty()) {
        GRAPHICS_LOG_WARNING("MeshBatch : la texture d'un maillage ne se decode pas (" +
                             mesh.imageMimeType + "), il se dessine en couleur unie");
    }
    if (!texture) {
        texture = createTexture(context, 1, 1, {0xFFFFFFFFU}, TextureFiltering::Smooth);
    }
    if (!texture) {
        return nullptr;
    }
    gpu->texture = std::move(*texture);
    gpu->indexCount = mesh.indices.size();
    gpu->baseColor = mesh.baseColor;
    _bytes += vertexBytes + indexBytes + textureWeight(gpu->texture.width, gpu->texture.height);

    _meshes.push_back(std::move(gpu));
    return _meshes.back().get();
}

void MeshBatch::clear() noexcept {
    // Les dessins designent les maillages, les maillages leurs liaisons et leur texture.
    _draws.clear();
    _drawable = false;
    _meshes.clear();
    _bytes = 0;
}

bool MeshBatch::ensureUniformCapacity(std::size_t drawCount) {
    if (drawCount <= _uniformSlots && _uniformBuffer) {
        return true;
    }
    const std::size_t slotCount = std::max<std::size_t>(drawCount, 8) * 2;
    auto buffer = std::unique_ptr<QRhiBuffer>(_rhi->newBuffer(
        QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
        static_cast<quint32>(slotCount * static_cast<std::size_t>(_uniformStride))));
    if (!buffer->create()) {
        return false;
    }
    _uniformBuffer = std::move(buffer);
    _uniformSlots = slotCount;
    // Les liaisons referencent le tampon : elles deviennent caduques avec lui.
    for (const std::unique_ptr<GpuMesh>& mesh : _meshes) {
        mesh->bindings.reset();
    }
    _layoutBindings.reset();
    return true;
}

QRhiShaderResourceBindings* MeshBatch::bindingsFor(GpuMesh& mesh) {
    if (mesh.bindings) {
        return mesh.bindings.get();
    }
    auto bindings = std::unique_ptr<QRhiShaderResourceBindings>(_rhi->newShaderResourceBindings());
    bindings->setBindings({
        QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            _uniformBuffer.get(), DRAW_BYTES),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                  mesh.texture.texture.get(), _sampler.get()),
    });
    if (!bindings->create()) {
        GRAPHICS_LOG_WARNING("MeshBatch : echec de creation des liaisons d'un maillage");
        return nullptr;
    }
    mesh.bindings = std::move(bindings);
    return mesh.bindings.get();
}

bool MeshBatch::ensurePipeline(QRhiRenderTarget* target) {
    QRhiRenderPassDescriptor* const pass = target->renderPassDescriptor();
    if (_pipeline && _pipelinePass == pass) {
        return true;
    }
    // Liaisons de reference : jamais utilisees pour dessiner, seulement pour decrire la
    // disposition attendue par le pipeline.
    if (!_layoutBindings) {
        _layoutBindings.reset(_rhi->newShaderResourceBindings());
        _layoutBindings->setBindings({
            QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
                0,
                QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                _uniformBuffer.get(), DRAW_BYTES),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                      nullptr, _sampler.get()),
        });
        if (!_layoutBindings->create()) {
            GRAPHICS_LOG_WARNING("MeshBatch : echec de creation des liaisons de reference");
            return false;
        }
    }

    auto pipeline = std::unique_ptr<QRhiGraphicsPipeline>(_rhi->newGraphicsPipeline());
    pipeline->setShaderStages({
        {QRhiShaderStage::Vertex, loadShader(":/shaders/mesh.vert.qsb")},
        {QRhiShaderStage::Fragment, loadShader(":/shaders/mesh.frag.qsb")},
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({{static_cast<quint32>(sizeof(core::MeshVertex))}});
    inputLayout.setAttributes({
        {0, 0, QRhiVertexInputAttribute::Float3, 0},
        {0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float)},
        {0, 2, QRhiVertexInputAttribute::Float2, 6 * sizeof(float)},
    });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(_layoutBindings.get());
    pipeline->setRenderPassDescriptor(pass);
    pipeline->setTopology(QRhiGraphicsPipeline::Triangles);
    // Aucune face ecartee : la profondeur decide (voir la classe).
    pipeline->setCullMode(QRhiGraphicsPipeline::None);
    pipeline->setDepthTest(true);
    pipeline->setDepthWrite(true);
    pipeline->setDepthOp(QRhiGraphicsPipeline::Less);

    // Le melange premultiplie des quads : a l'opacite 1, un maillage opaque remplace ce qu'il
    // couvre ; au-dessous, il laisse voir le fond, pour les calques grises de l'editeur.
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
        GRAPHICS_LOG_WARNING("MeshBatch : echec de creation du pipeline graphique");
        return false;
    }
    _pipeline = std::move(pipeline);
    _pipelinePass = pass;
    return true;
}

void MeshBatch::beginFrame() {
    _draws.clear();
    _drawable = false;
}

void MeshBatch::draw(MeshHandle mesh, const DirectX::XMFLOAT4X4& clip, float opacity) {
    // L'identite est verifiee : une poignee d'avant `clear` ne doit pas etre suivie.
    const auto found = std::ranges::find_if(
        _meshes, [mesh](const std::unique_ptr<GpuMesh>& known) { return known.get() == mesh; });
    if (found == _meshes.end() || opacity <= 0.0F) {
        return;
    }
    _draws.push_back(Draw{.mesh = found->get(), .clip = clip, .opacity = std::min(opacity, 1.0F)});
}

QRhiResourceUpdateBatch* MeshBatch::prepare(QRhiRenderTarget* target,
                                            QRhiResourceUpdateBatch* updates) {
    _drawable = !_draws.empty() && ensureUniformCapacity(_draws.size()) && ensurePipeline(target);
    if (!_drawable) {
        return updates;
    }
    if (updates == nullptr) {
        updates = _rhi->nextResourceUpdateBatch();
    }
    for (std::size_t index = 0; index < _draws.size(); ++index) {
        const Draw& draw = _draws[index];
        const QMatrix4x4 clip = toClipMatrix(_rhi, draw.clip);
        // La teinte : le facteur de couleur de base, et l'opacite du dessin sur son alpha.
        const std::array<float, 4> tint = {draw.mesh->baseColor[0], draw.mesh->baseColor[1],
                                           draw.mesh->baseColor[2],
                                           draw.mesh->baseColor[3] * draw.opacity};
        const auto offset = static_cast<quint32>(index * static_cast<std::size_t>(_uniformStride));
        updates->updateDynamicBuffer(_uniformBuffer.get(), offset, MATRIX_BYTES, clip.constData());
        updates->updateDynamicBuffer(_uniformBuffer.get(), offset + MATRIX_BYTES, TINT_BYTES,
                                     tint.data());
    }
    return updates;
}

void MeshBatch::record(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target) {
    if (!_drawable) {
        return;
    }
    const QSize pixelSize = target->pixelSize();
    commandBuffer->setGraphicsPipeline(_pipeline.get());
    commandBuffer->setViewport({0.0F, 0.0F, static_cast<float>(pixelSize.width()),
                                static_cast<float>(pixelSize.height())});
    for (std::size_t index = 0; index < _draws.size(); ++index) {
        GpuMesh& mesh = *_draws[index].mesh;
        QRhiShaderResourceBindings* const bindings = bindingsFor(mesh);
        if (bindings == nullptr) {
            continue;
        }
        const QRhiCommandBuffer::DynamicOffset offset{
            0, static_cast<quint32>(index * static_cast<std::size_t>(_uniformStride))};
        commandBuffer->setShaderResources(bindings, 1, &offset);
        const QRhiCommandBuffer::VertexInput vertexInput(mesh.vertices.get(), 0);
        commandBuffer->setVertexInput(0, 1, &vertexInput, mesh.indices.get(), 0,
                                      QRhiCommandBuffer::IndexUInt32);
        commandBuffer->drawIndexed(static_cast<quint32>(mesh.indexCount));
    }
}

}  // namespace hmi
