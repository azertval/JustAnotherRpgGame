// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/CombatModel.h"

#include <QVariantMap>
#include <algorithm>
#include <cstdint>
#include <string_view>
#include <utility>

#include "Core/Combat/BattleGrid.h"
#include "Core/Combat/CombatPreview.h"
#include "Core/Combat/Damage.h"
#include "Core/Combat/LineOfSight.h"
#include "Core/Combat/Pathfinding.h"
#include "Core/Rpg/Ability.h"
#include "Core/Rpg/ClassCapacities.h"
#include "Core/Rpg/Dice.h"
#include "Core/Rpg/Scale.h"
#include "Core/Rpg/Spell.h"
#include "HMI/HmiLog.h"
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
enum class TurnActionKind : std::uint8_t { ATTACK, SPELL, DODGE, DISENGAGE, DASH, WAIT, REACTION };

struct TurnActionEntry {
    TurnActionKind kind = TurnActionKind::ATTACK;
    /// L'indice de l'attaque, ou du sort, dans la liste de la session.
    std::size_t attack = 0;
    QString label;
    /// Faux pour un sort épuisé (`LOT-131`) : proposé grisé, jamais joué.
    bool available = true;
    /// Un sort d'action bonus (`LOT-134`) : il demande l'action bonus, pas l'action.
    bool bonusAction = false;
    /// Ce que la case écrit sous le nom (`LOT-140`) : le jet et les dés, la portée, les lancers.
    QString detail;
    /// Lancers restants d'un sort ; `-1` : à volonté, ou sans objet.
    int uses = -1;
    /// L'icône du cahier (`ui/icon/spell/<id>`), ou vide.
    QString iconKey;
};

[[nodiscard]] QStringList joined(const std::vector<std::string>& texts) {
    QStringList list;
    for (const std::string& text : texts) {
        list << toQt(text);
    }
    return list;
}

[[nodiscard]] QString signedNumber(int value) {
    return (value >= 0 ? QStringLiteral("+") : QString()) + QString::number(value);
}

// « 1d8+3 perforant », ou « 1d6 tranchant + 1d4 feu ».
[[nodiscard]] QString damageText(const std::vector<core::DamageClause>& clauses) {
    QStringList parts;
    for (const core::DamageClause& clause : clauses) {
        parts << toQt(core::formatDice(clause.dice)) + " " +
                     toQt(std::string(core::damageTypeLabel(clause.type)));
    }
    return parts.join(QStringLiteral(" + "));
}

// La portée d'une attaque en mètres : « 24 m », ou « 24 / 96 m » avec la longue portée.
[[nodiscard]] QString rangeText(const core::AttackProfile& profile) {
    if (!profile.range.has_value()) {
        return {};
    }
    const auto metres = [](int cells) {
        return QString::number(static_cast<double>(cells) * core::METERS_PER_TILE, 'g', 3);
    };
    if (profile.range->maximum > profile.range->normal) {
        return metres(profile.range->normal) + " / " + metres(profile.range->maximum) +
               CombatModel::tr(" m");
    }
    return metres(profile.range->normal) + CombatModel::tr(" m");
}

// Ce que la case d'une attaque écrit sous son nom : « +5 · 1d8+3 perforant · 24 / 96 m ».
[[nodiscard]] QString attackDetail(const core::AttackProfile& profile) {
    QStringList parts;
    parts << signedNumber(core::attackBonusOf(profile));
    if (!profile.damage.empty()) {
        parts << damageText(profile.damage);
    }
    if (const QString portee = rangeText(profile); !portee.isEmpty()) {
        parts << portee;
    }
    return parts.join(QStringLiteral(" · "));
}

// Ce que la case d'un sort écrit sous son nom : ses lancers, puis sa portée.
[[nodiscard]] QString spellDetail(const core::ArenaSpell& spell) {
    QStringList parts;
    parts << (spell.uses < 0 ? CombatModel::tr("a volonte")
                             : CombatModel::tr("%1 lancer(s)").arg(spell.uses));
    if (const QString portee = rangeText(spell.attack); !portee.isEmpty()) {
        parts << portee;
    }
    return parts.join(QStringLiteral(" · "));
}

[[nodiscard]] QString spellIconKey(const core::ArenaSpell& spell) {
    return QStringLiteral("ui/icon/spell/") + toQt(spell.id);
}

// L'icone d'une action du tour (piece `ui/icon/action` du cahier, LOT-141) : l'attaque selon sa
// portee, les actions du Manuel par leur nom.
[[nodiscard]] QString actionIconKey(const char* member) {
    return QStringLiteral("ui/icon/action/") + QLatin1String(member);
}

[[nodiscard]] QString capacityIconKey(const core::Capacity& capacity) {
    return QStringLiteral("ui/icon/capacity/") +
           toQt(capacity.iconId.empty() ? capacity.id : capacity.iconId);
}

// Les états d'un combattant, tels que l'écran les écrit : ceux de la session (LOT-137), puis
// « Ensanglante » et « Esquive » que la table voit aussi.
[[nodiscard]] QStringList conditionLabels(const core::ArenaSession& session, core::CombatantId id) {
    QStringList labels;
    const core::Combatant* combatant = session.combat().find(id);
    if (combatant == nullptr) {
        return labels;
    }
    for (const core::CombatCondition condition : session.conditionsOf(id)) {
        QString label = toQt(std::string(core::combatConditionLabel(condition)));
        if (!label.isEmpty()) {
            label[0] = label[0].toUpper();
        }
        labels << label;
    }
    const bool down = combatant->status == core::CombatantStatus::Down ||
                      combatant->status == core::CombatantStatus::Dead;
    if (core::isBloodied(combatant->profile) && !down) {
        labels << CombatModel::tr("Ensanglante");
    }
    if (session.isDodging(id)) {
        labels << CombatModel::tr("Esquive");
    }
    return labels;
}

// Deux lettres pour un jeton qui manque : « Bandit archer » → « BA », « Rat #2 » → « R2 »,
// « Grom » → « Gr ».
[[nodiscard]] QString initialsOf(const QString& name) {
    const QStringList words = name.split(' ', Qt::SkipEmptyParts);
    if (words.isEmpty()) {
        return {};
    }
    QString initials = words.front().left(1).toUpper();
    if (words.size() > 1) {
        const QString& last = words.back();
        const QString digits = last.section('#', -1);
        initials += digits.front().isDigit() ? digits.left(1) : last.left(1).toUpper();
    } else if (words.front().size() > 1) {
        initials += words.front().mid(1, 1).toLower();
    }
    return initials;
}

[[nodiscard]] QVariantMap previewLine(const QString& label, const QString& value) {
    return QVariantMap{{"label", label}, {"value", value}};
}

// Les actions du combattant `active` : ses attaques, ses sorts, puis les actions du Manuel, puis
// sa réaction.
[[nodiscard]] std::vector<TurnActionEntry> turnActionsOf(const core::ArenaSession& session,
                                                         core::CombatantId active) {
    std::vector<TurnActionEntry> entries;
    if (const std::vector<core::AttackProfile>* attacks = session.attacks(active)) {
        for (std::size_t i = 0; i < attacks->size(); ++i) {
            entries.push_back(
                {.kind = TurnActionKind::ATTACK,
                 .attack = i,
                 .label = toQt((*attacks)[i].label),
                 .detail = attackDetail((*attacks)[i]),
                 .iconKey = actionIconKey(
                     (*attacks)[i].kind == core::AttackKind::Ranged ? "ranged" : "melee")});
        }
    }
    if (const std::vector<core::ArenaSpell>* spells = session.spells(active)) {
        for (std::size_t i = 0; i < spells->size(); ++i) {
            const core::ArenaSpell& spell = (*spells)[i];
            // Un sort epuise reste dans la barre, grise : le joueur voit ce qu'un repos rendra.
            entries.push_back(
                {.kind = TurnActionKind::SPELL,
                 .attack = i,
                 .label =
                     spell.uses < 0
                         ? CombatModel::tr("Sort : %1").arg(toQt(spell.name))
                         : CombatModel::tr("Sort : %1 (%2)").arg(toQt(spell.name)).arg(spell.uses),
                 // L'arme spirituelle invoquee frappe sans lancer (LOT-134) : elle reste
                 // proposee, lancers epuises ou non.
                 .available = spell.available() ||
                              (spell.effect.has_value() &&
                               spell.effect->kind == core::SpellEffectKind::SpiritualWeapon &&
                               session.hasEffect(active, core::SpellEffectKind::SpiritualWeapon)),
                 .bonusAction = spell.bonusAction,
                 .detail = spellDetail(spell),
                 .uses = spell.uses,
                 .iconKey = spellIconKey(spell)});
        }
    }
    entries.push_back({.kind = TurnActionKind::DODGE,
                       .attack = 0,
                       .label = CombatModel::tr("Esquiver"),
                       .iconKey = actionIconKey("dodge")});
    entries.push_back({.kind = TurnActionKind::DISENGAGE,
                       .attack = 0,
                       .label = CombatModel::tr("Se desengager"),
                       .iconKey = actionIconKey("disengage")});
    entries.push_back({.kind = TurnActionKind::DASH,
                       .attack = 0,
                       .label = CombatModel::tr("Se precipiter"),
                       .iconKey = actionIconKey("dash")});
    // Attendre : rendre la main sans rien depenser (LOT-140) -- la fin du tour a sa case dans la
    // barre, pour la souris comme pour les touches numerotees.
    entries.push_back({.kind = TurnActionKind::WAIT,
                       .attack = 0,
                       .label = CombatModel::tr("Attendre"),
                       .iconKey = actionIconKey("wait")});
    entries.push_back({.kind = TurnActionKind::REACTION,
                       .attack = 0,
                       .label = session.takesOpportunities(active)
                                    ? CombatModel::tr("Reaction : saisir les opportunites")
                                    : CombatModel::tr("Reaction : laisser passer"),
                       .iconKey = actionIconKey("reaction")});
    return entries;
}

[[nodiscard]] QString kindName(TurnActionKind kind) {
    switch (kind) {
        case TurnActionKind::ATTACK:
            return QStringLiteral("attack");
        case TurnActionKind::SPELL:
            return QStringLiteral("spell");
        case TurnActionKind::DODGE:
            return QStringLiteral("dodge");
        case TurnActionKind::DISENGAGE:
            return QStringLiteral("disengage");
        case TurnActionKind::DASH:
            return QStringLiteral("dash");
        case TurnActionKind::WAIT:
            return QStringLiteral("wait");
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

[[nodiscard]] HealthDisplay healthDisplayOf(const core::CombatantProfile& profile, bool down,
                                            bool dead) {
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
    if (dead) {
        display.hitPoints = CombatModel::tr("mort");
        display.ratio = 0.0;
    } else if (down) {
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
    return loadHeroSource(playedCharacterFile(), problems);
}

std::optional<HeroContestantSource> CombatModel::loadHeroSource(
    const std::filesystem::path& characterFile, std::vector<std::string>& problems) {
    // Le heros, par le meme chemin que la fiche : un seul chargement, une seule verite sur ce
    // qu'il porte (LOT-87, LOT-112).
    const DemonstrationState demonstration = loadDemonstrationState(characterFile);
    if (demonstration.sheet.name.empty()) {
        problems.emplace_back("fiche absente ou illisible : " + characterFile.string());
        return std::nullopt;
    }
    HeroContestantSource hero{
        .sheet = demonstration.sheet,
        .proficiency = core::proficiencyBonus(demonstration.sheet, demonstration.experience),
        .armorClass = core::derivedStatsFor(demonstration.sheet, demonstration.inventory,
                                            demonstration.lookup(), demonstration.rules,
                                            demonstration.encumbrance)
                          .armorClass,
        .weapon = std::nullopt,
        .spells = {}};
    if (const core::Weapon* weapon = demonstration.equipment.findWeapon(
            demonstration.inventory.at(core::EquipmentSlot::MainHand))) {
        hero.weapon = *weapon;
    }
    // Les sorts que la fiche connait et que le moteur sait jouer (LOT-131) ; ceux qu'il ne joue
    // pas encore sont dits, pas tus (EX-RPG-051).
    if (const core::PlayableClass* playableClass =
            demonstration.options.findClass(demonstration.sheet.classId)) {
        std::vector<std::string> skipped;
        hero.spells = core::arenaSpellsFor(demonstration.sheet, *playableClass,
                                           demonstration.options.spells, hero.proficiency, skipped);
        for (const std::string& spell : skipped) {
            // Pas un probleme de montage : le combat se joue, ce sort n'y parait pas.
            HMI_LOG_WARNING("Sort connu du heros mais sans mecanisme joue (EX-RPG-051) : " + spell);
        }
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
        const Identity identity = identityOf(entry.combatant);
        list << QVariantMap{{"name", toQt(combatant->profile.name)},
                            {"total", entry.total},
                            {"side", sideName(entry.side)},
                            {"active", active == entry.combatant},
                            {"down", combatant->status == core::CombatantStatus::Down ||
                                         combatant->status == core::CombatantStatus::Dead},
                            {"token", identity.token},
                            {"initials", initialsOf(toQt(combatant->profile.name))}};
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
        // Le mort reste sur la grille, couche comme qui est a terre (LOT-137).
        const bool dead = combatant->status == core::CombatantStatus::Dead;
        const bool down = dead || combatant->status == core::CombatantStatus::Down;
        const HealthDisplay health = healthDisplayOf(profile, down, dead);
        list << QVariantMap{{"column", anchor->column},
                            {"row", anchor->row},
                            {"footprint", std::max(1, combat.grid().sideOf(id))},
                            {"side", sideName(profile.side)},
                            {"active", active == id},
                            {"down", down},
                            {"dead", dead},
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
    // Une attaque que l'action deja prise a laissee (Extra Attack, LOT-132) : les attaques
    // restent proposees, et elles seules.
    const bool extraAttack =
        combatant != nullptr && combatant->economy.remaining(core::EXTRA_ATTACK_RESOURCE) > 0;
    const bool bonusAction =
        combatant != nullptr && combatant->economy.remaining(core::BONUS_ACTION_RESOURCE) > 0;
    const std::vector<TurnActionEntry> entries = turnActionsOf(*_session, *active);
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const bool needsAction =
            entries[i].kind != TurnActionKind::REACTION && entries[i].kind != TurnActionKind::WAIT;
        const bool affordable =
            entries[i].bonusAction
                ? bonusAction
                : action || (extraAttack && entries[i].kind == TurnActionKind::ATTACK);
        list << QVariantMap{{"label", entries[i].label},
                            {"kind", kindName(entries[i].kind)},
                            {"enabled", (!needsAction || affordable) && entries[i].available},
                            {"selected", std::cmp_equal(i, _selectedAction)},
                            {"detail", entries[i].detail},
                            {"uses", entries[i].uses},
                            {"iconKey", entries[i].iconKey}};
    }
    return list;
}

int CombatModel::round() const {
    return _session != nullptr && _inCombat ? _session->combat().round() : 0;
}

QVariantMap CombatModel::activeProfile() const {
    QVariantMap map;
    if (_session == nullptr || !_inCombat) {
        return map;
    }
    const std::optional<core::CombatantId> active = _session->combat().activeCombatant();
    const core::Combatant* combatant =
        active.has_value() ? _session->combat().find(*active) : nullptr;
    if (combatant == nullptr) {
        return map;
    }
    const core::CombatantProfile& profile = combatant->profile;
    const bool dead = combatant->status == core::CombatantStatus::Dead;
    const bool down = dead || combatant->status == core::CombatantStatus::Down;
    const Identity identity = identityOf(*active);
    const HealthDisplay health = healthDisplayOf(profile, down, dead);
    map.insert("name", toQt(profile.name));
    map.insert("side", sideName(profile.side));
    map.insert("classId", identity.classId);
    map.insert("level", identity.level);
    map.insert("portrait", identity.portrait);
    map.insert("token", identity.token);
    map.insert("hitPoints", health.hitPoints);
    map.insert("hitPointsRatio", health.ratio);
    map.insert("armorClass", profile.armorClass);
    map.insert("speed", QString::number(
                            static_cast<double>(profile.movement) * core::METERS_PER_TILE, 'g', 3) +
                            tr(" m"));
    map.insert("conditions", conditionLabels(*_session, *active));
    // Les ressources du tour : ce qu'il en reste, sur ce que le tour en donne.
    const auto resource = [&](std::string_view id) -> std::pair<int, int> {
        for (const core::ActionResource& r : combatant->economy.resources()) {
            if (r.id == id) {
                return {r.remaining, r.perTurn};
            }
        }
        return {0, 0};
    };
    const auto [action, actionMax] = resource(core::ACTION_RESOURCE);
    const auto [bonus, bonusMax] = resource(core::BONUS_ACTION_RESOURCE);
    const auto [movement, movementMax] = resource(core::MOVEMENT_RESOURCE);
    map.insert("action", action);
    map.insert("actionMax", actionMax);
    map.insert("bonusAction", bonus);
    map.insert("bonusActionMax", bonusMax);
    map.insert("movement", movement);
    map.insert("movementMax", movementMax);
    // Ce que la fiche apporte au combat (LOT-131) : la table ne voit pas celles d'un ennemi.
    QVariantList capacities;
    QVariantList spells;
    if (profile.side == core::CombatSide::Allies) {
        for (const core::Capacity& capacity : _session->capacitiesOf(*active)) {
            capacities << QVariantMap{{"id", toQt(capacity.id)},
                                      {"name", toQt(capacity.name)},
                                      {"iconKey", capacityIconKey(capacity)},
                                      {"text", toQt(capacity.text)},
                                      {"narrative", capacity.narrative}};
        }
        if (const std::vector<core::ArenaSpell>* known = _session->spells(*active)) {
            for (const core::ArenaSpell& spell : *known) {
                spells << QVariantMap{{"id", toQt(spell.id)},
                                      {"name", toQt(spell.name)},
                                      {"level", spell.level},
                                      {"uses", spell.uses},
                                      {"iconKey", spellIconKey(spell)}};
            }
        }
    }
    map.insert("capacities", capacities);
    map.insert("spells", spells);
    return map;
}

QVariantMap CombatModel::preview() const {
    QVariantMap map;
    const std::optional<core::CombatantId> active = playerTurn();
    if (!active.has_value()) {
        return map;
    }
    const core::CombatState& combat = _session->combat();
    const std::vector<TurnActionEntry> entries = turnActionsOf(*_session, *active);
    const TurnActionEntry& chosen = entries[static_cast<std::size_t>(
        std::clamp(_selectedAction, 0, static_cast<int>(entries.size()) - 1))];
    QVariantList lines;
    QVariantList capacities;
    map.insert("valid", true);
    map.insert("expected", QString());
    switch (chosen.kind) {
        case TurnActionKind::DODGE:
            map.insert("kind", QStringLiteral("action"));
            map.insert("title", chosen.label);
            lines << previewLine(tr("Effet"), tr("Les attaques contre lui sont desavantagees "
                                                 "jusqu'a son prochain tour, s'il voit "
                                                 "l'attaquant."));
            break;
        case TurnActionKind::DISENGAGE:
            map.insert("kind", QStringLiteral("action"));
            map.insert("title", chosen.label);
            lines << previewLine(tr("Effet"), tr("Ses deplacements ne provoquent plus d'attaque "
                                                 "d'opportunite ce tour-ci."));
            break;
        case TurnActionKind::DASH:
            map.insert("kind", QStringLiteral("action"));
            map.insert("title", chosen.label);
            lines << previewLine(tr("Effet"),
                                 tr("Un deplacement supplementaire egal a sa vitesse."));
            break;
        case TurnActionKind::WAIT:
            map.insert("kind", QStringLiteral("action"));
            map.insert("title", chosen.label);
            lines << previewLine(tr("Effet"), tr("Rend la main : le tour passe au suivant, sans "
                                                 "rien depenser."));
            break;
        case TurnActionKind::REACTION:
            map.insert("kind", QStringLiteral("action"));
            map.insert("title", chosen.label);
            lines << previewLine(tr("Effet"), _session->takesOpportunities(*active)
                                                  ? tr("Il frappera l'ennemi qui quitte son "
                                                       "allonge. Confirmer pour le laisser passer.")
                                                  : tr("Il laissera passer l'ennemi qui quitte son "
                                                       "allonge. Confirmer pour frapper."));
            break;
        case TurnActionKind::ATTACK:
        case TurnActionKind::SPELL: {
            const std::optional<core::CombatantId> occupant = combat.grid().occupantAt(_cursor);
            const core::Combatant* self = combat.find(*active);
            const core::Combatant* other = occupant.has_value() ? combat.find(*occupant) : nullptr;
            if (chosen.kind == TurnActionKind::SPELL) {
                previewSpell(map, lines, chosen.attack, other);
                break;
            }
            if (other != nullptr && self != nullptr && other->profile.side != self->profile.side) {
                previewAttack(map, lines, capacities, *occupant, chosen.attack);
                break;
            }
            if (other != nullptr) {
                map.insert("kind", QStringLiteral("attack"));
                map.insert("title", toQt(other->profile.name));
                map.insert("valid", false);
                lines << previewLine(tr("Cible"), tr("Un allie : rien a frapper ici."));
                break;
            }
            previewMove(map, lines);
            break;
        }
    }
    map.insert("lines", lines);
    map.insert("capacities", capacities);
    return map;
}

void CombatModel::previewAttack(QVariantMap& map, QVariantList& lines, QVariantList& capacities,
                                core::CombatantId target, std::size_t attackIndex) const {
    const core::Combatant* other = _session->combat().find(target);
    const std::optional<core::AttackPreview> attack =
        core::previewAttack(*_session, target, attackIndex);
    map.insert("kind", QStringLiteral("attack"));
    if (!attack.has_value() || other == nullptr) {
        map.insert("title", QString());
        map.insert("valid", false);
        return;
    }
    map.insert("title", toQt(attack->label) + QStringLiteral(" › ") + toQt(other->profile.name));
    switch (attack->check) {
        case core::TargetCheck::Valid:
            break;
        case core::TargetCheck::OutOfReach:
            map.insert("valid", false);
            lines << previewLine(tr("Cible"), tr("Hors d'allonge ou de portee."));
            return;
        case core::TargetCheck::TotalCover:
            map.insert("valid", false);
            lines << previewLine(tr("Cible"), tr("Hors de vue : abri total."));
            return;
        case core::TargetCheck::NotOnGrid:
            map.insert("valid", false);
            lines << previewLine(tr("Cible"), tr("Cible invalide."));
            return;
    }
    // Le jet tel qu'il sera jete : d20 + bonus contre la CA vue, et la chance qui en sort.
    QString ca = tr("CA %1").arg(attack->armorClass);
    if (attack->cover != core::Cover::None) {
        ca += tr(", dont %1").arg(toQt(std::string(core::coverLabel(attack->cover))));
    }
    lines << previewLine(tr("Toucher"), tr("d20 %1 contre %2 · %3 %")
                                            .arg(signedNumber(attack->attackBonus))
                                            .arg(ca)
                                            .arg(attack->hitPercent()));
    for (const core::Modifier& modifier : attack->capacityModifiers) {
        lines << previewLine(toQt(modifier.source), signedNumber(modifier.value) + tr(" au jet"));
    }
    if (!attack->advantages.empty()) {
        lines << previewLine(tr("Avantage"), joined(attack->advantages).join(QStringLiteral(", ")));
    }
    if (!attack->disadvantages.empty()) {
        lines << previewLine(tr("Desavantage"),
                             joined(attack->disadvantages).join(QStringLiteral(", ")));
    }
    if (const std::vector<core::AttackProfile>* attacks = _session->attacks(*playerTurn());
        attacks != nullptr && attackIndex < attacks->size()) {
        lines << previewLine(tr("Degats"), damageText((*attacks)[attackIndex].damage));
    }
    // Les capacites qui jouent, et celles qui ne jouent pas -- et pourquoi (LOT-140).
    for (const core::ExtraDamagePreview& extra : attack->extraDamage) {
        capacities << QVariantMap{{"name", toQt(extra.source)},
                                  {"dice", toQt(core::formatDice(extra.dice))},
                                  {"applies", extra.applies},
                                  {"reason", toQt(extra.reason)}};
    }
    const long long tenths = attack->expectedTenths();
    map.insert("expected", QString::number(tenths / 10) + tr(",") + QString::number(tenths % 10));
}

void CombatModel::previewSpell(QVariantMap& map, QVariantList& lines, std::size_t spellIndex,
                               const core::Combatant* target) const {
    const std::optional<core::CombatantId> active = playerTurn();
    const std::vector<core::ArenaSpell>* spells =
        active.has_value() ? _session->spells(*active) : nullptr;
    map.insert("kind", QStringLiteral("spell"));
    if (spells == nullptr || spellIndex >= spells->size()) {
        map.insert("title", QString());
        map.insert("valid", false);
        return;
    }
    const core::ArenaSpell& spell = (*spells)[spellIndex];
    map.insert("title", target != nullptr
                            ? toQt(spell.name) + QStringLiteral(" › ") + toQt(target->profile.name)
                            : toQt(spell.name));
    lines << previewLine(tr("Lancers"),
                         spell.uses < 0 ? tr("a volonte") : tr("%1 restant(s)").arg(spell.uses));
    if (const QString portee = rangeText(spell.attack); !portee.isEmpty()) {
        lines << previewLine(tr("Portee"), portee);
    }
    if (spell.areaRadius > 0) {
        lines << previewLine(tr("Zone"), tr("sphere de %1 case(s) de rayon").arg(spell.areaRadius));
    } else if (spell.maxTargets > 1) {
        lines << previewLine(tr("Cibles"), tr("jusqu'a %1").arg(spell.maxTargets));
    }
    switch (spell.mechanism) {
        case core::SpellMechanism::AttackRoll:
            lines << previewLine(
                tr("Jet"),
                tr("d20 %1 contre la CA%2")
                    .arg(signedNumber(core::attackBonusOf(spell.attack)))
                    .arg(spell.projectiles > 1 ? tr(", %1 projectile(s)").arg(spell.projectiles)
                                               : QString()));
            break;
        case core::SpellMechanism::SavingThrow:
            lines << previewLine(
                tr("Jet"),
                tr("sauvegarde de %1 contre DD %2%3")
                    .arg(spell.save.has_value() ? toQt(std::string(core::abilityName(*spell.save)))
                                                : QString())
                    .arg(spell.saveDc)
                    .arg(spell.saveEffect == core::SaveEffect::Half ? tr(", degats de moitie")
                                                                    : tr(", annule")));
            break;
        case core::SpellMechanism::AutoHit:
            lines << previewLine(tr("Jet"),
                                 spell.projectiles > 1
                                     ? tr("touche, %1 projectile(s)").arg(spell.projectiles)
                                     : tr("touche"));
            break;
        case core::SpellMechanism::Effect:
            lines << previewLine(
                tr("Effet"), spell.effect.has_value()
                                 ? toQt(std::string(core::spellEffectKindName(spell.effect->kind)))
                                 : QString());
            break;
        case core::SpellMechanism::Healing:
            if (spell.healing.has_value()) {
                lines << previewLine(tr("Soin"), toQt(core::formatDice(*spell.healing)));
            }
            break;
        case core::SpellMechanism::Stabilize:
            lines << previewLine(tr("Effet"), tr("stabilise un mourant"));
            break;
        case core::SpellMechanism::Revive:
            if (spell.revival.has_value()) {
                lines << previewLine(
                    tr("Effet"),
                    tr("rend %1 point(s) de vie a un mort recent").arg(spell.revival->hitPoints));
            }
            break;
    }
    if (!spell.attack.damage.empty()) {
        lines << previewLine(tr("Degats"), damageText(spell.attack.damage));
    }
    if (spell.concentration) {
        lines << previewLine(tr("Concentration"), tr("un seul sort de concentration a la fois"));
    }
    if (spell.bonusAction) {
        lines << previewLine(tr("Action"), tr("action bonus"));
    }
    if (!spell.available() &&
        !(spell.effect.has_value() &&
          spell.effect->kind == core::SpellEffectKind::SpiritualWeapon && active.has_value() &&
          _session->hasEffect(*active, core::SpellEffectKind::SpiritualWeapon))) {
        map.insert("valid", false);
        lines << previewLine(tr("Cible"), tr("Sort epuise : un repos long le rendra."));
        return;
    }
    if (target != nullptr && active.has_value()) {
        switch (core::checkTarget(_session->combat(), *active, target->id, spell.attack)) {
            case core::TargetCheck::Valid:
                break;
            case core::TargetCheck::OutOfReach:
                map.insert("valid", false);
                lines << previewLine(tr("Cible"), tr("Hors de portee."));
                break;
            case core::TargetCheck::TotalCover:
                map.insert("valid", false);
                lines << previewLine(tr("Cible"), tr("Hors de vue : abri total."));
                break;
            case core::TargetCheck::NotOnGrid:
                map.insert("valid", false);
                lines << previewLine(tr("Cible"), tr("Cible invalide."));
                break;
        }
    }
}

void CombatModel::previewMove(QVariantMap& map, QVariantList& lines) const {
    map.insert("kind", QStringLiteral("move"));
    map.insert("title", tr("Deplacement"));
    const core::MovePreview move = core::previewMove(*_session, _cursor);
    if (!move.path.has_value()) {
        map.insert("valid", false);
        lines << previewLine(tr("Chemin"), tr("Case hors d'atteinte ce tour-ci."));
        return;
    }
    lines << previewLine(
        tr("Chemin"),
        tr("%1 case(s), il en restera %2").arg(move.path->cost).arg(move.movementLeft));
    if (!move.opportunities.empty()) {
        QStringList names;
        for (const core::CombatantId id : move.opportunities) {
            if (const core::Combatant* c = _session->combat().find(id)) {
                names << toQt(c->profile.name);
            }
        }
        lines << previewLine(tr("Opportunite"), names.join(QStringLiteral(", ")));
    }
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
        case core::ArenaActionResult::NoSpell:
        case core::ArenaActionResult::Exhausted:
            _status = tr("Attaque refusee.");
            break;
    }
}

void CombatModel::castAt(core::CombatantId target, std::size_t index) {
    const core::ArenaAttack cast = _session->castSpell(target, index);
    switch (cast.result) {
        case core::ArenaActionResult::Done:
            // La ligne du sort : un sort sans jet d'attaque n'a pas d'issue d'attaque (LOT-133).
            _status = toQt(cast.summary);
            break;
        case core::ArenaActionResult::Exhausted:
            _status = tr("Sort epuise : un repos long le rendra.");
            break;
        case core::ArenaActionResult::NoSpell:
            _status = tr("Ce combattant n'a pas ce sort.");
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
        // Un sort choisi dans la barre se lance sur la creature cliquee, alliee ou non : c'est
        // le sort qui sait qui il vise (LOT-133), et son refus le dit -- on ne retombe pas sur
        // l'arme en silence.
        const std::vector<TurnActionEntry> entries = turnActionsOf(*_session, *active);
        if (_selectedAction >= 0 && std::cmp_less(_selectedAction, entries.size()) &&
            entries[static_cast<std::size_t>(_selectedAction)].kind == TurnActionKind::SPELL) {
            castAt(*target, entries[static_cast<std::size_t>(_selectedAction)].attack);
            emitSceneChanged();
            return;
        }
        const core::Combatant* attacker = combat.find(*active);
        const core::Combatant* defender = combat.find(*target);
        if (attacker != nullptr && defender != nullptr &&
            defender->profile.side != attacker->profile.side) {
            // L'attaque choisie dans la barre si elle peut viser la cible, sinon la premiere qui
            // le peut : le clic ne refuse pas un tir que l'arc aurait reussi.
            std::optional<std::size_t> index;
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
        case TurnActionKind::ATTACK:
        case TurnActionKind::SPELL: {
            const core::CombatState& combat = _session->combat();
            const std::optional<core::CombatantId> occupant = combat.grid().occupantAt(_cursor);
            if (!occupant.has_value()) {
                moveTo(_cursor);
            } else if (chosen.kind == TurnActionKind::SPELL) {
                // Le sort sait qui il vise, allie ou ennemi (LOT-133).
                castAt(*occupant, chosen.attack);
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
        case TurnActionKind::WAIT:
            endTurn();
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
