// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Game/CombatContestants.h"

#include <utility>
#include <vector>

#include "Core/Combat/Attack.h"

namespace hmi {

core::ArenaContestant heroContestant(const HeroContestantSource& hero, core::CombatSide side) {
    // Le profil lit la fiche : vitesse et resistances des capacites comprises (LOT-131). La classe
    // d'armure, elle, vient de l'equipement porte (EX-CBT-030), recalculee par l'appelant.
    core::CombatantProfile profile = core::profileFor(hero.sheet, side);
    profile.armorClass = hero.armorClass;
    // L'arme d'abord, les mains nues ensuite. La maitrise de l'arme se lit dans la fiche, qui
    // reunit celles de la classe et de l'espece (LOT-131) : une arme non maitrisee se frappe sans
    // le bonus de maitrise.
    std::vector<core::AttackProfile> attacks;
    if (hero.weapon.has_value()) {
        const bool proficient = core::isProficientWith(hero.sheet, *hero.weapon);
        attacks.push_back(
            core::weaponAttackFor(hero.sheet, &*hero.weapon, hero.proficiency, proficient));
        if (std::optional<core::AttackProfile> thrown =
                core::thrownAttackFor(hero.sheet, *hero.weapon, hero.proficiency, proficient)) {
            attacks.push_back(std::move(*thrown));
        }
    }
    attacks.push_back(core::weaponAttackFor(hero.sheet, nullptr, hero.proficiency));
    return core::ArenaContestant{.profile = std::move(profile),
                                 .attacks = std::move(attacks),
                                 .position = std::nullopt,
                                 .markId = {},
                                 .behavior = {},
                                 .capacities = hero.sheet.capacities,
                                 .spells = hero.spells};
}

core::ArenaContestant creatureContestant(const core::Creature& creature, core::CombatSide side,
                                         const core::BehaviorCatalog* behaviors) {
    return core::ArenaContestant{
        .profile = core::profileFor(creature, side),
        .attacks = core::attacksFor(creature).attacks,
        .position = std::nullopt,
        .markId = {},
        // Une creature prend le profil que ses regles lui donnent (LOT-23).
        .behavior = behaviors != nullptr && !behaviors->profiles.empty()
                        ? core::behaviorFor(creature, *behaviors)
                        : std::string{},
        .capacities = {},
        .spells = {}};
}

}  // namespace hmi
