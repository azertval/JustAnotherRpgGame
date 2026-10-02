// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/SheetRowModel.h"

#include <utility>

namespace hmi {

SheetRowModel::SheetRowModel(QObject* parent) : QAbstractListModel(parent) {}

int SheetRowModel::rowCount(const QModelIndex& parent) const {
    // Une liste plate : un parent valide n'a pas d'enfants. Sans cette garde, une vue arborescente
    // afficherait la liste entière sous chacune de ses lignes.
    return parent.isValid() ? 0 : static_cast<int>(_rows.size());
}

QVariant SheetRowModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= _rows.size()) {
        return {};
    }
    const SheetRow& row = _rows.at(index.row());
    switch (role) {
        case IdRole:
            return row.id;
        case LabelRole:
            return row.label;
        case ValueRole:
            return row.value;
        case MarkedRole:
            return row.marked;
        default:
            return {};
    }
}

QHash<int, QByteArray> SheetRowModel::roleNames() const {
    // `rowId` et non `id` : `id` est un mot réservé du QML, et un rôle ainsi nommé serait
    // inaccessible depuis un délégué — sans la moindre erreur, seulement une valeur vide.
    return {
        {IdRole, QByteArrayLiteral("rowId")},
        {LabelRole, QByteArrayLiteral("label")},
        {ValueRole, QByteArrayLiteral("value")},
        {MarkedRole, QByteArrayLiteral("marked")},
    };
}

void SheetRowModel::setRows(QVector<SheetRow> rows) {
    beginResetModel();
    _rows = std::move(rows);
    endResetModel();
}

}  // namespace hmi
