// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/EnemyAi.h
 * @brief L'IA tactique : des heuristiques pondérées, déterministes, qui jouent dans les mêmes
 *        actions que le joueur (`LOT-23`, `EX-CBT-050`).
 *
 * ## Ce que le *Guide du Maître* en dit
 *
 * Le chapitre 8, « Le combat » (PDF p. 247-255), ne donne pas de tactique aux monstres : il dit au
 * MD **ce qu'il sait** et **comment il compte**. Ce fichier en tire ses règles, et nomme ce qu'il
 * décide au-delà.
 *
 * - **Les points de vie des monstres se suivent en secret** ; mais « si le monstre a moins de la
 *   moitié de ses points de vie », il est **ensanglanté**, et la table le voit. L'IA lit la même
 *   chose chez ses adversaires : l'état ensanglanté (`core::isBloodied`), jamais le nombre de
 * points de vie restants — c'est ce qu'`EX-CBT-050` appelle les mêmes informations.
 * - **Gérer les foules** : le résultat minimal au d20 pour toucher est « la CA de la cible moins le
 *   bonus d'attaque » (`core::requiredRoll`). L'IA en tire la chance de toucher
 *   (`core::hitChance`), et l'espérance de dégâts : les **dégâts moyens** du bloc, et au critique
 *   les dés « ajoutés aux dégâts moyens » (« Les monstres et les coups critiques »).
 * - **Le champ de vision et l'abri** se mesurent par la méthode du Guide, déjà celle du `LOT-22` :
 *   l'IA vise par `core::hasLineOfSight` et compte l'abri par `core::coverFrom`, comme le jet réel.
 * - **La prise en tenaille**, règle optionnelle, donne l'avantage au corps à corps
 *   (`core::isFlankedFrom`) : une IA dans une arène qui la joue cherche la case d'en face.
 * - **Le temps de réaction** : l'attaque d'opportunité interrompt son déclencheur ; la décision de
 *   la prendre est ici (`core::shouldTakeOpportunity`).
 *
 * ## Ce que ce fichier décide, et que le Guide ne dit pas
 *
 * Les **poids** : combien pèse une cible ensanglantée, une menace subie, une case à parcourir. Ce
 * sont des données (`Source/Elements/Rpg/rules/behaviors.json`, `EX-VIS-007`), pas des constantes.
 * Et deux garde-fous, qui sont des règles de l'IA et non des poids :
 *
 * - **Le suicide.** Une IA ne finit pas son tour au contact de plus d'ennemis que son profil n'en
 *   tolère (`toleratedThreats`) quand une case qui en tolère moins existait : la comparaison est
 *   **lexicographique**, l'excès d'abord, le score ensuite. Seules les attaques **de contact**
 *   comptent : un tireur couvre toute l'arène, et le compter interdirait d'approcher.
 * - **Le blocage.** Une IA qui peut attaquer attaque ; une IA qui ne le peut pas **avance** vers sa
 *   cible — c'est une clé de la comparaison, avant le score, sans quoi la menace d'un round entier
 *   la tiendrait hors de portée à jamais —, en se précipitant si le chemin est long. Le prudent
 *   choisit **d'où** frapper, et **recule** après, à menace égale le moins loin possible : il ne
 *   se cache pas indéfiniment.
 *
 * ## Déterministe, en entiers
 *
 * Aucun flottant : une chance de toucher est un nombre de quatre-centièmes (le carré d'un
 * vingtième, pour l'avantage), des dégâts moyens un nombre de demi-points, un poids un pourcentage.
 * Les cases se parcourent par indice croissant, les cibles par identifiant croissant, et une
 * égalité garde le premier candidat. Deux exécutions, deux compilateurs, deux modes de construction
 * donnent le même tour.
 */

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Combat/Arena.h"
#include "Core/Combat/Attack.h"
#include "Core/Rpg/Check.h"

namespace core {

struct Creature;

// --- Ce que la table sait ---------------------------------------------------------------------

/**
 * @brief Vrai si le combattant a **moins de la moitié** de ses points de vie : ensanglanté, ce que
 *        le *Guide du Maître* laisse voir à la table.
 */
[[nodiscard]] constexpr bool isBloodied(const CombatantProfile& profile) noexcept {
    return profile.currentHitPoints * 2 < profile.maximumHitPoints;
}

/**
 * @brief Le résultat minimal au d20 pour toucher : la CA moins le bonus d'attaque (*Guide du
 *        Maître*, « Gérer les foules »).
 */
[[nodiscard]] constexpr int requiredRoll(int armorClass, int attackBonus) noexcept {
    return armorClass - attackBonus;
}

/// @brief Le bonus d'un profil d'attaque : la somme de ses modificateurs.
[[nodiscard]] int attackBonusOf(const AttackProfile& profile) noexcept;

/// @brief Une chance, en **quatre-centièmes** : 400 est la certitude.
inline constexpr int CHANCE_SCALE = 400;

/**
 * @brief La chance de toucher, en quatre-centièmes, pour un résultat requis et une posture.
 *
 * Un 1 rate toujours, un 20 touche toujours : la chance d'un seul dé est entre 1 et 19 vingtièmes.
 * Avec l'avantage, on rate si les deux dés ratent ; avec le désavantage, on touche si les deux dés
 * touchent. Un résultat au moins égal au seuil critique touche aussi, quelle que soit la CA.
 */
[[nodiscard]] int hitChance(int required, RollStance stance, int criticalThreshold = 20) noexcept;

/// @brief La chance d'un critique, en quatre-centièmes, pour un seuil critique et une posture.
[[nodiscard]] int criticalChance(int criticalThreshold, RollStance stance) noexcept;

/**
 * @brief L'espérance de dégâts d'une attaque, en **huit-centièmes** de point de dégât.
 *
 * `chance × dégâts moyens + chance de critique × moyenne des dés`, les dégâts moyens en demi-points
 * pour rester entier. Les résistances de la cible n'y entrent pas : la table ne les connaît pas
 * avant de les avoir vues jouer (`EX-CBT-050`).
 */
[[nodiscard]] long long expectedDamage(const AttackProfile& profile, int armorClass,
                                       RollStance stance) noexcept;

// --- Les profils --------------------------------------------------------------------------------

/**
 * @brief Un profil de comportement : des poids, en pour cent, et deux règles.
 *
 * Tous les termes du score sont en points de dégâts attendus : ce qu'on inflige, ce qu'on subit, et
 * les cases à parcourir converties au taux `approachPerTile`. Un poids de 100 compte un point pour
 * un point.
 */
struct BehaviorProfile {
    std::string id;
    std::string name;
    /// Les dégâts qu'on espère infliger.
    int damageDealt = 100;
    /// Ce qui s'ajoute quand la cible est ensanglantée : l'achever.
    int bloodiedTarget = 0;
    /// Ce qui s'ajoute par allié déjà au contact de la cible : frapper ensemble.
    int focusFire = 0;
    /// Ce qui s'ajoute quand la cible est au contact d'un allié ensanglanté : le dégager.
    int protectBloodiedAlly = 0;
    /// Les dégâts qu'on s'attend à subir au prochain round, là où l'on finit.
    int threatTaken = 100;
    /// La même menace, quand on est soi-même ensanglanté.
    int threatWhenBloodied = 100;
    /// Les attaques d'opportunité qu'un chemin provoque.
    int opportunityTaken = 100;
    /// Le prix d'une case entre soi et la cible, en pour cent d'un point de dégât.
    int approachPerTile = 100;
    /// Combien d'ennemis peuvent frapper la case de fin de tour au contact, sans bouger, au plus.
    int toleratedThreats = 2;
    /// Le jet requis au-delà duquel on ne prend pas l'attaque d'opportunité (21 : toujours).
    int opportunityMaximumRoll = 21;
    /// Vrai si, faute de pouvoir attaquer et menacé, on esquive plutôt que de se précipiter.
    bool dodgeWhenThreatened = false;
    /// Vrai si, après avoir attaqué, on s'éloigne vers une case moins menacée.
    bool retreatAfterAttack = false;
};

/// @brief Une règle d'attribution : un trait, une créature, ou le goût du tir.
struct BehaviorAssignment {
    std::string behavior;
    /// Le nom d'un trait du bloc (« Tactique de groupe »).
    std::string trait;
    /// L'identifiant d'une créature.
    std::string creature;
    /// Vrai pour une créature qui frappe au moins aussi fort à distance qu'au contact.
    bool ranged = false;
};

/// @brief Les profils, les règles d'attribution dans l'ordre, et ce qui n'a pas pu être lu.
struct BehaviorCatalog {
    std::vector<BehaviorProfile> profiles;
    std::vector<BehaviorAssignment> assignments;
    std::string defaultBehavior;
    std::vector<std::string> errors;

    /// @brief Le profil de comportement d'identifiant @p id, ou `nullptr` s'il est inconnu.
    [[nodiscard]] const BehaviorProfile* find(std::string_view id) const;
};

/// @brief Charge les profils (`Source/Elements/Rpg/rules/behaviors.json`).
[[nodiscard]] BehaviorCatalog loadBehaviors(const std::filesystem::path& file);

/**
 * @brief Le profil d'une créature du bestiaire : la première règle qui la désigne, sinon le profil
 *        par défaut.
 */
[[nodiscard]] std::string behaviorFor(const Creature& creature, const BehaviorCatalog& catalog);

// --- Le tour ----------------------------------------------------------------------------------

/// @brief Ce que l'IA fait de son action.
enum class TurnAction : std::uint8_t {
    Attack,
    Dash,
    Dodge,
    Disengage,
    /// Rien : un tour sans action utile, qui se termine quand même.
    Wait,
};

/// @brief Un tour décidé : où aller, puis quoi faire.
struct TurnPlan {
    CombatantId actor{};
    /// L'ancre de fin de déplacement **avant** l'action, ou vide pour rester.
    std::optional<GridPosition> moveTo;
    TurnAction action = TurnAction::Wait;
    std::optional<CombatantId> target;
    std::size_t attackIndex = 0;
    /// Pour une approche : où aller **après** s'être précipité, le long du chemin.
    std::optional<GridPosition> dashTo;
    /// Le jet requis de l'attaque, et sa posture.
    int requiredRoll = 0;
    RollStance stance = RollStance::Normal;
    /// Les ennemis qui peuvent frapper la case de fin sans bouger.
    int immediateThreats = 0;
    long long score = 0;
    /// La ligne de journal qui dit la décision.
    std::string summary;
};

/**
 * @brief Décide le tour de @p actor, qui doit être le combattant actif de la session.
 *
 * Toutes les cases où finir le déplacement (`core::ReachableArea`), et la case de départ, sont
 * examinées avec chaque attaque et chaque ennemi debout ; si aucune attaque n'est possible, les
 * cases le long du chemin vers l'ennemi le plus proche (`core::findPath`). Rien n'est joué.
 */
[[nodiscard]] TurnPlan planTurn(const ArenaSession& session, CombatantId actor,
                                const BehaviorProfile& profile);

/**
 * @brief Joue le tour du combattant actif selon son profil, et le termine.
 *
 * Le plan passe par les actions de la session, **les mêmes que celles du joueur** : `move`,
 * `attack`, `dash`, `dodge`, `disengage`, `endTurn`. La décision s'écrit au journal avant d'être
 * jouée. Une attaque d'opportunité qui abat l'IA en chemin met fin à son tour.
 *
 * @return Faux si le combattant actif n'a pas de profil connu ou si aucun tour n'est en cours.
 */
bool playTurn(ArenaSession& session, const BehaviorCatalog& catalog);

/**
 * @brief Vrai si @p reactor, joué par @p profile, prend l'attaque d'opportunité sur @p mover : sa
 *        première attaque au contact, à un jet requis au plus égal à `opportunityMaximumRoll`.
 */
[[nodiscard]] bool shouldTakeOpportunity(const ArenaSession& session, CombatantId reactor,
                                         CombatantId mover, const BehaviorProfile& profile);

/**
 * @brief La politique d'opportunité d'une session où l'IA joue certains combattants : chacun d'eux
 *        décide par son profil, les autres prennent toutes leurs attaques.
 *
 * Le catalogue doit vivre aussi longtemps que la session.
 */
[[nodiscard]] OpportunityPolicy aiOpportunityPolicy(const BehaviorCatalog& catalog);

}  // namespace core
