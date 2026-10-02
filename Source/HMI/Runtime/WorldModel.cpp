// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/WorldModel.h"

#include <QVariantMap>
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <optional>
#include <string>
#include <system_error>
#include <utility>

#include "Core/Gameplay/Interaction.h"
#include "Core/Levels/Level.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/WorldTravel.h"
#include "HMI/Game/GameQuests.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Runtime/DemonstrationCharacter.h"
#include "HMI/Runtime/RuleLabels.h"

namespace hmi {

namespace {

// La partie en cours (`WorldModel::current`).
WorldModel*& partieCourante() noexcept {
    static WorldModel* partie = nullptr;
    return partie;
}

// Monte d'un niveau la fiche de @p id telle que la partie l'a laissee (registre applique) : le
// registre retient le niveau et les points de vie qui en resultent -- la montee n'est pas un soin,
// les blessures restent (core::gainExperience). Rien si la classe est inconnue ou le niveau
// maximal atteint.
[[nodiscard]] std::optional<core::MemberRecord> levelUpRecord(
    const std::string& id, const core::PartyCandidate& candidat,
    const core::MemberRecord* const ancien) {
    DemonstrationState etat = loadDemonstrationState(candidat.file);
    const core::PlayableClass* const classe = etat.options.findClass(etat.sheet.classId);
    if (classe == nullptr) {
        HMI_LOG_WARNING("Montee de niveau : classe inconnue pour " + id);
        return std::nullopt;
    }
    std::vector<std::string> manquants;
    const core::LevelUpResult resultat =
        core::levelUpTo(etat.sheet, etat.sheet.level + 1, *classe, etat.options, etat.rules,
                        etat.experience, manquants);
    if (!resultat.gainedLevel()) {
        HMI_LOG_INFO("Montee de niveau : " + id + " est deja au niveau maximal.");
        return std::nullopt;
    }
    core::MemberRecord record;
    if (ancien != nullptr) {
        record = *ancien;
    }
    record.level = resultat.newLevel;
    record.hitPoints = etat.sheet.currentHitPoints;
    HMI_LOG_INFO("Montee de niveau : " + id + " passe au niveau " +
                 std::to_string(resultat.newLevel) + " (+" +
                 std::to_string(resultat.hitPointsGained) + " PV).");
    return record;
}

}  // namespace

WorldModel* WorldModel::current() noexcept {
    return partieCourante();
}

WorldModel::WorldModel(QObject* parent) : QObject(parent) {
    partieCourante() = this;
    connect(this, &WorldModel::changed, this, &WorldModel::interactionChanged);
    connect(this, &WorldModel::heroMoved, this, &WorldModel::interactionChanged);
    _play = std::make_unique<WorldPlay>(
        core::WorldTravel::directoryLoader(dataDirectory() / "Levels"), dataDirectory() / "Assets");
    _clock.setInterval(STEP_MILLISECONDS);
    _clock.setTimerType(Qt::PreciseTimer);
    connect(&_clock, &QTimer::timeout, this, &WorldModel::step);

    const std::filesystem::path ville =
        dataDirectory() / "World" / "cities" / (std::string{START_CITY} + ".json");
    core::CityPlanResult lue = core::loadCityPlan(ville);
    if (lue.ok()) {
        _city = std::move(lue.plan);
    } else {
        // `EX-NFR-040` : l'ecran de jeu dira qu'il n'a rien a ouvrir, sans planter.
        HMI_LOG_WARNING("Monde : la ville de depart est illisible, " + lue.error);
    }
    installQuests();

    // Le groupe (LOT-138) : les fiches du dossier des personnages, les regles a cote du binaire
    // comme le reste du RPG.
    core::PartyCandidates candidats =
        core::loadPartyCandidates(executableDirectory() / "Rpg" / "characters");
    for (const std::string& erreur : candidats.errors) {
        HMI_LOG_WARNING("Groupe : " + erreur);
    }
    _candidates = std::move(candidats.candidates);
    _party = core::defaultParty(_candidates, STARTING_PARTY);
    applyParty();
}

WorldModel::~WorldModel() {
    if (partieCourante() == this) {
        partieCourante() = nullptr;
    }
}

void WorldModel::installQuests() {
    GameQuests lues = loadGameQuests(dataDirectory());
    for (const std::string& erreur : lues.errors) {
        // Nomme son fichier et sa ligne (EX-CNT-010) : la quete se corrige sans relancer deux fois.
        HMI_LOG_WARNING("Quete : " + erreur);
    }
    _play->session().setQuests(std::move(lues.catalog));
}

bool WorldModel::startNewGame() {
    _visitedDistricts.clear();
    _ledger.clear();
    if (!_startMapOverride.isEmpty()) {
        const bool ouverte = enterMap(_startMapOverride, _startArrivalOverride);
        if (ouverte) {
            placeHeroAtStartCell();
        }
        return ouverte;
    }
    const std::string depart = _city.startMap();
    if (depart.empty()) {
        _status = tr("La ville de départ ne s'ouvre pas.");
        emit changed();
        return false;
    }
    const bool ouverte =
        enterMap(QString::fromStdString(depart), QString::fromStdString(_city.startArrival));
    // La demo se rejoue avec le meneur qu'on choisit (LOT-142) : c'est lui qui parle et fait les
    // jets, les quatre restent du groupe.
    if (ouverte && !_choosingLeader) {
        _choosingLeader = true;
        emit partyChanged();
    }
    return ouverte;
}

void WorldModel::endLeaderChoice() {
    if (_choosingLeader) {
        _choosingLeader = false;
        emit partyChanged();
    }
}

void WorldModel::placeHeroAtStartCell() {
    if (!_startCell) {
        return;
    }
    const core::Level* const carte = _play->session().map();
    if (carte == nullptr || !carte->tileMap().inBounds(_startCell->column, _startCell->row)) {
        // `EX-NFR-040` : une case hors de la carte ne fait pas echouer le lancement -- on joue a
        // l'entree, et le journal dit pourquoi.
        HMI_LOG_WARNING("Monde : --at= hors de la carte, le heros reste a l'entree.");
        return;
    }
    _play->session().placeHero(core::cellCenter(*_startCell));
    ++_figuresRevision;
    emit heroMoved();
}

void WorldModel::setStartOverride(const QString& mapId, const QString& arrival) {
    _startMapOverride = mapId;
    _startArrivalOverride = arrival;
}

void WorldModel::setLevelDirectories(const std::vector<std::filesystem::path>& directories) {
    // La session est refaite : la carte courante et les drapeaux acquis appartiennent au chargeur
    // qu'on remplace. C'est pourquoi cet appel precede la premiere entree (LOT-EDITOR-10).
    _levelDirectories = directories;
    rebuildSession();
    for (const std::filesystem::path& dossier : directories) {
        HMI_LOG_INFO("Monde : cartes lues d'abord dans " + dossier.string());
    }
}

void WorldModel::rebuildSession() {
    // Le `Levels/` de l'executable vient TOUJOURS en dernier : l'editeur n'ecrit que les cartes
    // qu'il a ouvertes, et le monde autour doit rester jouable.
    std::vector<std::filesystem::path> dossiers = _levelDirectories;
    dossiers.push_back(dataDirectory() / "Levels");
    _play = std::make_unique<WorldPlay>(core::WorldTravel::directoriesLoader(std::move(dossiers)),
                                        dataDirectory() / "Assets");
    installQuests();
    applyParty();
}

void WorldModel::endGame() {
    // Une partie finie ne laisse rien derriere elle : ni carte, ni drapeau, ni combattant. Le
    // prochain `startNewGame` -- « Nouvelle partie », ou « Recommencer » sur l'ecran de mort --
    // repart donc de la porte de la ville, et non du sable ou l'on vient de tomber (LOT-119).
    _clock.stop();
    _combatFigures.reset();
    _lastInteractionCell.reset();
    _visitedDistricts.clear();
    _status.clear();
    _move = {};
    _interact = false;
    // Le groupe aussi repart de zero : le groupe preforme, le Brawler en tete, les fiches
    // pleines (LOT-139).
    _party = core::defaultParty(_candidates, STARTING_PARTY);
    _ledger.clear();
    rebuildSession();
    applyFlags(_startFlags);
    ++_sceneRevision;
    ++_figuresRevision;
    HMI_LOG_INFO("Monde : partie terminee, session remise a zero.");
    emit changed();
    emit figuresChanged();
}

void WorldModel::setStartCell(std::optional<core::GridPosition> cell) {
    _startCell = cell;
}

void WorldModel::setStartFlags(const QStringList& flags) {
    // Retenus : une partie neuve (`endGame`) les repose, comme le lancement les a poses.
    _startFlags = flags;
    applyFlags(flags);
}

void WorldModel::applyFlags(const QStringList& flags) {
    // Poses sur la SESSION, qui survit au changement de carte : un portail verrouille s'ouvre donc
    // aussi bien au premier pas qu'apres trois cartes.
    // `drapeau=valeur` donne sa valeur a un drapeau qu'une quete declare (`LOT-116`).
    core::WorldFlags& drapeaux = _play->session().flags();
    for (const QString& drapeau : flags) {
        const qsizetype egal = drapeau.indexOf(QLatin1Char('='));
        if (egal < 0) {
            drapeaux.set(drapeau.toStdString());
        } else if (!drapeaux.setValue(drapeau.left(egal).toStdString(),
                                      drapeau.mid(egal + 1).toStdString())) {
            HMI_LOG_WARNING("Monde : --flags=, valeur refusee : " + drapeau.toStdString());
        }
    }
    // Les quetes avancent tout de suite, carte ouverte ou non : le journal le montre des
    // l'ouverture.
    static_cast<void>(_play->session().refreshFromFlags());
}

bool WorldModel::enterMap(const QString& mapId, const QString& arrival) {
    const std::string carte = mapId.toStdString();
    if (!_play->enter(carte, arrival.toStdString())) {
        // Un échec de chargement est récupérable (`EX-NFR-040`) : l'écran le dit et reste debout.
        _status = tr("La carte « %1 » ne s'ouvre pas.").arg(mapId);
        _clock.stop();
        ++_sceneRevision;
        ++_figuresRevision;
        emit changed();
        HMI_LOG_WARNING("Monde : la carte " + carte + " ne s'ouvre pas.");
        return false;
    }
    _status.clear();
    _move = {};
    _interact = false;
    _lastInteractionCell.reset();
    noteDistrictVisit();
    ++_sceneRevision;
    ++_figuresRevision;
    _clock.start();
    emit changed();
    emit heroMoved();
    emit mapEntered(mapId);
    return true;
}

void WorldModel::setMove(qreal x, qreal y) {
    _move = {static_cast<float>(x), static_cast<float>(y)};
}

void WorldModel::interact() {
    _interact = true;
}

QVariantMap WorldModel::interactionTarget() const {
    const core::ExplorationSession& session = _play->session();
    const core::Level* map = session.map();
    if (map == nullptr) {
        return {};
    }
    std::vector<core::InteractionCandidate> candidates;
    const auto& interactables = session.interactables();
    for (std::size_t index = 0; index < interactables.size(); ++index) {
        candidates.push_back({.interactable = &interactables[index], .index = index});
    }
    const auto point = session.heroPoint();
    const auto target = core::findInteractionTarget({point.column, point.row}, session.facing(),
                                                    map->tileMap(), candidates, session.flags());
    if (!target.found()) {
        return {};
    }
    const auto& entity = *target.interactable;
    const QString prompt = entity.type == "npc"     ? tr("Parler")
                           : entity.type == "chest" ? tr("Ouvrir")
                           : entity.type == "sign"  ? tr("Lire")
                                                    : tr("Interagir");
    return {{"column", entity.position.column}, {"row", entity.position.row}, {"prompt", prompt}};
}

void WorldModel::releaseInput() noexcept {
    _move = {};
    _interact = false;
}

void WorldModel::step() {
    if (_play->session().map() == nullptr) {
        return;
    }
    const float seconds = static_cast<float>(STEP_MILLISECONDS) / 1000.0F;
    const core::ExplorationIntent intention{.move = _move, .interact = _interact};
    _interact = false;

    const auto previousFacing = _play->session().facing();
    const WorldPlayStep pas = _play->step(intention, seconds);
    if (!pas.heroMoved && previousFacing != _play->session().facing()) {
        emit interactionChanged();
    }
    // La carte ne se recompose que si elle a changé ; un pas du heros ne touche qu'aux figurines.
    if (pas.sceneChanged) {
        ++_sceneRevision;
    }
    // Les figurines changent a CHAQUE pas : a l'arret, elles respirent (la bande de repos avance
    // avec le temps, LOT-118), et une image de la carte coute 0,07 ms (audit de l'affichage).
    // Pendant un combat sur la carte, c'est le combat qui publie ses figurines.
    if (!_combatFigures.has_value()) {
        ++_figuresRevision;
    }
    if (pas.heroMoved) {
        emit heroMoved();
    } else {
        emit figuresChanged();  // rien n'a bouge, mais l'image a change : elle se redessine
    }

    for (const core::ExplorationEvent& evenement : pas.events) {
        switch (evenement.kind) {
            case core::ExplorationEventKind::MapEntered:
                noteDistrictVisit();
                emit changed();
                emit mapEntered(QString::fromStdString(evenement.value));
                break;
            case core::ExplorationEventKind::Dialogue:
                _lastInteractionCell = evenement.cell;
                releaseInput();
                emit dialogueRequested(QString::fromStdString(evenement.value));
                break;
            case core::ExplorationEventKind::Encounter:
                _lastInteractionCell = evenement.cell;
                releaseInput();
                emit encounterRequested(QString::fromStdString(evenement.value));
                break;
            case core::ExplorationEventKind::PortalLocked:
                emit portalLocked(QString::fromStdString(evenement.value));
                break;
            case core::ExplorationEventKind::PortalBroken:
                emit portalBroken(QString::fromStdString(evenement.value));
                break;
            case core::ExplorationEventKind::PortalSealed:
                emit portalSealed(QString::fromStdString(evenement.value));
                break;
            case core::ExplorationEventKind::Interacted:
                emit changed();
                break;
            case core::ExplorationEventKind::QuestAdvanced: {
                const QString valeur = QString::fromStdString(evenement.value);
                const qsizetype barre = valeur.indexOf(QLatin1Char('/'));
                HMI_LOG_INFO("Quete : etape atteinte, " + evenement.value);
                emit questAdvanced(valeur.left(barre), valeur.mid(barre + 1));
                break;
            }
        }
    }
}

void WorldModel::noteDistrictVisit() {
    const core::CityDistrict* const quartier = _city.districtOfMap(_play->session().mapId());
    if (quartier == nullptr) {
        return;
    }
    const QString identifiant = QString::fromStdString(quartier->id);
    if (!_visitedDistricts.contains(identifiant)) {
        _visitedDistricts.append(identifiant);
    }
}

QString WorldModel::mapOfDistrict(const QString& districtId) const {
    const core::CityDistrict* const quartier = _city.find(districtId.toStdString());
    return quartier != nullptr ? QString::fromStdString(quartier->map) : QString{};
}

QString WorldModel::districtId() const {
    const core::CityDistrict* const quartier = _city.districtOfMap(_play->session().mapId());
    return quartier != nullptr ? QString::fromStdString(quartier->id) : QString{};
}

QString WorldModel::mapId() const {
    return QString::fromStdString(_play->session().mapId());
}

QString WorldModel::mapName() const {
    const core::Level* const carte = _play->session().map();
    // Le nom d'une carte est une cle (`map.<identifiant>.name`, LOT-EDITOR-07) ; un nom qui n'en
    // est pas une revient tel quel.
    return carte != nullptr
               ? QString::fromStdString(hmi::ruleLabel(carte->name(), hmi::activeLanguage()))
               : QString{};
}

bool WorldModel::loaded() const {
    return _play->session().map() != nullptr;
}

int WorldModel::columns() const {
    const core::Level* const carte = _play->session().map();
    return carte != nullptr ? carte->tileMap().width() : 0;
}

int WorldModel::rows() const {
    const core::Level* const carte = _play->session().map();
    return carte != nullptr ? carte->tileMap().height() : 0;
}

qreal WorldModel::heroColumn() const {
    return _combatFigures.has_value() ? _combatHero.x : _play->session().heroPoint().column;
}

qreal WorldModel::heroRow() const {
    return _combatFigures.has_value() ? _combatHero.y : _play->session().heroPoint().row;
}

void WorldModel::setCombatFigures(std::vector<WorldFigureSnapshot> figures,
                                  core::Vector2 heroPoint) {
    const bool moved =
        !_combatFigures.has_value() || _combatHero.x != heroPoint.x || _combatHero.y != heroPoint.y;
    _combatFigures = std::move(figures);
    _combatHero = heroPoint;
    ++_figuresRevision;
    if (moved) {
        emit heroMoved();
    } else {
        emit figuresChanged();
    }
}

void WorldModel::clearCombatFigures() {
    if (!_combatFigures.has_value()) {
        return;
    }
    _combatFigures.reset();
    ++_figuresRevision;
    emit heroMoved();
}

void WorldModel::placeHero(core::CellPoint point) {
    _play->session().placeHero(point);
    ++_figuresRevision;
    emit heroMoved();
}

void WorldModel::setHeroFigure(const QString& figure) {
    if (_heroFigureOverride == figure.toStdString()) {
        return;
    }
    _heroFigureOverride = figure.toStdString();
    applyParty();
}

// --- Le groupe (LOT-138) ----------------------------------------------------------------------

std::string WorldModel::heroFigureOf(std::string_view classId) {
    return "Common/Characters/Heroes/" + std::string{classId};
}

const core::PartyCandidate* WorldModel::candidate(std::string_view characterId) const {
    const auto trouve = std::ranges::find(_candidates, characterId, &core::PartyCandidate::id);
    return trouve != _candidates.end() ? &*trouve : nullptr;
}

void WorldModel::applyParty() {
    const core::PartyCandidate* const meneur = candidate(_party.leader());
    std::string figure = _heroFigureOverride;
    if (figure.empty()) {
        figure = meneur != nullptr ? heroFigureOf(meneur->classId)
                                   : std::string{WorldPlay::DEFAULT_HERO_FIGURE};
    }
    if (_play->heroFigure() != figure) {
        _play->setHeroFigure(std::move(figure));
    }
    std::vector<std::string> suiveurs;
    for (std::size_t rang = 1; rang < _party.members().size(); ++rang) {
        const core::PartyCandidate* const membre = candidate(_party.members()[rang]);
        suiveurs.push_back(membre != nullptr ? heroFigureOf(membre->classId) : std::string{});
    }
    if (_play->followerFigures() != suiveurs) {
        _play->setFollowerFigures(std::move(suiveurs));
    }
    ++_sceneRevision;
    ++_figuresRevision;
    emit changed();
    emit figuresChanged();
    emit partyChanged();
}

bool WorldModel::levelUp(const QString& characterId) {
    std::vector<std::string> cibles;
    if (characterId == QString::fromUtf8(core::LEVEL_UP_PARTY.data(),
                                         static_cast<qsizetype>(core::LEVEL_UP_PARTY.size()))) {
        cibles = _party.members();
    } else {
        cibles.push_back(characterId.toStdString());
    }
    bool monte = false;
    for (const std::string& id : cibles) {
        const core::PartyCandidate* const candidat = candidate(id);
        if (candidat == nullptr) {
            HMI_LOG_WARNING("Montee de niveau : personnage inconnu, " + id);
            continue;
        }
        std::optional<core::MemberRecord> record = levelUpRecord(id, *candidat, _ledger.record(id));
        if (!record.has_value()) {
            continue;
        }
        _ledger.write(id, std::move(*record));
        monte = true;
    }
    if (monte) {
        emit partyChanged();
    }
    return monte;
}

bool WorldModel::rest(const QString& characterId) {
    std::vector<std::string> cibles;
    if (characterId == QString::fromUtf8(core::LEVEL_UP_PARTY.data(),
                                         static_cast<qsizetype>(core::LEVEL_UP_PARTY.size()))) {
        cibles = _party.members();
    } else {
        cibles.push_back(characterId.toStdString());
    }
    bool repose = false;
    for (const std::string& id : cibles) {
        const core::MemberRecord* const record = _ledger.record(id);
        if (record == nullptr || (!record->hitPoints.has_value() && record->spellUses.empty())) {
            continue;
        }
        _ledger.rest(id);
        repose = true;
    }
    if (repose) {
        HMI_LOG_INFO("Repos long : le groupe retrouve ses points de vie et ses sorts.");
        emit partyChanged();
    }
    return repose;
}

void WorldModel::showCharacter(const QString& characterId) {
    const std::string id = characterId.toStdString();
    if (_shownCharacterId == id) {
        return;
    }
    _shownCharacterId = candidate(id) != nullptr ? id : std::string{};
    emit partyChanged();
}

QString WorldModel::shownCharacterId() const {
    if (!_shownCharacterId.empty() && _party.contains(_shownCharacterId)) {
        return QString::fromStdString(_shownCharacterId);
    }
    return leaderId();
}

void WorldModel::recordMember(const std::string& characterId, core::MemberRecord record) {
    if (!record.inventory.has_value()) {
        if (const core::MemberRecord* previous = _ledger.record(characterId)) {
            record.inventory = previous->inventory;
        }
    }
    _ledger.write(characterId, std::move(record));
    emit partyChanged();
}

bool WorldModel::buryMember(const std::string& characterId) {
    // Le mort ne suit plus (LOT-139) ; s'il menait, le deuxieme mene -- `core::Party::remove`
    // garde l'ordre des autres. Le dernier reste : la defaite se joue ailleurs.
    if (_party.remove(characterId) != core::PartyChange::Done) {
        return false;
    }
    _ledger.erase(characterId);
    HMI_LOG_INFO("Groupe : " + characterId + " est mort et quitte le groupe.");
    applyParty();
    return true;
}

void WorldModel::setParty(const core::Party& party) {
    core::Party connus;
    for (const std::string& membre : party.members()) {
        if (candidate(membre) != nullptr) {
            static_cast<void>(connus.add(membre));
        }
    }
    if (connus.empty()) {
        HMI_LOG_WARNING("Groupe : aucun personnage connu, le groupe ne change pas.");
        return;
    }
    _party = std::move(connus);
    applyParty();
}

bool WorldModel::setLeader(const QString& characterId) {
    if (_party.leader() == characterId.toStdString()) {
        return true;
    }
    if (_party.setLeader(characterId.toStdString()) != core::PartyChange::Done) {
        return false;
    }
    applyParty();
    return true;
}

bool WorldModel::rotateLeader() {
    if (_party.size() < 2 || _party.rotateLeader() != core::PartyChange::Done) {
        return false;
    }
    applyParty();
    return true;
}

bool WorldModel::toggleMember(const QString& characterId) {
    const std::string id = characterId.toStdString();
    if (candidate(id) == nullptr) {
        return false;
    }
    const core::PartyChange change = _party.contains(id) ? _party.remove(id) : _party.add(id);
    if (change != core::PartyChange::Done) {
        return false;
    }
    applyParty();
    return true;
}

bool WorldModel::moveMember(const QString& characterId, int offset) {
    const std::vector<std::string>& membres = _party.members();
    const auto trouve = std::ranges::find(membres, characterId.toStdString());
    if (trouve == membres.end() || offset == 0) {
        return false;
    }
    const auto rang = static_cast<std::ptrdiff_t>(std::distance(membres.begin(), trouve));
    const std::ptrdiff_t cible = rang + (offset < 0 ? -1 : 1);
    if (cible < 0 || std::cmp_greater_equal(cible, membres.size())) {
        return false;
    }
    static_cast<void>(_party.swap(static_cast<std::size_t>(rang), static_cast<std::size_t>(cible)));
    applyParty();
    return true;
}

QVariantMap WorldModel::candidateRow(const core::PartyCandidate& candidate) const {
    const std::string figure = heroFigureOf(candidate.classId);
    const std::filesystem::path portrait = dataDirectory() / "Assets" / figure / "portrait.png";
    std::error_code erreur;
    QVariantMap ligne;
    ligne.insert(QStringLiteral("id"), QString::fromStdString(candidate.id));
    ligne.insert(QStringLiteral("name"), QString::fromStdString(candidate.name));
    ligne.insert(QStringLiteral("classId"), QString::fromStdString(candidate.classId));
    ligne.insert(QStringLiteral("figure"), QString::fromStdString(figure));
    // Sans portrait (la figurine de la classe n'est pas encore livree, LOT-136), le cadre prend
    // son etat vide : un mannequin n'a pas de visage a montrer.
    ligne.insert(QStringLiteral("portrait"),
                 std::filesystem::is_regular_file(portrait, erreur)
                     ? QUrl::fromLocalFile(QString::fromStdString(portrait.string()))
                     : QUrl{});
    ligne.insert(QStringLiteral("leader"), _party.leader() == candidate.id);
    const std::vector<std::string>& membres = _party.members();
    const auto rang = std::ranges::find(membres, candidate.id);
    ligne.insert(
        QStringLiteral("rank"),
        rang != membres.end() ? static_cast<int>(std::distance(membres.begin(), rang)) : -1);
    return ligne;
}

QVariantList WorldModel::partyMembers() const {
    QVariantList lignes;
    for (const std::string& membre : _party.members()) {
        if (const core::PartyCandidate* const trouve = candidate(membre)) {
            lignes.append(candidateRow(*trouve));
        }
    }
    return lignes;
}

QVariantList WorldModel::partyCandidates() const {
    QVariantList lignes;
    for (const core::PartyCandidate& candidat : _candidates) {
        lignes.append(candidateRow(candidat));
    }
    return lignes;
}

QString WorldModel::leaderId() const {
    return QString::fromStdString(std::string{_party.leader()});
}

QString WorldModel::leaderName() const {
    const core::PartyCandidate* const meneur = candidate(_party.leader());
    return meneur != nullptr ? QString::fromStdString(meneur->name) : QString{};
}

QUrl WorldModel::leaderPortrait() const {
    const core::PartyCandidate* const meneur = candidate(_party.leader());
    return meneur != nullptr ? candidateRow(*meneur).value(QStringLiteral("portrait")).toUrl()
                             : QUrl{};
}

std::filesystem::path WorldModel::leaderSheetFile() const {
    const core::PartyCandidate* const meneur = candidate(_party.leader());
    return meneur != nullptr ? meneur->file : std::filesystem::path{};
}

bool WorldModel::frozen() const {
    return _play->session().frozen();
}

void WorldModel::setFrozen(bool frozen) {
    if (frozen) {
        // Ce qui gele la carte (dialogue, combat) remplace la vue de jeu : detruite, elle ne voit
        // jamais le relachement des touches, et la derniere direction ferait repartir le heros
        // seul au retour.
        releaseInput();
    }
    if (_play->session().frozen() == frozen) {
        return;
    }
    _play->session().freeze(frozen);
    emit changed();
}

std::shared_ptr<const WorldSceneSnapshot> WorldModel::scene() const {
    return _play->scene();
}

std::vector<WorldFigureSnapshot> WorldModel::figures() const {
    return _combatFigures.has_value() ? *_combatFigures : _play->figures();
}

WorldSceneSnapshot WorldModel::snapshot() const {
    return _play->snapshot();
}

float WorldModel::diamondRatio() const {
    return _play->diamondRatio();
}

}  // namespace hmi
