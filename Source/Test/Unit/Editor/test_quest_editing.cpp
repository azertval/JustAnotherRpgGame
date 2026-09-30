// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_quest_editing.cpp
 * @brief Tests du mode Quêtes de l'éditeur (`LOT-144`, `EX-EDIT-100`) : l'écriture canonique,
 *        l'enregistrement qui refuse ce que le jeu refuserait, les textes du journal, qui se sert
 *        d'un drapeau, les renommages par un plan, l'état de partie qui atteint une étape, et le
 *        lieu d'une étape suivi par `--check`.
 */

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Gameplay/Quest.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelWriter.h"
#include "Editor/Logic/ContentCheck.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/MapRefactor.h"
#include "Editor/Logic/MapTexts.h"
#include "Editor/Logic/QuestEditing.h"

namespace {

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

// La quête d'essai, sous sa forme canonique : c'est ce que `core::writeQuest` doit rendre.
constexpr std::string_view ESSAI = R"({
  "id": "essai",
  "name": "Quete d'essai",
  "source": "original",
  "flags": [
    {
      "id": "quete.essai",
      "values": ["inconnue", "acceptee", "garde-vu", "rendue"],
      "initial": "inconnue"
    }
  ],
  "steps": [
    {
      "id": "acceptee",
      "at": "place#e1",
      "when": [{ "flag": "quete.essai", "equals": "acceptee" }]
    },
    {
      "id": "garde-vu",
      "when": [{ "flag": "quete.essai", "notEquals": ["inconnue", "acceptee"] }, { "flag": "essai/recompense", "isSet": false }],
      "effects": [{ "type": "setFlag", "flag": "essai/recompense" }, { "type": "clearFlag", "flag": "essai/porte" }]
    },
    {
      "id": "rendue",
      "when": [{ "flag": "quete.essai", "equals": "rendue" }],
      "effects": [{ "type": "setFlag", "flag": "quete.essai", "value": "rendue" }],
      "outcome": "success"
    }
  ]
}
)";

// Le dialogue de la mère, écrit à la main : un nœud porte le même mot que la valeur `garde-vu`.
constexpr std::string_view MERE = R"({
  "id": "mere",
  "name": "La mere",
  "source": "original",
  "speaker": { "languages": ["common"], "attitude": "friendly" },
  "start": "aiguillage",
  "nodes": [
    {
      "id": "aiguillage",
      "type": "condition",
      "flag": "quete.essai",
      "equals": ["acceptee",   "garde-vu"],
      "then": "garde-vu",
      "else": "demande"
    },
    {
      "id": "demande",
      "type": "action",
      "actions": [{ "type": "startQuest", "quest": "essai" }, { "type": "setFlag", "flag": "quete.essai", "value": "acceptee" }],
      "next": "fin"
    },
    {
      "id": "garde-vu",
      "type": "action",
      "actions": [ {"type":"setFlag","flag":"quete.essai","value":"garde-vu"} ],
      "next": "fin"
    },
    { "id": "fin", "type": "end" }
  ]
}
)";

// Une place : le garde (e1) paraît sous `acceptee`, la zone (e2) pose `garde-vu` quand on y entre.
constexpr std::string_view PLACE = R"({"version": 4, "name": "map.place.name", "width": 6,
  "height": 2, "nextEntityId": 3, "tiles": [ {"x": 0, "y": 0, "type": "entry"} ],
  "entities": [
    {"id": "e1", "type": "npc", "x": 3, "y": 0,
     "presenceFlag": "quete.essai", "presenceTest": "equals", "presenceValue": "acceptee|garde-vu"},
    {"id": "e2", "type": "zone", "x": 4, "y": 1, "width": 1, "height": 1,
     "triggerFlag": "quete.essai", "triggerValue": "garde-vu"} ]})";

constexpr std::string_view FR =
    "# Essai\nmap.place.name = La place\nquest.essai.title = Essai\n"
    "quest.essai.acceptee = Voir le garde.\nautre.cle = Autre\n";
constexpr std::string_view EN = "map.place.name = The square\nquest.essai.title = Test\n";

// Un projet jetable : la place, la quête, le dialogue et deux catalogues.
class ModeQuetesProjet : public ::testing::Test {
protected:
    std::filesystem::path racine;

    void SetUp() override {
        racine = std::filesystem::temp_directory_path() /
                 ("jadg-lot144-" + std::to_string(std::random_device{}()));
        const core::LevelLoadResult place = core::LevelLoader::loadFromString(std::string{PLACE});
        ASSERT_TRUE(place.ok()) << place.error;
        ecrire(racine / "Levels" / "place.json", core::LevelWriter::toJsonString(*place.level));
        ecrire(racine / "World" / "quests" / "essai.json", std::string{ESSAI});
        ecrire(racine / "World" / "dialogues" / "mere.json", std::string{MERE});
        ecrire(racine / "Localization" / "fr.lang", std::string{FR});
        ecrire(racine / "Localization" / "en.lang", std::string{EN});
    }
    void TearDown() override {
        std::error_code ignore;
        std::filesystem::remove_all(racine, ignore);
    }

    void appliquer(const hmi::RefactorPlan& plan) const {
        ASSERT_TRUE(plan.ok()) << plan.error;
        std::string erreur;
        ASSERT_TRUE(hmi::applyRefactorPlan(plan, erreur)) << erreur;
    }

    [[nodiscard]] std::string erreursDuRecit() const {
        std::string texte;
        for (const hmi::MapCheckFinding& constat : hmi::checkStoryContent(racine)) {
            texte += hmi::formatFinding(constat) + "\n";
        }
        return texte;
    }

    [[nodiscard]] core::Quest quete() const {
        core::QuestLoad lue = core::loadQuest(racine / "World" / "quests" / "essai.json");
        EXPECT_TRUE(lue.quest.has_value());
        return lue.quest.value_or(core::Quest{});
    }
};

[[nodiscard]] hmi::QuestDraft brouillon(core::Quest quest) {
    return hmi::QuestDraft{.quest = std::move(quest), .texts = {}};
}

}  // namespace

/**
 * @brief La forme canonique : une quête lue puis réécrite rend son texte octet pour octet, champs
 *        facultatifs, conditions composées et effets compris ; la quête de la racine d'essai aussi.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>Une quête se réécrit octet pour octet.</b><br/>
 * \tcat Unitaire · Mode Quêtes<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire la quête d'essai, écrite sous forme canonique.<br/>2. La réécrire par
 * `core::writeQuest`.<br/>3. Faire de même avec la quête de la racine d'essai.<br/>
 * \tattendu Les deux textes sont identiques à leur fichier.}
 */
TEST(QuestWriting, UneQueteSeReecritOctetPourOctet) {
    const core::QuestLoad lue = core::readQuest(ESSAI, "essai.json");
    ASSERT_TRUE(lue.quest) << lue.errors.front();
    EXPECT_EQ(lue.quest->name, "Quete d'essai");
    EXPECT_EQ(lue.quest->steps.front().at, "place#e1");
    EXPECT_EQ(core::writeQuest(*lue.quest), ESSAI);

    const std::filesystem::path fixture =
        std::filesystem::path(JADG_TEST_DATA_DIR) / "World" / "quests" / "essai-trois-etapes.json";
    const core::QuestLoad essai = core::loadQuest(fixture);
    ASSERT_TRUE(essai.quest);
    EXPECT_EQ(core::writeQuest(*essai.quest), lire(fixture));
}

/**
 * @brief Un lieu d'étape mal formé est refusé au chargement.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>Un lieu d'étape s'écrit carte#entité.</b><br/>
 * \tcat Unitaire · Mode Quêtes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire une quête dont une étape a `"at": "place"`.<br/>
 * \tattendu Refusée ; l'erreur nomme l'étape et le champ.}
 */
TEST(QuestWriting, UnLieuDEtapeSEcritCarteDiezeEntite) {
    const core::QuestLoad lue = core::readQuest(
        R"({"id": "q", "steps": [{"id": "s", "at": "place", "when": [{"flag": "f"}]}]})", "q.json");
    ASSERT_FALSE(lue.quest);
    EXPECT_NE(lue.errors.front().find("etape 's' : 'at'"), std::string::npos) << lue.errors.front();
}

/**
 * @brief Enregistrer écrit la quête sous sa forme canonique et ses textes dans chaque catalogue :
 *        une clé présente change à sa place, une nouvelle se range avec celles de la quête, celle
 *        d'une étape retirée s'en va ; un second enregistrement ne change rien.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>Enregistrer écrit la quête et ses textes de journal.</b><br/>
 * \tcat Unitaire · Mode Quêtes<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Retirer l'étape « acceptee », donner le texte de « rendue » en français et en
 * anglais, changer le titre anglais.<br/>2. Enregistrer.<br/>3. Enregistrer de nouveau.<br/>
 * \tattendu La quête relue est celle du brouillon ; `quest.essai.acceptee` a quitté le catalogue
 * français, `quest.essai.rendue` suit le titre ; le second plan ne récrit aucun catalogue.}
 */
TEST_F(ModeQuetesProjet, EnregistrerEcritLaQueteEtSesTextes) {
    hmi::QuestDraft draft = brouillon(quete());
    draft.quest.steps.erase(draft.quest.steps.begin());
    draft.texts["fr"] = {{"quest.essai.title", "Essai"}, {"quest.essai.rendue", "C'est rendu."}};
    draft.texts["en"] = {{"quest.essai.title", "Trial"}, {"quest.essai.rendue", "Given back."}};

    appliquer(hmi::planSaveQuest(racine, draft, /*isNew=*/false));

    EXPECT_EQ(lire(racine / "World" / "quests" / "essai.json"), core::writeQuest(draft.quest));
    EXPECT_EQ(lire(racine / "Localization" / "fr.lang"),
              "# Essai\nmap.place.name = La place\nquest.essai.title = Essai\n"
              "quest.essai.rendue = C'est rendu.\nautre.cle = Autre\n");
    EXPECT_EQ(lire(racine / "Localization" / "en.lang"),
              "map.place.name = The square\nquest.essai.title = Trial\n"
              "quest.essai.rendue = Given back.\n");

    const hmi::RefactorPlan encore = hmi::planSaveQuest(racine, draft, /*isNew=*/false);
    ASSERT_TRUE(encore.ok()) << encore.error;
    EXPECT_EQ(encore.edits.size(), 1U);  // la quête seule, inchangée
}

/**
 * @brief Critère du `LOT-144` : une étape sans condition, ou qui compare une valeur non déclarée,
 *        ne s'enregistre pas ; l'erreur nomme l'étape. Une quête neuve ne prend pas un nom pris.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>Ce que le jeu refuserait ne s'enregistre pas.</b><br/>
 * \tcat Unitaire · Mode Quêtes<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Ajouter une étape « vide » sans condition.<br/>2. Comparer `quete.essai` à
 * `perdue`, qu'il ne déclare pas.<br/>3. Déclarer un drapeau qu'une autre quête déclare.<br/>4.
 * Créer une quête « essai ».<br/>
 * \tattendu Chaque plan est refusé et nomme l'étape, la valeur ou le drapeau ; rien n'est écrit.}
 */
TEST_F(ModeQuetesProjet, CeQueLeJeuRefuseraitNeSEnregistrePas) {
    hmi::QuestDraft vide = brouillon(quete());
    vide.quest.steps.push_back(core::QuestStep{.id = "vide"});
    const hmi::RefactorPlan sansCondition = hmi::planSaveQuest(racine, vide, false);
    ASSERT_FALSE(sansCondition.ok());
    EXPECT_NE(sansCondition.error.find("etape 'vide'"), std::string::npos) << sansCondition.error;

    hmi::QuestDraft perdue = brouillon(quete());
    perdue.quest.steps.front().when.front().values = {"perdue"};
    const hmi::RefactorPlan nonDeclaree = hmi::planSaveQuest(racine, perdue, false);
    ASSERT_FALSE(nonDeclaree.ok());
    EXPECT_NE(nonDeclaree.error.find("etape 'acceptee'"), std::string::npos) << nonDeclaree.error;
    EXPECT_NE(nonDeclaree.error.find("'perdue'"), std::string::npos);

    // Une autre quête qui compare une valeur que `quete.essai` ne déclare pas.
    hmi::QuestDraft autre{.quest = core::Quest{.id = "autre"}, .texts = {}};
    autre.quest.steps.push_back(core::QuestStep{
        .id = "fin",
        .when = {core::FlagCondition{
            .flag = "quete.essai", .test = core::FlagTest::Equals, .values = {"oubliee"}}}});
    const hmi::RefactorPlan ailleurs = hmi::planSaveQuest(racine, autre, true);
    ASSERT_FALSE(ailleurs.ok());
    EXPECT_NE(ailleurs.error.find("'oubliee'"), std::string::npos) << ailleurs.error;

    hmi::QuestDraft doublon = autre;
    doublon.quest.steps.front().when.front() = core::FlagCondition{.flag = "essai/porte"};
    doublon.quest.flags.push_back(
        core::QuestFlag{.id = "quete.essai", .values = {"a"}, .initial = "a"});
    EXPECT_FALSE(hmi::planSaveQuest(racine, doublon, true).ok());

    EXPECT_FALSE(hmi::planSaveQuest(racine, brouillon(quete()), /*isNew=*/true).ok());
    EXPECT_EQ(lire(racine / "World" / "quests" / "essai.json"), ESSAI);
}

/**
 * @brief Qui se sert d'un drapeau : la déclaration, les étapes, le dialogue (conditions et
 *        actions), la présence du garde et le déclencheur de la zone — ce dernier par la source
 *        de ses propriétés, sans code par famille.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>Qui se sert d'une valeur de drapeau.</b><br/>
 * \tcat Unitaire · Mode Quêtes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire les usages du projet.<br/>2. Garder ceux de `quete.essai = garde-vu`.<br/>
 * \tattendu Cinq usages : le garde (lit) et la zone (pose), sur leur case, la déclaration, le
 * nœud condition et l'action du dialogue ; l'étape qui compare à d'autres valeurs n'y est pas.}
 */
TEST_F(ModeQuetesProjet, QuiSeSertDUneValeurDeDrapeau) {
    const std::vector<hmi::FlagUse> usages =
        hmi::usesOfFlag(hmi::flagUses(racine), "quete.essai", "garde-vu");
    std::vector<std::string> lignes;
    for (const hmi::Citation& citation : hmi::flagUseCitations(usages)) {
        lignes.push_back(hmi::formatCitation(citation, racine));
    }
    EXPECT_EQ(lignes,
              (std::vector<std::string>{
                  "place (3, 0): npc e1: presenceFlag (reads acceptee|garde-vu)",
                  "place (4, 1): zone e2: triggerFlag (writes garde-vu)",
                  "World/quests/essai.json: quest essai: flags (declares "
                  "inconnue|acceptee|garde-vu|rendue)",
                  "World/dialogues/mere.json: dialogue mere: node aiguillage (reads "
                  "acceptee|garde-vu)",
                  "World/dialogues/mere.json: dialogue mere: node garde-vu: setFlag (writes "
                  "garde-vu)",
              }));
    // Les faits que dialogues et quêtes posent d'eux-mêmes se voient aussi.
    EXPECT_FALSE(hmi::usesOfFlag(hmi::flagUses(racine), "quest/essai/started").empty());
}

/**
 * @brief Renommer une valeur : la déclaration et son initiale, les étapes, le dialogue — chaîne par
 *        chaîne, le reste du fichier gardé —, la présence `a|b` et le déclencheur de la carte.
 *        Le nœud qui porte le même mot garde son identifiant.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>Renommer une valeur la suit partout, et seulement elle.</b><br/>
 * \tcat Unitaire · Mode Quêtes<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Renommer `garde-vu` en `vu`.<br/>2. Renommer `inconnue` (l'initiale) en `neuve`.<br/>
 * \tattendu Le dialogue ne diffère que des deux chaînes de valeur ; le nœud `garde-vu` et sa
 * cible restent ; la carte cite `acceptee|vu` ; l'initiale est `neuve` ; le récit se contrôle sans
 * erreur.}
 */
TEST_F(ModeQuetesProjet, RenommerUneValeurLaSuitPartoutEtSeulementElle) {
    appliquer(hmi::planRenameFlagValue(racine, "quete.essai", "garde-vu", "vu"));
    appliquer(hmi::planRenameFlagValue(racine, "quete.essai", "inconnue", "neuve"));

    std::string attendu{MERE};
    attendu.replace(attendu.find(R"("garde-vu"],)"), 10, R"("vu")");
    attendu.replace(attendu.find(R"("value":"garde-vu")"), 18, R"("value":"vu")");
    EXPECT_EQ(lire(racine / "World" / "dialogues" / "mere.json"), attendu);

    const core::Quest renommee = quete();
    EXPECT_EQ(renommee.flags.front().values,
              (std::vector<std::string>{"neuve", "acceptee", "vu", "rendue"}));
    EXPECT_EQ(renommee.flags.front().initial, "neuve");
    EXPECT_EQ(renommee.steps[1].id, "garde-vu");
    EXPECT_EQ(renommee.steps[1].when.front().values,
              (std::vector<std::string>{"neuve", "acceptee"}));

    const std::string place = lire(racine / "Levels" / "place.json");
    EXPECT_NE(place.find(R"("presenceValue": "acceptee|vu")"), std::string::npos) << place;
    EXPECT_NE(place.find(R"("triggerValue": "vu")"), std::string::npos);
    EXPECT_EQ(erreursDuRecit(), "");

    EXPECT_FALSE(hmi::planRenameFlagValue(racine, "quete.essai", "vu", "acceptee").ok());
    EXPECT_FALSE(hmi::planRenameFlagValue(racine, "essai/porte", "a", "b").ok());
}

/**
 * @brief Renommer un drapeau déclaré le suit dans la quête, le dialogue et les deux entités.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>Renommer un drapeau déclaré.</b><br/>
 * \tcat Unitaire · Mode Quêtes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Renommer `quete.essai` en `quete.trial`.<br/>2. Tenter de renommer un fait non
 * déclaré.<br/>
 * \tattendu Aucun fichier ne cite plus `quete.essai` ; le récit se contrôle sans erreur ; le second
 * plan est refusé.}
 */
TEST_F(ModeQuetesProjet, RenommerUnDrapeauDeclare) {
    appliquer(hmi::planRenameFlag(racine, "quete.essai", "quete.trial"));
    for (const char* fichier :
         {"Levels/place.json", "World/dialogues/mere.json", "World/quests/essai.json"}) {
        EXPECT_EQ(lire(racine / fichier).find("quete.essai"), std::string::npos) << fichier;
    }
    EXPECT_EQ(erreursDuRecit(), "");
    EXPECT_FALSE(hmi::planRenameFlag(racine, "essai/porte", "essai/grille").ok());
}

/**
 * @brief Renommer une quête : son fichier, ses clés de journal (textes gardés), la quête que le
 *        dialogue démarre ; retirer une quête ôte son fichier et ses clés.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>Renommer puis retirer une quête.</b><br/>
 * \tcat Unitaire · Mode Quêtes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Renommer `essai` en `trial`.<br/>2. Retirer `trial`.<br/>
 * \tattendu `trial.json` existe, `essai.json` non ; `quest.trial.title = Essai` ; le dialogue
 * démarre `trial` ; puis plus de fichier ni de clé `quest.trial.`, l'autre clé gardée.}
 */
TEST_F(ModeQuetesProjet, RenommerPuisRetirerUneQuete) {
    appliquer(hmi::planRenameQuest(racine, "essai", "trial"));
    EXPECT_FALSE(std::filesystem::exists(racine / "World" / "quests" / "essai.json"));
    const core::QuestLoad trial = core::loadQuest(racine / "World" / "quests" / "trial.json");
    ASSERT_TRUE(trial.quest);
    EXPECT_EQ(trial.quest->id, "trial");
    EXPECT_NE(lire(racine / "Localization" / "fr.lang").find("quest.trial.title = Essai\n"),
              std::string::npos);
    EXPECT_NE(lire(racine / "World" / "dialogues" / "mere.json").find(R"("quest": "trial")"),
              std::string::npos);

    appliquer(hmi::planDeleteQuest(racine, "trial"));
    EXPECT_FALSE(std::filesystem::exists(racine / "World" / "quests" / "trial.json"));
    EXPECT_EQ(lire(racine / "Localization" / "fr.lang"),
              "# Essai\nmap.place.name = La place\nautre.cle = Autre\n");
}

/**
 * @brief Jouer une étape : l'état de partie qui l'atteint, condition par condition.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>L'état de partie qui atteint une étape.</b><br/>
 * \tcat Unitaire · Mode Quêtes<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Demander l'état de « acceptee », « garde-vu », « rendue » et d'une étape
 * inconnue.<br/>2. Faire avancer la quête depuis l'état de « garde-vu ».<br/>
 * \tattendu `quete.essai=acceptee` ; `quete.essai=garde-vu` (ni l'initiale ni `acceptee`, et
 * « non posé » n'écrit rien) ; `quete.essai=rendue` ; rien ; l'étape « garde-vu » est atteinte.}
 */
TEST_F(ModeQuetesProjet, LEtatDePartieQuiAtteintUneEtape) {
    const core::Quest quest = quete();
    EXPECT_EQ(hmi::worldStateReaching(quest, "acceptee", quest.flags),
              (std::vector<std::string>{"quete.essai=acceptee"}));
    EXPECT_EQ(hmi::worldStateReaching(quest, "garde-vu", quest.flags),
              (std::vector<std::string>{"quete.essai=garde-vu"}));
    EXPECT_EQ(hmi::worldStateReaching(quest, "rendue", quest.flags),
              (std::vector<std::string>{"quete.essai=rendue"}));
    EXPECT_TRUE(hmi::worldStateReaching(quest, "absente", quest.flags).empty());

    // Un fait non déclaré se pose tel quel.
    core::Quest fait = quest;
    fait.steps.front().when = {core::FlagCondition{.flag = "encounter/bandits/won"}};
    EXPECT_EQ(hmi::worldStateReaching(fait, "acceptee", fait.flags),
              (std::vector<std::string>{"encounter/bandits/won"}));

    std::string sortie;
    EXPECT_EQ(hmi::runQuestCommand({"--quest-state", "essai", "garde-vu"}, racine, sortie), 0);
    EXPECT_EQ(sortie, "--flags=quete.essai=garde-vu\n");
}

/**
 * @brief Le lieu d'une étape : `--check` refuse une entité qui n'existe pas, et renommer
 *        l'entité ou sa carte récrit le lieu, comme une propriété `EntityRefs`.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>Le lieu d'une étape suit l'entité.</b><br/>
 * \tcat Unitaire · Mode Quêtes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Contrôler le récit.<br/>2. Renommer `place#e1` en `place#garde`, puis la carte en
 * `parvis`.<br/>3. Faire pointer l'étape sur `parvis#e9`.<br/>
 * \tattendu Aucune erreur ; l'étape cite `parvis#garde`, et `--who-cites entity` la nomme ;
 * puis une erreur qui nomme la quête, l'étape et le lieu.}
 */
TEST_F(ModeQuetesProjet, LeLieuDUneEtapeSuitLEntite) {
    EXPECT_EQ(erreursDuRecit(), "");
    appliquer(hmi::planRenameEntityId(racine, "place", "e1", "garde"));
    EXPECT_EQ(quete().steps.front().at, "place#garde");
    appliquer(hmi::planRenameMap(racine, "place", "parvis"));
    EXPECT_EQ(quete().steps.front().at, "parvis#garde");
    EXPECT_EQ(erreursDuRecit(), "");
    const std::vector<hmi::Citation> cites = hmi::citationsOfEntity(racine, "parvis", "garde");
    EXPECT_TRUE(std::ranges::any_of(cites, [](const hmi::Citation& citation) {
        return citation.what == "quest essai: step acceptee: at";
    }));

    core::Quest perdue = quete();
    perdue.steps.front().at = "parvis#e9";
    ecrire(racine / "World" / "quests" / "essai.json", core::writeQuest(perdue));
    EXPECT_NE(erreursDuRecit().find("quest 'essai': step 'acceptee': at \"parvis#e9\" names no "
                                    "entity"),
              std::string::npos)
        << erreursDuRecit();
}

/**
 * @brief `--save-quest` enregistre comme le mode ; `--who-cites flag` liste les usages ; un
 *        renommage refusé n'écrit rien.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>Le mode Quêtes sans fenêtre.</b><br/>
 * \tcat Unitaire · Mode Quêtes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Écrire un brouillon de quête neuve avec son journal.<br/>2. `--save-quest`.<br/>3.
 * `--who-cites flag quete.essai rendue`.<br/>4. `--rename-flag-value quete.essai rendue
 * acceptee`.<br/>
 * \tattendu La quête est écrite sous sa forme canonique, ses deux textes aussi ; trois
 * citations ; le renommage sort en 1, « nothing written ».}
 */
TEST_F(ModeQuetesProjet, LeModeQuetesSansFenetre) {
    const std::filesystem::path fichier = racine / "brouillon.json";
    ecrire(fichier,
           R"({"id": "courrier", "steps": [{"id": "remis", "when": [{"flag": "essai/porte"}]}],
        "journal": {"fr": {"title": "Le courrier", "remis": "Remis."}}})");
    std::string sortie;
    EXPECT_EQ(hmi::runQuestCommand({"--save-quest", fichier.string()}, racine, sortie), 0)
        << sortie;
    EXPECT_EQ(lire(racine / "World" / "quests" / "courrier.json"),
              "{\n  \"id\": \"courrier\",\n  \"steps\": [\n    {\n      \"id\": \"remis\",\n"
              "      \"when\": [{ \"flag\": \"essai/porte\" }]\n    }\n  ]\n}\n");
    EXPECT_NE(lire(racine / "Localization" / "fr.lang")
                  .find("quest.courrier.title = Le courrier\nquest.courrier.remis = Remis.\n"),
              std::string::npos);

    sortie.clear();
    EXPECT_EQ(
        hmi::runRefactorCommand({"--who-cites", "flag", "quete.essai", "rendue"}, racine, sortie),
        0);
    EXPECT_NE(sortie.find("3 citations"), std::string::npos) << sortie;

    sortie.clear();
    EXPECT_EQ(hmi::runRefactorCommand({"--rename-flag-value", "quete.essai", "rendue", "acceptee"},
                                      racine, sortie),
              1);
    EXPECT_NE(sortie.find("nothing written"), std::string::npos);
}

/**
 * @brief Une clé de catalogue change à sa place ; une nouvelle rejoint son groupe ; le même texte
 *        ne touche à rien.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>Une clé de catalogue change à sa place.</b><br/>
 * \tcat Unitaire · Mode Quêtes<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Donner un texte à une clé présente, à une nouvelle clé du groupe, à une clé hors
 * groupe, et le même texte à une clé présente.<br/>
 * \tattendu Chaque ligne à sa place ; la nouvelle clé après la dernière du groupe ; la clé hors
 * groupe à la fin ; le texte inchangé.}
 */
TEST(CatalogEntry, UneCleChangeASaPlace) {
    const std::string texte = "a.x = 1\nq.p.title = T\nq.p.un = U\nb.y = 2";
    EXPECT_EQ(hmi::withCatalogEntry(texte, "q.p.un", "Un"),
              "a.x = 1\nq.p.title = T\nq.p.un = Un\nb.y = 2");
    EXPECT_EQ(hmi::withCatalogEntry(texte, "q.p.deux", "Deux\nlignes", "q.p."),
              "a.x = 1\nq.p.title = T\nq.p.un = U\nq.p.deux = Deux lignes\nb.y = 2");
    EXPECT_EQ(hmi::withCatalogEntry(texte, "c.z", "3"), texte + "\nc.z = 3\n");
    EXPECT_EQ(hmi::withCatalogEntry(texte, "q.p.title", "T"), texte);
}
