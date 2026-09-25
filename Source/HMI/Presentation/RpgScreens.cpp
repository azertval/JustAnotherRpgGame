// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Presentation/RpgScreens.h"

#include <array>

namespace hmi {
namespace {

// --- Ossature des neuf écrans (LOT-68) ---------------------------------------------------------
//
// Chaque écran est décrit ICI, en données pures : l'ossature est posée par les formulaires Qt Quick
// sans qu'aucun code ne connaisse un écran par son nom (`EX-IHM-090`).
//
// Un champ porte l'identifiant sous lequel sa valeur arrive, ou une chaîne vide. **Vide** veut
// dire : ce champ est à l'écran, et rien ne l'alimente encore — il garde son tiret cadratin,
// jamais un zéro, qui se lirait comme un état du jeu et mentirait. La colonne des identifiants
// vides EST le périmètre restant.

constexpr std::array IDENTITY_FIELDS = {
    RpgField{.labelKey = "rpg.field.name", .valueId = "sheet.name"},
    RpgField{.labelKey = "rpg.field.species", .valueId = "sheet.species"},
    RpgField{.labelKey = "rpg.field.class", .valueId = "sheet.class"},
    RpgField{.labelKey = "rpg.field.background", .valueId = "sheet.background"},
};

constexpr std::array PROGRESSION_FIELDS = {
    RpgField{.labelKey = "rpg.field.level", .valueId = "sheet.level"},
    RpgField{.labelKey = "rpg.field.experience", .valueId = "sheet.experience"},
};

constexpr std::array COMBAT_FIELDS = {
    RpgField{.labelKey = "rpg.field.hit_points", .valueId = "sheet.hit_points"},
    RpgField{.labelKey = "rpg.field.armor_class", .valueId = "sheet.armor_class"},
    RpgField{.labelKey = "rpg.field.initiative", .valueId = "sheet.initiative"},
    RpgField{.labelKey = "rpg.field.speed", .valueId = "sheet.speed"},
    RpgField{.labelKey = "rpg.field.proficiency_bonus", .valueId = "sheet.proficiency_bonus"},
    RpgField{.labelKey = "rpg.field.hit_dice", .valueId = "sheet.hit_dice"},
    RpgField{.labelKey = "rpg.field.passive_perception", .valueId = "sheet.passive_perception"},
};

constexpr std::array ABILITY_FIELDS = {
    RpgField{.labelKey = "rpg.ability.strength", .valueId = "sheet.ability.strength"},
    RpgField{.labelKey = "rpg.ability.dexterity", .valueId = "sheet.ability.dexterity"},
    RpgField{.labelKey = "rpg.ability.constitution", .valueId = "sheet.ability.constitution"},
    RpgField{.labelKey = "rpg.ability.intelligence", .valueId = "sheet.ability.intelligence"},
    RpgField{.labelKey = "rpg.ability.wisdom", .valueId = "sheet.ability.wisdom"},
    RpgField{.labelKey = "rpg.ability.charisma", .valueId = "sheet.ability.charisma"},
};

constexpr std::array SAVING_THROW_FIELDS = {
    RpgField{.labelKey = "rpg.ability.strength", .valueId = "sheet.save.strength"},
    RpgField{.labelKey = "rpg.ability.dexterity", .valueId = "sheet.save.dexterity"},
    RpgField{.labelKey = "rpg.ability.constitution", .valueId = "sheet.save.constitution"},
    RpgField{.labelKey = "rpg.ability.intelligence", .valueId = "sheet.save.intelligence"},
    RpgField{.labelKey = "rpg.ability.wisdom", .valueId = "sheet.save.wisdom"},
    RpgField{.labelKey = "rpg.ability.charisma", .valueId = "sheet.save.charisma"},
};

// La charge est calculée par le noyau du `LOT-14` (`core::derivedStatsFor`) : poids porté,
// capacité, et la bourse. Rien n'est accumulé — tout est relu depuis ce qui est porté.
constexpr std::array LOAD_FIELDS = {
    RpgField{.labelKey = "rpg.field.carried", .valueId = "inventory.carried"},
    RpgField{.labelKey = "rpg.field.capacity", .valueId = "inventory.capacity"},
    RpgField{.labelKey = "rpg.field.gold", .valueId = "inventory.purse"},
};

// Les seize emplacements du noyau (`core::EquipmentSlot`), dans son ordre. Les intitulés sont ceux
// du lexique, et les identifiants ceux que `hmi::inventoryValues` produit : une seule liste, et un
// test vérifie que les deux côtés ne divergent pas.
constexpr std::array EQUIPMENT_SLOTS = {
    RpgField{.labelKey = "rpg.slot.head", .valueId = "inventory.slot.head"},
    RpgField{.labelKey = "rpg.slot.neck", .valueId = "inventory.slot.neck"},
    RpgField{.labelKey = "rpg.slot.cloak", .valueId = "inventory.slot.cloak"},
    RpgField{.labelKey = "rpg.slot.torso", .valueId = "inventory.slot.torso"},
    RpgField{.labelKey = "rpg.slot.belt", .valueId = "inventory.slot.belt"},
    RpgField{.labelKey = "rpg.slot.hands", .valueId = "inventory.slot.hands"},
    RpgField{.labelKey = "rpg.slot.ring_left", .valueId = "inventory.slot.ring-left"},
    RpgField{.labelKey = "rpg.slot.ring_right", .valueId = "inventory.slot.ring-right"},
    RpgField{.labelKey = "rpg.slot.main_hand", .valueId = "inventory.slot.main-hand"},
    RpgField{.labelKey = "rpg.slot.off_hand", .valueId = "inventory.slot.off-hand"},
    RpgField{.labelKey = "rpg.slot.ranged", .valueId = "inventory.slot.ranged"},
    RpgField{.labelKey = "rpg.slot.ammunition", .valueId = "inventory.slot.ammunition"},
    RpgField{.labelKey = "rpg.slot.feet", .valueId = "inventory.slot.feet"},
    RpgField{.labelKey = "rpg.slot.bracers", .valueId = "inventory.slot.bracers"},
    RpgField{.labelKey = "rpg.slot.pouch", .valueId = "inventory.slot.pouch"},
    RpgField{.labelKey = "rpg.slot.trinket", .valueId = "inventory.slot.trinket"},
};

constexpr std::array PLACE_FIELDS = {
    RpgField{.labelKey = "rpg.field.region"},
    RpgField{.labelKey = "rpg.field.place_type"},
    RpgField{.labelKey = "rpg.field.danger"},
};

constexpr std::array SPEAKER_FIELDS = {
    RpgField{.labelKey = "rpg.field.name"},
    RpgField{.labelKey = "rpg.field.attitude"},
};

constexpr std::array PURSE_FIELDS = {RpgField{.labelKey = "rpg.field.gold"}};

constexpr std::array CONTRACT_FIELDS = {
    RpgField{.labelKey = "rpg.field.giver"},
    RpgField{.labelKey = "rpg.field.rank"},
    RpgField{.labelKey = "rpg.field.reward"},
};

constexpr std::array TARGET_FIELDS = {
    RpgField{.labelKey = "rpg.field.name"},
    RpgField{.labelKey = "rpg.field.hit_points"},
    RpgField{.labelKey = "rpg.field.armor_class"},
    RpgField{.labelKey = "rpg.field.conditions"},
};

// Les dix-huit competences du catalogue (LOT-43), dans son ordre. Leur INTITULE vient du lexique
// et se pose par le presentateur : la liste ne porte donc que les identifiants de valeur, et
// `hmi::characterSheetValues` produit un texte deja assemble -- << Acrobaties  +3 * >>.
constexpr std::array SKILL_VALUE_IDS = {
    "sheet.skill.acrobatics",      "sheet.skill.animal-handling", "sheet.skill.arcana",
    "sheet.skill.athletics",       "sheet.skill.deception",       "sheet.skill.history",
    "sheet.skill.insight",         "sheet.skill.intimidation",    "sheet.skill.investigation",
    "sheet.skill.medicine",        "sheet.skill.nature",          "sheet.skill.perception",
    "sheet.skill.performance",     "sheet.skill.persuasion",      "sheet.skill.religion",
    "sheet.skill.sleight-of-hand", "sheet.skill.stealth",         "sheet.skill.survival",
};

constexpr std::array CHARACTER_SHEET_LEFT = {
    RpgContentBlock{
        .titleKey = "rpg.block.identity", .kind = RpgBlockKind::Fields, .fields = IDENTITY_FIELDS},
    RpgContentBlock{.titleKey = "rpg.block.progression",
                    .kind = RpgBlockKind::Fields,
                    .fields = PROGRESSION_FIELDS},
    RpgContentBlock{
        .titleKey = "rpg.block.abilities", .kind = RpgBlockKind::Fields, .fields = ABILITY_FIELDS},
};
constexpr std::array CHARACTER_SHEET_RIGHT = {
    RpgContentBlock{
        .titleKey = "rpg.block.combat", .kind = RpgBlockKind::Fields, .fields = COMBAT_FIELDS},
    RpgContentBlock{.titleKey = "rpg.block.saving_throws",
                    .kind = RpgBlockKind::Fields,
                    .fields = SAVING_THROW_FIELDS},
    // Le catalogue en porte dix-huit (LOT-43) ; l'ossature les montre toutes, et défile.
    RpgContentBlock{
        .titleKey = "rpg.block.skills", .kind = RpgBlockKind::List, .valueIds = SKILL_VALUE_IDS},
};

constexpr std::array INVENTORY_LEFT = {
    RpgContentBlock{
        .titleKey = "rpg.block.equipment", .kind = RpgBlockKind::Fields, .fields = EQUIPMENT_SLOTS},
    RpgContentBlock{
        .titleKey = "rpg.block.load", .kind = RpgBlockKind::Fields, .fields = LOAD_FIELDS},
};
// Le sac est une LISTE, et non la grille de cases du LOT-68 : un inventaire de jeu de role se lit
// ligne a ligne, avec le nom et la quantite, la ou une grille de cases suppose des objets de meme
// encombrement -- ce que le modele ne dit nulle part.
constexpr std::array BACKPACK_VALUE_IDS = {"inventory.backpack"};
constexpr std::array INVENTORY_RIGHT = {
    RpgContentBlock{
        .titleKey = "rpg.block.bag", .kind = RpgBlockKind::Prose, .valueIds = BACKPACK_VALUE_IDS},
};

constexpr std::array JOURNAL_LEFT = {
    RpgContentBlock{.titleKey = "rpg.block.quests", .kind = RpgBlockKind::List, .rows = 8},
};
constexpr std::array JOURNAL_RIGHT = {
    RpgContentBlock{.titleKey = "rpg.block.quest_detail", .kind = RpgBlockKind::Prose},
    RpgContentBlock{.titleKey = "rpg.block.objectives", .kind = RpgBlockKind::List, .rows = 4},
};

constexpr std::array WORLD_MAP_LEFT = {
    // Treize régions à l'atlas (LOT-37) ; la liste en montre six et défile.
    RpgContentBlock{.titleKey = "rpg.block.regions", .kind = RpgBlockKind::List, .rows = 6},
    RpgContentBlock{
        .titleKey = "rpg.block.place", .kind = RpgBlockKind::Fields, .fields = PLACE_FIELDS},
};
constexpr std::array WORLD_MAP_RIGHT = {
    RpgContentBlock{.titleKey = "rpg.block.map", .kind = RpgBlockKind::Portrait},
};

constexpr std::array DIALOGUE_LEFT = {
    RpgContentBlock{.titleKey = "rpg.block.speaker", .kind = RpgBlockKind::Portrait},
    RpgContentBlock{.titleKey = "", .kind = RpgBlockKind::Fields, .fields = SPEAKER_FIELDS},
};
constexpr std::array DIALOGUE_RIGHT = {
    RpgContentBlock{.titleKey = "rpg.block.line", .kind = RpgBlockKind::Prose},
    RpgContentBlock{.titleKey = "rpg.block.replies", .kind = RpgBlockKind::List, .rows = 4},
};

constexpr std::array MERCHANT_LEFT = {
    RpgContentBlock{.titleKey = "rpg.block.goods", .kind = RpgBlockKind::List, .rows = 8},
    RpgContentBlock{
        .titleKey = "rpg.block.purse", .kind = RpgBlockKind::Fields, .fields = PURSE_FIELDS},
};
constexpr std::array MERCHANT_RIGHT = {
    RpgContentBlock{.titleKey = "rpg.block.your_bag", .kind = RpgBlockKind::List, .rows = 8},
};

constexpr std::array COMBAT_HUD_LEFT = {
    RpgContentBlock{.titleKey = "rpg.block.initiative", .kind = RpgBlockKind::Track, .columns = 6},
    RpgContentBlock{
        .titleKey = "rpg.block.target", .kind = RpgBlockKind::Fields, .fields = TARGET_FIELDS},
    RpgContentBlock{.titleKey = "rpg.block.actions", .kind = RpgBlockKind::ActionBar, .columns = 6},
};

// --- Ossature de la FEUILLE D'ÉQUIPE (planche 5, LOT-38) --------------------------------------
//
// La cinquième planche n'est pas la fiche d'un personnage : c'est celle de son ÉQUIPE — renommée,
// blason, quartier général, mécénat. La ranger dans la fiche aurait mêlé deux sujets sur un même
// écran ; elle en a donc un à elle.
//
// Cet écran est aussi la preuve de ce que le `LOT-68` affirmait : il s'ajoute par une entrée de
// table et ses clés de traduction, sans qu'aucun des huit autres, ni la feuille de style, ni le
// châssis, n'aient été touchés (`EX-IHM-090`).

constexpr std::array TEAM_FIELDS = {
    RpgField{.labelKey = "rpg.field.team_name"},
    RpgField{.labelKey = "rpg.field.career_points"},
    RpgField{.labelKey = "rpg.field.fame"},
    RpgField{.labelKey = "rpg.field.prestige"},
    RpgField{.labelKey = "rpg.field.beneficiary"},
    RpgField{.labelKey = "rpg.field.style"},
    RpgField{.labelKey = "rpg.field.specialization"},
};

constexpr std::array COMPANY_LEFT = {
    RpgContentBlock{
        .titleKey = "rpg.block.team", .kind = RpgBlockKind::Fields, .fields = TEAM_FIELDS},
    RpgContentBlock{.titleKey = "rpg.block.team_members", .kind = RpgBlockKind::List, .rows = 4},
    RpgContentBlock{.titleKey = "rpg.block.relations", .kind = RpgBlockKind::Prose},
    // Les contrats de la Guilde (onglet « Contrats », LOT-87 T3.7).
    RpgContentBlock{.titleKey = "rpg.block.contracts", .kind = RpgBlockKind::List, .rows = 6},
};
constexpr std::array COMPANY_RIGHT = {
    RpgContentBlock{.titleKey = "rpg.block.coat_of_arms", .kind = RpgBlockKind::Portrait},
    RpgContentBlock{.titleKey = "rpg.block.dream", .kind = RpgBlockKind::Prose},
    RpgContentBlock{.titleKey = "rpg.block.hidden_agenda", .kind = RpgBlockKind::Prose},
    RpgContentBlock{.titleKey = "rpg.block.legendary_rewards", .kind = RpgBlockKind::Prose},
    // Huit installations au quartier général de la planche.
    RpgContentBlock{.titleKey = "rpg.block.headquarters", .kind = RpgBlockKind::List, .rows = 8},
    RpgContentBlock{.titleKey = "rpg.block.contract", .kind = RpgBlockKind::Prose},
    RpgContentBlock{.titleKey = "", .kind = RpgBlockKind::Fields, .fields = CONTRACT_FIELDS},
};

// --- Ossature des COMPÉTENCES ET SORTS (maquette 10, LOT-87 T3.8) -------------------------------
//
// Aucune donnée avant le LOT-35 : l'écran est dessiné, chaque valeur passe par PendingData.
constexpr std::array SKILLS_LEFT = {
    RpgContentBlock{.titleKey = "rpg.block.attacks", .kind = RpgBlockKind::List, .rows = 2},
    RpgContentBlock{.titleKey = "rpg.block.cantrips", .kind = RpgBlockKind::List, .rows = 4},
};
constexpr std::array SKILLS_RIGHT = {
    RpgContentBlock{.titleKey = "rpg.block.spell_schools", .kind = RpgBlockKind::List, .rows = 8},
    RpgContentBlock{.titleKey = "rpg.block.spell", .kind = RpgBlockKind::Prose},
};

// --- La table ---------------------------------------------------------------------------------
//
// C'est ELLE, et rien d'autre, qu'un neuvième écran vient allonger (EX-IHM-090) : le châssis Qt ne
// connaît aucun écran par son nom, il peint ce que cette table décrit.
constexpr std::array<RpgScreenDescriptor, RPG_SCREEN_COUNT> SCREENS = {{
    {.id = RpgScreenId::CharacterSheet,
     .objectName = "RpgCharacterSheetScreen",
     .titleKey = "rpg.character_sheet.title",
     .superposition = RpgSuperposition::PausesGame,
     .layout = {.leftColumn = CHARACTER_SHEET_LEFT, .rightColumn = CHARACTER_SHEET_RIGHT}},
    // Les sorts se préparent à l'arrêt, comme la fiche d'où on les ouvre.
    {.id = RpgScreenId::Skills,
     .objectName = "RpgSkillsScreen",
     .titleKey = "rpg.skills.title",
     .superposition = RpgSuperposition::PausesGame,
     .layout = {.leftColumn = SKILLS_LEFT, .rightColumn = SKILLS_RIGHT}},
    {.id = RpgScreenId::Inventory,
     .objectName = "RpgInventoryScreen",
     .titleKey = "rpg.inventory.title",
     .superposition = RpgSuperposition::PausesGame,
     .layout = {.leftColumn = INVENTORY_LEFT, .rightColumn = INVENTORY_RIGHT}},
    {.id = RpgScreenId::QuestJournal,
     .objectName = "RpgJournalScreen",
     .titleKey = "rpg.journal.title",
     .superposition = RpgSuperposition::PausesGame,
     .layout = {.leftColumn = JOURNAL_LEFT, .rightColumn = JOURNAL_RIGHT}},
    // La carte se consulte EN MARCHANT : c'est ce pour quoi on l'ouvre -- savoir où l'on va sans
    // s'arrêter. Suspendre la simulation en ferait un écran de bilan.
    {.id = RpgScreenId::WorldMap,
     .objectName = "RpgWorldMapScreen",
     .titleKey = "rpg.world_map.title",
     .superposition = RpgSuperposition::WhileWalking,
     .layout = {.leftColumn = WORLD_MAP_LEFT, .rightColumn = WORLD_MAP_RIGHT}},
    // Un dialogue suspend : l'interlocuteur attend une réponse, il ne la reçoit pas en courant.
    {.id = RpgScreenId::Dialogue,
     .objectName = "RpgDialogueScreen",
     .titleKey = "rpg.dialogue.title",
     .superposition = RpgSuperposition::PausesGame,
     .layout = {.leftColumn = DIALOGUE_LEFT, .rightColumn = DIALOGUE_RIGHT}},
    {.id = RpgScreenId::Merchant,
     .objectName = "RpgMerchantScreen",
     .titleKey = "rpg.merchant.title",
     .superposition = RpgSuperposition::PausesGame,
     .layout = {.leftColumn = MERCHANT_LEFT, .rightColumn = MERCHANT_RIGHT}},
    // L'équipe de mercenaires (LOT-87, T3.7) : feuille d'équipe et tableau de la Guilde réunis.
    // Une équipe se consulte à l'arrêt, comme une fiche.
    {.id = RpgScreenId::Company,
     .objectName = "RpgCompanyScreen",
     .titleKey = "rpg.company.title",
     .superposition = RpgSuperposition::PausesGame,
     .layout = {.leftColumn = COMPANY_LEFT, .rightColumn = COMPANY_RIGHT}},
    // L'ATH de combat est le seul écran qui ne s'ouvre PAS par-dessus le jeu : il EST le jeu
    // pendant un combat. Il ne suspend donc rien, et c'est core::CombatState (LOT-20) qui tient son
    // rythme -- pas cette table.
    {.id = RpgScreenId::CombatHud,
     .objectName = "RpgCombatHudScreen",
     .titleKey = "rpg.combat_hud.title",
     .superposition = RpgSuperposition::WhileWalking,
     .layout = {.leftColumn = COMBAT_HUD_LEFT}},
}};

// Rend : Le rang de `screen` dans la table.
[[nodiscard]] std::size_t indexOf(RpgScreenId screen) noexcept {
    for (std::size_t rank = 0; rank < SCREENS.size(); ++rank) {
        if (SCREENS[rank].id == screen) {
            return rank;
        }
    }
    return 0;
}

}  // namespace

std::span<const RpgScreenDescriptor> rpgScreens() noexcept {
    return SCREENS;
}

const RpgScreenDescriptor& rpgScreenDescriptor(RpgScreenId screen) noexcept {
    return SCREENS[indexOf(screen)];
}

RpgScreenId nextRpgScreen(RpgScreenId screen) noexcept {
    return SCREENS[(indexOf(screen) + 1) % SCREENS.size()].id;
}

RpgScreenId previousRpgScreen(RpgScreenId screen) noexcept {
    return SCREENS[(indexOf(screen) + SCREENS.size() - 1) % SCREENS.size()].id;
}

bool pausesGame(RpgScreenId screen) noexcept {
    return rpgScreenDescriptor(screen).superposition == RpgSuperposition::PausesGame;
}

}  // namespace hmi
