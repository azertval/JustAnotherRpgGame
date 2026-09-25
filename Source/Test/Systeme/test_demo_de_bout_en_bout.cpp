// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_demo_de_bout_en_bout.cpp
 * @brief Tests système : la démo jouée de bout en bout **par les modèles du jeu**, sans fenêtre,
 *        depuis « Nouvelle partie » jusqu'à chacune de ses trois fins (`LOT-120`, `LOT-146`).
 *
 * Ce qui est rejoué ici est ce que les écrans QML font : `GameView` ouvre le dialogue qu'un PNJ
 * demande et gèle la carte, `Dialogue` engage la rencontre ou ouvre l'écran de fin,
 * `CombatHud` ouvre l'écran de mort à la défaite, `Death` et `DemoEnd` finissent la partie. Les
 * modèles sont ceux du jeu, le contenu est celui qui est livré, le hasard est **fixé** : la graine
 * du dialogue force l'issue du jet de Persuasion, celle du combat force son issue. Un test par
 * fin : la parole, l'arène gagnée, la mort sur le sable.
 */

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QObject>
#include <QString>
#include <QThread>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Gameplay/WorldFlags.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Math/DeterministicRandom.h"
#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Check.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/Rpg/Skill.h"
#include "Core/World/ExplorationSession.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Runtime/DialogueModel.h"
#include "HMI/Runtime/EncounterModel.h"
#include "HMI/Runtime/ScreenRouter.h"
#include "HMI/Runtime/WorldModel.h"

namespace {

const std::filesystem::path ELEMENTS{JADG_ELEMENTS_DIR};

constexpr std::string_view DRAPEAU = "quete.pommes";
constexpr std::string_view RENCONTRE = "arene-combattant";
constexpr const char* MARTPART = "central-empire/capital/martpart";
constexpr const char* ARENAREA = "central-empire/capital/arenarea";
constexpr const char* SABLE = "central-empire/capital/arenarea/arena-of-fate";
constexpr const char* VESTIAIRES = "central-empire/capital/arenarea/arena-of-fate/undercroft";
/// Le DD du jet de Persuasion du garde : le degré « moyenne » (`Rpg/rules/difficulty.json`).
constexpr int DD_PERSUASION = 15;

// Les cases des cartes de principe (voir Integration/test_quete_des_pommes.cpp).
constexpr core::GridPosition MARKET_GATE{0, 9};
constexpr core::GridPosition DEVANT_LA_MERE{9, 5};
constexpr core::GridPosition DEVANT_L_ENFANT_CHEZ_SA_MERE{12, 5};
constexpr core::GridPosition STRAVIAN_AVENUE{22, 4};
constexpr core::GridPosition HEROFATE_AVENUE{1, 5};
constexpr core::GridPosition ENTREE_DU_PARVIS{8, 5};
constexpr core::GridPosition DEVANT_LE_GARDE{11, 5};
constexpr core::GridPosition DEVANT_L_ESCALIER{22, 6};
constexpr core::GridPosition ARRIVEE_AUX_VESTIAIRES{9, 3};
constexpr core::GridPosition PIED_DE_L_ESCALIER{7, 2};
constexpr core::GridPosition DEVANT_LE_MAITRE{9, 10};
constexpr core::GridPosition PORTE_DU_TRIOMPHE{16, 4};

using Screen = hmi::ScreenRouter::Screen;
using RpgScreen = hmi::ScreenRouter::RpgScreen;

/// Fait tourner la boucle d'événements — les horloges des modèles — jusqu'à ce que @p condition
/// tienne, au plus @p millisecondes. @return Vrai si la condition a tenu.
bool pomper(const std::function<bool()>& condition, int millisecondes) {
    QElapsedTimer chrono;
    chrono.start();
    while (!condition()) {
        if (chrono.elapsed() > millisecondes) {
            return false;
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        QThread::msleep(2);
    }
    return true;
}

void attendre(int millisecondes) {
    static_cast<void>(pomper([] { return false; }, millisecondes));
}

/// La première graine, à partir de 1, dont le d20 du jet de Persuasion du héros de la démo
/// réussit (ou échoue) le DD « moyenne » : c'est celle que le test donne au dialogue du garde.
[[nodiscard]] int graineDePersuasion(bool reussite) {
    const std::filesystem::path rpg = ELEMENTS / "Rpg";
    const core::CharacterOptions options =
        core::loadCharacterOptions(rpg / "species", rpg / "backgrounds", rpg / "classes");
    const core::SkillCatalog competences = core::loadSkills(rpg / "skills");
    const core::ExperienceTable experience =
        core::loadExperienceTable(rpg / "rules" / "experience.json");
    const core::CharacterCreationRules regles =
        core::loadCharacterCreationRules(rpg / "rules" / "character-creation.json");
    core::LoadedCharacterSheet heros = core::loadCharacterSheet(
        rpg / "characters" / "heros-brawler.json", options, regles, experience);
    core::CharacterListener auditeur(heros.sheet, heros.inventory, experience, competences);
    const std::vector<core::Modifier> modificateurs = auditeur.skillModifiers("persuasion");
    for (int graine = 1; graine < 1000; ++graine) {
        core::DeterministicRandom hasard{static_cast<std::uint64_t>(graine)};
        const core::CheckResult jet =
            core::rollCheck(DD_PERSUASION, modificateurs, core::RollStance::Normal, hasard);
        if (jet.succeeded() == reussite) {
            return graine;
        }
    }
    return 0;
}

/**
 * @brief Le jeu sans fenêtre : ses modèles, et le câblage que ses écrans QML font entre eux.
 */
class Jeu {
public:
    Jeu() {
        // GameView : un PNJ demande un dialogue -> la carte gèle, l'écran de dialogue s'ouvre.
        QObject::connect(&monde, &hmi::WorldModel::dialogueRequested, [this](const QString& id) {
            monde.setFrozen(true);
            router.openDialogue(id);
            ouvrirLeDialogue(id);
        });
        // GameView : une rencontre demandée par la carte.
        QObject::connect(&monde, &hmi::WorldModel::encounterRequested,
                         [this](const QString& id) { engager(id); });
        QObject::connect(&monde, &hmi::WorldModel::questAdvanced,
                         [this](const QString& quete, const QString& etape) {
                             etapes.push_back((quete + "/" + etape).toStdString());
                         });
        rencontre.setContentRoot(ELEMENTS);
    }

    /// « Nouvelle partie » : le menu ouvre la vue de jeu, qui démarre la partie.
    void nouvellePartie() {
        router.openGame();
        ASSERT_TRUE(monde.startNewGame()) << monde.status().toStdString();
        ASSERT_EQ(router.currentScreen(), Screen::Game);
    }

    [[nodiscard]] core::GridPosition heros() const {
        return monde.play().session().heroCell();
    }
    [[nodiscard]] std::string carte() const {
        return monde.mapId().toStdString();
    }
    [[nodiscard]] std::string valeur() {
        return monde.flags().value(DRAPEAU).value_or("(non declare)");
    }

    /// Pose le héros en @p ou et le fait marcher dans le sens @p sens, au plus @p millisecondes,
    /// jusqu'à ce que @p condition tienne.
    bool marcher(core::GridPosition ou, core::Vector2 sens, const std::function<bool()>& condition,
                 int millisecondes = 4000) {
        monde.placeHero(core::cellCenter(ou));
        monde.setMove(sens.x, sens.y);
        const bool tenu = pomper(condition, millisecondes);
        monde.setMove(0.0, 0.0);
        attendre(40);
        return tenu;
    }

    /// Marche depuis @p ou jusqu'à entrer sur la carte @p cible.
    bool passerLePortail(core::GridPosition ou, core::Vector2 sens, const char* cible) {
        return marcher(ou, sens, [this, cible] { return carte() == cible; });
    }

    /// Le héros en @p ou, tourné vers @p regard, interagit : le dialogue qui s'ouvre, s'il en est.
    std::optional<std::string> parler(core::GridPosition ou, core::Vector2 regard = {1.0F, 0.0F}) {
        dialogue.reset();
        monde.placeHero(core::cellCenter(ou));
        monde.setMove(regard.x, regard.y);
        attendre(40);
        monde.setMove(0.0, 0.0);
        attendre(40);
        monde.interact();
        if (!pomper([this] { return dialogue != nullptr; }, 500)) {
            return std::nullopt;
        }
        return dialogue->dialogueId().toStdString();
    }

    /// Donne les réponses @p reponses au dialogue ouvert, une réplique sans réponse passant
    /// d'elle-même ; puis referme l'écran, comme `Dialogue.qml` quand la conversation est finie.
    void repondre(const std::vector<std::string>& reponses) {
        ASSERT_NE(dialogue, nullptr) << "aucun dialogue ouvert";
        for (const std::string& reponse : reponses) {
            ASSERT_FALSE(dialogue->finished()) << "le dialogue a fini avant « " << reponse << " »";
            dialogue->choose(QString::fromStdString(reponse));
        }
        EXPECT_TRUE(dialogue->finished()) << "des repliques attendent encore une reponse";
        fermerLeDialogue();
    }

    /// Joue la rencontre engagée, comme `CombatHud` : attaquer la cible visée, finir le tour, et
    /// laisser l'IA jouer ; jusqu'à l'issue. @return `victory`, `defeat` ou `flight`.
    std::string combattre() {
        for (int tour = 0; tour < 60 && !rencontre.ended(); ++tour) {
            jusquAuJoueur();
            if (rencontre.ended()) {
                break;
            }
            rencontre.selectAction(0);
            rencontre.cycleTarget(1);
            rencontre.confirm();
            rencontre.endTurn();
        }
        jusquAuJoueur();
        for (int pas = 0; pas < 2000 && rencontre.outcome().isEmpty(); ++pas) {
            rencontre.tick(1.0F / 60.0F);
        }
        return rencontre.outcome().toStdString();
    }

    /// Engage la rencontre du maître d'arène à la graine @p graine, la joue, et rend son issue.
    std::string combattreALaGraine(int graine) {
        rencontre.setSeed(graine);
        EXPECT_TRUE(dialogueEngage.has_value()) << "le maitre d'arene n'a rien engage";
        if (!dialogueEngage) {
            return {};
        }
        // Dialogue.qml : l'ecran de dialogue se ferme, la rencontre se monte, l'affichage de
        // combat s'ouvre.
        if (router.currentScreen() == Screen::RpgScreen) {
            router.closeRpgScreen();
        }
        EXPECT_TRUE(rencontre.begin(QString::fromStdString(*dialogueEngage)))
            << rencontre.status().toStdString();
        router.openRpgScreen(RpgScreen::CombatHud);
        EXPECT_EQ(router.currentScreen(), Screen::RpgScreen);
        EXPECT_EQ(router.currentRpgScreen(), RpgScreen::CombatHud);
        return combattre();
    }

    /// `CombatHud.leave()` : à la défaite, l'écran de mort ; sinon la rencontre est quittée.
    void quitterLeCombat() {
        if (rencontre.outcome() == QStringLiteral("defeat")) {
            router.openDeath();
            return;
        }
        rencontre.leave();
        router.closeRpgScreen();
        monde.setFrozen(rencontre.active());
    }

    /// `Death.activate()` puis `DemoEnd` : la partie finit.
    void finirLaPartie() {
        if (rencontre.active()) {
            rencontre.leave();
        }
        monde.endGame();
    }

    hmi::ScreenRouter router;
    hmi::WorldModel monde;
    hmi::EncounterModel rencontre;
    std::unique_ptr<hmi::DialogueModel> dialogue;
    /// La graine que reçoit le prochain dialogue ouvert ; 0, celle du jeu.
    int graineDuDialogue = 0;
    /// Ce que les dialogues ont demandé : la rencontre à engager, la voie de la fin.
    std::optional<std::string> dialogueEngage;
    std::optional<std::string> fin;
    std::vector<std::string> etapes;

private:
    void ouvrirLeDialogue(const QString& id) {
        dialogue = std::make_unique<hmi::DialogueModel>();
        dialogue->setSeed(graineDuDialogue);
        // Dialogue.qml : la rencontre s'engage à la fermeture de l'écran ; la fin de la démo
        // ouvre son écran.
        QObject::connect(
            dialogue.get(), &hmi::DialogueModel::encounterRequested,
            [this](const QString& rencontreId) { dialogueEngage = rencontreId.toStdString(); });
        QObject::connect(dialogue.get(), &hmi::DialogueModel::demoEnded,
                         [this](const QString& voie) {
                             fin = voie.toStdString();
                             router.closeRpgScreen();
                             router.openDemoEnd(voie);
                         });
        dialogue->setDialogueId(id);
    }

    void fermerLeDialogue() {
        if (router.currentScreen() == Screen::RpgScreen) {
            router.closeRpgScreen();
        }
        dialogue.reset();
        // GameView reprend le clavier : la carte ne reste gelée que si un combat est engagé.
        monde.setFrozen(rencontre.active());
    }

    void engager(const QString& id) {
        if (rencontre.begin(id)) {
            router.openRpgScreen(RpgScreen::CombatHud);
        }
    }

    void jusquAuJoueur() {
        for (int pas = 0; pas < 4000; ++pas) {
            rencontre.tick(1.0F / 60.0F);
            if (rencontre.ended() ? !rencontre.busy()
                                  : (!rencontre.busy() && !rencontre.turnActions().isEmpty())) {
                return;
            }
        }
        ADD_FAILURE() << "ni tour du joueur ni fin en 4000 pas";
    }
};

/// Le début commun : Nouvelle partie à Market Gate, la mère, Stravian Avenue, le parvis.
void jusquAuParvis(Jeu& jeu) {
    hmi::setDataDirectory(ELEMENTS);
    ASSERT_NO_FATAL_FAILURE(jeu.nouvellePartie());
    EXPECT_EQ(jeu.carte(), MARTPART);
    EXPECT_EQ(jeu.heros(), MARKET_GATE) << "Nouvelle partie entre par Market Gate";
    EXPECT_EQ(jeu.valeur(), "inconnue");

    ASSERT_EQ(jeu.parler(DEVANT_LA_MERE), "mere");
    ASSERT_NO_FATAL_FAILURE(jeu.repondre({"accepter", "continue"}));
    ASSERT_TRUE(pomper([&jeu] { return !jeu.etapes.empty(); }, 1000));
    EXPECT_EQ(jeu.etapes, (std::vector<std::string>{"pommes/acceptee"}));

    ASSERT_TRUE(jeu.passerLePortail(STRAVIAN_AVENUE, {1.0F, 0.0F}, ARENAREA));
    // La zone du parvis ouvre le dialogue du garde à l'entrée.
    ASSERT_TRUE(
        jeu.marcher(ENTREE_DU_PARVIS, {1.0F, 0.0F}, [&jeu] { return jeu.dialogue != nullptr; }));
    ASSERT_EQ(jeu.dialogue->dialogueId().toStdString(), "garde");
}

/// Le retour à l'étal depuis Herofate Avenue, et la fin de la démo par la voie @p voie.
void finirChezLaMere(Jeu& jeu, const char* voie) {
    ASSERT_TRUE(jeu.passerLePortail(HEROFATE_AVENUE, {-1.0F, 0.0F}, MARTPART));
    EXPECT_EQ(jeu.parler(DEVANT_L_ENFANT_CHEZ_SA_MERE, {-1.0F, 0.0F}), "enfant")
        << "l'enfant est rentre aupres de sa mere";
    ASSERT_NO_FATAL_FAILURE(jeu.repondre({"continue"}));
    ASSERT_EQ(jeu.parler(DEVANT_LA_MERE), "mere");
    ASSERT_NO_FATAL_FAILURE(jeu.repondre({"sourire"}));
    ASSERT_TRUE(jeu.fin.has_value());
    EXPECT_EQ(*jeu.fin, voie);
    EXPECT_EQ(jeu.router.currentScreen(), Screen::DemoEnd);
    EXPECT_EQ(jeu.router.ending().toStdString(), voie);
    jeu.finirLaPartie();
    EXPECT_FALSE(jeu.monde.loaded()) << "l'ecran de fin ferme la partie";
}

}  // namespace

/**
 * @brief La fin par la parole : la Persuasion réussit, à la graine qui la fait réussir.
 * \castest{<b>Nouvelle partie, puis la demo jusqu'a sa fin par la parole.</b><br/>
 * \tcat Systeme · Demo<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Nouvelle partie : Market Gate.<br/>2. La mere, accepter.<br/>3. Stravian Avenue, le
 * parvis ; le garde, convaincre, a une graine dont le d20 reussit le DD.<br/>4. Retour a l'etal par
 * les portails ; la mere.<br/>
 * \tattendu `enfant-libere` ; l'ecran « Fin de la demo » s'ouvre par la voie `parole` ; la partie
 * est finie.
 * }
 */
TEST(DemoDeBoutEnBout, LaFinParLaParole) {
    Jeu jeu;
    ASSERT_NO_FATAL_FAILURE(jusquAuParvis(jeu));

    // Le dialogue du garde est deja ouvert par la zone : sa graine est celle du jeu. On le quitte
    // et on lui reparle a la graine qui fait reussir le jet.
    ASSERT_NO_FATAL_FAILURE(jeu.repondre({"partir"}));
    jeu.graineDuDialogue = graineDePersuasion(/*reussite=*/true);
    ASSERT_NE(jeu.graineDuDialogue, 0);
    ASSERT_EQ(jeu.parler(DEVANT_LE_GARDE), "garde");
    ASSERT_NO_FATAL_FAILURE(jeu.repondre({"convaincre", "continue"}));
    EXPECT_EQ(jeu.valeur(), "enfant-libere");
    ASSERT_TRUE(pomper([&jeu] { return jeu.etapes.size() >= 2; }, 1000));
    EXPECT_EQ(jeu.etapes.back(), "pommes/enfant-libere");
    EXPECT_EQ(jeu.parler(DEVANT_LE_GARDE), std::nullopt) << "le garde a quitte le parvis";

    ASSERT_NO_FATAL_FAILURE(finirChezLaMere(jeu, "parole"));
}

/**
 * @brief La fin par l'arène : la Persuasion échoue, le joueur endosse le crime, gagne sur le sable.
 * \castest{<b>Nouvelle partie, puis la demo jusqu'a sa fin par l'arene.</b><br/>
 * \tcat Systeme · Demo<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Jusqu'au parvis ; le garde a une graine dont le d20 echoue : convaincre, puis
 * endosser.<br/>2. L'escalier de l'arene : le vestiaire, la porte close ; la porte du triomphe : le
 * sable.<br/>3. Le maitre d'arene engage la rencontre ; la jouer a la premiere graine qui la
 * gagne.<br/>4. Redescendre, passer la porte ouverte, revenir a l'etal.<br/>
 * \tattendu `persuasion-echouee` puis `condamne` ; la victoire pose `enfant-libere` ; l'ecran de
 * fin s'ouvre par la voie `arene`.
 * }
 */
TEST(DemoDeBoutEnBout, LaFinParLArene) {
    Jeu jeu;
    ASSERT_NO_FATAL_FAILURE(jusquAuParvis(jeu));
    ASSERT_NO_FATAL_FAILURE(jeu.repondre({"partir"}));
    jeu.graineDuDialogue = graineDePersuasion(/*reussite=*/false);
    ASSERT_NE(jeu.graineDuDialogue, 0);
    ASSERT_EQ(jeu.parler(DEVANT_LE_GARDE), "garde");
    ASSERT_NO_FATAL_FAILURE(jeu.repondre({"convaincre", "endosser", "continue"}));
    EXPECT_EQ(jeu.valeur(), "condamne");
    ASSERT_TRUE(pomper([&jeu] { return jeu.etapes.size() >= 3; }, 1000));
    EXPECT_EQ(jeu.etapes, (std::vector<std::string>{"pommes/acceptee", "pommes/persuasion-echouee",
                                                    "pommes/condamne"}));
    jeu.graineDuDialogue = 0;

    // L'escalier de l'arene : le vestiaire A, derriere la porte close.
    ASSERT_TRUE(jeu.passerLePortail(DEVANT_L_ESCALIER, {1.0F, 0.0F}, VESTIAIRES));
    EXPECT_EQ(jeu.heros(), ARRIVEE_AUX_VESTIAIRES);
    EXPECT_FALSE(jeu.passerLePortail(ARRIVEE_AUX_VESTIAIRES, {1.0F, 0.0F}, ARENAREA))
        << "la porte de l'arene est close : on ne revient pas au parvis";
    EXPECT_EQ(jeu.carte(), VESTIAIRES);
    ASSERT_TRUE(jeu.passerLePortail(PIED_DE_L_ESCALIER, {0.0F, -1.0F}, SABLE));

    // Le maitre d'arene ; le combat, a la premiere graine qui le gagne.
    ASSERT_EQ(jeu.parler(DEVANT_LE_MAITRE), "maitre-arene");
    ASSERT_NO_FATAL_FAILURE(jeu.repondre({"combattre"}));
    ASSERT_TRUE(jeu.dialogueEngage.has_value());
    std::optional<int> gagnante;
    for (int graine = 1; graine <= 30 && !gagnante; ++graine) {
        const std::string issue = jeu.combattreALaGraine(graine);
        ASSERT_FALSE(issue.empty()) << "graine " << graine << " : pas d'issue";
        if (issue == "victory") {
            gagnante = graine;
        }
        jeu.quitterLeCombat();
        ASSERT_NE(jeu.router.currentScreen(), Screen::Death) << "graine " << graine;
    }
    ASSERT_TRUE(gagnante.has_value()) << "aucune graine gagnante en trente";
    ::testing::Test::RecordProperty("graine_gagnante", *gagnante);
    EXPECT_TRUE(jeu.monde.flags().isSet(core::encounterWonFlag(RENCONTRE)));
    ASSERT_TRUE(pomper([&jeu] { return jeu.etapes.size() >= 5; }, 1000));
    EXPECT_EQ(jeu.etapes.back(), "pommes/enfant-libere");
    EXPECT_EQ(jeu.valeur(), "enfant-libere");
    EXPECT_EQ(jeu.parler(DEVANT_LE_MAITRE), std::nullopt) << "le maitre s'en est alle";

    // Le retour : l'escalier, la porte ouverte, le parvis, l'avenue, l'etal.
    ASSERT_TRUE(jeu.passerLePortail(PORTE_DU_TRIOMPHE, {0.0F, -1.0F}, VESTIAIRES));
    ASSERT_TRUE(jeu.passerLePortail(ARRIVEE_AUX_VESTIAIRES, {1.0F, 0.0F}, ARENAREA))
        << "la porte s'ouvre sous enfant-libere";
    ASSERT_NO_FATAL_FAILURE(finirChezLaMere(jeu, "arene"));
}

/**
 * @brief La mort sur le sable : la défaite ouvre l'écran de mort, et « Recommencer » rouvre une
 *        partie neuve à Market Gate.
 * \castest{<b>Nouvelle partie, puis la demo jusqu'a la mort sur le sable.</b><br/>
 * \tcat Systeme · Demo<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Jusqu'au sable, condamne.<br/>2. Le combat a la premiere graine qui le perd.<br/>
 * 3. Recommencer.<br/>
 * \tattendu L'ecran de mort s'ouvre ; ni le fait de la victoire ni `enfant-libere` ; la partie
 * neuve repart a Market Gate, la quete inconnue.
 * }
 */
TEST(DemoDeBoutEnBout, LaMortSurLeSable) {
    Jeu jeu;
    ASSERT_NO_FATAL_FAILURE(jusquAuParvis(jeu));
    ASSERT_NO_FATAL_FAILURE(jeu.repondre({"endosser", "continue"}));
    EXPECT_EQ(jeu.valeur(), "condamne");
    ASSERT_TRUE(jeu.passerLePortail(DEVANT_L_ESCALIER, {1.0F, 0.0F}, VESTIAIRES));
    ASSERT_TRUE(jeu.passerLePortail(PIED_DE_L_ESCALIER, {0.0F, -1.0F}, SABLE));
    ASSERT_EQ(jeu.parler(DEVANT_LE_MAITRE), "maitre-arene");
    ASSERT_NO_FATAL_FAILURE(jeu.repondre({"combattre"}));

    std::optional<int> perdante;
    for (int graine = 1; graine <= 30 && !perdante; ++graine) {
        const std::string issue = jeu.combattreALaGraine(graine);
        ASSERT_FALSE(issue.empty()) << "graine " << graine << " : pas d'issue";
        if (issue == "defeat") {
            perdante = graine;
            break;
        }
        // Une graine gagnante en chemin : on quitte le combat et on remet le de -- le fait de
        // la victoire, pose a la sortie, ne doit pas faire avancer la quete de ce test-ci.
        jeu.quitterLeCombat();
        static_cast<void>(jeu.monde.flags().clear(core::encounterWonFlag(RENCONTRE)));
    }
    ASSERT_TRUE(perdante.has_value()) << "aucune graine perdante en trente";
    ::testing::Test::RecordProperty("graine_perdante", *perdante);

    // CombatHud : la defaite ouvre l'ecran de mort, sans quitter la rencontre.
    jeu.quitterLeCombat();
    EXPECT_EQ(jeu.router.currentScreen(), Screen::Death);
    EXPECT_TRUE(jeu.rencontre.active());
    EXPECT_FALSE(jeu.monde.flags().isSet(core::encounterWonFlag(RENCONTRE)));

    // Death : « Recommencer » finit la partie et en rouvre une neuve, a Market Gate.
    jeu.finirLaPartie();
    EXPECT_FALSE(jeu.monde.loaded());
    jeu.router.openGame();
    ASSERT_TRUE(jeu.monde.startNewGame()) << jeu.monde.status().toStdString();
    EXPECT_EQ(jeu.carte(), MARTPART);
    EXPECT_EQ(jeu.heros(), MARKET_GATE);
    EXPECT_EQ(jeu.valeur(), "inconnue") << "une partie neuve a tout oublie";
}
