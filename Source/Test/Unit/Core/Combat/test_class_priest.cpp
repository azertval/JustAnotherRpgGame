// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_class_priest.cpp
 * @brief Le Priest du *Player's Guide to Tanares* (p. 200-203) se joue du niveau 1 au niveau 5
 *        (`LOT-134`) : *flamme sacrée*, *bénédiction*, *soin des blessures*, *arme spirituelle*,
 *        et les mécanismes qu'ils demandent — le soin, un effet sur plusieurs alliés, l'action
 *        bonus, l'arme qui frappe de nouveau sans lancer.
 *
 * Chaque sort joué a son test qui le lance et vérifie son effet et sa ligne de journal, sur la
 * fiche pré-tirée de la page 203 montée au niveau voulu.
 */

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/ActionEconomy.h"
#include "Core/Combat/Arena.h"
#include "Core/Rpg/Spell.h"
#include "Test/Support/ClassArena.h"

namespace {

using core::CombatantId;
using core::CombatSide;
using test_support::journalHas;
using test_support::journalLine;

const std::string PRIEST = "Helga Pierre-Sûre";

/// La fiche de la page 203 au niveau @p niveau, sans avertissement de la table.
[[nodiscard]] core::LoadedCharacterSheet priest(int niveau = 1) {
    core::LoadedCharacterSheet charge = test_support::loadPremade("heros-priest.json");
    EXPECT_TRUE(charge.errors.empty());
    if (niveau > 1) {
        EXPECT_TRUE(test_support::levelUpTo(charge.sheet, niveau).empty());
    }
    return charge;
}

/// La Priest en (1, 3), 200 PV, puis les autres.
[[nodiscard]] core::ArenaBout combatDe(const core::LoadedCharacterSheet& charge,
                                       std::vector<core::ArenaContestant> autres) {
    core::ArenaBout bout{.seed = 1, .lethal = true, .heroicMark = false};
    core::ArenaContestant heros =
        test_support::hero(charge.sheet, charge.inventory,
                           {test_support::weaponAttack(charge.sheet, "marteau-de-guerre")}, {1, 3});
    heros.profile.maximumHitPoints = 200;
    heros.profile.currentHitPoints = 200;
    bout.contestants.push_back(std::move(heros));
    for (core::ArenaContestant& autre : autres) {
        bout.contestants.push_back(std::move(autre));
    }
    return bout;
}

[[nodiscard]] std::size_t sortDe(const core::ArenaSession& session, const char* id,
                                 CombatantId lanceur = CombatantId{1}) {
    const std::vector<core::ArenaSpell>* grimoire = session.spells(lanceur);
    EXPECT_NE(grimoire, nullptr);
    for (std::size_t i = 0; grimoire != nullptr && i < grimoire->size(); ++i) {
        if ((*grimoire)[i].id == id) {
            return i;
        }
    }
    ADD_FAILURE() << id << " absent du grimoire";
    return 0;
}

void jusquAuTourDe(core::ArenaSession& session, CombatantId id) {
    for (int garde = 0; garde < 12 && session.combat().activeCombatant() != id; ++garde) {
        ASSERT_TRUE(session.endTurn());
    }
    ASSERT_EQ(session.combat().activeCombatant(), id);
}

}  // namespace

/**
 * @brief La fiche pre-tiree du Priest se joue avec ses sorts de niveau 1.
 * \castest{<b>La fiche de la page 203 connait lumiere, flamme sacree, benediction et soin des
 * blessures ; les trois qui se jouent en combat entrent au grimoire.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger heros-priest.json.<br/>2. Lire capacites, CA, grimoire de combat.<br/>
 * \tattendu Aucun avertissement ; CA 17 ; grimoire : flamme sacree (sauvegarde, DD 13), benediction
 * (effet, trois cibles), soin des blessures (soin 1d8+3) ; lumiere dite, sans effet en combat.
 * }
 */
TEST(ClassPriestTest, LaFichePreTireeSeJoueAvecSesSortsDeNiveau1) {
    const core::LoadedCharacterSheet charge = priest();
    EXPECT_TRUE(charge.warnings.empty()) << (charge.warnings.empty() ? "" : charge.warnings[0]);
    EXPECT_EQ(test_support::capacityIds(charge.sheet),
              (std::vector<std::string>{"simplified-spellcasting", "specific-cantrips"}));
    EXPECT_EQ(test_support::armorClassOf(charge.sheet, charge.inventory), 17);
    const test_support::RpgCatalogs& catalogues = test_support::rpgCatalogs();
    std::vector<std::string> ignores;
    const std::vector<core::ArenaSpell> grimoire =
        core::arenaSpellsFor(charge.sheet, *catalogues.options.findClass("priest"),
                             catalogues.options.spells, 2, ignores);
    ASSERT_EQ(grimoire.size(), 3U);
    EXPECT_EQ(grimoire[0].id, "sacred-flame");
    EXPECT_EQ(grimoire[0].mechanism, core::SpellMechanism::SavingThrow);
    EXPECT_EQ(grimoire[0].saveDc, 13) << "8 + maitrise 2 + Sag 3";
    EXPECT_EQ(grimoire[1].id, "bless");
    EXPECT_EQ(grimoire[1].maxTargets, 3);
    EXPECT_EQ(grimoire[2].id, "cure-wounds");
    EXPECT_EQ(grimoire[2].mechanism, core::SpellMechanism::Healing);
    ASSERT_TRUE(grimoire[2].healing.has_value());
    EXPECT_EQ(*grimoire[2].healing, (core::Dice{.count = 1, .faces = 8, .modifier = 3}));
    EXPECT_EQ(ignores, (std::vector<std::string>{"Lumiere"}));
}

/**
 * @brief Flamme sacree : une sauvegarde de Dexterite qui annule, 1d8 radiant, 2d8 au niveau 5.
 * \castest{<b>La flamme sacree fait sauvegarder le mannequin ; qui reussit ne perd rien, qui rate
 * perd les des lances ; au niveau 5, deux d8.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Priest N1 contre un mannequin a six cases, plusieurs graines.<br/>2. Priest N5.<br/>
 * \tattendu « sauvegarde de Dexterite DD 13 ; 1 creature(s) » ; les deux issues se voient ; une
 * reussite laisse les PV intacts ; N5 : 2d8.
 * }
 */
TEST(ClassPriestTest, FlammeSacreeAnnuleSurUneSauvegardeReussie) {
    const core::LoadedCharacterSheet charge = priest();
    core::ArenaSession session(test_support::room());
    core::ArenaBout bout = combatDe(charge, {test_support::dummy("Mannequin", {7, 3}, 10, 0)});
    bool reussie = false;
    bool ratee = false;
    for (std::uint64_t graine = 1; graine < 40 && !(reussie && ratee); ++graine) {
        bout.seed = graine;
        ASSERT_TRUE(session.mount(bout).refusals.empty());
        ASSERT_TRUE(session.start());
        const core::ArenaAttack lancer =
            session.castSpell(CombatantId{2}, sortDe(session, "sacred-flame"));
        ASSERT_EQ(lancer.result, core::ArenaActionResult::Done);
        EXPECT_NE(lancer.summary.find("sauvegarde de Dexterite DD 13 ; 1 creature(s)"),
                  std::string::npos)
            << lancer.summary;
        const std::string ligne = journalLine(session.journal(), "  Mannequin : d20");
        const core::Combatant* mannequin = session.combat().find(CombatantId{2});
        if (ligne.find("aucun degat") != std::string::npos) {
            reussie = true;
            EXPECT_EQ(mannequin->profile.currentHitPoints, 60);
        } else {
            ratee = true;
            EXPECT_LT(mannequin->profile.currentHitPoints, 60);
        }
    }
    EXPECT_TRUE(reussie && ratee);
    const core::LoadedCharacterSheet niveau5 = priest(5);
    const test_support::RpgCatalogs& catalogues = test_support::rpgCatalogs();
    std::vector<std::string> ignores;
    const std::vector<core::ArenaSpell> grimoire =
        core::arenaSpellsFor(niveau5.sheet, *catalogues.options.findClass("priest"),
                             catalogues.options.spells, 3, ignores);
    EXPECT_EQ(grimoire[0].attack.damage.front().dice.count, 2);
}

/**
 * @brief Soin des blessures : 1d8 + Sag a un allie au contact, qui se releve s'il etait a terre.
 * \castest{<b>La Priest soigne un allie tombe a 0 PV : il se releve avec 1d8+3 PV ; un ennemi,
 * un allie hors de portee sont refuses.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Priest N1, un allie au contact, un allie a cinq cases, un mannequin ennemi.<br/>2.
 * Porter l'allie a 0 PV.<br/>3. Soigner l'ennemi, l'allie lointain, puis l'allie tombe.<br/>
 * \tattendu InvalidTarget, OutOfReach, puis Done : « soin Helga Pierre-Sûre -> Allie : 1d8+3 »,
 * PV entre 4 et 11, debout.
 * }
 */
TEST(ClassPriestTest, SoinDesBlessuresReleveUnAllie) {
    const core::LoadedCharacterSheet charge = priest();
    core::ArenaSession session(test_support::room());
    ASSERT_TRUE(
        session
            .mount(combatDe(charge,
                            {test_support::dummy("Allie", {2, 3}, 10, 0, 20, CombatSide::Allies),
                             test_support::dummy("Loin", {6, 3}, 10, 0, 20, CombatSide::Allies),
                             test_support::dummy("Mannequin", {9, 6}, 10, 0)}))
            .refusals.empty());
    ASSERT_TRUE(session.start());
    session.combat().applyDamage(CombatantId{2}, 20);
    ASSERT_EQ(session.combat().find(CombatantId{2})->status, core::CombatantStatus::Down);
    const std::size_t soin = sortDe(session, "cure-wounds");
    EXPECT_EQ(session.castSpell(CombatantId{4}, soin).result,
              core::ArenaActionResult::InvalidTarget);
    EXPECT_EQ(session.castSpell(CombatantId{3}, soin).result, core::ArenaActionResult::OutOfReach);
    // Un allie a terre : le soin le releve.
    const core::ArenaAttack lancer = session.castSpell(CombatantId{2}, soin);
    ASSERT_EQ(lancer.result, core::ArenaActionResult::Done) << lancer.summary;
    EXPECT_NE(lancer.summary.find("soin " + PRIEST + " -> Allie : 1d8+3"), std::string::npos)
        << lancer.summary;
    const core::Combatant* allie = session.combat().find(CombatantId{2});
    EXPECT_EQ(allie->status, core::CombatantStatus::Standing);
    EXPECT_GE(allie->profile.currentHitPoints, 4);
    EXPECT_LE(allie->profile.currentHitPoints, 11);
    EXPECT_EQ((*session.spells(CombatantId{1}))[soin].uses, 1);
}

/**
 * @brief Benediction : trois allies, un d4 a leurs attaques et a leurs sauvegardes, dix rounds.
 * \castest{<b>La Priest benit un allie : elle et l'allie le plus proche d'elle le sont aussi, pas
 * le lointain ; l'attaque de l'allie porte « Benediction » ; la sauvegarde de la Priest contre une
 * boule de feu aussi ; l'effet cesse au bout de dix rounds.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Priest N1, allies en (2, 3), (1, 4) et (1, 7) ; un Mage ennemi N5 ; un mannequin.
 * <br/>2. Benediction sur (2, 3).<br/>3. Tour de l'allie : il attaque.<br/>4. Tour du Mage :
 * boule de feu sur la Priest.<br/>5. Dix rounds.<br/>
 * \tattendu Trois porteurs ; un modificateur Benediction de 1 a 4 sur l'attaque et sur la
 * sauvegarde ; « fin de l'effet Benediction … (duree ecoulee) ».
 * }
 */
TEST(ClassPriestTest, BenedictionAjouteUnD4AuxJetsDeTroisAllies) {
    const core::LoadedCharacterSheet charge = priest();
    core::LoadedCharacterSheet mage = test_support::loadPremade("heros-mage.json");
    ASSERT_TRUE(test_support::levelUpTo(mage.sheet, 5).empty());
    core::ArenaContestant adversaire =
        test_support::hero(mage.sheet, mage.inventory, {}, {9, 3}, CombatSide::Enemies, 50);
    core::ArenaSession session(test_support::room());
    ASSERT_TRUE(
        session
            .mount(combatDe(charge,
                            {test_support::dummy("Allie", {2, 3}, 10, 5, 60, CombatSide::Allies),
                             test_support::dummy("Proche", {1, 4}, 10, 5, 60, CombatSide::Allies),
                             test_support::dummy("Lointain", {1, 7}, 10, 5, 60, CombatSide::Allies),
                             adversaire, test_support::dummy("Mannequin", {3, 3}, 10, 0)}))
            .refusals.empty());
    ASSERT_TRUE(session.start());
    ASSERT_EQ(session.castSpell(CombatantId{2}, sortDe(session, "bless")).result,
              core::ArenaActionResult::Done);
    EXPECT_TRUE(journalHas(session.journal(),
                           "effet Benediction sur Allie, " + PRIEST + ", Proche : +1d4 aux jets"));
    EXPECT_FALSE(session.hasEffect(CombatantId{4}, core::SpellEffectKind::Bless));

    jusquAuTourDe(session, CombatantId{5});
    const std::size_t boule = sortDe(session, "fireball", CombatantId{5});
    ASSERT_EQ(session.castSpell(CombatantId{1}, boule).result, core::ArenaActionResult::Done);
    const std::string sauvegarde = journalLine(session.journal(), "  " + PRIEST + " : d20");
    EXPECT_NE(sauvegarde.find("(Benediction)"), std::string::npos) << sauvegarde;

    jusquAuTourDe(session, CombatantId{2});
    const core::ArenaAttack coup = session.attack(CombatantId{6});
    ASSERT_EQ(coup.result, core::ArenaActionResult::Done);
    const std::vector<core::Modifier>& modificateurs = coup.outcome->roll.check.modifiers;
    const auto de = std::ranges::find(modificateurs, "Benediction", &core::Modifier::source);
    ASSERT_NE(de, modificateurs.end());
    EXPECT_GE(de->value, 1);
    EXPECT_LE(de->value, 4);

    for (int garde = 0;
         garde < 80 && session.hasEffect(CombatantId{2}, core::SpellEffectKind::Bless); ++garde) {
        ASSERT_TRUE(session.endTurn());
    }
    EXPECT_TRUE(
        journalHas(session.journal(), "fin de l'effet Benediction sur Allie (duree ecoulee)"));
    EXPECT_LE(session.combat().round(), 12);
}

/**
 * @brief Arme spirituelle (N3) : action bonus, attaque de sort au corps a corps, puis l'arme
 *        frappe de nouveau a chaque tour sans lancer.
 * \castest{<b>La Priest invoque l'arme spirituelle sur un mannequin a cinq cases : action bonus
 * depensee, action gardee, 1d8+3 de force ; au tour suivant, l'arme frappe de nouveau et les
 * lancers ne bougent pas.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Priest N3 contre un mannequin en (6, 3).<br/>2. Arme spirituelle ; la relancer dans
 * le meme tour.<br/>3. Tour suivant : la relancer.<br/>
 * \tattendu Done, action bonus a 0, action a 1, un profil au corps a corps de 1d8+3 force,
 * « arme invoquee » ; NoAction ; puis « (l'arme frappe de nouveau) », 1 lancer restant.
 * }
 */
TEST(ClassPriestTest, ArmeSpirituelleFrappeParActionBonus) {
    const core::LoadedCharacterSheet charge = priest(3);
    core::ArenaSession session(test_support::room());
    ASSERT_TRUE(
        session.mount(combatDe(charge, {test_support::dummy("Mannequin", {6, 3}, 10, 0, 200)}))
            .refusals.empty());
    ASSERT_TRUE(session.start());
    const std::size_t arme = sortDe(session, "spiritual-weapon");
    const core::ArenaSpell& sort = (*session.spells(CombatantId{1}))[arme];
    EXPECT_TRUE(sort.bonusAction);
    EXPECT_EQ(sort.attack.kind, core::AttackKind::Melee);
    EXPECT_EQ(sort.attack.damage.front().dice, (core::Dice{.count = 1, .faces = 8, .modifier = 3}));
    ASSERT_EQ(session.castSpell(CombatantId{2}, arme).result, core::ArenaActionResult::Done);
    const core::Combatant* helga = session.combat().find(CombatantId{1});
    EXPECT_EQ(helga->economy.remaining(core::BONUS_ACTION_RESOURCE), 0);
    EXPECT_EQ(helga->economy.remaining(core::ACTION_RESOURCE), 1);
    EXPECT_TRUE(session.hasEffect(CombatantId{1}, core::SpellEffectKind::SpiritualWeapon));
    EXPECT_TRUE(journalHas(session.journal(), "arme invoquee"));
    EXPECT_EQ(session.castSpell(CombatantId{2}, arme).result, core::ArenaActionResult::NoAction);

    jusquAuTourDe(session, CombatantId{2});
    jusquAuTourDe(session, CombatantId{1});
    ASSERT_EQ(session.castSpell(CombatantId{2}, arme).result, core::ArenaActionResult::Done);
    EXPECT_TRUE(journalHas(session.journal(), "sort Arme spirituelle (l'arme frappe de nouveau)"));
    EXPECT_EQ((*session.spells(CombatantId{1}))[arme].uses, 1);
}

/**
 * @brief Du niveau 1 au niveau 5, la table du Priest se lit ; les sorts qui attendent le LOT-137
 *        sont connus et dits.
 * \castest{<b>Monter la Priest de la page 203 jusqu'au niveau 5 donne ses capacites et ses neuf
 * sorts ; epargner les mourants, restauration inferieure, lumiere du jour et revigorer ne se jouent
 * pas en combat et declarent ce qu'ils attendent.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Monter de 1 a 5.<br/>2. Lire capacites, sorts connus, grimoire de combat.<br/>
 * \tattendu Aucun manquant ; quatre capacites ; neuf sorts connus ; grimoire de quatre sorts ;
 * cinq sorts ignores, dont trois qui declarent un mecanisme requis.
 * }
 */
TEST(ClassPriestTest, DuNiveau1AuNiveau5LaTableSeLit) {
    core::LoadedCharacterSheet charge = priest();
    for (int niveau = 2; niveau <= 5; ++niveau) {
        EXPECT_TRUE(test_support::levelUpTo(charge.sheet, niveau).empty()) << niveau;
    }
    EXPECT_EQ(test_support::capacityIds(charge.sheet),
              (std::vector<std::string>{"simplified-spellcasting", "specific-cantrips",
                                        "experience", "ability-score-improvement"}));
    EXPECT_EQ(charge.sheet.knownSpells.size(), 9U);
    const test_support::RpgCatalogs& catalogues = test_support::rpgCatalogs();
    std::vector<std::string> ignores;
    const std::vector<core::ArenaSpell> grimoire =
        core::arenaSpellsFor(charge.sheet, *catalogues.options.findClass("priest"),
                             catalogues.options.spells, 3, ignores);
    EXPECT_EQ(grimoire.size(), 4U);
    EXPECT_EQ(ignores.size(), 5U);
    for (const char* id : {"spare-the-dying", "lesser-restoration", "revivify"}) {
        const core::Spell* sort = catalogues.options.spells.find(id);
        ASSERT_NE(sort, nullptr) << id;
        EXPECT_FALSE(sort->requiredMechanisms.empty()) << id;
    }
}
