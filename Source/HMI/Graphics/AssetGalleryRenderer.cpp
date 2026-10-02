// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/AssetGalleryRenderer.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include <rhi/qrhi.h>

#include "Core/Combat/IsoProjection.h"
#include "Core/Resources/MeshFile.h"
#include "Core/Resources/SkeletonPose.h"
#include "HMI/Graphics/GraphicsLog.h"
#include "HMI/Graphics/IsoView.h"
#include "HMI/Graphics/MeshBatch.h"
#include "HMI/Graphics/MissingTexture.h"
#include "HMI/Graphics/PlaceCamera.h"
#include "HMI/Graphics/SpriteBatch.h"
#include "HMI/Graphics/SpriteRenderer.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace hmi {

namespace {

struct Tint {
    float r;
    float g;
    float b;
    float a;
};

// Les lignes de cases : discrètes, sous tout le reste.
constexpr Tint GRID_TINT{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 0.07F};
// L'emprise, et son contour : l'or de la sélection, atténué.
constexpr Tint FOOTPRINT_FILL{.r = 0.88F, .g = 0.64F, .b = 0.29F, .a = 0.12F};
constexpr Tint FOOTPRINT_EDGE{.r = 0.88F, .g = 0.64F, .b = 0.29F, .a = 0.55F};
constexpr Tint SELECTION_EDGE{.r = 0.88F, .g = 0.64F, .b = 0.29F, .a = 1.0F};

void addRect(ComposedScene& scene, RenderLayer layer, TextureHandle white, float x, float y,
             float width, float height, Tint tint) {
    SpriteQuad quad;
    quad.x = x;
    quad.y = y;
    quad.width = width;
    quad.height = height;
    quad.r = tint.r;
    quad.g = tint.g;
    quad.b = tint.b;
    quad.a = tint.a;
    scene.addSprite(layer, white, 0, quad);
}

void addFrame(ComposedScene& scene, RenderLayer layer, TextureHandle white, float x, float y,
              float width, float height, float thickness, Tint tint) {
    addRect(scene, layer, white, x, y, width, thickness, tint);
    addRect(scene, layer, white, x, y + height - thickness, width, thickness, tint);
    addRect(scene, layer, white, x, y, thickness, height, tint);
    addRect(scene, layer, white, x + width - thickness, y, thickness, height, tint);
}

// Rectangle en pixels de la cible.
struct Area {
    float x;
    float y;
    float width;
    float height;
};

// Les lignes de cases d'un bloc, bords compris ; alignées au pixel.
void addGrid(ComposedScene& scene, TextureHandle white, const AssetGalleryDrawnBloc& bloc,
             float cell, float line) {
    const float width = static_cast<float>(bloc.columns) * cell;
    const float height = static_cast<float>(bloc.rows) * cell;
    for (int column = 0; column <= bloc.columns; ++column) {
        addRect(scene, RenderLayer::Background, white,
                std::round(bloc.x + (static_cast<float>(column) * cell)), bloc.y, line, height,
                GRID_TINT);
    }
    for (int row = 0; row <= bloc.rows; ++row) {
        addRect(scene, RenderLayer::Background, white, bloc.x,
                std::round(bloc.y + (static_cast<float>(row) * cell)), width, line, GRID_TINT);
    }
}

// Le quad de l'image courante d'un bloc, posé sur son emprise. Une texture en échec (le damier
// de remplacement) se montre entière, sans découpe en images.
[[nodiscard]] SpriteQuad artQuad(const LoadedTexture& texture, bool failed,
                                 const AssetGalleryDrawnBloc& bloc, float cell, Area footprint) {
    // L'art a l'echelle de son lieu : le losange que son manifeste declare occupe une case.
    const float artScale = cell / static_cast<float>(std::max(1, bloc.tilePixels));
    const int frameWidth =
        failed || bloc.frameWidth <= 0 ? texture.width : std::min(bloc.frameWidth, texture.width);
    const int frameHeight = failed || bloc.frameHeight <= 0
                                ? texture.height
                                : std::min(bloc.frameHeight, texture.height);
    const int frames = std::max(1, texture.width / std::max(1, frameWidth));
    const int index = failed ? 0 : std::clamp(bloc.frameIndex, 0, frames - 1);

    SpriteQuad quad;
    quad.width = static_cast<float>(frameWidth) * artScale;
    quad.height = static_cast<float>(frameHeight) * artScale;
    // Pieds sur le bas de l'emprise, centré sur elle ; au pixel près, pour la netteté.
    quad.x = std::round(footprint.x + (footprint.width / 2.0F) - (quad.width / 2.0F));
    quad.y = std::round(footprint.y + footprint.height - quad.height);
    quad.u0 = static_cast<float>(index * frameWidth) / static_cast<float>(texture.width);
    quad.u1 = static_cast<float>((index + 1) * frameWidth) / static_cast<float>(texture.width);
    quad.v1 = static_cast<float>(frameHeight) / static_cast<float>(texture.height);
    return quad;
}

}  // namespace

AssetGalleryRenderer::AssetGalleryRenderer(std::filesystem::path assetsRoot)
    : _root(std::move(assetsRoot)) {}

AssetGalleryRenderer::~AssetGalleryRenderer() {
    release();
}

bool AssetGalleryRenderer::ensureResources(QRhi* rhi) {
    if (rhi == nullptr) {
        return false;
    }
    if (created() && _rhi == rhi) {
        return true;
    }
    release();

    _rhi = rhi;
    _pendingUploads = rhi->nextResourceUpdateBatch();
    _resources.create(rhi, _pendingUploads);
    const RhiContext& context = _resources.context();
    if (std::optional<LoadedTexture> white = createTexture(context, 1, 1, {0xFFFFFFFFU})) {
        _white = std::move(*white);
    }
    const ProceduralAtlasImage checker = buildMissingTextureImage();
    if (std::optional<LoadedTexture> missing =
            createTexture(context, checker.width, checker.height, checker.pixels)) {
        _missing = std::move(*missing);
    }
    _meshes = std::make_unique<MeshBatch>(rhi);
    _resources.setFrameUpdates(nullptr);
    return true;
}

void AssetGalleryRenderer::release() noexcept {
    _composed.clear();
    _models.clear();
    _meshes.reset();
    _cache.clear();
    _white = LoadedTexture{};
    _missing = LoadedTexture{};
    if (_pendingUploads != nullptr) {
        _pendingUploads->release();
        _pendingUploads = nullptr;
    }
    _resources.release();
    _rhi = nullptr;
}

void AssetGalleryRenderer::setFrame(AssetGalleryFrame frame) {
    _frame = std::move(frame);
}

void AssetGalleryRenderer::updateCache(float deltaSeconds) {
    const std::set<std::string> wanted(_frame.wanted.begin(), _frame.wanted.end());
    const RhiContext& context = _resources.context();

    int uploads = 0;
    _loading = false;
    for (const std::string& path : wanted) {
        // Un modèle (LOT-1006) : lu une fois, gardé jusqu'à `release`.
        if (core::isMeshPath(path)) {
            if (_models.contains(path)) {
                continue;
            }
            if (uploads >= UPLOADS_PER_FRAME) {
                _loading = true;
                continue;
            }
            ++uploads;
            CachedModel model;
            core::MeshFileResult read = core::readMeshFile(_root / path);
            if (read.ok()) {
                model.mesh = _meshes->create(context, read.mesh);
                if (!read.mesh.skin.empty() &&
                    read.mesh.rig.joints.size() <= MeshBatch::MAX_BONES) {
                    model.rig = std::make_shared<const core::MeshRig>(std::move(read.mesh.rig));
                }
            }
            if (model.mesh == nullptr) {
                GRAPHICS_LOG_WARNING("Galerie des assets : modele illisible, " + path + " (" +
                                     read.message + ")");
            }
            _models.emplace(path, std::move(model));
            continue;
        }
        const auto found = _cache.find(path);
        if (found != _cache.end()) {
            found->second.unwantedSeconds = 0.0F;
            continue;
        }
        if (uploads >= UPLOADS_PER_FRAME) {
            _loading = true;
            continue;
        }
        ++uploads;
        CachedTexture entry;
        if (std::optional<LoadedTexture> texture = loadTextureFromFile(context, _root / path)) {
            entry.texture = std::move(*texture);
        } else {
            entry.failed = true;
            GRAPHICS_LOG_WARNING("Galerie des assets : texture illisible, " + path);
        }
        _cache.emplace(path, std::move(entry));
    }

    for (auto it = _cache.begin(); it != _cache.end();) {
        if (wanted.contains(it->first)) {
            ++it;
            continue;
        }
        it->second.unwantedSeconds += deltaSeconds;
        it = it->second.unwantedSeconds > EVICTION_SECONDS ? _cache.erase(it) : std::next(it);
    }
}

void AssetGalleryRenderer::addModel(const AssetGalleryDrawnBloc& bloc, float cell, float footprintX,
                                    float footprintY, float footprintWidth, float footprintHeight) {
    const auto found = _models.find(bloc.path);
    if (found == _models.end() || found->second.mesh == nullptr) {
        return;
    }
    // La caméra du jeu, une case de la galerie pour un losange : le modèle a la taille qu'il a sur
    // une carte. Il fait face à la caméra, les pieds au milieu de son emprise.
    const core::IsoProjection projection(1, 1, cell);
    const IsoView view(projection);
    ViewTransform toView =
        IsoView::turned(view.meshTransform({0.0F, 0.0F}, 0.0F),
                        (std::numbers::pi_v<float> / 2.0F) - FIGURE_HEADING_FRONT);
    toView[3] = footprintX + (footprintWidth / 2.0F);
    toView[7] = footprintY + footprintHeight - (projection.tileHeight() / 2.0F);
    toView[11] = 0.0F;
    const core::Rect bounds{
        {bloc.x, bloc.y},
        {static_cast<float>(bloc.columns) * cell, static_cast<float>(bloc.rows) * cell}};
    const CachedModel& model = found->second;
    if (model.rig == nullptr || model.rig->empty()) {
        _composed.addMesh(RenderLayer::Tile, model.mesh, toView, bounds);
        return;
    }
    static thread_local std::vector<float> pose;
    pose.assign(model.rig->joints.size() * 16, 0.0F);
    core::poseSkeleton(*model.rig, core::findClip(*model.rig, bloc.clip), bloc.clipSeconds, pose);
    _composed.addMesh(RenderLayer::Tile, model.mesh, toView, bounds, 0, pose);
}

void AssetGalleryRenderer::compose() {
    _composed.clear();
    const TextureHandle white = _white.handle();
    const float cell = std::max(1.0F, _frame.cellPixels);
    const float line = std::max(1.0F, std::floor(_frame.pixelScale));

    std::int32_t order = 0;
    for (const AssetGalleryDrawnBloc& bloc : _frame.drawn) {
        const float width = static_cast<float>(bloc.columns) * cell;
        const float height = static_cast<float>(bloc.rows) * cell;

        if (_frame.showGrid && white != nullptr) {
            addGrid(_composed, white, bloc, cell, line);
        }

        const float footprintX = bloc.x + (static_cast<float>(bloc.footprintColumn) * cell);
        const float footprintY = bloc.y + (static_cast<float>(bloc.footprintRow) * cell);
        const float footprintWidth = static_cast<float>(bloc.footprintColumns) * cell;
        const float footprintHeight = static_cast<float>(bloc.footprintRows) * cell;
        if (_frame.showFootprint && white != nullptr) {
            addRect(_composed, RenderLayer::Shadow, white, footprintX, footprintY, footprintWidth,
                    footprintHeight, FOOTPRINT_FILL);
            addFrame(_composed, RenderLayer::Shadow, white, footprintX, footprintY, footprintWidth,
                     footprintHeight, line, FOOTPRINT_EDGE);
        }

        if (bloc.mesh) {
            addModel(bloc, cell, footprintX, footprintY, footprintWidth, footprintHeight);
        }
        const auto cached = bloc.mesh ? _cache.end() : _cache.find(bloc.path);
        if (cached != _cache.end()) {
            const bool failed = cached->second.failed || cached->second.texture.texture == nullptr;
            const LoadedTexture& texture = failed ? _missing : cached->second.texture;
            if (texture.texture != nullptr && texture.width > 0 && texture.height > 0) {
                const SpriteQuad quad = artQuad(texture, failed, bloc, cell,
                                                Area{.x = footprintX,
                                                     .y = footprintY,
                                                     .width = footprintWidth,
                                                     .height = footprintHeight});
                _composed.addSprite(RenderLayer::Tile, texture.handle(), order++, quad);
            }
        }

        if (bloc.selected && white != nullptr) {
            addFrame(_composed, RenderLayer::UI, white, bloc.x, bloc.y, width, height, 2.0F * line,
                     SELECTION_EDGE);
        }
    }
    _composed.sort();
}

void AssetGalleryRenderer::render(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target,
                                  float realDeltaSeconds, const float* clear) {
    if (!created() || commandBuffer == nullptr || target == nullptr) {
        return;
    }
    QRhiResourceUpdateBatch* const updates = _pendingUploads != nullptr
                                                 ? std::exchange(_pendingUploads, nullptr)
                                                 : _rhi->nextResourceUpdateBatch();
    _resources.setFrameUpdates(updates);

    updateCache(std::max(0.0F, realDeltaSeconds));
    compose();

    const QSize pixels = target->pixelSize();
    SpriteBatch& sprites = _resources.sprites();
    sprites.beginFrame();
    submitComposedScene(sprites, screenProjectionMatrix(pixels.width(), pixels.height()),
                        _composed);
    // Les modèles : la même surface, un pixel par unité, et la profondeur de la vue du jeu ramenée
    // à l'étendue du tampon.
    PlaceCamera camera(std::max(1, pixels.width()), std::max(1, pixels.height()));
    camera.setZoom(1.0F / PlaceCamera::PIXELS_PER_UNIT);
    camera.setCenter(
        {static_cast<float>(pixels.width()) / 2.0F, static_cast<float>(pixels.height()) / 2.0F});
    camera.setDepthRange(
        IsoView(core::IsoProjection(1, 1, std::max(1.0F, _frame.cellPixels))).depthRange());
    _meshes->beginFrame();
    for (const ComposedMesh& mesh : _composed.meshes()) {
        _meshes->draw(mesh.mesh, camera.meshMatrix(mesh.toView), mesh.opacity,
                      _composed.poseOf(mesh));
    }
    // Une seule passe : la grille et les images d'abord, les modèles par-dessus -- ils se
    // départagent entre eux par la profondeur, que les images n'écrivent pas.
    QRhiResourceUpdateBatch* const uploads =
        _meshes->prepare(target, sprites.prepare(target, updates));
    commandBuffer->beginPass(target, QColor::fromRgbF(clear[0], clear[1], clear[2], clear[3]),
                             {1.0F, 0}, uploads);
    sprites.record(commandBuffer, target);
    _meshes->record(commandBuffer, target);
    commandBuffer->endPass();
    _resources.setFrameUpdates(nullptr);
}

}  // namespace hmi
