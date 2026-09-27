// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_exploration_session.cpp
 * @brief Tests de la session d'exploration du jeu (LOT-09) : marcher, buter sur un mur, franchir
 *        un portail, parler a ce qu'on regarde.
 */

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/ExplorationSession.h"
#include "Core/World/FollowTrail.h"

namespace {

using core::ExplorationEventKind;

core::MapEntity portail(std::string cible, std::string arrivee, core::GridPosition position) {
    return core::MapEntity{
        .type = std::string{core::PORTAL_ENTITY_TYPE},
        .position = position,
        .properties = {{std::string{core::PORTAL_TARGET_MAP_PROPERTY}, std::move(cible)},
                       {std::string{core::PORTAL_ARRIVAL_PROPERTY}, std::move(arrivee)}}};
}

core::MapEntity pointDArrivee(std::string nom, core::GridPosition position) {
    return core::MapEntity{
        .type = std::string{core::SPAWN_POINT_ENTITY_TYPE},
        .position = position,
        .properties = {{std::string{core::SPAWN_POINT_NAME_PROPERTY}, std::move(nom)}}};
}

core::MapEntity pnj(std::string dialogue, core::GridPosition position) {
    return core::MapEntity{
        .type = std::string{core::NPC_ENTITY_TYPE},
        .position = position,
        .properties = {{std::string{core::NPC_DIALOGUE_PROPERTY}, std::move(dialogue)}}};
}

// Une carte de 10 x 10 cases entouree de murs, son entree en (1, 1).
core::LevelData carteMuree(std::string nom, std::vector<core::MapEntity> entites) {
    core::TileMap grille{10, 10};
    for (int colonne = 0; colonne < 10; ++colonne) {
        grille.setTile(colonne, 0, core::TileType::Wall);
        grille.setTile(colonne, 9, core::TileType::Wall);
    }
    for (int ligne = 0; ligne < 10; ++ligne) {
        grille.setTile(0, ligne, core::TileType::Wall);
        grille.setTile(9, ligne, core::TileType::Wall);
    }
    core::LevelData donnees{.name = std::move(nom), .tileMap = std::move(grille)};
    donnees.entry = {1, 1};
    donnees.entities = std::move(entites);
    return donnees;
}

class DossierEnMemoire {
public:
    void poser(std::string mapId, core::LevelData donnees) {
        _cartes.emplace(std::move(mapId), std::move(donnees));
    }

    [[nodiscard]] core::WorldTravel::MapLoader chargeur() const {
        return [this](std::string_view mapId) {
            const auto trouvee = _cartes.find(std::string{mapId});
            if (trouvee == _cartes.end()) {
                return core::LevelLoadResult{.level = std::nullopt,
                                             .error = "carte absente",
                                             .errorCode = core::LevelValidationError::FileNotFound};
            }
            return core::LevelLoadResult{.level = core::Level{trouvee->second},
                                         .error = {},
                                         .errorCode = core::LevelValidationError::None};
        };
    }

private:
    std::map<std::string, core::LevelData> _cartes;
};

// Avance la session de @p pas pas de 1/60 s dans la direction donnee.
void marcher(core::ExplorationSession& session, core::Vector2 direction, int pas) {
    for (int rang = 0; rang < pas; ++rang) {
        session.update(core::ExplorationIntent{.move = direction, .interact = false}, 1.0F / 60.0F);
    }
}

}  // namespace

/**
 * @brief Le heros marche, et le mur l'arrete sans le bloquer le long.
 * \castest{<b>Le heros marche sur la carte et bute sur le mur, en glissant le long.</b><br/>
 * \tcat Unitaire · Exploration<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Entrer sur une carte muree, marcher vers la droite jusqu'au mur.<br/>
 * 2. Marcher en diagonale contre ce mur.<br/>
 * \tattendu Le heros s'arrete devant le mur, et la diagonale continue de le faire descendre : un
 * mur pris en biais fait glisser le long, il n'arrete pas les deux axes.
 * }
 */
TEST(ExplorationSessionTest, LeHerosMarcheEtLeMurLArrete) {
    DossierEnMemoire dossier;
    dossier.poser("place", carteMuree("place", {}));
    core::ExplorationSession session{dossier.chargeur()};

    ASSERT_TRUE(session.start("place", ""));
    EXPECT_EQ(session.heroPoint(), (core::CellPoint{1.5F, 1.5F}));

    marcher(session, {1.0F, 0.0F}, 300);
    // Le mur est en colonne 9 : le gabarit s'arrete a 9 - 0,3.
    EXPECT_NEAR(session.heroPoint().column, 8.7F, 0.05F);
    EXPECT_NEAR(session.heroPoint().row, 1.5F, 0.001F);

    const float avant = session.heroPoint().row;
    marcher(session, {1.0F, 1.0F}, 30);
    EXPECT_GT(session.heroPoint().row, avant) << "la diagonale doit glisser le long du mur";
    EXPECT_NEAR(session.heroPoint().column, 8.7F, 0.05F);
}

/**
 * @brief Un portail se franchit en arrivant dessus, et depose au point d'arrivee nomme.
 * \castest{<b>Marcher sur un portail depose le heros au point d'arrivee nomme de la cible.</b><br/>
 * \tcat Unitaire · Exploration<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Marcher jusqu'a la case du portail.<br/>
 * 2. Rester dessus quelques pas.<br/>
 * \tattendu Un evenement `MapEntered`, le heros au centre du point d'arrivee de la carte cible, et
 * AUCUNE seconde traversee tant qu'on ne quitte pas la case.
 * }
 */
TEST(ExplorationSessionTest, UnPortailDeposeAuPointDArriveeNomme) {
    DossierEnMemoire dossier;
    dossier.poser("place", carteMuree("place", {portail("cave", "seuil", {4, 1})}));
    dossier.poser("cave", carteMuree("cave", {pointDArrivee("seuil", {6, 7})}));
    core::ExplorationSession session{dossier.chargeur()};
    ASSERT_TRUE(session.start("place", ""));

    // On marche vers le portail et l'on s'arrete au premier evenement : continuer a presser la
    // touche ferait marcher le heros sur la carte d'arrivee, ce que le test ne mesure pas.
    std::vector<core::ExplorationEvent> vus;
    for (int rang = 0; rang < 300 && vus.empty(); ++rang) {
        for (core::ExplorationEvent& evenement : session.update(
                 core::ExplorationIntent{.move = {1.0F, 0.0F}, .interact = false}, 1.0F / 60.0F)) {
            vus.push_back(std::move(evenement));
        }
    }

    ASSERT_EQ(vus.size(), 1U);
    EXPECT_EQ(vus.front().kind, ExplorationEventKind::MapEntered);
    EXPECT_EQ(vus.front().value, "cave");
    EXPECT_EQ(session.mapId(), "cave");
    EXPECT_EQ(session.heroPoint(), (core::CellPoint{6.5F, 7.5F}));

    // Rester sur la case d'arrivee ne rejoue rien : le portail se franchit en y ARRIVANT.
    for (int rang = 0; rang < 10; ++rang) {
        EXPECT_TRUE(
            session.update(core::ExplorationIntent{.move = {}, .interact = false}, 1.0F / 60.0F)
                .empty());
    }
}

/**
 * @brief On parle au PNJ que l'on regarde, et a lui seul.
 * \castest{<b>L'interaction ouvre le dialogue du PNJ vise, pas celui d'un autre.</b><br/>
 * \tcat Unitaire · Exploration<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser deux PNJ de part et d'autre du heros.<br/>
 * 2. Regarder l'un, interagir ; puis regarder l'autre, interagir.<br/>
 * \tattendu Deux evenements `Dialogue`, chacun nommant le dialogue du PNJ regarde.
 * }
 */
TEST(ExplorationSessionTest, OnParleAuPnjQueLOnRegarde) {
    DossierEnMemoire dossier;
    dossier.poser("place", carteMuree("place", {pnj("garde", {5, 4}), pnj("myr-marche", {3, 4})}));
    core::ExplorationSession session{dossier.chargeur()};
    ASSERT_TRUE(session.start("place", ""));
    session.placeHero(core::cellCenter({4, 4}));

    // Regarder a droite : le garde.
    session.update(core::ExplorationIntent{.move = {1.0F, 0.0F}, .interact = false}, 0.0001F);
    std::vector<core::ExplorationEvent> vus =
        session.update(core::ExplorationIntent{.move = {}, .interact = true}, 1.0F / 60.0F);
    ASSERT_EQ(vus.size(), 1U);
    EXPECT_EQ(vus.front().kind, ExplorationEventKind::Dialogue);
    EXPECT_EQ(vus.front().value, "garde");

    // Regarder a gauche : l'autre PNJ, et son dialogue a lui.
    session.update(core::ExplorationIntent{.move = {-1.0F, 0.0F}, .interact = false}, 0.0001F);
    vus = session.update(core::ExplorationIntent{.move = {}, .interact = true}, 1.0F / 60.0F);
    ASSERT_EQ(vus.size(), 1U);
    EXPECT_EQ(vus.front().value, "myr-marche");
}

/**
 * @brief Une carte gelee ne bouge plus et n'interagit plus.
 * \castest{<b>Gelee, la session ne deplace plus le heros et n'ouvre plus rien.</b><br/>
 * \tcat Unitaire · Exploration<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Geler la session, marcher et interagir.<br/>
 * 2. Degeler, refaire les memes gestes.<br/>
 * \tattendu Rien pendant le gel ; tout reprend apres, le heros a la place ou il etait.
 * }
 */
TEST(ExplorationSessionTest, UneCarteGeleeNeBougePlus) {
    DossierEnMemoire dossier;
    dossier.poser("place", carteMuree("place", {pnj("garde", {5, 4})}));
    core::ExplorationSession session{dossier.chargeur()};
    ASSERT_TRUE(session.start("place", ""));
    session.placeHero(core::cellCenter({4, 4}));

    session.freeze(true);
    marcher(session, {1.0F, 0.0F}, 30);
    EXPECT_EQ(session.heroPoint(), core::cellCenter({4, 4}));
    EXPECT_TRUE(session.update(core::ExplorationIntent{.move = {}, .interact = true}, 1.0F / 60.0F)
                    .empty());

    session.freeze(false);
    marcher(session, {1.0F, 0.0F}, 1);
    EXPECT_GT(session.heroPoint().column, 4.5F);
}

// --- Le groupe (LOT-138) ------------------------------------------------------------------------

namespace {

// Une carte muree coupee par un mur interieur en colonne 5, des lignes 1 a 7 : pour passer de la
// moitie gauche a la droite, il faut descendre jusqu'a la ligne 8 et remonter -- un U, avec deux
// angles qu'un suiveur marchant droit vers le meneur ne franchirait pas.
core::LevelData carteEnU() {
    core::LevelData donnees = carteMuree("u", {});
    for (int ligne = 1; ligne <= 7; ++ligne) {
        donnees.tileMap.setTile(5, ligne, core::TileType::Wall);
    }
    return donnees;
}

// Vrai si le gabarit d'un membre du groupe tient en @p point : ses quatre coins hors du plein.
bool tient(const core::ExplorationSession& session, core::CellPoint point) {
    const core::TileMap& grille = session.map()->tileMap();
    constexpr float DEMI = core::ExplorationSession::HERO_HALF_SIZE_CELLS;
    for (const float dx : {-DEMI, DEMI}) {
        for (const float dy : {-DEMI, DEMI}) {
            const core::GridPosition coin =
                core::cellOf({.column = point.column + dx, .row = point.row + dy});
            if (!grille.inBounds(coin.column, coin.row) || grille.isSolid(coin.column, coin.row)) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace

/**
 * @brief Trois suiveurs passent les deux angles d'un U sans jamais entrer dans le mur
 *        (EX-EXP-013).
 * \castest{<b>Un groupe de quatre traverse une carte en U : aucun suiveur ne reste coince, aucun
 * n'entre dans un mur.</b><br/>
 * \tcat Unitaire · Groupe<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Un meneur et trois suiveurs a l'entree d'une carte coupee par un mur interieur.<br/>
 * 2. Descendre, passer sous le mur, remonter de l'autre cote.<br/>
 * 3. A chaque pas, verifier le gabarit de chaque suiveur.<br/>
 * \tattendu Aucun suiveur dans le plein a aucun pas ; a l'arrivee, les trois ont passe le mur, en
 * file a une case l'un de l'autre derriere le meneur.
 * }
 */
TEST(ExplorationSessionTest, UnGroupeDeQuatrePasseLesAnglesSansResterCoince) {
    DossierEnMemoire dossier;
    dossier.poser("u", carteEnU());
    core::ExplorationSession session{dossier.chargeur()};
    ASSERT_TRUE(session.start("u", ""));
    session.setFollowers(3);
    ASSERT_EQ(session.followers(), 3U);

    const auto marcherEnVerifiant = [&session](core::Vector2 direction, int pas) {
        for (int rang = 0; rang < pas; ++rang) {
            session.update(core::ExplorationIntent{.move = direction, .interact = false},
                           1.0F / 60.0F);
            for (std::size_t suiveur = 0; suiveur < session.followers(); ++suiveur) {
                ASSERT_TRUE(tient(session, session.followerPoint(suiveur)))
                    << "suiveur " << suiveur << " dans le plein en ("
                    << session.followerPoint(suiveur).column << ", "
                    << session.followerPoint(suiveur).row << ")";
            }
        }
    };
    marcherEnVerifiant({0.0F, 1.0F}, 300);   // jusqu'a la ligne 8
    marcherEnVerifiant({1.0F, 0.0F}, 300);   // sous le mur, jusqu'a la colonne 8
    marcherEnVerifiant({0.0F, -1.0F}, 180);  // on remonte de l'autre cote

    EXPECT_GT(session.heroPoint().column, 6.0F);
    EXPECT_LT(session.heroPoint().row, 4.0F);
    for (std::size_t suiveur = 0; suiveur < 3; ++suiveur) {
        const core::CellPoint point = session.followerPoint(suiveur);
        EXPECT_GT(point.column, 6.0F) << "le suiveur " << suiveur << " est reste de l'autre cote";
        // En file sur la colonne du meneur, a une case l'un de l'autre.
        EXPECT_NEAR(point.column, session.heroPoint().column, 0.01F);
        EXPECT_NEAR(point.row - session.heroPoint().row, static_cast<float>(suiveur + 1), 0.05F);
    }
}

/**
 * @brief A l'entree d'une carte, les suiveurs se rangent derriere le meneur, dans la place libre
 *        (EX-EXP-013).
 * \castest{<b>Reposer le meneur range les suiveurs dans son dos, et un mur arrete la file.</b><br/>
 * \tcat Unitaire · Groupe<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser le meneur au milieu de la carte, tourne vers le sud ; trois suiveurs.<br/>
 * 2. Le poser en (1, 2), tourne vers le sud : le mur du nord est a une case et demie.<br/>
 * \tattendu Au milieu, les trois en file au nord, a une case l'un de l'autre ; contre le mur, tous
 * tiennent, et ceux qui n'ont plus de place attendent sur le dernier point libre.
 * }
 */
TEST(ExplorationSessionTest, LesSuiveursSeRangentDansLeDosDuMeneur) {
    DossierEnMemoire dossier;
    dossier.poser("place", carteMuree("place", {}));
    core::ExplorationSession session{dossier.chargeur()};
    ASSERT_TRUE(session.start("place", ""));
    session.setFollowers(3);

    session.placeHero(core::cellCenter({4, 6}));
    for (std::size_t suiveur = 0; suiveur < 3; ++suiveur) {
        EXPECT_NEAR(session.followerPoint(suiveur).column, 4.5F, 0.001F);
        EXPECT_NEAR(session.followerPoint(suiveur).row, 6.5F - static_cast<float>(suiveur + 1),
                    0.05F);
    }

    session.placeHero(core::cellCenter({1, 2}));
    for (std::size_t suiveur = 0; suiveur < 3; ++suiveur) {
        EXPECT_TRUE(tient(session, session.followerPoint(suiveur))) << suiveur;
    }
    EXPECT_NEAR(session.followerPoint(0).row, 1.5F, 0.05F);
    EXPECT_EQ(session.followerPoint(1), session.followerPoint(2))
        << "sans place, les derniers attendent sur le meme point";
}

/**
 * @brief La trace se mesure le long du chemin, et ne garde que ce qu'il faut.
 * \castest{<b>Un point a une distance donnee derriere le meneur se lit le long du chemin, angles
 * compris ; la trace oublie ce qu'aucun suiveur n'atteint.</b><br/>
 * \tcat Unitaire · Groupe<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Une trace en L : (0, 0), (2, 0), (2, 2), le meneur au bout.<br/>
 * 2. Lire les points a 1, 3 et 10 cases ; puis garder 1,5 case et prolonger la trace.<br/>
 * \tattendu (2, 1), (1, 0), puis le plus ancien point ; la direction a 3 cases va vers l'est ; la
 * trace gardee couvre au moins 1,5 case et pas beaucoup plus.
 * }
 */
TEST(FollowTrailTest, LaTraceSeMesureLeLongDuChemin) {
    core::FollowTrail trace;
    trace.reset({{2.0F, 2.0F}, {2.0F, 0.0F}, {0.0F, 0.0F}});
    EXPECT_EQ(trace.pointBehind(1.0F), (core::TrailPoint{2.0F, 1.0F}));
    EXPECT_EQ(trace.pointBehind(3.0F), (core::TrailPoint{1.0F, 0.0F}));
    EXPECT_EQ(trace.pointBehind(10.0F), (core::TrailPoint{0.0F, 0.0F}));
    EXPECT_GT(trace.directionAt(3.0F).x, 0.0F);
    EXPECT_FLOAT_EQ(trace.length(), 4.0F);

    trace.keep(1.5F);
    for (int pas = 1; pas <= 20; ++pas) {
        trace.record({2.0F, 2.0F + (0.1F * static_cast<float>(pas))});
    }
    EXPECT_GE(trace.length(), 1.5F);
    EXPECT_LT(trace.length(), 1.7F);
    // Un pietinement ne pose pas de point.
    const std::size_t avant = trace.points().size();
    trace.record(trace.points().front());
    EXPECT_EQ(trace.points().size(), avant);
}
