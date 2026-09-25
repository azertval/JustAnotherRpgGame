// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Rpg/Dialogue.h"

/**
 * @file HMI/Presentation/DialogueScreen.h
 * @brief Ce que l'écran de dialogue affiche, lu d'un `core::DialogueRunner` hors de tout widget
 *        (`LOT-15`).
 */

namespace hmi {

/// @brief Identifiant de la ligne « Quitter la conversation », quand plus rien n'attend de réponse.
inline constexpr std::string_view DIALOGUE_LEAVE_REPLY = "leave";

/// @brief Une ligne de la liste des réponses : identifiant, texte, et le jet qu'elle annonce.
struct DialogueReply {
    std::string id;
    std::string label;
    /// « [Persuasion · DD 15] » si la réponse mène à un jet (`LOT-117`), vide sinon.
    std::string value;
};

/// @brief L'écran de dialogue, tout traduit.
struct DialogueScreenValues {
    std::string speakerName;
    std::string attitude;
    std::string line;
    /// Le jet que la dernière réponse a joué, restitué d'une ligne (« Persuasion · DD 15 — d20 :
    /// 12, total 16 — réussite »). Vide si le dernier geste n'a rien jeté.
    std::string checkOutcome;
    /// Le même jet, par morceaux, pour que l'écran le **montre** (`LOT-117`) : ce qui était jeté
    /// (« Persuasion · DD 15 »), le dé tiré (« 12 », vide s'il ne l'a pas été), le calcul
    /// (« 12 + 4 = 16 ») et l'issue (« réussite »).
    std::string checkTitle;
    std::string checkDie;
    std::string checkDetail;
    std::string checkVerdict;
    bool checkSucceeded = false;
    std::vector<DialogueReply> replies;
    /// Vrai quand la conversation a atteint une fin : l'écran se referme.
    bool finished = false;
};

/// @brief Une clé de traduction, et le texte qu'elle donne dans la langue active.
using TextLookup = std::function<std::string(std::string_view key)>;

/**
 * @brief Traduit l'état d'un runner en valeurs affichables.
 *
 * Logique **pure**, sans Qt ni disque : c'est ici que se décide ce qu'un refus faute de langue
 * montre, qu'une réponse annonce son jet, et qu'un jet se restitue sur la réplique qui en découle
 * — et un test le vérifie sans fenêtre. Le runner décide ; ceci ne fait que lire.
 *
 * - **Réplique en attente** : son texte, ses réponses dans l'ordre de la donnée.
 * - **Refus** (`EX-RPG-042`) : le texte du refus, et une seule réponse, « Quitter ».
 * - **Fin** : `finished`, et une seule réponse, « Quitter » — l'écran se referme de lui-même, la
 *   réponse n'est qu'un filet si un écran le garde ouvert.
 *
 * @param runner La conversation.
 * @param text   Le catalogue de traduction.
 */
[[nodiscard]] DialogueScreenValues dialogueScreenValues(const core::DialogueRunner& runner,
                                                        const TextLookup& text);

/// @brief `rpg.skill.animal_handling` pour `animal-handling` : la clé de lexique d'une compétence.
[[nodiscard]] std::string skillLabelKey(std::string_view skillId);

}  // namespace hmi
