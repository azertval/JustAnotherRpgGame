// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Rpg/CharacterOptions.h
 * @brief Espèces, historiques et classes : de quoi construire un personnage (`LOT-36`).
 */

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Rpg/Ability.h"
#include "Core/Rpg/ClassCapacities.h"
#include "Core/Rpg/RpgEnums.h"
#include "Core/Rpg/Spell.h"

namespace core {

/// @brief Un trait nommé, tel que le livre l'écrit — d'espèce ou d'historique.
struct NamedTrait {
    std::string name;
    std::string text;
};

/**
 * @brief Le **statut provisoire** d'une donnée (`EX-CNT-032`).
 *
 * Une donnée provisoire non marquée devient permanente par accident — c'est la façon la plus
 * banale dont un échafaudage finit en mur porteur. `removalCriterion` écrit **d'avance** ce qui la
 * fera disparaître ; sans lui, le champ ne serait qu'un aveu sans suite.
 */
struct ProvisionalStatus {
    bool provisional = false;
    std::string reason;
    std::string removalCriterion;
};

/**
 * @brief Une espèce jouable : ce que `species.schema.json` décrit.
 *
 * Les augmentations de caractéristique sont une **table**, jamais une phrase : c'est la seule
 * forme que le moteur puisse appliquer, et `abilityScoreIncrease` est indexée par `core::Ability`.
 * Une case à zéro signifie « pas d'augmentation », ce qui est la valeur juste — le livre n'accorde
 * jamais un bonus nul, et rien ne distingue ici l'absence du zéro parce que rien ne les distingue
 * dans la règle.
 */
struct Species {
    std::string id;
    std::string name;
    std::string source;
    CreatureSize size = CreatureSize::Medium;
    /// Vitesse de base, en **mètres**. Les livres de Tanares comptent en pieds ; la conversion est
    /// faite à l'extraction, pour qu'un seul système d'unités arrive jusqu'ici.
    float speed = 0.0F;
    std::array<int, 6> abilityScoreIncrease{};
    /**
     * @brief Points de vie que l'espèce ajoute **à chaque niveau** (`LOT-130`).
     *
     * La *Ténacité naine* du nain des collines — *« Votre maximum de points de vie augmente de 1
     * à chaque niveau »* — est la seule du corpus, et elle est ici sous la forme d'un **nombre**,
     * comme les augmentations de caractéristique sont une table : le moteur ne lit pas la prose
     * des traits, et une fiche de prêtre nain qui n'en tiendrait pas compte afficherait 11 points
     * de vie pour les 12 du livre, sans que rien ne le signale. Zéro pour toute autre espèce.
     */
    int hitPointsPerLevel = 0;
    /// Sous-espèce : l'identifiant de l'espèce dont celle-ci dérive. Vide sinon. Au chargement,
    /// la sous-espèce **hérite** de son parent (`LOT-130`) : augmentations additionnées, points de
    /// vie par niveau additionnés, langues, traits et mécanismes requis réunis — le fichier ne
    /// porte que ce que la sous-espèce ajoute, comme le livre l'écrit.
    std::string parentSpecies;
    std::vector<std::string> languages;
    /// Armes que l'espèce fait maîtriser (`LOT-131`), en identifiants d'armes du catalogue :
    /// l'*Entraînement aux armes naines*. Le moteur ne lit pas la prose des traits.
    std::vector<std::string> weaponProficiencies;
    std::vector<NamedTrait> traits;
    std::vector<std::string> requiredMechanisms;

    /// @brief L'augmentation qu'accorde cette espèce pour une caractéristique.
    [[nodiscard]] int increase(Ability which) const {
        return abilityScoreIncrease[static_cast<std::size_t>(which)];
    }
};

/// @brief Un historique : maîtrises, langues accordées et capacité.
struct Background {
    std::string id;
    std::string name;
    std::string source;
    /// Compétences maîtrisées, par identifiant du catalogue du `LOT-43`.
    std::vector<std::string> skillProficiencies;
    /// Nombre de langues **au choix** du joueur. `0` si l'historique n'en accorde aucune.
    int languageCount = 0;
    std::optional<NamedTrait> feature;
    std::string text;
};

/// @brief Une ligne de table de progression : ce qu'un niveau apporte.
struct ClassLevel {
    int level = 0;
    int proficiencyBonus = 0;
    /// Les capacités que ce niveau apporte : des identifiants du catalogue des capacités
    /// (`core::CapacityCatalog`, `LOT-131`).
    std::vector<std::string> features;
    /// Les sorts mineurs appris à ce niveau (identifiants de `spells/`), à volonté.
    std::vector<std::string> cantrips;
    /// Les sorts appris à ce niveau, chacun lançable `Spellcasting::castsPerDay` fois par jour.
    std::vector<std::string> spells;
};

/// @brief Les compétences de classe : un nombre au choix parmi une liste, comme le livre l'écrit.
struct SkillChoices {
    int count = 0;
    std::vector<std::string> from;
};

/**
 * @brief L'**incantation simplifiée** (*Player's Guide*, p. 196 et 200 ; `EX-RPG-025`).
 *
 * Les sorts sont fixés par la table de progression, et chacun se lance `castsPerDay` fois par
 * jour — pas d'emplacements. DD = 8 + maîtrise + modificateur ; attaque = maîtrise + modificateur.
 */
struct Spellcasting {
    Ability ability = Ability::Intelligence;
    int castsPerDay = 0;
};

/**
 * @brief Une classe jouable et sa table de progression.
 *
 * **La table de progression est une donnée, jamais une règle en C++** : le bonus de maîtrise se
 * lit ligne à ligne dans `progression`, et aucune formule du moteur ne le recalcule. Une classe
 * dont la progression sortirait de l'ordinaire — et le `LOT-84` en annonce trente et une — n'aurait
 * alors rien à changer dans le code.
 */
struct PlayableClass {
    std::string id;
    std::string name;
    std::string source;
    int hitDie = 0;
    std::vector<Ability> primaryAbility;
    std::vector<Ability> savingThrowProficiencies;
    /// Catégories d'armure maîtrisées (`light`, `medium`, `heavy`, `shields`), `LOT-131`.
    std::vector<std::string> armorProficiencies;
    /// Armes maîtrisées : une catégorie (`simple`, `martial`) ou l'identifiant d'une arme.
    std::vector<std::string> weaponProficiencies;
    SkillChoices skillChoices;
    /// Absente pour une classe qui ne lance pas de sorts.
    std::optional<Spellcasting> spellcasting;
    std::vector<ClassLevel> progression;
    ProvisionalStatus status;

    /// @brief La ligne de progression d'un niveau, ou `nullptr` si la table ne le porte pas.
    [[nodiscard]] const ClassLevel* atLevel(int level) const;

    /**
     * @brief Vrai si la classe maîtrise cette arme : par sa catégorie (`simple`, `martial`) ou par
     *        son identifiant.
     */
    [[nodiscard]] bool isProficientWithWeapon(std::string_view weaponId,
                                              std::string_view category) const;

    /// @brief Vrai si la classe maîtrise cette catégorie d'armure (`light`… ou `shields`).
    [[nodiscard]] bool isProficientWithArmor(std::string_view category) const;

    /// @brief Les sorts mineurs connus au niveau @p level : l'union des lignes jusqu'à ce niveau.
    [[nodiscard]] std::vector<std::string> cantripsAt(int level) const;
    /// @brief Les sorts connus au niveau @p level : l'union des lignes jusqu'à ce niveau.
    [[nodiscard]] std::vector<std::string> spellsAt(int level) const;
};

/**
 * @brief Les capacités **actives** d'une classe à un niveau : celles des lignes jusqu'à ce niveau,
 *        moins celles qu'une capacité acquise depuis remplace (`Capacity::replaces`).
 *
 * @param playableClass La classe, pour sa table de progression.
 * @param level Le niveau atteint : les lignes au-delà ne comptent pas.
 * @param catalog Le catalogue des capacités.
 * @param missing Reçoit l'identifiant de chaque capacité que la table nomme et que le catalogue
 *        ignore : une capacité absente ne joue pas, et le dire vaut mieux que de la laisser
 *        passer pour jouée (`EX-CNT-031`).
 * @return Des **copies** : la fiche les garde sans dépendre de la vie du catalogue.
 */
[[nodiscard]] std::vector<Capacity> resolveCapacities(const PlayableClass& playableClass, int level,
                                                      const CapacityCatalog& catalog,
                                                      std::vector<std::string>& missing);

/**
 * @brief Les trois catalogues chargés, et ce qui n'a pas pu l'être.
 *
 * Les erreurs voyagent **avec** les données, jamais à leur place : un catalogue dont une entrée
 * est illisible reste utilisable, et le refuser en bloc rendrait le jeu injouable pour une
 * virgule. Chaque échec nomme son fichier (`EX-CNT-010`).
 */
struct CharacterOptions {
    std::vector<Species> species;
    std::vector<Background> backgrounds;
    std::vector<PlayableClass> classes;
    /// Les capacités que les tables de progression désignent (`LOT-131`). Vide si le chargement
    /// ne les a pas demandées.
    CapacityCatalog capacities;
    /// Les sorts que les tables de progression désignent. Vide si le chargement ne les a pas
    /// demandés.
    SpellCatalog spells;
    std::vector<std::string> errors;

    /// @brief L'espèce d'identifiant @p id, ou `nullptr` si elle est inconnue.
    [[nodiscard]] const Species* findSpecies(std::string_view id) const;
    /// @brief L'historique d'identifiant @p id, ou `nullptr` s'il est inconnu.
    [[nodiscard]] const Background* findBackground(std::string_view id) const;
    /// @brief La classe d'identifiant @p id, ou `nullptr` si elle est inconnue.
    [[nodiscard]] const PlayableClass* findClass(std::string_view id) const;

    /**
     * @brief Les mécanismes que ces données exigent et que personne n'a encore honorés
     *        (`EX-CNT-031`).
     *
     * Le moteur les **liste au chargement** plutôt que de jouer en silence une augmentation de
     * caractéristique qu'il ne sait pas laisser choisir au joueur. C'est la différence entre une
     * espèce qu'on sait incomplète et une espèce qu'on croit jouable.
     */
    [[nodiscard]] std::vector<std::string> requiredMechanisms() const;

    /// @brief Les identifiants des classes marquées **provisoires** (`EX-CNT-032`).
    [[nodiscard]] std::vector<std::string> provisionalClassIds() const;
};

/**
 * @brief Applique les augmentations d'une espèce à une valeur de caractéristique.
 *
 * La seule opération que le moteur ait à faire sur une espèce, et elle est ici pour ne pas être
 * réécrite à chaque écran qui affiche une fiche.
 *
 * `maximumScore` est **un paramètre et non une constante** (`EX-VIS-007`) : le plafond vient de
 * `rules/character-creation.json`, où il est extrait de la phrase qui l'atteste — *« Vous ne
 * pouvez pas augmenter une valeur de caractéristique au-delà de 20 »*, *Basic Rules* p. 11. Écrit
 * ici, il ferait d'un ajustement d'équilibrage une recompilation.
 */
[[nodiscard]] int abilityScoreWith(const Species& species, Ability which, int baseScore,
                                   int maximumScore);

/**
 * @brief Charge les trois catalogues depuis leurs dossiers.
 *
 * Chaque dossier est **balayé**, jamais énuméré dans le code : une liste de noms écrite en C++
 * serait une seconde source de vérité, et la première espèce ajoutée par un lot suivant en
 * sortirait invisible.
 *
 * @param speciesDir Dossier des espèces (`Source/Elements/Rpg/species`).
 * @param backgroundsDir Dossier des historiques.
 * @param classesDir Dossier des classes.
 * @return Les catalogues et la liste des échecs. Ne lève jamais (`EX-NFR-040`). Un dossier absent
 *         produit une erreur, pas un catalogue vide : les deux se ressemblent à l'exécution, et
 *         les confondre fait chercher le défaut du mauvais côté.
 */
[[nodiscard]] CharacterOptions loadCharacterOptions(const std::filesystem::path& speciesDir,
                                                    const std::filesystem::path& backgroundsDir,
                                                    const std::filesystem::path& classesDir);

/**
 * @brief Les trois catalogues, **plus** les capacités et les sorts que les tables de progression
 *        désignent (`LOT-131`).
 *
 * Les deux dossiers de plus suivent la même règle : absents, ils produisent une erreur et non un
 * catalogue vide. Les capacités que les classes nomment et que le catalogue ignore ne sont pas
 * une erreur de chargement — la table peut annoncer un lot à venir — mais `resolveCapacities` les
 * rapporte à qui construit une fiche.
 */
[[nodiscard]] CharacterOptions loadCharacterOptions(const std::filesystem::path& speciesDir,
                                                    const std::filesystem::path& backgroundsDir,
                                                    const std::filesystem::path& classesDir,
                                                    const std::filesystem::path& capacitiesDir,
                                                    const std::filesystem::path& spellsDir);

/**
 * @brief Tous les catalogues d'options depuis la racine `Rpg/` d'une donnée de jeu : `species/`,
 *        `backgrounds/`, `classes/`, `capacities/` et `spells/`.
 */
[[nodiscard]] CharacterOptions loadCharacterOptions(const std::filesystem::path& rpgRoot);

}  // namespace core
