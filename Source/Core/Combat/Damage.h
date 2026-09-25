// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/Damage.h
 * @brief Les dégâts typés et leur pipeline à étapes nommées (`LOT-21`, `EX-CBT-031`,
 *        `EX-CBT-032`).
 *
 * ## Pourquoi un pipeline, et pas une soustraction
 *
 * Retirer des points de vie est la dernière chose que font des dégâts. Avant, ils ont une
 * **source** (magique, argentée, un sort), un **type** qu'une capacité peut convertir, une cible
 * qui y **résiste**, y est **vulnérable** ou **immunisée**, et des **réserves** — points de vie
 * temporaires, armure ablative — qui les absorbent. Les livres de Tanares greffent une capacité à
 * chacun de ces instants (§4bis de la feuille de route) : un entier qui descend ne laisserait
 * aucune place où la greffer, et chaque capacité réécrirait la soustraction à sa façon.
 *
 * Les étapes sont donc **nommées** (`core::DamageStage`), dans un ordre fixe, et chacune est un
 * point d'insertion (`core::DamagePipeline::insert`). Le pipeline se termine dans
 * `core::CombatState::applyDamage`, qui borne à 0, met à terre et évalue l'issue — une salve
 * entière en un appel.
 */

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Core/Levels/GridPosition.h"
#include "Core/Math/DeterministicRandom.h"
#include "Core/Rpg/Dice.h"
#include "Core/Rpg/RpgEnums.h"

namespace core {

class BattleGrid;
class CombatState;
struct Creature;
enum class CombatantId : std::uint32_t;

/**
 * @brief Ce qu'une source de dégâts **est**, au-delà de son type.
 *
 * Un démon résiste aux armes **non magiques** ; un sort n'est pas une arme. Le type seul ne
 * suffit pas à le dire, et le texte de l'arme ne se lit pas. Les deux
 * derniers drapeaux ne décrivent pas la source mais ce qu'elle **passe outre** — « ignore les
 * résistances », « ignore les points de vie temporaires » : des capacités les écrivent telles
 * quelles.
 */
enum class DamageFlag : std::uint8_t {
    Magical = 1U << 0U,
    Spell = 1U << 1U,
    IgnoresResistance = 1U << 2U,
    IgnoresReserves = 1U << 3U,
};

/// @brief Un ensemble de `core::DamageFlag`.
using DamageFlags = std::uint8_t;

/// @brief L'ensemble ne contenant que le drapeau @p flag.
[[nodiscard]] constexpr DamageFlags flagsOf(DamageFlag flag) noexcept {
    return static_cast<DamageFlags>(flag);
}

/// @brief Vrai si l'ensemble @p flags contient le drapeau @p flag.
[[nodiscard]] constexpr bool hasFlag(DamageFlags flags, DamageFlag flag) noexcept {
    return (flags & flagsOf(flag)) != 0U;
}

/// @brief Résistance, vulnérabilité ou immunité.
enum class DamageAffinityKind : std::uint8_t {
    Resistance,
    Vulnerability,
    Immunity,
};

/**
 * @brief Une affinité de la cible à un type de dégâts, et ce qui la **contourne**.
 *
 * `bypassedBy` vide : l'affinité vaut toujours. Sinon, une source portant **l'un** de ces drapeaux
 * la contourne — « résistance aux dégâts contondants des attaques non magiques » s'écrit
 * `{Bludgeoning, Resistance, Magical}`.
 */
struct DamageAffinity {
    DamageType type = DamageType::Bludgeoning;
    DamageAffinityKind kind = DamageAffinityKind::Resistance;
    DamageFlags bypassedBy = 0;

    [[nodiscard]] bool operator==(const DamageAffinity&) const = default;
};

/// @brief Les affinités d'une cible — combattant ou structure.
struct DamageTraits {
    std::vector<DamageAffinity> affinities;

    /// @brief Vrai si une affinité de ce genre s'applique à des dégâts de ce type et de cette
    /// source.
    [[nodiscard]] bool applies(DamageAffinityKind kind, DamageType type, DamageFlags flags) const;
};

/**
 * @brief Une clause de dégâts, telle qu'une attaque la porte **avant** les dés.
 *
 * Une attaque en a une ou plusieurs : « 1d4 + 4 perforants, et 3d6 de poison » en fait deux, de
 * deux types, que les résistances traitent séparément.
 */
struct DamageClause {
    Dice dice;
    DamageType type = DamageType::Bludgeoning;
    DamageFlags flags = 0;
};

/**
 * @brief Une clause lancée.
 *
 * Sur un coup critique, **les dés sont doublés, pas le modificateur** (`EX-CBT-031`) : `roll.dice`
 * porte alors deux fois plus de dés que la clause, et le même modificateur. L'erreur inverse —
 * doubler le total — fait croître la part fixe avec le niveau et fausse tout l'équilibrage.
 */
struct RolledDamage {
    DamageClause clause;
    DiceRoll roll;
    bool critical = false;
    /// Le total, jamais négatif : un 1d4-3 qui fait -2 ne soigne pas.
    int amount = 0;
};

/// @brief Lance chaque clause, en doublant les dés si le coup est critique.
[[nodiscard]] std::vector<RolledDamage> rollDamage(std::span<const DamageClause> clauses,
                                                   bool critical, DeterministicRandom& random);

/**
 * @brief Les étapes du pipeline, **dans l'ordre** où elles s'exécutent.
 *
 * | Étape | Ce que le moteur y fait | Ce qui s'y greffe |
 * |---|---|---|
 * | `Source` | rien | poser un drapeau (arme bénie devenue magique) |
 * | `Conversion` | rien | changer le type (*Shadowcaster*, zone *Maelstrom*) |
 * | `Resistances` | immunité, puis résistance, puis vulnérabilité | une réduction fixe, **avant** |
 * | `Reserves` | points de vie temporaires, réserves ablatives | un transfert (*Life Link*) |
 * | `HitPoints` | la perte, bornée à 0, en une salve | rien : c'est la fin |
 */
enum class DamageStage : std::uint8_t {
    Source,
    Conversion,
    Resistances,
    Reserves,
    HitPoints,
};

/**
 * @brief Nom français d'un type de dégâts, pour le journal : « tranchant », « feu ».
 *
 * Le nom de **donnée** reste `core::damageTypeName` (`slashing`) ; celui-ci est ce que le journal
 * écrit, dans la langue du Manuel, sans défaut silencieux (`EX-CBT-032`).
 */
[[nodiscard]] std::string_view damageTypeLabel(DamageType type) noexcept;

/// @brief Nom textuel d'une étape, pour le journal.
[[nodiscard]] std::string_view damageStageName(DamageStage stage) noexcept;

/**
 * @brief Une réserve de points de vie : ce qui absorbe les dégâts **avant** les PV.
 *
 * Les PV sont une pile de réserves, pas un entier : points de vie temporaires, exosquelette de 80
 * PV, PV mis en commun avec une monture. Une réserve **qui ne se cumule pas** — les points de vie
 * temporaires du Manuel — remplace celle de même source si elle est plus grande, et est ignorée
 * sinon.
 *
 * Manuel, chapitre 9, « Points de vie temporaires » : ils sont perdus **d'abord** ; ils ne se
 * cumulent pas, et le joueur « décide s'il conserve ceux qu'il possède ou les remplace » — le
 * moteur garde le plus grand, qui est le seul choix raisonnable tant qu'aucune réserve n'a de
 * durée ; les soins ne les rendent pas ; ils absorbent encore à 0 PV, sans relever personne.
 */
struct HitPointReserve {
    /// Ce qui l'a donnée : « points de vie temporaires », « Exosquelette ».
    std::string source;
    int amount = 0;
    bool stacks = false;

    [[nodiscard]] bool operator==(const HitPointReserve&) const = default;
};

/// La source des points de vie temporaires du Manuel : ils ne se cumulent pas.
inline constexpr std::string_view TEMPORARY_HIT_POINTS = "points de vie temporaires";

/// @brief Une ligne de la trace : ce qu'une étape a fait d'une clause.
struct DamageStep {
    DamageStage stage = DamageStage::Source;
    /// Ce qui a agi : « resistance (tranchant) », « points de vie temporaires ».
    std::string source;
    int before = 0;
    int after = 0;
};

/// @brief Une clause **en cours** de pipeline : type courant, drapeaux courants, montant courant.
struct DamagePortion {
    DamageType type = DamageType::Bludgeoning;
    DamageFlags flags = 0;
    int amount = 0;
};

/**
 * @brief Les dégâts qu'une cible reçoit, pendant qu'ils traversent le pipeline.
 *
 * C'est ce que reçoit un point d'insertion, **modifiable** : un crochet qui ne pourrait rien
 * changer ne servirait à rien. Chaque changement de montant s'inscrit dans `trace` par `adjust`,
 * pour que le journal dise « résistance : 10 → 5 » et non un 5 sorti de nulle part.
 */
struct DamageWork {
    /// Le combattant visé, ou vide pour une structure.
    std::optional<CombatantId> target;
    /// La case de la structure visée.
    std::optional<GridPosition> structure;
    std::vector<DamagePortion> portions;
    /// Absorbé par les réserves.
    int absorbed = 0;
    /// Perdu en points de vie — calculé à l'étape `Reserves`, appliqué à l'étape `HitPoints`.
    int hitPointLoss = 0;
    std::vector<DamageStep> trace;

    /// @brief Somme des portions.
    [[nodiscard]] int total() const;
    /// @brief Change le montant d'une portion, et l'inscrit dans la trace.
    void adjust(std::size_t portion, int amount, DamageStage stage, std::string source);
};

/// @brief Un point d'insertion : reçoit les dégâts d'une cible, et l'état du combat s'il y en a un.
using DamageListener = std::function<void(DamageWork&, CombatState*)>;

/// @brief Les dégâts d'une cible dans une salve : le combattant et ses clauses lancées.
struct DamageRequest {
    CombatantId target{};
    std::vector<RolledDamage> damage;
};

/// @brief Ce qu'une cible a reçu, étape par étape.
struct DamageReport {
    DamageWork work;
    int hitPointsBefore = 0;
    int hitPointsAfter = 0;
    /**
     * @brief Les dégâts **restants** une fois la cible tombée à 0 (Manuel, chapitre 9, « Mort
     *        instantanée »).
     *
     * Un clerc à 6 PV sur 12 qui en subit 18 tombe à 0 avec un excédent de 12 — égal à son
     * maximum : il meurt sur le coup. Ce lot borne à 0 et **rapporte** l'excédent ; c'est l'agonie
     * (`LOT-72`) qui en tire la mort.
     */
    int overflow = 0;
};

/**
 * @brief Le pipeline de dégâts : cinq étapes nommées, et ce qui s'y greffe.
 *
 * Deux greffons d'une même étape s'exécutent dans l'ordre de leur insertion, **avant** ce que le
 * moteur fait à cette étape : une réduction fixe précède la résistance (*Basic Rules* : « la
 * résistance puis la vulnérabilité s'appliquent après tous les autres modificateurs »), et un
 * transfert de PV précède l'absorption par les réserves.
 */
class DamagePipeline {
public:
    /// @brief Ajoute un greffon à l'étape @p stage, après ceux déjà insérés à cette étape.
    void insert(DamageStage stage, DamageListener listener);

    /**
     * @brief Fait traverser une salve à des combattants, et la termine en **un** appel à
     *        `core::CombatState::applyDamage`.
     *
     * Une cible absente, sortie du combat, ou touchée par un montant nul est rapportée sans rien
     * changer. Deux demandes pour la même cible s'appliquent l'une après l'autre dans la salve.
     */
    std::vector<DamageReport> apply(CombatState& combat,
                                    std::span<const DamageRequest> volley) const;

    /**
     * @brief Fait traverser des dégâts à une **structure** de la grille : les structures ont des
     *        PV, et les mêmes résistances — une porte de fer ne craint pas le poison.
     *
     * Pas de réserves : une structure n'en a pas. L'objet détruit quitte la grille
     * (`core::BattleGrid::damageObject`).
     */
    DamageReport applyToStructure(BattleGrid& grid, GridPosition cell,
                                  std::span<const RolledDamage> damage) const;

private:
    void runStage(DamageStage stage, DamageWork& work, CombatState* combat) const;
    static void applyAffinities(DamageWork& work, const DamageTraits& traits);

    std::vector<std::pair<DamageStage, DamageListener>> _listeners;
};

/**
 * @brief Les affinités d'une créature du bestiaire.
 *
 * Le bestiaire du SRD porte des listes nues : aucune n'y est conditionnelle, et la condition « non
 * magique » reste dans le texte tant que la donnée ne l'écrit pas (`requiredMechanisms`,
 * `EX-CNT-031`). Chaque type y devient une affinité sans contournement.
 */
[[nodiscard]] DamageTraits damageTraitsFor(const Creature& creature);

}  // namespace core
