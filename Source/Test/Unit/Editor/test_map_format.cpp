// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_map_format.cpp
 * @brief Tests de la garde du format v4 (`LOT-EDITOR-12`) : migration d'une carte, contrôle de
 *        toutes, entrée sans fenêtre de l'éditeur.
 */

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/BattleGrid.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelWriter.h"
#include "Editor/Logic/MapFormat.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

using hmi::MapCheckSeverity;

/// La racine des données livrées : `Source/Elements`.
[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path(JADG_LEVELS_DIR).parent_path();
}

[[nodiscard]] std::filesystem::path fixtures() {
    return std::filesystem::path(JADG_TEST_FIXTURES_DIR) / "Levels";
}

[[nodiscard]] core::Level charger(const std::string& json) {
    core::LevelLoadResult loaded = core::LevelLoader::loadFromString(json);
    EXPECT_TRUE(loaded.ok()) << loaded.error;
    return std::move(*loaded.level);
}

[[nodiscard]] core::Level chargerFichier(const std::filesystem::path& path) {
    core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(path);
    EXPECT_TRUE(loaded.ok()) << loaded.error;
    return std::move(*loaded.level);
}

// L'instantane de scene que le jeu dessine : pieces, figurines, lieu.
[[nodiscard]] hmi::WorldSceneSnapshot instantane(const core::Level& level) {
    const std::string lieu = hmi::scenePlaceOf(level);
    const hmi::PlaceAppearanceResult table = hmi::PlaceAppearance::loadFromFile(
        dataRoot() / "Assets" / "Scene" / lieu / "appearance.json");
    return hmi::snapshotWorldScene(level, table.appearance, hmi::npcFigures(level.entities(), 0));
}

// Vrai si deux cartes donnent la meme grille tactique : obstacles au sol et en vol, vue, terrain
// difficile et zones, case par case.
[[nodiscard]] testing::AssertionResult memeGrilleTactique(const core::Level& gauche,
                                                          const core::Level& droite) {
    const core::BattleGrid a(gauche);
    const core::BattleGrid b(droite);
    if (a.width() != b.width() || a.height() != b.height()) {
        return testing::AssertionFailure() << "dimensions differentes";
    }
    for (int row = 0; row < a.height(); ++row) {
        for (int column = 0; column < a.width(); ++column) {
            const core::GridPosition cell{.column = column, .row = row};
            if (a.isObstructed(cell) != b.isObstructed(cell) ||
                a.isObstructed(cell, core::Locomotion::Fly) !=
                    b.isObstructed(cell, core::Locomotion::Fly) ||
                a.blocksSight(cell) != b.blocksSight(cell) ||
                a.isDifficult(cell) != b.isDifficult(cell) ||
                a.zonesAt(cell).size() != b.zonesAt(cell).size()) {
                return testing::AssertionFailure()
                       << "case (" << column << ", " << row << ") differente";
            }
        }
    }
    return testing::AssertionSuccess();
}

// Un dossier de donnees jetable : Levels/ et, a cote, les planches livrees.
class DossierDeDonnees {
public:
    DossierDeDonnees() {
        _racine = std::filesystem::temp_directory_path() /
                  ("jadg-map-format-" + std::to_string(std::rand()));
        std::filesystem::create_directories(_racine / "Levels");
        // Le manifeste et la table suffisent : le contrôle ne lit aucune image.
        const std::filesystem::path planche = _racine / "Assets" / "Scene" / "martpart";
        std::filesystem::create_directories(planche);
        for (const char* fichier : {"manifest.json", "appearance.json"}) {
            std::filesystem::copy_file(dataRoot() / "Assets" / "Scene" / "martpart" / fichier,
                                       planche / fichier);
        }
        // Les figurines que les PNJ citent : le contrôle du contenu les cherche (LOT-EDITOR-07).
        std::filesystem::create_directories(_racine / "Assets" / "Npc");
        std::filesystem::copy_file(dataRoot() / "Assets" / "Npc" / "manifest.json",
                                   _racine / "Assets" / "Npc" / "manifest.json");
    }
    ~DossierDeDonnees() {
        std::error_code ignore;
        std::filesystem::remove_all(_racine, ignore);
    }
    DossierDeDonnees(const DossierDeDonnees&) = delete;
    DossierDeDonnees& operator=(const DossierDeDonnees&) = delete;
    DossierDeDonnees(DossierDeDonnees&&) = delete;
    DossierDeDonnees& operator=(DossierDeDonnees&&) = delete;

    [[nodiscard]] const std::filesystem::path& racine() const {
        return _racine;
    }

    void ecrire(const std::string& carte, const std::string& texte) const {
        std::ofstream file(_racine / "Levels" / (carte + ".json"), std::ios::binary);
        file << texte;
    }

private:
    std::filesystem::path _racine;
};

// Une variable d'environnement, sans l'avertissement de MSVC sur getenv.
[[nodiscard]] std::optional<std::string> variable(const char* name) {
#ifdef _MSC_VER
    char* value = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
        return std::nullopt;
    }
    std::string copy{value};
    std::free(value);  // NOLINT(cppcoreguidelines-no-malloc) : allouee par _dupenv_s
    return copy;
#else
    const char* value = std::getenv(name);  // NOLINT(concurrency-mt-unsafe)
    return value == nullptr ? std::nullopt : std::optional<std::string>{value};
#endif
}

[[nodiscard]] bool signale(const std::vector<hmi::MapCheckFinding>& constats,
                           MapCheckSeverity gravite, const std::string& fragment) {
    return std::ranges::any_of(constats, [&](const hmi::MapCheckFinding& constat) {
        return constat.severity == gravite && constat.message.find(fragment) != std::string::npos;
    });
}

}  // namespace

/**
 * @brief Migrer une carte v3 nomme ses pièces, donne ses identifiants, force les cases où la
 *        collision s'écarte des pièces — et le jeu la joue à l'identique.
 * \castest{<b>La migration ne change pas ce que le jeu joue.</b><br/>
 * \tcat Unitaire · Format v4<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Migrer `format-v3.json` avec la planche de Martpart.<br/>2. Relire la v4.<br/>
 * \tattendu Même instantané de scène, même grille tactique ; pièces du sol nommées ; deux
 * identifiants ; sept cases de vide forcées.
 * }
 */
TEST(MapFormatTest, LaMigrationNeChangePasCeQueLeJeuJoue) {
    const core::Level v3 = chargerFichier(fixtures() / "format-v3.json");

    const hmi::MapMigration migration =
        hmi::migrateLevel(v3, hmi::loadPlaceAssets(dataRoot(), "martpart"));
    ASSERT_TRUE(migration.ok()) << migration.error;
    const core::Level v4 = charger(migration.text);

    EXPECT_EQ(instantane(v4), instantane(v3));
    EXPECT_TRUE(memeGrilleTactique(v4, v3));
    EXPECT_EQ(migration.namedPieces, 3U) << "les trois cases de sol";
    EXPECT_EQ(v4.layers()[1].pieceAt(2, 0), "square");
    EXPECT_EQ(migration.newIds, 2U);
    EXPECT_EQ(v4.entities()[1].id, "e2");
    EXPECT_EQ(v4.nextEntityId(), 3);
    EXPECT_EQ(migration.newForcedCells, 7U) << "le vide que la v3 laissait franchissable";
}

/**
 * @brief Migrer une carte déjà migrée ne change rien.
 * \castest{<b>La migration est idempotente.</b><br/>
 * \tcat Unitaire · Format v4<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Migrer `format-v3.json`.<br/>2. Relire et migrer encore.<br/>
 * \tattendu Même texte ; rien de nommé, donné ni forcé la seconde fois.
 * }
 */
TEST(MapFormatTest, LaMigrationEstIdempotente) {
    const hmi::PlaceAssets planche = hmi::loadPlaceAssets(dataRoot(), "martpart");
    const hmi::MapMigration premiere =
        hmi::migrateLevel(chargerFichier(fixtures() / "format-v3.json"), planche);
    const hmi::MapMigration seconde = hmi::migrateLevel(charger(premiere.text), planche);

    EXPECT_EQ(seconde.text, premiere.text);
    EXPECT_EQ(seconde.namedPieces + seconde.newIds + seconde.newForcedCells, 0U);
}

/**
 * @brief Les cartes livrées passent le contrôle : c'est ce que la CI exige.
 * \castest{<b>Les cartes livrées passent le contrôle.</b><br/>
 * \tcat Unitaire · Format v4<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Contrôler `Source/Elements/Levels`.<br/>
 * \tattendu Au moins trois cartes, aucune erreur.
 * }
 */
TEST(MapFormatTest, LesCartesLivreesPassentLeControle) {
    const hmi::MapCheckReport bilan = hmi::checkAllMaps(dataRoot());

    EXPECT_GE(bilan.maps, 3U);
    for (const hmi::MapCheckFinding& constat : bilan.findings) {
        EXPECT_NE(constat.severity, MapCheckSeverity::Error) << hmi::formatFinding(constat);
    }
}

/**
 * @brief Une carte fautive fait sortir chaque sorte de défaut, et `--check` échoue.
 * \castest{<b>Chaque défaut sort, et le contrôle échoue.</b><br/>
 * \tcat Unitaire · Format v4<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Écrire une v4 non canonique, sans id, à pièce absente, à collision écartée, avec
 * une hauteur et un portail vers nulle part.<br/>2. Lancer `--check`.<br/>
 * \tattendu Chaque erreur et l'avertissement de hauteur sortent ; code de sortie 1.
 * }
 */
TEST(MapFormatTest, ChaqueDefautSortEtLeControleEchoue) {
    const DossierDeDonnees donnees;
    donnees.ecrire("fautive", R"({"version": 4, "name": "fautive", "width": 3, "height": 1,
      "tiles": [ {"x": 0, "y": 0, "type": "entry"} ],
      "layers": [ {"kind": "ground", "scene": "martpart", "tiles": [
        {"x": 0, "y": 0, "type": "dirt", "piece": "street"},
        {"x": 1, "y": 0, "type": "dirt", "piece": "introuvable", "elevation": 2},
        {"x": 2, "y": 0, "type": "dirt", "piece": "wall-left"} ]} ],
      "entities": [ {"type": "portal", "x": 1, "y": 0, "targetMap": "nulle-part",
                     "arrival": "porte"} ]
    })");

    const std::vector<hmi::MapCheckFinding> constats = hmi::checkMapFile(
        "fautive", donnees.racine() / "Levels" / "fautive.json", donnees.racine());

    EXPECT_TRUE(signale(constats, MapCheckSeverity::Error, "not canonical"));
    EXPECT_TRUE(signale(constats, MapCheckSeverity::Error, "has no id"));
    EXPECT_TRUE(signale(constats, MapCheckSeverity::Error, "piece missing"));
    EXPECT_TRUE(signale(constats, MapCheckSeverity::Error, "collision differs"));
    EXPECT_TRUE(signale(constats, MapCheckSeverity::Warning, "elevation"));

    std::string sortie;
    const std::optional<int> code =
        hmi::runMapCommand({"--check", "--data", donnees.racine().string()}, {}, sortie);
    ASSERT_TRUE(code.has_value());
    EXPECT_EQ(*code, 1);
    EXPECT_NE(sortie.find("does not exist"), std::string::npos) << sortie;
}

/**
 * @brief `--migrate` réécrit une carte en place, et la carte migrée passe le contrôle.
 * \castest{<b>--migrate rend une carte que --check accepte.</b><br/>
 * \tcat Unitaire · Format v4<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Copier `format-v3.json` dans un dossier de données.<br/>2. `--migrate` puis
 * `--check`.<br/>
 * \tattendu Codes 0 et 0 ; le fichier est en version 4.
 * }
 */
TEST(MapFormatTest, MigrerRendUneCarteQueLeControleAccepte) {
    const DossierDeDonnees donnees;
    std::filesystem::copy_file(fixtures() / "format-v3.json",
                               donnees.racine() / "Levels" / "rue.json");

    std::string sortie;
    EXPECT_EQ(
        hmi::runMapCommand({"--migrate", "rue", "--data", donnees.racine().string()}, {}, sortie),
        0)
        << sortie;
    EXPECT_EQ(hmi::runMapCommand({"--check", "--data", donnees.racine().string()}, {}, sortie), 0)
        << sortie;
    std::ifstream file(donnees.racine() / "Levels" / "rue.json");
    const std::string texte((std::istreambuf_iterator<char>(file)), {});
    EXPECT_NE(texte.find("\"version\": 4"), std::string::npos);
}

/**
 * @brief Sans `--check` ni `--migrate`, l'éditeur ouvre sa fenêtre.
 * \castest{<b>Sans commande, l'éditeur ouvre sa fenêtre.</b><br/>
 * \tcat Unitaire · Format v4<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Passer `--crash-test` seul.<br/>
 * \tattendu Aucun code de sortie : la fenêtre s'ouvre.
 * }
 */
TEST(MapFormatTest, SansCommandeLEditeurOuvreSaFenetre) {
    std::string sortie;
    EXPECT_FALSE(hmi::runMapCommand({"--crash-test"}, {}, sortie).has_value());
    EXPECT_TRUE(sortie.empty());
}

/**
 * @brief Acceptation du lot, sur les trois cartes livrées : leurs versions v3, rangées dans
 *        `JADG_V3_MAPS_DIR` (tirées de l'historique git), se jouent comme leurs v4 migrées.
 * \castest{<b>Les trois cartes migrées se jouent à l'identique.</b><br/>
 * \tcat Unitaire · Format v4<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Extraire les v3 : `git show 376c541da:Source/Elements/Levels/…` (le
 * dernier commit de main avant ce lot).<br/>
 * 2. `JADG_V3_MAPS_DIR=<dossier>` puis lancer ce test.<br/>
 * \tattendu Même instantané de scène, même grille tactique, pour chaque carte. Ignoré sans la
 * variable.
 * }
 */
TEST(MapFormatTest, LesTroisCartesMigreesSeJouentALIdentique) {
    const std::optional<std::string> dossier = variable("JADG_V3_MAPS_DIR");
    if (!dossier) {
        GTEST_SKIP() << "JADG_V3_MAPS_DIR non definie : versions v3 absentes";
    }
    for (const char* carte : {"coliseum", "capital/martpart", "capital/arenarea"}) {
        SCOPED_TRACE(carte);
        const core::Level v3 =
            chargerFichier(std::filesystem::path(*dossier) / (std::string{carte} + ".json"));
        const core::Level v4 =
            chargerFichier(std::filesystem::path(JADG_LEVELS_DIR) / (std::string{carte} + ".json"));
        EXPECT_EQ(instantane(v4), instantane(v3));
        EXPECT_TRUE(memeGrilleTactique(v4, v3));
        EXPECT_EQ(v4.entry(), v3.entry());
    }
}
