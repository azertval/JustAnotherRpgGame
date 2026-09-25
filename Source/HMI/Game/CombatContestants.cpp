// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Game/CombatContestants.h"

#include <utility>
#include <vector>

#include "Core/Combat/Attack.h"

namespace hmi {

core::ArenaContestant heroContestant(const HeroContestantSource& hero, core::CombatSide side) {
    core::CombatantProfile profile = core::profileFor(hero.sheet, side);
    profile.armorClass = hero.armorClass;
    // L'arme d'abord, les mains nues ensuite. Les classes provisoires ne declarent pas leurs
    // maitrises d'armes : maitrisee jusqu'au socle de classe (LOT-47).
    std::vector<core::AttackProfile> attacks;
    if (hero.weapon.has_value()) {
        attacks.push_back(core::weaponAttackFor(hero.sheet, &*hero.weapon, hero.proficiency));
        if (std::optional<core::AttackProfile> thrown =
                core::thrownAttackFor(hero.sheet, *hero.weapon, hero.proficiency)) {
            attacks.push_back(std::move(*thrown));
        }
    }
    attacks.push_back(core::weaponAttackFor(hero.sheet, nullptr, hero.proficiency));
    return core::ArenaContestant{.profile = std::move(profile),
                                 .attacks = std::move(attacks),
                                 .position = std::nullopt,
                                 .markId = {},
                                 .behavior = {}};
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
                        : std::string{}};
}

}  // namespace hmi
