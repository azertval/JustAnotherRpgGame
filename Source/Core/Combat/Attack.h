// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/Attack.h
 * @brief L'attaque : un jet de d20 contre la classe d'armure, amendable avant d'être figé, puis
 *        les dégâts (`LOT-21`, `EX-CBT-030`, `EX-CBT-031`, `EX-REG-003`).
 *
 * ## Ce que dit le Manuel, et où chaque règle vit
 *
 * Manuel des Joueurs, chapitre 9, « Effectuer une attaque » :
 *
 * - **choisir une cible** à distance d'attaque — l'allonge au corps à corps (`core::inReach`) ;
 * - **déterminer les modificateurs** — avantage, désavantage, bonus : `core::AttackRoll`, et les
 *   circonstances que la grille sait déjà dire (`core::attackCircumstances`) ;
 * - **résoudre** — le d20, puis les dés de dégâts si l'attaque touche (`core::resolveAttack`).
 *
 * « Faire 1 ou 20 » : un 20 au d20 **touche automatiquement**, quels que soient les modificateurs
 * et la CA, et c'est un coup critique ; un 1 **rate automatiquement**. Le critique double **les
 * dés** de dégâts, pas les modificateurs (`core::rollDamage`, `EX-CBT-031`).
 *
 * ## Un jet qui est un objet
 *
 * Les livres de Tanares lisent le d20 **brut** (*Omen*, *Augurs*), le relancent **avant** la
 * résolution, ajoutent un modificateur **après** avoir vu le résultat (*Future Guard*), substituent
 * un résultat stocké. Rien de cela ne s'écrit si le jet est un entier rendu par une fonction : le
 * jet est donc un `core::AttackRoll` que trois points d'insertion reçoivent, et l'issue n'est figée
 * qu'après le dernier. Chaque amendement s'inscrit dans le jet, pour que le journal le dise.
 *
 * ## Hors de ce fichier, nommément
 *
 * La ligne de vue et l'abri se calculent dans `Core/Combat/LineOfSight.h` (`LOT-22`) et se lisent
 * ici : `core::checkTarget` refuse une cible sous abri total, `core::resolveAttack` pose l'abri sur
 * le jet. L'inconscience, les jets contre la mort, la mort instantanée et le coup qui assomme sont
 * au `LOT-72`, qui lit l'excédent et le critique que le `LOT-21` rapporte ; les sorts et les
 * attaques de classe arrivent avec les classes.
 */

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "Core/Combat/CombatState.h"
#include "Core/Combat/Damage.h"
#include "Core/Combat/LineOfSight.h"
#include "Core/Math/DeterministicRandom.h"
#include "Core/Rpg/Check.h"

namespace core {

struct CharacterSheet;
struct Creature;
struct Weapon;

/// @brief Au corps à corps, ou à distance.
enum class AttackKind : std::uint8_t {
    Melee,
    Ranged,
};

/**
 * @brief Les deux portées d'une attaque à distance, en cases.
 *
 * Manuel, « Portée » : au-delà de la portée normale, le jet est désavantagé ; au-delà de la longue
 * portée, l'attaque est impossible. Une portée unique — celle d'un sort — s'écrit avec deux
 * nombres égaux.
 */
struct AttackRange {
    int normal = 0;
    int maximum = 0;
};

/**
 * @brief Ce qu'un combattant sait frapper : une arme, une morsure, un coup à mains nues.
 *
 * Les modificateurs du jet portent **leur origine** (`EX-REG-003`) : « Force +3 », « maîtrise +2 »,
 * ou le bonus d'un bloc de bestiaire tel que le livre l'imprime. Les dégâts sont une liste de
 * clauses typées : une morsure venimeuse en a deux.
 */
struct AttackProfile {
    /// Le nom tel que le journal l'écrit : « Épée longue », « Morsure », « coup à mains nues ».
    std::string label;
    AttackKind kind = AttackKind::Melee;
    std::vector<Modifier> modifiers;
    std::vector<DamageClause> damage;
    /// Allonge au corps à corps, en cases — 1 pour 1,50 m, la plupart des créatures.
    int reach = 1;
    /**
     * @brief Les portées d'une attaque à distance, si la donnée les porte.
     *
     * Les armes du catalogue et les actions du bestiaire les portent en mètres (`rangeNormal`,
     * `rangeLong`) depuis le `LOT-22`, converties en cases **arrondies vers le bas** : une portée
     * ne dépasse jamais ce que le texte promet. Une attaque à distance **sans** portée connue ne
     * vise qu'au contact — avec le désavantage que le Manuel impose au tir au contact d'un ennemi.
     */
    std::optional<AttackRange> range;
    /// Plus petit résultat du d20 qui fait un critique. 20 par défaut ; un Champion le baisse.
    int criticalThreshold = 20;
};

/// @brief Les attaques d'une créature, et les actions qu'on n'a pas su en tirer.
struct CreatureAttacks {
    std::vector<AttackProfile> attacks;
    /// Une action à bonus d'attaque dont les dégâts ne sont pas typés : jamais un type par défaut
    /// (`EX-CBT-032`), le nom de l'action et la raison.
    std::vector<std::string> refused;
};

/**
 * @brief Les attaques d'un bloc de bestiaire : chaque action qui a un bonus d'attaque et des
 * dégâts.
 *
 * Une action **avec** allonge est au corps à corps (allonge en cases, arrondie, au moins 1) ; une
 * action **sans** allonge est à distance, à sa portée si le bloc l'écrit (« portée 24/96 m ») ; une
 * action qui porte les deux donne deux attaques, au contact puis à distance. Le bonus du bloc est
 * pris tel quel : le livre l'a déjà
 * calculé. Une action qui a un bonus mais aucun dégât (la toile de l'araignée géante) n'est pas une
 * attaque de ce lot — elle entrave, et c'est l'affaire des conditions (`LOT-72`).
 *
 * @note Le bestiaire ne type que la **première** clause de dégâts (`core::loadBestiary`) : le
 *       moteur sait en porter plusieurs, la donnée n'en fournit qu'une.
 */
[[nodiscard]] CreatureAttacks attacksFor(const Creature& creature);

/**
 * @brief L'attaque d'un personnage avec une arme, ou à mains nues si @p weapon est nul.
 *
 * Manuel, chapitre 9, « Modificateurs du jet » : Force au corps à corps, Dextérité à distance, la
 * meilleure des deux pour une arme de finesse (`core::weaponAttackAbility`) ; le bonus de maîtrise
 * si l'arme est maîtrisée ; le **même** modificateur de caractéristique aux dégâts. « Une attaque à
 * mains nues inflige une quantité de dégâts contondants égale à 1 + votre modificateur de Force.
 * Vous maîtrisez automatiquement ce type d'attaques. »
 *
 * @param sheet La fiche, pour ses modificateurs de Force et de Dextérité.
 * @param weapon L'arme en main, ou `nullptr` pour le coup à mains nues.
 * @param proficiencyBonus Le bonus de maîtrise au niveau de la fiche (`core::proficiencyBonus`).
 * @param proficient Faux si le personnage ne maîtrise pas l'arme. Les classes provisoires du
 *        `LOT-36` ne déclarent pas leurs maîtrises d'armes : l'appelant passe vrai jusqu'au socle
 *        de classe (`LOT-47`). Sans effet à mains nues.
 */
[[nodiscard]] AttackProfile weaponAttackFor(const CharacterSheet& sheet, const Weapon* weapon,
                                            int proficiencyBonus, bool proficient = true);

/**
 * @brief L'attaque d'une arme **lancée** — dague, hachette, javeline —, si elle a la propriété
 *        `thrown`.
 *
 * Manuel, chapitre 5, « Lancer » : une arme de corps à corps lancée emploie la même caractéristique
 * qu'au corps à corps. L'attaque est à distance, à la portée de l'arme. Vide pour une arme sans
 * cette propriété : une épée longue lancée est une arme improvisée, qui n'est pas de ce lot.
 */
[[nodiscard]] std::optional<AttackProfile> thrownAttackFor(const CharacterSheet& sheet,
                                                           const Weapon& weapon,
                                                           int proficiencyBonus,
                                                           bool proficient = true);

/**
 * @brief La distance en cases entre deux combattants, **emprises comprises** : zéro contact
 *        impossible, 1 pour deux emprises adjacentes, diagonale comprise.
 *
 * Manuel, « Jouer sur un quadrillage » : on compte les cases en partant d'une case adjacente à la
 * première créature et en finissant sur la case de la seconde. Un ogre de taille G sur 2 × 2 cases
 * touche donc à 1 tout ce qui borde son emprise, et non ce qui borde son ancre.
 * @return Vide si l'un des deux n'est pas sur la grille.
 */
[[nodiscard]] std::optional<int> gridDistance(const CombatState& combat, CombatantId from,
                                              CombatantId to);

/// @brief La même distance, @p mover supposé ancré en @p moverAnchor.
[[nodiscard]] std::optional<int> gridDistanceFrom(const CombatState& combat, CombatantId mover,
                                                  GridPosition moverAnchor, CombatantId other);

/**
 * @brief Vrai si @p target est à portée de l'attaque : allonge au corps à corps ; au contact, ou
 *        dans la portée maximale si elle est connue, à distance.
 *
 * La distance seule : la vue est dans `core::checkTarget`.
 */
[[nodiscard]] bool inReach(const CombatState& combat, CombatantId attacker, CombatantId target,
                           const AttackProfile& profile);

/// @brief Ce qui rend une cible attaquable, ou ce qui l'en empêche.
enum class TargetCheck : std::uint8_t {
    Valid,
    /// L'attaquant et la cible sont le même, ou l'un des deux n'est pas sur la grille.
    NotOnGrid,
    /// Hors d'allonge, ou au-delà de la longue portée (`core::inReach`).
    OutOfReach,
    /// Sous abri total : aucun segment dégagé entre les deux emprises (`core::hasLineOfSight`).
    TotalCover,
};

/**
 * @brief Peut-on viser @p target avec cette attaque ? La distance, puis la vue.
 *
 * Manuel, chapitre 9 : « une cible qui bénéficie d'un abri total ne peut pas être ciblée
 * directement par des attaques » ; `EX-CBT-021` : une attaque à distance suppose une ligne de vue.
 * Au corps à corps aussi — frapper par le coin commun de deux murs est aussi impossible que de s'y
 * faufiler.
 */
[[nodiscard]] TargetCheck checkTarget(const CombatState& combat, CombatantId attacker,
                                      CombatantId target, const AttackProfile& profile);

/// @brief Les sources nommées d'avantage et de désavantage d'un jet.
struct AttackCircumstances {
    std::vector<std::string> advantages;
    std::vector<std::string> disadvantages;
};

/**
 * @brief Ce que la grille sait dire des circonstances d'une attaque (Manuel, chapitre 9).
 *
 * - **Attaque à distance dans un combat au corps à corps** : désavantage si une créature hostile
 *   debout « qui vous voit » (`core::hasLineOfSight`) se trouve à une case de l'attaquant ;
 * - **au-delà de la portée normale** : désavantage.
 *
 * L'abri n'est pas une circonstance mais un changement de CA (`core::AttackRoll::applyCover`).
 * Voir sans être vu suppose la lumière et les sens ; l'état de la cible est au `LOT-72`.
 */
[[nodiscard]] AttackCircumstances attackCircumstances(const CombatState& combat,
                                                      CombatantId attacker, CombatantId target,
                                                      const AttackProfile& profile);

/// @brief Les trois instants où un jet d'attaque se lit et s'amende, dans l'ordre.
enum class AttackRollStage : std::uint8_t {
    /// Avant les dés : ajouter une source d'avantage ou de désavantage, un bonus, changer la CA.
    BeforeRoll,
    /// Les d20 sont tombés : les lire bruts, en relancer un, en substituer un.
    DiceRolled,
    /// Le total est connu : ajouter un modificateur après l'avoir vu. Dernier instant avant
    /// l'issue.
    BeforeOutcome,
};

/**
 * @brief Un jet d'attaque : la demande, les dés, les amendements, et l'issue une fois figée.
 *
 * `check` suit le jet à chaque étape : dés lancés, dé retenu, modificateurs, total, seuil (la CA).
 */
struct AttackRoll {
    CombatantId attacker{};
    CombatantId target{};
    std::string label;
    /// La CA visée : celle du profil de la cible, plus le bonus de son abri (`applyCover`).
    int armorClass = 10;
    /// L'abri déjà compté dans `armorClass`.
    Cover cover = Cover::None;
    int criticalThreshold = 20;
    std::vector<std::string> advantages;
    std::vector<std::string> disadvantages;
    CheckResult check;
    /// Ce que les crochets ont changé, dans l'ordre : « relance (Chance) : 1 -> 14 ».
    std::vector<std::string> amendments;
    /// L'issue, figée après `BeforeOutcome`.
    bool hit = false;
    bool critical = false;

    /// @brief Remplace le d20 d'indice @p die par @p value (un résultat stocké, *Portent*).
    void substitute(std::size_t die, int value, const std::string& source);
    /// @brief Ajoute un modificateur, avec son origine.
    void addModifier(Modifier modifier);
    /**
     * @brief Place la cible derrière @p level : la CA gagne le bonus de l'abri, **une fois**.
     *
     * Les abris ne s'additionnent pas (Manuel, « Abri ») : un abri moins bon ou égal à celui déjà
     * compté ne change rien, un meilleur remplace l'ancien bonus au lieu de s'y ajouter. Poser deux
     * fois l'abri partiel laisse la CA à +2 ; l'abri important par-dessus la porte à +5, pas à +7.
     * `Cover::Total` est sans effet : ce n'est pas un bonus mais une cible qu'on ne vise pas
     * (`core::checkTarget`). Le changement s'inscrit dans `amendments`.
     */
    void applyCover(Cover level);
    /// @brief Recalcule le dé retenu et le total depuis les dés et les modificateurs.
    void recompute();
};

/// @brief Un point d'insertion d'un jet d'attaque.
using AttackRollListener = std::function<void(AttackRoll&, DeterministicRandom&)>;

/// @brief Les greffons des jets d'attaque, par étape, dans l'ordre d'insertion.
class AttackHooks {
public:
    /// @brief Ajoute un greffon à l'étape @p stage, après ceux déjà insérés à cette étape.
    void insert(AttackRollStage stage, AttackRollListener listener);
    /// @brief Applique à @p roll, dans l'ordre d'insertion, tous les greffons de l'étape @p stage.
    void run(AttackRollStage stage, AttackRoll& roll, DeterministicRandom& random) const;

private:
    std::vector<std::pair<AttackRollStage, AttackRollListener>> _listeners;
};

/**
 * @brief Jette le d20 d'une attaque : crochets, dés, amendements, puis l'issue.
 *
 * La posture se déduit des **nombres** de sources (`core::rollStance`, `EX-REG-002`). L'issue :
 * un 1 naturel rate ; un résultat au moins égal au seuil critique touche et est critique ; sinon,
 * le total atteint ou dépasse la CA.
 */
[[nodiscard]] AttackRoll rollAttack(AttackRoll request, const AttackHooks& hooks,
                                    DeterministicRandom& random);

/// @brief Une attaque résolue : le jet, les dégâts lancés, et ce que la cible en a reçu.
struct AttackOutcome {
    AttackRoll roll;
    std::string attackerName;
    std::string targetName;
    std::vector<RolledDamage> damage;
    std::optional<DamageReport> report;

    /**
     * @brief L'entrée de journal (`EX-REG-003`) : qui, quoi, le d20 et chaque modificateur avec
     *        son origine, la CA, l'issue, les dés de dégâts, chaque étape qui les a changés, et les
     *        points de vie avant et après.
     *
     * « 7 + 3 = 10 contre CA 15 : raté » est une information ; « tu as raté » n'en est pas une.
     */
    [[nodiscard]] std::string describe() const;
};

/// @brief Ce qu'une résolution consulte en plus du profil : greffons et circonstances.
struct AttackContext {
    const AttackHooks* hooks = nullptr;
    const DamagePipeline* pipeline = nullptr;
    /// Sources supplémentaires, que l'appelant connaît et la grille non (une esquive, une aide).
    AttackCircumstances circumstances;
};

/**
 * @brief Résout une attaque dans le combat : déclaration, jet, dégâts, points de vie.
 *
 * Annonce `CombatHook::AttackDeclared` **avant** le jet (`LOT-20`), ajoute les circonstances que la
 * grille dit, pose l'abri de la cible (`core::coverBetween`) **avant le premier greffon**
 * `BeforeRoll` — qui peut le relever, jamais le cumuler —, jette, et si l'attaque touche, fait
 * traverser les dégâts au pipeline — qui se termine dans `core::CombatState::applyDamage`. Ne
 * dépense **aucune** ressource : une attaque de l'action *attaquer* dépense l'action, une attaque
 * d'opportunité la réaction, et c'est l'appelant qui sait laquelle il joue. Ne vérifie ni la portée
 * ni la vue, pour la même raison (`core::checkTarget`).
 *
 * @return Vide si l'attaque ne peut pas être déclarée : l'un des deux n'est pas debout, ou le
 *         combat n'est pas en cours.
 */
[[nodiscard]] std::optional<AttackOutcome> resolveAttack(CombatState& combat, CombatantId attacker,
                                                         CombatantId target,
                                                         const AttackProfile& profile,
                                                         DeterministicRandom& random,
                                                         const AttackContext& context = {});

}  // namespace core
