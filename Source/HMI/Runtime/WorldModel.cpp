// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/WorldModel.h"

#include <filesystem>
#include <string>
#include <utility>

#include "Core/Levels/Level.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/WorldTravel.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Runtime/RuleLabels.h"

namespace hmi {

WorldModel::WorldModel(QObject* parent) : QObject(parent) {
    _play = std::make_unique<WorldPlay>(
        core::WorldTravel::directoryLoader(executableDirectory() / "Levels"),
        executableDirectory() / "Assets");
    _clock.setInterval(STEP_MILLISECONDS);
    _clock.setTimerType(Qt::PreciseTimer);
    connect(&_clock, &QTimer::timeout, this, &WorldModel::step);

    const std::filesystem::path ville =
        executableDirectory() / "World" / "cities" / (std::string{START_CITY} + ".json");
    core::CityPlanResult lue = core::loadCityPlan(ville);
    if (lue.ok()) {
        _city = std::move(lue.plan);
    } else {
        // `EX-NFR-040` : l'ecran de jeu dira qu'il n'a rien a ouvrir, sans planter.
        HMI_LOG_WARNING("Monde : la ville de depart est illisible, " + lue.error);
    }
}

WorldModel::~WorldModel() = default;

bool WorldModel::startNewGame() {
    _visitedDistricts.clear();
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
    return enterMap(QString::fromStdString(depart), QString::fromStdString(_city.startArrival));
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
    ++_sceneRevision;
    emit heroMoved();
}

void WorldModel::setStartOverride(const QString& mapId, const QString& arrival) {
    _startMapOverride = mapId;
    _startArrivalOverride = arrival;
}

void WorldModel::setLevelDirectories(const std::vector<std::filesystem::path>& directories) {
    // Le `Levels/` de l'executable vient TOUJOURS en dernier : l'editeur n'ecrit que les cartes
    // qu'il a ouvertes, et le monde autour doit rester jouable.
    std::vector<std::filesystem::path> dossiers = directories;
    dossiers.push_back(executableDirectory() / "Levels");
    // La session est refaite : la carte courante et les drapeaux acquis appartiennent au chargeur
    // qu'on remplace. C'est pourquoi cet appel precede la premiere entree (LOT-EDITOR-10).
    std::string figure = _play->heroFigure();
    _play = std::make_unique<WorldPlay>(core::WorldTravel::directoriesLoader(std::move(dossiers)),
                                        executableDirectory() / "Assets");
    _play->setHeroFigure(std::move(figure));
    for (const std::filesystem::path& dossier : directories) {
        HMI_LOG_INFO("Monde : cartes lues d'abord dans " + dossier.string());
    }
}

void WorldModel::setStartCell(core::GridPosition cell) {
    _startCell = cell;
}

void WorldModel::setStartFlags(const QStringList& flags) {
    // Poses sur la SESSION, qui survit au changement de carte : un portail verrouille s'ouvre donc
    // aussi bien au premier pas qu'apres trois cartes.
    for (const QString& drapeau : flags) {
        _play->session().flags().set(drapeau.toStdString());
    }
}

bool WorldModel::enterMap(const QString& mapId, const QString& arrival) {
    const std::string carte =
        mapId == QStringLiteral("coliseum") ? "capital/arena-of-brave" : mapId.toStdString();
    if (!_play->enter(carte, arrival.toStdString())) {
        // Un échec de chargement est récupérable (`EX-NFR-040`) : l'écran le dit et reste debout.
        _status = tr("La carte « %1 » ne s'ouvre pas.").arg(mapId);
        _clock.stop();
        ++_sceneRevision;
        emit changed();
        HMI_LOG_WARNING("Monde : la carte " + carte + " ne s'ouvre pas.");
        return false;
    }
    _status.clear();
    _move = {};
    _interact = false;
    noteDistrictVisit();
    ++_sceneRevision;
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

void WorldModel::step() {
    if (_play->session().map() == nullptr) {
        return;
    }
    const float seconds = static_cast<float>(STEP_MILLISECONDS) / 1000.0F;
    const core::ExplorationIntent intention{.move = _move, .interact = _interact};
    _interact = false;

    const WorldPlayStep pas = _play->step(intention, seconds);
    if (pas.sceneChanged) {
        ++_sceneRevision;
    }
    if (pas.heroMoved) {
        ++_sceneRevision;
        emit heroMoved();
    }

    for (const core::ExplorationEvent& evenement : pas.events) {
        switch (evenement.kind) {
            case core::ExplorationEventKind::MapEntered:
                noteDistrictVisit();
                emit changed();
                emit mapEntered(QString::fromStdString(evenement.value));
                break;
            case core::ExplorationEventKind::Dialogue:
                emit dialogueRequested(QString::fromStdString(evenement.value));
                break;
            case core::ExplorationEventKind::Encounter:
                emit encounterRequested(QString::fromStdString(evenement.value));
                break;
            case core::ExplorationEventKind::PortalLocked:
                emit portalLocked(QString::fromStdString(evenement.value));
                break;
            case core::ExplorationEventKind::PortalBroken:
                emit portalBroken(QString::fromStdString(evenement.value));
                break;
            case core::ExplorationEventKind::Interacted:
                emit changed();
                break;
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
    return _play->session().heroPoint().column;
}

qreal WorldModel::heroRow() const {
    return _play->session().heroPoint().row;
}

void WorldModel::setHeroFigure(const QString& figure) {
    if (heroFigure() == figure) {
        return;
    }
    _play->setHeroFigure(figure.toStdString());
    ++_sceneRevision;
    emit changed();
}

bool WorldModel::frozen() const {
    return _play->session().frozen();
}

void WorldModel::setFrozen(bool frozen) {
    if (_play->session().frozen() == frozen) {
        return;
    }
    _play->session().freeze(frozen);
    emit changed();
}

WorldSceneSnapshot WorldModel::snapshot() const {
    return _play->snapshot();
}

float WorldModel::diamondRatio() const {
    return _play->diamondRatio();
}

}  // namespace hmi
