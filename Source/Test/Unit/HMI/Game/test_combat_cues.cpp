// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_combat_cues.cpp
 * @brief Tests de la file des mouvements du combat (`LOT-118`) : une marche se joue case par case
 *        à la vitesse du monde, un coup porte au milieu de son geste, un mort reste à terre.
 */

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
    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Walk,
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
    file.push(hmi::CombatCue{.kind = hmi::CombatCueKind::Walk,
                             .actor = RAT,
                             .path = {{.column = 1, .row = 0}}});
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
