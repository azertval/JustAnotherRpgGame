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
#include "Core/Rpg/ClassCapacities.h"
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

/**
 * @brief Les capacités de classe entrent dans la prévisualisation comme dans le jet (`LOT-140`).
 * \castest{<b>Le bonus au jet d'une capacite s'ajoute au jet requis ; les des d'une capacite a
 * condition entrent dans l'esperance quand la condition tient, et la previsualisation nomme la
 * capacite et la raison quand elle ne joue pas.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Une heroine a « +2 au jet » et « 1d6 en plus, une fois par tour, si un allie est au
 * contact de la cible » ; un allie au contact d'une cible, une autre cible seule.<br/>2.
 * Previsualiser sur chaque cible.<br/>3. Frapper la premiere, previsualiser encore.<br/>
 * \tattendu Jet requis avec le +2 ; sur la cible flanquee, l'esperance porte le 1d6 et la capacite
 * est nommee ; sur la cible seule, la capacite est nommee sans jouer, avec la raison ; apres une
 * touche, la raison est « deja jouee ce tour ».
 * }
 */
TEST(CombatPreviewTest, LesCapacitesEntrentDansLaPrevisualisation) {
    core::Capacity viser{.id = "viser", .name = "Viser juste"};
    viser.effects.push_back({.kind = core::CapacityEffectKind::AttackBonus, .value = 2});
    core::Capacity sournoise{.id = "sournoise", .name = "Attaque sournoise"};
    core::CapacityEffect des;
    des.kind = core::CapacityEffectKind::ExtraDamage;
    des.dice = *core::parseDice("1d6");
    des.oncePerTurn = true;
    des.allyAdjacentToTarget = true;
    sournoise.effects.push_back(des);

    core::ArenaBout bout{.seed = 4, .lethal = false, .heroicMark = false};
    core::ArenaContestant heroine = combattant("Heroine", CombatSide::Allies, {4, 3}, 100, 12, 4);
    heroine.capacities = {viser, sournoise};
    bout.contestants = {heroine, combattant("Allie", CombatSide::Allies, {6, 3}, 0),
                        combattant("Flanque", CombatSide::Enemies, {5, 3}, -100, 12, 0),
                        combattant("Seul", CombatSide::Enemies, {4, 4}, -100, 12, 0)};
    core::ArenaSession session(salle(12, 8));
    session.mount(bout);
    ASSERT_TRUE(session.start());
    ASSERT_EQ(session.combat().activeCombatant(), CombatantId{1});

    const std::optional<core::AttackPreview> flanque =
        core::previewAttack(session, CombatantId{3}, 0);
    ASSERT_TRUE(flanque.has_value());
    EXPECT_EQ(flanque->attackBonus, 6);
    EXPECT_EQ(flanque->requiredRoll, 6);
    ASSERT_EQ(flanque->capacityModifiers.size(), 1U);
    EXPECT_EQ(flanque->capacityModifiers.front().source, "Viser juste");
    ASSERT_EQ(flanque->extraDamage.size(), 1U);
    EXPECT_EQ(flanque->extraDamage.front().source, "Attaque sournoise");
    EXPECT_TRUE(flanque->extraDamage.front().applies);

    const std::optional<core::AttackPreview> seul = core::previewAttack(session, CombatantId{4}, 0);
    ASSERT_TRUE(seul.has_value());
    ASSERT_EQ(seul->extraDamage.size(), 1U);
    EXPECT_FALSE(seul->extraDamage.front().applies);
    EXPECT_FALSE(seul->extraDamage.front().reason.empty());
    // Meme jet requis, meme chance : seuls les des different -- 1d6 de plus, soit 3,5 points par
    // touche.
    EXPECT_EQ(seul->hitChance, flanque->hitChance);
    EXPECT_GT(flanque->expectedDamage, seul->expectedDamage);
    EXPECT_EQ(flanque->expectedDamage - seul->expectedDamage,
              flanque->hitChance * 7 + core::criticalChance(20, flanque->stance) * 7);

    // Une fois jouee, la capacite ne joue plus ce tour : la previsualisation le dit.
    const core::ArenaAttack coup = session.attack(CombatantId{3}, 0);
    ASSERT_TRUE(coup.outcome.has_value());
    const std::optional<core::AttackPreview> apres =
        core::previewAttack(session, CombatantId{3}, 0);
    ASSERT_TRUE(apres.has_value());
    ASSERT_EQ(apres->extraDamage.size(), 1U);
    if (coup.outcome->roll.hit) {
        EXPECT_FALSE(apres->extraDamage.front().applies);
        EXPECT_EQ(apres->extraDamage.front().reason, "deja jouee ce tour");
    } else {
        EXPECT_TRUE(apres->extraDamage.front().applies);
    }
}
