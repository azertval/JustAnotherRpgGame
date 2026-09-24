// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/EntityPanel.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <variant>

#include "Core/Levels/LevelDraft.h"
#include "Editor/Logic/EntityReferences.h"
#include "Editor/Logic/EntityShapes.h"

namespace hmi {

namespace {

// Bornes du champ entier : un rang d'arene ou un compteur, jamais une coordonnee.
constexpr int INTEGER_MINIMUM = -9999;
constexpr int INTEGER_MAXIMUM = 9999;

[[nodiscard]] bool sameEntity(const core::MapEntity& a, const core::MapEntity& b) {
    return a.type == b.type && a.position == b.position && a.properties == b.properties &&
           a.id == b.id && a.cells == b.cells;
}

// Colonnes de la liste : identifiant, famille, étiquette, case.
constexpr int ID_COLUMN = 0;
constexpr int TYPE_COLUMN = 1;
constexpr int LABEL_COLUMN = 2;
constexpr int CELL_COLUMN = 3;

[[nodiscard]] QString valueText(const core::PropertyValue& value) {
    if (const auto* const flag = std::get_if<bool>(&value)) {
        return *flag ? QStringLiteral("true") : QStringLiteral("false");
    }
    if (const auto* const text = std::get_if<std::string>(&value)) {
        return QString::fromStdString(*text);
    }
    if (const auto* const integer = std::get_if<std::int64_t>(&value)) {
        return QString::number(*integer);
    }
    return QString::number(std::get<double>(value));
}

[[nodiscard]] QString cellText(core::GridPosition position) {
    return QStringLiteral("(%1, %2)").arg(position.column).arg(position.row);
}

// Une famille et une propriété se nomment par leur identifiant du format : l'éditeur est un outil
// interne, sans traduction, et l'identifiant est ce que l'auteur lit dans le fichier.
[[nodiscard]] QString kindLabel(const std::string& type) {
    return QString::fromStdString(type);
}

[[nodiscard]] QString propertyLabel(const std::string& key) {
    return QString::fromStdString(key);
}

}  // namespace

/// Les widgets du panneau : la famille que l'outil Entité pose, la liste des entités de la carte,
/// les propriétés de l'entité sélectionnée (formulaire construit depuis `core::knownEntityKinds`)
/// et les avertissements.
struct EntityPanel::Widgets {
    QComboBox* kindCombo;
    QLineEdit* filterEdit;
    QTableWidget* entityTable;
    QLabel* selectionLabel;
    QWidget* propertiesForm;
    QLabel* verdictLabel;
    QPushButton* removeButton;
    QListWidget* warningList;

    explicit Widgets(QWidget* panel)
        : kindCombo(new QComboBox(panel)),
          filterEdit(new QLineEdit(panel)),
          entityTable(new QTableWidget(0, 4, panel)),
          selectionLabel(new QLabel(panel)),
          propertiesForm(new QWidget(panel)),
          verdictLabel(new QLabel(panel)),
          removeButton(new QPushButton(QStringLiteral("Remove"), panel)),
          warningList(new QListWidget(panel)) {
        kindCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        entityTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        filterEdit->setPlaceholderText(QStringLiteral("Filter: kind, id or value"));
        filterEdit->setClearButtonEnabled(true);
        entityTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
        entityTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        entityTable->setHorizontalHeaderLabels({QStringLiteral("Id"), QStringLiteral("Kind"),
                                                QStringLiteral("Label"), QStringLiteral("Cell")});
        entityTable->horizontalHeader()->setStretchLastSection(true);
        entityTable->verticalHeader()->setVisible(false);
        selectionLabel->setWordWrap(true);
        verdictLabel->setWordWrap(true);
        verdictLabel->setVisible(false);
        removeButton->setEnabled(false);
        warningList->setWordWrap(true);
        warningList->setSelectionMode(QAbstractItemView::SingleSelection);

        auto* const placeRow = new QHBoxLayout;
        placeRow->addWidget(new QLabel(QStringLiteral("Place"), panel));
        placeRow->addWidget(kindCombo, 1);
        auto* const propertiesBox = new QGroupBox(QStringLiteral("Properties"), panel);
        auto* const propertiesLayout = new QVBoxLayout(propertiesBox);
        propertiesLayout->addWidget(selectionLabel);
        propertiesLayout->addWidget(verdictLabel);
        propertiesLayout->addWidget(propertiesForm);
        propertiesLayout->addWidget(removeButton);
        auto* const warningsBox = new QGroupBox(QStringLiteral("Warnings"), panel);
        auto* const warningsLayout = new QVBoxLayout(warningsBox);
        warningsLayout->addWidget(warningList);
        auto* const layout = new QVBoxLayout(panel);
        layout->addLayout(placeRow);
        layout->addWidget(filterEdit);
        layout->addWidget(entityTable);
        layout->addWidget(propertiesBox);
        layout->addWidget(warningsBox);
    }
};

EntityPanel::EntityPanel(QWidget* parent)
    : QWidget(parent),
      _ui(std::make_unique<Widgets>(this)),
      _form(new QFormLayout(_ui->propertiesForm)) {
    _form->setContentsMargins(0, 0, 0, 0);

    connect(_ui->kindCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        if (!_rebuilding) {
            emit kindToPlaceChanged(QString::fromStdString(kindToPlace()));
        }
    });
    connect(
        _ui->entityTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] {
            if (_rebuilding) {
                return;
            }
            // Le rang d'entité est porté par la ligne : la liste filtrée n'a pas l'ordre de
            // la carte. La ligne courante est la principale, la dernière cliquée.
            std::vector<std::size_t> indices;
            for (const QModelIndex& row :
                 _ui->entityTable->selectionModel()->selectedRows(ID_COLUMN)) {
                indices.push_back(static_cast<std::size_t>(row.data(Qt::UserRole).toULongLong()));
            }
            std::optional<std::size_t> primary;
            if (const QTableWidgetItem* const current =
                    _ui->entityTable->item(_ui->entityTable->currentRow(), ID_COLUMN)) {
                primary = static_cast<std::size_t>(current->data(Qt::UserRole).toULongLong());
            }
            emit entitiesSelected(indices, primary);
        });
    connect(_ui->filterEdit, &QLineEdit::textChanged, this, [this] { rebuildTable(); });
    connect(_ui->removeButton, &QPushButton::clicked, this, [this] {
        if (!_selection.empty()) {
            emit removeRequested();
        }
    });
    connect(_ui->warningList, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        if (item != nullptr) {
            emit entitySelected(static_cast<std::size_t>(item->data(Qt::UserRole).toULongLong()));
        }
    });
    rebuildKinds();
    rebuildForm();
    rebuildWarnings();
}

EntityPanel::~EntityPanel() = default;

std::string EntityPanel::kindToPlace() const {
    return _ui->kindCombo->currentData().toString().toStdString();
}

void EntityPanel::refresh(const core::LevelDraft& draft, const std::vector<std::size_t>& selection,
                          std::optional<std::size_t> selected,
                          const core::EntityReferenceContext& context,
                          const std::vector<EditorDiagnostic>& diagnostics,
                          const std::string& verdict) {
    _entities = draft.entities();
    _selection = selection;
    std::erase_if(_selection, [this](std::size_t index) { return index >= _entities.size(); });
    _selected = selected && *selected < _entities.size() ? selected : std::nullopt;
    _verdict = verdict;
    _context = context;
    _diagnostics = diagnostics;
    rebuildTable();
    rebuildForm();
    rebuildWarnings();
}

void EntityPanel::rebuildKinds() {
    _rebuilding = true;
    const QString current = _ui->kindCombo->currentData().toString();
    _ui->kindCombo->clear();
    _ui->kindCombo->addItem(QStringLiteral("(select only)"), QString{});
    for (const core::EntityKind& kind : core::knownEntityKinds()) {
        const QString type =
            QString::fromUtf8(kind.type.data(), static_cast<qsizetype>(kind.type.size()));
        _ui->kindCombo->addItem(kindLabel(type.toStdString()), type);
    }
    const int index = _ui->kindCombo->findData(current);
    _ui->kindCombo->setCurrentIndex(index >= 0 ? index : 0);
    _rebuilding = false;
}

void EntityPanel::rebuildTable() {
    _rebuilding = true;
    const QSignalBlocker block(_ui->entityTable);
    const std::vector<std::size_t> shown =
        filterEntities(_entities, _ui->filterEdit->text().toStdString());
    _ui->entityTable->clearContents();
    _ui->entityTable->setRowCount(static_cast<int>(shown.size()));
    QItemSelection selected;
    for (std::size_t row = 0; row < shown.size(); ++row) {
        const std::size_t index = shown[row];
        const core::MapEntity& entity = _entities[index];
        const int line = static_cast<int>(row);
        auto* const id = new QTableWidgetItem(QString::fromStdString(entity.id));
        id->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(index));
        _ui->entityTable->setItem(line, ID_COLUMN, id);
        _ui->entityTable->setItem(line, TYPE_COLUMN, new QTableWidgetItem(kindLabel(entity.type)));
        _ui->entityTable->setItem(
            line, LABEL_COLUMN, new QTableWidgetItem(QString::fromStdString(entityLabel(entity))));
        _ui->entityTable->setItem(line, CELL_COLUMN,
                                  new QTableWidgetItem(cellText(entity.position)));
        if (std::ranges::find(_selection, index) != _selection.end()) {
            const QModelIndex left = _ui->entityTable->model()->index(line, 0);
            const QModelIndex right = _ui->entityTable->model()->index(line, CELL_COLUMN);
            selected.select(left, right);
        }
        if (_selected == index) {
            _ui->entityTable->setCurrentCell(line, ID_COLUMN, QItemSelectionModel::NoUpdate);
        }
    }
    _ui->entityTable->selectionModel()->select(selected, QItemSelectionModel::ClearAndSelect);
    _rebuilding = false;
}

void EntityPanel::rebuildForm() {
    const core::MapEntity* const entity = _selected ? &_entities[*_selected] : nullptr;
    const core::EntityKind* const kind =
        entity != nullptr ? core::findEntityKind(entity->type) : nullptr;

    // Les propriétés de la famille, puis celles que toute famille porte : la condition de présence
    // (LOT-126).
    const std::vector<const core::EntityPropertySpec*> specs =
        kind != nullptr ? core::inspectedProperties(*kind)
                        : std::vector<const core::EntityPropertySpec*>{};
    std::vector<std::vector<std::string>> choices;
    for (const core::EntityPropertySpec* const spec : specs) {
        choices.push_back(entityChoices(*spec, *entity, _context));
    }
    _ui->verdictLabel->setText(QString::fromStdString(_verdict));
    _ui->verdictLabel->setVisible(!_verdict.empty());
    _ui->removeButton->setEnabled(!_selection.empty());
    const bool unchanged =
        _formIndex == _selected && _formChoices == choices &&
        ((entity == nullptr && !_formEntity) ||
         (entity != nullptr && _formEntity && sameEntity(*entity, *_formEntity)));
    if (unchanged && _form->rowCount() > 0) {
        return;
    }
    _formIndex = _selected;
    _formEntity = entity != nullptr ? std::make_optional(*entity) : std::nullopt;
    _formChoices = choices;

    clearForm();
    if (entity == nullptr) {
        _ui->selectionLabel->setText(QStringLiteral("No entity selected."));
        return;
    }
    const std::size_t index = *_selected;
    QString heading = kind != nullptr
                          ? kindLabel(entity->type) + QStringLiteral(" ") +
                                QString::fromStdString(entity->id) + QStringLiteral(" ") +
                                cellText(entity->position)
                          : QStringLiteral(
                                "Kind \"%1\" is unknown to the editor: its properties are "
                                "carried over unchanged.")
                                .arg(QString::fromStdString(entity->type));
    if (_selection.size() > 1) {
        heading = QStringLiteral("%1 entities selected; showing ").arg(_selection.size()) + heading;
    }
    _ui->selectionLabel->setText(heading);

    for (std::size_t specIndex = 0; specIndex < specs.size(); ++specIndex) {
        const core::EntityPropertySpec& spec = *specs[specIndex];
        const auto found = entity->properties.find(std::string{spec.key});
        addPropertyRow(index, spec,
                       found != entity->properties.end() ? found->second : spec.defaultValue,
                       choices[specIndex]);
    }
    // Proprietes que la table ne declare pas : transportees, montrees, jamais editees ici.
    for (const auto& [key, value] : entity->properties) {
        if (kind != nullptr && core::findInspectedProperty(*kind, key) != nullptr) {
            continue;
        }
        auto* const shown = new QLabel(valueText(value), _ui->propertiesForm);
        shown->setTextInteractionFlags(Qt::TextSelectableByMouse);
        _form->addRow(QString::fromStdString(key), shown);
    }
}

void EntityPanel::clearForm() {
    // Les champs sont retires puis detruits APRES le retour a la boucle d'evenements : une
    // reconstruction declenchee par le `editingFinished` d'un champ du formulaire se deroule
    // pendant l'emission de son signal, et `removeRow` le detruirait sous ses pieds.
    while (_form->rowCount() > 0) {
        const QFormLayout::TakeRowResult taken = _form->takeRow(0);
        for (QLayoutItem* const item : {taken.labelItem, taken.fieldItem}) {
            if (item == nullptr) {
                continue;
            }
            if (QWidget* const widget = item->widget()) {
                widget->hide();
                widget->deleteLater();
            }
            delete item;
        }
    }
}

void EntityPanel::addPropertyRow(std::size_t index, const core::EntityPropertySpec& spec,
                                 const core::PropertyValue& value,
                                 const std::vector<std::string>& choices) {
    const QString key = QString::fromUtf8(spec.key.data(), static_cast<qsizetype>(spec.key.size()));
    const QString label = propertyLabel(key.toStdString());
    switch (spec.kind) {
        case core::EntityPropertyKind::Choice: {
            addChoiceRow(index, spec, value, choices);
            break;
        }
        case core::EntityPropertyKind::Text: {
            auto* const edit = new QLineEdit(valueText(value), _ui->propertiesForm);
            connect(edit, &QLineEdit::editingFinished, this, [this, index, key, edit] {
                emit propertyChanged(index, key,
                                     core::PropertyValue{edit->text().trimmed().toStdString()});
            });
            _form->addRow(label, edit);
            break;
        }
        case core::EntityPropertyKind::Integer: {
            auto* const spin = new QSpinBox(_ui->propertiesForm);
            // Les bornes de la déclaration, dans celles du champ.
            spin->setRange(static_cast<int>(std::clamp<std::int64_t>(spec.minimum, INTEGER_MINIMUM,
                                                                     INTEGER_MAXIMUM)),
                           static_cast<int>(std::clamp<std::int64_t>(spec.maximum, INTEGER_MINIMUM,
                                                                     INTEGER_MAXIMUM)));
            const auto* const held = std::get_if<std::int64_t>(&value);
            spin->setValue(held != nullptr ? static_cast<int>(*held) : 0);
            connect(spin, &QSpinBox::editingFinished, this, [this, index, key, spin] {
                emit propertyChanged(index, key, core::PropertyValue{std::int64_t{spin->value()}});
            });
            _form->addRow(label, spin);
            break;
        }
        case core::EntityPropertyKind::Boolean: {
            auto* const check = new QCheckBox(_ui->propertiesForm);
            const auto* const held = std::get_if<bool>(&value);
            check->setChecked(held != nullptr && *held);
            connect(check, &QCheckBox::toggled, this, [this, index, key](bool checked) {
                emit propertyChanged(index, key, core::PropertyValue{checked});
            });
            _form->addRow(label, check);
            break;
        }
    }
}

void EntityPanel::addChoiceRow(std::size_t index, const core::EntityPropertySpec& spec,
                               const core::PropertyValue& value,
                               const std::vector<std::string>& choices) {
    const QString key = QString::fromUtf8(spec.key.data(), static_cast<qsizetype>(spec.key.size()));
    const QString label = propertyLabel(key.toStdString());
    auto* const combo = new QComboBox(_ui->propertiesForm);
    // Editable : l'auteur peut nommer une cible qui n'existe pas encore -- la carte
    // qu'il ecrira ensuite. L'avertissement le lui rappellera.
    combo->setEditable(spec.source != core::EntityChoiceSource::Fixed);
    if (!spec.required || spec.source != core::EntityChoiceSource::Fixed) {
        combo->addItem(QStringLiteral("(none)"), QString{});
    }
    for (const std::string& choice : choices) {
        combo->addItem(QString::fromStdString(choice), QString::fromStdString(choice));
    }
    const QString current = valueText(value);
    int currentIndex = combo->findData(current);
    if (currentIndex < 0 && !current.isEmpty()) {
        combo->addItem(current, current);
        currentIndex = combo->count() - 1;
    }
    combo->setCurrentIndex((std::max)(currentIndex, 0));
    const auto commit = [this, index, key, combo] {
        const QString chosen =
            combo->currentIndex() >= 0 &&
                    combo->currentText() == combo->itemText(combo->currentIndex())
                ? combo->currentData().toString()
                : combo->currentText().trimmed();
        emit propertyChanged(index, key, core::PropertyValue{chosen.toStdString()});
    };
    connect(combo, &QComboBox::activated, this, [commit](int) { commit(); });
    if (combo->isEditable()) {
        connect(combo->lineEdit(), &QLineEdit::editingFinished, this, commit);
    }
    _form->addRow(label, combo);
}

void EntityPanel::rebuildWarnings() {
    _ui->warningList->clear();
    if (_diagnostics.empty()) {
        auto* const none = new QListWidgetItem(QStringLiteral("No warnings."), _ui->warningList);
        none->setFlags(Qt::ItemIsEnabled);
        return;
    }
    for (const EditorDiagnostic& diagnostic : _diagnostics) {
        auto* const item = new QListWidgetItem(cellText(diagnostic.cell) + QStringLiteral(" ") +
                                                   QString::fromStdString(diagnostic.message),
                                               _ui->warningList);
        item->setData(Qt::UserRole, QVariant::fromValue<qulonglong>(diagnostic.entityIndex));
    }
}

}  // namespace hmi
