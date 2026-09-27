// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Rpg/Spell.h"

#include <algorithm>
#include <system_error>
#include <utility>

#include "Core/Data/JsonDocument.h"
#include "Core/Rpg/RpgEnumNames.h"

namespace core {

namespace {

constexpr int SANS_GARDE_DE_VERSION = 0;

[[nodiscard]] std::string lireTexte(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_string()) ? trouve->get<std::string>()
                                                          : std::string{};
}

[[nodiscard]] bool lireBooleen(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return trouve != objet.end() && trouve->is_boolean() && trouve->get<bool>();
}

[[nodiscard]] std::optional<Spell> lireSort(const nlohmann::json& racine,
                                            const std::string& fichier,
                                            std::vector<std::string>& erreurs) {
    Spell sort;
    sort.id = lireTexte(racine, "id");
    sort.name = lireTexte(racine, "name");
    sort.source = lireTexte(racine, "source");
    sort.school = lireTexte(racine, "school");
    sort.castingTime = lireTexte(racine, "castingTime");
    sort.range = lireTexte(racine, "range");
    sort.duration = lireTexte(racine, "duration");
    sort.text = lireTexte(racine, "text");
    sort.appliesCondition = lireTexte(racine, "appliesCondition");
    sort.concentration = lireBooleen(racine, "concentration");
    sort.ritual = lireBooleen(racine, "ritual");
    sort.attackRoll = lireBooleen(racine, "attackRoll");
    sort.narrative = lireBooleen(racine, "narratif");
    if (sort.id.empty() || sort.name.empty()) {
        erreurs.push_back(fichier + " : sort sans 'id' ou sans 'name'.");
        return std::nullopt;
    }
    const auto niveau = racine.find("level");
    if (niveau == racine.end() || !niveau->is_number_integer()) {
        erreurs.push_back(fichier + " : champ 'level' absent ou non entier.");
        return std::nullopt;
    }
    sort.level = niveau->get<int>();
    if (const auto portee = racine.find("rangeMeters");
        portee != racine.end() && portee->is_number()) {
        sort.rangeMeters = portee->get<float>();
    }
    if (const auto des = racine.find("damage"); des != racine.end() && des->is_string()) {
        sort.damage = parseDice(des->get<std::string>());
        if (!sort.damage.has_value()) {
            erreurs.push_back(fichier + " : des de degats '" + des->get<std::string>() +
                              "' illisibles.");
            return std::nullopt;
        }
    }
    if (const auto type = racine.find("damageType"); type != racine.end() && type->is_string()) {
        sort.damageType = parseDamageType(type->get<std::string>());
        if (!sort.damageType.has_value()) {
            // Un type inconnu ne recoit pas un type par defaut (EX-CBT-032).
            erreurs.push_back(fichier + " : type de degats '" + type->get<std::string>() +
                              "' inconnu du moteur.");
            return std::nullopt;
        }
    }
    if (const auto sauvegarde = racine.find("savingThrow");
        sauvegarde != racine.end() && sauvegarde->is_string()) {
        sort.savingThrow = parseAbility(sauvegarde->get<std::string>());
        if (!sort.savingThrow.has_value()) {
            erreurs.push_back(fichier + " : jet de sauvegarde '" + sauvegarde->get<std::string>() +
                              "' inconnu du moteur.");
            return std::nullopt;
        }
    }
    if (const auto mecanismes = racine.find("mecanismesRequis");
        mecanismes != racine.end() && mecanismes->is_array()) {
        for (const auto& element : *mecanismes) {
            if (element.is_string()) {
                sort.requiredMechanisms.push_back(element.get<std::string>());
            }
        }
    }
    return sort;
}

}  // namespace

const Spell* SpellCatalog::find(std::string_view id) const {
    const auto trouve = std::ranges::find(spells, id, &Spell::id);
    return trouve == spells.end() ? nullptr : &*trouve;
}

SpellCatalog loadSpells(const std::filesystem::path& spellsDir) {
    SpellCatalog catalogue;
    std::error_code code;
    if (!std::filesystem::is_directory(spellsDir, code)) {
        catalogue.errors.push_back(spellsDir.string() + " : dossier absent ou illisible.");
        return catalogue;
    }
    std::vector<std::filesystem::path> fichiers;
    for (const auto& entree : std::filesystem::directory_iterator(spellsDir, code)) {
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
        if (std::optional<Spell> sort =
                lireSort(document.root, chemin.filename().string(), catalogue.errors)) {
            catalogue.spells.push_back(std::move(*sort));
        }
    }
    std::ranges::sort(catalogue.spells, {}, &Spell::id);
    return catalogue;
}

bool isAttackSpell(const Spell& spell) noexcept {
    return spell.attackRoll && spell.damage.has_value() && spell.damageType.has_value();
}

}  // namespace core
