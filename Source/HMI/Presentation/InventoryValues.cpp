// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Presentation/InventoryValues.h"

#include <string>

namespace hmi {
namespace {

// Les taux du livre : cent pièces de cuivre valent une pièce d'or, dix une pièce d'argent.
// Écrits ici, et non dans le modèle, parce que ce sont des **grandeurs d'affichage** : le modèle
// ne compte qu'en cuivre.
constexpr int CUIVRE_PAR_ARGENT = 10;
constexpr int CUIVRE_PAR_OR = 100;

// Rend : Le nom d'un objet, cherché dans les trois catalogues, ou son identifiant à défaut.
//         L'identifiant plutôt qu'un tiret : un emplacement qui porte quelque chose que le
//         catalogue ignore doit se **voir**, et le tiret le ferait passer pour vide.
[[nodiscard]] std::string nomDe(const core::ItemLookup& catalogues, const std::string& id) {
    if (id.empty()) {
        return {};
    }
    if (catalogues.items != nullptr) {
        if (const core::Item* const objet = catalogues.items->find(id); objet != nullptr) {
            return objet->name;
        }
    }
    if (catalogues.equipment != nullptr) {
        if (const core::Weapon* const arme = catalogues.equipment->findWeapon(id);
            arme != nullptr) {
            return arme->name;
        }
        if (const core::Armor* const armure = catalogues.equipment->findArmor(id);
            armure != nullptr) {
            return armure->name;
        }
    }
    return id;
}

// Rend : Un poids en grammes, écrit en kilogrammes avec une décimale : « 12,5 kg ».
[[nodiscard]] std::string kilogrammes(int grammes) {
    const int dixiemes = (grammes + 50) / 100;  // arrondi au dixième de kilogramme
    return std::to_string(dixiemes / 10) + "," + std::to_string(dixiemes % 10) + " kg";
}

}  // namespace

void splitPurse(int copper, int& gold, int& silver, int& rest) {
    const int total = copper > 0 ? copper : 0;
    gold = total / CUIVRE_PAR_OR;
    silver = (total % CUIVRE_PAR_OR) / CUIVRE_PAR_ARGENT;
    rest = total % CUIVRE_PAR_ARGENT;
}

std::map<std::string, std::string> inventoryValues(const InventoryContext& context) {
    std::map<std::string, std::string> valeurs;
    if (context.inventory == nullptr) {
        // Aucun inventaire : une table VIDE. L'écran garde ses tirets, ce qui est la vérité.
        return valeurs;
    }
    const core::Inventory& sac = *context.inventory;

    // Les seize emplacements. Le nom de l'emplacement vient du NOYAU, jamais réécrit ici : deux
    // listes finiraient par diverger, et l'écart ne se verrait qu'à l'écran.
    for (std::size_t rang = 0; rang < core::EQUIPMENT_SLOT_COUNT; ++rang) {
        const auto emplacement = static_cast<core::EquipmentSlot>(rang);
        const std::string identifiant =
            std::string("inventory.slot.") + std::string(core::equipmentSlotName(emplacement));
        const std::string porte = nomDe(context.lookup, sac.at(emplacement));
        valeurs[identifiant] = porte.empty() ? context.emptyMark : porte;
    }

    // La bourse, répartie en or, argent et cuivre : le modèle ne compte qu'en cuivre, et cette
    // répartition est une règle d'AFFICHAGE.
    int pieceDOr = 0;
    int argent = 0;
    int cuivre = 0;
    splitPurse(sac.purseCopper, pieceDOr, argent, cuivre);
    valeurs["inventory.purse"] = std::to_string(pieceDOr) + " po " + std::to_string(argent) +
                                 " pa " + std::to_string(cuivre) + " pc";

    // La charge : ce que le personnage porte, contre ce qu'il PEUT porter. Les deux en kilogrammes
    // à l'écran, alors que le modèle compte en grammes -- un sac de 12 450 g ne se lit pas.
    valeurs["inventory.carried"] = kilogrammes(context.derived.carriedWeightGrams);
    valeurs["inventory.capacity"] = context.derived.carryingCapacityGrams > 0
                                        ? kilogrammes(context.derived.carryingCapacityGrams)
                                        : context.emptyMark;

    // Le sac : une ligne par pile, la quantité seulement quand il y en a plusieurs. « Torche ×1 »
    // se lit moins bien que « Torche », et l'inventaire d'un personnage en compte beaucoup.
    std::string contenu;
    for (const core::InventoryStack& ligne : sac.backpack) {
        if (ligne.quantity <= 0) {
            continue;
        }
        if (!contenu.empty()) {
            contenu += "\n";
        }
        contenu += nomDe(context.lookup, ligne.itemId);
        if (ligne.quantity > 1) {
            contenu += " ×" + std::to_string(ligne.quantity);
        }
    }
    valeurs["inventory.backpack"] = contenu.empty() ? context.emptyMark : contenu;

    return valeurs;
}

}  // namespace hmi
