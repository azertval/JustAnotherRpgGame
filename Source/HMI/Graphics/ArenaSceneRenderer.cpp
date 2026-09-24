// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/ArenaSceneRenderer.h"

#include <algorithm>
#include <optional>
#include <utility>

#include <rhi/qrhi.h>

#include "Core/Combat/Arena.h"
#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/World/CombatZone.h"
#include "HMI/Graphics/Camera2D.h"
#include "HMI/Graphics/GraphicsLog.h"
#include "HMI/Graphics/MissingTexture.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/SceneTextureTraits.h"
#include "HMI/Graphics/SpriteBatch.h"
#include "HMI/Graphics/SpriteRenderer.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace hmi {

namespace {

/// Marge du cadrage : la scène entière tient dans la surface, sans toucher ses bords.
constexpr float FRAMING_MARGIN = 0.95F;

}  // namespace

Camera2D arenaCamera(const core::IsoProjection& projection, int pixelWidth, int pixelHeight) {
    const int width = std::max(1, pixelWidth);
    const int height = std::max(1, pixelHeight);
    const core::Vector2 size = projection.sceneSize();
    Camera2D camera(width, height);
    camera.setCenter({size.x / 2.0F, size.y / 2.0F});
    camera.setZoom(Camera2D::fitZoom(static_cast<float>(width), static_cast<float>(height),
                                     std::max(size.x, 1.0F), std::max(size.y, 1.0F),
                                     FRAMING_MARGIN));
    return camera;
}

ArenaSceneRenderer::ArenaSceneRenderer(std::filesystem::path coliseumDirectory, bool productionMap)
    : _directory(std::move(coliseumDirectory)) {
    if (productionMap) {
        loadBattlefield();
    }
    // Lectures de fichiers, une fois : ni le catalogue ni les clips ne touchent au GPU, et une
    // recréation des ressources n'a pas à les relire.
    ArenaAppearanceCatalogResult catalog =
        ArenaAppearanceCatalog::loadFromFile(_directory / "manifest.json");
    if (catalog.ok()) {
        _catalog = std::move(*catalog.catalog);
    } else {
        GRAPHICS_LOG_WARNING("Arene : manifeste du Colisee illisible, " + catalog.error);
    }
    // Les PNJ de l'atelier (LOT-91) vivent a cote du Colisee ; leur manifeste peut mettre l'un
    // d'eux a la place d'un heros de la planche de production.
    const int replaced =
        _catalog.applyNpcManifest(_directory.parent_path() / "Npc" / "manifest.json");
    if (replaced > 0) {
        GRAPHICS_LOG_INFO("Arene : " + std::to_string(replaced) + " heros remplace(s) par un PNJ.");
    }

    const auto declare = [this](const std::string& sheet, core::CombatSide side) {
        const std::string directory = _catalog.sheetDirectory(sheet, side);
        ArenaFigureAnimationLoad load = loadArenaFigureAnimations(_directory / directory);
        for (const std::string& error : load.errors) {
            std::string message = "Arene : animation de " + sheet;
            message += ", ";
            message += error;
            GRAPHICS_LOG_WARNING(message);
        }
        _animation.setFigureAnimations(sheet, std::move(load.clips));
    };
    for (const std::string& hero : _catalog.heroes()) {
        declare(hero, core::CombatSide::Allies);
    }
    for (const std::string& gladiator : _catalog.gladiators()) {
        declare(gladiator, core::CombatSide::Enemies);
    }
}

void ArenaSceneRenderer::loadBattlefield() {
    const auto data = _directory.parent_path().parent_path();
    // La premiere arene qui nomme sa carte et sa zone : aucun nom de contenu n'est ecrit dans le
    // moteur (LOT-102). Sans arene jouable, le combat se joue sans decor -- ce n'est pas une
    // panne (`EX-NFR-040`).
    const core::ArenaCatalog arenas = core::loadArenas(data / "World" / "arena");
    const core::Arena* definition = nullptr;
    for (const core::Arena& arena : arenas.arenas) {
        if (!arena.map.empty() && !arena.zone.empty()) {
            definition = &arena;
            break;
        }
    }
    if (definition == nullptr) {
        GRAPHICS_LOG_WARNING("Arena battlefield: aucune arene ne nomme une carte et sa zone.");
        return;
    }
    loadBattlefieldFrom(*definition);
}

void ArenaSceneRenderer::loadBattlefieldFrom(const core::Arena& definition) {
    const auto data = _directory.parent_path().parent_path();
    const auto level = core::LevelLoader::loadFromFile(data / "Levels" / definition.map);
    if (!level.ok()) {
        GRAPHICS_LOG_WARNING("Arena battlefield: " + level.error);
        return;
    }
    const auto zones = core::combatZonesOf(*level.level);
    const auto* zone = core::findCombatZone(zones, definition.zone);
    if (zone == nullptr) {
        GRAPHICS_LOG_WARNING("Arena battlefield: unknown combat zone");
        return;
    }
    // Le decor est celui que la carte declare, pas un lieu ecrit dans le moteur.
    const auto appearance =
        PlaceAppearance::loadForPlace(_directory.parent_path(), scenePlaceOf(*level.level));
    if (!appearance.ok()) {
        GRAPHICS_LOG_WARNING("Arena battlefield appearance: " + appearance.message);
        return;
    }
    _battlefield = snapshotWorldScene(*level.level, appearance.appearance, {});
    _battlefieldOrigin = zone->origin;
}

ArenaSceneRenderer::~ArenaSceneRenderer() {
    release();
}

bool ArenaSceneRenderer::ensureResources(QRhi* rhi) {
    if (rhi == nullptr) {
        return false;
    }
    if (created() && _rhi == rhi) {
        return true;
    }
    // Une autre interface : tout ce qui a été créé appartient à l'ancienne et ne doit plus servir.
    release();

    _rhi = rhi;
    _pendingUploads = rhi->nextResourceUpdateBatch();
    _resources.create(rhi, _pendingUploads);
    loadTextures();
    _resources.setFrameUpdates(nullptr);
    GRAPHICS_LOG_INFO("Arene : ressources QRhi creees (" + std::string(rhi->backendName()) + ", " +
                      std::to_string(_textures.byPath.size()) + " textures).");
    return true;
}

void ArenaSceneRenderer::loadTextures() {
    const RhiContext& context = _resources.context();

    const ProceduralAtlasImage checker = buildMissingTextureImage();
    if (std::optional<LoadedTexture> missing =
            createTexture(context, checker.width, checker.height, checker.pixels)) {
        _missing = std::move(*missing);
        _textures.missing = ArenaTexture{
            .texture = _missing.handle(), .width = _missing.width, .height = _missing.height};
    }

    const std::vector<std::string> paths = arenaTexturePaths(_catalog);
    _loaded.reserve(paths.size());
    for (const std::string& path : paths) {
        std::optional<LoadedTexture> texture = loadTextureFromFile(context, _directory / path);
        if (!texture.has_value()) {
            // La composition retombe sur le damier : une pièce manquante se voit, sans planter.
            GRAPHICS_LOG_WARNING(missingTextureWarning(path));
            continue;
        }
        // Decoupe, echelle et ancre : ce que ses fichiers voisins disent de l'image (LOT-103).
        ArenaTexture& loaded = _textures.byPath[path] = ArenaTexture{
            .texture = texture->handle(), .width = texture->width, .height = texture->height};
        applySceneTextureTraits(loaded, readSceneTextureTraits(_directory, path, &_manifests));
        _loaded.push_back(std::move(*texture));
    }
    if (_battlefield) {
        loadBattlefieldTextures(context);
    }
}

void ArenaSceneRenderer::loadBattlefieldTextures(const RhiContext& context) {
    for (const auto& path : worldTexturePaths(*_battlefield)) {
        const auto file = _directory.parent_path() / path;
        auto texture = loadTextureFromFile(context, file);
        if (!texture) {
            GRAPHICS_LOG_WARNING(missingTextureWarning(path));
            continue;
        }
        SceneTexture descriptor{
            .texture = texture->handle(), .width = texture->width, .height = texture->height};
        applySceneTextureTraits(
            descriptor, readSceneTextureTraits(_directory.parent_path(), path, &_manifests));
        _battlefieldTextures.byPath[path] = descriptor;
        _loaded.push_back(std::move(*texture));
    }
    _battlefieldTextures.missing = {
        .texture = _missing.handle(), .width = _missing.width, .height = _missing.height};
    composeWorldScene(_battlefieldScene, *_battlefield,
                      core::IsoProjection(_battlefield->columns, _battlefield->rows,
                                          core::ARENA_TILE_WIDTH_UNITS, _battlefield->diamondRatio),
                      _battlefieldTextures);
}

void ArenaSceneRenderer::release() noexcept {
    // L'ordre : ce qui désigne une texture, puis les textures, puis la grappe qui porte le
    // pipeline. Le lot de création jamais soumis est rendu à QRhi, qui en garde un nombre borné.
    _composed.clear();
    _battlefieldScene.clear();
    _battlefieldTextures.byPath.clear();
    _battlefieldTextures.missing = {};
    _textures.byPath.clear();
    _textures.missing = ArenaTexture{};
    _loaded.clear();
    _missing = LoadedTexture{};
    if (_pendingUploads != nullptr) {
        _pendingUploads->release();
        _pendingUploads = nullptr;
    }
    _resources.release();
    _rhi = nullptr;
}

void ArenaSceneRenderer::setSnapshot(ArenaSceneSnapshot snapshot) {
    _snapshot = std::move(snapshot);

    std::set<core::CombatantId> present;
    for (const ArenaFigureSnapshot& figure : _snapshot.figures) {
        present.insert(figure.id);
        if (!_animated.contains(figure.id)) {
            // Seulement à l'arrivée : rejouer le repos à chaque instantané couperait une action en
            // cours. Déclencher attaque et coup d'après les événements est l'affaire de l'appelant.
            _animation.play(figure.id, _catalog.figureFor(figure.name, figure.side).sheet,
                            ArenaFigureAction::Idle);
        }
    }
    for (const core::CombatantId id : _animated) {
        if (!present.contains(id)) {
            _animation.remove(id);
        }
    }
    _animated = std::move(present);
}

void ArenaSceneRenderer::render(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target,
                                float realDeltaSeconds, const float* clear) {
    if (!created() || commandBuffer == nullptr || target == nullptr) {
        return;
    }
    // Le lot de cette image : celui de la création s'il attend encore, sinon un neuf. `submit` le
    // soumet avec l'ouverture de sa passe, qui en prend la propriété.
    QRhiResourceUpdateBatch* const updates = _pendingUploads != nullptr
                                                 ? std::exchange(_pendingUploads, nullptr)
                                                 : _rhi->nextResourceUpdateBatch();
    _resources.setFrameUpdates(updates);

    _animation.advance(std::max(0.0F, realDeltaSeconds));
    const ArenaAnimationState animation = _animation.snapshot();
    const core::IsoProjection projection(
        _snapshot.columns, _snapshot.rows, core::ARENA_TILE_WIDTH_UNITS,
        _battlefield ? _battlefield->diamondRatio : core::ARENA_DIAMOND_RATIO);

    _composed.clear();
    if (_battlefield) {
        const core::IsoProjection full(_battlefield->columns, _battlefield->rows,
                                       projection.tileWidth(), _battlefield->diamondRatio);
        const auto origin = full.gridToWorld({static_cast<float>(_battlefieldOrigin.column),
                                              static_cast<float>(_battlefieldOrigin.row)});
        const auto local = projection.gridToWorld({0, 0});
        const core::Vector2 delta{local.x - origin.x, local.y - origin.y};
        for (const auto& quad : _battlefieldScene.quads()) {
            auto sprite = quad.sprite;
            sprite.x += delta.x;
            sprite.y += delta.y;
            const auto order =
                quad.layer == RenderLayer::Object
                    ? ((quad.sortOrder / WORLD_DEPTH_SLOTS) + depthSortOrder(delta.y)) *
                          ARENA_DEPTH_SLOTS
                    : quad.sortOrder;
            _composed.addSprite(quad.layer, quad.texture, order, sprite);
        }
    }
    composeArenaScene(_composed, _snapshot, _catalog, animation, projection, _textures,
                      !_battlefield);
    _composed.sort();

    // Cadrage : la scène entière, centrée. La projection isométrique a déjà placé les pièces en
    // unités monde ; la caméra ne fait que déplacer et agrandir, en aval.
    const QSize pixels = target->pixelSize();
    const Camera2D camera = arenaCamera(projection, pixels.width(), pixels.height());

    SpriteBatch& sprites = _resources.sprites();
    sprites.beginFrame();
    submitComposedScene(sprites, camera.projectionMatrix(), _composed);
    sprites.submit(commandBuffer, target, updates, clear);
    _resources.setFrameUpdates(nullptr);
}

}  // namespace hmi
