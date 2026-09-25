// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Rpg/Dialogue.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <set>
#include <system_error>
#include <utility>
#include <variant>

#include "Core/Data/JsonDocument.h"
#include "Core/Gameplay/WorldFlags.h"
#include "Core/Rpg/Ability.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/Inventory.h"
#include "Core/Rpg/Skill.h"

namespace core {

namespace {

// Un dialogue est une donnee, pas un document de format : pas de champ `version`.
constexpr int SANS_GARDE_DE_VERSION = 0;

using Json = nlohmann::json;

// Rassemble les erreurs d'un document, chacune prefixee de son origine et de son noeud.
class Rapport {
public:
    explicit Rapport(std::string_view origine) : _origine(origine) {}

    void document(const std::string& message) {
        _erreurs.push_back(_origine + " : " + message);
    }
    void noeud(std::string_view noeud, const std::string& message) {
        _erreurs.push_back(_origine + " : noeud '" + std::string(noeud) + "' : " + message);
    }
    [[nodiscard]] bool vide() const noexcept {
        return _erreurs.empty();
    }
    [[nodiscard]] std::vector<std::string> extraire() {
        return std::move(_erreurs);
    }

private:
    std::string _origine;
    std::vector<std::string> _erreurs;
};

[[nodiscard]] std::optional<std::string> texte(const Json& objet, const char* champ) {
    const auto trouve = objet.find(champ);
    if (trouve == objet.end() || !trouve->is_string() || trouve->get<std::string>().empty()) {
        return std::nullopt;
    }
    return trouve->get<std::string>();
}

[[nodiscard]] std::optional<DialogueAttitude> attitudeDepuis(std::string_view mot) {
    if (mot == "friendly") {
        return DialogueAttitude::Friendly;
    }
    if (mot == "indifferent") {
        return DialogueAttitude::Indifferent;
    }
    if (mot == "hostile") {
        return DialogueAttitude::Hostile;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<DialogueNodeKind> natureDepuis(std::string_view mot) {
    if (mot == "line") {
        return DialogueNodeKind::Line;
    }
    if (mot == "condition") {
        return DialogueNodeKind::Condition;
    }
    if (mot == "action") {
        return DialogueNodeKind::Action;
    }
    if (mot == "check") {
        return DialogueNodeKind::Check;
    }
    if (mot == "end") {
        return DialogueNodeKind::End;
    }
    return std::nullopt;
}

// Exige un champ texte, et le nomme s'il manque.
[[nodiscard]] std::string exiger(const Json& objet, const char* champ, std::string_view noeud,
                                 Rapport& rapport) {
    auto valeur = texte(objet, champ);
    if (!valeur) {
        rapport.noeud(noeud, std::string("champ '") + champ + "' absent ou vide.");
        return {};
    }
    return std::move(*valeur);
}

// Une reponse d'une replique, ou rien si elle est inutilisable (erreur consignee).
[[nodiscard]] std::optional<DialogueChoice> lireReponse(const Json& reponse,
                                                        const std::string& noeudId,
                                                        std::set<std::string>& vus,
                                                        bool& uneSansCondition, Rapport& rapport) {
    if (!reponse.is_object()) {
        rapport.noeud(noeudId, "une reponse n'est pas un objet.");
        return std::nullopt;
    }
    DialogueChoice choix;
    const auto id = texte(reponse, "id");
    if (!id) {
        rapport.noeud(noeudId, "choix vide : une reponse n'a pas d'identifiant.");
        return std::nullopt;
    }
    choix.id = *id;
    if (!vus.insert(choix.id).second) {
        rapport.noeud(noeudId, "reponse '" + choix.id + "' en double.");
    }
    if (choix.id == DIALOGUE_CONTINUE_CHOICE) {
        rapport.noeud(noeudId, "l'identifiant 'continue' est reserve a la reponse implicite.");
    }
    choix.next = exiger(reponse, "next", noeudId + "' / reponse '" + choix.id, rapport);
    if (const auto condition = reponse.find("condition"); condition != reponse.end()) {
        FlagConditionRead lue = readFlagCondition(*condition);
        choix.condition = std::move(lue.condition);
        if (!choix.condition) {
            rapport.noeud(noeudId, "reponse '" + choix.id + "' : " + lue.error);
        }
    } else {
        uneSansCondition = true;
    }
    return choix;
}

// Les reponses d'une replique (`choices`), deja reconnu comme present.
void lireReponses(const Json& reponses, DialogueNode& noeud, Rapport& rapport) {
    if (!reponses.is_array()) {
        rapport.noeud(noeud.id, "'choices' n'est pas un tableau.");
        return;
    }
    if (reponses.empty()) {
        // Le « choix vide » du critere d'acceptation : le joueur resterait devant une replique
        // sans rien a repondre.
        rapport.noeud(noeud.id, "choix vide : 'choices' ne propose aucune reponse.");
        return;
    }
    std::set<std::string> vus;
    bool uneSansCondition = false;
    for (const Json& reponse : reponses) {
        if (std::optional<DialogueChoice> choix =
                lireReponse(reponse, noeud.id, vus, uneSansCondition, rapport)) {
            noeud.choices.push_back(std::move(*choix));
        }
    }
    if (!noeud.choices.empty() && !uneSansCondition) {
        // Des drapeaux qui les masqueraient toutes laisseraient une replique sans issue, et
        // cela ne se decouvrirait qu'en jeu, dans l'etat de monde precis qui la produit.
        rapport.noeud(noeud.id,
                      "toutes les reponses sont conditionnelles : il en faut une toujours "
                      "proposee.");
    }
}

void lireReplique(const Json& brut, DialogueNode& noeud, Rapport& rapport) {
    const auto reponses = brut.find("choices");
    const bool aSuite = brut.contains("next");
    if (reponses != brut.end()) {
        if (aSuite) {
            rapport.noeud(noeud.id, "une replique a des reponses OU une suite, pas les deux.");
        }
        lireReponses(*reponses, noeud, rapport);
        if (!reponses->is_array() || reponses->empty()) {
            return;
        }
    } else if (aSuite) {
        noeud.next = exiger(brut, "next", noeud.id, rapport);
    } else {
        rapport.noeud(noeud.id, "replique sans reponses ni suite ('choices' ou 'next').");
    }
    if (const auto attitude = brut.find("attitude"); attitude != brut.end()) {
        const auto lue =
            attitude->is_string() ? attitudeDepuis(attitude->get<std::string>()) : std::nullopt;
        if (!lue) {
            rapport.noeud(noeud.id, "attitude inconnue (friendly, indifferent, hostile).");
        }
        noeud.attitude = lue;
    }
}

void lireCondition(const Json& brut, DialogueNode& noeud, Rapport& rapport) {
    FlagConditionRead lue = readFlagCondition(brut);
    if (!lue.condition) {
        rapport.noeud(noeud.id, lue.error);
    } else {
        noeud.condition = std::move(*lue.condition);
    }
    noeud.whenTrue = exiger(brut, "then", noeud.id, rapport);
    noeud.whenFalse = exiger(brut, "else", noeud.id, rapport);
}

// Un effet d'un noeud d'action, ou rien si son type est inconnu (erreur consignee).
[[nodiscard]] std::optional<DialogueAction> lireEffet(const Json& effet, const std::string& noeudId,
                                                      Rapport& rapport) {
    const auto type = effet.is_object() ? texte(effet, "type") : std::nullopt;
    DialogueAction action;
    const char* champ = nullptr;
    if (type == "setFlag") {
        action.kind = DialogueActionKind::SetFlag;
        champ = "flag";
    } else if (type == "clearFlag") {
        action.kind = DialogueActionKind::ClearFlag;
        champ = "flag";
    } else if (type == "giveItem") {
        action.kind = DialogueActionKind::GiveItem;
        champ = "item";
    } else if (type == "startQuest") {
        action.kind = DialogueActionKind::StartQuest;
        champ = "quest";
    } else if (type == "startEncounter") {
        action.kind = DialogueActionKind::StartEncounter;
        champ = "encounter";
    } else if (type == "endDemo") {
        action.kind = DialogueActionKind::EndDemo;
        champ = "ending";
    } else {
        rapport.noeud(noeudId,
                      "action de type inconnu (setFlag, clearFlag, giveItem, "
                      "startQuest, startEncounter, endDemo).");
        return std::nullopt;
    }
    action.target = exiger(effet, champ, noeudId, rapport);
    if (action.kind == DialogueActionKind::SetFlag) {
        if (const auto valeur = effet.find("value"); valeur != effet.end()) {
            if (!valeur->is_string() || valeur->get<std::string>().empty()) {
                rapport.noeud(noeudId, "'value' doit etre un texte non vide.");
            } else {
                action.value = valeur->get<std::string>();
            }
        }
    }
    if (action.kind != DialogueActionKind::GiveItem) {
        return action;
    }
    if (const auto quantite = effet.find("quantity"); quantite != effet.end()) {
        if (!quantite->is_number_integer() || quantite->get<int>() < 1) {
            rapport.noeud(noeudId, "'quantity' doit etre un entier positif.");
        } else {
            action.quantity = quantite->get<int>();
        }
    }
    return action;
}

void lireAction(const Json& brut, DialogueNode& noeud, Rapport& rapport) {
    const auto effets = brut.find("actions");
    if (effets == brut.end() || !effets->is_array() || effets->empty()) {
        rapport.noeud(noeud.id, "'actions' absent, vide ou non tableau.");
    } else {
        for (const Json& effet : *effets) {
            if (std::optional<DialogueAction> action = lireEffet(effet, noeud.id, rapport)) {
                noeud.actions.push_back(std::move(*action));
            }
        }
    }
    noeud.next = exiger(brut, "next", noeud.id, rapport);
}

void lireJet(const Json& brut, DialogueNode& noeud, Rapport& rapport) {
    noeud.skill = exiger(brut, "skill", noeud.id, rapport);
    if (const auto seuil = brut.find("difficulty");
        seuil != brut.end() && seuil->is_number_integer()) {
        // Un nombre nu est le defaut que `EX-REG-021` interdit : il ne dit pas ce qu'il vaut, et
        // regler l'equilibre demanderait de relire chaque dialogue.
        rapport.noeud(noeud.id,
                      "'difficulty' nomme un degre de rules/difficulty.json, jamais un "
                      "nombre.");
    } else {
        noeud.difficulty = exiger(brut, "difficulty", noeud.id, rapport);
    }
    noeud.onSuccess = exiger(brut, "success", noeud.id, rapport);
    if (!texte(brut, "failure")) {
        // Le critere du LOT-117, nomme comme tel : l'auteur qui a oublie l'echec lit ce qu'il a
        // oublie, pas seulement le nom d'un champ.
        rapport.noeud(noeud.id, "jet sans branche d'echec : champ 'failure' absent ou vide.");
        return;
    }
    noeud.onFailure = exiger(brut, "failure", noeud.id, rapport);
    if (noeud.onFailure == noeud.onSuccess) {
        rapport.noeud(noeud.id,
                      "jet sans branche d'echec : 'failure' mene ou mene 'success', le jet ne "
                      "decide rien.");
    }
}

// Les cibles d'un noeud, dans l'ordre de la donnee.
[[nodiscard]] std::vector<const std::string*> ciblesDe(const DialogueNode& noeud) {
    std::vector<const std::string*> cibles;
    switch (noeud.kind) {
        case DialogueNodeKind::Line:
            if (noeud.choices.empty()) {
                cibles.push_back(&noeud.next);
            }
            for (const DialogueChoice& choix : noeud.choices) {
                cibles.push_back(&choix.next);
            }
            break;
        case DialogueNodeKind::Condition:
            cibles.push_back(&noeud.whenTrue);
            cibles.push_back(&noeud.whenFalse);
            break;
        case DialogueNodeKind::Action:
            cibles.push_back(&noeud.next);
            break;
        case DialogueNodeKind::Check:
            cibles.push_back(&noeud.onSuccess);
            cibles.push_back(&noeud.onFailure);
            break;
        case DialogueNodeKind::End:
            break;
    }
    return cibles;
}

// Un noeud ou le joueur CHOISIT : le seul par lequel une boucle a le droit de passer.
[[nodiscard]] bool estUnArret(const DialogueNode& noeud) {
    return noeud.kind == DialogueNodeKind::Line && !noeud.choices.empty();
}

// Indice de chaque noeud du graphe, par identifiant.
using IndicesDeNoeuds = std::map<std::string, std::size_t, std::less<>>;

// Indice d'un noeud dont l'existence est deja controlee.
[[nodiscard]] std::size_t indiceDe(const IndicesDeNoeuds& indices, const std::string& id) {
    return indices.find(id)->second;
}

// Orphelins : ce que rien n'atteint depuis l'entree. Rend les noeuds atteints.
[[nodiscard]] std::vector<bool> signalerLesOrphelins(const DialogueGraph& graphe,
                                                     const IndicesDeNoeuds& indices,
                                                     Rapport& rapport) {
    std::vector<bool> atteint(graphe.nodes.size(), false);
    std::vector<std::size_t> pile{indiceDe(indices, graphe.start)};
    atteint[pile.back()] = true;
    while (!pile.empty()) {
        const std::size_t i = pile.back();
        pile.pop_back();
        for (const std::string* cible : ciblesDe(graphe.nodes[i])) {
            const std::size_t j = indiceDe(indices, *cible);
            if (!atteint[j]) {
                atteint[j] = true;
                pile.push_back(j);
            }
        }
    }
    for (std::size_t i = 0; i < graphe.nodes.size(); ++i) {
        if (!atteint[i]) {
            rapport.noeud(graphe.nodes[i].id,
                          "orphelin : aucun chemin ne l'atteint depuis '" + graphe.start + "'.");
        }
    }
    return atteint;
}

// La trace d'un cycle : le chemin depuis la premiere occurrence de @p j, puis @p j.
[[nodiscard]] std::string traceDuCycle(const DialogueGraph& graphe,
                                       const std::vector<std::size_t>& chemin, std::size_t j) {
    const auto debut = std::ranges::find(chemin, j);
    std::string trace;
    for (auto k = debut; k != chemin.end(); ++k) {
        trace += graphe.nodes[*k].id + " -> ";
    }
    trace += graphe.nodes[j].id;
    return trace;
}

// Cycles non intentionnels : une boucle qui ne traverse aucun arret. Parcours en profondeur
// restreint aux noeuds qui ne sont pas des arrets -- une arete vers un arret termine le chemin.
void signalerLesCycles(const DialogueGraph& graphe, const IndicesDeNoeuds& indices,
                       Rapport& rapport) {
    enum class Couleur : std::uint8_t { BLANC, GRIS, NOIR };
    std::vector<Couleur> couleurs(graphe.nodes.size(), Couleur::BLANC);
    std::vector<std::size_t> chemin;
    std::set<std::string> dejaSignales;
    const std::function<void(std::size_t)> visiter = [&](std::size_t i) {
        couleurs[i] = Couleur::GRIS;
        chemin.push_back(i);
        for (const std::string* cible : ciblesDe(graphe.nodes[i])) {
            const std::size_t j = indiceDe(indices, *cible);
            if (estUnArret(graphe.nodes[j])) {
                continue;
            }
            if (couleurs[j] == Couleur::BLANC) {
                visiter(j);
            } else if (couleurs[j] == Couleur::GRIS &&
                       dejaSignales.insert(graphe.nodes[j].id).second) {
                rapport.noeud(graphe.nodes[j].id,
                              "cycle non intentionnel, sans reponse a donner : " +
                                  traceDuCycle(graphe, chemin, j) + ".");
            }
        }
        chemin.pop_back();
        couleurs[i] = Couleur::NOIR;
    };
    for (std::size_t i = 0; i < graphe.nodes.size(); ++i) {
        if (couleurs[i] == Couleur::BLANC && !estUnArret(graphe.nodes[i])) {
            visiter(i);
        }
    }
}

// Impasses : les noeuds atteints d'ou aucune fin n'est atteignable. Parcours inverse depuis
// les fins.
void signalerLesImpasses(const DialogueGraph& graphe, const IndicesDeNoeuds& indices,
                         const std::vector<bool>& atteint, Rapport& rapport) {
    std::vector<std::vector<std::size_t>> predecesseurs(graphe.nodes.size());
    for (std::size_t i = 0; i < graphe.nodes.size(); ++i) {
        for (const std::string* cible : ciblesDe(graphe.nodes[i])) {
            predecesseurs[indiceDe(indices, *cible)].push_back(i);
        }
    }
    std::vector<bool> menaUneFin(graphe.nodes.size(), false);
    std::vector<std::size_t> pile;
    for (std::size_t i = 0; i < graphe.nodes.size(); ++i) {
        if (graphe.nodes[i].kind == DialogueNodeKind::End) {
            menaUneFin[i] = true;
            pile.push_back(i);
        }
    }
    while (!pile.empty()) {
        const std::size_t i = pile.back();
        pile.pop_back();
        for (const std::size_t j : predecesseurs[i]) {
            if (!menaUneFin[j]) {
                menaUneFin[j] = true;
                pile.push_back(j);
            }
        }
    }
    for (std::size_t i = 0; i < graphe.nodes.size(); ++i) {
        if (atteint[i] && !menaUneFin[i]) {
            rapport.noeud(graphe.nodes[i].id, "impasse : aucune fin n'est atteignable d'ici.");
        }
    }
}

// Repliques qu'un jet rate peut laisser sans reponse (`LOT-117`). Une reponse qui mene a un jet
// disparait une fois ce jet rate : elle compte comme conditionnelle, et une replique doit garder
// une reponse toujours proposee -- sans condition, et qui ne jette pas.
void signalerLesRepliquesQuiPeuventSeVider(const DialogueGraph& graphe, Rapport& rapport) {
    const auto meneAUnJet = [&graphe](const DialogueChoice& choix) {
        const DialogueNode* suite = graphe.find(choix.next);
        return suite != nullptr && suite->kind == DialogueNodeKind::Check;
    };
    for (const DialogueNode& noeud : graphe.nodes) {
        if (!estUnArret(noeud) || !std::ranges::any_of(noeud.choices, meneAUnJet)) {
            continue;
        }
        const bool uneToujoursProposee =
            std::ranges::any_of(noeud.choices, [&meneAUnJet](const DialogueChoice& choix) {
                return !choix.condition && !meneAUnJet(choix);
            });
        if (!uneToujoursProposee) {
            rapport.noeud(noeud.id,
                          "un jet rate ne se propose plus : il faut une reponse toujours "
                          "proposee, sans condition et sans jet.");
        }
    }
}

// Les controles de graphe, sur un graphe dont chaque noeud est lu et chaque cible existe. Les
// faire sur un graphe incomplet produirait des orphelins et des impasses qui ne sont que l'ombre
// d'une cible mal orthographiee.
void controlerLeGraphe(const DialogueGraph& graphe, Rapport& rapport) {
    IndicesDeNoeuds indices;
    for (std::size_t i = 0; i < graphe.nodes.size(); ++i) {
        indices.emplace(graphe.nodes[i].id, i);
    }
    const std::vector<bool> atteint = signalerLesOrphelins(graphe, indices, rapport);
    signalerLesCycles(graphe, indices, rapport);
    signalerLesImpasses(graphe, indices, atteint, rapport);
    signalerLesRepliquesQuiPeuventSeVider(graphe, rapport);
}

[[nodiscard]] std::vector<std::filesystem::path> fichiersJson(const std::filesystem::path& dossier,
                                                              std::error_code& code) {
    std::vector<std::filesystem::path> fichiers;
    for (const auto& entree : std::filesystem::directory_iterator(dossier, code)) {
        if (entree.is_regular_file(code) && entree.path().extension() == ".json") {
            fichiers.push_back(entree.path());
        }
    }
    std::ranges::sort(fichiers);
    return fichiers;
}

}  // namespace

// ---------------------------------------------------------------------------------------------

const DialogueNode* DialogueGraph::find(std::string_view nodeId) const {
    const auto trouve = std::ranges::find(nodes, nodeId, &DialogueNode::id);
    return trouve == nodes.end() ? nullptr : &*trouve;
}

std::string dialogueSpeakerKey(std::string_view dialogueId) {
    return "dialogue." + std::string(dialogueId) + ".speaker";
}

std::string dialogueLineKey(std::string_view dialogueId, std::string_view nodeId) {
    return "dialogue." + std::string(dialogueId) + '.' + std::string(nodeId);
}

std::string dialogueChoiceKey(std::string_view dialogueId, std::string_view nodeId,
                              std::string_view choiceId) {
    return dialogueLineKey(dialogueId, nodeId) + '.' + std::string(choiceId);
}

std::string_view dialogueAttitudeName(DialogueAttitude attitude) noexcept {
    switch (attitude) {
        case DialogueAttitude::Friendly:
            return "friendly";
        case DialogueAttitude::Indifferent:
            return "indifferent";
        case DialogueAttitude::Hostile:
            return "hostile";
    }
    return "indifferent";
}

std::string demoEndingKey(std::string_view ending) {
    return "ending." + std::string(ending);
}

std::string dialogueAttitudeKey(DialogueAttitude attitude) {
    return "dialogue.attitude." + std::string(dialogueAttitudeName(attitude));
}

std::vector<std::string> dialogueTextKeys(const DialogueGraph& graph) {
    std::vector<std::string> cles;
    const auto ajouter = [&cles](std::string cle) {
        if (std::ranges::find(cles, cle) == cles.end()) {
            cles.push_back(std::move(cle));
        }
    };
    ajouter(dialogueSpeakerKey(graph.id));
    ajouter(dialogueAttitudeKey(graph.attitude));
    for (const DialogueNode& noeud : graph.nodes) {
        if (noeud.kind != DialogueNodeKind::Line) {
            continue;
        }
        ajouter(dialogueLineKey(graph.id, noeud.id));
        if (noeud.attitude) {
            ajouter(dialogueAttitudeKey(*noeud.attitude));
        }
        if (noeud.choices.empty()) {
            ajouter(std::string(DIALOGUE_CONTINUE_KEY));
        }
        for (const DialogueChoice& choix : noeud.choices) {
            ajouter(dialogueChoiceKey(graph.id, noeud.id, choix.id));
        }
    }
    // La voie d'une fin de demo se dit sur l'ecran de fin : sa cle est reclamee par le dialogue
    // qui la nomme, et le meme test la confronte aux deux catalogues.
    for (const DialogueNode& noeud : graph.nodes) {
        for (const DialogueAction& action : noeud.actions) {
            if (action.kind == DialogueActionKind::EndDemo) {
                ajouter(demoEndingKey(action.target));
            }
        }
    }
    return cles;
}

std::string questStartedFlag(std::string_view questId) {
    return "quest/" + std::string(questId) + "/started";
}

std::string dialogueCheckFailedFlag(std::string_view dialogueId, std::string_view checkNodeId) {
    return "dialogue/" + std::string(dialogueId) + '/' + std::string(checkNodeId) + "/failed";
}

// ---------------------------------------------------------------------------------------------

namespace {

// Les langues et l'attitude de l'interlocuteur (`speaker`).
void lireInterlocuteur(const Json& racine, DialogueGraph& graphe, Rapport& rapport) {
    const auto interlocuteur = racine.find("speaker");
    if (interlocuteur == racine.end() || !interlocuteur->is_object()) {
        rapport.document("champ 'speaker' absent ou non objet.");
        return;
    }
    const auto langues = interlocuteur->find("languages");
    if (langues == interlocuteur->end() || !langues->is_array() || langues->empty()) {
        // Sans langue declaree, « refuse faute de langue commune » ne se deciderait jamais :
        // le PNJ parlerait a tout le monde, ce que `EX-RPG-042` interdit de supposer.
        rapport.document(
            "'speaker.languages' absent ou vide : un PNJ parle au moins une "
            "langue.");
    } else {
        for (const Json& langue : *langues) {
            if (langue.is_string() && !langue.get<std::string>().empty()) {
                graphe.speakerLanguages.push_back(langue.get<std::string>());
            } else {
                rapport.document(
                    "'speaker.languages' contient une valeur qui n'est pas un "
                    "identifiant.");
            }
        }
    }
    if (const auto attitude = interlocuteur->find("attitude"); attitude != interlocuteur->end()) {
        const auto lue =
            attitude->is_string() ? attitudeDepuis(attitude->get<std::string>()) : std::nullopt;
        if (lue) {
            graphe.attitude = *lue;
        } else {
            rapport.document("'speaker.attitude' inconnue (friendly, indifferent, hostile).");
        }
    }
}

// Un noeud du graphe, ou rien s'il est inutilisable (erreur consignee).
[[nodiscard]] std::optional<DialogueNode> lireNoeud(const Json& brut, std::set<std::string>& vus,
                                                    Rapport& rapport) {
    if (!brut.is_object()) {
        rapport.document("un noeud n'est pas un objet.");
        return std::nullopt;
    }
    DialogueNode noeud;
    if (const auto id = texte(brut, "id")) {
        noeud.id = *id;
    } else {
        rapport.document("un noeud n'a pas d'identifiant.");
        return std::nullopt;
    }
    if (!vus.insert(noeud.id).second) {
        rapport.noeud(noeud.id, "identifiant en double.");
        return std::nullopt;
    }
    const auto type = texte(brut, "type");
    const auto nature = type ? natureDepuis(*type) : std::nullopt;
    if (!nature) {
        rapport.noeud(noeud.id, "type inconnu (line, condition, action, check, end) : '" +
                                    type.value_or("") + "'.");
        return std::nullopt;
    }
    noeud.kind = *nature;
    switch (noeud.kind) {
        case DialogueNodeKind::Line:
            lireReplique(brut, noeud, rapport);
            break;
        case DialogueNodeKind::Condition:
            lireCondition(brut, noeud, rapport);
            break;
        case DialogueNodeKind::Action:
            lireAction(brut, noeud, rapport);
            break;
        case DialogueNodeKind::Check:
            lireJet(brut, noeud, rapport);
            break;
        case DialogueNodeKind::End:
            break;
    }
    return noeud;
}

// Les noeuds du graphe (`nodes`).
void lireNoeuds(const Json& racine, DialogueGraph& graphe, Rapport& rapport) {
    const auto noeuds = racine.find("nodes");
    if (noeuds == racine.end() || !noeuds->is_array() || noeuds->empty()) {
        rapport.document("champ 'nodes' absent, vide ou non tableau.");
        return;
    }
    std::set<std::string> vus;
    for (const Json& brut : *noeuds) {
        if (std::optional<DialogueNode> noeud = lireNoeud(brut, vus, rapport)) {
            graphe.nodes.push_back(std::move(*noeud));
        }
    }
}

// Les cibles, une fois tous les noeuds connus.
void controlerLesCibles(const DialogueGraph& graphe, Rapport& rapport) {
    if (!graphe.start.empty() && !graphe.nodes.empty() && graphe.find(graphe.start) == nullptr) {
        rapport.document("noeud cible inconnu : l'entree 'start' nomme '" + graphe.start + "'.");
    }
    for (const DialogueNode& noeud : graphe.nodes) {
        for (const std::string* cible : ciblesDe(noeud)) {
            if (!cible->empty() && graphe.find(*cible) == nullptr) {
                rapport.noeud(noeud.id, "noeud cible inconnu : '" + *cible + "'.");
            }
        }
    }
    const bool fin = std::ranges::any_of(
        graphe.nodes, [](const DialogueNode& n) { return n.kind == DialogueNodeKind::End; });
    if (!graphe.nodes.empty() && !fin) {
        rapport.document("aucun noeud 'end' : la conversation ne pourrait pas se terminer.");
    }
}

}  // namespace

DialogueLoad readDialogue(std::string_view json, std::string_view origin) {
    DialogueLoad resultat;
    Rapport rapport(origin);

    const JsonDocument document = readJsonObject(json, SANS_GARDE_DE_VERSION, origin);
    if (!document.ok()) {
        resultat.errors.push_back(std::string(origin) + " : " + document.message);
        return resultat;
    }
    const Json& racine = document.root;

    DialogueGraph graphe;
    if (const auto id = texte(racine, "id")) {
        graphe.id = *id;
    } else {
        rapport.document("champ 'id' absent ou vide.");
    }
    if (const auto entree = texte(racine, "start")) {
        graphe.start = *entree;
    } else {
        rapport.document("champ 'start' absent ou vide.");
    }

    lireInterlocuteur(racine, graphe, rapport);
    lireNoeuds(racine, graphe, rapport);
    controlerLesCibles(graphe, rapport);

    if (rapport.vide()) {
        controlerLeGraphe(graphe, rapport);
    }
    if (!rapport.vide()) {
        resultat.errors = rapport.extraire();
        return resultat;
    }
    resultat.graph = std::move(graphe);
    return resultat;
}

DialogueLoad loadDialogue(const std::filesystem::path& path) {
    const JsonDocument lu = readJsonObjectFromFile(path, SANS_GARDE_DE_VERSION);
    if (!lu.ok()) {
        DialogueLoad echec;
        echec.errors.push_back(path.string() + " : " + lu.message);
        return echec;
    }
    return readDialogue(lu.root.dump(), path.string());
}

namespace {

// Les references d'un noeud (competence, degre, objet) vers les catalogues du jeu.
void verifierLesReferencesDuNoeud(const DialogueNode& noeud, const std::string& dialogueId,
                                  const DialogueReferences& references,
                                  std::vector<std::string>& erreurs) {
    const auto signaler = [&](const std::string& message) {
        erreurs.push_back("dialogue '" + dialogueId + "' : noeud '" + noeud.id + "' : " + message);
    };
    if (noeud.kind == DialogueNodeKind::Check) {
        if (references.skills != nullptr && references.skills->find(noeud.skill) == nullptr) {
            signaler("competence inconnue '" + noeud.skill + "'.");
        }
        if (references.difficulty != nullptr &&
            references.difficulty->find(noeud.difficulty) == nullptr) {
            signaler("degre de difficulte inconnu '" + noeud.difficulty + "'.");
        }
    }
    if (noeud.kind != DialogueNodeKind::Action || !references.itemExists) {
        return;
    }
    for (const DialogueAction& action : noeud.actions) {
        if (action.kind == DialogueActionKind::GiveItem && !references.itemExists(action.target)) {
            signaler("objet inconnu '" + action.target + "'.");
        }
    }
}

}  // namespace

std::vector<std::string> validateDialogueReferences(const DialogueGraph& graph,
                                                    const DialogueReferences& references) {
    std::vector<std::string> erreurs;
    if (references.languageExists) {
        for (const std::string& langue : graph.speakerLanguages) {
            if (!references.languageExists(langue)) {
                erreurs.push_back("dialogue '" + graph.id + "' : langue inconnue '" + langue +
                                  "'.");
            }
        }
    }
    for (const DialogueNode& noeud : graph.nodes) {
        verifierLesReferencesDuNoeud(noeud, graph.id, references, erreurs);
    }
    return erreurs;
}

const DialogueGraph* DialogueCatalog::find(std::string_view id) const {
    const auto trouve = std::ranges::find(dialogues, id, &DialogueGraph::id);
    return trouve == dialogues.end() ? nullptr : &*trouve;
}

DialogueCatalog loadDialogues(const std::filesystem::path& directory) {
    DialogueCatalog catalogue;
    std::error_code code;
    if (!std::filesystem::is_directory(directory, code)) {
        catalogue.errors.push_back(directory.string() + " : dossier absent ou illisible.");
        return catalogue;
    }
    for (const std::filesystem::path& chemin : fichiersJson(directory, code)) {
        DialogueLoad lu = loadDialogue(chemin);
        catalogue.errors.insert(catalogue.errors.end(), lu.errors.begin(), lu.errors.end());
        if (!lu.graph) {
            continue;
        }
        if (lu.graph->id != chemin.stem().string()) {
            // Une carte nomme un dialogue par son identifiant ; un fichier qui en porte un autre
            // se chargerait et ne s'ouvrirait jamais.
            catalogue.errors.push_back(chemin.string() + " : l'identifiant '" + lu.graph->id +
                                       "' doit etre le nom du fichier.");
            continue;
        }
        catalogue.dialogues.push_back(std::move(*lu.graph));
    }
    return catalogue;
}

// ---------------------------------------------------------------------------------------------

CharacterListener::CharacterListener(const CharacterSheet& sheet, Inventory& inventory,
                                     const ExperienceTable& experience, const SkillCatalog& skills)
    : _sheet(sheet), _inventory(inventory), _experience(experience), _skills(skills) {}

bool CharacterListener::speaks(std::string_view languageId) const {
    return _sheet.languages.contains(std::string(languageId));
}

std::vector<Modifier> CharacterListener::skillModifiers(std::string_view skillId) const {
    std::vector<Modifier> modificateurs;
    const SkillDefinition* competence = _skills.find(skillId);
    if (competence == nullptr) {
        return modificateurs;
    }
    // Detaille, et non le total de `skillModifier` : « +3 (charisma) + 2 (maitrise) » se
    // restitue, « +5 » ne dit pas d'ou il vient (EX-REG-003). La regle reste la sienne.
    const SkillCheckModifier total = skillModifier(_sheet, _experience, _skills, skillId);
    const int caracteristique = _sheet.modifier(competence->ability);
    modificateurs.push_back(
        {.source = std::string(abilityName(competence->ability)), .value = caracteristique});
    if (total.proficient) {
        modificateurs.push_back({.source = "maitrise", .value = total.value - caracteristique});
    }
    return modificateurs;
}

void CharacterListener::receiveItem(std::string_view itemId, int quantity) {
    addToBackpack(_inventory, std::string(itemId), quantity);
}

// ---------------------------------------------------------------------------------------------

DialogueRunner::DialogueRunner(const DialogueGraph& graph, WorldFlags& flags,
                               DialogueListener& listener, const DifficultyScale& difficulty,
                               DeterministicRandom& random)
    : _graph(graph), _flags(flags), _listener(listener), _difficulty(difficulty), _random(random) {}

DialogueState DialogueRunner::start() {
    if (_state != DialogueState::NotStarted) {
        return _state;
    }
    const bool commune = std::ranges::any_of(
        _graph.speakerLanguages, [this](const std::string& l) { return _listener.speaks(l); });
    if (!commune) {
        // Refuse AVANT tout noeud : une conversation qui n'a pas lieu ne pose aucun drapeau, sans
        // quoi un PNJ qu'on ne comprend pas ferait avancer une quete.
        std::string langues;
        for (const std::string& l : _graph.speakerLanguages) {
            langues += (langues.empty() ? "" : ", ") + l;
        }
        _journal.push_back("refus : aucune langue commune (" + langues + ")");
        _state = DialogueState::Refused;
        return _state;
    }
    _journal.push_back("debut : " + _graph.id);
    _automaticSteps = 0;
    advanceTo(_graph.start);
    return _state;
}

ChoiceResult DialogueRunner::choose(std::string_view choiceId) {
    if (_state != DialogueState::AwaitingChoice || _current == nullptr) {
        return ChoiceResult::NotAwaiting;
    }
    std::string suite;
    if (_current->choices.empty()) {
        if (choiceId != DIALOGUE_CONTINUE_CHOICE) {
            return ChoiceResult::Unavailable;
        }
        suite = _current->next;
    } else {
        const auto choix = std::ranges::find(_current->choices, choiceId, &DialogueChoice::id);
        // La condition est REEVALUEE : entre l'affichage et le geste, un drapeau a pu changer, et
        // l'ecran ne doit pas pouvoir faire passer une reponse que la donnee n'offre plus.
        if (choix == _current->choices.end() || !isOffered(*choix)) {
            return ChoiceResult::Unavailable;
        }
        suite = choix->next;
    }
    _journal.push_back("reponse : " + std::string(choiceId));
    _automaticSteps = 0;
    _lastCheck.reset();
    advanceTo(suite);
    return ChoiceResult::Advanced;
}

void DialogueRunner::advanceTo(const std::string& nodeId) {
    std::string id = nodeId;
    while (true) {
        const DialogueNode* noeud = _graph.find(id);
        // La validation rend ces deux gardes inatteignables pour un graphe charge ; un graphe
        // construit a la main peut les atteindre, et il vaut mieux finir que boucler ou planter.
        if (noeud == nullptr) {
            _journal.push_back("erreur : noeud inconnu '" + id + "', fin forcee");
            _current = nullptr;
            _state = DialogueState::Ended;
            return;
        }
        if (++_automaticSteps > _graph.nodes.size() + 1) {
            _journal.emplace_back("erreur : boucle sans reponse, fin forcee");
            _current = nullptr;
            _state = DialogueState::Ended;
            return;
        }
        switch (noeud->kind) {
            case DialogueNodeKind::Line:
                _journal.push_back("replique : " + noeud->id);
                _current = noeud;
                _state = DialogueState::AwaitingChoice;
                return;
            case DialogueNodeKind::End:
                _journal.push_back("fin : " + noeud->id);
                _current = nullptr;
                _state = DialogueState::Ended;
                return;
            case DialogueNodeKind::Condition: {
                const bool tient = noeud->condition.holds(_flags);
                _journal.push_back("condition : " + noeud->id + " (" + noeud->condition.flag +
                                   (tient ? ") tient" : ") ne tient pas"));
                id = tient ? noeud->whenTrue : noeud->whenFalse;
                break;
            }
            case DialogueNodeKind::Action:
                _journal.push_back("action : " + noeud->id);
                for (const DialogueAction& action : noeud->actions) {
                    apply(action);
                }
                id = noeud->next;
                break;
            case DialogueNodeKind::Check:
                runCheck(*noeud);
                id = (_lastCheck && _lastCheck->nodeId == noeud->id && !_lastCheck->alreadyFailed &&
                      _lastCheck->result.succeeded())
                         ? noeud->onSuccess
                         : noeud->onFailure;
                break;
        }
    }
}

void DialogueRunner::apply(const DialogueAction& action) {
    switch (action.kind) {
        case DialogueActionKind::SetFlag:
            if (action.value.empty()) {
                _flags.set(action.target);
                _journal.push_back("drapeau pose : " + action.target);
            } else if (_flags.setValue(action.target, action.value)) {
                _journal.push_back("drapeau pose : " + action.target + " = " + action.value);
            } else {
                // Une valeur hors de la declaration est refusee au chargement du catalogue
                // (`validateFlagUses`) ; si elle arrive ici, la trace le dit plutot que de taire.
                _journal.push_back("drapeau refuse : " + action.target + " = " + action.value);
            }
            break;
        case DialogueActionKind::ClearFlag:
            _flags.clear(action.target);
            _journal.push_back("drapeau retire : " + action.target);
            break;
        case DialogueActionKind::GiveItem:
            _listener.receiveItem(action.target, action.quantity);
            _journal.push_back("objet donne : " + action.target + " x" +
                               std::to_string(action.quantity));
            break;
        case DialogueActionKind::StartQuest:
            _flags.set(questStartedFlag(action.target));
            _journal.push_back("quete demarree : " + action.target);
            break;
        case DialogueActionKind::StartEncounter:
            _listener.startEncounter(action.target);
            _journal.push_back("rencontre demandee : " + action.target);
            break;
        case DialogueActionKind::EndDemo:
            _listener.endDemo(action.target);
            _journal.push_back("fin de la demo : " + action.target);
            break;
    }
}

void DialogueRunner::runCheck(const DialogueNode& node) {
    const DifficultyTier* degre = _difficulty.find(node.difficulty);
    if (degre == nullptr) {
        // Un degre inconnu ne se jette pas contre 0 -- tout reussirait. L'echec est la suite
        // prudente, et le journal le dit.
        _lastCheck.reset();
        _journal.push_back("jet : " + node.id + " -- degre inconnu '" + node.difficulty +
                           "', echec");
        return;
    }
    DialogueCheck jet;
    jet.nodeId = node.id;
    jet.skill = node.skill;
    jet.difficulty = node.difficulty;
    const std::string rate = dialogueCheckFailedFlag(_graph.id, node.id);
    if (_flags.isSet(rate)) {
        // Deja rate (LOT-117) : le de ne se relance pas -- sans quoi revenir par un autre chemin
        // suffirait a retenter sa chance, et le drapeau ne garderait rien. Aucun tirage : la suite
        // aleatoire reste celle qu'elle aurait ete.
        jet.alreadyFailed = true;
        jet.result.target = degre->dc;
        _journal.push_back("jet : " + node.id + " (" + node.skill + ") deja rate, echec");
        _lastCheck = std::move(jet);
        return;
    }
    const std::vector<Modifier> modificateurs = _listener.skillModifiers(node.skill);
    jet.result = rollCheck(degre->dc, modificateurs, RollStance::Normal, _random);
    _journal.push_back("jet : " + node.id + " (" + node.skill + ") " + jet.result.describe());
    if (!jet.result.succeeded()) {
        _flags.set(rate);
        _journal.push_back("drapeau pose : " + rate);
    }
    _lastCheck = std::move(jet);
}

bool DialogueRunner::isOffered(const DialogueChoice& choice) const {
    if (choice.condition && !choice.condition->holds(_flags)) {
        return false;
    }
    // Une reponse qui mene a un jet deja rate ne se propose plus (LOT-117).
    const DialogueNode* suite = _graph.find(choice.next);
    return suite == nullptr || suite->kind != DialogueNodeKind::Check ||
           !_flags.isSet(dialogueCheckFailedFlag(_graph.id, suite->id));
}

const DialogueNode* DialogueRunner::currentLine() const {
    return _state == DialogueState::AwaitingChoice ? _current : nullptr;
}

std::string DialogueRunner::lineKey() const {
    const DialogueNode* ligne = currentLine();
    return ligne == nullptr ? std::string{} : dialogueLineKey(_graph.id, ligne->id);
}

DialogueAttitude DialogueRunner::attitude() const {
    const DialogueNode* ligne = currentLine();
    return (ligne != nullptr && ligne->attitude) ? *ligne->attitude : _graph.attitude;
}

std::vector<AvailableChoice> DialogueRunner::choices() const {
    std::vector<AvailableChoice> proposees;
    const DialogueNode* ligne = currentLine();
    if (ligne == nullptr) {
        return proposees;
    }
    // Le jet qu'une reponse annonce : sa competence et son seuil, pour que le joueur choisisse en
    // sachant ce qu'il risque (LOT-117).
    const auto annoncer = [this](AvailableChoice& reponse, const std::string& cible) {
        const DialogueNode* suite = _graph.find(cible);
        if (suite == nullptr || suite->kind != DialogueNodeKind::Check) {
            return;
        }
        reponse.checkSkill = suite->skill;
        if (const DifficultyTier* degre = _difficulty.find(suite->difficulty)) {
            reponse.checkDc = degre->dc;
        }
    };
    if (ligne->choices.empty()) {
        AvailableChoice suite{.id = std::string(DIALOGUE_CONTINUE_CHOICE),
                              .textKey = std::string(DIALOGUE_CONTINUE_KEY),
                              .checkSkill = {},
                              .checkDc = 0};
        annoncer(suite, ligne->next);
        proposees.push_back(std::move(suite));
        return proposees;
    }
    for (const DialogueChoice& choix : ligne->choices) {
        if (!isOffered(choix)) {
            continue;
        }
        AvailableChoice reponse{.id = choix.id,
                                .textKey = dialogueChoiceKey(_graph.id, ligne->id, choix.id),
                                .checkSkill = {},
                                .checkDc = 0};
        annoncer(reponse, choix.next);
        proposees.push_back(std::move(reponse));
    }
    return proposees;
}

// ---------------------------------------------------------------------------------------------

std::optional<DialogueTrigger> dialogueTriggerFor(const MapEntity& entity) {
    if (entity.type != NPC_ENTITY_TYPE) {
        return std::nullopt;
    }
    const auto trouve = entity.properties.find(std::string(NPC_DIALOGUE_PROPERTY));
    if (trouve == entity.properties.end()) {
        return std::nullopt;
    }
    const std::string* dialogue = std::get_if<std::string>(&trouve->second);
    if (dialogue == nullptr || dialogue->empty()) {
        return std::nullopt;
    }
    return DialogueTrigger{.dialogueId = *dialogue, .position = entity.position};
}

}  // namespace core
