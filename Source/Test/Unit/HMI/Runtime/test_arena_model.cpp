// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_arena_model.cpp
 * @brief Tests de la vue-modèle du Colisée (`LOT-24`) : un combat joué par les seuls gestes du
 *        clavier et de la manette, ce que la grille montre et cache, la réaction du joueur.
 *
 * La vue-modèle charge ses catalogues de règles à côté de l'exécutable, comme le jeu : ces tests
 * tournent dans le dossier `bin` où le jeu déploie ses données. Son **contenu** — l'arène et sa
 * carte — vient de la racine d'essai, le jeu n'en ayant plus depuis le `LOT-102`.
 */

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <tuple>
#include <utility>

#include <gtest/gtest.h>

#include "HMI/Runtime/ArenaModel.h"

namespace {

/// Compose le personnage de démonstration contre un sanglier, ennemis joués par l'IA, et lance.
void lancer(hmi::ArenaModel& arena) {
    const QVariantList roster = arena.roster();
    ASSERT_FALSE(roster.isEmpty());
    const QString personnage = roster.front().toMap().value("id").toString();
    ASSERT_TRUE(personnage.startsWith("character:"));
    arena.addAlly(personnage);
    arena.addEnemy(QStringLiteral("boar"));
    arena.setSeed(2026);
    ASSERT_TRUE(arena.enemyAi());
    arena.launch();
    ASSERT_TRUE(arena.inCombat()) << arena.status().toStdString();
}

/// Le curseur pose sur l'ennemi le plus proche : sa case.
[[nodiscard]] std::pair<int, int> viserLePlusProche(hmi::ArenaModel& arena) {
    arena.selectAction(0);
    arena.cycleTarget(1);
    return {arena.cursorColumn(), arena.cursorRow()};
}

}  // namespace

/**
 * @brief La reaction du joueur se bascule depuis la barre d'actions.
 * \castest{<b>La derniere action du tour est la reaction : la confirmer fait laisser passer les
 * attaques d'opportunite, la confirmer encore les fait saisir ; les actions du Manuel se
 * choisissent au numero et en boucle.</b><br/>
 * \tcat Unitaire · IHM<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lancer ; lire les actions.<br/>2. Choisir la derniere, confirmer, relire ; confirmer
 * encore.<br/>3. Passer a l'action suivante depuis la derniere.<br/>
 * \tattendu Esquiver, se desengager, se precipiter et la reaction apres les attaques ; le libelle
 * de la reaction change a chaque confirmation ; l'action suivante de la derniere est la premiere.
 * }
 */
TEST(ArenaModelTest, LaReactionSeBasculeDepuisLaBarre) {
    hmi::ArenaModel arena{nullptr, JADG_TEST_DATA_DIR};
    lancer(arena);
    QVariantList actions = arena.turnActions();
    ASSERT_GE(actions.size(), 5);
    const int derniere = static_cast<int>(actions.size()) - 1;
    EXPECT_EQ(actions[derniere - 3].toMap().value("kind").toString(), "dodge");
    EXPECT_EQ(actions[derniere - 2].toMap().value("kind").toString(), "disengage");
    EXPECT_EQ(actions[derniere - 1].toMap().value("kind").toString(), "dash");
    EXPECT_EQ(actions[derniere].toMap().value("kind").toString(), "reaction");
    const QString avant = actions[derniere].toMap().value("label").toString();

    arena.selectAction(derniere);
    EXPECT_TRUE(arena.turnActions()[derniere].toMap().value("selected").toBool());
    EXPECT_FALSE(arena.preview().isEmpty());
    arena.confirm();
    const QString apres = arena.turnActions()[derniere].toMap().value("label").toString();
    EXPECT_NE(apres, avant);
    arena.confirm();
    EXPECT_EQ(arena.turnActions()[derniere].toMap().value("label").toString(), avant);

    arena.cycleAction(1);
    EXPECT_TRUE(arena.turnActions()[0].toMap().value("selected").toBool());
}

/**
 * @brief Le survol de la souris pose le curseur sur une case.
 * \castest{<b>pointCursor pose le curseur sur la case survolee, et ignore ce qui n'en est pas
 * une.</b><br/>
 * \tcat Unitaire · IHM<br/>
 * \tcrit Majeure<br/>
 * \tetapes 1. Avant le combat, pointer une case.<br/>2. Lancer ; pointer l'ennemi le plus proche
 * depuis une autre case.<br/>3. Pointer hors de la grille.<br/>
 * \tattendu Rien avant le combat ; le curseur rejoint la case pointee ; hors de la
 * grille, le curseur ne bouge pas.
 * }
 */
TEST(ArenaModelTest, LeSurvolPoseLeCurseur) {
    hmi::ArenaModel arena{nullptr, JADG_TEST_DATA_DIR};
    const int colonneInitiale = arena.cursorColumn();
    const int ligneInitiale = arena.cursorRow();
    arena.pointCursor(colonneInitiale + 1, ligneInitiale + 1);
    EXPECT_EQ(arena.cursorColumn(), colonneInitiale);
    EXPECT_EQ(arena.cursorRow(), ligneInitiale);

    lancer(arena);
    const auto [colonne, ligne] = viserLePlusProche(arena);
    arena.centerCursor();
    ASSERT_FALSE(arena.cursorColumn() == colonne && arena.cursorRow() == ligne);

    arena.pointCursor(colonne, ligne);
    EXPECT_EQ(arena.cursorColumn(), colonne);
    EXPECT_EQ(arena.cursorRow(), ligne);

    arena.pointCursor(-1, ligne);
    arena.pointCursor(colonne, arena.gridRows());
    EXPECT_EQ(arena.cursorColumn(), colonne);
    EXPECT_EQ(arena.cursorRow(), ligne);
}

/**
 * @brief Le calque de la grille : une entree par combattant et par case atteignable.
 * \castest{<b>fighters decrit chaque combattant sur la grille et garde secrets les points de vie
 * ennemis ; reachableCells ne liste que des cases libres.</b><br/>
 * \tcat Unitaire · IHM<br/>
 * \tcrit Majeure<br/>
 * \tetapes 1. Avant le combat, lire les deux listes.<br/>2. Lancer ; lire les combattants et les
 * cases atteignables.<br/>
 * \tattendu Vides avant le combat ; un allie et un ennemi, un seul au tour ; les points de vie de
 * l'allie en clair, jamais ceux de l'ennemi ; des cases atteignables, aucune occupee.
 * }
 */
TEST(ArenaModelTest, LeCalqueDeLaGrilleDecritCombattantsEtCasesAtteignables) {
    hmi::ArenaModel arena{nullptr, JADG_TEST_DATA_DIR};
    EXPECT_TRUE(arena.fighters().isEmpty());
    EXPECT_TRUE(arena.reachableCells().isEmpty());

    lancer(arena);
    const QVariantList fighters = arena.fighters();
    ASSERT_EQ(fighters.size(), 2);
    int actifs = 0;
    for (const QVariant& entry : fighters) {
        const QVariantMap fighter = entry.toMap();
        EXPECT_GE(fighter.value("footprint").toInt(), 1);
        actifs += fighter.value("active").toBool() ? 1 : 0;
        const QString hitPoints = fighter.value("hitPoints").toString();
        if (fighter.value("side").toString() == "allies") {
            EXPECT_TRUE(hitPoints.contains('/')) << hitPoints.toStdString();
        } else {
            EXPECT_EQ(fighter.value("side").toString(), "enemies");
            EXPECT_FALSE(hitPoints.contains('/')) << hitPoints.toStdString();
        }
    }
    EXPECT_EQ(actifs, 1);

    const QVariantList reachable = arena.reachableCells();
    EXPECT_FALSE(reachable.isEmpty());
    for (const QVariant& entry : reachable) {
        const QVariantMap cell = entry.toMap();
        for (const QVariant& other : fighters) {
            const QVariantMap fighter = other.toMap();
            EXPECT_FALSE(cell.value("column") == fighter.value("column") &&
                         cell.value("row") == fighter.value("row"));
        }
    }
}

/**
 * @brief Un ennemi joué par l'IA se rapproche du joueur, tour après tour, sur la vraie zone de
 *        combat d'une carte — pas seulement dans une salle synthétique (audit du `LOT-118`).
 * \castest{<b>L'ennemi de l'IA marche vers le joueur sur la carte de l'arene.</b><br/>
 * \tcat Unitaire · IHM · IA<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lancer le personnage contre un sanglier, IA en marche, sur la zone « salle » du
 * donjon d'essai (les deux camps entrent a dix-sept cases l'un de l'autre).<br/>2. Finir le tour
 * du joueur sans bouger, trois fois.<br/>
 * \tattendu A chaque tour de l'IA, la distance du sanglier au personnage diminue, ou le sanglier
 * l'attaque ; le journal porte ses pas, et jamais un deplacement refuse.
 * }
 */
TEST(ArenaModelTest, LEnnemiDeLIaMarcheVersLeJoueur) {
    hmi::ArenaModel arena{nullptr, JADG_TEST_DATA_DIR};
    lancer(arena);
    const auto distance = [&arena]() {
        int allie = -1;
        int ennemi = -1;
        int colonneA = 0;
        int ligneA = 0;
        int colonneE = 0;
        int ligneE = 0;
        for (const QVariant& entry : arena.fighters()) {
            const QVariantMap fighter = entry.toMap();
            if (fighter.value("side").toString() == "allies") {
                allie = 1;
                colonneA = fighter.value("column").toInt();
                ligneA = fighter.value("row").toInt();
            } else {
                ennemi = 1;
                colonneE = fighter.value("column").toInt();
                ligneE = fighter.value("row").toInt();
            }
        }
        EXPECT_EQ(allie, 1);
        EXPECT_EQ(ennemi, 1);
        return std::max(std::abs(colonneA - colonneE), std::abs(ligneA - ligneE));
    };
    int avant = distance();
    ASSERT_GT(avant, 2) << "les deux camps entrent loin l'un de l'autre";
    bool attaque = false;
    for (int tour = 0; tour < 3 && !arena.ended(); ++tour) {
        arena.endTurn();
        const int apres = distance();
        const QStringList journal = arena.journal();
        attaque = attaque || std::ranges::any_of(journal, [](const QString& ligne) {
                      return ligne.contains(QStringLiteral("attaque"));
                  });
        EXPECT_TRUE(apres < avant || attaque) << "tour " << tour << " : " << avant << " -> " << apres;
        EXPECT_FALSE(std::ranges::any_of(journal, [](const QString& ligne) {
            return ligne.contains(QStringLiteral("deplacement refuse"));
        }));
        avant = std::min(avant, apres);
    }
    EXPECT_TRUE(std::ranges::any_of(arena.journal(), [](const QString& ligne) {
        return ligne.startsWith(QStringLiteral("pas "));
    })) << "le journal porte les pas de l'IA";
}
