// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Presentation/DialogueScreen.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <numeric>

namespace hmi {

namespace {

// Remplace %1..%9 dans un gabarit traduit. L'ordre des mots change d'une langue a l'autre ; un
// gabarit par langue le permet, une concatenation en C++ l'interdirait.
[[nodiscard]] std::string remplir(std::string gabarit, const std::vector<std::string>& valeurs) {
    for (std::size_t i = 0; i < valeurs.size(); ++i) {
        const std::string marque = "%" + std::to_string(i + 1);
        for (std::size_t position = gabarit.find(marque); position != std::string::npos;
             position = gabarit.find(marque, position + valeurs[i].size())) {
            gabarit.replace(position, marque.size(), valeurs[i]);
        }
    }
    return gabarit;
}

/// « Persuasion · DD 15 », ou « Persuasion » seul si le degre n'a pas de seuil connu.
[[nodiscard]] std::string annonceDuJet(const TextLookup& text, const std::string& skill, int dc) {
    const std::string competence = text(skillLabelKey(skill));
    return dc > 0 ? remplir(text("dialogue.check.announce"), {competence, std::to_string(dc)})
                  : competence;
}

/// « 12 + 4 = 16 » : le de retenu, la somme des modificateurs, le total. Des chiffres et des
/// signes, que toutes les langues du jeu lisent pareil.
[[nodiscard]] std::string calculDuJet(const core::CheckResult& jet) {
    const int bonus = std::accumulate(
        jet.modifiers.begin(), jet.modifiers.end(), 0,
        [](int somme, const core::Modifier& modificateur) { return somme + modificateur.value; });
    return std::to_string(jet.keptDie) + (bonus < 0 ? " - " : " + ") +
           std::to_string(std::abs(bonus)) + " = " + std::to_string(jet.total);
}

}  // namespace

std::string skillLabelKey(std::string_view skillId) {
    std::string cle = "rpg.skill." + std::string(skillId);
    std::ranges::replace(cle, '-', '_');
    return cle;
}

DialogueScreenValues dialogueScreenValues(const core::DialogueRunner& runner,
                                          const TextLookup& text) {
    DialogueScreenValues valeurs;
    const core::DialogueGraph& graphe = runner.graph();
    valeurs.speakerName = text(core::dialogueSpeakerKey(graphe.id));
    valeurs.attitude = text(core::dialogueAttitudeKey(runner.attitude()));

    const auto quitter = [&text]() {
        return DialogueReply{
            .id = std::string(DIALOGUE_LEAVE_REPLY), .label = text("dialogue.leave"), .value = {}};
    };

    switch (runner.state()) {
        case core::DialogueState::Refused:
            valeurs.line = text("dialogue.refused");
            valeurs.replies.push_back(quitter());
            return valeurs;
        case core::DialogueState::Ended:
        case core::DialogueState::NotStarted:
            valeurs.finished = runner.state() == core::DialogueState::Ended;
            valeurs.replies.push_back(quitter());
            return valeurs;
        case core::DialogueState::AwaitingChoice:
            break;
    }

    valeurs.line = text(runner.lineKey());
    for (const core::AvailableChoice& choix : runner.choices()) {
        DialogueReply reponse{.id = choix.id, .label = text(choix.textKey), .value = {}};
        if (!choix.checkSkill.empty()) {
            // Annonce le jet AVANT qu'on le choisisse, comme une table l'annonce : un joueur qui
            // decouvre apres coup qu'il jouait sa Persuasion n'a pas choisi, il a subi. Et le
            // seuil avec (LOT-117) : DD 10 ou DD 25, ce n'est pas le meme pari.
            reponse.value = "[" + annonceDuJet(text, choix.checkSkill, choix.checkDc) + "]";
        }
        valeurs.replies.push_back(std::move(reponse));
    }

    if (const auto& jet = runner.lastCheck()) {
        const bool reussi = !jet->alreadyFailed && jet->result.succeeded();
        valeurs.checkSucceeded = reussi;
        valeurs.checkTitle = annonceDuJet(text, jet->skill, jet->result.target);
        valeurs.checkVerdict = text(reussi ? "dialogue.check.success" : "dialogue.check.failure");
        if (jet->alreadyFailed) {
            // Aucun de n'a roule : l'ecran le dit, plutot que d'inventer un tirage.
            valeurs.checkDetail = text("dialogue.check.already-failed");
            valeurs.checkOutcome =
                remplir(text("dialogue.check.repeat"),
                        {text(skillLabelKey(jet->skill)), std::to_string(jet->result.target),
                         valeurs.checkVerdict});
        } else {
            valeurs.checkDie = std::to_string(jet->result.keptDie);
            valeurs.checkDetail = calculDuJet(jet->result);
            valeurs.checkOutcome = remplir(
                text("dialogue.check.summary"),
                {text(skillLabelKey(jet->skill)), std::to_string(jet->result.target),
                 valeurs.checkDie, std::to_string(jet->result.total), valeurs.checkVerdict});
        }
    }
    return valeurs;
}

}  // namespace hmi
