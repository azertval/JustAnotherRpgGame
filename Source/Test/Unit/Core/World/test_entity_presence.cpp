// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_entity_presence.cpp
 * @brief Tests de la condition de présence d'une entité de carte (LOT-116) : lue sur trois
 *        propriétés plates, jouée par la session d'exploration sans recharger la carte, et
 *        contrôlée comme les autres références.
 */

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Gameplay/Quest.h"
#include "Core/Gameplay/WorldFlags.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/EntityPresence.h"
#include "Core/World/ExplorationSession.h"

namespace {

using core::ExplorationEventKind;

const std::vector<std::string> VALEURS = {"inconnue", "acceptee", "enfant-libere"};

// Un PNJ a dialogue, present sous la condition que portent @p proprietes.
[[nodiscard]] core::MapEntity garde(core::GridPosition position, core::PropertyMap proprietes) {
    proprietes.emplace(std::string{core::NPC_DIALOGUE_PROPERTY}, std::string{"garde"});
    return core::MapEntity{.type = std::string{core::NPC_ENTITY_TYPE},
                           .position = position,
                           .properties = std::move(proprietes)};
}

[[nodiscard]] core::PropertyMap presence(std::string drapeau, std::string test,
                                         std::string valeurs) {
    core::PropertyMap proprietes;
    proprietes.emplace(std::string{core::PRESENCE_FLAG_PROPERTY}, std::move(drapeau));
    if (!test.empty()) {
        proprietes.emplace(std::string{core::PRESENCE_TEST_PROPERTY}, std::move(test));
    }
    if (!valeurs.empty()) {
        proprietes.emplace(std::string{core::PRESENCE_VALUE_PROPERTY}, std::move(valeurs));
    }
    return proprietes;
}

// Une carte de 10 x 10 cases entouree de murs, son entree en (1, 1).
[[nodiscard]] core::LevelData carteMuree(std::vector<core::MapEntity> entites) {
    core::TileMap grille{10, 10};
    for (int i = 0; i < 10; ++i) {
        grille.setTile(i, 0, core::TileType::Wall);
        grille.setTile(i, 9, core::TileType::Wall);
        grille.setTile(0, i, core::TileType::Wall);
        grille.setTile(9, i, core::TileType::Wall);
    }
    core::LevelData donnees{.name = "parvis", .tileMap = std::move(grille)};
    donnees.entry = {1, 1};
    donnees.entities = std::move(entites);
    return donnees;
}

// Un chargeur qui compte ses lectures : « sans recharger la carte » se verifie.
struct ChargeurCompte {
    core::LevelData donnees;
    int lectures = 0;

    [[nodiscard]] core::WorldTravel::MapLoader chargeur() {
        return [this](std::string_view) {
            ++lectures;
            return core::LevelLoadResult{.level = core::Level{donnees},
                                         .error = {},
                                         .errorCode = core::LevelValidationError::None};
        };
    }
};

[[nodiscard]] core::QuestCatalog quetePommes() {
    core::QuestCatalog catalogue;
    catalogue.quests.push_back(core::Quest{
        .id = "pommes",
        .flags = {core::QuestFlag{.id = "quete.pommes", .values = VALEURS, .initial = "inconnue"}},
        .steps = {core::QuestStep{
            .id = "acceptee",
            .when = {core::FlagCondition{
                .flag = "quete.pommes", .test = core::FlagTest::Equals, .values = {"acceptee"}}},
            .effects = {},
            .outcome = core::QuestOutcome::None}}});
    return catalogue;
}

// Regarde a droite et interagit : ce que le heros en (4, 4) obtient du PNJ en (5, 4).
[[nodiscard]] std::vector<core::ExplorationEvent> parlerADroite(core::ExplorationSession& session) {
    session.update(core::ExplorationIntent{.move = {1.0F, 0.0F}, .interact = false}, 0.0001F);
    return session.update(core::ExplorationIntent{.move = {}, .interact = true}, 1.0F / 60.0F);
}

}  // namespace

/**
 * @brief Les trois propriétés de présence se lisent, et une forme fautive est nommée.
 * \castest{<b>La condition de presence se lit sur trois proprietes.</b><br/>
 * \tcat Unitaire · Quetes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire une entite sans condition, puis avec drapeau seul, drapeau et valeurs, test
 * explicite.<br/>2. Lire des formes fautives : test inconnu, `equals` sans valeur, valeurs sans
 * drapeau, drapeau non textuel.<br/>
 * \tattendu Sans condition : toujours presente. Drapeau seul : `IsSet` ; avec valeurs : `Equals`
 * sur la liste ; test explicite respecte. Chaque forme fautive donne son defaut, et l'entite reste
 * presente.
 * }
 */
TEST(EntityPresenceTest, LaConditionDePresenceSeLitSurTroisProprietes) {
    core::WorldFlags drapeaux;
    const core::MapEntity libre = garde({1, 1}, {});
    EXPECT_FALSE(core::presenceConditionOf(libre).condition.has_value());
    EXPECT_TRUE(core::isEntityPresent(libre, drapeaux));

    const auto seul = core::presenceConditionOf(garde({1, 1}, presence("porte-ouverte", "", "")));
    ASSERT_TRUE(seul.condition.has_value());
    EXPECT_EQ(seul.condition->test, core::FlagTest::IsSet);

    const auto parmi = core::presenceConditionOf(
        garde({1, 1}, presence("quete.pommes", "", "acceptee|persuasion-echouee")));
    ASSERT_TRUE(parmi.condition.has_value());
    EXPECT_EQ(parmi.condition->test, core::FlagTest::Equals);
    EXPECT_EQ(parmi.condition->values,
              (std::vector<std::string>{"acceptee", "persuasion-echouee"}));

    const auto sauf =
        core::presenceConditionOf(garde({1, 1}, presence("quete.pommes", "notEquals", "condamne")));
    ASSERT_TRUE(sauf.condition.has_value());
    EXPECT_EQ(sauf.condition->test, core::FlagTest::NotEquals);

    EXPECT_EQ(core::presenceConditionOf(garde({1, 1}, presence("f", "parmi", "a"))).issue,
              core::PresenceIssue::UnknownTest);
    EXPECT_EQ(core::presenceConditionOf(garde({1, 1}, presence("f", "equals", ""))).issue,
              core::PresenceIssue::MissingValue);
    core::PropertyMap sansDrapeau;
    sansDrapeau.emplace(std::string{core::PRESENCE_VALUE_PROPERTY}, std::string{"a"});
    EXPECT_EQ(core::presenceConditionOf(garde({1, 1}, sansDrapeau)).issue,
              core::PresenceIssue::MissingFlag);
    core::PropertyMap entier;
    entier.emplace(std::string{core::PRESENCE_FLAG_PROPERTY}, std::int64_t{3});
    const core::MapEntity fautive = garde({1, 1}, entier);
    EXPECT_EQ(core::presenceConditionOf(fautive).issue, core::PresenceIssue::WrongValueType);
    EXPECT_TRUE(core::isEntityPresent(fautive, drapeaux));
}

/**
 * @brief Un PNJ conditionné paraît et disparaît quand le drapeau change, sans recharger la carte.
 * \castest{<b>Un PNJ conditionne parait et disparait sans recharger la carte.</b><br/>
 * \tcat Unitaire · Quetes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser sur une carte un garde present sous `quete.pommes == acceptee`, en (5, 4).
 * <br/>2. Donner la quete a la session ; y entrer, heros en (4, 4) ; lui parler.<br/>3. Poser
 * `acceptee`, faire un pas, lui parler.<br/>4. Poser `enfant-libere`, faire un pas, lui parler.
 * <br/>
 * \tattendu Avant : aucun dialogue, le garde n'est pas interactif. Apres `acceptee` : le pas rend
 * l'etape de quete atteinte, et l'on parle au garde. Apres `enfant-libere` : plus de dialogue. La
 * carte n'a ete lue qu'une fois.
 * }
 */
TEST(EntityPresenceTest, UnPnjConditionneParaitEtDisparaitSansRechargerLaCarte) {
    ChargeurCompte disque{
        .donnees = carteMuree({garde({5, 4}, presence("quete.pommes", "equals", "acceptee"))})};
    core::ExplorationSession session{disque.chargeur()};
    session.setQuests(quetePommes());
    ASSERT_TRUE(session.start("parvis", ""));
    session.placeHero(core::cellCenter({4, 4}));

    EXPECT_TRUE(session.interactables().empty());
    EXPECT_TRUE(parlerADroite(session).empty());

    ASSERT_TRUE(session.flags().setValue("quete.pommes", "acceptee"));
    const std::vector<core::ExplorationEvent> pas =
        session.update(core::ExplorationIntent{}, 1.0F / 60.0F);
    ASSERT_EQ(pas.size(), 1U);
    EXPECT_EQ(pas.front().kind, ExplorationEventKind::QuestAdvanced);
    EXPECT_EQ(pas.front().value, "pommes/acceptee");
    EXPECT_EQ(session.interactables().size(), 1U);
    const std::vector<core::ExplorationEvent> parole = parlerADroite(session);
    ASSERT_EQ(parole.size(), 1U);
    EXPECT_EQ(parole.front().kind, ExplorationEventKind::Dialogue);
    EXPECT_EQ(parole.front().value, "garde");

    ASSERT_TRUE(session.flags().setValue("quete.pommes", "enfant-libere"));
    EXPECT_TRUE(session.update(core::ExplorationIntent{}, 1.0F / 60.0F).empty());
    EXPECT_TRUE(session.interactables().empty());
    EXPECT_TRUE(parlerADroite(session).empty());
    EXPECT_EQ(disque.lectures, 1);
}

/**
 * @brief Une condition de présence sur un drapeau que rien ne pose, ou mal formée, est relevée.
 * \castest{<b>Le controle releve une presence sur un drapeau jamais pose.</b><br/>
 * \tcat Unitaire · Quetes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Valider trois PNJ : l'un present sous un drapeau pose par un dialogue, l'autre sous
 * un drapeau que rien ne pose, le dernier au test inconnu.<br/>
 * \tattendu Deux problemes : `UnsetFlag` sur `presenceFlag` pour le second, `InvalidPresence` sur
 * `presenceTest` pour le dernier ; le premier passe.
 * }
 */
TEST(EntityPresenceTest, LeControleReleveUnePresenceSurUnDrapeauJamaisPose) {
    core::EntityReferenceContext contexte;
    contexte.dialogues = {"garde"};
    contexte.flags = {"quete.pommes"};
    // La quete declare la valeur que le premier attend (LOT-126 : sinon, elle serait relevee).
    contexte.flagValues = {{"quete.pommes", {"acceptee"}}};
    const std::vector<core::MapEntity> entites = {
        garde({1, 1}, presence("quete.pommes", "", "acceptee")),
        garde({2, 1}, presence("jamais-pose", "", "")),
        garde({3, 1}, presence("quete.pommes", "parmi", "acceptee"))};

    const std::vector<core::EntityIssue> problemes = core::validateMapEntities(entites, contexte);
    ASSERT_EQ(problemes.size(), 2U);
    EXPECT_EQ(problemes[0], (core::EntityIssue{.entityIndex = 1,
                                               .code = core::EntityIssueCode::UnsetFlag,
                                               .key = std::string{core::PRESENCE_FLAG_PROPERTY},
                                               .value = "jamais-pose"}));
    EXPECT_EQ(problemes[1].entityIndex, 2U);
    EXPECT_EQ(problemes[1].code, core::EntityIssueCode::InvalidPresence);
    EXPECT_EQ(problemes[1].key, core::PRESENCE_TEST_PROPERTY);
}
