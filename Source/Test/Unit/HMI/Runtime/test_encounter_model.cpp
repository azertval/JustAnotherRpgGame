// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_encounter_model.cpp
 * @brief Tests du combat sur la carte (`LOT-118`) : une rencontre engagée pendant l'exploration se
 *        monte sur la zone de combat de la carte, se joue par les gestes du Colisée, se montre par
 *        les figurines de la carte gelée, et rend l'exploration.
 *
 * La vue-modèle lit ses règles à côté de l'exécutable, comme le jeu ; la **carte** et le
 * **contenu** — la rencontre, le rat d'essai, le maître d'arène d'essai — viennent de la racine
 * d'essai (`Source/Test/Fixtures/GameData`).
 */

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <filesystem>
#include <set>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include "Core/Rpg/PartyLedger.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Runtime/EncounterModel.h"
#include "HMI/Runtime/PartyModel.h"
#include "HMI/Runtime/WorldModel.h"

namespace {

[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path{JADG_TEST_DATA_DIR};
}

/// Ouvre le donjon d'essai, le héros une case au nord du maître d'arène d'essai (24, 20), dans la
/// zone « salle » : la case qu'il regarde est celle du maître.
void ouvrirLeDonjon(hmi::WorldModel& monde, core::GridPosition heros = {.column = 24, .row = 19}) {
    monde.setLevelDirectories({dataRoot() / "Levels"});
    monde.setStartOverride(QStringLiteral("donjon"), QStringLiteral("sable"));
    monde.setStartCell(heros);
    ASSERT_TRUE(monde.startNewGame()) << monde.status().toStdString();
}

/// Avance la file et les tours de l'IA jusqu'au tour du joueur, ou la fin ; relève les bandes vues.
void jusquAuJoueur(hmi::EncounterModel& rencontre, const hmi::WorldModel& monde,
                   std::set<std::string>& bandes) {
    for (int pas = 0; pas < 2000; ++pas) {
        rencontre.tick(1.0F / 60.0F);
        for (const hmi::WorldFigureSnapshot& figure : monde.figures()) {
            bandes.insert(figure.clip);
        }
        if (rencontre.ended() ? !rencontre.busy()
                              : (!rencontre.busy() && !rencontre.turnActions().isEmpty())) {
            return;
        }
    }
    FAIL() << "ni tour du joueur ni fin en 2000 pas";
}

/// Fuit : chaque membre du groupe se retire a son tour (`LOT-139`, la fuite est celle de tous),
/// jusqu'a l'issue.
void fuir(hmi::EncounterModel& rencontre, const hmi::WorldModel& monde,
          std::set<std::string>& bandes) {
    for (int tours = 0; tours < 8 && !rencontre.ended(); ++tours) {
        jusquAuJoueur(rencontre, monde, bandes);
        if (!rencontre.ended()) {
            rencontre.withdraw();
        }
    }
    jusquAuJoueur(rencontre, monde, bandes);
}

}  // namespace

/**
 * @brief Une rencontre engagée depuis la carte se monte sur sa zone de combat, se joue, se voit et
 *        rend l'exploration. Tant qu'elle dure, c'est elle qui tient la carte gelée, d'où qu'elle
 *        ait été engagée (`EX-IHM-091`).
 * \castest{<b>Du declenchement sur la carte au retour a l'exploration, sans fenetre.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Ouvrir le donjon d'essai, le heros devant le maitre d'arene, dans la zone
 * « salle ».<br/>2. Engager « rats-du-donjon » a la graine 2026.<br/>3. Jouer : attaquer le rat
 * le plus proche, finir le tour, jusqu'a l'issue ou dix rounds.<br/>4. Quitter.<br/>
 * \tattendu Le combat est monte : carte gelee, quatre combattants, zone d'origine (10, 10), le
 * heros en (14, 9) de la grille ; les figurines publiees sur la carte ont montre une attaque et
 * une mort (un rat a 1 PV tombe sans passer par le touche) ; a la sortie, la carte degele, ses
 * figurines reprennent, le heros est la ou le combat l'a laisse, et `finished` porte l'issue.
 * }
 */
TEST(EncounterModelTest, DuDeclenchementAuRetourALExploration) {
    hmi::WorldModel monde;
    ouvrirLeDonjon(monde);
    const core::GridPosition depart = monde.play().session().heroCell();

    hmi::EncounterModel rencontre;
    rencontre.setContentRoot(dataRoot());
    rencontre.setSeed(2026);
    ASSERT_TRUE(rencontre.begin(QStringLiteral("rats-du-donjon")))
        << rencontre.status().toStdString();
    EXPECT_TRUE(rencontre.active());
    EXPECT_TRUE(monde.frozen());
    EXPECT_TRUE(monde.showsCombat());
    EXPECT_EQ(rencontre.zoneColumn(), 10);
    EXPECT_EQ(rencontre.zoneRow(), 10);
    ASSERT_NE(rencontre.setup(), nullptr);
    EXPECT_EQ(rencontre.setup()->heroCell, (core::GridPosition{.column = 14, .row = 9}));
    // Les quatre du groupe (LOT-139) et les trois rats.
    EXPECT_EQ(rencontre.setup()->partyCells.size(), 4U);
    EXPECT_EQ(rencontre.fighters().size(), 7);
    EXPECT_EQ(rencontre.partyMembers().size(), 4);
    EXPECT_EQ(rencontre.partyMembers().front().toMap().value("label").toString(),
              QStringLiteral("Grom Tranche-Écaille"));
    EXPECT_EQ(rencontre.encounterName(), QStringLiteral("Les rats du donjon"));
    // Les combattants tiennent lieu de figurines, sur la carte : leurs points sont en cases de la
    // carte, et le heros y est.
    bool heros = false;
    for (const hmi::WorldFigureSnapshot& figure : monde.figures()) {
        EXPECT_TRUE(figure.combatant);
        EXPECT_GE(figure.point.x, 10.0F);
        EXPECT_GE(figure.point.y, 10.0F);
        heros = heros || figure.hero;
    }
    EXPECT_TRUE(heros);

    std::set<std::string> bandes;
    QStringList fini;
    QObject::connect(&rencontre, &hmi::EncounterModel::finished,
                     [&fini](const QString& issue) { fini << issue; });
    for (int round = 0; round < 10 && !rencontre.ended(); ++round) {
        jusquAuJoueur(rencontre, monde, bandes);
        if (rencontre.ended()) {
            break;
        }
        rencontre.selectAction(0);
        rencontre.cycleTarget(1);
        rencontre.confirm();
        rencontre.endTurn();
    }
    jusquAuJoueur(rencontre, monde, bandes);
    EXPECT_TRUE(bandes.contains(std::string{hmi::figure_clips::ATTACK})) << "une attaque s'est vue";
    EXPECT_TRUE(bandes.contains(std::string{hmi::figure_clips::DEATH})) << "une mort s'est vue";

    if (!rencontre.ended()) {
        fuir(rencontre, monde, bandes);  // la rencontre est fuyable
    }
    ASSERT_TRUE(rencontre.ended());
    EXPECT_FALSE(rencontre.outcome().isEmpty());
    const QString issue = rencontre.outcome();

    rencontre.leave();
    EXPECT_FALSE(rencontre.active());
    EXPECT_FALSE(monde.frozen());
    EXPECT_FALSE(monde.showsCombat());
    ASSERT_EQ(fini.size(), 1);
    EXPECT_EQ(fini.front(), issue);
    // Le heros est reste sur la carte, dans la zone : la ou le combat l'a laisse.
    const core::GridPosition arrivee = monde.play().session().heroCell();
    EXPECT_TRUE(rencontre.setup() == nullptr);
    EXPECT_GE(arrivee.column, 10);
    EXPECT_GE(arrivee.row, 10);
    static_cast<void>(depart);
    // Les figurines de l'exploration ont repris : le maitre d'arene, sans figurine, est un
    // mannequin.
    bool mannequin = false;
    for (const hmi::WorldFigureSnapshot& figure : monde.figures()) {
        EXPECT_FALSE(figure.combatant);
        mannequin = mannequin || figure.figure == hmi::placeholderFigureDirectory("humanoid");
    }
    EXPECT_TRUE(mannequin);
}

/**
 * @brief Une rencontre inconnue, ou déclenchée hors de toute zone de combat, est refusée et
 *        l'exploration continue.
 * \castest{<b>Un refus de montage laisse l'exploration intacte.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ouvrir le donjon a la porte (19, 32), hors de la zone.<br/>2. Engager une rencontre
 * inconnue, puis « rats-du-donjon ».<br/>
 * \tattendu Les deux refus : pas de combat, carte non gelee, statut qui dit pourquoi.
 * }
 */
TEST(EncounterModelTest, UnRefusLaisseLExplorationIntacte) {
    hmi::WorldModel monde;
    ouvrirLeDonjon(monde, {.column = 19, .row = 31});
    hmi::EncounterModel rencontre;
    rencontre.setContentRoot(dataRoot());

    EXPECT_FALSE(rencontre.begin(QStringLiteral("dragons")));
    EXPECT_FALSE(rencontre.active());
    EXPECT_FALSE(rencontre.status().isEmpty());

    EXPECT_FALSE(rencontre.begin(QStringLiteral("rats-du-donjon")));
    EXPECT_FALSE(rencontre.active());
    EXPECT_FALSE(monde.frozen());
    EXPECT_FALSE(monde.showsCombat());
    EXPECT_NE(rencontre.status().indexOf(QStringLiteral("zone")), -1)
        << rencontre.status().toStdString();
}

/**
 * @brief Pendant qu'un mouvement se joue, les gestes attendent ; sauter l'animation les rouvre.
 * \castest{<b>Les gestes attendent la fin d'un mouvement.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Engager les rats, avancer d'un pas : l'IA a pu jouer, la file est occupee.<br/>
 * 2. Tant que la file joue, finir le tour ; puis sauter l'animation.<br/>
 * \tattendu Occupe, `endTurn` ne change pas le journal ; apres le saut, la file est vide.
 * }
 */
TEST(EncounterModelTest, LesGestesAttendentLaFinDUnMouvement) {
    hmi::WorldModel monde;
    ouvrirLeDonjon(monde);
    hmi::EncounterModel rencontre;
    rencontre.setContentRoot(dataRoot());
    rencontre.setSeed(7);
    ASSERT_TRUE(rencontre.begin(QStringLiteral("rats-du-donjon")))
        << rencontre.status().toStdString();
    // Jusqu'a ce qu'un mouvement joue : un tour de l'IA, ou un pas du joueur.
    for (int pas = 0; pas < 600 && !rencontre.busy(); ++pas) {
        rencontre.tick(1.0F / 60.0F);
        if (!rencontre.turnActions().isEmpty()) {
            rencontre.endTurn();
        }
    }
    ASSERT_TRUE(rencontre.busy()) << rencontre.journal().join(QStringLiteral("\n")).toStdString();
    const int lignes = rencontre.journal().size();
    rencontre.endTurn();
    rencontre.dodge();
    EXPECT_EQ(rencontre.journal().size(), lignes) << "un geste pendant l'animation n'a rien fait";
    rencontre.skipAnimations();
    EXPECT_FALSE(rencontre.busy());
    rencontre.leave();  // pas d'issue : refuse, le combat continue
    EXPECT_TRUE(rencontre.active());
    // La fuite est un geste du joueur : a son tour, membre par membre.
    std::set<std::string> bandes;
    fuir(rencontre, monde, bandes);
    ASSERT_TRUE(rencontre.ended()) << rencontre.journal().join(QStringLiteral("\n")).toStdString();
    rencontre.leave();
    EXPECT_FALSE(rencontre.active());
}

/**
 * @brief Le rejeu à graine fixée donne le même combat (`LOT-139`) : deux rencontres montées à la
 *        même graine, jouées par les mêmes gestes, écrivent le même journal — et chaque membre du
 *        groupe y est joué par le joueur à son tour (`EX-CBT-061`).
 * \castest{<b>Deux combats de groupe a la meme graine sont identiques.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Engager les rats a la graine 41, jouer trois rounds (attaquer, finir le tour),
 * relever le journal, fuir.<br/>2. Reposer le groupe, recommencer a la meme graine.<br/>
 * \tattendu Les deux journaux sont identiques, et ont vu plus d'un membre du groupe jouer.
 * }
 */
TEST(EncounterModelTest, LeRejeuAGraineFixeeDonneLeMemeCombat) {
    hmi::WorldModel monde;
    ouvrirLeDonjon(monde);
    hmi::EncounterModel rencontre;
    rencontre.setContentRoot(dataRoot());
    std::set<std::string> bandes;
    const auto jouer = [&]() {
        monde.placeHero(core::cellCenter({.column = 24, .row = 19}));
        rencontre.setSeed(41);
        EXPECT_TRUE(rencontre.begin(QStringLiteral("rats-du-donjon")))
            << rencontre.status().toStdString();
        std::set<int> membresJoues;
        for (int tour = 0; tour < 6 && !rencontre.ended(); ++tour) {
            jusquAuJoueur(rencontre, monde, bandes);
            if (rencontre.ended()) {
                break;
            }
            membresJoues.insert(rencontre.activeMember());
            rencontre.selectAction(0);
            rencontre.cycleTarget(1);
            rencontre.confirm();
            rencontre.endTurn();
        }
        const QStringList journal = rencontre.journal();
        if (!rencontre.ended()) {
            fuir(rencontre, monde, bandes);
        }
        rencontre.leave();
        return std::make_pair(journal, membresJoues);
    };
    const auto [premier, membres] = jouer();
    const auto [second, encore] = jouer();
    EXPECT_EQ(premier, second);
    EXPECT_GT(membres.size(), 1U) << "plus d'un membre du groupe a joue";
    EXPECT_FALSE(membres.contains(-1)) << "au tour du joueur, c'est un membre qui joue";
}

/**
 * @brief Ce que le combat laisse aux fiches (`LOT-139`) : les points de vie qui restent sont
 *        relus au combat suivant et montrés par l'écran de groupe ; un membre mort quitte le
 *        groupe, et s'il menait, le suivant mène.
 * \castest{<b>Le registre du groupe : points de vie relus, mort qui ne suit plus.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Noter au registre 5 PV pour le Brawler ; engager les rats.<br/>2. Fuir ; lire le
 * registre.<br/>3. Enterrer le Brawler, puis tenter d'enterrer tout le monde.<br/>
 * \tattendu Le Brawler entre en combat a 5 / 15, l'ecran de groupe le dit ; apres la fuite,
 * chaque membre a un enregistrement, au moins 1 PV ; enterre, le Brawler quitte le groupe, le
 * Priest mene, deux suiveurs ; le dernier ne s'enterre pas.
 * }
 */
TEST(EncounterModelTest, LeCombatLaisseAuxFichesCeQuIlEnReste) {
    hmi::WorldModel monde;
    ouvrirLeDonjon(monde);
    core::MemberRecord blesse;
    blesse.hitPoints = 5;
    monde.recordMember("heros-brawler", blesse);
    hmi::PartyModel groupe;
    EXPECT_EQ(groupe.leaderHitPoints(), QStringLiteral("5 / 15"));

    hmi::EncounterModel rencontre;
    rencontre.setContentRoot(dataRoot());
    rencontre.setSeed(2026);
    ASSERT_TRUE(rencontre.begin(QStringLiteral("rats-du-donjon")))
        << rencontre.status().toStdString();
    EXPECT_EQ(rencontre.heroHitPoints(), QStringLiteral("5 / 15"));
    EXPECT_EQ(rencontre.partyMembers().front().toMap().value("value").toString(),
              QStringLiteral("5 / 15"));
    std::set<std::string> bandes;
    fuir(rencontre, monde, bandes);
    ASSERT_TRUE(rencontre.ended());
    rencontre.leave();
    for (const std::string& membre : monde.party().members()) {
        const core::MemberRecord* const record = monde.ledger().record(membre);
        ASSERT_NE(record, nullptr) << membre;
        ASSERT_TRUE(record->hitPoints.has_value()) << membre;
        EXPECT_GE(*record->hitPoints, 1) << membre;
    }

    EXPECT_TRUE(monde.buryMember("heros-brawler"));
    EXPECT_EQ(monde.leaderId(), QStringLiteral("heros-priest"));
    EXPECT_EQ(monde.party().size(), 3U);
    EXPECT_EQ(monde.play().session().followers(), 2U);
    EXPECT_EQ(monde.ledger().record("heros-brawler"), nullptr);
    EXPECT_TRUE(monde.buryMember("heros-priest"));
    EXPECT_TRUE(monde.buryMember("heros-scoundrel"));
    EXPECT_FALSE(monde.buryMember("heros-mage")) << "le dernier ne s'enterre pas";
    EXPECT_EQ(monde.party().size(), 1U);
}

/**
 * @brief Le niveau donné survit au combat, et le repos rend la fiche pleine à ce niveau
 *        (`LOT-142`) : la série de l'arène donne un niveau et un repos entre deux combats.
 * \castest{<b>Un combat garde le niveau donne ; le repos rend les points de vie.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Donner un niveau au groupe ; engager les rats, fuir.<br/>2. Lire le registre.<br/>
 * 3. Donner un repos au groupe ; engager les rats.<br/>
 * \tattendu Apres la fuite, chaque membre garde le niveau 2 au registre ; apres le repos, il n'a
 * plus de points de vie retenus, et le Brawler entre en combat plein, au maximum du niveau 2.
 * }
 */
TEST(EncounterModelTest, LeNiveauDonneSurvitAuCombatEtLeReposSoigne) {
    hmi::WorldModel monde;
    ouvrirLeDonjon(monde);
    ASSERT_TRUE(monde.levelUp(QStringLiteral("party")));
    core::MemberRecord blesse = *monde.ledger().record("heros-brawler");
    blesse.hitPoints = 4;
    monde.recordMember("heros-brawler", blesse);

    hmi::EncounterModel rencontre;
    rencontre.setContentRoot(dataRoot());
    rencontre.setSeed(2026);
    ASSERT_TRUE(rencontre.begin(QStringLiteral("rats-du-donjon")))
        << rencontre.status().toStdString();
    std::set<std::string> bandes;
    fuir(rencontre, monde, bandes);
    ASSERT_TRUE(rencontre.ended());
    rencontre.leave();
    for (const std::string& membre : monde.party().members()) {
        const core::MemberRecord* const record = monde.ledger().record(membre);
        ASSERT_NE(record, nullptr) << membre;
        EXPECT_EQ(record->level, 2) << membre << " : le combat a garde le niveau donne";
    }

    EXPECT_TRUE(monde.rest(QStringLiteral("party")));
    EXPECT_FALSE(monde.rest(QStringLiteral("party"))) << "un groupe repose n'a rien a reposer";
    for (const std::string& membre : monde.party().members()) {
        const core::MemberRecord* const record = monde.ledger().record(membre);
        ASSERT_NE(record, nullptr) << membre;
        EXPECT_EQ(record->level, 2) << membre;
        EXPECT_FALSE(record->hitPoints.has_value()) << membre;
    }
    monde.placeHero(core::cellCenter({.column = 24, .row = 19}));
    ASSERT_TRUE(rencontre.begin(QStringLiteral("rats-du-donjon")))
        << rencontre.status().toStdString();
    const QStringList pv = rencontre.heroHitPoints().split(QStringLiteral(" / "));
    ASSERT_EQ(pv.size(), 2);
    EXPECT_EQ(pv.front(), pv.back()) << "plein apres le repos";
    EXPECT_GT(pv.back().toInt(), 15) << "au maximum du niveau 2";
    fuir(rencontre, monde, bandes);
    rencontre.leave();
}

/**
 * @brief L'interface de combat de groupe (`LOT-140`, `EX-IHM-108`) lit ce que la vue-modèle
 *        publie : le round, les jetons de l'ordre d'initiative, le panneau du combattant actif, le
 *        détail des actions et la prévisualisation de l'action choisie sur la case du curseur.
 * \castest{<b>Round, jetons, panneau de l'actif, actions detaillees, previsualisation.</b><br/>
 * \tcat Unitaire · Combat sur la carte<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Engager les rats a la graine 2026, jusqu'au tour du joueur.<br/>2. Lire `round`,
 * `turnOrder`, `activeProfile`, `turnActions`.<br/>3. Viser le rat le plus proche avec la premiere
 * attaque ; puis ramener le curseur sur soi ; puis viser une case libre atteignable.<br/>
 * \tattendu Round 1 ; chaque ligne de l'ordre a ses initiales, les membres du groupe leur jeton
 * s'il est installe ; le panneau porte la classe, le niveau 1, une action et un deplacement a
 * depenser, les capacites de la fiche ; la premiere action ecrit son jet et ses des ; la
 * previsualisation est une attaque titree « arme → cible » avec la ligne « Toucher », puis
 * refuse la case d'un allie, puis decrit un deplacement.
 * }
 */
TEST(EncounterModelTest, LInterfaceDeGroupeLitLaVueModele) {
    hmi::WorldModel monde;
    ouvrirLeDonjon(monde);
    hmi::EncounterModel rencontre;
    rencontre.setContentRoot(dataRoot());
    rencontre.setSeed(2026);
    ASSERT_TRUE(rencontre.begin(QStringLiteral("rats-du-donjon")))
        << rencontre.status().toStdString();
    std::set<std::string> bandes;
    jusquAuJoueur(rencontre, monde, bandes);
    ASSERT_FALSE(rencontre.ended());
    EXPECT_EQ(rencontre.round(), 1);

    // L'ordre d'initiative : des initiales pour tous, un jeton pour les membres du groupe.
    const QVariantList ordre = rencontre.turnOrder();
    ASSERT_EQ(ordre.size(), 7);
    for (const QVariant& ligne : ordre) {
        const QVariantMap rang = ligne.toMap();
        EXPECT_FALSE(rang.value("initials").toString().isEmpty());
        const QUrl jeton = rang.value("token").toUrl();
        if (rang.value("side").toString() == QStringLiteral("allies")) {
            const std::filesystem::path installe = hmi::dataDirectory() / "Assets" / "Common" /
                                                   "Characters" / "Heroes" / "brawler" /
                                                   "token.png";
            if (std::filesystem::exists(installe)) {
                EXPECT_TRUE(jeton.toString().endsWith(QStringLiteral("token.png")));
            }
        } else {
            EXPECT_TRUE(jeton.isEmpty());
        }
    }

    // Le panneau du combattant actif : un membre du groupe, sa classe, son niveau, son tour.
    const QVariantMap actif = rencontre.activeProfile();
    EXPECT_EQ(actif.value("name").toString(), rencontre.activeName());
    EXPECT_EQ(actif.value("side").toString(), QStringLiteral("allies"));
    const std::set<QString> classes{"brawler", "mage", "priest", "scoundrel"};
    EXPECT_TRUE(classes.contains(actif.value("classId").toString()))
        << actif.value("classId").toString().toStdString();
    EXPECT_EQ(actif.value("level").toInt(), 1);
    EXPECT_EQ(actif.value("action").toInt(), 1);
    EXPECT_EQ(actif.value("actionMax").toInt(), 1);
    EXPECT_GT(actif.value("movementMax").toInt(), 0);
    EXPECT_GT(actif.value("armorClass").toInt(), 9);
    EXPECT_FALSE(actif.value("capacities").toList().isEmpty()) << "chaque classe a une capacite";
    for (const QVariant& capacite : actif.value("capacities").toList()) {
        EXPECT_TRUE(capacite.toMap().value("iconKey").toString().startsWith(
            QStringLiteral("ui/icon/capacity/")));
    }

    // Les actions ecrivent leur detail : la premiere est une attaque, son jet signe.
    const QVariantList actions = rencontre.turnActions();
    ASSERT_FALSE(actions.isEmpty());
    EXPECT_EQ(actions.front().toMap().value("kind").toString(), QStringLiteral("attack"));
    EXPECT_TRUE(actions.front().toMap().value("detail").toString().startsWith('+'))
        << actions.front().toMap().value("detail").toString().toStdString();
    EXPECT_EQ(actions.front().toMap().value("uses").toInt(), -1);
    EXPECT_TRUE(actions.front().toMap().value("iconKey").toString().startsWith(
        QStringLiteral("ui/icon/action/")));

    // La previsualisation : l'attaque sur le rat le plus proche, puis la case d'un allie, puis un
    // deplacement.
    rencontre.selectAction(0);
    rencontre.cycleTarget(1);
    QVariantMap apercu = rencontre.preview();
    EXPECT_EQ(apercu.value("kind").toString(), QStringLiteral("attack"));
    EXPECT_TRUE(apercu.value("title").toString().contains(QStringLiteral("›")))
        << apercu.value("title").toString().toStdString();
    const QVariantList lignes = apercu.value("lines").toList();
    ASSERT_FALSE(lignes.isEmpty());
    if (apercu.value("valid").toBool()) {
        EXPECT_EQ(lignes.front().toMap().value("label").toString(), QStringLiteral("Toucher"));
        EXPECT_FALSE(apercu.value("expected").toString().isEmpty());
    } else {
        EXPECT_EQ(lignes.front().toMap().value("label").toString(), QStringLiteral("Cible"));
    }

    rencontre.centerCursor();
    apercu = rencontre.preview();
    EXPECT_EQ(apercu.value("kind").toString(), QStringLiteral("attack"));
    EXPECT_FALSE(apercu.value("valid").toBool()) << "sa propre case : rien a frapper";

    const QVariantList atteignables = rencontre.reachableCells();
    ASSERT_FALSE(atteignables.isEmpty());
    const QVariantMap libre = atteignables.front().toMap();
    rencontre.pointCursor(libre.value("column").toInt(), libre.value("row").toInt());
    apercu = rencontre.preview();
    EXPECT_EQ(apercu.value("kind").toString(), QStringLiteral("move"));
    EXPECT_TRUE(apercu.value("valid").toBool());
    EXPECT_EQ(apercu.value("lines").toList().front().toMap().value("label").toString(),
              QStringLiteral("Chemin"));

    fuir(rencontre, monde, bandes);
    rencontre.leave();
}
