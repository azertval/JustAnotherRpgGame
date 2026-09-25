// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_entity_kinds.cpp
 * @brief Tests unitaires des familles d'entités de carte et de la validation de leurs références
 *        (`LOT-11`).
 */

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/Arena.h"
#include "Core/Combat/CombatTransition.h"
#include "Core/Gameplay/MapEntitySpawner.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/EntityKinds.h"

namespace {

[[nodiscard]] core::MapEntity entity(std::string type, core::PropertyMap properties = {}) {
    return core::MapEntity{
        .type = std::move(type), .position = {.column = 1, .row = 1}, .properties = properties};
}

// Contexte type : un dialogue, une rencontre, deux cartes dont une porte le point « porte-nord ».
[[nodiscard]] core::EntityReferenceContext context() {
    core::EntityReferenceContext references;
    references.dialogues = {"garde"};
    references.encounters = {"colisee-fauves"};
    references.arrivalPointsByMap["village"] = {"porte-nord"};
    references.arrivalPointsByMap["foret"] = {};
    return references;
}

[[nodiscard]] std::vector<core::EntityIssueCode> codes(
    const std::vector<core::EntityIssue>& issues) {
    std::vector<core::EntityIssueCode> result;
    for (const core::EntityIssue& issue : issues) {
        result.push_back(issue.code);
    }
    return result;
}

}  // namespace

/**
 * @brief La table rassemble les types que le gameplay lit déjà, sous leurs constantes.
 * \castest{<b>La table des familles reprend les types du gameplay.</b><br/>
 * \tcat Unitaire · Familles d'entites<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Chercher chaque type lu par le gameplay dans la table.<br/>
 * \tattendu Coffre, panneau, PNJ, rencontre, portail, point d'arrivee et entree d'arene y sont ;
 * un type inconnu n'y est pas.
 * }
 */
TEST(FamillesDEntitesTest, LaTableReprendLesTypesDuGameplay) {
    for (const std::string_view type :
         {std::string_view{"chest"}, std::string_view{"sign"}, core::NPC_ENTITY_TYPE,
          core::ENCOUNTER_ENTITY_TYPE, core::PORTAL_ENTITY_TYPE, core::SPAWN_POINT_ENTITY_TYPE,
          core::ARENA_ENTRY_ENTITY_TYPE}) {
        EXPECT_NE(core::findEntityKind(type), nullptr) << type;
    }
    EXPECT_EQ(core::findEntityKind("dragon"), nullptr);
    ASSERT_NE(core::findEntityKind(core::NPC_ENTITY_TYPE), nullptr);
    EXPECT_NE(core::findEntityKind(core::NPC_ENTITY_TYPE)->find(core::NPC_DIALOGUE_PROPERTY),
              nullptr);
}

/**
 * @brief Une entité neuve porte chaque propriété déclarée à sa valeur par défaut.
 * \castest{<b>Une entite neuve porte ses proprietes par defaut.</b><br/>
 * \tcat Unitaire · Familles d'entites<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Fabriquer une entree d'arene en (3, 4).<br/>
 * \tattendu Type, case, camp « allies » et rang 1.
 * }
 */
TEST(FamillesDEntitesTest, EntiteNeuvePorteSesDefauts) {
    const core::EntityKind* const kind = core::findEntityKind(core::ARENA_ENTRY_ENTITY_TYPE);
    ASSERT_NE(kind, nullptr);
    const core::MapEntity made = core::makeEntity(*kind, {.column = 3, .row = 4});
    EXPECT_EQ(made.type, core::ARENA_ENTRY_ENTITY_TYPE);
    EXPECT_EQ(made.position, (core::GridPosition{.column = 3, .row = 4}));
    EXPECT_EQ(made.properties.at(std::string{core::ARENA_SIDE_PROPERTY}),
              core::PropertyValue{std::string{"allies"}});
    EXPECT_EQ(made.properties.at(std::string{core::ARENA_RANK_PROPERTY}),
              core::PropertyValue{std::int64_t{1}});
}

/**
 * @brief Des entités bien renseignées ne lèvent aucun problème.
 * \castest{<b>Une carte bien renseignee est muette.</b><br/>
 * \tcat Unitaire · Familles d'entites<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Valider un PNJ, une rencontre, un portail vers un point connu, un point d'arrivee et
 * un PNJ figurant.<br/>
 * \tattendu Aucun probleme.
 * }
 */
TEST(FamillesDEntitesTest, CarteBienRenseigneeEstMuette) {
    const std::vector<core::MapEntity> entities = {
        entity("npc", {{"dialogue", std::string{"garde"}}}),
        entity("npc"),
        entity("encounter", {{"encounterId", std::string{"colisee-fauves"}}, {"respawns", true}}),
        entity("portal",
               {{"targetMap", std::string{"village"}}, {"arrival", std::string{"porte-nord"}}}),
        entity("spawnPoint", {{"name", std::string{"puits"}}}),
    };
    EXPECT_TRUE(core::validateMapEntities(entities, context()).empty());
}

/**
 * @brief Chaque référence cassée est nommée, avec la propriété et la valeur en cause.
 * \castest{<b>Les references cassees sont nommees.</b><br/>
 * \tcat Unitaire · Familles d'entites<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Valider un PNJ au dialogue inconnu, une rencontre inconnue, un portail vers une
 * carte inconnue et un portail vers un point absent d'une carte connue.<br/>
 * \tattendu Un probleme chacun, dans l'ordre des entites ; le portail vers une carte inconnue ne
 * signale pas en plus son point d'arrivee.
 * }
 */
TEST(FamillesDEntitesTest, ReferencesCasseesNommees) {
    const std::vector<core::MapEntity> entities = {
        entity("npc", {{"dialogue", std::string{"inconnu"}}}),
        entity("encounter", {{"encounterId", std::string{"dragons"}}}),
        entity("portal", {{"targetMap", std::string{"nulle-part"}}, {"arrival", std::string{"x"}}}),
        entity("portal",
               {{"targetMap", std::string{"foret"}}, {"arrival", std::string{"lisiere"}}}),
    };
    const std::vector<core::EntityIssue> issues = core::validateMapEntities(entities, context());
    ASSERT_EQ(issues.size(), 4U);
    EXPECT_EQ(issues[0], (core::EntityIssue{.entityIndex = 0,
                                            .code = core::EntityIssueCode::UnknownDialogue,
                                            .key = "dialogue",
                                            .value = "inconnu"}));
    EXPECT_EQ(issues[1].code, core::EntityIssueCode::UnknownEncounter);
    EXPECT_EQ(issues[2], (core::EntityIssue{.entityIndex = 2,
                                            .code = core::EntityIssueCode::UnknownTargetMap,
                                            .key = "targetMap",
                                            .value = "nulle-part"}));
    EXPECT_EQ(issues[3], (core::EntityIssue{.entityIndex = 3,
                                            .code = core::EntityIssueCode::UnknownArrivalPoint,
                                            .key = "arrival",
                                            .value = "lisiere"}));
}

/**
 * @brief Propriétés manquantes, vides, mal typées ou hors liste, et type inconnu.
 * \castest{<b>Les proprietes mal renseignees sont signalees.</b><br/>
 * \tcat Unitaire · Familles d'entites<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Valider une rencontre sans rencontre, un portail a cible vide, une entree d'arene au
 * rang textuel et au camp inconnu, et un type inconnu.<br/>
 * \tattendu Manquante, manquante (cible) et manquante (arrivee), camp hors liste, rang mal type,
 * type inconnu.
 * }
 */
TEST(FamillesDEntitesTest, ProprietesMalRenseigneesSignalees) {
    const std::vector<core::MapEntity> entities = {
        entity("encounter"),
        entity("portal", {{"targetMap", std::string{}}}),
        entity("arenaEntry",
               {{"side", std::string{"spectateurs"}}, {"rank", std::string{"premier"}}}),
        entity("dragon"),
    };
    EXPECT_EQ(codes(core::validateMapEntities(entities, context())),
              (std::vector<core::EntityIssueCode>{
                  core::EntityIssueCode::MissingProperty, core::EntityIssueCode::MissingProperty,
                  core::EntityIssueCode::MissingProperty, core::EntityIssueCode::InvalidChoice,
                  core::EntityIssueCode::WrongValueType, core::EntityIssueCode::UnknownType}));
}

/**
 * @brief Deux points d'arrivée du même nom : le second est signalé, et les noms se relèvent sans
 * doublon.
 * \castest{<b>Un nom de point d'arrivee est unique dans la carte.</b><br/>
 * \tcat Unitaire · Familles d'entites<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser deux points « puits » et un point « gue ».<br/>2. Valider et relever les
 * noms.<br/>
 * \tattendu Un doublon, sur le second « puits » ; les noms sont gue et puits.
 * }
 */
TEST(FamillesDEntitesTest, NomDePointDArriveeUnique) {
    const std::vector<core::MapEntity> entities = {
        entity("spawnPoint", {{"name", std::string{"puits"}}}),
        entity("spawnPoint", {{"name", std::string{"gue"}}}),
        entity("spawnPoint", {{"name", std::string{"puits"}}}),
    };
    const std::vector<core::EntityIssue> issues = core::validateMapEntities(entities, context());
    ASSERT_EQ(issues.size(), 1U);
    EXPECT_EQ(issues[0].entityIndex, 2U);
    EXPECT_EQ(issues[0].code, core::EntityIssueCode::DuplicateArrivalPoint);
    EXPECT_EQ(core::arrivalPointNames(entities),
              (std::set<std::string, std::less<>>{"gue", "puits"}));
}

/**
 * @brief **Contrat d'extension** (§5, règle 2 de la feuille de route de l'éditeur) : toute famille
 * d'entité que le jeu lit est dans `core::knownEntityKinds`, et reçoit donc inspecteur, forme et
 * contrôle sans code d'éditeur.
 * \castest{<b>Aucune famille lue par le jeu n'echappe a l'editeur.</b><br/>
 * \tcat Unitaire · Familles d'entites<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Relever dans les sources du jeu (Core, HMI, App) chaque constante
 * <code>*_ENTITY_TYPE</code>.<br/>2. Y ajouter les familles interactives
 * (<code>core::knownInteractableKinds</code>) et chaque type des cartes livrees.<br/>3. Chercher
 * chacune dans la table.<br/>
 * \tattendu Toutes y sont ; le releve trouve au moins les onze familles d'aujourd'hui.
 * }
 */
TEST(FamillesDEntitesTest, ToutFamilleLueParLeJeuEstDansLaTable) {
    const std::filesystem::path source(JADG_SOURCE_DIR);
    // Pas de chaine brute : Doxygen la lit mal, et perd les blocs de documentation qui suivent.
    const std::regex constant(
        "inline\\s+constexpr\\s+std::string_view\\s+\\w+_ENTITY_TYPE\\s*=\\s*\"([^\"]+)\"");
    std::map<std::string, std::string, std::less<>> read;  // type -> ou il est lu
    for (const char* const part : {"Core", "HMI", "App"}) {
        for (const auto& file : std::filesystem::recursive_directory_iterator(source / part)) {
            if (file.path().extension() != ".h" && file.path().extension() != ".cpp") {
                continue;
            }
            std::ifstream stream(file.path());
            const std::string text((std::istreambuf_iterator<char>(stream)),
                                   std::istreambuf_iterator<char>());
            for (auto match = std::sregex_iterator(text.begin(), text.end(), constant);
                 match != std::sregex_iterator(); ++match) {
                read.emplace((*match)[1].str(), file.path().filename().string());
            }
        }
    }
    for (const core::InteractableKind& kind : core::knownInteractableKinds()) {
        read.emplace(std::string{kind.type}, "MapEntitySpawner.cpp");
    }
    for (const auto& file :
         std::filesystem::recursive_directory_iterator(source / "Elements" / "Levels")) {
        if (file.path().extension() != ".json" ||
            file.path().filename().string().ends_with(".editor.json")) {
            continue;
        }
        const core::LevelLoadResult loaded = core::LevelLoader::loadFromFile(file.path());
        ASSERT_TRUE(loaded.ok()) << file.path() << " : " << loaded.error;
        for (const core::MapEntity& entity : loaded.level->entities()) {
            read.emplace(entity.type, file.path().filename().string());
        }
    }
    EXPECT_GE(read.size(), 11U);
    for (const auto& [type, where] : read) {
        EXPECT_NE(core::findEntityKind(type), nullptr)
            << "La famille \"" << type << "\" (" << where
            << ") est lue par le jeu mais absente de core::knownEntityKinds : l'editeur ne sait ni "
               "la poser, ni l'inspecter, ni la controler (EX-EDIT-073).";
    }
}

/**
 * @brief Chaque forme a ce qu'il lui faut : un rectangle ou une zone déclare sa largeur et sa
 * hauteur en entiers d'au moins 1, une étiquette et une figurine nomment une propriété déclarée.
 * \castest{<b>La table des familles est coherente avec ses formes.</b><br/>
 * \tcat Unitaire · Familles d'entites<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Parcourir la table.<br/>
 * \tattendu Largeur et hauteur entieres, bornees a 1, pour chaque rectangle et chaque zone ;
 * l'etiquette et la figurine sont des proprietes declarees.
 * }
 */
TEST(FamillesDEntitesTest, LaTableEstCoherenteAvecSesFormes) {
    for (const core::EntityKind& kind : core::knownEntityKinds()) {
        if (kind.shape == core::EntityShape::Rectangle || kind.shape == core::EntityShape::Area) {
            for (const std::string_view key :
                 {core::SHAPE_WIDTH_PROPERTY, core::SHAPE_HEIGHT_PROPERTY}) {
                const core::EntityPropertySpec* const spec = kind.find(key);
                ASSERT_NE(spec, nullptr) << kind.type << "." << key;
                EXPECT_EQ(spec->kind, core::EntityPropertyKind::Integer) << kind.type;
                EXPECT_EQ(spec->minimum, 1) << kind.type;
            }
        }
        if (!kind.labelProperty.empty()) {
            EXPECT_NE(kind.find(kind.labelProperty), nullptr) << kind.type;
        }
        if (!kind.figureProperty.empty()) {
            ASSERT_NE(kind.find(kind.figureProperty), nullptr) << kind.type;
            EXPECT_EQ(kind.find(kind.figureProperty)->source, core::EntityChoiceSource::Figures);
        }
    }
}

/**
 * @brief Un entier hors de ses bornes, et une référence absente de son catalogue, sont signalés.
 * \castest{<b>Le schema type controle bornes et catalogues.</b><br/>
 * \tcat Unitaire · Familles d'entites<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Une zone de combat de largeur 0 ; une entree d'arene de rang 0.<br/>2. Un PNJ a
 * figurine inconnue, gardant un lieu inconnu ; un portail exigeant un drapeau que rien ne
 * pose.<br/>
 * 3. Les memes, references connues.<br/>
 * \tattendu Deux valeurs hors bornes ; figurine, lieu, drapeau signales ; puis rien.
 * }
 */
TEST(FamillesDEntitesTest, BornesEtCataloguesControles) {
    core::EntityReferenceContext references = context();
    const std::vector<core::MapEntity> bounded = {
        entity("combatZone", {{"name", std::string{"sable"}},
                              {"width", std::int64_t{0}},
                              {"height", std::int64_t{3}}}),
        entity("arenaEntry", {{"side", std::string{"allies"}}, {"rank", std::int64_t{0}}}),
    };
    EXPECT_EQ(codes(core::validateMapEntities(bounded, references)),
              (std::vector<core::EntityIssueCode>{core::EntityIssueCode::OutOfRange,
                                                  core::EntityIssueCode::OutOfRange}));

    const std::vector<core::MapEntity> referenced = {
        entity("npc", {{"figure", std::string{"anariel"}}, {"guards", std::string{"oldtown"}}}),
        entity("portal", {{"targetMap", std::string{"village"}},
                          {"arrival", std::string{"porte-nord"}},
                          {"requiresFlag", std::string{"pont-repare"}}}),
    };
    EXPECT_EQ(codes(core::validateMapEntities(referenced, references)),
              (std::vector<core::EntityIssueCode>{core::EntityIssueCode::UnknownFigure,
                                                  core::EntityIssueCode::UnknownLocation,
                                                  core::EntityIssueCode::UnsetFlag}));
    references.figures = {"anariel"};
    references.locations = {"oldtown"};
    references.flags = {"pont-repare"};
    EXPECT_TRUE(core::validateMapEntities(referenced, references).empty());
}
