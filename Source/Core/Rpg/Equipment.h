// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Rpg/Equipment.h
 * @brief Armes, armures et le calcul de la classe d'armure (`LOT-34`).
 */

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Rpg/Dice.h"
#include "Core/Rpg/RpgEnums.h"

namespace core {

struct CharacterSheet;
struct CharacterCreationRules;

/// @brief Catégorie d'armure. Ensemble **fermé** : le livre n'en connaît pas d'autre.
enum class ArmorCategory {
    Light,
    Medium,
    Heavy,
    /// Le bouclier n'est **pas** une armure : il *ajoute* à la classe d'armure au lieu de la
    /// remplacer. Le ranger dans la même énumération vient du livre, qui le met dans la même
    /// table ; les confondre dans le calcul donnerait un personnage en bouclier seul avec une CA
    /// de 2.
    Shield,
};

/// @brief Une armure ou un bouclier du catalogue.
struct Armor {
    std::string id;
    std::string name;
    std::string source;
    ArmorCategory category = ArmorCategory::Light;
    /// Classe d'armure de base — ou le **bonus** du bouclier, qui s'ajoute.
    int baseArmorClass = 0;
    /// Vrai si le modificateur de Dextérité s'ajoute.
    bool dexterityBonus = false;
    /// Plafond du modificateur de Dextérité, s'il y en a un (`+2` sur les armures intermédiaires).
    std::optional<int> dexterityBonusMax;
    /// Force exigée pour ne pas être ralenti. Absente si l'armure n'en demande pas.
    std::optional<int> strengthRequired;
    bool stealthDisadvantage = false;
    /// Prix en **pièces de cuivre** — l'unité interne du projet (`LOT-32`).
    int price = 0;
    /// Poids en **grammes**.
    int weightGrams = 0;
};

/// @brief Une arme du catalogue.
struct Weapon {
    std::string id;
    std::string name;
    std::string source;
    /// `simple` ou `martial`.
    std::string category;
    bool ranged = false;
    /// Dés de dégâts. **Absents** pour le filet, qui n'inflige rien et entrave.
    std::optional<Dice> damage;
    std::optional<DamageType> damageType;
    int price = 0;
    int weightGrams = 0;
    /// Les propriétés, telles que le livre les écrit — en toutes lettres, et en français.
    std::string text;
    /// Les mêmes, **structurées** (`finesse`, `light`, `reach`, `thrown`...), tirées de la colonne
    /// des propriétés **à l'extraction** (`LOT-22`). Une règle qui en dépend — la caractéristique
    /// d'attaque d'une arme de finesse, l'allonge d'une hallebarde — les lit ici, jamais dans la
    /// prose de `text`.
    std::vector<std::string> properties;
    /// Portée normale, en **mètres**, telle que le livre l'écrit (« portée 24 m/96 m »). Absente
    /// pour une arme qui ne se tire ni ne se lance.
    std::optional<float> rangeNormal;
    /// Longue portée, en mètres.
    std::optional<float> rangeLong;
};

/// @brief Vrai si l'arme porte la propriété @p property (`"finesse"`, `"reach"`, `"thrown"`...).
[[nodiscard]] bool hasProperty(const Weapon& weapon, std::string_view property);

/// @brief Les deux catalogues chargés, et ce qui n'a pas pu l'être.
struct EquipmentCatalog {
    std::vector<Weapon> weapons;
    std::vector<Armor> armors;
    std::vector<std::string> errors;

    /// @brief L'arme d'identifiant @p id, ou `nullptr` si elle est inconnue.
    [[nodiscard]] const Weapon* findWeapon(std::string_view id) const;
    /// @brief L'armure d'identifiant @p id, ou `nullptr` si elle est inconnue.
    [[nodiscard]] const Armor* findArmor(std::string_view id) const;
};

/// @brief Charge les armes et les armures depuis leurs dossiers.
[[nodiscard]] EquipmentCatalog loadEquipment(const std::filesystem::path& weaponsDir,
                                             const std::filesystem::path& armorsDir);

/**
 * @brief La classe d'armure d'un personnage, armure et bouclier compris.
 *
 * **Les trois formes de la table du livre disent trois règles différentes**, et les réduire à leur
 * premier nombre est la faute qui ne se voit pas :
 *
 * - `11 + Mod.Dex` — le modificateur de Dextérité s'ajoute **sans plafond** (armures légères) ;
 * - `14 + Mod.Dex (max +2)` — il s'ajoute **plafonné à 2** (armures intermédiaires) ;
 * - `18` — il ne s'ajoute **pas du tout** (armures lourdes).
 *
 * Appliquer la Dextérité au harnois rendrait le personnage plus résistant, jamais moins : le
 * défaut ne provoque aucune erreur et passe pour de l'équilibrage.
 *
 * Sans armure, la base vient de la **donnée** (`rules/character-creation.json`, `LOT-13`) et non
 * d'un `10` écrit ici (`EX-VIS-007`).
 *
 * @param sheet La fiche, pour son modificateur de Dextérité.
 * @param rules Les constantes de création, pour la base sans armure.
 * @param armor L'armure portée, ou `nullptr`.
 * @param shield Le bouclier porté, ou `nullptr`.
 */
[[nodiscard]] int armorClassFor(const CharacterSheet& sheet, const CharacterCreationRules& rules,
                                const Armor* armor, const Armor* shield);

/// @brief Une ligne d'inventaire : un objet et sa quantité.
struct InventoryEntry {
    /// Poids unitaire, en grammes.
    int weightGrams = 0;
    /// Quantité. Une quantité nulle ou négative ne pèse rien.
    int quantity = 1;
};

/**
 * @brief Le poids total d'un inventaire, en **grammes**.
 *
 * Une seule unité, et c'est la raison d'être de cette fonction : le livre écrit des kilogrammes et
 * des grammes dans la même table — « 6,5 kg », « 500 g » —, et mélanger les deux dans un total
 * donnerait un sac de cinq cents kilos pour une poignée de fléchettes. La conversion est faite à
 * l'extraction ; ici tout est déjà en grammes.
 */
[[nodiscard]] int totalWeightGrams(const std::vector<InventoryEntry>& entries);

/// @brief Nom textuel d'une catégorie d'armure, tel que la donnée l'écrit.
[[nodiscard]] std::string_view armorCategoryName(ArmorCategory category);
/// @brief Inverse d'`armorCategoryName`. `std::nullopt` si le nom est inconnu — jamais deviné.
[[nodiscard]] std::optional<ArmorCategory> parseArmorCategory(std::string_view name);

}  // namespace core
