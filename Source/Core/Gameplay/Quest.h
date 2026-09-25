// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Gameplay/Quest.h
 * @brief Les quêtes : décrites en données (drapeaux déclarés, étapes, conditions, effets), lues et
 *        validées au démarrage, avancées par les drapeaux de monde (`LOT-116`, `EX-EXP-007`,
 *        `EX-EXP-008`).
 */

#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Gameplay/FlagCondition.h"

namespace core {

class WorldFlags;
struct DialogueCatalog;

/**
 * @brief Un drapeau **à valeurs** qu'une quête déclare : `quete.pommes`, de `inconnue` à
 *        `enfant-libere`.
 *
 * La déclaration est ce qui **type** le drapeau : `core::WorldFlags` refuse ensuite une valeur hors
 * de la liste, et le chargement refuse un dialogue ou une quête qui en compare ou en pose une.
 */
struct QuestFlag {
    std::string id;
    std::vector<std::string> values;
    std::string initial;
};

/// @brief Ce qu'une étape fait au monde quand elle est atteinte.
struct QuestEffect {
    /// @brief La nature de l'effet : poser ou effacer un drapeau.
    enum class Kind {
        /// Pose un fait booléen, ou donne une valeur à un drapeau déclaré (`value`).
        SetFlag,
        /// Efface un fait ; un drapeau déclaré revient à sa valeur initiale.
        ClearFlag,
    };
    Kind kind = Kind::SetFlag;
    std::string flag;
    std::string value;
};

/// @brief Une étape qui **clôt** la quête, et comment.
enum class QuestOutcome {
    None,
    Success,
    Failure,
};

/**
 * @brief Une étape : l'entrée du journal qu'elle ouvre, ce qui l'atteint, ce qu'elle déclenche.
 *
 * **Une étape se lit dans le monde, elle ne s'ordonne pas.** Elle est atteinte dès que **toutes**
 * ses conditions tiennent, et le reste — la quête ne revient pas en arrière. C'est ce qui permet
 * les embranchements sans graphe : « persuadé » et « condamné » sont deux étapes que des valeurs
 * différentes du même drapeau atteignent, et le journal ne montre que celle qui l'a été.
 */
struct QuestStep {
    std::string id;
    /// Toutes doivent tenir. Jamais vide.
    std::vector<FlagCondition> when;
    std::vector<QuestEffect> effects;
    QuestOutcome outcome = QuestOutcome::None;
};

/// @brief Une quête : ses drapeaux et ses étapes, dans l'ordre du récit.
struct Quest {
    std::string id;
    std::vector<QuestFlag> flags;
    std::vector<QuestStep> steps;

    /// @brief L'étape portant cet identifiant, ou `nullptr`.
    [[nodiscard]] const QuestStep* find(std::string_view stepId) const;
};

/// @brief `quest/<quête>/step/<étape>` — le fait « cette étape a été atteinte ».
[[nodiscard]] std::string questStepFlag(std::string_view questId, std::string_view stepId);
/// @brief `quest.<quête>.title` — le titre de la quête dans le journal.
[[nodiscard]] std::string questTitleKey(std::string_view questId);
/// @brief `quest.<quête>.<étape>` — l'entrée du journal qu'ouvre une étape.
[[nodiscard]] std::string questStepKey(std::string_view questId, std::string_view stepId);
/// @brief Les clés de traduction qu'une quête réclame : son titre, puis une par étape.
[[nodiscard]] std::vector<std::string> questTextKeys(const Quest& quest);

/// @brief Une quête lue, ou ce qui l'en empêche. Mal formée, elle n'est pas une quête.
struct QuestLoad {
    std::optional<Quest> quest;
    /// Chacune nomme le fichier **et la ligne** : `quetes/pommes.json:14 : …`.
    std::vector<std::string> errors;
};

/**
 * @brief Lit et **valide** une quête.
 *
 * Refusé au chargement : un JSON malformé, un champ manquant ou du mauvais type ; un drapeau
 * déclaré sans valeur, avec une valeur en double ou une initiale hors de la liste ; deux étapes de
 * même identifiant, une étape sans condition, une condition ou un effet qui compare ou pose une
 * valeur qu'un drapeau **de la quête** ne déclare pas, une issue autre que `success`/`failure`.
 * Toutes les erreurs sont listées d'un coup.
 *
 * @param json   Le texte du document.
 * @param origin Le nom du fichier, pour les messages.
 */
[[nodiscard]] QuestLoad readQuest(std::string_view json, std::string_view origin);

/// @brief `readQuest` depuis un fichier.
[[nodiscard]] QuestLoad loadQuest(const std::filesystem::path& path);

/// @brief Toutes les quêtes d'un dossier, et ce qui a été refusé.
struct QuestCatalog {
    std::vector<Quest> quests;
    std::vector<std::string> errors;

    /// @brief La quête portant cet identifiant, ou `nullptr`.
    [[nodiscard]] const Quest* find(std::string_view id) const;
    /// @brief La déclaration d'un drapeau par l'une des quêtes, ou `nullptr`.
    [[nodiscard]] const QuestFlag* findFlag(std::string_view flag) const;
};

/**
 * @brief Lit les quêtes `*.json` de @p directory, dans l'ordre des noms.
 *
 * Refusées en plus de `readQuest` : un fichier dont le nom n'est pas l'identifiant, deux quêtes de
 * même identifiant, un drapeau déclaré par deux quêtes. Un dossier **absent** donne un catalogue
 * vide sans erreur : un jeu sans quête est un jeu.
 */
[[nodiscard]] QuestCatalog loadQuests(const std::filesystem::path& directory);

/// @brief Déclare à @p flags les drapeaux à valeurs de toutes les quêtes du catalogue.
void declareQuestFlags(const QuestCatalog& catalog, WorldFlags& flags);

/// @brief Une étape que `advanceQuests` vient d'atteindre.
struct QuestEvent {
    std::string quest;
    std::string step;
    QuestOutcome outcome = QuestOutcome::None;

    [[nodiscard]] bool operator==(const QuestEvent&) const = default;
};

/**
 * @brief Fait avancer les quêtes jusqu'au repos.
 *
 * Chaque étape non atteinte d'une quête non close dont les conditions tiennent est atteinte :
 * son fait `questStepFlag` est posé, ses effets appliqués, un événement rendu. Un effet peut en
 * atteindre une autre, d'où la répétition jusqu'à ce que rien ne bouge — ce qui termine toujours,
 * puisqu'une étape n'est atteinte qu'une fois.
 *
 * À appeler après tout ce qui écrit un drapeau : un dialogue, une interaction, un combat.
 */
std::vector<QuestEvent> advanceQuests(const QuestCatalog& catalog, WorldFlags& flags);

/// @brief Où en est une quête, telle que le journal la montre.
enum class QuestStatus {
    NotStarted,
    Active,
    Succeeded,
    Failed,
};

/// @brief Une quête vue du journal.
struct QuestProgress {
    QuestStatus status = QuestStatus::NotStarted;
    /// Les étapes atteintes, dans l'ordre du récit — la dernière est l'objectif du moment.
    std::vector<std::string> reachedSteps;
};

/// @brief L'avancement d'une quête : lu dans les drapeaux, jamais stocké à part.
[[nodiscard]] QuestProgress questProgress(const Quest& quest, const WorldFlags& flags);

/**
 * @brief Ce que dialogues et quêtes font d'un drapeau déclaré, confronté à sa déclaration.
 *
 * Un dialogue qui compare `quete.pommes` à `accepte` (sans `e`) serait une réponse qui ne paraît
 * jamais ; qui pose `quete.pommes` sans valeur, un geste que `core::WorldFlags` refuse en silence.
 * Les deux se refusent ici, au démarrage, fichier et nœud nommés.
 *
 * @param quests    Les quêtes, et leurs déclarations.
 * @param dialogues Les dialogues chargés.
 * @return Les erreurs, vides si tout est cohérent.
 */
[[nodiscard]] std::vector<std::string> validateFlagUses(const QuestCatalog& quests,
                                                        const DialogueCatalog& dialogues);

/**
 * @brief Les drapeaux que dialogues et quêtes **posent** : `setFlag`, le drapeau d'une quête
 *        démarrée, les effets des étapes, le fait de chaque étape atteinte — et les drapeaux que
 *        les quêtes déclarent, qui ont toujours une valeur, l'initiale.
 */
[[nodiscard]] std::set<std::string, std::less<>> flagsWrittenBy(const QuestCatalog& quests,
                                                                const DialogueCatalog& dialogues);

/// @brief Un drapeau qu'un dialogue ou une quête **lit**, et où.
struct FlagRead {
    std::string flag;
    /// `dialogue 'x' : noeud 'y'`, `quete 'x' : etape 'y'`.
    std::string where;

    [[nodiscard]] bool operator==(const FlagRead&) const = default;
};

/// @brief Les drapeaux que lisent les conditions des dialogues et des étapes de quête.
[[nodiscard]] std::vector<FlagRead> flagsReadBy(const QuestCatalog& quests,
                                                const DialogueCatalog& dialogues);

}  // namespace core
