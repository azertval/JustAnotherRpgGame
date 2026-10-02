// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/WorldSceneRenderer.h"

#include <algorithm>
#include <optional>
#include <utility>

#include <rhi/qrhi.h>

#include "Core/Resources/AssetMarker.h"
#include "Core/Resources/MeshFile.h"
#include "Core/Resources/SkeletonFile.h"
#include "HMI/Graphics/EntityMarkers.h"
#include "HMI/Graphics/GraphicsLog.h"
#include "HMI/Graphics/IsoView.h"
#include "HMI/Graphics/MaquetteTokens.h"
#include "HMI/Graphics/MeshBatch.h"
#include "HMI/Graphics/MissingTexture.h"
#include "HMI/Graphics/SceneTextureTraits.h"
#include "HMI/Graphics/SpriteBatch.h"
#include "HMI/Graphics/SpriteRenderer.h"

namespace hmi {

PlaceCamera worldCamera(const core::IsoProjection& projection, core::Vector2 focus, int pixelWidth,
                        int pixelHeight, float tilePixels) {
    const int width = std::max(1, pixelWidth);
    const int height = std::max(1, pixelHeight);
    PlaceCamera camera(width, height);

    // Un facteur LIBRE, fixe par la definition (EX-REN-013) : une case occupe la hauteur de la vue
    // divisee par 10,8. L'art est toujours reduit, jamais agrandi, et filtre par mipmaps : aucune
    // grille de pixels n'est plus a proteger.
    const float onScreen = tilePixels > 0.0F ? tilePixels : worldTilePixels(height);
    camera.setZoom(onScreen / (projection.tileWidth() * PlaceCamera::PIXELS_PER_UNIT));

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

PlaceCamera framedCamera(const WorldFraming& framing, int pixelWidth, int pixelHeight) {
    PlaceCamera camera(std::max(1, pixelWidth), std::max(1, pixelHeight));
    camera.setZoom(framing.pixelsPerUnit / PlaceCamera::PIXELS_PER_UNIT);
    camera.setCenter(framing.center);
    return camera;
}

core::Rect composedSceneBounds(const ComposedScene& scene, const core::Rect& base) {
    float left = base.position.x;
    float top = base.position.y;
    float right = base.position.x + base.size.x;
    float bottom = base.position.y + base.size.y;
    for (const ComposedQuad& quad : scene.quads()) {
        core::Rect bounds;
        switch (quad.kind) {
            case QuadKind::Sprite:
                bounds = spriteQuadBounds(quad.sprite);
                break;
            case QuadKind::Line:
                bounds = lineQuadBounds(quad.line);
                break;
            case QuadKind::Poly:
                bounds = polyQuadBounds(quad.poly);
                break;
        }
        left = std::min(left, bounds.position.x);
        top = std::min(top, bounds.position.y);
        right = std::max(right, bounds.position.x + bounds.size.x);
        bottom = std::max(bottom, bounds.position.y + bounds.size.y);
    }
    for (const ComposedMesh& mesh : scene.meshes()) {
        left = std::min(left, mesh.bounds.position.x);
        top = std::min(top, mesh.bounds.position.y);
        right = std::max(right, mesh.bounds.position.x + mesh.bounds.size.x);
        bottom = std::max(bottom, mesh.bounds.position.y + mesh.bounds.size.y);
    }
    return core::Rect{{left, top}, {right - left, bottom - top}};
}

namespace {

/// Ce que pese une texture `RGBA8` de @p width x @p height en memoire graphique ; lissee, elle
/// porte en plus sa chaine de mipmaps, un tiers de sa taille.
[[nodiscard]] std::size_t textureWeight(int width, int height, bool mipmapped) noexcept {
    const std::size_t pixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    const std::size_t bytes = pixels * 4U;
    return mipmapped ? bytes + (bytes / 3U) : bytes;
}

/// Applique @p opacity aux primitives de @p quads : l'alpha de chacune est multiplie, et celle que
/// l'appelant eteint (0 ou moins) est retiree. L'ordre de dessin ne change pas.
void applyQuadOpacity(std::vector<ComposedQuad>& quads, const WorldQuadOpacity& opacity) {
    std::erase_if(quads, [&opacity](ComposedQuad& quad) {
        const float extra = opacity(quad);
        if (extra <= 0.0F) {
            return true;
        }
        switch (quad.kind) {
            case QuadKind::Sprite:
                quad.sprite.a *= extra;
                break;
            case QuadKind::Line:
                quad.line.a *= extra;
                break;
            case QuadKind::Poly:
                quad.poly.a *= extra;
                break;
        }
        return false;
    });
}

/// La meme opacite pour les maillages : l'appelant la decide par calque et par etage, que la piece
/// soit une image ou un volume. Elle lui est demandee sur une primitive qui n'en porte que cela.
void applyMeshOpacity(std::vector<ComposedMesh>& meshes, const WorldQuadOpacity& opacity) {
    std::erase_if(meshes, [&opacity](ComposedMesh& mesh) {
        ComposedQuad proxy;
        proxy.layer = mesh.layer;
        proxy.storey = mesh.storey;
        const float extra = opacity(proxy);
        mesh.opacity *= extra;
        return extra <= 0.0F;
    });
}

}  // namespace

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
    _meshes = std::make_unique<MeshBatch>(rhi);
    _resources.setFrameUpdates(nullptr);
    GRAPHICS_LOG_INFO("Lieu : ressources QRhi creees (" + std::string(rhi->backendName()) + ").");
    return true;
}

std::size_t WorldSceneRenderer::textureBytes() const noexcept {
    return _textureBytes + (_meshes ? _meshes->bytes() : 0U);
}

void WorldSceneRenderer::ensureMeshes(const std::vector<std::string>& paths) {
    for (const std::string& path : paths) {
        // Ni deja tente : un maillage absent ne se redemande pas a chaque image.
        if (!_requested.insert(path).second) {
            continue;
        }
        const core::MeshFileResult read = core::readMeshFile(_directory / path);
        if (!read.ok()) {
            // La composition retombe sur le damier : une piece manquante se voit, sans planter.
            GRAPHICS_LOG_WARNING("Lieu : le maillage '" + path + "' ne se charge pas (" +
                                 read.message + ").");
            continue;
        }
        const MeshHandle handle = _meshes->create(_resources.context(), read.mesh);
        if (handle == nullptr) {
            GRAPHICS_LOG_WARNING("Lieu : le maillage '" + path + "' ne se cree pas sur le GPU.");
            continue;
        }
        // La hauteur d'un etage : ce que le manifeste de son dossier declare, rapporte a son
        // losange (LOT-129) -- la meme lecture que pour une image.
        const SceneTextureTraits traits = readSceneTextureTraits(_directory, path, &_manifests);
        SceneMesh& loaded = _textures.meshes[path] =
            SceneMesh{.mesh = handle, .minimum = read.mesh.minimum, .maximum = read.mesh.maximum};
        if (traits.storeyHeight && traits.artTile.x > 0.0F) {
            loaded.storeyTiles = *traits.storeyHeight / traits.artTile.x;
        }
        GRAPHICS_LOG_INFO("Lieu : maillage '" + path + "' charge (" +
                          std::to_string(read.mesh.triangleCount()) + " triangles).");
    }
}

void WorldSceneRenderer::ensureFigureModels(const std::vector<std::string>& paths) {
    for (const std::string& path : paths) {
        // Ni deja tente : un modele absent ne se redemande pas a chaque image.
        if (!_requested.insert(path).second) {
            continue;
        }
        core::MeshFileResult read = core::readMeshFile(_directory / path);
        if (!read.ok()) {
            // La composition retombe sur les bandes de la figurine, donc sur le damier : un modele
            // manquant se voit, sans planter.
            GRAPHICS_LOG_WARNING("Lieu : le modele '" + path + "' ne se charge pas (" +
                                 read.message + ").");
            continue;
        }
        const MeshHandle handle = _meshes->create(_resources.context(), read.mesh);
        if (handle == nullptr) {
            GRAPHICS_LOG_WARNING("Lieu : le modele '" + path + "' ne se cree pas sur le GPU.");
            continue;
        }
        SceneFigureModel& loaded = _textures.figures[path] = SceneFigureModel{
            .mesh = handle, .minimum = read.mesh.minimum, .maximum = read.mesh.maximum};
        const std::size_t triangles = read.mesh.triangleCount();
        const std::size_t clips = read.mesh.rig.clips.size();
        // Le squelette et ses clips ne valent que si le rendu sait les jouer : sinon le modele se
        // dessine dans sa pose de liaison (`MeshBatch::create` l'a deja dit).
        if (!read.mesh.skin.empty() && read.mesh.rig.joints.size() <= MeshBatch::MAX_BONES) {
            loaded.rig = std::make_shared<const core::MeshRig>(std::move(read.mesh.rig));
        }
        // Ce que le squelette declare de ses clips : la fiche du dossier dit lequel.
        const std::filesystem::path sheetFile =
            (_directory / path).parent_path() / core::CHARACTER_SHEET_FILE;
        if (const core::CharacterSheetFileResult sheet = core::readCharacterSheetFile(sheetFile);
            sheet.ok()) {
            auto known = _skeletons.find(sheet.sheet.skeleton);
            if (known == _skeletons.end()) {
                core::SkeletonFileResult skeleton = core::readSkeletonFile(
                    _directory / core::skeletonFilePath(sheet.sheet.skeleton));
                if (!skeleton.ok()) {
                    GRAPHICS_LOG_WARNING("Lieu : le squelette '" + sheet.sheet.skeleton +
                                         "' ne se lit pas (" + skeleton.message + ").");
                }
                known =
                    _skeletons
                        .emplace(sheet.sheet.skeleton,
                                 skeleton.ok() ? std::make_shared<const core::SkeletonDescription>(
                                                     std::move(skeleton.skeleton))
                                               : nullptr)
                        .first;
            }
            loaded.skeleton = known->second;
        }
        GRAPHICS_LOG_INFO("Lieu : modele '" + path + "' charge (" + std::to_string(triangles) +
                          " triangles, " + std::to_string(clips) + " clips).");
    }
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
                _textureBytes += textureWeight(painted->width, painted->height, false);
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
                _textureBytes += textureWeight(marqueur->width, marqueur->height, false);
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
        _textureBytes += textureWeight(texture->width, texture->height, true);
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
    _textures.meshes.clear();
    _textures.figures.clear();
    _skeletons.clear();
    _meshes.reset();
    _textures.missing = SceneTexture{};
    _textures.solid = SceneTexture{};
    _loaded.clear();
    _textureBytes = 0;
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

void WorldSceneRenderer::setComposeOptions(WorldComposeOptions options) noexcept {
    if (options == _composeOptions) {
        return;
    }
    _composeOptions = options;
    _sceneDirty = true;
}

core::IsoProjection WorldSceneRenderer::sceneProjection() const {
    return {_scene->columns, _scene->rows, core::ARENA_TILE_WIDTH_UNITS, _scene->diamondRatio};
}

void WorldSceneRenderer::refresh(const core::IsoProjection& projection) {
    // La carte : ses textures et sa composition, une fois par carte. Ce sont ses pieces qui disent
    // quoi charger, et la carte change au passage d'un portail.
    if (_sceneDirty) {
        ensureTextures(worldTexturePaths(*_scene));
        ensureMeshes(worldMeshPaths(*_scene));
        ensureFigureModels(worldFigureModelPaths(_scene->figures));
        _statics.build(*_scene, projection, _textures, _composeOptions);
        _sceneDirty = false;
    }
    // Les figurines : leurs bandes, quand elles changent (une figurine neuve, une autre bande).
    if (_figuresDirty) {
        ensureTextures(worldFigureTexturePaths(*_scene, _figures));
        ensureFigureModels(worldFigureModelPaths(_figures));
        _figuresDirty = false;
    }
}

bool WorldSceneRenderer::prepare() {
    if (!created()) {
        return false;
    }
    if (!_sceneDirty && !_figuresDirty) {
        return true;
    }
    // Hors image : les televersements attendent dans le lot que la prochaine image soumettra.
    if (_pendingUploads == nullptr) {
        _pendingUploads = _rhi->nextResourceUpdateBatch();
    }
    _resources.setFrameUpdates(_pendingUploads);
    refresh(sceneProjection());
    _resources.setFrameUpdates(nullptr);
    return true;
}

core::Rect WorldSceneRenderer::paintedBounds(const core::Rect& base) {
    if (!prepare()) {
        return base;
    }
    // Toute la carte et ses figurines, sans cadrage : ce qu'une image pourrait montrer.
    ComposedScene whole;
    _statics.compose(whole, _figures, _textures);
    return composedSceneBounds(whole, base);
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

    const core::IsoProjection projection = sceneProjection();
    refresh(projection);

    const QSize pixels = target->pixelSize();
    PlaceCamera camera = _framing ? framedCamera(*_framing, pixels.width(), pixels.height())
                                  : worldCamera(projection, projection.gridToWorld(_focus),
                                                pixels.width(), pixels.height(), _tilePixels);

    // L'image : ce que la camera montre de la carte, et les figurines.
    _composed.clear();
    _composed.setVisibleBounds(camera.visibleBounds());
    _statics.compose(_composed, _figures, _textures);
    if (_opacity) {
        // Les calques de l'editeur (LOT-1002) : la liste, deja triee, est retouchee en place.
        std::vector<ComposedQuad> quads;
        _composed.swapQuads(quads, 0, 0);
        applyQuadOpacity(quads, _opacity);
        _composed.swapQuads(quads, 0, 0);
        std::vector<ComposedMesh> meshes;
        _composed.swapMeshes(meshes);
        applyMeshOpacity(meshes, _opacity);
        _composed.swapMeshes(meshes);
    }

    // Les volumes (LOT-1003). Une image qui en a donne a chaque primitive sa profondeur, et la
    // camera ramene celle de toute la scene a l'etendue du tampon ; une image qui n'en a pas se
    // dessine comme avant le lot : profondeur nulle, ni test ni ecriture.
    const bool volumes = !_composed.meshes().empty();
    std::optional<SceneDepth> depth;
    if (volumes) {
        const IsoView view(projection);
        camera.setDepthRange(view.depthRange());
        depth = SceneDepth{.view = view, .range = camera.depthRange()};
    }

    SpriteBatch& sprites = _resources.sprites();
    sprites.setDepthTest(volumes);
    sprites.beginFrame();
    submitComposedScene(sprites, camera.projectionMatrix(), _composed, depth ? &*depth : nullptr);
    _meshes->beginFrame();
    for (const ComposedMesh& mesh : _composed.meshes()) {
        _meshes->draw(mesh.mesh, camera.meshMatrix(mesh.toView), mesh.opacity,
                      _composed.poseOf(mesh));
    }

    // Une seule passe : les televersements d'abord, puis les maillages, qui ecrivent la profondeur,
    // et les images, qui la testent dans l'ordre du peintre.
    QRhiResourceUpdateBatch* const uploads =
        _meshes->prepare(target, sprites.prepare(target, updates));
    commandBuffer->beginPass(target, QColor::fromRgbF(clear[0], clear[1], clear[2], clear[3]),
                             {1.0F, 0}, uploads);
    _meshes->record(commandBuffer, target);
    sprites.record(commandBuffer, target);
    commandBuffer->endPass();
    _resources.setFrameUpdates(nullptr);
}

}  // namespace hmi
