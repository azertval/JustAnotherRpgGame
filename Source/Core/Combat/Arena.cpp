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

#include "Core/Combat/AreaOfEffect.h"
#include "Core/Combat/BattleGrid.h"
#include "Core/Combat/CombatCounters.h"
#include "Core/Combat/Flanking.h"
#include "Core/Combat/Pathfinding.h"
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
            if (e.hook == CombatHook::RoundStart) {
                // Les effets de sort qui durent comptent leurs rounds (LOT-133).
                for (ArenaEffect& effet : _effects) {
                    if (effet.roundsLeft > 0) {
                        --effet.roundsLeft;
                    }
                }
                endEffects([](const ArenaEffect& effet) { return effet.roundsLeft == 0; },
                           "duree ecoulee");
            }
            if (e.hook == CombatHook::CombatantDowned && e.combatant.has_value()) {
                // Un lanceur qui tombe perd sa concentration.
                const CombatantId tombe = *e.combatant;
                endEffects(
                    [tombe](const ArenaEffect& effet) {
                        return effet.concentration && effet.caster == tombe;
                    },
                    "lanceur a terre");
            }
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
    _effects.clear();
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

bool ArenaSession::hasEffect(CombatantId combatant, SpellEffectKind kind) const {
    return std::ranges::any_of(_effects, [&](const ArenaEffect& effet) {
        return effet.bearer == combatant && effet.kind == kind;
    });
}

void ArenaSession::endEffects(const std::function<bool(const ArenaEffect&)>& ends,
                              const std::string& reason) {
    std::vector<ArenaEffect> finis;
    std::vector<ArenaEffect> restants;
    for (ArenaEffect& effet : _effects) {
        (ends(effet) ? finis : restants).push_back(std::move(effet));
    }
    _effects = std::move(restants);
    for (const ArenaEffect& effet : finis) {
        if (effet.kind == SpellEffectKind::Fly) {
            // Le vol rend ce qu'il avait pris : la marche, et son budget.
            static_cast<void>(_combat->setLocomotion(effet.bearer, effet.previousLocomotion,
                                                     effet.previousMovement));
        }
        const Combatant* porteur = _combat->find(effet.bearer);
        record("fin de l'effet " + effet.source + " sur " +
               (porteur == nullptr ? std::string("?") : porteur->profile.name) + " (" + reason +
               ")");
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
    // La benediction (LOT-134) : un d4 de plus, lance au moment du jet et nomme.
    if (hasEffect(attacker, SpellEffectKind::Bless)) {
        crochets.insert(AttackRollStage::BeforeRoll,
                        [this, attacker](AttackRoll& jet, DeterministicRandom&) {
                            if (std::optional<Modifier> de = blessingFor(attacker)) {
                                jet.addModifier(std::move(*de));
                            }
                        });
    }
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
    // Manuel, « Invisible » : les jets d'attaque contre la creature sont desavantages, les siens
    // avantages (LOT-133).
    if (hasEffect(target, SpellEffectKind::Invisible)) {
        contexte.circumstances.disadvantages.emplace_back("cible invisible");
    }
    if (hasEffect(attacker, SpellEffectKind::Invisible)) {
        contexte.circumstances.advantages.emplace_back("attaquant invisible");
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
    if (attaque.outcome.has_value()) {
        attaque.summary = attaque.outcome->describe();
    }
    // L'invisibilite cesse pour qui attaque -- apres l'attaque, qui en a profite.
    const CombatantId attaquantId = *actif;
    endEffects(
        [attaquantId](const ArenaEffect& effet) {
            return effet.bearer == attaquantId && effet.kind == SpellEffectKind::Invisible;
        },
        "il attaque");
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
    // L'arme spirituelle deja invoquee frappe de nouveau sans nouveau lancer (LOT-134).
    const CombatantId lanceurActif = *actif;
    const bool armeInvoquee =
        sort.effect.has_value() && sort.effect->kind == SpellEffectKind::SpiritualWeapon &&
        std::ranges::any_of(_effects, [&](const ArenaEffect& effet) {
            return effet.bearer == lanceurActif && effet.kind == SpellEffectKind::SpiritualWeapon &&
                   effet.source == sort.name;
        });
    // Un sort epuise se refuse AVANT toute depense : il ne se propose plus (LOT-131).
    if (!sort.available() && !armeInvoquee) {
        return {.result = ArenaActionResult::Exhausted, .outcome = std::nullopt};
    }
    const Combatant* lanceur = _combat->find(*actif);
    const Combatant* cible = _combat->find(target);
    // Une cible debout ; un soin releve aussi qui est a terre (LOT-134).
    const bool atteignable =
        cible != nullptr &&
        (cible->status == CombatantStatus::Standing ||
         (cible->status == CombatantStatus::Down && sort.mechanism == SpellMechanism::Healing));
    if (!atteignable) {
        return {.result = ArenaActionResult::InvalidTarget, .outcome = std::nullopt};
    }
    // La cible que le sort vise (LOT-133) : un sort qui blesse ne soigne pas un allie par erreur,
    // un sort qui aide ne se pose pas sur l'ennemi.
    const bool memeCamp = cible->profile.side == lanceur->profile.side;
    const bool cibleValide = [&] {
        switch (sort.target) {
            case SpellTarget::Enemy:
                return target != *actif && !memeCamp;
            case SpellTarget::Ally:
                return memeCamp;
            case SpellTarget::Self:
                return target == *actif;
        }
        return false;
    }();
    if (!cibleValide) {
        return {.result = ArenaActionResult::InvalidTarget, .outcome = std::nullopt};
    }
    if (target != *actif) {
        switch (checkTarget(*_combat, *actif, target, sort.attack)) {
            case TargetCheck::Valid:
                break;
            case TargetCheck::NotOnGrid:
                return {.result = ArenaActionResult::InvalidTarget, .outcome = std::nullopt};
            case TargetCheck::OutOfReach:
                return {.result = ArenaActionResult::OutOfReach, .outcome = std::nullopt};
            case TargetCheck::TotalCover:
                return {.result = ArenaActionResult::TotalCover, .outcome = std::nullopt};
        }
    }
    // Un sort d'action bonus depense l'action bonus (LOT-134).
    const std::string_view ressource = sort.bonusAction ? BONUS_ACTION_RESOURCE : ACTION_RESOURCE;
    if (lanceur->economy.remaining(ressource) <= 0) {
        return {.result = ArenaActionResult::NoAction, .outcome = std::nullopt};
    }
    _combat->spend(ressource);
    if (sort.uses > 0 && !armeInvoquee) {
        --sort.uses;
    }
    const std::string prefixe =
        "sort " + sort.name +
        (armeInvoquee    ? std::string(" (l'arme frappe de nouveau) : ")
         : sort.uses < 0 ? std::string(" : ")
                         : " (" + std::to_string(sort.uses) + " restant) : ");
    // Copie : un abonne peut enroler un renfort, et la table des sorts ne doit pas bouger sous la
    // resolution.
    const ArenaSpell lance = sort;
    const CombatantId lanceurId = *actif;
    if (lance.concentration && !armeInvoquee) {
        // On ne se concentre que sur un sort a la fois (Manuel, « Concentration »).
        endEffects(
            [lanceurId](const ArenaEffect& effet) {
                return effet.concentration && effet.caster == lanceurId;
            },
            "concentration sur " + lance.name);
    }
    // L'invisibilite cesse pour qui lance un sort : avant l'effet qu'il pose, apres les degats qui
    // en ont profite.
    const auto finInvisibilite = [this, lanceurId] {
        endEffects(
            [lanceurId](const ArenaEffect& effet) {
                return effet.bearer == lanceurId && effet.kind == SpellEffectKind::Invisible;
            },
            "il lance un sort");
    };
    switch (lance.mechanism) {
        case SpellMechanism::AttackRoll: {
            ArenaAttack issue = castAttackRolls(lanceurId, target, lance, prefixe);
            finInvisibilite();
            if (lance.effect.has_value() && !armeInvoquee) {
                // Un sort qui frappe et laisse quelque chose derriere lui : l'arme spirituelle
                // reste aupres de son lanceur.
                static_cast<void>(castEffect(lanceurId, lanceurId, lance, prefixe));
            }
            return issue;
        }
        case SpellMechanism::AutoHit: {
            ArenaAttack issue = castAutoHit(lanceurId, target, lance, prefixe);
            finInvisibilite();
            return issue;
        }
        case SpellMechanism::SavingThrow: {
            ArenaAttack issue = castSavingThrow(lanceurId, target, lance, prefixe);
            finInvisibilite();
            return issue;
        }
        case SpellMechanism::Effect:
            finInvisibilite();
            return castEffect(lanceurId, target, lance, prefixe);
        case SpellMechanism::Healing:
            finInvisibilite();
            return castHealing(lanceurId, target, lance, prefixe);
    }
    return {.result = ArenaActionResult::NoSpell, .outcome = std::nullopt};
}

ArenaAttack ArenaSession::castAttackRolls(CombatantId caster, CombatantId target,
                                          const ArenaSpell& spell, const std::string& prefix) {
    ArenaAttack issue{.result = ArenaActionResult::Done, .outcome = std::nullopt};
    for (int rayon = 0; rayon < spell.projectiles; ++rayon) {
        const Combatant* cible = _combat->find(target);
        if (cible == nullptr || cible->status != CombatantStatus::Standing ||
            _combat->phase() == CombatPhase::Ended) {
            // Le moteur dirige tous les rayons sur la meme cible : ceux qui restent quand elle
            // tombe sont perdus, et le dire vaut mieux que de les faire disparaitre.
            record(prefix + std::to_string(spell.projectiles - rayon) +
                   " projectile(s) perdu(s) : la cible est hors de combat");
            break;
        }
        const std::string rang =
            spell.projectiles > 1
                ? std::to_string(rayon + 1) + "/" + std::to_string(spell.projectiles) + " : "
                : std::string{};
        std::optional<AttackOutcome> coup =
            resolveAndRecord(caster, target, spell.attack, prefix + rang);
        if (coup.has_value() && !issue.outcome.has_value()) {
            issue.summary = prefix + rang + coup->describe();
            issue.outcome = std::move(coup);
        }
    }
    return issue;
}

ArenaAttack ArenaSession::castAutoHit(CombatantId caster, CombatantId target,
                                      const ArenaSpell& spell, const std::string& prefix) {
    // Chaque projectile lance ses des ; tous frappent en meme temps, en une salve.
    std::vector<RolledDamage> des;
    for (int projectile = 0; projectile < spell.projectiles; ++projectile) {
        std::vector<RolledDamage> lances = rollDamage(spell.attack.damage, false, _random);
        des.insert(des.end(), lances.begin(), lances.end());
    }
    // La ligne se reserve avant les degats : ce qu'ils declenchent s'ecrit ensuite.
    const std::size_t place = _journal.size();
    _journal.emplace_back();
    const std::vector<DamageRequest> salve{{.target = target, .damage = des}};
    std::vector<DamageReport> rapports = _damagePipeline.apply(*_combat, salve);
    const std::optional<DamageReport> rapport =
        rapports.empty() ? std::nullopt : std::optional<DamageReport>(std::move(rapports.front()));
    const Combatant* lanceur = _combat->find(caster);
    const Combatant* cible = _combat->find(target);
    _journal[place] = prefix + spell.name + " " +
                      (lanceur == nullptr ? std::string("?") : lanceur->profile.name) + " -> " +
                      (cible == nullptr ? std::string("?") : cible->profile.name) +
                      " : touche sans jet (" + std::to_string(spell.projectiles) +
                      " projectile(s))" + describeDamage(des, rapport);
    return {.result = ArenaActionResult::Done, .outcome = std::nullopt, .summary = _journal[place]};
}

ArenaAttack ArenaSession::castSavingThrow(CombatantId caster, CombatantId target,
                                          const ArenaSpell& spell, const std::string& prefix) {
    std::vector<CombatantId> cibles{target};
    if (spell.areaRadius > 0) {
        // La sphere se centre sur la cible (LOT-133) : le moteur ne vise pas encore un point
        // vide. Elle prend tout ce qu'elle touche, allies et lanceur compris.
        const std::optional<GridPosition> ancre = _combat->grid().positionOf(target);
        if (ancre.has_value()) {
            const int cote = _combat->grid().sideOf(target);
            const GridPoint centre{.x = (2 * ancre->column) + cote, .y = (2 * ancre->row) + cote};
            cibles = combatantsInArea(*_combat, {.shape = AreaShape::Sphere,
                                                 .origin = centre,
                                                 .toward = centre,
                                                 .size = spell.areaRadius,
                                                 .width = 1});
        }
    }
    const Combatant* lanceur = _combat->find(caster);
    const Ability caracteristique = spell.save.value_or(Ability::Dexterity);
    // Les des se lancent une fois pour toutes les cibles (Manuel, « Degats de zone »).
    const std::vector<RolledDamage> des = rollDamage(spell.attack.damage, false, _random);
    int lances = 0;
    for (const RolledDamage& lance : des) {
        lances += lance.amount;
    }
    const std::size_t place = _journal.size();
    _journal.emplace_back();
    std::vector<DamageRequest> salve;
    // Une ligne par creature ; `blesse` : elle a une demande dans la salve, donc un rapport.
    struct LigneDeCible {
        std::string texte;
        bool blesse = false;
    };
    std::vector<LigneDeCible> lignes;
    for (const CombatantId id : cibles) {
        const Combatant* creature = _combat->find(id);
        if (creature == nullptr || creature->status != CombatantStatus::Standing) {
            continue;
        }
        std::vector<Modifier> modificateurs{
            {.source = "sauvegarde de " + std::string(abilityLabel(caracteristique)),
             .value = creature->profile.savingThrows[static_cast<std::size_t>(caracteristique)]}};
        if (std::optional<Modifier> de = blessingFor(id)) {
            modificateurs.push_back(std::move(*de));
        }
        const CheckResult jet = rollCheck(spell.saveDc, modificateurs, RollStance::Normal, _random);
        std::string ligne = "  " + creature->profile.name + " : " + jet.describe();
        if (jet.succeeded() && spell.saveEffect == SaveEffect::Negates) {
            lignes.push_back({.texte = ligne + " ; aucun degat", .blesse = false});
            continue;
        }
        std::vector<RolledDamage> recus = des;
        if (jet.succeeded()) {
            for (RolledDamage& lance : recus) {
                lance.amount /= 2;
            }
            ligne += " ; moitie " + std::to_string(lances) + " -> " + std::to_string(lances / 2);
        }
        salve.push_back({.target = id, .damage = std::move(recus)});
        lignes.push_back({.texte = std::move(ligne), .blesse = true});
    }
    const std::vector<DamageReport> rapports = _damagePipeline.apply(*_combat, salve);
    // Chaque cible touchee retrouve son rapport, dans l'ordre de la salve.
    std::size_t rapport = 0;
    std::vector<std::string> texte;
    for (LigneDeCible& ligne : lignes) {
        if (ligne.blesse && rapport < rapports.size()) {
            for (const DamageStep& etape : rapports[rapport].work.trace) {
                ligne.texte += " ; " + etape.source + ' ' + std::to_string(etape.before) + " -> " +
                               std::to_string(etape.after);
            }
            ligne.texte += " ; PV " + std::to_string(rapports[rapport].hitPointsBefore) + " -> " +
                           std::to_string(rapports[rapport].hitPointsAfter);
            ++rapport;
        }
        texte.push_back(std::move(ligne.texte));
    }
    std::string entete = prefix + spell.name + " " +
                         (lanceur == nullptr ? std::string("?") : lanceur->profile.name) +
                         " : sauvegarde de " + std::string(abilityLabel(caracteristique)) + " DD " +
                         std::to_string(spell.saveDc) + " ; " + std::to_string(lignes.size()) +
                         " creature(s)" + describeDamage(des, std::nullopt);
    _journal[place] = entete;
    _journal.insert(_journal.begin() + static_cast<std::ptrdiff_t>(place) + 1, texte.begin(),
                    texte.end());
    return {.result = ArenaActionResult::Done, .outcome = std::nullopt, .summary = entete};
}

ArenaAttack ArenaSession::castHealing(CombatantId caster, CombatantId target,
                                      const ArenaSpell& spell, const std::string& prefix) {
    if (!spell.healing.has_value()) {
        return {.result = ArenaActionResult::NoSpell, .outcome = std::nullopt};
    }
    const DiceRoll soin = rollDice(*spell.healing, _random);
    const Combatant* avant = _combat->find(target);
    const int pvAvant = avant->profile.currentHitPoints;
    _combat->heal(target, soin.total);
    const Combatant* lanceur = _combat->find(caster);
    const Combatant* apres = _combat->find(target);
    std::string ligne =
        prefix + "soin " + (lanceur == nullptr ? std::string("?") : lanceur->profile.name) +
        " -> " + apres->profile.name + " : " + soin.describe() + " ; PV " +
        std::to_string(pvAvant) + " -> " + std::to_string(apres->profile.currentHitPoints);
    record(ligne);
    return {.result = ArenaActionResult::Done, .outcome = std::nullopt, .summary = ligne};
}

std::optional<Modifier> ArenaSession::blessingFor(CombatantId combatant) {
    const auto effet = std::ranges::find_if(_effects, [&](const ArenaEffect& e) {
        return e.bearer == combatant && e.kind == SpellEffectKind::Bless && e.dice.has_value();
    });
    if (effet == _effects.end()) {
        return std::nullopt;
    }
    return Modifier{.source = effet->source, .value = rollDice(*effet->dice, _random).total};
}

ArenaAttack ArenaSession::castEffect(CombatantId caster, CombatantId target,
                                     const ArenaSpell& spell, const std::string& prefix) {
    if (!spell.effect.has_value()) {
        return {.result = ArenaActionResult::NoSpell, .outcome = std::nullopt};
    }
    const SpellEffectKind genre = spell.effect->kind;
    // Les cibles : celle qu'on a choisie, puis -- pour un sort a plusieurs cibles -- les allies
    // debout les plus proches du lanceur, a portee, dans l'ordre des distances puis des
    // identifiants (LOT-134).
    std::vector<CombatantId> cibles{target};
    if (spell.maxTargets > 1) {
        const Combatant* lanceur = _combat->find(caster);
        std::vector<std::pair<int, CombatantId>> proches;
        for (const CombatantId autre : _combat->combatants()) {
            const Combatant* c = _combat->find(autre);
            if (autre == target || c == nullptr || c->status != CombatantStatus::Standing ||
                c->profile.side != lanceur->profile.side) {
                continue;
            }
            const std::optional<int> distance =
                autre == caster ? std::optional<int>(0) : gridDistance(*_combat, caster, autre);
            if (distance.has_value() &&
                (autre == caster ||
                 checkTarget(*_combat, caster, autre, spell.attack) == TargetCheck::Valid)) {
                proches.emplace_back(*distance, autre);
            }
        }
        std::ranges::sort(proches);
        for (const auto& [distance, autre] : proches) {
            if (std::cmp_greater_equal(cibles.size(), spell.maxTargets)) {
                break;
            }
            cibles.push_back(autre);
        }
    }
    std::string ligne = prefix + "effet " + spell.name + " sur ";
    for (std::size_t i = 0; i < cibles.size(); ++i) {
        const CombatantId porteurId = cibles[i];
        // Un meme effet ne se cumule pas : le nouveau remplace l'ancien.
        endEffects(
            [porteurId, genre](const ArenaEffect& effet) {
                return effet.bearer == porteurId && effet.kind == genre;
            },
            "relance");
        const Combatant* porteur = _combat->find(porteurId);
        ArenaEffect effet{
            .bearer = porteurId,
            .caster = caster,
            .kind = genre,
            .source = spell.name,
            .concentration = spell.concentration,
            .roundsLeft = spell.effect->durationRounds > 0 ? spell.effect->durationRounds : -1,
            .previousLocomotion = porteur->profile.locomotion,
            .previousMovement = porteur->profile.movement,
            .dice = spell.effect->dice};
        if (genre == SpellEffectKind::Fly) {
            static_cast<void>(_combat->setLocomotion(porteurId, Locomotion::Fly,
                                                     movementBudget(spell.effect->meters)));
        }
        _effects.push_back(std::move(effet));
        ligne += (i == 0 ? "" : ", ") + porteur->profile.name;
    }
    switch (genre) {
        case SpellEffectKind::Fly:
            ligne += " : vole, " + std::to_string(movementBudget(spell.effect->meters)) +
                     " cases par tour";
            break;
        case SpellEffectKind::Invisible:
            ligne += " : invisible";
            break;
        case SpellEffectKind::Bless:
            ligne +=
                " : +" +
                (spell.effect->dice.has_value() ? std::to_string(spell.effect->dice->count) + "d" +
                                                      std::to_string(spell.effect->dice->faces)
                                                : std::string("?")) +
                " aux jets d'attaque et de sauvegarde";
            break;
        case SpellEffectKind::SpiritualWeapon:
            ligne += " : arme invoquee, elle frappe de nouveau par une action bonus";
            break;
    }
    record(ligne);
    return {.result = ArenaActionResult::Done, .outcome = std::nullopt, .summary = ligne};
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
    // Manuel, « Attaque d'opportunite » : une creature « que vous pouvez voir ». L'invisible
    // passe (LOT-133).
    if (hasEffect(mover, SpellEffectKind::Invisible)) {
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
        endEffects(
            [opportuniste](const ArenaEffect& effet) {
                return effet.bearer == opportuniste && effet.kind == SpellEffectKind::Invisible;
            },
            "il attaque");
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
    const Ability incantation = playableClass.spellcasting->ability;
    // Le soin d'un sort, modificateur d'incantation compris s'il s'y ajoute (soin des blessures).
    const auto soinDe = [&](const Spell& sort) -> std::optional<Dice> {
        std::optional<Dice> soin = sort.healing;
        if (soin.has_value() && sort.addsAbilityModifier) {
            soin->modifier += sheet.modifier(incantation);
        }
        return soin;
    };
    for (const KnownSpell& connu : sheet.knownSpells) {
        const Spell* sort = spells.find(connu.spellId);
        if (sort == nullptr) {
            skipped.push_back(connu.spellId);
            continue;
        }
        const std::optional<SpellMechanism> mecanisme = spellMechanism(*sort);
        if (!mecanisme.has_value()) {
            // Connu, mais sans mecanisme joue en combat (EX-RPG-051) : dit, pas tu.
            skipped.push_back(sort->name);
            continue;
        }
        grimoire.push_back(
            {.id = sort->id,
             .name = sort->name,
             .level = sort->level,
             .uses = connu.perDay == 0 ? -1 : connu.remaining,
             .mechanism = *mecanisme,
             .attack = spellProfileFor(sheet, *sort, incantation, proficiencyBonus),
             .target = sort->target,
             .projectiles = sort->projectiles,
             .save = sort->savingThrow,
             // Player's Guide, p. 197 et 201 : DD = 8 + maitrise + modificateur d'incantation.
             .saveDc = 8 + proficiencyBonus + sheet.modifier(incantation),
             .saveEffect = sort->saveEffect,
             .areaRadius = sort->areaRadiusMeters > 0.0F
                               ? areaTilesFromMeters(sort->areaRadiusMeters).value_or(0)
                               : 0,
             .effect = sort->effect,
             .concentration = sort->concentration,
             .bonusAction = sort->bonusAction,
             .maxTargets = sort->maxTargets,
             .healing = soinDe(*sort)});
    }
    return grimoire;
}

}  // namespace core
