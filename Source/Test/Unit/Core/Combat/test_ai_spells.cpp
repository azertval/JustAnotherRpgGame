// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_ai_spells.cpp
 * @brief L'IA joue les sorts et les gestes gratuits des quatre classes (`LOT-142`) : le Priest
 *        relève, soigne et bénit, le Mage lance ses sorts et épargne ses alliés, le Brawler frappe
 *        deux fois au niveau 5, l'arme spirituelle frappe par l'action bonus.
 *
 * Chaque héros est la fiche pré-tirée, montée au niveau voulu, jouée par le profil `aggressive`
 * — celui de la simulation d'équilibrage — contre des mannequins.
 */

#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/Arena.h"
#include "Core/Combat/EnemyAi.h"
#include "Core/Rpg/Spell.h"
#include "Test/Support/ClassArena.h"

namespace {

using core::CombatantId;
using core::CombatSide;
using test_support::journalHas;

constexpr const char* PROFIL = "aggressive";

[[nodiscard]] core::BehaviorCatalog profils() {
    return core::loadBehaviors(std::filesystem::path(JADG_RPG_RULES_DIR) / "behaviors.json");
}

/// La fiche pré-tirée @p fichier au niveau @p niveau.
[[nodiscard]] core::LoadedCharacterSheet fiche(const char* fichier, int niveau) {
    core::LoadedCharacterSheet charge = test_support::loadPremade(fichier);
    EXPECT_TRUE(charge.errors.empty());
    if (niveau > 1) {
        EXPECT_TRUE(test_support::levelUpTo(charge.sheet, niveau).empty());
    }
    return charge;
}

/// Le héros de @p charge, joué par l'IA, en @p place, avec son arme @p arme.
[[nodiscard]] core::ArenaContestant joue(const core::LoadedCharacterSheet& charge, const char* arme,
                                         core::GridPosition place) {
    core::ArenaContestant heros = test_support::hero(
        charge.sheet, charge.inventory, {test_support::weaponAttack(charge.sheet, arme)}, place);
    heros.behavior = PROFIL;
    return heros;
}

/// Les lancers restants du sort @p id du combattant 1.
[[nodiscard]] int lancersDe(const core::ArenaSession& session, const char* id) {
    const std::vector<core::ArenaSpell>* grimoire = session.spells(CombatantId{1});
    for (std::size_t i = 0; grimoire != nullptr && i < grimoire->size(); ++i) {
        if ((*grimoire)[i].id == id) {
            return (*grimoire)[i].uses;
        }
    }
    ADD_FAILURE() << id << " absent du grimoire";
    return -2;
}

[[nodiscard]] std::string journal(const core::ArenaSession& session) {
    std::string texte;
    for (const std::string& ligne : session.journal()) {
        texte += ligne + "\n";
    }
    return texte;
}

}  // namespace

/**
 * @brief Un allié à terre passe avant l'ennemi : le Priest joué par l'IA le relève par *soin des
 *        blessures*.
 * \castest{<b>L'IA du Priest releve un allie tombe.</b><br/>
 * \tcat Unitaire · IA tactique<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Priest N1 en (1, 3), un allie en (3, 3) porte a 0 PV, un mannequin ennemi en (9,
 * 6).<br/>2. Jouer le tour du Priest par l'IA.<br/>
 * \tattendu Le journal dit « releve Allie » ; l'allie est debout ; un lancer de soin est
 * depense.
 * }
 */
TEST(AiSpellsTest, LePriestReleveUnAllieATerre) {
    const core::LoadedCharacterSheet priest = fiche("heros-priest.json", 1);
    core::ArenaSession session(test_support::room());
    core::ArenaBout bout{.seed = 3, .lethal = true, .heroicMark = false};
    bout.contestants.push_back(joue(priest, "marteau-de-guerre", {1, 3}));
    bout.contestants.push_back(test_support::dummy("Allie", {3, 3}, 10, 0, 20, CombatSide::Allies));
    bout.contestants.push_back(test_support::dummy("Mannequin", {9, 6}, 10, 0));
    const core::BehaviorCatalog catalogue = profils();
    session.setOpportunityPolicy(core::aiOpportunityPolicy(catalogue));
    ASSERT_TRUE(session.mount(bout).refusals.empty());
    ASSERT_TRUE(session.start());
    session.combat().applyDamage(CombatantId{2}, 20);
    ASSERT_EQ(session.combat().find(CombatantId{2})->status, core::CombatantStatus::Down);

    ASSERT_TRUE(core::playTurn(session, catalogue));
    EXPECT_TRUE(journalHas(session.journal(), "releve Allie")) << journal(session);
    EXPECT_EQ(session.combat().find(CombatantId{2})->status, core::CombatantStatus::Standing);
    EXPECT_EQ(lancersDe(session, "cure-wounds"), 1);
}

/**
 * @brief Sans ennemi à portée de ses sorts qui blessent, le Priest bénit : lui et ses alliés
 *        proches, sous concentration, une fois.
 * \castest{<b>L'IA du Priest benit le groupe, une seule fois.</b><br/>
 * \tcat Unitaire · IA tactique<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Priest N1 en (1, 3), deux allies en (2, 3) et (1, 4), un mannequin ennemi en (11,
 * 7).<br/>2. Jouer le tour du Priest, finir les tours des autres, rejouer le Priest.<br/>
 * \tattendu Premier tour : « benit 3 allie(s) », le Priest est beni et concentre ; second tour :
 * pas de seconde benediction.
 * }
 */
TEST(AiSpellsTest, LePriestBenitUneFois) {
    const core::LoadedCharacterSheet priest = fiche("heros-priest.json", 1);
    core::ArenaSession session(test_support::room());
    core::ArenaBout bout{.seed = 5, .lethal = true, .heroicMark = false};
    bout.contestants.push_back(joue(priest, "marteau-de-guerre", {1, 3}));
    bout.contestants.push_back(test_support::dummy("Un", {2, 3}, 10, 0, 20, CombatSide::Allies));
    bout.contestants.push_back(test_support::dummy("Deux", {1, 4}, 10, 0, 20, CombatSide::Allies));
    bout.contestants.push_back(test_support::dummy("Mannequin", {11, 7}, 10, 0, 200));
    const core::BehaviorCatalog catalogue = profils();
    ASSERT_TRUE(session.mount(bout).refusals.empty());
    ASSERT_TRUE(session.start());

    ASSERT_TRUE(core::playTurn(session, catalogue));
    EXPECT_TRUE(journalHas(session.journal(), "benit 3 allie(s)")) << journal(session);
    const std::vector<core::CombatCondition> etats = session.conditionsOf(CombatantId{1});
    EXPECT_NE(std::ranges::find(etats, core::CombatCondition::Blessed), etats.end());
    EXPECT_NE(std::ranges::find(etats, core::CombatCondition::Concentrating), etats.end());
    EXPECT_EQ(lancersDe(session, "bless"), 1);

    for (int garde = 0; garde < 8 && session.combat().activeCombatant() != CombatantId{1};
         ++garde) {
        ASSERT_TRUE(session.endTurn());
    }
    ASSERT_TRUE(core::playTurn(session, catalogue));
    EXPECT_EQ(lancersDe(session, "bless"), 1) << "pas de seconde benediction";
}

/**
 * @brief Le Mage joué par l'IA lance ses sorts : au niveau 1, *projectile magique* sur un ennemi
 *        à distance plutôt que son bâton.
 * \castest{<b>L'IA du Mage lance un sort qui blesse.</b><br/>
 * \tcat Unitaire · IA tactique<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Mage N1 en (1, 3), un mannequin ennemi en (7, 3).<br/>2. Jouer le tour du Mage.<br/>
 * \tattendu Le journal dit « lance » ; le mannequin a perdu des points de vie.
 * }
 */
TEST(AiSpellsTest, LeMageLanceUnSortQuiBlesse) {
    const core::LoadedCharacterSheet mage = fiche("heros-mage.json", 1);
    core::ArenaSession session(test_support::room());
    core::ArenaBout bout{.seed = 7, .lethal = true, .heroicMark = false};
    bout.contestants.push_back(joue(mage, "baton", {1, 3}));
    bout.contestants.push_back(test_support::dummy("Mannequin", {7, 3}, 10, 0));
    const core::BehaviorCatalog catalogue = profils();
    ASSERT_TRUE(session.mount(bout).refusals.empty());
    ASSERT_TRUE(session.start());

    ASSERT_TRUE(core::playTurn(session, catalogue));
    EXPECT_TRUE(journalHas(session.journal(), " : lance ")) << journal(session);
    EXPECT_TRUE(journalHas(session.journal(), "blesse Mannequin")) << journal(session);
    EXPECT_LT(session.combat().find(CombatantId{2})->profile.currentHitPoints, 60);
}

/**
 * @brief La *boule de feu* ne tombe pas sur un allié : un ennemi seul au contact d'un allié
 *        reçoit un autre sort ; trois ennemis groupés loin des alliés la reçoivent.
 * \castest{<b>L'IA du Mage epargne ses allies, et groupe ses cibles.</b><br/>
 * \tcat Unitaire · IA tactique<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Mage N5 en (1, 3) ; un ennemi en (7, 3) au contact d'un allie en (7, 4) ; jouer le
 * tour.<br/>2. Mage N5 en (1, 3) ; trois ennemis en (8, 3), (9, 3), (8, 4) ; jouer le tour.<br/>
 * \tattendu Premier cas : la boule de feu garde ses deux lancers ; second cas : elle en perd un.
 * }
 */
TEST(AiSpellsTest, LaBouleDeFeuEpargneLesAllies) {
    const core::LoadedCharacterSheet mage = fiche("heros-mage.json", 5);
    const core::BehaviorCatalog catalogue = profils();
    {
        core::ArenaSession session(test_support::room());
        core::ArenaBout bout{.seed = 11, .lethal = true, .heroicMark = false};
        bout.contestants.push_back(joue(mage, "baton", {1, 3}));
        bout.contestants.push_back(test_support::dummy("Ennemi", {7, 3}, 10, 0));
        bout.contestants.push_back(
            test_support::dummy("Allie", {7, 4}, 10, 0, 20, CombatSide::Allies));
        ASSERT_TRUE(session.mount(bout).refusals.empty());
        ASSERT_TRUE(session.start());
        const int avant = lancersDe(session, "fireball");
        ASSERT_TRUE(core::playTurn(session, catalogue));
        EXPECT_EQ(lancersDe(session, "fireball"), avant) << journal(session);
    }
    core::ArenaSession session(test_support::room());
    core::ArenaBout bout{.seed = 11, .lethal = true, .heroicMark = false};
    bout.contestants.push_back(joue(mage, "baton", {1, 3}));
    bout.contestants.push_back(test_support::dummy("Un", {8, 3}, 10, 0));
    bout.contestants.push_back(test_support::dummy("Deux", {9, 3}, 10, 0));
    bout.contestants.push_back(test_support::dummy("Trois", {8, 4}, 10, 0));
    ASSERT_TRUE(session.mount(bout).refusals.empty());
    ASSERT_TRUE(session.start());
    const int avant = lancersDe(session, "fireball");
    ASSERT_TRUE(core::playTurn(session, catalogue));
    EXPECT_EQ(lancersDe(session, "fireball"), avant - 1) << journal(session);
}

/**
 * @brief Au niveau 5, le Brawler joué par l'IA prend l'attaque que lui laisse *Extra Attack*.
 * \castest{<b>L'IA du Brawler frappe deux fois au niveau 5.</b><br/>
 * \tcat Unitaire · IA tactique<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Brawler N5 en (1, 3), un mannequin de 200 PV au contact en (2, 3).<br/>2. Jouer le
 * tour du Brawler.<br/>
 * \tattendu Le journal dit « attaque supplementaire ».
 * }
 */
TEST(AiSpellsTest, LeBrawlerFrappeDeuxFois) {
    const core::LoadedCharacterSheet brawler = fiche("heros-brawler.json", 5);
    core::ArenaSession session(test_support::room());
    core::ArenaBout bout{.seed = 13, .lethal = true, .heroicMark = false};
    bout.contestants.push_back(joue(brawler, "hache-a-deux-mains", {1, 3}));
    bout.contestants.push_back(test_support::dummy("Mannequin", {2, 3}, 10, 0, 200));
    const core::BehaviorCatalog catalogue = profils();
    ASSERT_TRUE(session.mount(bout).refusals.empty());
    ASSERT_TRUE(session.start());

    ASSERT_TRUE(core::playTurn(session, catalogue));
    EXPECT_TRUE(journalHas(session.journal(), "attaque supplementaire")) << journal(session);
}

/**
 * @brief Au niveau 3, le Priest joué par l'IA invoque l'*arme spirituelle* par son action bonus,
 *        en plus de son action.
 * \castest{<b>L'IA du Priest frappe de l'arme spirituelle par l'action bonus.</b><br/>
 * \tcat Unitaire · IA tactique<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Priest N3 en (1, 3), un mannequin de 200 PV en (5, 3).<br/>2. Jouer le tour du
 * Priest.<br/>
 * \tattendu Le journal dit « action bonus » ; l'arme spirituelle a perdu un lancer.
 * }
 */
TEST(AiSpellsTest, LArmeSpirituelleFrappeParLActionBonus) {
    const core::LoadedCharacterSheet priest = fiche("heros-priest.json", 3);
    core::ArenaSession session(test_support::room());
    core::ArenaBout bout{.seed = 17, .lethal = true, .heroicMark = false};
    bout.contestants.push_back(joue(priest, "marteau-de-guerre", {1, 3}));
    bout.contestants.push_back(test_support::dummy("Mannequin", {5, 3}, 10, 0, 200));
    const core::BehaviorCatalog catalogue = profils();
    ASSERT_TRUE(session.mount(bout).refusals.empty());
    ASSERT_TRUE(session.start());
    const int avant = lancersDe(session, "spiritual-weapon");

    ASSERT_TRUE(core::playTurn(session, catalogue));
    EXPECT_TRUE(journalHas(session.journal(), "action bonus")) << journal(session);
    EXPECT_EQ(lancersDe(session, "spiritual-weapon"), avant - 1);
}
