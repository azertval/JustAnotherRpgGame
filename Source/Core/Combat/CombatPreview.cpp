// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/CombatPreview.h"

#include <cstdint>
#include <string>

#include "Core/Combat/CombatCounters.h"
#include "Core/Combat/EnemyAi.h"
#include "Core/Combat/Flanking.h"
#include "Core/Combat/LineOfSight.h"
#include "Core/Rpg/ClassCapacities.h"

namespace core {

namespace {

/// Les dés que les capacités de @p actif ajouteraient a la touche, et s'ils joueraient : les memes
/// conditions que le crochet `Hit` de la session (`ArenaSession::hookCapacities`), lues sans rien
/// compter.
[[nodiscard]] std::vector<ExtraDamagePreview> extraDamagePreview(const ArenaSession& session,
                                                                 CombatantId actif,
                                                                 CombatantId target) {
    std::vector<ExtraDamagePreview> apercus;
    const std::string proprietaire = std::to_string(static_cast<std::uint32_t>(actif));
    for (const NamedExtraDamage& supplement : extraDamageFrom(session.capacitiesOf(actif))) {
        ExtraDamagePreview apercu{.source = supplement.source, .dice = supplement.dice};
        if (supplement.allyAdjacentToTarget &&
            !isAdjacentToAllyOf(session.combat(), actif, target)) {
            apercu.applies = false;
            apercu.reason = "un allie au contact de la cible est requis";
        } else if (supplement.oncePerTurn &&
                   session.combat().counters().value(CounterScope::Turn, proprietaire,
                                                     supplement.capacityId) > 0) {
            apercu.applies = false;
            apercu.reason = "deja jouee ce tour";
        }
        apercus.push_back(std::move(apercu));
    }
    return apercus;
}

}  // namespace

std::optional<AttackPreview> previewAttack(const ArenaSession& session, CombatantId target,
                                           std::size_t attackIndex) {
    const CombatState& combat = session.combat();
    const std::optional<CombatantId> actif = combat.activeCombatant();
    const std::vector<AttackProfile>* attaques =
        actif.has_value() ? session.attacks(*actif) : nullptr;
    const Combatant* cible = combat.find(target);
    if (!actif.has_value() || attaques == nullptr || attackIndex >= attaques->size() ||
        cible == nullptr) {
        return std::nullopt;
    }
    const AttackProfile& profil = (*attaques)[attackIndex];
    AttackPreview apercu{.check = checkTarget(combat, *actif, target, profil),
                         .attackIndex = attackIndex,
                         .label = profil.label,
                         .advantages = {},
                         .disadvantages = {}};
    // Les memes sources que resolveAttack, dans le meme ordre : la grille, puis la session.
    const AttackCircumstances grille = attackCircumstances(combat, *actif, target, profil);
    const AttackCircumstances parSession = session.circumstancesAgainst(*actif, target, profil);
    for (const AttackCircumstances* source : {&grille, &parSession}) {
        apercu.advantages.insert(apercu.advantages.end(), source->advantages.begin(),
                                 source->advantages.end());
        apercu.disadvantages.insert(apercu.disadvantages.end(), source->disadvantages.begin(),
                                    source->disadvantages.end());
    }
    apercu.stance = rollStance(static_cast<int>(apercu.advantages.size()),
                               static_cast<int>(apercu.disadvantages.size()));
    apercu.cover = coverBetween(combat, *actif, target);
    apercu.armorClass = cible->profile.armorClass + coverBonus(apercu.cover);
    // Les capacites entrent comme dans le jet (LOT-131) : le bonus au jet avant le de, les des
    // en plus a la touche. Le profil « tel qu'il sera jete » les porte, et l'esperance se calcule
    // sur lui : une seule formule, celle de l'IA.
    AttackProfile jete = profil;
    apercu.capacityModifiers = attackModifiersFrom(session.capacitiesOf(*actif));
    jete.modifiers.insert(jete.modifiers.end(), apercu.capacityModifiers.begin(),
                          apercu.capacityModifiers.end());
    apercu.attackBonus = attackBonusOf(jete);
    apercu.requiredRoll = requiredRoll(apercu.armorClass, apercu.attackBonus);
    if (!profil.damage.empty()) {
        apercu.extraDamage = extraDamagePreview(session, *actif, target);
        for (const ExtraDamagePreview& supplement : apercu.extraDamage) {
            if (supplement.applies) {
                jete.damage.push_back(
                    {.dice = supplement.dice, .type = profil.damage.front().type, .flags = 0});
            }
        }
    }
    if (apercu.check == TargetCheck::Valid) {
        apercu.hitChance = hitChance(apercu.requiredRoll, apercu.stance, profil.criticalThreshold);
        apercu.expectedDamage = expectedDamage(jete, apercu.armorClass, apercu.stance);
    }
    return apercu;
}

std::optional<std::size_t> firstValidAttack(const ArenaSession& session, CombatantId target) {
    const std::optional<CombatantId> actif = session.combat().activeCombatant();
    const std::vector<AttackProfile>* attaques =
        actif.has_value() ? session.attacks(*actif) : nullptr;
    if (attaques == nullptr) {
        return std::nullopt;
    }
    for (std::size_t i = 0; i < attaques->size(); ++i) {
        if (checkTarget(session.combat(), *actif, target, (*attaques)[i]) == TargetCheck::Valid) {
            return i;
        }
    }
    return std::nullopt;
}

MovePreview previewMove(const ArenaSession& session, GridPosition destination) {
    MovePreview apercu;
    const std::optional<ReachableArea> zone = session.combat().reachableArea();
    if (!zone.has_value()) {
        return apercu;
    }
    apercu.path = zone->pathTo(destination);
    apercu.movementLeft = zone->budget() - (apercu.path.has_value() ? apercu.path->cost : 0);
    if (apercu.path.has_value()) {
        apercu.opportunities = session.previewOpportunities(destination);
    }
    return apercu;
}

}  // namespace core
