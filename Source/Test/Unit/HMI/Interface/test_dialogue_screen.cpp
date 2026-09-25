// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_dialogue_screen.cpp
 * @brief Tests unitaires de ce que l'écran de dialogue affiche (`LOT-15`) : réplique, réponses,
 *        jet annoncé puis restitué, refus, fin.
 */

#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Gameplay/WorldFlags.h"
#include "Core/Math/DeterministicRandom.h"
#include "Core/Rpg/Check.h"
#include "Core/Rpg/Dialogue.h"
#include "HMI/Presentation/DialogueScreen.h"

namespace {

/// Un catalogue d'essai qui rend la cle entre crochets : ce qui s'affiche dit d'ou il vient.
std::string cle(std::string_view key) {
    if (key == "dialogue.check.announce") {
        return "%1 DD %2";
    }
    if (key == "dialogue.check.summary") {
        return "%1 DD %2 -- d20 %3, total %4 -- %5";
    }
    if (key == "dialogue.check.repeat") {
        return "%1 DD %2 -- deja -- %3";
    }
    return "<" + std::string(key) + ">";
}

class Auditeur final : public core::DialogueListener {
public:
    Auditeur(std::set<std::string> langues, int bonus)
        : _langues(std::move(langues)), _bonus(bonus) {}
    [[nodiscard]] bool speaks(std::string_view l) const override {
        return _langues.contains(std::string(l));
    }
    [[nodiscard]] std::vector<core::Modifier> skillModifiers(std::string_view) const override {
        return {{"essai", _bonus}};
    }
    void receiveItem(std::string_view, int) override {}

private:
    std::set<std::string> _langues;
    int _bonus;
};

core::DialogueGraph grapheDEssai() {
    const core::DialogueLoad lu = core::readDialogue(
        R"({"id":"garde","name":"Garde","source":"original",)"
        R"("speaker":{"languages":["common"],"attitude":"hostile"},"start":"halte","nodes":[)"
        R"({"id":"halte","type":"line","choices":[{"id":"negocier","next":"jet"},)"
        R"({"id":"partir","next":"fin"}]},)"
        R"({"id":"jet","type":"check","skill":"animal-handling","difficulty":"facile",)"
        R"("success":"passe","failure":"refus"},)"
        R"({"id":"passe","type":"line","attitude":"friendly","next":"fin"},)"
        R"({"id":"refus","type":"line","next":"halte"},)"
        R"({"id":"fin","type":"end"}]})",
        "garde.json");
    return *lu.graph;
}

core::DifficultyScale echelle() {
    core::DifficultyScale e;
    e.tiers.push_back({"facile", "Facile", 10});
    return e;
}

}  // namespace

/**
 * @brief Une réplique en attente s'affiche traduite, et une réponse qui mène à un jet l'annonce.
 * \castest{<b>L'ecran de dialogue annonce le jet avant le choix.</b><br/>
 * \tcat Unitaire · Interface<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ouvrir un dialogue de garde hostile.<br/>2. Lire les valeurs de l'ecran.<br/>
 * \tattendu Nom, attitude hostile et replique par leur cle ; deux reponses dans l'ordre ; la
 * premiere porte « [competence DD 10] » avec la cle de lexique a tiret bas et le seuil du degre
 * (LOT-117) ; aucune restitution de jet ; l'ecran n'est pas termine.
 * }
 */
TEST(DialogueScreenTest, UneRepliqueAnnonceLeJetDeSaReponse) {
    const core::DialogueGraph graphe = grapheDEssai();
    const core::DifficultyScale e = echelle();
    core::WorldFlags drapeaux;
    Auditeur auditeur({"common"}, 30);
    core::DeterministicRandom hasard(5);
    core::DialogueRunner runner(graphe, drapeaux, auditeur, e, hasard);
    ASSERT_EQ(runner.start(), core::DialogueState::AwaitingChoice);

    const hmi::DialogueScreenValues v = hmi::dialogueScreenValues(runner, cle);
    EXPECT_EQ(v.speakerName, "<dialogue.garde.speaker>");
    EXPECT_EQ(v.attitude, "<dialogue.attitude.hostile>");
    EXPECT_EQ(v.line, "<dialogue.garde.halte>");
    ASSERT_EQ(v.replies.size(), 2U);
    EXPECT_EQ(v.replies[0].id, "negocier");
    EXPECT_EQ(v.replies[0].label, "<dialogue.garde.halte.negocier>");
    EXPECT_EQ(v.replies[0].value, "[<rpg.skill.animal_handling> DD 10]");
    EXPECT_EQ(v.replies[1].value, "");
    EXPECT_TRUE(v.checkOutcome.empty());
    EXPECT_TRUE(v.checkTitle.empty());
    EXPECT_FALSE(v.finished);
}

/**
 * @brief Le jet joué par une réponse se restitue sur la réplique qui en découle, et seulement
 *        sur celle-là.
 * \castest{<b>Le jet se restitue sur la replique qui suit, puis s'efface.</b><br/>
 * \tcat Unitaire · Interface<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Negocier (bonus +30 contre 10).<br/>2. Lire les valeurs.<br/>3. Continuer jusqu'a la
 * fin.<br/>
 * \tattendu Apres le jet : attitude amicale ; restitution « competence DD 10 -- d20 de, total t
 * -- reussite », et par morceaux : titre « competence DD 10 », le de tire, le calcul
 * « de + 30 = t », l'issue ; une reponse « continuer ». A la fin : termine, restitution effacee,
 * une seule reponse « quitter ».
 * }
 */
TEST(DialogueScreenTest, LeJetSeRestitueSurLaRepliqueQuiSuit) {
    const core::DialogueGraph graphe = grapheDEssai();
    const core::DifficultyScale e = echelle();
    core::WorldFlags drapeaux;
    Auditeur auditeur({"common"}, 30);
    core::DeterministicRandom hasard(5);
    core::DialogueRunner runner(graphe, drapeaux, auditeur, e, hasard);
    ASSERT_EQ(runner.start(), core::DialogueState::AwaitingChoice);
    ASSERT_EQ(runner.choose("negocier"), core::ChoiceResult::Advanced);

    const hmi::DialogueScreenValues apres = hmi::dialogueScreenValues(runner, cle);
    EXPECT_EQ(apres.attitude, "<dialogue.attitude.friendly>");
    const std::string total = std::to_string(runner.lastCheck()->result.total);
    const std::string de = std::to_string(runner.lastCheck()->result.keptDie);
    EXPECT_EQ(apres.checkOutcome, "<rpg.skill.animal_handling> DD 10 -- d20 " + de + ", total " +
                                      total + " -- <dialogue.check.success>");
    EXPECT_EQ(apres.checkTitle, "<rpg.skill.animal_handling> DD 10");
    EXPECT_EQ(apres.checkDie, de);
    EXPECT_EQ(apres.checkDetail, de + " + 30 = " + total);
    EXPECT_EQ(apres.checkVerdict, "<dialogue.check.success>");
    EXPECT_TRUE(apres.checkSucceeded);
    ASSERT_EQ(apres.replies.size(), 1U);
    EXPECT_EQ(apres.replies[0].id, "continue");
    EXPECT_EQ(apres.replies[0].label, "<dialogue.continue>");

    ASSERT_EQ(runner.choose("continue"), core::ChoiceResult::Advanced);
    const hmi::DialogueScreenValues fin = hmi::dialogueScreenValues(runner, cle);
    EXPECT_TRUE(fin.finished);
    EXPECT_TRUE(fin.checkOutcome.empty());
    EXPECT_TRUE(fin.checkTitle.empty());
    ASSERT_EQ(fin.replies.size(), 1U);
    EXPECT_EQ(fin.replies[0].id, std::string(hmi::DIALOGUE_LEAVE_REPLY));
}

/**
 * @brief Un refus faute de langue commune montre le refus, et seulement « Quitter ».
 * \castest{<b>L'ecran montre le refus faute de langue commune.</b><br/>
 * \tcat Unitaire · Interface<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ouvrir le dialogue du garde avec un interlocuteur qui ne parle que l'elfique.<br/>
 * 2. Lire les valeurs.<br/>
 * \tattendu Replique = cle du refus ; une seule reponse « quitter » ; l'ecran ne se referme pas de
 * lui-meme, pour que le refus se lise.
 * }
 */
TEST(DialogueScreenTest, UnRefusMontreLeRefusEtQuitter) {
    const core::DialogueGraph graphe = grapheDEssai();
    const core::DifficultyScale e = echelle();
    core::WorldFlags drapeaux;
    Auditeur elfe({"elvish"}, 0);
    core::DeterministicRandom hasard(5);
    core::DialogueRunner runner(graphe, drapeaux, elfe, e, hasard);
    ASSERT_EQ(runner.start(), core::DialogueState::Refused);

    const hmi::DialogueScreenValues v = hmi::dialogueScreenValues(runner, cle);
    EXPECT_EQ(v.line, "<dialogue.refused>");
    EXPECT_FALSE(v.finished);
    ASSERT_EQ(v.replies.size(), 1U);
    EXPECT_EQ(v.replies[0].id, std::string(hmi::DIALOGUE_LEAVE_REPLY));
    EXPECT_EQ(v.replies[0].label, "<dialogue.leave>");
}

/**
 * @brief Un jet raté montre son dé et son calcul négatif ; la réponse qui y menait disparaît, et
 *        un jet raté atteint de nouveau se dit « déjà tenté », sans dé (`LOT-117`).
 * \castest{<b>L'ecran montre un echec, puis un jet deja tente sans de.</b><br/>
 * \tcat Unitaire · Interface<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Negocier avec -30 contre 10.<br/>2. Lire les valeurs.<br/>3. Continuer jusqu'a la
 * halte.<br/>4. Rouvrir le dialogue sur les memes drapeaux, sur un graphe ou la halte mene au jet
 * sans choix.<br/>
 * \tattendu Apres le jet : echec, calcul « de - 30 = t », de montre. A la halte : seule « partir »
 * reste. Sur le second graphe : titre du jet, de vide, detail « deja tente », restitution
 * « deja », echec.
 * }
 */
TEST(DialogueScreenTest, UnEchecSeMontreEtNeSeRetentePas) {
    const core::DialogueGraph graphe = grapheDEssai();
    const core::DifficultyScale e = echelle();
    core::WorldFlags drapeaux;
    Auditeur auditeur({"common"}, -30);
    core::DeterministicRandom hasard(5);
    core::DialogueRunner runner(graphe, drapeaux, auditeur, e, hasard);
    ASSERT_EQ(runner.start(), core::DialogueState::AwaitingChoice);
    ASSERT_EQ(runner.choose("negocier"), core::ChoiceResult::Advanced);

    const hmi::DialogueScreenValues rate = hmi::dialogueScreenValues(runner, cle);
    const std::string de = std::to_string(runner.lastCheck()->result.keptDie);
    const std::string total = std::to_string(runner.lastCheck()->result.total);
    EXPECT_FALSE(rate.checkSucceeded);
    EXPECT_EQ(rate.checkDie, de);
    EXPECT_EQ(rate.checkDetail, de + " - 30 = " + total);
    EXPECT_EQ(rate.checkVerdict, "<dialogue.check.failure>");

    ASSERT_EQ(runner.choose("continue"), core::ChoiceResult::Advanced);
    const hmi::DialogueScreenValues halte = hmi::dialogueScreenValues(runner, cle);
    ASSERT_EQ(halte.replies.size(), 1U);
    EXPECT_EQ(halte.replies[0].id, "partir") << "la reponse a jet ratee ne se propose plus";

    // Le meme jet, atteint sans choix : il echoue sans rouler, et l'ecran le dit.
    const core::DialogueLoad lu = core::readDialogue(
        R"({"id":"garde","name":"Garde","source":"original",)"
        R"("speaker":{"languages":["common"]},"start":"halte","nodes":[)"
        R"({"id":"halte","type":"line","next":"jet"},)"
        R"({"id":"jet","type":"check","skill":"animal-handling","difficulty":"facile",)"
        R"("success":"fin","failure":"refus"},)"
        R"({"id":"refus","type":"line","next":"fin"},)"
        R"({"id":"fin","type":"end"}]})",
        "garde.json");
    ASSERT_TRUE(lu.graph.has_value()) << lu.errors.front();
    core::DialogueRunner seconde(*lu.graph, drapeaux, auditeur, e, hasard);
    ASSERT_EQ(seconde.start(), core::DialogueState::AwaitingChoice);
    ASSERT_EQ(seconde.choose("continue"), core::ChoiceResult::Advanced);
    const hmi::DialogueScreenValues deja = hmi::dialogueScreenValues(seconde, cle);
    EXPECT_EQ(deja.checkTitle, "<rpg.skill.animal_handling> DD 10");
    EXPECT_TRUE(deja.checkDie.empty());
    EXPECT_EQ(deja.checkDetail, "<dialogue.check.already-failed>");
    EXPECT_EQ(deja.checkOutcome,
              "<rpg.skill.animal_handling> DD 10 -- deja -- <dialogue.check.failure>");
    EXPECT_FALSE(deja.checkSucceeded);
}
