// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Rpg/ClassCapacities.h"

#include <algorithm>
#include <array>
#include <system_error>
#include <utility>

#include "Core/Data/JsonDocument.h"
#include "Core/Rpg/RpgEnumNames.h"

namespace core {

namespace {

// Les entrees de catalogue ne portent pas de champ `version` : ce sont des donnees, pas des
// documents de format.
constexpr int SANS_GARDE_DE_VERSION = 0;

struct NomDeGenre {
    CapacityEffectKind genre;
    std::string_view nom;
};

// Les noms sont ceux de `capacity.schema.json` : le schema et le moteur doivent dire la meme liste.
constexpr std::array<NomDeGenre, 9> GENRES{{
    {CapacityEffectKind::AttackBonus, "attack-bonus"},
    {CapacityEffectKind::ArmorClassBonus, "armor-class-bonus"},
    {CapacityEffectKind::UnarmoredArmorClass, "unarmored-armor-class"},
    {CapacityEffectKind::DamageResistance, "damage-resistance"},
    {CapacityEffectKind::SpeedBonus, "speed-bonus"},
    {CapacityEffectKind::NoOpportunityAttacks, "no-opportunity-attacks"},
    {CapacityEffectKind::ExtraDamage, "extra-damage"},
    {CapacityEffectKind::ExtraAttack, "extra-attack"},
    {CapacityEffectKind::ProficientCheckBonus, "proficient-check-bonus"},
}};

[[nodiscard]] std::string lireTexte(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_string()) ? trouve->get<std::string>()
                                                          : std::string{};
}

[[nodiscard]] bool lireBooleen(const nlohmann::json& objet, const char* champ, bool defaut) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_boolean()) ? trouve->get<bool>() : defaut;
}

[[nodiscard]] std::optional<int> lireEntier(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    if (trouve == objet.end() || !trouve->is_number_integer()) {
        return std::nullopt;
    }
    return trouve->get<int>();
}

[[nodiscard]] std::vector<std::string> lireTextes(const nlohmann::json& objet, const char* champ) {
    std::vector<std::string> valeurs;
    const auto trouve = objet.find(champ);
    if (trouve == objet.end() || !trouve->is_array()) {
        return valeurs;
    }
    for (const auto& element : *trouve) {
        if (element.is_string()) {
            valeurs.push_back(element.get<std::string>());
        }
    }
    return valeurs;
}

// Un effet : son genre, puis les champs que ce genre EXIGE. Un champ manquant est une erreur,
// jamais une valeur devinee -- un `extra-damage` sans des jouerait zero de en silence.
[[nodiscard]] std::optional<CapacityEffect> lireEffet(const nlohmann::json& objet,
                                                      const std::string& fichier,
                                                      std::vector<std::string>& erreurs) {
    CapacityEffect effet;
    const std::string genre = lireTexte(objet, "kind");
    const std::optional<CapacityEffectKind> lu = parseCapacityEffectKind(genre);
    if (!lu.has_value()) {
        erreurs.push_back(fichier + " : genre d'effet '" + genre +
                          "' inconnu du moteur. Un effet ignore serait une capacite qui ne fait "
                          "rien sans le dire.");
        return std::nullopt;
    }
    effet.kind = *lu;
    switch (effet.kind) {
        case CapacityEffectKind::AttackBonus:
        case CapacityEffectKind::ArmorClassBonus:
        case CapacityEffectKind::ExtraAttack:
        case CapacityEffectKind::ProficientCheckBonus: {
            const std::optional<int> valeur = lireEntier(objet, "value");
            if (!valeur.has_value()) {
                erreurs.push_back(fichier + " : effet '" + genre + "' sans 'value'.");
                return std::nullopt;
            }
            if (effet.kind == CapacityEffectKind::ExtraAttack && *valeur <= 0) {
                // Zero attaque en plus serait une capacite nommee au journal qui ne fait rien.
                erreurs.push_back(fichier + " : effet '" + genre +
                                  "' dont 'value' n'est pas "
                                  "positive.");
                return std::nullopt;
            }
            effet.value = *valeur;
            break;
        }
        case CapacityEffectKind::UnarmoredArmorClass: {
            const std::optional<int> base = lireEntier(objet, "base");
            if (!base.has_value()) {
                erreurs.push_back(fichier + " : effet '" + genre + "' sans 'base'.");
                return std::nullopt;
            }
            effet.base = *base;
            effet.shieldAllowed = lireBooleen(objet, "shieldAllowed", true);
            for (const std::string& nom : lireTextes(objet, "abilities")) {
                const std::optional<Ability> caracteristique = parseAbility(nom);
                if (!caracteristique.has_value()) {
                    erreurs.push_back(fichier + " : caracteristique '" + nom +
                                      "' inconnue du moteur dans un effet '" + genre + "'.");
                    return std::nullopt;
                }
                effet.abilities.push_back(*caracteristique);
            }
            break;
        }
        case CapacityEffectKind::DamageResistance: {
            effet.allDamageTypes = lireBooleen(objet, "allTypes", false);
            for (const std::string& nom : lireTextes(objet, "types")) {
                const std::optional<DamageType> type = parseDamageType(nom);
                if (!type.has_value()) {
                    erreurs.push_back(fichier + " : type de degats '" + nom +
                                      "' inconnu du moteur (EX-CBT-032).");
                    return std::nullopt;
                }
                effet.damageTypes.push_back(*type);
            }
            if (!effet.allDamageTypes && effet.damageTypes.empty()) {
                erreurs.push_back(fichier + " : effet '" + genre +
                                  "' sans type : ni 'allTypes' ni 'types'.");
                return std::nullopt;
            }
            break;
        }
        case CapacityEffectKind::SpeedBonus: {
            const auto metres = objet.find("meters");
            if (metres == objet.end() || !metres->is_number()) {
                erreurs.push_back(fichier + " : effet '" + genre + "' sans 'meters'.");
                return std::nullopt;
            }
            effet.meters = metres->get<float>();
            break;
        }
        case CapacityEffectKind::NoOpportunityAttacks:
            break;
        case CapacityEffectKind::ExtraDamage: {
            const std::optional<Dice> des = parseDice(lireTexte(objet, "dice"));
            if (!des.has_value()) {
                erreurs.push_back(fichier + " : effet '" + genre + "' sans 'dice' lisibles.");
                return std::nullopt;
            }
            effet.dice = *des;
            effet.oncePerTurn = lireBooleen(objet, "oncePerTurn", false);
            effet.allyAdjacentToTarget = lireBooleen(objet, "allyAdjacentToTarget", false);
            break;
        }
    }
    return effet;
}

[[nodiscard]] std::optional<Capacity> lireCapacite(const nlohmann::json& racine,
                                                   const std::string& fichier,
                                                   std::vector<std::string>& erreurs) {
    Capacity capacite;
    capacite.id = lireTexte(racine, "id");
    capacite.name = lireTexte(racine, "name");
    capacite.source = lireTexte(racine, "source");
    capacite.text = lireTexte(racine, "text");
    capacite.replaces = lireTexte(racine, "replaces");
    capacite.narrative = lireBooleen(racine, "narratif", false);
    capacite.requiredMechanisms = lireTextes(racine, "mecanismesRequis");
    if (capacite.id.empty() || capacite.name.empty()) {
        erreurs.push_back(fichier + " : capacite sans 'id' ou sans 'name'.");
        return std::nullopt;
    }
    if (const auto effets = racine.find("effects"); effets != racine.end() && effets->is_array()) {
        for (const auto& element : *effets) {
            if (!element.is_object()) {
                continue;
            }
            std::optional<CapacityEffect> effet = lireEffet(element, fichier, erreurs);
            if (!effet.has_value()) {
                // La capacite entiere est refusee : jouee a moitie, elle tromperait plus
                // qu'absente et nommee dans les erreurs.
                return std::nullopt;
            }
            capacite.effects.push_back(std::move(*effet));
        }
    }
    return capacite;
}

}  // namespace

std::string_view capacityEffectKindName(CapacityEffectKind kind) noexcept {
    for (const NomDeGenre& entree : GENRES) {
        if (entree.genre == kind) {
            return entree.nom;
        }
    }
    return "?";
}

std::optional<CapacityEffectKind> parseCapacityEffectKind(std::string_view name) noexcept {
    for (const NomDeGenre& entree : GENRES) {
        if (entree.nom == name) {
            return entree.genre;
        }
    }
    return std::nullopt;
}

const Capacity* CapacityCatalog::find(std::string_view id) const {
    const auto trouve = std::ranges::find(capacities, id, &Capacity::id);
    return trouve == capacities.end() ? nullptr : &*trouve;
}

CapacityCatalog loadCapacities(const std::filesystem::path& capacitiesDir) {
    CapacityCatalog catalogue;
    std::error_code code;
    if (!std::filesystem::is_directory(capacitiesDir, code)) {
        // Un dossier absent n'est PAS un catalogue vide (EX-CNT-010).
        catalogue.errors.push_back(capacitiesDir.string() + " : dossier absent ou illisible.");
        return catalogue;
    }
    std::vector<std::filesystem::path> fichiers;
    for (const auto& entree : std::filesystem::directory_iterator(capacitiesDir, code)) {
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
        if (std::optional<Capacity> capacite =
                lireCapacite(document.root, chemin.filename().string(), catalogue.errors)) {
            catalogue.capacities.push_back(std::move(*capacite));
        }
    }
    std::ranges::sort(catalogue.capacities, {}, &Capacity::id);
    return catalogue;
}

// --- Lecture des effets ------------------------------------------------------------------------

std::vector<Modifier> attackModifiersFrom(std::span<const Capacity> capacities) {
    std::vector<Modifier> modificateurs;
    for (const Capacity& capacite : capacities) {
        for (const CapacityEffect& effet : capacite.effects) {
            if (effet.kind == CapacityEffectKind::AttackBonus && effet.value != 0) {
                modificateurs.push_back({.source = capacite.name, .value = effet.value});
            }
        }
    }
    return modificateurs;
}

int armorClassBonusFrom(std::span<const Capacity> capacities) {
    int total = 0;
    for (const Capacity& capacite : capacities) {
        for (const CapacityEffect& effet : capacite.effects) {
            if (effet.kind == CapacityEffectKind::ArmorClassBonus) {
                total += effet.value;
            }
        }
    }
    return total;
}

std::optional<UnarmoredArmorClass> unarmoredArmorClassFrom(std::span<const Capacity> capacities,
                                                           std::span<const int, 6> abilityScores) {
    std::optional<UnarmoredArmorClass> meilleure;
    for (const Capacity& capacite : capacities) {
        for (const CapacityEffect& effet : capacite.effects) {
            if (effet.kind != CapacityEffectKind::UnarmoredArmorClass) {
                continue;
            }
            int total = effet.base;
            for (const Ability caracteristique : effet.abilities) {
                total += abilityModifier(abilityScores[static_cast<std::size_t>(caracteristique)]);
            }
            // Deux formules ne s'additionnent pas : la meilleure seule compte, comme deux abris.
            if (!meilleure.has_value() || total > meilleure->armorClass) {
                meilleure = UnarmoredArmorClass{.armorClass = total,
                                                .shieldAllowed = effet.shieldAllowed,
                                                .source = capacite.name};
            }
        }
    }
    return meilleure;
}

float speedBonusFrom(std::span<const Capacity> capacities) {
    float total = 0.0F;
    for (const Capacity& capacite : capacities) {
        for (const CapacityEffect& effet : capacite.effects) {
            if (effet.kind == CapacityEffectKind::SpeedBonus) {
                total += effet.meters;
            }
        }
    }
    return total;
}

std::optional<std::string> opportunityImmunityFrom(std::span<const Capacity> capacities) {
    for (const Capacity& capacite : capacities) {
        for (const CapacityEffect& effet : capacite.effects) {
            if (effet.kind == CapacityEffectKind::NoOpportunityAttacks) {
                return capacite.name;
            }
        }
    }
    return std::nullopt;
}

std::vector<NamedResistance> resistancesFrom(std::span<const Capacity> capacities) {
    std::vector<NamedResistance> resistances;
    for (const Capacity& capacite : capacities) {
        for (const CapacityEffect& effet : capacite.effects) {
            if (effet.kind != CapacityEffectKind::DamageResistance) {
                continue;
            }
            if (effet.allDamageTypes) {
                resistances.push_back({.type = std::nullopt, .source = capacite.name});
                continue;
            }
            for (const DamageType type : effet.damageTypes) {
                resistances.push_back({.type = type, .source = capacite.name});
            }
        }
    }
    return resistances;
}

std::vector<NamedExtraDamage> extraDamageFrom(std::span<const Capacity> capacities) {
    std::vector<NamedExtraDamage> des;
    for (const Capacity& capacite : capacities) {
        for (const CapacityEffect& effet : capacite.effects) {
            if (effet.kind == CapacityEffectKind::ExtraDamage) {
                des.push_back({.dice = effet.dice,
                               .oncePerTurn = effet.oncePerTurn,
                               .capacityId = capacite.id,
                               .source = capacite.name,
                               .allyAdjacentToTarget = effet.allyAdjacentToTarget});
            }
        }
    }
    return des;
}

int proficientCheckBonusFrom(std::span<const Capacity> capacities) {
    int total = 0;
    for (const Capacity& capacite : capacities) {
        for (const CapacityEffect& effet : capacite.effects) {
            if (effet.kind == CapacityEffectKind::ProficientCheckBonus) {
                total += effet.value;
            }
        }
    }
    return total;
}

std::optional<NamedExtraAttacks> extraAttacksFrom(std::span<const Capacity> capacities) {
    std::optional<NamedExtraAttacks> meilleure;
    for (const Capacity& capacite : capacities) {
        for (const CapacityEffect& effet : capacite.effects) {
            // Deux sources d'attaques en plus ne s'additionnent pas : la plus genereuse compte.
            if (effet.kind == CapacityEffectKind::ExtraAttack &&
                (!meilleure.has_value() || effet.value > meilleure->count)) {
                meilleure = NamedExtraAttacks{.count = effet.value, .source = capacite.name};
            }
        }
    }
    return meilleure;
}

}  // namespace core
