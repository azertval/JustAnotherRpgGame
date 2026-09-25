// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_world_graph.cpp
 * @brief Tests du graphe statique du monde (LOT-11) : cartes, points d'arrivee, portails et leur
 *        statut, cartes injoignables, et lecture d'un dossier de niveaux.
 */

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/MapEntity.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/WorldGraph.h"

namespace {

using core::PortalLinkStatus;

// Un portail vers (@p cible, @p arrivee), pose en @p position.
core::MapEntity portail(std::string cible, std::string arrivee, core::GridPosition position = {}) {
    return core::MapEntity{
        .type = std::string{core::PORTAL_ENTITY_TYPE},
        .position = position,
        .properties = {{std::string{core::PORTAL_TARGET_MAP_PROPERTY}, std::move(cible)},
                       {std::string{core::PORTAL_ARRIVAL_PROPERTY}, std::move(arrivee)}}};
}

// Un point d'arrivee nomme @p nom.
core::MapEntity pointDArrivee(std::string nom) {
    return core::MapEntity{
        .type = std::string{core::SPAWN_POINT_ENTITY_TYPE},
        .properties = {{std::string{core::SPAWN_POINT_NAME_PROPERTY}, std::move(nom)}}};
}

core::WorldMapInput carte(std::string id, std::vector<core::MapEntity> entites = {}) {
    return core::WorldMapInput{.mapId = id, .name = id, .entities = std::move(entites)};
}

// Le statut de l'unique portail de la carte "depart", lance vers une carte "cible".
PortalLinkStatus statutUnique(core::MapEntity lePortail, core::WorldMapInput cible) {
    const core::WorldGraph graphe =
        core::buildWorldGraph({carte("depart", {std::move(lePortail)}), std::move(cible)});
    EXPECT_EQ(graphe.portals.size(), 1U);
    return graphe.portals.empty() ? PortalLinkStatus::Resolved : graphe.portals.front().status;
}

// Une carte version 3 minimale : une entree, puis les entites donnees en JSON.
std::string carteJson(const std::string& nom, const std::string& entites) {
    return R"({"version": 3, "name": ")" + nom + R"(", "width": 4, "height": 3, "tiles": [
    { "x": 0, "y": 0, "type": "entry" }
  ], "entities": [)" +
           entites + "]}";
}

// Fournit un dossier temporaire vierge par test (cree/supprime automatiquement).
class WorldGraphFileTest : public ::testing::Test {
protected:
    std::filesystem::path dir;

    void SetUp() override {
        dir = std::filesystem::temp_directory_path() /
              ("pg_worldgraph_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);
    }
    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(dir, error);
    }

    void ecrire(const std::string& fichier, const std::string& contenu) const {
        std::ofstream(dir / fichier, std::ios::binary) << contenu;
    }
};

}  // namespace

/**
 * @brief Un aller-retour A -> B -> A se resout dans les deux sens.
 * \castest{<b>Un aller-retour A -> B -> A se resout dans les deux sens.</b><br/>
 * \tcat Unitaire · Graphe du monde<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Construire deux cartes, chacune avec un point d'arrivee et un portail vers
 * l'autre.<br/>
 * 2. Lire les portails sortants et entrants de chaque carte.<br/>
 * \tattendu Deux portails resolus, un sortant et un entrant par carte, aucune carte injoignable.
 * }
 */
TEST(WorldGraphTest, UnAllerRetourSeResoutDansLesDeuxSens) {
    const core::WorldGraph graphe = core::buildWorldGraph({
        carte("village", {pointDArrivee("porte-est"), portail("foret", "lisiere", {3, 1})}),
        carte("foret", {pointDArrivee("lisiere"), portail("village", "porte-est", {0, 2})}),
    });

    ASSERT_EQ(graphe.portals.size(), 2U);
    for (const core::WorldPortalLink& lien : graphe.portals) {
        EXPECT_EQ(lien.status, PortalLinkStatus::Resolved) << lien.fromMap << " -> " << lien.toMap;
    }
    ASSERT_EQ(graphe.portalsFrom("village").size(), 1U);
    EXPECT_EQ(*graphe.portalsFrom("village").front(),
              (core::WorldPortalLink{.fromMap = "village",
                                     .position = {3, 1},
                                     .toMap = "foret",
                                     .arrival = "lisiere",
                                     .status = PortalLinkStatus::Resolved}));
    ASSERT_EQ(graphe.portalsTo("village").size(), 1U);
    EXPECT_EQ(graphe.portalsTo("village").front()->fromMap, "foret");
    EXPECT_TRUE(graphe.unreachableFrom("village").empty());
    EXPECT_TRUE(graphe.unreachableFrom("foret").empty());
}

/**
 * @brief Chaque defaut de portail recoit son statut, dans l'ordre de resolution annonce.
 * \castest{<b>Chaque defaut de portail recoit son statut.</b><br/>
 * \tcat Unitaire · Graphe du monde<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Construire un portail par defaut : cible vide, carte inconnue, carte illisible,
 * arrivee vide, arrivee inconnue.<br/>
 * 2. Combiner deux defauts (carte illisible et arrivee vide).<br/>
 * \tattendu Un statut distinct par defaut ; la carte illisible l'emporte sur l'arrivee vide.
 * }
 */
TEST(WorldGraphTest, ChaqueDefautDePortailRecoitSonStatut) {
    const core::WorldMapInput cibleSaine = carte("cible", {pointDArrivee("quai")});
    core::WorldMapInput cibleIllisible = carte("cible");
    cibleIllisible.loadError = "JSON malforme";

    EXPECT_EQ(statutUnique(portail("", "quai"), cibleSaine), PortalLinkStatus::MissingTarget);
    EXPECT_EQ(statutUnique(portail("", ""), cibleSaine), PortalLinkStatus::MissingTarget);
    EXPECT_EQ(statutUnique(portail("ailleurs", "quai"), cibleSaine), PortalLinkStatus::UnknownMap);
    EXPECT_EQ(statutUnique(portail("cible", "quai"), cibleIllisible),
              PortalLinkStatus::TargetUnreadable);
    EXPECT_EQ(statutUnique(portail("cible", ""), cibleIllisible),
              PortalLinkStatus::TargetUnreadable);
    EXPECT_EQ(statutUnique(portail("cible", ""), cibleSaine), PortalLinkStatus::MissingArrival);
    EXPECT_EQ(statutUnique(portail("cible", "gare"), cibleSaine), PortalLinkStatus::UnknownArrival);
    EXPECT_EQ(statutUnique(portail("cible", "quai"), cibleSaine), PortalLinkStatus::Resolved);
}

/**
 * @brief Une propriete non textuelle vaut une propriete vide.
 * \castest{<b>Une propriete non textuelle vaut une propriete vide.</b><br/>
 * \tcat Unitaire · Graphe du monde<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser un portail dont la cible est un entier, puis un dont l'arrivee est un
 * booleen.<br/>
 * 2. Poser un point d'arrivee dont le nom est un entier.<br/>
 * \tattendu Cible manquante, arrivee manquante, et aucun point d'arrivee retenu.
 * }
 */
TEST(WorldGraphTest, UneProprieteNonTextuelleVautVide) {
    core::MapEntity cibleEntiere = portail("cible", "quai");
    cibleEntiere.properties[std::string{core::PORTAL_TARGET_MAP_PROPERTY}] = std::int64_t{7};
    core::MapEntity arriveeBooleenne = portail("cible", "quai");
    arriveeBooleenne.properties[std::string{core::PORTAL_ARRIVAL_PROPERTY}] = true;
    core::MapEntity nomEntier = pointDArrivee("");
    nomEntier.properties[std::string{core::SPAWN_POINT_NAME_PROPERTY}] = std::int64_t{3};

    const core::WorldMapInput cible = carte("cible", {pointDArrivee("quai"), nomEntier});
    EXPECT_EQ(statutUnique(cibleEntiere, cible), PortalLinkStatus::MissingTarget);
    EXPECT_EQ(statutUnique(arriveeBooleenne, cible), PortalLinkStatus::MissingArrival);

    const core::WorldGraph graphe = core::buildWorldGraph({cible});
    ASSERT_NE(graphe.find("cible"), nullptr);
    EXPECT_EQ(graphe.find("cible")->arrivalPoints, (std::vector<std::string>{"quai"}));
}

/**
 * @brief Les points d'arrivee sont tries, sans doublon ni nom vide.
 * \castest{<b>Les points d'arrivee sont tries, sans doublon ni nom vide.</b><br/>
 * \tcat Unitaire · Graphe du monde<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Poser quatre points d'arrivee dans le desordre, dont un doublon et un nom vide.<br/>
 * \tattendu Deux noms, dans l'ordre alphabetique.
 * }
 */
TEST(WorldGraphTest, LesPointsDArriveeSontTriesSansDoublon) {
    const core::WorldGraph graphe =
        core::buildWorldGraph({carte("port", {pointDArrivee("quai"), pointDArrivee(""),
                                              pointDArrivee("digue"), pointDArrivee("quai")})});
    ASSERT_NE(graphe.find("port"), nullptr);
    EXPECT_EQ(graphe.find("port")->arrivalPoints, (std::vector<std::string>{"digue", "quai"}));
}

/**
 * @brief Un portail vers sa propre carte est legal et se resout.
 * \castest{<b>Un portail vers sa propre carte est legal.</b><br/>
 * \tcat Unitaire · Graphe du monde<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Construire une carte dont le portail ramene a un de ses propres points
 * d'arrivee.<br/>
 * \tattendu Le portail est resolu, sortant et entrant pour la meme carte.
 * }
 */
TEST(WorldGraphTest, UnPortailVersSaPropreCarteEstLegal) {
    const core::WorldGraph graphe = core::buildWorldGraph(
        {carte("donjon", {pointDArrivee("oubliettes"), portail("donjon", "oubliettes")})});
    ASSERT_EQ(graphe.portals.size(), 1U);
    EXPECT_EQ(graphe.portals.front().status, PortalLinkStatus::Resolved);
    EXPECT_EQ(graphe.portalsFrom("donjon").size(), 1U);
    EXPECT_EQ(graphe.portalsTo("donjon").size(), 1U);
    EXPECT_TRUE(graphe.unreachableFrom("donjon").empty());
}

/**
 * @brief Les cartes injoignables suivent le sens des portails resolus seulement.
 * \castest{<b>Les cartes injoignables suivent le sens des portails resolus.</b><br/>
 * \tcat Unitaire · Graphe du monde<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Relier A -> B par un portail resolu, B -> C par un portail casse ; D ne mene qu'a
 * A.<br/>
 * 2. Demander les cartes injoignables depuis A, puis depuis une carte inexistante.<br/>
 * \tattendu Depuis A : C et D, tries. Depuis une carte inexistante : toutes les cartes.
 * }
 */
TEST(WorldGraphTest, LesCartesInjoignablesSuiventLesPortailsResolus) {
    const core::WorldGraph graphe = core::buildWorldGraph({
        carte("d", {portail("a", "centre")}),
        carte("c", {pointDArrivee("seuil")}),
        carte("b", {pointDArrivee("seuil"), portail("c", "mauvais-nom")}),
        carte("a", {pointDArrivee("centre"), portail("b", "seuil")}),
    });

    EXPECT_EQ(graphe.unreachableFrom("a"), (std::vector<std::string>{"c", "d"}));
    EXPECT_EQ(graphe.unreachableFrom("d"), (std::vector<std::string>{"c"}));
    EXPECT_EQ(graphe.unreachableFrom("nulle-part"), (std::vector<std::string>{"a", "b", "c", "d"}));
}

/**
 * @brief Le graphe ne depend pas de l'ordre des cartes recues.
 * \castest{<b>Le graphe ne depend pas de l'ordre des cartes recues.</b><br/>
 * \tcat Unitaire · Graphe du monde<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Construire le meme monde avec les cartes dans deux ordres opposes.<br/>
 * \tattendu Cartes triees par identifiant, portails par carte source puis ordre des entites,
 * identiques dans les deux cas.
 * }
 */
TEST(WorldGraphTest, LOrdreDuGrapheEstDeterministe) {
    const std::vector<core::WorldMapInput> monde{
        carte("zeta", {portail("alpha", "x", {1, 0}), portail("", "", {2, 0})}),
        carte("alpha", {pointDArrivee("x"), portail("zeta", "y", {5, 5})}),
        carte("mu"),
    };
    const core::WorldGraph endroit = core::buildWorldGraph(monde);
    const core::WorldGraph envers =
        core::buildWorldGraph(std::vector<core::WorldMapInput>(monde.rbegin(), monde.rend()));

    ASSERT_EQ(endroit.maps.size(), 3U);
    EXPECT_EQ(endroit.maps[0].mapId, "alpha");
    EXPECT_EQ(endroit.maps[1].mapId, "mu");
    EXPECT_EQ(endroit.maps[2].mapId, "zeta");
    ASSERT_EQ(endroit.portals.size(), 3U);
    EXPECT_EQ(endroit.portals[0].fromMap, "alpha");
    EXPECT_EQ(endroit.portals[1].position, (core::GridPosition{1, 0}));
    EXPECT_EQ(endroit.portals[2].position, (core::GridPosition{2, 0}));
    EXPECT_EQ(endroit.portals, envers.portals);
    ASSERT_EQ(envers.maps.size(), endroit.maps.size());
    for (std::size_t rang = 0; rang < endroit.maps.size(); ++rang) {
        EXPECT_EQ(envers.maps[rang].mapId, endroit.maps[rang].mapId);
    }
}

/**
 * @brief Un dossier de niveaux se lit en graphe, carte illisible comprise.
 * \castest{<b>Un dossier de niveaux se lit en graphe, carte illisible comprise.</b><br/>
 * \tcat Unitaire · Graphe du monde<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ecrire deux cartes valides reliees par des portails, un fichier JSON malforme, un
 * fichier `sequence-*.json` et un fichier non JSON.<br/>
 * 2. Charger le graphe du dossier.<br/>
 * \tattendu Trois cartes (identifiant = nom de fichier), l'illisible avec son erreur ; le portail
 * vers elle est `TargetUnreadable`, l'aller-retour est resolu.
 * }
 */
TEST_F(WorldGraphFileTest, UnDossierDeNiveauxSeLitEnGraphe) {
    ecrire("village.json",
           carteJson("Village", R"({ "type": "spawnPoint", "x": 1, "y": 1, "name": "place" },
               { "type": "portal", "x": 3, "y": 1, "targetMap": "foret", "arrival": "lisiere" },
               { "type": "portal", "x": 2, "y": 2, "targetMap": "grotte", "arrival": "entree" })"));
    ecrire("foret.json",
           carteJson("Foret", R"({ "type": "spawnPoint", "x": 0, "y": 1, "name": "lisiere" },
               { "type": "portal", "x": 0, "y": 2, "targetMap": "village", "arrival": "place" })"));
    ecrire("grotte.json", "{ ceci n'est pas du JSON");
    ecrire("sequence-demo.json", R"({"steps": []})");
    ecrire("README.md", "# pas une carte");

    const core::WorldGraph graphe = core::loadWorldGraph(dir);

    ASSERT_EQ(graphe.maps.size(), 3U);
    EXPECT_EQ(graphe.maps[0].mapId, "foret");
    EXPECT_EQ(graphe.maps[1].mapId, "grotte");
    EXPECT_EQ(graphe.maps[2].mapId, "village");
    EXPECT_EQ(graphe.maps[2].name, "Village");
    EXPECT_TRUE(graphe.maps[2].loadError.empty());
    EXPECT_FALSE(graphe.maps[1].loadError.empty());
    EXPECT_EQ(graphe.maps[0].arrivalPoints, (std::vector<std::string>{"lisiere"}));

    ASSERT_EQ(graphe.portals.size(), 3U);
    EXPECT_EQ(graphe.portals[0].status, PortalLinkStatus::Resolved);
    EXPECT_EQ(graphe.portals[1].status, PortalLinkStatus::Resolved);
    EXPECT_EQ(graphe.portals[2].toMap, "grotte");
    EXPECT_EQ(graphe.portals[2].status, PortalLinkStatus::TargetUnreadable);
    EXPECT_EQ(graphe.unreachableFrom("village"), (std::vector<std::string>{"grotte"}));
}

/**
 * @brief Une carte d'un sous-dossier a pour identifiant son chemin relatif.
 * \castest{<b>Les cartes d'un sous-dossier entrent au graphe sous leur chemin relatif.</b><br/>
 * \tcat Unitaire · Graphe du monde<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ecrire `capital/martpart.json` et `capital/arenarea.json`, reliees par portails, et
 * `coliseum.json` a la racine.<br/>
 * 2. Charger le graphe du dossier.<br/>
 * \tattendu Trois cartes, `capital/arenarea`, `capital/martpart` et `coliseum` ; les portails
 * entre quartiers se resolvent (LOT-96).
 * }
 */
TEST_F(WorldGraphFileTest, UneCarteDUnSousDossierAPourIdentifiantSonCheminRelatif) {
    std::filesystem::create_directories(dir / "capital");
    ecrire("capital/martpart.json",
           carteJson("Martpart", R"({ "type": "spawnPoint", "x": 1, "y": 1, "name": "arenarea" },
               { "type": "portal", "x": 0, "y": 1, "targetMap": "capital/arenarea", "arrival": "martpart" })"));
    ecrire("capital/arenarea.json",
           carteJson("Arenarea", R"({ "type": "spawnPoint", "x": 2, "y": 1, "name": "martpart" },
               { "type": "portal", "x": 3, "y": 1, "targetMap": "capital/martpart", "arrival": "arenarea" })"));
    ecrire("coliseum.json",
           carteJson("Colisee", R"({ "type": "spawnPoint", "x": 1, "y": 1, "name": "porte" })"));

    const core::WorldGraph graphe = core::loadWorldGraph(dir);

    ASSERT_EQ(graphe.maps.size(), 3U);
    EXPECT_EQ(graphe.maps[0].mapId, "capital/arenarea");
    EXPECT_EQ(graphe.maps[1].mapId, "capital/martpart");
    EXPECT_EQ(graphe.maps[2].mapId, "coliseum");
    ASSERT_EQ(graphe.portals.size(), 2U);
    EXPECT_EQ(graphe.portals[0].status, PortalLinkStatus::Resolved);
    EXPECT_EQ(graphe.portals[1].status, PortalLinkStatus::Resolved);
    EXPECT_EQ(core::mapIdOf(dir, dir / "capital" / "martpart.json"), "capital/martpart");
}

/**
 * @brief Un dossier absent donne un graphe vide, sans lever.
 * \castest{<b>Un dossier absent donne un graphe vide.</b><br/>
 * \tcat Unitaire · Graphe du monde<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Charger le graphe d'un dossier qui n'existe pas.<br/>
 * \tattendu Aucune carte, aucun portail, aucune exception.
 * }
 */
TEST_F(WorldGraphFileTest, UnDossierAbsentDonneUnGrapheVide) {
    core::WorldGraph graphe;
    EXPECT_NO_THROW(graphe = core::loadWorldGraph(dir / "inexistant"));
    EXPECT_TRUE(graphe.maps.empty());
    EXPECT_TRUE(graphe.portals.empty());
}
