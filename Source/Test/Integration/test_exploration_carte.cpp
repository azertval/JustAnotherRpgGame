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
#include <memory>
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
    EXPECT_TRUE(snapshot.figures.back().hero);
    // La racine d'essai n'a pas le heros de la demo : son mannequin tient la place (LOT-145).
    EXPECT_EQ(snapshot.figures.back().figure, play.heroResolved().directory);
    EXPECT_EQ(play.heroResolved().directory, hmi::placeholderFigureDirectory("humanoid"));
    EXPECT_TRUE(play.heroResolved().placeholder);
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
 * @brief Un pas du héros ne refait pas la carte : seul un drapeau qui change ce qui s'y dessine la
 *        refait (audit de l'affichage d'un lieu, A3).
 * \castest{<b>Marcher ne recompose pas la carte.</b><br/>
 * \tcat Integration · Exploration · Rendu<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Entrer sur `bourg/place` et prendre la carte en valeurs.<br/>2. Marcher une
 * seconde, par pas de 1/60 s.<br/>3. Poser un drapeau, puis faire un pas.<br/>
 * \tattendu Pendant la marche, aucun pas n'annonce une carte changee et la carte est la meme
 * valeur partagee ; les figurines, elles, changent. Apres le drapeau, le pas annonce la carte
 * changee et une nouvelle valeur est faite.
 * }
 */
TEST(ExplorationCarteIntegration, UnPasNeRefaitPasLaCarte) {
    hmi::WorldPlay play = playFromDisk();
    ASSERT_TRUE(play.enter("bourg/place", {}));
    const std::shared_ptr<const hmi::WorldSceneSnapshot> before = play.scene();
    ASSERT_NE(before, nullptr);

    bool figuresChanged = false;
    for (int step = 0; step < 60; ++step) {
        const hmi::WorldPlayStep result =
            play.step({.move = core::Vector2{1.0F, 0.0F}, .interact = false}, 1.0F / 60.0F);
        EXPECT_FALSE(result.sceneChanged) << "pas " << step;
        figuresChanged = figuresChanged || result.figuresChanged || result.heroMoved;
    }
    EXPECT_TRUE(figuresChanged);
    EXPECT_EQ(play.scene().get(), before.get());

    play.session().flags().set("essai-affichage");
    const hmi::WorldPlayStep result = play.step({.move = {}, .interact = false}, 1.0F / 60.0F);
    EXPECT_TRUE(result.sceneChanged);
    EXPECT_NE(play.scene().get(), before.get());
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
    // Ses PNJ prennent leur figurine sous le niveau qui la range : la zone, le monde -- et c'est
    // ce dossier resolu que les figurines posees portent (LOT-145).
    const std::string anariel = "Regions/central-empire/capital/arenarea/Characters/anariel";
    const std::string garde = "Common/Characters/Peoples/human/guard";
    EXPECT_EQ(play.resolveFigure("anariel", {}).directory, anariel);
    EXPECT_EQ(play.resolveFigure("Peoples/human/guard", {}).directory, garde);
    EXPECT_FALSE(play.resolveFigure("anariel", {}).placeholder);
    for (const std::string& dossier : {anariel, garde}) {
        EXPECT_TRUE(std::ranges::any_of(snapshot.figures, [&dossier](const auto& figure) {
            return figure.figure == dossier;
        })) << dossier;
        EXPECT_EQ(snapshot.figureDirectories.at(dossier), dossier);
    }
    ASSERT_FALSE(snapshot.figures.empty());
    EXPECT_TRUE(snapshot.figures.back().hero);
    EXPECT_EQ(snapshot.figures.back().figure, play.heroResolved().directory);
}
