// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/SceneResources.h"

#include "HMI/Graphics/SpriteBatch.h"
#include "HMI/Graphics/TextureAtlas.h"

namespace hmi {

// Définis ici, et non `= default` dans l'en-tête : les membres sont des `unique_ptr` de types
// seulement DÉCLARÉS là-bas. Le destructeur implicite exigerait leur définition complète, et tout
// fichier incluant l'en-tête devrait alors tirer deux en-têtes de rendu dont il n'a que faire.
SceneResources::SceneResources() = default;

SceneResources::~SceneResources() {
    release();
}

void SceneResources::create(QRhi* rhi, QRhiResourceUpdateBatch* updates) {
    _context.rhi = rhi;
    _context.updates = updates;

    _spriteBatch = std::make_unique<SpriteBatch>(_context.rhi);
    _atlas = std::make_unique<TextureAtlas>(_context);
}

void SceneResources::release() noexcept {
    // L'ORDRE EST LA RAISON D'ÊTRE DE CETTE CLASSE. Ce qui tient une texture meurt avant elle, et
    // la texture avant le pipeline qui l'échantillonne. Libérer dans le désordre ne produit pas
    // une erreur nette mais un plantage à la fermeture, intermittent selon le pilote.
    _atlas.reset();
    _spriteBatch.reset();
    _context.rhi = nullptr;
    _context.updates = nullptr;
}

}  // namespace hmi
