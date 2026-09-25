// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Presentation/InventoryScreen.h"

#include <algorithm>
#include <utility>

#include "Core/Rpg/Dice.h"
#include "Core/Rpg/Equipment.h"

namespace hmi {
namespace {

[[nodiscard]] const core::Item* findItem(const core::ItemLookup& lookup, std::string_view id) {
    return lookup.items != nullptr ? lookup.items->find(id) : nullptr;
}

[[nodiscard]] const core::Weapon* findWeapon(const core::ItemLookup& lookup, std::string_view id) {
    return lookup.equipment != nullptr ? lookup.equipment->findWeapon(id) : nullptr;
}

[[nodiscard]] const core::Armor* findArmor(const core::ItemLookup& lookup, std::string_view id) {
    return lookup.equipment != nullptr ? lookup.equipment->findArmor(id) : nullptr;
}

// Rend : Le nom du catalogue, ou l'identifiant : un objet inconnu doit se VOIR, pas disparaître.
[[nodiscard]] std::string nameOf(const core::ItemLookup& lookup, std::string_view id) {
    if (const core::Weapon* const weapon = findWeapon(lookup, id); weapon != nullptr) {
        return weapon->name;
    }
    if (const core::Armor* const armor = findArmor(lookup, id); armor != nullptr) {
        return armor->name;
    }
    if (const core::Item* const item = findItem(lookup, id); item != nullptr) {
        return item->name;
    }
    return std::string(id);
}

// Rend : Un poids en grammes, en kilogrammes à une décimale (« 1,5 kg ») -- comme l'inventaire
// v1.
[[nodiscard]] std::string kilograms(int grams) {
    const int tenths = (std::max(grams, 0) + 50) / 100;
    return std::to_string(tenths / 10) + "," + std::to_string(tenths % 10) + " kg";
}

[[nodiscard]] std::string armorKind(core::ArmorCategory category) {
    switch (category) {
        case core::ArmorCategory::Light:
            return "Armure légère";
        case core::ArmorCategory::Medium:
            return "Armure intermédiaire";
        case core::ArmorCategory::Heavy:
            return "Armure lourde";
        case core::ArmorCategory::Shield:
            return "Bouclier";
    }
    return "Armure";
}

}  // namespace

ItemFamily familyOf(std::string_view itemId, const core::ItemLookup& lookup) {
    if (findWeapon(lookup, itemId) != nullptr || findArmor(lookup, itemId) != nullptr) {
        return ItemFamily::Equipment;
    }
    if (const core::Item* const item = findItem(lookup, itemId); item != nullptr) {
        if (item->category == "gear") {
            return ItemFamily::Gear;
        }
        if (item->category == "tool") {
            return ItemFamily::Tools;
        }
    }
    return ItemFamily::Other;
}

std::vector<InventoryCell> backpackCells(const core::Inventory& inventory,
                                         const core::ItemLookup& lookup, ItemFamily filter) {
    std::vector<InventoryCell> cells;
    for (const core::InventoryStack& stack : inventory.backpack) {
        if (stack.quantity <= 0) {
            continue;
        }
        const ItemFamily family = familyOf(stack.itemId, lookup);
        if (filter != ItemFamily::All && family != filter) {
            continue;
        }
        cells.push_back(InventoryCell{.itemId = stack.itemId,
                                      .name = nameOf(lookup, stack.itemId),
                                      .quantity = stack.quantity,
                                      .family = family});
    }
    return cells;
}

ItemSheet itemSheet(std::string_view itemId, const core::ItemLookup& lookup) {
    ItemSheet sheet;
    sheet.name = nameOf(lookup, itemId);
    sheet.equippable = naturalSlot(itemId, lookup).has_value();
    if (const core::Weapon* const weapon = findWeapon(lookup, itemId); weapon != nullptr) {
        sheet.kind =
            std::string(weapon->category == "martial" ? "Arme de guerre" : "Arme courante") +
            (weapon->ranged ? " à distance" : "");
        if (weapon->damage) {
            sheet.damage = core::formatDice(*weapon->damage);
        }
        sheet.weight = kilograms(weapon->weightGrams);
        sheet.text = weapon->text;
        return sheet;
    }
    if (const core::Armor* const armor = findArmor(lookup, itemId); armor != nullptr) {
        sheet.kind = armorKind(armor->category);
        if (armor->category == core::ArmorCategory::Shield) {
            sheet.armor = "+" + std::to_string(armor->baseArmorClass);
        } else {
            sheet.armor = "CA " + std::to_string(armor->baseArmorClass);
            if (armor->dexterityBonus) {
                sheet.armor +=
                    armor->dexterityBonusMax
                        ? " + Dex (max " + std::to_string(*armor->dexterityBonusMax) + ")"
                        : " + Dex";
            }
        }
        sheet.weight = kilograms(armor->weightGrams);
        return sheet;
    }
    if (const core::Item* const item = findItem(lookup, itemId); item != nullptr) {
        sheet.kind = "Matériel";
        if (item->category == "tool") {
            sheet.kind = "Outil";
        } else if (item->category == "mount") {
            sheet.kind = "Monture";
        }
        sheet.weight = kilograms(item->weightGrams);
        sheet.text = item->text;
    }
    return sheet;
}

std::optional<core::EquipmentSlot> naturalSlot(std::string_view itemId,
                                               const core::ItemLookup& lookup) {
    if (const core::Weapon* const weapon = findWeapon(lookup, itemId); weapon != nullptr) {
        return weapon->ranged ? core::EquipmentSlot::Ranged : core::EquipmentSlot::MainHand;
    }
    if (const core::Armor* const armor = findArmor(lookup, itemId); armor != nullptr) {
        return armor->category == core::ArmorCategory::Shield ? core::EquipmentSlot::OffHand
                                                              : core::EquipmentSlot::Torso;
    }
    return std::nullopt;
}

bool equipFromBackpack(core::Inventory& inventory, std::string_view itemId,
                       const core::ItemLookup& lookup) {
    const std::optional<core::EquipmentSlot> slot = naturalSlot(itemId, lookup);
    if (!slot) {
        return false;
    }
    const std::string id(itemId);
    if (core::removeFromBackpack(inventory, id, 1) != 1) {
        return false;
    }
    std::string previous = core::equip(inventory, *slot, id);
    if (!previous.empty()) {
        core::addToBackpack(inventory, previous, 1);
    }
    return true;
}

bool unequipToBackpack(core::Inventory& inventory, core::EquipmentSlot slot) {
    std::string removed = core::unequip(inventory, slot);
    if (removed.empty()) {
        return false;
    }
    core::addToBackpack(inventory, removed, 1);
    return true;
}

bool dropFromBackpack(core::Inventory& inventory, std::string_view itemId) {
    return core::removeFromBackpack(inventory, std::string(itemId), 1) == 1;
}

void sortBackpack(core::Inventory& inventory, const core::ItemLookup& lookup) {
    std::ranges::stable_sort(inventory.backpack, [&lookup](const core::InventoryStack& left,
                                                           const core::InventoryStack& right) {
        const std::string leftName = nameOf(lookup, left.itemId);
        const std::string rightName = nameOf(lookup, right.itemId);
        return leftName != rightName ? leftName < rightName : left.itemId < right.itemId;
    });
}

}  // namespace hmi
