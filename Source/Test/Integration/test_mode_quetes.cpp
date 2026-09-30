// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_mode_quetes.cpp
 * @brief Test d'intégration : le mode Quêtes de l'éditeur (`LOT-144`) sur le **contenu livré** —
 *        la quête « Des pommes pour l'arène » (`LOT-120`), ses dialogues et ses cartes de principe.
 *
 * Les critères d'acceptation du lot, sans fenêtre : la quête réécrite entièrement par le mode rend
 * son fichier octet pour octet ; l'étape « acceptee » jouée fait paraître le garde et l'enfant au
 * canevas d'Arenarea ; renommer `condamne` suit la quête, le dialogue du garde et la porte de
 * l'arène, et `--check` reste vert.
 */

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Gameplay/Quest.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/EntityKinds.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/MapTexts.h"
#include "Editor/Logic/QuestEditing.h"
#include "Editor/Logic/WorldState.h"

namespace {

const std::filesystem::path ELEMENTS{JADG_ELEMENTS_DIR};
constexpr std::string_view ARENAREA = "central-empire/capital/arenarea";

[[nodiscard]] std::string lire(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

[[nodiscard]] core::FlagCondition egal(std::string valeur) {
    return core::FlagCondition{
        .flag = "quete.pommes", .test = core::FlagTest::Equals, .values = {std::move(valeur)}};
}

[[nodiscard]] core::FlagCondition fait(std::string drapeau) {
    return core::FlagCondition{
        .flag = std::move(drapeau), .test = core::FlagTest::IsSet, .values = {}};
}

// La quête des pommes telle que l'auteur la saisit dans le mode, champ par champ : son fichier
// n'est pas ouvert.
[[nodiscard]] core::Quest pommesSaisie() {
    core::Quest quete;
    quete.id = "pommes";
    quete.name = "Des pommes pour l'arène";
    quete.source = "original";
    quete.flags = {core::QuestFlag{
        .id = "quete.pommes",
        .values = {"inconnue", "acceptee", "persuasion-echouee", "condamne", "enfant-libere"},
        .initial = "inconnue"}};
    quete.steps = {
        core::QuestStep{.id = "acceptee", .when = {egal("acceptee")}},
        core::QuestStep{.id = "persuasion-echouee",
                        .when = {fait("dialogue/garde/jet-persuasion/failed")}},
        core::QuestStep{.id = "condamne", .when = {egal("condamne")}},
        core::QuestStep{.id = "victoire",
                        .when = {fait("encounter/arene-bandits/won")},
                        .effects = {core::QuestEffect{.kind = core::QuestEffect::Kind::SetFlag,
                                                      .flag = "quete.pommes",
                                                      .value = "enfant-libere"}}},
        core::QuestStep{.id = "enfant-libere", .when = {egal("enfant-libere")}},
        core::QuestStep{.id = "rendue",
                        .when = {fait("pommes/rendue")},
                        .outcome = core::QuestOutcome::Success},
    };
    return quete;
}

// Une copie du contenu livré, images exceptées : ce que le renommage récrit et que `--check` lit.
class ContenuLivre : public ::testing::Test {
protected:
    static inline std::filesystem::path racine;

    static void SetUpTestSuite() {
        racine = std::filesystem::temp_directory_path() /
                 ("jadg-lot144-livre-" + std::to_string(std::random_device{}()));
        for (const auto& entry : std::filesystem::recursive_directory_iterator(ELEMENTS)) {
            const std::string extension = entry.path().extension().string();
            if (!entry.is_regular_file() || extension == ".png" || extension == ".webp" ||
                extension == ".jpg") {
                continue;
            }
            const std::filesystem::path copie =
                racine / std::filesystem::relative(entry.path(), ELEMENTS);
            std::filesystem::create_directories(copie.parent_path());
            std::filesystem::copy_file(entry.path(), copie);
        }
    }
    static void TearDownTestSuite() {
        std::error_code ignore;
        std::filesystem::remove_all(racine, ignore);
    }
};

// Les PNJ d'Arenarea présents sous @p etat, par le dialogue qu'ils portent.
[[nodiscard]] std::vector<std::string> presentsAArenarea(
    const std::vector<std::string>& etat, const std::vector<core::QuestFlag>& declares) {
    const core::LevelLoadResult carte =
        core::LevelLoader::loadFromFile(ELEMENTS / "Levels" / (std::string{ARENAREA} + ".json"));
    EXPECT_TRUE(carte.ok()) << carte.error;
    const std::vector<core::MapEntity>& entites = carte.level->data().entities;
    const std::vector<bool> presence =
        hmi::presenceUnder(entites, hmi::worldStateFlags(etat, declares).flags);
    std::vector<std::string> presents;
    for (std::size_t i = 0; i < entites.size(); ++i) {
        const auto dialogue = entites[i].properties.find(std::string{core::NPC_DIALOGUE_PROPERTY});
        if (presence[i] && dialogue != entites[i].properties.end()) {
            presents.push_back(std::get<std::string>(dialogue->second));
        }
    }
    std::ranges::sort(presents);
    return presents;
}

}  // namespace

/**
 * @brief Critère du `LOT-144` : « Des pommes pour l'arène » se réécrit entièrement dans le mode,
 *        sans ouvrir son fichier ; le fichier produit est identique octet pour octet à celui livré,
 *        et ses textes de journal ne changent pas les catalogues.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>La quête de la démo se réécrit octet pour octet.</b><br/>
 * \tcat Intégration · Mode Quêtes<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Saisir la quête champ par champ, ses textes pris aux catalogues.<br/>2. Calculer
 * l'enregistrement sur le contenu livré.<br/>
 * \tattendu Le plan est accepté ; il n'écrit que la quête, et son texte est celui de
 * `World/quests/pommes.json`.}
 */
TEST(ModeQuetes, LaQueteDeLaDemoSeReecritOctetPourOctet) {
    hmi::QuestDraft draft{.quest = pommesSaisie(), .texts = {}};
    for (const auto& [langue, catalogue] :
         hmi::loadTranslationCatalogs(hmi::localizationDirectory(ELEMENTS))) {
        for (const std::string& cle : core::questTextKeys(draft.quest)) {
            if (const auto texte = catalogue.find(cle); texte != catalogue.end()) {
                draft.texts[langue][cle] = texte->second;
            }
        }
    }
    const hmi::RefactorPlan plan = hmi::planSaveQuest(ELEMENTS, draft, /*isNew=*/false);
    ASSERT_TRUE(plan.ok()) << plan.error;
    ASSERT_EQ(plan.edits.size(), 1U);
    ASSERT_TRUE(plan.edits.front().text.has_value());
    EXPECT_EQ(*plan.edits.front().text, lire(ELEMENTS / "World" / "quests" / "pommes.json"));
}

/**
 * @brief Critère du `LOT-144` : sous « acceptee » choisie comme étape jouée, le garde et l'enfant
 *        paraissent au canevas d'Arenarea, sans autre geste.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>Jouer « acceptee » fait paraître le garde et l'enfant.</b><br/>
 * \tcat Intégration · Mode Quêtes<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire la présence des PNJ d'Arenarea sous l'état initial.<br/>2. Jouer l'étape
 * « acceptee » (`hmi::worldStateReaching`).<br/>
 * \tattendu Ni le garde ni l'enfant au départ ; tous deux sous l'état de l'étape.}
 */
TEST(ModeQuetes, JouerAccepteeFaitParaitreLeGardeEtLEnfant) {
    const core::Quest pommes = pommesSaisie();
    const std::vector<std::string> avant = presentsAArenarea({}, pommes.flags);
    EXPECT_EQ(std::ranges::count(avant, "garde"), 0);
    EXPECT_EQ(std::ranges::count(avant, "enfant"), 0);

    const std::vector<std::string> etat = hmi::worldStateReaching(pommes, "acceptee", pommes.flags);
    EXPECT_EQ(etat, (std::vector<std::string>{"quete.pommes=acceptee"}));
    const std::vector<std::string> apres = presentsAArenarea(etat, pommes.flags);
    EXPECT_EQ(std::ranges::count(apres, "garde"), 1);
    EXPECT_EQ(std::ranges::count(apres, "enfant"), 1);
}

/**
 * @brief Critère du `LOT-144` : renommer la valeur `condamne` met à jour la quête, le dialogue du
 *        garde et la condition de présence des portes de l'arène ; `--check` reste vert.
 * Exigence : `EX-EDIT-100`.
 * \castest{<b>Renommer `condamne` laisse le contrôle vert.</b><br/>
 * \tcat Intégration · Mode Quêtes<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Contrôler la copie du contenu livré.<br/>2. Renommer la valeur `condamne` de
 * `quete.pommes` en `sacrifie`.<br/>3. Contrôler de nouveau.<br/>
 * \tattendu Plus aucune valeur `condamne` : la quête, `garde.json` et la porte des vestiaires
 * disent `sacrifie` ; le nœud et l'étape `condamne` gardent leur nom ; aucune erreur, autant
 * d'avertissements qu'avant.}
 */
TEST_F(ContenuLivre, RenommerCondamneLaisseLeControleVert) {
    const hmi::MapCheckReport avant = hmi::checkAllMaps(racine);
    ASSERT_TRUE(avant.ok());

    const hmi::RefactorPlan plan =
        hmi::planRenameFlagValue(racine, "quete.pommes", "condamne", "sacrifie");
    ASSERT_TRUE(plan.ok()) << plan.error;
    std::string erreur;
    ASSERT_TRUE(hmi::applyRefactorPlan(plan, erreur)) << erreur;

    const std::string garde = lire(racine / "World" / "dialogues" / "garde.json");
    EXPECT_NE(garde.find(R"("flag": "quete.pommes", "value": "sacrifie")"), std::string::npos);
    EXPECT_NE(garde.find(R"("id": "condamne")"), std::string::npos);
    EXPECT_EQ(garde.find(R"("value": "condamne")"), std::string::npos);

    const core::QuestLoad pommes = core::loadQuest(racine / "World" / "quests" / "pommes.json");
    ASSERT_TRUE(pommes.quest);
    EXPECT_TRUE(std::ranges::find(pommes.quest->flags.front().values, "sacrifie") !=
                pommes.quest->flags.front().values.end());
    EXPECT_NE(pommes.quest->find("condamne"), nullptr);
    EXPECT_EQ(pommes.quest->find("condamne")->when.front().values,
              (std::vector<std::string>{"sacrifie"}));

    const std::string vestiaires =
        lire(racine / "Levels" / (std::string{ARENAREA} + "/arena-of-fate/undercroft.json"));
    EXPECT_NE(vestiaires.find(R"("presenceValue": "sacrifie")"), std::string::npos);

    const hmi::MapCheckReport apres = hmi::checkAllMaps(racine);
    EXPECT_TRUE(apres.ok());
    EXPECT_EQ(apres.count(hmi::MapCheckSeverity::Warning),
              avant.count(hmi::MapCheckSeverity::Warning));
}
