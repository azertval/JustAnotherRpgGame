// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_gesture_script.cpp
 * @brief Tests de l'éditeur sans fenêtre (`LOT-EDITOR-13`) : un scénario `--apply` par outil,
 *        comparé à un fichier attendu, les gestes refusés, et l'acceptation du lot — une rue de
 *        Martpart effacée puis retracée rend la carte livrée, octet pour octet.
 *
 * Les scénarios vivent dans `Source/Test/Fixtures/Gestures/` : `<outil>.json` rejoué sur
 * `terrain.json` doit rendre `<outil>.attendu.json`. Un changement voulu se régénère par l'éditeur
 * lui-même, puis se relit en diff :
 *
 * @code
 * LevelEditor --data Source/Elements --apply Source/Test/Fixtures/Gestures/paint.json
 *     Source/Test/Fixtures/Gestures/terrain.json
 *     --output Source/Test/Fixtures/Gestures/paint.attendu.json
 * @endcode
 */

#include <array>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Editor/Logic/EditorSidecar.h"
#include "Editor/Logic/GestureScript.h"
#include "Editor/Logic/MapFormat.h"

namespace {

using nlohmann::json;

[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path(JADG_LEVELS_DIR).parent_path();
}

[[nodiscard]] std::filesystem::path gestures() {
    return std::filesystem::path(JADG_TEST_FIXTURES_DIR) / "Gestures";
}

[[nodiscard]] std::string lire(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream text;
    text << file.rdbuf();
    return text.str();
}

/// Un scénario : le nombre de gestes, et combien changent la carte (un pas d'annulation chacun).
struct Scenario {
    const char* outil;
    std::size_t gestes;
    std::size_t pas;
};

constexpr std::array<Scenario, 11> SCENARIOS = {{
    {.outil = "paint", .gestes = 2, .pas = 2},
    {.outil = "eraser", .gestes = 4, .pas = 3},
    {.outil = "rectangle", .gestes = 2, .pas = 2},
    {.outil = "line", .gestes = 2, .pas = 1},
    {.outil = "bucket", .gestes = 2, .pas = 2},
    {.outil = "pipette", .gestes = 4, .pas = 2},
    {.outil = "selection", .gestes = 4, .pas = 3},
    {.outil = "entity", .gestes = 7, .pas = 6},
    {.outil = "shape", .gestes = 5, .pas = 5},
    {.outil = "measure", .gestes = 1, .pas = 0},
    {.outil = "note", .gestes = 3, .pas = 0},
}};

/// La carte-témoin chargée, avec le manifeste de son lieu, prête à recevoir des gestes.
struct Terrain {
    hmi::PlaceAssets lieu;
    core::LevelDraft draft;
    hmi::EditorSidecar annexe;
};

[[nodiscard]] Terrain terrain() {
    core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(gestures() / "terrain.json");
    EXPECT_TRUE(loaded.ok()) << loaded.error;
    Terrain carte{.lieu = hmi::loadPlaceAssets(dataRoot(), "martpart"),
                  .draft = core::LevelDraft::fromLevel(*loaded.level),
                  .annexe = {}};
    carte.draft.setPieceManifest(
        std::make_shared<const core::ScenePieceManifest>(*carte.lieu.manifest));
    return carte;
}

/// Un fichier de gestes fait de @p gestes.
[[nodiscard]] json script(json gestes) {
    return json{{"format", hmi::GESTURE_SCRIPT_FORMAT},
                {"version", hmi::GESTURE_SCRIPT_VERSION},
                {"gestures", std::move(gestes)}};
}

/// Rejoue @p gestes sur la carte-témoin et rend le message d'échec.
[[nodiscard]] std::string refus(json gestes) {
    Terrain carte = terrain();
    return hmi::applyGestureScript(script(std::move(gestes)), carte.draft, carte.annexe, carte.lieu)
        .error;
}

}  // namespace

/**
 * @brief Chaque outil a son scénario `--apply`, et il rend le fichier attendu (règle 4 de la
 *        feuille de route : le scénario tient lieu de test d'IHM).
 * \castest{<b>Un scénario --apply par outil rend le fichier attendu.</b><br/>
 * \tcat Unitaire · Editeur · Sans fenetre<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Pour chacun des onze outils, rejouer `Fixtures/Gestures/<outil>.json` sur
 *          `terrain.json`.<br/>
 * \tattendu Le texte de la carte égale `<outil>.attendu.json` octet pour octet ; autant de pas
 *           d'annulation que de gestes qui changent la carte.
 * }
 */
TEST(GestureScriptTest, UnScenarioParOutilRendLeFichierAttendu) {
    for (const Scenario& scenario : SCENARIOS) {
        SCOPED_TRACE(scenario.outil);
        std::filesystem::path carte;
        const hmi::GestureFileResult rendu =
            hmi::applyGestureFile(gestures() / (std::string{scenario.outil} + ".json"),
                                  (gestures() / "terrain.json").string(), dataRoot(), carte);
        ASSERT_TRUE(rendu.script.ok()) << rendu.script.error;
        EXPECT_EQ(rendu.script.gestures, scenario.gestes);
        EXPECT_EQ(rendu.script.steps, scenario.pas);
        EXPECT_EQ(rendu.mapText,
                  lire(gestures() / (std::string{scenario.outil} + ".attendu.json")));
    }
}

/**
 * @brief Les outils qui ne touchent pas à la carte disent ce qu'ils ont fait : la mesure dans le
 *        compte rendu, les notes dans l'annexe.
 * \castest{<b>La mesure et les notes rendent leur compte rendu et leur annexe.</b><br/>
 * \tcat Unitaire · Editeur · Sans fenetre<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Rejouer `measure.json` puis `note.json`.<br/>
 * \tattendu La mesure écrit `7 × 4 · 6 cells = 30 ft` ; l'annexe égale
 *           `note.attendu.editor.json` : une note gardée, une retirée.
 * }
 */
TEST(GestureScriptTest, LaMesureEtLesNotesRendentLeurCompteRendu) {
    std::filesystem::path carte;
    const std::string temoin = (gestures() / "terrain.json").string();
    const hmi::GestureFileResult mesure =
        hmi::applyGestureFile(gestures() / "measure.json", temoin, dataRoot(), carte);
    ASSERT_TRUE(mesure.script.ok()) << mesure.script.error;
    ASSERT_EQ(mesure.script.log.size(), 1U);
    EXPECT_NE(mesure.script.log.front().find("7 × 4 · 6 cells = 30 ft"), std::string::npos)
        << mesure.script.log.front();
    EXPECT_FALSE(mesure.sidecar.has_value());

    const hmi::GestureFileResult notes =
        hmi::applyGestureFile(gestures() / "note.json", temoin, dataRoot(), carte);
    ASSERT_TRUE(notes.script.ok()) << notes.script.error;
    ASSERT_TRUE(notes.sidecar.has_value());
    EXPECT_EQ(hmi::sidecarJson(*notes.sidecar), lire(gestures() / "note.attendu.editor.json"));
}

/**
 * @brief Acceptation du lot : la rue d'Arenarea, à Martpart, effacée puis retracée par
 *        `--apply` — pavés, seuils, façade, portes, fenêtres, lanternes — rend la carte livrée.
 *
 * Les gestes sont ceux de la main dans la fenêtre : une sélection gommée par couche, un rectangle
 * de pavés, un trait par variante de pavé, une ligne de façade, un trait de fenêtres, de portes et
 * de lanternes. Qu'ils rendent le fichier que le script des quartiers avait écrit montre que les
 * outils posent la pièce, le type de sa case et la collision comme la carte livrée les porte.
 * \castest{<b>Une rue de Martpart refaite par --apply rend la carte livrée.</b><br/>
 * \tcat Unitaire · Editeur · Sans fenetre<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Rejouer `Fixtures/Gestures/martpart-rue.json` sur `capital/martpart`.<br/>
 * \tattendu Dix gestes, dix pas d'annulation ; le texte égale `martpart.json` octet pour octet.
 * }
 */
TEST(GestureScriptTest, UneRueDeMartpartRefaiteRendLaCarteLivree) {
    std::filesystem::path carte;
    const hmi::GestureFileResult rendu =
        hmi::applyGestureFile(gestures() / "martpart-rue.json", {}, dataRoot(), carte);
    ASSERT_TRUE(rendu.script.ok()) << rendu.script.error;
    EXPECT_EQ(rendu.mapId, "capital/martpart");
    EXPECT_EQ(rendu.script.gestures, 10U);
    EXPECT_EQ(rendu.script.steps, 10U);
    EXPECT_EQ(rendu.mapText, lire(dataRoot() / "Levels" / "capital" / "martpart.json"));
}

/**
 * @brief Un geste que la fenêtre refuserait arrête le fichier, et le message nomme le geste.
 * \castest{<b>Un geste refusé rend une erreur lisible.</b><br/>
 * \tcat Unitaire · Editeur · Sans fenetre<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Rejouer un geste sur une couche verrouillée, hors de la carte, avec une pièce
 *          inconnue, sur un identifiant absent, un déplacement qui sort une entité, un outil
 *          inconnu, un fichier d'un autre format.<br/>
 * \tattendu Chaque fois, une erreur qui nomme le geste et sa raison.
 * }
 */
TEST(GestureScriptTest, UnGesteRefuseRendUneErreurLisible) {
    const auto contient = [](const std::string& message, const std::string& attendu) {
        EXPECT_NE(message.find(attendu), std::string::npos) << message;
    };
    contient(refus(json::array({json{{"lock", {"relief"}}},
                                json{{"piece", "light"}, {"tool", "paint"}, {"at", {6, 3}}}})),
             "gesture 2 (paint): The decor layer is locked.");
    contient(refus(json::array({json{{"piece", "light"}, {"tool", "bucket"}, {"at", {12, 3}}}})),
             "gesture 1 (bucket): cell [12, 3] is outside the map (12 × 10)");
    contient(refus(json::array({json{{"piece", "fontaine"}, {"tool", "paint"}, {"at", {1, 1}}}})),
             "piece \"fontaine\" is not on the sheet of this place");
    contient(refus(json::array({json{{"tool", "entity"}, {"select", {"e9"}}, {"then", "delete"}}})),
             "no entity with id \"e9\"");
    // Le groupe du point d'arrivée (1, 1) et du coffre (10, 8), glissé de deux cases vers la
    // droite : le coffre sortirait de la carte.
    contient(refus(json::array({json{{"tool", "entity"}, {"select", {"e1", "e4"}}},
                                json{{"tool", "entity"}, {"at", {1, 1}}, {"to", {3, 1}}}})),
             "gesture 2 (entity): move refused: an entity would leave the map");
    contient(refus(json::array({json{{"tool", "lasso"}, {"at", {1, 1}}}})),
             "unknown tool \"lasso\"");
    Terrain carte = terrain();
    EXPECT_NE(
        hmi::applyGestureScript(json{{"format", "autre"}}, carte.draft, carte.annexe, carte.lieu)
            .error.find("not a gesture file"),
        std::string::npos);
}

/**
 * @brief `--apply` refusé n'écrit rien ; accepté, il écrit la carte et son annexe.
 * \castest{<b>Un geste refusé ne touche pas au fichier.</b><br/>
 * \tcat Unitaire · Editeur · Sans fenetre<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. `LevelEditor --apply` d'un fichier dont le deuxième geste est refusé, avec
 *          `--output`.<br/>2. Le même, avec des notes seulement.<br/>
 * \tattendu Code 1, « nothing written », aucun fichier écrit ; puis code 0, la carte et son annexe
 *           écrites.
 * }
 */
TEST(GestureScriptTest, UnGesteRefuseNeTouchePasAuFichier) {
    const std::filesystem::path dossier =
        std::filesystem::temp_directory_path() / ("jadg-gestes-" + std::to_string(std::rand()));
    std::filesystem::create_directories(dossier);
    const std::filesystem::path gestes = dossier / "gestes.json";
    const std::filesystem::path sortie = dossier / "carte.json";
    {
        std::ofstream file(gestes, std::ios::binary);
        file << script(json::array({json{{"piece", "light"}, {"tool", "paint"}, {"at", {6, 3}}},
                                    json{{"piece", "light"}, {"tool", "paint"}, {"at", {30, 3}}}}))
                    .dump(2);
    }
    std::string compteRendu;
    const std::string temoin = (gestures() / "terrain.json").string();
    EXPECT_EQ(hmi::runMapCommand({"--data", dataRoot().string(), "--apply", gestes.string(), temoin,
                                  "--output", sortie.string()},
                                 {}, compteRendu),
              1);
    EXPECT_NE(compteRendu.find("nothing written"), std::string::npos) << compteRendu;
    EXPECT_FALSE(std::filesystem::exists(sortie));

    compteRendu.clear();
    EXPECT_EQ(hmi::runMapCommand(
                  {"--data", dataRoot().string(), "--apply", (gestures() / "note.json").string(),
                   temoin, "--output", sortie.string()},
                  {}, compteRendu),
              0)
        << compteRendu;
    EXPECT_EQ(lire(sortie), lire(gestures() / "note.attendu.json"));
    EXPECT_EQ(lire(hmi::sidecarPath(sortie)), lire(gestures() / "note.attendu.editor.json"));

    std::error_code ignore;
    std::filesystem::remove_all(dossier, ignore);
}
