// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/WorldModel.h"

#include <filesystem>
#include <string>
#include <utility>

#include "Core/Levels/Level.h"
#include "Core/Rpg/Dialogue.h"
#include "Core/World/WorldTravel.h"
#include "HMI/Game/GameQuests.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Runtime/RuleLabels.h"

namespace hmi {

namespace {

/// La partie en cours (`WorldModel::current`).
WorldModel* partieCourante = nullptr;

}  // namespace

WorldModel* WorldModel::current() noexcept {
    return partieCourante;
}

WorldModel::WorldModel(QObject* parent) : QObject(parent) {
    partieCourante = this;
    _play = std::make_unique<WorldPlay>(
        core::WorldTravel::directoryLoader(dataDirectory() / "Levels"),
        dataDirectory() / "Assets");
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
}

WorldModel::~WorldModel() {
    if (partieCourante == this) {
        partieCourante = nullptr;
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
    ++_figuresRevision;
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
    dossiers.push_back(dataDirectory() / "Levels");
    // La session est refaite : la carte courante et les drapeaux acquis appartiennent au chargeur
    // qu'on remplace. C'est pourquoi cet appel precede la premiere entree (LOT-EDITOR-10).
    std::string figure = _play->heroFigure();
    _play = std::make_unique<WorldPlay>(core::WorldTravel::directoriesLoader(std::move(dossiers)),
                                        dataDirectory() / "Assets");
    _play->setHeroFigure(std::move(figure));
    installQuests();
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
    const std::string carte =
        mapId == QStringLiteral("coliseum") ? "capital/arena-of-brave" : mapId.toStdString();
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

void WorldModel::step() {
    if (_play->session().map() == nullptr) {
        return;
    }
    const float seconds = static_cast<float>(STEP_MILLISECONDS) / 1000.0F;
    const core::ExplorationIntent intention{.move = _move, .interact = _interact};
    _interact = false;

    const WorldPlayStep pas = _play->step(intention, seconds);
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
                emit dialogueRequested(QString::fromStdString(evenement.value));
                break;
            case core::ExplorationEventKind::Encounter:
                _lastInteractionCell = evenement.cell;
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
    const bool moved = !_combatFigures.has_value() || _combatHero.x != heroPoint.x ||
                       _combatHero.y != heroPoint.y;
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
    if (heroFigure() == figure) {
        return;
    }
    _play->setHeroFigure(figure.toStdString());
    ++_sceneRevision;
    ++_figuresRevision;
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
