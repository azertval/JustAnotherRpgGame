// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Test/Support/ClassArena.h
 * @brief Les classes du livre en combat (`LOT-132` à `LOT-135`) : les catalogues de
 *        `Source/Elements/Rpg`, une fiche pré-tirée montée au niveau voulu, une salle sans mur, un
 *        mannequin à frapper, et la lecture du journal.
 *
 * Les quatre classes se testent de la même façon — la fiche de la page, sa classe lue dans la
 * donnée, une session d'arène —, et chaque fichier de test ne dit que ce qui est propre à sa
 * classe. Sans GoogleTest : un échec se dit par une valeur, que le test compare.
 */

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Core/Combat/Arena.h"
#include "Core/Combat/Attack.h"
#include "Core/Combat/CombatState.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/TileMap.h"
#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Equipment.h"
#include "Core/Rpg/Inventory.h"

namespace test_support {

/// @brief Les catalogues de `Source/Elements/Rpg` qu'une fiche jouée lit.
struct RpgCatalogs {
    core::CharacterOptions options;
    core::ExperienceTable experience;
    core::CharacterCreationRules rules;
    core::EquipmentCatalog equipment;
    core::ItemCatalog items;
    core::EncumbranceRules encumbrance;

    /// @brief De quoi résoudre un objet d'inventaire.
    [[nodiscard]] core::ItemLookup lookup() const {
        return {.items = &items, .equipment = &equipment};
    }
};

/// @return Les catalogues du jeu, lus une fois.
inline const RpgCatalogs& rpgCatalogs() {
    static const RpgCatalogs catalogs = [] {
        const std::filesystem::path rpg{JADG_RPG_DIR};
        RpgCatalogs read;
        read.options = core::loadCharacterOptions(rpg);
        read.experience = core::loadExperienceTable(rpg / "rules" / "experience.json");
        read.rules = core::loadCharacterCreationRules(rpg / "rules" / "character-creation.json");
        read.equipment = core::loadEquipment(rpg / "weapons", rpg / "armors");
        read.items = core::loadItems(rpg / "items");
        read.encumbrance = core::loadEncumbranceRules(rpg / "rules" / "encumbrance.json");
        return read;
    }();
    return catalogs;
}

/// @return La fiche pré-tirée @p file de `Rpg/characters/`, erreurs et avertissements compris.
inline core::LoadedCharacterSheet loadPremade(const char* file) {
    const RpgCatalogs& catalogs = rpgCatalogs();
    return core::loadCharacterSheet(std::filesystem::path{JADG_RPG_DIR} / "characters" / file,
                                    catalogs.options, catalogs.rules, catalogs.experience);
}

/**
 * @brief Monte @p sheet jusqu'au niveau @p level par l'expérience — le chemin d'une partie —,
 *        puis pose ce que sa classe donne à ce niveau.
 * @return Les capacités et les sorts que la table nomme et qu'aucun catalogue ne porte.
 */
inline std::vector<std::string> levelUpTo(core::CharacterSheet& sheet, int level) {
    const RpgCatalogs& catalogs = rpgCatalogs();
    std::vector<std::string> missing;
    const core::PlayableClass* playableClass = catalogs.options.findClass(sheet.classId);
    if (playableClass == nullptr) {
        missing.push_back(sheet.classId);
        return missing;
    }
    const int threshold = catalogs.experience.thresholdAt(level);
    if (threshold > sheet.experiencePoints) {
        core::gainExperience(sheet, catalogs.experience, playableClass->hitDie,
                             threshold - sheet.experiencePoints);
    }
    core::applyClassFeatures(sheet, *playableClass, catalogs.options, catalogs.rules, missing);
    return missing;
}

/// @return Les identifiants des capacités actives de @p sheet, dans l'ordre de la table.
inline std::vector<std::string> capacityIds(const core::CharacterSheet& sheet) {
    std::vector<std::string> ids;
    for (const core::Capacity& capacity : sheet.capacities) {
        ids.push_back(capacity.id);
    }
    return ids;
}

/// @return La classe d'armure de @p loaded avec ce qu'il porte (`EX-CBT-030`).
inline int armorClassOf(const core::CharacterSheet& sheet, const core::Inventory& inventory) {
    const RpgCatalogs& catalogs = rpgCatalogs();
    return core::derivedStatsFor(sheet, inventory, catalogs.lookup(), catalogs.rules,
                                 catalogs.encumbrance)
        .armorClass;
}

/// @return Une salle @p columns × @p rows sans mur : les places sont données à la main.
inline core::Level room(int columns = 12, int rows = 8) {
    return core::Level(core::LevelData{
        .name = "salle", .tileMap = core::TileMap(columns, rows), .entities = {}, .entry = {1, 1}});
}

/// @return L'attaque de l'arme @p weaponId du catalogue, maîtrise lue dans la fiche.
inline core::AttackProfile weaponAttack(const core::CharacterSheet& sheet, const char* weaponId) {
    const RpgCatalogs& catalogs = rpgCatalogs();
    const core::Weapon* weapon = catalogs.equipment.findWeapon(weaponId);
    if (weapon == nullptr) {
        return core::weaponAttackFor(sheet, nullptr,
                                     core::proficiencyBonus(sheet, catalogs.experience));
    }
    return core::weaponAttackFor(sheet, weapon, core::proficiencyBonus(sheet, catalogs.experience),
                                 core::isProficientWith(sheet, *weapon));
}

/**
 * @brief Un héros tel que le jeu le monte : profil de la fiche, CA de ce qu'il porte, @p attacks,
 *        capacités et sorts de sa classe.
 *
 * L'initiative est un jet : le héros joue en premier, sans parier sur ses dés.
 */
inline core::ArenaContestant hero(const core::CharacterSheet& sheet,
                                  const core::Inventory& inventory,
                                  std::vector<core::AttackProfile> attacks,
                                  core::GridPosition place,
                                  core::CombatSide side = core::CombatSide::Allies,
                                  int initiative = 100) {
    const RpgCatalogs& catalogs = rpgCatalogs();
    core::CombatantProfile profile = core::profileFor(sheet, side);
    profile.armorClass = armorClassOf(sheet, inventory);
    profile.initiativeModifier = initiative;
    std::vector<core::ArenaSpell> spells;
    if (const core::PlayableClass* playableClass = catalogs.options.findClass(sheet.classId)) {
        std::vector<std::string> skipped;
        spells = core::arenaSpellsFor(sheet, *playableClass, catalogs.options.spells,
                                      core::proficiencyBonus(sheet, catalogs.experience), skipped);
    }
    return {.profile = std::move(profile),
            .attacks = std::move(attacks),
            .position = place,
            .markId = {},
            .behavior = {},
            .capacities = sheet.capacities,
            .spells = std::move(spells)};
}

/**
 * @brief Un mannequin : @p armorClass de CA, @p bonus au toucher, 1d6+2 tranchants, @p hitPoints
 *        PV ; il joue après le héros.
 */
inline core::ArenaContestant dummy(const char* name, core::GridPosition place, int armorClass,
                                   int bonus, int hitPoints = 60,
                                   core::CombatSide side = core::CombatSide::Enemies) {
    core::CombatantProfile profile{.name = name,
                                   .side = side,
                                   .maximumHitPoints = hitPoints,
                                   .currentHitPoints = hitPoints,
                                   .dexterity = 10,
                                   .initiativeModifier = -100,
                                   .movement = 6};
    profile.armorClass = armorClass;
    core::AttackProfile blow;
    blow.label = "Cimeterre";
    blow.modifiers = {{.source = "bonus d'attaque", .value = bonus}};
    blow.damage = {
        {.dice = *core::parseDice("1d6+2"), .type = core::DamageType::Slashing, .flags = 0}};
    return {.profile = profile, .attacks = {blow}, .position = place, .markId = {}, .behavior = {}};
}

/// @return Vrai si une ligne du journal contient @p text.
inline bool journalHas(const std::vector<std::string>& journal, const std::string& text) {
    return std::ranges::any_of(
        journal, [&](const std::string& line) { return line.find(text) != std::string::npos; });
}

/// @return La première ligne du journal qui contient @p text, ou vide.
inline std::string journalLine(const std::vector<std::string>& journal, const std::string& text) {
    const auto found = std::ranges::find_if(
        journal, [&](const std::string& line) { return line.find(text) != std::string::npos; });
    return found == journal.end() ? std::string{} : *found;
}

/// @return Le nombre de lignes du journal qui contiennent @p text.
inline std::size_t journalCount(const std::vector<std::string>& journal, const std::string& text) {
    return static_cast<std::size_t>(std::ranges::count_if(
        journal, [&](const std::string& line) { return line.find(text) != std::string::npos; }));
}

}  // namespace test_support
