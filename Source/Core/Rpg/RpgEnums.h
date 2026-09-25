// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Rpg/RpgEnums.h
 * @brief Les quatre énumérations **fermées** du RPG : dégâts, conditions, écoles, tailles.
 */

namespace core {

/**
 * @brief Type de dégâts infligés (`EX-CBT-032`, `EX-CNT-011`).
 *
 * Ensemble **fermé** : treize valeurs, fixées par les règles et non par le corpus. Les trois
 * premières sont les dégâts *physiques* — contondant, perforant, tranchant — dont dépend la
 * résistance des créatures aux armes non magiques ; les dix autres sont élémentaires ou
 * surnaturels. Un type inconnu du moteur ne doit **jamais** tomber dans un cas par défaut
 * silencieux (`EX-CBT-032`) — d'où le `switch` exhaustif de `Core/Rpg/RpgEnumNames.h`.
 *
 * Les noms textuels sont ceux du lexique de traduction (`LOT-30`), catégorie *type de dégâts* :
 * `scripts/checks/check_rpg_data.py` vérifie que les treize valeurs de cette énumération sont
 * exactement les treize termes anglais que le lexique porte sous cette catégorie. Le moteur, les
 * schémas et la table d'autorité de traduction disent donc le même mot pour la même chose, ou la CI
 * échoue.
 */
enum class DamageType {
    Acid,
    Bludgeoning,
    Cold,
    Fire,
    Force,
    Lightning,
    Necrotic,
    Piercing,
    Poison,
    Psychic,
    Radiant,
    Slashing,
    Thunder,
};

/**
 * @brief État affectant une créature (`EX-REG-040`, `EX-CNT-011`).
 *
 * Ensemble **fermé** : quinze valeurs. `Exhaustion` est la seule à ne pas être binaire — elle se
 * mesure en six niveaux — mais elle reste une condition, et l'omettre de cette liste (comme le
 * glossaire du corpus l'omettait de sa catégorie) reviendrait à la rendre invisible au moteur.
 *
 * @note L'énumération ne dit **rien** de l'effet d'une condition : c'est l'affaire de la donnée,
 *       dont `Source/Elements/Rpg/schema/condition.schema.json` fixe la forme (aucun catalogue de
 *       conditions n'est encore livré). Une énumération qui porterait les effets les figerait dans
 *       le C++, ce qu'`EX-VIS-007` interdit.
 */
enum class Condition {
    Blinded,
    Charmed,
    Deafened,
    Exhaustion,
    Frightened,
    Grappled,
    Incapacitated,
    Invisible,
    Paralyzed,
    Petrified,
    Poisoned,
    Prone,
    Restrained,
    Stunned,
    Unconscious,
};

/**
 * @brief École de magie d'un sort (`EX-CNT-011`).
 *
 * Ensemble **fermé** : huit valeurs. Attention au faux ami que le lexique fige : `Conjuration` se
 * traduit **« invocation »** en français, jamais « conjuration ».
 */
enum class MagicSchool {
    Abjuration,
    Conjuration,
    Divination,
    Enchantment,
    Evocation,
    Illusion,
    Necromancy,
    Transmutation,
};

/**
 * @brief Catégorie de taille d'une créature (`EX-CNT-011`).
 *
 * Ensemble **fermé** : six valeurs, fixées par les règles. Ce n'est pas une donnée décorative —
 * la taille détermine l'emprise sur la grille tactique (`LOT-19`) et ce qu'une créature peut
 * agripper ou engloutir.
 *
 * @note Le lexique du `LOT-30` n'en porte que **cinq** sous la catégorie *taille* : « Moyenne »
 *       manque à l'extraction du glossaire. C'est pourquoi la coïncidence de cette énumération se
 *       vérifie contre `common.schema.json`, qui les porte toutes les six, et non contre le
 *       lexique comme les trois autres.
 */
enum class CreatureSize {
    Tiny,
    Small,
    Medium,
    Large,
    Huge,
    Gargantuan,
};

}  // namespace core
