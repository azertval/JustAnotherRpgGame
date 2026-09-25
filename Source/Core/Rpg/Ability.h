// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Rpg/Ability.h
 * @brief Les six caractéristiques et leur modificateur (`EX-REG-010`).
 */

#include <optional>
#include <span>
#include <string_view>

namespace core {

/**
 * @brief Une des six caractéristiques d'un personnage ou d'une créature (`EX-REG-010`).
 *
 * Ensemble **fermé** : six valeurs, fixées par les règles. Les noms textuels sont ceux du lexique
 * (`LOT-30`) et de `common.schema.json`, et `scripts/checks/check_rpg_data.py` vérifie que les
 * trois listes coïncident.
 */
enum class Ability {
    Strength,
    Dexterity,
    Constitution,
    Intelligence,
    Wisdom,
    Charisma,
};

/**
 * @brief Modificateur d'une valeur de caractéristique : `(score - 10) / 2`, **arrondi vers le
 *        bas**.
 *
 * L'arrondi est le piège de cette fonction, et il ne se voit que sur les scores **impairs
 * inférieurs à 10**. En C++, la division entière tronque **vers zéro** : `(7 - 10) / 2` vaut `-1`,
 * alors que la règle donne `-2`. Un personnage avec 7 en Force serait donc moins pénalisé qu'il ne
 * doit l'être, sur chacun de ses jets, pendant toute la partie — et le défaut est invisible à la
 * lecture puisque la formule *a l'air* juste.
 *
 * L'implémentation décale donc de 1 vers le bas avant de diviser, ce qui revient au plancher pour
 * les valeurs négatives comme positives.
 *
 * @param score Valeur de caractéristique, typiquement dans `[1, 30]`.
 * @return Le modificateur : `-5` pour 1, `0` pour 10 et 11, `+5` pour 20.
 */
[[nodiscard]] constexpr int abilityModifier(int score) noexcept {
    const int ecart = score - 10;
    // `ecart / 2` tronquerait vers zéro pour un écart négatif impair : -3 / 2 == -1, et non -2.
    return ecart >= 0 ? ecart / 2 : (ecart - 1) / 2;
}

/// @brief Nom textuel d'une caractéristique, tel que les données l'écrivent.
[[nodiscard]] std::string_view abilityName(Ability ability) noexcept;

/// @brief Inverse d'`abilityName`. `std::nullopt` si le nom est inconnu — jamais deviné.
[[nodiscard]] std::optional<Ability> parseAbility(std::string_view name) noexcept;

/// @brief Les six caractéristiques, dans l'ordre de la fiche.
[[nodiscard]] std::span<const Ability> allAbilities() noexcept;

}  // namespace core
