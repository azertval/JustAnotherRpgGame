// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_quest_map_editor.cpp
 * @brief Tests du côté éditeur de ce que la quête demande aux cartes (`LOT-126`) : l'état de
 *        partie, le canevas qui grise ce qu'il rend absent, l'inspecteur qui propose les valeurs
 *        déclarées, et `--check` sur un escalier condamné et un transfert.
 */

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/Level.h"
#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelWriter.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/EntityPresence.h"
#include "Core/World/WorldGraph.h"
#include "Editor/Logic/CanvasScene.h"
#include "Editor/Logic/ContentCheck.h"
#include "Editor/Logic/EntityReferences.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/WorldGraphLayout.h"
#include "Editor/Logic/WorldState.h"
#include "HMI/Graphics/PlaceAppearance.h"

namespace {

using hmi::MapCheckSeverity;

const core::QuestFlag POMMES{.id = "quete.pommes",
                             .values = {"inconnue", "acceptee", "condamne", "enfant-libere"},
                             .initial = "inconnue"};

// La quete des pommes, en fichier : elle declare le drapeau a cinq valeurs.
constexpr const char* QUETE = R"({ "id": "pommes",
  "flags": [ { "id": "quete.pommes",
               "values": ["inconnue", "acceptee", "condamne", "enfant-libere"],
               "initial": "inconnue" } ],
  "steps": [ { "id": "acceptee", "when": [ { "flag": "quete.pommes", "equals": "acceptee" } ] } ]
})";

// Un projet jetable : des cartes, leurs noms, la quete des pommes.
class Projet {
public:
    Projet() {
        _racine =
            std::filesystem::temp_directory_path() / ("jadg-lot126-" + std::to_string(std::rand()));
        std::filesystem::create_directories(_racine / "Levels");
        std::filesystem::create_directories(_racine / "Localization");
        std::filesystem::create_directories(_racine / "World" / "quests");
        std::ofstream(_racine / "World" / "quests" / "pommes.json", std::ios::binary) << QUETE;
        std::ofstream(_racine / "Localization" / "fr.lang", std::ios::binary)
            << "map.parvis.name = Parvis\nmap.arene.name = Arene\n";
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

    void carte(const std::string& id, const std::string& json) const {
        const core::LevelLoadResult lu = core::LevelLoader::loadFromString(json);
        ASSERT_TRUE(lu.ok()) << id << " : " << lu.error;
        std::ofstream file(_racine / "Levels" / (id + ".json"), std::ios::binary);
        file << core::LevelWriter::toJsonString(*lu.level);
    }

private:
    std::filesystem::path _racine;
};

[[nodiscard]] std::string tout(const std::vector<hmi::MapCheckFinding>& constats) {
    std::string texte;
    for (const hmi::MapCheckFinding& constat : constats) {
        texte += hmi::formatFinding(constat) + "\n";
    }
    return texte;
}

// Le parvis : un couloir de 6 x 1, la zone (3, 0) qui transfere au vestiaire A sous `acceptee`.
constexpr const char* PARVIS = R"({"version": 4, "name": "map.parvis.name", "width": 6,
  "height": 1, "nextEntityId": 2, "tiles": [ {"x": 0, "y": 0, "type": "entry"} ],
  "entities": [
    {"id": "e1", "type": "zone", "x": 3, "y": 0, "width": 1, "height": 1,
     "triggerFlag": "quete.pommes", "triggerValue": "condamne",
     "triggerMap": "arene", "triggerArrival": "vestiaire-a",
     "presenceFlag": "quete.pommes", "presenceValue": "acceptee"} ]})";

// L'arene : l'entree a gauche d'un mur ; le vestiaire A, un maitre d'arene et l'escalier des
// catacombes a droite, que seul le transfert dessert.
[[nodiscard]] std::string arene(bool scelle) {
    return std::string{R"({"version": 4, "name": "map.arene.name", "width": 6, "height": 1,
  "nextEntityId": 4, "tiles": [ {"x": 0, "y": 0, "type": "entry"},
    {"x": 1, "y": 0, "type": "wall"} ],
  "entities": [
    {"id": "e1", "type": "spawnPoint", "x": 3, "y": 0, "name": "vestiaire-a"},
    {"id": "e2", "type": "npc", "x": 4, "y": 0},
    {"id": "e3", "type": "portal", "x": 5, "y": 0)"} +
           (scelle ? R"(, "sealed": true)" : "") + "} ]}";
}

[[nodiscard]] core::MapEntity pnj(std::string id, core::GridPosition position,
                                  std::string valeurs) {
    return core::MapEntity{
        .type = std::string{core::NPC_ENTITY_TYPE},
        .position = position,
        .properties = {{std::string{core::NPC_FIGURE_PROPERTY}, std::string{"citizen"}},
                       {std::string{core::PRESENCE_FLAG_PROPERTY}, std::string{"quete.pommes"}},
                       {std::string{core::PRESENCE_VALUE_PROPERTY}, std::move(valeurs)}},
        .id = std::move(id)};
}

}  // namespace

/**
 * @brief L'état de partie s'écrit comme `--flags=` et se relit contre les déclarations.
 * \castest{<b>L'etat de partie se lit comme la ligne de commande du jeu.</b><br/>
 * \tcat Unitaire · Editeur<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire `vu`, `quete.pommes=acceptee`, `quete.pommes=faux`, `quete.pommes`.<br/>
 * \tattendu `vu` est un fait ; le drapeau vaut `acceptee` ; `faux` et le fait pose sur un drapeau
 * declare sont refuses ; sans entree, le drapeau vaut son initiale.
 * }
 */
TEST(QuestMapEditorTest, LEtatDePartieSeLitCommeLaLigneDeCommande) {
    EXPECT_EQ(hmi::parseWorldStateEntry("a=b"),
              (hmi::WorldStateEntry{.flag = "a", .value = std::string{"b"}}));
    const hmi::WorldStateFlags etat = hmi::worldStateFlags(
        {"vu", "quete.pommes=acceptee", "quete.pommes=faux", "quete.pommes"}, {POMMES});
    EXPECT_TRUE(etat.flags.isSet("vu"));
    EXPECT_EQ(etat.flags.value("quete.pommes"), "acceptee");
    EXPECT_EQ(etat.refused, (std::vector<std::string>{"quete.pommes=faux", "quete.pommes"}));
    EXPECT_EQ(hmi::worldStateFlags({}, {POMMES}).flags.value("quete.pommes"), "inconnue");
    EXPECT_EQ(hmi::worldStateValue({"quete.pommes=acceptee", "vu"}, "quete.pommes"), "acceptee");
}

/**
 * @brief Critère 1 : le garde et l'enfant paraissent sous `acceptee` et disparaissent sous
 *        `enfant-libere`, au canevas, sans recharger le brouillon.
 * \castest{<b>Le canevas montre la carte sous un etat de partie.</b><br/>
 * \tcat Unitaire · Editeur<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Un brouillon portant le garde et l'enfant, presents sous `acceptee`.<br/>2. Composer
 * le canevas sans etat, sous `acceptee`, puis sous `enfant-libere`.<br/>
 * \tattendu Deux figurines sans etat et sous `acceptee` ; aucune sous `enfant-libere` ;
 * `presenceUnder` le dit entite par entite.
 * }
 */
TEST(QuestMapEditorTest, LeCanevasGriseCeQueLEtatRendAbsent) {
    core::LevelDraft brouillon = core::LevelDraft::empty("parvis", 6, 4);
    ASSERT_TRUE(brouillon.placeEntity(pnj({}, {1, 1}, "acceptee")));
    ASSERT_TRUE(brouillon.placeEntity(pnj({}, {2, 1}, "acceptee|persuasion-echouee")));
    const hmi::PlaceAppearance apparence;

    EXPECT_EQ(hmi::canvasSnapshot(brouillon, apparence).figures.size(), 2U);
    const hmi::WorldStateFlags acceptee = hmi::worldStateFlags({"quete.pommes=acceptee"}, {POMMES});
    EXPECT_EQ(hmi::canvasSnapshot(brouillon, apparence, &acceptee.flags).figures.size(), 2U);
    const hmi::WorldStateFlags libere =
        hmi::worldStateFlags({"quete.pommes=enfant-libere"}, {POMMES});
    EXPECT_TRUE(hmi::canvasSnapshot(brouillon, apparence, &libere.flags).figures.empty());
    EXPECT_EQ(hmi::presenceUnder(brouillon.entities(), libere.flags),
              (std::vector<bool>{false, false}));
}

/**
 * @brief L'inspecteur propose les valeurs que la quête déclare, et les drapeaux qu'une zone pose.
 * \castest{<b>L'inspecteur propose les valeurs declarees.</b><br/>
 * \tcat Unitaire · Editeur<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Des references dont une quete declare `quete.pommes`.<br/>2. Demander les choix de
 * `presenceValue` d'un PNJ conditionne, et le contexte d'un brouillon dont une zone pose `vu`.<br/>
 * \tattendu Les quatre valeurs ; `vu` compte parmi les drapeaux poses.
 * }
 */
TEST(QuestMapEditorTest, LInspecteurProposeLesValeursDeclarees) {
    hmi::EditorReferences references;
    references.declaredFlags = {POMMES};
    references.flags = {"quete.pommes"};
    const core::MapEntity zone{
        .type = std::string{core::ZONE_ENTITY_TYPE},
        .position = {0, 0},
        .properties = {{std::string{core::ZONE_TRIGGER_FLAG_PROPERTY}, std::string{"vu"}}}};
    const core::MapEntity garde = pnj("e1", {1, 1}, "acceptee");
    const core::EntityReferenceContext contexte =
        hmi::referenceContext(references, "parvis", {zone, garde});
    EXPECT_TRUE(contexte.flags.contains("vu"));

    const core::EntityKind* const kind = core::findEntityKind(core::NPC_ENTITY_TYPE);
    ASSERT_NE(kind, nullptr);
    const core::EntityPropertySpec* const valeur =
        core::findInspectedProperty(*kind, core::PRESENCE_VALUE_PROPERTY);
    ASSERT_NE(valeur, nullptr);
    EXPECT_EQ(hmi::entityChoices(*valeur, garde, contexte),
              (std::vector<std::string>{"acceptee", "condamne", "enfant-libere", "inconnue"}));
}

/**
 * @brief Critères 2 et 3 : `--check` passe sur une carte à l'escalier condamné, échoue sans
 *        `sealed` ; il atteint le vestiaire A par le transfert de la zone du parvis.
 * \castest{<b>--check accepte un escalier condamne et suit un transfert.</b><br/>
 * \tcat Unitaire · Controle du contenu<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Un projet : la quete des pommes, le parvis (zone qui transfere au vestiaire A sous
 * `acceptee`), l'arene (vestiaire A, un PNJ et l'escalier condamne derriere un mur).<br/>2. Lancer
 * `--check` ; retirer `sealed` et relancer.<br/>
 * \tattendu Premier controle vert, sans avertissement : le vestiaire et le PNJ sont atteints, le
 * vestiaire est nomme. Sans `sealed` : code 1, le portail n'a pas de cible.
 * }
 */
TEST(QuestMapEditorTest, LeControleSuitLeTransfertEtAccepteLEscalierCondamne) {
    const Projet projet;
    projet.carte("parvis", PARVIS);
    projet.carte("arene", arene(true));

    const hmi::MapCheckReport bilan = hmi::checkAllMaps(projet.racine());
    EXPECT_TRUE(bilan.ok()) << tout(bilan.findings);
    EXPECT_EQ(bilan.count(MapCheckSeverity::Warning), 0U) << tout(bilan.findings);

    projet.carte("arene", arene(false));
    std::string sortie;
    EXPECT_EQ(hmi::runMapCommand({"--check", "--data", projet.racine().string()}, {}, sortie), 1);
    EXPECT_NE(sortie.find("targetMap"), std::string::npos) << sortie;
}

/**
 * @brief Le graphe des cartes montre un portail condamné en pointillé, sans erreur.
 * \castest{<b>Un portail condamne n'est pas une fleche cassee.</b><br/>
 * \tcat Unitaire · Graphe du monde<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Disposer le graphe d'une carte portant un escalier condamne sans cible.<br/>
 * \tattendu Une fleche vers le fantome sans cible, condamnee et non cassee.
 * }
 */
TEST(QuestMapEditorTest, LeGrapheMontreLePortailCondamneEnPointille) {
    const core::MapEntity escalier{
        .type = std::string{core::PORTAL_ENTITY_TYPE},
        .position = {5, 0},
        .properties = {{std::string{core::PORTAL_SEALED_PROPERTY}, true}}};
    const hmi::WorldGraphLayout disposition = hmi::layoutWorldGraph(
        core::buildWorldGraph({core::WorldMapInput{.mapId = "arene", .entities = {escalier}}}));
    ASSERT_EQ(disposition.edges.size(), 1U);
    EXPECT_TRUE(disposition.edges[0].sealed);
    EXPECT_FALSE(disposition.edges[0].broken);
}

/**
 * @brief Un décor présent se compose ; en maquette, un décor qui arrête le pas s'extrude en mur.
 * \castest{<b>La porte close se voit, meme en maquette.</b><br/>
 * \tcat Unitaire · Composition<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Un brouillon sans lieu portant une porte `prop` de 1 x 2, presente sous
 * `condamne`.<br/>2. Composer le canevas sous `condamne`, puis sous `enfant-libere`.<br/>
 * \tattendu Sous `condamne`, ses deux cases sont des murs de decor ; sous `enfant-libere`, rien.
 * }
 */
TEST(QuestMapEditorTest, LaPorteCloseSeVoitMemeEnMaquette) {
    core::LevelDraft brouillon = core::LevelDraft::empty("arene", 4, 4);
    ASSERT_TRUE(brouillon.placeEntity(core::MapEntity{
        .type = std::string{core::PROP_ENTITY_TYPE},
        .position = {2, 1},
        .properties = {{std::string{core::PROP_PIECE_PROPERTY}, std::string{"gate"}},
                       {std::string{core::SHAPE_WIDTH_PROPERTY}, std::int64_t{1}},
                       {std::string{core::SHAPE_HEIGHT_PROPERTY}, std::int64_t{2}},
                       {std::string{core::PRESENCE_FLAG_PROPERTY}, std::string{"quete.pommes"}},
                       {std::string{core::PRESENCE_VALUE_PROPERTY}, std::string{"condamne"}}}}));
    const hmi::PlaceAppearance apparence;
    const hmi::WorldStateFlags close = hmi::worldStateFlags({"quete.pommes=condamne"}, {POMMES});
    const hmi::WorldSceneSnapshot fermee = hmi::canvasSnapshot(brouillon, apparence, &close.flags);
    EXPECT_EQ(fermee.reliefTypeAt({2, 1}), core::TileType::Wall);
    EXPECT_EQ(fermee.reliefTypeAt({2, 2}), core::TileType::Wall);
    EXPECT_EQ(fermee.reliefTypeAt({2, 3}), core::TileType::Empty);

    const hmi::WorldStateFlags ouverte =
        hmi::worldStateFlags({"quete.pommes=enfant-libere"}, {POMMES});
    EXPECT_EQ(hmi::canvasSnapshot(brouillon, apparence, &ouverte.flags).reliefTypeAt({2, 1}),
              core::TileType::Empty);
}
