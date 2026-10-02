// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_combat_cues.cpp
 * @brief Tests de la file des mouvements du combat (`LOT-118`) : une marche se joue case par case
 *        à la vitesse du monde, un coup porte au milieu de son geste, un mort reste à terre.
 */

#include <vector>

#include <gtest/gtest.h>

#include "HMI/Game/CombatCues.h"

namespace {

constexpr core::CombatantId HEROS{1};
constexpr core::CombatantId RAT{2};

}  // namespace

/**
 * @brief Une marche de trois cases dure une seconde et demie, se voit case par case, et finit au
 *        repos sur la case d'arrivée, tournée dans la direction du dernier pas.
 * \castest{<b>Une marche se rejoue a deux cases par seconde, puis revient au repos.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser le heros en (0, 0) ; pousser une marche par (1, 0), (2, 0), (2, 1).<br/>
 * 2. Avancer de 0,25 s, puis de 0,5 s, puis jusqu'au bout.<br/>
 * \tattendu A 0,25 s il est a mi-chemin de la premiere case, en marche, tourne au sud-est ; a
 * 0,75 s il est entre la premiere et la deuxieme ; a 1,5 s il est au repos en (2, 1), tourne au
 * sud-ouest, et la file est vide.
 * }
 */
TEST(CombatCuesTest, UneMarcheSeRejoueCaseParCase) {
    hmi::CombatCueTrack file;
    file.place(HEROS, {.column = 0, .row = 0});
    EXPECT_FALSE(file.busy());
    file.push(hmi::CombatCue{
        .kind = hmi::CombatCueKind::Walk,
        .actor = HEROS,
        .path = {{.column = 1, .row = 0}, {.column = 2, .row = 0}, {.column = 2, .row = 1}}});
    ASSERT_TRUE(file.busy());

    file.advance(0.25F);
    const hmi::FigureMotion* heros = file.motionOf(HEROS);
    ASSERT_NE(heros, nullptr);
    EXPECT_EQ(heros->clip, hmi::figure_clips::WALK);
    EXPECT_NEAR(heros->point.x, 1.0F, 1e-4F);
    EXPECT_NEAR(heros->point.y, 0.5F, 1e-4F);
    EXPECT_EQ(heros->facing, hmi::FigureFacing::SouthEast);

    file.advance(0.5F);
    EXPECT_NEAR(heros->point.x, 2.0F, 1e-4F) << "une case et demie parcourue";
    EXPECT_NEAR(heros->point.y, 0.5F, 1e-4F);

    file.advance(0.75F);
    EXPECT_FALSE(file.busy());
    EXPECT_EQ(heros->clip, hmi::figure_clips::IDLE);
    EXPECT_NEAR(heros->point.x, 2.5F, 1e-4F);
    EXPECT_NEAR(heros->point.y, 1.5F, 1e-4F);
    EXPECT_EQ(heros->facing, hmi::FigureFacing::SouthWest);
}

/**
 * @brief Le coup porte au milieu du geste : le touché de la cible commence à mi-attaque, et une
 *        chute laisse la cible à terre pour de bon.
 * \castest{<b>Attaque, touche et mort s'enchainent a l'instant de l'impact.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser le heros en (0, 0) et le rat en (1, 0) ; pousser une attaque du heros sur le
 * rat, un touche du rat, une mort du rat.<br/>2. Avancer d'un quart de geste, puis jusqu'a la
 * moitie et au-dela, puis d'une seconde de plus.<br/>
 * \tattendu Au quart : le heros attaque, tourne vers le rat, le rat est encore au repos. Passe la
 * moitie : le rat tombe (bande de mort, a terre). Une seconde plus tard : le heros est revenu au
 * repos, le rat reste a terre sur sa bande de mort, dont les secondes continuent de courir, et un
 * touche pousse ensuite ne le releve pas.
 * }
 */
TEST(CombatCuesTest, LeCoupPorteAMiGesteEtUnMortResteATerre) {
    using hmi::CombatCueTrack;
    hmi::CombatCueTrack file;
    file.place(HEROS, {.column = 0, .row = 0}, hmi::FigureFacing::NorthWest);
    file.place(RAT, {.column = 1, .row = 0});
    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Attack,
                             .actor = HEROS,
                             .path = {},
                             .target = core::GridPosition{.column = 1, .row = 0}});
    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Hit, .actor = RAT});
    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Death, .actor = RAT});

    file.advance(CombatCueTrack::ACTION_SECONDS * 0.25F);
    const hmi::FigureMotion* heros = file.motionOf(HEROS);
    const hmi::FigureMotion* rat = file.motionOf(RAT);
    ASSERT_NE(heros, nullptr);
    ASSERT_NE(rat, nullptr);
    EXPECT_EQ(heros->clip, hmi::figure_clips::ATTACK);
    EXPECT_EQ(heros->facing, hmi::FigureFacing::SouthEast) << "tourne vers sa cible";
    EXPECT_EQ(rat->clip, hmi::figure_clips::IDLE) << "le coup n'a pas encore porte";

    file.advance(CombatCueTrack::ACTION_SECONDS * 0.35F);
    EXPECT_EQ(rat->clip, hmi::figure_clips::DEATH);
    EXPECT_TRUE(rat->dead);

    file.advance(1.0F);
    EXPECT_FALSE(file.busy());
    EXPECT_EQ(heros->clip, hmi::figure_clips::IDLE);
    EXPECT_EQ(rat->clip, hmi::figure_clips::DEATH);
    EXPECT_GT(rat->clipSeconds, CombatCueTrack::ACTION_SECONDS);

    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Hit, .actor = RAT});
    file.finishAll();
    EXPECT_EQ(rat->clip, hmi::figure_clips::DEATH) << "un mort n'encaisse plus rien";
    EXPECT_FALSE(file.busy());
}

/**
 * @brief Ce qui n'a pas de figurine ne se montre pas, une marche vide non plus, et `finishAll`
 *        vide la file en posant chacun à son arrivée.
 * \castest{<b>La file ignore l'inconnu et sait tout finir d'un coup.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Pousser une marche pour un combattant jamais pose, et une marche vide pour le
 * heros.<br/>2. Pousser une vraie marche et une attaque, puis tout finir.<br/>
 * \tattendu Rien n'attend apres les deux premieres ; apres `finishAll`, la file est vide et le
 * heros est au repos sur sa case d'arrivee.
 * }
 */
TEST(CombatCuesTest, LInconnuEstIgnoreEtToutPeutFinirDUnCoup) {
    hmi::CombatCueTrack file;
    file.place(HEROS, {.column = 0, .row = 0});
    file.push(hmi::CombatCue{
        .kind = hmi::CombatCueKind::Walk, .actor = RAT, .path = {{.column = 1, .row = 0}}});
    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Walk, .actor = HEROS, .path = {}});
    EXPECT_FALSE(file.busy());
    EXPECT_EQ(file.pending(), 0U);

    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Walk,
                             .actor = HEROS,
                             .path = {{.column = 0, .row = 1}, {.column = 0, .row = 2}}});
    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Attack, .actor = HEROS});
    EXPECT_EQ(file.pending(), 2U);
    file.finishAll();
    EXPECT_FALSE(file.busy());
    const hmi::FigureMotion* heros = file.motionOf(HEROS);
    ASSERT_NE(heros, nullptr);
    EXPECT_EQ(heros->clip, hmi::figure_clips::IDLE);
    EXPECT_NEAR(heros->point.y, 2.5F, 1e-4F);
    file.remove(HEROS);
    EXPECT_EQ(file.motionOf(HEROS), nullptr);
}

/**
 * @brief Un tir joue la bande `ranged` ; sa fleche vole de l'archer a la cible jusqu'a l'impact,
 *        puis le rate parait a la cible (`LOT-136`).
 * \castest{<b>Le tir, sa fleche et le rate s'enchainent sur le geste.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser le heros en (0, 0) et le rat en (3, 0) ; pousser un tir du heros sur le rat,
 * la fleche en vol, un rate a la case du rat.<br/>2. Avancer de 0,16 s, puis jusqu'a 0,4 s, puis
 * d'une seconde et demie.<br/>
 * \tattendu A 0,16 s : le heros joue `ranged`, un seul effet, `arrow` (la cible est a droite de
 * l'ecran), a mi-chemin. A 0,4 s : la fleche est arrivee et partie, le rate est a la case du rat.
 * A la fin : plus aucun effet, la file est vide, le heros est au repos.
 * }
 */
TEST(CombatCuesTest, UnTirJoueSaBandeEtSaFlecheVole) {
    hmi::CombatCueTrack file;
    file.place(HEROS, {.column = 0, .row = 0});
    file.place(RAT, {.column = 3, .row = 0});
    const core::GridPosition rat{.column = 3, .row = 0};
    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Attack,
                             .actor = HEROS,
                             .path = {},
                             .target = rat,
                             .ranged = true});
    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Effect,
                             .actor = HEROS,
                             .path = {},
                             .target = rat,
                             .ranged = false,
                             .effect = "arrow",
                             .travels = true});
    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Effect,
                             .actor = HEROS,
                             .path = {},
                             .target = rat,
                             .ranged = false,
                             .effect = "miss",
                             .travels = false});

    file.advance(0.16F);
    const hmi::FigureMotion* heros = file.motionOf(HEROS);
    ASSERT_NE(heros, nullptr);
    EXPECT_EQ(heros->clip, hmi::figure_clips::RANGED);
    std::vector<hmi::EffectMotion> effets = file.effects();
    ASSERT_EQ(effets.size(), 1U) << "le rate attend l'impact";
    EXPECT_EQ(effets[0].effect, "arrow");
    EXPECT_NEAR(effets[0].point.x, 2.0F, 1e-3F) << "a mi-vol";
    EXPECT_NEAR(effets[0].point.y, 0.5F, 1e-3F);

    file.advance(0.24F);
    effets = file.effects();
    ASSERT_EQ(effets.size(), 1U);
    EXPECT_EQ(effets[0].effect, "miss");
    EXPECT_NEAR(effets[0].point.x, 3.5F, 1e-4F);

    file.advance(1.5F);
    EXPECT_TRUE(file.effects().empty());
    EXPECT_FALSE(file.busy());
    EXPECT_EQ(heros->clip, hmi::figure_clips::IDLE);
}

/**
 * @brief Un projectile qui vole vers la gauche de l'ecran prend sa bande miroir, et un effet pose
 *        sans geste se joue seul (`LOT-136`).
 * \castest{<b>Un trait de feu vers la gauche de l'ecran joue `fire-bolt-left`.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Heros en (0, 0), rat en (0, 3) ; pousser un sort du heros sur le rat et son trait de
 * feu en vol.<br/>2. Avancer de 0,1 s, puis tout finir.<br/>3. Pousser seul un impact sur le rat
 * et avancer de 0,1 s.<br/>
 * \tattendu Le heros joue `cast` ; l'effet est `fire-bolt-left` ; seul, l'impact parait tout de
 * suite a la case du rat, puis la file se vide.
 * }
 */
TEST(CombatCuesTest, UnProjectileVersLaGaucheEstLeMiroir) {
    hmi::CombatCueTrack file;
    file.place(HEROS, {.column = 0, .row = 0});
    file.place(RAT, {.column = 0, .row = 3});
    const core::GridPosition rat{.column = 0, .row = 3};
    file.push(hmi::CombatCue{
        .kind = hmi::CombatCueKind::Cast, .actor = HEROS, .path = {}, .target = rat});
    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Effect,
                             .actor = HEROS,
                             .path = {},
                             .target = rat,
                             .ranged = false,
                             .effect = "fire-bolt",
                             .travels = true});
    file.advance(0.1F);
    EXPECT_EQ(file.motionOf(HEROS)->clip, hmi::figure_clips::CAST);
    ASSERT_EQ(file.effects().size(), 1U);
    EXPECT_EQ(file.effects()[0].effect, "fire-bolt-left");
    file.finishAll();

    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Effect,
                             .actor = RAT,
                             .path = {},
                             .target = rat,
                             .ranged = false,
                             .effect = "impact",
                             .travels = false});
    file.advance(0.1F);
    ASSERT_EQ(file.effects().size(), 1U);
    EXPECT_EQ(file.effects()[0].effect, "impact");
    EXPECT_NEAR(file.effects()[0].point.y, 3.5F, 1e-4F);
    file.finishAll();
    EXPECT_TRUE(file.effects().empty());
}

/**
 * @brief Les gestes d'un modèle durent ses clips et portent à leur image clé (`LOT-1005`) : le
 *        touché de la cible part à l'image clé de l'attaque, pas au milieu de la bande.
 * \castest{<b>Les signaux du combat partent a l'image cle du clip.</b><br/>
 * \tcat Unitaire · Combat sur la carte · Squelette<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Donner au heros les durees d'un squelette dont l'attaque dure 1,0 s et porte a
 * 0,7 s, et au rat un touche de 0,3 s.<br/>2. Pousser une attaque du heros sur le rat et le
 * touche du rat.<br/>3. Avancer a 0,6 s, a 0,75 s, a 0,95 s, puis a 1,05 s.<br/>
 * \tattendu A 0,6 s le heros attaque et le rat est au repos : le coup n'a pas porte, alors qu'une
 * bande l'aurait fait porter a 0,32 s. A 0,75 s le rat encaisse, depuis 0,05 s. A 0,95 s il n'a
 * pas fini (0,25 s sur 0,3), le heros attaque encore. A 1,05 s tous deux sont au repos : le geste
 * a dure son clip. Le cap du heros est celui de sa cible.
 * }
 */
TEST(CombatCuesTest, LesSignauxPartentALImageCleDuClip) {
    core::SkeletonDescription squelette;
    squelette.silhouette = "essai";
    squelette.clips = {
        core::SkeletonClip{.name = "attack", .duration = 1.0F, .loop = false, .key = 0.7F},
        core::SkeletonClip{.name = "hit", .duration = 0.3F, .loop = false, .key = std::nullopt},
        core::SkeletonClip{.name = "cast", .duration = 0.6F, .loop = false, .key = std::nullopt},
    };
    const hmi::FigureTimings durees = hmi::CombatCueTrack::timingsOf(&squelette);
    EXPECT_EQ(durees.attack, (hmi::GestureTiming{.seconds = 1.0F, .impact = 0.7F}));
    EXPECT_EQ(durees.ranged, durees.attack) << "le tir sans clip de tir est une attaque";
    EXPECT_EQ(durees.cast, (hmi::GestureTiming{.seconds = 0.6F, .impact = 0.3F}))
        << "sans image cle, le milieu du geste";
    EXPECT_FLOAT_EQ(durees.hit, 0.3F);
    EXPECT_FLOAT_EQ(durees.death, hmi::CombatCueTrack::ACTION_SECONDS) << "non declare : la bande";
    EXPECT_EQ(hmi::CombatCueTrack::timingsOf(nullptr), hmi::CombatCueTrack::stripTimings());

    hmi::CombatCueTrack file;
    file.place(HEROS, {.column = 0, .row = 0});
    file.place(RAT, {.column = 0, .row = 1});
    file.setTimings(HEROS, durees);
    file.setTimings(RAT, durees);
    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Attack,
                             .actor = HEROS,
                             .path = {},
                             .target = core::GridPosition{.column = 0, .row = 1}});
    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Hit, .actor = RAT});

    file.advance(0.6F);
    const hmi::FigureMotion* heros = file.motionOf(HEROS);
    const hmi::FigureMotion* rat = file.motionOf(RAT);
    ASSERT_NE(heros, nullptr);
    ASSERT_NE(rat, nullptr);
    EXPECT_EQ(heros->clip, hmi::figure_clips::ATTACK);
    EXPECT_NEAR(heros->clipSeconds, 0.6F, 1e-4F);
    EXPECT_NEAR(heros->heading, hmi::figureHeadingOf(hmi::FigureFacing::SouthWest), 1e-4F)
        << "tourne vers sa cible, une ligne plus bas";
    EXPECT_EQ(rat->clip, hmi::figure_clips::IDLE) << "le coup n'a pas encore porte";

    file.advance(0.15F);
    EXPECT_EQ(rat->clip, hmi::figure_clips::HIT);
    EXPECT_NEAR(rat->clipSeconds, 0.05F, 1e-4F);

    file.advance(0.2F);
    EXPECT_EQ(rat->clip, hmi::figure_clips::HIT);
    EXPECT_EQ(heros->clip, hmi::figure_clips::ATTACK);

    file.advance(0.1F);
    EXPECT_EQ(rat->clip, hmi::figure_clips::IDLE);
    EXPECT_EQ(heros->clip, hmi::figure_clips::IDLE);
    EXPECT_FALSE(file.busy());
}
