// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/LevelProperties.h"

/**
 * @file Core/Levels/MapEntity.h
 * @brief Entité placée sur une carte : PNJ, coffre, panneau, portail, déclencheur (`LOT-04`).
 */

namespace core {

/**
 * @brief Une entité de carte — ce qui n'est **pas** une tuile.
 *
 * La grille ne porte qu'un `core::TileType` par case, sans métadonnée : un PNJ a un nom et un
 * dialogue, un coffre un contenu et un état « déjà ouvert », un portail une carte cible et un
 * point d'arrivée. Des vecteurs annexes indexés par position, un par famille, obligeraient à
 * ajouter un vecteur à chaque famille nouvelle. Une **liste d'objets à propriétés** les porte
 * toutes.
 *
 * `Core` ne connaît **aucune** sémantique de `type` : c'est une chaîne, interprétée par le
 * gameplay (`LOT-10` et suivants). Le chargeur ne valide donc pas qu'un type existe — une entité
 * inconnue est une erreur de conception tolérée, pas une carte invalide (`EX-NFR-040`).
 */
struct MapEntity {
    /// Type de l'entité, libre : `"npc"`, `"chest"`, `"sign"`, `"portal"`, `"encounter"`…
    std::string type;
    /// Case occupée par l'entité — pour une zone peinte, sa case de référence.
    GridPosition position{};
    /// Propriétés libres (`core::PropertyMap`), y compris les clés que le chargeur n'a pas
    /// reconnues — elles sont réémises telles quelles à l'écriture.
    PropertyMap properties{};
    // Les champs du format v4 viennent APRES les trois d'origine : une initialisation positionnelle
    // `{type, position, properties}` garde son sens. Un champ ajouté en tête la décalerait sans
    // erreur de compilation — un type `"chest"` deviendrait un identifiant.
    /// Identifiant court, unique dans la carte, donné par l'éditeur et **jamais réemployé**
    /// (format v4, décision D8, `EX-LVL-021`). Quêtes, drapeaux et sauvegardes citent l'entité par
    /// `carte#id` : déplacer un coffre ne casse plus rien. Vide pour une entité lue d'une carte
    /// antérieure à la v4, jusqu'à sa migration.
    std::string id{};
    /// Hauteur de l'entité. **Réservée** (décision D11) : lue, gardée, réécrite, jamais jouée.
    int elevation = 0;
    /// Forme **peinte** d'une zone (décision D13) : les cases qu'elle couvre. Vide pour une entité
    /// ponctuelle ou une zone rectangle (propriétés `width` et `height`).
    std::vector<GridPosition> cells{};

    /// Deux entités sont égales si tout l'est, identifiant compris : ce que compare un tampon
    /// relu (`LOT-EDITOR-08`) ou un test qui vérifie qu'une carte n'a pas bougé.
    [[nodiscard]] bool operator==(const MapEntity&) const = default;
};

/// @return L'identifiant que l'éditeur donne à la @p number-ième entité d'une carte : `e<number>`.
[[nodiscard]] std::string entityIdFor(int number);

/// @return Le numéro d'un identifiant de la forme `e<number>`, ou `std::nullopt` pour un autre.
[[nodiscard]] std::optional<int> entityIdNumber(std::string_view id) noexcept;

/**
 * @brief Type d'entité d'une **zone** de règles : un rectangle (`width` × `height` depuis sa case)
 *        ou un ensemble de cases peint (`MapEntity::cells`), dont les propriétés s'appliquent à
 *        chaque case couverte (décision D13, `EX-LVL-022`, lue par `core::BattleGrid::zonesAt`).
 */
inline constexpr std::string_view ZONE_ENTITY_TYPE = "zone";

/// @brief Propriété d'une zone rectangle : sa largeur, en cases.
inline constexpr std::string_view ZONE_WIDTH_PROPERTY = "width";
/// @brief Propriété d'une zone rectangle : sa hauteur, en cases.
inline constexpr std::string_view ZONE_HEIGHT_PROPERTY = "height";

/**
 * @brief Les cases que couvre la zone @p entity : ses cases peintes si elle en a, sinon son
 *        rectangle (`width` × `height`, 1 × 1 par défaut) depuis sa case.
 */
[[nodiscard]] std::vector<GridPosition> zoneCells(const MapEntity& entity);

}  // namespace core
