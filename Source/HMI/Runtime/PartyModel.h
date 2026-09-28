// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QtQmlIntegration>
#include <map>
#include <string>

/**
 * @file HMI/Runtime/PartyModel.h
 * @brief Le groupe tel que l'écran « Groupe » et l'affichage tête haute le montrent (`LOT-138`).
 */

namespace hmi {

/**
 * @brief La vue-modèle du groupe : qui en est, qui mène, et ce que dit la fiche de chacun.
 *
 * ## Ce qu'elle tient, et ce qu'elle ne tient pas
 *
 * Le groupe **appartient à la partie** (`hmi::WorldModel`, un singleton) : ce modèle n'en garde
 * aucune copie. Il lit, à sa construction, la fiche de chaque personnage qu'on peut prendre — les
 * catalogues une fois, les fiches ensuite (`hmi::loadCharacterValues`) — et relit la composition
 * dans la partie à chaque changement. Ce que l'écran demande (prendre, retirer, mener, avancer
 * dans l'ordre de marche) passe à la partie, qui décide et l'annonce (`partyChanged`).
 *
 * Sans partie (l'atelier), le groupe est vide et le modèle aussi.
 */
class PartyModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    /**
     * Le groupe, meneur en tête, comme l'affichage tête haute le lit : `id`, `label` (le nom),
     * `value` (les points de vie, « 15 / 15 »), `ratio` (leur part), `role` (la classe),
     * `portrait`, `leader`.
     */
    Q_PROPERTY(QVariantList members READ members NOTIFY changed)
    /**
     * Les personnages qu'on peut prendre, dans l'ordre de leurs identifiants : les clés de
     * `members`, plus `rank` (rang dans l'ordre de marche, -1 hors du groupe), `species`,
     * `className`, `level`, `armorClass`, `speed`.
     */
    Q_PROPERTY(QVariantList candidates READ candidates NOTIFY changed)
    /// Le nom, le niveau et les points de vie du meneur : le portrait principal du HUD.
    Q_PROPERTY(QString leaderName READ leaderName NOTIFY changed)
    Q_PROPERTY(QString leaderLevel READ leaderLevel NOTIFY changed)
    Q_PROPERTY(QString leaderHitPoints READ leaderHitPoints NOTIFY changed)
    /// Le nombre de membres, et le plafond (quatre).
    Q_PROPERTY(int size READ size NOTIFY changed)
    Q_PROPERTY(int maxSize READ maxSize CONSTANT)

public:
    explicit PartyModel(QObject* parent = nullptr);

    [[nodiscard]] QVariantList members() const;
    [[nodiscard]] QVariantList candidates() const;
    [[nodiscard]] QString leaderName() const;
    [[nodiscard]] QString leaderLevel() const;
    [[nodiscard]] QString leaderHitPoints() const;
    [[nodiscard]] int size() const;
    [[nodiscard]] static int maxSize() noexcept;

    /// @brief Prend @p characterId dans le groupe, ou l'en retire (`WorldModel::toggleMember`).
    Q_INVOKABLE bool toggleMember(const QString& characterId);
    /// @brief Fait de @p characterId le meneur (`WorldModel::setLeader`).
    Q_INVOKABLE bool setLeader(const QString& characterId);
    /// @brief Avance (-1) ou recule (+1) @p characterId dans l'ordre de marche.
    Q_INVOKABLE bool moveMember(const QString& characterId, int offset);

signals:
    void changed();

private:
    /// Les valeurs de fiche d'un personnage (`sheet.*`), par identifiant.
    std::map<std::string, std::map<std::string, std::string>, std::less<>> _sheets;

    /// @return La valeur @p key de la fiche de @p characterId, ou un tiret.
    [[nodiscard]] QString sheetValue(const QString& characterId, const std::string& key) const;
    /// @return Les points de vie « courant / maximum » de @p characterId : ceux de la fiche, ou
    ///         ce que le dernier combat en a laissé (`LOT-139`).
    [[nodiscard]] QString hitPointsOf(const QString& characterId) const;
    /// @return La ligne @p row de la partie, complétée de la fiche.
    [[nodiscard]] QVariantMap withSheet(QVariantMap row) const;
};

}  // namespace hmi
