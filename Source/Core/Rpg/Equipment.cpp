// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Rpg/Equipment.h"

#include <algorithm>
#include <system_error>

#include "Core/Data/JsonDocument.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/RpgEnumNames.h"

namespace core {

namespace {

// Les entrees de catalogue ne portent pas de champ `version` : ce sont des donnees, pas des
// documents de format.
constexpr int SANS_GARDE_DE_VERSION = 0;

[[nodiscard]] std::string lireTexte(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_string()) ? trouve->get<std::string>()
                                                          : std::string{};
}

[[nodiscard]] int lireEntier(const nlohmann::json& objet, const char* champ, int defaut = 0) {
    const auto trouve = objet.find(champ);
    return (trouve != objet.end() && trouve->is_number_integer()) ? trouve->get<int>() : defaut;
}

[[nodiscard]] bool lireBooleen(const nlohmann::json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    return trouve != objet.end() && trouve->is_boolean() && trouve->get<bool>();
}

template <typename Lecteur>
void balayer(const std::filesystem::path& dossier, std::vector<std::string>& erreurs,
             Lecteur lecteur) {
    std::error_code code;
    if (!std::filesystem::is_directory(dossier, code)) {
        erreurs.push_back(dossier.string() + " : dossier absent ou illisible.");
        return;
    }
    std::vector<std::filesystem::path> fichiers;
    for (const auto& entree : std::filesystem::directory_iterator(dossier, code)) {
        if (entree.is_regular_file(code) && entree.path().extension() == ".json") {
            fichiers.push_back(entree.path());
        }
    }
    std::ranges::sort(fichiers);
    for (const std::filesystem::path& chemin : fichiers) {
        const JsonDocument document = readJsonObjectFromFile(chemin, SANS_GARDE_DE_VERSION);
        if (!document.ok()) {
            erreurs.push_back(document.message);
            continue;
        }
        lecteur(document.root, chemin.filename().string());
    }
}

}  // namespace

std::string_view armorCategoryName(ArmorCategory category) {
    // switch exhaustif sans default : une categorie ajoutee sans nom casse la compilation.
    switch (category) {
        case ArmorCategory::Light:
            return "light";
        case ArmorCategory::Medium:
            return "medium";
        case ArmorCategory::Heavy:
            return "heavy";
        case ArmorCategory::Shield:
            return "shield";
    }
    return {};
}

std::optional<ArmorCategory> parseArmorCategory(std::string_view name) {
    for (const ArmorCategory categorie : {ArmorCategory::Light, ArmorCategory::Medium,
                                          ArmorCategory::Heavy, ArmorCategory::Shield}) {
        if (armorCategoryName(categorie) == name) {
            return categorie;
        }
    }
    return std::nullopt;
}

bool hasProperty(const Weapon& weapon, std::string_view property) {
    return std::ranges::find(weapon.properties, property) != weapon.properties.end();
}

const Weapon* EquipmentCatalog::findWeapon(std::string_view id) const {
    const auto trouve = std::ranges::find(weapons, id, &Weapon::id);
    return trouve == weapons.end() ? nullptr : &*trouve;
}

const Armor* EquipmentCatalog::findArmor(std::string_view id) const {
    const auto trouve = std::ranges::find(armors, id, &Armor::id);
    return trouve == armors.end() ? nullptr : &*trouve;
}

namespace {

[[nodiscard]] Weapon lireArme(const nlohmann::json& racine, const std::string& fichier,
                              std::vector<std::string>& erreurs) {
    Weapon arme;
    arme.id = lireTexte(racine, "id");
    arme.name = lireTexte(racine, "name");
    arme.source = lireTexte(racine, "source");
    arme.category = lireTexte(racine, "category");
    arme.ranged = lireBooleen(racine, "ranged");
    arme.price = lireEntier(racine, "price");
    arme.weightGrams = lireEntier(racine, "weightGrams");
    arme.text = lireTexte(racine, "text");
    if (const auto proprietes = racine.find("properties");
        proprietes != racine.end() && proprietes->is_array()) {
        for (const nlohmann::json& propriete : *proprietes) {
            if (propriete.is_string()) {
                arme.properties.push_back(propriete.get<std::string>());
            }
        }
    }
    for (const auto& [cle, portee] :
         {std::pair{"rangeNormal", &arme.rangeNormal}, std::pair{"rangeLong", &arme.rangeLong}}) {
        if (const auto valeur = racine.find(cle); valeur != racine.end() && valeur->is_number()) {
            *portee = valeur->get<float>();
        }
    }
    const std::string des = lireTexte(racine, "damage");
    if (!des.empty()) {
        arme.damage = parseDice(des);
        if (!arme.damage.has_value()) {
            erreurs.push_back(fichier + " : notation de des illisible ('" + des + "').");
        }
    }
    const std::string type = lireTexte(racine, "damageType");
    if (!type.empty()) {
        arme.damageType = parseDamageType(type);
        if (!arme.damageType.has_value()) {
            erreurs.push_back(fichier + " : type de degats '" + type + "' inconnu du moteur.");
        }
    }
    return arme;
}

[[nodiscard]] std::optional<Armor> lireArmure(const nlohmann::json& racine,
                                              const std::string& fichier,
                                              std::vector<std::string>& erreurs) {
    Armor armure;
    armure.id = lireTexte(racine, "id");
    armure.name = lireTexte(racine, "name");
    armure.source = lireTexte(racine, "source");
    const std::optional<ArmorCategory> categorie =
        parseArmorCategory(lireTexte(racine, "category"));
    if (!categorie.has_value()) {
        erreurs.push_back(fichier + " : categorie d'armure inconnue.");
        return std::nullopt;
    }
    armure.category = *categorie;
    armure.baseArmorClass = lireEntier(racine, "baseArmorClass");
    armure.dexterityBonus = lireBooleen(racine, "dexterityBonus");
    if (const auto plafond = racine.find("dexterityBonusMax");
        plafond != racine.end() && plafond->is_number_integer()) {
        armure.dexterityBonusMax = plafond->get<int>();
    }
    if (const auto force = racine.find("strengthRequired");
        force != racine.end() && force->is_number_integer()) {
        armure.strengthRequired = force->get<int>();
    }
    armure.stealthDisadvantage = lireBooleen(racine, "stealthDisadvantage");
    armure.price = lireEntier(racine, "price");
    armure.weightGrams = lireEntier(racine, "weightGrams");
    return armure;
}

}  // namespace

EquipmentCatalog loadEquipment(const std::filesystem::path& weaponsDir,
                               const std::filesystem::path& armorsDir) {
    EquipmentCatalog catalogue;

    balayer(weaponsDir, catalogue.errors,
            [&catalogue](const nlohmann::json& racine, const std::string& fichier) {
                catalogue.weapons.push_back(lireArme(racine, fichier, catalogue.errors));
            });

    balayer(armorsDir, catalogue.errors,
            [&catalogue](const nlohmann::json& racine, const std::string& fichier) {
                if (std::optional<Armor> armure = lireArmure(racine, fichier, catalogue.errors)) {
                    catalogue.armors.push_back(std::move(*armure));
                }
            });

    std::ranges::sort(catalogue.weapons, {}, &Weapon::id);
    std::ranges::sort(catalogue.armors, {}, &Armor::id);
    return catalogue;
}

int armorClassFor(const CharacterSheet& sheet, const CharacterCreationRules& rules,
                  const Armor* armor, const Armor* shield) {
    const int dexterite = sheet.modifier(Ability::Dexterity);

    int total = 0;
    bool bouclierPermis = true;
    if (armor == nullptr || armor->category == ArmorCategory::Shield) {
        // Sans armure : la base vient de la DONNEE, jamais d'un 10 ecrit ici (EX-VIS-007). Un
        // bouclier passe en `armor` n'est pas une armure : il ajoute, il ne remplace pas, et le
        // traiter comme tel donnerait une CA de 2 a un personnage en bouclier seul.
        total = rules.unarmoredArmorClass + dexterite;
        // Une capacite de classe peut calculer la CA sans armure AUTREMENT (EX-CBT-030, LOT-131) :
        // Tough as Nails, 10 + Dex + Con ; Arcane Protection, 13 + Dex. La meilleure formule
        // compte, jamais la somme : une formule remplace la regle generale, elle ne s'y ajoute pas.
        if (const std::optional<UnarmoredArmorClass> formule =
                unarmoredArmorClassFrom(sheet.capacities, sheet.abilities);
            formule.has_value() && formule->armorClass > total) {
            total = formule->armorClass;
            bouclierPermis = formule->shieldAllowed;
        }
    } else {
        total = armor->baseArmorClass;
        if (armor->dexterityBonus) {
            // `11 + Mod.Dex` sans plafond, `14 + Mod.Dex (max +2)` avec. Une armure lourde n'a pas
            // `dexterityBonus` du tout : lui appliquer la Dexterite rendrait le personnage plus
            // resistant, jamais moins, et le defaut passerait pour de l'equilibrage.
            total += armor->dexterityBonusMax.has_value()
                         ? std::min(dexterite, *armor->dexterityBonusMax)
                         : dexterite;
        }
    }

    if (shield != nullptr && shield->category == ArmorCategory::Shield && bouclierPermis) {
        total += shield->baseArmorClass;
    }
    // Un bonus fixe (Holy Shield, Scoundrel's Agility au niveau 5) s'ajoute a toute forme.
    return total + armorClassBonusFrom(sheet.capacities);
}

int totalWeightGrams(const std::vector<InventoryEntry>& entries) {
    int total = 0;
    for (const InventoryEntry& ligne : entries) {
        if (ligne.quantity <= 0) {
            continue;
        }
        total += ligne.weightGrams * ligne.quantity;
    }
    return total;
}

}  // namespace core
