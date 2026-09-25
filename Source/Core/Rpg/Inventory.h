// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Rpg/Inventory.h
 * @brief Objets portés, emplacements d'équipement et statistiques dérivées (`LOT-14`,
 *        `EX-INV-020`).
 */

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Rpg/Ability.h"
#include "Core/Rpg/Dice.h"
#include "Core/Rpg/Equipment.h"
#include "Core/Rpg/RpgEnums.h"

namespace core {

struct CharacterSheet;
struct CharacterCreationRules;

/// @brief Un objet du catalogue qui n'est ni une arme ni une armure : matériel, outil, monture,
///        marchandise, babiole, objet magique.
struct Item {
    std::string id;
    std::string name;
    std::string source;
    /// Famille d'usage, telle que la donnée l'écrit (`gear`, `tool`, `mount`...).
    std::string category;
    /// Prix en **pièces de cuivre** — l'unité interne du projet (`LOT-32`).
    int price = 0;
    /// Poids en **grammes**. Un objet sans poids déclaré ne pèse rien : c'est le cas des objets
    /// dont le livre ne donne pas la masse, et non un oubli à corriger par une valeur inventée.
    int weightGrams = 0;
    /// Vrai si plusieurs exemplaires se rangent dans une même ligne d'inventaire.
    bool stackable = false;
    std::string rarity;
    bool requiresAttunement = false;
    std::string text;
};

/// @brief Le catalogue d'objets, et ce qui n'a pas pu être lu.
struct ItemCatalog {
    std::vector<Item> items;
    std::vector<std::string> errors;

    /// @brief L'objet d'identifiant @p id, ou `nullptr` s'il est inconnu.
    [[nodiscard]] const Item* find(std::string_view id) const;
};

/// @brief Charge les objets depuis leur dossier (`Rpg/items/`).
[[nodiscard]] ItemCatalog loadItems(const std::filesystem::path& itemsDir);

/**
 * @brief Les bornes de charge, lues dans la donnée (`rules/encumbrance.json`).
 *
 * En **grammes par point de Force**, et non en kilogrammes : les poids d'objets sont déjà des
 * grammes entiers dans les catalogues, et mêler les deux unités dans une somme est l'erreur que
 * l'unité unique supprime.
 */
struct EncumbranceRules {
    int carryingCapacityGramsPerStrength = 0;
    int encumberedGramsPerStrength = 0;
    int heavilyEncumberedGramsPerStrength = 0;
    float encumberedSpeedPenaltyMeters = 0.0F;
    float heavilyEncumberedSpeedPenaltyMeters = 0.0F;

    /// @return Vrai si les bornes ont été lues. Une règle absente ne se remplace pas par des
    ///         valeurs « raisonnables » : le calcul de charge se déclare alors indisponible.
    [[nodiscard]] bool isLoaded() const noexcept {
        return carryingCapacityGramsPerStrength > 0;
    }
};

/// @brief Charge les bornes de charge depuis `rules/encumbrance.json`.
/// @return Les bornes, ou une règle non chargée (`isLoaded()` faux) si le fichier manque.
[[nodiscard]] EncumbranceRules loadEncumbranceRules(const std::filesystem::path& rulesFile);

/// @brief État de charge d'un personnage, dans l'ordre croissant de gêne.
enum class EncumbranceLevel {
    Unencumbered,       ///< Rien ne le ralentit.
    Encumbered,         ///< Au-delà du premier seuil : vitesse diminuée.
    HeavilyEncumbered,  ///< Au-delà du second : vitesse diminuée davantage, et désavantage.
    OverCapacity,       ///< Au-delà de la capacité de charge : il ne peut plus porter.
};

/**
 * @brief Les seize emplacements d'équipement de la planche 2 du corpus.
 *
 * L'ordre est celui de la feuille, colonne gauche puis colonne droite, et il est **relevé** et non
 * choisi : c'est celui que l'écran d'inventaire suit pour poser ses lignes.
 */
enum class EquipmentSlot {
    Head,
    Neck,
    Cloak,
    Torso,
    Belt,
    Hands,
    RingLeft,
    RingRight,
    MainHand,
    OffHand,
    Ranged,
    Ammunition,
    Feet,
    Bracers,
    Pouch,
    Trinket,
};

/// Nombre d'emplacements, pour dimensionner les tableaux qui les indexent.
inline constexpr std::size_t EQUIPMENT_SLOT_COUNT = 16;

/// @brief Nom textuel d'un emplacement, tel que la donnée et les clés de traduction l'écrivent.
[[nodiscard]] std::string_view equipmentSlotName(EquipmentSlot slot);
/// @brief Inverse d'`equipmentSlotName`. `std::nullopt` si le nom est inconnu — jamais deviné.
[[nodiscard]] std::optional<EquipmentSlot> parseEquipmentSlot(std::string_view name);

/// @brief Une ligne du sac : un identifiant de catalogue et une quantité.
struct InventoryStack {
    std::string itemId;
    int quantity = 1;
};

/**
 * @brief Ce qu'un personnage porte : son sac, et ce qu'il a sur lui.
 *
 * ## Aucune statistique n'est stockée ici
 *
 * C'est **tout le lot**. La classe d'armure, les dégâts de l'arme, la vitesse encombrée ne sont
 * pas des champs de cette structure : ce sont des fonctions de ce qu'elle contient, recalculées à
 * chaque lecture (`derivedStatsFor`). Le défaut classique consiste à appliquer un bonus en
 * l'ajoutant à la volée (`ca += 2`) et à le retrancher au retrait ; après trois équipements et
 * deux retraits dans le désordre, la classe d'armure a **dérivé**, et rien ne le signale — le
 * personnage est simplement devenu un peu plus, ou un peu moins, résistant.
 *
 * Une valeur qu'on ne stocke pas ne peut pas dériver. C'est un critère d'acceptation du lot, pas
 * une préférence de style.
 */
struct Inventory {
    /// L'équipement porté, par emplacement : un identifiant de catalogue, ou une chaîne vide.
    std::array<std::string, EQUIPMENT_SLOT_COUNT> equipped{};
    /// Le sac, dans l'ordre où il a été rempli.
    std::vector<InventoryStack> backpack;
    /// Bourse, en **pièces de cuivre** — la même unité que les prix du catalogue.
    int purseCopper = 0;

    /// @return L'identifiant porté à cet emplacement, ou une chaîne vide.
    [[nodiscard]] const std::string& at(EquipmentSlot slot) const {
        return equipped[static_cast<std::size_t>(slot)];
    }
    /// @return Vrai si cet emplacement porte quelque chose.
    [[nodiscard]] bool isEquipped(EquipmentSlot slot) const {
        return !at(slot).empty();
    }
};

/**
 * @brief Équipe @p itemId à @p slot, et rend ce que l'emplacement portait.
 *
 * Aucune règle de compatibilité n'est appliquée ici : ranger une armure à l'emplacement de tête
 * est une question de **donnée** et d'interface, pas de cette fonction. Ce qu'elle garantit est
 * plus étroit et plus utile : l'emplacement ne porte jamais deux choses, et ce qui en sort est
 * rendu à l'appelant plutôt que perdu.
 *
 * @return L'identifiant précédemment porté, ou une chaîne vide.
 */
std::string equip(Inventory& inventory, EquipmentSlot slot, std::string itemId);

/// @brief Retire ce que @p slot porte. @return L'identifiant retiré, ou une chaîne vide.
std::string unequip(Inventory& inventory, EquipmentSlot slot);

/// @brief Ajoute @p quantity exemplaires de @p itemId au sac, en empilant si la ligne existe déjà.
void addToBackpack(Inventory& inventory, const std::string& itemId, int quantity = 1);

/// @brief Retire @p quantity exemplaires de @p itemId. @return Ce qui a réellement été retiré.
int removeFromBackpack(Inventory& inventory, const std::string& itemId, int quantity = 1);

/// @brief Les trois catalogues qu'un inventaire consulte pour peser et pour dériver.
struct ItemLookup {
    const ItemCatalog* items = nullptr;
    const EquipmentCatalog* equipment = nullptr;

    /// @return Le poids en grammes de @p id, cherché dans les trois catalogues. Un identifiant
    ///         inconnu ne pèse rien — et il est **compté** par `unknownIds`, plutôt que d'être
    ///         confondu avec un objet réellement sans masse.
    [[nodiscard]] int weightGramsOf(std::string_view id) const;
};

/// @brief Le poids total porté, sac et équipement compris, en grammes.
[[nodiscard]] int carriedWeightGrams(const Inventory& inventory, const ItemLookup& lookup);

/// @brief Les identifiants que les catalogues ne portent pas — un sac qui en contient ne pèse pas
///        ce qu'il devrait, et le dire vaut mieux que de peser faux en silence.
[[nodiscard]] std::vector<std::string> unknownIds(const Inventory& inventory,
                                                  const ItemLookup& lookup);

/**
 * @brief Ce que l'équipement porté produit — **recalculé**, jamais accumulé.
 *
 * Tous les champs sont fonction de l'inventaire et des catalogues au moment de l'appel. Équiper
 * puis retirer, dans n'importe quel ordre, redonne donc exactement les valeurs de départ : il n'y
 * a rien à annuler, puisqu'il n'y a rien eu à appliquer.
 */
struct DerivedStats {
    /// Classe d'armure, armure et bouclier compris (`core::armorClassFor`, `LOT-34`).
    int armorClass = 0;
    /// Dés de dégâts de l'arme en main directrice. Absents si elle n'en a pas — le filet, ou une
    /// main vide.
    std::optional<Dice> damage;
    std::optional<DamageType> damageType;
    /// Caractéristique du jet d'attaque : Force au corps à corps, Dextérité à distance.
    Ability attackAbility = Ability::Strength;
    /// Poids porté, en grammes, et la charge qui en découle.
    int carriedWeightGrams = 0;
    int carryingCapacityGrams = 0;
    EncumbranceLevel encumbrance = EncumbranceLevel::Unencumbered;
    /// Vitesse une fois la charge retirée, en mètres. Jamais négative.
    float speedMeters = 0.0F;
};

/**
 * @brief La caractéristique du jet d'attaque avec @p weapon (Manuel, chapitre 9, « Modificateurs
 *        du jet »).
 *
 * Force au corps à corps, Dextérité à distance ; une arme de **finesse** laisse le choix, et le
 * personnage prend la meilleure des deux. La même caractéristique s'ajoute aux dégâts. Une seule
 * écriture de la règle, que la fiche (`derivedStatsFor`) et l'attaque (`core::weaponAttackFor`)
 * lisent toutes deux.
 */
[[nodiscard]] Ability weaponAttackAbility(const CharacterSheet& sheet, const Weapon& weapon);

/**
 * @brief Calcule tout ce que l'équipement porté produit, depuis la fiche et les catalogues.
 *
 * @param sheet     La fiche, pour ses caractéristiques et sa vitesse de base.
 * @param inventory Ce qui est porté.
 * @param lookup    Les catalogues.
 * @param creation  Les constantes de création, pour la CA sans armure.
 * @param carrying  Les bornes de charge. Non chargées, la charge reste `Unencumbered` et la
 *                  capacité vaut zéro : la fiche ne prétend pas connaître une règle qui manque.
 */
[[nodiscard]] DerivedStats derivedStatsFor(const CharacterSheet& sheet, const Inventory& inventory,
                                           const ItemLookup& lookup,
                                           const CharacterCreationRules& creation,
                                           const EncumbranceRules& carrying);

}  // namespace core
