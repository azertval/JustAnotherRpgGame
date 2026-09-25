// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/DialogueModel.h"

#include <QStringList>
#include <QVector>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "Core/Gameplay/WorldFlags.h"
#include "Core/Math/DeterministicRandom.h"
#include "Core/Rpg/Check.h"
#include "Core/Rpg/Dialogue.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Presentation/DialogueScreen.h"
#include "HMI/Runtime/DemonstrationCharacter.h"
#include "HMI/Runtime/RuleLabels.h"
#include "HMI/Runtime/WorldModel.h"

namespace hmi {

namespace {

[[nodiscard]] QString toQt(const std::string& texte) {
    return QString::fromStdString(texte);
}

/**
 * Les drapeaux de monde des conversations : ceux de la partie (`LOT-116`), que la carte lit aussi
 * -- une porte ouverte par un dialogue s'ouvre sur la carte, un PNJ appele par une quete y parait.
 * Sans partie (le designer, un test de l'ecran seul), un ensemble le temps du processus, pour
 * qu'un heraut n'oublie pas qu'on lui a parle a chaque ouverture de l'ecran.
 */
[[nodiscard]] core::WorldFlags& drapeauxDeLaPartie() {
    if (WorldModel* const partie = WorldModel::current()) {
        return partie->flags();
    }
    static core::WorldFlags drapeaux;
    return drapeaux;
}

/**
 * Une graine par conversation, tiree d'un compteur et non de l'horloge (EX-NFR-002) : deux
 * lancements du jeu rejouent les memes des dans le meme ordre de conversations.
 */
[[nodiscard]] std::uint64_t graineSuivante() {
    static std::uint64_t compteur = 0;
    return core::deriveSeed(0x15D1A106ULL, compteur++, 0);
}

/// Garde les dialogues dont toutes les références se résolvent ; journalise les autres.
[[nodiscard]] std::vector<core::DialogueGraph> validDialogues(
    std::vector<core::DialogueGraph> graphes, const core::DialogueReferences& references) {
    std::vector<core::DialogueGraph> valides;
    for (core::DialogueGraph& graphe : graphes) {
        const std::vector<std::string> erreurs =
            core::validateDialogueReferences(graphe, references);
        for (const std::string& error : erreurs) {
            HMI_LOG_WARNING("Dialogue : " + error);
        }
        if (erreurs.empty()) {
            valides.push_back(std::move(graphe));
        }
    }
    return valides;
}

/**
 * @brief L'interlocuteur du PNJ, tel que l'ECRAN l'entend : le personnage, et un rappel de plus.
 *
 * `core::CharacterListener` est `final` -- et c'est bien : ce qu'il sait faire d'une fiche et d'un
 * sac n'a pas a se redefinir. On le DELEGUE donc, et l'on n'ajoute que ce qui regarde l'ecran :
 * quand le heraut envoie sur le sable (`startCombat`, LOT-09), le modele emet son signal, et c'est
 * l'ecran qui ouvre le Colisee.
 */
class EcouteurDEcran final : public core::DialogueListener {
public:
    using SurCombat = std::function<void(std::string)>;

    EcouteurDEcran(const core::CharacterSheet& fiche, core::Inventory& sac,
                   const core::ExperienceTable& experience, const core::SkillCatalog& competences,
                   SurCombat surCombat, SurCombat surRencontre, SurCombat surFin)
        : _personnage(fiche, sac, experience, competences),
          _surCombat(std::move(surCombat)),
          _surRencontre(std::move(surRencontre)),
          _surFin(std::move(surFin)) {}

    [[nodiscard]] bool speaks(std::string_view languageId) const override {
        return _personnage.speaks(languageId);
    }
    [[nodiscard]] std::vector<core::Modifier> skillModifiers(
        std::string_view skillId) const override {
        return _personnage.skillModifiers(skillId);
    }
    void receiveItem(std::string_view itemId, int quantity) override {
        _personnage.receiveItem(itemId, quantity);
    }
    void startCombat(std::string_view arenaId) override {
        if (_surCombat) {
            _surCombat(std::string{arenaId});
        }
    }
    void startEncounter(std::string_view encounterId) override {
        if (_surRencontre) {
            _surRencontre(std::string{encounterId});
        }
    }
    void endDemo(std::string_view ending) override {
        if (_surFin) {
            _surFin(std::string{ending});
        }
    }

private:
    core::CharacterListener _personnage;
    SurCombat _surCombat;
    SurCombat _surRencontre;
    SurCombat _surFin;
};

}  // namespace

struct DialogueModel::Session {
    DemonstrationState character;
    core::DialogueCatalog dialogues;
    core::DifficultyScale difficulty;
    std::vector<std::string> problems;

    std::string dialogueId;
    const core::DialogueGraph* graph = nullptr;
    std::optional<EcouteurDEcran> listener;
    std::optional<core::DeterministicRandom> random;
    std::optional<core::DialogueRunner> runner;
    DialogueScreenValues values;
    bool left = false;
};

DialogueModel::DialogueModel(QObject* parent)
    : QObject(parent), _session(std::make_unique<Session>()) {
    Session& s = *_session;
    const std::filesystem::path root = executableDirectory();

    s.character = loadDemonstrationState();
    if (s.character.sheet.name.empty()) {
        s.problems.emplace_back("personnage de demonstration absent");
    }
    s.difficulty = core::loadDifficultyScale(root / "Rpg" / "rules" / "difficulty.json");
    for (const std::string& error : s.difficulty.errors) {
        HMI_LOG_WARNING("Dialogue : degres de difficulte, " + error);
    }
    s.dialogues = core::loadDialogues(dataDirectory() / "World" / "dialogues");
    for (const std::string& error : s.dialogues.errors) {
        // Nomme son fichier et son noeud : l'auteur du dialogue le corrige sans lancer le jeu deux
        // fois (EX-CNT-010).
        HMI_LOG_WARNING("Dialogue : " + error);
    }

    core::DialogueReferences references;
    references.skills = &s.character.skills;
    references.difficulty = &s.difficulty;
    references.itemExists = [&s](std::string_view id) {
        return s.character.items.find(id) != nullptr ||
               s.character.equipment.findWeapon(id) != nullptr ||
               s.character.equipment.findArmor(id) != nullptr;
    };
    references.languageExists = [root](std::string_view id) {
        return std::filesystem::exists(root / "Rpg" / "languages" / (std::string(id) + ".json"));
    };
    s.dialogues.dialogues = validDialogues(std::move(s.dialogues.dialogues), references);
    refresh();
}

DialogueModel::~DialogueModel() = default;

void DialogueModel::open() {
    Session& s = *_session;
    s.runner.reset();
    s.random.reset();
    s.listener.reset();
    s.left = false;
    s.graph = s.dialogues.find(s.dialogueId);
    if (s.graph == nullptr || s.character.sheet.name.empty()) {
        if (!s.dialogueId.empty()) {
            HMI_LOG_WARNING("Dialogue : '" + s.dialogueId +
                            "' introuvable ou refuse au chargement.");
        }
        refresh();
        return;
    }
    s.listener.emplace(
        s.character.sheet, s.character.inventory, s.character.experience, s.character.skills,
        [this](const std::string& arena) { emit combatRequested(toQt(arena)); },
        [this](const std::string& rencontre) { emit encounterRequested(toQt(rencontre)); },
        [this](const std::string& voie) { emit demoEnded(toQt(voie)); });
    s.random.emplace(graineSuivante());
    s.runner.emplace(*s.graph, drapeauxDeLaPartie(), *s.listener, s.difficulty, *s.random);
    static_cast<void>(s.runner->start());
    for (const std::string& ligne : s.runner->journal()) {
        HMI_LOG_INFO("Dialogue : " + ligne);
    }
    refresh();
}

void DialogueModel::refresh() {
    Session& s = *_session;
    if (s.runner) {
        s.values = dialogueScreenValues(
            *s.runner, [](std::string_view key) { return ruleLabel(key, activeLanguage()); });
    } else {
        s.values = DialogueScreenValues{};
        s.values.line = ruleLabel("dialogue.unavailable", activeLanguage());
        s.values.replies.push_back({.id = std::string(DIALOGUE_LEAVE_REPLY),
                                    .label = ruleLabel("dialogue.leave", activeLanguage()),
                                    .value = {}});
    }
    QVector<SheetRow> lignes;
    for (const DialogueReply& reponse : s.values.replies) {
        lignes.push_back(
            {.id = toQt(reponse.id), .label = toQt(reponse.label), .value = toQt(reponse.value)});
    }
    _replies.setRows(std::move(lignes));
    emit changed();
}

QString DialogueModel::dialogueId() const {
    return toQt(_session->dialogueId);
}

void DialogueModel::setDialogueId(const QString& id) {
    const std::string lu = id.toStdString();
    if (lu == _session->dialogueId && _session->runner) {
        return;
    }
    _session->dialogueId = lu;
    open();
}

QString DialogueModel::speakerName() const {
    return toQt(_session->values.speakerName);
}

QString DialogueModel::attitude() const {
    return toQt(_session->values.attitude);
}

QString DialogueModel::line() const {
    return toQt(_session->values.line);
}

QString DialogueModel::checkOutcome() const {
    return toQt(_session->values.checkOutcome);
}

QString DialogueModel::checkTitle() const {
    return toQt(_session->values.checkTitle);
}

QString DialogueModel::checkDie() const {
    return toQt(_session->values.checkDie);
}

QString DialogueModel::checkDetail() const {
    return toQt(_session->values.checkDetail);
}

QString DialogueModel::checkVerdict() const {
    return toQt(_session->values.checkVerdict);
}

bool DialogueModel::checkSucceeded() const noexcept {
    return _session->values.checkSucceeded;
}

bool DialogueModel::finished() const noexcept {
    return _session->values.finished || _session->left;
}

QString DialogueModel::status() const {
    const Session& s = *_session;
    QStringList parts;
    for (const std::string& probleme : s.problems) {
        parts << toQt(probleme);
    }
    if (!s.dialogueId.empty() && s.graph == nullptr) {
        parts << QStringLiteral("dialogue inconnu : ") + toQt(s.dialogueId);
    }
    return parts.join(QStringLiteral(" ; "));
}

QStringList DialogueModel::dialogueIds() const {
    QStringList ids;
    for (const core::DialogueGraph& graphe : _session->dialogues.dialogues) {
        ids.push_back(toQt(graphe.id));
    }
    ids.sort();
    return ids;
}

void DialogueModel::choose(const QString& rowId) {
    Session& s = *_session;
    if (rowId.toStdString() == DIALOGUE_LEAVE_REPLY) {
        s.left = true;
        emit changed();
        return;
    }
    if (!s.runner) {
        return;
    }
    const std::size_t avant = s.runner->journal().size();
    if (s.runner->choose(rowId.toStdString()) != core::ChoiceResult::Advanced) {
        return;
    }
    for (std::size_t i = avant; i < s.runner->journal().size(); ++i) {
        HMI_LOG_INFO("Dialogue : " + s.runner->journal()[i]);
    }
    refresh();
}

void DialogueModel::chooseAt(int index) {
    const std::vector<DialogueReply>& reponses = _session->values.replies;
    if (index < 0 || static_cast<std::size_t>(index) >= reponses.size()) {
        return;
    }
    choose(toQt(reponses[static_cast<std::size_t>(index)].id));
}

void DialogueModel::restart() {
    open();
}

}  // namespace hmi
