// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Rpg/Inventory.h"

#include <algorithm>
#include <array>
#include <system_error>
#include <utility>

#include "Core/Data/JsonDocument.h"
#include "Core/Rpg/CharacterSheet.h"

namespace core {
namespace {

// Les entrees de catalogue ne portent pas de champ `version` : ce sont des donnees, pas des
// documents de format. Meme convention que `loadEquipment` (LOT-34).
constexpr int SANS_GARDE_DE_VERSION = 0;

[[nodiscard]] std::string lireTexte(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_string()) ? trouve->get<std::string>()
                                                          : std::string{};
}

[[nodiscard]] int lireEntier(const nlohmann::json& objet, const char* champ, int defaut = 0) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_number_integer()) ? trouve->get<int>() : defaut;
}

[[nodiscard]] float lireReel(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_number()) ? trouve->get<float>() : 0.0F;
}

[[nodiscard]] bool lireBooleen(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return trouve != objet.end() && trouve->is_boolean() && trouve->get<bool>();
}

// Les seize emplacements et leur nom de donnee. Un seul endroit, parcouru dans les deux sens :
// deux listes finiraient par diverger, et l'ecart ne se verrait qu'a l'ecran.
constexpr std::array<std::pair<EquipmentSlot, const char*>, EQUIPMENT_SLOT_COUNT> EMPLACEMENTS = {{
    {EquipmentSlot::Head, "head"},
    {EquipmentSlot::Neck, "neck"},
    {EquipmentSlot::Cloak, "cloak"},
    {EquipmentSlot::Torso, "torso"},
    {EquipmentSlot::Belt, "belt"},
    {EquipmentSlot::Hands, "hands"},
    {EquipmentSlot::RingLeft, "ring-left"},
    {EquipmentSlot::RingRight, "ring-right"},
    {EquipmentSlot::MainHand, "main-hand"},
    {EquipmentSlot::OffHand, "off-hand"},
    {EquipmentSlot::Ranged, "ranged"},
    {EquipmentSlot::Ammunition, "ammunition"},
    {EquipmentSlot::Feet, "feet"},
    {EquipmentSlot::Bracers, "bracers"},
    {EquipmentSlot::Pouch, "pouch"},
    {EquipmentSlot::Trinket, "trinket"},
}};

}  // namespace

std::string_view equipmentSlotName(EquipmentSlot slot) {
    for (const auto& [valeur, nom] : EMPLACEMENTS) {
        if (valeur == slot) {
            return nom;
        }
    }
    return {};
}

std::optional<EquipmentSlot> parseEquipmentSlot(std::string_view name) {
    for (const auto& [valeur, nom] : EMPLACEMENTS) {
        if (name == nom) {
            return valeur;
        }
    }
    return std::nullopt;
}

const Item* ItemCatalog::find(std::string_view id) const {
    const auto trouve = std::ranges::find(items, id, &Item::id);
    return trouve == items.end() ? nullptr : &*trouve;
}

ItemCatalog loadItems(const std::filesystem::path& itemsDir) {
    ItemCatalog catalogue;
    std::error_code code;
    if (!std::filesystem::is_directory(itemsDir, code)) {
        catalogue.errors.push_back(itemsDir.string() + " : dossier absent ou illisible.");
        return catalogue;
    }
    std::vector<std::filesystem::path> fichiers;
    for (const auto& entree : std::filesystem::directory_iterator(itemsDir, code)) {
        if (entree.is_regular_file(code) && entree.path().extension() == ".json") {
            fichiers.push_back(entree.path());
        }
    }
    std::ranges::sort(fichiers);
    for (const std::filesystem::path& chemin : fichiers) {
        const JsonDocument document = readJsonObjectFromFile(chemin, SANS_GARDE_DE_VERSION);
        if (!document.ok()) {
            catalogue.errors.push_back(document.message);
            continue;
        }
        const nlohmann::json& racine = document.root;
        Item objet;
        objet.id = lireTexte(racine, "id");
        objet.name = lireTexte(racine, "name");
        objet.source = lireTexte(racine, "source");
        objet.category = lireTexte(racine, "category");
        objet.price = lireEntier(racine, "price");
        objet.weightGrams = lireEntier(racine, "weightGrams");
        objet.stackable = lireBooleen(racine, "stackable");
        objet.rarity = lireTexte(racine, "rarity");
        objet.requiresAttunement = lireBooleen(racine, "requiresAttunement");
        objet.text = lireTexte(racine, "text");
        if (objet.id.empty()) {
            catalogue.errors.push_back(chemin.filename().string() + " : objet sans identifiant.");
            continue;
        }
        catalogue.items.push_back(std::move(objet));
    }
    return catalogue;
}

EncumbranceRules loadEncumbranceRules(const std::filesystem::path& rulesFile) {
    EncumbranceRules regles;
    const JsonDocument document = readJsonObjectFromFile(rulesFile, SANS_GARDE_DE_VERSION);
    if (!document.ok()) {
        return regles;
    }
    const nlohmann::json& racine = document.root;
    regles.carryingCapacityGramsPerStrength =
        lireEntier(racine, "carryingCapacityGramsPerStrength");
    regles.encumberedGramsPerStrength = lireEntier(racine, "encumberedGramsPerStrength");
    regles.heavilyEncumberedGramsPerStrength =
        lireEntier(racine, "heavilyEncumberedGramsPerStrength");
    regles.encumberedSpeedPenaltyMeters = lireReel(racine, "encumberedSpeedPenaltyMeters");
    regles.heavilyEncumberedSpeedPenaltyMeters =
        lireReel(racine, "heavilyEncumberedSpeedPenaltyMeters");
    return regles;
}

std::string equip(Inventory& inventory, EquipmentSlot slot, std::string itemId) {
    std::string precedent = std::move(inventory.equipped[static_cast<std::size_t>(slot)]);
    inventory.equipped[static_cast<std::size_t>(slot)] = std::move(itemId);
    return precedent;
}

std::string unequip(Inventory& inventory, EquipmentSlot slot) {
    return equip(inventory, slot, std::string{});
}

void addToBackpack(Inventory& inventory, const std::string& itemId, int quantity) {
    if (itemId.empty() || quantity <= 0) {
        return;
    }
    const auto trouve = std::ranges::find(inventory.backpack, itemId, &InventoryStack::itemId);
    if (trouve != inventory.backpack.end()) {
        trouve->quantity += quantity;
        return;
    }
    inventory.backpack.push_back({.itemId = itemId, .quantity = quantity});
}

int removeFromBackpack(Inventory& inventory, const std::string& itemId, int quantity) {
    if (quantity <= 0) {
        return 0;
    }
    const auto trouve = std::ranges::find(inventory.backpack, itemId, &InventoryStack::itemId);
    if (trouve == inventory.backpack.end()) {
        return 0;
    }
    const int retire = std::min(quantity, trouve->quantity);
    trouve->quantity -= retire;
    if (trouve->quantity <= 0) {
        inventory.backpack.erase(trouve);
    }
    return retire;
}

int ItemLookup::weightGramsOf(std::string_view id) const {
    if (id.empty()) {
        return 0;
    }
    if (items != nullptr) {
        if (const Item* const objet = items->find(id); objet != nullptr) {
            return objet->weightGrams;
        }
    }
    if (equipment != nullptr) {
        if (const Weapon* const arme = equipment->findWeapon(id); arme != nullptr) {
            return arme->weightGrams;
        }
        if (const Armor* const armure = equipment->findArmor(id); armure != nullptr) {
            return armure->weightGrams;
        }
    }
    return 0;
}

namespace {

// Vrai si @p id est connu de l'un des catalogues.
[[nodiscard]] bool connu(const ItemLookup& lookup, const std::string& id) {
    if (id.empty()) {
        return true;
    }
    if (lookup.items != nullptr && lookup.items->find(id) != nullptr) {
        return true;
    }
    if (lookup.equipment != nullptr) {
        return lookup.equipment->findWeapon(id) != nullptr ||
               lookup.equipment->findArmor(id) != nullptr;
    }
    return false;
}

}  // namespace

int carriedWeightGrams(const Inventory& inventory, const ItemLookup& lookup) {
    int total = 0;
    for (const std::string& porte : inventory.equipped) {
        total += lookup.weightGramsOf(porte);
    }
    for (const InventoryStack& ligne : inventory.backpack) {
        if (ligne.quantity > 0) {
            total += lookup.weightGramsOf(ligne.itemId) * ligne.quantity;
        }
    }
    return total;
}

std::vector<std::string> unknownIds(const Inventory& inventory, const ItemLookup& lookup) {
    std::vector<std::string> inconnus;
    for (const std::string& porte : inventory.equipped) {
        if (!connu(lookup, porte)) {
            inconnus.push_back(porte);
        }
    }
    for (const InventoryStack& ligne : inventory.backpack) {
        if (!connu(lookup, ligne.itemId)) {
            inconnus.push_back(ligne.itemId);
        }
    }
    std::ranges::sort(inconnus);
    const auto reste = std::ranges::unique(inconnus);
    inconnus.erase(reste.begin(), reste.end());
    return inconnus;
}

Ability weaponAttackAbility(const CharacterSheet& sheet, const Weapon& weapon) {
    // Une arme de FINESSE laisse le choix -- mais aucune arme du catalogue ne declare encore cette
    // propriete autrement qu'en toutes lettres dans son texte francais, et lire une regle dans de
    // la prose est exactement ce que ce projet evite. La branche existe donc, et elle attend que la
    // donnee porte `properties` (LOT-49).
    const bool finesse =
        std::ranges::any_of(weapon.properties, [](const std::string& p) { return p == "finesse"; });
    if (finesse) {
        return sheet.modifier(Ability::Dexterity) > sheet.modifier(Ability::Strength)
                   ? Ability::Dexterity
                   : Ability::Strength;
    }
    return weapon.ranged ? Ability::Dexterity : Ability::Strength;
}

DerivedStats derivedStatsFor(const CharacterSheet& sheet, const Inventory& inventory,
                             const ItemLookup& lookup, const CharacterCreationRules& creation,
                             const EncumbranceRules& carrying) {
    DerivedStats derivees;

    // --- Classe d'armure : RECALCULEE depuis ce qui est porte ---
    //
    // C'est le coeur du lot. Aucun bonus n'est accumule : on relit l'armure et le bouclier portes,
    // et on redemande leur somme a `armorClassFor` (LOT-34). Equiper puis retirer, dans n'importe
    // quel ordre, redonne donc la valeur de depart -- il n'y a rien a annuler, puisqu'il n'y a rien
    // eu a appliquer.
    const Armor* armure = nullptr;
    const Armor* bouclier = nullptr;
    if (lookup.equipment != nullptr) {
        armure = lookup.equipment->findArmor(inventory.at(EquipmentSlot::Torso));
        bouclier = lookup.equipment->findArmor(inventory.at(EquipmentSlot::OffHand));
        // Un bouclier range au torse resterait un bouclier : c'est sa CATEGORIE qui dit comment il
        // compte, pas l'emplacement ou on l'a mis.
        if (armure != nullptr && armure->category == ArmorCategory::Shield) {
            std::swap(armure, bouclier);
        }
        if (bouclier != nullptr && bouclier->category != ArmorCategory::Shield) {
            bouclier = nullptr;
        }
    }
    derivees.armorClass = armorClassFor(sheet, creation, armure, bouclier);

    // --- L'arme en main directrice ---
    if (lookup.equipment != nullptr) {
        const Weapon* const arme =
            lookup.equipment->findWeapon(inventory.at(EquipmentSlot::MainHand));
        if (arme != nullptr) {
            derivees.damage = arme->damage;
            derivees.damageType = arme->damageType;
            derivees.attackAbility = weaponAttackAbility(sheet, *arme);
        }
    }

    // --- La charge ---
    derivees.carriedWeightGrams = carriedWeightGrams(inventory, lookup);
    derivees.speedMeters = sheet.speedMeters;
    if (carrying.isLoaded()) {
        const int force = sheet.ability(Ability::Strength);
        derivees.carryingCapacityGrams = carrying.carryingCapacityGramsPerStrength * force;
        const int encombre = carrying.encumberedGramsPerStrength * force;
        const int lourdement = carrying.heavilyEncumberedGramsPerStrength * force;
        if (derivees.carriedWeightGrams > derivees.carryingCapacityGrams) {
            derivees.encumbrance = EncumbranceLevel::OverCapacity;
            // Au-dela de la capacite, le personnage ne porte plus : la vitesse ne se contente pas
            // d'etre diminuee, elle tombe a zero. C'est le comportement DEFINI au depassement que
            // le lot demandait -- et il est defini ici plutot que laisse a chaque appelant.
            derivees.speedMeters = 0.0F;
        } else if (derivees.carriedWeightGrams > lourdement) {
            derivees.encumbrance = EncumbranceLevel::HeavilyEncumbered;
            derivees.speedMeters -= carrying.heavilyEncumberedSpeedPenaltyMeters;
        } else if (derivees.carriedWeightGrams > encombre) {
            derivees.encumbrance = EncumbranceLevel::Encumbered;
            derivees.speedMeters -= carrying.encumberedSpeedPenaltyMeters;
        }
    }
    derivees.speedMeters = std::max(0.0F, derivees.speedMeters);
    return derivees;
}

}  // namespace core
