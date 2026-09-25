// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_quest.cpp
 * @brief Tests des drapeaux typés, des conditions et des quêtes en données (LOT-116).
 */

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Data/JsonDocument.h"
#include "Core/Gameplay/FlagCondition.h"
#include "Core/Gameplay/Quest.h"
#include "Core/Gameplay/WorldFlags.h"
#include "Core/Rpg/Dialogue.h"

namespace {

const std::vector<std::string> VALEURS = {"inconnue", "acceptee", "condamne", "enfant-libere"};

// Une quete bien formee a trois etapes, la troisieme close et posant un fait.
constexpr const char* QUETE_VALIDE = R"({
  "id": "essai",
  "flags": [
    { "id": "quete.essai", "values": ["inconnue", "acceptee", "garde-vu", "rendue"] }
  ],
  "steps": [
    { "id": "acceptee", "when": [{ "flag": "quete.essai", "equals": "acceptee" }] },
    { "id": "garde-vu", "when": [{ "flag": "quete.essai", "equals": "garde-vu" }] },
    {
      "id": "rendue",
      "when": [{ "flag": "quete.essai", "equals": "rendue" }],
      "effects": [{ "type": "setFlag", "flag": "essai/recompense" }],
      "outcome": "success"
    }
  ]
})";

[[nodiscard]] core::Quest queteValide() {
    core::QuestLoad lue = core::readQuest(QUETE_VALIDE, "essai.json");
    EXPECT_TRUE(lue.errors.empty()) << (lue.errors.empty() ? "" : lue.errors.front());
    return lue.quest.value_or(core::Quest{});
}

[[nodiscard]] core::QuestCatalog catalogueDe(core::Quest quete) {
    core::QuestCatalog catalogue;
    catalogue.quests.push_back(std::move(quete));
    return catalogue;
}

[[nodiscard]] core::FlagCondition condition(const char* json) {
    return core::readFlagCondition(nlohmann::json::parse(json))
        .condition.value_or(core::FlagCondition{});
}

}  // namespace

/**
 * @brief Un drapeau déclaré à valeurs refuse une valeur hors de sa liste et rend son initiale.
 * \castest{<b>Un drapeau a valeurs est type.</b><br/>
 * \tcat Unitaire · Quetes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Declarer `quete.pommes` a quatre valeurs, initiale `inconnue`.<br/>2. Lire, poser
 * `acceptee`, puis une valeur absente, puis le poser sans valeur.<br/>3. L'effacer.<br/>
 * \tattendu L'initiale se lit avant toute pose ; `acceptee` est acceptee, la valeur absente et la
 * pose sans valeur sont refusees sans rien changer ; l'effacement ramene l'initiale ; chaque
 * changement fait avancer la revision, un refus non.
 * }
 */
TEST(QuestFlagsTest, UnDrapeauAValeursEstType) {
    core::WorldFlags drapeaux;
    ASSERT_TRUE(drapeaux.declare("quete.pommes", VALEURS, "inconnue"));
    EXPECT_EQ(drapeaux.value("quete.pommes"), "inconnue");
    EXPECT_FALSE(drapeaux.isSet("quete.pommes"));

    const auto avant = drapeaux.revision();
    EXPECT_TRUE(drapeaux.setValue("quete.pommes", "acceptee"));
    EXPECT_EQ(drapeaux.value("quete.pommes"), "acceptee");
    EXPECT_GT(drapeaux.revision(), avant);

    const auto apres = drapeaux.revision();
    EXPECT_FALSE(drapeaux.setValue("quete.pommes", "accepte"));
    EXPECT_FALSE(drapeaux.set("quete.pommes"));
    EXPECT_EQ(drapeaux.value("quete.pommes"), "acceptee");
    EXPECT_EQ(drapeaux.revision(), apres);

    drapeaux.clear("quete.pommes");
    EXPECT_EQ(drapeaux.value("quete.pommes"), "inconnue");
    EXPECT_FALSE(drapeaux.setValue("non-declare", "x"));
    EXPECT_FALSE(drapeaux.declare("vide", {}, "x"));
    EXPECT_FALSE(drapeaux.declare("hors", VALEURS, "absente"));
}

/**
 * @brief Les trois formes d'une condition se lisent et s'évaluent, la valeur initiale comprise.
 * \castest{<b>Une condition sur drapeau se lit et s'evalue.</b><br/>
 * \tcat Unitaire · Quetes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire `isSet`, `equals` (une valeur, une liste) et `notEquals`.<br/>2. Les evaluer
 * sur un drapeau declare a son initiale, puis a `condamne`.<br/>3. Lire des formes fautives.<br/>
 * \tattendu Chaque forme donne son test ; `equals: inconnue` tient a l'initiale ; la liste tient
 * pour chacune de ses valeurs ; deux formes a la fois, une liste vide ou un `flag` absent sont
 * refusees avec un message.
 * }
 */
TEST(QuestFlagsTest, UneConditionSurDrapeauSeLitEtSEvalue) {
    core::WorldFlags drapeaux;
    ASSERT_TRUE(drapeaux.declare("quete.pommes", VALEURS, "inconnue"));

    const core::FlagCondition initiale =
        condition(R"({"flag":"quete.pommes","equals":"inconnue"})");
    const core::FlagCondition parmi =
        condition(R"({"flag":"quete.pommes","equals":["condamne","enfant-libere"]})");
    const core::FlagCondition sauf = condition(R"({"flag":"quete.pommes","notEquals":"condamne"})");
    const core::FlagCondition absent = condition(R"({"flag":"coffre","isSet":false})");
    EXPECT_EQ(initiale.test, core::FlagTest::Equals);
    EXPECT_EQ(absent.test, core::FlagTest::IsUnset);
    EXPECT_EQ(core::describeFlagCondition(parmi), "quete.pommes == condamne|enfant-libere");

    EXPECT_TRUE(initiale.holds(drapeaux));
    EXPECT_FALSE(parmi.holds(drapeaux));
    EXPECT_TRUE(sauf.holds(drapeaux));
    EXPECT_TRUE(absent.holds(drapeaux));

    ASSERT_TRUE(drapeaux.setValue("quete.pommes", "condamne"));
    drapeaux.set("coffre");
    EXPECT_FALSE(initiale.holds(drapeaux));
    EXPECT_TRUE(parmi.holds(drapeaux));
    EXPECT_FALSE(sauf.holds(drapeaux));
    EXPECT_FALSE(absent.holds(drapeaux));

    for (const char* fautive :
         {R"({"flag":"f","isSet":true,"equals":"v"})", R"({"flag":"f","equals":[]})",
          R"({"equals":"v"})", R"({"flag":"f","isSet":"oui"})"}) {
        const core::FlagConditionRead lue = core::readFlagCondition(nlohmann::json::parse(fautive));
        EXPECT_FALSE(lue.condition.has_value()) << fautive;
        EXPECT_FALSE(lue.error.empty()) << fautive;
    }
    EXPECT_EQ(core::splitFlagValues(" acceptee | condamne||acceptee "),
              (std::vector<std::string>{"acceptee", "condamne"}));
}

/**
 * @brief Un chemin JSON retrouve la ligne de sa valeur dans le texte.
 * \castest{<b>Un pointeur JSON donne la ligne de sa valeur.</b><br/>
 * \tcat Unitaire · Donnees<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Chercher `/steps/2/when`, `/steps/1` et `/id` dans la quete d'essai.<br/>2. Chercher
 * un chemin absent.<br/>
 * \tattendu Les lignes sont celles du texte (7, 6 et 2) ; le chemin absent rend la ligne 0.
 * }
 */
TEST(QuestLoadTest, UnPointeurJsonDonneLaLigneDeSaValeur) {
    using Pointeur = nlohmann::json::json_pointer;
    EXPECT_EQ(core::positionOfPointer(QUETE_VALIDE, Pointeur("/id")).line, 2);
    EXPECT_EQ(core::positionOfPointer(QUETE_VALIDE, Pointeur("/steps/1")).line, 8);
    EXPECT_EQ(core::positionOfPointer(QUETE_VALIDE, Pointeur("/steps/2/when")).line, 11);
    EXPECT_EQ(core::positionOfPointer(QUETE_VALIDE, Pointeur("/steps/9")).line, 0);
}

/**
 * @brief Une quête bien formée se lit : drapeaux, étapes, conditions, effets et issue.
 * \castest{<b>Une quete bien formee se lit.</b><br/>
 * \tcat Unitaire · Quetes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire la quete d'essai a trois etapes.<br/>
 * \tattendu Aucune erreur ; un drapeau a quatre valeurs d'initiale `inconnue` (la premiere) ;
 * trois etapes dans l'ordre ; la troisieme pose un fait et clot la quete en reussite ; les cles
 * du titre et des trois etapes sont fabriquees.
 * }
 */
TEST(QuestLoadTest, UneQueteBienFormeeSeLit) {
    const core::Quest quete = queteValide();
    EXPECT_EQ(quete.id, "essai");
    ASSERT_EQ(quete.flags.size(), 1U);
    EXPECT_EQ(quete.flags.front().initial, "inconnue");
    ASSERT_EQ(quete.steps.size(), 3U);
    EXPECT_EQ(quete.steps[2].outcome, core::QuestOutcome::Success);
    ASSERT_EQ(quete.steps[2].effects.size(), 1U);
    EXPECT_EQ(quete.steps[2].effects.front().flag, "essai/recompense");
    EXPECT_EQ(core::questTextKeys(quete),
              (std::vector<std::string>{"quest.essai.title", "quest.essai.acceptee",
                                        "quest.essai.garde-vu", "quest.essai.rendue"}));
}

/**
 * @brief Un fichier de quête mal formé est refusé, et le message nomme la ligne.
 * \castest{<b>Une quete mal formee est refusee en nommant la ligne.</b><br/>
 * \tcat Unitaire · Quetes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire un JSON a la virgule manquante.<br/>2. Lire une quete dont la deuxieme etape
 * compare une valeur non declaree et dont la troisieme n'a pas de condition.<br/>
 * \tattendu Aucune quete n'est rendue ; l'erreur de syntaxe nomme `fichier:4` ; les deux erreurs
 * de sens sont listees d'un coup, chacune avec sa ligne (`:8` et `:9`).
 * }
 */
TEST(QuestLoadTest, UneQueteMalFormeeEstRefuseeEnNommantLaLigne) {
    const core::QuestLoad syntaxe =
        core::readQuest("{\n  \"id\": \"x\",\n  \"steps\": [\n  {} {}\n]}", "quetes/x.json");
    EXPECT_FALSE(syntaxe.quest.has_value());
    ASSERT_EQ(syntaxe.errors.size(), 1U);
    EXPECT_NE(syntaxe.errors.front().find("quetes/x.json:4"), std::string::npos)
        << syntaxe.errors.front();

    constexpr const char* FAUTIVE = R"({
  "id": "fautive",
  "flags": [
    { "id": "quete.f", "values": ["a", "b"] }
  ],
  "steps": [
    { "id": "un", "when": [{ "flag": "quete.f", "equals": "a" }] },
    { "id": "deux", "when": [{ "flag": "quete.f", "equals": "c" }] },
    { "id": "trois" }
  ]
})";
    const core::QuestLoad sens = core::readQuest(FAUTIVE, "quetes/fautive.json");
    EXPECT_FALSE(sens.quest.has_value());
    ASSERT_EQ(sens.errors.size(), 2U);
    EXPECT_NE(sens.errors[0].find("quetes/fautive.json:8"), std::string::npos) << sens.errors[0];
    EXPECT_NE(sens.errors[0].find("'c'"), std::string::npos) << sens.errors[0];
    EXPECT_NE(sens.errors[1].find("quetes/fautive.json:9"), std::string::npos) << sens.errors[1];
    EXPECT_NE(sens.errors[1].find("'when'"), std::string::npos) << sens.errors[1];
}

/**
 * @brief Le catalogue refuse un fichier mal nommé et un drapeau déclaré par deux quêtes.
 * \castest{<b>Le catalogue des quetes refuse les doublons.</b><br/>
 * \tcat Unitaire · Quetes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ecrire trois quetes dans un dossier temporaire : une valide, une dont le nom de
 * fichier differe de l'identifiant, une qui redeclare le drapeau de la premiere.<br/>2. Charger le
 * dossier, puis un dossier absent.<br/>
 * \tattendu Seule la valide est gardee, deux erreurs nomment les fichiers refuses ; le dossier
 * absent donne un catalogue vide sans erreur.
 * }
 */
TEST(QuestLoadTest, LeCatalogueDesQuetesRefuseLesDoublons) {
    const std::filesystem::path dossier =
        std::filesystem::temp_directory_path() / "jadg-lot116-quetes";
    std::filesystem::remove_all(dossier);
    std::filesystem::create_directories(dossier);
    const auto ecrire = [&dossier](const char* nom, const std::string& texte) {
        std::ofstream(dossier / nom, std::ios::binary) << texte;
    };
    ecrire("essai.json", QUETE_VALIDE);
    std::string renommee = QUETE_VALIDE;
    renommee.replace(renommee.find("\"essai\""), 7, "\"autre\"");
    ecrire("mal-nommee.json", renommee);
    std::string doublon = renommee;
    doublon.replace(doublon.find("\"autre\""), 7, "\"zeta\"");
    ecrire("zeta.json", doublon);

    const core::QuestCatalog catalogue = core::loadQuests(dossier);
    ASSERT_EQ(catalogue.quests.size(), 1U);
    EXPECT_EQ(catalogue.quests.front().id, "essai");
    ASSERT_EQ(catalogue.errors.size(), 2U);
    EXPECT_NE(catalogue.errors[0].find("mal-nommee.json"), std::string::npos);
    EXPECT_NE(catalogue.errors[1].find("quete.essai"), std::string::npos);
    ASSERT_NE(catalogue.findFlag("quete.essai"), nullptr);

    const core::QuestCatalog vide = core::loadQuests(dossier / "absent");
    EXPECT_TRUE(vide.quests.empty());
    EXPECT_TRUE(vide.errors.empty());
    std::filesystem::remove_all(dossier);
}

/**
 * @brief Les drapeaux font avancer une quête de trois étapes jusqu'à sa clôture, une fois chacune.
 * \castest{<b>Une quete de trois etapes avance par les drapeaux.</b><br/>
 * \tcat Unitaire · Quetes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Declarer les drapeaux de la quete d'essai.<br/>2. Poser tour a tour `acceptee`,
 * `garde-vu`, `rendue`, en faisant avancer les quetes a chaque fois.<br/>3. Faire avancer une
 * fois de plus.<br/>
 * \tattendu Chaque pose atteint exactement son etape ; la derniere pose `essai/recompense` et clot
 * la quete en reussite, trois etapes au journal ; un nouvel appel ne rend rien.
 * }
 */
TEST(QuestAdvanceTest, UneQueteDeTroisEtapesAvanceParLesDrapeaux) {
    const core::QuestCatalog catalogue = catalogueDe(queteValide());
    core::WorldFlags drapeaux;
    core::declareQuestFlags(catalogue, drapeaux);
    EXPECT_TRUE(core::advanceQuests(catalogue, drapeaux).empty());
    EXPECT_EQ(core::questProgress(catalogue.quests.front(), drapeaux).status,
              core::QuestStatus::NotStarted);

    const std::vector<std::pair<const char*, core::QuestOutcome>> pas = {
        {"acceptee", core::QuestOutcome::None},
        {"garde-vu", core::QuestOutcome::None},
        {"rendue", core::QuestOutcome::Success}};
    for (const auto& [valeur, issue] : pas) {
        ASSERT_TRUE(drapeaux.setValue("quete.essai", valeur));
        const std::vector<core::QuestEvent> evenements = core::advanceQuests(catalogue, drapeaux);
        ASSERT_EQ(evenements.size(), 1U) << valeur;
        EXPECT_EQ(evenements.front(), (core::QuestEvent{"essai", valeur, issue}));
    }
    EXPECT_TRUE(drapeaux.isSet("essai/recompense"));
    const core::QuestProgress fin = core::questProgress(catalogue.quests.front(), drapeaux);
    EXPECT_EQ(fin.status, core::QuestStatus::Succeeded);
    EXPECT_EQ(fin.reachedSteps, (std::vector<std::string>{"acceptee", "garde-vu", "rendue"}));
    EXPECT_TRUE(core::advanceQuests(catalogue, drapeaux).empty());
}

/**
 * @brief Un dialogue qui compare ou pose une valeur non déclarée est relevé au démarrage.
 * \castest{<b>Les usages de drapeaux sont confrontes aux declarations.</b><br/>
 * \tcat Unitaire · Quetes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire un dialogue qui pose `quete.essai = acceptee`, compare a `accepte` (faute) et
 * pose `quete.essai` sans valeur.<br/>2. Le confronter a la quete d'essai.<br/>3. Relever ce
 * qu'il pose et ce qu'il lit.<br/>
 * \tattendu Deux erreurs, la valeur fautive et la pose sans valeur, nommant le dialogue ; les
 * drapeaux poses comprennent la valeur, le fait de chaque etape et le drapeau declare ; le
 * drapeau lu est `quete.essai`.
 * }
 */
TEST(QuestAdvanceTest, LesUsagesDeDrapeauxSontConfrontesAuxDeclarations) {
    constexpr const char* DIALOGUE = R"({
  "id": "mere",
  "speaker": { "languages": ["common"], "attitude": "friendly" },
  "start": "test",
  "nodes": [
    { "id": "test", "type": "condition", "flag": "quete.essai", "equals": "accepte",
      "then": "pose", "else": "pose" },
    { "id": "pose", "type": "action", "next": "fin", "actions": [
      { "type": "setFlag", "flag": "quete.essai", "value": "acceptee" },
      { "type": "setFlag", "flag": "quete.essai" } ] },
    { "id": "fin", "type": "end" }
  ]
})";
    core::DialogueLoad lu = core::readDialogue(DIALOGUE, "mere.json");
    ASSERT_TRUE(lu.graph.has_value()) << (lu.errors.empty() ? "" : lu.errors.front());
    core::DialogueCatalog dialogues;
    dialogues.dialogues.push_back(*lu.graph);
    const core::QuestCatalog quetes = catalogueDe(queteValide());

    const std::vector<std::string> erreurs = core::validateFlagUses(quetes, dialogues);
    ASSERT_EQ(erreurs.size(), 2U);
    EXPECT_NE(erreurs[0].find("'accepte'"), std::string::npos) << erreurs[0];
    EXPECT_NE(erreurs[1].find("'value'"), std::string::npos) << erreurs[1];
    EXPECT_NE(erreurs[1].find("dialogue 'mere'"), std::string::npos) << erreurs[1];

    const auto poses = core::flagsWrittenBy(quetes, dialogues);
    EXPECT_TRUE(poses.contains("quete.essai"));
    EXPECT_TRUE(poses.contains(core::questStepFlag("essai", "rendue")));
    EXPECT_TRUE(poses.contains("essai/recompense"));
    const std::vector<core::FlagRead> lus = core::flagsReadBy(quetes, dialogues);
    EXPECT_TRUE(std::ranges::all_of(
        lus, [](const core::FlagRead& lu) { return lu.flag == "quete.essai"; }));
    EXPECT_EQ(lus.size(), 4U);
}

/**
 * @brief Un dialogue qui engage une rencontre pose, par la victoire, le fait de la rencontre
 *        gagnée : une quête peut le lire sans que le contrôle le dise « lu sans être posé ».
 * \castest{<b>Une rencontre engagee par un dialogue compte parmi les drapeaux poses.</b><br/>
 * \tcat Unitaire · Quetes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Un dialogue dont un noeud d'action engage la rencontre `arene`.<br/>
 * \tattendu `encounter/arene/won` est parmi les drapeaux que le recit pose.
 * }
 */
TEST(QuestTest, UneRencontreEngageeParUnDialoguePoseLeFaitDeSaVictoire) {
    static constexpr const char* DIALOGUE = R"({
  "id": "maitre",
  "name": "Le maitre d'arene",
  "source": "original",
  "speaker": { "languages": ["common"] },
  "start": "defi",
  "nodes": [
    { "id": "defi", "type": "line", "choices": [{ "id": "combattre", "next": "engage" }] },
    { "id": "engage", "type": "action", "next": "fin",
      "actions": [{ "type": "startEncounter", "encounter": "arene" }] },
    { "id": "fin", "type": "end" }
  ]
})";
    core::DialogueLoad lu = core::readDialogue(DIALOGUE, "maitre.json");
    ASSERT_TRUE(lu.graph.has_value()) << (lu.errors.empty() ? "" : lu.errors.front());
    core::DialogueCatalog dialogues;
    dialogues.dialogues.push_back(*lu.graph);

    const auto poses = core::flagsWrittenBy(core::QuestCatalog{}, dialogues);
    EXPECT_TRUE(poses.contains(core::encounterWonFlag("arene")));
    EXPECT_EQ(core::encounterWonFlag("arene"), "encounter/arene/won");
}
