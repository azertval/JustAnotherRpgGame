// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/EncounterModel.h"

#include <QUrl>
#include <QVariantMap>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <system_error>
#include <utility>

#include "Core/Combat/Attack.h"
#include "Core/Combat/CombatTransition.h"
#include "Core/Combat/Encounter.h"
#include "Core/Combat/EnemyAi.h"
#include "Core/Rpg/Bestiary.h"
#include "Core/Rpg/Party.h"
#include "Core/Rpg/Scale.h"
#include "Core/World/ExplorationSession.h"
#include "HMI/Game/CombatContestants.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"
#include "HMI/Runtime/DemonstrationCharacter.h"
#include "HMI/Runtime/WorldModel.h"

namespace hmi {

// Les catalogues du combat sur la carte, lus une fois : le bestiaire, les rencontres, les profils
// de l'IA, le héros.
struct EncounterModel::Catalogs {
    core::Bestiary bestiary;
    core::EncounterCatalog encounters;
    core::BehaviorCatalog behaviors;
    /// Les fiches du groupe lues comme sources de combattant (`LOT-139`), par fichier : une fiche
    /// se lit à sa première rencontre, pleine ; ce que les combats en ont laissé est dans le
    /// registre de la partie, appliqué au montage.
    std::map<std::string, HeroContestantSource> heroes;
};

namespace {

EncounterModel*& rencontreCourante() noexcept {
    static EncounterModel* rencontre = nullptr;
    return rencontre;
}

[[nodiscard]] QString toQt(const std::string& text) {
    return QString::fromStdString(text);
}

// Les points de vie « 12 / 15 » d'un combattant, et leur part.
[[nodiscard]] QString hitPointsText(const core::CombatantProfile& profile) {
    return QString::number(profile.currentHitPoints) + " / " +
           QString::number(profile.maximumHitPoints);
}

[[nodiscard]] double hitPointsRatioOf(const core::CombatantProfile& profile) {
    return std::clamp(
        static_cast<double>(profile.currentHitPoints) / std::max(1, profile.maximumHitPoints), 0.0,
        1.0);
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

// Journalise les erreurs d'un catalogue, sous son préfixe.
void logCatalogErrors(const std::string& prefix, const std::vector<std::string>& errors) {
    for (const std::string& error : errors) {
        std::string message = "Rencontre : ";
        message += prefix;
        message += error;
        HMI_LOG_WARNING(message);
    }
}

// L'entité `encounter` de la carte qui porte @p encounter, s'il y en a une : c'est LA que la
// rencontre paraît -- sa formation s'écrit autour d'elle, et l'éditeur la contrôle là (LOT-146,
// LOT-139) --, que le combat soit engagé en marchant dessus ou par un dialogue. Une entité posée
// se combat une fois : sa clé de drapeau la fait disparaître pour de bon
// (`core::encounterTriggerFor`). Sans entité, un combat engagé par un dialogue n'a pas de clé :
// ce sont les drapeaux de la quête qui en tirent les conséquences (LOT-116).
[[nodiscard]] std::optional<core::EncounterTrigger> encounterMarkerFor(
    const core::Level& map, const std::string& mapId, const core::Encounter& encounter) {
    for (const core::MapEntity& entity : map.entities()) {
        if (const std::optional<core::EncounterTrigger> declencheur =
                core::encounterTriggerFor(entity, mapId);
            declencheur.has_value() && declencheur->encounterId == encounter.id) {
            return declencheur;
        }
    }
    return std::nullopt;
}

// La fiche en cache pleine : ses points de vie au maximum, ses lancers au compte du jour. Le cache
// garde la fiche telle qu'elle a ete lue la premiere fois, registre d'alors applique ; sans ce
// plein, un repos (LOT-142), qui retire du registre les blessures, les laisserait au cache.
void refill(HeroContestantSource& source) {
    source.sheet.currentHitPoints = source.sheet.maximumHitPoints;
    for (core::KnownSpell& connu : source.sheet.knownSpells) {
        if (connu.perDay > 0) {
            connu.remaining = connu.perDay;
        }
    }
    for (core::ArenaSpell& sort : source.spells) {
        const auto connu =
            std::ranges::find(source.sheet.knownSpells, sort.id, &core::KnownSpell::spellId);
        if (sort.uses >= 0 && connu != source.sheet.knownSpells.end() && connu->perDay > 0) {
            sort.uses = connu->perDay;
        }
    }
}

// Le registre de la partie sur la fiche lue : ce que les combats precedents en ont laisse, les
// lancers restants de chaque sort compris.
void applyMemberRecord(HeroContestantSource& source, const core::MemberRecord& record) {
    core::applyRecord(source.sheet, record);
    for (core::ArenaSpell& sort : source.spells) {
        const auto restant = record.spellUses.find(sort.id);
        if (restant != record.spellUses.end() && sort.uses >= 0) {
            sort.uses = std::max(0, restant->second);
        }
    }
}

}  // namespace

EncounterModel* EncounterModel::current() noexcept {
    return rencontreCourante();
}

EncounterModel::EncounterModel(QObject* parent)
    : CombatModel(parent), _contentRoot(dataDirectory()) {
    rencontreCourante() = this;
    _clock.setInterval(STEP_MILLISECONDS);
    _clock.setTimerType(Qt::PreciseTimer);
    connect(&_clock, &QTimer::timeout, this, &EncounterModel::step);
}

EncounterModel::~EncounterModel() {
    if (rencontreCourante() == this) {
        rencontreCourante() = nullptr;
    }
}

void EncounterModel::setContentRoot(std::filesystem::path root) {
    _contentRoot = std::move(root);
    _catalogs.reset();
}

const core::BehaviorCatalog* EncounterModel::behaviors() const {
    return _catalogs != nullptr ? &_catalogs->behaviors : nullptr;
}

CombatModel::Identity EncounterModel::identityOf(core::CombatantId combatant) const {
    Identity identity;
    const auto membre = std::ranges::find(_members, combatant, &Member::combatant);
    if (membre == _members.end()) {
        return identity;
    }
    identity.classId = toQt(membre->classId);
    // Le niveau, sur la fiche lue au montage ; l'image, a cote de la figurine de la classe.
    const WorldModel* const world = WorldModel::current();
    if (world != nullptr && _catalogs != nullptr) {
        const auto candidat =
            std::ranges::find(world->candidates(), membre->characterId, &core::PartyCandidate::id);
        if (candidat != world->candidates().end()) {
            const core::MemberRecord* const record = world->ledger().record(membre->characterId);
            const auto lue = _catalogs->heroes.find(
                candidat->file.string() + "#" +
                std::to_string(record != nullptr && record->level.has_value() ? *record->level
                                                                              : 0));
            if (lue != _catalogs->heroes.end()) {
                identity.level = lue->second.sheet.level;
            }
        }
    }
    const std::filesystem::path figure =
        dataDirectory() / "Assets" / WorldModel::heroFigureOf(membre->classId);
    std::error_code erreur;
    for (const auto& [nom, cible] :
         {std::pair{"portrait.png", &identity.portrait}, std::pair{"token.png", &identity.token}}) {
        const std::filesystem::path image = figure / nom;
        if (std::filesystem::is_regular_file(image, erreur)) {
            *cible = QUrl::fromLocalFile(QString::fromStdString(image.string()));
        }
    }
    return identity;
}

bool EncounterModel::ensureCatalogs() {
    if (_catalogs != nullptr) {
        return true;
    }
    auto catalogs = std::make_unique<Catalogs>();
    catalogs->bestiary = core::loadBestiary(_contentRoot / "Rpg" / "creatures");
    catalogs->encounters = core::loadEncounters(_contentRoot / "Rpg" / "encounters");
    catalogs->behaviors =
        core::loadBehaviors(executableDirectory() / "Rpg" / "rules" / "behaviors.json");
    logCatalogErrors("bestiaire, ", catalogs->bestiary.errors);
    logCatalogErrors("catalogue, ", catalogs->encounters.errors);
    logCatalogErrors("profils de comportement, ", catalogs->behaviors.errors);
    if (catalogs->behaviors.profiles.empty()) {
        HMI_LOG_WARNING("Rencontre : aucun profil d'IA, les ennemis ne joueront pas.");
    }
    _catalogs = std::move(catalogs);
    return true;
}

std::vector<std::pair<std::string, HeroContestantSource>> EncounterModel::partySources(
    const WorldModel& world) {
    // Les quatre entrent en combat (LOT-139), dans l'ordre de marche : chaque fiche se lit une
    // fois, et le registre de la partie dit ce que les combats precedents en ont laisse.
    std::vector<std::pair<std::string, HeroContestantSource>> sources;
    for (const std::string& membre : world.party().members()) {
        const auto candidat =
            std::ranges::find(world.candidates(), membre, &core::PartyCandidate::id);
        if (candidat == world.candidates().end()) {
            HMI_LOG_WARNING("Rencontre : membre du groupe sans fiche, " + membre);
            continue;
        }
        // La cle porte le niveau donne (LOT-141) : une fiche montee se relit, et la lecture
        // applique le registre (loadDemonstrationState).
        const core::MemberRecord* const record = world.ledger().record(membre);
        const std::string fichier =
            candidat->file.string() + "#" +
            std::to_string(record != nullptr && record->level.has_value() ? *record->level : 0);
        auto lue = _catalogs->heroes.find(fichier);
        if (lue == _catalogs->heroes.end()) {
            std::vector<std::string> problemes;
            std::optional<HeroContestantSource> source = loadHeroSource(candidat->file, problemes);
            logCatalogErrors("", problemes);
            if (!source.has_value()) {
                continue;
            }
            lue = _catalogs->heroes.emplace(fichier, std::move(*source)).first;
        }
        HeroContestantSource source = lue->second;
        refill(source);
        if (record != nullptr) {
            applyMemberRecord(source, *record);
        }
        sources.emplace_back(membre, std::move(source));
    }
    return sources;
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
    static_cast<void>(ensureCatalogs());
    const std::vector<std::pair<std::string, HeroContestantSource>> party = partySources(*world);
    if (party.empty()) {
        _status = tr("Aucun membre du groupe n'a de fiche : rien a engager.");
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

    // Le lieu : l'entite `encounter` de la carte qui porte cette rencontre, la ou elle parait ;
    // a defaut la case de la derniere interaction -- le PNJ dont le dialogue engage le combat --,
    // a defaut la case que le heros regarde.
    const core::ExplorationSession& session = world->play().session();
    const core::Level* const map = session.map();
    const std::optional<core::EncounterTrigger> marqueur =
        encounterMarkerFor(*map, session.mapId(), *encounter);
    const core::GridPosition trigger =
        marqueur.has_value() ? marqueur->position
                             : world->lastInteractionCell().value_or(session.aimedCell());
    std::string defeatFlagKey = marqueur.has_value() ? marqueur->defeatFlagKey : std::string{};
    const core::ExplorationSnapshot exploration{
        .playerPosition = {session.heroPoint().column, session.heroPoint().row},
        .playerFacing = session.facing(),
        .cameraPosition = {session.heroPoint().column, session.heroPoint().row},
        .captured = true};
    // Le groupe entre la ou il marche (LOT-139) : le meneur, puis chaque suiveur dans ses pas.
    std::vector<core::GridPosition> partyCells{session.heroCell()};
    for (std::size_t rang = 0; rang + 1 < party.size() && rang < session.followers(); ++rang) {
        partyCells.push_back(core::cellOf(session.followerPoint(rang)));
    }
    core::MapEncounterResult prepared =
        core::prepareMapEncounter(*map, session.mapId(), *encounter, trigger, partyCells,
                                  exploration, std::move(defeatFlagKey));
    if (!prepared.ok()) {
        _status = toQt(prepared.issue);
        HMI_LOG_WARNING("Rencontre : " + prepared.issue);
        emit changed();
        return false;
    }
    _setup = std::move(*prepared.setup);
    _encounterName = encounter->name;

    const core::ArenaMount mount = mountBout(party);
    if (!keepMount(mount, party)) {
        emit changed();
        return false;
    }

    bindFigures(*world, mount);

    subscribeCues();
    _inCombat = _session->start();
    _outcome.clear();
    _cues.clear();
    placeCues();
    followActive();
    world->setFrozen(true);
    publishFigures();
    _clock.start();
    emitSceneChanged();
    HMI_LOG_INFO("Rencontre : " + encounter->id + " engagee sur la zone « " + _setup->zone.name +
                 " » de " + session.mapId() + ".");
    return true;
}

core::ArenaMount EncounterModel::mountBout(
    const std::vector<std::pair<std::string, HeroContestantSource>>& party) {
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
                         // La prise en tenaille, regle optionnelle du Guide du Maitre, se joue
                         // sur la carte (LOT-139) : a quatre, la place de chacun compte, et
                         // l'IA la cherche autant que le joueur.
                         .flanking = true,
                         .escapable = _setup->run.escapable};
    for (std::size_t rang = 0; rang < party.size(); ++rang) {
        core::ArenaContestant membre = heroContestant(party[rang].second, core::CombatSide::Allies);
        if (rang < _setup->partyCells.size()) {
            membre.position = _setup->partyCells[rang];
        }
        bout.contestants.push_back(std::move(membre));
    }
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
    return mount;
}

void EncounterModel::bindFigures(WorldModel& world, const core::ArenaMount& mount) {
    // Ce que chacun dessine : le meneur sa figurine (celle que --hero-figure impose, sinon celle
    // de sa classe), chaque suiveur celle de sa classe, chaque creature la sienne ou son
    // mannequin.
    _bindings.clear();
    for (const Member& membre : _members) {
        const bool meneur = _hero.has_value() && membre.combatant == *_hero;
        const ResolvedFigure& figure =
            meneur ? world.play().heroResolved()
                   : world.play().resolveHero(WorldModel::heroFigureOf(membre.classId));
        _bindings[membre.combatant] =
            Binding{.directory = figure.directory, .oriented = figure.oriented, .hero = meneur};
    }
    std::size_t rang = 0;
    for (const core::CombatantPlacement& placement : _setup->run.placements) {
        const core::Creature* const creature = _catalogs->bestiary.find(placement.creatureId);
        if (creature == nullptr || rang >= mount.enemies.size()) {
            continue;
        }
        const ResolvedFigure& figure =
            world.play().resolveFigure(creature->id, creature->silhouette);
        _bindings[mount.enemies[rang++]] =
            Binding{.directory = figure.directory, .oriented = figure.oriented, .hero = false};
    }
}

bool EncounterModel::keepMount(
    const core::ArenaMount& mount,
    const std::vector<std::pair<std::string, HeroContestantSource>>& party) {
    if (mount.allies.empty() || mount.enemies.empty()) {
        _status += tr(" Un camp est vide apres le montage : rien a engager.");
        HMI_LOG_WARNING("Rencontre : un camp est vide apres le montage.");
        _session.reset();
        _setup.reset();
        return false;
    }
    // Les allies montes, dans l'ordre presente : un membre refuse (sans place) decale les
    // suivants, et le montage l'a dit ; on apparie par le nom pour ne pas se tromper de fiche.
    _members.clear();
    for (const core::CombatantId id : mount.allies) {
        const core::Combatant* const combattant = _session->combat().find(id);
        if (combattant == nullptr) {
            continue;
        }
        const auto source = std::ranges::find_if(party, [&](const auto& membre) {
            return membre.second.sheet.name == combattant->profile.name &&
                   std::ranges::none_of(_members, [&](const Member& deja) {
                       return deja.characterId == membre.first;
                   });
        });
        if (source == party.end()) {
            continue;
        }
        _members.push_back(Member{.characterId = source->first,
                                  .classId = source->second.sheet.classId,
                                  .combatant = id});
    }
    if (_members.empty()) {
        _status += tr(" Aucun membre du groupe n'est monte : rien a engager.");
        _session.reset();
        _setup.reset();
        return false;
    }
    _hero = _members.front().combatant;
    return true;
}

void EncounterModel::placeCues() {
    for (const core::CombatantId id : _session->combat().combatants()) {
        if (const std::optional<core::GridPosition> cell =
                _session->combat().grid().positionOf(id)) {
            _cues.place(id, *cell);
        }
    }
}

void EncounterModel::subscribeCues() {
    // Les pas : leur chemin, pour que la figurine marche case par case.
    _session->setMoveObserver([this](core::CombatantId mover, const core::Path& path) {
        _gesture.reset();
        _cues.push(CombatCue{.kind = CombatCueKind::Walk,
                             .actor = mover,
                             .path = path.steps,
                             .target = std::nullopt,
                             .effect = {}});
    });
    // Les attaques et les sorts du combattant actif : le geste, le tir, le sort et son effet
    // (`LOT-136`).
    _session->setActionObserver(
        [this](const core::ArenaActionNotice& notice) { showAction(notice); });
    core::CombatState& combat = _session->combat();
    combat.subscribe(core::CombatHook::AttackDeclared,
                     [this](core::CombatState& state, const core::CombatEvent& event) {
                         // Le geste d'une attaque que la session joue est deja dans la file ;
                         // reste celui d'une attaque d'opportunite.
                         if (!event.combatant.has_value() || _gesture == event.combatant) {
                             return;
                         }
                         _cues.push(CombatCue{.kind = CombatCueKind::Attack,
                                              .actor = *event.combatant,
                                              .path = {},
                                              .target = event.target.has_value()
                                                            ? state.grid().positionOf(*event.target)
                                                            : std::nullopt,
                                              .effect = {}});
                     });
    combat.subscribe(core::CombatHook::DamageTaken,
                     [this](core::CombatState&, const core::CombatEvent& event) {
                         if (event.combatant.has_value() && event.amount > 0) {
                             _cues.push(CombatCue{.kind = CombatCueKind::Hit,
                                                  .actor = *event.combatant,
                                                  .path = {},
                                                  .target = std::nullopt,
                                                  .effect = {}});
                             pushEffect(*event.combatant, *event.combatant, "impact", false);
                         }
                     });
    combat.subscribe(core::CombatHook::CombatantDowned,
                     [this](core::CombatState&, const core::CombatEvent& event) {
                         if (event.combatant.has_value()) {
                             _cues.push(CombatCue{.kind = CombatCueKind::Death,
                                                  .actor = *event.combatant,
                                                  .path = {},
                                                  .target = std::nullopt,
                                                  .effect = {}});
                         }
                     });
    combat.subscribe(core::CombatHook::CombatantLeft,
                     [this](core::CombatState&, const core::CombatEvent& event) {
                         if (event.combatant.has_value()) {
                             _cues.remove(*event.combatant);
                         }
                     });
}

namespace {

// Les sorts qui volent du lanceur a la cible : leur effet est un projectile (`LOT-136`).
[[nodiscard]] bool isProjectileSpell(std::string_view spell) {
    return spell == "fire-bolt" || spell == "magic-missile" || spell == "scorching-ray";
}

// L'effet d'un tir a l'arme : la fleche (`Common/Fx/arrow.png`).
constexpr std::string_view ARROW_EFFECT = "arrow";
// L'effet d'un jet d'attaque manque.
constexpr std::string_view MISS_EFFECT = "miss";

}  // namespace

void EncounterModel::showAction(const core::ArenaActionNotice& notice) {
    if (notice.phase == core::ArenaActionPhase::End) {
        if (notice.missed) {
            pushEffect(notice.actor, notice.target, std::string{MISS_EFFECT}, false);
        }
        _gesture.reset();
        return;
    }
    _gesture = notice.actor;
    const std::optional<core::GridPosition> cell =
        _session->combat().grid().positionOf(notice.target);
    const bool spell = !notice.spell.empty();
    _cues.push(CombatCue{.kind = spell ? CombatCueKind::Cast : CombatCueKind::Attack,
                         .actor = notice.actor,
                         .path = {},
                         .target = notice.target == notice.actor ? std::nullopt : cell,
                         .ranged = notice.ranged,
                         .effect = {},
                         .travels = false});
    if (spell) {
        pushEffect(notice.actor, notice.target, notice.spell, isProjectileSpell(notice.spell));
    } else if (notice.ranged) {
        pushEffect(notice.actor, notice.target, std::string{ARROW_EFFECT}, true);
    }
}

void EncounterModel::pushEffect(core::CombatantId actor, core::CombatantId target,
                                std::string effect, bool travels) {
    const std::optional<core::GridPosition> cell = _session->combat().grid().positionOf(target);
    if (!cell.has_value()) {
        return;  // une cible hors de la grille : rien a montrer
    }
    _cues.push(CombatCue{.kind = CombatCueKind::Effect,
                         .actor = actor,
                         .path = {},
                         .target = cell,
                         .ranged = false,
                         .effect = std::move(effect),
                         .travels = travels});
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
    // Les effets, apres les figurines : a profondeur egale, ils se dessinent devant (`LOT-136`).
    for (const EffectMotion& effect : _cues.effects()) {
        figures.push_back(WorldFigureSnapshot{
            .figure = std::string{FX_DIRECTORY},
            .clip = effect.effect,
            .point = core::Vector2{effect.point.x + static_cast<float>(_setup->zone.origin.column),
                                   effect.point.y + static_cast<float>(_setup->zone.origin.row)},
            .frame = 0,
            .facing = FigureFacing::None,
            .seconds = effect.seconds,
            .hero = false,
            .combatant = false});
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
        // Le groupe reprend la marche la ou se tient son meneur -- ou, s'il est mort, le
        // premier membre qui tient encore debout (LOT-139).
        std::optional<core::GridPosition> reprise;
        for (const Member& membre : _members) {
            const core::Combatant* const combattant = _session->combat().find(membre.combatant);
            if (combattant == nullptr || combattant->status == core::CombatantStatus::Dead) {
                continue;
            }
            reprise = _session->combat().grid().positionOf(membre.combatant);
            if (reprise.has_value()) {
                break;
            }
        }
        settleParty(*world, *outcome);
        if (reprise.has_value()) {
            world->placeHero(core::cellCenter(core::zoneToMap(_setup->zone, *reprise)));
        }
    }
    teardown();
    HMI_LOG_INFO("Rencontre : quittee, issue " + issue.toStdString() + ".");
    emit finished(issue);
    emitSceneChanged();
}

void EncounterModel::settleParty(WorldModel& world, core::CombatOutcome outcome) const {
    // Une defaite ne laisse rien : la partie s'y termine (LOT-119), et l'ecran de mort quitte
    // le combat lui-meme.
    if (outcome == core::CombatOutcome::Defeat) {
        return;
    }
    std::vector<std::string> morts;
    for (const Member& membre : _members) {
        const core::Combatant* const combattant = _session->combat().find(membre.combatant);
        if (combattant == nullptr) {
            continue;
        }
        if (combattant->status == core::CombatantStatus::Dead) {
            morts.push_back(membre.characterId);
            continue;
        }
        // Le registre garde ce que le combat ne touche pas : le niveau donne (LOT-141). Un
        // enregistrement neuf l'effacait, et la fiche retombait au niveau de son fichier au
        // combat suivant (LOT-142).
        const core::MemberRecord* const avant = world.ledger().record(membre.characterId);
        core::MemberRecord record;
        record.level = avant != nullptr ? avant->level : std::nullopt;
        // A terre a la fin d'un combat gagne, un personnage se releve a 1 point de vie : le
        // Manuel le rend a 1 PV apres 1d4 heures une fois stabilise ; ici, la victoire vaut ce
        // repos (decision du LOT-139). Debout, il garde ce qui lui reste.
        record.hitPoints = combattant->status == core::CombatantStatus::Down
                               ? 1
                               : std::max(1, combattant->profile.currentHitPoints);
        if (const std::vector<core::ArenaSpell>* const sorts = _session->spells(membre.combatant)) {
            for (const core::ArenaSpell& sort : *sorts) {
                if (sort.uses >= 0) {
                    record.spellUses.emplace(sort.id, sort.uses);
                }
            }
        }
        world.recordMember(membre.characterId, std::move(record));
    }
    for (const std::string& mort : morts) {
        static_cast<void>(world.buryMember(mort));
    }
}

void EncounterModel::teardown() {
    _clock.stop();
    _inCombat = false;
    _outcome.clear();
    _cues.clear();
    _bindings.clear();
    _hero.reset();
    _members.clear();
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
    return hero != nullptr ? hitPointsText(hero->profile) : QString{};
}

qreal EncounterModel::heroHitPointsRatio() const {
    if (_session == nullptr || !_hero.has_value()) {
        return 0.0;
    }
    const core::Combatant* const hero = _session->combat().find(*_hero);
    return hero != nullptr ? hitPointsRatioOf(hero->profile) : 0.0;
}

QVariantList EncounterModel::partyMembers() const {
    QVariantList rows;
    if (_session == nullptr || !_inCombat) {
        return rows;
    }
    const WorldModel* const world = WorldModel::current();
    const std::optional<core::CombatantId> active = _session->combat().activeCombatant();
    for (const Member& membre : _members) {
        const core::Combatant* const combattant = _session->combat().find(membre.combatant);
        if (combattant == nullptr) {
            continue;
        }
        QVariantMap row;
        row.insert(QStringLiteral("id"), toQt(membre.characterId));
        row.insert(QStringLiteral("label"), toQt(combattant->profile.name));
        row.insert(QStringLiteral("value"), hitPointsText(combattant->profile));
        row.insert(QStringLiteral("ratio"), hitPointsRatioOf(combattant->profile));
        row.insert(QStringLiteral("active"), active == membre.combatant);
        row.insert(QStringLiteral("dead"), combattant->status == core::CombatantStatus::Dead);
        row.insert(QStringLiteral("down"), combattant->status == core::CombatantStatus::Down ||
                                               combattant->status == core::CombatantStatus::Dead);
        QUrl portrait;
        if (world != nullptr) {
            for (const QVariant& ligne : world->partyMembers()) {
                const QVariantMap membreDuMonde = ligne.toMap();
                if (membreDuMonde.value(QStringLiteral("id")).toString() ==
                    toQt(membre.characterId)) {
                    portrait = membreDuMonde.value(QStringLiteral("portrait")).toUrl();
                }
            }
        }
        row.insert(QStringLiteral("portrait"), portrait);
        rows.append(row);
    }
    return rows;
}

int EncounterModel::activeMember() const {
    if (_session == nullptr || !_inCombat) {
        return -1;
    }
    const std::optional<core::CombatantId> active = _session->combat().activeCombatant();
    for (std::size_t rang = 0; rang < _members.size(); ++rang) {
        if (active == _members[rang].combatant) {
            return static_cast<int>(rang);
        }
    }
    return -1;
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
    const bool dead = target->status == core::CombatantStatus::Dead;
    const bool down = dead || target->status == core::CombatantStatus::Down;
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
        QString label;
        double ratio = 1.0;
        if (dead) {
            label = tr("mort");
            ratio = 0.0;
        } else if (down) {
            label = tr("a terre");
            ratio = 0.0;
        } else if (core::isBloodied(profile)) {
            label = tr("ensanglante");
            ratio = 0.5;
        }
        map.insert("hitPoints", label);
        map.insert("hitPointsRatio", ratio);
    }
    map.insert("armorClass", QString::number(profile.armorClass));
    map.insert("speed", QString::number(
                            static_cast<double>(profile.movement) * core::METERS_PER_TILE, 'g', 3) +
                            tr(" m"));
    // Les etats de la session (LOT-137) : inconscient, a terre, stabilise, mort, beni...
    QStringList conditions;
    for (const core::CombatCondition condition : _session->conditionsOf(*occupant)) {
        QString label = toQt(std::string(core::combatConditionLabel(condition)));
        if (!label.isEmpty()) {
            label[0] = label[0].toUpper();
        }
        conditions << label;
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
