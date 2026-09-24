// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/World/EntityPresence.h
 * @brief La condition de présence d'une entité de carte : un PNJ qui paraît sous un drapeau et
 *        disparaît sous un autre (`LOT-116`, `EX-EXP-009`).
 */

#include <optional>
#include <string_view>
#include <vector>

#include "Core/Gameplay/FlagCondition.h"
#include "Core/Levels/MapEntity.h"

namespace core {

class WorldFlags;

/**
 * @brief Le drapeau dont dépend la présence de l'entité. Absent : l'entité est toujours là.
 *
 * **Trois propriétés plates** plutôt qu'un objet : une propriété de carte ne tient qu'un scalaire
 * (`core::PropertyValue`), et un objet serait jeté au chargement sans un mot. C'est aussi la forme
 * que l'inspecteur de l'éditeur présente (drapeau, test, valeurs — `LOT-126`).
 */
inline constexpr std::string_view PRESENCE_FLAG_PROPERTY = "presenceFlag";
/// @brief Le test : `set`, `unset`, `equals`, `notEquals`. Absent : `equals` si des valeurs sont
///        données, `set` sinon.
inline constexpr std::string_view PRESENCE_TEST_PROPERTY = "presenceTest";
/// @brief Les valeurs de `equals`/`notEquals`, séparées par `|` : `acceptee|persuasion-echouee`.
inline constexpr std::string_view PRESENCE_VALUE_PROPERTY = "presenceValue";

/// @brief Ce qui ne va pas dans une condition de présence. `Core` n'écrit pas de texte.
enum class PresenceIssue {
    None,
    /// L'une des trois propriétés n'est pas un texte.
    WrongValueType,
    /// `presenceTest` n'est pas l'un des quatre mots.
    UnknownTest,
    /// `equals`/`notEquals` sans valeur.
    MissingValue,
    /// Un test ou des valeurs sans `presenceFlag`.
    MissingFlag,
};

/// @brief La condition lue sur une entité : aucune (toujours présente), ou la condition, ou le
///        défaut qui l'empêche.
struct PresenceRead {
    std::optional<FlagCondition> condition;
    PresenceIssue issue = PresenceIssue::None;
};

/// @brief Lit la condition de présence de @p entity.
[[nodiscard]] PresenceRead presenceConditionOf(const MapEntity& entity);

/**
 * @brief Vrai si l'entité est là sous ces drapeaux.
 *
 * Une condition **mal formée** laisse l'entité présente : un PNJ qui disparaît à cause d'une faute
 * de frappe ne se remarque pas, un PNJ toujours là se voit — et `LevelEditor --check` la refuse.
 */
[[nodiscard]] bool isEntityPresent(const MapEntity& entity, const WorldFlags& flags);

/// @brief Les entités de @p entities présentes sous @p flags, dans leur ordre.
[[nodiscard]] std::vector<const MapEntity*> presentEntities(const std::vector<MapEntity>& entities,
                                                            const WorldFlags& flags);

}  // namespace core
