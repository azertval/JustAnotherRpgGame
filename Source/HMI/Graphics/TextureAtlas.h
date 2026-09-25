// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <memory>

#include "Core/Ecs/Components/Sprite.h"
#include "HMI/Graphics/RenderLayer.h"

class QRhiTexture;

/**
 * @file HMI/Graphics/TextureAtlas.h
 * @brief Atlas de textures (spritesheet) et ses régions de tuiles.
 */

namespace hmi {

struct RhiContext;

/**
 * @brief Texture d'atlas **procédurale** et table de régions de tuiles (`EX-REN-041`/`EX-REN-042`).
 *
 * L'atlas est une grille de tuiles 16×16 générée en mémoire (`hmi::buildProceduralAtlasImage`) :
 * une couleur distincte par type de tuile, et une case à zones **transparentes** pour valider le
 * rendu alpha. Aucun fichier n'est lu, donc aucun échec de chargement possible (`EX-NFR-040`). Le
 * canevas de l'éditeur s'en sert pour distinguer les types de terrain à l'œil.
 */
class TextureAtlas {
public:
    /// Côté d'une tuile, en pixels (`EX-ARCH-021`).
    static constexpr int TILE_SIZE = 16;
    /// Nombre de tuiles par ligne et par colonne dans la grille générée. Les cases ne bougent
    /// pas quand un type de tuile disparaît : la couleur d'un type déjà posé ne change jamais.
    static constexpr int TILES_PER_SIDE = 6;

    /**
     * @brief Génère l'atlas procédural et crée la texture GPU associée.
     * @param context Interface de rendu et lot de mises à jour de l'image courante.
     */
    explicit TextureAtlas(const RhiContext& context);

    /// @return Largeur de l'atlas, en pixels.
    [[nodiscard]] int width() const {
        return _width;
    }

    /// @return Hauteur de l'atlas, en pixels.
    [[nodiscard]] int height() const {
        return _height;
    }

    /**
     * @brief Région (en pixels) de la tuile à une position de la grille.
     *
     * Pure arithmétique de grille (aucun état d'instance) : `static`, testable sans GPU
     * (`EX-NFR-010`).
     * @param column Colonne de la tuile (0 à TILES_PER_SIDE-1).
     * @param row    Ligne de la tuile (0 à TILES_PER_SIDE-1).
     * @return La région d'atlas correspondante, en pixels.
     */
    [[nodiscard]] static core::AtlasRegion tile(int column, int row);

private:
    int _width = 0;
    int _height = 0;
    std::shared_ptr<QRhiTexture> _texture;
};

}  // namespace hmi
