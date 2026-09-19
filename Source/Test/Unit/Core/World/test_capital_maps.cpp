// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_capital_maps.cpp
 * @brief Les cartes des quartiers de la Capitale, telles qu'elles sont LIVREES (LOT-96) : elles
 *        se chargent, elles se tiennent, et tout ce qu'elles nomment existe.
 *
 * Tests de CONTENU, comme ceux du Colisee (`test_coliseum_map.cpp`) : une piece de planche absente,
 * un point d'arrivee dans un mur ou une ruelle inatteignable ne se voient dans aucun test de code.
 * Chaque quartier qui recoit sa carte entre dans la liste `QUARTIERS`, et passe les memes
 * controles.
 */

#include <algorithm>
#include <deque>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Gameplay/WorldFlags.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/CityBlock.h"
#include "Core/World/CityPlan.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/WorldTravel.h"
#include "HMI/Graphics/PlaceAppearance.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

const std::filesystem::path NIVEAUX{JADG_LEVELS_DIR};
const std::filesystem::path ASSETS{JADG_ASSETS_DIR};
const std::filesystem::path MONDE{JADG_WORLD_DIR};

/// @brief Un quartier livre : son identifiant de carte, son lieu, sa taille.
struct Quartier {
    const char* carte;
    const char* nom;
    /// Le champ `name` de la carte : une cle de traduction (LOT-EDITOR-07).
    const char* cleDuNom;
    const char* lieu;
    int largeur;
    int hauteur;
};

/// @brief Ce que CTest affiche du parametre : l'identifiant de carte, pas ses octets.
void PrintTo(const Quartier& quartier, std::ostream* flux) {
    *flux << quartier.carte;
}

// Les quartiers qui ont leur carte. Les dix autres ont une porte gardee (LOT-96, phase 4).
const Quartier QUARTIERS[] = {
    {"capital/martpart", "Martpart", "map.capital.martpart.name", "martpart", 48, 40},
    // Arenarea emprunte la planche de Martpart, faute de planche propre (LOT-96, phase 2).
    {"capital/arenarea", "Arenarea", "map.capital.arenarea.name", "martpart", 48, 40},
};

[[nodiscard]] core::Level charger(const Quartier& quartier) {
    core::LevelLoadResult lu =
        core::LevelLoader::loadFromFile(NIVEAUX / (std::string{quartier.carte} + ".json"));
    EXPECT_TRUE(lu.ok()) << quartier.carte << " : " << lu.error;
    return lu.level.has_value() ? std::move(*lu.level)
                                : core::Level{core::LevelData{.tileMap = core::TileMap{1, 1}}};
}

[[nodiscard]] hmi::PlaceAppearance tableDu(const Quartier& quartier) {
    hmi::PlaceAppearanceResult table =
        hmi::PlaceAppearance::loadFromFile(ASSETS / "Scene" / quartier.lieu / "appearance.json");
    EXPECT_TRUE(table.ok()) << quartier.lieu << " : " << table.message;
    return std::move(table.appearance);
}

/// @return La valeur texte de la propriete @p cle de @p entite, vide sinon.
[[nodiscard]] std::string texteDe(const core::MapEntity& entite, std::string_view cle) {
    const auto trouvee = entite.properties.find(std::string{cle});
    if (trouvee == entite.properties.end()) {
        return {};
    }
    const std::string* texte = std::get_if<std::string>(&trouvee->second);
    return texte != nullptr ? *texte : std::string{};
}

/// @return La case du premier portail de @p carte vers @p cible, `{-1, -1}` s'il n'y en a pas.
[[nodiscard]] core::GridPosition portailVers(const core::Level& carte, std::string_view cible) {
    for (const core::MapEntity& entite : carte.entities()) {
        const std::optional<core::PortalTarget> portail = core::portalAt(carte, entite.position);
        if (entite.type == core::PORTAL_ENTITY_TYPE && portail && portail->map == cible) {
            return entite.position;
        }
    }
    return {-1, -1};
}

/// @return La case d'entree de la carte (tuile `entry`), `{-1, -1}` s'il n'y en a pas.
[[nodiscard]] core::GridPosition entreeDe(const core::Level& carte) {
    const core::TileMap& grille = carte.tileMap();
    for (int ligne = 0; ligne < grille.height(); ++ligne) {
        for (int colonne = 0; colonne < grille.width(); ++colonne) {
            if (grille.tile(colonne, ligne) == core::TileType::Entry) {
                return {colonne, ligne};
            }
        }
    }
    return {-1, -1};
}

/// @return Les cases franchissables atteintes depuis @p depart, par pas orthogonaux.
[[nodiscard]] std::set<std::pair<int, int>> atteintes(const core::TileMap& grille,
                                                      core::GridPosition depart) {
    std::set<std::pair<int, int>> vues{{depart.column, depart.row}};
    std::deque<std::pair<int, int>> file{{depart.column, depart.row}};
    while (!file.empty()) {
        const auto [colonne, ligne] = file.front();
        file.pop_front();
        const std::pair<int, int> voisins[4] = {
            {colonne + 1, ligne}, {colonne - 1, ligne}, {colonne, ligne + 1}, {colonne, ligne - 1}};
        for (const auto& [x, y] : voisins) {
            if (!grille.inBounds(x, y) || grille.isSolid(x, y) || vues.contains({x, y})) {
                continue;
            }
            vues.insert({x, y});
            file.push_back({x, y});
        }
    }
    return vues;
}

class CapitalMapTest : public ::testing::TestWithParam<Quartier> {};

}  // namespace

/**
 * @brief La carte livree se charge, et c'est un lieu, pas une grille nue.
 * \castest{<b>Chaque quartier livre se charge a sa taille, avec son lieu et une camera qui
 * suit.</b><br/>
 * \tcat Unitaire · Quartiers de la Capitale<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger `Source/Elements/Levels/capital/<quartier>.json`.<br/>
 * \tattendu Aucun defaut de chargement ; le nom, la taille, le lieu de planches et une entree ;
 * aucun defaut de carte (`core::validateWorldMap`).
 * }
 */
TEST_P(CapitalMapTest, LaCarteLivreeSeCharge) {
    const Quartier& quartier = GetParam();
    const core::Level carte = charger(quartier);

    EXPECT_EQ(carte.name(), quartier.cleDuNom);
    EXPECT_EQ(carte.tileMap().width(), quartier.largeur);
    EXPECT_EQ(carte.tileMap().height(), quartier.hauteur);
    EXPECT_EQ(hmi::scenePlaceOf(carte), quartier.lieu);
    EXPECT_NE(entreeDe(carte).column, -1) << "aucune tuile d'entree";
    EXPECT_TRUE(core::validateWorldMap(quartier.carte, carte).empty());
}

/**
 * @brief Chaque piece nommee par la carte existe sur la planche du lieu.
 * \castest{<b>Toute piece qu'un quartier demande existe dans Assets/Scene/\<lieu\>/.</b><br/>
 * \tcat Unitaire · Quartiers de la Capitale<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Tirer l'instantane du quartier avec la table d'apparence livree.<br/>
 * 2. Verifier que chaque chemin de texture demande existe sur le disque.<br/>
 * \tattendu Aucun chemin manquant : une piece absente tomberait sur le damier.
 * }
 */
TEST_P(CapitalMapTest, ChaquePieceNommeeExisteSurLaPlanche) {
    const Quartier& quartier = GetParam();
    const hmi::WorldSceneSnapshot instantane =
        hmi::snapshotWorldScene(charger(quartier), tableDu(quartier), {});
    const std::vector<std::string> chemins = hmi::worldTexturePaths(instantane);
    EXPECT_GT(chemins.size(), 10U);
    for (const std::string& chemin : chemins) {
        EXPECT_TRUE(std::filesystem::exists(ASSETS / chemin)) << "piece absente : " << chemin;
    }
}

/**
 * @brief Tout le quartier se parcourt depuis son entree, et rien n'est sans sol.
 * \castest{<b>Chaque case franchissable d'un quartier est atteinte depuis son entree, et porte
 * un sol.</b><br/>
 * \tcat Unitaire · Quartiers de la Capitale<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Remplir la carte depuis la tuile d'entree, en ne passant que par le
 * franchissable.<br/>
 * 2. Comparer aux cases franchissables de la carte.<br/>
 * \tattendu Aucune ruelle, aucune cour inatteignable ; aucune case franchissable sans piece de
 * sol : on ne marche pas dans le vide.
 * }
 */
TEST_P(CapitalMapTest, ToutLeQuartierSeParcourtDepuisSonEntree) {
    const Quartier& quartier = GetParam();
    const core::Level carte = charger(quartier);
    const core::TileMap& grille = carte.tileMap();
    const std::set<std::pair<int, int>> vues = atteintes(grille, entreeDe(carte));
    const hmi::WorldSceneSnapshot instantane =
        hmi::snapshotWorldScene(carte, tableDu(quartier), {});

    for (int ligne = 0; ligne < grille.height(); ++ligne) {
        for (int colonne = 0; colonne < grille.width(); ++colonne) {
            if (grille.isSolid(colonne, ligne)) {
                continue;
            }
            EXPECT_TRUE(vues.contains({colonne, ligne}))
                << quartier.carte << " : inatteignable (" << colonne << ", " << ligne << ")";
            EXPECT_FALSE(instantane.floorAt({colonne, ligne}).empty())
                << quartier.carte << " : case franchissable sans sol (" << colonne << ", " << ligne
                << ")";
        }
    }
}

/**
 * @brief Chaque point d'arrivee est pose sur une case ou l'on peut se tenir.
 * \castest{<b>Aucun point d'arrivee d'un quartier n'est dans un mur.</b><br/>
 * \tcat Unitaire · Quartiers de la Capitale<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Relever les entites `spawnPoint` de chaque quartier.<br/>
 * \tattendu Chacune est sur une case franchissable : un portail qui y mene poserait sinon le
 * heros dans une facade.
 * }
 */
TEST_P(CapitalMapTest, ChaquePointDArriveeEstFranchissable) {
    const Quartier& quartier = GetParam();
    const core::Level carte = charger(quartier);
    int points = 0;
    for (const core::MapEntity& entite : carte.entities()) {
        if (entite.type != core::SPAWN_POINT_ENTITY_TYPE) {
            continue;
        }
        ++points;
        EXPECT_FALSE(carte.tileMap().isSolid(entite.position.column, entite.position.row))
            << quartier.carte << " : point d'arrivee dans un mur (" << entite.position.column
            << ", " << entite.position.row << ")";
    }
    EXPECT_GT(points, 0);
}

/**
 * @brief Chaque quartier a ses ilots, dans sa carte, avec leur libelle dans les deux langues.
 * \castest{<b>Les ilots d'un quartier sont dans sa carte, et nommes en francais et en
 * anglais.</b><br/>
 * \tcat Unitaire · Quartiers de la Capitale<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire les ilots de chaque quartier.<br/>
 * 2. Chercher `city_block.<nom>` dans `fr.lang` et `en.lang`.<br/>
 * \tattendu Au moins trois ilots, chacun dans les bornes de la carte, et libelle dans les deux
 * catalogues ; l'entree du quartier est dans un ilot (LOT-96).
 * }
 */
TEST_P(CapitalMapTest, LesIlotsSontDansLaCarteEtNommes) {
    const Quartier& quartier = GetParam();
    const core::Level carte = charger(quartier);
    const std::vector<core::CityBlock> ilots = core::cityBlocksOf(carte);
    EXPECT_GE(ilots.size(), 3U);

    const auto catalogue = [](const char* langue) {
        std::ifstream flux(NIVEAUX.parent_path() / "Localization" / langue);
        std::stringstream texte;
        texte << flux.rdbuf();
        return texte.str();
    };
    const std::string francais = catalogue("fr.lang");
    const std::string anglais = catalogue("en.lang");
    for (const core::CityBlock& ilot : ilots) {
        EXPECT_GE(ilot.origin.column, 0);
        EXPECT_GE(ilot.origin.row, 0);
        EXPECT_LE(ilot.origin.column + ilot.columns, carte.tileMap().width()) << ilot.name;
        EXPECT_LE(ilot.origin.row + ilot.rows, carte.tileMap().height()) << ilot.name;
        const std::string cle = "\ncity_block." + ilot.name + " = ";
        EXPECT_NE(francais.find(cle), std::string::npos) << "fr.lang : " << ilot.name;
        EXPECT_NE(anglais.find(cle), std::string::npos) << "en.lang : " << ilot.name;
    }
    const core::GridPosition entree = entreeDe(carte);
    EXPECT_TRUE(std::ranges::any_of(
        ilots, [entree](const core::CityBlock& ilot) { return ilot.contains(entree); }))
        << quartier.carte << " : l'entree n'est dans aucun ilot";
}

/**
 * @brief Martpart -> Arenarea -> Martpart ramene a la bonne case, sans rien recharger.
 * \castest{<b>L'avenue se parcourt aller et retour, par points d'arrivee nommes, et la carte
 * quittee est conservee.</b><br/>
 * \tcat Unitaire · Quartiers de la Capitale<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Entrer a Martpart par le chargeur du jeu, sur le dossier des niveaux livre.<br/>
 * 2. Franchir le portail vers Arenarea, puis celui du retour.<br/>
 * \tattendu A Arenarea, le heros est au point « martpart » ; de retour, au point « arenarea » de
 * Martpart. Deux cartes chargees en tout, et Martpart est la meme carte qu'a l'aller : l'etat
 * de la carte quittee est conserve (LOT-96).
 * }
 */
TEST(CapitalTravelTest, MartpartArenareaMartpartRameneALaBonneCase) {
    core::WorldTravel voyage{core::WorldTravel::directoryLoader(NIVEAUX)};
    const core::WorldFlags drapeaux;

    ASSERT_EQ(voyage.enter("capital/martpart", ""), core::TravelResult::Moved);
    const core::Level* const martpart = voyage.currentMap();
    ASSERT_NE(martpart, nullptr);
    const core::GridPosition aller = portailVers(*martpart, "capital/arenarea");
    ASSERT_NE(aller.column, -1) << "Martpart n'a pas de portail vers Arenarea";

    ASSERT_EQ(voyage.cross(aller, drapeaux), core::TravelResult::Moved);
    EXPECT_EQ(voyage.currentMapId(), "capital/arenarea");
    const core::Level* const arenarea = voyage.currentMap();
    ASSERT_NE(arenarea, nullptr);
    EXPECT_EQ(voyage.position(), core::arrivalPointAt(*arenarea, "martpart"));
    const core::GridPosition retour = portailVers(*arenarea, "capital/martpart");
    ASSERT_NE(retour.column, -1) << "Arenarea n'a pas de portail vers Martpart";

    ASSERT_EQ(voyage.cross(retour, drapeaux), core::TravelResult::Moved);
    EXPECT_EQ(voyage.currentMapId(), "capital/martpart");
    EXPECT_EQ(voyage.position(), core::arrivalPointAt(*martpart, "arenarea"));
    EXPECT_EQ(voyage.currentMap(), martpart) << "Martpart a ete rechargee au retour";
    EXPECT_EQ(voyage.loadedMapCount(), 2U);
}

/**
 * @brief Chaque quartier ferme a sa sentinelle, sur la carte que la ville lui donne.
 * \castest{<b>Les dix portes gardees ont chacune leur sentinelle Ironhand, qui ouvre un dialogue
 * du catalogue.</b><br/>
 * \tcat Unitaire · Quartiers de la Capitale<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire la Capitale livree et le catalogue des dialogues.<br/>
 * 2. Pour chaque quartier ferme, relever les PNJ de sa carte de garde qui le gardent.<br/>
 * \tattendu Exactement une sentinelle par quartier ferme, au bord de la carte, sur une case ou
 * l'on peut aller lui parler, avec un dialogue accepte et une figurine qui a au moins son
 * marqueur ; aucune sentinelle ne garde un quartier qui a sa carte (LOT-96).
 * }
 */
TEST(CapitalGuardTest, ChaqueQuartierFermeASaSentinelle) {
    const core::CityPlanResult ville = core::loadCityPlan(MONDE / "cities" / "capital.json");
    ASSERT_TRUE(ville.ok()) << ville.error;
    const core::DialogueCatalog dialogues = core::loadDialogues(MONDE / "dialogues");

    int fermes = 0;
    for (const core::CityDistrict& quartier : ville.plan.districts) {
        const core::LevelLoadResult lu = core::LevelLoader::loadFromFile(
            NIVEAUX / ((quartier.hasMap() ? quartier.map : quartier.guardMap) + ".json"));
        ASSERT_TRUE(lu.ok()) << quartier.id << " : " << lu.error;
        const core::Level& carte = *lu.level;
        const core::TileMap& grille = carte.tileMap();

        int sentinelles = 0;
        for (const core::MapEntity& entite : carte.entities()) {
            if (entite.type != core::NPC_ENTITY_TYPE ||
                texteDe(entite, core::NPC_GUARDED_DISTRICT_PROPERTY) != quartier.id) {
                continue;
            }
            ++sentinelles;
            const std::string dialogue = texteDe(entite, core::NPC_DIALOGUE_PROPERTY);
            EXPECT_NE(dialogues.find(dialogue), nullptr) << quartier.id << " : " << dialogue;
            EXPECT_FALSE(
                hmi::figureMarkerKey(
                    hmi::figureStripPath(texteDe(entite, core::NPC_FIGURE_PROPERTY), "idle"))
                    .empty())
                << quartier.id << " : sentinelle sans figurine ni marqueur";
            const core::GridPosition position = entite.position;
            EXPECT_TRUE(position.column == 0 || position.row == 0 ||
                        position.column == grille.width() - 1 ||
                        position.row == grille.height() - 1)
                << quartier.id << " : la sentinelle n'est pas au bord de la carte";
            EXPECT_FALSE(grille.isSolid(position.column, position.row))
                << quartier.id << " : la sentinelle est dans un mur";
        }
        if (quartier.hasMap()) {
            EXPECT_EQ(sentinelles, 0) << quartier.id << " a sa carte, et une sentinelle";
        } else {
            ++fermes;
            EXPECT_EQ(sentinelles, 1) << quartier.id;
        }
    }
    EXPECT_EQ(fermes, 10);
}

INSTANTIATE_TEST_SUITE_P(Quartiers, CapitalMapTest, ::testing::ValuesIn(QUARTIERS),
                         [](const ::testing::TestParamInfo<Quartier>& info) {
                             return std::string{info.param.nom};
                         });
