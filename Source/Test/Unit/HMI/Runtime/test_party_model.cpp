// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_party_model.cpp
 * @brief Tests du groupe dans la partie (`LOT-138`) : le meneur change la figurine menée, le
 *        portrait et la voix du dialogue, et celui qui combat ; les suiveurs se dessinent derrière
 *        lui ; l'écran de groupe lit les fiches des quatre.
 *
 * Comme les autres tests des vues-modèles, les règles et les fiches se lisent à côté de
 * l'exécutable ; la carte et la rencontre viennent de la racine d'essai.
 */

#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Runtime/DialogueModel.h"
#include "HMI/Runtime/EncounterModel.h"
#include "HMI/Runtime/PartyModel.h"
#include "HMI/Runtime/WorldModel.h"

namespace {

[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path{JADG_TEST_DATA_DIR};
}

/// Ouvre le donjon d'essai, le héros une case au nord du maître d'arène d'essai.
void ouvrirLeDonjon(hmi::WorldModel& monde) {
    monde.setLevelDirectories({dataRoot() / "Levels"});
    monde.setStartOverride(QStringLiteral("donjon"), QStringLiteral("sable"));
    monde.setStartCell(core::GridPosition{.column = 24, .row = 19});
    ASSERT_TRUE(monde.startNewGame()) << monde.status().toStdString();
}

/// Fuit la rencontre en cours et la quitte : la file avance jusqu'au tour du joueur, il se retire,
/// la file avance jusqu'a l'issue.
void fuir(hmi::EncounterModel& rencontre) {
    const auto avancer = [&rencontre]() {
        for (int pas = 0; pas < 2000; ++pas) {
            rencontre.tick(1.0F / 60.0F);
            if (rencontre.ended() ? !rencontre.busy()
                                  : (!rencontre.busy() && !rencontre.turnActions().isEmpty())) {
                return;
            }
        }
        FAIL() << "ni tour du joueur ni fin en 2000 pas";
    };
    // Chaque membre se retire a son tour (LOT-139) : la fuite est celle de tous.
    for (int tours = 0; tours < 8 && !rencontre.ended(); ++tours) {
        avancer();
        if (!rencontre.ended()) {
            rencontre.withdraw();
        }
    }
    avancer();
    rencontre.leave();
}

[[nodiscard]] QStringList identifiants(const QVariantList& lignes) {
    QStringList ids;
    for (const QVariant& ligne : lignes) {
        ids << ligne.toMap().value(QStringLiteral("id")).toString();
    }
    return ids;
}

}  // namespace

/**
 * @brief Changer de meneur change la figurine menee et le portrait du dialogue (EX-EXP-014).
 * \castest{<b>Le groupe de depart compte les quatre fiches pre-tirees ; passer la main change la
 * figurine menee, le portrait du meneur et la voix du dialogue.</b><br/>
 * \tcat Unitaire · Groupe<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Nouvelle partie dans le donjon d'essai.<br/>
 * 2. Passer la main au suivant (la touche `Tab`).<br/>
 * 3. Ouvrir un dialogue.<br/>
 * \tattendu Au depart, le groupe preforme : le Brawler mene, avec sa figurine et son portrait,
 * suivi du Priest, du Scoundrel et du Mage ; apres, le Priest mene : sa figurine (celle de sa
 * classe), un autre portrait, le Brawler passe en queue ; le dialogue parle par Helga Pierre-Sure.
 * }
 */
TEST(PartyModelTest, ChangerDeMeneurChangeLaFigurineEtLePortrait) {
    hmi::WorldModel monde;
    ouvrirLeDonjon(monde);

    EXPECT_EQ(identifiants(monde.partyMembers()),
              (QStringList{"heros-brawler", "heros-priest", "heros-scoundrel", "heros-mage"}));
    EXPECT_EQ(monde.leaderId(), QStringLiteral("heros-brawler"));
    EXPECT_EQ(monde.heroFigure(), QStringLiteral("Common/Characters/Heroes/brawler"));
    EXPECT_EQ(monde.play().followerFigures(),
              (std::vector<std::string>{"Common/Characters/Heroes/priest",
                                        "Common/Characters/Heroes/scoundrel",
                                        "Common/Characters/Heroes/mage"}));
    const std::filesystem::path portraitDuBrawler =
        hmi::dataDirectory() / "Assets" / "Common/Characters/Heroes/brawler/portrait.png";
    const QUrl avant = monde.leaderPortrait();
    if (std::filesystem::exists(portraitDuBrawler)) {
        EXPECT_EQ(avant, QUrl::fromLocalFile(QString::fromStdString(portraitDuBrawler.string())));
    }

    int annonces = 0;
    QObject::connect(&monde, &hmi::WorldModel::partyChanged, [&annonces]() { ++annonces; });
    ASSERT_TRUE(monde.rotateLeader());
    EXPECT_EQ(annonces, 1);
    EXPECT_EQ(monde.leaderId(), QStringLiteral("heros-priest"));
    EXPECT_EQ(monde.leaderName(), QStringLiteral("Helga Pierre-Sûre"));
    EXPECT_EQ(monde.heroFigure(), QStringLiteral("Common/Characters/Heroes/priest"));
    EXPECT_EQ(monde.play().followerFigures().back(), "Common/Characters/Heroes/brawler");
    EXPECT_NE(monde.leaderPortrait(), avant);

    // La figurine menee est la derniere de l'image : le heros, par-dessus ses trois suiveurs.
    const std::vector<hmi::WorldFigureSnapshot> figures = monde.figures();
    ASSERT_GE(figures.size(), 4U);
    EXPECT_TRUE(figures.back().hero);
    std::size_t suiveurs = 0;
    for (std::size_t rang = figures.size() - 4; rang + 1 < figures.size(); ++rang) {
        suiveurs += figures[rang].hero ? 0 : 1;
    }
    EXPECT_EQ(suiveurs, 3U);

    hmi::DialogueModel dialogue;
    EXPECT_EQ(dialogue.partyVoice(), QStringLiteral("Helga Pierre-Sûre"));
}

/**
 * @brief Le joueur choisit qui parle pour le groupe, et c'est lui qui jette (D-28).
 * \castest{<b>Dans le dialogue, le menu du bas donne la parole a un membre du groupe : le jet de
 * Persuasion se fait avec ses modificateurs.</b><br/>
 * \tcat Unitaire · Groupe<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ouvrir le dialogue du garde a la graine 7 : le meneur (Grom, Charisme 8) parle ;
 * tenter de le convaincre.<br/>
 * 2. Rouvrir a la meme graine, donner la parole a Nessa (Charisme 13), tenter de nouveau.<br/>
 * \tattendu Quatre voix, le meneur d'abord et choisi ; le meme d20 les deux fois, mais un total
 * plus haut pour Nessa ; un personnage hors du groupe ne prend pas la parole.
 * }
 */
TEST(PartyModelTest, LeJoueurChoisitQuiParle) {
    hmi::WorldModel monde;
    ouvrirLeDonjon(monde);

    const auto jet = [](const QString& voix, QString& de) {
        hmi::DialogueModel dialogue;
        dialogue.setSeed(7);
        dialogue.setDialogueId(QStringLiteral("garde"));
        if (!voix.isEmpty()) {
            EXPECT_TRUE(dialogue.selectVoice(voix));
        }
        EXPECT_FALSE(dialogue.selectVoice(QStringLiteral("heros-inconnu")));
        dialogue.chooseAt(0);  // convaincre : le jet de Persuasion
        de = dialogue.checkDie();
        const QString detail = dialogue.checkDetail();
        return detail.section(QLatin1Char('='), -1).trimmed().toInt();
    };

    {
        hmi::DialogueModel dialogue;
        const QVariantList voix = dialogue.voices();
        ASSERT_EQ(voix.size(), 4);
        EXPECT_TRUE(voix.front().toMap().value(QStringLiteral("current")).toBool());
        EXPECT_EQ(dialogue.voiceId(), QStringLiteral("heros-brawler"));
        dialogue.cycleVoice(-1);
        EXPECT_EQ(dialogue.voiceId(), QStringLiteral("heros-mage")) << "le precedent du premier";
    }

    QString deDeGrom;
    QString deDeNessa;
    const int totalDeGrom = jet({}, deDeGrom);
    // Le drapeau d'echec eventuel du premier essai ne doit pas aiguiller le second.
    monde.endGame();
    ouvrirLeDonjon(monde);
    const int totalDeNessa = jet(QStringLiteral("heros-scoundrel"), deDeNessa);
    ASSERT_FALSE(deDeGrom.isEmpty());
    EXPECT_EQ(deDeGrom, deDeNessa);
    EXPECT_GT(totalDeNessa, totalDeGrom);
}

/**
 * @brief Le meneur est celui qui combat sur la carte (EX-EXP-014).
 * \castest{<b>Une rencontre engagee apres un changement de meneur met le nouveau meneur en
 * jeu.</b><br/>
 * \tcat Unitaire · Groupe<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Engager « rats-du-donjon », puis fuir.<br/>
 * 2. Faire mener la Scoundrel, engager de nouveau.<br/>
 * \tattendu Le premier combat met Grom Tranche-Ecaille en jeu, le second Nessa Double-Vie : le
 * heros de combat se relit quand le meneur change.
 * }
 */
TEST(PartyModelTest, LeMeneurEstCeluiQuiCombat) {
    hmi::WorldModel monde;
    ouvrirLeDonjon(monde);

    hmi::EncounterModel rencontre;
    rencontre.setContentRoot(dataRoot());
    rencontre.setSeed(2026);
    ASSERT_TRUE(rencontre.begin(QStringLiteral("rats-du-donjon")))
        << rencontre.status().toStdString();
    EXPECT_EQ(rencontre.heroName(), QStringLiteral("Grom Tranche-Écaille"));
    fuir(rencontre);
    ASSERT_FALSE(rencontre.active());

    ASSERT_TRUE(monde.setLeader(QStringLiteral("heros-scoundrel")));
    monde.placeHero(core::cellCenter({.column = 24, .row = 19}));
    ASSERT_TRUE(rencontre.begin(QStringLiteral("rats-du-donjon")))
        << rencontre.status().toStdString();
    EXPECT_EQ(rencontre.heroName(), QStringLiteral("Nessa Double-Vie"));
    fuir(rencontre);
}

/**
 * @brief L'ecran de groupe compose : prendre, laisser, mener, avancer (EX-EXP-013).
 * \castest{<b>L'ecran de groupe lit la fiche des quatre et compose le groupe de la partie.</b><br/>
 * \tcat Unitaire · Groupe<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ouvrir le modele de l'ecran de groupe.<br/>
 * 2. Laisser la Scoundrel, puis tenter de laisser tous les autres.<br/>
 * 3. Reprendre la Scoundrel, la faire mener, reculer le Mage.<br/>
 * \tattendu Chaque ligne porte les points de vie, la classe et la classe d'armure de la fiche ;
 * laisser un membre raccourcit la file (trois suiveurs de moins un) ; le dernier ne se laisse
 * pas ; la Scoundrel reprise mene ; le Mage recule d'un rang.
 * }
 */
TEST(PartyModelTest, LEcranDeGroupeCompose) {
    hmi::WorldModel monde;
    ouvrirLeDonjon(monde);
    hmi::PartyModel groupe;

    ASSERT_EQ(groupe.candidates().size(), 4);
    const QVariantMap brawler = groupe.candidates().front().toMap();
    EXPECT_EQ(brawler.value(QStringLiteral("value")).toString(), QStringLiteral("15 / 15"));
    EXPECT_EQ(brawler.value(QStringLiteral("armorClass")).toString(), QStringLiteral("14"));
    EXPECT_EQ(brawler.value(QStringLiteral("rank")).toInt(), 0);
    EXPECT_TRUE(brawler.value(QStringLiteral("leader")).toBool());
    EXPECT_DOUBLE_EQ(groupe.members().front().toMap().value(QStringLiteral("ratio")).toDouble(),
                     1.0);
    EXPECT_EQ(groupe.leaderHitPoints(), QStringLiteral("15 / 15"));

    ASSERT_TRUE(groupe.toggleMember(QStringLiteral("heros-scoundrel")));
    EXPECT_EQ(groupe.size(), 3);
    EXPECT_EQ(monde.play().session().followers(), 2U);
    EXPECT_TRUE(groupe.toggleMember(QStringLiteral("heros-mage")));
    EXPECT_TRUE(groupe.toggleMember(QStringLiteral("heros-priest")));
    EXPECT_FALSE(groupe.toggleMember(QStringLiteral("heros-brawler")))
        << "le dernier membre ne se laisse pas";
    EXPECT_EQ(monde.play().session().followers(), 0U);

    EXPECT_TRUE(groupe.toggleMember(QStringLiteral("heros-mage")));
    EXPECT_TRUE(groupe.toggleMember(QStringLiteral("heros-scoundrel")));
    EXPECT_TRUE(groupe.setLeader(QStringLiteral("heros-scoundrel")));
    EXPECT_EQ(identifiants(groupe.members()),
              (QStringList{"heros-scoundrel", "heros-brawler", "heros-mage"}));
    EXPECT_TRUE(groupe.moveMember(QStringLiteral("heros-brawler"), 1));
    EXPECT_EQ(identifiants(groupe.members()),
              (QStringList{"heros-scoundrel", "heros-mage", "heros-brawler"}));
    EXPECT_FALSE(groupe.moveMember(QStringLiteral("heros-brawler"), 1)) << "deja en queue";
    EXPECT_EQ(groupe.leaderName(), QStringLiteral("Nessa Double-Vie"));

    // Une partie neuve rend le groupe preforme.
    monde.endGame();
    EXPECT_EQ(identifiants(monde.partyMembers()),
              (QStringList{"heros-brawler", "heros-priest", "heros-scoundrel", "heros-mage"}));
}

/**
 * @brief Une partie ouverte sur une carte imposée — `--map=`, le lanceur de cartes, les tests —
 *        ne demande pas de meneur (`LOT-142`) : seule « Nouvelle partie » ouvre ce choix, que le
 *        test système du parcours de la démo éprouve.
 * \castest{<b>Une carte imposee ne demande pas de meneur.</b><br/>
 * \tcat Unitaire · Groupe<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Ouvrir le donjon d'essai par une carte imposee.<br/>2. Clore un choix qui n'est
 * pas en cours.<br/>
 * \tattendu `choosingLeader` est faux, et le reste.
 * }
 */
TEST(PartyModelTest, UneCarteImposeeNeDemandePasDeMeneur) {
    hmi::WorldModel monde;
    ouvrirLeDonjon(monde);
    EXPECT_FALSE(monde.choosingLeader());
    monde.endLeaderChoice();
    EXPECT_FALSE(monde.choosingLeader());
}
