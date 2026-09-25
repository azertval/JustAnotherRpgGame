// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file HMI/Game/CombatContestants.h
 * @brief De la fiche et du bestiaire au combattant d'une session (`core::ArenaContestant`) —
 *        ce que le Colisée et le combat sur la carte composent de la même façon (`LOT-118`).
 *
 * Le Colisée (`hmi::ArenaModel`) fabriquait ses combattants dans une fonction privée ; le combat
 * sur la carte a besoin des mêmes — le héros avec son arme, une créature avec ses attaques et son
 * profil d'IA —, et deux copies de cette fabrique divergeraient au premier réglage. Elle vit donc
 * ici, sans Qt, entre les deux.
 */

#include <optional>
#include <string_view>

#include "Core/Combat/Arena.h"
#include "Core/Combat/CombatState.h"
#include "Core/Combat/EnemyAi.h"
#include "Core/Rpg/Bestiary.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Equipment.h"

namespace hmi {

/**
 * @brief Ce dont un héros a besoin pour devenir combattant : sa fiche, et ce que son équipement
 *        en fait (`EX-CBT-030` : la classe d'armure se recalcule depuis ce qui est porté).
 */
struct HeroContestantSource {
    core::CharacterSheet sheet;
    /// Bonus de maîtrise, d'après son expérience.
    int proficiency = 2;
    /// Classe d'armure recalculée depuis l'équipement porté.
    int armorClass = 10;
    /// L'arme en main directrice, ou rien : il frappe alors à mains nues.
    std::optional<core::Weapon> weapon;
};

/// @brief Le héros comme combattant du camp @p side : son arme, son jet, ses mains nues.
[[nodiscard]] core::ArenaContestant heroContestant(const HeroContestantSource& hero,
                                                   core::CombatSide side);

/**
 * @brief Une créature du bestiaire comme combattant du camp @p side.
 * @param creature  La créature.
 * @param side      Son camp.
 * @param behaviors Les profils de l'IA (`LOT-23`) : la créature prend celui que ses règles lui
 *                  donnent. `nullptr`, ou un catalogue vide : elle se commande à la main.
 */
[[nodiscard]] core::ArenaContestant creatureContestant(const core::Creature& creature,
                                                       core::CombatSide side,
                                                       const core::BehaviorCatalog* behaviors);

}  // namespace hmi
