// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_class_scoundrel.cpp
 * @brief Le Scoundrel du *Player's Guide to Tanares* (p. 204-207) se joue du niveau 1 au
 *        niveau 5 (`LOT-135`) : *Sneak Attack Simplified*, *Scoundrel's Agility*, *Adventurer's
 *        Aptitude*, *Precise Striker*, et la table qui les donne.
 *
 * L'attaque sournoise n'a de sens qu'en groupe : ses tests posent un allié au contact de la cible,
 * puis le retirent, et vérifient que les dés suivent.
 */

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/ActionEconomy.h"
#include "Core/Combat/Arena.h"
#include "Core/Rpg/ClassCapacities.h"
#include "Core/Rpg/Skill.h"
#include "Test/Support/ClassArena.h"

namespace {

using core::CombatantId;
using core::CombatSide;
using test_support::journalHas;
using test_support::journalLine;

const std::string SCOUNDREL = "Nessa Double-Vie";

/// La fiche de la page 207 au niveau @p niveau, sans avertissement de la table.
[[nodiscard]] core::LoadedCharacterSheet scoundrel(int niveau = 1) {
    core::LoadedCharacterSheet charge = test_support::loadPremade("heros-scoundrel.json");
    EXPECT_TRUE(charge.errors.empty());
    if (niveau > 1) {
        EXPECT_TRUE(test_support::levelUpTo(charge.sheet, niveau).empty());
    }
    return charge;
}

/// La Scoundrel en (3, 3) a la rapiere, un mannequin a la CA 0 en (4, 3), et @p allie.
[[nodiscard]] core::ArenaBout combatDe(const core::LoadedCharacterSheet& charge,
                                       std::optional<core::GridPosition> allie) {
    core::ArenaBout bout{.seed = 1, .lethal = true, .heroicMark = false};
    bout.contestants.push_back(
        test_support::hero(charge.sheet, charge.inventory,
                           {test_support::weaponAttack(charge.sheet, "rapiere")}, {3, 3}));
    bout.contestants.push_back(test_support::dummy("Mannequin", {4, 3}, 0, 0, 200));
    if (allie.has_value()) {
        bout.contestants.push_back(
            test_support::dummy("Allie", *allie, 10, 0, 60, CombatSide::Allies));
    }
    return bout;
}

/// La premiere attaque qui touche, en cherchant la graine : un 1 naturel rate toujours.
[[nodiscard]] std::optional<core::AttackOutcome> premiereTouche(core::ArenaSession& session,
                                                                core::ArenaBout bout) {
    for (std::uint64_t graine = 1; graine < 40; ++graine) {
        bout.seed = graine;
        EXPECT_TRUE(session.mount(bout).refusals.empty());
        EXPECT_TRUE(session.start());
        const core::ArenaAttack coup = session.attack(CombatantId{2});
        if (coup.outcome.has_value() && coup.outcome->roll.hit) {
            return coup.outcome;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::size_t desSournois(const core::AttackOutcome& coup) {
    std::size_t clauses = 0;
    for (const core::RolledDamage& degats : coup.damage) {
        if (degats.source == "Sneak Attack Simplified") {
            ++clauses;
        }
    }
    return clauses;
}

}  // namespace

/**
 * @brief La fiche pre-tiree du Scoundrel porte ses deux capacites de niveau 1 et ses 40 ft.
 * \castest{<b>La fiche de la page 207 se charge avec Sneak Attack Simplified et Scoundrel's
 * Agility ; sa vitesse est de 12 m, huit cases.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger heros-scoundrel.json.<br/>2. Lire capacites, vitesse, CA, profil.<br/>
 * \tattendu Aucun avertissement ; deux capacites ; 12 m, 8 cases ; CA 14.
 * }
 */
TEST(ClassScoundrelTest, LaFichePreTireePorteSesCapacitesDeNiveau1) {
    const core::LoadedCharacterSheet charge = scoundrel();
    EXPECT_TRUE(charge.warnings.empty()) << (charge.warnings.empty() ? "" : charge.warnings[0]);
    EXPECT_EQ(test_support::capacityIds(charge.sheet),
              (std::vector<std::string>{"sneak-attack-simplified", "scoundrels-agility"}));
    EXPECT_FLOAT_EQ(charge.sheet.effectiveSpeedMeters(), 12.0F);
    EXPECT_EQ(core::profileFor(charge.sheet).movement, 8);
    EXPECT_EQ(test_support::armorClassOf(charge.sheet, charge.inventory), 14);
}

/**
 * @brief Sneak Attack Simplified : +1d8 a la premiere touche du tour contre une cible adjacente a
 *        un allie, et seulement alors.
 * \castest{<b>Avec un allie au contact du mannequin, la rapiere ajoute 1d8 nomme ; une seconde
 * touche du meme tour n'ajoute rien ; sans allie au contact, ou avec un allie a terre, rien.</b>
 * <br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Scoundrel N1, mannequin CA 0, allie en (5, 3).<br/>2. Attaquer, octroyer une
 * action, rattaquer.<br/>3. Refaire avec l'allie en (8, 7), puis avec l'allie au contact mais a
 * terre.<br/>
 * \tattendu Une clause 1d8 « (Sneak Attack Simplified) », puis aucune ; aucune sans allie ;
 * aucune avec l'allie a terre.
 * }
 */
TEST(ClassScoundrelTest, LAttaqueSournoiseDemandeUnAllieAuContact) {
    const core::LoadedCharacterSheet charge = scoundrel();
    {
        core::ArenaSession session(test_support::room());
        const std::optional<core::AttackOutcome> coup =
            premiereTouche(session, combatDe(charge, core::GridPosition{5, 3}));
        ASSERT_TRUE(coup.has_value());
        ASSERT_EQ(desSournois(*coup), 1U);
        const auto sournois =
            std::ranges::find(coup->damage, "Sneak Attack Simplified", &core::RolledDamage::source);
        EXPECT_EQ(sournois->clause.dice, (core::Dice{.count = 1, .faces = 8, .modifier = 0}));
        EXPECT_EQ(sournois->clause.type, core::DamageType::Piercing) << "du type de l'arme";
        EXPECT_NE(journalLine(session.journal(), "attaque " + SCOUNDREL + " -> Mannequin")
                      .find("perforant (Sneak Attack Simplified)"),
                  std::string::npos);
        session.combat().economy(CombatantId{1})->grant(core::ACTION_RESOURCE);
        const core::ArenaAttack second = session.attack(CombatantId{2});
        ASSERT_TRUE(second.outcome.has_value());
        EXPECT_EQ(desSournois(*second.outcome), 0U) << "une fois par tour";
    }
    {
        core::ArenaSession session(test_support::room());
        const std::optional<core::AttackOutcome> coup =
            premiereTouche(session, combatDe(charge, core::GridPosition{8, 7}));
        ASSERT_TRUE(coup.has_value());
        EXPECT_EQ(desSournois(*coup), 0U) << "aucun allie au contact de la cible";
        EXPECT_EQ(session.combat().counters().value(core::CounterScope::Turn, "1",
                                                    "sneak-attack-simplified"),
                  0)
            << "sans allie, la fois du tour n'est pas consommee";
    }
    {
        core::ArenaSession session(test_support::room());
        core::ArenaBout bout = combatDe(charge, core::GridPosition{5, 3});
        std::optional<core::AttackOutcome> coup;
        for (std::uint64_t graine = 1; graine < 40 && !coup.has_value(); ++graine) {
            bout.seed = graine;
            ASSERT_TRUE(session.mount(bout).refusals.empty());
            ASSERT_TRUE(session.start());
            session.combat().applyDamage(CombatantId{3}, 60);
            const core::ArenaAttack tentative = session.attack(CombatantId{2});
            if (tentative.outcome.has_value() && tentative.outcome->roll.hit) {
                coup = tentative.outcome;
            }
        }
        ASSERT_TRUE(coup.has_value());
        EXPECT_EQ(desSournois(*coup), 0U) << "un allie a terre ne compte pas";
    }
}

/**
 * @brief Les des de l'attaque sournoise suivent la table : 2d8 au niveau 3, 3d8 au niveau 5.
 * \castest{<b>Au niveau 3 la capacite active donne 2d8, au niveau 5 3d8, et une seule a la
 * fois.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Monter le Scoundrel au niveau 3, puis 5.<br/>2. Lire les des en plus.<br/>
 * \tattendu Une seule source, 2d8 puis 3d8, une fois par tour, allie au contact exige.
 * }
 */
TEST(ClassScoundrelTest, LesDesSournoisMontentAvecLaTable) {
    for (const auto& [niveau, des] : {std::pair{3, 2}, std::pair{5, 3}}) {
        const core::LoadedCharacterSheet charge = scoundrel(niveau);
        const std::vector<core::NamedExtraDamage> supplements =
            core::extraDamageFrom(charge.sheet.capacities);
        ASSERT_EQ(supplements.size(), 1U) << niveau;
        EXPECT_EQ(supplements.front().dice, (core::Dice{.count = des, .faces = 8, .modifier = 0}));
        EXPECT_TRUE(supplements.front().oncePerTurn);
        EXPECT_TRUE(supplements.front().allyAdjacentToTarget);
    }
}

/**
 * @brief Scoundrel's Agility : pas d'attaque d'opportunite, +10 ft ; au niveau 5, +2 a la CA.
 * \castest{<b>La Scoundrel quitte l'allonge du mannequin sans etre frappee, et le journal nomme
 * la capacite ; au niveau 5 sa CA passe a 16.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Scoundrel N1 au contact d'un mannequin qui frappe.<br/>2. Marcher a quatre
 * cases.<br/>3. Lire la CA au niveau 5.<br/>
 * \tattendu Aucune ligne « opportunite : » ; « sans attaque d'opportunite Nessa Double-Vie
 * (Scoundrel's Agility) » ; CA 16.
 * }
 */
TEST(ClassScoundrelTest, ScoundrelsAgilityEviteLesAttaquesDOpportunite) {
    const core::LoadedCharacterSheet charge = scoundrel();
    core::ArenaSession session(test_support::room());
    ASSERT_TRUE(session.mount(combatDe(charge, std::nullopt)).refusals.empty());
    ASSERT_TRUE(session.start());
    EXPECT_TRUE(session.previewOpportunities({0, 3}).empty());
    ASSERT_EQ(session.move({0, 3}).result, core::MoveResult::Moved);
    EXPECT_FALSE(journalHas(session.journal(), "opportunite :"));
    EXPECT_TRUE(journalHas(session.journal(),
                           "sans attaque d'opportunite " + SCOUNDREL + " (Scoundrel's Agility)"));

    const core::LoadedCharacterSheet niveau5 = scoundrel(5);
    EXPECT_EQ(test_support::armorClassOf(niveau5.sheet, niveau5.inventory), 16)
        << "cuir 11 + Dex 3 + 2";
    EXPECT_FLOAT_EQ(niveau5.sheet.effectiveSpeedMeters(), 12.0F) << "les 10 ft restent";
    EXPECT_TRUE(core::opportunityImmunityFrom(niveau5.sheet.capacities).has_value());
}

/**
 * @brief Adventurer's Aptitude (N3) : +1 aux tests maitrises, rien aux autres.
 * \castest{<b>Au niveau 3, Acrobaties (maitrisee) passe de +5 a +6 ; Athletisme (non maitrisee)
 * reste a -1.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Scoundrel N1 puis N3.<br/>2. Lire les modificateurs de competence.<br/>
 * \tattendu N1 : +5 et -1 ; N3 : +6 et -1.
 * }
 */
TEST(ClassScoundrelTest, AdventurersAptitudeAjouteAuxTestsMaitrises) {
    const core::SkillCatalog competences =
        core::loadSkills(std::filesystem::path{JADG_RPG_DIR} / "skills");
    const core::ExperienceTable& table = test_support::rpgCatalogs().experience;
    const core::LoadedCharacterSheet n1 = scoundrel();
    const core::LoadedCharacterSheet n3 = scoundrel(3);
    EXPECT_EQ(core::skillModifier(n1.sheet, table, competences, "acrobatics").value, 5);
    EXPECT_EQ(core::skillModifier(n3.sheet, table, competences, "acrobatics").value, 6);
    EXPECT_EQ(core::skillModifier(n1.sheet, table, competences, "athletics").value, -1);
    EXPECT_EQ(core::skillModifier(n3.sheet, table, competences, "athletics").value, -1);
}

/**
 * @brief Precise Striker (N5) : +1 aux jets d'attaque, nomme.
 * \castest{<b>Au niveau 5, le jet de la rapiere porte « + 1 (Precise Striker) ».</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Scoundrel N5 contre le mannequin.<br/>2. Attaquer.<br/>
 * \tattendu Le modificateur Precise Striker vaut +1.
 * }
 */
TEST(ClassScoundrelTest, PreciseStrikerAjouteUnAuJet) {
    const core::LoadedCharacterSheet charge = scoundrel(5);
    core::ArenaSession session(test_support::room());
    ASSERT_TRUE(session.mount(combatDe(charge, std::nullopt)).refusals.empty());
    ASSERT_TRUE(session.start());
    const core::ArenaAttack coup = session.attack(CombatantId{2});
    ASSERT_TRUE(coup.outcome.has_value());
    const std::vector<core::Modifier>& modificateurs = coup.outcome->roll.check.modifiers;
    const auto bonus = std::ranges::find(modificateurs, "Precise Striker", &core::Modifier::source);
    ASSERT_NE(bonus, modificateurs.end());
    EXPECT_EQ(bonus->value, 1);
    EXPECT_TRUE(journalHas(session.journal(), "+ 1 (Precise Striker)"));
}

/**
 * @brief Du niveau 1 au niveau 5, la table du Scoundrel se lit marche par marche.
 * \castest{<b>Monter la Scoundrel de la page 207 jusqu'au niveau 5 donne les capacites de la
 * table, chaque amelioration remplacant la precedente.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Monter de 1 a 5, lire les capacites.<br/>
 * \tattendu Aucun manquant ; au niveau 5 : Experience, Adventurer's Aptitude, Ability Score
 * Improvement, attaque sournoise a 3d8, Scoundrel's Agility du niveau 5, Precise Striker.
 * }
 */
TEST(ClassScoundrelTest, DuNiveau1AuNiveau5LaTableSeLit) {
    core::LoadedCharacterSheet charge = scoundrel();
    for (int niveau = 2; niveau <= 5; ++niveau) {
        EXPECT_TRUE(test_support::levelUpTo(charge.sheet, niveau).empty()) << niveau;
    }
    EXPECT_EQ(test_support::capacityIds(charge.sheet),
              (std::vector<std::string>{"experience", "adventurers-aptitude",
                                        "ability-score-improvement", "sneak-attack-simplified-3d8",
                                        "scoundrels-agility-armor", "precise-striker"}));
    EXPECT_EQ(core::proficiencyBonus(charge.sheet, test_support::rpgCatalogs().experience), 3);
}
