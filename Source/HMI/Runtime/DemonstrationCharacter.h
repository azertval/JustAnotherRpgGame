// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Equipment.h"
#include "Core/Rpg/Inventory.h"
#include "Core/Rpg/PartyLedger.h"
#include "Core/Rpg/Skill.h"

/**
 * @file HMI/Runtime/DemonstrationCharacter.h
 * @brief Le personnage de démonstration, chargé **une fois** pour tous les écrans (`LOT-86`).
 */

namespace hmi {

/// Ce qu'un chargement du personnage de démonstration produit, prêt à être publié au QML.
struct DemonstrationCharacter {
    /// Valeurs de la fiche, indexées (`sheet.hit_points`, `sheet.skill.athletics`…).
    std::map<std::string, std::string> sheet;
    /// Valeurs de l'inventaire, indexées (`inventory.slot.head`, `inventory.purse`…).
    std::map<std::string, std::string> inventory;
    /// Compétences du catalogue de règles : identifiant et nom lisible, dans l'ordre du catalogue.
    std::vector<std::pair<std::string, std::string>> skills;
};

/**
 * @brief Le personnage de démonstration et tout ce qu'il faut pour le recalculer : la fiche, ce
 *        qu'il porte, et les catalogues (`LOT-87`, T3.5).
 *
 * L'inventaire de la charte v2 **agit** — équiper, retirer, jeter, trier — et chaque action doit
 * recalculer la classe d'armure et la charge depuis l'inventaire modifié. Une table de valeurs
 * figée ne le permet pas : l'écran garde donc cet état, le modifie, et en redemande les valeurs.
 */
struct DemonstrationState {
    core::CharacterOptions options;
    core::SkillCatalog skills;
    core::ExperienceTable experience;
    core::CharacterCreationRules rules;
    core::CharacterSheet sheet;
    core::Inventory inventory;
    core::ItemCatalog items;
    core::EquipmentCatalog equipment;
    core::EncumbranceRules encumbrance;

    /// Les catalogues qu'un inventaire consulte. Pointe dans cet état : ne pas le copier ensuite.
    [[nodiscard]] core::ItemLookup lookup() const {
        return core::ItemLookup{.items = &items, .equipment = &equipment};
    }
};

/**
 * @return La fiche du personnage joué : celle du **meneur** du groupe de la partie en cours
 *         (`LOT-138`), ou, sans partie, celle du Brawler pré-tiré.
 */
[[nodiscard]] std::filesystem::path playedCharacterFile();

/// @brief Charge le personnage joué (`playedCharacterFile`) et ses catalogues, en journalisant
///        chaque manque.
[[nodiscard]] DemonstrationState loadDemonstrationState();

/**
 * @brief Charge la fiche @p characterFile et ses catalogues : un membre du groupe qui ne mène
 *        pas, que l'écran de groupe montre.
 *
 * **Ce que la partie en a fait s'applique** (`LOT-141`) : si une partie est en cours et que la
 * fiche est celle d'un de ses personnages, le registre du groupe (`core::PartyLedger`) est
 * appliqué — le niveau donné (`core::levelUpTo`), puis les points de vie et les lancers qui
 * restent. Fiche, inventaire, dialogue, groupe et combat lisent ainsi **la même** fiche.
 */
[[nodiscard]] DemonstrationState loadDemonstrationState(const std::filesystem::path& characterFile);
/**
 * @brief Applique à @p state ce que le registre dit de son personnage (`LOT-141`) : le niveau
 *        donné, puis les points de vie et les lancers restants (`core::applyRecord`).
 * @param state La fiche et ses catalogues.
 * @param record L'enregistrement du membre.
 */
void applyMemberRecord(DemonstrationState& state, const core::MemberRecord& record);

/**
 * @brief Les tables de valeurs de chaque fiche de @p characterFiles, dans cet ordre : les
 *        catalogues lus **une fois**, puis chaque fiche (`LOT-138`, l'écran de groupe).
 *
 * Une fiche illisible donne des tables partielles, et son erreur est journalisée.
 */
[[nodiscard]] std::vector<DemonstrationCharacter> loadCharacterValues(
    const std::vector<std::filesystem::path>& characterFiles);

/// @brief Les deux tables de valeurs d'un état, statistiques dérivées **recalculées**.
[[nodiscard]] DemonstrationCharacter demonstrationValues(const DemonstrationState& state);

/**
 * @brief Charge le personnage de démonstration livré en donnée, et calcule ses deux tables.
 *
 * **Le personnage joué est le meneur du groupe** (`LOT-138`) : `playedCharacterFile` le désigne,
 * et c'est **cette fonction** qui a changé — pas les écrans, qui ne consomment que des valeurs
 * nommées, d'où qu'elles viennent. Il n'existe toujours pas de sauvegarde : la fiche se relit de
 * son fichier, telle qu'elle a été pré-tirée.
 *
 * **Pourquoi une fonction partagée.** La fiche et l'inventaire décrivent le **même** personnage :
 * charger deux fois les huit catalogues aurait doublé le travail et, surtout, permis aux deux
 * écrans de diverger — la classe d'armure de la fiche vient de ce que l'inventaire contient, et
 * deux chargements séparés auraient pu ne pas voir le même équipement.
 *
 * Une donnée manquante n'interrompt rien : l'erreur est journalisée en nommant son fichier
 * (`EX-CNT-010`) et les tables sont partielles. Un écran partiel garde ses tirets, ce qui est la
 * vérité — et vaut mieux qu'un écran vide.
 */
/// @note Nommee `loadDemonstrationValues` et non `…Character` : les vues-modeles exposent au
/// QML une methode `loadDemonstrationCharacter`, et deux noms identiques auraient fait que la
/// methode s'appelle elle-meme -- la recherche de nom trouve le membre avant la fonction libre.
[[nodiscard]] DemonstrationCharacter loadDemonstrationValues();

}  // namespace hmi
