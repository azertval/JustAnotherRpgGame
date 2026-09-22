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
 * Le vocabulaire a été étoffé pour que la maquette dise ce que les plans montrent : pavé et ruelle,
 * étals, gradins, marbre des colonnes y ont chacun leur type et la teinte de leur légende. Elle
 * *se lit comme* le plan du planning, elle ne s'y superpose pas (décision D4).
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
            // Terre battue : plus brune que le pavé, qui a repris la teinte « pavé » des plans.
            return maquetteColorOf(0xb08d5f);
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
            // Pierre claire : la teinte « ruelle » des plans revient à `Alley`.
            return maquetteColorOf(0xe4d8bc);
        case core::TileType::Pavement:
            return maquetteColorOf(0xd9c7a3);  // « Pavé » des plans
        case core::TileType::Alley:
            return maquetteColorOf(0xc9b48a);  // « Ruelle » des plans
        case core::TileType::Planks:
            return maquetteColorOf(0x9a7650);
        case core::TileType::Flagstone:
            return maquetteColorOf(0xb7b0a2);
        case core::TileType::Snow:
            return maquetteColorOf(0xe9eef0);
        case core::TileType::Mud:
            return maquetteColorOf(0x6b5a3c);
        case core::TileType::Rubble:
            return maquetteColorOf(0x8f877a);
        case core::TileType::Door:
            return maquetteColorOf(0x5b3a22);
        case core::TileType::Bush:
            return maquetteColorOf(0x2f5a2b);
        case core::TileType::Tree:
            return maquetteColorOf(0x234a22);
        case core::TileType::Rock:
            return maquetteColorOf(0x77705f);
        case core::TileType::Fence:
            return maquetteColorOf(0x7a5a36);
        case core::TileType::LowWall:
            return maquetteColorOf(0x9a8f7c);
        case core::TileType::Stall:
            return maquetteColorOf(0x8e2335);  // « Étals / auvents » des plans
        case core::TileType::Crate:
            return maquetteColorOf(0xa27a48);
        case core::TileType::Column:
            return maquetteColorOf(0xefe6d2);  // le marbre des plans
        case core::TileType::Roof:
            return maquetteColorOf(0x8a4a3a);
        case core::TileType::Tiers:
            return maquetteColorOf(0x6f6a5d);  // « Gradins » des plans
        case core::TileType::Pit:
            return maquetteColorOf(0x1b1712);
        case core::TileType::Lava:
            return maquetteColorOf(0xc4501f);
    }
    return MaquetteColor{};
}

/**
 * @brief La forme d'un type en maquette : un losange plat, ou un bloc d'une hauteur et d'une
 *        emprise données.
 *
 * Un bloc d'une case de côté et d'une case de haut dit « mur » ; il ne dit ni une colonne, ni une
 * palissade, ni un arbre. La hauteur et l'emprise sont ce qui les distingue d'un coup d'œil, sans
 * texture.
 */
struct MaquetteShape {
    /// Hauteur du bloc, en hauteurs de losange ; `0` : losange plat, sans bloc.
    float height = 0.0F;
    /// Fraction du losange de la case qu'occupe la base du bloc, centrée (`1` : toute la case).
    float footprint = 1.0F;

    [[nodiscard]] friend constexpr bool operator==(const MaquetteShape&,
                                                   const MaquetteShape&) noexcept = default;
};

/**
 * @brief La forme de maquette d'un type de tuile.
 * @param type Type de tuile.
 * @return Un bloc pour la matière et le mobilier, un losange plat (`height == 0`) pour les sols.
 */
[[nodiscard]] constexpr MaquetteShape maquetteShape(core::TileType type) noexcept {
    switch (type) {
        case core::TileType::Wall:
        case core::TileType::Solid:
        case core::TileType::Cliff:
            return MaquetteShape{.height = 1.0F, .footprint = 1.0F};
        case core::TileType::Tree:
            return MaquetteShape{.height = 2.0F, .footprint = 0.6F};
        case core::TileType::Column:
            return MaquetteShape{.height = 2.0F, .footprint = 0.45F};
        case core::TileType::Roof:
        case core::TileType::Tiers:
            return MaquetteShape{.height = 1.5F, .footprint = 1.0F};
        case core::TileType::Stall:
            return MaquetteShape{.height = 0.7F, .footprint = 0.9F};
        case core::TileType::Rock:
            return MaquetteShape{.height = 0.6F, .footprint = 0.75F};
        case core::TileType::Crate:
            return MaquetteShape{.height = 0.5F, .footprint = 0.7F};
        case core::TileType::Fence:
            return MaquetteShape{.height = 0.45F, .footprint = 1.0F};
        case core::TileType::LowWall:
            return MaquetteShape{.height = 0.4F, .footprint = 1.0F};
        case core::TileType::Bush:
            return MaquetteShape{.height = 0.35F, .footprint = 0.85F};
        case core::TileType::Empty:
        case core::TileType::Entry:
        case core::TileType::Grass:
        case core::TileType::Dirt:
        case core::TileType::Sand:
        case core::TileType::Water:
        case core::TileType::DeepWater:
        case core::TileType::Bridge:
        case core::TileType::Stairs:
        case core::TileType::Pavement:
        case core::TileType::Alley:
        case core::TileType::Planks:
        case core::TileType::Flagstone:
        case core::TileType::Snow:
        case core::TileType::Mud:
        case core::TileType::Rubble:
        case core::TileType::Door:
        case core::TileType::Pit:
        case core::TileType::Lava:
            return MaquetteShape{};
    }
    return MaquetteShape{};
}

/**
 * @brief Une case de ce type se dessine-t-elle en **bloc extrudé** plutôt qu'en losange plat ?
 *
 * La matière pleine — bâtie (`Wall`), générique (`Solid`) ou naturelle (`Cliff`) — et le mobilier
 * doivent se **lire** comme des volumes et masquer ce qui est derrière eux. `DeepWater`, la fosse
 * et la lave bloquent le pas (`core::isSolid`) mais ne sont pas de la matière : ils restent des
 * losanges plats, et on voit par-dessus.
 * @param type Type de tuile.
 * @return `true` quand `maquetteShape` lui donne une hauteur.
 */
[[nodiscard]] constexpr bool maquetteExtrudes(core::TileType type) noexcept {
    return maquetteShape(type).height > 0.0F;
}

}  // namespace hmi
