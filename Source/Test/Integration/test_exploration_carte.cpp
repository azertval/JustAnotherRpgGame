// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_exploration_carte.cpp
 * @brief Test d'intégration : d'un fichier de carte jusqu'à l'instantané que le rendu du lieu
 *        dessine — la chaîne que le jeu et l'essai immédiat de l'éditeur partagent
 *        (`hmi::WorldPlay`), sans GPU.
 *
 * Les cartes sont celles de la racine d'essai de l'éditeur (`Source/Test/Fixtures/GameData`).
 * Jusqu'au `LOT-123` c'étaient les cartes **livrées** : la table rase du `LOT-102` les emporte, et
 * ce test serait tombé avec elles. Ce qu'il éprouve ne change pas — une carte sur disque, son lieu,
 * ses planches, et le héros qui y marche.
 */

#include <algorithm>
#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "Core/World/ExplorationSession.h"
#include "Core/World/WorldTravel.h"
#include "HMI/Game/WorldPlay.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path{JADG_TEST_DATA_DIR};
}

[[nodiscard]] hmi::WorldPlay playFromDisk() {
    return hmi::WorldPlay(core::WorldTravel::directoryLoader(dataRoot() / "Levels"),
                          dataRoot() / "Assets");
}

[[nodiscard]] bool hasDrawnFloor(const hmi::WorldSceneSnapshot& snapshot) {
    return std::ranges::any_of(snapshot.floors,
                               [](const std::string& piece) { return !piece.empty(); });
}

}  // namespace

/**
 * @brief Une carte du disque s'ouvre, nomme son lieu et se compose avec ses planches.
 * \castest{<b>Une carte du disque se charge et se compose.</b><br/>
 * \tcat Integration · Exploration<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Entrer sur la carte `donjon` a son point d'arrivee.<br/>2. Prendre l'instantane
 * de la scene.<br/>
 * \tattendu Le lieu est nomme, la grille a les dimensions de la carte, des pieces de sol sont
 * dessinees et le heros figure parmi les figurines.
 * }
 */
TEST(ExplorationCarteIntegration, UneCarteSeChargeEtSeCompose) {
    hmi::WorldPlay play = playFromDisk();
    ASSERT_TRUE(play.enter("donjon", {}));

    const hmi::WorldSceneSnapshot snapshot = play.snapshot();
    EXPECT_FALSE(snapshot.place.empty());
    EXPECT_EQ(snapshot.columns, play.session().map()->tileMap().width());
    EXPECT_EQ(snapshot.rows, play.session().map()->tileMap().height());
    EXPECT_TRUE(hasDrawnFloor(snapshot));
    ASSERT_FALSE(snapshot.figures.empty());
    EXPECT_EQ(snapshot.figures.back().figure, hmi::WorldPlay::DEFAULT_HERO_FIGURE);
}

/**
 * @brief Le héros marche : un pas d'intention le déplace, et sa bande passe à la marche.
 * \castest{<b>Marcher sur une carte deplace le heros et change sa bande.</b><br/>
 * \tcat Integration · Exploration<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Entrer sur `bourg/place`.<br/>2. Avancer d'une seconde, par pas de 1/60 s, dans une
 * direction libre.<br/>
 * \tattendu Le heros a quitte son point d'arrivee et sa figurine joue la bande « walk ».
 * }
 */
TEST(ExplorationCarteIntegration, LeHerosMarcheSurUneCarte) {
    hmi::WorldPlay play = playFromDisk();
    ASSERT_TRUE(play.enter("bourg/place", {}));
    const core::CellPoint start = play.session().heroPoint();

    // Une direction au moins est libre au point d'arrivée : on essaie les quatre.
    bool moved = false;
    for (const core::Vector2 direction : {core::Vector2{1.0F, 0.0F}, core::Vector2{-1.0F, 0.0F},
                                          core::Vector2{0.0F, 1.0F}, core::Vector2{0.0F, -1.0F}}) {
        for (int step = 0; step < 60; ++step) {
            static_cast<void>(play.step({.move = direction, .interact = false}, 1.0F / 60.0F));
        }
        if (play.session().heroPoint() != start) {
            moved = true;
            break;
        }
    }
    ASSERT_TRUE(moved);
    EXPECT_EQ(play.figures().back().clip, "walk");
}

/**
 * @brief Une carte d'Arenarea qui cite une pièce de chacun des quatre niveaux se joue comme le jeu
 *        la joue (`LOT-124`) : chaque pièce se cherche sous le niveau qui la déclare.
 * \castest{<b>Une carte qui puise dans quatre niveaux se joue.</b><br/>
 * \tcat Integration · Exploration · Arborescence<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Entrer sur `central-empire/capital/arenarea` de la racine LevelTree.<br/>2. Prendre
 * l'instantane de la scene.<br/>
 * \tattendu Le lieu est le chemin de l'Arenarea ; ses pieces se lisent sous la zone, la ville,
 * l'Empire et le monde, et chaque image existe ; le heros figure.
 * }
 */
TEST(ExplorationCarteIntegration, UneCarteQuiPuiseDansQuatreNiveauxSeJoue) {
    const std::filesystem::path tree{JADG_LEVEL_TREE_DIR};
    hmi::WorldPlay play(core::WorldTravel::directoryLoader(tree / "Levels"), tree / "Assets");
    ASSERT_TRUE(play.enter("central-empire/capital/arenarea", {}));

    const hmi::WorldSceneSnapshot snapshot = play.snapshot();
    EXPECT_EQ(snapshot.place, "central-empire/capital/arenarea");
    EXPECT_TRUE(hasDrawnFloor(snapshot));
    for (const char* level : {"Regions/central-empire/capital/arenarea/Scene/",
                              "Regions/central-empire/capital/Common/",
                              "Regions/central-empire/Common/", "Common/Terrain/"}) {
        EXPECT_TRUE(std::ranges::any_of(snapshot.pieceFiles, [level](const auto& entry) {
            return entry.second.starts_with(level);
        })) << level;
    }
    for (const auto& [piece, file] : snapshot.pieceFiles) {
        EXPECT_TRUE(std::filesystem::is_regular_file(tree / "Assets" / file)) << piece;
    }
    // Ses PNJ prennent leur figurine sous le niveau qui la range : la zone, le monde.
    EXPECT_EQ(snapshot.figureDirectories.at("anariel"),
              "Regions/central-empire/capital/arenarea/Characters/anariel");
    EXPECT_EQ(snapshot.figureDirectories.at("Peoples/human/guard"),
              "Common/Characters/Peoples/human/guard");
    ASSERT_FALSE(snapshot.figures.empty());
    EXPECT_EQ(snapshot.figures.back().figure, hmi::WorldPlay::DEFAULT_HERO_FIGURE);
}
