// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/World/EntityPresence.h"

#include <string>

#include "Core/Gameplay/WorldFlags.h"

namespace core {

namespace {

// Une propriété lue : absente, présente et textuelle (`texte`), ou présente d'un autre type.
struct Lu {
    bool present = false;
    const std::string* texte = nullptr;
};

[[nodiscard]] Lu lire(const MapEntity& entite, std::string_view cle) {
    const auto trouve = entite.properties.find(std::string(cle));
    if (trouve == entite.properties.end()) {
        return {};
    }
    return {.present = true, .texte = std::get_if<std::string>(&trouve->second)};
}

// Le test nommé par le mot (`set`, `unset`, `equals`, `notEquals`) ; mot vide : `Equals` si des
// valeurs sont données, `IsSet` sinon ; `std::nullopt` si le mot est inconnu.
[[nodiscard]] std::optional<FlagTest> testDepuis(const std::string& mot, bool sansValeurs) {
    if (mot.empty()) {
        return sansValeurs ? FlagTest::IsSet : FlagTest::Equals;
    }
    if (mot == "set") {
        return FlagTest::IsSet;
    }
    if (mot == "unset") {
        return FlagTest::IsUnset;
    }
    if (mot == "equals") {
        return FlagTest::Equals;
    }
    if (mot == "notEquals") {
        return FlagTest::NotEquals;
    }
    return std::nullopt;
}

}  // namespace

PresenceRead presenceConditionOf(const MapEntity& entity) {
    const Lu drapeau = lire(entity, PRESENCE_FLAG_PROPERTY);
    const Lu test = lire(entity, PRESENCE_TEST_PROPERTY);
    const Lu valeurs = lire(entity, PRESENCE_VALUE_PROPERTY);
    if ((drapeau.present && drapeau.texte == nullptr) || (test.present && test.texte == nullptr) ||
        (valeurs.present && valeurs.texte == nullptr)) {
        return {.condition = std::nullopt, .issue = PresenceIssue::WrongValueType};
    }
    const bool sansDrapeau = !drapeau.present || drapeau.texte->empty();
    if (sansDrapeau) {
        const bool reste =
            (test.present && !test.texte->empty()) || (valeurs.present && !valeurs.texte->empty());
        return {.condition = std::nullopt, .issue = reste ? PresenceIssue::MissingFlag : PresenceIssue::None};
    }

    FlagCondition condition;
    condition.flag = *drapeau.texte;
    if (valeurs.present) {
        condition.values = splitFlagValues(*valeurs.texte);
    }
    const std::string mot = test.present ? *test.texte : std::string();
    const std::optional<FlagTest> forme = testDepuis(mot, condition.values.empty());
    if (!forme) {
        return {.condition = std::nullopt, .issue = PresenceIssue::UnknownTest};
    }
    condition.test = *forme;
    const bool compare =
        condition.test == FlagTest::Equals || condition.test == FlagTest::NotEquals;
    if (compare && condition.values.empty()) {
        return {.condition = std::nullopt, .issue = PresenceIssue::MissingValue};
    }
    if (!compare) {
        condition.values.clear();
    }
    return {.condition = std::move(condition), .issue = PresenceIssue::None};
}

bool isEntityPresent(const MapEntity& entity, const WorldFlags& flags) {
    const PresenceRead lue = presenceConditionOf(entity);
    return !lue.condition || lue.condition->holds(flags);
}

std::vector<const MapEntity*> presentEntities(const std::vector<MapEntity>& entities,
                                              const WorldFlags& flags) {
    std::vector<const MapEntity*> presentes;
    presentes.reserve(entities.size());
    for (const MapEntity& entite : entities) {
        if (isEntityPresent(entite, flags)) {
            presentes.push_back(&entite);
        }
    }
    return presentes;
}

}  // namespace core
