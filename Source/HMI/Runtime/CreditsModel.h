// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQmlIntegration>

/**
 * @file HMI/Runtime/CreditsModel.h
 * @brief Les crédits d'une colonne de l'écran, exposés au QML (`LOT-87`, T3.3).
 */

namespace hmi {

/**
 * @brief Une colonne de l'écran des crédits : ses sections, dans la langue courante.
 *
 * Le jumeau en pose deux (`column: 0` et `column: 1`) et lie `language` à `OptionsModel` : un
 * changement de langue relit les titres et les rôles. Le fichier est embarqué dans la ressource
 * (`:/jadg/credits/credits.json`) : des attributions obligatoires ne dépendent pas d'un fichier
 * posé à côté de l'exécutable, qu'un déploiement pourrait oublier.
 *
 * `sections` est une **liste de tables**, et non un `QAbstractListModel` : un `Repeater` la lit
 * par `modelData`, et un modèle de liste déclaré type QML obligerait le registrar à enregistrer sa
 * classe de base (voir `SheetRowModel`). Chaque section porte `sectionId`, `title`, `iconKey`
 * (`ui/icon/credits-section/<id>`) et `lines`, chaque ligne `role` et `names` (un nom par ligne).
 */
class CreditsModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    /// La colonne de l'écran que cette instance sert (`0` ou `1`) : `sections` se relit quand elle
    /// change.
    Q_PROPERTY(int column READ column WRITE setColumn NOTIFY sectionsChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY sectionsChanged)
    Q_PROPERTY(QVariantList sections READ sections NOTIFY sectionsChanged)

public:
    explicit CreditsModel(QObject* parent = nullptr);

    [[nodiscard]] int column() const noexcept {
        return _column;
    }
    [[nodiscard]] QString language() const {
        return _language;
    }
    [[nodiscard]] QVariantList sections() const {
        return _sections;
    }

    void setColumn(int column);
    void setLanguage(const QString& language);

signals:
    void sectionsChanged();

private:
    void reload();

    int _column = 0;
    QString _language = QStringLiteral("fr");
    QVariantList _sections;
};

}  // namespace hmi
