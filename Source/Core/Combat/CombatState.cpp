// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/CombatState.h"

#include <algorithm>
#include <array>
#include <cstdlib>

#include "Core/Rpg/Ability.h"
#include "Core/Rpg/Bestiary.h"
#include "Core/Rpg/CharacterSheet.h"

namespace core {

// Tient la profondeur d'appel. En sortant de l'appel **extérieur** — celui qui ne vient d'aucun
// abonné —, la machine règle ce que l'appel a changé.
class CombatState::Operation {
public:
    explicit Operation(CombatState& state) : _state(state) {
        ++_state._depth;
    }
    ~Operation() {
        if (--_state._depth == 0) {
            _state.settle();
        }
    }
    Operation(const Operation&) = delete;
    Operation& operator=(const Operation&) = delete;
    Operation(Operation&&) = delete;
    Operation& operator=(Operation&&) = delete;

private:
    CombatState& _state;
};

CombatantProfile profileFor(const CharacterSheet& sheet, CombatSide side) {
    return {
        .name = sheet.name,
        .side = side,
        .maximumHitPoints = sheet.maximumHitPoints,
        .currentHitPoints = sheet.currentHitPoints,
        .dexterity = sheet.ability(Ability::Dexterity),
        .initiativeModifier = sheet.modifier(Ability::Dexterity),
        .initiativeStance = RollStance::Normal,
        .movement = movementBudget(sheet),
        .locomotion = Locomotion::Walk,
        .size = CreatureSize::Medium,
        .floating = false,
        .armorClass = sheet.armorClass,
        .damageTraits = {},
    };
}

CombatantProfile profileFor(const Creature& creature, CombatSide side) {
    const bool flies = creature.speed.fly.has_value() && *creature.speed.fly > creature.speed.walk;
    const Locomotion locomotion = flies ? Locomotion::Fly : Locomotion::Walk;
    const int dexterity = creature.ability(Ability::Dexterity);
    return {
        .name = creature.name,
        .side = side,
        .maximumHitPoints = creature.hitPoints,
        .currentHitPoints = creature.hitPoints,
        .dexterity = dexterity,
        .initiativeModifier = abilityModifier(dexterity),
        .initiativeStance = RollStance::Normal,
        .movement = movementBudget(creature, locomotion),
        .locomotion = locomotion,
        .size = creature.size,
        .floating = false,
        .armorClass = creature.armorClass,
        .damageTraits = damageTraitsFor(creature),
    };
}

CombatState::CombatState(BattleGrid grid) : _grid(std::move(grid)) {}

// --- Montage ---------------------------------------------------------------------------------

EnlistResult CombatState::enlist(CombatantProfile profile, std::optional<GridPosition> anchor) {
    if (_phase != CombatPhase::Setup) {
        return {.combatant = std::nullopt, .placement = PlacementResult::InvalidCombatant};
    }
    return admit(std::move(profile), anchor);
}

bool CombatState::addInitiativeMarker(InitiativeMarker marker) {
    return _order.addMarker(std::move(marker));
}

void CombatState::subscribe(CombatHook hook, CombatListener listener) {
    _listeners.emplace_back(hook, std::move(listener));
}

bool CombatState::start(DeterministicRandom& random) {
    if (reentrant() || _phase != CombatPhase::Setup) {
        return false;
    }
    Operation operation(*this);
    for (Combatant& combatant : _combatants) {
        if (!combatant.profile.floating) {
            rollInitiative(combatant, random);
        }
    }
    _phase = CombatPhase::Starting;
    _advancePending = true;
    dispatch({.hook = CombatHook::BeforeFirstTurn,
              .round = 0,
              .combatant = std::nullopt,
              .target = std::nullopt,
              .marker = {}});
    return true;
}

// --- Lecture ---------------------------------------------------------------------------------

namespace {

// L'identifiant vaut sa position plus un : la même recherche, constante ou non.
template <typename Combatants>
auto* findIn(Combatants& combatants, CombatantId combatant) {
    const auto index = static_cast<std::size_t>(combatant);
    return index == 0 || index > combatants.size() ? nullptr : &combatants[index - 1];
}

}  // namespace

const Combatant* CombatState::find(CombatantId combatant) const {
    return findIn(_combatants, combatant);
}

Combatant* CombatState::findMutable(CombatantId combatant) {
    return findIn(_combatants, combatant);
}

std::vector<CombatantId> CombatState::combatants() const {
    std::vector<CombatantId> ids;
    ids.reserve(_combatants.size());
    for (const Combatant& combatant : _combatants) {
        ids.push_back(combatant.id);
    }
    return ids;
}

// --- Le tour ---------------------------------------------------------------------------------

ActionEconomy* CombatState::economy(CombatantId combatant) {
    Combatant* found = findMutable(combatant);
    return found == nullptr ? nullptr : &found->economy;
}

bool CombatState::spend(std::string_view resource, int amount) {
    if (_phase != CombatPhase::TurnActive || !_active.has_value()) {
        return false;
    }
    return findMutable(*_active)->economy.spend(resource, amount);
}

std::optional<ReachableArea> CombatState::reachableArea() const {
    if (_phase != CombatPhase::TurnActive || !_active.has_value() ||
        !_grid.positionOf(*_active).has_value()) {
        return std::nullopt;
    }
    return ReachableArea(_grid, moverFor(*_active),
                         find(*_active)->economy.remaining(MOVEMENT_RESOURCE));
}

MoveOutcome CombatState::move(GridPosition destination) {
    if (_phase != CombatPhase::TurnActive || !_active.has_value()) {
        return {.result = MoveResult::NoActiveTurn, .path = {}};
    }
    const CombatantId mover = *_active;
    if (!_grid.positionOf(mover).has_value()) {
        return {.result = MoveResult::NotPlaced, .path = {}};
    }
    std::optional<Path> path = reachableArea()->pathTo(destination);
    if (!path.has_value()) {
        return {.result = MoveResult::Unreachable, .path = {}};
    }
    Operation operation(*this);
    Combatant* combatant = findMutable(mover);
    // `pathTo` a vérifié que le chemin tient dans ce qui reste, et que l'emprise tient à
    // l'arrivée : ni la dépense ni le déplacement ne peuvent être refusés ici.
    static_cast<void>(combatant->economy.spend(MOVEMENT_RESOURCE, path->cost));
    static_cast<void>(_grid.moveTo(mover, destination, combatant->profile.locomotion));
    return {.result = MoveResult::Moved, .path = std::move(*path)};
}

bool CombatState::endTurn() {
    if (reentrant() || _phase != CombatPhase::TurnActive || !_active.has_value()) {
        return false;
    }
    Operation operation(*this);
    finishTurn();
    _advancePending = true;
    return true;
}

bool CombatState::interject(CombatantId floatingActor) {
    Combatant* actor = findMutable(floatingActor);
    // Pas avant le premier round : le drapeau « a joué ce round-ci » serait remis à zéro au début
    // du round 1, et l'acteur jouerait deux fois.
    if (!running() || _round == 0 || actor == nullptr || !actor->profile.floating ||
        actor->status != CombatantStatus::Standing || actor->actedThisRound) {
        return false;
    }
    actor->actedThisRound = true;
    _interjections.push_back(floatingActor);
    return true;
}

bool CombatState::declareAttack(CombatantId attacker, CombatantId target) {
    const Combatant* from = find(attacker);
    const Combatant* to = find(target);
    if (!running() || from == nullptr || to == nullptr ||
        from->status != CombatantStatus::Standing || to->status != CombatantStatus::Standing) {
        return false;
    }
    Operation operation(*this);
    dispatch({.hook = CombatHook::AttackDeclared,
              .round = _round,
              .combatant = attacker,
              .target = target,
              .marker = {}});
    return true;
}

// --- Entrées, sorties, points de vie ---------------------------------------------------------

EnlistResult CombatState::join(CombatantProfile profile, GridPosition anchor,
                               DeterministicRandom& random) {
    if (!running()) {
        return {.combatant = std::nullopt, .placement = PlacementResult::InvalidCombatant};
    }
    Operation operation(*this);
    EnlistResult result = admit(std::move(profile), anchor);
    if (!result.combatant.has_value()) {
        return result;
    }
    Combatant& joined = _combatants.back();
    if (!joined.profile.floating) {
        rollInitiative(joined, random);
    }
    dispatch({.hook = CombatHook::CombatantJoined,
              .round = _round,
              .combatant = result.combatant,
              .target = std::nullopt,
              .marker = {}});
    return result;
}

EnlistResult CombatState::joinAtInitiative(CombatantProfile profile, GridPosition anchor,
                                           int initiative) {
    if (!running()) {
        return {.combatant = std::nullopt, .placement = PlacementResult::InvalidCombatant};
    }
    Operation operation(*this);
    EnlistResult result = admit(std::move(profile), anchor);
    if (!result.combatant.has_value()) {
        return result;
    }
    Combatant& joined = _combatants.back();
    if (!joined.profile.floating) {
        takeFixedInitiative(joined, initiative);
    }
    dispatch({.hook = CombatHook::CombatantJoined,
              .round = _round,
              .combatant = result.combatant,
              .target = std::nullopt,
              .marker = {}});
    return result;
}

WithdrawResult CombatState::withdraw(CombatantId combatant) {
    Combatant* leaving = findMutable(combatant);
    if (!running() || leaving == nullptr || leaving->status == CombatantStatus::Withdrawn) {
        return WithdrawResult::NotInCombat;
    }
    if (leaving->profile.side == CombatSide::Allies && !_escapable) {
        return WithdrawResult::NotEscapable;
    }
    Operation operation(*this);
    leaving->status = CombatantStatus::Withdrawn;
    static_cast<void>(_grid.remove(combatant));
    static_cast<void>(_order.remove(combatant));
    std::erase(_interjections, combatant);
    dispatch({.hook = CombatHook::CombatantLeft,
              .round = _round,
              .combatant = combatant,
              .target = std::nullopt,
              .marker = {}});
    return WithdrawResult::Withdrawn;
}

void CombatState::applyDamage(CombatantId combatant, int amount) {
    Operation operation(*this);
    damage({.target = combatant, .amount = amount, .critical = false});
}

void CombatState::applyDamage(std::span<const HitPointChange> changes) {
    Operation operation(*this);
    for (const HitPointChange& change : changes) {
        damage(change);
    }
}

void CombatState::heal(CombatantId combatant, int amount) {
    Combatant* target = findMutable(combatant);
    if (target == nullptr || target->status == CombatantStatus::Withdrawn || amount <= 0) {
        return;
    }
    Operation operation(*this);
    target->profile.currentHitPoints =
        std::min(target->profile.maximumHitPoints, target->profile.currentHitPoints + amount);
    if (target->profile.currentHitPoints > 0) {
        target->status = CombatantStatus::Standing;
    }
}

bool CombatState::grantReserve(CombatantId combatant, HitPointReserve reserve) {
    Combatant* target = findMutable(combatant);
    if (target == nullptr || target->status == CombatantStatus::Withdrawn || reserve.amount <= 0) {
        return false;
    }
    if (!reserve.stacks) {
        const auto same = std::ranges::find_if(target->reserves, [&](const HitPointReserve& r) {
            return !r.stacks && r.source == reserve.source;
        });
        if (same != target->reserves.end()) {
            if (same->amount >= reserve.amount) {
                return false;
            }
            same->amount = reserve.amount;
            return true;
        }
    }
    target->reserves.push_back(std::move(reserve));
    return true;
}

std::vector<HitPointReserve>* CombatState::reserves(CombatantId combatant) {
    Combatant* target = findMutable(combatant);
    return target == nullptr ? nullptr : &target->reserves;
}

Mover CombatState::moverFor(CombatantId combatant) const {
    const Combatant* found = find(combatant);
    return {.combatant = combatant,
            .locomotion = found == nullptr ? Locomotion::Walk : found->profile.locomotion,
            .canPassThrough = [this, combatant](CombatantId other) {
                return canPassThrough(combatant, other);
            }};
}

// --- Mécanique interne -----------------------------------------------------------------------

EnlistResult CombatState::admit(CombatantProfile profile, std::optional<GridPosition> anchor) {
    const CombatantId id{static_cast<std::uint32_t>(_combatants.size() + 1)};
    if (anchor.has_value()) {
        const PlacementResult placed =
            _grid.place(id, *anchor, footprintSide(profile.size), profile.locomotion);
        if (placed != PlacementResult::Placed) {
            return {.combatant = std::nullopt, .placement = placed};
        }
    }
    const CombatantStatus status =
        profile.currentHitPoints > 0 ? CombatantStatus::Standing : CombatantStatus::Down;
    const int movement = profile.movement;
    _combatants.push_back({.id = id,
                           .profile = std::move(profile),
                           .status = status,
                           .economy = ActionEconomy::standard(movement),
                           .initiativeRoll = std::nullopt,
                           .actedThisRound = false,
                           .reserves = {}});
    return {.combatant = id, .placement = PlacementResult::Placed};
}

void CombatState::rollInitiative(Combatant& combatant, DeterministicRandom& random) {
    // L'initiative est un test de Dextérité (Manuel des Joueurs, « Initiative ») : il passe par le
    // même jet que tous les autres, et reste restituable.
    const std::array<Modifier, 1> modifiers{
        Modifier{.source = "Initiative", .value = combatant.profile.initiativeModifier}};
    CheckResult roll = rollCheck(0, modifiers, combatant.profile.initiativeStance, random);
    static_cast<void>(_order.add({.combatant = combatant.id,
                                  .total = roll.total,
                                  .modifier = combatant.profile.initiativeModifier,
                                  .dexterity = combatant.profile.dexterity,
                                  .side = combatant.profile.side}));
    combatant.initiativeRoll = std::move(roll);
}

void CombatState::takeFixedInitiative(Combatant& combatant, int initiative) {
    static_cast<void>(_order.add({.combatant = combatant.id,
                                  .total = initiative,
                                  .modifier = combatant.profile.initiativeModifier,
                                  .dexterity = combatant.profile.dexterity,
                                  .side = combatant.profile.side}));
}

void CombatState::damage(const HitPointChange& change) {
    Combatant* target = findMutable(change.target);
    if (target == nullptr || target->status == CombatantStatus::Withdrawn || change.amount <= 0) {
        return;
    }
    const int before = target->profile.currentHitPoints;
    target->profile.currentHitPoints = std::max(0, before - change.amount);
    const bool fell =
        target->profile.currentHitPoints == 0 && target->status != CombatantStatus::Down;
    if (target->profile.currentHitPoints == 0) {
        target->status = CombatantStatus::Down;
    }
    CombatEvent event{.hook = CombatHook::DamageTaken,
                      .round = _round,
                      .combatant = change.target,
                      .target = std::nullopt,
                      .marker = {},
                      .amount = change.amount,
                      .hitPointsBefore = before,
                      .hitPointsAfter = target->profile.currentHitPoints,
                      .maximumHitPoints = target->profile.maximumHitPoints,
                      .overflow = std::max(0, change.amount - before),
                      .critical = change.critical};
    // L'abonne peut enroler un renfort et reallouer la liste : `target` n'est plus lu apres.
    dispatch(event);
    if (fell) {
        event.hook = CombatHook::CombatantDowned;
        dispatch(event);
    }
}

void CombatState::dispatch(const CombatEvent& event) {
    ++_depth;
    // Un abonné peut s'abonner à son tour : il sera averti à la prochaine annonce, pas à celle-ci.
    // L'abonné est copié avant l'appel, parce qu'un nouvel abonnement peut réallouer la liste.
    const std::size_t count = _listeners.size();
    for (std::size_t index = 0; index < count; ++index) {
        if (_listeners[index].first != event.hook) {
            continue;
        }
        const CombatListener listener = _listeners[index].second;
        listener(*this, event);
    }
    --_depth;
}

void CombatState::settle() {
    // Dans cet ordre, une conséquence à la fois : la fin du combat prime tout ; un tour que son
    // combattant ne peut plus jouer se termine ; puis la machine cherche la place suivante.
    while (running()) {
        if (reachEndIfDecided()) {
            return;
        }
        if (_active.has_value() && find(*_active)->status != CombatantStatus::Standing) {
            finishTurn();
            _advancePending = true;
            continue;
        }
        if (_advancePending && !_active.has_value()) {
            step();
            continue;
        }
        return;
    }
}

bool CombatState::reachEndIfDecided() {
    bool allyStanding = false;
    bool allyWithdrawn = false;
    bool enemyStanding = false;
    for (const Combatant& combatant : _combatants) {
        const bool standing = combatant.status == CombatantStatus::Standing;
        if (combatant.profile.side == CombatSide::Allies) {
            allyStanding = allyStanding || standing;
            allyWithdrawn = allyWithdrawn || combatant.status == CombatantStatus::Withdrawn;
        } else {
            enemyStanding = enemyStanding || standing;
        }
    }
    if (allyStanding && enemyStanding) {
        return false;
    }
    // Le camp allié d'abord : si les deux tombent ensemble, c'est une défaite.
    if (!allyStanding) {
        _outcome = allyWithdrawn ? CombatOutcome::Flight : CombatOutcome::Defeat;
    } else {
        _outcome = CombatOutcome::Victory;
    }
    _phase = CombatPhase::Ended;
    _active.reset();
    _interjections.clear();
    _advancePending = false;
    dispatch({.hook = CombatHook::CombatEnded,
              .round = _round,
              .combatant = std::nullopt,
              .target = std::nullopt,
              .marker = {}});
    _counters.clear(CounterScope::Turn);
    _counters.clear(CounterScope::Round);
    _counters.clear(CounterScope::Encounter);
    return true;
}

bool CombatState::startInterjection() {
    while (!_interjections.empty()) {
        const CombatantId actor = _interjections.front();
        _interjections.pop_front();
        const Combatant* found = find(actor);
        if (found != nullptr && found->status == CombatantStatus::Standing) {
            startTurn(actor);
            return true;
        }
    }
    return false;
}

void CombatState::step() {
    _advancePending = false;

    if (startInterjection()) {
        return;
    }

    std::optional<TurnSlot> next;
    if (_round > 0) {
        next = _cursor.has_value() ? _order.slotAfter(*_cursor) : _order.firstSlot();
    }

    if (!next.has_value()) {
        // Le round est fini. Les acteurs flottants qui n'ont pas choisi leur moment jouent
        // maintenant : un acteur qui ne se décide pas ne perd pas son tour.
        if (_round > 0 && !_closingRound) {
            _closingRound = true;
            for (Combatant& combatant : _combatants) {
                if (combatant.profile.floating && combatant.status == CombatantStatus::Standing &&
                    !combatant.actedThisRound) {
                    combatant.actedThisRound = true;
                    _interjections.push_back(combatant.id);
                }
            }
            _advancePending = true;
            return;
        }
        // Un round sans aucun tour suppose que plus personne n'est debout, et l'issue l'a déjà
        // dit. S'arrêter plutôt que d'ouvrir des rounds vides à l'infini.
        if (_round > 0 && !_turnThisRound) {
            return;
        }
        _closingRound = false;
        _turnThisRound = false;
        _cursor.reset();
        ++_round;
        for (Combatant& combatant : _combatants) {
            combatant.actedThisRound = false;
        }
        _counters.clear(CounterScope::Round);
        _phase = CombatPhase::RoundStart;
        _advancePending = true;
        dispatch({.hook = CombatHook::RoundStart,
                  .round = _round,
                  .combatant = std::nullopt,
                  .target = std::nullopt,
                  .marker = {}});
        return;
    }

    _cursor = next;
    _advancePending = true;
    if (!next->entry.has_value()) {
        _phase = CombatPhase::RoundStart;
        dispatch({.hook = CombatHook::InitiativeCount,
                  .round = _round,
                  .combatant = std::nullopt,
                  .target = std::nullopt,
                  .marker = next->marker.name});
        return;
    }
    const Combatant* found = find(next->entry->combatant);
    if (found != nullptr && found->status == CombatantStatus::Standing) {
        _advancePending = false;
        startTurn(found->id);
    }
}

void CombatState::startTurn(CombatantId combatant) {
    Combatant* actor = findMutable(combatant);
    _active = combatant;
    _turnThisRound = true;
    // La réaction revient ici, au début du tour de son porteur — jamais à la fin du tour d'un
    // autre (`EX-CBT-011`).
    actor->economy.refresh();
    _phase = CombatPhase::TurnActive;
    dispatch({.hook = CombatHook::TurnStart,
              .round = _round,
              .combatant = combatant,
              .target = std::nullopt,
              .marker = {}});
}

void CombatState::finishTurn() {
    const CombatantId ending = *_active;
    _phase = CombatPhase::TurnEnd;
    dispatch({.hook = CombatHook::TurnEnd,
              .round = _round,
              .combatant = ending,
              .target = std::nullopt,
              .marker = {}});
    _counters.clear(CounterScope::Turn);
    _active.reset();
}

bool CombatState::canPassThrough(CombatantId mover, CombatantId other) const {
    const Combatant* self = find(mover);
    const Combatant* crossed = find(other);
    if (self == nullptr || crossed == nullptr) {
        return false;
    }
    if (self->profile.side == crossed->profile.side) {
        return true;
    }
    const int gap =
        std::abs(static_cast<int>(self->profile.size) - static_cast<int>(crossed->profile.size));
    return gap >= 2;
}

// --- Montage d'une rencontre -----------------------------------------------------------------

EncounterMount mountEncounter(CombatState& combat, const EncounterRun& run,
                              const Bestiary& bestiary, std::span<const PartyMember> party) {
    EncounterMount mount;
    combat.setEscapable(run.escapable);
    for (const PartyMember& member : party) {
        const EnlistResult enlisted = combat.enlist(member.profile, member.position);
        if (enlisted.combatant.has_value()) {
            mount.allies.push_back(*enlisted.combatant);
        } else {
            mount.refusals.push_back({.who = member.profile.name,
                                      .position = member.position,
                                      .placement = enlisted.placement});
        }
    }
    for (const CombatantPlacement& placement : run.placements) {
        const Creature* creature = bestiary.find(placement.creatureId);
        if (creature == nullptr) {
            mount.refusals.push_back({.who = placement.creatureId,
                                      .position = placement.position,
                                      .placement = std::nullopt});
            continue;
        }
        const EnlistResult enlisted =
            combat.enlist(profileFor(*creature, CombatSide::Enemies), placement.position);
        if (enlisted.combatant.has_value()) {
            mount.enemies.push_back(*enlisted.combatant);
        } else {
            mount.refusals.push_back({.who = placement.creatureId,
                                      .position = placement.position,
                                      .placement = enlisted.placement});
        }
    }
    return mount;
}

}  // namespace core
