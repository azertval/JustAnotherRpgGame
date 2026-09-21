// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include "Core/Levels/TileType.h"

/**
 * @file HMI/Graphics/MaquettePalette.h
 * @brief La palette du **rendu de maquette** : la teinte de chaque type de tuile quand aucune
 *        pièce ne l'habille (`LOT-128`, décision D5).
 *
 * ## Pourquoi dans le code, et pas dans un fichier
 *
 * La maquette doit se dessiner quand **aucun** fichier d'asset n'est présent (`EX-EXP-005`) :
 * c'est toute sa raison d'être. Une palette chargée depuis le disque réintroduirait exactement la
 * dépendance que le lot supprime. C'est un **standard** — comme l'échelle ou le rapport du losange
 * —, pas du contenu : elle vit donc ici, en-tête seul, sans allocation ni chargement.
 *
 * ## Pourquoi pas les couleurs de l'atlas procédural
 *
 * `hmi::regionForTile` donne des teintes vives, faites pour qu'un développeur distingue douze types
 * sur un canevas de travail. Une maquette n'est pas un canevas de travail : c'est une **image de la
 * carte**, qu'on regarde en jouant. Les teintes sont donc celles des plans de principe du planning
 * (`Planning/versions/.../maquettes/plan-*.svg`), sourdes et accordées entre elles.
 *
 * Les douze types génériques ne diront jamais un gradin d'un podium (décision D4) : la maquette
 * *se lit comme* le plan du planning, elle ne s'y superpose pas.
 */

namespace hmi {

/// @brief Une teinte de maquette, en composantes `[0, 1]` — ce que les primitives attendent.
struct MaquetteColor {
    float r = 1.0F;
    float g = 1.0F;
    float b = 1.0F;

    [[nodiscard]] friend constexpr bool operator==(const MaquetteColor&,
                                                   const MaquetteColor&) noexcept = default;
};

/// @brief Une teinte depuis son écriture hexadécimale à six chiffres (`0x3f6b34`).
[[nodiscard]] constexpr MaquetteColor maquetteColorOf(unsigned int rgb) noexcept {
    return MaquetteColor{.r = static_cast<float>((rgb >> 16U) & 0xFFU) / 255.0F,
                         .g = static_cast<float>((rgb >> 8U) & 0xFFU) / 255.0F,
                         .b = static_cast<float>(rgb & 0xFFU) / 255.0F};
}

/**
 * @brief La teinte de maquette d'un type de tuile.
 *
 * `switch` exhaustif et sans `default` : ajouter un type au vocabulaire du terrain fait désigner ce
 * point par le compilateur, plutôt que de laisser la case nouvelle se peindre en blanc sans que
 * personne ne s'en aperçoive.
 * @param type Type de tuile ; `Empty` n'est jamais dessiné — sa teinte n'est là que pour la
 *             complétude du `switch`.
 * @return Sa teinte.
 */
[[nodiscard]] constexpr MaquetteColor maquetteColor(core::TileType type) noexcept {
    switch (type) {
        case core::TileType::Empty:
            return maquetteColorOf(0x000000);
        case core::TileType::Solid:
            return maquetteColorOf(0x8d8272);
        case core::TileType::Entry:
            return maquetteColorOf(0xd9c7a3);
        case core::TileType::Grass:
            return maquetteColorOf(0x3f6b34);
        case core::TileType::Dirt:
            return maquetteColorOf(0xd9c7a3);
        case core::TileType::Sand:
            return maquetteColorOf(0xc9a86a);
        case core::TileType::Water:
            return maquetteColorOf(0x2f7f86);
        case core::TileType::DeepWater:
            return maquetteColorOf(0x1d4f57);
        case core::TileType::Wall:
            return maquetteColorOf(0x6b5a43);
        case core::TileType::Cliff:
            return maquetteColorOf(0x5a5346);
        case core::TileType::Bridge:
            return maquetteColorOf(0x8e6f45);
        case core::TileType::Stairs:
            return maquetteColorOf(0xc9b48a);
    }
    return MaquetteColor{};
}

/**
 * @brief Une case de ce type se dessine-t-elle en **bloc extrudé** plutôt qu'en losange plat ?
 *
 * La matière pleine — bâtie (`Wall`), générique (`Solid`) ou naturelle (`Cliff`) — doit se **lire**
 * comme un obstacle et masquer ce qui est derrière elle. `DeepWater` bloque le pas
 * (`core::isSolid`) mais n'est pas de la matière : elle reste un losange plat, plus sombre que
 * l'eau vive, et on voit par-dessus.
 * @param type Type de tuile.
 * @return `true` pour `Wall`, `Solid` et `Cliff`.
 */
[[nodiscard]] constexpr bool maquetteExtrudes(core::TileType type) noexcept {
    return type == core::TileType::Wall || type == core::TileType::Solid ||
           type == core::TileType::Cliff;
}

}  // namespace hmi
