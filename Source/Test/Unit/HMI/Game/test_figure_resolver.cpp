// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_figure_resolver.cpp
 * @brief Tests du résolveur de figurines (`LOT-145`) : la figurine nommée si elle existe, sinon
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

/**
 * @brief Une figurine en modèle : son dossier porte une fiche qui nomme un `.glb` présent. Elle
 *        passe avant les bandes, pour la figurine nommée comme pour le mannequin.
 * \castest{<b>Le resolveur reconnait un personnage en modele, et son mannequin.</b><br/>
 * \tcat Unitaire · Mannequins · Squelette<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Resoudre le pantin de la carte d'essai, qui a une fiche et un modele.<br/>
 * 2. Resoudre la figurine temoin de la meme racine, qui n'a que des bandes.<br/>3. Dans un dossier
 * d'essai, installer un mannequin humanoide en modele a cote du mannequin en bandes, et resoudre
 * un garde absent.<br/>4. Y poser une fiche dont le modele manque, et resoudre ce personnage.<br/>
 * \tattendu 1 : le dossier du pantin, son modele `Npc/pantin/pantin.glb`, la description de son
 * squelette (l'attaque porte a 0,4 s). 2 : des bandes, sans modele. 3 : le mannequin en modele,
 * avant celui en bandes. 4 : la fiche sans son fichier ne fait pas un modele -- le mannequin.
 * }
 */
TEST(FigureResolverTest, UnPersonnageEnModeleEtSonMannequin) {
    const std::filesystem::path essai = std::filesystem::path(JADG_MESH_FIXTURE_DIR) / "Assets";
    hmi::FigureResolver resolveur{essai};
    const hmi::PlaceAppearance table;

    const hmi::ResolvedFigure& pantin = resolveur.resolve("pantin", {}, table);
    EXPECT_EQ(pantin.directory, "Npc/pantin");
    EXPECT_EQ(pantin.model, "Npc/pantin/pantin.glb");
    EXPECT_TRUE(pantin.oriented) << "un modele garde l'orientation de qui le pose";
    EXPECT_FALSE(pantin.placeholder);
    ASSERT_NE(pantin.skeleton, nullptr);
    const core::SkeletonClip* const attaque = pantin.skeleton->clip("attack");
    ASSERT_NE(attaque, nullptr);
    ASSERT_TRUE(attaque->key.has_value());
    EXPECT_FLOAT_EQ(*attaque->key, 0.4F);

    const hmi::ResolvedFigure& temoin = resolveur.resolve("temoin", {}, table);
    EXPECT_EQ(temoin.directory, "Npc/temoin");
    EXPECT_TRUE(temoin.model.empty());
    EXPECT_EQ(temoin.skeleton, nullptr);

    const std::filesystem::path racine = assets();
    const std::string mannequin = hmi::mannequinFigureDirectory(hmi::DEFAULT_SILHOUETTE);
    std::filesystem::create_directories(racine / mannequin);
    std::ofstream{racine / mannequin / "humanoid.glb"} << "glb";
    std::ofstream{racine / mannequin / "character.json"}
        << R"({"version":1,"model":"humanoid.glb","skeleton":"humanoid"})";
    std::filesystem::create_directories(racine / "Npc/sans-fichier");
    std::ofstream{racine / "Npc/sans-fichier/character.json"}
        << R"({"version":1,"model":"absent.glb","skeleton":"humanoid"})";
    hmi::FigureResolver second{racine};

    const hmi::ResolvedFigure& garde = second.resolve("guard", {}, table);
    EXPECT_EQ(garde.directory, mannequin);
    EXPECT_EQ(garde.model, mannequin + "/humanoid.glb");
    EXPECT_TRUE(garde.placeholder);
    EXPECT_EQ(garde.skeleton, nullptr) << "sans description lisible, pas de durees declarees";

    const hmi::ResolvedFigure& fantome = second.resolve("Npc/sans-fichier", {}, table);
    EXPECT_TRUE(fantome.placeholder);
    EXPECT_EQ(fantome.directory, mannequin);
    std::filesystem::remove_all(racine);
}
