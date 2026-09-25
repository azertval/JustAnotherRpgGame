// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/CombatModel.h"

#include <QVariantMap>
#include <algorithm>
#include <cstdint>
#include <utility>

#include "Core/Combat/BattleGrid.h"
#include "Core/Combat/CombatPreview.h"
#include "Core/Combat/Pathfinding.h"
#include "HMI/Runtime/DemonstrationCharacter.h"

namespace hmi {

namespace {

[[nodiscard]] QString toQt(const std::string& text) {
    return QString::fromStdString(text);
}

[[nodiscard]] QString sideName(core::CombatSide side) {
    return side == core::CombatSide::Allies ? QStringLiteral("allies") : QStringLiteral("enemies");
}

// Une action du tour telle que l'écran la propose.
enum class TurnActionKind : std::uint8_t { ATTACK, DODGE, DISENGAGE, DASH, REACTION };

struct TurnActionEntry {
    TurnActionKind kind = TurnActionKind::ATTACK;
    std::size_t attack = 0;
    QString label;
};

// Les actions du combattant `active` : ses attaques, puis les actions du Manuel, puis sa
// réaction.
[[nodiscard]] std::vector<TurnActionEntry> turnActionsOf(const core::ArenaSession& session,
                                                         core::CombatantId active) {
    std::vector<TurnActionEntry> entries;
    if (const std::vector<core::AttackProfile>* attacks = session.attacks(active)) {
        for (std::size_t i = 0; i < attacks->size(); ++i) {
            entries.push_back(
                {.kind = TurnActionKind::ATTACK, .attack = i, .label = toQt((*attacks)[i].label)});
        }
    }
    entries.push_back(
        {.kind = TurnActionKind::DODGE, .attack = 0, .label = CombatModel::tr("Esquiver")});
    entries.push_back({.kind = TurnActionKind::DISENGAGE,
                       .attack = 0,
                       .label = CombatModel::tr("Se desengager")});
    entries.push_back(
        {.kind = TurnActionKind::DASH, .attack = 0, .label = CombatModel::tr("Se precipiter")});
    entries.push_back({.kind = TurnActionKind::REACTION,
                       .attack = 0,
                       .label = session.takesOpportunities(active)
                                    ? CombatModel::tr("Reaction : saisir les opportunites")
                                    : CombatModel::tr("Reaction : laisser passer")});
    return entries;
}

[[nodiscard]] QString kindName(TurnActionKind kind) {
    switch (kind) {
        case TurnActionKind::ATTACK:
            return QStringLiteral("attack");
        case TurnActionKind::DODGE:
            return QStringLiteral("dodge");
        case TurnActionKind::DISENGAGE:
            return QStringLiteral("disengage");
        case TurnActionKind::DASH:
            return QStringLiteral("dash");
        case TurnActionKind::REACTION:
            return QStringLiteral("reaction");
    }
    return {};
}

// Ce que l'écran montre de la santé d'un combattant : un texte et une jauge.
struct HealthDisplay {
    QString hitPoints;
    double ratio = 1.0;
};

[[nodiscard]] HealthDisplay healthDisplayOf(const core::CombatantProfile& profile, bool down) {
    HealthDisplay display;
    if (profile.side == core::CombatSide::Allies) {
        display.hitPoints = QString::number(profile.currentHitPoints) + "/" +
                            QString::number(profile.maximumHitPoints);
        display.ratio = std::clamp(
            static_cast<double>(profile.currentHitPoints) / std::max(1, profile.maximumHitPoints),
            0.0, 1.0);
        return display;
    }
    // Guide du Maitre, chapitre 8 : les points de vie d'un monstre se suivent en secret ;
    // sous la moitie, il est ensanglante, et cela se voit.
    if (down) {
        display.hitPoints = CombatModel::tr("a terre");
        display.ratio = 0.0;
    } else if (core::isBloodied(profile)) {
        display.hitPoints = CombatModel::tr("ensanglante");
        display.ratio = 0.5;
    }
    return display;
}

}  // namespace

CombatModel::CombatModel(QObject* parent) : QObject(parent) {
    // Ce qui change le combat change aussi ce que le curseur montre.
    connect(this, &CombatModel::changed, this, &CombatModel::cursorChanged);
}

CombatModel::~CombatModel() = default;

std::optional<HeroContestantSource> CombatModel::loadHeroSource(
    std::vector<std::string>& problems) {
    // Le heros de la demo, par le meme chemin que la fiche : un seul chargement, une seule verite
    // sur ce qu'il porte (LOT-87, LOT-112).
    const DemonstrationState demonstration = loadDemonstrationState();
    if (demonstration.sheet.name.empty()) {
        problems.emplace_back("personnage de demonstration absent");
        return std::nullopt;
    }
    HeroContestantSource hero{
        .sheet = demonstration.sheet,
        .proficiency = core::proficiencyBonus(demonstration.sheet, demonstration.experience),
        .armorClass = core::derivedStatsFor(demonstration.sheet, demonstration.inventory,
                                            demonstration.lookup(), demonstration.rules,
                                            demonstration.encumbrance)
                          .armorClass,
        .weapon = std::nullopt};
    if (const core::Weapon* weapon = demonstration.equipment.findWeapon(
            demonstration.inventory.at(core::EquipmentSlot::MainHand))) {
        hero.weapon = *weapon;
    }
    return hero;
}

void CombatModel::emitSceneChanged() {
    emit changed();
    emit combatSceneChanged();
}

bool CombatModel::playOneAiTurn() {
    if (_session == nullptr || !_inCombat || ended() || behaviors() == nullptr) {
        return false;
    }
    const std::optional<core::CombatantId> active = _session->combat().activeCombatant();
    if (!active.has_value() || _session->behaviorOf(*active).empty()) {
        return false;
    }
    return core::playTurn(*_session, *behaviors());
}

void CombatModel::playAiTurns() {
    if (_session == nullptr || !_inCombat) {
        return;
    }
    // Chaque tour joue termine le tour ou le combat : la garde n'est la que contre une regression.
    for (int guard = 0; guard < 256 && !ended(); ++guard) {
        if (!playOneAiTurn()) {
            break;
        }
    }
    if (ended()) {
        _status = toQt(_session->journal().back());
    }
    followActive();
}

void CombatModel::followActive() {
    if (_session == nullptr || !_inCombat) {
        _followed.reset();
        return;
    }
    const std::optional<core::CombatantId> active = _session->combat().activeCombatant();
    if (active == _followed) {
        return;
    }
    _followed = active;
    _selectedAction = 0;
    if (active.has_value()) {
        if (const std::optional<core::GridPosition> cell =
                _session->combat().grid().positionOf(*active)) {
            _cursor = *cell;
        }
    }
}

std::optional<core::CombatantId> CombatModel::playerTurn() const {
    if (_session == nullptr || !_inCombat || ended() ||
        _session->combat().phase() != core::CombatPhase::TurnActive) {
        return std::nullopt;
    }
    const std::optional<core::CombatantId> active = _session->combat().activeCombatant();
    if (!active.has_value() || !_session->behaviorOf(*active).empty()) {
        return std::nullopt;
    }
    return active;
}

void CombatModel::refreshMessage(const core::ArenaMount& mount) {
    QStringList lines;
    for (const core::MountRefusal& refusal : mount.refusals) {
        QString reason = QStringLiteral("inconnu du bestiaire");
        if (refusal.placement.has_value()) {
            switch (*refusal.placement) {
                case core::PlacementResult::Placed:
                    reason = QStringLiteral("place");
                    break;
                case core::PlacementResult::OutOfBounds:
                    reason = QStringLiteral("plus de point d'entree libre");
                    break;
                case core::PlacementResult::Obstructed:
                    reason = QStringLiteral("case obstruee");
                    break;
                case core::PlacementResult::Occupied:
                    reason = QStringLiteral("case occupee");
                    break;
                case core::PlacementResult::InvalidCombatant:
                    reason = QStringLiteral("combattant invalide");
                    break;
            }
        }
        lines << QStringLiteral("refuse : ") + toQt(refusal.who) + " (" + reason + ")";
    }
    _status = lines.join(QStringLiteral(" ; "));
}

// --- Lecture ----------------------------------------------------------------------------------

bool CombatModel::ended() const {
    return _inCombat && _session != nullptr && _session->outcome().has_value();
}

int CombatModel::gridColumns() const {
    return _session != nullptr ? _session->level().tileMap().width() : 0;
}

int CombatModel::gridRows() const {
    return _session != nullptr ? _session->level().tileMap().height() : 0;
}

QVariantList CombatModel::turnOrder() const {
    QVariantList list;
    if (_session == nullptr || !_inCombat) {
        return list;
    }
    const std::optional<core::CombatantId> active = _session->combat().activeCombatant();
    for (const core::InitiativeEntry& entry : _session->combat().turnOrder().entries()) {
        const core::Combatant* combatant = _session->combat().find(entry.combatant);
        if (combatant == nullptr) {
            continue;
        }
        list << QVariantMap{{"name", toQt(combatant->profile.name)},
                            {"total", entry.total},
                            {"side", sideName(entry.side)},
                            {"active", active == entry.combatant},
                            {"down", combatant->status == core::CombatantStatus::Down}};
    }
    return list;
}

QString CombatModel::activeName() const {
    if (_session == nullptr || !_inCombat) {
        return {};
    }
    const std::optional<core::CombatantId> active = _session->combat().activeCombatant();
    if (!active.has_value()) {
        return {};
    }
    const core::Combatant* combatant = _session->combat().find(*active);
    return combatant == nullptr ? QString() : toQt(combatant->profile.name);
}

QString CombatModel::activeResources() const {
    if (_session == nullptr || !_inCombat) {
        return {};
    }
    const std::optional<core::CombatantId> active = _session->combat().activeCombatant();
    if (!active.has_value()) {
        return {};
    }
    const core::Combatant* combatant = _session->combat().find(*active);
    if (combatant == nullptr) {
        return {};
    }
    QStringList parts;
    for (const core::ActionResource& resource : combatant->economy.resources()) {
        parts << toQt(resource.id) + " " + QString::number(resource.remaining);
    }
    return parts.join(QStringLiteral(" · "));
}

QStringList CombatModel::journal() const {
    QStringList lines;
    if (_session == nullptr) {
        return lines;
    }
    for (const std::string& line : _session->journal()) {
        lines << toQt(line);
    }
    return lines;
}

QVariantList CombatModel::fighters() const {
    QVariantList list;
    if (_session == nullptr || !_inCombat) {
        return list;
    }
    const core::CombatState& combat = _session->combat();
    const std::optional<core::CombatantId> active =
        ended() ? std::nullopt : combat.activeCombatant();
    for (const core::CombatantId id : combat.combatants()) {
        const core::Combatant* const combatant = combat.find(id);
        // Sorti : plus de figurine, plus d'interface.
        if (combatant == nullptr || combatant->status == core::CombatantStatus::Withdrawn) {
            continue;
        }
        const std::optional<core::GridPosition> anchor = combat.grid().positionOf(id);
        if (!anchor.has_value()) {
            continue;
        }
        const core::CombatantProfile& profile = combatant->profile;
        const bool down = combatant->status == core::CombatantStatus::Down;
        const HealthDisplay health = healthDisplayOf(profile, down);
        list << QVariantMap{{"column", anchor->column},
                            {"row", anchor->row},
                            {"footprint", std::max(1, combat.grid().sideOf(id))},
                            {"side", sideName(profile.side)},
                            {"active", active == id},
                            {"down", down},
                            {"hitPoints", health.hitPoints},
                            {"hitPointsRatio", health.ratio}};
    }
    return list;
}

QVariantList CombatModel::reachableCells() const {
    QVariantList list;
    if (_session == nullptr || !_inCombat || ended()) {
        return list;
    }
    const core::BattleGrid& grid = _session->combat().grid();
    const std::optional<core::ReachableArea> area = _session->combat().reachableArea();
    if (!area.has_value()) {
        return list;
    }
    for (int row = 0; row < grid.height(); ++row) {
        for (int column = 0; column < grid.width(); ++column) {
            const core::GridPosition cell{.column = column, .row = row};
            if (area->canEndAt(cell) && !grid.occupantAt(cell).has_value()) {
                list << QVariantMap{{"column", column}, {"row", row}};
            }
        }
    }
    return list;
}

QVariantList CombatModel::pathCells() const {
    QVariantList list;
    if (!playerTurn().has_value() || _session->combat().grid().occupantAt(_cursor).has_value()) {
        return list;
    }
    const std::optional<core::ReachableArea> area = _session->combat().reachableArea();
    const std::optional<core::Path> path =
        area.has_value() ? area->pathTo(_cursor) : std::optional<core::Path>{};
    if (path.has_value()) {
        for (const core::GridPosition cell : path->steps) {
            list << QVariantMap{{"column", cell.column}, {"row", cell.row}};
        }
    }
    return list;
}

QVariantList CombatModel::turnActions() const {
    QVariantList list;
    const std::optional<core::CombatantId> active = playerTurn();
    if (!active.has_value()) {
        return list;
    }
    const core::Combatant* combatant = _session->combat().find(*active);
    const bool action =
        combatant != nullptr && combatant->economy.remaining(core::ACTION_RESOURCE) > 0;
    const std::vector<TurnActionEntry> entries = turnActionsOf(*_session, *active);
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const bool needsAction = entries[i].kind != TurnActionKind::REACTION;
        list << QVariantMap{{"label", entries[i].label},
                            {"kind", kindName(entries[i].kind)},
                            {"enabled", !needsAction || action},
                            {"selected", std::cmp_equal(i, _selectedAction)}};
    }
    return list;
}

// --- Les gestes -------------------------------------------------------------------------------

void CombatModel::attackAt(core::CombatantId target, std::optional<std::size_t> index) {
    // La premiere attaque qui peut viser la cible : l'epee au contact, l'arc a distance. Sans
    // aucune, la premiere, pour que le refus dise pourquoi.
    const std::size_t chosen =
        index.has_value() ? *index : core::firstValidAttack(*_session, target).value_or(0);
    const core::ArenaAttack attack = _session->attack(target, chosen);
    switch (attack.result) {
        case core::ArenaActionResult::Done:
            // L'entree du journal elle-meme : chaque jet affiche se retrouve au journal.
            _status = attack.outcome.has_value() ? toQt(attack.outcome->describe()) : QString();
            break;
        case core::ArenaActionResult::OutOfReach:
            _status = tr("Hors d'allonge ou de portee.");
            break;
        case core::ArenaActionResult::TotalCover:
            _status = tr("Cible hors de vue : abri total.");
            break;
        case core::ArenaActionResult::NoAction:
            _status = tr("L'action de ce tour est deja depensee.");
            break;
        case core::ArenaActionResult::NoAttack:
            _status = tr("Ce combattant n'a aucune attaque.");
            break;
        case core::ArenaActionResult::NoActiveTurn:
        case core::ArenaActionResult::InvalidTarget:
            _status = tr("Attaque refusee.");
            break;
    }
}

void CombatModel::moveTo(core::GridPosition cell) {
    const core::MoveOutcome move = _session->move(cell);
    switch (move.result) {
        case core::MoveResult::Moved:
            _status = tr("Deplacement : %1 case(s).").arg(move.path.cost);
            break;
        case core::MoveResult::Unreachable:
            _status = tr("Case hors de portee de ce qui reste du deplacement.");
            break;
        case core::MoveResult::NoActiveTurn:
        case core::MoveResult::NotPlaced:
            _status = tr("Aucun combattant a deplacer.");
            break;
    }
    // Une attaque d'opportunite a pu terminer le combat.
    if (ended()) {
        _status = toQt(_session->journal().back());
    }
}

void CombatModel::tapCell(int column, int row) {
    if (_session == nullptr || !_inCombat || ended() || !acceptsInput()) {
        return;
    }
    core::CombatState& combat = _session->combat();
    const std::optional<core::CombatantId> active = combat.activeCombatant();
    if (!active.has_value()) {
        return;
    }
    _cursor = {.column = column, .row = row};
    if (const std::optional<core::CombatantId> target = combat.grid().occupantAt(_cursor)) {
        const core::Combatant* attacker = combat.find(*active);
        const core::Combatant* defender = combat.find(*target);
        if (attacker != nullptr && defender != nullptr &&
            defender->profile.side != attacker->profile.side) {
            // L'attaque choisie dans la barre si elle peut viser la cible, sinon la premiere qui
            // le peut : le clic ne refuse pas un tir que l'arc aurait reussi.
            std::optional<std::size_t> index;
            const std::vector<TurnActionEntry> entries = turnActionsOf(*_session, *active);
            if (_selectedAction >= 0 && std::cmp_less(_selectedAction, entries.size())) {
                const TurnActionEntry& chosen = entries[static_cast<std::size_t>(_selectedAction)];
                if (chosen.kind == TurnActionKind::ATTACK &&
                    core::checkTarget(combat, *active, *target,
                                      (*_session->attacks(*active))[chosen.attack]) ==
                        core::TargetCheck::Valid) {
                    index = chosen.attack;
                }
            }
            attackAt(*target, index);
            emitSceneChanged();
            return;
        }
    }
    moveTo(_cursor);
    emitSceneChanged();
}

void CombatModel::moveCursor(int columns, int rows) {
    if (_session == nullptr || !_inCombat) {
        return;
    }
    const core::BattleGrid& grid = _session->combat().grid();
    _cursor = {.column = std::clamp(_cursor.column + columns, 0, grid.width() - 1),
               .row = std::clamp(_cursor.row + rows, 0, grid.height() - 1)};
    emit cursorChanged();
}

void CombatModel::pointCursor(int column, int row) {
    if (_session == nullptr || !_inCombat) {
        return;
    }
    const core::BattleGrid& grid = _session->combat().grid();
    if (column < 0 || row < 0 || column >= grid.width() || row >= grid.height() ||
        (_cursor.column == column && _cursor.row == row)) {
        return;
    }
    _cursor = {.column = column, .row = row};
    emit cursorChanged();
}

void CombatModel::centerCursor() {
    if (_session == nullptr || !_inCombat) {
        return;
    }
    if (const std::optional<core::CombatantId> active = _session->combat().activeCombatant()) {
        if (const std::optional<core::GridPosition> cell =
                _session->combat().grid().positionOf(*active)) {
            _cursor = *cell;
        }
    }
    emit cursorChanged();
}

void CombatModel::cycleTarget(int step) {
    const std::optional<core::CombatantId> active = playerTurn();
    if (!active.has_value() || step == 0) {
        return;
    }
    const core::CombatState& combat = _session->combat();
    const core::CombatSide side = combat.find(*active)->profile.side;
    std::vector<std::pair<int, core::CombatantId>> targets;
    for (const core::CombatantId id : combat.combatants()) {
        const core::Combatant* c = combat.find(id);
        const std::optional<int> distance = core::gridDistance(combat, *active, id);
        if (c != nullptr && c->profile.side != side &&
            c->status == core::CombatantStatus::Standing && distance.has_value()) {
            targets.emplace_back(*distance, id);
        }
    }
    if (targets.empty()) {
        return;
    }
    std::ranges::sort(targets);
    const std::optional<core::CombatantId> current = combat.grid().occupantAt(_cursor);
    const auto found = std::ranges::find(
        targets, current, [](const auto& t) { return std::optional<core::CombatantId>(t.second); });
    const int count = static_cast<int>(targets.size());
    int next = step > 0 ? 0 : count - 1;
    if (found != targets.end()) {
        next = (((static_cast<int>(found - targets.begin()) + step) % count) + count) % count;
    }
    _cursor = *combat.grid().positionOf(targets[static_cast<std::size_t>(next)].second);
    emit cursorChanged();
}

void CombatModel::selectAction(int index) {
    const std::optional<core::CombatantId> active = playerTurn();
    if (!active.has_value()) {
        return;
    }
    const int count = static_cast<int>(turnActionsOf(*_session, *active).size());
    if (index < 0 || index >= count) {
        return;
    }
    _selectedAction = index;
    emit cursorChanged();
}

void CombatModel::cycleAction(int step) {
    const std::optional<core::CombatantId> active = playerTurn();
    if (!active.has_value()) {
        return;
    }
    const int count = static_cast<int>(turnActionsOf(*_session, *active).size());
    _selectedAction = (((_selectedAction + step) % count) + count) % count;
    emit cursorChanged();
}

void CombatModel::confirm() {
    const std::optional<core::CombatantId> active = playerTurn();
    if (!active.has_value() || !acceptsInput()) {
        return;
    }
    const std::vector<TurnActionEntry> entries = turnActionsOf(*_session, *active);
    const TurnActionEntry chosen = entries[static_cast<std::size_t>(
        std::clamp(_selectedAction, 0, static_cast<int>(entries.size()) - 1))];
    switch (chosen.kind) {
        case TurnActionKind::ATTACK: {
            const core::CombatState& combat = _session->combat();
            const std::optional<core::CombatantId> occupant = combat.grid().occupantAt(_cursor);
            if (!occupant.has_value()) {
                moveTo(_cursor);
            } else if (combat.find(*occupant)->profile.side != combat.find(*active)->profile.side) {
                attackAt(*occupant, chosen.attack);
            } else {
                _status = tr("Rien a faire sur cette case.");
            }
            emitSceneChanged();
            return;
        }
        case TurnActionKind::DODGE:
            dodge();
            return;
        case TurnActionKind::DISENGAGE:
            disengage();
            return;
        case TurnActionKind::DASH:
            dash();
            return;
        case TurnActionKind::REACTION: {
            const bool takes = !_session->takesOpportunities(*active);
            _session->setTakesOpportunities(*active, takes);
            _status = takes ? tr("Il frappera l'ennemi qui quitte son allonge.")
                            : tr("Il laissera passer l'ennemi qui quitte son allonge.");
            emit changed();
            return;
        }
    }
}

void CombatModel::dodge() {
    if (_session == nullptr || !_inCombat || ended() || !acceptsInput()) {
        return;
    }
    _status = _session->dodge() ? toQt(_session->journal().back())
                                : QStringLiteral("L'action de ce tour est deja depensee.");
    emit changed();
}

void CombatModel::disengage() {
    if (_session == nullptr || !_inCombat || ended() || !acceptsInput()) {
        return;
    }
    _status = _session->disengage() ? toQt(_session->journal().back())
                                    : QStringLiteral("L'action de ce tour est deja depensee.");
    emit changed();
}

void CombatModel::dash() {
    if (_session == nullptr || !_inCombat || ended() || !acceptsInput()) {
        return;
    }
    _status = _session->dash() ? toQt(_session->journal().back())
                               : QStringLiteral("L'action de ce tour est deja depensee.");
    emit changed();
}

void CombatModel::endTurn() {
    if (_session == nullptr || !_inCombat || !acceptsInput()) {
        return;
    }
    if (_session->endTurn()) {
        _status.clear();
    }
    playAiTurns();
    if (_session->outcome().has_value()) {
        _status = toQt(_session->journal().back());
    }
    emitSceneChanged();
}

void CombatModel::withdraw() {
    if (_session == nullptr || !_inCombat || !acceptsInput()) {
        return;
    }
    switch (_session->withdraw()) {
        case core::WithdrawResult::Withdrawn:
            _status = tr("Sorti du combat.");
            break;
        case core::WithdrawResult::NotEscapable:
            _status = tr("On ne fuit pas ce combat.");
            break;
        case core::WithdrawResult::NotInCombat:
            _status = tr("Personne a retirer.");
            break;
    }
    playAiTurns();
    emitSceneChanged();
}

}  // namespace hmi
