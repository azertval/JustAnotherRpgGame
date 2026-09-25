// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/QuadRecorder.h"

#include <algorithm>
#include <cmath>

namespace hmi {

// Capture l'etat d'une scene composee (copie : la scene peut ensuite etre reutilisee).
void QuadRecorder::record(const ComposedScene& scene) {
    _quads = scene.quads();
    _statistics = scene.statistics();
}

// Oublie la capture courante.
void QuadRecorder::clear() noexcept {
    _quads.clear();
    _statistics = SceneStatistics{};
}

// true si aucune primitive n'est soumise avant une primitive d'une BANDE inferieure
// (EX-REN-014). La comparaison porte sur la bande et non sur le calque : Object et Player en
// partagent une, et s'y entrelacent legitimement par profondeur depuis le LOT-07 (EX-REN-018) --
// exiger un ordre de calque strict y interdirait precisement ce que le tri par Y doit produire.
bool QuadRecorder::isLayerOrderRespected() const {
    for (std::size_t i = 1; i < _quads.size(); ++i) {
        if (renderBand(_quads[i].layer) < renderBand(_quads[i - 1].layer)) {
            return false;
        }
    }
    return true;
}

// true si, a l'interieur de chaque calque, aucune texture n'apparait dans deux groupes distincts :
// les quads d'une meme texture sont contigus, un seul groupe par texture. La contiguite ne vaut
// qu'**a calque constant** : une meme texture presente sur deux calques differents produit
// legitimement deux passes, puisque le calque prime sur la texture.
bool QuadRecorder::areTextureGroupsContiguous() const {
    std::vector<TextureHandle> seenInLayer;  // textures deja closes dans le calque courant
    for (std::size_t i = 0; i < _quads.size(); ++i) {
        const bool newLayer = i == 0 || _quads[i].layer != _quads[i - 1].layer;
        if (newLayer) {
            seenInLayer.clear();
        } else if (_quads[i].texture != _quads[i - 1].texture) {
            seenInLayer.push_back(_quads[i - 1].texture);
            if (std::ranges::find(seenInLayer, _quads[i].texture) != seenInLayer.end()) {
                return false;  // texture deja close plus haut dans ce calque : groupe rompu
            }
        }
    }
    return true;
}

// Suite des calques rencontres, sans repetition consecutive.
std::vector<RenderLayer> QuadRecorder::layerSequence() const {
    std::vector<RenderLayer> sequence;
    for (std::size_t i = 0; i < _quads.size(); ++i) {
        if (i == 0 || _quads[i].layer != _quads[i - 1].layer) {
            sequence.push_back(_quads[i].layer);
        }
    }
    return sequence;
}

// Suite des textures liees, sans repetition consecutive : une entree par passe begin/end.
std::vector<TextureHandle> QuadRecorder::textureSequence() const {
    std::vector<TextureHandle> sequence;
    for (std::size_t i = 0; i < _quads.size(); ++i) {
        if (i == 0 || _quads[i].texture != _quads[i - 1].texture) {
            sequence.push_back(_quads[i].texture);
        }
    }
    return sequence;
}

// Nombre de primitives capturees sur un calque donne.
int QuadRecorder::countOnLayer(RenderLayer layer) const {
    return static_cast<int>(
        std::count_if(_quads.begin(), _quads.end(),
                      [layer](const ComposedQuad& composed) { return composed.layer == layer; }));
}

// Nombre de primitives capturees liees a une texture donnee.
int QuadRecorder::countWithTexture(TextureHandle texture) const {
    return static_cast<int>(std::count_if(
        _quads.begin(), _quads.end(),
        [texture](const ComposedQuad& composed) { return composed.texture == texture; }));
}

// true si au moins un rectangle capture est a la position monde donnee.
bool QuadRecorder::containsSpriteAt(float x, float y, float tolerance) const {
    return std::ranges::any_of(_quads, [&](const ComposedQuad& composed) {
        return composed.kind == QuadKind::Sprite && std::fabs(composed.sprite.x - x) <= tolerance &&
               std::fabs(composed.sprite.y - y) <= tolerance;
    });
}

// Resume lisible de la capture, a joindre au message d'echec d'une assertion.
std::string QuadRecorder::describe() const {
    std::string summary = "Scene capturee : " + std::to_string(_quads.size()) + " primitive(s), " +
                          std::to_string(_statistics.batches) + " passe(s), " +
                          std::to_string(_statistics.culled) + " ecartee(s).";
    // Une ligne par passe (groupe contigu de meme calque ET meme texture) : c'est la granularite
    // a laquelle une regression d'ordre se lit d'un coup d'oeil.
    std::size_t index = 0;
    while (index < _quads.size()) {
        const ComposedQuad& first = _quads[index];
        std::size_t last = index;
        while (last + 1 < _quads.size() && _quads[last + 1].layer == first.layer &&
               _quads[last + 1].texture == first.texture) {
            ++last;
        }
        summary += "\n  " + std::string{renderLayerName(first.layer)} + " / texture #" +
                   std::to_string(first.textureRank) + " : " + std::to_string(last - index + 1) +
                   " primitive(s)";
        index = last + 1;
    }
    return summary;
}

}  // namespace hmi
