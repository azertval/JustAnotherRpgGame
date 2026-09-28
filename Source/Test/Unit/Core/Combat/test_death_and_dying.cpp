// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_death_and_dying.cpp
 * @brief Tests des états, de l'agonie et de la mort (`LOT-137`, `EX-CBT-040`, `EX-CBT-041`) :
 *        les jets contre la mort de la machine à états, puis l'arène — soin, *épargner les
 *        mourants*, *revigorer*, concentration, critique au contact —, puis l'IA qui achève ou
 *        épargne.
 *
 * Manuel des Joueurs, « Tomber à 0 point de vie » (PDF p. 199) et annexe A (p. 293-294). La machine
 * à états reçoit ses d20 du test (`recordDeathSave`) : « trois échecs tuent, un 20 relève » se
 * vérifie sans parier sur une graine.
 */

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/ActionEconomy.h"
#include "Core/Combat/Arena.h"
#include "Core/Combat/BattleGrid.h"
#include "Core/Combat/CombatState.h"
#include "Core/Combat/EnemyAi.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/TileMap.h"
#include "Core/Math/DeterministicRandom.h"
#include "Test/Support/ClassArena.h"

namespace {

using core::CombatantId;
using core::CombatantStatus;
using core::CombatCondition;
using core::CombatHook;
using core::CombatSide;
using core::DeathSaveOutcome;
using test_support::journalCount;
using test_support::journalHas;

constexpr int PREMIER = 100;
constexpr int SECOND = 50;
constexpr int DERNIER = -100;

[[nodiscard]] core::CombatantProfile profil(const std::string& nom, CombatSide camp, int pv,
                                            int initiative) {
    return {.name = nom,
            .side = camp,
            .maximumHitPoints = pv,
            .currentHitPoints = pv,
            .dexterity = 10,
            .initiativeModifier = initiative,
            .movement = 6};
}

/// Deux alliés de 10 PV, Aldric puis Brune — et Cédric, sur demande —, et deux gobelins de 10 PV ;
/// le combat a commencé et c'est le tour d'Aldric.
struct Escarmouche {
    core::CombatState combat{core::BattleGrid(core::TileMap(12, 8))};
    core::DeterministicRandom des{1};
    CombatantId aldric{};
    CombatantId brune{};
    CombatantId gobelin{};

    explicit Escarmouche(core::AtZeroHitPoints monstres = core::AtZeroHitPoints::Dies,
                         bool cedric = false) {
        aldric = *combat
                      .enlist(profil("Aldric", CombatSide::Allies, 10, PREMIER),
                              core::GridPosition{1, 1})
                      .combatant;
        brune =
            *combat
                 .enlist(profil("Brune", CombatSide::Allies, 10, SECOND), core::GridPosition{1, 3})
                 .combatant;
        if (cedric) {
            static_cast<void>(combat.enlist(profil("Cedric", CombatSide::Allies, 10, 0),
                                            core::GridPosition{1, 5}));
        }
        core::CombatantProfile monstre = profil("Gobelin", CombatSide::Enemies, 10, DERNIER);
        monstre.atZero = monstres;
        gobelin = *combat.enlist(monstre, core::GridPosition{8, 1}).combatant;
        monstre.name = "Second gobelin";
        static_cast<void>(combat.enlist(monstre, core::GridPosition{8, 3}));
        EXPECT_TRUE(combat.start(des));
        EXPECT_EQ(combat.activeCombatant(), aldric);
    }
};

// --- L'arène ----------------------------------------------------------------------------------

const std::string BRAN = "Bran";

/// La Priest de la page 203 au niveau @p niveau.
[[nodiscard]] core::LoadedCharacterSheet priest(int niveau) {
    core::LoadedCharacterSheet charge = test_support::loadPremade("heros-priest.json");
    EXPECT_TRUE(charge.errors.empty());
    if (niveau > 1) {
        EXPECT_TRUE(test_support::levelUpTo(charge.sheet, niveau).empty());
    }
    return charge;
}

/// La Priest (#1) en (1, 3), 200 PV ; Bran (#2), allié de 10 PV, à son contact en (2, 3) ; un
/// gobelin (#3) de 60 PV au contact de Bran, en (3, 3), qui touche à +30. Combat où l'on meurt.
[[nodiscard]] core::ArenaBout veillee(const core::LoadedCharacterSheet& charge) {
    core::ArenaBout bout{.seed = 3, .lethal = true, .heroicMark = false};
    core::ArenaContestant heros =
        test_support::hero(charge.sheet, charge.inventory,
                           {test_support::weaponAttack(charge.sheet, "marteau-de-guerre")}, {1, 3});
    heros.profile.maximumHitPoints = 200;
    heros.profile.currentHitPoints = 200;
    bout.contestants.push_back(std::move(heros));
    bout.contestants.push_back(
        test_support::dummy(BRAN.c_str(), {2, 3}, 10, 0, 10, CombatSide::Allies));
    bout.contestants.push_back(test_support::dummy("Gobelin", {3, 3}, 10, 30, 60));
    return bout;
}

[[nodiscard]] std::size_t sortDe(const core::ArenaSession& session, const char* id) {
    const std::vector<core::ArenaSpell>* grimoire = session.spells(CombatantId{1});
    EXPECT_NE(grimoire, nullptr);
    for (std::size_t i = 0; grimoire != nullptr && i < grimoire->size(); ++i) {
        if ((*grimoire)[i].id == id) {
            return i;
        }
    }
    ADD_FAILURE() << id << " absent du grimoire";
    return 0;
}

[[nodiscard]] bool porte(const core::ArenaSession& session, CombatantId id, CombatCondition etat) {
    const std::vector<CombatCondition> etats = session.conditionsOf(id);
    return std::ranges::find(etats, etat) != etats.end();
}

}  // namespace

// --- La machine à états -------------------------------------------------------------------------

/**
 * @brief Trois echecs tuent ; un mort ne se soigne pas (EX-CBT-040).
 * \castest{<b>Un allie tombe a 0 PV : il agonise ; deux echecs et un succes ne le tuent pas, le
 * troisieme echec le tue, une fois, et plus rien ne le soigne.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Aldric tombe a 0 PV.<br/>2. Jets : 9, 15, 5, puis 3.<br/>3. Le soigner, lui faire
 * jeter encore.<br/>
 * \tattendu A terre et agonisant ; echec, succes, echec, puis mort ; la mort annoncee une fois ;
 * soin sans effet ; jet ignore.
 * }
 */
TEST(DeathAndDyingTest, TroisEchecsTuent) {
    Escarmouche e;
    int morts = 0;
    e.combat.subscribe(CombatHook::CombatantDied,
                       [&](core::CombatState&, const core::CombatEvent&) { ++morts; });
    e.combat.applyDamage(e.aldric, 10);
    EXPECT_EQ(e.combat.find(e.aldric)->status, CombatantStatus::Down);
    EXPECT_TRUE(e.combat.isDying(e.aldric));
    EXPECT_TRUE(e.combat.find(e.aldric)->prone);
    EXPECT_EQ(e.combat.activeCombatant(), e.brune) << "son tour se termine a la chute";

    EXPECT_EQ(e.combat.recordDeathSave(e.aldric, 9, 9), DeathSaveOutcome::Failure);
    EXPECT_EQ(e.combat.recordDeathSave(e.aldric, 15, 15), DeathSaveOutcome::Success);
    EXPECT_EQ(e.combat.recordDeathSave(e.aldric, 5, 5), DeathSaveOutcome::Failure);
    EXPECT_EQ(e.combat.find(e.aldric)->deathSaves,
              (core::DeathSaves{.successes = 1, .failures = 2, .stable = false}));
    EXPECT_EQ(morts, 0);
    EXPECT_EQ(e.combat.recordDeathSave(e.aldric, 3, 3), DeathSaveOutcome::Died);
    EXPECT_EQ(e.combat.find(e.aldric)->status, CombatantStatus::Dead);
    EXPECT_EQ(morts, 1);

    e.combat.heal(e.aldric, 5);
    EXPECT_EQ(e.combat.find(e.aldric)->status, CombatantStatus::Dead);
    EXPECT_EQ(e.combat.find(e.aldric)->profile.currentHitPoints, 0);
    EXPECT_EQ(e.combat.recordDeathSave(e.aldric, 20, 20), DeathSaveOutcome::Ignored);
    EXPECT_FALSE(e.combat.declareAttack(e.gobelin, e.aldric)) << "un mort ne s'attaque plus";
}

/**
 * @brief Un 20 naturel releve ; un 1 naturel compte deux echecs ; trois succes stabilisent ; un
 *        soin remet le compteur a zero (EX-CBT-040, EX-CBT-041).
 * \castest{<b>Les d20 extremes du Manuel : 20 rend 1 PV, 1 compte deux echecs, meme quand la
 * benediction porterait le total a 10 ; trois succes stabilisent, des degats refont agoniser, et
 * le soin repart d'un compteur vide.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Aldric a terre jette 1 (total 12).<br/>2. Trois succes, dont un 8 porte a 11.<br/>3.
 * Un point de degats.<br/>4. Un soin de 3.<br/>5. Brune a terre jette 20.<br/>
 * \tattendu Deux echecs ; stabilise, compteur vide ; un echec ; debout a 3 PV, compteur vide, a
 * terre jusqu'a son tour ; Brune debout a 1 PV.
 * }
 */
TEST(DeathAndDyingTest, UnVingtReleveUnUnCompteDouble) {
    Escarmouche e;
    e.combat.applyDamage(e.aldric, 10);
    EXPECT_EQ(e.combat.recordDeathSave(e.aldric, 1, 12), DeathSaveOutcome::Failure);
    EXPECT_EQ(e.combat.find(e.aldric)->deathSaves.failures, 2);
    EXPECT_EQ(e.combat.recordDeathSave(e.aldric, 12, 12), DeathSaveOutcome::Success);
    EXPECT_EQ(e.combat.recordDeathSave(e.aldric, 8, 11), DeathSaveOutcome::Success)
        << "la benediction aide le jet";
    EXPECT_EQ(e.combat.recordDeathSave(e.aldric, 10, 10), DeathSaveOutcome::Stabilized);
    EXPECT_EQ(e.combat.find(e.aldric)->deathSaves,
              (core::DeathSaves{.successes = 0, .failures = 0, .stable = true}));
    EXPECT_FALSE(e.combat.isDying(e.aldric));

    e.combat.applyDamage(e.aldric, 1);
    EXPECT_TRUE(e.combat.isDying(e.aldric)) << "des degats refont agoniser";
    EXPECT_EQ(e.combat.find(e.aldric)->deathSaves.failures, 1);

    e.combat.heal(e.aldric, 3);
    const core::Combatant* aldric = e.combat.find(e.aldric);
    EXPECT_EQ(aldric->status, CombatantStatus::Standing);
    EXPECT_EQ(aldric->profile.currentHitPoints, 3);
    EXPECT_EQ(aldric->deathSaves, core::DeathSaves{});
    EXPECT_TRUE(aldric->prone) << "a terre jusqu'a son tour";

    e.combat.applyDamage(e.brune, 10);
    EXPECT_EQ(e.combat.recordDeathSave(e.brune, 20, 20), DeathSaveOutcome::Revived);
    EXPECT_EQ(e.combat.find(e.brune)->status, CombatantStatus::Standing);
    EXPECT_EQ(e.combat.find(e.brune)->profile.currentHitPoints, 1);
}

/**
 * @brief Blesse a terre, un echec, deux sur un critique ; la mort instantanee ; le monstre meurt a
 *        0 PV (EX-CBT-040).
 * \castest{<b>Degats a 0 point de vie et mort instantanee, Manuel p. 199 : un coup a terre coute un
 * echec, un critique deux ; des degats restants au moins egaux au maximum tuent sur le coup ; un
 * monstre meurt des 0 PV.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Aldric a terre : 1 degat, puis 1 degat critique.<br/>2. Brune (10 PV) prend 20
 * degats.<br/>3. Le gobelin tombe a 0.<br/>
 * \tattendu 1 puis 3 echecs : mort ; Brune morte sur le coup, chute et mort annoncees ; le gobelin
 * mort.
 * }
 */
TEST(DeathAndDyingTest, LesDegatsATerreEtLaMortInstantanee) {
    Escarmouche e;
    std::vector<CombatHook> annonces;
    for (const CombatHook crochet : {CombatHook::CombatantDowned, CombatHook::CombatantDied}) {
        e.combat.subscribe(crochet, [&](core::CombatState&, const core::CombatEvent& evenement) {
            annonces.push_back(evenement.hook);
        });
    }
    e.combat.applyDamage(e.aldric, 10);
    e.combat.applyDamage(e.aldric, 1);
    EXPECT_EQ(e.combat.find(e.aldric)->deathSaves.failures, 1);
    const std::vector<core::HitPointChange> critique{
        {.target = e.aldric, .amount = 1, .critical = true}};
    e.combat.applyDamage(critique);
    EXPECT_EQ(e.combat.find(e.aldric)->status, CombatantStatus::Dead);

    annonces.clear();
    e.combat.applyDamage(e.brune, 20);
    EXPECT_EQ(e.combat.find(e.brune)->status, CombatantStatus::Dead);
    EXPECT_EQ(annonces,
              (std::vector<CombatHook>{CombatHook::CombatantDowned, CombatHook::CombatantDied}));

    EXPECT_EQ(e.combat.outcome(), core::CombatOutcome::Defeat) << "tous a terre ou morts";
}

/**
 * @brief Un monstre meurt a 0 PV ; un combat sans mort ne tue personne.
 * \castest{<b>Les monstres et la mort (Manuel p. 199) ; et la Marque Heroique des Arenes : sans
 * mort, on tombe sans agoniser, meme sous des degats massifs.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Le gobelin tombe a 0.<br/>2. Un combat sans mort : Aldric prend 30 degats, les tours
 * passent.<br/>
 * \tattendu Gobelin mort ; Aldric a terre, n'agonise pas, aucun jet annonce.
 * }
 */
TEST(DeathAndDyingTest, UnMonstreMeurtEtLaMarqueProtege) {
    Escarmouche letal;
    letal.combat.applyDamage(letal.gobelin, 10);
    EXPECT_EQ(letal.combat.find(letal.gobelin)->status, CombatantStatus::Dead);

    Escarmouche sansMort(core::AtZeroHitPoints::DeathSaves);
    sansMort.combat.setLethal(false);
    int jets = 0;
    sansMort.combat.subscribe(CombatHook::DeathSaveDue,
                              [&](core::CombatState&, const core::CombatEvent&) { ++jets; });
    sansMort.combat.applyDamage(sansMort.aldric, 30);
    EXPECT_EQ(sansMort.combat.find(sansMort.aldric)->status, CombatantStatus::Down);
    EXPECT_FALSE(sansMort.combat.isDying(sansMort.aldric));
    for (int tour = 0; tour < 6; ++tour) {
        ASSERT_TRUE(sansMort.combat.endTurn());
    }
    EXPECT_EQ(jets, 0);
    sansMort.combat.applyDamage(sansMort.gobelin, 10);
    EXPECT_EQ(sansMort.combat.find(sansMort.gobelin)->status, CombatantStatus::Down);
}

/**
 * @brief Le jet contre la mort s'annonce a la place du mourant ; releve par un 20, il joue ce
 *        tour-ci, et se relever lui coute la moitie de son deplacement (EX-CBT-040).
 * \castest{<b>« A chaque fois que vous commencez un tour a 0 point de vie » : la machine annonce
 * DeathSaveDue a la place d'Aldric, pas a celle d'un stabilise ; un 20 le releve et son tour
 * s'ouvre, debout, trois cases de moins.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Aldric tombe ; Brune stabilisee a terre ; Cedric debout.<br/>2. Les tours passent
 * jusqu'au round 2 ; l'abonne jette 20 pour Aldric.<br/>
 * \tattendu Une annonce, pour Aldric seul ; c'est son tour ; il n'est plus a terre ; 3 cases de
 * deplacement sur 6.
 * }
 */
TEST(DeathAndDyingTest, LeJetSeFaitASaPlaceEtUnVingtRejoue) {
    Escarmouche e(core::AtZeroHitPoints::Dies, true);
    std::vector<CombatantId> jets;
    e.combat.subscribe(CombatHook::DeathSaveDue,
                       [&](core::CombatState& etat, const core::CombatEvent& evenement) {
                           jets.push_back(*evenement.combatant);
                           static_cast<void>(etat.recordDeathSave(*evenement.combatant, 20, 20));
                       });
    e.combat.applyDamage(e.aldric, 10);
    e.combat.applyDamage(e.brune, 10);
    ASSERT_TRUE(e.combat.stabilize(e.brune));
    // Cedric et les deux gobelins jouent, puis le round 2 revient a Aldric.
    for (int tour = 0; tour < 3; ++tour) {
        ASSERT_TRUE(e.combat.endTurn());
    }
    EXPECT_EQ(jets, (std::vector<CombatantId>{e.aldric}));
    EXPECT_EQ(e.combat.round(), 2);
    ASSERT_EQ(e.combat.activeCombatant(), e.aldric);
    const core::Combatant* aldric = e.combat.find(e.aldric);
    EXPECT_FALSE(aldric->prone);
    EXPECT_EQ(aldric->economy.remaining(core::MOVEMENT_RESOURCE), 3);
}

/**
 * @brief Revive ramene un mort, a terre jusqu'a son tour ; rien d'autre.
 * \castest{<b>Seul un mort revient : revive refuse un vivant, et le revenant se releve avec ses
 * points de vie, compteur vide.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. revive sur Brune debout.<br/>2. Aldric meurt, revive a 1 PV.<br/>
 * \tattendu Refus ; Aldric debout a 1 PV, a terre, compteur vide.
 * }
 */
TEST(DeathAndDyingTest, ReviveNeRameneQueLesMorts) {
    Escarmouche e;
    EXPECT_FALSE(e.combat.revive(e.brune, 1));
    e.combat.applyDamage(e.aldric, 20);
    ASSERT_EQ(e.combat.find(e.aldric)->status, CombatantStatus::Dead);
    EXPECT_TRUE(e.combat.find(e.aldric)->diedAtRound.has_value());
    ASSERT_TRUE(e.combat.revive(e.aldric, 1));
    const core::Combatant* aldric = e.combat.find(e.aldric);
    EXPECT_EQ(aldric->status, CombatantStatus::Standing);
    EXPECT_EQ(aldric->profile.currentHitPoints, 1);
    EXPECT_TRUE(aldric->prone);
    EXPECT_EQ(aldric->deathSaves, core::DeathSaves{});
    EXPECT_FALSE(aldric->diedAtRound.has_value());
}

// --- L'arène ----------------------------------------------------------------------------------

/**
 * @brief Un allie a terre se releve par soin des blessures et rejoue a son tour (EX-CBT-041).
 * \castest{<b>Critere du LOT-137 : Bran tombe, inconscient et a terre ; la Priest le soigne au
 * contact ; il se releve, et son tour vient a sa place, ou il se remet debout.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Bran tombe a 0 PV pendant le tour de la Priest.<br/>2. Elle lance soin des
 * blessures sur lui.<br/>3. Elle termine son tour.<br/>
 * \tattendu Inconscient et a terre ; soin joue, debout, compteur vide ; c'est le tour de Bran,
 * qui n'est plus a terre et a paye la moitie de son deplacement.
 * }
 */
TEST(DeathAndDyingTest, UnAllieATerreSeReleveParSoinEtRejoue) {
    core::ArenaSession session(test_support::room());
    const core::LoadedCharacterSheet charge = priest(1);
    session.mount(veillee(charge));
    ASSERT_TRUE(session.start());
    ASSERT_EQ(session.combat().activeCombatant(), CombatantId{1});

    session.combat().applyDamage(CombatantId{2}, 10);
    EXPECT_TRUE(porte(session, CombatantId{2}, CombatCondition::Unconscious));
    EXPECT_TRUE(porte(session, CombatantId{2}, CombatCondition::Prone));
    EXPECT_TRUE(journalHas(session.journal(), "a terre " + BRAN));

    ASSERT_EQ(session.castSpell(CombatantId{2}, sortDe(session, "cure-wounds")).result,
              core::ArenaActionResult::Done);
    const core::Combatant* bran = session.combat().find(CombatantId{2});
    EXPECT_EQ(bran->status, CombatantStatus::Standing);
    EXPECT_GT(bran->profile.currentHitPoints, 0);
    EXPECT_EQ(session.conditionsOf(CombatantId{2}),
              (std::vector<CombatCondition>{CombatCondition::Prone}));

    ASSERT_TRUE(session.endTurn());
    EXPECT_EQ(session.combat().activeCombatant(), CombatantId{2}) << "il rejoue a son tour";
    bran = session.combat().find(CombatantId{2});
    EXPECT_FALSE(bran->prone);
    EXPECT_EQ(bran->economy.remaining(core::MOVEMENT_RESOURCE), 3);
}

/**
 * @brief Le jet contre la mort se jette dans l'arene, a la place du mourant, et s'ecrit
 *        (EX-CBT-040).
 * \castest{<b>Dans une session, qui tient les des jette le d20 de Bran a sa place et l'ecrit :
 * reussite, echec, ou 20 qui le releve.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Bran tombe.<br/>2. La Priest termine son tour.<br/>
 * \tattendu Une ligne « jet contre la mort Bran » ; son compteur ou ses PV la suivent.
 * }
 */
TEST(DeathAndDyingTest, LeJetContreLaMortSeJetteDansLArene) {
    core::ArenaSession session(test_support::room());
    session.mount(veillee(priest(1)));
    ASSERT_TRUE(session.start());
    session.combat().applyDamage(CombatantId{2}, 10);
    ASSERT_TRUE(session.endTurn());
    EXPECT_EQ(journalCount(session.journal(), "jet contre la mort " + BRAN), 1U);
    const core::Combatant* bran = session.combat().find(CombatantId{2});
    if (bran->status == CombatantStatus::Standing) {
        EXPECT_EQ(bran->profile.currentHitPoints, 1);
        EXPECT_TRUE(journalHas(session.journal(), "20 naturel, reprend 1 PV"));
    } else {
        ASSERT_EQ(bran->status, CombatantStatus::Down);
        EXPECT_GE(bran->deathSaves.successes + bran->deathSaves.failures, 1);
    }
}

/**
 * @brief Epargner les mourants stabilise ; plus de jet contre la mort.
 * \castest{<b>La Priest de niveau 5 lance epargner les mourants sur Bran a terre : il est
 * stabilise, et son tour passe sans jet.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Bran tombe.<br/>2. Epargner les mourants sur lui ; puis sur la Priest debout.<br/>3.
 * Fin du tour.<br/>
 * \tattendu Stabilise, inconscient ; refus sur une cible debout ; aucun jet contre la mort ; c'est
 * le tour du gobelin.
 * }
 */
TEST(DeathAndDyingTest, EpargnerLesMourantsStabilise) {
    core::ArenaSession session(test_support::room());
    session.mount(veillee(priest(5)));
    ASSERT_TRUE(session.start());
    session.combat().applyDamage(CombatantId{2}, 10);
    const std::size_t epargner = sortDe(session, "spare-the-dying");
    EXPECT_EQ(session.castSpell(CombatantId{1}, epargner).result,
              core::ArenaActionResult::InvalidTarget);
    ASSERT_EQ(session.castSpell(CombatantId{2}, epargner).result, core::ArenaActionResult::Done);
    EXPECT_TRUE(journalHas(session.journal(), "stabilisation"));
    EXPECT_TRUE(porte(session, CombatantId{2}, CombatCondition::Stable));
    EXPECT_TRUE(porte(session, CombatantId{2}, CombatCondition::Unconscious));
    EXPECT_FALSE(session.combat().isDying(CombatantId{2}));
    ASSERT_TRUE(session.endTurn());
    EXPECT_FALSE(journalHas(session.journal(), "jet contre la mort " + BRAN));
    EXPECT_EQ(session.combat().activeCombatant(), CombatantId{3});
}

/**
 * @brief Revigorer ramene un mort de moins d'une minute, pas au-dela ; le soin ne ramene pas.
 * \castest{<b>Bran meurt sous des degats massifs ; soin des blessures ne le ramene pas, revigorer
 * si : 1 PV. Mort de nouveau, onze rounds plus tard, revigorer refuse.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Bran tombe, puis prend 10 degats a terre.<br/>2. Soin, puis revigorer.<br/>3. Il
 * remeurt ; onze rounds passent ; revigorer.<br/>
 * \tattendu Mort, le journal dit l'agonie et la mort ; soin refuse ; debout a 1 PV ; refus apres
 * dix rounds.
 * }
 */
TEST(DeathAndDyingTest, RevigorerRameneUnMortDeMoinsDUneMinute) {
    core::ArenaSession session(test_support::room());
    session.mount(veillee(priest(5)));
    ASSERT_TRUE(session.start());
    session.combat().applyDamage(CombatantId{2}, 10);
    session.combat().applyDamage(CombatantId{2}, 10);
    ASSERT_EQ(session.combat().find(CombatantId{2})->status, CombatantStatus::Dead);
    EXPECT_TRUE(journalHas(session.journal(), "agonie " + BRAN + " : blesse a terre"));
    EXPECT_TRUE(journalHas(session.journal(), "mort " + BRAN));
    EXPECT_TRUE(porte(session, CombatantId{2}, CombatCondition::Dead));

    EXPECT_EQ(session.castSpell(CombatantId{2}, sortDe(session, "cure-wounds")).result,
              core::ArenaActionResult::InvalidTarget);
    const std::size_t revigorer = sortDe(session, "revivify");
    ASSERT_EQ(session.castSpell(CombatantId{2}, revigorer).result, core::ArenaActionResult::Done);
    EXPECT_TRUE(journalHas(session.journal(), "retour a la vie"));
    EXPECT_EQ(session.combat().find(CombatantId{2})->status, CombatantStatus::Standing);
    EXPECT_EQ(session.combat().find(CombatantId{2})->profile.currentHitPoints, 1);

    session.combat().applyDamage(CombatantId{2}, 11);
    ASSERT_EQ(session.combat().find(CombatantId{2})->status, CombatantStatus::Dead);
    const int mort = session.combat().round();
    for (int garde = 0; garde < 60 && session.combat().round() <= mort + 10; ++garde) {
        ASSERT_TRUE(session.endTurn());
    }
    while (session.combat().activeCombatant() != CombatantId{1}) {
        ASSERT_TRUE(session.endTurn());
    }
    EXPECT_EQ(session.castSpell(CombatantId{2}, revigorer).result,
              core::ArenaActionResult::InvalidTarget);
}

/**
 * @brief Frapper un inconscient au contact : avantage, critique, deux echecs (EX-CBT-040).
 * \castest{<b>Manuel, annexe A : Bran, stabilise a terre, est attaque par le gobelin a son contact
 * avec avantage ; touche, le coup est critique et compte deux echecs.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Bran tombe et se stabilise.<br/>2. Tour du gobelin : il attaque Bran.<br/>
 * \tattendu Attaque jouee, avantage « cible inconsciente » et « cible a terre au contact » ; si
 * elle touche, critique « cible inconsciente au contact », et Bran mort ou a deux echecs.
 * }
 */
TEST(DeathAndDyingTest, FrapperUnInconscientAuContactEstCritique) {
    core::ArenaSession session(test_support::room());
    session.mount(veillee(priest(1)));
    ASSERT_TRUE(session.start());
    session.combat().applyDamage(CombatantId{2}, 10);
    ASSERT_TRUE(session.combat().stabilize(CombatantId{2}));
    ASSERT_TRUE(session.endTurn());
    ASSERT_EQ(session.combat().activeCombatant(), CombatantId{3});
    const core::ArenaAttack coup = session.attack(CombatantId{2});
    ASSERT_EQ(coup.result, core::ArenaActionResult::Done);
    ASSERT_TRUE(coup.outcome.has_value());
    const std::vector<std::string>& avantages = coup.outcome->roll.advantages;
    EXPECT_NE(std::ranges::find(avantages, "cible inconsciente"), avantages.end());
    EXPECT_NE(std::ranges::find(avantages, "cible a terre au contact"), avantages.end());
    if (coup.outcome->roll.hit) {
        EXPECT_TRUE(coup.outcome->roll.critical);
        const core::Combatant* bran = session.combat().find(CombatantId{2});
        EXPECT_TRUE(bran->status == CombatantStatus::Dead || bran->deathSaves.failures == 2);
        EXPECT_TRUE(journalHas(session.journal(), "critique (cible inconsciente au contact)"));
    }
}

/**
 * @brief Des degats rompent la concentration faute de sauvegarde ; les etats se lisent.
 * \castest{<b>La Priest benie et concentree prend 100 degats : DD 50, la sauvegarde de Constitution
 * echoue, la benediction prend fin.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Benediction sur elle-meme.<br/>2. 100 degats.<br/>
 * \tattendu Benie et concentree ; ligne « concentration … rompue » ; plus de benediction, plus
 * concentree.
 * }
 */
TEST(DeathAndDyingTest, DesDegatsRompentLaConcentration) {
    core::ArenaSession session(test_support::room());
    session.mount(veillee(priest(1)));
    ASSERT_TRUE(session.start());
    ASSERT_EQ(session.castSpell(CombatantId{1}, sortDe(session, "bless")).result,
              core::ArenaActionResult::Done);
    EXPECT_TRUE(porte(session, CombatantId{1}, CombatCondition::Blessed));
    EXPECT_TRUE(porte(session, CombatantId{1}, CombatCondition::Concentrating));
    EXPECT_EQ(core::combatConditionLabel(CombatCondition::Concentrating), "concentre");

    session.combat().applyDamage(CombatantId{1}, 100);
    EXPECT_TRUE(journalHas(session.journal(), "< 50 : echec ; rompue"));
    EXPECT_TRUE(journalHas(session.journal(), "(concentration rompue)"));
    EXPECT_FALSE(session.hasEffect(CombatantId{1}, core::SpellEffectKind::Bless));
    EXPECT_FALSE(porte(session, CombatantId{1}, CombatCondition::Concentrating));
}

// --- L'IA ---------------------------------------------------------------------------------------

/**
 * @brief L'IA acheve ou epargne selon son profil (LOT-23, EX-CBT-050) -- et ne s'acharne pas sur
 *        un personnage a terre quand un autre la menace (LOT-139).
 * \castest{<b>Critere du LOT-137 et du LOT-139 : un gobelin au contact de Bran, a terre. Aldric,
 * debout, est a trois cases : un profil qui n'acheve pas va frapper Aldric ; un profil qui
 * acheve frappe Bran. Aldric revenu au contact, meme le profil qui acheve frappe Aldric. Les
 * profils livres disent qui acheve.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Aldric a trois cases : planifier le tour du gobelin avec finishDowned 0, puis
 * 1000.<br/>2. Aldric au contact : planifier avec finishDowned 1000.<br/>3. Lire
 * behaviors.json.<br/>
 * \tattendu Aldric, puis Bran ; puis Aldric ; agressif 75, meute 100, prudent, soutien et
 * archer 0.
 * }
 */
TEST(DeathAndDyingTest, LIaAcheveOuEpargneSelonSonProfil) {
    const auto monter = [](core::GridPosition aldric) {
        core::ArenaBout bout{.seed = 5, .lethal = true, .heroicMark = false};
        core::ArenaContestant gobelin = test_support::dummy("Gobelin", {5, 3}, 12, 4, 20);
        gobelin.profile.initiativeModifier = PREMIER;
        bout.contestants.push_back(gobelin);
        core::ArenaContestant bran =
            test_support::dummy(BRAN.c_str(), {4, 3}, 12, 4, 10, CombatSide::Allies);
        bran.profile.currentHitPoints = 0;
        bout.contestants.push_back(bran);
        bout.contestants.push_back(
            test_support::dummy("Aldric", aldric, 12, 4, 10, CombatSide::Allies));
        auto session = std::make_unique<core::ArenaSession>(test_support::room());
        session->mount(bout);
        EXPECT_TRUE(session->start());
        EXPECT_EQ(session->combat().activeCombatant(), CombatantId{1});
        EXPECT_EQ(session->combat().find(CombatantId{2})->status, CombatantStatus::Down);
        return session;
    };
    core::BehaviorProfile epargne{.id = "epargne", .name = "Epargne"};
    core::BehaviorProfile acheve{.id = "acheve", .name = "Acheve"};
    acheve.finishDowned = 1000;

    // Aldric a trois cases : personne ne menace le gobelin au contact.
    const std::unique_ptr<core::ArenaSession> loin = monter({8, 3});
    const core::TurnPlan clement = core::planTurn(*loin, CombatantId{1}, epargne);
    EXPECT_EQ(clement.action, core::TurnAction::Attack);
    EXPECT_EQ(clement.target, CombatantId{3});
    const core::TurnPlan cruel = core::planTurn(*loin, CombatantId{1}, acheve);
    EXPECT_EQ(cruel.action, core::TurnAction::Attack);
    EXPECT_EQ(cruel.target, CombatantId{2});

    // Aldric au contact : l'IA repartit ses coups (LOT-139), le blesse attend.
    const std::unique_ptr<core::ArenaSession> pres = monter({6, 3});
    const core::TurnPlan menace = core::planTurn(*pres, CombatantId{1}, acheve);
    EXPECT_EQ(menace.action, core::TurnAction::Attack);
    EXPECT_EQ(menace.target, CombatantId{3});

    const core::BehaviorCatalog profils =
        core::loadBehaviors(std::filesystem::path(JADG_RPG_RULES_DIR) / "behaviors.json");
    ASSERT_TRUE(profils.errors.empty());
    EXPECT_EQ(profils.find("aggressive")->finishDowned, 75);
    EXPECT_EQ(profils.find("pack")->finishDowned, 100);
    for (const char* id : {"cautious", "support", "archer"}) {
        EXPECT_EQ(profils.find(id)->finishDowned, 0) << id;
    }
}
