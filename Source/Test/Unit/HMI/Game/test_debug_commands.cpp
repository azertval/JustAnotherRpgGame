// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "HMI/Game/DebugCommands.h"

/**
 * @file Unit/HMI/Game/test_debug_commands.cpp
 * @brief Le catalogue des options du binaire et l'analyse d'une ligne tapée dans la console de
 *        debug (`F9`) : la console ne doit connaître ni plus ni moins d'options que `Main.cpp`.
 */

namespace {

/**
 * @brief Chaque option que `App/Game/Main.cpp` et `App/Common/Bootstrap.cpp` lisent est au
 *        catalogue, et chaque option du catalogue est lue quelque part : une divergence se
 *        verrait ici, pas à l'usage.
 * \castest{<b>Le catalogue et les sources du jeu lisent les memes options.</b><br/>
 * \tcat Unitaire · Console de debug<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Relever les noms d'options que Main.cpp, Bootstrap.cpp et WorldMap.qml lisent.<br/>
 * 2. Comparer a ceux du catalogue.<br/>
 * \tattendu Aucune option lue par le jeu n'est absente du catalogue, et aucune option du catalogue
 * n'est ignoree par le jeu.
 * }
 */
TEST(DebugCommands, LeCatalogueSuitLesSourcesDuJeu) {
    std::string sources;
    for (const char* fichier : {JADG_SOURCE_DIR "/App/Game/Main.cpp",
                                JADG_SOURCE_DIR "/App/Common/Bootstrap.cpp",
                                JADG_SOURCE_DIR "/App/Game/Qml/Screens/WorldMap.qml"}) {
        std::ifstream flux{fichier};
        ASSERT_TRUE(flux.is_open()) << fichier;
        sources.append(std::istreambuf_iterator<char>{flux}, std::istreambuf_iterator<char>{});
    }
    for (const hmi::DebugOption& option : hmi::debugOptionCatalog()) {
        const std::string cite = '"' + std::string{option.name} + '"';
        EXPECT_NE(sources.find(cite), std::string::npos)
            << option.name << " est au catalogue mais aucune source du jeu ne le lit.";
    }
    // L'inverse : toute chaine `"--xxx="` ou `"--xxx"` des sources est une option du catalogue.
    std::size_t position = 0;
    while ((position = sources.find("\"--", position)) != std::string::npos) {
        const std::size_t fin = sources.find('"', position + 1);
        ASSERT_NE(fin, std::string::npos);
        const std::string nom = sources.substr(position + 1, fin - position - 1);
        position = fin + 1;
        // Seuls les NOMS d'option : `"--at="`, `"--crash-test"`. Un message qui commence par un
        // tiret double (`"--at= attend ..."`) n'en est pas un.
        const bool estUnNom = nom.find_first_not_of("abcdefghijklmnopqrstuvwxyz-=") ==
                                  std::string::npos &&
                              nom.find('=') >= nom.size() - 1;
        if (!estUnNom) {
            continue;
        }
        EXPECT_NE(hmi::findDebugOption(nom), nullptr)
            << nom << " est lu par le jeu mais absent du catalogue.";
    }
}

/**
 * @brief Un mot se sépare au premier `=`, le `=` restant au nom : la forme du catalogue.
 * \castest{<b>Un mot se separe au premier = ; une option sans valeur garde son nom entier.</b><br/>
 * \tcat Unitaire · Console de debug<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Separer `--map=capital/arenarea@martpart`, `--flags=a=1,b`, `--crash-test`,
 * `aide`.<br/>
 * \tattendu Nom et valeur attendus pour chacun.
 * }
 */
TEST(DebugCommands, SepareNomEtValeurAuPremierEgal) {
    EXPECT_EQ(hmi::splitDebugArgument("--map=capital/arenarea@martpart"),
              (hmi::DebugArgument{.name = "--map=", .value = "capital/arenarea@martpart"}));
    EXPECT_EQ(hmi::splitDebugArgument("--flags=a=1,b"),
              (hmi::DebugArgument{.name = "--flags=", .value = "a=1,b"}));
    EXPECT_EQ(hmi::splitDebugArgument("--crash-test"),
              (hmi::DebugArgument{.name = "--crash-test", .value = ""}));
    EXPECT_EQ(hmi::splitDebugArgument("aide"), (hmi::DebugArgument{.name = "aide", .value = ""}));
    EXPECT_TRUE(hmi::findDebugOption("--map=")->takesValue());
    EXPECT_FALSE(hmi::findDebugOption("--crash-test")->takesValue());
    EXPECT_EQ(hmi::findDebugOption("--inconnue="), nullptr);
}

/**
 * @brief La ligne se découpe sur les blancs, et les guillemets doubles gardent un chemin avec
 *        espaces en un seul mot.
 * \castest{<b>Les guillemets gardent un chemin avec espaces en un seul mot.</b><br/>
 * \tcat Unitaire · Console de debug<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Decouper une ligne a blancs multiples et un chemin entre guillemets.<br/>
 * \tattendu Trois mots, le chemin entier et sans guillemets.
 * }
 */
TEST(DebugCommands, DecoupeLaLigneEnRespectantLesGuillemets) {
    EXPECT_EQ(hmi::splitCommandLine("  --map=donjon   --screenshot=\"C:\\Mes captures\\a.png\" "
                                    "--at=1,2\t"),
              (std::vector<std::string>{"--map=donjon", "--screenshot=C:\\Mes captures\\a.png",
                                        "--at=1,2"}));
    EXPECT_TRUE(hmi::splitCommandLine("   ").empty());
}

/**
 * @brief `--window-size=` attend `<largeur>x<hauteur>`, deux entiers positifs.
 * \castest{<b>La taille de fenetre se lit en LxH, et rien d'autre.</b><br/>
 * \tcat Unitaire · Console de debug<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Lire `1920x1080`, puis `1920`, `0x10`, `axb`.<br/>
 * \tattendu La premiere donne (1920, 1080) ; les autres rien.
 * }
 */
TEST(DebugCommands, LitLaTailleDeFenetre) {
    ASSERT_TRUE(hmi::parseWindowSize("1920x1080").has_value());
    EXPECT_EQ(*hmi::parseWindowSize("1920x1080"), (std::pair{1920, 1080}));
    EXPECT_FALSE(hmi::parseWindowSize("1920").has_value());
    EXPECT_FALSE(hmi::parseWindowSize("0x10").has_value());
    EXPECT_FALSE(hmi::parseWindowSize("axb").has_value());
}

}  // namespace
