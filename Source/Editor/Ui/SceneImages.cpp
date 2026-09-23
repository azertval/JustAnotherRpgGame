// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/SceneImages.h"

#include <algorithm>
#include <utility>

#include "Core/Ecs/Components/Sprite.h"  // core::AtlasRegion
#include "Core/Resources/AssetMarker.h"
#include "HMI/Graphics/EntityMarkers.h"
#include "HMI/Graphics/MaquettePalette.h"
#include "HMI/Graphics/MaquetteTokens.h"
#include "HMI/Graphics/MissingTexture.h"
#include "HMI/Graphics/ProceduralAtlas.h"
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

[[nodiscard]] std::size_t bytesOf(const QImage& image) noexcept {
    return image.isNull() ? 0 : static_cast<std::size_t>(image.sizeInBytes());
}

}  // namespace

QImage SceneImage::level(int level) {
    if (_levels.empty() || _levels.front().isNull()) {
        if (_owner == nullptr || _path.empty()) {
            return {};
        }
        QImage full = _owner->readFile(_path);
        if (full.isNull()) {
            return {};
        }
        _levels.assign(1, std::move(full));
        _owner->account(static_cast<std::ptrdiff_t>(bytesOf(_levels.front())));
    }
    const int wanted = std::clamp(level, 0, levelCount() - 1);
    // Chaque niveau se tire du précédent, réduit de moitié : la moyenne de quatre texels, ce que
    // fait la génération de mipmaps du GPU.
    while (static_cast<int>(_levels.size()) <= wanted) {
        const QImage& previous = _levels.back();
        QImage next =
            previous.scaled(std::max(1, previous.width() / 2), std::max(1, previous.height() / 2),
                            Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        _levels.push_back(std::move(next));
        if (_owner != nullptr && !_path.empty()) {
            _owner->account(static_cast<std::ptrdiff_t>(bytesOf(_levels.back())));
        }
    }
    QImage result = _levels[static_cast<std::size_t>(wanted)];
    if (_owner != nullptr && !_path.empty()) {
        _owner->touch(*this);
    }
    return result;
}

int SceneImage::levelCount() const noexcept {
    if (!_smooth) {
        return 1;
    }
    int count = 1;
    for (int side = std::max(_width, _height); side > 1; side /= 2) {
        ++count;
    }
    return count;
}

SceneImages::SceneImages(std::filesystem::path assetsDirectory, std::size_t budgetBytes)
    : _directory(std::move(assetsDirectory)), _budget(budgetBytes) {
    const ProceduralAtlasImage checker = buildMissingTextureImage();
    pin(_missing, fromRgba8(checker.width, checker.height, checker.pixels));
    _textures.missing = SceneTexture{.texture = sceneImageHandle(&_missing),
                                     .width = _missing.width(),
                                     .height = _missing.height()};
    // L'aplat du rendu de maquette (LOT-128) : l'image statique, jamais rechargee.
    _textures.solid = SceneTexture{.texture = solid(), .width = 1, .height = 1};
    const ProceduralAtlasImage atlas = buildProceduralAtlasImage();
    pin(_atlas, fromRgba8(atlas.width, atlas.height, atlas.pixels));
}

std::shared_ptr<SceneImages> SceneImages::shared(const std::filesystem::path& assetsDirectory) {
    // Le fil de l'interface seul : aucun verrou (voir la classe).
    static std::map<std::filesystem::path, std::weak_ptr<SceneImages>> instances;
    std::error_code error;
    std::filesystem::path key = std::filesystem::weakly_canonical(assetsDirectory, error);
    if (error) {
        key = assetsDirectory.lexically_normal();
    }
    if (std::shared_ptr<SceneImages> alive = instances[key].lock()) {
        return alive;
    }
    auto created = std::make_shared<SceneImages>(assetsDirectory);
    instances[key] = created;
    return created;
}

TextureHandle SceneImages::solid() noexcept {
    static const SceneImage solidImage = [] {
        SceneImage image;
        QImage white(1, 1, QImage::Format_ARGB32_Premultiplied);
        white.fill(Qt::white);
        image._width = 1;
        image._height = 1;
        image._solid = true;
        image._levels.assign(1, std::move(white));
        return image;
    }();
    return sceneImageHandle(&solidImage);
}

QColor SceneImages::tileColor(core::TileType type) {
    // La palette de maquette, et non le pixel central de l'atlas procedural (LOT-128, decision
    // D5) : la vignette de la palette montre desormais la couleur que la case prendra vraiment.
    const MaquetteColor tint = maquetteColor(type);
    return QColor::fromRgbF(tint.r, tint.g, tint.b);
}

const SceneImage* SceneImages::marker(const std::string& key) {
    const auto found = _markers.find(key);
    if (found != _markers.end()) {
        return found->second.pinned().isNull() ? nullptr : &found->second;
    }
    const core::MarkerImage image =
        core::assetMarker(key, ENTITY_MARKER_SIZE_PIXELS, ENTITY_MARKER_SIZE_PIXELS);
    SceneImage& stored = _markers[key];
    pin(stored, image.isEmpty() ? QImage{}
                                : fromRgba8(image.width, image.height, markerPixelsRgba8(image)));
    return stored.pinned().isNull() ? nullptr : &stored;
}

SceneImage* SceneImages::image(const std::string& path) {
    ensure({path});
    const auto found = _images.find(path);
    return found == _images.end() ? nullptr : &found->second;
}

void SceneImages::pin(SceneImage& target, QImage image) {
    target._width = image.width();
    target._height = image.height();
    target._levels.assign(1, std::move(image));
}

QImage SceneImages::readFile(const std::string& path) const {
    QImage loaded(QString::fromStdWString((_directory / path).wstring()));
    return loaded.isNull() ? loaded : loaded.convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

void SceneImages::account(std::ptrdiff_t bytes) {
    _resident = static_cast<std::size_t>(static_cast<std::ptrdiff_t>(_resident) + bytes);
}

void SceneImages::touch(SceneImage& image) {
    if (image._listed) {
        _recent.splice(_recent.begin(), _recent, image._recent);
    } else {
        _recent.push_front(&image);
        image._recent = _recent.begin();
        image._listed = true;
    }
    evictBeyondBudget(image);
}

void SceneImages::evictBeyondBudget(const SceneImage& keep) {
    // La moins récemment peinte d'abord ; jamais celle qu'on est en train de peindre.
    while (_resident > _budget && !_recent.empty() && _recent.back() != &keep) {
        SceneImage* const oldest = _recent.back();
        _recent.pop_back();
        oldest->_listed = false;
        for (const QImage& level : oldest->_levels) {
            account(-static_cast<std::ptrdiff_t>(bytesOf(level)));
        }
        oldest->_levels.clear();
    }
}

void SceneImages::ensure(const std::vector<std::string>& paths) {
    for (const std::string& path : paths) {
        if (!_requested.insert(path).second) {
            continue;
        }
        // Un jeton n'est pas un fichier : il se peint (LOT-128, decision D2). La meme image, au
        // pixel pres, que celle que le jeu televerse.
        if (const core::MarkerImage token = maquetteTokenImage(path, MAQUETTE_TOKEN_SIZE_PIXELS);
            !token.isEmpty()) {
            SceneImage& stored = _images[path];
            pin(stored, fromRgba8(token.width, token.height, markerPixelsRgba8(token)));
            _textures.byPath[path] = SceneTexture{.texture = sceneImageHandle(&stored),
                                                  .width = stored.width(),
                                                  .height = stored.height()};
            continue;
        }
        QImage loaded = readFile(path);
        if (!loaded.isNull()) {
            // L'art peint : lissé et réduit par niveaux, comme le GPU (`TextureFiltering::Smooth`),
            // et ses pixels comptés au budget.
            SceneImage& stored = _images[path];
            stored._owner = this;
            stored._path = path;
            stored._smooth = true;
            stored._width = loaded.width();
            stored._height = loaded.height();
            stored._levels.assign(1, std::move(loaded));
            account(static_cast<std::ptrdiff_t>(bytesOf(stored._levels.front())));
            touch(stored);
            // Decoupe, echelle et ancre : les memes traits que le jeu lit (LOT-103), chaque
            // manifeste lu une fois (LOT-125).
            SceneTexture& texture = _textures.byPath[path] =
                SceneTexture{.texture = sceneImageHandle(&stored),
                             .width = stored.width(),
                             .height = stored.height()};
            applySceneTextureTraits(texture, readSceneTextureTraits(_directory, path, &_manifests));
            continue;
        }
        // Une figurine sans image se dessine par son marqueur, comme en jeu (LOT-39, LOT-96).
        const std::string key = figureMarkerKey(path);
        if (!key.empty()) {
            const core::MarkerImage marker =
                core::assetMarker(key, FIGURE_MARKER_WIDTH_PIXELS, FIGURE_MARKER_HEIGHT_PIXELS);
            if (!marker.isEmpty()) {
                SceneImage& stored = _images[path];
                pin(stored, fromRgba8(marker.width, marker.height, markerPixelsRgba8(marker)));
                _textures.byPath[path] = SceneTexture{.texture = sceneImageHandle(&stored),
                                                      .width = stored.width(),
                                                      .height = stored.height(),
                                                      .frameWidth = stored.width()};
                continue;
            }
        }
        // La composition retombe sur le damier : une pièce manquante se voit, sans planter.
        HMI_LOG_WARNING("Editeur : image introuvable, damier a la place : " + path);
    }
}

}  // namespace hmi
