// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/Arena.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <tuple>
#include <utility>
#include <variant>

#include "Core/Combat/BattleGrid.h"
#include "Core/Combat/CombatCounters.h"
#include "Core/Combat/Flanking.h"
#include "Core/Rpg/Ability.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Spell.h"

namespace core {
namespace {

[[nodiscard]] std::optional<CombatSide> campDepuis(const PropertyMap& proprietes) {
    const auto trouve = proprietes.find(std::string(ARENA_SIDE_PROPERTY));
    if (trouve == proprietes.end()) {
        return std::nullopt;
    }
    const std::string* texte = std::get_if<std::string>(&trouve->second);
    if (texte == nullptr) {
        return std::nullopt;
    }
    if (*texte == "allies") {
        return CombatSide::Allies;
    }
    if (*texte == "enemies") {
        return CombatSide::Enemies;
    }
    return std::nullopt;
}

[[nodiscard]] int rangDepuis(const PropertyMap& proprietes) {
    const auto trouve = proprietes.find(std::string(ARENA_RANK_PROPERTY));
    if (trouve == proprietes.end()) {
        return 0;
    }
    if (const std::int64_t* entier = std::get_if<std::int64_t>(&trouve->second)) {
        return static_cast<int>(*entier);
    }
    if (const double* reel = std::get_if<double>(&trouve->second)) {
        return static_cast<int>(*reel);
    }
    return 0;
}

[[nodiscard]] std::string_view nomDuCrochet(CombatHook crochet) noexcept {
    switch (crochet) {
        case CombatHook::BeforeFirstTurn:
            return "avant le premier tour";
        case CombatHook::RoundStart:
            return "round";
        case CombatHook::InitiativeCount:
            return "repere";
        case CombatHook::TurnStart:
            return "debut du tour";
        case CombatHook::TurnEnd:
            return "fin du tour";
        case CombatHook::AttackDeclared:
            return "attaque declaree";
        case CombatHook::DamageTaken:
            return "degats";
        case CombatHook::CombatantDowned:
            return "a terre";
        case CombatHook::CombatantJoined:
            return "entree";
        case CombatHook::CombatantLeft:
            return "sortie";
        case CombatHook::CombatEnded:
            return "issue";
    }
    return "?";
}

[[nodiscard]] std::string_view nomDeLIssue(CombatOutcome issue) noexcept {
    switch (issue) {
        case CombatOutcome::Victory:
            return "victoire";
        case CombatOutcome::Flight:
            return "fuite";
        case CombatOutcome::Defeat:
            return "defaite";
    }
    return "?";
}

// La derniere case ou l'on peut se tenir, au plus tard @p sortie : on ne s'arrete pas sur la case
// d'un allie qu'on traverse. Zero si aucune avant.
[[nodiscard]] std::size_t derniereCaseTenable(const ReachableArea& zone,
                                              const std::vector<GridPosition>& cases,
                                              std::size_t sortie) {
    std::size_t arret = sortie;
    while (arret > 0 && !zone.canEndAt(cases[arret])) {
        --arret;
    }
    return arret;
}

}  // namespace

// --- Points d'entree --------------------------------------------------------------------------

std::vector<ArenaEntryPoint> arenaEntryPoints(const Level& level) {
    std::vector<ArenaEntryPoint> entrees;
    for (const MapEntity& entite : level.entities()) {
        if (entite.type != ARENA_ENTRY_ENTITY_TYPE) {
            continue;
        }
        const std::optional<CombatSide> camp = campDepuis(entite.properties);
        if (!camp.has_value()) {
            continue;
        }
        entrees.push_back(
            {.side = *camp, .rank = rangDepuis(entite.properties), .position = entite.position});
    }
    std::ranges::sort(entrees, [](const ArenaEntryPoint& a, const ArenaEntryPoint& b) {
        return std::tuple(a.side, a.rank, a.position.row, a.position.column) <
               std::tuple(b.side, b.rank, b.position.row, b.position.column);
    });
    return entrees;
}

// --- Session ----------------------------------------------------------------------------------

ArenaSession::ArenaSession(Level level)
    : _level(std::move(level)), _combat(std::make_unique<CombatState>(BattleGrid(_level))) {}

void ArenaSession::record(std::string line) {
    _journal.push_back(std::move(line));
}

void ArenaSession::subscribe() {
    const auto nommer = [this](std::optional<CombatantId> id) -> std::string {
        if (!id.has_value()) {
            return "-";
        }
        const Combatant* c = _combat->find(*id);
        return (c == nullptr ? std::string("?") : c->profile.name) + " #" +
               std::to_string(static_cast<std::uint32_t>(*id));
    };
    // Les degats subis ne font pas une ligne a eux seuls : l'attaque qui les inflige les ecrit
    // deja, etape par etape. La chute, elle, en fait une.
    constexpr std::array<CombatHook, 10> CROCHETS{
        CombatHook::BeforeFirstTurn, CombatHook::RoundStart,      CombatHook::InitiativeCount,
        CombatHook::TurnStart,       CombatHook::TurnEnd,         CombatHook::AttackDeclared,
        CombatHook::CombatantDowned, CombatHook::CombatantJoined, CombatHook::CombatantLeft,
        CombatHook::CombatEnded};
    for (const CombatHook crochet : CROCHETS) {
        _combat->subscribe(crochet, [this, nommer](CombatState& etat, const CombatEvent& e) {
            std::string ligne = std::string(nomDuCrochet(e.hook));
            switch (e.hook) {
                case CombatHook::RoundStart:
                    ligne += " " + std::to_string(e.round);
                    break;
                case CombatHook::InitiativeCount:
                    ligne += " " + e.marker;
                    break;
                case CombatHook::TurnStart:
                    // L'esquive dure « jusqu'au debut de votre prochain tour ».
                    if (e.combatant.has_value()) {
                        _dodging.erase(*e.combatant);
                    }
                    ligne += " " + nommer(e.combatant);
                    break;
                case CombatHook::TurnEnd:
                    // Se desengager vaut « jusqu'a la fin du tour ».
                    if (e.combatant.has_value()) {
                        _disengaged.erase(*e.combatant);
                    }
                    ligne += " " + nommer(e.combatant);
                    break;
                case CombatHook::CombatantDowned:
                case CombatHook::DamageTaken:
                case CombatHook::CombatantJoined:
                case CombatHook::CombatantLeft:
                    ligne += " " + nommer(e.combatant);
                    break;
                case CombatHook::AttackDeclared:
                    ligne += " " + nommer(e.combatant) + " -> " + nommer(e.target);
                    break;
                case CombatHook::CombatEnded:
                    ligne += " : ";
                    ligne += etat.outcome().has_value() ? nomDeLIssue(*etat.outcome()) : "?";
                    break;
                case CombatHook::BeforeFirstTurn:
                    break;
            }
            record(std::move(ligne));
            if (e.hook == CombatHook::CombatEnded) {
                restoreAll();
            }
        });
    }
}

void ArenaSession::restoreAll() {
    if (_bout.lethal) {
        return;
    }
    // La Marque Heroique releve tout le monde, y compris ceux qui sont tombes : personne ne meurt
    // dans une arene, et un affrontement se rejoue autant de fois qu'on veut.
    for (const CombatantId id : _combat->combatants()) {
        const Combatant* c = _combat->find(id);
        if (c != nullptr && c->status != CombatantStatus::Withdrawn) {
            _combat->heal(id, c->profile.maximumHitPoints - c->profile.currentHitPoints);
        }
    }
    record("marque heroique : tous releves");
}

ArenaMount ArenaSession::mount(const ArenaBout& bout) {
    _bout = bout;
    _random = DeterministicRandom(bout.seed);
    _combat = std::make_unique<CombatState>(BattleGrid(_level));
    _attacks.clear();
    _capacities.clear();
    _spells.clear();
    _behaviors.clear();
    // Les choix de reaction du joueur survivent au rejeu : les memes identifiants, le meme choix.
    _dodging.clear();
    _disengaged.clear();
    _journal.clear();
    subscribe();
    _combat->setEscapable(bout.escapable);

    ArenaMount montage;
    std::vector<ArenaEntryPoint> entrees = arenaEntryPoints(_level);
    const auto prochaineEntree = [&](CombatSide camp) -> std::optional<GridPosition> {
        for (auto it = entrees.begin(); it != entrees.end(); ++it) {
            if (it->side == camp) {
                const GridPosition position = it->position;
                entrees.erase(it);
                return position;
            }
        }
        return std::nullopt;
    };

    for (const ArenaContestant& concurrent : bout.contestants) {
        const std::optional<GridPosition> place = concurrent.position.has_value()
                                                      ? concurrent.position
                                                      : prochaineEntree(concurrent.profile.side);
        if (!place.has_value()) {
            montage.refusals.push_back({.who = concurrent.profile.name,
                                        .position = {},
                                        .placement = PlacementResult::OutOfBounds});
            continue;
        }
        const EnlistResult enrolement = _combat->enlist(concurrent.profile, *place);
        if (!enrolement.combatant.has_value()) {
            montage.refusals.push_back({.who = concurrent.profile.name,
                                        .position = *place,
                                        .placement = enrolement.placement});
            continue;
        }
        const CombatantId id = *enrolement.combatant;
        _attacks[id] = concurrent.attacks;
        if (!concurrent.capacities.empty()) {
            _capacities[id] = concurrent.capacities;
            // Le journal nomme ce que le combattant apporte (LOT-131) : ses effets statiques --
            // CA, resistances, vitesse -- sont deja dans son profil et ne feraient sinon aucune
            // ligne.
            std::string ligne = "capacites " + concurrent.profile.name + " :";
            for (std::size_t i = 0; i < concurrent.capacities.size(); ++i) {
                ligne += (i == 0 ? " " : ", ") + concurrent.capacities[i].name;
            }
            record(std::move(ligne));
        }
        if (!concurrent.spells.empty()) {
            _spells[id] = concurrent.spells;
        }
        if (!concurrent.behavior.empty()) {
            _behaviors[id] = concurrent.behavior;
        }
        if (bout.heroicMark) {
            _combat->economy(id)->declare(HEROIC_ACTION_RESOURCE, 1);
        }
        (concurrent.profile.side == CombatSide::Allies ? montage.allies : montage.enemies)
            .push_back(id);
    }
    record("montage : " + std::to_string(montage.allies.size()) + " allies, " +
           std::to_string(montage.enemies.size()) + " ennemis, " +
           std::to_string(montage.refusals.size()) + " refus");
    return montage;
}

bool ArenaSession::start() {
    if (!_combat->start(_random)) {
        return false;
    }
    for (const InitiativeEntry& place : _combat->turnOrder().entries()) {
        const Combatant* c = _combat->find(place.combatant);
        record("initiative " + (c == nullptr ? std::string("?") : c->profile.name) + " #" +
               std::to_string(static_cast<std::uint32_t>(place.combatant)) + " = " +
               std::to_string(place.total));
    }
    return true;
}

ArenaMount ArenaSession::replay() {
    ArenaMount montage = mount(_bout);
    start();
    return montage;
}

const std::vector<AttackProfile>* ArenaSession::attacks(CombatantId combatant) const {
    const auto trouve = _attacks.find(combatant);
    return trouve == _attacks.end() ? nullptr : &trouve->second;
}

const std::vector<ArenaSpell>* ArenaSession::spells(CombatantId combatant) const {
    const auto trouve = _spells.find(combatant);
    return trouve == _spells.end() ? nullptr : &trouve->second;
}

std::span<const Capacity> ArenaSession::capacitiesOf(CombatantId combatant) const {
    const auto trouve = _capacities.find(combatant);
    return trouve == _capacities.end() ? std::span<const Capacity>{}
                                       : std::span<const Capacity>(trouve->second);
}

void ArenaSession::hookCapacities(AttackHooks& hooks, CombatantId attacker) {
    const std::span<const Capacity> capacites = capacitiesOf(attacker);
    if (capacites.empty()) {
        return;
    }
    // Le bonus au jet, au nom de la capacite : << + 2 (Hit the Mark) >> au journal (EX-REG-003).
    const std::vector<Modifier> bonus = attackModifiersFrom(capacites);
    if (!bonus.empty()) {
        hooks.insert(AttackRollStage::BeforeRoll, [bonus](AttackRoll& jet, DeterministicRandom&) {
            for (const Modifier& modificateur : bonus) {
                jet.addModifier(modificateur);
            }
        });
    }
    // Les des en plus, si l'attaque touche. << Une fois par tour >> se compte dans la memoire du
    // tour (core::ScopedCounters), que la fin du tour vide : une attaque d'opportunite pendant le
    // tour d'un autre compte aussi, comme le Manuel le veut pour l'attaque sournoise.
    const std::vector<NamedExtraDamage> des = extraDamageFrom(capacites);
    if (!des.empty()) {
        const std::string proprietaire = std::to_string(static_cast<std::uint32_t>(attacker));
        hooks.insert(
            AttackRollStage::Hit, [this, des, proprietaire](AttackRoll& jet, DeterministicRandom&) {
                if (!jet.damageType.has_value()) {
                    return;
                }
                for (const NamedExtraDamage& supplement : des) {
                    ScopedCounters& compteurs = _combat->counters();
                    if (supplement.oncePerTurn && compteurs.value(CounterScope::Turn, proprietaire,
                                                                  supplement.capacityId) > 0) {
                        continue;
                    }
                    compteurs.increment(CounterScope::Turn, proprietaire, supplement.capacityId);
                    jet.bonusDamage.push_back(
                        {.clause = {.dice = supplement.dice, .type = *jet.damageType, .flags = 0},
                         .source = supplement.source});
                }
            });
    }
}

const std::string& ArenaSession::behaviorOf(CombatantId combatant) const {
    static const std::string joueur;
    const auto trouve = _behaviors.find(combatant);
    return trouve == _behaviors.end() ? joueur : trouve->second;
}

const AttackProfile* ArenaSession::meleeAttack(CombatantId combatant) const {
    const std::vector<AttackProfile>* liste = attacks(combatant);
    if (liste == nullptr) {
        return nullptr;
    }
    const auto trouve = std::ranges::find(*liste, AttackKind::Melee, &AttackProfile::kind);
    return trouve == liste->end() ? nullptr : &*trouve;
}

std::optional<AttackOutcome> ArenaSession::resolveAndRecord(CombatantId attacker,
                                                            CombatantId target,
                                                            const AttackProfile& profile,
                                                            const std::string& prefix) {
    // La ligne de l'attaque se reserve apres la declaration et avant les des : ce que les degats
    // declenchent (une chute, l'issue, la Marque) s'ecrit ensuite, dans l'ordre ou c'est arrive.
    std::optional<std::size_t> place;
    AttackHooks crochets = _attackHooks;
    crochets.insert(AttackRollStage::BeforeRoll, [this, &place](AttackRoll&, DeterministicRandom&) {
        place = _journal.size();
        _journal.emplace_back();
    });
    // Les capacites de l'attaquant (LOT-131) : leurs effets se branchent sur ce jet, et sur lui
    // seul -- l'arene ne connait aucune classe, elle branche des effets nommes.
    hookCapacities(crochets, attacker);
    AttackContext contexte = contextAgainst(attacker, target, profile);
    contexte.hooks = &crochets;
    std::optional<AttackOutcome> issue =
        resolveAttack(*_combat, attacker, target, profile, _random, contexte);
    if (issue.has_value() && place.has_value()) {
        _journal[*place] = prefix + issue->describe();
    }
    return issue;
}

AttackContext ArenaSession::contextAgainst(CombatantId attacker, CombatantId target,
                                           const AttackProfile& profile) const {
    AttackContext contexte{
        .hooks = &_attackHooks, .pipeline = &_damagePipeline, .circumstances = {}};
    // Manuel, « Esquiver » : les attaques contre vous sont desavantagees « si vous pouvez voir
    // l'attaquant ». La lumiere et les sens ne sont pas encore la : voir, c'est la ligne de vue.
    if (_dodging.contains(target) && hasLineOfSight(*_combat, target, attacker)) {
        contexte.circumstances.disadvantages.emplace_back("esquive de la cible");
    }
    // Guide du Maitre, « la prise en tenaille » : avantage aux jets d'attaque au corps a corps.
    if (_bout.flanking && profile.kind == AttackKind::Melee &&
        isFlanked(*_combat, attacker, target)) {
        contexte.circumstances.advantages.emplace_back("prise en tenaille");
    }
    return contexte;
}

ArenaAttack ArenaSession::attack(CombatantId target, std::size_t attackIndex) {
    const std::optional<CombatantId> actif = _combat->activeCombatant();
    if (!actif.has_value() || _combat->phase() != CombatPhase::TurnActive) {
        return {.result = ArenaActionResult::NoActiveTurn, .outcome = std::nullopt};
    }
    const Combatant* attaquant = _combat->find(*actif);
    const Combatant* cible = _combat->find(target);
    if (cible == nullptr || target == *actif || cible->profile.side == attaquant->profile.side ||
        cible->status != CombatantStatus::Standing) {
        return {.result = ArenaActionResult::InvalidTarget, .outcome = std::nullopt};
    }
    const std::vector<AttackProfile>* liste = attacks(*actif);
    if (liste == nullptr || attackIndex >= liste->size()) {
        return {.result = ArenaActionResult::NoAttack, .outcome = std::nullopt};
    }
    // Copie : un abonne peut enroler un renfort, et la table des attaques ne doit pas bouger sous
    // la resolution.
    const AttackProfile profil = (*liste)[attackIndex];
    switch (checkTarget(*_combat, *actif, target, profil)) {
        case TargetCheck::Valid:
            break;
        case TargetCheck::NotOnGrid:
            return {.result = ArenaActionResult::InvalidTarget, .outcome = std::nullopt};
        case TargetCheck::OutOfReach:
            return {.result = ArenaActionResult::OutOfReach, .outcome = std::nullopt};
        case TargetCheck::TotalCover:
            return {.result = ArenaActionResult::TotalCover, .outcome = std::nullopt};
    }
    // Une attaque que l'action deja prise a laissee (Extra Attack, LOT-132) passe avant l'action :
    // la seconde attaque du tour ne coute rien de plus.
    const std::optional<NamedExtraAttacks> enPlus = extraAttacksFrom(capacitiesOf(*actif));
    if (attaquant->economy.remaining(EXTRA_ATTACK_RESOURCE) > 0) {
        static_cast<void>(_combat->economy(*actif)->spend(EXTRA_ATTACK_RESOURCE));
        record("attaque supplementaire " + attaquant->profile.name + " (" +
               (enPlus.has_value() ? enPlus->source : std::string("?")) + ")");
    } else {
        if (attaquant->economy.remaining(ACTION_RESOURCE) <= 0) {
            return {.result = ArenaActionResult::NoAction, .outcome = std::nullopt};
        }
        _combat->spend(ACTION_RESOURCE);
        if (enPlus.has_value()) {
            _combat->economy(*actif)->grant(EXTRA_ATTACK_RESOURCE, enPlus->count);
        }
    }
    ArenaAttack attaque{.result = ArenaActionResult::Done, .outcome = std::nullopt};
    attaque.outcome = resolveAndRecord(*actif, target, profil, {});
    return attaque;
}

ArenaAttack ArenaSession::castSpell(CombatantId target, std::size_t spellIndex) {
    const std::optional<CombatantId> actif = _combat->activeCombatant();
    if (!actif.has_value() || _combat->phase() != CombatPhase::TurnActive) {
        return {.result = ArenaActionResult::NoActiveTurn, .outcome = std::nullopt};
    }
    const auto grimoire = _spells.find(*actif);
    if (grimoire == _spells.end() || spellIndex >= grimoire->second.size()) {
        return {.result = ArenaActionResult::NoSpell, .outcome = std::nullopt};
    }
    ArenaSpell& sort = grimoire->second[spellIndex];
    // Un sort epuise se refuse AVANT toute depense : il ne se propose plus (LOT-131).
    if (!sort.available()) {
        return {.result = ArenaActionResult::Exhausted, .outcome = std::nullopt};
    }
    const Combatant* lanceur = _combat->find(*actif);
    const Combatant* cible = _combat->find(target);
    if (cible == nullptr || target == *actif || cible->profile.side == lanceur->profile.side ||
        cible->status != CombatantStatus::Standing) {
        return {.result = ArenaActionResult::InvalidTarget, .outcome = std::nullopt};
    }
    const AttackProfile profil = sort.attack;
    switch (checkTarget(*_combat, *actif, target, profil)) {
        case TargetCheck::Valid:
            break;
        case TargetCheck::NotOnGrid:
            return {.result = ArenaActionResult::InvalidTarget, .outcome = std::nullopt};
        case TargetCheck::OutOfReach:
            return {.result = ArenaActionResult::OutOfReach, .outcome = std::nullopt};
        case TargetCheck::TotalCover:
            return {.result = ArenaActionResult::TotalCover, .outcome = std::nullopt};
    }
    if (lanceur->economy.remaining(ACTION_RESOURCE) <= 0) {
        return {.result = ArenaActionResult::NoAction, .outcome = std::nullopt};
    }
    _combat->spend(ACTION_RESOURCE);
    if (sort.uses > 0) {
        --sort.uses;
    }
    const std::string prefixe =
        "sort " + sort.name +
        (sort.uses < 0 ? std::string(" : ") : " (" + std::to_string(sort.uses) + " restant) : ");
    ArenaAttack lancer{.result = ArenaActionResult::Done, .outcome = std::nullopt};
    lancer.outcome = resolveAndRecord(*actif, target, profil, prefixe);
    return lancer;
}

bool ArenaSession::dodge() {
    const std::optional<CombatantId> actif = _combat->activeCombatant();
    if (!actif.has_value() || !_combat->spend(ACTION_RESOURCE)) {
        return false;
    }
    _dodging.insert(*actif);
    record("esquive " + _combat->find(*actif)->profile.name);
    return true;
}

bool ArenaSession::disengage() {
    const std::optional<CombatantId> actif = _combat->activeCombatant();
    if (!actif.has_value() || !_combat->spend(ACTION_RESOURCE)) {
        return false;
    }
    _disengaged.insert(*actif);
    record("desengagement " + _combat->find(*actif)->profile.name);
    return true;
}

bool ArenaSession::provokes(CombatantId mover, CombatantId reactor, GridPosition from,
                            GridPosition to) const {
    const Combatant* mobile = _combat->find(mover);
    const Combatant* c = _combat->find(reactor);
    const AttackProfile* coup = meleeAttack(reactor);
    if (mobile == nullptr || c == nullptr || coup == nullptr ||
        c->profile.side == mobile->profile.side || c->status != CombatantStatus::Standing ||
        c->economy.remaining(REACTION_RESOURCE) <= 0 || _declinesOpportunities.contains(reactor)) {
        return false;
    }
    const std::optional<int> avant = gridDistanceFrom(*_combat, mover, from, reactor);
    const std::optional<int> apres = gridDistanceFrom(*_combat, mover, to, reactor);
    if (!avant.has_value() || !apres.has_value() || *avant > coup->reach || *apres <= coup->reach) {
        return false;
    }
    // « Une creature hostile, situee dans votre champ de vision » : vue depuis la case qu'elle
    // quitte.
    const std::optional<GridPosition> ancre = _combat->grid().positionOf(reactor);
    const bool voit =
        ancre.has_value() &&
        hasLineOfSight(_combat->grid(), {.anchor = from, .side = _combat->grid().sideOf(mover)},
                       {.anchor = *ancre, .side = _combat->grid().sideOf(reactor)});
    return voit && (!_opportunityPolicy || _opportunityPolicy(*this, reactor, mover));
}

std::vector<CombatantId> ArenaSession::previewOpportunities(GridPosition destination) const {
    std::vector<CombatantId> opportunistes;
    const std::optional<CombatantId> actif = _combat->activeCombatant();
    const std::optional<ReachableArea> zone = _combat->reachableArea();
    if (!actif.has_value() || !zone.has_value() || _disengaged.contains(*actif) ||
        opportunityImmunityFrom(capacitiesOf(*actif)).has_value()) {
        return opportunistes;
    }
    const std::optional<Path> chemin = zone->pathTo(destination);
    if (!chemin.has_value()) {
        return opportunistes;
    }
    std::vector<GridPosition> cases{zone->origin()};
    cases.insert(cases.end(), chemin->steps.begin(), chemin->steps.end());
    // Chacun ne frappe qu'une fois : sa reaction est depensee au premier coup.
    for (std::size_t i = 0; i + 1 < cases.size(); ++i) {
        for (const CombatantId autre : _combat->combatants()) {
            if (std::ranges::find(opportunistes, autre) == opportunistes.end() &&
                provokes(*actif, autre, cases[i], cases[i + 1])) {
                opportunistes.push_back(autre);
            }
        }
    }
    return opportunistes;
}

void ArenaSession::setTakesOpportunities(CombatantId combatant, bool takes) {
    if (takes) {
        _declinesOpportunities.erase(combatant);
    } else {
        _declinesOpportunities.insert(combatant);
    }
}

AttackCircumstances ArenaSession::circumstancesAgainst(CombatantId attacker, CombatantId target,
                                                       const AttackProfile& profile) const {
    return contextAgainst(attacker, target, profile).circumstances;
}

bool ArenaSession::dash() {
    const std::optional<CombatantId> actif = _combat->activeCombatant();
    if (!actif.has_value() || !_combat->spend(ACTION_RESOURCE)) {
        return false;
    }
    const Combatant* c = _combat->find(*actif);
    // « Vous obtenez un deplacement supplementaire pour le tour en cours », egal a votre vitesse :
    // un octroi, que le debut du prochain tour efface (core::ActionEconomy::grant).
    _combat->economy(*actif)->grant(MOVEMENT_RESOURCE, c->profile.movement);
    record("precipitation " + c->profile.name);
    return true;
}

MoveOutcome ArenaSession::move(GridPosition destination) {
    const std::optional<CombatantId> actif = _combat->activeCombatant();
    if (!actif.has_value()) {
        return _combat->move(destination);
    }
    MoveOutcome parcours{.result = MoveResult::NoActiveTurn, .path = {}};
    // Un pas vers `vers` : s'il aboutit, il est noté, signalé à l'observateur et cumulé.
    const auto avancer = [&](GridPosition vers) {
        const MoveOutcome pas = _combat->move(vers);
        if (pas.result != MoveResult::Moved) {
            return pas;
        }
        record("pas " + _combat->find(*actif)->profile.name + " " + std::to_string(vers.column) +
               "," + std::to_string(vers.row) + " (" + std::to_string(pas.path.cost) + ")");
        if (_moveObserver) {
            _moveObserver(*actif, pas.path);
        }
        parcours.result = MoveResult::Moved;
        parcours.path.steps.insert(parcours.path.steps.end(), pas.path.steps.begin(),
                                   pas.path.steps.end());
        parcours.path.cost += pas.path.cost;
        return pas;
    };

    // Chaque tour de boucle depense au moins une reaction, ou finit le deplacement : la garde n'est
    // la que contre une regression.
    for (std::size_t garde = 0; garde <= _combat->combatants().size(); ++garde) {
        const bool toujoursLui =
            _combat->phase() == CombatPhase::TurnActive && _combat->activeCombatant() == actif;
        if (!toujoursLui) {
            return parcours;
        }
        const std::optional<ReachableArea> zone = _combat->reachableArea();
        const std::optional<Path> chemin =
            zone.has_value() ? zone->pathTo(destination) : std::optional<Path>{};
        if (!chemin.has_value()) {
            return parcours.result == MoveResult::Moved ? parcours : _combat->move(destination);
        }

        // Les cases successives de l'ancre, depart compris, et la premiere sortie d'allonge.
        std::vector<GridPosition> cases{zone->origin()};
        cases.insert(cases.end(), chemin->steps.begin(), chemin->steps.end());
        std::vector<CombatantId> opportunistes;
        const std::optional<std::size_t> sortie = firstProvokingStep(*actif, cases, opportunistes);
        if (!sortie.has_value()) {
            // Une capacite qui soustrait aux attaques d'opportunite (LOT-131) a joue si le pas en
            // aurait provoque une : le journal la nomme, comme tout ce qui a joue (EX-REG-003).
            if (const std::optional<std::string> capacite =
                    opportunityImmunityFrom(capacitiesOf(*actif))) {
                std::vector<CombatantId> evites;
                if (firstExitFromReach(*actif, cases, evites).has_value()) {
                    record("sans attaque d'opportunite " + _combat->find(*actif)->profile.name +
                           " (" + *capacite + ")");
                }
            }
            const MoveOutcome pas = avancer(destination);
            return parcours.result == MoveResult::Moved ? parcours : pas;
        }

        // On ne s'arrete pas sur la case d'un allie qu'on traverse : l'attaque tombe a la derniere
        // case ou l'on peut se tenir avant la sortie.
        const std::size_t arret = derniereCaseTenable(*zone, cases, *sortie);
        if (arret > 0) {
            avancer(cases[arret]);
        }
        takeOpportunities(*actif, opportunistes);
    }
    return parcours;
}

std::optional<std::size_t> ArenaSession::firstProvokingStep(
    CombatantId mover, const std::vector<GridPosition>& cases,
    std::vector<CombatantId>& reactors) const {
    // Desengage, ou soustrait aux attaques d'opportunite par une capacite (LOT-131) : rien ne
    // provoque. La session ne sait pas laquelle ; elle lit un effet nomme.
    if (_disengaged.contains(mover) || opportunityImmunityFrom(capacitiesOf(mover)).has_value()) {
        return std::nullopt;
    }
    return firstExitFromReach(mover, cases, reactors);
}

std::optional<std::size_t> ArenaSession::firstExitFromReach(
    CombatantId mover, const std::vector<GridPosition>& cases,
    std::vector<CombatantId>& reactors) const {
    std::optional<std::size_t> sortie;
    for (std::size_t i = 0; i + 1 < cases.size() && !sortie.has_value(); ++i) {
        for (const CombatantId autre : _combat->combatants()) {
            if (provokes(mover, autre, cases[i], cases[i + 1])) {
                reactors.push_back(autre);
            }
        }
        if (!reactors.empty()) {
            sortie = i;
        }
    }
    return sortie;
}

void ArenaSession::takeOpportunities(CombatantId mover, const std::vector<CombatantId>& reactors) {
    for (const CombatantId opportuniste : reactors) {
        const Combatant* cible = _combat->find(mover);
        const Combatant* c = _combat->find(opportuniste);
        if (cible == nullptr || cible->status != CombatantStatus::Standing ||
            _combat->phase() == CombatPhase::Ended || c == nullptr ||
            c->status != CombatantStatus::Standing) {
            break;
        }
        const AttackProfile coup = *meleeAttack(opportuniste);
        static_cast<void>(_combat->economy(opportuniste)->spend(REACTION_RESOURCE));
        static_cast<void>(resolveAndRecord(opportuniste, mover, coup, "opportunite : "));
    }
}

bool ArenaSession::endTurn() {
    return _combat->endTurn();
}

WithdrawResult ArenaSession::withdraw() {
    const std::optional<CombatantId> actif = _combat->activeCombatant();
    if (!actif.has_value()) {
        return WithdrawResult::NotInCombat;
    }
    return _combat->withdraw(*actif);
}

std::optional<CombatOutcome> ArenaSession::outcome() const {
    return _combat->outcome();
}

std::vector<ArenaSpell> arenaSpellsFor(const CharacterSheet& sheet,
                                       const PlayableClass& playableClass,
                                       const SpellCatalog& spells, int proficiencyBonus,
                                       std::vector<std::string>& skipped) {
    std::vector<ArenaSpell> grimoire;
    if (!playableClass.spellcasting.has_value()) {
        return grimoire;
    }
    for (const KnownSpell& connu : sheet.knownSpells) {
        const Spell* sort = spells.find(connu.spellId);
        if (sort == nullptr) {
            skipped.push_back(connu.spellId);
            continue;
        }
        std::optional<AttackProfile> attaque =
            spellAttackFor(sheet, *sort, playableClass.spellcasting->ability, proficiencyBonus);
        if (!attaque.has_value()) {
            // Connu, mais sans mecanisme joue ici (EX-RPG-051) : dit, pas tu.
            skipped.push_back(sort->name);
            continue;
        }
        grimoire.push_back({.id = sort->id,
                            .name = sort->name,
                            .level = sort->level,
                            .uses = connu.perDay == 0 ? -1 : connu.remaining,
                            .attack = std::move(*attaque)});
    }
    return grimoire;
}

}  // namespace core
