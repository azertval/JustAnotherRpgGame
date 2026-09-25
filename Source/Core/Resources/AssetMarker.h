// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Resources/AssetMarker.h
 * @brief Le **marqueur** qui tient lieu d'illustration tant qu'aucune n'existe (`LOT-39`,
 *        `EX-CNT-041`).
 */

#include <cstdint>
#include <string_view>
#include <vector>

namespace core {

/// Une couleur RVBA, sans dépendance au rendu — `Core` ne connaît ni Qt ni GPU (`EX-ARCH-001`).
struct MarkerColor {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;

    [[nodiscard]] friend bool operator==(const MarkerColor&, const MarkerColor&) noexcept = default;
};

/**
 * @brief Les pixels d'un marqueur, en RVBA, ligne par ligne depuis le haut.
 */
struct MarkerImage {
    int width = 0;
    int height = 0;
    /// `width * height` pixels.
    std::vector<MarkerColor> pixels;

    /// @brief Vrai si l'image n'a pas de pixel : dimension nulle ou tampon vide.
    [[nodiscard]] bool isEmpty() const noexcept {
        return width <= 0 || height <= 0 || pixels.empty();
    }
    /// @brief Le pixel de la colonne @p x et de la ligne @p y, comptées depuis le haut-gauche. Sans
    /// contrôle des bornes.
    [[nodiscard]] MarkerColor at(int x, int y) const {
        return pixels[static_cast<std::size_t>((y * width) + x)];
    }
};

/**
 * @brief Peint le marqueur d'une clé d'asset.
 *
 * ## Pourquoi un marqueur plutôt qu'une image manquante
 *
 * `EX-CNT-041` : le jeu doit tourner **complet** avant qu'aucune illustration ne soit produite.
 * Sans marqueur, trois cents entrées s'afficheraient comme des trous, la production graphique
 * deviendrait un préalable bloquant, et rien ne pourrait être joué ni relu avant elle. Avec, elle
 * devient un **remplacement progressif** : on remplace un marqueur par une image, une à la fois,
 * et le jeu ne cesse jamais d'être jouable.
 *
 * ## Déterministe, et c'est le point
 *
 * La même clé donne **toujours** le même marqueur — même teinte, même figure. Un marqueur tiré au
 * hasard changerait à chaque lancement : le loup ne serait plus reconnaissable d'une partie à
 * l'autre, deux captures d'écran du même combat différeraient, et un test ne pourrait rien en
 * dire. La teinte est donc dérivée de la clé elle-même, par un hachage stable écrit ici plutôt
 * qu'emprunté à `std::hash`, dont la valeur n'est **pas** garantie d'une plateforme ou d'une
 * version de bibliothèque à l'autre.
 *
 * Le marqueur reste **manifestement** un marqueur : deux diagonales barrent la vignette, de sorte
 * qu'on ne le prenne jamais pour une illustration définitive livrée un peu vite.
 *
 * @param key    La clé, `famille/identifiant`. Une clé malformée rend une image vide.
 * @param width  Largeur voulue, celle que la famille attend.
 * @param height Hauteur voulue.
 */
[[nodiscard]] MarkerImage assetMarker(std::string_view key, int width, int height);

/// @brief Le hachage stable dont `assetMarker` tire sa teinte. Exposé pour être **testé** : c'est
///        lui qui garantit qu'une clé donne toujours le même marqueur.
[[nodiscard]] std::uint32_t stableAssetHash(std::string_view key);

}  // namespace core
