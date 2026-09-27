// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Rpg/ClassCapacities.h
 * @brief Les capacités de classe comme **effets nommés** (`LOT-131`, `EX-RPG-024`).
 *
 * Une capacité n'est pas une fonction : c'est une liste d'effets que le moteur sait brancher sur
 * les crochets du combat (`LOT-20`, `LOT-21`) — un bonus au jet d'attaque, une formule de classe
 * d'armure sans armure, une résistance, une vitesse, des dés de dégâts en plus, l'immunité aux
 * attaques d'opportunité. La table de progression d'une classe (`core::PlayableClass`) les désigne
 * par identifiant ; ce fichier les charge (`core::loadCapacities`) et les **lit** : chaque fonction
 * `…From` répond à une question du moteur — « quel bonus au jet ? », « quelle vitesse ? » — en
 * nommant la capacité qui répond, pour que le journal la cite (`EX-REG-003`).
 *
 * Aucun nom de capacité ni de classe n'est écrit ici : une classe de test à trois capacités se
 * charge et agit en combat sans une ligne de C++ qui la nomme — c'est le critère du lot.
 */

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Rpg/Ability.h"
#include "Core/Rpg/Check.h"
#include "Core/Rpg/Dice.h"
#include "Core/Rpg/RpgEnums.h"

namespace core {

/// @brief Les genres d'effet que le moteur sait jouer. Un genre inconnu de la donnée est refusé.
enum class CapacityEffectKind : std::uint8_t {
    /// Un bonus fixe aux jets d'attaque (*Hit the Mark*).
    AttackBonus,
    /// Un bonus fixe à la classe d'armure, quelle que soit l'armure (*Holy Shield*).
    ArmorClassBonus,
    /// Sans armure, la CA se calcule autrement : une base et des modificateurs (*Tough as Nails*,
    /// *Arcane Protection*).
    UnarmoredArmorClass,
    /// Résistance à des types de dégâts, ou à tous.
    DamageResistance,
    /// Vitesse ajoutée, en mètres (*Scoundrel's Agility*, *Powerful Legs*).
    SpeedBonus,
    /// Les déplacements ne provoquent pas d'attaque d'opportunité.
    NoOpportunityAttacks,
    /// Des dés ajoutés aux dégâts d'une attaque qui touche, du type de l'arme (*Sneak Attack*,
    /// *Deadly*).
    ExtraDamage,
    /// Des attaques en plus quand le personnage prend l'action *Attaquer* (*Extra Attack*,
    /// `LOT-132`).
    ExtraAttack,
    /// Un bonus fixe aux tests de caractéristique maîtrisés (*Adventurer's Aptitude*, `LOT-135`).
    ProficientCheckBonus,
};

/// @brief Nom de donnée d'un genre d'effet (`attack-bonus`…), celui du schéma.
[[nodiscard]] std::string_view capacityEffectKindName(CapacityEffectKind kind) noexcept;
/// @brief Inverse de `capacityEffectKindName`. `std::nullopt` si le nom est inconnu — jamais
/// deviné.
[[nodiscard]] std::optional<CapacityEffectKind> parseCapacityEffectKind(
    std::string_view name) noexcept;

/**
 * @brief Un effet, tel que `capacity.schema.json` l'écrit.
 *
 * Une seule structure pour tous les genres : chaque genre ne lit que ses champs, les autres
 * restent à leur valeur neutre. Un `std::variant` aurait été plus strict et bien plus verbeux pour
 * sept genres à deux champs.
 */
struct CapacityEffect {
    CapacityEffectKind kind = CapacityEffectKind::AttackBonus;
    /// `AttackBonus`, `ArmorClassBonus` : le bonus. `ExtraAttack` : les attaques ajoutées.
    int value = 0;
    /// `UnarmoredArmorClass` : la base de la formule.
    int base = 0;
    /// `UnarmoredArmorClass` : les modificateurs qui s'ajoutent à la base.
    std::vector<Ability> abilities;
    /// `UnarmoredArmorClass` : le bouclier s'ajoute encore.
    bool shieldAllowed = true;
    /// `DamageResistance` : résistance à **tous** les types.
    bool allDamageTypes = false;
    /// `DamageResistance` : les types résistés, si ce n'est pas tous.
    std::vector<DamageType> damageTypes;
    /// `SpeedBonus` : les mètres ajoutés.
    float meters = 0.0F;
    /// `ExtraDamage` : les dés ajoutés.
    Dice dice{};
    /// `ExtraDamage` : une seule fois par tour.
    bool oncePerTurn = false;
    /// `ExtraDamage` : seulement contre une cible adjacente à un allié de l'attaquant.
    bool allyAdjacentToTarget = false;
};

/// @brief Une capacité de classe : ce que le livre nomme, et les effets qui le jouent.
struct Capacity {
    std::string id;
    std::string name;
    std::string source;
    std::string text;
    /// La capacité que celle-ci remplace à un niveau supérieur, ou vide.
    std::string replaces;
    std::vector<CapacityEffect> effects;
    /// Déclarée sans mécanisme joué (`EX-RPG-051`, transposé aux capacités).
    bool narrative = false;
    /// Les mécanismes que la capacité exige et que le moteur n'honore pas encore (`EX-CNT-031`).
    std::vector<std::string> requiredMechanisms;
};

/// @brief Le catalogue des capacités, et ce qui n'a pas pu l'être.
struct CapacityCatalog {
    std::vector<Capacity> capacities;
    std::vector<std::string> errors;

    /// @brief La capacité d'identifiant @p id, ou `nullptr` si elle est inconnue.
    [[nodiscard]] const Capacity* find(std::string_view id) const;
};

/**
 * @brief Charge les capacités depuis leur dossier (`Source/Elements/Rpg/capacities`).
 *
 * Balaye le dossier, jamais une liste de noms. Un effet dont le genre est inconnu, ou auquel
 * manque un champ que son genre exige (les dés d'un `extra-damage`, la base d'une formule de CA),
 * est **refusé avec la capacité** : une capacité jouée à moitié tromperait plus qu'une capacité
 * absente et nommée dans les erreurs (`EX-CNT-010`). Ne lève jamais (`EX-NFR-040`).
 */
[[nodiscard]] CapacityCatalog loadCapacities(const std::filesystem::path& capacitiesDir);

// --- Ce que des capacités actives disent au moteur -------------------------------------------

/// @brief Les modificateurs de jet d'attaque, un par capacité qui en donne, à son nom.
[[nodiscard]] std::vector<Modifier> attackModifiersFrom(std::span<const Capacity> capacities);

/// @brief La somme des bonus fixes à la classe d'armure.
[[nodiscard]] int armorClassBonusFrom(std::span<const Capacity> capacities);

/// @brief Une classe d'armure sans armure calculée par une capacité.
struct UnarmoredArmorClass {
    /// La base plus les modificateurs, **sans** le bouclier.
    int armorClass = 0;
    bool shieldAllowed = true;
    /// La capacité qui la donne.
    std::string source;
};

/**
 * @brief La formule de CA sans armure des capacités, si l'une en porte une.
 *
 * @param capacities Les capacités actives de la fiche.
 * @param abilityScores Les six valeurs de caractéristique, dans l'ordre de `core::Ability`.
 * @return La meilleure formule si plusieurs capacités en donnent (elles ne s'additionnent pas),
 *         `std::nullopt` si aucune.
 */
[[nodiscard]] std::optional<UnarmoredArmorClass> unarmoredArmorClassFrom(
    std::span<const Capacity> capacities, std::span<const int, 6> abilityScores);

/// @brief La vitesse que les capacités ajoutent, en mètres.
[[nodiscard]] float speedBonusFrom(std::span<const Capacity> capacities);

/// @brief Le nom de la capacité qui soustrait aux attaques d'opportunité, ou vide.
[[nodiscard]] std::optional<std::string> opportunityImmunityFrom(
    std::span<const Capacity> capacities);

/// @brief Une résistance nommée : le type, et la capacité qui la donne.
struct NamedResistance {
    /// `std::nullopt` : tous les types.
    std::optional<DamageType> type;
    std::string source;
};

/// @brief Les résistances que les capacités donnent, chacune à son nom.
[[nodiscard]] std::vector<NamedResistance> resistancesFrom(std::span<const Capacity> capacities);

/// @brief Des dés de dégâts en plus, nommés, et leur cadence.
struct NamedExtraDamage {
    Dice dice;
    bool oncePerTurn = false;
    /// L'identifiant de la capacité : la clé du compteur « une fois par tour ».
    std::string capacityId;
    /// Son nom, pour le journal.
    std::string source;
    /// Seulement contre une cible adjacente à un allié de l'attaquant (*Sneak Attack*).
    bool allyAdjacentToTarget = false;
};

/// @brief Les dés que les capacités ajoutent à une attaque qui touche.
[[nodiscard]] std::vector<NamedExtraDamage> extraDamageFrom(std::span<const Capacity> capacities);

/// @brief Des attaques en plus à l'action *Attaquer*, et la capacité qui les donne.
struct NamedExtraAttacks {
    /// Les attaques **ajoutées** à la première : 1 pour deux attaques par action.
    int count = 0;
    std::string source;
};

/// @brief La somme des bonus aux tests de caractéristique maîtrisés (`LOT-135`).
[[nodiscard]] int proficientCheckBonusFrom(std::span<const Capacity> capacities);

/**
 * @brief Les attaques que les capacités ajoutent à l'action *Attaquer* (`LOT-132`).
 *
 * Deux capacités qui en donnent ne s'additionnent pas : la plus généreuse seule compte, comme le
 * Manuel le dit des attaques supplémentaires de deux classes. `std::nullopt` si aucune n'en donne.
 */
[[nodiscard]] std::optional<NamedExtraAttacks> extraAttacksFrom(
    std::span<const Capacity> capacities);

}  // namespace core
