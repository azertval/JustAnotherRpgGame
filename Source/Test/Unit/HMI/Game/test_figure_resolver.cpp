// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_figure_resolver.cpp
 * @brief Tests du résolveur de figurines (`LOT-316`) : la figurine nommée si elle existe, sinon
 *        le mannequin de sa silhouette, sinon l'humanoïde, sinon le marqueur.
 */

#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "HMI/Game/FigureResolver.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

/// Un dossier d'assets d'essai : une figurine orientee, un mannequin quadrupede et un humanoide
/// sans orientation, rien d'autre.
[[nodiscard]] std::filesystem::path assets() {
    const std::filesystem::path racine =
        std::filesystem::temp_directory_path() / "jadg_figure_resolver";
    std::filesystem::remove_all(racine);
    const auto bande = [&](const std::string& relatif) {
        const std::filesystem::path fichier = racine / relatif;
        std::filesystem::create_directories(fichier.parent_path());
        std::ofstream{fichier} << "png";
    };
    bande("Common/Characters/Heroes/brawler/idle-se.png");
    bande("Common/Characters/Placeholders/quadruped/idle.png");
    bande("Common/Characters/Placeholders/humanoid/idle.png");
    return racine;
}

}  // namespace

/**
 * @brief La figurine nommée l'emporte ; à défaut le mannequin de la silhouette ; à défaut
 *        l'humanoïde ; à défaut la figurine telle quelle.
 * \castest{<b>Le resolveur applique la regle de repli en trois temps.</b><br/>
 * \tcat Unitaire · Mannequins<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Resoudre le heros (installe, oriente).<br/>2. Resoudre un loup absent, silhouette
 * quadrupede.<br/>3. Resoudre un garde absent, sans silhouette.<br/>4. Resoudre un oiseau absent,
 * silhouette volante (pas de mannequin volant).<br/>5. Retirer l'humanoide et resoudre le garde
 * a nouveau, resolveur vide.<br/>
 * \tattendu 1 : son dossier, oriente, pas un mannequin. 2 : le quadrupede, mannequin, non
 * oriente. 3 : l'humanoide. 4 : l'humanoide aussi. 5 : le dossier du garde tel quel, ni
 * oriente ni mannequin -- le rendu lui donnera son marqueur.
 * }
 */
TEST(FigureResolverTest, LaRegleDeRepliEnTroisTemps) {
    const std::filesystem::path racine = assets();
    hmi::FigureResolver resolveur{racine};
    const hmi::PlaceAppearance table;

    const hmi::ResolvedFigure& heros =
        resolveur.resolve("Common/Characters/Heroes/brawler", {}, table);
    EXPECT_EQ(heros.directory, "Common/Characters/Heroes/brawler");
    EXPECT_TRUE(heros.oriented);
    EXPECT_FALSE(heros.placeholder);

    const hmi::ResolvedFigure& loup = resolveur.resolve("wolf", "quadruped", table);
    EXPECT_EQ(loup.directory, hmi::placeholderFigureDirectory("quadruped"));
    EXPECT_FALSE(loup.oriented);
    EXPECT_TRUE(loup.placeholder);

    const hmi::ResolvedFigure& garde = resolveur.resolve("guard", {}, table);
    EXPECT_EQ(garde.directory, hmi::placeholderFigureDirectory(hmi::DEFAULT_SILHOUETTE));
    EXPECT_TRUE(garde.placeholder);

    const hmi::ResolvedFigure& oiseau = resolveur.resolve("crow", "flying", table);
    EXPECT_EQ(oiseau.directory, hmi::placeholderFigureDirectory(hmi::DEFAULT_SILHOUETTE))
        << "sans mannequin volant, l'humanoide tient la place";

    std::filesystem::remove_all(racine / "Common/Characters/Placeholders/humanoid");
    resolveur.clear();
    const hmi::ResolvedFigure& sansRien = resolveur.resolve("guard", {}, table);
    EXPECT_EQ(sansRien.directory, table.figureDirectory("guard"));
    EXPECT_FALSE(sansRien.placeholder);
    EXPECT_FALSE(sansRien.oriented);
    std::filesystem::remove_all(racine);
}

/**
 * @brief La réponse se retient : le disque ne se relit pas tant que `clear` n'est pas appelé.
 * \castest{<b>Le resolveur retient ce qu'il a trouve jusqu'a ce qu'on l'oublie.</b><br/>
 * \tcat Unitaire · Mannequins<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Resoudre un garde absent (humanoide).<br/>2. Installer sa bande de repos sur le
 * disque, resoudre a nouveau.<br/>3. Oublier, resoudre a nouveau.<br/>
 * \tattendu 2 : encore le mannequin ; 3 : son propre dossier.
 * }
 */
TEST(FigureResolverTest, LaReponseSeRetientJusquAClear) {
    const std::filesystem::path racine = assets();
    hmi::FigureResolver resolveur{racine};
    const hmi::PlaceAppearance table;
    EXPECT_TRUE(resolveur.resolve("Npc/guard", {}, table).placeholder);

    std::filesystem::create_directories(racine / "Npc/guard");
    std::ofstream{racine / "Npc/guard/idle.png"} << "png";
    EXPECT_TRUE(resolveur.resolve("Npc/guard", {}, table).placeholder);
    resolveur.clear();
    const hmi::ResolvedFigure& propre = resolveur.resolve("Npc/guard", {}, table);
    EXPECT_FALSE(propre.placeholder);
    EXPECT_EQ(propre.directory, "Npc/guard");
    std::filesystem::remove_all(racine);
}
