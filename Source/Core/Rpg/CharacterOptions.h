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
#include "Core/Rpg/RpgEnums.h"

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
    /// Sous-espèce : l'identifiant de l'espèce dont celle-ci dérive. Vide sinon.
    std::string parentSpecies;
    std::vector<std::string> languages;
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
    std::vector<std::string> features;
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
    std::vector<ClassLevel> progression;
    ProvisionalStatus status;

    /// @brief La ligne de progression d'un niveau, ou `nullptr` si la table ne le porte pas.
    [[nodiscard]] const ClassLevel* atLevel(int level) const;
};

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

}  // namespace core
