// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Rpg/Bestiary.h
 * @brief Chargement du catalogue de créatures (`LOT-33`, `EX-CNT-010`, `EX-CNT-031`).
 */

#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Rpg/Ability.h"
#include "Core/Rpg/Dice.h"
#include "Core/Rpg/RpgEnums.h"

namespace core {

/// @brief Un trait de créature : un nom et sa description, tels que le livre les écrit.
struct CreatureTrait {
    std::string name;
    std::string text;
};

/**
 * @brief Une action de créature, et ce que le moteur sait en jouer.
 *
 * Le texte est **toujours** présent ; les champs typés ne le sont pas. Une action qui n'inflige
 * aucun dégât — l'invisibilité du diablotin, la toile de l'araignée géante — n'a ni bonus ni dés,
 * et ce n'est pas une extraction incomplète : c'est la donnée. D'où les `std::optional`, qui
 * distinguent « pas de dégâts » de « zéro dégât ».
 */
struct CreatureAction {
    std::string name;
    std::string text;
    /// Bonus au jet d'attaque. Absent si l'action n'est pas une attaque.
    std::optional<int> attackBonus;
    /// Allonge en mètres. Absente pour une action à distance ou sans portée.
    std::optional<float> reach;
    /// Dés de dégâts de la **première** clause de l'action. Voir la note de `loadBestiary`.
    std::optional<Dice> damage;
    /// Type des dégâts ci-dessus.
    std::optional<DamageType> damageType;
    /// Portée normale d'une action à distance, en mètres (« portée 24/96 m » : 24). `LOT-22`.
    std::optional<float> rangeNormal;
    /// Longue portée, en mètres (96). Égale à la normale pour une portée unique.
    std::optional<float> rangeLong;
};

/// @brief Vitesses de déplacement d'une créature, en **mètres**. `walk` est toujours renseignée.
struct CreatureSpeed {
    float walk = 0.0F;
    std::optional<float> fly;
    std::optional<float> swim;
    std::optional<float> climb;
    std::optional<float> burrow;
};

/**
 * @brief Un bloc de statistiques : ce que `creature.schema.json` décrit.
 *
 * Les champs obligatoires du schéma sont des valeurs nues ; les facultatifs sont vides quand la
 * donnée les omet. Aucun n'a de valeur de repli inventée — une caractéristique laissée à 10 par
 * défaut masquerait une extraction incomplète, ce que le schéma refuse déjà en exigeant les six.
 */
struct Creature {
    std::string id;
    std::string name;
    std::string source;
    CreatureSize size = CreatureSize::Medium;
    /// Type de créature, en français et **ouvert** : le Sourcebook en introduira que le SRD ignore.
    std::string creatureType;
    std::string alignment;
    int armorClass = 0;
    int hitPoints = 0;
    /// Dés de vie — `4d10+4`. Absents si la donnée ne les porte pas.
    std::optional<Dice> hitDice;
    CreatureSpeed speed;
    /// Les six caractéristiques, indexées par `core::Ability`, dans l'ordre de la fiche.
    std::array<int, 6> abilities{};
    /// Facteur de puissance. Fractionnaire sous 1 — 1/8, 1/4, 1/2 — d'où le nombre flottant.
    float challengeRating = 0.0F;
    /// Bonus de compétence, par identifiant du catalogue (`stealth`, `perception`).
    std::map<std::string, int> skills;
    /// Sens, en français : « vision dans le noir 18 m », « Perception passive 13 ».
    std::vector<std::string> senses;
    /// Langues, par identifiant du catalogue des seize langues.
    std::vector<std::string> languages;
    std::vector<DamageType> damageResistances;
    std::vector<DamageType> damageImmunities;
    std::vector<DamageType> damageVulnerabilities;
    std::vector<Condition> conditionImmunities;
    std::vector<CreatureTrait> traits;
    std::vector<CreatureAction> actions;
    /// Le paragraphe d'ambiance du livre. Vide pour les créatures qui n'en ont pas.
    std::string description;
    /// Mécanismes que cette créature exige du moteur (`EX-CNT-030`).
    std::vector<std::string> requiredMechanisms;
    /// La silhouette de son mannequin de remplacement (`LOT-316`) : `humanoid`, `quadruped`,
    /// `flying`. Vide : humanoïde. L'extraction ne la devine pas ; elle se pose à la main.
    std::string silhouette;

    /// @brief La valeur d'une caractéristique.
    [[nodiscard]] int ability(Ability which) const {
        return abilities[static_cast<std::size_t>(which)];
    }

    /// @brief L'action portant ce nom, ou `nullptr`.
    [[nodiscard]] const CreatureAction* action(std::string_view actionName) const;
};

/**
 * @brief Le catalogue chargé, et ce qui n'a pas pu l'être.
 *
 * Les deux **ensemble**, jamais l'un ou l'autre : un catalogue dont une entrée sur quatre-vingt-
 * quatorze est illisible reste utilisable, et le refuser en bloc rendrait le jeu injouable pour
 * une virgule. Chaque échec nomme son fichier (`EX-CNT-010`).
 */
struct Bestiary {
    /// Les créatures chargées, triées par identifiant.
    std::vector<Creature> creatures;
    /// Un message par fichier refusé, nommant le fichier et la raison.
    std::vector<std::string> errors;
    /**
     * @brief Les mécanismes que les créatures chargées exigent et que personne n'a encore honorés
     *        (`EX-CNT-031`).
     *
     * L'union des `mecanismesRequis`, dédupliquée. Le moteur la liste au chargement plutôt que de
     * jouer en silence une résistance conditionnelle qu'il ne sait pas appliquer.
     */
    [[nodiscard]] std::vector<std::string> requiredMechanisms() const;

    /// @brief La créature portant cet identifiant, ou `nullptr`.
    [[nodiscard]] const Creature* find(std::string_view id) const;
};

/**
 * @brief Charge toutes les créatures d'un dossier — un fichier JSON par créature.
 *
 * Le dossier est balayé, jamais énuméré dans le code : une liste de quatre-vingt-quatorze noms
 * écrite en C++ serait une seconde source de vérité, et la première créature ajoutée par le
 * `LOT-46` en sortirait invisible.
 *
 * @note **Une seule clause de dégâts est typée par action.** Une morsure qui inflige « 7 (1d10 +
 *       2) dégâts perforants + 5 (1d10) dégâts de poison » en porte deux ; `CreatureAction::damage`
 *       ne rend que la première, et `text` reste ce qui fait foi jusqu'à ce que le `LOT-21` sache
 *       composer plusieurs clauses. Le champ n'est pas *faux*, il est **partiel**, et c'est écrit
 *       ici parce que rien dans la donnée ne le dit.
 *
 * @param directory Dossier des créatures (`Source/Elements/Rpg/creatures`).
 * @return Le catalogue et la liste des échecs. Ne lève jamais (`EX-NFR-040`). Un dossier absent
 *         produit une erreur, pas un catalogue vide : un bestiaire vide se confondrait avec un
 *         bestiaire non installé.
 */
[[nodiscard]] Bestiary loadBestiary(const std::filesystem::path& directory);

}  // namespace core
