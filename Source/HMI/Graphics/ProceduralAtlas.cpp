// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/ProceduralAtlas.h"

#include <array>
#include <cstdint>

#include "HMI/Graphics/TextureAtlas.h"

namespace hmi {

namespace {
// Assemble une couleur RVBA (octets) en un pixel `R8G8B8A8_UNORM` (ordre mémoire R,G,B,A).
std::uint32_t pack(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha) {
    return static_cast<std::uint32_t>(red) | (static_cast<std::uint32_t>(green) << 8) |
           (static_cast<std::uint32_t>(blue) << 16) | (static_cast<std::uint32_t>(alpha) << 24);
}

// Couleur opaque de base d'une tuile, selon son index dans la grille (déterministe). Rangée
// (`TextureAtlas::TILES_PER_SIDE` par ligne, actuellement `6`). INVARIANT : une case ne change
// jamais de couleur ni de place -- les tuiles déjà posées dans les cartes livrées gardent leur
// teinte (`hmi::regionForTile`). La DERNIÈRE case est réservée au damier de transparence ; les
// cases sans type de tuile sont libres et gardent leur couleur pour un type à venir.
std::uint32_t tileColor(int tileIndex) {
    static const std::array<std::uint32_t, 36> palette{
        // Ligne 0
        pack(200, 60, 60, 255),
        pack(60, 200, 60, 255),  // (1,0) Entry : vert
        pack(60, 60, 200, 255),
        pack(200, 200, 60, 255),
        pack(255, 215, 0, 255),  // (4,0) or (libre)
        pack(0, 150, 255, 255),  // (5,0) azur (libre)
        // Ligne 1
        pack(200, 60, 200, 255),
        pack(60, 200, 200, 255),
        pack(230, 140, 40, 255),
        pack(140, 40, 230, 255),
        pack(90, 170, 80, 255),   // (4,1) Grass : vert herbe, plus sourd que le vert de Entry
        pack(140, 160, 60, 255),  // (5,1) kaki (libre)
        // Ligne 2
        pack(120, 120, 120, 255),  // (0,2) Solid : gris
        pack(80, 160, 120, 255),
        pack(160, 80, 120, 255),
        pack(120, 80, 160, 255),
        pack(140, 105, 70, 255),   // (4,2) Dirt : terre battue
        pack(235, 150, 170, 255),  // (5,2) rose (libre)
        // Ligne 3
        pack(200, 200, 200, 255),
        pack(90, 90, 90, 255),
        pack(225, 205, 145, 255),  // (2,3) Sand : sable
        pack(70, 150, 220, 255),   // (3,3) Water : eau peu profonde, traversable
        pack(25, 60, 140, 255),    // (4,3) DeepWater : eau profonde, franchement plus sombre --
                                   // la rive doit se lire d'un coup d'oeil
        pack(160, 205, 240, 255),  // (5,3) givre (libre)
        // Ligne 4
        pack(95, 85, 75, 255),     // (0,4) Wall : pierre batie, distincte du gris neutre de Solid
        pack(70, 60, 55, 255),     // (1,4) Cliff : roche sombre
        pack(155, 120, 75, 255),   // (2,4) Bridge : bois
        pack(110, 70, 20, 255),    // (3,4) brun fonce (libre)
        pack(180, 175, 165, 255),  // (4,4) Stairs : pierre claire
        pack(30, 25, 20, 255),     // (5,4) Pit : fosse
        // Ligne 5
        pack(210, 80, 30, 255),  // (0,5) Lava : lave
        pack(0, 0, 0, 255),
        pack(0, 0, 0, 255),
        pack(0, 0, 0, 255),
        pack(0, 0, 0, 255),
        pack(0, 0, 0, 255),
    };
    const int count = static_cast<int>(std::size(palette));
    return palette[((tileIndex % count) + count) % count];
}

}  // namespace

// Génère, en mémoire, l'atlas procédural.
ProceduralAtlasImage buildProceduralAtlasImage() {
    const int gridSide = TextureAtlas::TILE_SIZE * TextureAtlas::TILES_PER_SIDE;

    ProceduralAtlasImage image;
    image.width = gridSide;
    image.height = gridSide;
    image.pixels.assign(
        static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height),
        pack(0, 0, 0, 0));

    // Dernière tuile réservée au test de transparence : damier opaque / transparent.
    const int transparentTileIndex =
        (TextureAtlas::TILES_PER_SIDE * TextureAtlas::TILES_PER_SIDE) - 1;

    for (int y = 0; y < gridSide; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const int tileColumn = x / TextureAtlas::TILE_SIZE;
            const int tileRow = y / TextureAtlas::TILE_SIZE;
            const int tileIndex = (tileRow * TextureAtlas::TILES_PER_SIDE) + tileColumn;

            std::uint32_t color = tileColor(tileIndex);
            if (tileIndex == transparentTileIndex) {
                // Un damier 4×4 pixels : une case sur deux est entièrement transparente.
                const bool transparent = (((x / 4) + (y / 4)) % 2) == 0;
                color = transparent ? pack(0, 0, 0, 0) : pack(240, 240, 240, 255);
            }
            image.pixels[(static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width)) +
                         static_cast<std::size_t>(x)] = color;
        }
    }

    return image;
}

}  // namespace hmi
