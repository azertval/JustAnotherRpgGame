// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/PartyModel.h"

#include <QVariantMap>
#include <filesystem>
#include <utility>
#include <vector>

#include "Core/Rpg/Party.h"
#include "HMI/Runtime/DemonstrationCharacter.h"
#include "HMI/Runtime/WorldModel.h"

namespace hmi {

namespace {

// Le signe des champs sans source, comme sur les autres ecrans.
constexpr const char* EMPTY_MARK = "—";

[[nodiscard]] QString toQt(const std::string& text) {
    return QString::fromStdString(text);
}

// Les points de vie « 12 / 15 » en part de leur maximum ; 0 si le texte n'en est pas.
[[nodiscard]] double hitPointsRatio(const QString& text) {
    const QStringList parts = text.split(QLatin1Char('/'));
    if (parts.size() != 2) {
        return 0.0;
    }
    bool currentOk = false;
    bool maximumOk = false;
    const double current = parts[0].trimmed().toDouble(&currentOk);
    const double maximum = parts[1].trimmed().toDouble(&maximumOk);
    return (currentOk && maximumOk && maximum > 0.0) ? current / maximum : 0.0;
}

}  // namespace

PartyModel::PartyModel(QObject* parent) : QObject(parent) {
    WorldModel* const world = WorldModel::current();
    if (world == nullptr) {
        return;
    }
    reloadSheets();
    // Les fiches se relisent quand le groupe change (LOT-141) : une montee de niveau, des points
    // de vie laisses par un combat, se voient a l'ecran de groupe sans le rouvrir.
    connect(world, &WorldModel::partyChanged, this, [this]() {
        reloadSheets();
        emit changed();
    });
}

void PartyModel::reloadSheets() {
    const WorldModel* const world = WorldModel::current();
    if (world == nullptr) {
        return;
    }
    std::vector<std::filesystem::path> files;
    std::vector<std::string> ids;
    for (const core::PartyCandidate& candidate : world->candidates()) {
        files.push_back(candidate.file);
        ids.push_back(candidate.id);
    }
    std::vector<DemonstrationCharacter> values = loadCharacterValues(files);
    _sheets.clear();
    for (std::size_t rank = 0; rank < values.size() && rank < ids.size(); ++rank) {
        _sheets.emplace(ids[rank], std::move(values[rank].sheet));
    }
}

QString PartyModel::sheetValue(const QString& characterId, const std::string& key) const {
    const auto sheet = _sheets.find(characterId.toStdString());
    if (sheet == _sheets.end()) {
        return QString::fromUtf8(EMPTY_MARK);
    }
    const auto value = sheet->second.find(key);
    return value != sheet->second.end() ? toQt(value->second) : QString::fromUtf8(EMPTY_MARK);
}

QString PartyModel::hitPointsOf(const QString& characterId) const {
    // Les points de vie de la fiche, ou ce que le dernier combat en a laisse (LOT-139) : le
    // registre de la partie ne dit que le courant, le maximum reste celui de la fiche.
    const QString ecrit = sheetValue(characterId, "sheet.hit_points");
    const WorldModel* const world = WorldModel::current();
    if (world == nullptr) {
        return ecrit;
    }
    const core::MemberRecord* const record = world->ledger().record(characterId.toStdString());
    if (record == nullptr || !record->hitPoints.has_value()) {
        return ecrit;
    }
    const QStringList parts = ecrit.split(QLatin1Char('/'));
    if (parts.size() != 2) {
        return ecrit;
    }
    return QString::number(*record->hitPoints) + " / " + parts[1].trimmed();
}

QVariantMap PartyModel::withSheet(QVariantMap row) const {
    const QString id = row.value(QStringLiteral("id")).toString();
    const QString hitPoints = hitPointsOf(id);
    row.insert(QStringLiteral("label"), row.value(QStringLiteral("name")));
    row.insert(QStringLiteral("value"), hitPoints);
    row.insert(QStringLiteral("ratio"), hitPointsRatio(hitPoints));
    row.insert(QStringLiteral("role"), sheetValue(id, "sheet.class"));
    row.insert(QStringLiteral("className"), sheetValue(id, "sheet.class"));
    row.insert(QStringLiteral("species"), sheetValue(id, "sheet.species"));
    row.insert(QStringLiteral("level"), sheetValue(id, "sheet.level"));
    row.insert(QStringLiteral("armorClass"), sheetValue(id, "sheet.armor_class"));
    row.insert(QStringLiteral("speed"), sheetValue(id, "sheet.speed"));
    return row;
}

QVariantList PartyModel::members() const {
    QVariantList rows;
    if (const WorldModel* const world = WorldModel::current()) {
        for (const QVariant& row : world->partyMembers()) {
            rows.append(withSheet(row.toMap()));
        }
    }
    return rows;
}

QVariantList PartyModel::candidates() const {
    QVariantList rows;
    if (const WorldModel* const world = WorldModel::current()) {
        for (const QVariant& row : world->partyCandidates()) {
            rows.append(withSheet(row.toMap()));
        }
    }
    return rows;
}

QString PartyModel::leaderName() const {
    const WorldModel* const world = WorldModel::current();
    return world != nullptr ? world->leaderName() : QString{};
}

QString PartyModel::leaderLevel() const {
    const WorldModel* const world = WorldModel::current();
    return world != nullptr ? sheetValue(world->leaderId(), "sheet.level") : QString{};
}

QString PartyModel::leaderHitPoints() const {
    const WorldModel* const world = WorldModel::current();
    return world != nullptr ? hitPointsOf(world->leaderId()) : QString{};
}

int PartyModel::size() const {
    const WorldModel* const world = WorldModel::current();
    return world != nullptr ? static_cast<int>(world->party().size()) : 0;
}

int PartyModel::maxSize() noexcept {
    return static_cast<int>(core::Party::MAX_MEMBERS);
}

bool PartyModel::toggleMember(const QString& characterId) {
    WorldModel* const world = WorldModel::current();
    return world != nullptr && world->toggleMember(characterId);
}

bool PartyModel::setLeader(const QString& characterId) {
    WorldModel* const world = WorldModel::current();
    return world != nullptr && world->setLeader(characterId);
}

bool PartyModel::moveMember(const QString& characterId, int offset) {
    WorldModel* const world = WorldModel::current();
    return world != nullptr && world->moveMember(characterId, offset);
}

}  // namespace hmi
