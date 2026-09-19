// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_map_refactor.cpp
 * @brief Tests de « renommer et remplacer » (`LOT-EDITOR-14`) : sur une copie des données livrées,
 *        un renommage suit tout ce qui cite, et une carte change de planche sans être repeinte.
 */

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileMap.h"
#include "Editor/Logic/EditorSidecar.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/MapRefactor.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

[[nodiscard]] std::filesystem::path elements() {
    return std::filesystem::path(JADG_LEVELS_DIR).parent_path();
}

[[nodiscard]] std::string lire(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void ecrire(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << text;
}

// Ce que le contrôle lit des données livrées, sans le récrire : ce que les entités citent, et les
// manifestes des figurines — pas leurs images.
constexpr const char* DOSSIERS_LUS[] = {"World/dialogues", "World/locations", "Rpg/encounters",
                                        "Rpg/creatures",   "Rpg/items",       "Assets/Npc",
                                        "Assets/Monsters"};
// Ce qu'un test peut récrire : les cartes, les villes, les catalogues, les planches.
constexpr const char* DOSSIERS_RECRITS[] = {"Levels", "Localization", "World/cities",
                                            "Assets/Scene"};

// Recopie les fichiers de @p dossier des données livrées sous @p racine, images exceptées.
void copier(const std::filesystem::path& racine, const char* dossier) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(elements() / dossier)) {
        const std::string extension = entry.path().extension().string();
        if (!entry.is_regular_file() || extension == ".png" || extension == ".webp") {
            continue;
        }
        const std::filesystem::path copie =
            racine / std::filesystem::relative(entry.path(), elements());
        std::filesystem::create_directories(copie.parent_path());
        std::filesystem::copy_file(entry.path(), copie);
    }
}

// Une copie des données par suite ; ce qu'un test récrit est recopié avant chacun. Copier tout à
// chaque test coûtait quatre secondes sur ce poste.
class Donnees : public ::testing::Test {
protected:
    static inline std::filesystem::path racine;

    static void SetUpTestSuite() {
        racine = std::filesystem::temp_directory_path() /
                 ("jadg-map-refactor-" + std::to_string(std::rand()));
        for (const char* dossier : DOSSIERS_LUS) {
            copier(racine, dossier);
        }
    }
    static void TearDownTestSuite() {
        std::error_code ignore;
        std::filesystem::remove_all(racine, ignore);
    }
    void SetUp() override {
        for (const char* dossier : DOSSIERS_RECRITS) {
            std::error_code ignore;
            std::filesystem::remove_all(racine / dossier, ignore);
            copier(racine, dossier);
        }
    }

    [[nodiscard]] std::filesystem::path carte(std::string_view id) const {
        return racine / "Levels" / (std::string{id} + ".json");
    }

    // Écrit le plan ; échoue le test s'il est refusé.
    void appliquer(const hmi::RefactorPlan& plan) const {
        ASSERT_TRUE(plan.ok()) << plan.error;
        std::string erreur;
        ASSERT_TRUE(hmi::applyRefactorPlan(plan, erreur)) << erreur;
    }

    [[nodiscard]] std::string constats() const {
        std::string texte;
        for (const hmi::MapCheckFinding& finding : hmi::checkAllMaps(racine).findings) {
            texte += hmi::formatFinding(finding) + "\n";
        }
        return texte;
    }
};

[[nodiscard]] bool cite(const std::vector<hmi::Citation>& citations, std::string_view fragment) {
    return std::ranges::any_of(citations, [fragment](const hmi::Citation& citation) {
        return citation.what.find(fragment) != std::string::npos;
    });
}

}  // namespace

/**
 * @brief Acceptation du `LOT-EDITOR-14` : renommer `capital/martpart` laisse le `--check` vert. Le
 *        portail d'Arenarea, les quartiers et portes gardées de la ville et la clé du nom suivent.
 * \castest{<b>Renommer Martpart laisse le contrôle vert.</b><br/>
 * \tcat Unitaire · Renommer et remplacer<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Copier les données livrées.<br/>2. Renommer `capital/martpart` en
 * `capital/marche`.<br/>3. Contrôler toutes les cartes.<br/>
 * \tattendu Aucune erreur, autant d'avertissements qu'avant ; l'ancien fichier a disparu ; le
 * portail, la ville et les deux catalogues citent le nouvel identifiant, le texte gardé.
 * }
 */
TEST_F(Donnees, RenommerMartpartLaisseLeControleVert) {
    const hmi::MapCheckReport avant = hmi::checkAllMaps(racine);
    ASSERT_TRUE(avant.ok()) << constats();

    appliquer(hmi::planRenameMap(racine, "capital/martpart", "capital/marche"));

    const hmi::MapCheckReport apres = hmi::checkAllMaps(racine);
    EXPECT_TRUE(apres.ok()) << constats();
    EXPECT_EQ(apres.count(hmi::MapCheckSeverity::Warning),
              avant.count(hmi::MapCheckSeverity::Warning));
    EXPECT_FALSE(std::filesystem::exists(carte("capital/martpart")));
    EXPECT_EQ(core::LevelLoader::loadFromFile(carte("capital/marche")).level->name(),
              "map.capital.marche.name");
    EXPECT_NE(lire(carte("capital/arenarea")).find(R"("targetMap": "capital/marche")"),
              std::string::npos);
    const std::string ville = lire(racine / "World" / "cities" / "capital.json");
    EXPECT_EQ(ville.find("capital/martpart"), std::string::npos);
    EXPECT_NE(ville.find(R"("map": "capital/marche")"), std::string::npos);
    const std::string fr = lire(racine / "Localization" / "fr.lang");
    EXPECT_NE(fr.find("map.capital.marche.name = Martpart"), std::string::npos);
    EXPECT_EQ(fr.find("map.capital.martpart.name"), std::string::npos);
}

/**
 * @brief Une carte change de dossier, son annexe avec elle ; le dossier qu'elle quitte, vidé, ne
 *        reste pas.
 * \castest{<b>Une carte change de dossier, son annexe la suit.</b><br/>
 * \tcat Unitaire · Renommer et remplacer<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Donner une note d'auteur au Colisée.<br/>2. Le renommer `arenes/coliseum`, puis
 * de nouveau `coliseum`.<br/>
 * \tattendu La note est dans `arenes/coliseum.editor.json`, puis de retour ; le dossier `arenes`
 * a disparu ; le contrôle reste sans erreur.
 * }
 */
TEST_F(Donnees, UneCarteChangeDeDossierSonAnnexeLaSuit) {
    hmi::EditorSidecar notes;
    ASSERT_TRUE(hmi::setNote(notes, {.column = 3, .row = 4}, "loge"));
    ASSERT_TRUE(hmi::writeSidecar(hmi::sidecarPath(carte("coliseum")), notes));

    appliquer(hmi::planRenameMap(racine, "coliseum", "arenes/coliseum"));
    EXPECT_FALSE(std::filesystem::exists(hmi::sidecarPath(carte("coliseum"))));
    const hmi::SidecarReadResult lues =
        hmi::readSidecar(hmi::sidecarPath(carte("arenes/coliseum")));
    ASSERT_NE(hmi::noteAt(lues.sidecar, {.column = 3, .row = 4}), nullptr);
    EXPECT_TRUE(hmi::checkAllMaps(racine).ok()) << constats();

    appliquer(hmi::planRenameMap(racine, "arenes/coliseum", "coliseum"));
    EXPECT_TRUE(std::filesystem::exists(hmi::sidecarPath(carte("coliseum"))));
    EXPECT_FALSE(std::filesystem::exists(racine / "Levels" / "arenes"));
}

/**
 * @brief Un renommage impossible est refusé avant toute écriture : nom pris, identifiant invalide,
 *        carte illisible dans le projet.
 * \castest{<b>Un renommage impossible n'écrit rien.</b><br/>
 * \tcat Unitaire · Renommer et remplacer<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Renommer Martpart en Arenarea, puis en `a:b`.<br/>2. Ajouter une carte
 * illisible et renommer Martpart.<br/>
 * \tattendu Trois refus, chacun sans rien à écrire ; le dernier nomme la carte illisible.
 * }
 */
TEST_F(Donnees, UnRenommageImpossibleNEcritRien) {
    const hmi::RefactorPlan pris =
        hmi::planRenameMap(racine, "capital/martpart", "capital/arenarea");
    EXPECT_FALSE(pris.ok());
    EXPECT_TRUE(pris.edits.empty());
    EXPECT_FALSE(hmi::planRenameMap(racine, "capital/martpart", "a:b").ok());

    ecrire(carte("cassee"), "{");
    const hmi::RefactorPlan illisible =
        hmi::planRenameMap(racine, "capital/martpart", "capital/marche");
    EXPECT_FALSE(illisible.ok());
    EXPECT_NE(illisible.error.find("cassee"), std::string::npos) << illisible.error;
    EXPECT_TRUE(illisible.edits.empty());
}

/**
 * @brief Renommer un point d'arrivée récrit les portails qui y mènent et la porte de départ de la
 *        ville, pas les points homonymes d'autres cartes.
 * \castest{<b>Renommer un point d'arrivée suit portails et ville.</b><br/>
 * \tcat Unitaire · Renommer et remplacer<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Renommer le point `porte-est` de Martpart, départ de la Capitale.<br/>2. Renommer
 * son point `arenarea`, où mène le portail d'Arenarea.<br/>
 * \tattendu La ville part de `porte-orientale` ; le portail d'Arenarea arrive à
 * `vers-arenarea` ; le contrôle reste sans erreur.
 * }
 */
TEST_F(Donnees, RenommerUnPointDArriveeSuitPortailsEtVille) {
    appliquer(hmi::planRenameArrival(racine, "capital/martpart", "porte-est", "porte-orientale"));
    EXPECT_NE(
        lire(racine / "World" / "cities" / "capital.json").find(R"("arrival": "porte-orientale")"),
        std::string::npos);

    const std::vector<hmi::Citation> portails =
        hmi::citationsOfArrival(racine, "capital/martpart", "arenarea");
    ASSERT_EQ(portails.size(), 1U);
    EXPECT_EQ(portails.front().mapId, "capital/arenarea");
    appliquer(hmi::planRenameArrival(racine, "capital/martpart", "arenarea", "vers-arenarea"));
    EXPECT_NE(lire(carte("capital/arenarea")).find(R"("arrival": "vers-arenarea")"),
              std::string::npos);
    EXPECT_FALSE(
        hmi::planRenameArrival(racine, "capital/martpart", "vers-arenarea", "porte-orientale")
            .ok());
    EXPECT_TRUE(hmi::checkAllMaps(racine).ok()) << constats();
}

/**
 * @brief Un identifiant d'entité se renomme, sauf en un identifiant pris, invalide, ou que
 *        l'éditeur pourrait redonner (décision D8).
 * \castest{<b>Renommer un identifiant d'entité, et ses refus.</b><br/>
 * \tcat Unitaire · Renommer et remplacer<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Renommer `e6` de Martpart en `e1`, `e999`, `a#b`.<br/>2. Le renommer
 * `depart-est`.<br/>
 * \tattendu Trois refus ; puis l'entité s'appelle `depart-est` et le contrôle reste sans erreur.
 * }
 */
TEST_F(Donnees, RenommerUnIdentifiantDEntite) {
    EXPECT_FALSE(hmi::planRenameEntityId(racine, "capital/martpart", "e6", "e1").ok());
    EXPECT_FALSE(hmi::planRenameEntityId(racine, "capital/martpart", "e6", "e999").ok());
    EXPECT_FALSE(hmi::planRenameEntityId(racine, "capital/martpart", "e6", "a#b").ok());

    appliquer(hmi::planRenameEntityId(racine, "capital/martpart", "e6", "depart-est"));
    const auto lue = core::LevelLoader::loadFromFile(carte("capital/martpart"));
    ASSERT_TRUE(lue.ok());
    EXPECT_TRUE(std::ranges::any_of(lue.level->entities(),
                                    [](const auto& entity) { return entity.id == "depart-est"; }));
    EXPECT_TRUE(hmi::checkAllMaps(racine).ok()) << constats();
}

/**
 * @brief « Qui cite ceci ? » : une carte par ses portails, ses villes et la clé de son nom ; une
 *        pièce, carte par carte et couche par couche.
 * \castest{<b>Qui cite une carte, qui pose une pièce.</b><br/>
 * \tcat Unitaire · Renommer et remplacer<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander qui cite Martpart.<br/>2. Demander qui pose `wall-corner`.<br/>
 * \tattendu Le portail d'Arenarea, le quartier et une porte gardée de la ville, la clé de chaque
 * catalogue ; `wall-corner` sur les trois cartes livrées. Rien n'est écrit.
 * }
 */
TEST_F(Donnees, QuiCiteUneCarteQuiPoseUnePiece) {
    const std::vector<hmi::Citation> carteCitee = hmi::citationsOfMap(racine, "capital/martpart");
    EXPECT_TRUE(cite(carteCitee, "portal e2: targetMap"));
    EXPECT_TRUE(cite(carteCitee, "district central-empire-the-capital-city-martpart: map"));
    EXPECT_TRUE(cite(carteCitee, "guard map"));
    EXPECT_TRUE(cite(carteCitee, "map.capital.martpart.name"));

    const std::vector<hmi::Citation> piece = hmi::citationsOfPiece(racine, "wall-corner");
    for (const char* id : {"capital/arenarea", "capital/martpart", "coliseum"}) {
        EXPECT_TRUE(std::ranges::any_of(piece, [id](const hmi::Citation& c) {
            return c.mapId == id;
        })) << id;
    }
    EXPECT_TRUE(std::filesystem::exists(carte("capital/martpart")));
}

/**
 * @brief Remplacer une pièce sur toutes les cartes qui la posent, par le brouillon ; refusé pour
 *        une pièce absente de la planche ou d'une autre classe (`EX-EDIT-083`).
 * \castest{<b>Remplacer une pièce sur toutes les cartes.</b><br/>
 * \tcat Unitaire · Renommer et remplacer<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Remplacer `street` par `wall-left`, puis par `nope`.<br/>2. Remplacer `street-2`
 * par `street-3` partout.<br/>
 * \tattendu Deux refus ; puis plus aucune carte ne pose `street-2`, et le contrôle reste sans
 * erreur.
 * }
 */
TEST_F(Donnees, RemplacerUnePieceSurToutesLesCartes) {
    EXPECT_FALSE(hmi::planReplacePiece(racine, "street", "wall-left", {}).ok());
    EXPECT_FALSE(hmi::planReplacePiece(racine, "street", "nope", {}).ok());
    ASSERT_FALSE(hmi::citationsOfPiece(racine, "street-2").empty());

    appliquer(hmi::planReplacePiece(racine, "street-2", "street-3", {}));

    EXPECT_TRUE(hmi::citationsOfPiece(racine, "street-2").empty());
    EXPECT_TRUE(hmi::checkAllMaps(racine).ok()) << constats();
}

/**
 * @brief Acceptation du `LOT-EDITOR-14`, sur une planche d'essai : Arenarea passe de la planche de
 *        Martpart à une planche à elle sans être repeinte. Les pièces de même nom (ou d'un ancien
 *        nom) se retrouvent seules ; celle qui n'a pas de correspondant demande la table
 *        (`EX-EDIT-084`).
 * \castest{<b>Arenarea change de planche sans être repeinte.</b><br/>
 * \tcat Unitaire · Renommer et remplacer<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Installer une planche `arenarea`, copie de celle de Martpart où `street-2` devient
 * `paving-2` et `street-3` devient `cobbles` (ancien nom `street-3`).<br/>2. Changer de planche
 * sans table, puis avec la table `street-2 → paving-2`.<br/>
 * \tattendu Sans table : refus qui nomme `street-2`. Avec : la carte nomme `arenarea`, pose
 * `paving-2` et `cobbles`, sa collision n'a pas bougé, la planche reçoit une table d'apparence
 * traduite, et le contrôle reste sans erreur.
 * }
 */
TEST_F(Donnees, ArenareaChangeDePlancheSansEtreRepeinte) {
    std::string manifeste = lire(racine / "Assets" / "Scene" / "martpart" / "manifest.json");
    const auto remplacer = [&manifeste](std::string_view avant, std::string_view apres) {
        const std::size_t at = manifeste.find(avant);
        ASSERT_NE(at, std::string::npos) << avant;
        manifeste.replace(at, avant.size(), apres);
    };
    remplacer(R"("disposition": "martpart")", R"("disposition": "arenarea")");
    remplacer(R"("scene/martpart/street-2": {)", R"("scene/arenarea/paving-2": {)");
    remplacer(R"("scene/martpart/street-3": {)",
              R"("scene/arenarea/cobbles": {"aliases": ["street-3"], )");
    ecrire(racine / "Assets" / "Scene" / "arenarea" / "manifest.json", manifeste);
    const core::TileMap avant =
        core::LevelLoader::loadFromFile(carte("capital/arenarea")).level->tileMap();

    const hmi::RefactorPlan sansTable =
        hmi::planChangeScene(racine, "capital/arenarea", "arenarea", {});
    ASSERT_FALSE(sansTable.ok());
    EXPECT_NE(sansTable.error.find("street-2"), std::string::npos) << sansTable.error;
    EXPECT_EQ(sansTable.error.find("street-3"), std::string::npos) << sansTable.error;

    const std::filesystem::path table = racine / "table.json";
    ecrire(table, R"({"format": "jadg-piece-table", "version": 1,
                      "pieces": {"street-2": "paving-2"}})");
    const hmi::PieceTableResult lue = hmi::readPieceTable(table);
    ASSERT_TRUE(lue.ok()) << lue.error;
    appliquer(hmi::planChangeScene(racine, "capital/arenarea", "arenarea", lue.table));

    const core::LevelLoadResult apres = core::LevelLoader::loadFromFile(carte("capital/arenarea"));
    ASSERT_TRUE(apres.ok()) << apres.error;
    EXPECT_EQ(hmi::scenePlaceOf(apres.level->layers()), "arenarea");
    for (int row = 0; row < avant.height(); ++row) {
        for (int column = 0; column < avant.width(); ++column) {
            EXPECT_EQ(apres.level->tileMap().tile(column, row), avant.tile(column, row))
                << column << ", " << row;
        }
    }
    const std::string apparence =
        lire(racine / "Assets" / "Scene" / "arenarea" / "appearance.json");
    EXPECT_NE(apparence.find(R"("place": "arenarea")"), std::string::npos) << apparence;
    EXPECT_NE(apparence.find("paving-2"), std::string::npos) << apparence;
    EXPECT_EQ(hmi::citationsOfPiece(racine, "street-2").size(), 1U);  // Martpart seule
    EXPECT_FALSE(hmi::citationsOfPiece(racine, "paving-2").empty());
    EXPECT_FALSE(hmi::citationsOfPiece(racine, "cobbles").empty());
    EXPECT_TRUE(hmi::checkAllMaps(racine).ok()) << constats();
}

/**
 * @brief Une table de correspondance mal formée est refusée, et dit pourquoi.
 * \castest{<b>Une table de correspondance mal formée est refusée.</b><br/>
 * \tcat Unitaire · Renommer et remplacer<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Lire une table sans `format`, puis une pièce sans remplaçante.<br/>
 * \tattendu Deux refus, qui nomment le fichier.
 * }
 */
TEST_F(Donnees, UneTableMalFormeeEstRefusee) {
    const std::filesystem::path table = racine / "table.json";
    ecrire(table, R"({"pieces": {"a": "b"}})");
    EXPECT_FALSE(hmi::readPieceTable(table).ok());
    ecrire(table, R"({"format": "jadg-piece-table", "version": 1, "pieces": {"a": ""}})");
    const hmi::PieceTableResult vide = hmi::readPieceTable(table);
    EXPECT_FALSE(vide.ok());
    EXPECT_NE(vide.error.find("table.json"), std::string::npos);
}
