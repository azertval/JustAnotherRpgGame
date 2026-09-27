// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Rpg/Spell.h
 * @brief Le catalogue des sorts (`spell.schema.json`), et ce que le moteur sait en jouer
 *        (`LOT-131`, `EX-RPG-050`, `EX-RPG-051`).
 *
 * Un sort est une **combinaison déclarée de mécanismes**, jamais une fonction. Ce lot n'en joue
 * qu'un : le sort à **jet d'attaque** (*fire bolt*, *sacred flame* n'en est pas un), résolu comme
 * une attaque à distance au modificateur de la caractéristique d'incantation plus la maîtrise
 * (`core::spellAttackFor`). Les sorts à jet de sauvegarde, de soin ou de condition déclarent leurs
 * champs ici et attendent leurs mécanismes (`LOT-133`, `LOT-134`, `LOT-137`) ; `isAttackSpell`
 * dit, sort par sort, ce que le moteur sait faire aujourd'hui — aucun ne tombe dans un cas par
 * défaut.
 */

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Rpg/Ability.h"
#include "Core/Rpg/Dice.h"
#include "Core/Rpg/RpgEnums.h"

namespace core {

/// @brief Un sort, tel que `spell.schema.json` l'écrit.
struct Spell {
    std::string id;
    std::string name;
    std::string source;
    /// 0 : sort mineur (*cantrip*), à volonté.
    int level = 0;
    std::string school;
    std::string castingTime;
    /// Le texte du livre (« 36 mètres »).
    std::string range;
    /// La portée en **mètres**, si le sort vise à distance ; 0 sinon.
    float rangeMeters = 0.0F;
    std::string duration;
    bool concentration = false;
    bool ritual = false;
    /// Vrai pour un sort qui demande un jet d'attaque de sort contre la CA.
    bool attackRoll = false;
    std::optional<Dice> damage;
    std::optional<DamageType> damageType;
    std::optional<Ability> savingThrow;
    std::string appliesCondition;
    std::string text;
    /// Déclaré sans mécanisme joué (`EX-RPG-051`).
    bool narrative = false;
    std::vector<std::string> requiredMechanisms;
};

/// @brief Le catalogue des sorts, et ce qui n'a pas pu l'être.
struct SpellCatalog {
    std::vector<Spell> spells;
    std::vector<std::string> errors;

    /// @brief Le sort d'identifiant @p id, ou `nullptr` s'il est inconnu.
    [[nodiscard]] const Spell* find(std::string_view id) const;
};

/**
 * @brief Charge les sorts depuis leur dossier (`Source/Elements/Rpg/spells`).
 *
 * Balaye le dossier, jamais une liste de noms. Un type de dégâts inconnu refuse le sort
 * (`EX-CBT-032`) ; des dés illisibles aussi. Ne lève jamais (`EX-NFR-040`).
 */
[[nodiscard]] SpellCatalog loadSpells(const std::filesystem::path& spellsDir);

/**
 * @brief Vrai si le moteur sait jouer ce sort comme une **attaque** : jet d'attaque, dés et type
 *        de dégâts. C'est le seul mécanisme de sort de ce lot.
 */
[[nodiscard]] bool isAttackSpell(const Spell& spell) noexcept;

}  // namespace core
