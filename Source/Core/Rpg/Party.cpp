// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Rpg/Party.h"

#include <algorithm>
#include <system_error>
#include <utility>

#include "Core/Data/JsonDocument.h"

namespace core {

namespace {

// Les fiches de personnage ne portent pas de numero de version (`core::loadCharacterSheet`).
constexpr int SANS_GARDE_DE_VERSION = 0;

}  // namespace

Party::Party(const std::vector<std::string>& members) {
    for (const std::string& member : members) {
        static_cast<void>(add(member));
    }
}

bool Party::contains(std::string_view characterId) const noexcept {
    return std::ranges::find(_members, characterId) != _members.end();
}

PartyChange Party::add(std::string characterId) {
    if (contains(characterId)) {
        return PartyChange::AlreadyMember;
    }
    if (_members.size() >= MAX_MEMBERS) {
        return PartyChange::Full;
    }
    _members.push_back(std::move(characterId));
    return PartyChange::Done;
}

PartyChange Party::remove(std::string_view characterId) {
    const auto found = std::ranges::find(_members, characterId);
    if (found == _members.end()) {
        return PartyChange::NotMember;
    }
    if (_members.size() == 1) {
        return PartyChange::LastMember;
    }
    _members.erase(found);
    return PartyChange::Done;
}

PartyChange Party::setLeader(std::string_view characterId) {
    const auto found = std::ranges::find(_members, characterId);
    if (found == _members.end()) {
        return PartyChange::NotMember;
    }
    // Une rotation du debut jusqu'a lui : il passe en tete, et ceux qui le precedaient gardent
    // leur ordre derriere lui.
    std::rotate(_members.begin(), found, std::next(found));
    return PartyChange::Done;
}

PartyChange Party::rotateLeader() {
    if (_members.empty()) {
        return PartyChange::NotMember;
    }
    std::rotate(_members.begin(), std::next(_members.begin()), _members.end());
    return PartyChange::Done;
}

PartyChange Party::swap(std::size_t first, std::size_t second) {
    if (first >= _members.size() || second >= _members.size()) {
        return PartyChange::NotMember;
    }
    std::swap(_members[first], _members[second]);
    return PartyChange::Done;
}

PartyCandidates loadPartyCandidates(const std::filesystem::path& directory) {
    PartyCandidates result;
    std::error_code error;
    if (!std::filesystem::is_directory(directory, error)) {
        result.errors.push_back(directory.string() + " : dossier des personnages absent.");
        return result;
    }
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory, error)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            files.push_back(entry.path());
        }
    }
    std::ranges::sort(files);
    for (const std::filesystem::path& file : files) {
        const JsonDocument document = readJsonObjectFromFile(file, SANS_GARDE_DE_VERSION);
        if (!document.ok()) {
            result.errors.push_back(document.message);
            continue;
        }
        const auto text = [&document](const char* field) {
            const auto found = document.root.find(field);
            return (found != document.root.end() && found->is_string()) ? found->get<std::string>()
                                                                        : std::string{};
        };
        PartyCandidate candidate{
            .id = text("id"), .name = text("name"), .classId = text("classId"), .file = file};
        if (candidate.id.empty()) {
            result.errors.push_back(file.string() + " : fiche sans identifiant.");
            continue;
        }
        result.candidates.push_back(std::move(candidate));
    }
    std::ranges::sort(result.candidates, {}, &PartyCandidate::id);
    return result;
}

Party defaultParty(const std::vector<PartyCandidate>& candidates,
                   const std::vector<std::string>& order) {
    Party party;
    for (const std::string& id : order) {
        if (std::ranges::find(candidates, id, &PartyCandidate::id) != candidates.end()) {
            static_cast<void>(party.add(id));
        }
    }
    for (const PartyCandidate& candidate : candidates) {
        if (party.size() >= Party::MAX_MEMBERS) {
            break;
        }
        if (party.add(candidate.id) == PartyChange::Full) {
            break;
        }
    }
    return party;
}

}  // namespace core
