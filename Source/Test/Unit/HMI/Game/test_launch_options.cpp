// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "HMI/Game/LaunchOptions.h"

/**
 * @file Unit/HMI/Game/test_launch_options.cpp
 * @brief La ligne de commande de l'essai complet (`LOT-EDITOR-10`) : ce que l'éditeur écrit, le
 *        jeu doit le relire — et rien de ce qui n'a pas été demandé ne doit paraître.
 */

namespace {

/**
 * @brief Une carte seule : une seule option, et surtout pas d'options vides à analyser.
 * \castest{<b>Une carte seule ne pose aucune option vide.</b><br/>
 * \tcat Unitaire · Essai complet<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Construire la ligne de commande d'un essai qui ne nomme qu'une carte.<br/>
 * \tattendu Une seule option, `--map=` ; ni `--at=`, ni `--flags=`, ni `--levels=` vides.
 * }
 */
TEST(LaunchOptions, CarteSeuleNePosePasDOptionVide) {
    const hmi::GameLaunchOptions options{.mapId = "capital/martpart"};
    EXPECT_EQ(hmi::gameLaunchArguments(options),
              (std::vector<std::string>{"--map=capital/martpart"}));
}

/**
 * @brief Le point d'arrivée s'écrit sur la carte elle-même, comme le jeu le lit déjà (`LOT-96`).
 * \castest{<b>Le point d'arrivee s'ecrit sur l'option de carte.</b><br/>
 * \tcat Unitaire · Essai complet<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Construire la ligne de commande d'une carte et d'un point d'arrivee.<br/>
 * \tattendu L'option vaut `--map=<carte>@<arrivee>`, la forme que le jeu lit deja.
 * }
 */
TEST(LaunchOptions, PointDArriveeSurLOptionDeCarte) {
    const hmi::GameLaunchOptions options{.mapId = "capital/arenarea", .arrival = "martpart"};
    EXPECT_EQ(hmi::gameLaunchArguments(options).front(), "--map=capital/arenarea@martpart");
}

/**
 * @brief Tout ce qui est demandé paraît, et se relit à l'identique.
 * \castest{<b>Ce que l'editeur ecrit, le jeu le relit a l'identique.</b><br/>
 * \tcat Unitaire · Essai complet<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Construire la ligne de commande d'un essai complet (carte, case, drapeaux,
 * dossiers).<br/>2. Relire chaque option par les fonctions d'analyse du jeu.<br/>
 * \tattendu La case, les drapeaux et les dossiers relus valent ceux demandes.
 * }
 */
TEST(LaunchOptions, AllerRetourCompletParLAnalyse) {
    const hmi::GameLaunchOptions options{
        .mapId = "capital/martpart",
        .arrival = {},
        .cell = core::GridPosition{.column = 12, .row = 39},
        .flags = {"quete-du-heraut", "porte-est-ouverte"},
        .levelDirectories = {"C:/tmp/essai", "D:/depot/Source/Elements/Levels"}};
    const std::vector<std::string> arguments = hmi::gameLaunchArguments(options);
    ASSERT_EQ(arguments.size(), 4U);
    EXPECT_EQ(arguments[1], "--at=12,39");
    EXPECT_EQ(arguments[2], "--flags=quete-du-heraut,porte-est-ouverte");

    const std::optional<core::GridPosition> relue = hmi::parseStartCell("12,39");
    ASSERT_TRUE(relue.has_value());
    EXPECT_EQ(relue->column, 12);
    EXPECT_EQ(relue->row, 39);
    EXPECT_EQ(hmi::parseWorldFlags("quete-du-heraut,porte-est-ouverte"), options.flags);
    EXPECT_EQ(hmi::parseLevelDirectories(arguments[3].substr(std::string_view{"--levels="}.size())),
              options.levelDirectories);
}

/**
 * @brief Un chemin de Windows porte un deux-points : c'est bien le point-virgule qui sépare.
 * \castest{<b>Les dossiers de cartes se separent au point-virgule.</b><br/>
 * \tcat Unitaire · Essai complet<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Analyser `--levels=` sur deux chemins Windows portant chacun un deux-points.<br/>
 * \tattendu Deux dossiers, entiers : la lettre de lecteur n'a coupe personne.
 * }
 */
TEST(LaunchOptions, LesDossiersSeSeparentAuPointVirgule) {
    const std::vector<std::filesystem::path> dossiers =
        hmi::parseLevelDirectories("C:/tmp/essai;D:/depot/Levels");
    ASSERT_EQ(dossiers.size(), 2U);
    EXPECT_EQ(dossiers[0], std::filesystem::path{"C:/tmp/essai"});
    EXPECT_EQ(dossiers[1], std::filesystem::path{"D:/depot/Levels"});
}

/**
 * @brief Une case illisible n'est pas une case à zéro : le jeu doit pouvoir la refuser.
 * \castest{<b>Une case illisible est refusee, jamais ramenee a zero.</b><br/>
 * \tcat Unitaire · Essai complet<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Analyser des valeurs de `--at=` incompletes, non numeriques ou negatives.<br/>
 * \tattendu Aucune n'est acceptee : le jeu partira de l'entree, et le dira.
 * }
 */
TEST(LaunchOptions, CaseIllisibleRefusee) {
    EXPECT_FALSE(hmi::parseStartCell("").has_value());
    EXPECT_FALSE(hmi::parseStartCell("12").has_value());
    EXPECT_FALSE(hmi::parseStartCell("12,").has_value());
    EXPECT_FALSE(hmi::parseStartCell("12,39,4").has_value());
    EXPECT_FALSE(hmi::parseStartCell("douze,39").has_value());
    EXPECT_FALSE(hmi::parseStartCell("12,-3").has_value());
    EXPECT_FALSE(hmi::parseStartCell("12,39x").has_value());
}

/**
 * @brief Deux fois le même drapeau ne vaut qu'une, et un séparateur en trop ne pose rien.
 * \castest{<b>Les drapeaux se lisent sans doublon ni valeur vide.</b><br/>
 * \tcat Unitaire · Essai complet<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Analyser `--flags=` avec un separateur en trop et un drapeau repete.<br/>
 * \tattendu Deux drapeaux, dans l'ordre donne ; une liste vide ne pose rien.
 * }
 */
TEST(LaunchOptions, DrapeauxSansDoublonNiVide) {
    EXPECT_EQ(hmi::parseWorldFlags("a,,b,a"), (std::vector<std::string>{"a", "b"}));
    EXPECT_TRUE(hmi::parseWorldFlags("").empty());
}

}  // namespace
