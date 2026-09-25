// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QString>
#include <QVector>

/**
 * @file HMI/Runtime/SheetRowModel.h
 * @brief Les parties **répétitives** d'un écran, exposées au QML comme un modèle (`LOT-86`).
 */

namespace hmi {

/// Une ligne : ce qu'elle désigne, comment on l'appelle, ce qu'elle vaut.
struct SheetRow {
    QString id;     ///< Identifiant stable (`strength`, `athletics`) — clé de traduction.
    QString label;  ///< Libellé lisible, tel que le catalogue de règles le donne.
    QString value;  ///< Valeur **déjà formatée** par la couche pure (« 16 (+3) », « +5 • »).
};

// Volontairement PAS déclaré comme type QML. Un modèle ne s'instancie jamais depuis le QML : il
// est fourni par la vue-modèle, et le QML le consomme par son type de base `QAbstractItemModel*`,
// comme tout modèle Qt. L'exposer aurait obligé le registrar à enregistrer aussi sa classe de
// base, qui appartient à un autre module — et la compilation du fichier engendré échouait.
/**
 * @brief Liste de lignes libellé/valeur, pour un `Repeater` ou une `ListView`.
 *
 * **Pourquoi un modèle et non une table de chaînes.** Les six caractéristiques, les six jets de
 * sauvegarde et les dix-huit compétences sont des répétitions : la conception veut en décrire
 * **une** et laisser le nombre à la donnée. Avec une table indexée par clé, chaque ligne aurait dû
 * être posée à la main dans le fichier d'interface — ce qui remettait la structure de la fiche
 * dans le fichier, c'est-à-dire exactement ce dont le `LOT-85` s'était plaint.
 *
 * **Ce que ce modèle ne fait pas** : il ne formate rien. La valeur arrive déjà mise en forme par
 * `hmi::characterSheetValues`, logique **pure** et testée. Reformater ici aurait produit deux
 * vérités sur la façon d'écrire un modificateur.
 */
class SheetRowModel : public QAbstractListModel {
    Q_OBJECT

public:
    /// Rôles lisibles depuis QML : `model.rowId`, `model.label`, `model.value`.
    enum Role {
        IdRole = Qt::UserRole + 1,
        LabelRole,
        ValueRole,
    };

    explicit SheetRowModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    /// Remplace toutes les lignes. Réinitialisation complète et non des insertions une à une : la
    /// liste est recalculée d'un bloc à chaque changement de fiche, jamais modifiée par morceaux.
    void setRows(QVector<SheetRow> rows);

private:
    QVector<SheetRow> _rows;
};

}  // namespace hmi
