// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/ArenaModel.h"

#include <QVariantMap>
#include <cstdint>
#include <filesystem>
#include <utility>

#include "Core/Combat/EnemyAi.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/World/CombatZone.h"
#include "HMI/Game/CombatContestants.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"

namespace hmi {

/// Un combattant composé : ce que l'écran a choisi, prêt à devenir un `core::ArenaContestant`.
struct ArenaModel::Fighter {
    QString id;
    core::ArenaContestant contestant;
};

/// Les catalogues chargés une fois, à la construction : le bestiaire, le héros de la démo, les
/// arènes, les Marques, et la carte de l'arène jouable.
struct ArenaModel::Catalogs {
    core::Bestiary bestiary;
    std::optional<HeroContestantSource> hero;
    core::ArenaCatalog arenas;
    core::HeroicMarkCatalog marks;
    /// Les profils de l'IA tactique (`LOT-23`).
    core::BehaviorCatalog behaviors;
    const core::Arena* playable = nullptr;
    std::optional<core::Level> level;
    QStringList problems;
};

namespace {

[[nodiscard]] QString toQt(const std::string& text) {
    return QString::fromStdString(text);
}

constexpr const char* CHARACTER_PREFIX = "character:";

/// Journalise chaque erreur d'un catalogue, précédée de @p prefix.
void logErrors(const std::string& prefix, const std::vector<std::string>& errors) {
    for (const std::string& error : errors) {
        HMI_LOG_WARNING(prefix + error);
    }
}

/// @return La première arène qui a une carte, ou nullptr.
[[nodiscard]] const core::Arena* firstPlayableArena(const core::ArenaCatalog& catalog) {
    for (const core::Arena& arena : catalog.arenas) {
        if (!arena.map.empty()) {
            return &arena;
        }
    }
    return nullptr;
}

[[nodiscard]] QVariantList describeFighters(const auto& fighters) {
    QVariantList list;
    for (const auto& fighter : fighters) {
        list << QVariantMap{{"id", fighter.id},
                            {"name", toQt(fighter.contestant.profile.name)},
                            {"mark", toQt(fighter.contestant.markId)}};
    }
    return list;
}

}  // namespace

ArenaModel::ArenaModel(QObject* parent, std::filesystem::path contentRoot)
    : CombatModel(parent),
      _contentRoot(contentRoot.empty() ? dataDirectory() : std::move(contentRoot)),
      _catalogs(std::make_unique<Catalogs>()) {
    loadCatalogs();
}

ArenaModel::~ArenaModel() = default;

const core::BehaviorCatalog* ArenaModel::behaviors() const {
    return &_catalogs->behaviors;
}

void ArenaModel::loadCatalogs() {
    const std::filesystem::path root = executableDirectory();
    Catalogs& c = *_catalogs;

    c.bestiary = core::loadBestiary(root / "Rpg" / "creatures");
    logErrors("Arene : bestiaire, ", c.bestiary.errors);
    if (c.bestiary.creatures.empty()) {
        c.problems << QStringLiteral("bestiaire vide");
    }

    std::vector<std::string> problemes;
    c.hero = loadHeroSource(problemes);
    for (const std::string& probleme : problemes) {
        c.problems << toQt(probleme);
    }

    c.arenas = core::loadArenas(_contentRoot / "World" / "arena");
    logErrors("Arene : catalogue, ", c.arenas.errors);
    c.marks = core::loadHeroicMarks(root / "Rpg" / "rules" / "heroic-marks.json");
    logErrors("Arene : marques heroiques, ", c.marks.errors);
    c.behaviors = core::loadBehaviors(root / "Rpg" / "rules" / "behaviors.json");
    logErrors("Arene : profils de comportement, ", c.behaviors.errors);
    if (c.behaviors.profiles.empty()) {
        c.problems << QStringLiteral("aucun profil d'IA : les ennemis se commandent a la main");
    }

    if (!loadPlayableLevel()) {
        return;
    }
    resetSession();
    _status = c.problems.join(QStringLiteral(" ; "));
}

bool ArenaModel::loadPlayableLevel() {
    Catalogs& c = *_catalogs;
    c.playable = firstPlayableArena(c.arenas);
    if (c.playable == nullptr) {
        c.problems << QStringLiteral("aucune arene n'a de carte");
        return false;
    }
    const core::LevelLoadResult loaded =
        core::LevelLoader::loadFromFile(_contentRoot / "Levels" / c.playable->map);
    if (!loaded.ok()) {
        HMI_LOG_WARNING("Arene : carte " + c.playable->map + ", " + loaded.error);
        c.problems << QStringLiteral("carte illisible : ") + toQt(loaded.error);
        return false;
    }
    // L'arene joue sur sa ZONE, pas sur la carte entiere (LOT-09) : le Colisee est un lieu, et
    // l'on ne se bat que sur son sable. La carte reduite a la zone est la grille tactique, et les
    // cases du dehors sont inconnues de la session.
    if (c.playable->zone.empty()) {
        c.level = loaded.level;
        return true;
    }
    const std::vector<core::CombatZone> zones = core::combatZonesOf(*loaded.level);
    const core::CombatZone* const zone = core::findCombatZone(zones, c.playable->zone);
    if (zone == nullptr) {
        HMI_LOG_WARNING("Arene : la carte " + c.playable->map + " n'a pas de zone « " +
                        c.playable->zone + " ».");
        c.problems << QStringLiteral("zone de combat inconnue : ") + toQt(c.playable->zone);
        return false;
    }
    const std::vector<core::WorldIssue> defauts =
        core::validateCombatZones(c.playable->map, *loaded.level);
    if (!defauts.empty()) {
        c.problems << QStringLiteral("zone de combat invalide : ") + toQt(defauts.front().value);
        return false;
    }
    c.level = core::cropLevelToZone(*loaded.level, *zone);
    return true;
}

void ArenaModel::resetSession() {
    _session = std::make_unique<core::ArenaSession>(*_catalogs->level);
    // Le catalogue vit aussi longtemps que le modele, donc que la session.
    _session->setOpportunityPolicy(core::aiOpportunityPolicy(_catalogs->behaviors));
}

// --- Lecture ----------------------------------------------------------------------------------

QString ArenaModel::arenaName() const {
    if (_catalogs->playable == nullptr) {
        return QStringLiteral("—");
    }
    return toQt(_catalogs->playable->name);
}

QVariantList ArenaModel::roster() const {
    QVariantList entries;
    if (_catalogs->hero.has_value()) {
        const core::CharacterSheet& sheet = _catalogs->hero->sheet;
        entries << QVariantMap{{"id", QString(CHARACTER_PREFIX) + toQt(sheet.name)},
                               {"name", toQt(sheet.name)},
                               {"kind", QStringLiteral("personnage")},
                               {"hitPoints", sheet.maximumHitPoints},
                               {"armorClass", sheet.armorClass}};
    }
    for (const core::Creature& creature : _catalogs->bestiary.creatures) {
        entries << QVariantMap{{"id", toQt(creature.id)},
                               {"name", toQt(creature.name)},
                               {"kind", QStringLiteral("creature")},
                               {"hitPoints", creature.hitPoints},
                               {"armorClass", creature.armorClass}};
    }
    return entries;
}

QVariantList ArenaModel::allies() const {
    return describeFighters(_allies);
}

QVariantList ArenaModel::enemies() const {
    return describeFighters(_enemies);
}

QStringList ArenaModel::marks() const {
    QStringList ids;
    for (const core::HeroicMark& mark : _catalogs->marks.marks) {
        ids << toQt(mark.id);
    }
    return ids;
}

int ArenaModel::seed() const noexcept {
    return _seed;
}

void ArenaModel::setSeed(int seed) {
    if (seed == _seed) {
        return;
    }
    _seed = seed;
    emit changed();
}

bool ArenaModel::enemyAi() const noexcept {
    return _enemyAi;
}

void ArenaModel::setEnemyAi(bool enabled) {
    // Le choix vaut pour le prochain lancement : un combat monte garde qui le joue.
    if (enabled == _enemyAi || _inCombat) {
        return;
    }
    _enemyAi = enabled;
    emit changed();
}

// --- Composition ------------------------------------------------------------------------------

std::optional<ArenaModel::Fighter> ArenaModel::fighterFor(const QString& id,
                                                          core::CombatSide side) const {
    if (id.startsWith(CHARACTER_PREFIX)) {
        if (!_catalogs->hero.has_value()) {
            return std::nullopt;
        }
        return Fighter{.id = id, .contestant = heroContestant(*_catalogs->hero, side)};
    }
    const core::Creature* creature = _catalogs->bestiary.find(id.toStdString());
    if (creature == nullptr) {
        return std::nullopt;
    }
    // Le profil d'IA se decide au lancement (`composeBout`), selon le reglage de l'ecran.
    return Fighter{.id = id, .contestant = creatureContestant(*creature, side, nullptr)};
}

void ArenaModel::addAlly(const QString& id) {
    if (_inCombat) {
        return;
    }
    if (std::optional<Fighter> fighter = fighterFor(id, core::CombatSide::Allies)) {
        _allies.push_back(std::move(*fighter));
        emit changed();
    }
}

void ArenaModel::addEnemy(const QString& id) {
    if (_inCombat) {
        return;
    }
    if (std::optional<Fighter> fighter = fighterFor(id, core::CombatSide::Enemies)) {
        _enemies.push_back(std::move(*fighter));
        emit changed();
    }
}

void ArenaModel::removeAlly(int index) {
    if (_inCombat || index < 0 || std::cmp_greater_equal(index, _allies.size())) {
        return;
    }
    _allies.erase(_allies.begin() + index);
    emit changed();
}

void ArenaModel::removeEnemy(int index) {
    if (_inCombat || index < 0 || std::cmp_greater_equal(index, _enemies.size())) {
        return;
    }
    _enemies.erase(_enemies.begin() + index);
    emit changed();
}

void ArenaModel::assignMark(bool ally, int index, const QString& markId) {
    std::vector<Fighter>& camp = ally ? _allies : _enemies;
    if (_inCombat || index < 0 || std::cmp_greater_equal(index, camp.size())) {
        return;
    }
    const std::string id = markId.toStdString();
    camp[static_cast<std::size_t>(index)].contestant.markId =
        (id.empty() || _catalogs->marks.find(id) != nullptr) ? id : std::string{};
    emit changed();
}

core::ArenaBout ArenaModel::composeBout() const {
    core::ArenaBout bout{
        .contestants = {},
        .seed = static_cast<std::uint64_t>(static_cast<unsigned>(_seed)),
        .lethal = _catalogs->playable != nullptr && _catalogs->playable->lethal,
        .heroicMark = _catalogs->playable == nullptr || _catalogs->playable->heroicMark,
        .flanking = _catalogs->playable != nullptr && _catalogs->playable->flanking,
        .escapable = true};
    for (const Fighter& fighter : _allies) {
        bout.contestants.push_back(fighter.contestant);
    }
    for (const Fighter& fighter : _enemies) {
        core::ArenaContestant contestant = fighter.contestant;
        if (_enemyAi && !_catalogs->behaviors.profiles.empty()) {
            // Une creature prend le profil que ses regles lui donnent ; le personnage, le defaut.
            const core::Creature* creature = _catalogs->bestiary.find(fighter.id.toStdString());
            contestant.behavior = creature != nullptr
                                      ? core::behaviorFor(*creature, _catalogs->behaviors)
                                      : _catalogs->behaviors.defaultBehavior;
        }
        bout.contestants.push_back(std::move(contestant));
    }
    return bout;
}

// --- Le combat --------------------------------------------------------------------------------

void ArenaModel::launch() {
    if (_session == nullptr || _inCombat) {
        return;
    }
    if (_allies.empty() || _enemies.empty()) {
        _status = QStringLiteral("Il faut au moins un combattant dans chaque camp.");
        emit changed();
        return;
    }
    const core::ArenaMount mount = _session->mount(composeBout());
    refreshMessage(mount);
    if (mount.allies.empty() || mount.enemies.empty()) {
        _status += QStringLiteral(" Un camp est vide apres le montage : rien a lancer.");
        // Le montage a deja pose ce qu'il a pu sur la grille : la scene en a ete changee.
        emitSceneChanged();
        return;
    }
    _inCombat = _session->start();
    playAiTurns();
    emitSceneChanged();
}

void ArenaModel::replay() {
    if (_session == nullptr || !_inCombat) {
        return;
    }
    const core::ArenaMount mount = _session->replay();
    refreshMessage(mount);
    _status = QStringLiteral("Rejeu a la graine ") + QString::number(_seed) +
              (_status.isEmpty() ? QString() : QStringLiteral(" ; ") + _status);
    playAiTurns();
    emitSceneChanged();
}

void ArenaModel::backToSetup() {
    if (!_inCombat) {
        return;
    }
    _inCombat = false;
    resetSession();
    _status.clear();
    emitSceneChanged();
}

}  // namespace hmi
