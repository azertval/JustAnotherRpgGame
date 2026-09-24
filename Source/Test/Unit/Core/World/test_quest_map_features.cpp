// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_quest_map_features.cpp
 * @brief Tests de ce que la quête demande aux cartes (`LOT-126`) : valeurs de drapeau contrôlées,
 *        portail condamné, zone à déclencheur, décor qui change.
 */

#include <algorithm>
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
#include "Core/Rpg/Dialogue.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/EntityPresence.h"
#include "Core/World/ExplorationSession.h"
#include "Core/World/WorldGraph.h"
#include "Core/World/WorldTravel.h"

namespace {

using core::ExplorationEventKind;

[[nodiscard]] core::MapEntity entite(std::string_view type, core::GridPosition position,
                                     core::PropertyMap proprietes, std::string id = {}) {
    return core::MapEntity{.type = std::string{type},
                           .position = position,
                           .properties = std::move(proprietes),
                           .id = std::move(id)};
}

// Une carte de @p largeur x 3, ouverte, son entree en (0, 1).
[[nodiscard]] core::LevelData couloir(std::string nom, int largeur,
                                      std::vector<core::MapEntity> entites) {
    core::LevelData donnees{.name = std::move(nom), .tileMap = core::TileMap{largeur, 3}};
    donnees.entry = {0, 1};
    donnees.entities = std::move(entites);
    return donnees;
}

// Un monde en memoire : identifiant de carte -> carte.
[[nodiscard]] core::WorldTravel::MapLoader monde(std::map<std::string, core::LevelData> cartes) {
    return [cartes = std::move(cartes)](std::string_view id) {
        const auto trouvee = cartes.find(std::string{id});
        if (trouvee == cartes.end()) {
            return core::LevelLoadResult{.level = std::nullopt,
                                         .error = "absente",
                                         .errorCode = core::LevelValidationError::FileNotFound};
        }
        return core::LevelLoadResult{.level = core::Level{trouvee->second},
                                     .error = {},
                                     .errorCode = core::LevelValidationError::None};
    };
}

[[nodiscard]] core::QuestCatalog quetePommes() {
    core::QuestCatalog catalogue;
    catalogue.quests.push_back(core::Quest{
        .id = "pommes",
        .flags = {core::QuestFlag{.id = "quete.pommes",
                                  .values = {"inconnue", "acceptee", "condamne", "enfant-libere"},
                                  .initial = "inconnue"}},
        .steps = {}});
    return catalogue;
}

// Marche vers la droite de @p cases cases, par petits pas.
std::vector<core::ExplorationEvent> marcher(core::ExplorationSession& session, float cases) {
    std::vector<core::ExplorationEvent> tous;
    const float pas = 1.0F / 60.0F;
    const int n =
        static_cast<int>(cases / (core::ExplorationSession::WALK_SPEED_CELLS_PER_SECOND * pas));
    for (int i = 0; i < n; ++i) {
        std::vector<core::ExplorationEvent> evenements =
            session.update(core::ExplorationIntent{.move = {1.0F, 0.0F}, .interact = false}, pas);
        tous.insert(tous.end(), evenements.begin(), evenements.end());
    }
    return tous;
}

[[nodiscard]] bool contient(const std::vector<core::ExplorationEvent>& evenements,
                            ExplorationEventKind genre, std::string_view valeur) {
    return std::ranges::any_of(evenements, [&](const core::ExplorationEvent& evenement) {
        return evenement.kind == genre && evenement.value == valeur;
    });
}

}  // namespace

/**
 * @brief La condition de présence est déclarée pour toute famille, et ses valeurs se proposent.
 * \castest{<b>La presence se declare au contrat des familles.</b><br/>
 * \tcat Unitaire · Familles d'entites<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire les proprietes inspectees d'un PNJ et d'un coffre.<br/>
 * \tattendu Les deux portent `presenceFlag` (drapeaux), `presenceTest` (quatre mots) et
 * `presenceValue` (valeurs du drapeau que nomme `presenceFlag`) ; aucune n'est posee a la
 * creation.
 * }
 */
TEST(QuestMapFeaturesTest, LaPresenceSeDeclarePourToutFamille) {
    for (const std::string_view type : {core::NPC_ENTITY_TYPE, std::string_view{"chest"}}) {
        const core::EntityKind* const kind = core::findEntityKind(type);
        ASSERT_NE(kind, nullptr);
        const core::EntityPropertySpec* const valeur =
            core::findInspectedProperty(*kind, core::PRESENCE_VALUE_PROPERTY);
        ASSERT_NE(valeur, nullptr) << type;
        EXPECT_EQ(valeur->source, core::EntityChoiceSource::FlagValues);
        EXPECT_EQ(valeur->relatedKey, core::PRESENCE_FLAG_PROPERTY);
        EXPECT_NE(core::findInspectedProperty(*kind, core::PRESENCE_FLAG_PROPERTY), nullptr);
        EXPECT_EQ(
            core::findInspectedProperty(*kind, core::PRESENCE_TEST_PROPERTY)->fixedChoices.size(),
            4U);
        const core::MapEntity neuve = core::makeEntity(*kind, {0, 0});
        EXPECT_FALSE(neuve.properties.contains(std::string{core::PRESENCE_FLAG_PROPERTY}));
    }
}

/**
 * @brief Une valeur qu'aucune quête ne déclare est relevée, qu'une entité la lise ou la pose.
 * \castest{<b>Le controle refuse une valeur qu'aucune quete ne declare.</b><br/>
 * \tcat Unitaire · Quetes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Un PNJ present sous `acceptee|acepte`, une zone qui pose `quete.pommes` sans
 * valeur, une zone qui pose `acceptee`, une zone qui donne une valeur a un fait.<br/>
 * \tattendu `UndeclaredFlagValue` pour `acepte` et pour la valeur du fait ; `MissingProperty`
 * pour la zone qui pose un drapeau declare sans valeur ; rien pour la zone juste.
 * }
 */
TEST(QuestMapFeaturesTest, UneValeurQuAucuneQueteNeDeclareEstRelevee) {
    core::EntityReferenceContext contexte;
    contexte.flags = {"quete.pommes", "vu"};
    contexte.flagValues = {{"quete.pommes", {"inconnue", "acceptee"}}};
    const std::vector<core::MapEntity> entites = {
        entite("chest", {0, 0},
               {{std::string{core::PRESENCE_FLAG_PROPERTY}, std::string{"quete.pommes"}},
                {std::string{core::PRESENCE_VALUE_PROPERTY}, std::string{"acceptee|acepte"}}}),
        entite(core::ZONE_ENTITY_TYPE, {1, 0},
               {{std::string{core::ZONE_TRIGGER_FLAG_PROPERTY}, std::string{"quete.pommes"}}}),
        entite(core::ZONE_ENTITY_TYPE, {2, 0},
               {{std::string{core::ZONE_TRIGGER_FLAG_PROPERTY}, std::string{"quete.pommes"}},
                {std::string{core::ZONE_TRIGGER_VALUE_PROPERTY}, std::string{"acceptee"}}}),
        entite(core::ZONE_ENTITY_TYPE, {3, 0},
               {{std::string{core::ZONE_TRIGGER_FLAG_PROPERTY}, std::string{"vu"}},
                {std::string{core::ZONE_TRIGGER_VALUE_PROPERTY}, std::string{"oui"}}})};

    const std::vector<core::EntityIssue> problemes = core::validateMapEntities(entites, contexte);
    ASSERT_EQ(problemes.size(), 3U);
    EXPECT_EQ(problemes[0], (core::EntityIssue{.entityIndex = 0,
                                               .code = core::EntityIssueCode::UndeclaredFlagValue,
                                               .key = std::string{core::PRESENCE_VALUE_PROPERTY},
                                               .value = "acepte"}));
    EXPECT_EQ(problemes[1].entityIndex, 1U);
    EXPECT_EQ(problemes[1].code, core::EntityIssueCode::MissingProperty);
    EXPECT_EQ(problemes[1].key, core::ZONE_TRIGGER_VALUE_PROPERTY);
    EXPECT_EQ(problemes[2].entityIndex, 3U);
    EXPECT_EQ(problemes[2].code, core::EntityIssueCode::UndeclaredFlagValue);
}

/**
 * @brief Un portail condamné n'exige ni cible ni arrivée, et le graphe ne le compte pas en faute.
 * \castest{<b>Un portail condamne est legal ; le meme, non condamne, est une faute.</b><br/>
 * \tcat Unitaire · Graphe du monde<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Une carte portant un escalier `sealed` sans cible.<br/>2. Valider ses entites et
 * son graphe ; recommencer sans `sealed`.<br/>
 * \tattendu Condamne : aucun probleme, statut `Sealed`, aucun defaut de graphe. Sans `sealed` :
 * `targetMap` et `arrival` manquent, et le graphe releve `MissingTargetMap`.
 * }
 */
TEST(QuestMapFeaturesTest, UnPortailCondamneEstLegal) {
    core::MapEntity escalier = entite(core::PORTAL_ENTITY_TYPE, {2, 1},
                                      {{std::string{core::PORTAL_SEALED_PROPERTY}, true}});
    EXPECT_TRUE(core::validateMapEntities({escalier}, {}).empty());
    core::WorldGraph graphe = core::buildWorldGraph(
        {core::WorldMapInput{.mapId = "arene", .name = "arene", .entities = {escalier}}});
    ASSERT_EQ(graphe.portals.size(), 1U);
    EXPECT_EQ(graphe.portals[0].status, core::PortalLinkStatus::Sealed);
    EXPECT_TRUE(core::validateWorldGraph(graphe).empty());

    escalier.properties.erase(std::string{core::PORTAL_SEALED_PROPERTY});
    const std::vector<core::EntityIssue> problemes = core::validateMapEntities({escalier}, {});
    ASSERT_EQ(problemes.size(), 2U);
    EXPECT_EQ(problemes[0].code, core::EntityIssueCode::MissingProperty);
    graphe = core::buildWorldGraph(
        {core::WorldMapInput{.mapId = "arene", .name = "arene", .entities = {escalier}}});
    const std::vector<core::WorldIssue> defauts = core::validateWorldGraph(graphe);
    ASSERT_EQ(defauts.size(), 1U);
    EXPECT_EQ(defauts[0].code, core::WorldIssueCode::MissingTargetMap);
}

/**
 * @brief On marche sur un portail condamné : il le dit, et l'on reste.
 * \castest{<b>Un portail condamne ne s'ouvre pas.</b><br/>
 * \tcat Unitaire · Exploration<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Un couloir dont la case (2, 1) porte un escalier condamne vers `catacombes`.<br/>
 * 2. Marcher dessus.<br/>
 * \tattendu Un evenement `PortalSealed` qui nomme `catacombes` ; la carte ne change pas.
 * }
 */
TEST(QuestMapFeaturesTest, UnPortailCondamneNeSOuvrePas) {
    core::ExplorationSession session(
        monde({{"arene", couloir("arene", 6,
                                 {entite(core::PORTAL_ENTITY_TYPE, {2, 1},
                                         {{std::string{core::PORTAL_SEALED_PROPERTY}, true},
                                          {std::string{core::PORTAL_TARGET_MAP_PROPERTY},
                                           std::string{"catacombes"}}})})}}));
    ASSERT_TRUE(session.start("arene", ""));
    const std::vector<core::ExplorationEvent> evenements = marcher(session, 2.2F);
    EXPECT_TRUE(contient(evenements, ExplorationEventKind::PortalSealed, "catacombes"));
    EXPECT_EQ(session.mapId(), "arene");
}

/**
 * @brief Une zone déclenche à l'entrée : drapeau posé, dialogue, transfert — une fois si voulu.
 * \castest{<b>La zone du parvis transfere au vestiaire A.</b><br/>
 * \tcat Unitaire · Exploration<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Un parvis dont la zone (3..4, 1) pose `quete.pommes = condamne`, ouvre le dialogue
 * `garde` et transfere au point `vestiaire-a` de l'arene ; presente sous `acceptee` seulement.
 * <br/>2. Marcher sur la zone sous `inconnue`, puis sous `acceptee`.<br/>3. Construire le graphe.
 * <br/>
 * \tattendu Sous `inconnue`, rien. Sous `acceptee` : le drapeau vaut `condamne`, un dialogue
 * `garde`, le heros est au vestiaire A. Le graphe porte une arete `Transfer` resolue.
 * }
 */
TEST(QuestMapFeaturesTest, LaZoneDuParvisTransfereAuVestiaire) {
    const core::MapEntity zone =
        entite(core::ZONE_ENTITY_TYPE, {3, 1},
               {{std::string{core::ZONE_WIDTH_PROPERTY}, std::int64_t{2}},
                {std::string{core::ZONE_HEIGHT_PROPERTY}, std::int64_t{1}},
                {std::string{core::ZONE_TRIGGER_FLAG_PROPERTY}, std::string{"quete.pommes"}},
                {std::string{core::ZONE_TRIGGER_VALUE_PROPERTY}, std::string{"condamne"}},
                {std::string{core::ZONE_TRIGGER_DIALOGUE_PROPERTY}, std::string{"garde"}},
                {std::string{core::ZONE_TRIGGER_MAP_PROPERTY}, std::string{"arene"}},
                {std::string{core::ZONE_TRIGGER_ARRIVAL_PROPERTY}, std::string{"vestiaire-a"}},
                {std::string{core::PRESENCE_FLAG_PROPERTY}, std::string{"quete.pommes"}},
                {std::string{core::PRESENCE_VALUE_PROPERTY}, std::string{"acceptee"}}});
    const core::MapEntity vestiaire =
        entite(core::SPAWN_POINT_ENTITY_TYPE, {1, 1},
               {{std::string{core::SPAWN_POINT_NAME_PROPERTY}, std::string{"vestiaire-a"}}});
    const std::map<std::string, core::LevelData> cartes = {
        {"parvis", couloir("parvis", 8, {zone})}, {"arene", couloir("arene", 4, {vestiaire})}};

    core::ExplorationSession libre(monde(cartes));
    libre.setQuests(quetePommes());
    ASSERT_TRUE(libre.start("parvis", ""));
    EXPECT_TRUE(marcher(libre, 4.0F).empty());
    EXPECT_EQ(libre.flags().value("quete.pommes"), "inconnue");

    core::ExplorationSession session(monde(cartes));
    session.setQuests(quetePommes());
    ASSERT_TRUE(session.flags().setValue("quete.pommes", "acceptee"));
    ASSERT_TRUE(session.start("parvis", ""));
    const std::vector<core::ExplorationEvent> evenements = marcher(session, 2.6F);
    EXPECT_EQ(session.flags().value("quete.pommes"), "condamne");
    EXPECT_TRUE(contient(evenements, ExplorationEventKind::Dialogue, "garde"));
    EXPECT_TRUE(contient(evenements, ExplorationEventKind::MapEntered, "arene"));
    EXPECT_EQ(session.mapId(), "arene");
    EXPECT_EQ(session.heroCell(), (core::GridPosition{1, 1}));

    const core::WorldGraph graphe =
        core::buildWorldGraph({core::WorldMapInput{.mapId = "parvis", .entities = {zone}},
                               core::WorldMapInput{.mapId = "arene", .entities = {vestiaire}}});
    ASSERT_EQ(graphe.portals.size(), 1U);
    EXPECT_EQ(graphe.portals[0].kind, core::WorldLinkKind::Transfer);
    EXPECT_EQ(graphe.portals[0].status, core::PortalLinkStatus::Resolved);
    EXPECT_EQ(graphe.find("parvis")->triggerFlags, std::vector<std::string>{"quete.pommes"});
}

/**
 * @brief Une zone « une fois » ne redéclenche pas ; on n'entre pas dans une zone où l'on arrive.
 * \castest{<b>Une zone une fois ne se declenche qu'une fois.</b><br/>
 * \tcat Unitaire · Exploration<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Un couloir dont l'entree est dans une zone a dialogue, et une zone `triggerOnce` en
 * (3, 1).<br/>2. Demarrer ; marcher a travers la seconde, revenir, repasser.<br/>
 * \tattendu Rien au depart ; un seul dialogue `annonce`.
 * }
 */
TEST(QuestMapFeaturesTest, UneZoneUneFoisNeSeDeclenchePasDeuxFois) {
    const std::map<std::string, core::LevelData> cartes = {
        {"rue",
         couloir(
             "rue", 8,
             {entite(core::ZONE_ENTITY_TYPE, {0, 1},
                     {{std::string{core::ZONE_TRIGGER_DIALOGUE_PROPERTY}, std::string{"accueil"}}}),
              entite(core::ZONE_ENTITY_TYPE, {3, 1},
                     {{std::string{core::ZONE_TRIGGER_DIALOGUE_PROPERTY}, std::string{"annonce"}},
                      {std::string{core::ZONE_TRIGGER_ONCE_PROPERTY}, true}})})}};
    core::ExplorationSession session(monde(cartes));
    ASSERT_TRUE(session.start("rue", ""));
    std::vector<core::ExplorationEvent> evenements = marcher(session, 4.0F);
    session.placeHero(core::cellCenter({1, 1}));
    const std::vector<core::ExplorationEvent> retour = marcher(session, 4.0F);
    evenements.insert(evenements.end(), retour.begin(), retour.end());
    EXPECT_FALSE(contient(evenements, ExplorationEventKind::Dialogue, "accueil"));
    EXPECT_EQ(std::ranges::count_if(evenements,
                                    [](const core::ExplorationEvent& evenement) {
                                        return evenement.value == "annonce";
                                    }),
              1);
}

/**
 * @brief Un décor présent arrête le pas sur son emprise ; absent, on passe — sans recharger.
 * \castest{<b>Les portes de l'arene sont closes sous condamne.</b><br/>
 * \tcat Unitaire · Exploration<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Un couloir barre en (3, 0..2) par une porte `prop` d'emprise 1 x 3, presente sous
 * `condamne`.<br/>2. Poser `condamne` et marcher ; puis `enfant-libere` et marcher encore.<br/>
 * \tattendu La porte arrete le heros avant la colonne 3 ; une fois ouverte, il la franchit.
 * }
 */
TEST(QuestMapFeaturesTest, LesPortesDeLAreneSontClosesSousCondamne) {
    const core::MapEntity porte =
        entite(core::PROP_ENTITY_TYPE, {3, 0},
               {{std::string{core::PROP_PIECE_PROPERTY}, std::string{"door"}},
                {std::string{core::PROP_BLOCKS_PROPERTY}, true},
                {std::string{core::SHAPE_WIDTH_PROPERTY}, std::int64_t{1}},
                {std::string{core::SHAPE_HEIGHT_PROPERTY}, std::int64_t{3}},
                {std::string{core::PRESENCE_FLAG_PROPERTY}, std::string{"quete.pommes"}},
                {std::string{core::PRESENCE_VALUE_PROPERTY}, std::string{"condamne"}}});
    core::ExplorationSession session(monde({{"arene", couloir("arene", 8, {porte})}}));
    session.setQuests(quetePommes());
    ASSERT_TRUE(session.start("arene", ""));
    ASSERT_TRUE(session.flags().setValue("quete.pommes", "condamne"));
    marcher(session, 5.0F);
    EXPECT_EQ(session.heroCell().column, 2);
    EXPECT_EQ(session.blockedByProps().size(), 3U);

    ASSERT_TRUE(session.flags().setValue("quete.pommes", "enfant-libere"));
    marcher(session, 3.0F);
    EXPECT_GE(session.heroCell().column, 4);
    EXPECT_TRUE(session.blockedByProps().empty());
}
