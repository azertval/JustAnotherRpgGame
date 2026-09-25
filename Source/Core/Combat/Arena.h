// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Core/Combat/CombatState.h"
#include "Core/Levels/Level.h"
#include "Core/Math/DeterministicRandom.h"
#include "Core/Rpg/Bestiary.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Check.h"
#include "Core/Rpg/Dice.h"

/**
 * @file Core/Combat/Arena.h
 * @brief Le Colisée : un affrontement qui se monte, se joue et se rejoue à graine fixée
 *        (`LOT-50`).
 *
 * ## Ce que l'arène est, et ce qu'elle n'est pas
 *
 * Les Arènes de Tanares sont une institution du monde : deux camps y règlent un litige par leurs
 * champions, **sans mort**, sous la protection d'un rituel de Marque Héroïque. C'est ce qui en
 * fait le banc d'essai naturel du combat — un affrontement s'y rejoue indéfiniment, et c'est la
 * fiction qui l'explique, pas une entorse aux règles.
 *
 * Tout ce qui est ici est du `Core` pur : une carte (`core::Level`), une composition
 * (`core::ArenaBout`), et une session (`core::ArenaSession`) qui tient la machine à états du
 * combat (`core::CombatState`, `LOT-20`) sur la grille de la carte (`core::BattleGrid`, `LOT-19`).
 * Rien ne dépend d'une fenêtre : le modèle de présentation du combat (`hmi::CombatModel`) ne fait
 * que présenter et commander.
 *
 * Le **catalogue d'arènes** (une donnée par variante régionale, dans un dossier du monde) et
 * l'écran de mise en place du Colisée ont été retirés à la recette de la 0.0.1, le 25 septembre
 * 2026 : aucune arène ne vit dans `Source/Elements`. La composition vient désormais de la
 * **rencontre de carte** (`core::prepareMapEncounter`, `hmi::EncounterModel`), qui monte cette
 * session sur la zone de combat d'une carte, aux points d'entrée que la carte déclare
 * (`core::arenaEntryPoints`).
 *
 * ## Les attaques du Manuel
 *
 * Le coup d'essai du `LOT-50` est remplacé par les attaques du `LOT-21` (`core::resolveAttack`) :
 * les profils se tirent du bestiaire (`core::attacksFor`) ou de l'arme de la fiche
 * (`core::weaponAttackFor`), le jet s'amende par `attackHooks`, les dégâts traversent
 * `damagePipeline`. Ce que le `LOT-50` avait posé reste : la session, la déclaration avant le jet,
 * la suite aléatoire unique, le journal. S'y ajoutent, du chapitre 9 du Manuel, l'**attaque
 * d'opportunité** au déplacement, et les actions **esquiver** et **se désengager** qui la règlent.
 */

#include "Core/Combat/Attack.h"

namespace core {

/// Type d'entité de carte qui marque un point d'entrée de l'arène (`Source/Elements/Levels/`).
inline constexpr std::string_view ARENA_ENTRY_ENTITY_TYPE = "arenaEntry";
/// Propriété d'un point d'entrée : le camp qu'il accueille, `"allies"` ou `"enemies"`.
inline constexpr std::string_view ARENA_SIDE_PROPERTY = "side";
/// Propriété d'un point d'entrée : son rang dans l'ordre d'appel. Deux rangs égaux se départagent
/// par la position (ligne, puis colonne) — l'ordre de la donnée, jamais celui de la mémoire.
inline constexpr std::string_view ARENA_RANK_PROPERTY = "rank";
/// La **troisième économie d'action** des Marques Héroïques (§4bis) : une ressource déclarée à
/// `core::ActionEconomy`, que le rituel de l'arène accorde à chaque combattant marqué.
inline constexpr std::string_view HEROIC_ACTION_RESOURCE = "heroicAction";

/// @brief Un point d'entrée de l'arène : une case, un camp, un rang d'appel.
struct ArenaEntryPoint {
    CombatSide side = CombatSide::Allies;
    int rank = 0;
    GridPosition position;

    [[nodiscard]] bool operator==(const ArenaEntryPoint&) const = default;
};

/**
 * @brief Les points d'entrée d'une carte, rangés par camp, rang, puis position.
 *
 * Une entité `arenaEntry` sans camp lisible est ignorée : la carte reste jouable avec les entrées
 * qu'elle déclare correctement, et c'est le montage qui dira qu'il manque une place.
 */
[[nodiscard]] std::vector<ArenaEntryPoint> arenaEntryPoints(const Level& level);

/// @brief Un combattant tel que l'écran de mise en place le compose.
struct ArenaContestant {
    /// Le profil, classe d'armure et affinités comprises.
    CombatantProfile profile;
    /// Ce qu'il sait frapper, la première attaque étant celle qu'on joue par défaut.
    std::vector<AttackProfile> attacks;
    /// Case demandée, ou absente : le prochain point d'entrée libre de son camp.
    std::optional<GridPosition> position;
    /// Rôle de Marque Héroïque revendiqué (un identifiant de `Rpg/rules/heroic-marks.json`), ou
    /// vide.
    std::string markId;
    /// Le profil de comportement qui le joue (`core::BehaviorProfile::id`, `LOT-23`), ou vide
    /// pour un combattant que le joueur commande.
    std::string behavior;
};

/// @brief Une composition d'affrontement : qui, contre qui, à quelle graine, sous quelle règle.
struct ArenaBout {
    std::vector<ArenaContestant> contestants;
    std::uint64_t seed = 0;
    bool lethal = false;
    bool heroicMark = true;
    /// La prise en tenaille, règle optionnelle du *Guide du Maître* (`core::isFlanked`). Fausse par
    /// défaut : une règle optionnelle s'active, elle ne se présume pas.
    bool flanking = false;
    /// Vrai si l'on peut se retirer : toujours au Colisée ; sur la carte, ce que la rencontre dit
    /// (`core::Encounter::escapable`, `LOT-118`).
    bool escapable = true;
};

class ArenaSession;

/**
 * @brief Décide si @p reactor prend l'attaque d'opportunité que @p mover lui offre.
 *
 * Le Manuel dit qu'une créature « peut » la prendre : c'est une décision, et c'est à qui commande
 * la créature de la prendre — le comportement (`LOT-23`), l'écran de combat (`LOT-24`).
 */
using OpportunityPolicy =
    std::function<bool(const ArenaSession&, CombatantId reactor, CombatantId mover)>;

/**
 * @brief Ce que l'écran veut savoir d'un pas : qui a marché, par où (`LOT-118`).
 *
 * Le journal note déjà chaque pas en clair ; l'écran, lui, veut le **chemin** pour faire marcher
 * la figurine case par case — un tour de l'IA se joue d'un bloc, et sans ce crochet il ne resterait
 * que des positions d'arrivée, ce qui se lit comme une téléportation.
 */
using MoveObserver = std::function<void(CombatantId mover, const Path& path)>;

/// @brief Ce que le montage d'un affrontement a produit : les enrôlés par camp, et les refus.
struct ArenaMount {
    std::vector<CombatantId> allies;
    std::vector<CombatantId> enemies;
    std::vector<MountRefusal> refusals;
};

/// @brief Ce qu'une action de l'arène a donné.
enum class ArenaActionResult : std::uint8_t {
    /// L'action a eu lieu — une attaque a été jetée, qu'elle ait touché ou non.
    Done,
    /// Aucun tour actif : le combat n'a pas commencé, ou il est fini.
    NoActiveTurn,
    /// L'action de ce tour est déjà dépensée.
    NoAction,
    /// La cible est hors d'allonge ou de portée.
    OutOfReach,
    /// La cible est sous abri total : aucune ligne de vue ne la relie à l'attaquant (`LOT-22`).
    TotalCover,
    /// Inconnue, soi-même, un allié, ou une cible qui n'est pas debout.
    InvalidTarget,
    /// Le combattant n'a pas d'attaque de cet indice.
    NoAttack,
};

/// @brief Une attaque jouée dans l'arène : le refus, ou l'attaque résolue.
struct ArenaAttack {
    ArenaActionResult result = ArenaActionResult::InvalidTarget;
    std::optional<AttackOutcome> outcome;
};

/**
 * @brief Une session d'arène : la carte, la composition, et le combat qui s'y joue.
 *
 * C'est le premier endroit où `core::CombatState` est **tenu** par quelque chose du jeu, et non
 * seulement par un test. La session garde la carte et la composition pour pouvoir **rejouer** :
 * `replay` remonte le même affrontement à la même graine, et deux exécutions donnent alors le
 * même journal — c'est ce qui permet de comparer deux versions d'une mécanique.
 *
 * Le journal est une liste de lignes lisibles : crochets du combat, pas, coups, issue. Il est la
 * matière du rejeu vérifié par test, et ce que l'écran affiche.
 */
class ArenaSession {
public:
    /// @param level La carte de l'arène, conservée pour chaque rejeu.
    explicit ArenaSession(Level level);

    /**
     * @brief Monte l'affrontement : enrôle chaque combattant à sa case ou au prochain point
     *        d'entrée libre de son camp, en nommant chaque refus.
     *
     * Un combattant sans case et sans point d'entrée restant est refusé avec `OutOfBounds` : la
     * carte n'a plus de place pour lui, et le dire vaut mieux que le poser dans un mur. Le rituel
     * de Marque Héroïque, s'il s'applique, déclare la troisième économie d'action à chacun.
     */
    ArenaMount mount(const ArenaBout& bout);

    /// @brief Jette l'initiative à la graine de la composition, et ouvre le premier tour.
    bool start();

    /// @brief Remonte la même composition à la même graine, journal vidé.
    ArenaMount replay();

    /// @return La machine à états du combat, montée ou non.
    [[nodiscard]] CombatState& combat() noexcept {
        return *_combat;
    }
    /// @brief La machine à états du combat, en lecture seule.
    [[nodiscard]] const CombatState& combat() const noexcept {
        return *_combat;
    }

    /// @return La carte de l'arène.
    [[nodiscard]] const Level& level() const noexcept {
        return _level;
    }

    /// @return La composition montée.
    [[nodiscard]] const ArenaBout& bout() const noexcept {
        return _bout;
    }

    /// @return Les attaques d'un combattant enrôlé, ou `nullptr`.
    [[nodiscard]] const std::vector<AttackProfile>* attacks(CombatantId combatant) const;

    /**
     * @brief L'action *attaquer* du combattant actif, avec son attaque @p attackIndex.
     *
     * Vérifie la cible, la portée et la vue (`core::checkTarget`), dépense l'action, puis résout
     * (`core::resolveAttack`) : déclaration, abri, jet, dégâts. Une cible qui esquive et voit son
     * attaquant impose le désavantage. Tout passe par la suite aléatoire de la session : un rejeu
     * redonne les mêmes coups.
     */
    ArenaAttack attack(CombatantId target, std::size_t attackIndex = 0);

    /**
     * @brief L'action *esquiver* : jusqu'au début de son prochain tour, les attaques contre le
     *        combattant actif sont désavantagées (Manuel, chapitre 9).
     * @return Faux sans tour actif, ou sans action.
     */
    bool dodge();

    /**
     * @brief L'action *se désengager* : jusqu'à la fin du tour, le déplacement du combattant actif
     *        ne provoque pas d'attaque d'opportunité.
     * @return Faux sans tour actif, ou sans action.
     */
    bool disengage();

    /**
     * @brief L'action *se précipiter* : le combattant actif gagne, pour ce tour, un déplacement
     *        supplémentaire égal à sa vitesse (Manuel, chapitre 9).
     * @return Faux sans tour actif, ou sans action.
     */
    bool dash();

    /// @brief Vrai si le combattant esquive — ce que la table voit, et donc ce que l'IA lit.
    [[nodiscard]] bool isDodging(CombatantId combatant) const {
        return _dodging.contains(combatant);
    }

    /// @brief Le profil de comportement qui joue ce combattant, vide si c'est le joueur (`LOT-23`).
    [[nodiscard]] const std::string& behaviorOf(CombatantId combatant) const;

    /**
     * @brief Le joueur laisse passer, ou non, les attaques d'opportunité de ce combattant
     *        (`LOT-24`).
     *
     * Le Manuel dit qu'une créature « peut » frapper le fuyard. Le choix se fait **avant** que
     * l'ennemi ne bouge, comme on tient une réaction prête : suspendre le déplacement d'autrui au
     * milieu de son chemin pour poser la question ferait d'un tour d'IA une suite de fenêtres.
     * Il survit au rejeu, dont les identifiants sont les mêmes. Vrai par défaut.
     */
    void setTakesOpportunities(CombatantId combatant, bool takes);
    /// @brief Vrai si @p combatant prend ses attaques d'opportunité (le défaut) ; faux s'il y a
    /// renoncé.
    [[nodiscard]] bool takesOpportunities(CombatantId combatant) const {
        return !_declinesOpportunities.contains(combatant);
    }

    /**
     * @brief Les créatures qui frapperaient le combattant actif s'il allait en @p destination,
     *        chacune une fois, dans l'ordre où elles frapperaient — la prévisualisation du
     *        déplacement. Même règle que `move`, politique et choix du joueur compris.
     */
    [[nodiscard]] std::vector<CombatantId> previewOpportunities(GridPosition destination) const;

    /**
     * @brief Les circonstances qu'ajoute la session à une attaque : l'esquive de la cible, la prise
     *        en tenaille. Celles de la grille sont dans `core::attackCircumstances`.
     */
    [[nodiscard]] AttackCircumstances circumstancesAgainst(CombatantId attacker, CombatantId target,
                                                           const AttackProfile& profile) const;

    /// @brief Qui décide des attaques d'opportunité. Sans politique, chacune est prise.
    void setOpportunityPolicy(OpportunityPolicy policy) {
        _opportunityPolicy = std::move(policy);
    }

    /// @brief Prévient @p observer à chaque pas effectué, avec son chemin (`LOT-118`).
    void setMoveObserver(MoveObserver observer) {
        _moveObserver = std::move(observer);
    }

    /// @brief Ajoute une ligne au journal : la décision d'un comportement, pour qu'elle se relise.
    void note(std::string line) {
        record(std::move(line));
    }

    /**
     * @brief Déplace le combattant actif ; le chemin est payé sur son budget restant.
     *
     * **Attaque d'opportunité** (Manuel, chapitre 9) : quand le chemin sort de l'allonge d'une
     * créature hostile debout qui a encore sa réaction et voit le fuyard (« située dans votre
     * champ de vision », `core::hasLineOfSight`), elle frappe « juste avant que la créature
     * ne sorte de sa zone d'allonge », avec sa première attaque au corps à corps, et dépense sa
     * réaction. Le déplacement s'arrête à la dernière case où l'on peut se tenir avant la sortie,
     * les attaques se jouent par identifiant croissant, et le déplacement reprend si le combattant
     * tient encore debout. Chaque créature éligible frappe si la politique d'opportunité
     * (`setOpportunityPolicy`) l'accepte — toutes, sans politique : l'écran qui laisse le joueur
     * décliner est au `LOT-24`, le comportement qui choisit au `LOT-23`.
     */
    MoveOutcome move(GridPosition destination);

    /// @brief Termine le tour actif.
    bool endTurn();

    /// @brief Le combattant actif quitte l'arène.
    WithdrawResult withdraw();

    /// @return L'issue du combat, absente tant qu'il court.
    [[nodiscard]] std::optional<CombatOutcome> outcome() const;

    /// @return Le journal de la partie en cours, dans l'ordre.
    [[nodiscard]] const std::vector<std::string>& journal() const noexcept {
        return _journal;
    }

private:
    void subscribe();
    void record(std::string line);
    /// Relève tout le monde : le rituel de la Marque Héroïque, qui fait des Arènes un lieu
    /// sans mort. Rien dans une arène létale.
    void restoreAll();
    /// Résout une attaque et l'écrit au journal à sa place : après la déclaration, avant ce que
    /// ses dégâts déclenchent.
    std::optional<AttackOutcome> resolveAndRecord(CombatantId attacker, CombatantId target,
                                                  const AttackProfile& profile,
                                                  const std::string& prefix);
    /// Les circonstances qu'ajoute la session : l'esquive de la cible, si elle voit l'attaquant ;
    /// la prise en tenaille au corps à corps, si l'arène la joue.
    [[nodiscard]] AttackContext contextAgainst(CombatantId attacker, CombatantId target,
                                               const AttackProfile& profile) const;
    /// Vrai si @p reactor frappe @p mover quand il passe de @p from à @p to : hostile, debout, la
    /// réaction disponible, l'allonge quittée, le fuyard vu, et ni le joueur ni la politique ne
    /// la déclinent.
    [[nodiscard]] bool provokes(CombatantId mover, CombatantId reactor, GridPosition from,
                                GridPosition to) const;
    /// La première attaque au corps à corps d'un combattant, ou `nullptr`.
    [[nodiscard]] const AttackProfile* meleeAttack(CombatantId combatant) const;
    /// Le premier pas de @p cases qui sort de l'allonge d'un ennemi, et ceux qu'il provoque,
    /// ajoutés à @p reactors ; vide pour un combattant désengagé.
    [[nodiscard]] std::optional<std::size_t> firstProvokingStep(
        CombatantId mover, const std::vector<GridPosition>& cases,
        std::vector<CombatantId>& reactors) const;
    /// Les attaques d'opportunité de @p reactors contre @p mover, tant qu'il est debout, que le
    /// combat dure et que l'opportuniste l'est aussi.
    void takeOpportunities(CombatantId mover, const std::vector<CombatantId>& reactors);

    Level _level;
    ArenaBout _bout;
    DeterministicRandom _random{0};
    std::unique_ptr<CombatState> _combat;
    std::map<CombatantId, std::vector<AttackProfile>> _attacks;
    std::map<CombatantId, std::string> _behaviors;
    OpportunityPolicy _opportunityPolicy;
    MoveObserver _moveObserver;
    std::set<CombatantId> _declinesOpportunities;
    AttackHooks _attackHooks;
    DamagePipeline _damagePipeline;
    std::set<CombatantId> _dodging;
    std::set<CombatantId> _disengaged;
    std::vector<std::string> _journal;
};

}  // namespace core
