// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_level_tree.cpp
 * @brief L'éditeur et l'arborescence par niveaux (`LOT-124`) : une carte puise dans son lieu **et**
 *        dans ses niveaux communs, et l'éditeur le montre, le contrôle et le réécrit.
 *
 * La racine d'essai `Fixtures/LevelTree` (voir son README) porte une Arenarea qui cite une pièce de
 * chacun des quatre niveaux — la zone, la Capitale, l'Empire, le monde —, dont une fontaine de zone
 * qui masque celle de la ville, et un Martpart qui pose la fontaine de la ville.
 */

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "Core/Levels/LevelLoader.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "Editor/Logic/EntityReferences.h"
#include "Editor/Logic/GestureScript.h"
#include "Editor/Logic/LevelFileOperations.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/MapRefactor.h"
#include "Editor/Logic/PieceCatalog.h"
#include "Editor/Logic/Stamps.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

constexpr const char* ARENAREA = "central-empire/capital/arenarea";
constexpr const char* MARTPART = "central-empire/capital/martpart";
constexpr const char* ZONE_LEVEL = "Regions/central-empire/capital/arenarea/Scene";
constexpr const char* CITY_LEVEL = "Regions/central-empire/capital/Common/Scene";

[[nodiscard]] std::filesystem::path tree() {
    return std::filesystem::path(JADG_TEST_FIXTURES_DIR) / "LevelTree";
}

[[nodiscard]] std::string lire(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

[[nodiscard]] nlohmann::ordered_json lireJson(const std::filesystem::path& file) {
    return nlohmann::ordered_json::parse(lire(file));
}

void ecrireJson(const std::filesystem::path& file, const nlohmann::ordered_json& json) {
    std::filesystem::create_directories(file.parent_path());
    std::ofstream(file, std::ios::binary) << json.dump(2) << '\n';
}

/// Les constats d'un contrôle, un par ligne : ce qu'un échec montre.
[[nodiscard]] std::string constats(const hmi::MapCheckReport& report) {
    std::string text;
    for (const hmi::MapCheckFinding& finding : report.findings) {
        text += hmi::formatFinding(finding) + "\n";
    }
    return text;
}

// Une copie de la racine LevelTree par test : la promotion d'une pièce récrit manifestes et cartes.
class Arborescence : public ::testing::Test {
protected:
    std::filesystem::path racine;

    void SetUp() override {
        racine = std::filesystem::temp_directory_path() /
                 ("jadg-level-tree-" + std::to_string(std::rand()));
        std::filesystem::remove_all(racine);
        std::filesystem::copy(tree(), racine, std::filesystem::copy_options::recursive);
    }
    void TearDown() override {
        std::error_code ignore;
        std::filesystem::remove_all(racine, ignore);
    }

    [[nodiscard]] std::filesystem::path carte(std::string_view id) const {
        return racine / "Levels" / (std::string{id} + ".json");
    }
    [[nodiscard]] std::filesystem::path niveau(std::string_view directory) const {
        return racine / "Assets" / std::filesystem::path(std::string{directory});
    }

    /// Déplace l'entrée @p key du manifeste de @p from vers celui de @p to, sous @p newKey, et son
    /// image avec elle.
    void deplacer(std::string_view from, std::string_view key, std::string_view to,
                  std::string_view newKey, bool garder = false) const {
        nlohmann::ordered_json source = lireJson(niveau(from) / "manifest.json");
        nlohmann::ordered_json target = lireJson(niveau(to) / "manifest.json");
        nlohmann::ordered_json entry = source["textures"][std::string{key}];
        const std::string file = entry["file"].get<std::string>();
        std::filesystem::create_directories((niveau(to) / file).parent_path());
        std::filesystem::copy_file(niveau(from) / file, niveau(to) / file,
                                   std::filesystem::copy_options::overwrite_existing);
        target["textures"][std::string{newKey}] = entry;
        if (!garder) {
            source["textures"].erase(std::string{key});
            std::filesystem::remove(niveau(from) / file);
        }
        ecrireJson(niveau(from) / "manifest.json", source);
        ecrireJson(niveau(to) / "manifest.json", target);
    }
};

}  // namespace

/**
 * @brief La racine d'essai passe le contrôle : chaque pièce de ses deux cartes se trouve à l'un des
 *        niveaux de leur lieu, et la collision égale la déduction faite de ces pièces.
 * \castest{<b>Une carte qui cite quatre niveaux passe --check.</b><br/>
 * \tcat Unitaire · Editeur · Arborescence<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Controler toutes les cartes de la racine LevelTree.<br/>
 * \tattendu Deux cartes, aucun constat : ni piece absente, ni collision ecartee de la deduction.
 * }
 */
TEST(LevelTreeTest, UneCarteQuiCiteQuatreNiveauxPasseLeControle) {
    const hmi::MapCheckReport report = hmi::checkAllMaps(tree());
    EXPECT_EQ(report.maps, 2U);
    EXPECT_TRUE(report.findings.empty()) << constats(report);
}

/**
 * @brief Les tables d'apparence s'empilent comme les pièces : pour un type, la plus propre gagne.
 * \castest{<b>La table d'un lieu empile celles de ses niveaux.</b><br/>
 * \tcat Unitaire · Editeur · Arborescence<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire la table de l'Arenarea.<br/>
 * \tattendu Le dallage vient de la zone, le pave de la ville (qui masque le repli du monde), le
 * gazon du monde, le mur de la ville ; chaque piece donne son fichier relatif a Assets/.
 * }
 */
TEST(LevelTreeTest, LaTableDUnLieuEmpileSesNiveaux) {
    const hmi::PlaceAppearanceResult read =
        hmi::PlaceAppearance::loadForPlace(tree() / "Assets", ARENAREA);
    ASSERT_TRUE(read.ok()) << read.message;
    const hmi::PlaceAppearance& table = read.appearance;
    const core::GridPosition cell{};
    EXPECT_EQ(table.floorPiece(core::TileType::Flagstone, cell), "cobbles");
    EXPECT_EQ(table.floorPiece(core::TileType::Pavement, cell), "paving");
    EXPECT_EQ(table.floorPiece(core::TileType::Grass, cell), "grass");
    EXPECT_EQ(table.reliefPiece(core::TileType::Wall, cell), "wall-arcade");
    EXPECT_EQ(table.pieceFile("paving"),
              "Regions/central-empire/capital/Common/Scene/floors/paving.png");
    EXPECT_EQ(table.pieceFile("grass"), "Common/Terrain/grass.png");
    EXPECT_EQ(table.pieceFile("fountain"),
              "Regions/central-empire/capital/arenarea/Scene/fountain.png");
    ASSERT_NE(table.pieceManifest(), nullptr);
    EXPECT_EQ(table.pieceManifest()->masked().size(), 1U);
}

/**
 * @brief La composition de l'Arenarea demande les images des quatre niveaux, et toutes existent.
 * \castest{<b>La scene d'une carte cherche chaque piece sous son niveau.</b><br/>
 * \tcat Unitaire · Editeur · Arborescence<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Tirer l'instantane de l'Arenarea avec sa table, puis ses chemins d'images.<br/>
 * \tattendu Un chemin sous la zone, la ville, l'Empire et le monde ; chaque image demandee existe ;
 * la fontaine est celle de la zone.
 * }
 */
TEST(LevelTreeTest, LaSceneChercheChaquePieceSousSonNiveau) {
    const core::LevelLoadResult map = core::LevelLoader::loadFromFile(
        tree() / "Levels" / "central-empire" / "capital" / "arenarea.json");
    ASSERT_TRUE(map.ok()) << map.error;
    const hmi::PlaceAppearanceResult table =
        hmi::PlaceAppearance::loadForPlace(tree() / "Assets", ARENAREA);
    ASSERT_TRUE(table.ok()) << table.message;
    const hmi::WorldSceneSnapshot snapshot =
        hmi::snapshotWorldScene(*map.level, table.appearance, {});
    const std::vector<std::string> paths = hmi::worldTexturePaths(snapshot);
    for (const char* prefix : {ZONE_LEVEL, CITY_LEVEL, "Regions/central-empire/Common/Scene",
                               "Common/Terrain", "Common/Props"}) {
        EXPECT_TRUE(std::ranges::any_of(paths, [prefix](const std::string& path) {
            return path.starts_with(prefix);
        })) << prefix;
    }
    for (const std::string& path : paths) {
        EXPECT_TRUE(std::filesystem::is_regular_file(tree() / "Assets" / path)) << path;
    }
    EXPECT_NE(std::ranges::find(paths, std::string{ZONE_LEVEL} + "/fountain.png"), paths.end());
    EXPECT_EQ(std::ranges::find(paths, std::string{CITY_LEVEL} + "/props/fountain.png"),
              paths.end());
}

/**
 * @brief Un scénario `--apply` pose des pièces des quatre niveaux, et rend l'Arenarea octet pour
 *        octet.
 * \castest{<b>--apply pose une piece commune.</b><br/>
 * \tcat Unitaire · Editeur · Sans fenetre · Arborescence<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Rejouer `Fixtures/Gestures/niveaux.json` sur l'Arenarea de LevelTree : effacer le sol
 * et le relief, puis poser le pavage de la ville, les paves de la zone, le gazon du monde, la
 * banniere de l'Empire, l'arcade, la fontaine, l'etal et la caisse.<br/>
 * \tattendu Dix gestes, dix pas ; la carte revient telle qu'elle est sur disque : type, piece et
 * collision de chaque case viennent du bon niveau.
 * }
 */
TEST(LevelTreeTest, UnScenarioPoseDesPiecesCommunes) {
    std::filesystem::path mapFile;
    const hmi::GestureFileResult rendu = hmi::applyGestureFile(
        std::filesystem::path(JADG_TEST_FIXTURES_DIR) / "Gestures" / "niveaux.json", {}, tree(),
        mapFile);
    ASSERT_TRUE(rendu.script.ok()) << rendu.script.error;
    EXPECT_EQ(rendu.mapId, ARENAREA);
    EXPECT_EQ(rendu.script.gestures, 10U);
    EXPECT_EQ(rendu.script.steps, 10U);
    EXPECT_EQ(rendu.mapText, lire(mapFile));
}

/**
 * @brief La palette groupe les pièces par niveau et dit ce qui masque quoi ; la recherche traverse
 *        tous les niveaux.
 * \castest{<b>La palette groupe les pieces par niveau.</b><br/>
 * \tcat Unitaire · Editeur · Arborescence<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Faire le catalogue de l'Arenarea.<br/>
 *          2. Le filtrer sur « fountain ».<br/>
 * \tattendu Les niveaux Arenarea, Capital, Central Empire, World, dans cet ordre ; la fontaine de
 * la zone masque la Capitale, celle de la ville est montree masquee par l'Arenarea ; la recherche
 * rend les deux, chacune sous son niveau.
 * }
 */
TEST(LevelTreeTest, LaPaletteGroupeLesPiecesParNiveau) {
    const core::ScenePieceManifestResult read =
        core::ScenePieceManifest::resolve(tree() / "Assets", ARENAREA);
    ASSERT_TRUE(read.ok()) << read.message;
    const std::vector<hmi::PieceCatalogGroup> catalog = hmi::pieceCatalog(&read.manifest, {});
    std::vector<std::string> levels;
    for (const hmi::PieceCatalogGroup& group : catalog) {
        if (levels.empty() || levels.back() != group.level) {
            levels.push_back(group.level);
        }
    }
    EXPECT_EQ(levels, (std::vector<std::string>{"Arenarea", "Capital", "Central Empire", "World"}));

    const std::vector<hmi::PieceCatalogGroup> found = hmi::filterPieceCatalog(catalog, "fountain");
    ASSERT_EQ(found.size(), 2U);
    EXPECT_EQ(found[0].level, "Arenarea");
    ASSERT_EQ(found[0].pieces.size(), 1U);
    EXPECT_EQ(found[0].pieces[0].masks, "Capital");
    EXPECT_EQ(found[0].pieces[0].file, std::string{ZONE_LEVEL} + "/fountain.png");
    EXPECT_EQ(found[1].level, "Capital");
    ASSERT_EQ(found[1].pieces.size(), 1U);
    EXPECT_EQ(found[1].pieces[0].maskedBy, "Arenarea");
    EXPECT_NE(hmi::pieceDescription(found[1].pieces[0]).find("masked by the Arenarea"),
              std::string::npos);
}

/**
 * @brief Une pièce promue de la zone à la ville **sous la même clé** ne change aucun octet des
 *        cartes : elles citent un nom court, que la résolution trouve plus haut.
 * \castest{<b>Promouvoir une piece sous sa cle ne change aucune carte.</b><br/>
 * \tcat Unitaire · Editeur · Arborescence<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Deplacer les paves de l'Arenarea (manifeste et image) vers le kit de la
 * Capitale.<br/>
 *          2. Controler les cartes, puis les migrer sans les ecrire.<br/>
 * \tattendu Le controle passe ; la migration rend chaque carte octet pour octet ; les paves se
 * resolvent maintenant dans la ville.
 * }
 */
TEST_F(Arborescence, PromouvoirSousSaCleNeChangeAucuneCarte) {
    deplacer(ZONE_LEVEL, "scene/arenarea/cobbles", CITY_LEVEL, "scene/capital/cobbles");

    const hmi::MapCheckReport report = hmi::checkAllMaps(racine);
    EXPECT_TRUE(report.ok()) << constats(report);
    for (const char* id : {ARENAREA, MARTPART}) {
        const hmi::MapMigration migration = hmi::migrateMapFile(carte(id), racine);
        ASSERT_TRUE(migration.ok()) << migration.error;
        EXPECT_EQ(migration.text, lire(carte(id))) << id;
    }
    const core::ScenePieceManifestResult read =
        core::ScenePieceManifest::resolve(racine / "Assets", ARENAREA);
    ASSERT_TRUE(read.ok()) << read.message;
    ASSERT_NE(read.manifest.find("cobbles"), nullptr);
    EXPECT_EQ(read.manifest.find("cobbles")->directory, CITY_LEVEL);
}

/**
 * @brief Promue **sous une autre clé**, la fontaine de l'Arenarea se remplace sur les seules cartes
 *        qui la tiennent de l'Arenarea : le Martpart, qui pose la fontaine de la ville sous le même
 *        nom court, ne bouge pas.
 * \castest{<b>Promouvoir sous une autre cle ne recrit que les cartes concernees.</b><br/>
 * \tcat Unitaire · Editeur · Arborescence<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Copier la fontaine de l'Arenarea dans le kit de la Capitale sous
 * `fountain-arena`.<br/>
 *          2. Planifier `--replace-piece fountain fountain-arena` sans niveau, puis avec le niveau
 *          de la zone ; appliquer ce dernier.<br/>
 *          3. Retirer la fontaine de la zone ; controler.<br/>
 * \tattendu Sans niveau, les deux cartes changeraient ; avec, l'Arenarea seule ; le Martpart garde
 * ses octets ; le controle passe ; « who cites fountain » ne trouve plus que le Martpart, qui la
 * tient de la ville.
 * }
 */
TEST_F(Arborescence, PromouvoirSousUneAutreCleNeRecritQueSesCartes) {
    deplacer(ZONE_LEVEL, "scene/arenarea/fountain", CITY_LEVEL, "scene/capital/fountain-arena",
             /*garder=*/true);
    const std::string martpart = lire(carte(MARTPART));

    EXPECT_EQ(hmi::planReplacePiece(racine, "fountain", "fountain-arena", {}).edits.size(), 2U);
    const hmi::RefactorPlan plan =
        hmi::planReplacePiece(racine, "fountain", "fountain-arena", {}, ZONE_LEVEL);
    ASSERT_TRUE(plan.ok()) << plan.error;
    ASSERT_EQ(plan.edits.size(), 1U);
    EXPECT_EQ(plan.edits.front().file, carte(ARENAREA));
    std::string error;
    ASSERT_TRUE(hmi::applyRefactorPlan(plan, error)) << error;

    nlohmann::ordered_json zone = lireJson(niveau(ZONE_LEVEL) / "manifest.json");
    zone["textures"].erase("scene/arenarea/fountain");
    ecrireJson(niveau(ZONE_LEVEL) / "manifest.json", zone);

    const hmi::MapCheckReport report = hmi::checkAllMaps(racine);
    EXPECT_TRUE(report.ok()) << constats(report);
    EXPECT_EQ(lire(carte(MARTPART)), martpart);
    const std::vector<hmi::Citation> cited = hmi::citationsOfPiece(racine, "fountain");
    ASSERT_EQ(cited.size(), 1U);
    EXPECT_EQ(cited.front().mapId, MARTPART);
    EXPECT_NE(cited.front().what.find(CITY_LEVEL), std::string::npos) << cited.front().what;
}

/**
 * @brief « Qui cite cette pièce ? » suit les niveaux : deux fontaines de même nom ne se citent pas
 *        l'une l'autre.
 * \castest{<b>Qui cite une piece, niveau par niveau.</b><br/>
 * \tcat Unitaire · Editeur · Arborescence<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander qui cite `fountain`, sans niveau, puis au niveau de la zone, puis de la
 * ville, puis par la ligne de commande.<br/>
 * \tattendu Les deux cartes ; l'Arenarea seule ; le Martpart seul ; la commande nomme le niveau.
 * }
 */
TEST(LevelTreeTest, QuiCiteUnePieceNiveauParNiveau) {
    EXPECT_EQ(hmi::citationsOfPiece(tree(), "fountain").size(), 2U);
    const std::vector<hmi::Citation> zone = hmi::citationsOfPiece(tree(), "fountain", ZONE_LEVEL);
    ASSERT_EQ(zone.size(), 1U);
    EXPECT_EQ(zone.front().mapId, ARENAREA);
    const std::vector<hmi::Citation> city =
        hmi::citationsOfPiece(tree(), "fountain", std::string{CITY_LEVEL} + "/");
    ASSERT_EQ(city.size(), 1U);
    EXPECT_EQ(city.front().mapId, MARTPART);

    std::string output;
    EXPECT_EQ(
        hmi::runRefactorCommand({"--who-cites", "piece", "fountain", ZONE_LEVEL}, tree(), output),
        0);
    EXPECT_NE(output.find(std::string{"from "} + ZONE_LEVEL), std::string::npos) << output;
    EXPECT_EQ(output.find(MARTPART), std::string::npos) << output;
}

/**
 * @brief « New map » range la carte sous le chemin de son lieu.
 * \castest{<b>Une carte neuve se range sous le chemin de son lieu.</b><br/>
 * \tcat Unitaire · Editeur · Arborescence<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Creer la carte `arena-of-fate` du lieu `…/arenarea/arena-of-fate`.<br/>
 * \tattendu Le fichier est `Levels/central-empire/capital/arenarea/arena-of-fate.json`, nomme son
 * lieu, et passe le controle.
 * }
 */
TEST_F(Arborescence, UneCarteNeuveSeRangeSousSonLieu) {
    EXPECT_EQ(hmi::levelFolderOf("central-empire/capital/arenarea"), "central-empire/capital");
    EXPECT_EQ(hmi::levelFolderOf("bourg"), "");
    EXPECT_EQ(hmi::levelFolderOf(""), "");

    const hmi::LevelFileOperations ops(racine / "Levels");
    const std::string place = std::string{ARENAREA} + "/arena-of-fate";
    const hmi::FileOperationResult created = ops.create("arena-of-fate", 6, 5, place);
    ASSERT_TRUE(created.ok()) << created.error;
    const std::filesystem::path file = carte(place);
    EXPECT_EQ(created.path, file);
    ASSERT_TRUE(std::filesystem::is_regular_file(file));
    const core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(file);
    ASSERT_TRUE(loaded.ok()) << loaded.error;
    EXPECT_EQ(hmi::scenePlaceOf(loaded.level->layers()), place);
    const std::vector<hmi::MapCheckFinding> findings = hmi::checkMapFile(place, file, racine);
    EXPECT_TRUE(std::ranges::none_of(findings, [](const hmi::MapCheckFinding& finding) {
        return finding.severity == hmi::MapCheckSeverity::Error;
    }));
}

/**
 * @brief Un préfabriqué fait du seul kit de la ville se range sous la ville et sert à tous ses
 *        quartiers ; un préfabriqué qui pose une pièce de zone reste à la zone.
 * \castest{<b>Un prefabrique se range au niveau de ses pieces.</b><br/>
 * \tcat Unitaire · Editeur · Arborescence<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. `--save-prefab` d'un rectangle du Martpart (pavage, fontaine et arcade de la
 * ville).<br/>
 *          2. Le lister et le relire depuis l'Arenarea, puis depuis un lieu d'une autre ville.<br/>
 *          3. `--save-prefab` d'un rectangle de l'Arenarea qui prend son etal.<br/>
 * \tattendu Le premier est ecrit sous `Editor/Prefabs/central-empire/capital/`, propose a
 * l'Arenarea et relu depuis elle, pas a l'autre ville ; le second sous le dossier de l'Arenarea.
 * }
 */
TEST_F(Arborescence, UnPrefabriqueSeRangeAuNiveauDeSesPieces) {
    std::string output;
    ASSERT_EQ(hmi::runPrefabCommand(
                  {"--save-prefab", MARTPART, "fontaine", "--from", "1,0", "--to", "4,2"}, racine,
                  output),
              0)
        << output;
    EXPECT_NE(output.find("saved central-empire/capital/fontaine"), std::string::npos) << output;
    EXPECT_TRUE(std::filesystem::is_regular_file(racine / "Editor" / "Prefabs" / "central-empire" /
                                                 "capital" / "fontaine.json"));

    const std::vector<hmi::PrefabEntry> offered = hmi::availablePrefabs(racine, ARENAREA);
    ASSERT_EQ(offered.size(), 1U);
    EXPECT_EQ(offered.front(),
              (hmi::PrefabEntry{.name = "fontaine", .level = "central-empire/capital"}));
    std::string error;
    EXPECT_TRUE(hmi::readPrefab(racine, ARENAREA, "fontaine", error).has_value()) << error;
    EXPECT_TRUE(hmi::availablePrefabs(racine, "central-empire/skybell-city/harbour").empty());

    output.clear();
    ASSERT_EQ(
        hmi::runPrefabCommand({"--save-prefab", ARENAREA, "etal", "--from", "2,3", "--to", "3,3"},
                              racine, output),
        0)
        << output;
    EXPECT_TRUE(std::filesystem::is_regular_file(racine / "Editor" / "Prefabs" / "central-empire" /
                                                 "capital" / "arenarea" / "etal.json"))
        << output;
    EXPECT_EQ(hmi::prefabNames(racine, MARTPART), std::vector<std::string>{"fontaine"});
}

/**
 * @brief Les modèles de carte se rangent aussi par niveau : celui de la ville n'est proposé qu'à
 *        ses quartiers.
 * \castest{<b>Les modeles d'un niveau servent les lieux qui en descendent.</b><br/>
 * \tcat Unitaire · Editeur · Arborescence<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lister les modeles sans lieu, pour l'Arenarea, puis pour un lieu d'une autre
 * ville.<br/>
 * \tattendu La rue seule ; la place et la rue ; la rue seule.
 * }
 */
TEST(LevelTreeTest, LesModelesDUnNiveauServentSesDescendants) {
    const auto ids = [](const std::vector<hmi::MapTemplate>& models) {
        std::vector<std::string> names;
        for (const hmi::MapTemplate& model : models) {
            names.push_back(model.id);
        }
        return names;
    };
    EXPECT_EQ(ids(hmi::mapTemplates(tree())), std::vector<std::string>{"street"});
    EXPECT_EQ(ids(hmi::mapTemplates(tree(), ARENAREA)),
              (std::vector<std::string>{"plaza", "street"}));
    EXPECT_EQ(ids(hmi::mapTemplates(tree(), "central-empire/skybell-city/harbour")),
              std::vector<std::string>{"street"});
    EXPECT_TRUE(hmi::checkEditorLibrary(tree()).empty());
}

/**
 * @brief Les PNJ de l'Arenarea prennent leur figurine sous le niveau qui la range, et l'éditeur ne
 *        propose un PNJ nommé qu'aux cartes de sa zone.
 * \castest{<b>Les figurines d'une carte viennent de ses niveaux.</b><br/>
 * \tcat Unitaire · Editeur · Arborescence<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Tirer l'instantane de l'Arenarea avec ses PNJ, puis les chemins de leurs bandes.<br/>
 *          2. Faire le contexte des references pour l'Arenarea, puis pour le Martpart.<br/>
 * \tattendu Le PNJ nomme et le citadin sous la zone, le garde sous le monde, chaque bande existe ;
 * le PNJ nomme est propose a l'Arenarea, pas au Martpart ; le garde du monde aux deux.
 * }
 */
TEST(LevelTreeTest, LesFiguresDUneCarteViennentDeSesNiveaux) {
    const core::LevelLoadResult map = core::LevelLoader::loadFromFile(
        tree() / "Levels" / "central-empire" / "capital" / "arenarea.json");
    ASSERT_TRUE(map.ok()) << map.error;
    const hmi::PlaceAppearanceResult table =
        hmi::PlaceAppearance::loadForPlace(tree() / "Assets", ARENAREA);
    ASSERT_TRUE(table.ok()) << table.message;
    const hmi::WorldSceneSnapshot snapshot = hmi::snapshotWorldScene(
        *map.level, table.appearance, hmi::npcFigures(map.level->entities(), 0));
    ASSERT_EQ(snapshot.figures.size(), 3U);
    EXPECT_EQ(snapshot.figureDirectories.at("anariel"),
              "Regions/central-empire/capital/arenarea/Characters/anariel");
    EXPECT_EQ(snapshot.figureDirectories.at("citizen"),
              "Regions/central-empire/capital/arenarea/Characters/citizen");
    EXPECT_EQ(snapshot.figureDirectories.at("Peoples/human/guard"),
              "Common/Characters/Peoples/human/guard");
    for (const std::string& path : hmi::worldTexturePaths(snapshot)) {
        EXPECT_TRUE(std::filesystem::is_regular_file(tree() / "Assets" / path)) << path;
    }

    const hmi::EditorReferences references = hmi::loadEditorReferences(tree());
    const core::EntityReferenceContext arena =
        hmi::referenceContext(references, ARENAREA, map.level->entities(), ARENAREA);
    EXPECT_TRUE(arena.figures.contains("anariel"));
    EXPECT_TRUE(arena.figures.contains("Peoples/human/guard"));
    const core::EntityReferenceContext mart =
        hmi::referenceContext(references, MARTPART, {}, MARTPART);
    EXPECT_FALSE(mart.figures.contains("anariel"));
    EXPECT_TRUE(mart.figures.contains("citizen"));
    EXPECT_TRUE(mart.figures.contains("Peoples/human/guard"));
}
