// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/WorldState.h"

#include "Core/World/EntityPresence.h"

namespace hmi {

WorldStateEntry parseWorldStateEntry(std::string_view entry) {
    const std::size_t equal = entry.find('=');
    if (equal == std::string_view::npos) {
        return WorldStateEntry{.flag = std::string{entry}, .value = std::nullopt};
    }
    return WorldStateEntry{.flag = std::string{entry.substr(0, equal)},
                           .value = std::string{entry.substr(equal + 1)}};
}

std::optional<std::string> worldStateValue(const std::vector<std::string>& entries,
                                           std::string_view flag) {
    std::optional<std::string> value;
    // La derniere l'emporte, comme au jeu qui les pose dans l'ordre.
    for (const std::string& entry : entries) {
        WorldStateEntry read = parseWorldStateEntry(entry);
        if (read.flag == flag && read.value) {
            value = std::move(read.value);
        }
    }
    return value;
}

WorldStateFlags worldStateFlags(const std::vector<std::string>& entries,
                                const std::vector<core::QuestFlag>& declared) {
    WorldStateFlags state;
    for (const core::QuestFlag& flag : declared) {
        state.flags.declare(flag.id, flag.values, flag.initial);
    }
    for (const std::string& entry : entries) {
        const WorldStateEntry read = parseWorldStateEntry(entry);
        if (read.flag.empty()) {
            continue;
        }
        const bool accepted = read.value ? state.flags.setValue(read.flag, *read.value)
                                         : (state.flags.set(read.flag) ||
                                            state.flags.declaredValues(read.flag) == nullptr);
        if (!accepted) {
            state.refused.push_back(entry);
        }
    }
    return state;
}

std::vector<bool> presenceUnder(const std::vector<core::MapEntity>& entities,
                                const core::WorldFlags& flags) {
    std::vector<bool> present;
    present.reserve(entities.size());
    for (const core::MapEntity& entity : entities) {
        present.push_back(core::isEntityPresent(entity, flags));
    }
    return present;
}

}  // namespace hmi
