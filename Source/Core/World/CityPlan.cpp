// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/World/CityPlan.h"

#include <algorithm>
#include <utility>

#include "Core/Data/JsonDocument.h"

namespace core {

namespace {

// Les donnees de monde ne portent pas de champ `version` : meme convention que les arenes et les
// rencontres.
constexpr int SANS_GARDE_DE_VERSION = 0;

[[nodiscard]] std::string lireTexte(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_string()) ? trouve->get<std::string>()
                                                          : std::string{};
}

}  // namespace

const CityDistrict* CityPlan::find(std::string_view districtId) const {
    const auto trouve = std::ranges::find(districts, districtId, &CityDistrict::id);
    return trouve != districts.end() ? &*trouve : nullptr;
}

const CityDistrict* CityPlan::districtOfMap(std::string_view mapId) const {
    if (mapId.empty()) {
        return nullptr;
    }
    // La carte du quartier, ou une carte rangee sous la sienne : une sous-zone (D-16).
    const auto trouve = std::ranges::find_if(districts, [mapId](const CityDistrict& quartier) {
        return !quartier.map.empty() &&
               (mapId == quartier.map ||
                (mapId.size() > quartier.map.size() && mapId.starts_with(quartier.map) &&
                 mapId[quartier.map.size()] == '/'));
    });
    return trouve != districts.end() ? &*trouve : nullptr;
}

std::string CityPlan::startMap() const {
    const CityDistrict* const depart = find(startDistrict);
    return depart != nullptr ? depart->map : std::string{};
}

CityPlanResult loadCityPlan(const std::filesystem::path& file) {
    CityPlanResult resultat;
    const JsonDocument document = readJsonObjectFromFile(file, SANS_GARDE_DE_VERSION);
    if (!document.ok()) {
        resultat.error = document.message;
        return resultat;
    }
    const nlohmann::json& racine = document.root;
    CityPlan& plan = resultat.plan;
    plan.id = lireTexte(racine, "id");
    plan.name = lireTexte(racine, "name");
    plan.location = lireTexte(racine, "location");
    if (const auto depart = racine.find("start"); depart != racine.end() && depart->is_object()) {
        plan.startDistrict = lireTexte(*depart, "district");
        plan.startArrival = lireTexte(*depart, "arrival");
    }

    const auto quartiers = racine.find("districts");
    if (quartiers == racine.end() || !quartiers->is_array() || quartiers->empty()) {
        resultat.error = file.string() + " : une ville sans quartier.";
        return resultat;
    }
    for (const nlohmann::json& entree : *quartiers) {
        if (!entree.is_object()) {
            resultat.error = file.string() + " : un quartier n'est pas un objet.";
            return resultat;
        }
        CityDistrict quartier{.id = lireTexte(entree, "id"), .map = lireTexte(entree, "map"),
                              .guardMap = {}};
        if (const auto garde = entree.find("guard"); garde != entree.end() && garde->is_object()) {
            quartier.guardMap = lireTexte(*garde, "map");
        }
        // Un quartier SOIT se parcourt, SOIT est ferme : les deux, ou aucun, sont une saisie
        // fautive, pas un cas a interpreter.
        if (quartier.id.empty() || quartier.map.empty() == quartier.guardMap.empty()) {
            resultat.error = file.string() + " : le quartier « " + quartier.id +
                             " » doit avoir soit une carte, soit une porte gardee.";
            return resultat;
        }
        plan.districts.push_back(std::move(quartier));
    }

    if (plan.startMap().empty() || plan.startArrival.empty()) {
        resultat.error = file.string() + " : le depart « " + plan.startDistrict +
                         " » n'est pas un quartier qui a sa carte.";
    }
    return resultat;
}

}  // namespace core
