// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>

/**
 * @file HMI/Graphics/Quad.h
 * @brief Primitives de dessin 2D (quads texturés), **indépendantes de Direct3D**.
 *
 * Ces structures décrivent *quoi* dessiner, jamais *comment* : elles ne référencent aucune
 * ressource Direct3D et sont donc manipulables par la **composition** du rendu
 * (`hmi::ComposedScene`), qui doit rester testable sans GPU (`EX-NFR-004`). `hmi::SpriteBatch` les
 * consomme côté soumission ; son contrat public est inchangé (il les expose toujours via
 * `HMI/Graphics/SpriteBatch.h`, qui inclut ce fichier).
 */

namespace hmi {

/**
 * @brief Un quad texturé à dessiner : rectangle en **unités monde**, région de texture
 *        (coordonnées normalisées) et teinte.
 *
 * Le coin (`x`, `y`) est haut-gauche **avant rotation**, l'axe Y va vers le bas (convention du
 * projet). Les coordonnées de texture `u,v` sont normalisées dans [0, 1] ; la conversion depuis
 * une région d'atlas en pixels est faite en amont (par le rendu des sprites). `rotation` (radians)
 * tourne le rectangle autour de **son propre centre** — nul par défaut, donc
 * sans coût ni différence visuelle pour les quads alignés aux axes (tuiles, personnage).
 */
struct SpriteQuad {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float rotation = 0.0f;
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

/**
 * @brief Un quadrilatère à **quatre sommets libres**, en unités monde, d'une seule teinte — la
 *        primitive du **rendu de maquette** (`LOT-128`, décision D1).
 *
 * Un losange isométrique n'est ni un `SpriteQuad` (rectangle aligné sur les axes) ni un `LineQuad`
 * (segment) : au rapport 0,62, ce n'est même pas un carré tourné, puisque sa hauteur et sa largeur
 * ne sont pas dans le même rapport que ses côtés. Les faces d'un bloc extrudé sont, elles, des
 * parallélogrammes. Un quad à sommets libres couvre les deux, et n'est **pas** un cas nouveau pour
 * le GPU : un quad, ce sont déjà quatre sommets, et `SpriteBatch::draw(const LineQuad&)` en produit
 * déjà à des positions libres.
 *
 * Les sommets se donnent dans l'ordre du **pourtour** (sans croisement), même convention que les
 * coins des deux autres primitives : le tampon d'indices attend un quadrilatère convexe cohérent.
 * La texture liée est l'aplat blanc 1 × 1 (`hmi::SceneImages::solid` côté éditeur), de sorte que la
 * teinte seule décide de la couleur, et que le culling, le regroupement par texture et le tri
 * restent ceux de toutes les autres primitives.
 */
struct PolyQuad {
    std::array<float, 4> x{};
    std::array<float, 4> y{};
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

/**
 * @brief Un segment épais à dessiner : deux extrémités en **unités monde**, une épaisseur
 *        (perpendiculaire au segment, unités monde) et une teinte.
 *
 * Contrairement à `SpriteQuad` (rectangle toujours aligné aux axes), ce quad peut être **orienté**
 * dans n'importe quelle direction — utilisé pour les liens de mécanismes (flèches
 * déclencheur → cible). Mêmes conventions que `SpriteQuad` (Y vers le bas, UV normalisées).
 */
struct LineQuad {
    float ax = 0.0f;
    float ay = 0.0f;
    float bx = 0.0f;
    float by = 0.0f;
    float thickness = 0.0f;
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

}  // namespace hmi
