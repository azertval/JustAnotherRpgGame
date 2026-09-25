// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Rpg/CharacterSheet.h
 * @brief La fiche d'une créature : caractéristiques, PV, CA, niveau, maîtrises (`LOT-13`).
 */

#include <array>
#include <cstddef>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Rpg/Ability.h"
#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/Inventory.h"
#include "Core/Rpg/Skill.h"

namespace core {

/**
 * @brief Une ligne de la table d'expérience : le seuil d'un niveau et son bonus de maîtrise.
 *
 * Les deux vont ensemble parce que le livre les donne ensemble, dans la même table.
 */
struct ExperienceLevel {
    int level = 0;
    int experience = 0;
    int proficiencyBonus = 0;
};

/**
 * @brief La table d'expérience, du niveau 1 au niveau 20.
 *
 * **Aucune de ces quarante valeurs n'est écrite en C++** (`EX-VIS-007`). C'est la donnée la plus
 * tentante à coder en dur du projet — vingt seuils et vingt bonus tiendraient en trois lignes — et
 * la plus coûteuse à y laisser : équilibrer la progression d'un RPG demanderait alors une
 * recompilation à chaque essai, et sans équilibrage un RPG n'est pas jouable.
 */
struct ExperienceTable {
    std::vector<ExperienceLevel> levels;
    std::vector<std::string> errors;

    /// @brief Le niveau qu'atteint un total de points d'expérience.
    [[nodiscard]] int levelFor(int experiencePoints) const;
    /// @brief Le bonus de maîtrise d'un niveau. `0` si la table ne le porte pas.
    [[nodiscard]] int proficiencyBonusAt(int level) const;
    /// @brief Le seuil d'expérience d'un niveau. `0` si la table ne le porte pas.
    [[nodiscard]] int thresholdAt(int level) const;
    /// @brief Le niveau le plus élevé que la table porte.
    [[nodiscard]] int maximumLevel() const;
};

/**
 * @brief Les deux constantes de règle qu'une fiche emploie, lues dans la donnée.
 *
 * `EX-VIS-007` interdit qu'elles vivent dans le C++ : un `10` nu dans un calcul de classe d'armure
 * ne dit pas ce qu'il représente, et ajuster le plafond d'une caractéristique demanderait une
 * recompilation. Elles viennent de `rules/character-creation.json`, où chacune porte **la phrase
 * du livre qui l'atteste**.
 */
struct CharacterCreationRules {
    int unarmoredArmorClass = 0;
    int maximumAbilityScore = 0;
    std::vector<std::string> errors;

    /// @brief Vrai si les deux valeurs ont été lues. Une règle absente ne se devine pas.
    [[nodiscard]] bool ok() const {
        return unarmoredArmorClass > 0 && maximumAbilityScore > 0;
    }
};

/// @brief Charge les constantes depuis `Source/Elements/Rpg/rules/character-creation.json`.
[[nodiscard]] CharacterCreationRules loadCharacterCreationRules(const std::filesystem::path& path);

/// @brief Charge la table d'expérience depuis `Source/Elements/Rpg/rules/experience.json`.
[[nodiscard]] ExperienceTable loadExperienceTable(const std::filesystem::path& path);

/**
 * @brief La fiche d'une créature jouable ou non : héros, PNJ, ennemi.
 *
 * **Un objet autonome, jamais un singleton joueur.** La décision de cadrage est « un héros au
 * départ, quatre à terme » : le passage au groupe (`LOT-29`) ne doit rien changer à ce type. Aucune
 * fonction de ce fichier ne prend « le personnage » implicitement — toutes reçoivent la fiche sur
 * laquelle elles travaillent, et quatre fiches vivent côte à côte sans se connaître.
 *
 * La fiche porte des **valeurs**, pas des règles : le bonus de maîtrise ne s'y trouve pas, il se
 * lit dans la table d'expérience au niveau courant. Le stocker le figerait au moment de la
 * construction, et une montée de niveau laisserait un personnage avec le bonus de l'ancien.
 *
 * **Une fiche se construit, elle ne se déclare pas.** Tous les champs chiffrés partent de zéro, et
 * aucun ne porte de valeur « raisonnable » par défaut : un `armorClass = 10` dans cette structure
 * serait la règle du livre écrite en C++ (`EX-VIS-007`), et une fiche à demi construite passerait
 * pour une fiche jouable. `buildCharacterSheet()` est la porte d'entrée, et elle prend les
 * constantes dans la donnée.
 */
struct CharacterSheet {
    std::string name;
    /// Identifiants des trois choix qui ont construit la fiche. Vides pour une créature du
    /// bestiaire, qui n'en a aucun.
    std::string speciesId;
    std::string classId;
    std::string backgroundId;

    /// Les six valeurs **finales**, augmentations d'espèce comprises, indexées par `core::Ability`.
    std::array<int, 6> abilities{};

    int level = 1;
    int experiencePoints = 0;
    int maximumHitPoints = 0;
    int currentHitPoints = 0;
    int armorClass = 0;
    /// Vitesse de base, en **mètres** — l'unité du corpus. `speedInTiles()` la convertit.
    float speedMeters = 0.0F;

    /// Compétences maîtrisées, par identifiant du catalogue.
    std::set<std::string> skillProficiencies;
    /// Jets de sauvegarde maîtrisés.
    std::set<Ability> savingThrowProficiencies;
    /**
     * @brief Langues parlées, par identifiant du catalogue (`EX-RPG-042`, `LOT-15`).
     *
     * Celles de l'espèce, recopiées à la construction, plus celles que la fiche **choisit** — un
     * historique en accorde un nombre, pas une liste. Elles ne sont pas décoratives : un dialogue
     * se refuse faute de langue commune (`core::DialogueRunner`).
     */
    std::set<std::string> languages;

    /// @brief La valeur d'une caractéristique.
    [[nodiscard]] int ability(Ability which) const {
        return abilities[static_cast<std::size_t>(which)];
    }
    /// @brief Le modificateur d'une caractéristique.
    [[nodiscard]] int modifier(Ability which) const {
        return abilityModifier(ability(which));
    }
    /// @brief La vitesse en **cases** de la grille tactique (`LOT-19`, `LOT-22`).
    [[nodiscard]] float speedInTiles() const;
};

/**
 * @brief Ce qu'une montée de niveau a produit — de quoi le dire au joueur (`EX-REG-003`).
 *
 * Le détail, pas seulement le total : un joueur qui gagne deux niveaux d'un coup doit pouvoir
 * reconstituer ce qui vient d'arriver, et un test doit pouvoir vérifier chaque marche.
 */
struct LevelUpResult {
    int previousLevel = 0;
    int newLevel = 0;
    int hitPointsGained = 0;
    int previousProficiencyBonus = 0;
    int newProficiencyBonus = 0;

    /// @brief Vrai si un niveau au moins a été gagné.
    [[nodiscard]] bool gainedLevel() const {
        return newLevel > previousLevel;
    }
};

/**
 * @brief Une fiche chargée depuis un fichier, et ce qui n'a pas pu l'être.
 *
 * Les erreurs voyagent **avec** la donnée, jamais à sa place (`EX-CNT-010`) : une fiche dont un
 * champ est illisible reste affichable, et la refuser en bloc laisserait un écran vide sans dire
 * pourquoi.
 */
struct LoadedCharacterSheet {
    CharacterSheet sheet;
    /// Ce que le personnage PORTE (`LOT-14`), lu dans le même fichier. Vide si le fichier n'en
    /// déclare pas : un personnage sans inventaire est un personnage les mains vides, pas une
    /// erreur.
    Inventory inventory;
    std::vector<std::string> errors;

    /// @brief Vrai si la fiche s'est construite sans erreur.
    [[nodiscard]] bool ok() const {
        return errors.empty();
    }
};

/**
 * @brief Charge un personnage depuis son fichier JSON (`Source/Elements/Rpg/characters/`, schéma
 *        `character.schema.json`) et **construit** sa fiche.
 *
 * Le fichier ne porte que des **choix** — espèce, classe, historique, valeurs de caractéristique
 * avant augmentation, niveau visé. Tout le reste est dérivé ici par `buildCharacterSheet` et par
 * la montée en expérience : points de vie, classe d'armure, valeurs finales, bonus de maîtrise.
 * Écrire ces valeurs dans le fichier en ferait une **seconde source**, qui différerait de la
 * première au premier ajustement de règle — et personne ne saurait laquelle croit.
 *
 * Le niveau s'atteint par **gain d'expérience**, le même chemin qu'une partie empruntera : donc
 * les mêmes points de vie qu'un personnage monté en jouant, et non une variante propre au
 * chargement.
 *
 * Ne lève jamais (`EX-NFR-040`) : un fichier absent, mal formé ou référençant un identifiant
 * inconnu rend une fiche partielle **et** une erreur nommée (`EX-CNT-010`).
 */
[[nodiscard]] LoadedCharacterSheet loadCharacterSheet(const std::filesystem::path& path,
                                                      const CharacterOptions& options,
                                                      const CharacterCreationRules& rules,
                                                      const ExperienceTable& table);

/// @brief Le bonus de maîtrise de la fiche, **lu dans la table** au niveau courant.
[[nodiscard]] int proficiencyBonus(const CharacterSheet& sheet, const ExperienceTable& table);

/**
 * @brief Le modificateur d'un jet de sauvegarde : caractéristique, plus la maîtrise si elle
 *        s'applique.
 */
[[nodiscard]] int savingThrowModifier(const CharacterSheet& sheet, const ExperienceTable& table,
                                      Ability which);

/**
 * @brief Le modificateur d'un jet de compétence.
 *
 * La caractéristique vient du **catalogue**, pas d'un `switch` : c'est lui qui décide que la
 * Discrétion se jette en Dextérité. Une compétence inconnue du catalogue renvoie le modificateur
 * nu et **le signale** par `found`, plutôt que de laisser croire à une maîtrise absente.
 */
struct SkillCheckModifier {
    int value = 0;
    bool found = false;
    bool proficient = false;
};
/**
 * @brief Le modificateur de la compétence @p skillId pour @p sheet : caractéristique du catalogue,
 *        plus le bonus de maîtrise de @p table si la fiche maîtrise la compétence.
 */
[[nodiscard]] SkillCheckModifier skillModifier(const CharacterSheet& sheet,
                                               const ExperienceTable& table,
                                               const SkillCatalog& catalog,
                                               std::string_view skillId);

/**
 * @brief Les points de vie d'un niveau, selon la règle du livre.
 *
 * *« Vous pouvez aussi choisir d'utiliser la valeur fixe indiquée dans la description de votre
 * classe, qui se trouve être la valeur moyenne (arrondie au supérieur) du dé »* — *Basic Rules*
 * p. 11. C'est la voie **déterministe**, et c'est celle que le moteur emploie : des points de vie
 * tirés au dé rendraient une partie irrejouable, ce qu'`EX-NFR-002` interdit.
 *
 * Le niveau 1 reçoit le **maximum** du dé ; chaque niveau suivant sa moyenne arrondie au
 * supérieur. Le modificateur de Constitution s'ajoute **à chaque niveau**, et le gain d'un niveau
 * vaut au minimum 1 : un personnage à la Constitution désastreuse gagne peu de points de vie, il
 * n'en perd pas.
 *
 * @param hitDie Le dé de vie de la classe, **lu dans sa donnée** (`hitDie` du catalogue).
 * @param level Le niveau atteint, à partir de 1.
 * @param constitutionModifier Le modificateur de Constitution.
 */
[[nodiscard]] int maximumHitPointsFor(int hitDie, int level, int constitutionModifier);

/**
 * @brief Ajoute des points d'expérience et applique la montée de niveau qui en découle.
 *
 * **Reproductible** : aucune part de hasard, et le résultat ne dépend que de la fiche, de la table
 * et du montant. Le franchissement de plusieurs seuils d'un coup est traité comme le livre le
 * décrit — le personnage atteint le niveau que son total lui donne, pas le suivant.
 *
 * @param sheet La fiche, modifiée sur place.
 * @param table La table d'expérience.
 * @param hitDie Le dé de vie de la classe, pour recalculer les points de vie.
 * @param amount Les points gagnés. Un montant négatif est ignoré : perdre de l'expérience n'est
 *        pas une règle de ce jeu, et l'accepter en silence ferait descendre un personnage de
 *        niveau sans que rien ne l'annonce.
 * @return Ce que la montée a produit, niveau par niveau.
 */
LevelUpResult gainExperience(CharacterSheet& sheet, const ExperienceTable& table, int hitDie,
                             int amount);

/**
 * @brief Construit une fiche depuis une espèce, une classe et un historique.
 *
 * Les trois catalogues du `LOT-36` deviennent ici une fiche jouable : les augmentations de
 * l'espèce s'appliquent aux valeurs de base, la classe donne le dé de vie et les jets de
 * sauvegarde, l'historique les compétences maîtrisées.
 *
 * @param name Le nom du personnage, tel qu'il s'affichera.
 * @param baseAbilities Les six valeurs **avant** augmentation d'espèce, dans l'ordre de
 *        `core::Ability`.
 * @param species Peut être `nullptr` — une créature sans espèce est légitime.
 * @param playableClass Peut être `nullptr`, auquel cause la fiche n'a ni dé de vie ni sauvegardes.
 * @param background Peut être `nullptr`.
 * @param rules Les constantes de création, lues dans la donnée.
 * @param table La table d'expérience, pour le seuil du niveau de départ.
 */
[[nodiscard]] CharacterSheet buildCharacterSheet(
    std::string name, const std::array<int, 6>& baseAbilities, const Species* species,
    const PlayableClass* playableClass, const Background* background,
    const CharacterCreationRules& rules, const ExperienceTable& table);

}  // namespace core
