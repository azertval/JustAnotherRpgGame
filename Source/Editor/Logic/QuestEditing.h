// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Gameplay/Quest.h"
#include "Editor/Logic/MapRefactor.h"

/**
 * @file Editor/Logic/QuestEditing.h
 * @brief Le **mode Quêtes** de l'éditeur, sans fenêtre (`LOT-144`, `EX-EDIT-100`, décision D-24).
 *
 * Une quête s'écrit à côté des cartes qu'elle traverse. Ce fichier en tient la logique pure, que
 * la fenêtre et `LevelEditor` appellent de même (règle 4) :
 *
 * - **enregistrer** : le texte canonique (`core::writeQuest`), relu par `core::readQuest` et
 *   confronté aux autres quêtes et aux dialogues avant d'être écrit — une quête que le jeu
 *   refuserait ne s'écrit pas ; ses textes de journal vont dans chaque catalogue de langue ;
 * - **qui s'en sert** : pour un drapeau et ses valeurs, les entités des cartes, les dialogues et
 *   les quêtes qui le lisent ou le posent (`hmi::flagUses`) ;
 * - **renommer** un drapeau déclaré, une de ses valeurs, une quête : un plan, puis une écriture,
 *   comme `--rename-map` (`LOT-EDITOR-14`) ;
 * - **jouer une étape** : l'état de partie qui l'atteint (`hmi::worldStateReaching`).
 *
 * Le mode n'a pas sa propre idée d'une quête valide : le chargeur et la validation restent ceux de
 * `core`. Les cartes se récrivent par l'écrivain canonique, les quêtes par `core::writeQuest`, et
 * les dialogues — écrits à la main (D-24) — **chaîne par chaîne** : seul le texte de la valeur
 * renommée change, le reste du fichier garde ses octets.
 */

namespace hmi {

/// @brief Les textes d'une quête dans une langue : clé (`quest.pommes.title`) → texte.
using QuestLanguageTexts = std::map<std::string, std::string, std::less<>>;

/// @brief Les textes du journal d'une quête : langue (`fr`, `en`) → clé → texte.
using QuestTexts = std::map<std::string, QuestLanguageTexts, std::less<>>;

/// @brief Ce que le mode Quêtes édite : la quête, et ses textes de journal.
struct QuestDraft {
    core::Quest quest;
    QuestTexts texts;
};

/// @return Le fichier de la quête @p questId : `World/quests/<id>.json`.
[[nodiscard]] std::filesystem::path questFile(const std::filesystem::path& dataRoot,
                                              std::string_view questId);

/// @return Les identifiants des quêtes de @p dataRoot, triés (les fichiers, lisibles ou non).
[[nodiscard]] std::vector<std::string> questIds(const std::filesystem::path& dataRoot);

/// @brief Une quête lue pour l'édition, ou pourquoi elle ne l'est pas.
struct QuestDraftLoad {
    std::optional<QuestDraft> draft;
    std::vector<std::string> errors;
};

/// @return La quête @p questId et ses textes, lus dans chaque catalogue de langue.
[[nodiscard]] QuestDraftLoad loadQuestDraft(const std::filesystem::path& dataRoot,
                                            std::string_view questId);

/**
 * @return Vrai si @p id peut nommer une quête, une étape ou une valeur : non vide, lettres
 *         minuscules, chiffres, `-` et `_` — et, pour un drapeau (@p allowSlash), `.` et `/`
 *         (`quete.pommes`, `pommes/rendue`). Une clé de journal `quest.<quête>.<étape>` ne se
 *         confond ainsi avec aucune autre.
 */
[[nodiscard]] bool isValidQuestName(std::string_view id, bool allowSlash = false);

/**
 * @brief Le plan d'enregistrement de @p draft : son fichier canonique et ses textes.
 *
 * Refusé, sans rien écrire :
 * - un identifiant invalide, ou pris par une autre quête quand @p isNew ;
 * - une quête que `core::readQuest` refuse relue dans son texte canonique — une étape sans
 *   condition, une valeur que le drapeau ne déclare pas… ; l'erreur nomme l'étape ;
 * - un drapeau déjà déclaré par une autre quête ;
 * - un usage de drapeau que `core::validateFlagUses` refuse et qu'il ne refusait pas avant :
 *   comparer une valeur que le drapeau d'une autre quête ne déclare pas.
 *
 * Chaque catalogue reçoit le titre et le texte de chaque étape (`core::questTextKeys`) : une
 * clé déjà là change de texte à sa place, une nouvelle se range après les autres de la quête ; les
 * clés d'une étape retirée s'en vont. Un texte vide n'est pas écrit.
 *
 * @param dataRoot La racine des données.
 * @param draft    La quête et ses textes.
 * @param isNew    Vrai pour une quête qui n'existe pas encore.
 */
[[nodiscard]] RefactorPlan planSaveQuest(const std::filesystem::path& dataRoot,
                                         const QuestDraft& draft, bool isNew);

/// @brief Retire la quête @p questId : son fichier et ses clés de journal. Ce qui lit ses drapeaux
///        reste, et `--check` le dira ; le plan le cite.
[[nodiscard]] RefactorPlan planDeleteQuest(const std::filesystem::path& dataRoot,
                                           std::string_view questId);

/// @brief Ce qu'un usage fait du drapeau.
enum class FlagUseRole {
    /// Une quête le déclare, avec ses valeurs.
    Declares,
    /// Une condition le lit : une étape, une réponse, la présence d'une entité, un portail.
    Reads,
    /// Un effet, une action, un déclencheur le pose.
    Writes,
};

/// @brief Un endroit qui se sert d'un drapeau.
struct FlagUse {
    std::string flag;
    /// Les valeurs comparées, posées ou déclarées ; vide : le drapeau seul.
    std::vector<std::string> values;
    FlagUseRole role = FlagUseRole::Reads;
    /// Où : une entité de carte (`mapId`, `entityId`, case), un dialogue, une quête.
    Citation where;

    [[nodiscard]] bool operator==(const FlagUse&) const = default;
};

/**
 * @brief Tout ce qui se sert d'un drapeau, dans les cartes, les dialogues et les quêtes de
 *        @p dataRoot.
 *
 * Sur une carte, une propriété d'entité compte par sa **source** dans `core::knownEntityKinds` et
 * `core::commonEntityProperties` : `Flags` lit, `WrittenFlags` pose, et `FlagValues` porte les
 * valeurs du drapeau que nomme sa `relatedKey` — sans code par famille (règle 2). Une carte
 * illisible est passée.
 */
[[nodiscard]] std::vector<FlagUse> flagUses(const std::filesystem::path& dataRoot);

/// @return Les usages de @p uses qui nomment @p flag — et, si @p value n'est pas vide, cette
/// valeur.
[[nodiscard]] std::vector<FlagUse> usesOfFlag(const std::vector<FlagUse>& uses,
                                              std::string_view flag, std::string_view value = {});

/// @return Les usages en citations : `portal e2: requiresFlag (reads)`, pour `--who-cites flag`.
[[nodiscard]] std::vector<Citation> flagUseCitations(const std::vector<FlagUse>& uses);

/**
 * @brief Renomme le drapeau déclaré @p oldFlag en @p newFlag partout où il est lu ou posé.
 *
 * Refusé : un drapeau qu'aucune quête ne déclare — un fait de dialogue se renomme dans son
 * dialogue —, un nouveau nom invalide ou déjà employé, une carte illisible.
 */
[[nodiscard]] RefactorPlan planRenameFlag(const std::filesystem::path& dataRoot,
                                          std::string_view oldFlag, std::string_view newFlag);

/**
 * @brief Renomme la valeur @p oldValue du drapeau déclaré @p flag en @p newValue : la
 *        déclaration et son initiale, les conditions et les effets des quêtes, les dialogues, les
 *        entités des cartes (`a|b` compris).
 *
 * Seules les valeurs **de ce drapeau** changent : un nœud de dialogue ou une étape qui porte le
 * même mot garde son identifiant.
 */
[[nodiscard]] RefactorPlan planRenameFlagValue(const std::filesystem::path& dataRoot,
                                               std::string_view flag, std::string_view oldValue,
                                               std::string_view newValue);

/**
 * @brief Renomme la quête @p oldId en @p newId : son fichier, ses clés de journal (textes gardés),
 *        les dialogues qui la démarrent (`startQuest`), et les faits qu'elle pose
 *        (`quest/<id>/started`, `quest/<id>/step/<étape>`) là où ils sont lus.
 */
[[nodiscard]] RefactorPlan planRenameQuest(const std::filesystem::path& dataRoot,
                                           std::string_view oldId, std::string_view newId);

/**
 * @brief L'état de partie qui **atteint** l'étape @p stepId de @p quest : pour chaque condition,
 *        une entrée (`fait`, `drapeau=valeur`) qui la fait tenir.
 *
 * `equals` prend sa première valeur ; `notEquals` l'initiale si elle convient, sinon la première
 * valeur déclarée qui convient ; « non posé » n'écrit rien. C'est l'état que *World state…* montre
 * et que `P` et `F5` jouent (`LOT-126`).
 *
 * @param quest    La quête.
 * @param stepId   L'étape.
 * @param declared Les drapeaux que les quêtes déclarent (`EditorReferences::declaredFlags`).
 * @return Les entrées, dans l'ordre des conditions ; vide si l'étape n'existe pas.
 */
[[nodiscard]] std::vector<std::string> worldStateReaching(
    const core::Quest& quest, std::string_view stepId,
    const std::vector<core::QuestFlag>& declared);

/**
 * @brief Le lieu des étapes (`at`, `carte#id`) suit une entité ou une carte renommée
 *        (`LOT-EDITOR-14`) : chaque étape dont @p rename rend une valeur est citée dans @p plan,
 *        et, si @p write, sa quête récrite.
 * @param dataRoot La racine des données.
 * @param rename   Le nouveau lieu d'une étape, ou `std::nullopt` si elle ne cite pas ce qui change.
 * @param write    Faux : les citations seules (« qui cite ceci ? »).
 * @param plan     Le plan qui reçoit citations et écritures.
 */
void citeStepPlaces(const std::filesystem::path& dataRoot,
                    const std::function<std::optional<std::string>(std::string_view)>& rename,
                    bool write, RefactorPlan& plan);

/**
 * @brief L'entrée sans fenêtre du mode Quêtes :
 *
 * - `--who-cites flag <drapeau> [<valeur>]` ;
 * - `--rename-flag <ancien> <nouveau>`, `--rename-flag-value <drapeau> <ancienne> <nouvelle>`,
 *   `--rename-quest <ancienne> <nouvelle>` ;
 * - `--save-quest <brouillon.json>` : une quête, et ses textes sous `"journal": {"fr": {"title":
 *   …, "<étape>": …}}`, enregistrée comme le mode l'enregistre ;
 * - `--quest-state <quête> <étape>` : l'état de partie qui atteint l'étape, en `--flags=`.
 *
 * @return Le code de sortie, ou `std::nullopt` si aucune de ces commandes n'est demandée.
 */
[[nodiscard]] std::optional<int> runQuestCommand(const std::vector<std::string>& arguments,
                                                 const std::filesystem::path& dataRoot,
                                                 std::string& output);

}  // namespace hmi
