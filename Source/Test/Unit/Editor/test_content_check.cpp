// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_content_check.cpp
 * @brief Tests du contrôle du contenu (`LOT-EDITOR-07`) : ce qu'une carte promet au joueur et ne
 *        tient pas, sur toutes les cartes, et les clés de traduction qu'elles citent.
 */

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelWriter.h"
#include "Editor/Logic/ContentCheck.h"
#include "Editor/Logic/LevelFileOperations.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/MapTexts.h"

namespace {

using hmi::MapCheckSeverity;

/// La racine des données livrées : `Source/Elements`.
// La racine d'essai de l'éditeur (`LOT-123`) : deux cartes reliées, une planche, une ville, un
// dialogue, une rencontre, leurs textes. Ce test lisait les cartes LIVRÉES, que la table rase
// du `LOT-102` emporte. Voir `Fixtures/GameData/README.md`.
[[nodiscard]] std::filesystem::path elements() {
    return std::filesystem::path(JADG_TEST_DATA_DIR);
}

[[nodiscard]] std::string lire(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Un projet jetable : des cartes, deux catalogues, les rencontres, les créatures et les figurines
// livrées.
class Projet {
public:
    Projet() {
        _racine = std::filesystem::temp_directory_path() /
                  ("jadg-content-check-" + std::to_string(std::rand()));
        std::filesystem::create_directories(_racine / "Levels");
        std::filesystem::create_directories(_racine / "Localization");
        std::filesystem::create_directories(_racine / "Assets" / "Npc");
        std::filesystem::create_directories(_racine / "Rpg");
        for (const char* dossier : {"encounters", "creatures"}) {
            std::filesystem::copy(elements() / "Rpg" / dossier, _racine / "Rpg" / dossier,
                                  std::filesystem::copy_options::recursive);
        }
        std::filesystem::copy_file(elements() / "Assets" / "Npc" / "manifest.json",
                                   _racine / "Assets" / "Npc" / "manifest.json");
    }
    ~Projet() {
        std::error_code ignore;
        std::filesystem::remove_all(_racine, ignore);
    }
    Projet(const Projet&) = delete;
    Projet& operator=(const Projet&) = delete;
    Projet(Projet&&) = delete;
    Projet& operator=(Projet&&) = delete;

    [[nodiscard]] const std::filesystem::path& racine() const {
        return _racine;
    }

    void catalogue(const std::string& langue, const std::string& texte) const {
        std::ofstream file(_racine / "Localization" / (langue + ".lang"), std::ios::binary);
        file << texte;
    }

    // La carte, écrite canonique : le contrôle du format n'a rien à en dire.
    void carte(const std::string& id, const std::string& json) const {
        const core::LevelLoadResult lu = core::LevelLoader::loadFromString(json);
        ASSERT_TRUE(lu.ok()) << id << " : " << lu.error;
        std::ofstream file(_racine / "Levels" / (id + ".json"), std::ios::binary);
        file << core::LevelWriter::toJsonString(*lu.level);
    }

    // Écrite telle quelle : une variante se lit avec sa base.
    void brut(const std::string& id, const std::string& json) const {
        std::ofstream file(_racine / "Levels" / (id + ".json"), std::ios::binary);
        file << json;
    }

private:
    std::filesystem::path _racine;
};

[[nodiscard]] bool signale(const std::vector<hmi::MapCheckFinding>& constats,
                           MapCheckSeverity gravite, std::string_view carte,
                           std::string_view fragment) {
    return std::ranges::any_of(constats, [&](const hmi::MapCheckFinding& constat) {
        return constat.severity == gravite && constat.mapId == carte &&
               constat.message.find(fragment) != std::string::npos;
    });
}

[[nodiscard]] std::string tout(const std::vector<hmi::MapCheckFinding>& constats) {
    std::string texte;
    for (const hmi::MapCheckFinding& constat : constats) {
        texte += hmi::formatFinding(constat) + "\n";
    }
    return texte;
}

// Une carte de 8 x 5 : l'entrée en haut à gauche, un mur plein en colonne 4 qui coupe la carte en
// deux. Chaque entité porte un défaut, et un seul.
constexpr const char* FAUTIVE = R"({"version": 4, "name": "fautive", "width": 8, "height": 5,
  "nextEntityId": 10,
  "tiles": [ {"x": 0, "y": 0, "type": "entry"},
    {"x": 4, "y": 0, "type": "wall"}, {"x": 4, "y": 1, "type": "wall"},
    {"x": 4, "y": 2, "type": "wall"}, {"x": 4, "y": 3, "type": "wall"},
    {"x": 4, "y": 4, "type": "wall"} ],
  "entities": [
    {"id": "e1", "type": "npc", "x": 1, "y": 0, "dialogue": "inconnu"},
    {"id": "e2", "type": "npc", "x": 6, "y": 2},
    {"id": "e3", "type": "chest", "x": 4, "y": 1},
    {"id": "e4", "type": "portal", "x": 2, "y": 0, "targetMap": "autre", "arrival": "quai",
     "requiresFlag": "jamais-pose"},
    {"id": "e5", "type": "spawnPoint", "x": 1, "y": 1, "name": "orpheline"},
    {"id": "e6", "type": "cityBlock", "x": 0, "y": 0, "name": "ilot-sans-cle", "width": 1,
     "height": 1},
    {"id": "e7", "type": "encounter", "x": 0, "y": 3, "encounterId": "rats-du-donjon"},
    {"id": "e8", "type": "zone", "x": 6, "y": 4, "width": 1, "height": 1},
    {"id": "e9", "type": "mystere", "x": 0, "y": 1}
  ]})";

}  // namespace

/**
 * @brief La carte de test du lot : un défaut de chaque sorte, et chacun sort.
 * \castest{<b>Chaque défaut de contenu sort, et la CI échoue.</b><br/>
 * \tcat Unitaire · Contrôle du contenu<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Un projet de trois cartes et une variante : une carte fautive (nom qui n'est pas une
 * clé, îlot sans clé, dialogue inconnu, drapeau que rien ne pose, famille inconnue, rencontre dont
 * un rat tombe hors de la carte, PNJ muré, coffre dans un mur, zone hors d'atteinte, portail sans
 * retour, point d'arrivée orphelin), sa cible, et la variante d'une base dont un mur est tombé
 * sur son PNJ.<br/>2. Lancer `--check`.<br/>
 * \tattendu Chaque défaut sort, à sa gravité ; code de sortie 1.
 * }
 */
TEST(ContentCheckTest, ChaqueDefautDeContenuSort) {
    const Projet projet;
    projet.catalogue("fr", "map.autre.name = Autre\nmap.base.name = Base\nmap.nuit.name = Nuit\n");
    projet.catalogue("en", "map.autre.name = Other\nmap.base.name = Base\n");
    projet.carte("fautive", FAUTIVE);
    projet.carte("autre", R"({"version": 4, "name": "map.autre.name", "width": 3, "height": 1,
      "nextEntityId": 2, "tiles": [ {"x": 0, "y": 0, "type": "entry"} ],
      "entities": [ {"id": "e1", "type": "spawnPoint", "x": 2, "y": 0, "name": "quai"} ]})");
    // La base a reçu un mur en (1, 1), là où sa variante pose son PNJ.
    projet.carte("base", R"({"version": 4, "name": "map.base.name", "width": 3, "height": 3,
      "nextEntityId": 1, "tiles": [ {"x": 0, "y": 0, "type": "entry"},
      {"x": 1, "y": 1, "type": "wall"} ], "entities": []})");
    projet.brut("nuit", R"({"version": 4, "name": "map.nuit.name", "base": "base",
      "nextEntityId": 2,
      "entities": [ {"id": "e1", "type": "npc", "x": 1, "y": 1} ]})");

    const hmi::MapCheckReport bilan = hmi::checkAllMaps(projet.racine());
    const std::vector<hmi::MapCheckFinding>& constats = bilan.findings;

    // Les textes.
    EXPECT_TRUE(signale(constats, MapCheckSeverity::Error, "fautive",
                        "map name \"fautive\" is not a translation key of en.lang, fr.lang "
                        "(expected map.fautive.name)"))
        << tout(constats);
    EXPECT_TRUE(signale(constats, MapCheckSeverity::Error, "fautive",
                        "key city_block.ilot-sans-cle is missing"));
    EXPECT_TRUE(signale(constats, MapCheckSeverity::Error, "nuit",
                        "map name \"map.nuit.name\" is not a translation key of en.lang"));
    // Les références : dialogue, drapeau de monde ; une famille inconnue n'est qu'un avertissement.
    EXPECT_TRUE(signale(constats, MapCheckSeverity::Error, "fautive", "dialogue \"inconnu\""));
    EXPECT_TRUE(
        signale(constats, MapCheckSeverity::Error, "fautive", "no dialogue sets flag \"jamais"));
    EXPECT_TRUE(signale(constats, MapCheckSeverity::Warning, "fautive", "\"mystere\" is unknown"));
    // Le terrain : la rencontre du LOT-11, sur toute carte ; au bord, un rat tomberait dehors.
    EXPECT_TRUE(signale(constats, MapCheckSeverity::Error, "fautive",
                        "Encounter \"rats-du-donjon\": \"rat-d-essai\" would stand off the map"))
        << tout(constats);
    // L'atteignabilité.
    EXPECT_TRUE(signale(constats, MapCheckSeverity::Error, "fautive",
                        "npc e2 cannot be reached from the entry or an arrival point"));
    EXPECT_TRUE(
        signale(constats, MapCheckSeverity::Error, "fautive", "chest e3 stands on a blocked cell"));
    EXPECT_TRUE(signale(constats, MapCheckSeverity::Error, "fautive",
                        "zone e8: no cell of it can be reached"));
    EXPECT_FALSE(signale(constats, MapCheckSeverity::Error, "fautive", "npc e1 cannot be reached"));
    // Le graphe.
    EXPECT_TRUE(signale(constats, MapCheckSeverity::Warning, "fautive",
                        "portal e4 (autre) has no way back"));
    EXPECT_TRUE(signale(constats, MapCheckSeverity::Warning, "fautive",
                        "spawnPoint e5 (orpheline) is named by no portal and no city start"));
    EXPECT_FALSE(signale(constats, MapCheckSeverity::Warning, "autre", "quai"))
        << "un point qu'un portail nomme n'est pas orphelin";
    // La variante, contrôlée sur les cases de sa base.
    EXPECT_TRUE(
        signale(constats, MapCheckSeverity::Error, "nuit", "npc e1 stands on a blocked cell"));

    // L'entité d'un constat est nommée : le panneau la sélectionne.
    const auto mure = std::ranges::find_if(constats, [](const hmi::MapCheckFinding& constat) {
        return constat.message.starts_with("npc e2");
    });
    ASSERT_NE(mure, constats.end());
    EXPECT_EQ(mure->entityId, "e2");
    EXPECT_EQ(mure->cell, (core::GridPosition{.column = 6, .row = 2}));

    std::string sortie;
    EXPECT_EQ(hmi::runMapCommand({"--check", "--data", projet.racine().string()}, {}, sortie), 1);
}

/**
 * @brief Un seul défaut de contenu suffit à faire échouer `--check`, donc la CI.
 * \castest{<b>Un PNJ muré fait échouer la CI.</b><br/>
 * \tcat Unitaire · Contrôle du contenu<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Une carte canonique, au nom traduit, dont le seul défaut est un PNJ muré.<br/>
 * 2. Lancer `--check`.<br/>
 * \tattendu Une erreur, celle du PNJ ; code de sortie 1.
 * }
 */
TEST(ContentCheckTest, UnSeulDefautFaitEchouerLaCi) {
    const Projet projet;
    projet.catalogue("fr", "map.seule.name = Seule\n");
    projet.carte("seule", R"({"version": 4, "name": "map.seule.name", "width": 3, "height": 1,
      "nextEntityId": 2, "tiles": [ {"x": 0, "y": 0, "type": "entry"},
      {"x": 1, "y": 0, "type": "wall"} ],
      "entities": [ {"id": "e1", "type": "npc", "x": 2, "y": 0} ]})");

    std::string sortie;
    EXPECT_EQ(hmi::runMapCommand({"--check", "--data", projet.racine().string()}, {}, sortie), 1);
    EXPECT_NE(sortie.find("seule (2, 0): error: npc e1 cannot be reached"), std::string::npos)
        << sortie;
    EXPECT_NE(sortie.find("1 errors"), std::string::npos) << sortie;
}

/**
 * @brief Un point d'arrivée qu'un portail nomme est un départ : ce qu'on atteint depuis lui compte.
 * \castest{<b>On atteint une carte par ses points d'arrivée.</b><br/>
 * \tcat Unitaire · Contrôle du contenu<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Une carte coupée en deux : l'entrée d'un côté, un point d'arrivée nommé par le
 * portail d'une autre carte de l'autre, et un PNJ près de lui.<br/>2. Contrôler.<br/>
 * \tattendu Le PNJ n'est pas signalé ; le portail a son retour.
 * }
 */
TEST(ContentCheckTest, UnPointDArriveeNommeEstUnDepart) {
    const Projet projet;
    projet.catalogue("fr", "map.quai.name = Quai\nmap.port.name = Port\n");
    projet.carte("quai", R"({"version": 4, "name": "map.quai.name", "width": 6, "height": 1,
      "nextEntityId": 4, "tiles": [ {"x": 0, "y": 0, "type": "entry"},
      {"x": 2, "y": 0, "type": "wall"} ],
      "entities": [ {"id": "e1", "type": "spawnPoint", "x": 5, "y": 0, "name": "ponton"},
                    {"id": "e2", "type": "npc", "x": 3, "y": 0},
                    {"id": "e3", "type": "portal", "x": 4, "y": 0, "targetMap": "port",
                     "arrival": "cale"} ]})");
    projet.carte("port", R"({"version": 4, "name": "map.port.name", "width": 2, "height": 1,
      "nextEntityId": 3, "tiles": [ {"x": 0, "y": 0, "type": "entry"} ],
      "entities": [ {"id": "e1", "type": "spawnPoint", "x": 0, "y": 0, "name": "cale"},
                    {"id": "e2", "type": "portal", "x": 1, "y": 0, "targetMap": "quai",
                     "arrival": "ponton"} ]})");

    const hmi::MapCheckReport bilan = hmi::checkAllMaps(projet.racine());
    EXPECT_TRUE(bilan.ok()) << tout(bilan.findings);
    EXPECT_EQ(bilan.count(MapCheckSeverity::Warning), 0U) << tout(bilan.findings);
}

/**
 * @brief Les textes d'une carte : la clé de son nom, et les catalogues qu'on complète.
 * \castest{<b>Une clé ajoutée garde les traductions qu'elle reprend.</b><br/>
 * \tcat Unitaire · Contrôle du contenu<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Former la clé de `bourg/place`.<br/>2. Ajouter une clé nouvelle, puis une clé
 * qui reprend une clé existante, à deux catalogues dont l'un n'a pas de fin de ligne.<br/>
 * \tattendu `map.bourg.place.name` ; la clé nouvelle a le texte donné partout ; la clé reprise
 * a la traduction de chaque langue ; une clé déjà là n'est pas réécrite.
 * }
 */
TEST(ContentCheckTest, LesCataloguesSeCompletentSansRienPerdre) {
    EXPECT_EQ(hmi::mapNameKey("bourg/place"), "map.bourg.place.name");
    EXPECT_EQ(hmi::mapNameKey("donjon"), "map.donjon.name");

    const Projet projet;
    projet.catalogue("fr", "# Catalogue\nmap.a.name = Ancienne");
    projet.catalogue("en", "map.a.name = Old\n");
    const std::filesystem::path dossier = hmi::localizationDirectory(projet.racine());

    ASSERT_TRUE(hmi::addTranslation(dossier, "map.b.name", "b"));
    ASSERT_TRUE(hmi::addTranslation(dossier, "map.c.name", "c", "map.a.name"));
    ASSERT_TRUE(hmi::addTranslation(dossier, "map.a.name", "a"));

    EXPECT_EQ(lire(dossier / "fr.lang"),
              "# Catalogue\nmap.a.name = Ancienne\nmap.b.name = b\nmap.c.name = Ancienne\n");
    EXPECT_EQ(lire(dossier / "en.lang"), "map.a.name = Old\nmap.b.name = b\nmap.c.name = Old\n");
    const hmi::TranslationCatalogs catalogues = hmi::loadTranslationCatalogs(dossier);
    EXPECT_TRUE(hmi::languagesMissing(catalogues, "map.c.name").empty());
    EXPECT_EQ(hmi::languagesMissing(catalogues, "map.z.name"),
              (std::vector<std::string>{"en", "fr"}));
}

/**
 * @brief Une carte créée, renommée ou dupliquée porte son nom en clé, que les catalogues ont :
 *        elle passe le contrôle telle quelle (`EX-EDIT-077`, `LOT-EDITOR-07`).
 * \castest{<b>Une carte neuve a son nom dans chaque catalogue.</b><br/>
 * \tcat Unitaire · Contrôle du contenu<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Créer « Echoppe » au lieu `bourg` dans un projet à deux catalogues.<br/>
 * 2. La contrôler ; la renommer « Etal », la dupliquer.<br/>
 * \tattendu Le nom est `map.Echoppe.name`, les deux catalogues l'ont ; aucune erreur ; le nom
 * renommé est `map.Etal.name`, aux traductions de l'ancien ; la copie a sa propre clé.
 * }
 */
TEST(ContentCheckTest, UneCarteNeuveASonNomDansChaqueCatalogue) {
    const Projet projet;
    projet.catalogue("fr", "");
    projet.catalogue("en", "");
    const std::filesystem::path planche = projet.racine() / "Assets" / "Scene" / "bourg";
    std::filesystem::create_directories(planche);
    for (const char* fichier : {"manifest.json", "appearance.json"}) {
        std::filesystem::copy_file(elements() / "Assets" / "Scene" / "bourg" / fichier,
                                   planche / fichier);
    }
    const hmi::LevelFileOperations operations(projet.racine() / "Levels");

    const hmi::FileOperationResult cree = operations.create("Echoppe", 12, 8, "bourg");
    ASSERT_TRUE(cree.ok()) << cree.error;
    const core::LevelLoadResult lue = core::LevelLoader::loadFromFile(cree.path);
    ASSERT_TRUE(lue.ok()) << lue.error;
    EXPECT_EQ(lue.level->name(), "map.Echoppe.name");
    const hmi::MapCheckReport bilan = hmi::checkAllMaps(projet.racine());
    EXPECT_TRUE(bilan.ok()) << tout(bilan.findings);

    // Le catalogue anglais traduit ; le renommage reprend la traduction.
    projet.catalogue("en", "map.Echoppe.name = The stall\n");
    const hmi::FileOperationResult renomme = operations.rename(cree.path, "Etal");
    ASSERT_TRUE(renomme.ok()) << renomme.error;
    EXPECT_EQ(core::LevelLoader::loadFromFile(renomme.path).level->name(), "map.Etal.name");
    const hmi::TranslationCatalogs catalogues =
        hmi::loadTranslationCatalogs(hmi::localizationDirectory(projet.racine()));
    EXPECT_EQ(catalogues.at("en").at("map.Etal.name"), "The stall");
    EXPECT_EQ(catalogues.at("fr").at("map.Etal.name"), "Echoppe");

    const hmi::FileOperationResult copie = operations.duplicate(renomme.path);
    ASSERT_TRUE(copie.ok()) << copie.error;
    EXPECT_EQ(core::LevelLoader::loadFromFile(copie.path).level->name(),
              hmi::mapNameKey(copie.path.stem().string()));
    EXPECT_TRUE(hmi::checkAllMaps(projet.racine()).ok());
}
