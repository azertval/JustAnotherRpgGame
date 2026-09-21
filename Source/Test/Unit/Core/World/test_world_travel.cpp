// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_world_travel.cpp
 * @brief Tests du graphe du monde JOUE (LOT-09) : franchir un portail, arriver a un point nomme,
 *        garder la carte quittee, et refuser au chargement ce qui ne mene nulle part.
 */

#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Gameplay/WorldFlags.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelWriter.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/WorldGraph.h"
#include "Core/World/WorldTravel.h"

namespace {

using core::TravelResult;
using core::WorldIssueCode;

core::MapEntity portail(std::string cible, std::string arrivee, core::GridPosition position,
                        std::string drapeau = {}) {
    core::MapEntity entite{
        .type = std::string{core::PORTAL_ENTITY_TYPE},
        .position = position,
        .properties = {{std::string{core::PORTAL_TARGET_MAP_PROPERTY}, std::move(cible)},
                       {std::string{core::PORTAL_ARRIVAL_PROPERTY}, std::move(arrivee)}}};
    if (!drapeau.empty()) {
        entite.properties[std::string{core::PORTAL_REQUIRED_FLAG_PROPERTY}] = std::move(drapeau);
    }
    return entite;
}

core::MapEntity pointDArrivee(std::string nom, core::GridPosition position) {
    return core::MapEntity{
        .type = std::string{core::SPAWN_POINT_ENTITY_TYPE},
        .position = position,
        .properties = {{std::string{core::SPAWN_POINT_NAME_PROPERTY}, std::move(nom)}}};
}

// Une carte de 8 x 8 cases, son entree en (1, 1), et les entites donnees.
core::LevelData carteData(std::string nom, std::vector<core::MapEntity> entites) {
    core::LevelData donnees{.name = std::move(nom), .tileMap = core::TileMap{8, 8}};
    donnees.entry = {1, 1};
    donnees.entities = std::move(entites);
    return donnees;
}

/**
 * @brief Un dossier de niveaux en memoire : il compte ses lectures.
 *
 * Le compteur est ce qui prouve qu'un retour ne recharge pas la carte -- la seule maniere, sans
 * ecrire sur disque, de distinguer « la carte gardee » de « la carte relue a l'identique ».
 */
class DossierEnMemoire {
public:
    void poser(std::string mapId, core::LevelData donnees) {
        _cartes.emplace(std::move(mapId), std::move(donnees));
    }

    [[nodiscard]] core::WorldTravel::MapLoader chargeur() {
        return [this](std::string_view mapId) {
            ++_lectures[std::string{mapId}];
            const auto trouvee = _cartes.find(std::string{mapId});
            if (trouvee == _cartes.end()) {
                return core::LevelLoadResult{.level = std::nullopt,
                                             .error = "carte absente : " + std::string{mapId},
                                             .errorCode = core::LevelValidationError::FileNotFound};
            }
            return core::LevelLoadResult{.level = core::Level{trouvee->second},
                                         .error = {},
                                         .errorCode = core::LevelValidationError::None};
        };
    }

    [[nodiscard]] int lectures(const std::string& mapId) const {
        const auto trouvee = _lectures.find(mapId);
        return trouvee == _lectures.end() ? 0 : trouvee->second;
    }

    /// @brief Le graphe statique des cartes posees, tel que le jeu le lirait du dossier.
    [[nodiscard]] core::WorldGraph graphe() const {
        std::vector<core::WorldMapInput> entrees;
        for (const auto& [mapId, donnees] : _cartes) {
            entrees.push_back(core::WorldMapInput{
                .mapId = mapId, .name = donnees.name, .entities = donnees.entities});
        }
        return core::buildWorldGraph(std::move(entrees));
    }

private:
    std::map<std::string, core::LevelData> _cartes;
    std::map<std::string, int> _lectures;
};

/**
 * @brief Les cinq cartes de fixture, en chaine : `un` -> `deux` -> `trois` -> `quatre` -> `cinq`,
 *        chacune avec le portail de retour et son point d'arrivee.
 */
DossierEnMemoire cinqCartes() {
    DossierEnMemoire dossier;
    const std::vector<std::string> noms{"un", "deux", "trois", "quatre", "cinq"};
    for (std::size_t rang = 0; rang < noms.size(); ++rang) {
        std::vector<core::MapEntity> entites;
        if (rang + 1 < noms.size()) {
            entites.push_back(pointDArrivee("porte-avant", {2, 2}));
            entites.push_back(portail(noms[rang + 1], "porte-arriere", {6, 2}));
        }
        if (rang > 0) {
            entites.push_back(pointDArrivee("porte-arriere", {3, 3}));
            entites.push_back(portail(noms[rang - 1], "porte-avant", {0, 3}));
        }
        dossier.poser(noms[rang], carteData(noms[rang], std::move(entites)));
    }
    return dossier;
}

}  // namespace

/**
 * @brief Cinq cartes se parcourent dans les deux sens par points d'arrivee nommes.
 * \castest{<b>Un parcours de cinq cartes, aller et retour, par points d'arrivee nommes.</b><br/>
 * \tcat Unitaire · Monde parcouru<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Entrer sur la premiere carte, puis franchir quatre portails a la file.<br/>
 * 2. Revenir par les portails de retour jusqu'a la premiere carte.<br/>
 * \tattendu Chaque traversee pose le personnage sur le point d'arrivee NOMME de la carte
 * atteinte, jamais sur une coordonnee du portail de depart.
 * }
 */
TEST(WorldTravelTest, CinqCartesSeParcourentDansLesDeuxSens) {
    DossierEnMemoire dossier = cinqCartes();
    core::WorldTravel voyage{dossier.chargeur()};
    const core::WorldFlags drapeaux;

    ASSERT_EQ(voyage.enter("un", ""), TravelResult::Moved);
    EXPECT_EQ(voyage.position(), (core::GridPosition{1, 1}));

    for (const std::string_view attendue : {"deux", "trois", "quatre", "cinq"}) {
        ASSERT_EQ(voyage.cross({6, 2}, drapeaux), TravelResult::Moved) << attendue;
        EXPECT_EQ(voyage.currentMapId(), attendue);
        EXPECT_EQ(voyage.position(), (core::GridPosition{3, 3}));
    }

    for (const std::string_view attendue : {"quatre", "trois", "deux", "un"}) {
        ASSERT_EQ(voyage.cross({0, 3}, drapeaux), TravelResult::Moved) << attendue;
        EXPECT_EQ(voyage.currentMapId(), attendue);
        EXPECT_EQ(voyage.position(), (core::GridPosition{2, 2}));
    }

    // Les cinq cartes ont ete lues une fois chacune : le retour a retrouve celles qu'on avait
    // quittees, il ne les a pas relues.
    EXPECT_EQ(voyage.loadedMapCount(), 5U);
    for (const std::string& nom : {std::string{"un"}, std::string{"deux"}, std::string{"trois"},
                                   std::string{"quatre"}, std::string{"cinq"}}) {
        EXPECT_EQ(dossier.lectures(nom), 1) << nom;
    }
}

/**
 * @brief Une case sans portail n'est pas une traversee.
 * \castest{<b>Une case ordinaire ne declenche aucune traversee.</b><br/>
 * \tcat Unitaire · Monde parcouru<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Entrer sur une carte, puis tenter de franchir une case vide.<br/>
 * \tattendu `NoPortal`, la carte et la case du personnage sont inchangees.
 * }
 */
TEST(WorldTravelTest, UneCaseSansPortailNEstPasUneTraversee) {
    DossierEnMemoire dossier = cinqCartes();
    core::WorldTravel voyage{dossier.chargeur()};
    const core::WorldFlags drapeaux;

    ASSERT_EQ(voyage.enter("un", ""), TravelResult::Moved);
    EXPECT_EQ(voyage.cross({4, 4}, drapeaux), TravelResult::NoPortal);
    EXPECT_EQ(voyage.currentMapId(), "un");
    EXPECT_EQ(voyage.position(), (core::GridPosition{1, 1}));
}

/**
 * @brief Un portail a drapeau reste ferme tant que le drapeau n'est pas pose.
 * \castest{<b>Un portail exigeant un drapeau ne s'ouvre qu'une fois le drapeau pose.</b><br/>
 * \tcat Unitaire · Monde parcouru<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Franchir un portail qui exige un drapeau absent.<br/>
 * 2. Poser le drapeau, puis franchir le meme portail.<br/>
 * \tattendu `Locked` d'abord, `Moved` ensuite : le drapeau vit dans le monde, pas dans la carte.
 * }
 */
TEST(WorldTravelTest, UnPortailADrapeauResteFermeSansLeDrapeau) {
    DossierEnMemoire dossier;
    dossier.poser(
        "place",
        carteData("place", {portail("repaire", "seuil", {5, 5}, "quest/enfants/indices")}));
    dossier.poser("repaire", carteData("repaire", {pointDArrivee("seuil", {1, 4})}));

    core::WorldTravel voyage{dossier.chargeur()};
    core::WorldFlags drapeaux;
    ASSERT_EQ(voyage.enter("place", ""), TravelResult::Moved);

    EXPECT_EQ(voyage.cross({5, 5}, drapeaux), TravelResult::Locked);
    EXPECT_EQ(voyage.currentMapId(), "place");

    drapeaux.set("quest/enfants/indices");
    ASSERT_EQ(voyage.cross({5, 5}, drapeaux), TravelResult::Moved);
    EXPECT_EQ(voyage.currentMapId(), "repaire");
    EXPECT_EQ(voyage.position(), (core::GridPosition{1, 4}));
}

/**
 * @brief Un portail orphelin est refuse au chargement, pas a la traversee.
 * \castest{<b>Chaque defaut de portail devient un code exploitable au chargement.</b><br/>
 * \tcat Unitaire · Monde parcouru<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Batir un dossier ou un portail vise une carte absente et un autre un point
 * d'arrivee absent.<br/>
 * 2. Valider le graphe.<br/>
 * \tattendu Un defaut par portail fautif, nomme et situe, releve AVANT que le joueur n'y marche.
 * }
 */
TEST(WorldTravelTest, UnPortailOrphelinEstRefuseAuChargement) {
    DossierEnMemoire dossier;
    dossier.poser("place", carteData("place", {portail("nulle-part", "seuil", {5, 5}),
                                               portail("repaire", "cave", {6, 5}),
                                               portail("", "seuil", {7, 5})}));
    dossier.poser("repaire", carteData("repaire", {pointDArrivee("seuil", {1, 4})}));

    const std::vector<core::WorldIssue> defauts = core::validateWorldGraph(dossier.graphe());
    ASSERT_EQ(defauts.size(), 3U);
    EXPECT_EQ(defauts[0], (core::WorldIssue{.mapId = "place",
                                            .position = {5, 5},
                                            .code = WorldIssueCode::UnknownTargetMap,
                                            .value = "nulle-part"}));
    EXPECT_EQ(defauts[1], (core::WorldIssue{.mapId = "place",
                                            .position = {6, 5},
                                            .code = WorldIssueCode::UnknownArrivalPoint,
                                            .value = "cave"}));
    EXPECT_EQ(defauts[2].code, WorldIssueCode::MissingTargetMap);

    // Le meme dossier, joue : la traversee du portail orphelin refuse et dit pourquoi.
    core::WorldTravel voyage{dossier.chargeur()};
    ASSERT_EQ(voyage.enter("place", ""), TravelResult::Moved);
    EXPECT_EQ(voyage.cross({5, 5}, core::WorldFlags{}), TravelResult::UnreadableMap);
    ASSERT_TRUE(voyage.lastIssue().has_value());
    EXPECT_EQ(voyage.lastIssue()->code, WorldIssueCode::UnreadableMap);
}

/**
 * @brief Un point d'arrivee en double est releve sur la carte.
 * \castest{<b>Deux points d'arrivee du meme nom sont un defaut de la carte.</b><br/>
 * \tcat Unitaire · Monde parcouru<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser deux points d'arrivee « seuil » sur la meme carte.<br/>
 * 2. Valider la carte lue, puis le graphe.<br/>
 * \tattendu La carte signale le doublon, que le graphe ne peut pas voir : il deduplique les noms.
 * }
 */
TEST(WorldTravelTest, UnPointDArriveeEnDoubleEstReleveSurLaCarte) {
    const core::Level carte{
        carteData("place", {pointDArrivee("seuil", {1, 4}), pointDArrivee("seuil", {6, 6})})};

    const std::vector<core::WorldIssue> defauts = core::validateWorldMap("place", carte);
    ASSERT_EQ(defauts.size(), 1U);
    EXPECT_EQ(defauts.front(), (core::WorldIssue{.mapId = "place",
                                                 .position = {6, 6},
                                                 .code = WorldIssueCode::DuplicateArrivalPoint,
                                                 .value = "seuil"}));
    // Le premier l'emporte, et c'est bien pourquoi le doublon doit etre signale.
    const std::optional<core::GridPosition> premier = core::arrivalPointAt(carte, "seuil");
    ASSERT_TRUE(premier.has_value());
    EXPECT_EQ(*premier, (core::GridPosition{1, 4}));
}

/**
 * @brief Une arrivee inconnue refuse l'entree et nomme ce qui manque.
 * \castest{<b>Entrer par un point d'arrivee absent est refuse, le nom cite.</b><br/>
 * \tcat Unitaire · Monde parcouru<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Entrer sur une carte par un point d'arrivee qu'elle n'offre pas.<br/>
 * \tattendu `UnknownArrival`, le defaut cite le nom demande, et la carte courante ne change pas.
 * }
 */
TEST(WorldTravelTest, UneArriveeInconnueRefuseLEntree) {
    DossierEnMemoire dossier = cinqCartes();
    core::WorldTravel voyage{dossier.chargeur()};

    ASSERT_EQ(voyage.enter("un", ""), TravelResult::Moved);
    EXPECT_EQ(voyage.enter("deux", "cave"), TravelResult::UnknownArrival);
    ASSERT_TRUE(voyage.lastIssue().has_value());
    EXPECT_EQ(voyage.lastIssue()->code, WorldIssueCode::UnknownArrivalPoint);
    EXPECT_EQ(voyage.lastIssue()->value, "cave");
    EXPECT_EQ(voyage.currentMapId(), "un");
}

/**
 * @brief Le chargeur a plusieurs dossiers sert le premier qui porte la carte, et n'invente rien.
 * \castest{<b>Les cartes du brouillon passent devant celles du jeu.</b><br/>
 * \tcat Unitaire · Monde parcouru<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser deux dossiers, la meme carte dans les deux sous un nom different, et une
 * seconde carte dans le dernier seulement.<br/>
 * \tattendu La carte commune vient du PREMIER dossier ; celle qui n'est que dans le second en
 * vient ; une carte absente des deux echoue.
 * }
 */
TEST(WorldTravelTest, LesCartesDuPremierDossierPassentDevant) {
    const std::filesystem::path racine =
        std::filesystem::temp_directory_path() / "pg_world_travel_dirs";
    std::error_code erreur;
    std::filesystem::remove_all(racine, erreur);
    ASSERT_TRUE(std::filesystem::create_directories(racine / "brouillons"));
    ASSERT_TRUE(std::filesystem::create_directories(racine / "livrees"));

    // Ecrite puis RELUE : le fichier doit donc porter la case d'entree, que le chargeur exige.
    const auto surDisque = [](std::string nom) {
        core::LevelData donnees = carteData(std::move(nom), {});
        donnees.tileMap.setTile(1, 1, core::TileType::Entry);
        return core::Level{donnees};
    };
    const core::Level brouillon = surDisque("Martpart retouche");
    const core::Level livree = surDisque("Martpart livre");
    const core::Level voisine = surDisque("Arenarea");
    ASSERT_TRUE(core::LevelWriter::saveToFile(brouillon, racine / "brouillons" / "martpart.json"));
    ASSERT_TRUE(core::LevelWriter::saveToFile(livree, racine / "livrees" / "martpart.json"));
    ASSERT_TRUE(core::LevelWriter::saveToFile(voisine, racine / "livrees" / "arenarea.json"));

    const core::WorldTravel::MapLoader chargeur =
        core::WorldTravel::directoriesLoader({racine / "brouillons", racine / "livrees"});

    const core::LevelLoadResult retouchee = chargeur("martpart");
    ASSERT_TRUE(retouchee.ok()) << retouchee.error;
    EXPECT_EQ(retouchee.level->name(), "Martpart retouche");

    const core::LevelLoadResult autour = chargeur("arenarea");
    ASSERT_TRUE(autour.ok()) << autour.error;
    EXPECT_EQ(autour.level->name(), "Arenarea");

    const core::LevelLoadResult absente = chargeur("coliseum");
    EXPECT_FALSE(absente.ok());
    EXPECT_EQ(absente.errorCode, core::LevelValidationError::FileNotFound);

    std::filesystem::remove_all(racine, erreur);
}
