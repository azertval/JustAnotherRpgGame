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

#include <gtest/gtest.h>

#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/Runtime/EncounterModel.h"
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

}  // namespace

/**
 * @brief Une rencontre engagée depuis la carte se monte sur sa zone de combat, se joue, se voit et
 *        rend l'exploration.
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
    EXPECT_EQ(rencontre.fighters().size(), 4);
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
        rencontre.withdraw();  // la rencontre est fuyable
        jusquAuJoueur(rencontre, monde, bandes);
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
    // La fuite est un geste du joueur : a son tour.
    std::set<std::string> bandes;
    jusquAuJoueur(rencontre, monde, bandes);
    if (!rencontre.ended()) {
        rencontre.withdraw();
        jusquAuJoueur(rencontre, monde, bandes);
    }
    ASSERT_TRUE(rencontre.ended()) << rencontre.journal().join(QStringLiteral("\n")).toStdString();
    rencontre.leave();
    EXPECT_FALSE(rencontre.active());
}
