// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/Attack.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "Core/Combat/BattleGrid.h"
#include "Core/Rpg/Ability.h"
#include "Core/Rpg/Bestiary.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Equipment.h"
#include "Core/Rpg/Inventory.h"
#include "Core/Rpg/Scale.h"

namespace core {
namespace {

[[nodiscard]] std::string_view nomDeCaracteristique(Ability caracteristique) noexcept {
    switch (caracteristique) {
        case Ability::Strength:
            return "Force";
        case Ability::Dexterity:
            return "Dexterite";
        case Ability::Constitution:
            return "Constitution";
        case Ability::Intelligence:
            return "Intelligence";
        case Ability::Wisdom:
            return "Sagesse";
        case Ability::Charisma:
            return "Charisme";
    }
    return "?";
}

[[nodiscard]] std::string signe(int valeur) {
    return (valeur >= 0 ? " + " : " - ") + std::to_string(valeur >= 0 ? valeur : -valeur);
}

// Distance entre deux emprises carrees : l'ecart sur chaque axe, puis le plus grand des deux.
[[nodiscard]] int ecartEntreEmprises(GridPosition a, int coteA, GridPosition b,
                                     int coteB) noexcept {
    const int dx =
        std::max({0, b.column - (a.column + coteA - 1), a.column - (b.column + coteB - 1)});
    const int dy = std::max({0, b.row - (a.row + coteA - 1), a.row - (b.row + coteB - 1)});
    return std::max(dx, dy);
}

// Une portee du corpus en cases, arrondie vers le bas : 24 m font 16 cases, 7,50 m en font 5. La
// marge evite de perdre une case sur l'arrondi d'une division par 1,5.
[[nodiscard]] int casesDePortee(float metres) noexcept {
    return static_cast<int>(std::floor(tilesFromMeters(metres) + 1e-4F));
}

// Les portees d'une arme ou d'une action, si la donnee en porte une. Une portee unique fait deux
// nombres egaux.
[[nodiscard]] std::optional<AttackRange> porteeDe(std::optional<float> normale,
                                                  std::optional<float> longue) {
    if (!normale.has_value() || casesDePortee(*normale) <= 0) {
        return std::nullopt;
    }
    const int proche = casesDePortee(*normale);
    return AttackRange{
        .normal = proche,
        .maximum = std::max(proche, longue.has_value() ? casesDePortee(*longue) : 0)};
}

[[nodiscard]] const DamagePipeline& pipelineVide() {
    static const DamagePipeline vide;
    return vide;
}

[[nodiscard]] const AttackHooks& crochetsVides() {
    static const AttackHooks vides;
    return vides;
}

}  // namespace

// --- Profils ----------------------------------------------------------------------------------

CreatureAttacks attacksFor(const Creature& creature) {
    CreatureAttacks attaques;
    for (const CreatureAction& action : creature.actions) {
        if (!action.attackBonus.has_value() || !action.damage.has_value()) {
            continue;
        }
        if (!action.damageType.has_value()) {
            attaques.refused.push_back(action.name + " : degats sans type");
            continue;
        }
        AttackProfile profil;
        profil.label = action.name;
        profil.modifiers.push_back({.source = "bonus d'attaque", .value = *action.attackBonus});
        profil.damage.push_back({.dice = *action.damage, .type = *action.damageType, .flags = 0});
        const std::optional<AttackRange> portee = porteeDe(action.rangeNormal, action.rangeLong);
        if (action.reach.has_value()) {
            AttackProfile contact = profil;
            contact.kind = AttackKind::Melee;
            // 1,50 m = 1 case, 3 m = 2, 4,50 m = 3. Une allonge de 0 m (une nuee qui entre dans
            // l'emplacement de sa cible) frappe au contact : deux creatures ne partagent jamais une
            // case sur cette grille (LOT-19).
            contact.reach =
                std::max(1, static_cast<int>(std::lround(tilesFromMeters(*action.reach))));
            attaques.attacks.push_back(std::move(contact));
        }
        if (!action.reach.has_value() || portee.has_value()) {
            profil.kind = AttackKind::Ranged;
            profil.range = portee;
            attaques.attacks.push_back(std::move(profil));
        }
    }
    return attaques;
}

AttackProfile weaponAttackFor(const CharacterSheet& sheet, const Weapon* weapon,
                              int proficiencyBonus, bool proficient) {
    AttackProfile profil;
    if (weapon == nullptr) {
        const int force = sheet.modifier(Ability::Strength);
        profil.label = "coup a mains nues";
        profil.modifiers = {{.source = "Force", .value = force},
                            {.source = "maitrise", .value = proficiencyBonus}};
        profil.damage.push_back({.dice = Dice{.count = 0, .faces = 0, .modifier = 1 + force},
                                 .type = DamageType::Bludgeoning,
                                 .flags = 0});
        return profil;
    }
    const Ability caracteristique = weaponAttackAbility(sheet, *weapon);
    const int modificateur = sheet.modifier(caracteristique);
    profil.label = weapon->name;
    profil.kind = weapon->ranged ? AttackKind::Ranged : AttackKind::Melee;
    // Manuel, chapitre 5, « Allonge » : l'arme ajoute 1,50 m a l'allonge.
    profil.reach = hasProperty(*weapon, "reach") ? 2 : 1;
    if (weapon->ranged) {
        profil.range = porteeDe(weapon->rangeNormal, weapon->rangeLong);
    }
    profil.modifiers.push_back(
        {.source = std::string(nomDeCaracteristique(caracteristique)), .value = modificateur});
    if (proficient) {
        profil.modifiers.push_back({.source = "maitrise", .value = proficiencyBonus});
    }
    // Le filet n'inflige aucun degat : il touche, et entrave (LOT-72). Une arme a des sans type
    // n'existe pas au catalogue -- le schema l'interdit --, et n'en recoit pas un par defaut.
    if (weapon->damage.has_value() && weapon->damageType.has_value()) {
        Dice des = *weapon->damage;
        des.modifier += modificateur;
        profil.damage.push_back({.dice = des, .type = *weapon->damageType, .flags = 0});
    }
    return profil;
}

std::optional<AttackProfile> thrownAttackFor(const CharacterSheet& sheet, const Weapon& weapon,
                                             int proficiencyBonus, bool proficient) {
    if (!hasProperty(weapon, "thrown")) {
        return std::nullopt;
    }
    // La caracteristique est celle de l'arme telle qu'elle est (weaponAttackAbility) : Force pour
    // une javeline, la meilleure des deux pour une dague de finesse, Dexterite pour une flechette.
    AttackProfile profil = weaponAttackFor(sheet, &weapon, proficiencyBonus, proficient);
    profil.label = weapon.name + " (lancer)";
    profil.kind = AttackKind::Ranged;
    profil.reach = 1;
    profil.range = porteeDe(weapon.rangeNormal, weapon.rangeLong);
    return profil;
}

// --- Geometrie --------------------------------------------------------------------------------

std::optional<int> gridDistanceFrom(const CombatState& combat, CombatantId mover,
                                    GridPosition moverAnchor, CombatantId other) {
    const Combatant* a = combat.find(mover);
    const Combatant* b = combat.find(other);
    const std::optional<GridPosition> ancreB = combat.grid().positionOf(other);
    if (a == nullptr || b == nullptr || !ancreB.has_value()) {
        return std::nullopt;
    }
    return ecartEntreEmprises(moverAnchor, footprintSide(a->profile.size), *ancreB,
                              footprintSide(b->profile.size));
}

std::optional<int> gridDistance(const CombatState& combat, CombatantId from, CombatantId to) {
    const std::optional<GridPosition> ancre = combat.grid().positionOf(from);
    if (!ancre.has_value()) {
        return std::nullopt;
    }
    return gridDistanceFrom(combat, from, *ancre, to);
}

bool inReach(const CombatState& combat, CombatantId attacker, CombatantId target,
             const AttackProfile& profile) {
    const std::optional<int> distance = gridDistance(combat, attacker, target);
    if (!distance.has_value() || attacker == target) {
        return false;
    }
    if (profile.kind == AttackKind::Melee) {
        return *distance <= profile.reach;
    }
    return profile.range.has_value() ? *distance <= profile.range->maximum : *distance <= 1;
}

TargetCheck checkTarget(const CombatState& combat, CombatantId attacker, CombatantId target,
                        const AttackProfile& profile) {
    if (attacker == target || !combat.grid().positionOf(attacker).has_value() ||
        !combat.grid().positionOf(target).has_value()) {
        return TargetCheck::NotOnGrid;
    }
    if (!inReach(combat, attacker, target, profile)) {
        return TargetCheck::OutOfReach;
    }
    return hasLineOfSight(combat, attacker, target) ? TargetCheck::Valid : TargetCheck::TotalCover;
}

AttackCircumstances attackCircumstances(const CombatState& combat, CombatantId attacker,
                                        CombatantId target, const AttackProfile& profile) {
    AttackCircumstances circonstances;
    if (profile.kind != AttackKind::Ranged) {
        return circonstances;
    }
    const Combatant* tireur = combat.find(attacker);
    if (tireur == nullptr) {
        return circonstances;
    }
    for (const CombatantId autre : combat.combatants()) {
        const Combatant* c = combat.find(autre);
        if (c == nullptr || c->profile.side == tireur->profile.side ||
            c->status != CombatantStatus::Standing) {
            continue;
        }
        // « Une creature hostile qui vous voit » : un gobelin de l'autre cote d'un mur, a une case,
        // ne gene pas le tir.
        if (gridDistance(combat, attacker, autre) == 1 && hasLineOfSight(combat, autre, attacker)) {
            circonstances.disadvantages.emplace_back("tir au contact d'un ennemi");
            break;
        }
    }
    const std::optional<int> distance = gridDistance(combat, attacker, target);
    if (profile.range.has_value() && distance.has_value() && *distance > profile.range->normal) {
        circonstances.disadvantages.emplace_back("longue portee");
    }
    return circonstances;
}

// --- Le jet -----------------------------------------------------------------------------------

void AttackRoll::recompute() {
    if (check.dice.empty()) {
        check.keptDie = 0;
    } else {
        switch (check.stance) {
            case RollStance::Advantage:
                check.keptDie = *std::ranges::max_element(check.dice);
                break;
            case RollStance::Disadvantage:
                check.keptDie = *std::ranges::min_element(check.dice);
                break;
            case RollStance::Normal:
                check.keptDie = check.dice.front();
                break;
        }
    }
    check.target = armorClass;
    check.total = check.keptDie;
    for (const Modifier& modificateur : check.modifiers) {
        check.total += modificateur.value;
    }
}

void AttackRoll::substitute(std::size_t die, int value, const std::string& source) {
    if (die >= check.dice.size()) {
        return;
    }
    const int avant = check.dice[die];
    check.dice[die] = std::clamp(value, 1, D20_FACES);
    amendments.push_back("substitution (" + source + ") : " + std::to_string(avant) + " -> " +
                         std::to_string(check.dice[die]));
    recompute();
}

void AttackRoll::addModifier(Modifier modifier) {
    check.modifiers.push_back(std::move(modifier));
    recompute();
}

void AttackRoll::applyCover(Cover level) {
    // Total n'est pas un bonus ; un abri egal ou moindre est deja compte.
    if (level == Cover::Total || level <= cover) {
        return;
    }
    const int avant = armorClass;
    armorClass += coverBonus(level) - coverBonus(cover);
    cover = level;
    amendments.push_back(std::string(coverLabel(level)) + " : CA " + std::to_string(avant) +
                         " -> " + std::to_string(armorClass));
    recompute();
}

void AttackHooks::insert(AttackRollStage stage, AttackRollListener listener) {
    _listeners.emplace_back(stage, std::move(listener));
}

void AttackHooks::run(AttackRollStage stage, AttackRoll& roll, DeterministicRandom& random) const {
    for (const auto& [etape, greffon] : _listeners) {
        if (etape == stage) {
            greffon(roll, random);
        }
    }
}

AttackRoll rollAttack(AttackRoll request, const AttackHooks& hooks, DeterministicRandom& random) {
    AttackRoll jet = std::move(request);
    hooks.run(AttackRollStage::BeforeRoll, jet, random);

    jet.check.stance = rollStance(static_cast<int>(jet.advantages.size()),
                                  static_cast<int>(jet.disadvantages.size()));
    jet.check.dice.clear();
    jet.check.dice.push_back(random.nextInt(1, D20_FACES));
    if (jet.check.stance != RollStance::Normal) {
        jet.check.dice.push_back(random.nextInt(1, D20_FACES));
    }
    jet.recompute();
    hooks.run(AttackRollStage::DiceRolled, jet, random);
    jet.recompute();
    hooks.run(AttackRollStage::BeforeOutcome, jet, random);
    jet.recompute();

    // L'issue est figee ici, et nulle part ailleurs. Manuel, « Faire 1 ou 20 » : le 1 rate et le
    // 20 touche « peu importe les modificateurs ou la CA de la cible ».
    if (jet.check.isNaturalOne()) {
        jet.hit = false;
        jet.critical = false;
    } else if (jet.check.keptDie >= jet.criticalThreshold) {
        jet.hit = true;
        jet.critical = true;
    } else {
        jet.hit = jet.check.succeeded();
        jet.critical = false;
    }
    return jet;
}

// --- La resolution ----------------------------------------------------------------------------

std::optional<AttackOutcome> resolveAttack(CombatState& combat, CombatantId attacker,
                                           CombatantId target, const AttackProfile& profile,
                                           DeterministicRandom& random,
                                           const AttackContext& context) {
    if (attacker == target || !combat.declareAttack(attacker, target)) {
        return std::nullopt;
    }
    const Combatant* attaquant = combat.find(attacker);
    const Combatant* cible = combat.find(target);
    if (attaquant == nullptr || cible == nullptr) {
        return std::nullopt;
    }

    AttackOutcome issue;
    issue.attackerName = attaquant->profile.name;
    issue.targetName = cible->profile.name;

    AttackRoll demande;
    demande.attacker = attacker;
    demande.target = target;
    demande.label = profile.label;
    demande.armorClass = cible->profile.armorClass;
    demande.criticalThreshold = profile.criticalThreshold;
    demande.check.modifiers = profile.modifiers;
    const AttackCircumstances grille = attackCircumstances(combat, attacker, target, profile);
    for (const AttackCircumstances* source : {&grille, &context.circumstances}) {
        demande.advantages.insert(demande.advantages.end(), source->advantages.begin(),
                                  source->advantages.end());
        demande.disadvantages.insert(demande.disadvantages.end(), source->disadvantages.begin(),
                                     source->disadvantages.end());
    }
    // L'abri que la grille dit, avant tout greffon : un greffon qui en pose un autre passe par
    // applyCover, et le meilleur seul compte.
    demande.applyCover(coverBetween(combat, attacker, target));

    issue.roll = rollAttack(std::move(demande),
                            context.hooks != nullptr ? *context.hooks : crochetsVides(), random);
    if (!issue.roll.hit) {
        return issue;
    }
    issue.damage = rollDamage(profile.damage, issue.roll.critical, random);
    const std::vector<DamageRequest> salve{{.target = target, .damage = issue.damage}};
    const DamagePipeline& pipeline =
        context.pipeline != nullptr ? *context.pipeline : pipelineVide();
    std::vector<DamageReport> rapports = pipeline.apply(combat, salve);
    if (!rapports.empty()) {
        issue.report = std::move(rapports.front());
    }
    return issue;
}

std::string AttackOutcome::describe() const {
    const CheckResult& jet = roll.check;
    std::string texte =
        "attaque " + attackerName + " -> " + targetName + " (" + roll.label + ") : d20";
    if (jet.stance != RollStance::Normal) {
        texte += " (";
        texte += rollStanceName(jet.stance);
        for (std::size_t i = 0; i < jet.dice.size(); ++i) {
            texte += (i == 0 ? " : " : ", ") + std::to_string(jet.dice[i]);
        }
        const std::vector<std::string>& sources =
            jet.stance == RollStance::Advantage ? roll.advantages : roll.disadvantages;
        for (const std::string& source : sources) {
            texte += " ; " + source;
        }
        texte += ')';
    }
    for (const std::string& amendement : roll.amendments) {
        texte += " [" + amendement + ']';
    }
    texte += " = " + std::to_string(jet.keptDie);
    for (const Modifier& modificateur : jet.modifiers) {
        texte += signe(modificateur.value) + " (" + modificateur.source + ')';
    }
    texte += " = " + std::to_string(jet.total) + " contre CA " + std::to_string(roll.armorClass);
    if (!roll.hit) {
        texte += jet.isNaturalOne() ? " : rate (1 naturel)" : " : rate";
        return texte;
    }
    texte +=
        roll.critical ? " : critique (" + std::to_string(jet.keptDie) + " naturel)" : " : touche";
    for (const RolledDamage& lance : damage) {
        texte += " ; degats " + lance.roll.describe();
        if (lance.critical) {
            texte += " (des doubles)";
        }
        texte += ' ';
        texte += damageTypeLabel(lance.clause.type);
    }
    if (report.has_value()) {
        for (const DamageStep& etape : report->work.trace) {
            texte += " ; " + etape.source + ' ' + std::to_string(etape.before) + " -> " +
                     std::to_string(etape.after);
        }
        texte += " ; PV " + std::to_string(report->hitPointsBefore) + " -> " +
                 std::to_string(report->hitPointsAfter);
    }
    return texte;
}

}  // namespace core
