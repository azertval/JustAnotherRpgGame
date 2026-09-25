// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Gameplay/FlagCondition.h"

#include <algorithm>

#include "Core/Gameplay/WorldFlags.h"

namespace core {

namespace {

using Json = nlohmann::json;

// `"v"` ou `["v", "w"]`, sans valeur vide ni doublon ; `std::nullopt` si la forme est autre.
[[nodiscard]] std::optional<std::vector<std::string>> valeursDepuis(const Json& brut) {
    std::vector<std::string> valeurs;
    const auto ajouter = [&valeurs](const Json& valeur) {
        if (!valeur.is_string() || valeur.get<std::string>().empty()) {
            return false;
        }
        std::string texte = valeur.get<std::string>();
        if (std::ranges::find(valeurs, texte) == valeurs.end()) {
            valeurs.push_back(std::move(texte));
        }
        return true;
    };
    if (brut.is_array()) {
        if (brut.empty()) {
            return std::nullopt;
        }
        for (const Json& valeur : brut) {
            if (!ajouter(valeur)) {
                return std::nullopt;
            }
        }
        return valeurs;
    }
    if (!ajouter(brut)) {
        return std::nullopt;
    }
    return valeurs;
}

}  // namespace

bool FlagCondition::holds(const WorldFlags& flags) const {
    switch (test) {
        case FlagTest::IsSet:
            return flags.isSet(flag);
        case FlagTest::IsUnset:
            return !flags.isSet(flag);
        case FlagTest::Equals:
        case FlagTest::NotEquals: {
            const std::optional<std::string> valeur = flags.value(flag);
            const bool parmi = valeur && std::ranges::find(values, *valeur) != values.end();
            return test == FlagTest::Equals ? parmi : !parmi;
        }
    }
    return false;
}

std::string describeFlagCondition(const FlagCondition& condition) {
    switch (condition.test) {
        case FlagTest::IsSet:
            return condition.flag;
        case FlagTest::IsUnset:
            return "!" + condition.flag;
        case FlagTest::Equals:
        case FlagTest::NotEquals: {
            std::string texte =
                condition.flag + (condition.test == FlagTest::Equals ? " == " : " != ");
            for (std::size_t i = 0; i < condition.values.size(); ++i) {
                texte += (i == 0 ? "" : "|") + condition.values[i];
            }
            return texte;
        }
    }
    return condition.flag;
}

FlagConditionRead readFlagCondition(const Json& object) {
    if (!object.is_object()) {
        return {std::nullopt, "une condition est un objet { \"flag\": ... }."};
    }
    const auto drapeau = object.find("flag");
    if (drapeau == object.end() || !drapeau->is_string() || drapeau->get<std::string>().empty()) {
        return {std::nullopt, "condition sans 'flag'."};
    }
    FlagCondition condition;
    condition.flag = drapeau->get<std::string>();

    const auto estPose = object.find("isSet");
    const auto egal = object.find("equals");
    const auto different = object.find("notEquals");
    const int formes = static_cast<int>(estPose != object.end()) +
                       static_cast<int>(egal != object.end()) +
                       static_cast<int>(different != object.end());
    if (formes > 1) {
        return {std::nullopt, "condition sur '" + condition.flag +
                                  "' : 'isSet', 'equals' et 'notEquals' s'excluent."};
    }
    if (estPose != object.end()) {
        if (!estPose->is_boolean()) {
            return {std::nullopt, "condition sur '" + condition.flag + "' : 'isSet' non booleen."};
        }
        condition.test = estPose->get<bool>() ? FlagTest::IsSet : FlagTest::IsUnset;
        return {condition, {}};
    }
    if (egal == object.end() && different == object.end()) {
        return {condition, {}};
    }
    const bool estEgal = egal != object.end();
    auto valeurs = valeursDepuis(estEgal ? *egal : *different);
    if (!valeurs) {
        return {std::nullopt, "condition sur '" + condition.flag + "' : '" +
                                  (estEgal ? "equals" : "notEquals") +
                                  "' attend une valeur ou une liste de valeurs non vides."};
    }
    condition.test = estEgal ? FlagTest::Equals : FlagTest::NotEquals;
    condition.values = std::move(*valeurs);
    return {condition, {}};
}

std::vector<std::string> splitFlagValues(std::string_view text) {
    std::vector<std::string> valeurs;
    std::size_t debut = 0;
    while (debut <= text.size()) {
        const std::size_t fin = std::min(text.find('|', debut), text.size());
        std::string_view morceau = text.substr(debut, fin - debut);
        while (!morceau.empty() && morceau.front() == ' ') {
            morceau.remove_prefix(1);
        }
        while (!morceau.empty() && morceau.back() == ' ') {
            morceau.remove_suffix(1);
        }
        if (!morceau.empty() && std::ranges::find(valeurs, morceau) == valeurs.end()) {
            valeurs.emplace_back(morceau);
        }
        debut = fin + 1;
    }
    return valeurs;
}

}  // namespace core
