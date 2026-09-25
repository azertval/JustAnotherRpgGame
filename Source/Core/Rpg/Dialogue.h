// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Rpg/Dialogue.h
 * @brief Les dialogues : un graphe de nœuds écrit en JSON, validé au chargement, et la machine à
 *        états pure qui le joue (`LOT-15`, `EX-VIS-003`, `EX-RPG-042`).
 */

#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Gameplay/FlagCondition.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Math/DeterministicRandom.h"
#include "Core/Rpg/Check.h"

namespace core {

class WorldFlags;
struct SkillCatalog;
struct CharacterSheet;
struct Inventory;
struct ExperienceTable;

// ---------------------------------------------------------------------------------------------
// Le graphe
// ---------------------------------------------------------------------------------------------

/**
 * @brief Ce que fait un nœud.
 *
 * Deux natures, et la différence compte pour la validation : une **réplique** attend le joueur,
 * les trois nœuds **automatiques** (condition, action, jet) s'enchaînent sans lui, et `End` clôt.
 * Un cycle qui ne passe par aucune réplique à choix tournerait sans fin — c'est la définition du
 * « cycle non intentionnel » que le chargement refuse.
 */
enum class DialogueNodeKind {
    /// L'interlocuteur parle ; le joueur répond par un choix, ou continue.
    Line,
    /// Un drapeau de monde oriente la suite.
    Condition,
    /// Des effets sur le monde : poser un drapeau, donner un objet, démarrer une quête.
    Action,
    /// Un jet de compétence contre un degré de difficulté, et ses deux suites.
    Check,
    /// La conversation se termine.
    End,
};

/**
 * @brief L'attitude de l'interlocuteur, dans le vocabulaire des règles : amical, indifférent,
 *        hostile (*Guide du Maître*, « Interactions sociales »).
 */
enum class DialogueAttitude {
    Friendly,
    Indifferent,
    Hostile,
};

/// @brief Une réponse proposée au joueur.
struct DialogueChoice {
    /// Identifiant, unique dans sa réplique : il fabrique la clé de traduction.
    std::string id;
    /// Nœud où mène la réponse.
    std::string next;
    /// Absente : la réponse est toujours proposée. Présente : proposée seulement si elle tient.
    std::optional<FlagCondition> condition;
};

/// @brief Ce qu'un nœud d'action fait au monde.
enum class DialogueActionKind {
    SetFlag,
    ClearFlag,
    /// Donne un objet à l'interlocuteur du PNJ (`core::DialogueListener::receiveItem`).
    GiveItem,
    /// Démarre une quête : pose `core::questStartedFlag(id)`, que le `LOT-16` lira.
    StartQuest,
    /// **Engage une rencontre sur la carte** (`LOT-118`) : le maître d'arène lance le combat nommé
    /// par `target`, ici même, sur la zone de combat de la carte
    /// (`core::DialogueListener::startEncounter`).
    StartEncounter,
    /// **Termine la démo** (`LOT-119`) : l'écran « Fin de la démo » s'ouvre, et dit la voie
    /// nommée par `target` (`core::demoEndingKey`). Le dialogue ne connaît pas l'écran ; il le
    /// demande (`core::DialogueListener::endDemo`).
    EndDemo,
};

/// @brief Un effet d'un nœud d'action.
struct DialogueAction {
    DialogueActionKind kind = DialogueActionKind::SetFlag;
    /// Le drapeau, l'objet ou la quête visé.
    std::string target;
    /// Pour `GiveItem` seulement.
    int quantity = 1;
    /// Pour `SetFlag` seulement : la valeur d'un drapeau **déclaré à valeurs** par une quête
    /// (`LOT-116`). Vide, le drapeau est un fait booléen.
    std::string value;
};

/**
 * @brief Un nœud du graphe.
 *
 * Une structure unique plutôt qu'une hiérarchie : les cinq natures partagent l'identifiant et la
 * suite, et un graphe se parcourt, se valide et se sérialise mieux à plat. Seuls les champs de sa
 * nature sont renseignés.
 */
struct DialogueNode {
    std::string id;
    DialogueNodeKind kind = DialogueNodeKind::End;

    /// `Line` : les réponses. Vide si la réplique n'a qu'une suite (`next`).
    std::vector<DialogueChoice> choices;
    /// `Line` sans réponses, et `Action` : le nœud suivant.
    std::string next;
    /// `Line` : l'attitude le temps de cette réplique ; absente, celle du graphe.
    std::optional<DialogueAttitude> attitude;

    /// `Condition` : ce qui est testé, et les deux suites.
    FlagCondition condition;
    std::string whenTrue;
    std::string whenFalse;

    /// `Action` : les effets, dans l'ordre.
    std::vector<DialogueAction> actions;

    /// `Check` : la compétence, le degré de difficulté (**nommé**, jamais un nombre), les suites.
    std::string skill;
    std::string difficulty;
    std::string onSuccess;
    std::string onFailure;
};

/**
 * @brief Un dialogue : l'interlocuteur et le graphe de sa conversation.
 *
 * **Le graphe ne porte aucun texte.** Chaque réplique, chaque réponse et le nom de
 * l'interlocuteur ont une clé **fabriquée** depuis les identifiants (`dialogueLineKey`,
 * `dialogueChoiceKey`, `dialogueSpeakerKey`), résolue dans le catalogue de traduction
 * (`EX-REN-033`). Un texte écrit dans le JSON serait du français en dur ; une clé écrite à la main
 * pourrait être fausse. Fabriquée, elle ne peut qu'exister ou manquer, et un test vérifie qu'elle
 * existe dans les deux langues.
 */
struct DialogueGraph {
    std::string id;
    /// Le nœud d'entrée.
    std::string start;
    /// Les langues de l'interlocuteur. Aucune en commun avec le personnage : dialogue refusé.
    std::vector<std::string> speakerLanguages;
    /// L'attitude de départ de l'interlocuteur.
    DialogueAttitude attitude = DialogueAttitude::Indifferent;
    std::vector<DialogueNode> nodes;

    /// @brief Le nœud portant cet identifiant, ou `nullptr`.
    [[nodiscard]] const DialogueNode* find(std::string_view nodeId) const;
};

/// @brief Identifiant de la réponse implicite d'une réplique sans choix : « Continuer ».
inline constexpr std::string_view DIALOGUE_CONTINUE_CHOICE = "continue";
/// @brief Clé de traduction de la réponse implicite.
inline constexpr std::string_view DIALOGUE_CONTINUE_KEY = "dialogue.continue";

/// @brief `dialogue.<dialogue>.speaker` — le nom de l'interlocuteur.
[[nodiscard]] std::string dialogueSpeakerKey(std::string_view dialogueId);
/// @brief `dialogue.<dialogue>.<nœud>` — le texte d'une réplique.
[[nodiscard]] std::string dialogueLineKey(std::string_view dialogueId, std::string_view nodeId);
/// @brief `dialogue.<dialogue>.<nœud>.<réponse>` — le texte d'une réponse.
[[nodiscard]] std::string dialogueChoiceKey(std::string_view dialogueId, std::string_view nodeId,
                                            std::string_view choiceId);
/// @brief La clé de traduction d'une voie de fin de la démo (`LOT-119`) : « ending. » suivi de
///        la voie. Son texte est ce que dit l'écran de fin, « par la voie de l'arène ».
[[nodiscard]] std::string demoEndingKey(std::string_view ending);
/// @brief `dialogue.attitude.<friendly|indifferent|hostile>`.
[[nodiscard]] std::string dialogueAttitudeKey(DialogueAttitude attitude);
/// @brief `friendly`, `indifferent`, `hostile` — le mot de la donnée.
[[nodiscard]] std::string_view dialogueAttitudeName(DialogueAttitude attitude) noexcept;

/**
 * @brief Toutes les clés de traduction qu'un graphe réclame, sans doublon.
 *
 * Celles qu'il **fabrique** — interlocuteur, répliques, réponses, attitudes employées — et la
 * réponse implicite s'il en a l'usage. C'est la liste qu'un test confronte aux deux catalogues.
 */
[[nodiscard]] std::vector<std::string> dialogueTextKeys(const DialogueGraph& graph);

/**
 * @brief `quest/<quête>/started` — le drapeau qu'une action « démarrer une quête » pose.
 *
 * Les quêtes lisent des drapeaux (`LOT-16`, note de conception) : démarrer une quête depuis un
 * dialogue n'a donc besoin d'aucun objet quête, et le journal du `LOT-16` n'aura qu'à lire ce
 * drapeau. Une clé fabriquée, pour que les deux lots ne puissent pas l'écrire différemment.
 */
[[nodiscard]] std::string questStartedFlag(std::string_view questId);

/**
 * @brief `dialogue/<dialogue>/<jet>/failed` — le drapeau qu'un jet **raté** pose (`LOT-117`).
 *
 * Un jet raté ne se retente pas : à une table, le garde qui a dit non ne se laisse pas convaincre
 * par la même tirade cinq minutes plus tard. Le runner pose ce drapeau à l'échec, retire les
 * réponses qui mènent à ce jet, et un jet atteint de nouveau par un autre chemin échoue sans
 * relancer le dé. Un drapeau de monde, et non une mémoire du runner : il survit à la conversation,
 * et une quête peut le lire. Fabriqué, pour qu'aucun auteur n'ait à l'écrire.
 */
[[nodiscard]] std::string dialogueCheckFailedFlag(std::string_view dialogueId,
                                                  std::string_view checkNodeId);

// ---------------------------------------------------------------------------------------------
// Le chargement, et ce qu'il refuse
// ---------------------------------------------------------------------------------------------

/**
 * @brief Un dialogue lu, ou ce qui l'en empêche.
 *
 * **Un graphe mal formé n'est pas un graphe** : dès qu'une erreur est relevée, `graph` reste vide.
 * Toutes les erreurs sont listées d'un coup, chacune nommant son fichier et son nœud — un auteur
 * qui corrige une faute pour découvrir la suivante au chargement d'après perd son après-midi.
 */
struct DialogueLoad {
    std::optional<DialogueGraph> graph;
    std::vector<std::string> errors;
};

/**
 * @brief Lit et **valide** un dialogue.
 *
 * Refusé au chargement, jamais découvert en jeu :
 *
 * - un champ manquant, une nature de nœud inconnue, deux nœuds de même identifiant ;
 * - un **nœud cible inconnu** — entrée, réponse, suite, branche de condition ou de jet ;
 * - un **choix vide** : une réplique dont la liste de réponses existe mais est vide, une réponse
 *   sans identifiant, deux réponses de même identifiant, ou une réplique dont **toutes** les
 *   réponses sont conditionnelles — que les drapeaux les masquent toutes, et le joueur resterait
 *   devant une réplique sans issue ;
 * - un **cycle non intentionnel** : une boucle qui ne passe par aucune réplique à réponses. Une
 *   boucle qui revient à une réplique à réponses est un « hub » voulu (« Autre chose ? ») ; une
 *   boucle de conditions et d'actions tournerait sans fin, et une boucle de répliques sans réponse
 *   enfermerait le joueur dans un monologue ;
 * - un **nœud orphelin**, que rien n'atteint depuis l'entrée — presque toujours une faute de
 *   frappe dans une cible ;
 * - une **impasse** : un nœud d'où aucune fin n'est atteignable ;
 * - une **réponse à jet sans branche d'échec** (`LOT-117`) : un jet sans `failure`, ou dont
 *   l'échec mène où mène la réussite — le jet ne déciderait rien ;
 * - une réplique dont **toutes** les réponses peuvent disparaître : une réponse qui mène à un jet
 *   n'est plus proposée une fois ce jet raté (`core::dialogueCheckFailedFlag`), et compte donc
 *   comme conditionnelle.
 *
 * @param json   Le texte du document.
 * @param origin Le nom du fichier, pour les messages.
 */
[[nodiscard]] DialogueLoad readDialogue(std::string_view json, std::string_view origin);

/// @brief `readDialogue` depuis un fichier.
[[nodiscard]] DialogueLoad loadDialogue(const std::filesystem::path& path);

/**
 * @brief Les catalogues contre lesquels un dialogue vérifie ce qu'il **nomme**.
 *
 * Un catalogue absent (pointeur nul, fonction vide) n'est pas vérifié : le test d'un graphe n'a
 * pas à charger les deux cents objets du jeu pour éprouver une condition.
 */
struct DialogueReferences {
    const SkillCatalog* skills = nullptr;
    const DifficultyScale* difficulty = nullptr;
    std::function<bool(std::string_view)> itemExists;
    std::function<bool(std::string_view)> languageExists;
};

/**
 * @brief Les références d'un graphe que les catalogues ne connaissent pas.
 *
 * Une compétence mal orthographiée, un degré de difficulté inventé, un objet ou une langue
 * inconnus : chacun ferait un jet contre rien ou un cadeau vide, ce qui se joue et ne se voit pas.
 */
[[nodiscard]] std::vector<std::string> validateDialogueReferences(
    const DialogueGraph& graph, const DialogueReferences& references);

/// @brief Les dialogues d'un dossier, et ce qui n'a pas pu l'être.
struct DialogueCatalog {
    std::vector<DialogueGraph> dialogues;
    std::vector<std::string> errors;

    /// @brief Le dialogue portant cet identifiant, ou `nullptr`.
    [[nodiscard]] const DialogueGraph* find(std::string_view id) const;
};

/**
 * @brief Charge les dialogues de @p directory (`Source/Elements/World/dialogues`).
 *
 * Un fichier refusé n'empêche pas les autres de se charger ; son identifiant doit être le nom du
 * fichier, pour qu'un dialogue se retrouve depuis la carte qui le nomme. Ne lève jamais.
 */
[[nodiscard]] DialogueCatalog loadDialogues(const std::filesystem::path& directory);

// ---------------------------------------------------------------------------------------------
// Qui écoute
// ---------------------------------------------------------------------------------------------

/**
 * @brief Celui qui parle **au** PNJ, tel que le runner a besoin de le connaître.
 *
 * Trois questions et un geste, rien de plus : le runner ne voit ni fiche, ni inventaire, ni groupe.
 * Le jour où le groupe de quatre existera (`LOT-29`), « connaît-il cette langue » deviendra « l'un
 * d'eux la connaît-il » dans une autre implémentation, et le runner n'en saura rien.
 */
class DialogueListener {
public:
    DialogueListener() = default;
    virtual ~DialogueListener() = default;
    DialogueListener(const DialogueListener&) = delete;
    DialogueListener& operator=(const DialogueListener&) = delete;
    DialogueListener(DialogueListener&&) = delete;
    DialogueListener& operator=(DialogueListener&&) = delete;

    /// @return Vrai s'il parle cette langue.
    [[nodiscard]] virtual bool speaks(std::string_view languageId) const = 0;
    /// @return Les modificateurs d'un jet de cette compétence, avec leur origine.
    [[nodiscard]] virtual std::vector<Modifier> skillModifiers(std::string_view skillId) const = 0;
    /// Reçoit un objet que le PNJ lui donne.
    virtual void receiveItem(std::string_view itemId, int quantity) = 0;
    /// Le PNJ engage la rencontre @p encounterId sur la carte (`LOT-118`). Sans effet par défaut :
    /// un interlocuteur qui n'a pas d'écran — un test, un rejeu — n'a rien à ouvrir, et l'action
    /// reste consignée au journal du runner.
    virtual void startEncounter(std::string_view encounterId) {
        static_cast<void>(encounterId);
    }
    /// Le PNJ clôt la démo par la voie @p ending (`LOT-119`). Sans effet par défaut, pour la même
    /// raison que `startEncounter`.
    virtual void endDemo(std::string_view ending) {
        static_cast<void>(ending);
    }
};

/**
 * @brief Un personnage, sa fiche et son sac, comme interlocuteur d'un PNJ.
 *
 * Le modificateur de compétence est celui de la fiche (`core::skillModifier`) : caractéristique,
 * plus la maîtrise si la compétence est maîtrisée — une seule règle pour la fiche affichée et pour
 * le jet joué.
 */
class CharacterListener final : public DialogueListener {
public:
    CharacterListener(const CharacterSheet& sheet, Inventory& inventory,
                      const ExperienceTable& experience, const SkillCatalog& skills);

    [[nodiscard]] bool speaks(std::string_view languageId) const override;
    [[nodiscard]] std::vector<Modifier> skillModifiers(std::string_view skillId) const override;
    void receiveItem(std::string_view itemId, int quantity) override;

private:
    const CharacterSheet& _sheet;
    Inventory& _inventory;
    const ExperienceTable& _experience;
    const SkillCatalog& _skills;
};

// ---------------------------------------------------------------------------------------------
// Le runner
// ---------------------------------------------------------------------------------------------

/// @brief Où en est une conversation.
enum class DialogueState {
    /// `start` n'a pas encore été appelé.
    NotStarted,
    /// Une réplique est affichée, le joueur doit répondre.
    AwaitingChoice,
    /// La conversation a atteint une fin.
    Ended,
    /// Elle n'a pas eu lieu : aucune langue commune (`EX-RPG-042`).
    Refused,
};

/// @brief Une réponse que le joueur peut donner **maintenant**, conditions évaluées.
struct AvailableChoice {
    std::string id;
    std::string textKey;
    /// Si la réponse mène à un jet : la compétence jetée — l'écran l'annonce avant qu'on la
    /// choisisse, comme une table l'annonce (« Persuasion · DD 15 »). Vide sinon.
    std::string checkSkill;
    /// Si la réponse mène à un jet : son degré de difficulté, en nombre (`LOT-117`). Le contenu
    /// le **nomme** ; le joueur, lui, voit le seuil qu'il doit atteindre. 0 sinon, ou si le degré
    /// est inconnu.
    int checkDc = 0;
};

/// @brief Ce qu'une réponse a produit.
enum class ChoiceResult {
    /// La conversation a avancé.
    Advanced,
    /// Aucune réponse proposée ne porte cet identifiant — ou sa condition ne tient plus.
    Unavailable,
    /// Aucune réplique n'attend de réponse.
    NotAwaiting,
};

/// @brief Un jet de compétence joué en conversation, de quoi le restituer (`EX-REG-003`).
struct DialogueCheck {
    std::string nodeId;
    std::string skill;
    std::string difficulty;
    CheckResult result;
    /// Vrai si le jet n'a **pas** été lancé : déjà raté une fois, il échoue sans dé
    /// (`core::dialogueCheckFailedFlag`). `result` ne porte alors que le seuil.
    bool alreadyFailed = false;
};

/**
 * @brief La machine à états qui **joue** un dialogue.
 *
 * ## Pure, et c'est tout son intérêt
 *
 * Elle ne connaît ni Qt, ni le rendu, ni le monde : elle consomme un graphe, des drapeaux, un
 * interlocuteur et une suite aléatoire, et produit une réplique courante et des réponses. Un
 * dialogue se joue donc **en test**, nœud par nœud, sans fenêtre : c'est ce qui permet de vérifier
 * un arbre de vingt nœuds sans le cliquer. L'écran n'affiche que ce qu'elle décide.
 *
 * ## Le joueur ne s'arrête que sur une réplique
 *
 * Conditions, actions et jets s'enchaînent **dans le même appel** jusqu'à la prochaine réplique ou
 * à la fin : l'écran ne voit jamais un nœud automatique, et une conversation n'a que deux états
 * observables — une réplique qui attend, ou une fin.
 *
 * ## Rejouable
 *
 * La suite aléatoire est **fournie** et le runner n'en crée aucune : à graine égale, les mêmes
 * réponses donnent les mêmes jets. Le journal (`journal`) retrace chaque nœud traversé, chaque
 * effet et chaque jet détaillé — c'est lui qu'un test compare.
 *
 * Les références (graphe, drapeaux, interlocuteur, échelle, suite) doivent survivre au runner.
 */
class DialogueRunner {
public:
    /**
     * @brief Prépare un runner sur @p graph sans le démarrer (`start`).
     * @param graph      Le graphe à jouer.
     * @param flags      Les drapeaux du monde, lus par les conditions et écrits par les actions.
     * @param listener   L'interlocuteur qui répond aux jets et aux effets.
     * @param difficulty L'échelle qui traduit une difficulté nommée en cible de jet.
     * @param random     La suite aléatoire des jets, fournie pour le rejeu.
     */
    DialogueRunner(const DialogueGraph& graph, WorldFlags& flags, DialogueListener& listener,
                   const DifficultyScale& difficulty, DeterministicRandom& random);

    /**
     * @brief Ouvre la conversation.
     *
     * **Refusée** si l'interlocuteur ne parle aucune des langues du PNJ : rien n'est alors joué,
     * aucun drapeau n'est posé. Sinon, avance jusqu'à la première réplique.
     * Sans effet si la conversation est déjà ouverte.
     */
    DialogueState start();

    /// @brief Donne la réponse @p choiceId, puis avance jusqu'à la réplique suivante ou la fin.
    ChoiceResult choose(std::string_view choiceId);

    /// @brief L'état observable de la conversation (`core::DialogueState`).
    [[nodiscard]] DialogueState state() const noexcept {
        return _state;
    }

    /// @return La réplique affichée, ou `nullptr` si aucune n'attend.
    [[nodiscard]] const DialogueNode* currentLine() const;
    /// @return La clé de traduction de la réplique affichée, ou une chaîne vide.
    [[nodiscard]] std::string lineKey() const;
    /// @return L'attitude courante : celle de la réplique si elle en déclare une, sinon celle du
    ///         graphe.
    [[nodiscard]] DialogueAttitude attitude() const;
    /// @return Les réponses proposées **maintenant**, dans l'ordre de la donnée, conditions
    ///         évaluées. Vide si aucune réplique n'attend.
    [[nodiscard]] std::vector<AvailableChoice> choices() const;
    /// @return Le jet de compétence joué par le **dernier geste** (ouverture ou réponse), s'il y
    ///         en a eu un — l'écran l'annonce sur la réplique qui en découle, pas sur les
    ///         suivantes.
    [[nodiscard]] const std::optional<DialogueCheck>& lastCheck() const noexcept {
        return _lastCheck;
    }
    /// @return Le journal de la conversation, une ligne par nœud, effet ou jet.
    [[nodiscard]] const std::vector<std::string>& journal() const noexcept {
        return _journal;
    }
    /// @brief Le graphe que ce runner joue.
    [[nodiscard]] const DialogueGraph& graph() const noexcept {
        return _graph;
    }

private:
    /// Vrai si @p choice est proposée maintenant : sa condition tient, et le jet où elle mène
    /// n'a pas déjà été raté.
    [[nodiscard]] bool isOffered(const DialogueChoice& choice) const;
    void advanceTo(const std::string& nodeId);
    void apply(const DialogueAction& action);
    void runCheck(const DialogueNode& node);

    const DialogueGraph& _graph;
    WorldFlags& _flags;
    DialogueListener& _listener;
    const DifficultyScale& _difficulty;
    DeterministicRandom& _random;

    DialogueState _state = DialogueState::NotStarted;
    const DialogueNode* _current = nullptr;
    std::optional<DialogueCheck> _lastCheck;
    std::vector<std::string> _journal;
    /// Nœuds automatiques traversés depuis le dernier geste — la garde contre une donnée qui aurait
    /// échappé à la validation.
    std::size_t _automaticSteps = 0;
};

// ---------------------------------------------------------------------------------------------
// Sur la carte
// ---------------------------------------------------------------------------------------------

/// @brief Type d'entité de carte d'un PNJ.
inline constexpr std::string_view NPC_ENTITY_TYPE = "npc";
/// @brief Propriété d'un PNJ qui nomme son dialogue.
inline constexpr std::string_view NPC_DIALOGUE_PROPERTY = "dialogue";
/// @brief Propriété d'un PNJ qui nomme sa **figurine** (`Assets/Npc/<slug>`, atelier du `LOT-91`,
///        lue au `LOT-09`) — ou, avec une barre, un dossier depuis `Assets/` : les sentinelles
///        Ironhand portent `Monsters/ironhand-soldier` (`LOT-93`). Vide : le PNJ n'est pas encore
///        dessiné, et ne se dessine pas.
inline constexpr std::string_view NPC_FIGURE_PROPERTY = "figure";
/// @brief Propriété d'un PNJ **sentinelle** : la fiche d'atlas du quartier dont il garde la porte
///        (`LOT-96`). Vide pour un PNJ qui ne garde rien.
inline constexpr std::string_view NPC_GUARDED_DISTRICT_PROPERTY = "guards";

/// @brief Un PNJ relevé sur la carte, et le dialogue qu'il ouvre.
struct DialogueTrigger {
    std::string dialogueId;
    GridPosition position;
};

/**
 * @brief Lit @p entity comme un PNJ à qui parler, s'il en est un.
 *
 * Un PNJ **sans** dialogue n'est pas un déclencheur : il se voit et ne répond pas. Le refuser
 * comme carte invalide ferait disparaître un figurant dont le dialogue n'est pas encore écrit.
 *
 * @return Le déclencheur, ou `std::nullopt` si l'entité n'est pas un PNJ ou ne nomme aucun
 *         dialogue.
 */
[[nodiscard]] std::optional<DialogueTrigger> dialogueTriggerFor(const MapEntity& entity);

}  // namespace core
