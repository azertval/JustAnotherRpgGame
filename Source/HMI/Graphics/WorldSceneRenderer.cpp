// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/WorldSceneRenderer.h"

#include <algorithm>
#include <optional>
#include <utility>

#include <rhi/qrhi.h>

#include "Core/Resources/AssetMarker.h"
#include "HMI/Graphics/EntityMarkers.h"
#include "HMI/Graphics/GraphicsLog.h"
#include "HMI/Graphics/MaquetteTokens.h"
#include "HMI/Graphics/MissingTexture.h"
#include "HMI/Graphics/SceneTextureTraits.h"
#include "HMI/Graphics/SpriteBatch.h"
#include "HMI/Graphics/SpriteRenderer.h"

namespace hmi {

Camera2D worldCamera(const core::IsoProjection& projection, core::Vector2 focus, int pixelWidth,
                     int pixelHeight, float tilePixels) {
    const int width = std::max(1, pixelWidth);
    const int height = std::max(1, pixelHeight);
    Camera2D camera(width, height);

    // Un facteur LIBRE, fixe par la definition (EX-REN-013) : une case occupe la hauteur de la vue
    // divisee par 10,8. L'art est toujours reduit, jamais agrandi, et filtre par mipmaps : aucune
    // grille de pixels n'est plus a proteger.
    const float onScreen = tilePixels > 0.0F ? tilePixels : worldTilePixels(height);
    camera.setZoom(onScreen / (projection.tileWidth() * Camera2D::PIXELS_PER_UNIT));

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
    : _directory(std::move(assetsDirectory)),
      _scene(std::make_shared<const WorldSceneSnapshot>()) {}

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

std::optional<LoadedTexture> WorldSceneRenderer::figureMarker(const std::string& path) {
    const std::string cle = figureMarkerKey(path);
    if (cle.empty()) {
        return std::nullopt;
    }
    const core::MarkerImage image =
        core::assetMarker(cle, FIGURE_MARKER_WIDTH_PIXELS, FIGURE_MARKER_HEIGHT_PIXELS);
    if (image.isEmpty()) {
        return std::nullopt;
    }
    GRAPHICS_LOG_INFO("Lieu : la figurine " + cle + " n'a pas d'image, son marqueur la remplace.");
    return createTexture(_resources.context(), image.width, image.height, markerPixelsRgba8(image));
}

void WorldSceneRenderer::ensureTextures(const std::vector<std::string>& paths) {
    // Ce qui reste a charger : ni deja tente (une piece absente ne se redemande pas a chaque
    // image), ni un jeton, qui se peint.
    std::vector<std::string> files;
    for (const std::string& path : paths) {
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
        files.push_back(path);
    }
    if (files.empty()) {
        return;
    }
    // Les images se decodent sur tous les coeurs ; les textures se creent ici, sur le fil de rendu,
    // dans le lot de l'image (audit de l'affichage, A5).
    std::vector<std::filesystem::path> absolute;
    absolute.reserve(files.size());
    for (const std::string& path : files) {
        absolute.push_back(_directory / path);
    }
    std::vector<std::optional<DecodedImage>> decoded = decodeImageFiles(absolute);
    for (std::size_t index = 0; index < files.size(); ++index) {
        const std::string& path = files[index];
        std::optional<LoadedTexture> texture =
            decoded[index]
                ? createTexture(_resources.context(), decoded[index]->width, decoded[index]->height,
                                decoded[index]->pixels, TextureFiltering::Smooth)
                : std::nullopt;
        decoded[index].reset();  // les pixels sont copies dans le lot : on les rend tout de suite
        if (!texture.has_value()) {
            // Une figurine sans image se dessine par son marqueur (LOT-39, LOT-96) : la
            // sentinelle se voit avant que l'atelier ne l'ait dessinee. Une case de large.
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
        // Decoupe, echelle et ancre : ce que ses fichiers voisins disent de l'image (LOT-103), lus
        // dans des manifestes qui ne se relisent pas.
        SceneTexture& loaded = _textures.byPath[path] = SceneTexture{
            .texture = texture->handle(), .width = texture->width, .height = texture->height};
        applySceneTextureTraits(loaded, readSceneTextureTraits(_directory, path, &_manifests));
        _loaded.push_back(std::move(*texture));
    }
}

void WorldSceneRenderer::release() noexcept {
    // L'ordre : ce qui designe une texture, puis les textures, puis la grappe qui porte le
    // pipeline. Le lot de creation jamais soumis est rendu a QRhi.
    _composed.clear();
    _statics.clear();
    // Les poignees de la carte composee appartiennent aux textures liberees : elle se recompose.
    _sceneDirty = true;
    _figuresDirty = true;
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
    std::vector<WorldFigureSnapshot> figures = snapshot.figures;
    setScene(std::make_shared<const WorldSceneSnapshot>(std::move(snapshot)));
    setFigures(std::move(figures));
}

void WorldSceneRenderer::setScene(std::shared_ptr<const WorldSceneSnapshot> scene) {
    _scene = scene != nullptr ? std::move(scene) : std::make_shared<const WorldSceneSnapshot>();
    _sceneDirty = true;
}

void WorldSceneRenderer::setFigures(std::vector<WorldFigureSnapshot> figures) {
    _figures = std::move(figures);
    _figuresDirty = true;
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

    const core::IsoProjection projection(_scene->columns, _scene->rows,
                                         core::ARENA_TILE_WIDTH_UNITS, _scene->diamondRatio);
    // La carte : ses textures et sa composition, une fois par carte. Ce sont ses pieces qui disent
    // quoi charger, et la carte change au passage d'un portail.
    if (_sceneDirty) {
        ensureTextures(worldTexturePaths(*_scene));
        _statics.build(*_scene, projection, _textures);
        _sceneDirty = false;
    }
    // Les figurines : leurs bandes, quand elles changent (une figurine neuve, une autre bande).
    if (_figuresDirty) {
        ensureTextures(worldFigureTexturePaths(*_scene, _figures));
        _figuresDirty = false;
    }

    const QSize pixels = target->pixelSize();
    const Camera2D camera = worldCamera(projection, projection.gridToWorld(_focus), pixels.width(),
                                        pixels.height(), _tilePixels);

    // L'image : ce que la camera montre de la carte, et les figurines.
    _composed.clear();
    _composed.setVisibleBounds(camera.visibleBounds());
    _statics.compose(_composed, _figures, _textures);

    SpriteBatch& sprites = _resources.sprites();
    sprites.beginFrame();
    submitComposedScene(sprites, camera.projectionMatrix(), _composed);
    sprites.submit(commandBuffer, target, updates, clear);
    _resources.setFrameUpdates(nullptr);
}

}  // namespace hmi
