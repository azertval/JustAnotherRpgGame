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
#include "HMI/Graphics/LightingBlock.h"
#include "HMI/Graphics/RhiContext.h"
#include "HMI/Graphics/RhiShaders.h"

namespace hmi {

namespace {

// Le bloc uniforme d'un dessin (std140) : la matrice maillage -> clip, puis la teinte RVBA.
constexpr int MATRIX_BYTES = 16 * static_cast<int>(sizeof(float));
constexpr int TINT_BYTES = 4 * static_cast<int>(sizeof(float));
// Depuis le LOT-1007, deux matrices de plus : maillage -> vue, maillage -> clip de la carte
// d'ombres. Le meme bloc sert a l'image et a la passe d'ombres.
constexpr int VIEW_OFFSET = MATRIX_BYTES + TINT_BYTES;
constexpr int SHADOW_OFFSET = VIEW_OFFSET + MATRIX_BYTES;
constexpr int DRAW_BYTES = SHADOW_OFFSET + MATRIX_BYTES;
// Le biais de la passe d'ombres : ce dont une face recule dans la carte, pour ne pas s'ombrer
// elle-meme par l'arrondi de sa profondeur. Constant, et proportionnel a sa pente vue du soleil.
constexpr int SHADOW_DEPTH_BIAS = 2;
constexpr float SHADOW_SLOPE_BIAS = 2.0F;
// Le bloc d'os d'un dessin anime (std140) : `MAX_BONES` matrices.
constexpr std::size_t MATRIX_FLOATS = 16;
constexpr std::size_t POSE_FLOATS = MeshBatch::MAX_BONES * MATRIX_FLOATS;
constexpr int BONES_BYTES = static_cast<int>(POSE_FLOATS * sizeof(float));
// La liaison d'un sommet televersee : quatre rangs d'os, en flottants, puis quatre poids.
constexpr std::size_t SKIN_FLOATS = 8;

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
    // Le bloc d'eclairage neutre (LOT-1007) : lie tant que l'appelant n'en donne pas un autre.
    _ownLighting = std::make_unique<LightingBlock>(_rhi);
    _uniformStride = _rhi->ubufAligned(DRAW_BYTES);
    _boneStride = _rhi->ubufAligned(BONES_BYTES);
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

    // La liaison au squelette (LOT-1005) : un second tampon, pour que le sommet d'un maillage fixe
    // reste celui d'avant. Un squelette plus grand que le bloc d'os du shader ne s'anime pas : le
    // maillage se dessine dans sa pose de liaison, et c'est dit.
    std::size_t skinBytes = 0;
    if (mesh.skin.size() == mesh.vertices.size() && !mesh.rig.joints.empty()) {
        if (mesh.rig.joints.size() > MAX_BONES) {
            GRAPHICS_LOG_WARNING("MeshBatch : un squelette de " +
                                 std::to_string(mesh.rig.joints.size()) + " os depasse les " +
                                 std::to_string(MAX_BONES) +
                                 " du rendu, le maillage reste dans sa pose de liaison");
        } else {
            std::vector<float> skin(mesh.skin.size() * SKIN_FLOATS);
            for (std::size_t index = 0; index < mesh.skin.size(); ++index) {
                for (std::size_t part = 0; part < 4; ++part) {
                    skin[(index * SKIN_FLOATS) + part] =
                        static_cast<float>(mesh.skin[index].joints[part]);
                    skin[(index * SKIN_FLOATS) + 4 + part] = mesh.skin[index].weights[part];
                }
            }
            skinBytes = skin.size() * sizeof(float);
            gpu->skin.reset(_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                            static_cast<quint32>(skinBytes)));
            if (!gpu->skin->create()) {
                GRAPHICS_LOG_WARNING("MeshBatch : echec de creation du tampon d'os d'un maillage");
                return nullptr;
            }
            context.updates->uploadStaticBuffer(gpu->skin.get(), skin.data());
            gpu->boneCount = mesh.rig.joints.size();
        }
    }

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
    _bytes += vertexBytes + indexBytes + skinBytes +
              textureWeight(gpu->texture.width, gpu->texture.height);

    _meshes.push_back(std::move(gpu));
    return _meshes.back().get();
}

void MeshBatch::clear() noexcept {
    // Les dessins designent les maillages, les maillages leurs liaisons et leur texture.
    _draws.clear();
    _poses.clear();
    _drawable = false;
    _meshes.clear();
    _bytes = 0;
}

void MeshBatch::destroy(MeshHandle mesh) noexcept {
    const auto found = std::ranges::find_if(
        _meshes, [mesh](const std::unique_ptr<GpuMesh>& known) { return known.get() == mesh; });
    if (found == _meshes.end()) {
        return;
    }
    // Un dessin enregistre le designerait encore.
    _draws.clear();
    _poses.clear();
    _drawable = false;
    _shadowDrawable = false;
    _meshes.erase(found);
}

void MeshBatch::setLighting(LightingBlock* lighting) {
    if (lighting == _lighting) {
        return;
    }
    _lighting = lighting;
    resetBindings(false);
    _lightingRevision = this->lighting()->revision();
}

void MeshBatch::resetBindings(bool skinnedOnly) {
    for (const std::unique_ptr<GpuMesh>& mesh : _meshes) {
        if (!skinnedOnly) {
            mesh->bindings.reset();
        }
        mesh->skinnedBindings.reset();
    }
    if (!skinnedOnly) {
        _layoutBindings.reset();
        _shadowBindings.reset();
    }
    _skinnedLayoutBindings.reset();
    _skinnedShadowBindings.reset();
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
    resetBindings(false);
    return true;
}

bool MeshBatch::ensureBoneCapacity(std::size_t poseCount) {
    if (poseCount <= _boneSlots && _boneBuffer) {
        return true;
    }
    const std::size_t slotCount = std::max<std::size_t>(poseCount, 4) * 2;
    auto buffer = std::unique_ptr<QRhiBuffer>(
        _rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                        static_cast<quint32>(slotCount * static_cast<std::size_t>(_boneStride))));
    if (!buffer->create()) {
        return false;
    }
    _boneBuffer = std::move(buffer);
    _boneSlots = slotCount;
    resetBindings(true);
    return true;
}

QRhiShaderResourceBindings* MeshBatch::skinnedBindingsFor(GpuMesh& mesh) {
    if (mesh.skinnedBindings) {
        return mesh.skinnedBindings.get();
    }
    auto bindings = std::unique_ptr<QRhiShaderResourceBindings>(_rhi->newShaderResourceBindings());
    bindings->setBindings({
        QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            _uniformBuffer.get(), DRAW_BYTES),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                  mesh.texture.texture.get(), _sampler.get()),
        QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
            2, QRhiShaderResourceBinding::VertexStage, _boneBuffer.get(), BONES_BYTES),
        lighting()->uniformBinding(),
        lighting()->shadowBinding(),
    });
    if (!bindings->create()) {
        GRAPHICS_LOG_WARNING("MeshBatch : echec de creation des liaisons d'un maillage anime");
        return nullptr;
    }
    mesh.skinnedBindings = std::move(bindings);
    return mesh.skinnedBindings.get();
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
        lighting()->uniformBinding(),
        lighting()->shadowBinding(),
    });
    if (!bindings->create()) {
        GRAPHICS_LOG_WARNING("MeshBatch : echec de creation des liaisons d'un maillage");
        return nullptr;
    }
    mesh.bindings = std::move(bindings);
    return mesh.bindings.get();
}

bool MeshBatch::ensureLayouts(bool skinned) {
    // La carte d'ombres a change d'objet : les liaisons qui citaient l'ancienne sont caduques.
    if (_lightingRevision != lighting()->revision()) {
        resetBindings(false);
        _lightingRevision = lighting()->revision();
    }
    const QRhiShaderResourceBinding::StageFlags drawStages =
        QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    // Liaisons de reference : jamais utilisees pour dessiner, seulement pour decrire la
    // disposition attendue par le pipeline. Celles de la passe d'ombres, elles, dessinent : elles
    // ne citent aucune texture, et valent donc pour tous les maillages.
    if (!skinned && !_layoutBindings) {
        _layoutBindings.reset(_rhi->newShaderResourceBindings());
        _layoutBindings->setBindings({
            QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
                0, drawStages, _uniformBuffer.get(), DRAW_BYTES),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                      nullptr, _sampler.get()),
            lighting()->uniformBinding(),
            lighting()->shadowBinding(),
        });
        _shadowBindings.reset(_rhi->newShaderResourceBindings());
        _shadowBindings->setBindings({
            QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
                0, drawStages, _uniformBuffer.get(), DRAW_BYTES),
        });
        if (!_layoutBindings->create() || !_shadowBindings->create()) {
            GRAPHICS_LOG_WARNING("MeshBatch : echec de creation des liaisons de reference");
            _layoutBindings.reset();
            _shadowBindings.reset();
            return false;
        }
    }
    if (skinned && !_skinnedLayoutBindings) {
        _skinnedLayoutBindings.reset(_rhi->newShaderResourceBindings());
        _skinnedLayoutBindings->setBindings({
            QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
                0, drawStages, _uniformBuffer.get(), DRAW_BYTES),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                      nullptr, _sampler.get()),
            QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
                2, QRhiShaderResourceBinding::VertexStage, _boneBuffer.get(), BONES_BYTES),
            lighting()->uniformBinding(),
            lighting()->shadowBinding(),
        });
        _skinnedShadowBindings.reset(_rhi->newShaderResourceBindings());
        _skinnedShadowBindings->setBindings({
            QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
                0, drawStages, _uniformBuffer.get(), DRAW_BYTES),
            QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(
                2, QRhiShaderResourceBinding::VertexStage, _boneBuffer.get(), BONES_BYTES),
        });
        if (!_skinnedLayoutBindings->create() || !_skinnedShadowBindings->create()) {
            GRAPHICS_LOG_WARNING("MeshBatch : echec de creation des liaisons de reference animees");
            _skinnedLayoutBindings.reset();
            _skinnedShadowBindings.reset();
            return false;
        }
    }
    return true;
}

bool MeshBatch::ensurePipeline(Pipeline& kept, QRhiRenderTarget* target, bool skinned,
                               bool shadow) {
    QRhiRenderPassDescriptor* const pass = target->renderPassDescriptor();
    // Le pipeline suit aussi le nombre d'echantillons de la cible : une cible multi-echantillonnee
    // refuse un pipeline qui ne l'est pas autant qu'elle.
    const int samples = target->sampleCount();
    const bool layouts = ensureLayouts(skinned);
    if (!layouts) {
        return false;
    }
    if (kept.pipeline && kept.pass == pass && kept.samples == samples) {
        return true;
    }

    auto pipeline = std::unique_ptr<QRhiGraphicsPipeline>(_rhi->newGraphicsPipeline());
    const char* const vertex =
        shadow ? (skinned ? ":/shaders/mesh_skinned_shadow.vert.qsb"
                          : ":/shaders/mesh_shadow.vert.qsb")
               : (skinned ? ":/shaders/mesh_skinned.vert.qsb" : ":/shaders/mesh.vert.qsb");
    pipeline->setShaderStages({
        {QRhiShaderStage::Vertex, loadShader(vertex)},
        {QRhiShaderStage::Fragment,
         loadShader(shadow ? ":/shaders/shadow.frag.qsb" : ":/shaders/mesh.frag.qsb")},
    });
    // Le sommet du chargeur, tel quel ; puis, pour un maillage anime, ses os et ses poids.
    QRhiVertexInputLayout inputLayout;
    if (skinned) {
        inputLayout.setBindings({{static_cast<quint32>(sizeof(core::MeshVertex))},
                                 {static_cast<quint32>(SKIN_FLOATS * sizeof(float))}});
        inputLayout.setAttributes({
            {0, 0, QRhiVertexInputAttribute::Float3, 0},
            {0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float)},
            {0, 2, QRhiVertexInputAttribute::Float2, 6 * sizeof(float)},
            {1, 3, QRhiVertexInputAttribute::Float4, 0},
            {1, 4, QRhiVertexInputAttribute::Float4, 4 * sizeof(float)},
        });
    } else {
        inputLayout.setBindings({{static_cast<quint32>(sizeof(core::MeshVertex))}});
        inputLayout.setAttributes({
            {0, 0, QRhiVertexInputAttribute::Float3, 0},
            {0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float)},
            {0, 2, QRhiVertexInputAttribute::Float2, 6 * sizeof(float)},
        });
    }
    pipeline->setVertexInputLayout(inputLayout);
    if (shadow) {
        pipeline->setShaderResourceBindings(skinned ? _skinnedShadowBindings.get()
                                                    : _shadowBindings.get());
    } else {
        pipeline->setShaderResourceBindings(skinned ? _skinnedLayoutBindings.get()
                                                    : _layoutBindings.get());
    }
    pipeline->setRenderPassDescriptor(pass);
    pipeline->setSampleCount(samples);
    pipeline->setTopology(QRhiGraphicsPipeline::Triangles);
    // Aucune face ecartee : la profondeur decide (voir la classe).
    pipeline->setCullMode(QRhiGraphicsPipeline::None);
    pipeline->setDepthTest(true);
    pipeline->setDepthWrite(true);
    pipeline->setDepthOp(QRhiGraphicsPipeline::Less);
    if (shadow) {
        // La carte d'ombres n'a pas de couleur : seule la profondeur s'ecrit, un peu reculee.
        pipeline->setDepthBias(SHADOW_DEPTH_BIAS);
        pipeline->setSlopeScaledDepthBias(SHADOW_SLOPE_BIAS);
    } else {
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
    }

    if (!pipeline->create()) {
        GRAPHICS_LOG_WARNING(
            std::string("MeshBatch : echec de creation du pipeline des maillages") +
            (skinned ? " animes" : "") + (shadow ? " (ombres)" : ""));
        return false;
    }
    kept.pipeline = std::move(pipeline);
    kept.pass = pass;
    kept.samples = samples;
    return true;
}

void MeshBatch::beginFrame() {
    _draws.clear();
    _poses.clear();
    _drawable = false;
}

void MeshBatch::draw(MeshHandle mesh, const DirectX::XMFLOAT4X4& clip, float opacity,
                     std::span<const float> bones, const MeshLighting& lighting) {
    // L'identite est verifiee : une poignee d'avant `clear` ne doit pas etre suivie.
    const auto found = std::ranges::find_if(
        _meshes, [mesh](const std::unique_ptr<GpuMesh>& known) { return known.get() == mesh; });
    if (found == _meshes.end() || opacity <= 0.0F) {
        return;
    }
    Draw draw{.mesh = found->get(),
              .clip = clip,
              .opacity = std::min(opacity, 1.0F),
              .pose = -1,
              .lighting = lighting};
    // Une pose ne vaut que pour le squelette du maillage : os pour os.
    const std::size_t boneCount = draw.mesh->boneCount;
    if (boneCount > 0 && bones.size() == boneCount * MATRIX_FLOATS) {
        draw.pose = static_cast<int>(_poses.size() / POSE_FLOATS);
        _poses.insert(_poses.end(), bones.begin(), bones.end());
        // Le bloc du shader a `MAX_BONES` matrices : l'identite au-dela du squelette.
        for (std::size_t bone = boneCount; bone < MAX_BONES; ++bone) {
            _poses.insert(_poses.end(), core::MESH_IDENTITY.begin(), core::MESH_IDENTITY.end());
        }
    }
    _draws.push_back(draw);
}

QRhiResourceUpdateBatch* MeshBatch::prepare(QRhiRenderTarget* target,
                                            QRhiResourceUpdateBatch* updates) {
    const std::size_t poseCount = _poses.size() / POSE_FLOATS;
    _shadowDrawable = false;
    _drawable = !_draws.empty() && ensureUniformCapacity(_draws.size()) &&
                ensurePipeline(_pipeline, target, false, false);
    // Sans pipeline anime, les maillages lies se dessinent dans leur pose de liaison.
    if (_drawable && poseCount > 0 &&
        !(ensureBoneCapacity(poseCount) && ensurePipeline(_skinnedPipeline, target, true, false))) {
        for (Draw& draw : _draws) {
            draw.pose = -1;
        }
        _poses.clear();
    }
    // Le bloc d'eclairage neutre, pose a la construction du bloc lie, part avec la premiere image.
    if (QRhiResourceUpdateBatch* const neutral = lighting()->takePendingUpload()) {
        if (updates != nullptr) {
            updates->merge(neutral);
            neutral->release();
        } else {
            updates = neutral;
        }
    }
    if (!_drawable) {
        return updates;
    }
    if (updates == nullptr) {
        updates = _rhi->nextResourceUpdateBatch();
    }
    for (std::size_t pose = 0; pose < _poses.size() / POSE_FLOATS; ++pose) {
        updates->updateDynamicBuffer(
            _boneBuffer.get(), static_cast<quint32>(pose * static_cast<std::size_t>(_boneStride)),
            BONES_BYTES, _poses.data() + (pose * POSE_FLOATS));
    }
    for (std::size_t index = 0; index < _draws.size(); ++index) {
        const Draw& draw = _draws[index];
        const QMatrix4x4 clip = toClipMatrix(_rhi, draw.clip);
        // La teinte : le facteur de couleur de base, et l'opacite du dessin sur son alpha.
        const std::array<float, 4> tint = {draw.mesh->baseColor[0], draw.mesh->baseColor[1],
                                           draw.mesh->baseColor[2],
                                           draw.mesh->baseColor[3] * draw.opacity};
        // La matrice vers la carte d'ombres recoit la meme correction de clip que celle de l'image.
        const QMatrix4x4 shadowClip = _rhi->clipSpaceCorrMatrix() *
                                      QMatrix4x4(draw.lighting.toShadowClip.data()).transposed();
        const auto offset = static_cast<quint32>(index * static_cast<std::size_t>(_uniformStride));
        updates->updateDynamicBuffer(_uniformBuffer.get(), offset, MATRIX_BYTES, clip.constData());
        updates->updateDynamicBuffer(_uniformBuffer.get(), offset + MATRIX_BYTES, TINT_BYTES,
                                     tint.data());
        updates->updateDynamicBuffer(_uniformBuffer.get(), offset + VIEW_OFFSET, MATRIX_BYTES,
                                     draw.lighting.toView.data());
        updates->updateDynamicBuffer(_uniformBuffer.get(), offset + SHADOW_OFFSET, MATRIX_BYTES,
                                     shadowClip.constData());
    }
    return updates;
}

bool MeshBatch::prepareShadow(QRhiRenderTarget* target) {
    _shadowDrawable = false;
    if (!_drawable || target == nullptr) {
        return false;
    }
    const bool skinned =
        std::ranges::any_of(_draws, [](const Draw& draw) { return draw.pose >= 0; });
    _shadowDrawable = ensurePipeline(_shadowPipeline, target, false, true) &&
                      (!skinned || ensurePipeline(_skinnedShadowPipeline, target, true, true));
    return _shadowDrawable;
}

void MeshBatch::record(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target) {
    if (!_drawable) {
        return;
    }
    const QSize pixelSize = target->pixelSize();
    const QRhiViewport viewport{0.0F, 0.0F, static_cast<float>(pixelSize.width()),
                                static_cast<float>(pixelSize.height())};
    // Le pipeline lie : aucun au depart, puis celui du dernier dessin -- les maillages fixes
    // d'abord dans une carte ordinaire, les figurines ensuite, donc un ou deux changements.
    const QRhiGraphicsPipeline* bound = nullptr;
    for (std::size_t index = 0; index < _draws.size(); ++index) {
        const Draw& draw = _draws[index];
        // Une boite d'ombre ne parait pas a l'image.
        if (draw.lighting.castsOnly) {
            continue;
        }
        GpuMesh& mesh = *draw.mesh;
        const bool skinned = draw.pose >= 0;
        QRhiShaderResourceBindings* const bindings =
            skinned ? skinnedBindingsFor(mesh) : bindingsFor(mesh);
        if (bindings == nullptr) {
            continue;
        }
        QRhiGraphicsPipeline* const pipeline =
            skinned ? _skinnedPipeline.pipeline.get() : _pipeline.pipeline.get();
        if (pipeline != bound) {
            commandBuffer->setGraphicsPipeline(pipeline);
            commandBuffer->setViewport(viewport);
            bound = pipeline;
        }
        const std::array<QRhiCommandBuffer::DynamicOffset, 2> offsets{
            QRhiCommandBuffer::DynamicOffset{
                0, static_cast<quint32>(index * static_cast<std::size_t>(_uniformStride))},
            QRhiCommandBuffer::DynamicOffset{
                2, static_cast<quint32>(static_cast<std::size_t>(std::max(draw.pose, 0)) *
                                        static_cast<std::size_t>(_boneStride))}};
        commandBuffer->setShaderResources(bindings, skinned ? 2 : 1, offsets.data());
        const std::array<QRhiCommandBuffer::VertexInput, 2> inputs{
            QRhiCommandBuffer::VertexInput(mesh.vertices.get(), 0),
            QRhiCommandBuffer::VertexInput(mesh.skin.get(), 0)};
        commandBuffer->setVertexInput(0, skinned ? 2 : 1, inputs.data(), mesh.indices.get(), 0,
                                      QRhiCommandBuffer::IndexUInt32);
        commandBuffer->drawIndexed(static_cast<quint32>(mesh.indexCount));
    }
}

void MeshBatch::recordShadow(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target) {
    if (!_shadowDrawable) {
        return;
    }
    const QSize pixelSize = target->pixelSize();
    const QRhiViewport viewport{0.0F, 0.0F, static_cast<float>(pixelSize.width()),
                                static_cast<float>(pixelSize.height())};
    const QRhiGraphicsPipeline* bound = nullptr;
    for (std::size_t index = 0; index < _draws.size(); ++index) {
        const Draw& draw = _draws[index];
        // Un maillage que l'editeur grise ne jette pas d'ombre pleine : il n'en jette pas.
        if (draw.opacity < 1.0F && !draw.lighting.castsOnly) {
            continue;
        }
        GpuMesh& mesh = *draw.mesh;
        const bool skinned = draw.pose >= 0;
        QRhiGraphicsPipeline* const pipeline =
            skinned ? _skinnedShadowPipeline.pipeline.get() : _shadowPipeline.pipeline.get();
        if (pipeline != bound) {
            commandBuffer->setGraphicsPipeline(pipeline);
            commandBuffer->setViewport(viewport);
            bound = pipeline;
        }
        const std::array<QRhiCommandBuffer::DynamicOffset, 2> offsets{
            QRhiCommandBuffer::DynamicOffset{
                0, static_cast<quint32>(index * static_cast<std::size_t>(_uniformStride))},
            QRhiCommandBuffer::DynamicOffset{
                2, static_cast<quint32>(static_cast<std::size_t>(std::max(draw.pose, 0)) *
                                        static_cast<std::size_t>(_boneStride))}};
        commandBuffer->setShaderResources(
            skinned ? _skinnedShadowBindings.get() : _shadowBindings.get(), skinned ? 2 : 1,
            offsets.data());
        const std::array<QRhiCommandBuffer::VertexInput, 2> inputs{
            QRhiCommandBuffer::VertexInput(mesh.vertices.get(), 0),
            QRhiCommandBuffer::VertexInput(mesh.skin.get(), 0)};
        commandBuffer->setVertexInput(0, skinned ? 2 : 1, inputs.data(), mesh.indices.get(), 0,
                                      QRhiCommandBuffer::IndexUInt32);
        commandBuffer->drawIndexed(static_cast<quint32>(mesh.indexCount));
    }
}

}  // namespace hmi
