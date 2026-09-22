// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/SceneImages.h"

#include <utility>

#include "Core/Ecs/Components/Sprite.h"  // core::AtlasRegion
#include "Core/Resources/AssetMarker.h"
#include "HMI/Graphics/AnimationCatalog.h"
#include "HMI/Graphics/EntityMarkers.h"
#include "HMI/Graphics/MaquettePalette.h"
#include "HMI/Graphics/MaquetteTokens.h"
#include "HMI/Graphics/MissingTexture.h"
#include "HMI/Graphics/ProceduralAtlas.h"
#include "HMI/Graphics/ScenePiecePlacement.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/HmiLog.h"

namespace hmi {

namespace {

/// Des pixels `R8G8B8A8` en image prête à peindre (prémultipliée : le format rapide de QPainter).
[[nodiscard]] QImage fromRgba8(int width, int height, const std::vector<std::uint32_t>& pixels) {
    if (width <= 0 || height <= 0 ||
        pixels.size() < static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {
        return {};
    }
    // Les pixels sont lus en place puis copiés : l'image rendue ne dépend pas du vecteur.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* const bytes = reinterpret_cast<const uchar*>(pixels.data());
    return QImage(bytes, width, height, static_cast<qsizetype>(width) * 4, QImage::Format_RGBA8888)
        .convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

[[nodiscard]] const QImage& solidImage() {
    static const QImage solid = [] {
        QImage image(1, 1, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::white);
        return image;
    }();
    return solid;
}

/// Le chemin du `.anim.json` d'une bande : `idle.png` -> `idle.anim.json`.
[[nodiscard]] std::filesystem::path animationDescriptionOf(const std::filesystem::path& band) {
    std::filesystem::path description = band;
    description.replace_extension();
    description += ".anim.json";
    return description;
}

[[nodiscard]] SceneTexture textureOf(const QImage& image, int frameWidth) {
    return SceneTexture{.texture = sceneImageHandle(&image),
                        .width = image.width(),
                        .height = image.height(),
                        .frameWidth = frameWidth};
}

}  // namespace

SceneImages::SceneImages(std::filesystem::path assetsDirectory)
    : _directory(std::move(assetsDirectory)) {
    const ProceduralAtlasImage checker = buildMissingTextureImage();
    _missing = fromRgba8(checker.width, checker.height, checker.pixels);
    _textures.missing = textureOf(_missing, 0);
    // L'aplat du rendu de maquette (LOT-128) : l'image statique, jamais rechargee.
    _textures.solid = textureOf(solidImage(), 0);
    const ProceduralAtlasImage atlas = buildProceduralAtlasImage();
    _atlas = fromRgba8(atlas.width, atlas.height, atlas.pixels);
}

TextureHandle SceneImages::solid() noexcept {
    return sceneImageHandle(&solidImage());
}

QColor SceneImages::tileColor(core::TileType type) const {
    // La palette de maquette, et non le pixel central de l'atlas procedural (LOT-128, decision
    // D5) : la vignette de la palette montre desormais la couleur que la case prendra vraiment.
    const MaquetteColor tint = maquetteColor(type);
    return QColor::fromRgbF(tint.r, tint.g, tint.b);
}

const QImage* SceneImages::marker(const std::string& key) {
    const auto found = _markers.find(key);
    if (found != _markers.end()) {
        return found->second.isNull() ? nullptr : &found->second;
    }
    const core::MarkerImage image =
        core::assetMarker(key, ENTITY_MARKER_SIZE_PIXELS, ENTITY_MARKER_SIZE_PIXELS);
    QImage& stored = _markers[key];
    if (!image.isEmpty()) {
        stored = fromRgba8(image.width, image.height, markerPixelsRgba8(image));
    }
    return stored.isNull() ? nullptr : &stored;
}

const QImage* SceneImages::image(const std::string& path) {
    ensure({path});
    const auto found = _images.find(path);
    return found == _images.end() ? nullptr : &found->second;
}

int SceneImages::bandFrameWidth(const std::string& path) const {
    // Seules les figurines ont un `.anim.json` ; une pièce de décor n'en a pas.
    const AnimationDescriptionResult read =
        AnimationCatalog::loadFromFile(animationDescriptionOf(_directory / path));
    return read.ok() ? read.description->frameWidth : 0;
}

void SceneImages::ensure(const std::vector<std::string>& paths) {
    for (const std::string& path : paths) {
        if (!_requested.insert(path).second) {
            continue;
        }
        // Un jeton n'est pas un fichier : il se peint (LOT-128, decision D2). La meme image, au
        // pixel pres, que celle que le jeu televerse.
        if (const core::MarkerImage token =
                maquetteTokenImage(path, MAQUETTE_TOKEN_SIZE_PIXELS);
            !token.isEmpty()) {
            QImage& stored = _images[path] =
                fromRgba8(token.width, token.height, markerPixelsRgba8(token));
            _textures.byPath[path] = textureOf(stored, 0);
            continue;
        }
        QImage loaded(QString::fromStdWString((_directory / path).wstring()));
        if (!loaded.isNull()) {
            QImage& stored = _images[path] =
                loaded.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            _textures.byPath[path] = textureOf(stored, bandFrameWidth(path));
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
            continue;
        }
        // Une figurine sans image se dessine par son marqueur, comme en jeu (LOT-39, LOT-96).
        const std::string key = figureMarkerKey(path);
        if (!key.empty()) {
            const core::MarkerImage marker =
                core::assetMarker(key, FIGURE_FRAME_WIDTH_PIXELS, FIGURE_FRAME_HEIGHT_PIXELS);
            if (!marker.isEmpty()) {
                QImage& stored = _images[path] =
                    fromRgba8(marker.width, marker.height, markerPixelsRgba8(marker));
                _textures.byPath[path] = textureOf(stored, stored.width());
                continue;
            }
        }
        // La composition retombe sur le damier : une pièce manquante se voit, sans planter.
        HMI_LOG_WARNING("Editeur : image introuvable, damier a la place : " + path);
    }
}

}  // namespace hmi
