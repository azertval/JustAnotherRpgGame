// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Gameplay/Quest.h"
#include "Core/Gameplay/WorldFlags.h"
#include "Core/Levels/MapEntity.h"

/**
 * @file Editor/Logic/WorldState.h
 * @brief L'**état de partie** sous lequel l'éditeur montre et essaie une carte (`LOT-126`).
 *
 * Une carte de quête change avec les drapeaux : le garde et l'enfant paraissent sous
 * `quete.pommes = acceptee`, la porte de l'arène se ferme sous `condamne`. L'auteur choisit un état
 * — des faits acquis, une valeur par drapeau déclaré — et le canevas grise ce qui en est absent ;
 * l'essai immédiat (`P`) et l'essai complet (`F5`) partent de ce même état.
 *
 * L'état s'écrit comme la ligne de commande du jeu le lit (`--flags=`, `hmi::parseWorldFlags`) :
 * `fait` pour un fait acquis, `drapeau=valeur` pour un drapeau qu'une quête déclare. Une seule
 * forme, donc, de l'éditeur au jeu.
 */

namespace hmi {

/// @brief Une entrée d'état lue : le drapeau, et sa valeur s'il en porte une.
struct WorldStateEntry {
    std::string flag;
    std::optional<std::string> value;

    [[nodiscard]] bool operator==(const WorldStateEntry&) const = default;
};

/// @return @p entry lu : `a` → `{a}`, `a=b` → `{a, b}`.
[[nodiscard]] WorldStateEntry parseWorldStateEntry(std::string_view entry);

/// @return La valeur que @p entries donne à @p flag, s'il lui en donne une.
[[nodiscard]] std::optional<std::string> worldStateValue(const std::vector<std::string>& entries,
                                                         std::string_view flag);

/// @brief Des drapeaux, et les entrées qu'ils ont refusées.
struct WorldStateFlags {
    core::WorldFlags flags;
    /// Les entrées refusées : une valeur hors de la déclaration, un fait posé sur un drapeau
    /// déclaré (`core::WorldFlags::set` le refuse).
    std::vector<std::string> refused;
};

/**
 * @brief Les drapeaux de monde de @p entries : ceux que les quêtes déclarent, à leur valeur
 *        initiale, puis chaque entrée dans l'ordre.
 * @param entries  L'état (`fait`, `drapeau=valeur`).
 * @param declared Les drapeaux que les quêtes déclarent (`EditorReferences::declaredFlags`).
 */
[[nodiscard]] WorldStateFlags worldStateFlags(const std::vector<std::string>& entries,
                                              const std::vector<core::QuestFlag>& declared);

/// @return Pour chaque entité de @p entities, vrai si elle est présente sous @p flags
///         (`core::isEntityPresent`).
[[nodiscard]] std::vector<bool> presenceUnder(const std::vector<core::MapEntity>& entities,
                                              const core::WorldFlags& flags);

}  // namespace hmi
