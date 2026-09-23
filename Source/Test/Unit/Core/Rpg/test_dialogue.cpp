// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_dialogue.cpp
 * @brief Tests unitaires des dialogues (`LOT-15`) : chargement et refus des graphes mal formés,
 *        parcours headless, jets de compétence, refus faute de langue commune, traduction.
 */

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Gameplay/MapEntitySpawner.h"
#include "Core/Gameplay/WorldFlags.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Math/DeterministicRandom.h"
#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Check.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/Rpg/Inventory.h"
#include "Core/Rpg/Skill.h"
#include "HMI/Localization/Localization.h"

namespace {

const std::filesystem::path RPG{JADG_RPG_DIR};
const std::filesystem::path WORLD{JADG_WORLD_DIR};

const std::string HERAUT = "heraut-colisee";
const std::string RENCONTRE = "dialogue/heraut-colisee/rencontre";
const std::string ECHEC = "dialogue/heraut-colisee/persuasion-echouee";

[[nodiscard]] const core::DifficultyScale& echelle() {
    static const core::DifficultyScale lue =
        core::loadDifficultyScale(RPG / "rules" / "difficulty.json");
    return lue;
}

[[nodiscard]] const core::DialogueCatalog& dialogues() {
    static const core::DialogueCatalog lus = core::loadDialogues(WORLD / "dialogues");
    return lus;
}

/// Un interlocuteur d'essai : ses langues, un modificateur fixe par jet, et ce qu'il a recu.
class Auditeur final : public core::DialogueListener {
public:
    Auditeur(std::set<std::string> langues, int modificateur)
        : _langues(std::move(langues)), _modificateur(modificateur) {}

    [[nodiscard]] bool speaks(std::string_view langue) const override {
        return _langues.contains(std::string(langue));
    }
    [[nodiscard]] std::vector<core::Modifier> skillModifiers(std::string_view) const override {
        return {{"essai", _modificateur}};
    }
    void receiveItem(std::string_view objet, int quantite) override {
        recus.emplace_back(std::string(objet), quantite);
    }

    std::vector<std::pair<std::string, int>> recus;

private:
    std::set<std::string> _langues;
    int _modificateur;
};

[[nodiscard]] std::vector<std::string> identifiants(const std::vector<core::AvailableChoice>& c) {
    std::vector<std::string> ids;
    for (const core::AvailableChoice& choix : c) {
        ids.push_back(choix.id);
    }
    return ids;
}

/// Les noeuds traverses, lus dans le journal (« replique : x », « jet : x (competence) »…).
[[nodiscard]] std::set<std::string> noeudsTraverses(const std::vector<std::string>& journal) {
    std::set<std::string> noeuds;
    for (const std::string& ligne : journal) {
        for (const std::string_view prefixe :
             {"replique : ", "condition : ", "action : ", "jet : ", "fin : "}) {
            if (ligne.starts_with(prefixe)) {
                std::string reste = ligne.substr(prefixe.size());
                noeuds.insert(reste.substr(0, reste.find(' ')));
            }
        }
    }
    return noeuds;
}

/// Un graphe minimal valide, a completer : une replique qui mene a la fin.
[[nodiscard]] std::string graphe(const std::string& noeuds, const std::string& entree = "a") {
    return R"({"id":"essai","name":"Essai","source":"original",)"
           R"("speaker":{"languages":["common"]},"start":")" +
           entree + R"(","nodes":[)" + noeuds + "]}";
}

[[nodiscard]] bool contient(const std::vector<std::string>& erreurs, std::string_view morceau) {
    return std::ranges::any_of(
        erreurs, [morceau](const std::string& e) { return e.find(morceau) != std::string::npos; });
}

[[nodiscard]] std::unordered_map<std::string, std::string> catalogue(const char* chemin) {
    std::ifstream flux{std::filesystem::path(chemin), std::ios::binary};
    std::stringstream contenu;
    contenu << flux.rdbuf();
    return hmi::Localization::parseCatalog(contenu.str());
}

}  // namespace

/**
 * @brief Le dialogue de démonstration se charge, dix nœuds au moins, deux conditions et un jet de
 *        Persuasion, et tout ce qu'il nomme existe dans les catalogues.
 * \castest{<b>Le dialogue du heraut se charge et ses references existent.</b><br/>
 * \tcat Unitaire · Dialogue<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Charger Source/Elements/World/dialogues.<br/>2. Charger competences, degres de
 * difficulte, objets et langues.<br/>3. Valider les references du heraut.<br/>
 * \tattendu Aucune erreur ; au moins dix noeuds, deux conditions et un jet de persuasion ; aucune
 * reference inconnue.
 * }
 */
TEST(DialogueTest, LeDialogueDeDemonstrationSeChargeEtSesReferencesExistent) {
    ASSERT_TRUE(dialogues().errors.empty()) << dialogues().errors.front();
    ASSERT_TRUE(echelle().errors.empty()) << echelle().errors.front();
    const core::DialogueGraph* heraut = dialogues().find(HERAUT);
    ASSERT_NE(heraut, nullptr);

    EXPECT_GE(heraut->nodes.size(), 10U);
    EXPECT_GE(std::ranges::count(heraut->nodes, core::DialogueNodeKind::Condition,
                                 &core::DialogueNode::kind),
              2);
    EXPECT_TRUE(std::ranges::any_of(heraut->nodes, [](const core::DialogueNode& n) {
        return n.kind == core::DialogueNodeKind::Check && n.skill == "persuasion";
    }));

    const core::SkillCatalog competences = core::loadSkills(RPG / "skills");
    const core::ItemCatalog objets = core::loadItems(RPG / "items");
    core::DialogueReferences references;
    references.skills = &competences;
    references.difficulty = &echelle();
    references.itemExists = [&objets](std::string_view id) { return objets.find(id) != nullptr; };
    references.languageExists = [](std::string_view id) {
        return std::filesystem::exists(RPG / "languages" / (std::string(id) + ".json"));
    };
    for (const core::DialogueGraph& dialogue : dialogues().dialogues) {
        const std::vector<std::string> erreurs =
            core::validateDialogueReferences(dialogue, references);
        EXPECT_TRUE(erreurs.empty()) << erreurs.front();
    }
}

/**
 * @brief Critère d'acceptation : le dialogue de dix nœuds, deux conditions et un jet de Persuasion
 *        se parcourt **en headless**, réussite puis retour, les drapeaux orientant la seconde
 *        conversation.
 * \castest{<b>Le dialogue du heraut se parcourt sans fenetre.</b><br/>
 * \tcat Unitaire · Dialogue<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Ouvrir la conversation avec un interlocuteur parlant le commun, +20 aux jets.<br/>
 * 2. Demander la Marque, continuer, demander l'inscription, convaincre.<br/>3. Continuer jusqu'a la
 * fin.<br/>4. Rouvrir une seconde conversation sur les memes drapeaux, demander l'inscription.<br/>
 * \tattendu Premiere conversation : presentation, marque, retour, demande, jet reussi contre 15,
 * quete demarree, fin. Seconde : retour (condition de rencontre), deja inscrit (condition de
 * quete), fin. Au moins dix noeuds distincts traverses.
 * }
 */
TEST(DialogueTest, LeDialogueDuHerautSeParcourtEnHeadless) {
    const core::DialogueGraph* heraut = dialogues().find(HERAUT);
    ASSERT_NE(heraut, nullptr);
    core::WorldFlags drapeaux;
    Auditeur brenna({"common", "elvish"}, 20);
    core::DeterministicRandom hasard(2026);

    core::DialogueRunner premiere(*heraut, drapeaux, brenna, echelle(), hasard);
    ASSERT_EQ(premiere.start(), core::DialogueState::AwaitingChoice);
    EXPECT_EQ(premiere.lineKey(), "dialogue.heraut-colisee.presentation");
    EXPECT_TRUE(drapeaux.isSet(RENCONTRE)) << "l'action de premiere rencontre a pose son drapeau";
    EXPECT_EQ(identifiants(premiere.choices()),
              (std::vector<std::string>{"marque", "inscription", "partir"}));
    EXPECT_EQ(premiere.attitude(), core::DialogueAttitude::Indifferent);

    ASSERT_EQ(premiere.choose("marque"), core::ChoiceResult::Advanced);
    EXPECT_EQ(premiere.lineKey(), "dialogue.heraut-colisee.marque");
    ASSERT_EQ(identifiants(premiere.choices()), (std::vector<std::string>{"continue"}));
    EXPECT_EQ(premiere.choices().front().textKey, "dialogue.continue");
    ASSERT_EQ(premiere.choose("continue"), core::ChoiceResult::Advanced);
    EXPECT_EQ(premiere.lineKey(), "dialogue.heraut-colisee.retour");

    ASSERT_EQ(premiere.choose("inscription"), core::ChoiceResult::Advanced);
    EXPECT_EQ(premiere.lineKey(), "dialogue.heraut-colisee.demande");
    const std::vector<core::AvailableChoice> demande = premiere.choices();
    ASSERT_EQ(identifiants(demande), (std::vector<std::string>{"convaincre", "renoncer"}));
    EXPECT_EQ(demande.front().checkSkill, "persuasion") << "l'ecran annonce le jet avant le choix";

    ASSERT_EQ(premiere.choose("convaincre"), core::ChoiceResult::Advanced);
    ASSERT_TRUE(premiere.lastCheck().has_value());
    EXPECT_EQ(premiere.lastCheck()->skill, "persuasion");
    EXPECT_EQ(premiere.lastCheck()->result.target, 15) << "« moyenne », lue dans difficulty.json";
    EXPECT_TRUE(premiere.lastCheck()->result.succeeded());
    EXPECT_TRUE(drapeaux.isSet(core::questStartedFlag("champion-du-colisee")));
    EXPECT_EQ(premiere.lineKey(), "dialogue.heraut-colisee.accepte-replique");
    EXPECT_EQ(premiere.attitude(), core::DialogueAttitude::Friendly);

    ASSERT_EQ(premiere.choose("continue"), core::ChoiceResult::Advanced);
    EXPECT_EQ(premiere.state(), core::DialogueState::Ended);
    EXPECT_TRUE(premiere.choices().empty());
    EXPECT_EQ(premiere.choose("continue"), core::ChoiceResult::NotAwaiting);

    core::DialogueRunner seconde(*heraut, drapeaux, brenna, echelle(), hasard);
    ASSERT_EQ(seconde.start(), core::DialogueState::AwaitingChoice);
    EXPECT_EQ(seconde.lineKey(), "dialogue.heraut-colisee.retour") << "condition de rencontre";
    ASSERT_EQ(seconde.choose("inscription"), core::ChoiceResult::Advanced);
    EXPECT_EQ(seconde.lineKey(), "dialogue.heraut-colisee.deja-inscrit") << "condition de quete";
    ASSERT_EQ(seconde.choose("continue"), core::ChoiceResult::Advanced);
    EXPECT_EQ(seconde.state(), core::DialogueState::Ended);

    std::set<std::string> traverses = noeudsTraverses(premiere.journal());
    traverses.merge(noeudsTraverses(seconde.journal()));
    EXPECT_GE(traverses.size(), 10U) << "dix noeuds au moins, conditions, actions et jet compris";
    EXPECT_TRUE(contient(premiere.journal(), "jet : jet-persuasion (persuasion)"));
    EXPECT_TRUE(contient(premiere.journal(), "quete demarree : champion-du-colisee"));
}

/**
 * @brief Un jet raté mène à l'autre suite, pose son drapeau, et la réponse conditionnelle qui
 *        permettait de retenter disparaît — et se refuse même demandée directement.
 * \castest{<b>Un echec de Persuasion ferme la reponse de retentative.</b><br/>
 * \tcat Unitaire · Dialogue<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Parcourir jusqu'a la demande avec -20 aux jets.<br/>2. Convaincre.<br/>
 * 3. Continuer, redemander l'inscription.<br/>4. Choisir « convaincre » malgre tout.<br/>
 * \tattendu Le jet echoue, la replique hostile s'affiche, le drapeau d'echec est pose, la quete ne
 * l'est pas ; la demande ne propose plus que « renoncer », et « convaincre » est refuse.
 * }
 */
TEST(DialogueTest, UnEchecMeneALAutreSuiteEtFermeLaReponseConditionnelle) {
    const core::DialogueGraph* heraut = dialogues().find(HERAUT);
    ASSERT_NE(heraut, nullptr);
    core::WorldFlags drapeaux;
    Auditeur maladroit({"common"}, -20);
    core::DeterministicRandom hasard(7);
    core::DialogueRunner runner(*heraut, drapeaux, maladroit, echelle(), hasard);

    ASSERT_EQ(runner.start(), core::DialogueState::AwaitingChoice);
    ASSERT_EQ(runner.choose("inscription"), core::ChoiceResult::Advanced);
    ASSERT_EQ(runner.choose("convaincre"), core::ChoiceResult::Advanced);
    ASSERT_TRUE(runner.lastCheck().has_value());
    EXPECT_FALSE(runner.lastCheck()->result.succeeded());
    EXPECT_EQ(runner.lineKey(), "dialogue.heraut-colisee.refuse-replique");
    EXPECT_EQ(runner.attitude(), core::DialogueAttitude::Hostile);
    EXPECT_TRUE(drapeaux.isSet(ECHEC));
    EXPECT_FALSE(drapeaux.isSet(core::questStartedFlag("champion-du-colisee")));

    ASSERT_EQ(runner.choose("continue"), core::ChoiceResult::Advanced);
    ASSERT_EQ(runner.choose("inscription"), core::ChoiceResult::Advanced);
    EXPECT_EQ(identifiants(runner.choices()), (std::vector<std::string>{"renoncer"}));
    EXPECT_EQ(runner.choose("convaincre"), core::ChoiceResult::Unavailable);
    EXPECT_EQ(runner.choose("inexistante"), core::ChoiceResult::Unavailable);
    EXPECT_EQ(runner.lineKey(), "dialogue.heraut-colisee.demande") << "un refus n'avance rien";
}

/**
 * @brief `EX-RPG-042` : sans langue commune, le dialogue est **refusé** et rien n'est joué.
 * \castest{<b>Un dialogue est refuse faute de langue commune.</b><br/>
 * \tcat Unitaire · Dialogue<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ouvrir le heraut (commun) avec un interlocuteur qui ne parle que le nain.<br/>
 * 2. Tenter une reponse.<br/>
 * \tattendu Etat Refused ; aucun drapeau pose ; aucune replique ni reponse ; une reponse rend
 * NotAwaiting ; le journal nomme la langue manquante.
 * }
 */
TEST(DialogueTest, UnDialogueEstRefuseFauteDeLangueCommune) {
    const core::DialogueGraph* heraut = dialogues().find(HERAUT);
    ASSERT_NE(heraut, nullptr);
    core::WorldFlags drapeaux;
    Auditeur nain({"dwarvish"}, 0);
    core::DeterministicRandom hasard(1);
    core::DialogueRunner runner(*heraut, drapeaux, nain, echelle(), hasard);

    EXPECT_EQ(runner.start(), core::DialogueState::Refused);
    EXPECT_EQ(drapeaux.size(), 0U) << "une conversation qui n'a pas lieu ne pose rien";
    EXPECT_EQ(runner.currentLine(), nullptr);
    EXPECT_TRUE(runner.choices().empty());
    EXPECT_EQ(runner.choose("marque"), core::ChoiceResult::NotAwaiting);
    EXPECT_TRUE(contient(runner.journal(), "aucune langue commune (common)"));
    EXPECT_EQ(runner.start(), core::DialogueState::Refused) << "rouvrir ne change pas le refus";
}

/**
 * @brief Critère d'acceptation : un graphe mal formé est **rejeté au chargement**, avec un message
 *        qui nomme le fichier, le nœud et la faute.
 * \castest{<b>Les graphes mal formes sont refuses au chargement.</b><br/>
 * \tcat Unitaire · Dialogue<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire une serie de graphes fautifs : cible inconnue, entree inconnue, choix vide,
 * reponse sans identifiant, reponses toutes conditionnelles, cycle sans reponse, monologue en
 * boucle, orphelin, impasse, difficulte chiffree, noeud en double, type inconnu, sans fin, sans
 * langue.<br/>
 * \tattendu Aucun graphe produit ; chaque message contient l'origine et le motif attendu.
 * }
 */
TEST(DialogueTest, UnGrapheMalFormeEstRejeteAuChargement) {
    const std::string fin = R"({"id":"fin","type":"end"})";
    struct Cas {
        std::string json;
        std::string motif;
    };
    const std::vector<Cas> cas = {
        {graphe(R"({"id":"a","type":"line","next":"nulle-part"},)" + fin),
         "noeud 'a' : noeud cible inconnu : 'nulle-part'"},
        {graphe(fin, "absente"), "l'entree 'start' nomme 'absente'"},
        {graphe(R"({"id":"a","type":"line","choices":[]},)" + fin), "choix vide"},
        {graphe(R"({"id":"a","type":"line","choices":[{"next":"fin"}]},)" + fin),
         "choix vide : une reponse n'a pas d'identifiant"},
        {graphe(R"({"id":"a","type":"line","choices":[{"id":"x","next":"fin",)"
                R"("condition":{"flag":"f"}}]},)" +
                fin),
         "toutes les reponses sont conditionnelles"},
        {graphe(R"({"id":"a","type":"condition","flag":"f","then":"b","else":"fin"},)"
                R"({"id":"b","type":"action","actions":[{"type":"setFlag","flag":"g"}],)"
                R"("next":"a"},)" +
                fin),
         "cycle non intentionnel"},
        {graphe(
             R"({"id":"a","type":"line","next":"b"},{"id":"b","type":"line","next":"a"},)"
             R"({"id":"c","type":"line","choices":[{"id":"x","next":"a"},{"id":"y","next":"fin"}]},)" +
                 fin,
             "c"),
         "cycle non intentionnel"},
        {graphe(
             R"({"id":"a","type":"line","next":"fin"},{"id":"perdu","type":"line","next":"fin"},)" +
             fin),
         "noeud 'perdu' : orphelin"},
        {graphe(
             R"({"id":"a","type":"line","choices":[{"id":"x","next":"b"},{"id":"y","next":"fin"}]},)"
             R"({"id":"b","type":"line","choices":[{"id":"z","next":"b"}]},)" +
             fin),
         "noeud 'b' : impasse"},
        {graphe(R"({"id":"a","type":"check","skill":"persuasion","difficulty":15,)"
                R"("success":"fin","failure":"fin"},)" +
                fin),
         "jamais un nombre"},
        {graphe(R"({"id":"a","type":"line","next":"fin"},{"id":"a","type":"end"},)" + fin),
         "identifiant en double"},
        {graphe(R"({"id":"a","type":"monologue"},)" + fin), "type inconnu"},
        {graphe(R"({"id":"a","type":"line","next":"a"})"), "aucun noeud 'end'"},
        {R"({"id":"essai","name":"E","source":"original","speaker":{"languages":[]},)"
         R"("start":"fin","nodes":[{"id":"fin","type":"end"}]})",
         "un PNJ parle au moins une langue"},
    };
    for (const Cas& un : cas) {
        const core::DialogueLoad lu = core::readDialogue(un.json, "essai.json");
        EXPECT_FALSE(lu.graph.has_value()) << un.motif;
        EXPECT_TRUE(contient(lu.errors, un.motif))
            << "motif attendu : " << un.motif
            << (lu.errors.empty() ? std::string("\n(aucune erreur)")
                                  : "\npremiere : " + lu.errors.front());
        EXPECT_TRUE(contient(lu.errors, "essai.json")) << "le message nomme son fichier";
    }
}

/**
 * @brief Une boucle qui revient à une réplique à réponses est un « hub » voulu, et se charge.
 * \castest{<b>Une boucle par une replique a reponses est acceptee.</b><br/>
 * \tcat Unitaire · Dialogue<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire un graphe ou une question revient au menu par une replique et une action.<br/>
 * 2. Le jouer trois tours de boucle, puis partir.<br/>
 * \tattendu Aucune erreur ; la boucle se joue autant de fois que demande et la conversation se
 * termine.
 * }
 */
TEST(DialogueTest, UneBouclePasseeParUnChoixEstUnHubVoulu) {
    const core::DialogueLoad lu = core::readDialogue(
        graphe(R"({"id":"menu","type":"line","choices":[{"id":"encore","next":"note"},)"
               R"({"id":"partir","next":"fin"}]},)"
               R"({"id":"note","type":"action","actions":[{"type":"setFlag","flag":"vu"}],)"
               R"("next":"reponse"},)"
               R"({"id":"reponse","type":"line","next":"menu"},{"id":"fin","type":"end"})",
               "menu"),
        "hub.json");
    ASSERT_TRUE(lu.graph.has_value()) << lu.errors.front();

    core::WorldFlags drapeaux;
    Auditeur quiconque({"common"}, 0);
    core::DeterministicRandom hasard(3);
    core::DialogueRunner runner(*lu.graph, drapeaux, quiconque, echelle(), hasard);
    ASSERT_EQ(runner.start(), core::DialogueState::AwaitingChoice);
    for (int tour = 0; tour < 3; ++tour) {
        ASSERT_EQ(runner.choose("encore"), core::ChoiceResult::Advanced);
        ASSERT_EQ(runner.choose("continue"), core::ChoiceResult::Advanced);
        EXPECT_EQ(runner.lineKey(), "dialogue.essai.menu");
    }
    ASSERT_EQ(runner.choose("partir"), core::ChoiceResult::Advanced);
    EXPECT_EQ(runner.state(), core::DialogueState::Ended);
    EXPECT_TRUE(drapeaux.isSet("vu"));
}

/**
 * @brief Les actions touchent le monde : donner un objet, retirer un drapeau, démarrer une quête.
 * \castest{<b>Les quatre actions d'un dialogue s'appliquent.</b><br/>
 * \tcat Unitaire · Dialogue<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Jouer un noeud qui pose puis retire un drapeau, donne trois torches et demarre une
 * quete.<br/>
 * \tattendu Le drapeau retire n'est plus leve, l'interlocuteur a recu trois torches, le drapeau de
 * quete est pose, et le journal liste les quatre effets dans l'ordre.
 * }
 */
TEST(DialogueTest, LesActionsTouchentLeMonde) {
    const core::DialogueLoad lu = core::readDialogue(
        graphe(R"({"id":"a","type":"action","actions":[)"
               R"({"type":"setFlag","flag":"porte"},{"type":"clearFlag","flag":"porte"},)"
               R"({"type":"giveItem","item":"torche","quantity":3},)"
               R"({"type":"startQuest","quest":"le-puits"}],"next":"fin"},)"
               R"({"id":"fin","type":"end"})"),
        "actions.json");
    ASSERT_TRUE(lu.graph.has_value()) << lu.errors.front();
    core::WorldFlags drapeaux;
    Auditeur receveur({"common"}, 0);
    core::DeterministicRandom hasard(3);
    core::DialogueRunner runner(*lu.graph, drapeaux, receveur, echelle(), hasard);

    EXPECT_EQ(runner.start(), core::DialogueState::Ended);
    EXPECT_FALSE(drapeaux.isSet("porte"));
    EXPECT_TRUE(drapeaux.isSet("quest/le-puits/started"));
    ASSERT_EQ(receveur.recus.size(), 1U);
    EXPECT_EQ(receveur.recus.front(), (std::pair<std::string, int>{"torche", 3}));
    const std::vector<std::string>& journal = runner.journal();
    const auto pose = std::ranges::find(journal, "drapeau pose : porte");
    const auto retire = std::ranges::find(journal, "drapeau retire : porte");
    ASSERT_NE(pose, journal.end());
    ASSERT_NE(retire, journal.end());
    EXPECT_LT(pose, retire);
}

/**
 * @brief À graine égale, les mêmes réponses donnent les mêmes jets et le même journal.
 * \castest{<b>Un dialogue se rejoue a l'identique a graine fixee.</b><br/>
 * \tcat Unitaire · Dialogue<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Jouer le heraut jusqu'au jet, sans bonus, a la graine 42, deux fois.<br/>2. Le jouer
 * sur une plage de graines.<br/>
 * \tattendu Les deux journaux sont identiques ; sur la plage, le jet reussit au moins une fois et
 * echoue au moins une fois -- le de compte.
 * }
 */
TEST(DialogueTest, UnDialogueSeRejoueAGraineFixee) {
    const core::DialogueGraph* heraut = dialogues().find(HERAUT);
    ASSERT_NE(heraut, nullptr);
    const auto jouer = [heraut](std::uint64_t graine) {
        core::WorldFlags drapeaux;
        Auditeur neutre({"common"}, 0);
        core::DeterministicRandom hasard(graine);
        core::DialogueRunner runner(*heraut, drapeaux, neutre, echelle(), hasard);
        static_cast<void>(runner.start());
        static_cast<void>(runner.choose("inscription"));
        static_cast<void>(runner.choose("convaincre"));
        return std::make_pair(runner.journal(),
                              runner.lastCheck() && runner.lastCheck()->result.succeeded());
    };
    EXPECT_EQ(jouer(42).first, jouer(42).first);

    bool reussi = false;
    bool rate = false;
    for (std::uint64_t graine = 1; graine <= 40; ++graine) {
        (jouer(graine).second ? reussi : rate) = true;
    }
    EXPECT_TRUE(reussi);
    EXPECT_TRUE(rate);
}

/**
 * @brief Critère d'acceptation : traduction fr/en complète — chaque clé qu'un dialogue réclame, et
 *        celles de l'écran, existent dans les deux catalogues.
 * \castest{<b>Les dialogues sont traduits en francais et en anglais.</b><br/>
 * \tcat Unitaire · Dialogue<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire fr.lang et en.lang.<br/>2. Pour chaque dialogue livre, fabriquer ses cles.<br/>
 * 3. Ajouter les cles de l'ecran (quitter, refus, jet).<br/>
 * \tattendu Toutes les cles existent dans les deux catalogues, avec une valeur non vide.
 * }
 */
TEST(DialogueTest, LesDialoguesSontTraduitsEnFrancaisEtEnAnglais) {
    const auto fr = catalogue(JADG_FR_LANG_PATH);
    const auto en = catalogue(JADG_EN_LANG_PATH);
    ASSERT_FALSE(dialogues().dialogues.empty());

    std::vector<std::string> cles = {"dialogue.leave",
                                     "dialogue.refused",
                                     "dialogue.unavailable",
                                     "dialogue.check.success",
                                     "dialogue.check.failure",
                                     "dialogue.check.summary",
                                     "dialogue.attitude.friendly",
                                     "dialogue.attitude.indifferent",
                                     "dialogue.attitude.hostile"};
    for (const core::DialogueGraph& dialogue : dialogues().dialogues) {
        const std::vector<std::string> fabriquees = core::dialogueTextKeys(dialogue);
        cles.insert(cles.end(), fabriquees.begin(), fabriquees.end());
    }
    for (const std::string& cle : cles) {
        for (const auto* langue : {&fr, &en}) {
            const auto trouve = langue->find(cle);
            EXPECT_TRUE(trouve != langue->end() && !trouve->second.empty())
                << "cle absente de " << (langue == &fr ? "fr.lang" : "en.lang") << " : " << cle;
        }
    }
}

/**
 * @brief L'échelle des degrés de difficulté se lit de la donnée, et un fichier absent le dit.
 * \castest{<b>Les degres de difficulte se chargent.</b><br/>
 * \tcat Unitaire · Jet de d20<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Charger rules/difficulty.json.<br/>2. Charger un chemin inexistant.<br/>
 * \tattendu Six degres, « moyenne » a 15 et « quasi-impossible » a 30 ; le chemin absent rend une
 * erreur nommee et aucun degre.
 * }
 */
TEST(DialogueTest, LesDegresDeDifficulteSeChargent) {
    ASSERT_TRUE(echelle().errors.empty());
    EXPECT_EQ(echelle().tiers.size(), 6U);
    ASSERT_NE(echelle().find("moyenne"), nullptr);
    EXPECT_EQ(echelle().find("moyenne")->dc, 15);
    ASSERT_NE(echelle().find("quasi-impossible"), nullptr);
    EXPECT_EQ(echelle().find("quasi-impossible")->dc, 30);
    EXPECT_EQ(echelle().find("inventee"), nullptr);

    const core::DifficultyScale absente = core::loadDifficultyScale(RPG / "rules" / "absent.json");
    EXPECT_TRUE(absente.tiers.empty());
    EXPECT_FALSE(absente.errors.empty());
}

/**
 * @brief Le personnage de démonstration parle les langues de son espèce et celle qu'il a choisie,
 *        et ses jets de compétence détaillent caractéristique et maîtrise.
 * \castest{<b>La fiche ecoute un PNJ : langues et modificateurs.</b><br/>
 * \tcat Unitaire · Dialogue<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger la fiche de demonstration.<br/>2. L'envelopper dans CharacterListener.<br/>
 * 3. Interroger ses langues, ses modificateurs de Persuasion et d'Athletisme.<br/>4. Lui donner un
 * objet.<br/>
 * \tattendu Commun et orc (espece) et draconique (choisi) ; pas l'elfique. Persuasion : un seul
 * modificateur, de Charisme ; Athletisme : Force puis maitrise. L'objet arrive dans le sac.
 * }
 */
TEST(DialogueTest, LaFicheEcouteUnPnjAvecSesLanguesEtSesModificateurs) {
    const core::CharacterOptions options =
        core::loadCharacterOptions(RPG / "species", RPG / "backgrounds", RPG / "classes");
    const core::SkillCatalog competences = core::loadSkills(RPG / "skills");
    const core::ExperienceTable experience =
        core::loadExperienceTable(RPG / "rules" / "experience.json");
    const core::CharacterCreationRules regles =
        core::loadCharacterCreationRules(RPG / "rules" / "character-creation.json");
    core::LoadedCharacterSheet heros = core::loadCharacterSheet(
        RPG / "characters" / "heros-brawler.json", options, regles, experience);
    ASSERT_TRUE(heros.errors.empty()) << heros.errors.front();

    core::CharacterListener auditeur(heros.sheet, heros.inventory, experience, competences);
    EXPECT_TRUE(auditeur.speaks("common"));
    EXPECT_TRUE(auditeur.speaks("orc"));
    EXPECT_TRUE(auditeur.speaks("draconic")) << "langue choisie par la fiche";
    EXPECT_FALSE(auditeur.speaks("elvish"));

    const std::vector<core::Modifier> persuasion = auditeur.skillModifiers("persuasion");
    ASSERT_EQ(persuasion.size(), 1U);
    EXPECT_EQ(persuasion.front().source, "charisma");
    EXPECT_EQ(persuasion.front().value, heros.sheet.modifier(core::Ability::Charisma));

    const std::vector<core::Modifier> athletisme = auditeur.skillModifiers("athletics");
    ASSERT_EQ(athletisme.size(), 2U);
    EXPECT_EQ(athletisme[1].source, "maitrise");
    EXPECT_EQ(athletisme[0].value + athletisme[1].value,
              core::skillModifier(heros.sheet, experience, competences, "athletics").value);
    EXPECT_TRUE(auditeur.skillModifiers("inexistante").empty());

    auditeur.receiveItem("corde-en-soie-15-m", 1);
    EXPECT_TRUE(std::ranges::any_of(heros.inventory.backpack, [](const core::InventoryStack& s) {
        return s.itemId == "corde-en-soie-15-m";
    }));
}

/**
 * @brief Un PNJ posé sur la carte est une cible d'interaction qui se reparle, et nomme son
 *        dialogue.
 * \castest{<b>Un PNJ de carte ouvre son dialogue.</b><br/>
 * \tcat Unitaire · Dialogue<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Chercher la famille « npc » dans la table des interactifs.<br/>2. Lire trois entites
 * : un PNJ avec dialogue, un PNJ muet, un coffre.<br/>
 * \tattendu La famille existe et n'est pas consommable ; seul le premier donne un declencheur, avec
 * son dialogue et sa case.
 * }
 */
TEST(DialogueTest, UnPnjDeCarteOuvreSonDialogue) {
    const auto famille = std::ranges::find(core::knownInteractableKinds(), core::NPC_ENTITY_TYPE,
                                           &core::InteractableKind::type);
    ASSERT_NE(famille, core::knownInteractableKinds().end());
    EXPECT_FALSE(famille->consumable) << "un PNJ se reparle";

    core::MapEntity heraut;
    heraut.type = std::string(core::NPC_ENTITY_TYPE);
    heraut.position = {4, 2};
    heraut.properties[std::string(core::NPC_DIALOGUE_PROPERTY)] = HERAUT;
    const auto declencheur = core::dialogueTriggerFor(heraut);
    ASSERT_TRUE(declencheur.has_value());
    EXPECT_EQ(declencheur->dialogueId, HERAUT);
    EXPECT_EQ(declencheur->position, (core::GridPosition{4, 2}));

    core::MapEntity muet;
    muet.type = std::string(core::NPC_ENTITY_TYPE);
    EXPECT_FALSE(core::dialogueTriggerFor(muet).has_value());

    core::MapEntity coffre;
    coffre.type = "chest";
    coffre.properties[std::string(core::NPC_DIALOGUE_PROPERTY)] = HERAUT;
    EXPECT_FALSE(core::dialogueTriggerFor(coffre).has_value());
}
