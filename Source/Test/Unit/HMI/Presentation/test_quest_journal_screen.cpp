// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_quest_journal_screen.cpp
 * @brief Tests de ce que le journal de quêtes montre (LOT-116), tiré des drapeaux sans fenêtre.
 */

#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Gameplay/Quest.h"
#include "Core/Gameplay/WorldFlags.h"
#include "HMI/Presentation/QuestJournalScreen.h"

namespace {

// La « traduction » d'essai rend la cle elle-meme : le test lit ce qui est demande.
const hmi::TextLookup CLE = [](std::string_view key) { return std::string{key}; };

[[nodiscard]] core::Quest quete(std::string id, std::string drapeau) {
    const auto etape = [&drapeau](std::string etapeId, core::QuestOutcome issue) {
        return core::QuestStep{
            .id = etapeId,
            .when = {core::FlagCondition{
                .flag = drapeau, .test = core::FlagTest::Equals, .values = {etapeId}}},
            .effects = {},
            .outcome = issue};
    };
    return core::Quest{
        .id = std::move(id),
        .flags = {core::QuestFlag{
            .id = drapeau, .values = {"inconnue", "un", "deux", "fin"}, .initial = "inconnue"}},
        .steps = {etape("un", core::QuestOutcome::None), etape("deux", core::QuestOutcome::None),
                  etape("fin", core::QuestOutcome::Failure)}};
}

}  // namespace

/**
 * @brief Le journal montre les quêtes commencées, la choisie marquée, et ses étapes atteintes.
 * \castest{<b>Le journal se lit dans les drapeaux.</b><br/>
 * \tcat Unitaire · Journal de quetes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Deux quetes ; aucune commencee.<br/>2. Faire atteindre `un` puis `deux` a la
 * premiere, `un` a la seconde.<br/>3. Choisir la seconde, puis la voisine suivante et precedente.
 * <br/>4. Clore la premiere en echec.<br/>
 * \tattendu Vide : aucune ligne, le detail dit `journal.empty`. Ensuite : deux quetes « en cours »,
 * la premiere marquee par defaut, son detail est l'entree de `deux`, ses etapes `un` (franchie) et
 * `deux` (en cours). Le choix deplace la marque ; la voisine ne sort pas de la liste. Close, la
 * premiere porte l'etat « echouee » sur sa ligne et sa derniere etape.
 * }
 */
TEST(QuestJournalScreenTest, LeJournalSeLitDansLesDrapeaux) {
    core::QuestCatalog catalogue;
    catalogue.quests = {quete("pommes", "quete.pommes"), quete("caves", "quete.caves")};
    core::WorldFlags drapeaux;
    core::declareQuestFlags(catalogue, drapeaux);

    const hmi::QuestJournalValues vide = hmi::questJournalValues(catalogue, drapeaux, {}, CLE);
    EXPECT_TRUE(vide.quests.empty());
    EXPECT_TRUE(vide.selected.empty());
    EXPECT_EQ(vide.detail, "journal.empty");

    for (const char* valeur : {"un", "deux"}) {
        drapeaux.setValue("quete.pommes", valeur);
        core::advanceQuests(catalogue, drapeaux);
    }
    drapeaux.setValue("quete.caves", "un");
    core::advanceQuests(catalogue, drapeaux);

    const hmi::QuestJournalValues ouvert = hmi::questJournalValues(catalogue, drapeaux, {}, CLE);
    ASSERT_EQ(ouvert.quests.size(), 2U);
    EXPECT_EQ(ouvert.selected, "pommes");
    EXPECT_EQ(ouvert.quests[0],
              (hmi::QuestJournalRow{"pommes", "› quest.pommes.title", "journal.status.active"}));
    EXPECT_EQ(ouvert.quests[1].label, "quest.caves.title");
    EXPECT_EQ(ouvert.detail, "quest.pommes.deux");
    ASSERT_EQ(ouvert.objectives.size(), 2U);
    EXPECT_EQ(ouvert.objectives[0], (hmi::QuestJournalRow{"un", "quest.pommes.un", "✓"}));
    EXPECT_EQ(ouvert.objectives[1], (hmi::QuestJournalRow{"deux", "quest.pommes.deux", ""}));

    const hmi::QuestJournalValues seconde =
        hmi::questJournalValues(catalogue, drapeaux, "caves", CLE);
    EXPECT_EQ(seconde.selected, "caves");
    EXPECT_EQ(seconde.quests[1].label, "› quest.caves.title");
    EXPECT_EQ(seconde.quests[0].label, "quest.pommes.title");
    EXPECT_EQ(hmi::neighbourQuest(seconde, 1), "caves");
    EXPECT_EQ(hmi::neighbourQuest(seconde, -1), "pommes");
    EXPECT_EQ(hmi::neighbourQuest(ouvert, -1), "pommes");

    drapeaux.setValue("quete.pommes", "fin");
    core::advanceQuests(catalogue, drapeaux);
    const hmi::QuestJournalValues close = hmi::questJournalValues(catalogue, drapeaux, {}, CLE);
    EXPECT_EQ(close.quests[0].value, "journal.status.failed");
    ASSERT_EQ(close.objectives.size(), 3U);
    EXPECT_EQ(close.objectives[2].value, "journal.status.failed");
}
