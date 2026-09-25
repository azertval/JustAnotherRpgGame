// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstdint>

/**
 * @file HMI/Graphics/RenderLayer.h
 * @brief Ordonnancement de calques **unique et explicite** du rendu (`EX-REN-014`).
 */

namespace hmi {

/**
 * @brief Calques de dessin, du plus **arrière** au plus **avant** (`EX-REN-014`).
 *
 * L'**ordre de déclaration est l'ordre de dessin** : une valeur déclarée plus bas est dessinée
 * par-dessus les précédentes. C'est le **seul** ordonnancement de calques du projet — aucun lot ne
 * doit en inventer un concurrent, sous peine de rendre l'ordre visuel dépendant de qui dessine en
 * dernier (`EX-REN-014`).
 *
 * Le calque de chaque primitive est choisi par le composeur qui la produit
 * (`hmi::WorldSceneComposer`, `hmi::DraftRenderer`) ; la galerie des
 * assets range aussi chaque image sous le calque où elle sera dessinée.
 *
 * Notion de **présentation** (`HMI`) : `Core` l'ignore et continue de ne connaître que
 * `core::Sprite::layer`, entier de tri **fin à l'intérieur** d'un calque (`EX-NFR-011`).
 */
enum class RenderLayer : std::int32_t {
    /// Fond, sous tout le reste.
    Background = 0,
    /// Ombres portées, sous les tuiles.
    Shadow,
    /// Sol et tuiles de la carte.
    Tile,
    /// Décor et objets posés sur la carte : tout ce qui partage la profondeur du personnage
    /// (`LOT-07`).
    Object,
    /// Personnage joueur. Même **bande de profondeur** que `Object` depuis le `LOT-07` : les deux
    /// s'ordonnent entre eux par le Y de leur pied, pas par leur rang de calque.
    Player,
    /// Texte et interface en scène (`EX-REN-031`).
    UI,
    /// Aides d'édition : grille de repère, aperçu de sélection.
    EditorOverlay,
};

/**
 * @brief Le calque appartient-il à la **bande de profondeur** ?
 *
 * Dans une vue de dessus, « devant » et « derrière » ne se décident plus par un rang de calque
 * fixe mais par la **position** : le personnage passe devant ce qui est au-dessus de lui à
 * l'écran, derrière ce qui est en dessous. Les primitives d'une bande de profondeur s'ordonnent
 * donc entre elles par leur `sortOrder` — le Y de leur pied (`hmi::depthSortOrder`) — **avant**
 * tout regroupement de texture.
 *
 * C'est le prix à payer, et il est assumé : regrouper par texture d'abord ferait passer tout le
 * décor devant le personnage, ou tout derrière, selon l'ordre d'apparition des textures. Un
 * personnage et un arbre n'ayant jamais la même texture, il n'existe aucun ordre de calque qui
 * rende les deux cas justes — seule la profondeur le peut, au prix de passes de dessin
 * supplémentaires (`EX-REN-018`).
 */
[[nodiscard]] constexpr bool sortsByDepth(RenderLayer layer) noexcept {
    return layer == RenderLayer::Object || layer == RenderLayer::Player;
}

/**
 * @brief Bande de tri d'un calque : `Object` et `Player` en partagent **une seule**.
 *
 * Sans cela, le rang de calque trancherait avant la profondeur et le personnage passerait
 * **toujours** devant un objet, quelle que soit sa position — exactement ce que le tri par Y doit
 * corriger. Tous les autres calques restent leur propre bande : le sol ne se mélange jamais au
 * décor, et les aides d'édition restent au-dessus de tout (`EX-REN-014`).
 */
[[nodiscard]] constexpr std::int32_t renderBand(RenderLayer layer) noexcept {
    return static_cast<std::int32_t>(sortsByDepth(layer) ? RenderLayer::Object : layer);
}

/**
 * @brief Identité **opaque** d'une texture liée, du point de vue de la composition.
 *
 * La composition du rendu ne fait que **comparer** et **regrouper** des textures ; elle n'a
 * jamais besoin d'en connaître le type Direct3D. Ce typage volontairement opaque permet de la
 * garder libre de toute dépendance GPU (et donc testable sans carte, `EX-NFR-004`) : côté
 * soumission, `hmi::SpriteBatch` reconvertit la valeur en `QRhiTexture*`, seule couche qui en
 * connaisse le type réel. Une valeur nulle désigne « aucune texture liée ».
 */
using TextureHandle = void*;

/**
 * @brief Nom lisible d'un calque, pour la journalisation et les messages d'échec de test.
 * @param layer Calque à nommer.
 * @return Le nom du calque (chaîne statique, jamais nulle).
 */
[[nodiscard]] constexpr const char* renderLayerName(RenderLayer layer) noexcept {
    switch (layer) {
        case RenderLayer::Background:
            return "Background";
        case RenderLayer::Shadow:
            return "Shadow";
        case RenderLayer::Tile:
            return "Tile";
        case RenderLayer::Object:
            return "Object";
        case RenderLayer::Player:
            return "Player";
        case RenderLayer::UI:
            return "UI";
        case RenderLayer::EditorOverlay:
            return "EditorOverlay";
    }
    return "Inconnu";
}

}  // namespace hmi
