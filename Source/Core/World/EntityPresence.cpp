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
    return {true, std::get_if<std::string>(&trouve->second)};
}

}  // namespace

PresenceRead presenceConditionOf(const MapEntity& entity) {
    const Lu drapeau = lire(entity, PRESENCE_FLAG_PROPERTY);
    const Lu test = lire(entity, PRESENCE_TEST_PROPERTY);
    const Lu valeurs = lire(entity, PRESENCE_VALUE_PROPERTY);
    if ((drapeau.present && drapeau.texte == nullptr) || (test.present && test.texte == nullptr) ||
        (valeurs.present && valeurs.texte == nullptr)) {
        return {std::nullopt, PresenceIssue::WrongValueType};
    }
    const bool sansDrapeau = !drapeau.present || drapeau.texte->empty();
    if (sansDrapeau) {
        const bool reste =
            (test.present && !test.texte->empty()) || (valeurs.present && !valeurs.texte->empty());
        return {std::nullopt, reste ? PresenceIssue::MissingFlag : PresenceIssue::None};
    }

    FlagCondition condition;
    condition.flag = *drapeau.texte;
    if (valeurs.present) {
        condition.values = splitFlagValues(*valeurs.texte);
    }
    const std::string mot = test.present ? *test.texte : std::string();
    if (mot.empty()) {
        condition.test = condition.values.empty() ? FlagTest::IsSet : FlagTest::Equals;
    } else if (mot == "set") {
        condition.test = FlagTest::IsSet;
    } else if (mot == "unset") {
        condition.test = FlagTest::IsUnset;
    } else if (mot == "equals") {
        condition.test = FlagTest::Equals;
    } else if (mot == "notEquals") {
        condition.test = FlagTest::NotEquals;
    } else {
        return {std::nullopt, PresenceIssue::UnknownTest};
    }
    const bool compare =
        condition.test == FlagTest::Equals || condition.test == FlagTest::NotEquals;
    if (compare && condition.values.empty()) {
        return {std::nullopt, PresenceIssue::MissingValue};
    }
    if (!compare) {
        condition.values.clear();
    }
    return {std::move(condition), PresenceIssue::None};
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
