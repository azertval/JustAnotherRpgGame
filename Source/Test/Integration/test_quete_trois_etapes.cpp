// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_quete_trois_etapes.cpp
 * @brief Test d'intégration : une quête de trois étapes jouée sans fenêtre, de la carte au journal
 *        (`LOT-116`).
 *
 * La quête et ses deux dialogues sont ceux de la racine d'essai (`Fixtures/GameData/World`), lus
 * et validés comme le jeu les lit au démarrage (`hmi::loadGameQuests`). La carte est posée en
 * mémoire : ce qui s'éprouve ici est le mécanisme — un dialogue pose un drapeau, la quête avance,
 * un PNJ conditionné paraît puis s'en va, le journal le dit —, pas une carte du jeu.
 */

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Gameplay/Quest.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Core/Math/DeterministicRandom.h"
#include "Core/Rpg/Check.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/EntityPresence.h"
#include "Core/World/ExplorationSession.h"
#include "HMI/Game/GameQuests.h"
#include "HMI/Game/WorldPlay.h"
#include "HMI/Presentation/QuestJournalScreen.h"

namespace {

constexpr std::string_view QUETE = "essai-trois-etapes";
constexpr std::uint64_t GRAINE = 116;

[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path{JADG_TEST_DATA_DIR};
}

[[nodiscard]] core::MapEntity pnj(core::GridPosition position, std::string dialogue) {
    return core::MapEntity{
        .type = std::string{core::NPC_ENTITY_TYPE},
        .position = position,
        .properties = {{std::string{core::NPC_DIALOGUE_PROPERTY}, std::move(dialogue)},
                       {std::string{core::NPC_FIGURE_PROPERTY}, std::string{"figurant"}}}};
}

// Le parvis : la mere en (5, 4), le garde en (7, 6), present sous `quete.essai == acceptee`.
[[nodiscard]] core::LevelData parvis() {
    core::TileMap grille{10, 10};
    for (int i = 0; i < 10; ++i) {
        grille.setTile(i, 0, core::TileType::Wall);
        grille.setTile(i, 9, core::TileType::Wall);
        grille.setTile(0, i, core::TileType::Wall);
        grille.setTile(9, i, core::TileType::Wall);
    }
    core::LevelData donnees{.name = "parvis", .tileMap = std::move(grille)};
    donnees.entry = {1, 1};
    core::MapEntity garde = pnj({7, 6}, "essai-garde");
    garde.properties.emplace(std::string{core::PRESENCE_FLAG_PROPERTY}, std::string{"quete.essai"});
    garde.properties.emplace(std::string{core::PRESENCE_VALUE_PROPERTY}, std::string{"acceptee"});
    donnees.entities = {pnj({5, 4}, "essai-mere"), std::move(garde)};
    return donnees;
}

// Un interlocuteur qui parle le commun et ne fait aucun jet : la quete d'essai n'en demande pas.
class Voyageur final : public core::DialogueListener {
public:
    [[nodiscard]] bool speaks(std::string_view langue) const override {
        return langue == "common";
    }
    [[nodiscard]] std::vector<core::Modifier> skillModifiers(std::string_view) const override {
        return {};
    }
    void receiveItem(std::string_view, int) override {}
};

// La partie telle que le jeu la monte : la carte, les quetes du demarrage, les dialogues.
class Partie {
public:
    Partie()
        : _play(
              [](std::string_view) {
                  return core::LevelLoadResult{.level = core::Level{parvis()},
                                               .error = {},
                                               .errorCode = core::LevelValidationError::None};
              },
              dataRoot() / "Assets"),
          _dialogues(core::loadDialogues(dataRoot() / "World" / "dialogues")) {
        hmi::GameQuests quetes = hmi::loadGameQuests(dataRoot());
        erreurs = quetes.errors;
        _play.session().setQuests(std::move(quetes.catalog));
    }

    [[nodiscard]] hmi::WorldPlay& play() {
        return _play;
    }

    /// Le heros en @p ou, tourne vers la droite, interagit : le dialogue que la carte demande.
    [[nodiscard]] std::optional<std::string> parlerDepuis(core::GridPosition ou) {
        _play.session().placeHero(core::cellCenter(ou));
        static_cast<void>(_play.step(core::ExplorationIntent{.move = {1.0F, 0.0F}}, 0.0001F));
        const hmi::WorldPlayStep pas =
            _play.step(core::ExplorationIntent{.interact = true}, 1.0F / 60.0F);
        for (const core::ExplorationEvent& evenement : pas.events) {
            if (evenement.kind == core::ExplorationEventKind::Dialogue) {
                return evenement.value;
            }
        }
        return std::nullopt;
    }

    /// Joue le dialogue @p id en donnant les reponses @p reponses, sur les drapeaux de la partie.
    void converser(const std::string& id, const std::vector<std::string>& reponses) {
        const core::DialogueGraph* graphe = _dialogues.find(id);
        ASSERT_NE(graphe, nullptr) << id;
        Voyageur voyageur;
        core::DeterministicRandom hasard{GRAINE};
        core::DialogueRunner runner(*graphe, _play.session().flags(), voyageur, _echelle, hasard);
        ASSERT_EQ(runner.start(), core::DialogueState::AwaitingChoice) << id;
        for (const std::string& reponse : reponses) {
            static_cast<void>(runner.choose(reponse));
        }
        EXPECT_EQ(runner.state(), core::DialogueState::Ended) << id;
    }

    /// Un pas sans geste : les consequences des drapeaux, et les etapes atteintes.
    [[nodiscard]] std::vector<std::string> etapesAtteintes(bool* sceneChangee = nullptr) {
        const hmi::WorldPlayStep pas = _play.step(core::ExplorationIntent{}, 1.0F / 60.0F);
        if (sceneChangee != nullptr) {
            *sceneChangee = pas.sceneChanged;
        }
        std::vector<std::string> etapes;
        for (const core::ExplorationEvent& evenement : pas.events) {
            if (evenement.kind == core::ExplorationEventKind::QuestAdvanced) {
                etapes.push_back(evenement.value);
            }
        }
        return etapes;
    }

    [[nodiscard]] std::size_t figuresDePnj() const {
        const std::vector<hmi::WorldFigureSnapshot> figures = _play.snapshot().figures;
        return static_cast<std::size_t>(std::ranges::count_if(
            figures, [](const hmi::WorldFigureSnapshot& figure) { return !figure.hero; }));
    }

    std::vector<std::string> erreurs;

private:
    hmi::WorldPlay _play;
    core::DialogueCatalog _dialogues;
    core::DifficultyScale _echelle;
};

}  // namespace

/**
 * @brief Une quête de trois étapes se joue sans fenêtre, de la carte au journal.
 * \castest{<b>Une quete de trois etapes se joue sans fenetre.</b><br/>
 * \tcat Integration · Quetes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Monter la partie : quetes et dialogues de la racine d'essai, un parvis en memoire
 * ou la mere attend et ou le garde ne parait que sous `quete.essai == acceptee`.<br/>2. Parler a
 * la mere, accepter.<br/>3. Parler au garde, lui faire relacher l'enfant.<br/>4. Revenir a la
 * mere, lui rendre l'enfant.<br/>5. Lire le journal.<br/>
 * \tattendu Aucune erreur au demarrage. Chaque conversation atteint exactement l'etape suivante au
 * pas d'apres. Le garde est absent avant l'acceptation, parait apres (la scene change, une figure
 * de plus), repart quand il a relache l'enfant. La quete finit reussie, son effet pose ; le
 * journal la montre terminee, trois etapes, la derniere l'entree de `rendue`.
 * }
 */
TEST(QueteIntegration, UneQueteDeTroisEtapesSeJoueSansFenetre) {
    Partie partie;
    ASSERT_TRUE(partie.erreurs.empty()) << partie.erreurs.front();
    ASSERT_NE(partie.play().session().quests().find(QUETE), nullptr);
    ASSERT_TRUE(partie.play().enter("parvis", {}));
    EXPECT_EQ(partie.figuresDePnj(), 1U);
    EXPECT_EQ(partie.parlerDepuis({6, 6}), std::nullopt) << "le garde n'est pas encore la";

    ASSERT_EQ(partie.parlerDepuis({4, 4}), "essai-mere");
    partie.converser("essai-mere", {"accepter"});
    bool sceneChangee = false;
    EXPECT_EQ(partie.etapesAtteintes(&sceneChangee),
              (std::vector<std::string>{"essai-trois-etapes/acceptee"}));
    EXPECT_TRUE(sceneChangee);
    EXPECT_EQ(partie.figuresDePnj(), 2U);

    ASSERT_EQ(partie.parlerDepuis({6, 6}), "essai-garde");
    partie.converser("essai-garde", {"relacher"});
    EXPECT_EQ(partie.etapesAtteintes(), (std::vector<std::string>{"essai-trois-etapes/garde-vu"}));
    EXPECT_EQ(partie.figuresDePnj(), 1U);
    EXPECT_EQ(partie.parlerDepuis({6, 6}), std::nullopt) << "le garde est reparti";

    ASSERT_EQ(partie.parlerDepuis({4, 4}), "essai-mere");
    partie.converser("essai-mere", {"rendre"});
    EXPECT_EQ(partie.etapesAtteintes(), (std::vector<std::string>{"essai-trois-etapes/rendue"}));

    const core::WorldFlags& drapeaux = partie.play().session().flags();
    EXPECT_TRUE(drapeaux.isSet("essai/recompense-donnee"));
    const core::Quest& quete = *partie.play().session().quests().find(QUETE);
    EXPECT_EQ(core::questProgress(quete, drapeaux).status, core::QuestStatus::Succeeded);

    const hmi::QuestJournalValues journal =
        hmi::questJournalValues(partie.play().session().quests(), drapeaux, {},
                                [](std::string_view key) { return std::string{key}; });
    ASSERT_EQ(journal.quests.size(), 1U);
    EXPECT_EQ(journal.quests.front().value, "journal.status.succeeded");
    EXPECT_EQ(journal.objectives.size(), 3U);
    EXPECT_EQ(journal.detail, "quest.essai-trois-etapes.rendue");
}
