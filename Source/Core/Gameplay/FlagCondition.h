// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Gameplay/FlagCondition.h
 * @brief Une condition sur un drapeau de monde, commune aux dialogues, aux quêtes et à la présence
 *        des entités de carte (`LOT-116`).
 */

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace core {

class WorldFlags;

/// @brief Ce qu'une condition teste.
enum class FlagTest {
    /// Le fait est acquis (drapeau booléen), ou le drapeau a reçu une valeur.
    IsSet,
    /// Le contraire.
    IsUnset,
    /// La valeur du drapeau est l'une de `values` — la valeur initiale d'un drapeau déclaré compte.
    Equals,
    /// La valeur du drapeau n'est aucune de `values`.
    NotEquals,
};

/**
 * @brief « Le drapeau @c flag est levé », « vaut `acceptee` », « ne vaut ni `condamne` ni
 *        `enfant-libere` »…
 *
 * Une seule forme pour trois lecteurs : une réponse de dialogue, une étape de quête et un PNJ
 * conditionné ne doivent pas pouvoir comprendre le même drapeau de trois façons.
 */
struct FlagCondition {
    std::string flag;
    FlagTest test = FlagTest::IsSet;
    /// `Equals` et `NotEquals` seulement : au moins une valeur.
    std::vector<std::string> values;

    /// @brief Vrai si les drapeaux satisfont la condition.
    [[nodiscard]] bool holds(const WorldFlags& flags) const;

    [[nodiscard]] bool operator==(const FlagCondition&) const = default;
};

/// @brief `quete.pommes == acceptee|condamne`, `!coffre` — la forme lisible des traces et tests.
[[nodiscard]] std::string describeFlagCondition(const FlagCondition& condition);

/// @brief Une condition lue, ou ce qui l'en empêche.
struct FlagConditionRead {
    std::optional<FlagCondition> condition;
    std::string error;
};

/**
 * @brief Lit une condition écrite en JSON.
 *
 * Trois formes, exclusives :
 *
 * - `{ "flag": "f" }`, `{ "flag": "f", "isSet": false }` — un fait acquis ou non (`LOT-15`) ;
 * - `{ "flag": "f", "equals": "v" }` ou `"equals": ["v", "w"]` — la valeur est l'une d'elles ;
 * - `{ "flag": "f", "notEquals": ... }` — elle n'en est aucune.
 *
 * @param object Le nœud JSON de la condition.
 */
[[nodiscard]] FlagConditionRead readFlagCondition(const nlohmann::json& object);

/// @brief Sépare `a|b|c` en valeurs, sans les vides : la forme d'une propriété de carte.
[[nodiscard]] std::vector<std::string> splitFlagValues(std::string_view text);

}  // namespace core
