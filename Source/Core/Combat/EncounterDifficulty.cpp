// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/EncounterDifficulty.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "Core/Data/JsonDocument.h"

namespace core {
namespace {

constexpr int SANS_GARDE_DE_VERSION = 0;
// Les tailles de groupe qui changent le multiplicateur (Guide du Maitre, p. 83).
constexpr int PETIT_GROUPE = 3;
constexpr int GRAND_GROUPE = 6;

[[nodiscard]] double lireReel(const nlohmann::json& objet, const char* champ, double defaut) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_number()) ? trouve->get<double>() : defaut;
}

}  // namespace

EncounterDifficultyRules loadEncounterDifficultyRules(const std::filesystem::path& path) {
    EncounterDifficultyRules rules;
    const JsonDocument document = readJsonObjectFromFile(path, SANS_GARDE_DE_VERSION);
    if (!document.ok()) {
        rules.errors.push_back(document.message);
        return rules;
    }
    const nlohmann::json& racine = document.root;
    const std::string fichier = path.string();

    const auto categories = racine.find("categories");
    if (categories == racine.end() || !categories->is_array()) {
        rules.errors.push_back(fichier + " : champ 'categories' absent ou non tableau.");
    } else {
        for (const nlohmann::json& categorie : *categories) {
            if (categorie.is_string()) {
                rules.categories.push_back(categorie.get<std::string>());
            }
        }
    }

    const auto seuils = racine.find("thresholds");
    if (seuils == racine.end() || !seuils->is_array()) {
        rules.errors.push_back(fichier + " : champ 'thresholds' absent ou non tableau.");
    } else {
        for (const nlohmann::json& ligne : *seuils) {
            const auto niveau = ligne.is_object() ? ligne.find("level") : ligne.end();
            if (niveau == ligne.end() || !niveau->is_number_integer()) {
                rules.errors.push_back(fichier + " : seuil sans niveau.");
                continue;
            }
            DifficultyThresholds seuil{.level = niveau->get<int>(), .experience = {}};
            for (const std::string& categorie : rules.categories) {
                const auto valeur = ligne.find(categorie);
                if (valeur == ligne.end() || !valeur->is_number_integer()) {
                    rules.errors.push_back(fichier + " : niveau " + std::to_string(seuil.level) +
                                           " sans seuil « " + categorie + " ».");
                    continue;
                }
                seuil.experience.emplace(categorie, valeur->get<int>());
            }
            rules.thresholds.push_back(std::move(seuil));
        }
        std::ranges::sort(rules.thresholds, {}, &DifficultyThresholds::level);
    }

    const auto multiplicateurs = racine.find("multipliers");
    if (multiplicateurs == racine.end() || !multiplicateurs->is_array()) {
        rules.errors.push_back(fichier + " : champ 'multipliers' absent ou non tableau.");
    } else {
        for (const nlohmann::json& ligne : *multiplicateurs) {
            if (!ligne.is_object()) {
                continue;
            }
            rules.multipliers.push_back(EncounterMultiplier{
                .minMonsters = static_cast<int>(lireReel(ligne, "minMonsters", 1.0)),
                .multiplier = lireReel(ligne, "multiplier", 1.0)});
        }
        std::ranges::sort(rules.multipliers, {}, &EncounterMultiplier::minMonsters);
    }
    rules.smallPartyMultiplier = lireReel(racine, "smallPartyMultiplier", 1.0);
    rules.largePartyMultiplier = lireReel(racine, "largePartyMultiplier", 1.0);

    const auto px = racine.find("experienceByChallenge");
    if (px == racine.end() || !px->is_array()) {
        rules.errors.push_back(fichier + " : champ 'experienceByChallenge' absent ou non tableau.");
    } else {
        for (const nlohmann::json& ligne : *px) {
            if (!ligne.is_object()) {
                continue;
            }
            rules.experienceByChallenge.push_back(ChallengeExperience{
                .challengeRating = static_cast<float>(lireReel(ligne, "challengeRating", 0.0)),
                .experience = static_cast<int>(lireReel(ligne, "experience", 0.0))});
        }
    }
    return rules;
}

int experienceForChallenge(const EncounterDifficultyRules& rules, float challengeRating) {
    // Les indices fractionnaires (1/8, 1/4, 1/2) se comparent a une tolerance : 0.125 ecrit en
    // donnee et lu en flottant ne differe de lui-meme que par le bruit du format.
    constexpr float TOLERANCE = 0.001F;
    for (const ChallengeExperience& entree : rules.experienceByChallenge) {
        if (std::fabs(entree.challengeRating - challengeRating) < TOLERANCE) {
            return entree.experience;
        }
    }
    return 0;
}

double encounterMultiplierFor(const EncounterDifficultyRules& rules, int monsters, int partySize) {
    if (rules.multipliers.empty() || monsters <= 0) {
        return 1.0;
    }
    // La categorie du nombre de monstres : la derniere dont le plancher est atteint.
    std::size_t categorie = 0;
    for (std::size_t i = 0; i < rules.multipliers.size(); ++i) {
        if (monsters >= rules.multipliers[i].minMonsters) {
            categorie = i;
        }
    }
    // Moins de trois personnages : la categorie superieure ; six ou plus : l'inferieure.
    if (partySize > 0 && partySize < PETIT_GROUPE) {
        if (categorie + 1 >= rules.multipliers.size()) {
            return rules.smallPartyMultiplier;
        }
        return rules.multipliers[categorie + 1].multiplier;
    }
    if (partySize >= GRAND_GROUPE) {
        if (categorie == 0) {
            return rules.largePartyMultiplier;
        }
        return rules.multipliers[categorie - 1].multiplier;
    }
    return rules.multipliers[categorie].multiplier;
}

std::map<std::string, int> partyThresholds(const EncounterDifficultyRules& rules,
                                           std::span<const int> partyLevels) {
    std::map<std::string, int> total;
    if (rules.thresholds.empty()) {
        return total;
    }
    for (const int niveau : partyLevels) {
        // Le seuil du niveau, ou du plus proche : un niveau 0 ou 25 n'a pas de ligne.
        const DifficultyThresholds* proche = &rules.thresholds.front();
        for (const DifficultyThresholds& seuil : rules.thresholds) {
            if (std::abs(seuil.level - niveau) < std::abs(proche->level - niveau)) {
                proche = &seuil;
            }
        }
        for (const auto& [categorie, valeur] : proche->experience) {
            total[categorie] += valeur;
        }
    }
    return total;
}

EncounterBudget rateEncounter(const EncounterDifficultyRules& rules, const Encounter& encounter,
                              const Bestiary& bestiary, std::span<const int> partyLevels) {
    EncounterBudget budget;
    // Etape 3 : la somme des PX des monstres.
    for (const EncounterCombatant& combattant : encounter.combatants) {
        const Creature* const creature = bestiary.find(combattant.creatureId);
        if (creature == nullptr) {
            budget.unknownCreatures.push_back(combattant.creatureId);
            continue;
        }
        ++budget.monsters;
        budget.monsterExperience += experienceForChallenge(rules, creature->challengeRating);
    }
    // Etape 4 : le multiplicateur selon leur nombre, et la taille du groupe.
    budget.multiplier =
        encounterMultiplierFor(rules, budget.monsters, static_cast<int>(partyLevels.size()));
    budget.adjustedExperience = static_cast<int>(
        std::lround(static_cast<double>(budget.monsterExperience) * budget.multiplier));
    // Etapes 1 et 2 : les seuils du groupe ; etape 5 : le seuil inferieur le plus proche.
    budget.thresholds = partyThresholds(rules, partyLevels);
    for (const std::string& categorie : rules.categories) {
        const auto seuil = budget.thresholds.find(categorie);
        if (seuil != budget.thresholds.end() && budget.adjustedExperience >= seuil->second) {
            budget.category = categorie;
        }
    }
    return budget;
}

}  // namespace core
