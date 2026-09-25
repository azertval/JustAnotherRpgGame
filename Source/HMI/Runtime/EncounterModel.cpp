// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/EncounterModel.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <utility>

#include "Core/Combat/Attack.h"
#include "Core/Combat/CombatTransition.h"
#include "Core/Combat/Encounter.h"
#include "Core/Combat/EnemyAi.h"
#include "Core/Rpg/Bestiary.h"
#include "Core/Rpg/Scale.h"
#include "Core/World/ExplorationSession.h"
#include "HMI/Game/CombatContestants.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Runtime/WorldModel.h"

namespace hmi {

// Les catalogues du combat sur la carte, lus une fois : le bestiaire, les rencontres, les profils
// de l'IA, le héros.
struct EncounterModel::Catalogs {
    core::Bestiary bestiary;
    core::EncounterCatalog encounters;
    core::BehaviorCatalog behaviors;
    std::optional<HeroContestantSource> hero;
};

namespace {

EncounterModel* rencontreCourante = nullptr;

[[nodiscard]] QString toQt(const std::string& text) {
    return QString::fromStdString(text);
}

[[nodiscard]] QString outcomeName(core::CombatOutcome outcome) {
    switch (outcome) {
        case core::CombatOutcome::Victory:
            return QStringLiteral("victory");
        case core::CombatOutcome::Flight:
            return QStringLiteral("flight");
        case core::CombatOutcome::Defeat:
            return QStringLiteral("defeat");
    }
    return {};
}

// Une graine tirée à l'horloge quand l'écran n'en impose pas : deux rencontres ne se ressemblent
// pas, et le journal dit laquelle a servi pour la rejouer.
[[nodiscard]] std::uint64_t graineHorloge() {
    return static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
}

}  // namespace

EncounterModel* EncounterModel::current() noexcept {
    return rencontreCourante;
}

EncounterModel::EncounterModel(QObject* parent)
    : CombatModel(parent), _contentRoot(dataDirectory()) {
    rencontreCourante = this;
    _clock.setInterval(STEP_MILLISECONDS);
    _clock.setTimerType(Qt::PreciseTimer);
    connect(&_clock, &QTimer::timeout, this, &EncounterModel::step);
}

EncounterModel::~EncounterModel() {
    if (rencontreCourante == this) {
        rencontreCourante = nullptr;
    }
}

void EncounterModel::setContentRoot(std::filesystem::path root) {
    _contentRoot = std::move(root);
    _catalogs.reset();
}

const core::BehaviorCatalog* EncounterModel::behaviors() const {
    return _catalogs != nullptr ? &_catalogs->behaviors : nullptr;
}

bool EncounterModel::ensureCatalogs() {
    if (_catalogs != nullptr) {
        return _catalogs->hero.has_value();
    }
    auto catalogs = std::make_unique<Catalogs>();
    catalogs->bestiary = core::loadBestiary(_contentRoot / "Rpg" / "creatures");
    catalogs->encounters = core::loadEncounters(_contentRoot / "Rpg" / "encounters");
    catalogs->behaviors =
        core::loadBehaviors(executableDirectory() / "Rpg" / "rules" / "behaviors.json");
    for (const std::string& error : catalogs->bestiary.errors) {
        HMI_LOG_WARNING("Rencontre : bestiaire, " + error);
    }
    for (const std::string& error : catalogs->encounters.errors) {
        HMI_LOG_WARNING("Rencontre : catalogue, " + error);
    }
    for (const std::string& error : catalogs->behaviors.errors) {
        HMI_LOG_WARNING("Rencontre : profils de comportement, " + error);
    }
    std::vector<std::string> problemes;
    catalogs->hero = loadHeroSource(problemes);
    for (const std::string& probleme : problemes) {
        HMI_LOG_WARNING("Rencontre : " + probleme);
    }
    if (catalogs->behaviors.profiles.empty()) {
        HMI_LOG_WARNING("Rencontre : aucun profil d'IA, les ennemis ne joueront pas.");
    }
    _catalogs = std::move(catalogs);
    return _catalogs->hero.has_value();
}

void EncounterModel::setSeed(int seed) {
    if (seed == _seed) {
        return;
    }
    _seed = seed;
    emit changed();
}

// --- Le montage -------------------------------------------------------------------------------

bool EncounterModel::begin(const QString& encounterId) {
    if (_inCombat) {
        _status = tr("Un combat est deja engage.");
        emit changed();
        return false;
    }
    WorldModel* const world = WorldModel::current();
    if (world == nullptr || !world->loaded()) {
        _status = tr("Aucune carte ou engager le combat.");
        emit changed();
        return false;
    }
    if (!ensureCatalogs()) {
        _status = tr("Le heros de la demo n'a pas de fiche : rien a engager.");
        emit changed();
        return false;
    }
    const core::Encounter* const encounter = _catalogs->encounters.find(encounterId.toStdString());
    if (encounter == nullptr) {
        _status = tr("Rencontre inconnue : %1").arg(encounterId);
        HMI_LOG_WARNING("Rencontre : '" + encounterId.toStdString() + "' est inconnue.");
        emit changed();
        return false;
    }

    // Le lieu : la case de la derniere interaction -- le PNJ dont le dialogue engage le combat,
    // l'entite `encounter` --, a defaut la case que le heros regarde.
    const core::ExplorationSession& session = world->play().session();
    const core::Level* const map = session.map();
    const core::GridPosition trigger = world->lastInteractionCell().value_or(session.aimedCell());
    // Une entite `encounter` posee la se combat une fois : sa cle de drapeau la fait disparaitre
    // pour de bon (`core::encounterTriggerFor`). Un combat engage par un dialogue n'a pas de cle :
    // ce sont les drapeaux de la quete qui en tirent les consequences (LOT-116).
    std::string defeatFlagKey;
    for (const core::MapEntity& entity : map->entities()) {
        if (entity.position != trigger) {
            continue;
        }
        if (const std::optional<core::EncounterTrigger> declencheur =
                core::encounterTriggerFor(entity, session.mapId());
            declencheur.has_value() && declencheur->encounterId == encounter->id) {
            defeatFlagKey = declencheur->defeatFlagKey;
            break;
        }
    }
    const core::ExplorationSnapshot exploration{
        .playerPosition = {session.heroPoint().column, session.heroPoint().row},
        .playerFacing = session.facing(),
        .cameraPosition = {session.heroPoint().column, session.heroPoint().row},
        .captured = true};
    core::MapEncounterResult prepared =
        core::prepareMapEncounter(*map, session.mapId(), *encounter, trigger, session.heroCell(),
                                  exploration, std::move(defeatFlagKey));
    if (!prepared.ok()) {
        _status = toQt(prepared.issue);
        HMI_LOG_WARNING("Rencontre : " + prepared.issue);
        emit changed();
        return false;
    }
    _setup = std::move(*prepared.setup);
    _encounterName = encounter->name;

    // La session, sur la grille de la zone.
    _session = std::make_unique<core::ArenaSession>(_setup->battlefield);
    _session->setOpportunityPolicy(core::aiOpportunityPolicy(_catalogs->behaviors));

    core::ArenaBout bout{.contestants = {},
                         .seed = _seed != 0
                                     ? static_cast<std::uint64_t>(static_cast<unsigned>(_seed))
                                     : graineHorloge(),
                         // Un combat sur la carte est LETAL : la defaite est la mort, et la demo
                         // s'y termine (LOT-119). Les Marques sont celles du Colisee.
                         .lethal = true,
                         .heroicMark = false,
                         .flanking = false,
                         .escapable = _setup->run.escapable};
    core::ArenaContestant hero = heroContestant(*_catalogs->hero, core::CombatSide::Allies);
    hero.position = _setup->heroCell;
    bout.contestants.push_back(std::move(hero));
    for (const core::CombatantPlacement& placement : _setup->run.placements) {
        const core::Creature* const creature = _catalogs->bestiary.find(placement.creatureId);
        if (creature == nullptr) {
            HMI_LOG_WARNING("Rencontre : creature inconnue du bestiaire, " + placement.creatureId);
            continue;
        }
        core::ArenaContestant enemy =
            creatureContestant(*creature, core::CombatSide::Enemies, &_catalogs->behaviors);
        enemy.position = placement.position;
        bout.contestants.push_back(std::move(enemy));
    }
    const core::ArenaMount mount = _session->mount(bout);
    refreshMessage(mount);
    for (const std::string& note : _setup->notes) {
        _session->note(note);
    }
    _session->note("graine " + std::to_string(bout.seed));
    if (mount.allies.empty() || mount.enemies.empty()) {
        _status += tr(" Un camp est vide apres le montage : rien a engager.");
        HMI_LOG_WARNING("Rencontre : un camp est vide apres le montage.");
        _session.reset();
        _setup.reset();
        emit changed();
        return false;
    }
    _hero = mount.allies.front();

    // Ce que chacun dessine : le heros sa figurine, chaque creature la sienne ou son mannequin.
    _bindings.clear();
    const ResolvedFigure& heroFigure = world->play().heroResolved();
    _bindings[*_hero] =
        Binding{.directory = heroFigure.directory, .oriented = heroFigure.oriented, .hero = true};
    std::size_t rang = 0;
    for (const core::CombatantPlacement& placement : _setup->run.placements) {
        const core::Creature* const creature = _catalogs->bestiary.find(placement.creatureId);
        if (creature == nullptr || rang >= mount.enemies.size()) {
            continue;
        }
        const ResolvedFigure& figure =
            world->play().resolveFigure(creature->id, creature->silhouette);
        _bindings[mount.enemies[rang++]] =
            Binding{.directory = figure.directory, .oriented = figure.oriented, .hero = false};
    }

    subscribeCues();
    _inCombat = _session->start();
    _outcome.clear();
    _cues.clear();
    for (const core::CombatantId id : _session->combat().combatants()) {
        if (const std::optional<core::GridPosition> cell =
                _session->combat().grid().positionOf(id)) {
            _cues.place(id, *cell);
        }
    }
    followActive();
    world->setFrozen(true);
    publishFigures();
    _clock.start();
    emitSceneChanged();
    HMI_LOG_INFO("Rencontre : " + encounter->id + " engagee sur la zone « " + _setup->zone.name +
                 " » de " + session.mapId() + ".");
    return true;
}

void EncounterModel::subscribeCues() {
    // Les pas : leur chemin, pour que la figurine marche case par case.
    _session->setMoveObserver([this](core::CombatantId mover, const core::Path& path) {
        _cues.push(CombatCue{.kind = CombatCueKind::Walk, .actor = mover, .path = path.steps});
    });
    core::CombatState& combat = _session->combat();
    combat.subscribe(core::CombatHook::AttackDeclared,
                     [this](core::CombatState& state, const core::CombatEvent& event) {
                         if (!event.combatant.has_value()) {
                             return;
                         }
                         _cues.push(CombatCue{.kind = CombatCueKind::Attack,
                                              .actor = *event.combatant,
                                              .path = {},
                                              .target = event.target.has_value()
                                                            ? state.grid().positionOf(*event.target)
                                                            : std::nullopt});
                     });
    combat.subscribe(
        core::CombatHook::DamageTaken, [this](core::CombatState&, const core::CombatEvent& event) {
            if (event.combatant.has_value() && event.amount > 0) {
                _cues.push(CombatCue{.kind = CombatCueKind::Hit, .actor = *event.combatant});
            }
        });
    combat.subscribe(core::CombatHook::CombatantDowned, [this](core::CombatState&,
                                                               const core::CombatEvent& event) {
        if (event.combatant.has_value()) {
            _cues.push(CombatCue{.kind = CombatCueKind::Death, .actor = *event.combatant});
        }
    });
    combat.subscribe(core::CombatHook::CombatantLeft,
                     [this](core::CombatState&, const core::CombatEvent& event) {
                         if (event.combatant.has_value()) {
                             _cues.remove(*event.combatant);
                         }
                     });
}

// --- Le temps ---------------------------------------------------------------------------------

void EncounterModel::step() {
    tick(static_cast<float>(STEP_MILLISECONDS) / 1000.0F);
}

void EncounterModel::playAiTurns() {
    // Rien d'un bloc : `tick` joue un tour de l'IA quand la file des mouvements est vide, et le
    // suivant quand celui-la s'est vu.
}

void EncounterModel::tick(float seconds) {
    if (!_inCombat || _session == nullptr) {
        return;
    }
    const bool wasBusy = _cues.busy();
    _cues.advance(seconds);
    bool played = false;
    if (!_cues.busy() && !ended()) {
        // Un tour de l'IA par vidage de la file : il se voit avant que le suivant se joue.
        played = playOneAiTurn();
        if (played) {
            followActive();
        }
    }
    publishFigures();
    settleOutcome();
    if (played) {
        emitSceneChanged();
    } else if (wasBusy != _cues.busy()) {
        emit changed();  // `busy` a change : les gestes se rouvrent, ou se ferment
    }
}

void EncounterModel::settleOutcome() {
    if (!ended() || _cues.busy() || !_outcome.isEmpty()) {
        return;
    }
    _outcome = outcomeName(*_session->outcome());
    _status = toQt(_session->journal().back());
    emit changed();
}

void EncounterModel::skipAnimations() {
    if (!_inCombat) {
        return;
    }
    _cues.finishAll();
    publishFigures();
    settleOutcome();
    emit changed();
}

void EncounterModel::publishFigures() {
    WorldModel* const world = WorldModel::current();
    if (world == nullptr || _session == nullptr || !_setup.has_value()) {
        return;
    }
    const core::CombatState& combat = _session->combat();
    std::vector<WorldFigureSnapshot> figures;
    core::Vector2 heroPoint{};
    for (const core::CombatantId id : combat.combatants()) {
        const core::Combatant* const combatant = combat.find(id);
        const FigureMotion* const motion = _cues.motionOf(id);
        const auto binding = _bindings.find(id);
        if (combatant == nullptr || combatant->status == core::CombatantStatus::Withdrawn ||
            motion == nullptr || binding == _bindings.end()) {
            continue;
        }
        const core::Vector2 point{motion->point.x + static_cast<float>(_setup->zone.origin.column),
                                  motion->point.y + static_cast<float>(_setup->zone.origin.row)};
        if (binding->second.hero) {
            heroPoint = point;
        }
        figures.push_back(WorldFigureSnapshot{
            .figure = binding->second.directory,
            .clip = std::string{motion->clip},
            .point = point,
            .frame = 0,
            .facing = binding->second.oriented ? motion->facing : FigureFacing::None,
            .seconds = motion->clipSeconds,
            .hero = binding->second.hero,
            .combatant = true});
    }
    world->setCombatFigures(std::move(figures), heroPoint);
}

// --- La sortie --------------------------------------------------------------------------------

void EncounterModel::leave() {
    if (!_inCombat || _session == nullptr) {
        return;
    }
    // L'issue, meme si la file n'a pas fini de la montrer : on part, l'image suit.
    _cues.finishAll();
    const std::optional<core::CombatOutcome> outcome = _session->outcome();
    if (!outcome.has_value()) {
        _status = tr("Le combat n'est pas fini.");
        emit changed();
        return;
    }
    const QString issue = outcomeName(*outcome);
    WorldModel* const world = WorldModel::current();
    if (world != nullptr && _setup.has_value()) {
        // Seule une victoire acquiert le drapeau (`core::endEncounter`). Le heros reste ou le
        // combat l'a laisse : la carte EST le champ de bataille, et le ramener a la case d'avant
        // contredirait ce que le joueur vient de voir (decision D3 du LOT-118 ; l'instantane
        // d'exploration ne restitue que l'orientation).
        static_cast<void>(core::endEncounter(_setup->run, *outcome, world->flags()));
        if (_hero.has_value()) {
            if (const std::optional<core::GridPosition> cell =
                    _session->combat().grid().positionOf(*_hero)) {
                world->placeHero(core::cellCenter(core::zoneToMap(_setup->zone, *cell)));
            }
        }
    }
    teardown();
    HMI_LOG_INFO("Rencontre : quittee, issue " + issue.toStdString() + ".");
    emit finished(issue);
    emitSceneChanged();
}

void EncounterModel::teardown() {
    _clock.stop();
    _inCombat = false;
    _outcome.clear();
    _cues.clear();
    _bindings.clear();
    _hero.reset();
    _session.reset();
    _setup.reset();
    _followed.reset();
    if (WorldModel* const world = WorldModel::current()) {
        world->clearCombatFigures();
        // Les drapeaux d'une victoire ont pu changer la carte : le prochain pas de la session
        // en tire les consequences (`core::ExplorationSession::refreshFromFlags`).
        world->setFrozen(false);
    }
}

// --- Lecture ----------------------------------------------------------------------------------

QString EncounterModel::encounterName() const {
    return toQt(_encounterName);
}

int EncounterModel::zoneColumn() const noexcept {
    return _setup.has_value() ? _setup->zone.origin.column : 0;
}

int EncounterModel::zoneRow() const noexcept {
    return _setup.has_value() ? _setup->zone.origin.row : 0;
}

QString EncounterModel::heroName() const {
    if (_session == nullptr || !_hero.has_value()) {
        return {};
    }
    const core::Combatant* const hero = _session->combat().find(*_hero);
    return hero != nullptr ? toQt(hero->profile.name) : QString{};
}

QString EncounterModel::heroHitPoints() const {
    if (_session == nullptr || !_hero.has_value()) {
        return {};
    }
    const core::Combatant* const hero = _session->combat().find(*_hero);
    if (hero == nullptr) {
        return {};
    }
    return QString::number(hero->profile.currentHitPoints) + " / " +
           QString::number(hero->profile.maximumHitPoints);
}

qreal EncounterModel::heroHitPointsRatio() const {
    if (_session == nullptr || !_hero.has_value()) {
        return 0.0;
    }
    const core::Combatant* const hero = _session->combat().find(*_hero);
    if (hero == nullptr) {
        return 0.0;
    }
    return std::clamp(static_cast<double>(hero->profile.currentHitPoints) /
                          std::max(1, hero->profile.maximumHitPoints),
                      0.0, 1.0);
}

QVariantMap EncounterModel::target() const {
    QVariantMap map;
    if (_session == nullptr || !_inCombat) {
        return map;
    }
    const core::CombatState& combat = _session->combat();
    const std::optional<core::CombatantId> occupant = combat.grid().occupantAt(_cursor);
    const core::Combatant* const target = occupant.has_value() ? combat.find(*occupant) : nullptr;
    if (target == nullptr) {
        return map;
    }
    const core::CombatantProfile& profile = target->profile;
    const bool ally = profile.side == core::CombatSide::Allies;
    const bool down = target->status == core::CombatantStatus::Down;
    map.insert("name", toQt(profile.name));
    map.insert("side", ally ? QStringLiteral("allies") : QStringLiteral("enemies"));
    // Guide du Maitre, chapitre 8 : les points de vie d'un monstre se suivent en secret.
    if (ally) {
        map.insert("hitPoints", QString::number(profile.currentHitPoints) + " / " +
                                    QString::number(profile.maximumHitPoints));
        map.insert("hitPointsRatio", std::clamp(static_cast<double>(profile.currentHitPoints) /
                                                    std::max(1, profile.maximumHitPoints),
                                                0.0, 1.0));
    } else {
        map.insert("hitPoints", down ? tr("a terre")
                                     : (core::isBloodied(profile) ? tr("ensanglante") : QString()));
        map.insert("hitPointsRatio", down ? 0.0 : (core::isBloodied(profile) ? 0.5 : 1.0));
    }
    map.insert("armorClass", QString::number(profile.armorClass));
    map.insert("speed", QString::number(
                            static_cast<double>(profile.movement) * core::METERS_PER_TILE, 'g', 3) +
                            tr(" m"));
    QStringList conditions;
    if (down) {
        conditions << tr("A terre");
    }
    if (core::isBloodied(profile) && !down) {
        conditions << tr("Ensanglante");
    }
    if (_session->isDodging(*occupant)) {
        conditions << tr("Esquive");
    }
    map.insert("conditions", conditions.isEmpty() ? tr("Aucun") : conditions.join(", "));
    return map;
}

}  // namespace hmi
