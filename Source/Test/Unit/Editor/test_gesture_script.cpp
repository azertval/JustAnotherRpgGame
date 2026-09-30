// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_gesture_script.cpp
 * @brief Tests de l'éditeur sans fenêtre (`LOT-EDITOR-13`) : un scénario `--apply` par outil,
 *        comparé à un fichier attendu, les gestes refusés, et l'acceptation du lot — une rue de
 *        la carte d'essai effacée puis retracée la rend, octet pour octet.
 *
 * Les scénarios vivent dans `Source/Test/Fixtures/Gestures/` : `<outil>.json` rejoué sur
 * `terrain.json` doit rendre `<outil>.attendu.json`. Un changement voulu se régénère par l'éditeur
 * lui-même, puis se relit en diff :
 *
 * @code
 * LevelEditor --data Source/Test/Fixtures/GameData --apply Source/Test/Fixtures/Gestures/paint.json
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
#include <vector>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Editor/Logic/EditorSidecar.h"
#include "Editor/Logic/GestureScript.h"
#include "Editor/Logic/MapFormat.h"

namespace {

using nlohmann::json;

// La racine d'essai de l'éditeur (`LOT-123`) : ces scénarios se rejouaient sur la planche et
// la carte LIVRÉES, que la table rase du `LOT-102` emporte.
[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path(JADG_TEST_DATA_DIR);
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
    Terrain carte{.lieu = hmi::loadPlaceAssets(dataRoot(), "bourg"),
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
 * @brief Acceptation du lot : la rue du nord de la carte d'essai, effacée puis retracée par
 *        `--apply` — pavés, seuils, façade, portes, fenêtres, lanternes — rend la carte.
 *
 * Les gestes sont ceux de la main dans la fenêtre : une sélection gommée par couche, un rectangle
 * de pavés, un trait par variante de pavé, une ligne de façade, un trait de fenêtres, de portes et
 * de lanternes. Qu'ils rendent le fichier tel qu'il est sur disque montre que les outils posent
 * la pièce, le type de sa case et la collision comme la carte les porte.
 * \castest{<b>Une rue refaite par --apply rend la carte, octet pour octet.</b><br/>
 * \tcat Unitaire · Editeur · Sans fenetre<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Rejouer `Fixtures/Gestures/rue.json` sur `bourg/place`.<br/>
 * \tattendu Dix gestes, dix pas d'annulation ; le texte égale `place.json` octet pour octet.
 * }
 */
TEST(GestureScriptTest, UneRueRefaiteRendLaCarteALOctet) {
    std::filesystem::path carte;
    const hmi::GestureFileResult rendu =
        hmi::applyGestureFile(gestures() / "rue.json", {}, dataRoot(), carte);
    ASSERT_TRUE(rendu.script.ok()) << rendu.script.error;
    EXPECT_EQ(rendu.mapId, "bourg/place");
    EXPECT_EQ(rendu.script.gestures, 10U);
    EXPECT_EQ(rendu.script.steps, 10U);
    EXPECT_EQ(rendu.mapText, lire(dataRoot() / "Levels" / "bourg" / "place.json"));
}

/**
 * @brief Acceptation du `LOT-143` : le sable de l'Arena of Fate porte le groupe de quatre contre
 *        les six bandits, la rencontre pèse ce que le Guide du Maître dit ; réduit de moitié par sa
 *        poignée, le sable perd la formation et son verdict passe au rouge (`EX-EDIT-101`).
 *
 * Le verdict est celui que le canevas écrit à côté de l'entité, pendant qu'on tire comme après
 * (`hmi::entityVerdicts`) : l'outil `inspect` le verse au compte rendu.
 * \castest{<b>Le sable de l'Arena of Fate porte quatre contre six, et rougit réduit de
 * moitié.</b><br/>
 * \tcat Unitaire · Editeur · Sans fenetre<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Rejouer `Fixtures/Gestures/combat-zone.json` sur la carte livrée : inspecter le
 *          sable, puis la rencontre au niveau 1, tirer la poignée sud-est du sable de 22 à 11
 *          colonnes, inspecter le sable.<br/>
 * \tattendu Le sable : 4 contre 6, quatre places, 244 cases pour 40 ; la rencontre : 300 PX
 *           modifiés, « difficile » pour quatre niveaux 1 ; les cinq rencontres de la série de
 *           l'arène (`LOT-142`) tiennent aussi ; réduit, le sable est refusé : les six marqueurs
 *           et leurs combattants hors de la zone. Un seul pas d'annulation.
 * }
 */
TEST(GestureScriptTest, LeSableDeLArenaOfFatePorteLeGroupeEtRougitReduit) {
    std::filesystem::path carte;
    const hmi::GestureFileResult rendu =
        hmi::applyGestureFile(gestures() / "combat-zone.json", {},
                              std::filesystem::path(JADG_LEVELS_DIR).parent_path(), carte);
    ASSERT_TRUE(rendu.script.ok()) << rendu.script.error;
    EXPECT_EQ(rendu.script.gestures, 4U);
    EXPECT_EQ(rendu.script.steps, 1U);
    // Le sable porte la demo et la serie de l'arene (LOT-142) : six rencontres.
    const std::vector<std::string> attendu{
        "inspect e4: sable: 22 x 14, 244 free cells of 308, 0 arena entries inside, 0 outside.",
        "inspect e4: arene-bandits: 4 vs 6, party places 4/4, 244 free cells reached (40 "
        "required).",
        "inspect e4: arene-gladiateurs: 4 vs 7, party places 4/4, 244 free cells reached (44 "
        "required).",
        "inspect e4: arene-morts: 4 vs 9, party places 4/4, 244 free cells reached (52 "
        "required).",
        "inspect e4: arene-veteran: 4 vs 5, party places 4/4, 244 free cells reached (36 "
        "required).",
        "inspect e4: arene-capitaine: 4 vs 7, party places 4/4, 244 free cells reached (44 "
        "required).",
        "inspect e4: arene-champion: 4 vs 6, party places 4/4, 244 free cells reached (40 "
        "required).",
        "inspect e6: arene-bandits: 300 XP adjusted (6 foes, 150 XP x 2), difficile for 4 of "
        "level 1 (facile 100, moyenne 200, difficile 300, mortelle 400).",
        "inspect e6: arene-bandits: 4 vs 6, party places 4/4, 244 free cells reached (40 "
        "required).",
        "inspect e4 [refused]: sable: 11 x 14, 122 free cells of 154, 0 arena entries inside, 0 "
        "outside.",
        "inspect e4 [refused]: arene-bandits: 4 vs 6, party places 4/4, 122 free cells reached "
        "(40 required).",
        "inspect e4 [refused]: Encounter \"arene-bandits\": its marker stands outside combat "
        "zone \"sable\".",
    };
    std::string journal;
    for (const std::string& ligne : rendu.script.log) {
        journal += ligne + "\n";
    }
    // Les six rencontres et leurs 40 combattants : 12 lignes exactes, puis une ligne par
    // combattant hors de la zone, et pour les cinq autres rencontres le verdict et le marqueur.
    ASSERT_EQ(rendu.script.log.size(), 62U) << journal;
    for (std::size_t ligne = 0; ligne < attendu.size(); ++ligne) {
        EXPECT_EQ(rendu.script.log[ligne], attendu[ligne]);
    }
    // Le sable reduit refuse tout : chaque combattant hors de la zone, chaque marqueur aussi.
    std::size_t marqueurs = 1;
    for (std::size_t ligne = attendu.size(); ligne < rendu.script.log.size(); ++ligne) {
        const std::string& texte = rendu.script.log[ligne];
        EXPECT_NE(texte.find("inspect e4 [refused]: "), std::string::npos) << texte;
        const bool horsZone =
            texte.find("would stand outside combat zone \"sable\"") != std::string::npos;
        const bool marqueur =
            texte.find("its marker stands outside combat zone \"sable\"") != std::string::npos;
        const bool verdict = texte.find("122 free cells reached") != std::string::npos;
        EXPECT_TRUE(horsZone || marqueur || verdict) << texte;
        marqueurs += marqueur ? 1U : 0U;
    }
    EXPECT_EQ(marqueurs, 6U) << journal;
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
