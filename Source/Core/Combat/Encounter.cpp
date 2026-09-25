// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/Encounter.h"

#include <algorithm>
#include <system_error>
#include <utility>

#include "Core/Data/JsonDocument.h"

namespace core {
namespace {

// Les entrees de catalogue ne portent pas de champ `version` : ce sont des donnees, pas des
// documents de format. Meme convention que `loadEquipment` (LOT-34) et `loadItems` (LOT-14).
constexpr int SANS_GARDE_DE_VERSION = 0;

[[nodiscard]] std::string lireTexte(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_string()) ? trouve->get<std::string>()
                                                          : std::string{};
}

[[nodiscard]] int lireEntier(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_number_integer()) ? trouve->get<int>() : 0;
}

// Les combattants d'une rencontre ; une entree sans creature est signalee puis ignoree.
void lireCombattants(const nlohmann::json& combattants, const std::filesystem::path& chemin,
                     Encounter& rencontre, std::vector<std::string>& erreurs) {
    for (const nlohmann::json& entree : combattants) {
        if (!entree.is_object()) {
            continue;
        }
        EncounterCombatant combattant;
        combattant.creatureId = lireTexte(entree, "creatureId");
        combattant.columnOffset = lireEntier(entree, "columnOffset");
        combattant.rowOffset = lireEntier(entree, "rowOffset");
        if (combattant.creatureId.empty()) {
            erreurs.push_back(chemin.filename().string() +
                              " : combattant sans identifiant de creature.");
            continue;
        }
        rencontre.combatants.push_back(std::move(combattant));
    }
}

}  // namespace

const Encounter* EncounterCatalog::find(std::string_view id) const {
    const auto trouve = std::ranges::find(encounters, id, &Encounter::id);
    return trouve == encounters.end() ? nullptr : &*trouve;
}

EncounterCatalog loadEncounters(const std::filesystem::path& encountersDir) {
    EncounterCatalog catalogue;
    std::error_code code;
    if (!std::filesystem::is_directory(encountersDir, code)) {
        catalogue.errors.push_back(encountersDir.string() + " : dossier absent ou illisible.");
        return catalogue;
    }
    std::vector<std::filesystem::path> fichiers;
    for (const auto& entree : std::filesystem::directory_iterator(encountersDir, code)) {
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
        Encounter rencontre;
        rencontre.id = lireTexte(racine, "id");
        rencontre.name = lireTexte(racine, "name");
        rencontre.source = lireTexte(racine, "source");
        if (const auto fuite = racine.find("escapable");
            fuite != racine.end() && fuite->is_boolean()) {
            rencontre.escapable = fuite->get<bool>();
        }
        if (rencontre.id.empty()) {
            catalogue.errors.push_back(chemin.filename().string() +
                                       " : rencontre sans identifiant.");
            continue;
        }

        const auto combattants = racine.find("combatants");
        if (combattants == racine.end() || !combattants->is_array() || combattants->empty()) {
            // Une rencontre SANS combattant se declencherait et se terminerait aussitot par une
            // victoire, en posant son drapeau : le joueur verrait un ennemi disparaitre sans
            // combat, et rien ne le signalerait.
            catalogue.errors.push_back(chemin.filename().string() +
                                       " : rencontre sans aucun combattant.");
            continue;
        }
        lireCombattants(*combattants, chemin, rencontre, catalogue.errors);
        if (rencontre.combatants.empty()) {
            continue;  // deja signale ci-dessus
        }
        catalogue.encounters.push_back(std::move(rencontre));
    }
    return catalogue;
}

std::vector<CombatantPlacement> placeCombatants(const Encounter& encounter, GridPosition trigger) {
    std::vector<CombatantPlacement> places;
    places.reserve(encounter.combatants.size());
    for (const EncounterCombatant& combattant : encounter.combatants) {
        places.push_back({.creatureId = combattant.creatureId,
                          .position = {.column = trigger.column + combattant.columnOffset,
                                       .row = trigger.row + combattant.rowOffset}});
    }
    return places;
}

}  // namespace core
