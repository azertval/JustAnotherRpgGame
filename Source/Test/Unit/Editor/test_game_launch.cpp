// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "Editor/Logic/GameLaunch.h"

/**
 * @file Unit/Editor/test_game_launch.cpp
 * @brief Ce que l'essai complet pose sur le disque (`LOT-EDITOR-10`) : les brouillons ouverts,
 *        dans un dossier à eux, et rien de ce qu'un essai précédent y avait laissé.
 */

namespace {

class GameLaunch : public ::testing::Test {
protected:
    std::filesystem::path dir;

    void SetUp() override {
        dir = std::filesystem::temp_directory_path() /
              ("pg_gamelaunch_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
    }
    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(dir, error);
    }
};

/**
 * @brief Une carte d'un sous-dossier s'écrit sous ce sous-dossier : c'est son identifiant qui
 *        donne son chemin, et le jeu la retrouvera par là.
 * \castest{<b>L'identifiant d'une carte donne son chemin dans le dossier d'essai.</b><br/>
 * \tcat Unitaire · Essai complet<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ecrire le brouillon de `bourg/place` dans un dossier d'essai neuf.<br/>
 * \tattendu Le fichier est `bourg/place.json`, et porte le brouillon tel quel.
 * }
 */
TEST_F(GameLaunch, LIdentifiantDonneLeChemin) {
    const std::string erreur =
        hmi::writeDraftMaps(dir, {hmi::DraftMap{.mapId = "bourg/place", .json = "{\"a\":1}"}});
    EXPECT_TRUE(erreur.empty()) << erreur;
    const std::filesystem::path fichier = dir / "bourg" / "place.json";
    ASSERT_TRUE(std::filesystem::exists(fichier));
    std::ifstream lu(fichier, std::ios::binary);
    std::string contenu;
    std::getline(lu, contenu);
    EXPECT_EQ(contenu, "{\"a\":1}");
}

/**
 * @brief Un essai ne laisse pas jouer la carte du précédent : le dossier est vidé d'abord.
 * \castest{<b>Le dossier d'essai ne garde rien de l'essai precedent.</b><br/>
 * \tcat Unitaire · Essai complet<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ecrire deux brouillons, puis un seul.<br/>
 * \tattendu La carte du premier essai a disparu du dossier : elle ne passera plus devant celle
 * du jeu.
 * }
 */
TEST_F(GameLaunch, LeDossierNeGardeRienDeLEssaiPrecedent) {
    ASSERT_TRUE(hmi::writeDraftMaps(dir, {hmi::DraftMap{.mapId = "bourg/place", .json = "{}"},
                                          hmi::DraftMap{.mapId = "donjon", .json = "{}"}})
                    .empty());
    ASSERT_TRUE(std::filesystem::exists(dir / "donjon.json"));

    ASSERT_TRUE(
        hmi::writeDraftMaps(dir, {hmi::DraftMap{.mapId = "bourg/place", .json = "{}"}}).empty());
    EXPECT_TRUE(std::filesystem::exists(dir / "bourg" / "place.json"));
    EXPECT_FALSE(std::filesystem::exists(dir / "donjon.json"));
}

/**
 * @brief Aucune carte ouverte : un dossier vide, pas une erreur.
 * \castest{<b>Sans brouillon, le dossier d'essai est vide et l'ecriture reussit.</b><br/>
 * \tcat Unitaire · Essai complet<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Ecrire une liste de brouillons vide.<br/>
 * \tattendu Aucune erreur, et rien dans le dossier.
 * }
 */
TEST_F(GameLaunch, SansBrouillonLeDossierEstVide) {
    EXPECT_TRUE(hmi::writeDraftMaps(dir, {}).empty());
    EXPECT_FALSE(std::filesystem::exists(dir / "bourg"));
}

/**
 * @brief Le dossier d'essai vit hors du dépôt, dans le temporaire du système.
 * \castest{<b>Le dossier d'essai vit hors du depot.</b><br/>
 * \tcat Unitaire · Essai complet<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander le dossier de l'essai complet.<br/>
 * \tattendu Il porte un nom a lui, sous le dossier temporaire du systeme : un essai n'ecrit
 * jamais dans les sources.
 * }
 */
TEST_F(GameLaunch, LeDossierDEssaiEstTemporaire) {
    const std::filesystem::path essai = hmi::playtestDirectory();
    EXPECT_EQ(essai.filename(), std::filesystem::path{"JustAnotherRpgGame-playtest"});
    // `equivalent` et non une egalite de chemins : le temporaire du systeme se rend avec un
    // separateur final sous Windows, et la comparaison textuelle echouerait pour cela seul.
    EXPECT_TRUE(
        std::filesystem::equivalent(essai.parent_path(), std::filesystem::temp_directory_path()));
}

/**
 * @brief Sans jeu construit à côté, l'éditeur rend un chemin vide — il le dira, il ne lancera pas
 *        un programme qui n'existe pas.
 * \castest{<b>Sans jeu construit a cote, le chemin rendu est vide.</b><br/>
 * \tcat Unitaire · Essai complet<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Chercher le jeu dans un dossier qui ne le contient pas.<br/>
 * \tattendu Un chemin vide : la fenetre le dira plutot que de lancer un programme absent.
 * }
 */
TEST_F(GameLaunch, SansJeuAcoteLeCheminEstVide) {
    std::filesystem::create_directories(dir);
    EXPECT_TRUE(hmi::gameExecutable(dir).empty());
}

/**
 * @brief Le jeu est cherché à côté de l'éditeur : les deux binaires sortent du même `bin/`.
 * \castest{<b>Le jeu est cherche a cote de l'editeur.</b><br/>
 * \tcat Unitaire · Essai complet<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser un exécutable du nom du jeu dans un dossier, puis l'y chercher.<br/>
 * \tattendu Son chemin est rendu tel quel.
 * }
 */
TEST_F(GameLaunch, LeJeuEstCherchePresDeLEditeur) {
    std::filesystem::create_directories(dir);
#ifdef _WIN32
    const std::filesystem::path jeu = dir / "JustAnotherRpgGame.exe";
#else
    const std::filesystem::path jeu = dir / "JustAnotherRpgGame";
#endif
    std::ofstream(jeu, std::ios::binary) << "MZ";
    EXPECT_EQ(hmi::gameExecutable(dir), jeu);
}

}  // namespace
