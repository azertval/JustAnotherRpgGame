// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_arena_animation_driver.cpp
 * @brief Tests unitaires du pilote d'animation des figurines du Colisée (Phase 4 du `LOT-86`) :
 *        transition idle -> attack -> idle, coup, franchissement de l'état mort — sans GPU.
 */

#include <filesystem>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "Core/Combat/BattleGrid.h"
#include "Core/Ecs/AnimationClip.h"
#include "HMI/Graphics/ArenaAnimationDriver.h"

namespace {

using core::AnimationClip;
using core::ClipEndMode;
using core::ClipSet;
using core::CombatantId;
using hmi::ArenaAnimationDriver;
using hmi::ArenaFigureAction;
using hmi::ArenaFigureAnimationSet;

constexpr CombatantId HERO = CombatantId{1};

std::shared_ptr<const ClipSet> loopingClip(const char* name, int frameCount, float frameDuration) {
    AnimationClip clip;
    clip.name = name;
    for (int i = 0; i < frameCount; ++i) {
        clip.frames.push_back(i);
    }
    clip.frameDuration = frameDuration;
    clip.endMode = ClipEndMode::Loop;
    auto clips = std::make_shared<ClipSet>();
    clips->addClip(clip);
    return clips;
}

std::shared_ptr<const ClipSet> oneShotClip(const char* name, int frameCount, float frameDuration) {
    AnimationClip clip;
    clip.name = name;
    for (int i = 0; i < frameCount; ++i) {
        clip.frames.push_back(i);
    }
    clip.frameDuration = frameDuration;
    clip.endMode = ClipEndMode::OneShot;
    auto clips = std::make_shared<ClipSet>();
    clips->addClip(clip);
    return clips;
}

// Une figurine avec ses cinq actions : idle/walk bouclent, attack/hit/death sont ponctuelles.
ArenaFigureAnimationSet heroFigure() {
    ArenaFigureAnimationSet figure;
    figure.idle = loopingClip("idle", 4, 0.5F);
    figure.walk = loopingClip("walk", 4, 0.12F);
    figure.attack = oneShotClip("attack", 4, 0.1F);
    figure.hit = oneShotClip("hit", 4, 0.1F);
    figure.death = oneShotClip("death", 4, 0.15F);
    return figure;
}

ArenaAnimationDriver driverWithHero() {
    ArenaAnimationDriver driver;
    driver.setFigureAnimations("bram", heroFigure());
    return driver;
}

/**
 * @brief Un combattant fraîchement joué reste sur `Idle`, première image.
 * \castest{<b>Un combattant non déclenché ne bouge pas.</b><br/>
 * \tcat Unitaire · Pilote d'animation de l'arène<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Avancer un pilote vide de dix secondes.<br/>
 * \tattendu `snapshot()` ne contient aucune figurine ; `actionOf` retombe sur `Idle`.
 * }
 */
TEST(ArenaAnimationDriverTest, CombattantNonSuiviResteIdle) {
    ArenaAnimationDriver driver = driverWithHero();
    driver.advance(10.0F);
    EXPECT_TRUE(driver.snapshot().figures.empty());
    EXPECT_EQ(driver.actionOf(HERO), ArenaFigureAction::Idle);
}

/**
 * @brief `Idle` boucle : l'image avance puis revient à la première au bout de la bande.
 * \castest{<b>La boucle idle ne s'arrête jamais.</b><br/>
 * \tcat Unitaire · Pilote d'animation de l'arène<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Jouer Idle. 2. Avancer de deux durées d'image. 3. Avancer de deux de plus.<br/>
 * \tattendu L'image progresse (0->1->2), puis revient à 0 après la quatrième image de la bande.
 * }
 */
TEST(ArenaAnimationDriverTest, IdleBoucle) {
    ArenaAnimationDriver driver = driverWithHero();
    driver.play(HERO, "bram", ArenaFigureAction::Idle);
    EXPECT_EQ(driver.snapshot().frameOf(HERO), 0);

    driver.advance(1.0F);  // deux images de 0.5s : 0 -> 1 -> 2
    EXPECT_EQ(driver.snapshot().frameOf(HERO), 2);

    driver.advance(1.0F);  // deux images de plus : 2 -> 3 -> 0 (boucle, 4 images)
    EXPECT_EQ(driver.snapshot().frameOf(HERO), 0);
    EXPECT_EQ(driver.actionOf(HERO), ArenaFigureAction::Idle);
}

/**
 * @brief Une attaque se joue puis revient automatiquement sur `Idle`.
 * \castest{<b>Transition idle -> attack -> idle.</b><br/>
 * \tcat Unitaire · Pilote d'animation de l'arène<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Jouer Idle. 2. Jouer Attack. 3. Avancer jusqu'à la fin de la bande d'attaque.<br/>
 * \tattendu L'action bascule sur `Attack`, joue ses quatre images, puis retombe seule sur `Idle`,
 *           première image.
 * }
 */
TEST(ArenaAnimationDriverTest, TransitionIdleAttackIdle) {
    ArenaAnimationDriver driver = driverWithHero();
    driver.play(HERO, "bram", ArenaFigureAction::Idle);
    driver.advance(0.5F);  // installe l'idle en cours (image 1)

    driver.play(HERO, "bram", ArenaFigureAction::Attack);
    EXPECT_EQ(driver.actionOf(HERO), ArenaFigureAction::Attack);
    EXPECT_EQ(driver.snapshot().frameOf(HERO), 0);

    driver.advance(0.3F);  // trois images de 0.1s : 0 -> 1 -> 2 -> 3 (derniere)
    EXPECT_EQ(driver.actionOf(HERO), ArenaFigureAction::Attack);
    EXPECT_EQ(driver.snapshot().frameOf(HERO), 3);

    driver.advance(0.1F);  // la bande se termine : retour automatique sur Idle
    EXPECT_EQ(driver.actionOf(HERO), ArenaFigureAction::Idle);
    EXPECT_EQ(driver.snapshot().frameOf(HERO), 0);
}

/**
 * @brief Un coup encaissé se rejoue même appelé au milieu d'une attaque en cours.
 * \castest{<b>Hit relance toujours, même coup sur coup.</b><br/>
 * \tcat Unitaire · Pilote d'animation de l'arène<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Jouer Hit, avancer d'une image. 2. Rejouer Hit.<br/>
 * \tattendu La seconde relance repart de l'image 0, sans attendre la fin de la premiere.
 * }
 */
TEST(ArenaAnimationDriverTest, HitRelanceCoupSurCoup) {
    ArenaAnimationDriver driver = driverWithHero();
    driver.play(HERO, "bram", ArenaFigureAction::Hit);
    driver.advance(0.1F);
    EXPECT_EQ(driver.snapshot().frameOf(HERO), 1);

    driver.play(HERO, "bram", ArenaFigureAction::Hit);
    EXPECT_EQ(driver.snapshot().frameOf(HERO), 0);
    EXPECT_EQ(driver.actionOf(HERO), ArenaFigureAction::Hit);
}

/**
 * @brief La mort se joue une fois et y reste : aucun appel ultérieur ne la relance.
 * \castest{<b>Franchissement de l'état mort.</b><br/>
 * \tcat Unitaire · Pilote d'animation de l'arène<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Jouer Death, avancer jusqu'à la derniere image. 2. Rejouer Idle puis Attack.<br/>
 * \tattendu L'animation reste figée sur la dernière image de Death ; les appels suivants n'ont
 *           aucun effet.
 * }
 */
TEST(ArenaAnimationDriverTest, MortResteFigee) {
    ArenaAnimationDriver driver = driverWithHero();
    driver.play(HERO, "bram", ArenaFigureAction::Death);
    driver.advance(1.0F);  // largement plus que les trois images de 0.15s : arrive sur la derniere
    EXPECT_EQ(driver.actionOf(HERO), ArenaFigureAction::Death);
    EXPECT_EQ(driver.snapshot().frameOf(HERO), 3);

    driver.advance(10.0F);  // pas de retour sur Idle apres Death, contrairement a Attack/Hit
    EXPECT_EQ(driver.actionOf(HERO), ArenaFigureAction::Death);
    EXPECT_EQ(driver.snapshot().frameOf(HERO), 3);

    driver.play(HERO, "bram", ArenaFigureAction::Idle);
    driver.play(HERO, "bram", ArenaFigureAction::Attack);
    EXPECT_EQ(driver.actionOf(HERO), ArenaFigureAction::Death);
    EXPECT_EQ(driver.snapshot().frameOf(HERO), 3);
}

/**
 * @brief Une action sans fichier pour la planche retombe silencieusement sur Idle.
 * \castest{<b>Repli sur Idle quand l'action n'existe pas (ex. un ennemi sans attack.png).</b><br/>
 * \tcat Unitaire · Pilote d'animation de l'arène<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Déclarer une figurine avec seulement `idle`. 2. Lui jouer Attack.<br/>
 * \tattendu L'action reste `Idle`, aucun plantage.
 * }
 */
TEST(ArenaAnimationDriverTest, ActionAbsenteReplieSurIdle) {
    ArenaAnimationDriver driver;
    ArenaFigureAnimationSet enemyFigure;
    enemyFigure.idle = loopingClip("idle", 8, 0.2F);
    driver.setFigureAnimations("archer", enemyFigure);

    driver.play(HERO, "archer", ArenaFigureAction::Attack);
    EXPECT_EQ(driver.actionOf(HERO), ArenaFigureAction::Idle);
    EXPECT_EQ(driver.snapshot().frameOf(HERO), 0);
}

/**
 * @brief `remove` retire un combattant du pilotage.
 * \castest{<b>Un combattant sorti de la grille quitte le pilotage.</b><br/>
 * \tcat Unitaire · Pilote d'animation de l'arène<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Jouer Idle. 2. Retirer le combattant.<br/>
 * \tattendu `snapshot()` ne le porte plus.
 * }
 */
TEST(ArenaAnimationDriverTest, RemoveRetireDuPilotage) {
    ArenaAnimationDriver driver = driverWithHero();
    driver.play(HERO, "bram", ArenaFigureAction::Idle);
    ASSERT_FALSE(driver.snapshot().figures.empty());

    driver.remove(HERO);
    EXPECT_TRUE(driver.snapshot().figures.empty());
    EXPECT_EQ(driver.actionOf(HERO), ArenaFigureAction::Idle);
}

/**
 * @brief Les planches d'un kit d'arène se lisent sans erreur, chaque action déclarée existante.
 * \castest{<b>Les `.anim.json` d'un kit d'arène sont valides.</b><br/>
 * \tcat Unitaire · Pilote d'animation de l'arène<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire un héros (cinq actions). 2. Lire un gladiateur au repos seul.<br/>
 * \tattendu Succès, sans erreur ; `bram` porte ses cinq jeux, `archer` seulement `idle`.
 * }
 */
TEST(ArenaAnimationDriverTest, PlanchesDUnKitValides) {
    const std::filesystem::path characters =
        std::filesystem::path(JADG_TEST_DATA_DIR) / "Assets" / "Arena" / "characters" / "champion";
    ASSERT_TRUE(std::filesystem::exists(characters)) << characters.string();
    const hmi::ArenaFigureAnimationLoad hero = hmi::loadArenaFigureAnimations(characters);
    EXPECT_TRUE(hero.errors.empty()) << (hero.errors.empty() ? "" : hero.errors.front());
    EXPECT_NE(hero.clips.idle, nullptr);
    EXPECT_NE(hero.clips.walk, nullptr);
    EXPECT_NE(hero.clips.attack, nullptr);
    EXPECT_NE(hero.clips.hit, nullptr);
    EXPECT_NE(hero.clips.death, nullptr);

    const std::filesystem::path enemy =
        std::filesystem::path(JADG_TEST_DATA_DIR) / "Assets" / "Arena" / "enemies" / "tireur";
    ASSERT_TRUE(std::filesystem::exists(enemy)) << enemy.string();
    const hmi::ArenaFigureAnimationLoad tireur = hmi::loadArenaFigureAnimations(enemy);
    EXPECT_TRUE(tireur.errors.empty()) << (tireur.errors.empty() ? "" : tireur.errors.front());
    EXPECT_NE(tireur.clips.idle, nullptr);
    EXPECT_EQ(tireur.clips.attack, nullptr);
    EXPECT_EQ(tireur.clips.hit, nullptr);
    EXPECT_EQ(tireur.clips.death, nullptr);
}

}  // namespace
