// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Combat/Damage.h"

#include <algorithm>
#include <map>

#include "Core/Combat/BattleGrid.h"
#include "Core/Combat/CombatState.h"
#include "Core/Rpg/Bestiary.h"

namespace core {

bool DamageTraits::applies(DamageAffinityKind kind, DamageType type, DamageFlags flags) const {
    return std::ranges::any_of(affinities, [&](const DamageAffinity& affinite) {
        return affinite.kind == kind && affinite.type == type &&
               (affinite.bypassedBy & flags) == 0U;
    });
}

std::vector<RolledDamage> rollDamage(std::span<const DamageClause> clauses, bool critical,
                                     DeterministicRandom& random) {
    std::vector<RolledDamage> lances;
    lances.reserve(clauses.size());
    for (const DamageClause& clause : clauses) {
        // Manuel, chapitre 9, « Coups critiques » : on lance deux fois TOUS les des de degats, puis
        // on ajoute les modificateurs comme a l'accoutumee. Le modificateur n'est jamais double.
        Dice des = clause.dice;
        if (critical) {
            des.count *= 2;
        }
        RolledDamage lance{.clause = clause, .roll = rollDice(des, random), .critical = critical};
        // « Vous pouvez infliger un nombre de degats egal a 0, mais jamais une quantite negative. »
        lance.amount = std::max(0, lance.roll.total);
        lances.push_back(std::move(lance));
    }
    return lances;
}

std::string_view damageTypeLabel(DamageType type) noexcept {
    // Les noms du Manuel, chapitre 9, « Types de degats ». switch exhaustif sans default.
    switch (type) {
        case DamageType::Acid:
            return "acide";
        case DamageType::Bludgeoning:
            return "contondant";
        case DamageType::Cold:
            return "froid";
        case DamageType::Fire:
            return "feu";
        case DamageType::Force:
            return "force";
        case DamageType::Lightning:
            return "foudre";
        case DamageType::Necrotic:
            return "necrotique";
        case DamageType::Piercing:
            return "perforant";
        case DamageType::Poison:
            return "poison";
        case DamageType::Psychic:
            return "psychique";
        case DamageType::Radiant:
            return "radiant";
        case DamageType::Slashing:
            return "tranchant";
        case DamageType::Thunder:
            return "tonnerre";
    }
    return "?";
}

std::string_view damageStageName(DamageStage stage) noexcept {
    switch (stage) {
        case DamageStage::Source:
            return "source";
        case DamageStage::Conversion:
            return "conversion";
        case DamageStage::Resistances:
            return "resistances";
        case DamageStage::Reserves:
            return "reserves";
        case DamageStage::HitPoints:
            return "points de vie";
    }
    return "?";
}

int DamageWork::total() const {
    int somme = 0;
    for (const DamagePortion& portion : portions) {
        somme += portion.amount;
    }
    return somme;
}

void DamageWork::adjust(std::size_t portion, int amount, DamageStage stage, std::string source) {
    if (portion >= portions.size()) {
        return;
    }
    const int avant = portions[portion].amount;
    const int apres = std::max(0, amount);
    if (avant == apres) {
        return;
    }
    portions[portion].amount = apres;
    trace.push_back({.stage = stage, .source = std::move(source), .before = avant, .after = apres});
}

void DamagePipeline::insert(DamageStage stage, DamageListener listener) {
    _listeners.emplace_back(stage, std::move(listener));
}

void DamagePipeline::runStage(DamageStage stage, DamageWork& work, CombatState* combat) const {
    for (const auto& [etape, greffon] : _listeners) {
        if (etape == stage) {
            greffon(work, combat);
        }
    }
}

void DamagePipeline::applyAffinities(DamageWork& work, const DamageTraits& traits) {
    for (std::size_t indice = 0; indice < work.portions.size(); ++indice) {
        const DamagePortion portion = work.portions[indice];
        if (portion.amount <= 0 || hasFlag(portion.flags, DamageFlag::IgnoresResistance)) {
            continue;
        }
        const std::string type(damageTypeLabel(portion.type));
        if (traits.applies(DamageAffinityKind::Immunity, portion.type, portion.flags)) {
            work.adjust(indice, 0, DamageStage::Resistances, "immunite (" + type + ")");
            continue;
        }
        // Manuel, chapitre 9 : « la resistance PUIS la vulnerabilite sont appliquees apres tous les
        // autres modificateurs », et plusieurs resistances au meme type ne comptent qu'une fois.
        // Moitie arrondie a l'inferieur : 25 resiste et vulnerable donne 12 puis 24, pas 25.
        if (traits.applies(DamageAffinityKind::Resistance, portion.type, portion.flags)) {
            work.adjust(indice, work.portions[indice].amount / 2, DamageStage::Resistances,
                        "resistance (" + type + ")");
        }
        if (traits.applies(DamageAffinityKind::Vulnerability, portion.type, portion.flags)) {
            work.adjust(indice, work.portions[indice].amount * 2, DamageStage::Resistances,
                        "vulnerabilite (" + type + ")");
        }
    }
}

namespace {

[[nodiscard]] DamageWork travailDepuis(std::span<const RolledDamage> degats) {
    DamageWork travail;
    for (const RolledDamage& lance : degats) {
        travail.portions.push_back(
            {.type = lance.clause.type, .flags = lance.clause.flags, .amount = lance.amount});
    }
    return travail;
}

// Consomme les reserves, la plus recente d'abord, et rend ce qui reste a perdre en PV.
int absorber(DamageWork& travail, std::vector<HitPointReserve>& reserves, int montant) {
    for (auto it = reserves.rbegin(); it != reserves.rend() && montant > 0; ++it) {
        const int pris = std::min(it->amount, montant);
        if (pris <= 0) {
            continue;
        }
        travail.trace.push_back({.stage = DamageStage::Reserves,
                                 .source = it->source,
                                 .before = montant,
                                 .after = montant - pris});
        it->amount -= pris;
        montant -= pris;
        travail.absorbed += pris;
    }
    std::erase_if(reserves, [](const HitPointReserve& r) { return r.amount <= 0; });
    return montant;
}

}  // namespace

std::vector<DamageReport> DamagePipeline::apply(CombatState& combat,
                                                std::span<const DamageRequest> volley) const {
    std::vector<DamageReport> rapports;
    std::vector<HitPointChange> pertes;
    // Les PV que chaque cible aura une fois les pertes precedentes de la salve appliquees : deux
    // demandes pour la meme cible se suivent, et l'excedent de la seconde se mesure apres la
    // premiere.
    std::map<CombatantId, int> pvCourants;
    for (const DamageRequest& demande : volley) {
        DamageReport rapport;
        rapport.work = travailDepuis(demande.damage);
        rapport.work.target = demande.target;
        const Combatant* cible = combat.find(demande.target);
        if (cible == nullptr || cible->status == CombatantStatus::Withdrawn) {
            rapports.push_back(std::move(rapport));
            continue;
        }
        const DamageTraits traits = cible->profile.damageTraits;
        const bool critique = std::ranges::any_of(
            demande.damage, [](const RolledDamage& lance) { return lance.critical; });
        const auto position =
            pvCourants.try_emplace(demande.target, cible->profile.currentHitPoints).first;

        runStage(DamageStage::Source, rapport.work, &combat);
        runStage(DamageStage::Conversion, rapport.work, &combat);
        runStage(DamageStage::Resistances, rapport.work, &combat);
        applyAffinities(rapport.work, traits);
        runStage(DamageStage::Reserves, rapport.work, &combat);

        int reste = 0;
        int absorbable = 0;
        for (const DamagePortion& portion : rapport.work.portions) {
            (hasFlag(portion.flags, DamageFlag::IgnoresReserves) ? reste : absorbable) +=
                portion.amount;
        }
        if (std::vector<HitPointReserve>* reserves = combat.reserves(demande.target)) {
            absorbable = absorber(rapport.work, *reserves, absorbable);
        }
        rapport.work.hitPointLoss = reste + absorbable;
        runStage(DamageStage::HitPoints, rapport.work, &combat);

        rapport.hitPointsBefore = position->second;
        rapport.hitPointsAfter = std::max(0, position->second - rapport.work.hitPointLoss);
        rapport.overflow = std::max(0, rapport.work.hitPointLoss - position->second);
        position->second = rapport.hitPointsAfter;
        if (rapport.work.hitPointLoss > 0) {
            pertes.push_back({.target = demande.target,
                              .amount = rapport.work.hitPointLoss,
                              .critical = critique});
        }
        rapports.push_back(std::move(rapport));
    }
    // Une salve, un appel : l'issue ne depend pas de l'ordre des cibles (LOT-20).
    combat.applyDamage(pertes);
    return rapports;
}

DamageReport DamagePipeline::applyToStructure(BattleGrid& grid, GridPosition cell,
                                              std::span<const RolledDamage> damage) const {
    DamageReport rapport;
    rapport.work = travailDepuis(damage);
    rapport.work.structure = cell;
    const GridObject* objet = grid.objectAt(cell);
    if (objet == nullptr) {
        return rapport;
    }
    const DamageTraits traits = objet->damageTraits;
    rapport.hitPointsBefore = objet->hitPoints;
    runStage(DamageStage::Source, rapport.work, nullptr);
    runStage(DamageStage::Conversion, rapport.work, nullptr);
    runStage(DamageStage::Resistances, rapport.work, nullptr);
    applyAffinities(rapport.work, traits);
    rapport.work.hitPointLoss = rapport.work.total();
    runStage(DamageStage::HitPoints, rapport.work, nullptr);
    rapport.hitPointsAfter = std::max(0, rapport.hitPointsBefore - rapport.work.hitPointLoss);
    rapport.overflow = std::max(0, rapport.work.hitPointLoss - rapport.hitPointsBefore);
    grid.damageObject(cell, rapport.work.hitPointLoss);
    return rapport;
}

DamageTraits damageTraitsFor(const Creature& creature) {
    DamageTraits traits;
    const auto ajouter = [&](const std::vector<DamageType>& types, DamageAffinityKind genre) {
        for (const DamageType type : types) {
            traits.affinities.push_back({.type = type, .kind = genre, .bypassedBy = 0});
        }
    };
    ajouter(creature.damageImmunities, DamageAffinityKind::Immunity);
    ajouter(creature.damageResistances, DamageAffinityKind::Resistance);
    ajouter(creature.damageVulnerabilities, DamageAffinityKind::Vulnerability);
    return traits;
}

}  // namespace core
