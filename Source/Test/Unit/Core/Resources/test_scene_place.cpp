// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_scene_place.cpp
 * @brief Les niveaux d'un lieu et le catalogue résolu (`LOT-124`) : une carte puise dans son lieu
 *        **et** dans ses niveaux communs, du plus propre au monde.
 *
 * La racine d'essai `Fixtures/LevelTree` porte une pièce à chacun des niveaux de l'Arenarea, et une
 * fontaine de zone qui masque celle de la ville.
 */

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Resources/ScenePieceManifest.h"
#include "Core/Resources/ScenePlace.h"

namespace {

constexpr const char* ARENAREA = "central-empire/capital/arenarea";

[[nodiscard]] std::filesystem::path treeAssets() {
    return std::filesystem::path(JADG_TEST_FIXTURES_DIR) / "LevelTree" / "Assets";
}

[[nodiscard]] std::vector<std::string> directoriesOf(const std::vector<core::SceneLevel>& levels) {
    std::vector<std::string> directories;
    for (const core::SceneLevel& level : levels) {
        directories.push_back(level.directory);
    }
    return directories;
}

}  // namespace

/**
 * @brief Les niveaux d'une sous-zone, du plus propre au monde.
 * \castest{<b>Les niveaux d'un lieu vont de la sous-zone au monde.</b><br/>
 * \tcat Unitaire · Assets · Arborescence<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Demander les niveaux candidats de
 * `central-empire/capital/arenarea/arena-of-fate`.<br/>
 * \tattendu La sous-zone, la zone, la ville (propre puis commun), la region, puis les trois
 * dossiers du monde ; chaque niveau porte son nom lisible et le lieu qu'il couvre.
 * }
 */
TEST(ScenePlaceTest, LesNiveauxDUneSousZoneVontJusquAuMonde) {
    const std::vector<core::SceneLevel> levels =
        core::sceneLevelCandidates("central-empire/capital/arenarea/arena-of-fate");
    EXPECT_EQ(directoriesOf(levels),
              (std::vector<std::string>{
                  "Regions/central-empire/capital/arenarea/arena-of-fate/Scene",
                  "Regions/central-empire/capital/arenarea/arena-of-fate/Common/Scene",
                  "Regions/central-empire/capital/arenarea/Scene",
                  "Regions/central-empire/capital/arenarea/Common/Scene",
                  "Regions/central-empire/capital/Scene",
                  "Regions/central-empire/capital/Common/Scene",
                  "Regions/central-empire/Common/Scene",
                  "Common/Terrain",
                  "Common/Nature",
                  "Common/Props",
              }));
    EXPECT_EQ(levels.front().label, "Arena of Fate");
    EXPECT_EQ(levels.front().place, "central-empire/capital/arenarea/arena-of-fate");
    EXPECT_EQ(levels[5].label, "Capital");
    EXPECT_EQ(levels[5].place, "central-empire/capital");
    EXPECT_EQ(levels[6].label, "Central Empire");
    EXPECT_EQ(levels.back().label, "World");
    EXPECT_TRUE(levels.back().place.empty());
}

/**
 * @brief Un lieu à plat reste un dossier d'essai, qui remonte au monde ; un lieu mal formé n'a
 * aucun niveau.
 * \castest{<b>Un lieu a plat et un lieu mal forme.</b><br/>
 * \tcat Unitaire · Assets · Arborescence<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander les niveaux de `bourg`, puis de lieux vides ou contenant `..`.<br/>
 * \tattendu `Scene/bourg` puis le monde ; aucun niveau pour un lieu mal forme, et aucun chemin de
 * repli.
 * }
 */
TEST(ScenePlaceTest, UnLieuAPlatEtUnLieuMalForme) {
    const std::vector<core::SceneLevel> levels = core::sceneLevelCandidates("bourg");
    ASSERT_EQ(levels.size(), 4U);
    EXPECT_EQ(levels.front().directory, "Scene/bourg");
    EXPECT_EQ(levels.front().label, "Bourg");
    EXPECT_EQ(core::fallbackScenePiecePath("bourg", "street"), "Scene/bourg/street.png");
    EXPECT_EQ(core::fallbackScenePiecePath(ARENAREA, "stall"),
              "Regions/central-empire/capital/arenarea/Scene/stall.png");
    for (const char* bad : {"", "a//b", "../a", "a/./b", "a\\b", "a/"}) {
        EXPECT_FALSE(core::isValidScenePlace(bad)) << bad;
        EXPECT_TRUE(core::sceneLevelCandidates(bad).empty()) << bad;
        EXPECT_TRUE(core::fallbackScenePiecePath(bad, "x").empty()) << bad;
    }
}

/**
 * @brief La filiation des lieux : ce qu'un préfabriqué ou un modèle d'un niveau peut servir.
 * \castest{<b>Un lieu descend de ses prefixes et du monde.</b><br/>
 * \tcat Unitaire · Assets · Arborescence<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Comparer des lieux a leurs prefixes, et lister la filiation d'une zone.<br/>
 * \tattendu L'Arenarea descend de la Capitale, de l'Empire et du monde, pas du Martpart ni d'un
 * prefixe coupe au milieu d'un segment.
 * }
 */
TEST(ScenePlaceTest, UnLieuDescendDeSesPrefixes) {
    EXPECT_TRUE(core::scenePlaceDescendsFrom(ARENAREA, "central-empire/capital"));
    EXPECT_TRUE(core::scenePlaceDescendsFrom(ARENAREA, ARENAREA));
    EXPECT_TRUE(core::scenePlaceDescendsFrom(ARENAREA, ""));
    EXPECT_FALSE(core::scenePlaceDescendsFrom(ARENAREA, "central-empire/capital/martpart"));
    EXPECT_FALSE(core::scenePlaceDescendsFrom(ARENAREA, "central-empire/cap"));
    EXPECT_EQ(core::scenePlaceAncestry(ARENAREA),
              (std::vector<std::string>{ARENAREA, "central-empire/capital", "central-empire", ""}));
    EXPECT_EQ(core::scenePlaceAncestry("bourg"), (std::vector<std::string>{"bourg", ""}));
}

/**
 * @brief Le catalogue résolu de l'Arenarea : une pièce de chaque niveau, chacune sous son dossier
 *        d'origine, et la fontaine de la ville masquée par celle de la zone.
 * \castest{<b>Le catalogue d'un lieu empile ses niveaux.</b><br/>
 * \tcat Unitaire · Assets · Arborescence<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Resoudre le lieu `central-empire/capital/arenarea` de la racine LevelTree.<br/>
 * \tattendu Le gazon vient du monde, le pavage de la ville, la banniere de l'Empire, les paves et
 * la fontaine de la zone ; chaque piece donne son fichier relatif a Assets/ ; la fontaine de la
 * ville est gardee comme masquee par l'Arenarea ; le losange est celui du niveau le plus propre.
 * }
 */
TEST(ScenePlaceTest, LeCatalogueDUnLieuEmpileSesNiveaux) {
    const core::ScenePieceManifestResult read =
        core::ScenePieceManifest::resolve(treeAssets(), ARENAREA);
    ASSERT_TRUE(read.ok()) << read.message;
    const core::ScenePieceManifest& catalog = read.manifest;
    EXPECT_EQ(catalog.place(), ARENAREA);
    EXPECT_EQ(catalog.tileWidth(), 68);

    struct Expected {
        const char* name;
        const char* level;
        const char* path;
    };
    for (const Expected& expected : {
             Expected{"grass", "World", "Common/Terrain/grass.png"},
             Expected{"crate", "World", "Common/Props/crate.png"},
             Expected{"banner-lion", "Central Empire",
                      "Regions/central-empire/Common/Scene/props/banner-lion.png"},
             Expected{"paving", "Capital",
                      "Regions/central-empire/capital/Common/Scene/floors/paving.png"},
             Expected{"cobbles", "Arenarea",
                      "Regions/central-empire/capital/arenarea/Scene/cobbles.png"},
             Expected{"fountain", "Arenarea",
                      "Regions/central-empire/capital/arenarea/Scene/fountain.png"},
         }) {
        const core::ScenePiece* piece = catalog.find(expected.name);
        ASSERT_NE(piece, nullptr) << expected.name;
        EXPECT_EQ(piece->level, expected.level) << expected.name;
        EXPECT_EQ(piece->path(), expected.path) << expected.name;
        EXPECT_TRUE(std::filesystem::is_regular_file(treeAssets() / piece->path()))
            << expected.name;
    }
    // La sous-zone n'est pas un niveau de la zone : son sable n'y paraît pas.
    EXPECT_EQ(catalog.find("sand"), nullptr);

    ASSERT_EQ(catalog.masked().size(), 1U);
    EXPECT_EQ(catalog.masked().front().piece.name, "fountain");
    EXPECT_EQ(catalog.masked().front().piece.level, "Capital");
    EXPECT_EQ(catalog.masked().front().by, "Arenarea");
    EXPECT_EQ(
        std::ranges::count(catalog.pieces(), std::string{"fountain"}, &core::ScenePiece::name), 1);

    std::vector<std::string> levels;
    for (const core::SceneLevel& level : catalog.levels()) {
        levels.push_back(level.label);
    }
    EXPECT_EQ(levels, (std::vector<std::string>{"Arenarea", "Capital", "Central Empire", "World",
                                                "World"}));
}

/**
 * @brief Une zone sans pièce propre tient tout de la ville : le Martpart pose la fontaine commune.
 * \castest{<b>Une zone sans piece propre puise dans la ville.</b><br/>
 * \tcat Unitaire · Assets · Arborescence<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Resoudre `central-empire/capital/martpart`.<br/>
 * \tattendu La fontaine vient de la ville, rien n'est masque.
 * }
 */
TEST(ScenePlaceTest, UneZoneSansPieceProprePuiseDansLaVille) {
    const core::ScenePieceManifestResult read =
        core::ScenePieceManifest::resolve(treeAssets(), "central-empire/capital/martpart");
    ASSERT_TRUE(read.ok()) << read.message;
    const core::ScenePiece* fountain = read.manifest.find("fountain");
    ASSERT_NE(fountain, nullptr);
    EXPECT_EQ(fountain->directory, "Regions/central-empire/capital/Common/Scene");
    EXPECT_TRUE(read.manifest.masked().empty());
}

/**
 * @brief L'arbre des lieux : les zones et sous-zones qui ont un dossier de pièces propre.
 * \castest{<b>L'arbre des lieux liste les zones et sous-zones.</b><br/>
 * \tcat Unitaire · Assets · Arborescence<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lister les lieux de la racine LevelTree.<br/>
 * \tattendu L'Arenarea, sa sous-zone et le Martpart, tries ; ni la ville, ni la region, ni un
 * dossier Characters.
 * }
 */
TEST(ScenePlaceTest, LArbreDesLieuxListeLesZones) {
    EXPECT_EQ(core::scenePlaces(treeAssets()),
              (std::vector<std::string>{ARENAREA, "central-empire/capital/arenarea/arena-of-fate",
                                        "central-empire/capital/martpart"}));
}

/**
 * @brief Un manifeste illisible à un niveau commun fait échouer la résolution, et le message nomme
 *        son dossier ; un lieu sans aucun manifeste n'est pas trouvé.
 * \castest{<b>Un niveau commun illisible fait echouer la resolution.</b><br/>
 * \tcat Unitaire · Assets · Arborescence<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ecrire une zone lisible sous une ville dont le manifeste est casse, et la
 * resoudre.<br/>
 *          2. Resoudre un lieu qui n'a aucun manifeste.<br/>
 * \tattendu Un echec qui nomme `Regions/r/v/Common/Scene/manifest.json` ; puis `FileNotFound`.
 * }
 */
TEST(ScenePlaceTest, UnNiveauCommunIllisibleFaitEchouerLaResolution) {
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "jadg-test-scene-place";
    std::filesystem::remove_all(root);
    const auto write = [](const std::filesystem::path& file, const std::string& text) {
        std::filesystem::create_directories(file.parent_path());
        std::ofstream(file, std::ios::binary) << text;
    };
    write(root / "Regions/r/v/z/Scene/manifest.json",
          R"({"version": 1, "disposition": "z", "textures": {}})");
    write(root / "Regions/r/v/Common/Scene/manifest.json", "{ pas du json");

    const core::ScenePieceManifestResult broken = core::ScenePieceManifest::resolve(root, "r/v/z");
    EXPECT_FALSE(broken.ok());
    EXPECT_NE(broken.message.find("Regions/r/v/Common/Scene/manifest.json"), std::string::npos)
        << broken.message;

    const core::ScenePieceManifestResult none = core::ScenePieceManifest::resolve(root, "r/w");
    EXPECT_EQ(none.error, core::ScenePieceManifestError::FileNotFound);
    std::filesystem::remove_all(root);
}

/**
 * @brief Les figurines se cherchent comme les pièces : du `Characters/` de la zone au monde, le
 * plus propre gagnant (`LOT-124`).
 * \castest{<b>Les figurines d'un lieu viennent de ses niveaux.</b><br/>
 * \tcat Unitaire · Assets · Arborescence<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lister les niveaux de figurines de l'Arenarea.<br/>
 *          2. Resoudre ses figurines, puis celles du Martpart.<br/>
 *          3. Demander le dossier d'une figurine inconnue, d'un chemin, d'un slug seul.<br/>
 * \tattendu La zone, la ville, la region, le monde ; le PNJ nomme et le citadin viennent de la zone
 * (le citadin masque celui de la ville), le garde du monde ; au Martpart, le citadin est celui de
 * la ville et le PNJ nomme de l'Arenarea est inconnu ; un chemin reste un chemin, un slug seul se
 * cherche dans l'atelier a plat.
 * }
 */
TEST(ScenePlaceTest, LesFiguresDUnLieuViennentDeSesNiveaux) {
    std::vector<std::string> directories;
    for (const core::SceneLevel& level : core::characterLevelCandidates(ARENAREA)) {
        directories.push_back(level.directory);
    }
    EXPECT_EQ(directories, (std::vector<std::string>{
                               "Regions/central-empire/capital/arenarea/Characters",
                               "Regions/central-empire/capital/arenarea/Common/Characters",
                               "Regions/central-empire/capital/Characters",
                               "Regions/central-empire/capital/Common/Characters",
                               "Regions/central-empire/Common/Characters",
                               "Common/Characters",
                           }));
    EXPECT_EQ(core::characterLevelCandidates("bourg").size(), 1U);

    const core::FigureDirectories arena = core::resolveFigures(treeAssets(), ARENAREA);
    EXPECT_EQ(core::figureDirectory(arena, "anariel"),
              "Regions/central-empire/capital/arenarea/Characters/anariel");
    EXPECT_EQ(core::figureDirectory(arena, "citizen"),
              "Regions/central-empire/capital/arenarea/Characters/citizen");
    EXPECT_EQ(core::figureDirectory(arena, "Peoples/human/guard"),
              "Common/Characters/Peoples/human/guard");
    for (const auto& [slug, directory] : arena) {
        EXPECT_TRUE(std::filesystem::is_regular_file(treeAssets() / directory / "idle.png"))
            << slug;
    }

    const core::FigureDirectories mart =
        core::resolveFigures(treeAssets(), "central-empire/capital/martpart");
    EXPECT_EQ(core::figureDirectory(mart, "citizen"),
              "Regions/central-empire/capital/Common/Characters/citizen");
    EXPECT_FALSE(mart.contains("anariel"));

    EXPECT_EQ(core::figureDirectory({}, "Monsters/lion"), "Monsters/lion");
    EXPECT_EQ(core::figureDirectory({}, "figurant"), "Npc/figurant");
    EXPECT_EQ(core::figureDirectory({}, ""), "");
}
