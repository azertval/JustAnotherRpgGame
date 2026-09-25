// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/InventoryModel.h"

#include <algorithm>
#include <array>
#include <optional>
#include <utility>

#include "Core/Rpg/Inventory.h"
#include "HMI/Presentation/InventoryScreen.h"
#include "HMI/Presentation/InventoryValues.h"
#include "HMI/Runtime/DemonstrationCharacter.h"

namespace hmi {
namespace {

constexpr const char* EMPTY_MARK = "—";

// Les seize emplacements, dans l'ordre où une feuille de personnage les grave : de la tête aux
// pieds, puis ce qui se porte à part.
constexpr std::array<core::EquipmentSlot, core::EQUIPMENT_SLOT_COUNT> SLOTS{
    core::EquipmentSlot::Head,       core::EquipmentSlot::Neck,     core::EquipmentSlot::Cloak,
    core::EquipmentSlot::Torso,      core::EquipmentSlot::Belt,     core::EquipmentSlot::Hands,
    core::EquipmentSlot::Bracers,    core::EquipmentSlot::RingLeft, core::EquipmentSlot::RingRight,
    core::EquipmentSlot::MainHand,   core::EquipmentSlot::OffHand,  core::EquipmentSlot::Ranged,
    core::EquipmentSlot::Ammunition, core::EquipmentSlot::Feet,     core::EquipmentSlot::Pouch,
    core::EquipmentSlot::Trinket,
};

// Les filtres des onglets, dans l'ordre de `InventoryModel::filter`.
constexpr std::array<ItemFamily, 4> FILTERS{ItemFamily::All, ItemFamily::Equipment,
                                            ItemFamily::Gear, ItemFamily::Tools};

[[nodiscard]] QString toQt(const std::string& text) {
    return QString::fromStdString(text);
}

}  // namespace

InventoryModel::InventoryModel(QObject* parent) : QObject(parent) {}

InventoryModel::~InventoryModel() = default;

QString InventoryModel::value(const char* key) const {
    const auto found = _values.find(key);
    if (found == _values.end() || found->second.empty()) {
        return QString::fromUtf8(EMPTY_MARK);
    }
    return toQt(found->second);
}

QString InventoryModel::sheetValue(const char* key) const {
    const auto found = _sheetValues.find(key);
    if (found == _sheetValues.end() || found->second.empty()) {
        return QString::fromUtf8(EMPTY_MARK);
    }
    return toQt(found->second);
}

void InventoryModel::loadDemonstrationCharacter() {
    _state = std::make_unique<DemonstrationState>(loadDemonstrationState());
    _selectedItem.clear();
    _selectedSlot.clear();
    refresh();
}

void InventoryModel::refresh() {
    if (_state == nullptr) {
        emit changed();
        return;
    }
    const DemonstrationCharacter values = demonstrationValues(*_state);
    _values = values.inventory;
    _sheetValues = values.sheet;

    const core::DerivedStats derived = core::derivedStatsFor(
        _state->sheet, _state->inventory, _state->lookup(), _state->rules, _state->encumbrance);
    _loadRatio = derived.carryingCapacityGrams > 0
                     ? std::clamp(static_cast<qreal>(derived.carriedWeightGrams) /
                                      static_cast<qreal>(derived.carryingCapacityGrams),
                                  0.0, 1.0)
                     : 0.0;

    // Une sélection qui ne désigne plus rien (objet jeté jusqu'au dernier, emplacement vidé) tombe.
    if (!_selectedItem.isEmpty()) {
        const std::string id = _selectedItem.toStdString();
        const bool present =
            std::any_of(_state->inventory.backpack.begin(), _state->inventory.backpack.end(),
                        [&id](const core::InventoryStack& stack) {
                            return stack.itemId == id && stack.quantity > 0;
                        });
        if (!present) {
            _selectedItem.clear();
        }
    }
    if (!_selectedSlot.isEmpty()) {
        const std::optional<core::EquipmentSlot> slot =
            core::parseEquipmentSlot(_selectedSlot.toStdString());
        if (!slot || !_state->inventory.isEquipped(*slot)) {
            _selectedSlot.clear();
        }
    }

    emit changed();
}

QVariantMap InventoryModel::equipped() const {
    QVariantMap table;
    if (_state == nullptr) {
        return table;
    }
    const core::ItemLookup lookup = _state->lookup();
    for (const core::EquipmentSlot slot : SLOTS) {
        const std::string& itemId = _state->inventory.at(slot);
        table.insert(
            toQt(std::string(core::equipmentSlotName(slot))),
            QVariantMap{{QStringLiteral("itemId"), toQt(itemId)},
                        {QStringLiteral("name"),
                         itemId.empty() ? QString() : toQt(itemSheet(itemId, lookup).name)}});
    }
    return table;
}

void InventoryModel::setFilter(int filter) {
    const int bounded = std::clamp(filter, 0, static_cast<int>(FILTERS.size()) - 1);
    if (_filter == bounded) {
        return;
    }
    _filter = bounded;
    emit changed();
}

QVariantList InventoryModel::cells() const {
    QVariantList list;
    if (_state == nullptr) {
        return list;
    }
    for (const InventoryCell& cell : backpackCells(_state->inventory, _state->lookup(),
                                                   FILTERS[static_cast<std::size_t>(_filter)])) {
        list.append(QVariantMap{{QStringLiteral("itemId"), toQt(cell.itemId)},
                                {QStringLiteral("name"), toQt(cell.name)},
                                {QStringLiteral("quantity"), cell.quantity}});
    }
    return list;
}

QVariantMap InventoryModel::selection() const {
    if (_state == nullptr) {
        return {};
    }
    const core::ItemLookup lookup = _state->lookup();
    std::string itemId;
    const bool fromSlot = !_selectedSlot.isEmpty();
    if (fromSlot) {
        if (const std::optional<core::EquipmentSlot> slot =
                core::parseEquipmentSlot(_selectedSlot.toStdString())) {
            itemId = _state->inventory.at(*slot);
        }
    } else {
        itemId = _selectedItem.toStdString();
    }
    if (itemId.empty()) {
        return {};
    }
    const ItemSheet sheet = itemSheet(itemId, lookup);
    return QVariantMap{{QStringLiteral("itemId"), toQt(itemId)},
                       {QStringLiteral("name"), toQt(sheet.name)},
                       {QStringLiteral("kind"), toQt(sheet.kind)},
                       {QStringLiteral("damage"), toQt(sheet.damage)},
                       {QStringLiteral("armor"), toQt(sheet.armor)},
                       {QStringLiteral("weight"), toQt(sheet.weight)},
                       {QStringLiteral("text"), toQt(sheet.text)},
                       {QStringLiteral("canEquip"), !fromSlot && sheet.equippable},
                       {QStringLiteral("canUnequip"), fromSlot},
                       {QStringLiteral("canDrop"), !fromSlot}};
}

QString InventoryModel::gold() const {
    if (_state == nullptr) {
        return QString::fromUtf8(EMPTY_MARK);
    }
    int gold = 0;
    int silver = 0;
    int copper = 0;
    splitPurse(_state->inventory.purseCopper, gold, silver, copper);
    return QString::number(gold);
}

void InventoryModel::selectItem(const QString& itemId) {
    _selectedItem = itemId;
    _selectedSlot.clear();
    emit changed();
}

void InventoryModel::selectSlot(const QString& slot) {
    _selectedSlot = slot;
    _selectedItem.clear();
    refresh();
}

void InventoryModel::equipSelected() {
    if (_state == nullptr || _selectedItem.isEmpty()) {
        return;
    }
    if (equipFromBackpack(_state->inventory, _selectedItem.toStdString(), _state->lookup())) {
        // L'objet est désormais porté : la sélection le suit à son emplacement.
        const std::optional<core::EquipmentSlot> slot =
            naturalSlot(_selectedItem.toStdString(), _state->lookup());
        _selectedItem.clear();
        _selectedSlot = slot ? toQt(std::string(core::equipmentSlotName(*slot))) : QString();
        refresh();
    }
}

void InventoryModel::unequipSelected() {
    if (_state == nullptr || _selectedSlot.isEmpty()) {
        return;
    }
    const std::optional<core::EquipmentSlot> slot =
        core::parseEquipmentSlot(_selectedSlot.toStdString());
    if (!slot) {
        return;
    }
    const std::string removed = _state->inventory.at(*slot);
    if (unequipToBackpack(_state->inventory, *slot)) {
        _selectedSlot.clear();
        _selectedItem = toQt(removed);
        refresh();
    }
}

void InventoryModel::dropSelected() {
    if (_state == nullptr || _selectedItem.isEmpty()) {
        return;
    }
    if (dropFromBackpack(_state->inventory, _selectedItem.toStdString())) {
        refresh();
    }
}

void InventoryModel::sortBackpack() {
    if (_state == nullptr) {
        return;
    }
    hmi::sortBackpack(_state->inventory, _state->lookup());
    refresh();
}

}  // namespace hmi
