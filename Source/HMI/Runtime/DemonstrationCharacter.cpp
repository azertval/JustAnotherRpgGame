// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/DemonstrationCharacter.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Equipment.h"
#include "Core/Rpg/Inventory.h"
#include "Core/Rpg/Skill.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Presentation/CharacterSheetValues.h"
#include "HMI/Presentation/InventoryValues.h"
#include "HMI/Runtime/WorldModel.h"

namespace hmi {
namespace {

// Fichier du personnage joué quand aucune partie n'en désigne un, dans `Rpg/characters/` : le
// Brawler pré-tiré (`LOT-112`), meneur du groupe de départ.
constexpr const char* DEMONSTRATION_CHARACTER_FILE = "heros-brawler.json";

// Le même signe que les autres écrans posent sur un champ sans source.
constexpr const char* EMPTY_MARK = "—";

// Ce que la partie en cours dit de la fiche `characterFile` -- niveau donne, points de vie,
// lancers -- s'applique a `state` (LOT-141). Sans partie, ou pour une fiche qui n'est pas celle
// d'un de ses personnages : rien.
void applyPartyRecordFor(DemonstrationState& state, const std::filesystem::path& characterFile) {
    const WorldModel* const partie = WorldModel::current();
    if (partie == nullptr) {
        return;
    }
    std::error_code erreur;
    for (const core::PartyCandidate& candidat : partie->candidates()) {
        if (candidat.file != characterFile &&
            !std::filesystem::equivalent(candidat.file, characterFile, erreur)) {
            continue;
        }
        if (const core::MemberRecord* const record = partie->ledger().record(candidat.id)) {
            applyMemberRecord(state, *record);
        }
        return;
    }
}

// Chaque message au journal, en avertissement, precede de `prefix`.
void logWarnings(const std::string& prefix, const std::vector<std::string>& messages) {
    for (const std::string& message : messages) {
        HMI_LOG_WARNING(prefix + message);
    }
}

}  // namespace

void applyMemberRecord(DemonstrationState& state, const core::MemberRecord& record) {
    if (record.level.has_value() && *record.level > state.sheet.level) {
        if (const core::PlayableClass* const classe =
                state.options.findClass(state.sheet.classId)) {
            std::vector<std::string> manquants;
            static_cast<void>(core::levelUpTo(state.sheet, *record.level, *classe, state.options,
                                              state.rules, state.experience, manquants));
            for (const std::string& manquant : manquants) {
                HMI_LOG_WARNING("Montee de niveau : '" + manquant +
                                "' nomme par la classe, que le moteur ne joue pas (EX-CNT-031).");
            }
        }
    }
    core::applyRecord(state.sheet, record);
    if (record.inventory.has_value()) {
        state.inventory = *record.inventory;
    }
}

std::filesystem::path playedCharacterFile() {
    // Le meneur du groupe de la partie en cours (LOT-138) : c'est lui qui combat, qui parle a
    // l'ouverture d'un dialogue (le joueur peut donner la parole a un autre, D-28), et dont la
    // fiche s'ouvre.
    if (const WorldModel* const partie = WorldModel::current()) {
        if (std::filesystem::path meneur = partie->leaderSheetFile(); !meneur.empty()) {
            return meneur;
        }
    }
    return executableDirectory() / "Rpg" / "characters" / DEMONSTRATION_CHARACTER_FILE;
}

DemonstrationState loadDemonstrationState() {
    return loadDemonstrationState(playedCharacterFile());
}

DemonstrationState loadDemonstrationState(const std::filesystem::path& characterFile) {
    const std::filesystem::path rpg = executableDirectory() / "Rpg";

    DemonstrationState state;
    // Especes, historiques, classes, et ce que les tables de classe designent : capacites et
    // sorts (LOT-131).
    state.options = core::loadCharacterOptions(rpg);
    logWarnings("Options de personnage : ", state.options.errors);
    state.skills = core::loadSkills(rpg / "skills");
    state.experience = core::loadExperienceTable(rpg / "rules" / "experience.json");
    state.rules = core::loadCharacterCreationRules(rpg / "rules" / "character-creation.json");

    core::LoadedCharacterSheet loaded =
        core::loadCharacterSheet(characterFile, state.options, state.rules, state.experience);
    // Journalise et poursuit : une fiche partielle vaut mieux qu'un écran vide, et l'erreur
    // nomme son fichier (EX-CNT-010).
    logWarnings("Personnage de demonstration : ", loaded.errors);
    // Une capacite ou un sort que la classe nomme et que le moteur ne joue pas encore
    // (EX-CNT-031) : dit au journal, jamais joue en silence.
    logWarnings("Personnage de demonstration : ", loaded.warnings);
    state.sheet = std::move(loaded.sheet);
    state.inventory = std::move(loaded.inventory);
    applyPartyRecordFor(state, characterFile);

    state.items = core::loadItems(rpg / "items");
    state.equipment = core::loadEquipment(rpg / "weapons", rpg / "armors");
    state.encumbrance = core::loadEncumbranceRules(rpg / "rules" / "encumbrance.json");
    logWarnings("Catalogue d'objets : ", state.items.errors);
    for (const std::string& unknown : core::unknownIds(state.inventory, state.lookup())) {
        // Un identifiant que rien ne porte ne pèse rien et s'affiche tel quel : le dire vaut mieux
        // que de peser faux en silence (EX-CNT-010).
        HMI_LOG_WARNING("Inventaire de demonstration : objet inconnu '" + unknown + "'.");
    }
    return state;
}

std::vector<DemonstrationCharacter> loadCharacterValues(
    const std::vector<std::filesystem::path>& characterFiles) {
    std::vector<DemonstrationCharacter> result;
    if (characterFiles.empty()) {
        return result;
    }
    // Les catalogues avec la premiere fiche ; les suivantes se lisent dans les memes catalogues.
    DemonstrationState state = loadDemonstrationState(characterFiles.front());
    result.push_back(demonstrationValues(state));
    for (std::size_t rank = 1; rank < characterFiles.size(); ++rank) {
        core::LoadedCharacterSheet loaded = core::loadCharacterSheet(
            characterFiles[rank], state.options, state.rules, state.experience);
        for (const std::string& error : loaded.errors) {
            HMI_LOG_WARNING("Groupe : " + error);
        }
        state.sheet = std::move(loaded.sheet);
        state.inventory = std::move(loaded.inventory);
        applyPartyRecordFor(state, characterFiles[rank]);
        result.push_back(demonstrationValues(state));
    }
    return result;
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
