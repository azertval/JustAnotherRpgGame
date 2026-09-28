// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Rpg/PartyLedger.h"

#include <algorithm>
#include <utility>

namespace core {

const MemberRecord* PartyLedger::record(std::string_view characterId) const {
    const auto trouve = _records.find(characterId);
    return trouve == _records.end() ? nullptr : &trouve->second;
}

void PartyLedger::write(std::string characterId, MemberRecord record) {
    _records.insert_or_assign(std::move(characterId), std::move(record));
}

void PartyLedger::erase(std::string_view characterId) {
    const auto trouve = _records.find(characterId);
    if (trouve != _records.end()) {
        _records.erase(trouve);
    }
}

void applyRecord(CharacterSheet& sheet, const MemberRecord& record) {
    if (record.hitPoints.has_value()) {
        sheet.currentHitPoints = std::clamp(*record.hitPoints, 0, sheet.maximumHitPoints);
    }
    for (KnownSpell& spell : sheet.knownSpells) {
        const auto restant = record.spellUses.find(spell.spellId);
        if (restant != record.spellUses.end() && spell.perDay > 0) {
            spell.remaining = std::clamp(restant->second, 0, spell.perDay);
        }
    }
}

}  // namespace core
