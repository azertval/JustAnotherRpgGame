// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/DemonstrationCharacter.h"

#include <filesystem>
#include <utility>

#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Equipment.h"
#include "Core/Rpg/Inventory.h"
#include "Core/Rpg/Skill.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Presentation/CharacterSheetValues.h"
#include "HMI/Presentation/InventoryValues.h"

namespace hmi {
namespace {

// Fichier du personnage joué, dans `Rpg/characters/` : le héros de la démo, la fiche pré-tirée du
// Brawler (`LOT-112`). La démo n'a pas de création de personnage ; le groupe et la sauvegarde
// fourniront le leur.
constexpr const char* DEMONSTRATION_CHARACTER_FILE = "heros-brawler.json";

// Le même signe que les autres écrans posent sur un champ sans source.
constexpr const char* EMPTY_MARK = "—";

}  // namespace

DemonstrationState loadDemonstrationState() {
    const std::filesystem::path rpg = executableDirectory() / "Rpg";

    DemonstrationState state;
    state.options =
        core::loadCharacterOptions(rpg / "species", rpg / "backgrounds", rpg / "classes");
    state.skills = core::loadSkills(rpg / "skills");
    state.experience = core::loadExperienceTable(rpg / "rules" / "experience.json");
    state.rules = core::loadCharacterCreationRules(rpg / "rules" / "character-creation.json");

    core::LoadedCharacterSheet loaded =
        core::loadCharacterSheet(rpg / "characters" / DEMONSTRATION_CHARACTER_FILE, state.options,
                                 state.rules, state.experience);
    for (const std::string& error : loaded.errors) {
        // Journalise et poursuit : une fiche partielle vaut mieux qu'un écran vide, et l'erreur
        // nomme son fichier (EX-CNT-010).
        HMI_LOG_WARNING("Personnage de demonstration : " + error);
    }
    state.sheet = std::move(loaded.sheet);
    state.inventory = std::move(loaded.inventory);

    state.items = core::loadItems(rpg / "items");
    state.equipment = core::loadEquipment(rpg / "weapons", rpg / "armors");
    state.encumbrance = core::loadEncumbranceRules(rpg / "rules" / "encumbrance.json");
    for (const std::string& error : state.items.errors) {
        HMI_LOG_WARNING("Catalogue d'objets : " + error);
    }
    for (const std::string& unknown : core::unknownIds(state.inventory, state.lookup())) {
        // Un identifiant que rien ne porte ne pèse rien et s'affiche tel quel : le dire vaut mieux
        // que de peser faux en silence (EX-CNT-010).
        HMI_LOG_WARNING("Inventaire de demonstration : objet inconnu '" + unknown + "'.");
    }
    return state;
}

DemonstrationCharacter loadDemonstrationValues() {
    return demonstrationValues(loadDemonstrationState());
}

DemonstrationCharacter demonstrationValues(const DemonstrationState& state) {
    const core::ItemLookup lookup = state.lookup();
    const core::CharacterOptions& options = state.options;
    const core::SkillCatalog& skills = state.skills;
    const core::ExperienceTable& experience = state.experience;

    // Les statistiques dérivées sont RECALCULÉES ici, jamais retenues : c'est ce qui les empêche
    // de dériver quand on équipe et retire dans le désordre (LOT-14).
    const core::DerivedStats derived =
        core::derivedStatsFor(state.sheet, state.inventory, lookup, state.rules, state.encumbrance);

    DemonstrationCharacter result;
    // L'inventaire D'ABORD, parce que la fiche en dépend : sa classe d'armure et sa vitesse
    // viennent de ce qui est porté, pas de la construction du personnage.
    result.inventory = inventoryValues({.inventory = &state.inventory,
                                        .lookup = lookup,
                                        .derived = derived,
                                        .emptyMark = EMPTY_MARK});
    result.sheet = characterSheetValues({.sheet = &state.sheet,
                                         .options = &options,
                                         .experience = &experience,
                                         .skills = &skills,
                                         .derived = &derived,
                                         .emptyMark = EMPTY_MARK});

    result.skills.reserve(skills.skills.size());
    for (const core::SkillDefinition& skill : skills.skills) {
        result.skills.emplace_back(skill.id, skill.name);
    }
    return result;
}

}  // namespace hmi
