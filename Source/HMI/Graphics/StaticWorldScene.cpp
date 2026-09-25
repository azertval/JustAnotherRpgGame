// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/StaticWorldScene.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <numeric>
#include <utility>

namespace hmi {

namespace {

// La boite d'une primitive composee, quelle que soit sa nature.
[[nodiscard]] core::Rect boundsOf(const ComposedQuad& quad) noexcept {
    switch (quad.kind) {
        case QuadKind::Sprite:
            return spriteQuadBounds(quad.sprite);
        case QuadKind::Line:
            return lineQuadBounds(quad.line);
        case QuadKind::Poly:
            return polyQuadBounds(quad.poly);
    }
    return {};
}

void seeThrough(ComposedQuad& quad) noexcept {
    switch (quad.kind) {
        case QuadKind::Sprite:
            quad.sprite.a *= STOREY_SEE_THROUGH_OPACITY;
            break;
        case QuadKind::Poly:
            quad.poly.a *= STOREY_SEE_THROUGH_OPACITY;
            break;
        case QuadKind::Line:
            quad.line.a *= STOREY_SEE_THROUGH_OPACITY;
            break;
    }
}

}  // namespace

bool hidesHero(const ComposedQuad& quad, const std::optional<WorldHeroPlacement>& hero) noexcept {
    return hero && quad.storey > 0 && quad.occlusion.size.x > 0.0F &&
           quad.sortOrder > hero->sortOrder && quad.occlusion.intersects(hero->bounds);
}

void fadeStoreysOverHero(std::span<ComposedQuad> quads,
                         const std::optional<WorldHeroPlacement>& hero) noexcept {
    if (!hero) {
        return;
    }
    for (ComposedQuad& quad : quads) {
        if (hidesHero(quad, hero)) {
            seeThrough(quad);
        }
    }
}

void StaticWorldScene::clear() noexcept {
    _figureContext = WorldSceneSnapshot{};
    _projection = core::IsoProjection{0, 0};
    _scene.clear();
    _bounds.clear();
    _buckets.clear();
    _bucketColumns = 0;
    _bucketRows = 0;
    _stamps.clear();
    _frame = 0;
    _indexed = false;
}

void StaticWorldScene::build(const WorldSceneSnapshot& snapshot,
                             const core::IsoProjection& projection,
                             const ScenePieceTextures& textures, WorldComposeOptions options) {
    clear();
    _projection = projection;
    // Les figurines ne lisent de l'instantane que leur lieu et leurs dossiers : on ne garde
    // qu'eux, pas les dizaines de milliers de noms de pieces.
    _figureContext.place = snapshot.place;
    _figureContext.figureDirectories = snapshot.figureDirectories;

    composeWorldStatics(_scene, snapshot, projection, textures, options);
    _scene.sort();

    _indexed = false;
}

void StaticWorldScene::index() const {
    // La grille ne sert qu'a une image cadree : une composition complete (le canevas de l'editeur,
    // une capture) ne la paie pas.
    _indexed = true;
    const std::vector<ComposedQuad>& quads = _scene.quads();
    _bounds.reserve(quads.size());
    float left = std::numeric_limits<float>::infinity();
    float top = std::numeric_limits<float>::infinity();
    float right = -std::numeric_limits<float>::infinity();
    float bottom = -std::numeric_limits<float>::infinity();
    for (const ComposedQuad& quad : quads) {
        const core::Rect bounds = boundsOf(quad);
        _bounds.push_back(bounds);
        left = std::min(left, bounds.position.x);
        top = std::min(top, bounds.position.y);
        right = std::max(right, bounds.position.x + bounds.size.x);
        bottom = std::max(bottom, bounds.position.y + bounds.size.y);
    }
    _stamps.assign(quads.size(), 0U);
    _frame = 0;
    if (quads.empty() || !std::isfinite(left) || !std::isfinite(top) || !std::isfinite(right) ||
        !std::isfinite(bottom)) {
        return;
    }

    // La grille couvre tout ce qui est peint, reliefs hauts compris.
    _origin = {left, top};
    _bucketSize = std::max(1.0F, BUCKET_TILES * _projection.tileWidth());
    _bucketColumns = std::max(1, static_cast<int>(std::ceil((right - left) / _bucketSize)));
    _bucketRows = std::max(1, static_cast<int>(std::ceil((bottom - top) / _bucketSize)));
    _buckets.assign(
        static_cast<std::size_t>(_bucketColumns) * static_cast<std::size_t>(_bucketRows), {});
    const auto bucketOf = [this](float value, float origin, int count) {
        return std::clamp(static_cast<int>(std::floor((value - origin) / _bucketSize)), 0,
                          count - 1);
    };
    for (std::size_t index = 0; index < quads.size(); ++index) {
        const core::Rect& bounds = _bounds[index];
        const int firstColumn = bucketOf(bounds.position.x, _origin.x, _bucketColumns);
        const int lastColumn =
            bucketOf(bounds.position.x + bounds.size.x, _origin.x, _bucketColumns);
        const int firstRow = bucketOf(bounds.position.y, _origin.y, _bucketRows);
        const int lastRow = bucketOf(bounds.position.y + bounds.size.y, _origin.y, _bucketRows);
        for (int row = firstRow; row <= lastRow; ++row) {
            for (int column = firstColumn; column <= lastColumn; ++column) {
                _buckets[(static_cast<std::size_t>(row) *
                          static_cast<std::size_t>(_bucketColumns)) +
                         static_cast<std::size_t>(column)]
                    .push_back(static_cast<std::uint32_t>(index));
            }
        }
    }
}

void StaticWorldScene::visibleIndices(const core::Rect& bounds,
                                      std::vector<std::uint32_t>& indices) const {
    if (!_indexed) {
        index();
    }
    if (_buckets.empty()) {
        return;
    }
    // Une nouvelle marque par image ; au tour du compteur, toutes les marques repartent de zero.
    if (++_frame == 0) {
        std::ranges::fill(_stamps, 0U);
        _frame = 1;
    }
    const auto range = [this](float from, float to, float origin, int count) {
        const int first = static_cast<int>(std::floor((from - origin) / _bucketSize));
        const int last = static_cast<int>(std::floor((to - origin) / _bucketSize));
        return std::pair{std::max(0, first), std::min(count - 1, last)};
    };
    const auto [firstColumn, lastColumn] =
        range(bounds.position.x, bounds.position.x + bounds.size.x, _origin.x, _bucketColumns);
    const auto [firstRow, lastRow] =
        range(bounds.position.y, bounds.position.y + bounds.size.y, _origin.y, _bucketRows);
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            const std::vector<std::uint32_t>& bucket =
                _buckets[(static_cast<std::size_t>(row) *
                          static_cast<std::size_t>(_bucketColumns)) +
                         static_cast<std::size_t>(column)];
            for (const std::uint32_t index : bucket) {
                if (_stamps[index] == _frame) {
                    continue;
                }
                _stamps[index] = _frame;
                if (_bounds[index].intersects(bounds)) {
                    indices.push_back(index);
                }
            }
        }
    }
    // Les indices croissants sont l'ordre de dessin : la partie fixe est deja triee.
    std::ranges::sort(indices);
}

void StaticWorldScene::compose(ComposedScene& out, std::span<const WorldFigureSnapshot> figures,
                               const ScenePieceTextures& textures) const {
    // Ce que l'appelant avait deja compose : garde tel quel, devant, et c'est lui qui retriera.
    std::vector<ComposedQuad> existing;
    out.swapQuads(existing, 0, 0);

    // Les rangs d'abord : ceux de la partie fixe, dans son ordre, pour que la fusion rende l'ordre
    // meme d'une composition complete.
    for (const TextureHandle texture : _scene.textureOrder()) {
        out.registerTexture(texture);
    }

    // Les figurines de l'image, sous le cadrage de l'appelant, triees entre elles.
    composeWorldFigures(out, _figureContext, figures, _projection, textures);
    out.sort();
    std::vector<ComposedQuad> dynamic;
    out.swapQuads(dynamic, 0, 0);

    // La partie fixe sous le cadrage, deja triee.
    _visible.clear();
    const std::vector<ComposedQuad>& statics = _scene.quads();
    if (out.isCullingEnabled()) {
        visibleIndices(out.cullingBounds(), _visible);
    } else {
        _visible.resize(statics.size());
        std::iota(_visible.begin(), _visible.end(), 0U);
    }

    // La fusion : a cle egale, la piece fixe passe devant la figurine, comme dans un tri stable de
    // la composition complete (la carte, puis les figurines).
    const std::optional<WorldHeroPlacement> hero =
        placeWorldHero(_figureContext, figures, _projection, textures);
    _merged.clear();
    _merged.reserve(existing.size() + _visible.size() + dynamic.size());
    _merged.insert(_merged.end(), std::make_move_iterator(existing.begin()),
                   std::make_move_iterator(existing.end()));
    std::size_t figure = 0;
    for (const std::uint32_t index : _visible) {
        const ComposedQuad& fixed = statics[index];
        while (figure < dynamic.size() && ComposedScene::drawsBefore(dynamic[figure], fixed)) {
            _merged.push_back(dynamic[figure++]);
        }
        _merged.push_back(fixed);
        if (hidesHero(fixed, hero)) {
            seeThrough(_merged.back());
        }
    }
    _merged.insert(_merged.end(), dynamic.begin() + static_cast<std::ptrdiff_t>(figure),
                   dynamic.end());

    const auto considered = static_cast<int>(statics.size());
    const int culled = considered - static_cast<int>(_visible.size());
    out.swapQuads(_merged, considered, culled);
}

}  // namespace hmi
