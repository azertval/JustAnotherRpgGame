// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QtQmlIntegration>

/**
 * @file HMI/Runtime/WorldMapModel.h
 * @brief Les trois niveaux de l'écran « Carte » — monde, région, ville — exposés au QML (`LOT-94`,
 * `LOT-95`).
 */

namespace hmi {

/**
 * @brief Vue-modèle de l'écran « Carte » : le monde, ses treize régions, les plans de ville.
 *
 * Charge l'atlas du `LOT-37` (`World/`, à côté de l'exécutable) et les positions relevées sur les
 * cartes de l'auteur (`Maps/world-maps.json`), puis les joint par `hmi::joinWorldMaps`. Chaque
 * écart entre les deux est journalisé plutôt que tu.
 *
 * Les images sont désignées par leur **nom de fichier** : le formulaire les cherche sous
 * `Elements/Assets/Maps/`, le même chemin dans l'atelier, depuis les sources et dans la ressource.
 *
 * - `regions` : une table par région — `regionId`, `name`, `image`, `x`, `y` (repère sur le
 *   monde), `frame` (`x`, `y`, `width`, `height` : d'où part le zoom), `government`, `faction`,
 *   `population`, `grades` (sept entiers de 0 à 4, ordre de `core::RegionAxis`), `places` (une
 *   table par lieu : `placeId`, `name`, `description`, `placed`, `x`, `y`, `hasCityMap`) et
 *   `labels` (`name`, `kind`, `x`, `y`).
 * - `city(placeId)` : le plan d'une ville — `cityId`, `name`, `regionId`, `image`, `points` (une
 *   table par quartier ou lieu numéroté : `pointId`, `number`, `name`, `description`, `x`, `y`) et
 *   `labels` — ou une table vide.
 */
class WorldMapModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    /// Nom de fichier de la carte du monde, sous `Elements/Assets/Maps/`.
    Q_PROPERTY(QString worldImage READ worldImage NOTIFY changed)
    Q_PROPERTY(QVariantList regions READ regions NOTIFY changed)

public:
    explicit WorldMapModel(QObject* parent = nullptr);

    /// Charge l'atlas et les cartes livrés à côté de l'exécutable.
    Q_INVOKABLE void load();

    /// Le plan de la ville `placeId`, ou une table vide si ce lieu n'a pas de plan.
    [[nodiscard]] Q_INVOKABLE QVariantMap city(const QString& placeId) const;

    /// L'indice, dans `regions`, de la région `regionId`, ou -1.
    [[nodiscard]] Q_INVOKABLE int regionIndex(const QString& regionId) const;

    [[nodiscard]] QString worldImage() const {
        return _worldImage;
    }
    [[nodiscard]] QVariantList regions() const {
        return _regions;
    }

signals:
    void changed();

private:
    QString _worldImage;
    QVariantList _regions;
    QVariantMap _cities;  ///< Identifiant de lieu → plan.
};

}  // namespace hmi
