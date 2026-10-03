// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/DialogueModel.h"

#include <QStringList>
#include <QUrl>
#include <QVariantMap>
#include <QVector>
#include <algorithm>
#include <cstddef>
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

// Les drapeaux de monde des conversations : ceux de la partie (`LOT-116`), que la carte lit aussi
// -- une porte ouverte par un dialogue s'ouvre sur la carte, un PNJ appele par une quete y parait.
// Sans partie (le designer, un test de l'ecran seul), un ensemble le temps du processus, pour
// qu'un heraut n'oublie pas qu'on lui a parle a chaque ouverture de l'ecran.
[[nodiscard]] core::WorldFlags& drapeauxDeLaPartie() {
    if (WorldModel* const partie = WorldModel::current()) {
        return partie->flags();
    }
    static core::WorldFlags drapeaux;
    return drapeaux;
}

// Une graine par conversation, tiree d'un compteur et non de l'horloge (EX-NFR-002) : deux
// lancements du jeu rejouent les memes des dans le meme ordre de conversations.
[[nodiscard]] std::uint64_t graineSuivante() {
    static std::uint64_t compteur = 0;
    return core::deriveSeed(0x15D1A106ULL, compteur++, 0);
}

// Garde les dialogues dont toutes les références se résolvent ; journalise les autres.
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

// L'interlocuteur du PNJ, tel que l'ECRAN l'entend : le personnage, et un rappel de plus.
//
// Le personnage CHANGE en cours de conversation : le joueur choisit qui parle pour le groupe
// (LOT-138, D-28). Le runner tient une reference sur cet ecouteur, pas sur le personnage : on
// remplace donc le personnage delegue (`parlerPar`), jamais l'ecouteur.
//
// `core::CharacterListener` est `final` -- et c'est bien : ce qu'il sait faire d'une fiche et d'un
// sac n'a pas a se redefinir. On le DELEGUE donc, et l'on n'ajoute que ce qui regarde l'ecran :
// quand un PNJ engage une rencontre (`startEncounter`, LOT-118) ou clot la demo (`endDemo`,
// LOT-119), le modele emet son signal, et c'est l'ecran qui ouvre ce qui suit.
class EcouteurDEcran final : public core::DialogueListener {
public:
    using SurCombat = std::function<void(std::string)>;

    EcouteurDEcran(const core::CharacterSheet& fiche, core::Inventory& sac,
                   const core::ExperienceTable& experience, const core::SkillCatalog& competences,
                   SurCombat surRencontre, SurCombat surFin)
        : _experience(experience),
          _competences(competences),
          _surRencontre(std::move(surRencontre)),
          _surFin(std::move(surFin)) {
        parlerPar(fiche, sac);
    }

    /// Le personnage qui parle desormais pour le groupe : ses langues, ses jets, son sac.
    void parlerPar(const core::CharacterSheet& fiche, core::Inventory& sac) {
        _personnage.emplace(fiche, sac, _experience, _competences);
    }

    [[nodiscard]] bool speaks(std::string_view languageId) const override {
        return _personnage->speaks(languageId);
    }
    [[nodiscard]] std::vector<core::Modifier> skillModifiers(
        std::string_view skillId) const override {
        return _personnage->skillModifiers(skillId);
    }
    void receiveItem(std::string_view itemId, int quantity) override {
        _personnage->receiveItem(itemId, quantity);
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
    // La montee de niveau donnee par un PNJ (LOT-141) : elle n'ouvre rien, la partie l'applique.
    void levelUp(std::string_view characterId) override {
        if (WorldModel* const partie = WorldModel::current()) {
            static_cast<void>(partie->levelUp(
                QString::fromUtf8(characterId.data(), static_cast<qsizetype>(characterId.size()))));
        } else {
            HMI_LOG_WARNING("Dialogue : montee de niveau sans partie en cours.");
        }
    }
    // Le repos donne par un PNJ (LOT-142) : la partie oublie blessures et lancers depenses.
    void rest(std::string_view characterId) override {
        if (WorldModel* const partie = WorldModel::current()) {
            static_cast<void>(partie->rest(
                QString::fromUtf8(characterId.data(), static_cast<qsizetype>(characterId.size()))));
        } else {
            HMI_LOG_WARNING("Dialogue : repos sans partie en cours.");
        }
    }

private:
    const core::ExperienceTable& _experience;
    const core::SkillCatalog& _competences;
    std::optional<core::CharacterListener> _personnage;
    SurCombat _surRencontre;
    SurCombat _surFin;
};

}  // namespace

// Un membre du groupe qui peut parler : sa fiche et son sac, lus dans les catalogues du meneur.
struct Voix {
    std::string id;
    QUrl portrait;
    core::CharacterSheet sheet;
    core::Inventory inventory;
};

struct DialogueModel::Session {
    DemonstrationState character;
    /// Le groupe, dans l'ordre de marche : qui peut parler (LOT-138, D-28). Le meneur d'abord.
    std::vector<Voix> voices;
    /// Celui qui parle, rang dans `voices`.
    std::size_t voice = 0;
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
    loadVoices();
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
    Voix& voix = s.voices[s.voice];
    s.listener.emplace(
        voix.sheet, voix.inventory, s.character.experience, s.character.skills,
        [this](const std::string& rencontre) { emit encounterRequested(toQt(rencontre)); },
        [this](const std::string& voie) { emit demoEnded(toQt(voie)); });
    // La graine du compteur, sauf si l'appelant en a fixe une : un test force ainsi l'issue d'un
    // jet sans toucher au dialogue (LOT-120), comme EncounterModel::setSeed pour un combat.
    s.random.emplace(_seed != 0 ? static_cast<std::uint64_t>(static_cast<unsigned>(_seed))
                                : graineSuivante());
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

void DialogueModel::loadVoices() {
    Session& s = *_session;
    s.voices.clear();
    s.voice = 0;
    // Le meneur d'abord, deja lu avec les catalogues ; puis les autres membres, dans l'ordre de
    // marche, lus dans les memes catalogues.
    s.voices.push_back(Voix{
        .id = {}, .portrait = {}, .sheet = s.character.sheet, .inventory = s.character.inventory});
    const WorldModel* const partie = WorldModel::current();
    if (partie == nullptr) {
        return;
    }
    const QVariantList membres = partie->partyMembers();
    for (qsizetype rang = 0; rang < membres.size(); ++rang) {
        const QVariantMap membre = membres[rang].toMap();
        const std::string id = membre.value(QStringLiteral("id")).toString().toStdString();
        const QUrl portrait = membre.value(QStringLiteral("portrait")).toUrl();
        if (rang == 0) {
            s.voices.front().id = id;
            s.voices.front().portrait = portrait;
            continue;
        }
        const auto candidat =
            std::ranges::find(partie->candidates(), id, &core::PartyCandidate::id);
        if (candidat == partie->candidates().end()) {
            continue;
        }
        core::LoadedCharacterSheet lue = core::loadCharacterSheet(
            candidat->file, s.character.options, s.character.rules, s.character.experience);
        for (const std::string& erreur : lue.errors) {
            HMI_LOG_WARNING("Dialogue : " + erreur);
        }
        s.voices.push_back(Voix{.id = id,
                                .portrait = portrait,
                                .sheet = std::move(lue.sheet),
                                .inventory = std::move(lue.inventory)});
    }
}

QUrl DialogueModel::speakerPortrait() const {
    const WorldModel* const partie = WorldModel::current();
    return partie != nullptr ? partie->interlocutorPortrait() : QUrl{};
}

QString DialogueModel::partyVoice() const {
    return toQt(_session->voices[_session->voice].sheet.name);
}

QString DialogueModel::voiceId() const {
    return toQt(_session->voices[_session->voice].id);
}

QUrl DialogueModel::voicePortrait() const {
    return _session->voices[_session->voice].portrait;
}

QVariantList DialogueModel::voices() const {
    QVariantList lignes;
    for (std::size_t rang = 0; rang < _session->voices.size(); ++rang) {
        const Voix& voix = _session->voices[rang];
        QVariantMap ligne;
        ligne.insert(QStringLiteral("id"), toQt(voix.id));
        ligne.insert(QStringLiteral("name"), toQt(voix.sheet.name));
        ligne.insert(QStringLiteral("portrait"), voix.portrait);
        ligne.insert(QStringLiteral("current"), rang == _session->voice);
        lignes.append(ligne);
    }
    return lignes;
}

bool DialogueModel::selectVoice(const QString& characterId) {
    Session& s = *_session;
    const auto trouvee = std::ranges::find(s.voices, characterId.toStdString(), &Voix::id);
    if (trouvee == s.voices.end()) {
        return false;
    }
    const auto rang = static_cast<std::size_t>(std::distance(s.voices.begin(), trouvee));
    if (rang != s.voice) {
        s.voice = rang;
        // Le prochain jet sera le sien : l'ecouteur parle desormais par lui.
        if (s.listener) {
            s.listener->parlerPar(trouvee->sheet, trouvee->inventory);
        }
        HMI_LOG_INFO("Dialogue : " + trouvee->sheet.name + " parle pour le groupe.");
        emit changed();
    }
    return true;
}

void DialogueModel::cycleVoice(int step) {
    Session& s = *_session;
    if (s.voices.size() < 2 || step == 0) {
        return;
    }
    const auto taille = static_cast<int>(s.voices.size());
    const int suivant = ((static_cast<int>(s.voice) + step) % taille + taille) % taille;
    static_cast<void>(selectVoice(toQt(s.voices[static_cast<std::size_t>(suivant)].id)));
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

void DialogueModel::setSeed(int seed) {
    if (_seed == seed) {
        return;
    }
    _seed = seed;
    emit changed();
}

}  // namespace hmi
