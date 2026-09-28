// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/CombatState.h
 * @brief La machine à états du combat : rounds, tours, crochets et fins (`LOT-20`, `EX-CBT-010`,
 *        `EX-CBT-011`, `EX-CBT-012`).
 */

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "Core/Combat/ActionEconomy.h"
#include "Core/Combat/BattleGrid.h"
#include "Core/Combat/CombatCounters.h"
#include "Core/Combat/CombatTransition.h"
#include "Core/Combat/Damage.h"
#include "Core/Combat/Pathfinding.h"
#include "Core/Combat/TurnOrder.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Math/DeterministicRandom.h"
#include "Core/Rpg/Check.h"
#include "Core/Rpg/RpgEnums.h"

namespace core {

struct Bestiary;
struct CharacterSheet;
struct Creature;

/**
 * @brief L'état **nommé** du combat.
 *
 * Pas de drapeaux épars (`enCombat`, `tourFini`, `roundCommence`) dont une combinaison sur deux
 * n'aurait pas de sens : un seul état, et des transitions qui le changent.
 *
 * - `Setup` — la rencontre se monte : on enrôle et on place, personne n'a encore d'initiative ;
 * - `Starting` — les initiatives sont jetées, le premier round n'a pas commencé : c'est la fenêtre
 *   « avant le premier tour » (la réaction du *Natural Strategist*) ;
 * - `RoundStart` — un round commence, ou un repère d'initiative fixe se joue ;
 * - `TurnActive` — un combattant joue, et le tour attend **qu'on le termine** (`EX-CBT-012`) ;
 * - `TurnEnd` — le tour vient de se terminer (les actions légendaires se jouent ici) ;
 * - `Ended` — une des trois fins est atteinte ; plus rien ne change.
 */
enum class CombatPhase : std::uint8_t {
    Setup,
    Starting,
    RoundStart,
    TurnActive,
    TurnEnd,
    Ended,
};

/**
 * @brief Les **points d'insertion** du combat : où une capacité peut se greffer.
 *
 * Aucun n'a de consommateur dans le `LOT-20`, et tous en auront un : les livres de Tanares
 * placent des capacités à chacun de ces instants (§4bis de la feuille de route). Les poser après
 * coup aurait coûté une refonte de la machine ; les poser maintenant ne coûte qu'une énumération.
 *
 * | Crochet | Ce qui s'y greffe |
 * |---|---|
 * | `BeforeFirstTurn` | une réaction avant le premier tour (*Natural Strategist*) |
 * | `RoundStart` | les effets « au début de chaque round » |
 * | `InitiativeCount` | un repère fixe : actions de repaire (20), renforts (0) |
 * | `TurnStart` | les effets « au début de votre tour » |
 * | `TurnEnd` | les actions légendaires, dépensées en fin de tour d'autrui |
 * | `AttackDeclared` | les postures du moine, qui répondent à une attaque **déclarée** |
 * | `DamageTaken` | un seuil franchi (*Battle Fury* sous 50 %), des dégâts subis à 0 PV |
 * | `CombatantDowned` | un déclencheur à la chute (explosion, apparition) ; la concentration perdue
 * | | `DeathSaveDue` | le jet contre la mort d'un mourant, à sa place dans l'ordre (`LOT-137`) | |
 * `CombatantDied` | la mort : le journal, *revigorer* qui compte ses rounds (`LOT-137`) | |
 * `CombatantJoined`, `CombatantLeft` | un renfort, une fuite | | `CombatEnded` | l'expérience
 * (`LOT-74`), le retour à l'exploration (`LOT-18`) |
 */
enum class CombatHook : std::uint8_t {
    BeforeFirstTurn,
    RoundStart,
    InitiativeCount,
    TurnStart,
    TurnEnd,
    AttackDeclared,
    DamageTaken,
    CombatantDowned,
    DeathSaveDue,
    CombatantDied,
    CombatantJoined,
    CombatantLeft,
    CombatEnded,
};

/// @brief Ce qu'un crochet annonce.
struct CombatEvent {
    CombatHook hook = CombatHook::RoundStart;
    int round = 0;
    /// Le combattant concerné : celui dont c'est le tour, qui entre, qui sort, ou qui attaque.
    std::optional<CombatantId> combatant;
    /// La cible d'une attaque déclarée.
    std::optional<CombatantId> target;
    /// Le nom du repère d'initiative fixe.
    std::string marker;
    /// `DamageTaken` et `CombatantDowned` : les points de vie perdus, avant, après, le maximum.
    int amount = 0;
    int hitPointsBefore = 0;
    int hitPointsAfter = 0;
    int maximumHitPoints = 0;
    /// Les dégâts restants une fois la cible tombée à 0 — la mort instantanée (`LOT-137`).
    int overflow = 0;
    /// Vrai si les dégâts viennent d'un coup critique : subis à 0 PV, ils comptent deux échecs.
    bool critical = false;
};

/**
 * @brief Vrai si l'événement fait passer les points de vie **sous** la fraction
 *        @p numerator / @p denominator du maximum — « sous 50 % » s'écrit `(e, 1, 2)`.
 *
 * Franchir, pas être sous : une créature déjà sous la moitié qui reprend un coup ne déclenche pas
 * une seconde fois sa *Battle Fury*.
 */
[[nodiscard]] constexpr bool crossedBelow(const CombatEvent& event, int numerator,
                                          int denominator) noexcept {
    const long long seuil = static_cast<long long>(event.maximumHitPoints) * numerator;
    return static_cast<long long>(event.hitPointsBefore) * denominator >= seuil &&
           static_cast<long long>(event.hitPointsAfter) * denominator < seuil;
}

class CombatState;

/**
 * @brief Un abonné à un crochet.
 *
 * Il reçoit l'état **mutable** : un crochet qui ne pourrait rien changer ne servirait à rien. Il
 * peut infliger des dégâts, faire entrer ou sortir un combattant, octroyer une ressource, poser un
 * repère ou demander un tour flottant. Il ne peut pas **terminer un tour** — `endTurn` y est
 * refusé : la fin d'un tour est une décision de celui qui joue (`EX-CBT-012`), et un tour terminé
 * depuis le début de ce même tour rendrait l'ordre des abonnés significatif.
 */
using CombatListener = std::function<void(CombatState&, const CombatEvent&)>;

/// @brief Où en est un combattant.
enum class CombatantStatus : std::uint8_t {
    /// En état de combattre.
    Standing,
    /// À 0 point de vie, **inconscient** et à terre : il garde sa place dans l'ordre, et ses tours
    /// sont passés tant qu'il n'est pas relevé ; à sa place, il fait son jet contre la mort
    /// (`CombatHook::DeathSaveDue`, `LOT-137`).
    Down,
    /// Mort (`LOT-137`) : trois échecs, des dégâts massifs, ou un monstre à 0 point de vie. Il
    /// reste
    /// sur la grille — son corps —, et seul *revigorer* le ramène (`CombatState::revive`).
    Dead,
    /// Sorti du combat : il a quitté la grille et l'ordre.
    Withdrawn,
};

/**
 * @brief Ce qui arrive à un combattant qui tombe à 0 point de vie (Manuel des Joueurs, « Tomber à
 *        0 point de vie », PDF p. 199).
 */
enum class AtZeroHitPoints : std::uint8_t {
    /// Il perd conscience et fait ses jets contre la mort : un personnage.
    DeathSaves,
    /// Il meurt : « la plupart des MD considèrent que les monstres meurent dès qu'ils atteignent
    /// 0 point de vie ».
    Dies,
};

/// @brief Le compteur des jets contre la mort d'un combattant à terre.
struct DeathSaves {
    int successes = 0;
    int failures = 0;
    /// Stabilisé : trois succès, *épargner les mourants*. Il ne jette plus, et reste inconscient.
    bool stable = false;

    [[nodiscard]] bool operator==(const DeathSaves&) const = default;
};

/// @brief Ce qu'un jet contre la mort a donné (`CombatState::recordDeathSave`).
enum class DeathSaveOutcome : std::uint8_t {
    /// 10 ou plus : un succès de plus.
    Success,
    /// Moins de 10 : un échec de plus ; un 1 naturel en compte deux.
    Failure,
    /// Le troisième succès : il est stabilisé.
    Stabilized,
    /// Le troisième échec : il meurt.
    Died,
    /// Un 20 naturel : il récupère 1 point de vie et se relève.
    Revived,
    /// Personne à faire jeter : inconnu, debout, mort, déjà stable, ou combat sans mort.
    Ignored,
};

/**
 * @brief Ce que le combat doit savoir d'un combattant — ni fiche, ni bloc de statistiques.
 *
 * Le combat ne dépend ni de `core::CharacterSheet` ni de `core::Creature` : un personnage, un
 * compagnon et un gobelin s'y présentent sous la même forme, et `profileFor` la construit depuis
 * l'un ou l'autre. C'est ce qui garde la machine à états indifférente à « qui est le joueur ».
 */
struct CombatantProfile {
    std::string name;
    CombatSide side = CombatSide::Allies;
    int maximumHitPoints = 0;
    int currentHitPoints = 0;
    /// Valeur de Dextérité, second critère de départage de l'initiative.
    int dexterity = 10;
    /// Modificateur du test d'initiative — la Dextérité, et ce qui s'y ajoute.
    int initiativeModifier = 0;
    RollStance initiativeStance = RollStance::Normal;
    /// Budget de déplacement, en cases (`core::movementBudget`).
    int movement = 0;
    Locomotion locomotion = Locomotion::Walk;
    CreatureSize size = CreatureSize::Medium;
    /**
     * @brief Vrai pour un acteur **flottant** (*Law of Time*) : il n'a pas de place dans l'ordre,
     *        et joue une fois par round **avant n'importe quel tour** de son choix
     *        (`CombatState::interject`) — ou à la fin du round, s'il n'a pas choisi.
     */
    bool floating = false;
    /**
     * @brief La classe d'armure contre laquelle on l'attaque (`EX-CBT-030`).
     *
     * **Recalculée depuis ses sources** au moment où le profil se construit — l'équipement porté
     * (`core::derivedStatsFor`), le bloc de bestiaire —, jamais tenue à jour à la main : un
     * équipement ne change pas au milieu d'un combat, et ce qui la change en combat (un abri, une
     * posture) s'ajoute au jet par un crochet, sans toucher à ce nombre.
     */
    int armorClass = 10;
    /// Résistances, vulnérabilités et immunités (`EX-CBT-032`).
    DamageTraits damageTraits;
    /**
     * @brief Le modificateur de chaque jet de sauvegarde, dans l'ordre de `core::Ability`
     *        (`LOT-133`) : ce que lit un sort qui en demande un (*boule de feu*).
     */
    std::array<int, 6> savingThrows{};
    /// Ce qui lui arrive à 0 point de vie : un personnage agonise, un monstre meurt (`LOT-137`).
    AtZeroHitPoints atZero = AtZeroHitPoints::DeathSaves;
};

/**
 * @brief Le profil d'un personnage : ses points de vie courants, sa Dextérité, sa vitesse, ses
 *        sauvegardes.
 *
 * La fiche ne porte pas de taille : un personnage est de taille M jusqu'à ce qu'elle en porte une.
 *
 * @param sheet La fiche.
 * @param side Son camp.
 * @param proficiencyBonus Le bonus de maîtrise à son niveau, ajouté aux sauvegardes qu'il
 *        maîtrise ; 0 laisse les modificateurs nus.
 */
[[nodiscard]] CombatantProfile profileFor(const CharacterSheet& sheet,
                                          CombatSide side = CombatSide::Allies,
                                          int proficiencyBonus = 0);

/**
 * @brief Le profil d'une créature du bestiaire.
 *
 * Elle se déplace **en vol** si sa vitesse de vol dépasse sa vitesse de marche — une chauve-souris
 * (1,5 m au sol, 9 m en vol) vole, un griffon qui marche aussi vite qu'il vole marche. Choisir
 * autrement, créature par créature, est l'affaire du comportement (`LOT-23`).
 */
[[nodiscard]] CombatantProfile profileFor(const Creature& creature,
                                          CombatSide side = CombatSide::Enemies);

/// @brief Un combattant engagé : son profil, son état, son économie et son jet d'initiative.
struct Combatant {
    CombatantId id{};
    CombatantProfile profile;
    CombatantStatus status = CombatantStatus::Standing;
    ActionEconomy economy;
    /// Le jet d'initiative, restituable (`EX-REG-003`). Vide pour un acteur flottant et pour un
    /// combattant entré à une initiative imposée.
    std::optional<CheckResult> initiativeRoll;
    /// Pour un acteur flottant : a-t-il déjà joué ce round-ci ?
    bool actedThisRound = false;
    /// Ce qui absorbe les dégâts avant les points de vie, dans l'ordre où on les a reçus.
    std::vector<HitPointReserve> reserves;
    /// Ses jets contre la mort, tant qu'il est à terre ; remis à zéro dès qu'il est soigné.
    DeathSaves deathSaves;
    /**
     * @brief À terre (Manuel, annexe A) : tombé à 0 point de vie, et encore allongé une fois
     *        relevé par un soin, jusqu'au début de son tour où il se relève.
     */
    bool prone = false;
    /// Le round de sa mort : *revigorer* ne ramène qu'un mort « depuis moins d'une minute ».
    std::optional<int> diedAtRound;
};

/// @brief Ce qu'un enrôlement a donné : l'identifiant, ou la raison du refus.
struct EnlistResult {
    std::optional<CombatantId> combatant;
    /// `Placed` si le combattant a été enrôlé — y compris sans case, avant le combat.
    PlacementResult placement = PlacementResult::Placed;
};

/// @brief Pourquoi une sortie a été refusée — ou qu'elle a eu lieu.
enum class WithdrawResult : std::uint8_t {
    Withdrawn,
    /// Un allié ne fuit pas une rencontre dont on ne fuit pas (`core::Encounter::escapable`).
    NotEscapable,
    /// Inconnu, déjà sorti, ou combat non commencé ou terminé.
    NotInCombat,
};

/// @brief Ce qu'un déplacement a donné.
enum class MoveResult : std::uint8_t {
    Moved,
    /// Aucun tour n'est en cours.
    NoActiveTurn,
    /// Le combattant actif n'est pas sur la grille.
    NotPlaced,
    /// La destination n'est pas une fin de déplacement permise dans ce qui reste du budget.
    Unreachable,
};

/// @brief Un déplacement : son issue, et le chemin suivi.
struct MoveOutcome {
    MoveResult result = MoveResult::NoActiveTurn;
    Path path;
};

/// @brief Une variation de points de vie, pour en appliquer plusieurs d'un seul coup.
struct HitPointChange {
    CombatantId target{};
    int amount = 0;
    /// Vrai si la perte vient d'un coup critique — recopié dans l'événement.
    bool critical = false;
};

/**
 * @brief Un combat, du montage de la rencontre à sa fin.
 *
 * ## Le cycle
 *
 * `start` jette l'initiative de chacun — **une fois** : l'ordre ne bouge plus (`EX-CBT-010`) —,
 * ouvre la fenêtre d'avant le premier tour, puis le premier round. Chaque round parcourt les places
 * de l'ordre ; un repère d'initiative fixe y annonce son crochet, un combattant à terre y est
 * passé, un combattant debout y ouvre son tour, et le combat **attend** : rien n'avance tant que
 * `endTurn` n'a pas été appelé (`EX-CBT-012`). Épuiser ses ressources ne termine pas le tour — ne
 * rien faire est une décision.
 *
 * ## Les trois fins
 *
 * Évaluées après **chaque** changement, et non à la fin d'un tour : un combat gagné par une
 * attaque d'opportunité pendant le tour d'un ennemi est gagné tout de suite.
 *
 * - **victoire** — plus aucun ennemi n'est debout : tous à terre, ou partis ;
 * - **défaite** — plus aucun allié n'est debout, et aucun n'est parti : tous à terre ;
 * - **fuite** — plus aucun allié n'est debout, et au moins un est **parti**. Ceux qui restent à
 *   terre derrière lui sont l'affaire de l'agonie (`LOT-137`), pas de l'issue.
 *
 * « Debout » compte seul : un allié à terre, stabilisé ou mort n'empêche pas la défaite du groupe
 * — tous à terre, c'est perdu (`LOT-137`).
 *
 * ## L'agonie et la mort (`LOT-137`)
 *
 * Manuel des Joueurs, « Tomber à 0 point de vie » (PDF p. 199). Un combattant qui tombe à 0 point
 * de vie est à terre, inconscient ; s'il ne fait pas de jets contre la mort
 * (`AtZeroHitPoints::Dies`), ou si les dégâts restants atteignent son maximum, il meurt sur le
 * coup. Blessé à 0 point de vie, il note un échec — deux sur un critique —, et meurt si ces dégâts
 * atteignent son maximum. À sa place dans l'ordre, la machine annonce `CombatHook::DeathSaveDue` :
 * qui tient les dés jette le d20 (`recordDeathSave`). Un soin le relève et remet le compteur à
 * zéro ; un mort ne se soigne pas, seul `revive` le ramène.
 *
 * Un combat **sans mort** (`setLethal(false)` : le rituel de la Marque Héroïque) ne tue personne
 * et ne fait pas jeter : on y tombe, et l'on se relève à la fin.
 *
 * Si un même changement abat les deux camps à la fois, la défaite l'emporte : on ne gagne pas un
 * combat où plus personne ne se tient debout de son côté.
 *
 * ## Les changements en cascade
 *
 * Un abonné peut changer l'état pendant qu'on l'avertit : infliger des dégâts qui abattent le
 * combattant dont le tour commence, faire entrer un renfort. Ces conséquences ne sont **jamais**
 * tirées au milieu d'une annonce : la machine les règle quand l'appel extérieur se termine, une par
 * une et dans un ordre fixe — la fin du combat d'abord, puis le tour d'un combattant qui ne peut
 * plus le jouer, puis la place suivante. Deux abonnés de même crochet sont avertis dans l'ordre de
 * leur abonnement.
 *
 * ## Sans héros unique
 *
 * Rien ici ne suppose un personnage seul : les alliés sont un camp, en nombre quelconque, et un
 * test en monte quatre (`LOT-29`).
 *
 * L'état n'est ni copiable ni déplaçable : les `core::Mover` qu'il construit et les abonnés qui le
 * reçoivent le désignent par son adresse.
 */
class CombatState {
public:
    /// @param grid La grille de la rencontre (`LOT-19`), que le combat possède désormais.
    explicit CombatState(BattleGrid grid);

    CombatState(const CombatState&) = delete;
    CombatState& operator=(const CombatState&) = delete;
    CombatState(CombatState&&) = delete;
    CombatState& operator=(CombatState&&) = delete;
    ~CombatState() = default;

    // --- Montage -------------------------------------------------------------------------------

    /**
     * @brief Enrôle un combattant avant le début du combat, et le place si @p anchor est donné.
     *
     * Les identifiants sont attribués **dans l'ordre des enrôlements**, à partir de 1 : c'est
     * l'ordre de la donnée, et le dernier critère de départage de l'initiative. Un placement
     * refusé n'enrôle personne et ne consomme aucun identifiant.
     */
    EnlistResult enlist(CombatantProfile profile,
                        std::optional<GridPosition> anchor = std::nullopt);

    /// @brief Pose un repère d'initiative fixe (actions de repaire à 20, renforts à 0).
    bool addInitiativeMarker(InitiativeMarker marker);

    /// @brief Permet ou interdit la fuite des alliés — recopié de la rencontre.
    void setEscapable(bool escapable) noexcept {
        _escapable = escapable;
    }

    /**
     * @brief Un combat où l'on meurt (le défaut), ou non : la Marque Héroïque des Arènes
     *        (`LOT-137`). Sans mort, personne ne meurt et personne ne fait de jet contre la mort.
     */
    void setLethal(bool lethal) noexcept {
        _lethal = lethal;
    }

    /// @brief Abonne @p listener à @p hook.
    void subscribe(CombatHook hook, CombatListener listener);

    /**
     * @brief Jette l'initiative de chacun et ouvre le premier tour.
     *
     * Les jets se font par identifiant croissant : à graine égale, les mêmes dés tombent sur les
     * mêmes combattants, et le rejeu est exact.
     *
     * @return Faux si le combat n'est plus en montage, ou si l'appel vient d'un abonné.
     */
    bool start(DeterministicRandom& random);

    // --- Lecture -------------------------------------------------------------------------------

    [[nodiscard]] CombatPhase phase() const noexcept {
        return _phase;
    }
    /// @brief Le round en cours, à partir de 1 ; 0 avant le premier.
    [[nodiscard]] int round() const noexcept {
        return _round;
    }
    /// @brief Le combattant dont c'est le tour.
    [[nodiscard]] std::optional<CombatantId> activeCombatant() const noexcept {
        return _active;
    }
    /// @brief L'ordre d'initiative courant, en lecture seule.
    [[nodiscard]] const TurnOrder& turnOrder() const noexcept {
        return _order;
    }
    /// @brief L'issue, une fois le combat terminé.
    [[nodiscard]] std::optional<CombatOutcome> outcome() const noexcept {
        return _outcome;
    }
    /// @brief Vrai si les alliés peuvent fuir ce combat (recopié de la rencontre, `setEscapable`).
    [[nodiscard]] bool escapable() const noexcept {
        return _escapable;
    }
    /// @brief Vrai si l'on peut mourir dans ce combat (`setLethal`).
    [[nodiscard]] bool lethal() const noexcept {
        return _lethal;
    }
    /// @brief Vrai si @p combatant agonise : à terre, pas stabilisé, dans un combat où l'on meurt.
    [[nodiscard]] bool isDying(CombatantId combatant) const;
    /// @brief Le combattant enrôlé d'identifiant @p combatant, sorti compris, ou `nullptr`.
    [[nodiscard]] const Combatant* find(CombatantId combatant) const;
    /// @brief Tous les combattants enrôlés, sortis compris, par identifiant croissant.
    [[nodiscard]] std::vector<CombatantId> combatants() const;

    /// @brief La grille tactique du combat, en lecture seule.
    [[nodiscard]] const BattleGrid& grid() const noexcept {
        return _grid;
    }
    /// @brief La grille, pour ce qui la change en combat : terrain difficile créé, objet détruit.
    [[nodiscard]] BattleGrid& grid() noexcept {
        return _grid;
    }

    /**
     * @brief Les compteurs à portée du combat.
     *
     * La machine vide la portée `Turn` à chaque fin de tour, `Round` à chaque début de round, et
     * `Turn`, `Round` et `Encounter` à la fin du combat. Elle ne touche jamais `Day`.
     */
    [[nodiscard]] ScopedCounters& counters() noexcept {
        return _counters;
    }
    /// @brief Les mêmes compteurs, en lecture : ce que la prévisualisation consulte.
    [[nodiscard]] const ScopedCounters& counters() const noexcept {
        return _counters;
    }

    // --- Le tour -------------------------------------------------------------------------------

    /**
     * @brief L'économie d'action d'un combattant — pour dépenser une réaction hors de son tour, ou
     *        octroyer une ressource. `nullptr` s'il est inconnu.
     */
    [[nodiscard]] ActionEconomy* economy(CombatantId combatant);

    /// @brief Dépense une ressource du combattant actif. Faux s'il n'y a pas de tour, ou pas assez.
    bool spend(std::string_view resource, int amount = 1);

    /**
     * @brief Où le combattant actif peut aller, avec **ce qui reste** de son déplacement.
     * @return Vide s'il n'y a pas de tour ou si le combattant n'est pas placé.
     */
    [[nodiscard]] std::optional<ReachableArea> reachableArea() const;

    /**
     * @brief Déplace le combattant actif jusqu'à @p destination, et paie le chemin.
     *
     * Le chemin est celui de `core::ReachableArea::pathTo`, calculé sur ce qui reste du budget :
     * le déplacement se fractionne, et chaque fraction se paie (`core::ActionEconomy`).
     */
    MoveOutcome move(GridPosition destination);

    /**
     * @brief Termine **explicitement** le tour en cours (`EX-CBT-012`).
     * @return Faux s'il n'y a pas de tour en cours, ou si l'appel vient d'un abonné.
     */
    bool endTurn();

    /**
     * @brief Demande qu'un acteur flottant joue **avant le prochain tour**.
     *
     * Appelé pendant un tour, l'acteur joue dès que ce tour se termine ; appelé depuis un crochet
     * de début de round ou de repère, il joue avant la place suivante. Une fois par round.
     *
     * @return Faux si le combattant n'est pas un acteur flottant debout, ou s'il a déjà joué ce
     *         round-ci.
     */
    bool interject(CombatantId floatingActor);

    /**
     * @brief Annonce qu'une attaque est **déclarée**, avant tout jet
     * (`CombatHook::AttackDeclared`).
     *
     * Le jet et les dégâts sont dans `core::resolveAttack` (`LOT-21`) ; ce crochet est la fenêtre
     * où une posture répond à l'intention, avant que le dé ne tombe.
     *
     * Une cible à terre se déclare : l'achever est une attaque (`LOT-137`) ; un mort, non.
     *
     * @return Faux si l'attaquant n'est pas debout, si la cible est morte ou sortie, ou si le
     *         combat n'est pas en cours.
     */
    bool declareAttack(CombatantId attacker, CombatantId target);

    // --- Entrées, sorties, points de vie -------------------------------------------------------

    /**
     * @brief Fait entrer un combattant **en cours** de combat : un renfort.
     *
     * Il jette son initiative et prend sa place : rangée après la place en cours, il joue ce
     * round-ci ; avant, au round suivant — ce que dit le Manuel, sans cas particulier.
     */
    EnlistResult join(CombatantProfile profile, GridPosition anchor, DeterministicRandom& random);

    /// @brief Fait entrer un combattant à une initiative **imposée** — un renfort « au rang 0 ».
    EnlistResult joinAtInitiative(CombatantProfile profile, GridPosition anchor, int initiative);

    /**
     * @brief Fait sortir un combattant : il quitte la grille et l'ordre, et ne revient pas.
     *
     * S'il était en train de jouer, son tour se termine — le crochet de fin de tour est annoncé,
     * parce que les actions légendaires ne distinguent pas un tour fini d'un tour interrompu.
     */
    WithdrawResult withdraw(CombatantId combatant);

    /**
     * @brief Retire @p amount points de vie, sans descendre sous 0 ; à 0, le combattant est à
     * terre.
     *
     * C'est la **dernière** étape du pipeline de dégâts (`core::DamagePipeline`, `LOT-21`) : les
     * types, les résistances et les réserves sont déjà passés. Annonce `CombatHook::DamageTaken`,
     * puis `CombatHook::CombatantDowned` si le combattant vient de tomber. Sans effet sur un
     * combattant sorti, ou pour un montant non positif.
     */
    void applyDamage(CombatantId combatant, int amount);

    /**
     * @brief Applique plusieurs dégâts **d'un seul coup** : l'issue n'est évaluée qu'une fois.
     *
     * Une boule de feu qui abat le dernier allié et le dernier ennemi est une défaite (voir la
     * classe), et non une victoire ou une défaite selon l'ordre dans lequel on a rangé les cibles.
     */
    void applyDamage(std::span<const HitPointChange> changes);

    /**
     * @brief Rend @p amount points de vie, sans dépasser le maximum ; relève un combattant à
     *        terre, dont les jets contre la mort repartent de zéro. Les réserves ne se soignent
     *        pas ; un mort ne récupère rien (Manuel, « Soins »).
     *
     * Relevé, il reste **à terre** jusqu'au début de son tour, où il se relève.
     */
    void heal(CombatantId combatant, int amount);

    /**
     * @brief Note le jet contre la mort de @p combatant (Manuel, « Jets de sauvegarde contre la
     *        mort ») : 10 ou plus, un succès ; un 1 naturel, deux échecs ; un 20 naturel, 1 point
     *        de vie. Trois succès stabilisent, trois échecs tuent.
     *
     * @param combatant Un combattant qui agonise (`isDying`).
     * @param natural Le d20 tel qu'il est tombé.
     * @param total Le d20 et ce qui s'y ajoute (*bénédiction*).
     */
    DeathSaveOutcome recordDeathSave(CombatantId combatant, int natural, int total);

    /**
     * @brief Stabilise un combattant à terre (*épargner les mourants*) : il ne jette plus, et
     *        reste inconscient jusqu'à ce qu'on le soigne ; de nouveaux dégâts le refont agoniser.
     * @return Faux s'il n'est pas à terre.
     */
    bool stabilize(CombatantId combatant);

    /**
     * @brief Ramène un mort avec @p hitPoints points de vie (*revigorer*) : il se relève, à terre
     *        jusqu'à son tour.
     * @return Faux s'il n'est pas mort.
     */
    bool revive(CombatantId combatant, int hitPoints);

    /**
     * @brief Donne une réserve de points de vie.
     *
     * Une réserve qui ne se cumule pas remplace celle de même source si elle est plus grande, et
     * est ignorée sinon ; une réserve qui se cumule s'ajoute à la pile. Ne relève personne.
     * @return Faux si le combattant est inconnu ou sorti, ou si la réserve est ignorée.
     */
    bool grantReserve(CombatantId combatant, HitPointReserve reserve);

    /// @brief Les réserves d'un combattant, pour que le pipeline les consomme. `nullptr` s'il est
    /// inconnu.
    [[nodiscard]] std::vector<HitPointReserve>* reserves(CombatantId combatant);

    /**
     * @brief Le `core::Mover` d'un combattant, droit de passage compris.
     *
     * Manuel des Joueurs, « Se déplacer au milieu d'autres créatures » (PDF p. 193) : on traverse
     * la case d'une créature **non hostile** — ici, d'un allié —, et celle d'une créature hostile
     * seulement si elle a **deux catégories de taille** de plus ou de moins. Il désigne l'état par
     * son adresse, et ne vit pas plus longtemps que lui.
     */
    [[nodiscard]] Mover moverFor(CombatantId combatant) const;

    /**
     * @brief Change la manière dont @p combatant se déplace et son budget par tour — le *vol*
     *        d'un sort (`LOT-133`), et sa fin.
     *
     * Ce qu'il a déjà marché ce tour reste dépensé : il gagne la différence des budgets, ou la
     * perd. Un octroi en cours (*se précipiter*) est gardé.
     *
     * @return Faux si le combattant est inconnu.
     */
    bool setLocomotion(CombatantId combatant, Locomotion locomotion, int movement);

private:
    /// Tient la profondeur d'appel : les conséquences ne se règlent qu'en sortant de l'appel
    /// extérieur.
    class Operation;

    [[nodiscard]] Combatant* findMutable(CombatantId combatant);
    [[nodiscard]] bool reentrant() const noexcept {
        return _depth > 0;
    }
    [[nodiscard]] bool running() const noexcept {
        return _phase != CombatPhase::Setup && _phase != CombatPhase::Ended;
    }

    EnlistResult admit(CombatantProfile profile, std::optional<GridPosition> anchor);
    void rollInitiative(Combatant& combatant, DeterministicRandom& random);
    void takeFixedInitiative(Combatant& combatant, int initiative);
    void damage(const HitPointChange& change);
    /// Tue @p combatant et annonce `CombatHook::CombatantDied`.
    void kill(Combatant& combatant);
    /// Relève @p combatant avec des points de vie : debout, compteur remis à zéro, à terre.
    static void standUp(Combatant& combatant);
    void dispatch(const CombatEvent& event);

    void settle();
    [[nodiscard]] bool reachEndIfDecided();
    /// Ouvre le tour du premier acteur intercalé encore debout ; faux s'il n'y en a pas.
    [[nodiscard]] bool startInterjection();
    void step();
    void startTurn(CombatantId combatant);
    void finishTurn();
    [[nodiscard]] bool canPassThrough(CombatantId mover, CombatantId other) const;

    BattleGrid _grid;
    /// Par identifiant croissant : l'identifiant vaut sa position plus un.
    std::vector<Combatant> _combatants;
    TurnOrder _order;
    std::vector<std::pair<CombatHook, CombatListener>> _listeners;
    ScopedCounters _counters;

    CombatPhase _phase = CombatPhase::Setup;
    int _round = 0;
    std::optional<CombatantId> _active;
    std::optional<CombatOutcome> _outcome;
    bool _escapable = true;
    bool _lethal = true;

    /// La dernière place parcourue dans le round.
    std::optional<TurnSlot> _cursor;
    /// Les tours d'acteurs flottants à jouer avant la place suivante.
    std::deque<CombatantId> _interjections;
    /// Vrai quand le round a déjà proposé leur tour aux acteurs flottants qui n'avaient pas joué.
    bool _closingRound = false;
    bool _turnThisRound = false;
    /// Vrai quand la machine doit chercher la place suivante.
    bool _advancePending = false;
    int _depth = 0;
};

/// @brief Un membre du groupe à engager, et sa case.
struct PartyMember {
    CombatantProfile profile;
    GridPosition position;
};

/// @brief Un combattant que le montage n'a pas pu poser, et pourquoi.
struct MountRefusal {
    /// L'identifiant de créature, ou le nom du membre du groupe.
    std::string who;
    GridPosition position;
    /// La raison du refus de la grille ; vide si la créature est **inconnue** du bestiaire.
    std::optional<PlacementResult> placement;
};

/// @brief Ce que le montage a donné.
struct EncounterMount {
    std::vector<CombatantId> allies;
    std::vector<CombatantId> enemies;
    std::vector<MountRefusal> refusals;
};

/**
 * @brief Monte la rencontre @p run sur @p combat : le groupe, puis les créatures, dans cet ordre.
 *
 * Les placements viennent de `core::beginEncounter` (`LOT-18`), qui ne les a pas vérifiés : c'est
 * ici qu'une case voulue tombe dans un mur, et le montage **le dit** plutôt que de déplacer la
 * créature d'office — une formation mal écrite est une information pour l'auteur. Un combattant
 * refusé n'est pas enrôlé : un combattant sans case ne peut pas combattre sur la grille.
 *
 * @param combat   Un combat encore en montage.
 * @param run      La rencontre engagée : ses placements, et si l'on peut la fuir.
 * @param bestiary Le bestiaire où chercher chaque créature de la rencontre.
 * @param party    Le groupe à engager, chaque membre avec sa case.
 * @return Les alliés et ennemis enrôlés, et chaque refus avec sa raison.
 */
[[nodiscard]] EncounterMount mountEncounter(CombatState& combat, const EncounterRun& run,
                                            const Bestiary& bestiary,
                                            std::span<const PartyMember> party);

}  // namespace core
