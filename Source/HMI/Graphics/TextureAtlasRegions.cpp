// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/TextureAtlas.h"

// Fichier : HMI/Graphics/TextureAtlasRegions.cpp
// Mapping (colonne, ligne) -> région d'atlas de `hmi::TextureAtlas` (`tile`).
//
// Séparé de `TextureAtlas.cpp` (création de la texture, dépendante de Qt/QRhi) : cette pure
// arithmétique de grille ne dépend que des constantes de la classe, ce qui permet de la compiler
// et de la tester **sans GPU ni Qt** (`EX-NFR-010`), comme le reste de la logique pure du projet
// (`Source/Test/CMakeLists.txt`).

namespace hmi {

// Région (en pixels) de la tuile à une position de la grille.
core::AtlasRegion TextureAtlas::tile(int column, int row) {
    return core::AtlasRegion{
        .x = column * TILE_SIZE, .y = row * TILE_SIZE, .width = TILE_SIZE, .height = TILE_SIZE};
}

}  // namespace hmi
