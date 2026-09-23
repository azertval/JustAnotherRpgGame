// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QImage>
#include <filesystem>

#include "Core/Math/Vector2.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace core {
class IsoProjection;
struct CityBlock;
}  // namespace core

/**
 * @file HMI/Graphics/CityBlockRender.h
 * @brief L'**îlot** vu sur le plan : la carte du quartier telle que le jeu la dessine, cadrée sur
 *        un rectangle nommé (`LOT-96`).
 *
 * *Décision de l'auteur, 18 septembre 2026* : l'îlot n'a pas d'image à lui. L'écran « Carte » le
 * montre par le **même** rendu que le lieu qu'on parcourt (`hmi::WorldSceneRenderer`), hors écran,
 * sur un `QRhi` sans fenêtre — les mêmes planches, les mêmes marqueurs, le héros à sa case. Rien à
 * peindre, et un plan qui ne peut pas diverger du terrain.
 */

namespace hmi {

/// @brief Le cadrage d'un îlot : le point à suivre, et la taille d'image qui le contient.
struct CityBlockFraming {
    /// Le point, en cases, que le rendu suit (`WorldSceneRenderer::setFocus`).
    core::Vector2 focus{};
    int pixelWidth = 0;
    int pixelHeight = 0;
};

/// Largeur d'une case dans l'image d'un îlot, en pixels : celle d'une case à 1080p
/// (`hmi::worldTilePixels`). Une définition d'image, que l'écran « Carte » réduit en la lissant.
inline constexpr float CITY_BLOCK_TILE_PIXELS = 100.0F;

/**
 * @brief Cadre l'îlot @p block de la carte que @p projection projette.
 *
 * L'image couvre le losange englobant de l'îlot, à @p tilePixels pixels par case, plus, en haut,
 * l'élévation de la pièce la plus haute du lieu (@p maximumRise, lue dans son manifeste par
 * `hmi::PlaceAppearance::maximumRise`) : un mur posé au fond de l'îlot se dresse au-dessus de sa
 * case, et le couper ferait un plan décapité.
 *
 * @param projection  La projection de la carte.
 * @param block       L'îlot.
 * @param maximumRise Élévation de la pièce la plus haute au-dessus de sa case, en largeurs de case.
 * @param tilePixels  Largeur d'une case dans l'image, en pixels.
 */
[[nodiscard]] CityBlockFraming cityBlockFraming(const core::IsoProjection& projection,
                                                const core::CityBlock& block, float maximumRise,
                                                float tilePixels = CITY_BLOCK_TILE_PIXELS);

/**
 * @brief Dessine @p snapshot cadré sur @p block, hors écran.
 *
 * @param assetsDirectory Le dossier des assets, où les chemins de l'instantané se résolvent.
 * @param snapshot L'instantané du lieu à peindre, tel que `WorldSceneRenderer` le produit.
 * @param block L'îlot sur lequel le rendu se cadre.
 * @return L'image, ou une image nulle si aucune interface QRhi n'est disponible — l'écran le dit
 *         plutôt que de planter (`EX-NFR-040`).
 */
[[nodiscard]] QImage renderCityBlock(const std::filesystem::path& assetsDirectory,
                                     const WorldSceneSnapshot& snapshot,
                                     const core::CityBlock& block);

}  // namespace hmi
