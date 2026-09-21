// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/WorldSceneRenderer.h"

#include <algorithm>
#include <optional>
#include <utility>

#include <rhi/qrhi.h>

#include "Core/Resources/AssetMarker.h"
#include "HMI/Graphics/AnimationCatalog.h"
#include "HMI/Graphics/EntityMarkers.h"
#include "HMI/Graphics/GraphicsLog.h"
#include "HMI/Graphics/MaquetteTokens.h"
#include "HMI/Graphics/MissingTexture.h"
#include "HMI/Graphics/ScenePiecePlacement.h"
#include "HMI/Graphics/SpriteBatch.h"
#include "HMI/Graphics/SpriteRenderer.h"

namespace hmi {

namespace {

/// Le chemin du `.anim.json` d'une bande : `idle.png` -> `idle.anim.json`.
[[nodiscard]] std::filesystem::path animationDescriptionOf(const std::filesystem::path& band) {
    std::filesystem::path description = band;
    description.replace_extension();
    description += ".anim.json";
    return description;
}

}  // namespace

Camera2D worldCamera(const core::IsoProjection& projection, core::Vector2 focus, int pixelWidth,
                     int pixelHeight) {
    const int width = std::max(1, pixelWidth);
    const int height = std::max(1, pixelHeight);
    Camera2D camera(width, height);

    // Un agrandissement ENTIER, jamais un cadrage ajuste : le pixel art se brouille des qu'on le
    // met a l'echelle 0,62. L'art est dessine pour 720 lignes ; au-dela, on double.
    camera.setZoom(static_cast<float>(std::max(1, height / WORLD_ART_HEIGHT_PIXELS)));

    // Le point suivi, ramene dans la scene : la vue ne montre pas le vide autour de la carte. Sur
    // un axe ou la scene est plus petite que la vue, elle reste centree. La scene occupe
    // [0, sceneSize] en unites monde (`core::IsoProjection::gridToWorld` y place la case (0, 0)).
    const core::Vector2 scene = projection.sceneSize();
    const core::Rect visible = camera.visibleBounds();
    core::Vector2 centre = focus;
    centre.x = scene.x <= visible.size.x
                   ? scene.x / 2.0F
                   : std::clamp(centre.x, visible.size.x / 2.0F, scene.x - (visible.size.x / 2.0F));
    centre.y = scene.y <= visible.size.y
                   ? scene.y / 2.0F
                   : std::clamp(centre.y, visible.size.y / 2.0F, scene.y - (visible.size.y / 2.0F));
    camera.setCenter(centre);
    return camera;
}

WorldSceneRenderer::WorldSceneRenderer(std::filesystem::path assetsDirectory)
    : _directory(std::move(assetsDirectory)) {}

WorldSceneRenderer::~WorldSceneRenderer() {
    release();
}

bool WorldSceneRenderer::ensureResources(QRhi* rhi) {
    if (rhi == nullptr) {
        return false;
    }
    if (created() && _rhi == rhi) {
        return true;
    }
    // Une autre interface : tout ce qui a ete cree appartient a l'ancienne et ne doit plus servir.
    release();

    _rhi = rhi;
    _pendingUploads = rhi->nextResourceUpdateBatch();
    _resources.create(rhi, _pendingUploads);

    const ProceduralAtlasImage checker = buildMissingTextureImage();
    if (std::optional<LoadedTexture> missing =
            createTexture(_resources.context(), checker.width, checker.height, checker.pixels)) {
        _missing = std::move(*missing);
        _textures.missing = SceneTexture{
            .texture = _missing.handle(), .width = _missing.width, .height = _missing.height};
    }
    // L'aplat du rendu de maquette : un pixel blanc, que la teinte de chaque primitive colore
    // (LOT-128). Un seul pixel, donc une seule texture pour toutes les cases d'une carte nue.
    if (std::optional<LoadedTexture> solid =
            createTexture(_resources.context(), 1, 1, {0xFFFFFFFFU})) {
        _solid = std::move(*solid);
        _textures.solid = SceneTexture{
            .texture = _solid.handle(), .width = _solid.width, .height = _solid.height};
    }
    _resources.setFrameUpdates(nullptr);
    GRAPHICS_LOG_INFO("Lieu : ressources QRhi creees (" + std::string(rhi->backendName()) + ").");
    return true;
}

int WorldSceneRenderer::bandFrameWidth(const std::string& path) {
    const auto connue = _bandFrameWidths.find(path);
    if (connue != _bandFrameWidths.end()) {
        return connue->second;
    }
    // Seules les figurines ont un `.anim.json` ; une piece de decor n'en a pas, et son absence est
    // un cas legitime, pas une anomalie.
    const AnimationDescriptionResult lue =
        AnimationCatalog::loadFromFile(animationDescriptionOf(_directory / path));
    const int largeur = lue.ok() ? lue.description->frameWidth : 0;
    _bandFrameWidths.emplace(path, largeur);
    return largeur;
}

std::optional<LoadedTexture> WorldSceneRenderer::figureMarker(const std::string& path) {
    const std::string cle = figureMarkerKey(path);
    if (cle.empty()) {
        return std::nullopt;
    }
    const core::MarkerImage image =
        core::assetMarker(cle, FIGURE_FRAME_WIDTH_PIXELS, FIGURE_FRAME_HEIGHT_PIXELS);
    if (image.isEmpty()) {
        return std::nullopt;
    }
    GRAPHICS_LOG_INFO("Lieu : la figurine " + cle + " n'a pas d'image, son marqueur la remplace.");
    return createTexture(_resources.context(), image.width, image.height, markerPixelsRgba8(image));
}

void WorldSceneRenderer::ensureTextures(const std::vector<std::string>& paths) {
    for (const std::string& path : paths) {
        // Deja tente : une piece absente ne doit pas etre redemandee a chaque image.
        if (!_requested.insert(path).second) {
            continue;
        }
        // Un jeton n'est pas un fichier : il se peint (LOT-128, decision D2). La meme image, au
        // pixel pres, que celle que l'editeur dessine.
        if (const core::MarkerImage token = maquetteTokenImage(path, MAQUETTE_TOKEN_SIZE_PIXELS);
            !token.isEmpty()) {
            if (std::optional<LoadedTexture> painted = createTexture(
                    _resources.context(), token.width, token.height, markerPixelsRgba8(token))) {
                _textures.byPath[path] = SceneTexture{.texture = painted->handle(),
                                                      .width = painted->width,
                                                      .height = painted->height};
                _loaded.push_back(std::move(*painted));
            }
            continue;
        }
        std::optional<LoadedTexture> texture =
            loadTextureFromFile(_resources.context(), _directory / path);
        if (!texture.has_value()) {
            // Une figurine sans image se dessine par son marqueur (LOT-39, LOT-96) : la
            // sentinelle se voit avant que l'atelier ne l'ait dessinee.
            if (std::optional<LoadedTexture> marqueur = figureMarker(path)) {
                _textures.byPath[path] = SceneTexture{.texture = marqueur->handle(),
                                                      .width = marqueur->width,
                                                      .height = marqueur->height,
                                                      .frameWidth = marqueur->width};
                _loaded.push_back(std::move(*marqueur));
                continue;
            }
            // La composition retombe sur le damier : une piece manquante se voit, sans planter.
            GRAPHICS_LOG_WARNING(missingTextureWarning(path));
            continue;
        }
        _textures.byPath[path] = SceneTexture{.texture = texture->handle(),
                                              .width = texture->width,
                                              .height = texture->height,
                                              .frameWidth = bandFrameWidth(path)};
        if (path.starts_with("Scene/")) {
            const auto file = _directory / path;
            const auto document =
                core::readJsonObjectFromFile(file.parent_path() / "manifest.json", 1);
            if (document.ok()) {
                _textures.byPath[path].anchor =
                    scenePieceAnchor(document.root, file.filename().string());
                _textures.byPath[path].depthOffset =
                    scenePieceDepthOffset(document.root, file.filename().string());
            }
        }
        _loaded.push_back(std::move(*texture));
    }
}

void WorldSceneRenderer::release() noexcept {
    // L'ordre : ce qui designe une texture, puis les textures, puis la grappe qui porte le
    // pipeline. Le lot de creation jamais soumis est rendu a QRhi.
    _composed.clear();
    _textures.byPath.clear();
    _textures.missing = SceneTexture{};
    _textures.solid = SceneTexture{};
    _loaded.clear();
    _missing = LoadedTexture{};
    _solid = LoadedTexture{};
    _requested.clear();
    if (_pendingUploads != nullptr) {
        _pendingUploads->release();
        _pendingUploads = nullptr;
    }
    _resources.release();
    _rhi = nullptr;
}

void WorldSceneRenderer::setSnapshot(WorldSceneSnapshot snapshot) {
    _snapshot = std::move(snapshot);
}

void WorldSceneRenderer::render(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target,
                                const float* clear) {
    if (!created() || commandBuffer == nullptr || target == nullptr) {
        return;
    }
    // Le lot de cette image : celui de la creation s'il attend encore, sinon un neuf.
    QRhiResourceUpdateBatch* const updates = _pendingUploads != nullptr
                                                 ? std::exchange(_pendingUploads, nullptr)
                                                 : _rhi->nextResourceUpdateBatch();
    _resources.setFrameUpdates(updates);

    // Les textures du lieu ne sont pas connues d'avance : la carte change au passage d'un portail,
    // et ce sont ses pieces qui disent quoi charger.
    ensureTextures(worldTexturePaths(_snapshot));

    const core::IsoProjection projection(_snapshot.columns, _snapshot.rows,
                                         core::ARENA_TILE_WIDTH_UNITS, _snapshot.diamondRatio);
    _composed.clear();
    composeWorldScene(_composed, _snapshot, projection, _textures);
    _composed.sort();

    const QSize pixels = target->pixelSize();
    const Camera2D camera =
        worldCamera(projection, projection.gridToWorld(_focus), pixels.width(), pixels.height());

    SpriteBatch& sprites = _resources.sprites();
    sprites.beginFrame();
    submitComposedScene(sprites, camera.projectionMatrix(), _composed);
    sprites.submit(commandBuffer, target, updates, clear);
    _resources.setFrameUpdates(nullptr);
}

}  // namespace hmi
