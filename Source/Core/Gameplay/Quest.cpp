// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Gameplay/Quest.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>
#include <system_error>

#include "Core/Data/JsonDocument.h"
#include "Core/Gameplay/WorldFlags.h"
#include "Core/Rpg/Dialogue.h"

namespace core {

namespace {

using Json = nlohmann::json;
using Pointeur = Json::json_pointer;

/// Aucune garde de version : le format naît avec ce lot, et un champ `version` absent ne dit rien.
constexpr int SANS_GARDE_DE_VERSION = 0;

/// Rassemble les erreurs d'un document, chacune préfixée de son fichier et de sa ligne.
class Rapport {
public:
    Rapport(std::string_view texte, std::string_view origine) : _texte(texte), _origine(origine) {}

    void signaler(const Pointeur& ou, const std::string& message) {
        const TextPosition position = positionOfPointer(_texte, ou);
        std::string prefixe = _origine;
        if (position.line > 0) {
            prefixe += ':' + std::to_string(position.line);
        }
        _erreurs.push_back(prefixe + " : " + message);
    }
    [[nodiscard]] bool vide() const noexcept {
        return _erreurs.empty();
    }
    [[nodiscard]] std::vector<std::string> extraire() {
        return std::move(_erreurs);
    }

private:
    std::string_view _texte;
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

[[nodiscard]] bool contient(const std::vector<std::string>& valeurs, std::string_view valeur) {
    return std::ranges::find(valeurs, valeur) != valeurs.end();
}

void lireDrapeaux(const Json& racine, Quest& quete, Rapport& rapport) {
    const auto drapeaux = racine.find("flags");
    if (drapeaux == racine.end()) {
        return;
    }
    if (!drapeaux->is_array()) {
        rapport.signaler(Pointeur("/flags"), "'flags' doit etre un tableau.");
        return;
    }
    std::set<std::string, std::less<>> vus;
    for (std::size_t i = 0; i < drapeaux->size(); ++i) {
        const Json& brut = (*drapeaux)[i];
        const Pointeur ou = Pointeur("/flags") / i;
        const auto id = brut.is_object() ? texte(brut, "id") : std::nullopt;
        if (!id) {
            rapport.signaler(ou, "drapeau sans 'id'.");
            continue;
        }
        QuestFlag drapeau{.id = *id, .values = {}, .initial = {}};
        if (!vus.insert(drapeau.id).second) {
            rapport.signaler(ou, "drapeau '" + drapeau.id + "' declare deux fois.");
        }
        const auto valeurs = brut.find("values");
        if (valeurs == brut.end() || !valeurs->is_array() || valeurs->empty()) {
            rapport.signaler(ou, "drapeau '" + drapeau.id + "' : 'values' absent ou vide.");
            continue;
        }
        for (std::size_t v = 0; v < valeurs->size(); ++v) {
            const Json& valeur = (*valeurs)[v];
            if (!valeur.is_string() || valeur.get<std::string>().empty()) {
                rapport.signaler(ou / "values" / v,
                                 "drapeau '" + drapeau.id + "' : une valeur n'est pas un texte.");
            } else if (contient(drapeau.values, valeur.get<std::string>())) {
                rapport.signaler(ou / "values" / v, "drapeau '" + drapeau.id + "' : valeur '" +
                                                        valeur.get<std::string>() + "' en double.");
            } else {
                drapeau.values.push_back(valeur.get<std::string>());
            }
        }
        if (const auto initiale = texte(brut, "initial")) {
            drapeau.initial = *initiale;
        } else if (!drapeau.values.empty()) {
            drapeau.initial = drapeau.values.front();
        }
        if (!drapeau.values.empty() && !contient(drapeau.values, drapeau.initial)) {
            rapport.signaler(ou / "initial", "drapeau '" + drapeau.id + "' : initiale '" +
                                                 drapeau.initial + "' hors de ses valeurs.");
        }
        quete.flags.push_back(std::move(drapeau));
    }
}

/// Une valeur comparée ou posée doit être l'une de celles que la quête déclare pour ce drapeau.
void verifierValeur(const Quest& quete, std::string_view drapeau, std::string_view valeur,
                    const Pointeur& ou, Rapport& rapport) {
    const auto declaration = std::ranges::find(quete.flags, drapeau, &QuestFlag::id);
    if (declaration != quete.flags.end() && !contient(declaration->values, valeur)) {
        rapport.signaler(ou, "valeur '" + std::string(valeur) + "' que le drapeau '" +
                                 std::string(drapeau) + "' ne declare pas.");
    }
}

void lireConditions(const Json& brut, const Pointeur& ou, const Quest& quete, QuestStep& etape,
                    Rapport& rapport) {
    const auto conditions = brut.find("when");
    if (conditions == brut.end() || !conditions->is_array() || conditions->empty()) {
        rapport.signaler(ou, "etape '" + etape.id + "' : 'when' absent ou vide.");
        return;
    }
    for (std::size_t c = 0; c < conditions->size(); ++c) {
        const Pointeur ici = ou / "when" / c;
        FlagConditionRead lue = readFlagCondition((*conditions)[c]);
        if (!lue.condition) {
            rapport.signaler(ici, "etape '" + etape.id + "' : " + lue.error);
            continue;
        }
        for (const std::string& valeur : lue.condition->values) {
            verifierValeur(quete, lue.condition->flag, valeur, ici, rapport);
        }
        etape.when.push_back(std::move(*lue.condition));
    }
}

void lireEffets(const Json& brut, const Pointeur& ou, const Quest& quete, QuestStep& etape,
                Rapport& rapport) {
    const auto effets = brut.find("effects");
    if (effets == brut.end()) {
        return;
    }
    if (!effets->is_array()) {
        rapport.signaler(ou / "effects", "etape '" + etape.id + "' : 'effects' non tableau.");
        return;
    }
    for (std::size_t e = 0; e < effets->size(); ++e) {
        const Json& effet = (*effets)[e];
        const Pointeur ici = ou / "effects" / e;
        const auto type = effet.is_object() ? texte(effet, "type") : std::nullopt;
        const auto drapeau = effet.is_object() ? texte(effet, "flag") : std::nullopt;
        if (type != "setFlag" && type != "clearFlag") {
            rapport.signaler(
                ici, "etape '" + etape.id + "' : effet de type inconnu (setFlag, clearFlag).");
            continue;
        }
        if (!drapeau) {
            rapport.signaler(ici, "etape '" + etape.id + "' : effet sans 'flag'.");
            continue;
        }
        QuestEffect lu{
            .kind = type == "setFlag" ? QuestEffect::Kind::SetFlag : QuestEffect::Kind::ClearFlag,
            .flag = *drapeau,
            .value = {}};
        if (const auto valeur = effet.find("value"); valeur != effet.end()) {
            if (lu.kind != QuestEffect::Kind::SetFlag || !valeur->is_string() ||
                valeur->get<std::string>().empty()) {
                rapport.signaler(ici / "value",
                                 "etape '" + etape.id + "' : 'value' est un texte, pour setFlag.");
                continue;
            }
            lu.value = valeur->get<std::string>();
            verifierValeur(quete, lu.flag, lu.value, ici / "value", rapport);
        } else if (lu.kind == QuestEffect::Kind::SetFlag &&
                   std::ranges::find(quete.flags, lu.flag, &QuestFlag::id) != quete.flags.end()) {
            rapport.signaler(ici, "etape '" + etape.id + "' : le drapeau '" + lu.flag +
                                      "' a des valeurs ; 'value' manque.");
        }
        etape.effects.push_back(std::move(lu));
    }
}

void lireEtapes(const Json& racine, Quest& quete, Rapport& rapport) {
    const auto etapes = racine.find("steps");
    if (etapes == racine.end() || !etapes->is_array() || etapes->empty()) {
        rapport.signaler(Pointeur(), "'steps' absent, vide ou non tableau.");
        return;
    }
    std::set<std::string, std::less<>> vus;
    for (std::size_t i = 0; i < etapes->size(); ++i) {
        const Json& brut = (*etapes)[i];
        const Pointeur ou = Pointeur("/steps") / i;
        const auto id = brut.is_object() ? texte(brut, "id") : std::nullopt;
        if (!id) {
            rapport.signaler(ou, "etape sans 'id'.");
            continue;
        }
        QuestStep etape;
        etape.id = *id;
        if (!vus.insert(etape.id).second) {
            rapport.signaler(ou, "etape '" + etape.id + "' en double.");
        }
        lireConditions(brut, ou, quete, etape, rapport);
        lireEffets(brut, ou, quete, etape, rapport);
        if (const auto issue = brut.find("outcome"); issue != brut.end()) {
            if (*issue == "success") {
                etape.outcome = QuestOutcome::Success;
            } else if (*issue == "failure") {
                etape.outcome = QuestOutcome::Failure;
            } else {
                rapport.signaler(ou / "outcome",
                                 "etape '" + etape.id + "' : issue inconnue (success, failure).");
            }
        }
        quete.steps.push_back(std::move(etape));
    }
}

[[nodiscard]] std::optional<std::string> lireFichier(const std::filesystem::path& chemin) {
    std::ifstream flux(chemin, std::ios::binary);
    if (!flux) {
        return std::nullopt;
    }
    return std::string(std::istreambuf_iterator<char>(flux), std::istreambuf_iterator<char>());
}

}  // namespace

const QuestStep* Quest::find(std::string_view stepId) const {
    const auto trouve = std::ranges::find(steps, stepId, &QuestStep::id);
    return trouve == steps.end() ? nullptr : &*trouve;
}

std::string questStepFlag(std::string_view questId, std::string_view stepId) {
    return "quest/" + std::string(questId) + "/step/" + std::string(stepId);
}

std::string questTitleKey(std::string_view questId) {
    return "quest." + std::string(questId) + ".title";
}

std::string questStepKey(std::string_view questId, std::string_view stepId) {
    return "quest." + std::string(questId) + '.' + std::string(stepId);
}

std::vector<std::string> questTextKeys(const Quest& quest) {
    std::vector<std::string> cles{questTitleKey(quest.id)};
    for (const QuestStep& etape : quest.steps) {
        cles.push_back(questStepKey(quest.id, etape.id));
    }
    return cles;
}

QuestLoad readQuest(std::string_view json, std::string_view origin) {
    QuestLoad resultat;
    const JsonDocument document = readJsonObject(json, SANS_GARDE_DE_VERSION, origin);
    if (!document.ok()) {
        // Le message de la brique commune porte déjà `fichier:ligne:colonne`.
        resultat.errors.push_back(document.message);
        return resultat;
    }
    Rapport rapport(json, origin);
    const Json& racine = document.root;

    Quest quete;
    if (const auto id = texte(racine, "id")) {
        quete.id = *id;
    } else {
        rapport.signaler(Pointeur(), "champ 'id' absent ou vide.");
    }
    lireDrapeaux(racine, quete, rapport);
    lireEtapes(racine, quete, rapport);

    if (!rapport.vide()) {
        resultat.errors = rapport.extraire();
        return resultat;
    }
    resultat.quest = std::move(quete);
    return resultat;
}

QuestLoad loadQuest(const std::filesystem::path& path) {
    const std::optional<std::string> contenu = lireFichier(path);
    if (!contenu) {
        QuestLoad echec;
        echec.errors.push_back(path.string() + " : fichier absent ou illisible.");
        return echec;
    }
    // Le texte d'origine, pas un arbre réécrit : c'est lui qui porte les numéros de ligne.
    return readQuest(*contenu, path.string());
}

const Quest* QuestCatalog::find(std::string_view id) const {
    const auto trouve = std::ranges::find(quests, id, &Quest::id);
    return trouve == quests.end() ? nullptr : &*trouve;
}

const QuestFlag* QuestCatalog::findFlag(std::string_view flag) const {
    for (const Quest& quete : quests) {
        const auto trouve = std::ranges::find(quete.flags, flag, &QuestFlag::id);
        if (trouve != quete.flags.end()) {
            return &*trouve;
        }
    }
    return nullptr;
}

QuestCatalog loadQuests(const std::filesystem::path& directory) {
    QuestCatalog catalogue;
    std::error_code code;
    if (!std::filesystem::is_directory(directory, code)) {
        return catalogue;
    }
    std::vector<std::filesystem::path> fichiers;
    for (const auto& entree : std::filesystem::directory_iterator(directory, code)) {
        if (entree.is_regular_file() && entree.path().extension() == ".json") {
            fichiers.push_back(entree.path());
        }
    }
    std::ranges::sort(fichiers);

    std::set<std::string, std::less<>> drapeaux;
    for (const std::filesystem::path& fichier : fichiers) {
        QuestLoad lue = loadQuest(fichier);
        if (!lue.quest) {
            catalogue.errors.insert(catalogue.errors.end(), lue.errors.begin(), lue.errors.end());
            continue;
        }
        const std::string attendu = fichier.stem().string();
        if (lue.quest->id != attendu) {
            catalogue.errors.push_back(fichier.string() + " : l'identifiant '" + lue.quest->id +
                                       "' n'est pas le nom du fichier ('" + attendu + "').");
            continue;
        }
        if (catalogue.find(lue.quest->id) != nullptr) {
            catalogue.errors.push_back(fichier.string() + " : quete '" + lue.quest->id +
                                       "' en double.");
            continue;
        }
        bool doublon = false;
        for (const QuestFlag& drapeau : lue.quest->flags) {
            if (drapeaux.contains(drapeau.id)) {
                catalogue.errors.push_back(fichier.string() + " : le drapeau '" + drapeau.id +
                                           "' est deja declare par une autre quete.");
                doublon = true;
            }
        }
        if (doublon) {
            continue;
        }
        for (const QuestFlag& drapeau : lue.quest->flags) {
            drapeaux.insert(drapeau.id);
        }
        catalogue.quests.push_back(std::move(*lue.quest));
    }
    return catalogue;
}

void declareQuestFlags(const QuestCatalog& catalog, WorldFlags& flags) {
    for (const Quest& quete : catalog.quests) {
        for (const QuestFlag& drapeau : quete.flags) {
            flags.declare(drapeau.id, drapeau.values, drapeau.initial);
        }
    }
}

std::vector<QuestEvent> advanceQuests(const QuestCatalog& catalog, WorldFlags& flags) {
    std::vector<QuestEvent> evenements;
    bool bouge = true;
    while (bouge) {
        bouge = false;
        for (const Quest& quete : catalog.quests) {
            const QuestStatus statut = questProgress(quete, flags).status;
            if (statut == QuestStatus::Succeeded || statut == QuestStatus::Failed) {
                continue;
            }
            for (const QuestStep& etape : quete.steps) {
                const std::string fait = questStepFlag(quete.id, etape.id);
                if (flags.isSet(fait) ||
                    !std::ranges::all_of(etape.when, [&flags](const FlagCondition& condition) {
                        return condition.holds(flags);
                    })) {
                    continue;
                }
                flags.set(fait);
                for (const QuestEffect& effet : etape.effects) {
                    if (effet.kind == QuestEffect::Kind::ClearFlag) {
                        flags.clear(effet.flag);
                    } else if (effet.value.empty()) {
                        flags.set(effet.flag);
                    } else {
                        flags.setValue(effet.flag, effet.value);
                    }
                }
                evenements.push_back({quete.id, etape.id, etape.outcome});
                bouge = true;
                if (etape.outcome != QuestOutcome::None) {
                    break;
                }
            }
        }
    }
    return evenements;
}

QuestProgress questProgress(const Quest& quest, const WorldFlags& flags) {
    QuestProgress avancement;
    for (const QuestStep& etape : quest.steps) {
        if (!flags.isSet(questStepFlag(quest.id, etape.id))) {
            continue;
        }
        avancement.reachedSteps.push_back(etape.id);
        if (etape.outcome == QuestOutcome::Success) {
            avancement.status = QuestStatus::Succeeded;
        } else if (etape.outcome == QuestOutcome::Failure) {
            avancement.status = QuestStatus::Failed;
        } else if (avancement.status == QuestStatus::NotStarted) {
            avancement.status = QuestStatus::Active;
        }
    }
    return avancement;
}

namespace {

/// Confronte une valeur comparée ou posée à la déclaration du drapeau, s'il en a une.
void verifierUsage(const QuestCatalog& quetes, std::string_view drapeau,
                   const std::vector<std::string>& valeurs, bool poseSansValeur,
                   const std::string& ou, std::vector<std::string>& erreurs) {
    const QuestFlag* declaration = quetes.findFlag(drapeau);
    if (declaration == nullptr) {
        if (!valeurs.empty()) {
            erreurs.push_back(ou + " : le drapeau '" + std::string(drapeau) +
                              "' n'est declare par aucune quete ; il n'a pas de valeurs.");
        }
        return;
    }
    if (poseSansValeur) {
        erreurs.push_back(ou + " : le drapeau '" + std::string(drapeau) +
                          "' a des valeurs ; il se pose avec 'value'.");
    }
    for (const std::string& valeur : valeurs) {
        if (!contient(declaration->values, valeur)) {
            erreurs.push_back(ou + " : valeur '" + valeur + "' que le drapeau '" +
                              std::string(drapeau) + "' ne declare pas.");
        }
    }
}

}  // namespace

std::vector<std::string> validateFlagUses(const QuestCatalog& quests,
                                          const DialogueCatalog& dialogues) {
    std::vector<std::string> erreurs;
    for (const DialogueGraph& graphe : dialogues.dialogues) {
        for (const DialogueNode& noeud : graphe.nodes) {
            const std::string ou = "dialogue '" + graphe.id + "' : noeud '" + noeud.id + "'";
            if (noeud.kind == DialogueNodeKind::Condition) {
                verifierUsage(quests, noeud.condition.flag, noeud.condition.values, false, ou,
                              erreurs);
            }
            for (const DialogueChoice& choix : noeud.choices) {
                if (choix.condition) {
                    verifierUsage(quests, choix.condition->flag, choix.condition->values, false,
                                  ou + " / reponse '" + choix.id + "'", erreurs);
                }
            }
            for (const DialogueAction& action : noeud.actions) {
                if (action.kind != DialogueActionKind::SetFlag) {
                    continue;
                }
                std::vector<std::string> valeurs;
                if (!action.value.empty()) {
                    valeurs.push_back(action.value);
                }
                verifierUsage(quests, action.target, valeurs, action.value.empty(), ou, erreurs);
            }
        }
    }
    // Les quêtes entre elles : une étape peut lire ou poser le drapeau qu'une autre déclare.
    for (const Quest& quete : quests.quests) {
        for (const QuestStep& etape : quete.steps) {
            const std::string ou = "quete '" + quete.id + "' : etape '" + etape.id + "'";
            for (const FlagCondition& condition : etape.when) {
                verifierUsage(quests, condition.flag, condition.values, false, ou, erreurs);
            }
            for (const QuestEffect& effet : etape.effects) {
                if (effet.kind != QuestEffect::Kind::SetFlag) {
                    continue;
                }
                std::vector<std::string> valeurs;
                if (!effet.value.empty()) {
                    valeurs.push_back(effet.value);
                }
                verifierUsage(quests, effet.flag, valeurs, effet.value.empty(), ou, erreurs);
            }
        }
    }
    return erreurs;
}

std::set<std::string, std::less<>> flagsWrittenBy(const QuestCatalog& quests,
                                                  const DialogueCatalog& dialogues) {
    std::set<std::string, std::less<>> poses;
    for (const DialogueGraph& graphe : dialogues.dialogues) {
        for (const DialogueNode& noeud : graphe.nodes) {
            for (const DialogueAction& action : noeud.actions) {
                if (action.kind == DialogueActionKind::SetFlag) {
                    poses.insert(action.target);
                } else if (action.kind == DialogueActionKind::StartQuest) {
                    poses.insert(questStartedFlag(action.target));
                }
            }
        }
    }
    for (const Quest& quete : quests.quests) {
        for (const QuestFlag& drapeau : quete.flags) {
            poses.insert(drapeau.id);
        }
        for (const QuestStep& etape : quete.steps) {
            poses.insert(questStepFlag(quete.id, etape.id));
            for (const QuestEffect& effet : etape.effects) {
                if (effet.kind == QuestEffect::Kind::SetFlag) {
                    poses.insert(effet.flag);
                }
            }
        }
    }
    return poses;
}

std::vector<FlagRead> flagsReadBy(const QuestCatalog& quests, const DialogueCatalog& dialogues) {
    std::vector<FlagRead> lus;
    for (const DialogueGraph& graphe : dialogues.dialogues) {
        for (const DialogueNode& noeud : graphe.nodes) {
            const std::string ou = "dialogue '" + graphe.id + "' : noeud '" + noeud.id + "'";
            if (noeud.kind == DialogueNodeKind::Condition) {
                lus.push_back({noeud.condition.flag, ou});
            }
            for (const DialogueChoice& choix : noeud.choices) {
                if (choix.condition) {
                    lus.push_back({choix.condition->flag, ou + " / reponse '" + choix.id + "'"});
                }
            }
        }
    }
    for (const Quest& quete : quests.quests) {
        for (const QuestStep& etape : quete.steps) {
            for (const FlagCondition& condition : etape.when) {
                lus.push_back(
                    {condition.flag, "quete '" + quete.id + "' : etape '" + etape.id + "'"});
            }
        }
    }
    return lus;
}

}  // namespace core
