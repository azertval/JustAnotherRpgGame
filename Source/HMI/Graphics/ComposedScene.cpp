// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/ComposedScene.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace hmi {

// Vide la scene (capacite conservee) et remet les compteurs a zero. Le cadrage est conserve.
void ComposedScene::clear() noexcept {
    _quads.clear();
    _textureOrder.clear();
    _considered = 0;
    _culled = 0;
}

// Restreint la composition aux primitives visibles dans un cadrage donne (EX-NFR-005). Le
// rectangle elargi de la marge est calcule une fois ici, et non a chaque primitive testee.
void ComposedScene::setVisibleBounds(const core::Rect& worldBounds) noexcept {
    _visibleBounds = core::Rect{core::Vector2{worldBounds.position.x - CULLING_MARGIN_UNITS,
                                              worldBounds.position.y - CULLING_MARGIN_UNITS},
                                core::Vector2{worldBounds.size.x + (2.0F * CULLING_MARGIN_UNITS),
                                              worldBounds.size.y + (2.0F * CULLING_MARGIN_UNITS)}};
}

// Desactive le culling : toutes les primitives ajoutees sont conservees.
void ComposedScene::clearVisibleBounds() noexcept {
    _visibleBounds.reset();
}

// Rectangle reellement utilise pour le culling : le cadrage elargi de la marge.
core::Rect ComposedScene::cullingBounds() const noexcept {
    return _visibleBounds.value_or(core::Rect{});
}

// Rang de premiere apparition d'une texture, ajoutee a la table si elle est nouvelle. Le nombre de
// textures distinctes par image se compte sur les doigts d'une main : une recherche lineaire est
// plus rapide (et bien plus simple) qu'une table de hachage.
int ComposedScene::textureRank(TextureHandle texture) {
    const auto found = std::ranges::find(_textureOrder, texture);
    if (found != _textureOrder.end()) {
        return static_cast<int>(std::distance(_textureOrder.begin(), found));
    }
    _textureOrder.push_back(texture);
    return static_cast<int>(_textureOrder.size()) - 1;
}

// true si la boite englobante est visible (ou si le culling est desactive).
bool ComposedScene::isVisible(const core::Rect& bounds) const {
    if (!_visibleBounds) {
        return true;
    }
    return _visibleBounds->intersects(bounds);
}

// Ajoute un rectangle texture a la scene, s'il est visible.
// true si la primitive a ete conservee, false si le culling l'a ecartee.
bool ComposedScene::addSprite(RenderLayer layer, TextureHandle texture, std::int32_t sortOrder,
                              const SpriteQuad& quad, int storey) {
    ++_considered;
    if (!isVisible(spriteQuadBounds(quad))) {
        ++_culled;
        return false;
    }
    ComposedQuad composed;
    composed.layer = layer;
    composed.texture = texture;
    composed.textureRank = textureRank(texture);
    composed.sortOrder = sortOrder;
    composed.kind = QuadKind::Sprite;
    composed.sprite = quad;
    composed.storey = storey;
    _quads.push_back(composed);
    return true;
}

// Ajoute un segment epais a la scene, s'il est visible.
// true si la primitive a ete conservee, false si le culling l'a ecartee.
bool ComposedScene::addLine(RenderLayer layer, TextureHandle texture, std::int32_t sortOrder,
                            const LineQuad& quad) {
    ++_considered;
    if (!isVisible(lineQuadBounds(quad))) {
        ++_culled;
        return false;
    }
    ComposedQuad composed;
    composed.layer = layer;
    composed.texture = texture;
    composed.textureRank = textureRank(texture);
    composed.sortOrder = sortOrder;
    composed.kind = QuadKind::Line;
    composed.line = quad;
    _quads.push_back(composed);
    return true;
}

// Ajoute un quadrilatere a quatre sommets libres a la scene, s'il est visible.
// true si la primitive a ete conservee, false si le culling l'a ecartee.
bool ComposedScene::addPoly(RenderLayer layer, TextureHandle texture, std::int32_t sortOrder,
                            const PolyQuad& quad) {
    ++_considered;
    if (!isVisible(polyQuadBounds(quad))) {
        ++_culled;
        return false;
    }
    ComposedQuad composed;
    composed.layer = layer;
    composed.texture = texture;
    composed.textureRank = textureRank(texture);
    composed.sortOrder = sortOrder;
    composed.kind = QuadKind::Poly;
    composed.poly = quad;
    _quads.push_back(composed);
    return true;
}

// Ordre de profondeur d'une primitive, a partir du pied de son quad (voir en-tete).
std::int32_t depthSortOrder(float footWorldY) noexcept {
    return static_cast<std::int32_t>(std::lround(footWorldY * DEPTH_SUBDIVISIONS_PER_UNIT));
}

// Ordonne la scene (bande, puis profondeur ou texture, puis le reste), de facon stable.
void ComposedScene::sort() {
    // La BANDE est prioritaire sur tout : regrouper par texture ne doit jamais faire passer une
    // primitive devant une primitive d'une bande inferieure (EX-REN-014). Le tri stable preserve
    // l'ordre de composition a cle egale.
    //
    // A l'interieur d'une bande, deux regimes (LOT-07) :
    //  - bande de PROFONDEUR (Object + Player) : le sortOrder -- le Y du pied -- tranche AVANT la
    //    texture. C'est la seule facon qu'un personnage passe derriere un arbre plus bas et devant
    //    un arbre plus haut, les deux n'ayant jamais la meme texture que lui (EX-REN-018) ;
    //  - partout ailleurs : la texture regroupe d'abord (une passe de dessin par groupe), le
    //    sortOrder ne departageant que l'interieur d'un groupe -- comportement d'avant le lot.
    std::ranges::stable_sort(_quads, [](const ComposedQuad& lhs, const ComposedQuad& rhs) {
        const std::int32_t leftBand = renderBand(lhs.layer);
        const std::int32_t rightBand = renderBand(rhs.layer);
        if (leftBand != rightBand) {
            return leftBand < rightBand;
        }
        if (sortsByDepth(lhs.layer)) {
            if (lhs.sortOrder != rhs.sortOrder) {
                return lhs.sortOrder < rhs.sortOrder;
            }
            return lhs.textureRank < rhs.textureRank;
        }
        if (lhs.textureRank != rhs.textureRank) {
            return lhs.textureRank < rhs.textureRank;
        }
        return lhs.sortOrder < rhs.sortOrder;
    });
}

// Le nombre de passes begin/end : groupes contigus de meme texture.
int ComposedScene::batchCount() const noexcept {
    int batches = 0;
    TextureHandle current = nullptr;
    for (std::size_t i = 0; i < _quads.size(); ++i) {
        if (i == 0 || _quads[i].texture != current) {
            ++batches;
            current = _quads[i].texture;
        }
    }
    return batches;
}

// Les compteurs de l'image composee (EX-NFR-005).
SceneStatistics ComposedScene::statistics() const noexcept {
    SceneStatistics stats;
    stats.considered = _considered;
    stats.culled = _culled;
    stats.submitted = static_cast<int>(_quads.size());
    stats.batches = batchCount();
    return stats;
}

// Resume des compteurs d'une image, pour la journalisation de diagnostic (EX-NFR-005).
std::string formatSceneStatistics(const SceneStatistics& statistics) {
    return "Rendu : " + std::to_string(statistics.considered) + " primitive(s) composee(s), " +
           std::to_string(statistics.culled) + " ecartee(s) hors cadrage, " +
           std::to_string(statistics.submitted) + " soumise(s) en " +
           std::to_string(statistics.batches) + " passe(s).";
}

// Boite englobante d'un rectangle texture, en unites monde -- tient compte de la rotation
// : un quad pivote occupe un rectangle englobant plus grand que sa taille propre, le culling
// doit donc le juger sur ce rectangle-la, jamais sur (x, y, width, height) brut (une entite pivotee
// pres du bord du cadrage disparaitrait sinon a tort).
core::Rect spriteQuadBounds(const SpriteQuad& quad) noexcept {
    if (quad.rotation == 0.0F) {
        return core::Rect{core::Vector2{quad.x, quad.y}, core::Vector2{quad.width, quad.height}};
    }
    const float halfWidth = quad.width * 0.5F;
    const float halfHeight = quad.height * 0.5F;
    const float centerX = quad.x + halfWidth;
    const float centerY = quad.y + halfHeight;
    const float cosR = std::fabs(std::cos(quad.rotation));
    const float sinR = std::fabs(std::sin(quad.rotation));
    const float extentX = (halfWidth * cosR) + (halfHeight * sinR);
    const float extentY = (halfWidth * sinR) + (halfHeight * cosR);
    return core::Rect{core::Vector2{centerX - extentX, centerY - extentY},
                      core::Vector2{extentX * 2.0F, extentY * 2.0F}};
}

// Boite englobante d'un segment epais, en unites monde (extremites elargies d'une demi-epaisseur).
core::Rect lineQuadBounds(const LineQuad& quad) noexcept {
    const float half = quad.thickness * 0.5F;
    const float left = (std::min)(quad.ax, quad.bx) - half;
    const float top = (std::min)(quad.ay, quad.by) - half;
    const float right = (std::max)(quad.ax, quad.bx) + half;
    const float bottom = (std::max)(quad.ay, quad.by) + half;
    return core::Rect{core::Vector2{left, top}, core::Vector2{right - left, bottom - top}};
}

// Boite englobante d'un quadrilatere a sommets libres : les extremes de ses quatre sommets. Pas de
// demi-epaisseur a ajouter ici, contrairement au segment : un quad a toujours une aire, sauf a etre
// degenere -- auquel cas il ne dessine rien de toute facon.
core::Rect polyQuadBounds(const PolyQuad& quad) noexcept {
    const auto [minX, maxX] = std::ranges::minmax(quad.x);
    const auto [minY, maxY] = std::ranges::minmax(quad.y);
    return core::Rect{core::Vector2{minX, minY}, core::Vector2{maxX - minX, maxY - minY}};
}

}  // namespace hmi
