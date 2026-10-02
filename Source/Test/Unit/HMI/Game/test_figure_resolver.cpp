// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_figure_resolver.cpp
 * @brief Tests du résolveur de figurines (`LOT-145`, `LOT-1006`) : le modèle de la figurine nommée
 *        s'il existe, sinon le mannequin de sa silhouette, sinon l'humanoïde, sinon le marqueur.
 */

#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "HMI/Game/FigureResolver.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

/// Installe un personnage en modèle dans @p dossier : sa fiche, et un fichier à la place de son
/// `.glb` (le résolveur ne lit que la fiche et la présence du modèle).
void modele(const std::filesystem::path& racine, const std::string& dossier,
            const std::string& fichier = "modele.glb") {
    std::filesystem::create_directories(racine / dossier);
    std::ofstream{racine / dossier / fichier} << "glb";
    std::ofstream{racine / dossier / "character.json"}
        << R"({"version":1,"model":")" << fichier << R"(","skeleton":"humanoid"})";
}

/// Un dossier d'assets d'essai : un héros en modèle, un mannequin quadrupède et un mannequin
/// humanoïde, rien d'autre.
[[nodiscard]] std::filesystem::path assets() {
    const std::filesystem::path racine =
        std::filesystem::temp_directory_path() / "jadg_figure_resolver";
    std::filesystem::remove_all(racine);
    modele(racine, "Common/Characters/Heroes/brawler", "brawler.glb");
    modele(racine, hmi::mannequinFigureDirectory("quadruped"));
    modele(racine, hmi::mannequinFigureDirectory(hmi::DEFAULT_SILHOUETTE));
    return racine;
}

}  // namespace

/**
 * @brief Le modèle de la figurine nommée l'emporte ; à défaut le mannequin de la silhouette ; à
 *        défaut l'humanoïde ; à défaut la figurine telle quelle.
 * \castest{<b>Le resolveur applique la regle de repli en trois temps.</b><br/>
 * \tcat Unitaire · Mannequins<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Resoudre le heros (installe, en modele).<br/>2. Resoudre un loup absent, silhouette
 * quadrupede.<br/>3. Resoudre un garde absent, sans silhouette.<br/>4. Resoudre un oiseau absent,
 * silhouette volante (pas de mannequin volant).<br/>5. Retirer l'humanoide et resoudre le garde
 * a nouveau, resolveur vide.<br/>
 * \tattendu 1 : son dossier et son modele, pas un mannequin. 2 : le mannequin quadrupede.
 * 3 : l'humanoide. 4 : l'humanoide aussi. 5 : le dossier du garde tel quel, sans modele ni
 * mannequin -- le rendu lui donnera son marqueur.
 * }
 */
TEST(FigureResolverTest, LaRegleDeRepliEnTroisTemps) {
    const std::filesystem::path racine = assets();
    hmi::FigureResolver resolveur{racine};
    const hmi::PlaceAppearance table;

    const hmi::ResolvedFigure& heros =
        resolveur.resolve("Common/Characters/Heroes/brawler", {}, table);
    EXPECT_EQ(heros.directory, "Common/Characters/Heroes/brawler");
    EXPECT_EQ(heros.model, "Common/Characters/Heroes/brawler/brawler.glb");
    EXPECT_FALSE(heros.placeholder);
    EXPECT_EQ(heros.named, heros.directory);

    const hmi::ResolvedFigure& loup = resolveur.resolve("wolf", "quadruped", table);
    EXPECT_EQ(loup.directory, hmi::mannequinFigureDirectory("quadruped"));
    EXPECT_EQ(loup.model, hmi::mannequinFigureDirectory("quadruped") + "/modele.glb");
    EXPECT_TRUE(loup.placeholder);
    EXPECT_EQ(loup.named, table.figureDirectory("wolf"))
        << "le dossier nomme reste connu sous le mannequin : le jeton du loup y est";

    const hmi::ResolvedFigure& garde = resolveur.resolve("guard", {}, table);
    EXPECT_EQ(garde.directory, hmi::mannequinFigureDirectory(hmi::DEFAULT_SILHOUETTE));
    EXPECT_TRUE(garde.placeholder);

    const hmi::ResolvedFigure& oiseau = resolveur.resolve("crow", "flying", table);
    EXPECT_EQ(oiseau.directory, hmi::mannequinFigureDirectory(hmi::DEFAULT_SILHOUETTE))
        << "sans mannequin volant, l'humanoide tient la place";

    std::filesystem::remove_all(racine / hmi::mannequinFigureDirectory(hmi::DEFAULT_SILHOUETTE));
    resolveur.clear();
    const hmi::ResolvedFigure& sansRien = resolveur.resolve("guard", {}, table);
    EXPECT_EQ(sansRien.directory, table.figureDirectory("guard"));
    EXPECT_FALSE(sansRien.placeholder);
    EXPECT_TRUE(sansRien.model.empty());
    std::filesystem::remove_all(racine);
}

/**
 * @brief La réponse se retient : le disque ne se relit pas tant que `clear` n'est pas appelé.
 * \castest{<b>Le resolveur retient ce qu'il a trouve jusqu'a ce qu'on l'oublie.</b><br/>
 * \tcat Unitaire · Mannequins<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Resoudre un garde absent (humanoide).<br/>2. Installer son modele sur le disque,
 * resoudre a nouveau.<br/>3. Oublier, resoudre a nouveau.<br/>
 * \tattendu 2 : encore le mannequin ; 3 : son propre dossier, son propre modele.
 * }
 */
TEST(FigureResolverTest, LaReponseSeRetientJusquAClear) {
    const std::filesystem::path racine = assets();
    hmi::FigureResolver resolveur{racine};
    const hmi::PlaceAppearance table;
    EXPECT_TRUE(resolveur.resolve("Npc/guard", {}, table).placeholder);

    modele(racine, "Npc/guard", "guard.glb");
    EXPECT_TRUE(resolveur.resolve("Npc/guard", {}, table).placeholder);
    resolveur.clear();
    const hmi::ResolvedFigure& propre = resolveur.resolve("Npc/guard", {}, table);
    EXPECT_FALSE(propre.placeholder);
    EXPECT_EQ(propre.directory, "Npc/guard");
    EXPECT_EQ(propre.model, "Npc/guard/guard.glb");
    std::filesystem::remove_all(racine);
}

/**
 * @brief Une figurine n'existe que par une fiche lisible qui nomme un `.glb` présent ; sa fiche
 *        dit aussi son squelette, dont la description donne les durées de ses clips.
 * \castest{<b>Une figurine est un modele : une fiche, un fichier, un squelette.</b><br/>
 * \tcat Unitaire · Mannequins · Squelette<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Resoudre le pantin de la carte d'essai, qui a une fiche, un modele et un squelette
 * decrit.<br/>2. Dans un dossier d'essai, resoudre un personnage dont la fiche nomme un fichier
 * absent, un personnage qui n'a que des images, et un personnage dont la fiche est illisible.<br/>
 * \tattendu 1 : le dossier du pantin, son modele `Npc/pantin/pantin.glb`, la description de son
 * squelette (l'attaque porte a 0,4 s). 2 : aucun des trois n'est une figurine -- le mannequin
 * tient la place, sans description de squelette puisque le dossier d'essai n'en a pas.
 * }
 */
TEST(FigureResolverTest, UneFigurineEstUnModele) {
    const std::filesystem::path essai = std::filesystem::path(JADG_MESH_FIXTURE_DIR) / "Assets";
    hmi::FigureResolver resolveur{essai};
    const hmi::PlaceAppearance table;

    const hmi::ResolvedFigure& pantin = resolveur.resolve("pantin", {}, table);
    EXPECT_EQ(pantin.directory, "Npc/pantin");
    EXPECT_EQ(pantin.model, "Npc/pantin/pantin.glb");
    EXPECT_FALSE(pantin.placeholder);
    ASSERT_NE(pantin.skeleton, nullptr);
    const core::SkeletonClip* const attaque = pantin.skeleton->clip("attack");
    ASSERT_NE(attaque, nullptr);
    ASSERT_TRUE(attaque->key.has_value());
    EXPECT_FLOAT_EQ(*attaque->key, 0.4F);

    const std::filesystem::path racine = assets();
    const std::string mannequin = hmi::mannequinFigureDirectory(hmi::DEFAULT_SILHOUETTE);
    std::filesystem::create_directories(racine / "Npc/sans-fichier");
    std::ofstream{racine / "Npc/sans-fichier/character.json"}
        << R"({"version":1,"model":"absent.glb","skeleton":"humanoid"})";
    std::filesystem::create_directories(racine / "Npc/en-images");
    std::ofstream{racine / "Npc/en-images/idle.png"} << "png";
    std::ofstream{racine / "Npc/en-images/idle-se.png"} << "png";
    std::filesystem::create_directories(racine / "Npc/illisible");
    std::ofstream{racine / "Npc/illisible/illisible.glb"} << "glb";
    std::ofstream{racine / "Npc/illisible/character.json"} << "pas du json";
    hmi::FigureResolver second{racine};

    for (const char* const figure : {"Npc/sans-fichier", "Npc/en-images", "Npc/illisible"}) {
        const hmi::ResolvedFigure& resolue = second.resolve(figure, {}, table);
        EXPECT_TRUE(resolue.placeholder) << figure;
        EXPECT_EQ(resolue.directory, mannequin) << figure;
        EXPECT_EQ(resolue.model, mannequin + "/modele.glb") << figure;
        EXPECT_EQ(resolue.skeleton, nullptr) << figure;
    }
    std::filesystem::remove_all(racine);
}
