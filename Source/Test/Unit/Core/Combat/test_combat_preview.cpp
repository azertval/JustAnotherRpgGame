// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_combat_preview.cpp
 * @brief Tests unitaires de la prévisualisation de combat (`LOT-24`) : ce que l'écran montre avant
 *        l'engagement est ce que le jet jette, et le joueur décline ses attaques d'opportunité.
 */

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/Arena.h"
#include "Core/Combat/CombatPreview.h"
#include "Core/Combat/EnemyAi.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/TileMap.h"
#include "Core/Rpg/Dice.h"

namespace {

using core::CombatantId;
using core::CombatSide;
using core::GridPosition;

core::Level salle(int largeur, int hauteur) {
    core::TileMap carte(largeur, hauteur);
    for (int x = 0; x < largeur; ++x) {
        carte.setTile(x, 0, core::TileType::Wall);
        carte.setTile(x, hauteur - 1, core::TileType::Wall);
    }
    for (int y = 0; y < hauteur; ++y) {
        carte.setTile(0, y, core::TileType::Wall);
        carte.setTile(largeur - 1, y, core::TileType::Wall);
    }
    return core::Level(core::LevelData{
        .name = "salle", .tileMap = std::move(carte), .entities = {}, .entry = {1, 1}});
}

core::ArenaContestant combattant(const std::string& nom, CombatSide camp, GridPosition case_,
                                 int initiative, int ca = 12, int bonus = 4) {
    core::CombatantProfile profil{.name = nom,
                                  .side = camp,
                                  .maximumHitPoints = 40,
                                  .currentHitPoints = 40,
                                  .dexterity = 10,
                                  .initiativeModifier = initiative,
                                  .movement = 6};
    profil.armorClass = ca;
    core::AttackProfile coup;
    coup.label = nom;
    coup.modifiers = {{.source = "bonus d'attaque", .value = bonus}};
    coup.damage = {
        {.dice = *core::parseDice("1d6+2"), .type = core::DamageType::Slashing, .flags = 0}};
    return {.profile = profil, .attacks = {coup}, .position = case_, .markId = {}, .behavior = {}};
}

/// Compare une prévisualisation au jet que la session jette ensuite.
void attendreLeMemeJet(core::ArenaSession& session, CombatantId cible) {
    const std::optional<core::AttackPreview> apercu = core::previewAttack(session, cible, 0);
    ASSERT_TRUE(apercu.has_value());
    ASSERT_EQ(apercu->check, core::TargetCheck::Valid);
    const core::ArenaAttack attaque = session.attack(cible, 0);
    ASSERT_TRUE(attaque.outcome.has_value());
    const core::AttackRoll& jet = attaque.outcome->roll;
    EXPECT_EQ(jet.armorClass, apercu->armorClass);
    EXPECT_EQ(jet.cover, apercu->cover);
    EXPECT_EQ(jet.check.stance, apercu->stance);
    EXPECT_EQ(jet.advantages, apercu->advantages);
    EXPECT_EQ(jet.disadvantages, apercu->disadvantages);
    EXPECT_EQ(apercu->hitChance, core::hitChance(apercu->requiredRoll, apercu->stance));
    // Le jet requis se relit au journal : la CA y est ecrite, le bonus aussi.
    EXPECT_NE(session.journal().back().find(std::to_string(apercu->armorClass)), std::string::npos);
}

}  // namespace

/**
 * @brief La prévisualisation d'une attaque est le jet que la session jette ensuite.
 * \castest{<b>Ce que l'ecran montre avant l'attaque -- CA abri compris, posture, sources d'avantage
 * et de desavantage, chance de toucher -- est exactement ce que le jet jette.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Une attaque au contact d'un ennemi pris en tenaille, dans une arene a
 * tenaille.<br/>2. Un tir a longue portee, a travers un allie, sur une cible qui esquive.<br/>3.
 * Une cible hors de portee.<br/>
 * \tattendu Meme CA, meme abri, meme posture, memes sources au jet ; chance tiree du jet requis ;
 * hors de portee, le refus sans chance.
 * }
 */
TEST(CombatPreviewTest, LaPrevisualisationEstLeJet) {
    {
        core::ArenaBout bout{.seed = 2, .lethal = false, .heroicMark = false, .flanking = true};
        bout.contestants = {combattant("A", CombatSide::Allies, {4, 3}, 100),
                            combattant("B", CombatSide::Allies, {6, 3}, 0),
                            combattant("Cible", CombatSide::Enemies, {5, 3}, -100)};
        core::ArenaSession session(salle(10, 8));
        session.mount(bout);
        ASSERT_TRUE(session.start());
        ASSERT_EQ(session.combat().activeCombatant(), CombatantId{1});
        const std::optional<core::AttackPreview> apercu =
            core::previewAttack(session, CombatantId{3}, 0);
        ASSERT_TRUE(apercu.has_value());
        EXPECT_EQ(apercu->stance, core::RollStance::Advantage);
        EXPECT_EQ(apercu->advantages, (std::vector<std::string>{"prise en tenaille"}));
        EXPECT_EQ(apercu->requiredRoll, 8);
        EXPECT_EQ(apercu->hitPercent(), 88);
        attendreLeMemeJet(session, CombatantId{3});
    }
    {
        core::ArenaBout bout{.seed = 6, .lethal = false, .heroicMark = false};
        core::ArenaContestant archere = combattant("Archere", CombatSide::Allies, {2, 3}, 50);
        archere.attacks[0].kind = core::AttackKind::Ranged;
        archere.attacks[0].range = core::AttackRange{.normal = 2, .maximum = 10};
        bout.contestants = {archere, combattant("Bouclier", CombatSide::Allies, {5, 3}, -100),
                            combattant("Cible", CombatSide::Enemies, {9, 3}, 100)};
        core::ArenaSession session(salle(12, 8));
        session.mount(bout);
        ASSERT_TRUE(session.start());
        ASSERT_EQ(session.combat().activeCombatant(), CombatantId{3});
        ASSERT_TRUE(session.dodge());
        ASSERT_TRUE(session.endTurn());
        ASSERT_EQ(session.combat().activeCombatant(), CombatantId{1});
        const std::optional<core::AttackPreview> apercu =
            core::previewAttack(session, CombatantId{3}, 0);
        ASSERT_TRUE(apercu.has_value());
        EXPECT_EQ(apercu->cover, core::Cover::Half);
        EXPECT_EQ(apercu->armorClass, 14);
        EXPECT_EQ(apercu->stance, core::RollStance::Disadvantage);
        EXPECT_EQ(apercu->disadvantages.size(), 2U);
        attendreLeMemeJet(session, CombatantId{3});
    }
    {
        core::ArenaBout bout{.seed = 1, .lethal = false, .heroicMark = false};
        bout.contestants = {combattant("A", CombatSide::Allies, {2, 3}, 100),
                            combattant("Loin", CombatSide::Enemies, {8, 3}, -100)};
        core::ArenaSession session(salle(10, 8));
        session.mount(bout);
        ASSERT_TRUE(session.start());
        const std::optional<core::AttackPreview> apercu =
            core::previewAttack(session, CombatantId{2}, 0);
        ASSERT_TRUE(apercu.has_value());
        EXPECT_EQ(apercu->check, core::TargetCheck::OutOfReach);
        EXPECT_EQ(apercu->hitChance, 0);
        EXPECT_FALSE(core::firstValidAttack(session, CombatantId{2}).has_value());
        EXPECT_FALSE(core::previewAttack(session, CombatantId{2}, 3).has_value());
    }
}

/**
 * @brief Le déplacement se prévisualise, et le joueur décline ses attaques d'opportunité.
 * \castest{<b>La previsualisation d'un deplacement donne le chemin, le deplacement restant et qui
 * frappera en chemin ; un combattant dont le joueur laisse passer les opportunites ne frappe
 * pas.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Une heroine au contact d'un ogre ; previsualiser un pas qui sort de son
 * allonge.<br/>2. Le joueur dit que l'ogre laisse passer ; previsualiser, puis jouer le pas.<br/>
 * \tattendu Un chemin, le budget restant, l'ogre annonce ; puis personne, et aucune attaque
 * d'opportunite au journal, la reaction de l'ogre intacte.
 * }
 */
TEST(CombatPreviewTest, LeDeplacementSePrevisualiseEtLOpportuniteSeDecline) {
    core::ArenaBout bout{.seed = 9, .lethal = false, .heroicMark = false};
    bout.contestants = {combattant("Heroine", CombatSide::Allies, {2, 3}, 100),
                        combattant("Ogre", CombatSide::Enemies, {3, 3}, -100)};
    core::ArenaSession session(salle(10, 8));
    session.mount(bout);
    ASSERT_TRUE(session.start());
    ASSERT_EQ(session.combat().activeCombatant(), CombatantId{1});

    const core::MovePreview pas = core::previewMove(session, {2, 5});
    ASSERT_TRUE(pas.path.has_value());
    EXPECT_EQ(pas.path->cost, 2);
    EXPECT_EQ(pas.movementLeft, 4);
    EXPECT_EQ(pas.opportunities, (std::vector<CombatantId>{CombatantId{2}}));
    EXPECT_FALSE(core::previewMove(session, {9, 9}).path.has_value());

    EXPECT_TRUE(session.takesOpportunities(CombatantId{2}));
    session.setTakesOpportunities(CombatantId{2}, false);
    EXPECT_TRUE(core::previewMove(session, {2, 5}).opportunities.empty());
    ASSERT_EQ(session.move({2, 5}).result, core::MoveResult::Moved);
    EXPECT_TRUE(std::ranges::none_of(
        session.journal(), [](const std::string& l) { return l.starts_with("opportunite : "); }));
    EXPECT_EQ(session.combat().find(CombatantId{2})->economy.remaining(core::REACTION_RESOURCE), 1);

    // Le choix survit au rejeu.
    session.replay();
    EXPECT_FALSE(session.takesOpportunities(CombatantId{2}));
}
