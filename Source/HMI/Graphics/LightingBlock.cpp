// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/LightingBlock.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

#include <rhi/qrhi.h>

#include "HMI/Graphics/GraphicsLog.h"

namespace hmi {

namespace {

constexpr int UNIFORM_BYTES = static_cast<int>(sizeof(LightingUniforms));

}  // namespace

LightingBlock::LightingBlock(QRhi* rhi) : _rhi(rhi) {
    if (_rhi == nullptr) {
        throw std::runtime_error("LightingBlock : QRhi nul");
    }
    _buffer.reset(_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UNIFORM_BYTES));
    // Lue par comparaison, lissee : quatre texels compares se melangent en une lecture, ce qui
    // adoucit deja le bord d'une ombre. Hors de la carte, le bord est repete -- le shader n'y lit
    // pas.
    _sampler.reset(_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                    QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    _sampler->setTextureCompareOp(QRhiSampler::LessOrEqual);
    if (!_buffer->create() || !_sampler->create() || !createMap(1, false)) {
        throw std::runtime_error("LightingBlock : echec de creation du bloc d'eclairage");
    }
    // Le bloc neutre : tant que personne n'en televerse un autre, les shaders dessinent comme
    // avant le lot.
    QRhiResourceUpdateBatch* const initial = _rhi->nextResourceUpdateBatch();
    upload(initial, LightingUniforms{});
    _pendingUpload = initial;
}

LightingBlock::~LightingBlock() {
    if (_pendingUpload != nullptr) {
        _pendingUpload->release();
    }
}

QRhiShaderResourceBinding LightingBlock::uniformBinding() const {
    return QRhiShaderResourceBinding::uniformBuffer(
        UNIFORM_BINDING, QRhiShaderResourceBinding::FragmentStage, _buffer.get());
}

QRhiShaderResourceBinding LightingBlock::shadowBinding() const {
    return QRhiShaderResourceBinding::sampledTexture(
        SHADOW_BINDING, QRhiShaderResourceBinding::FragmentStage, _map.get(), _sampler.get());
}

void LightingBlock::upload(QRhiResourceUpdateBatch* updates, const LightingUniforms& uniforms) {
    if (updates == nullptr) {
        return;
    }
    // Le bloc neutre de la construction n'a plus lieu d'etre : joint au lot apres ce
    // televersement, il l'ecraserait -- et la premiere image serait sans lumiere.
    if (updates != _pendingUpload && _pendingUpload != nullptr) {
        _pendingUpload->release();
        _pendingUpload = nullptr;
    }
    updates->updateDynamicBuffer(_buffer.get(), 0, UNIFORM_BYTES, &uniforms);
}

QRhiResourceUpdateBatch* LightingBlock::takePendingUpload() noexcept {
    return std::exchange(_pendingUpload, nullptr);
}

bool LightingBlock::createMap(int size, bool withTarget) {
    std::unique_ptr<QRhiTexture> map(
        _rhi->newTexture(QRhiTexture::D32F, QSize(size, size), 1, QRhiTexture::RenderTarget));
    if (!map->create()) {
        return false;
    }
    std::unique_ptr<QRhiTextureRenderTarget> target;
    std::unique_ptr<QRhiRenderPassDescriptor> pass;
    if (withTarget) {
        QRhiTextureRenderTargetDescription description;
        description.setDepthTexture(map.get());
        target.reset(_rhi->newTextureRenderTarget(description));
        pass.reset(target->newCompatibleRenderPassDescriptor());
        target->setRenderPassDescriptor(pass.get());
        if (!target->create()) {
            return false;
        }
    }
    // L'ordre : la cible designe la carte et sa passe.
    _target = std::move(target);
    _pass = std::move(pass);
    _map = std::move(map);
    _size = size;
    ++_revision;
    return true;
}

QRhiTextureRenderTarget* LightingBlock::ensureShadowMap(int size) {
    const int wanted = std::clamp(size, MINIMUM_SHADOW_SIZE, MAXIMUM_SHADOW_SIZE);
    if (_target && _size == wanted) {
        return _target.get();
    }
    if (!createMap(wanted, true)) {
        GRAPHICS_LOG_WARNING("Eclairage : la carte d'ombres de " + std::to_string(wanted) +
                             " texels ne se cree pas, l'image se dessine sans ombres");
        return nullptr;
    }
    return _target.get();
}

}  // namespace hmi
