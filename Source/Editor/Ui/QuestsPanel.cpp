// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/QuestsPanel.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <array>
#include <set>
#include <utility>

#include "Editor/Logic/EntityReferences.h"
#include "Editor/Logic/MapTexts.h"

namespace hmi {

namespace {

[[nodiscard]] QString qs(const std::string& text) {
    return QString::fromStdString(text);
}

/// Les tests d'une condition, dans l'ordre de la liste.
constexpr std::array<std::pair<const char*, core::FlagTest>, 4> TESTS{{
    {"is set", core::FlagTest::IsSet},
    {"is not set", core::FlagTest::IsUnset},
    {"equals", core::FlagTest::Equals},
    {"not equals", core::FlagTest::NotEquals},
}};

/// Hauteur de la liste des étapes : cinq ou six lignes, le reste pour l'étape choisie.
constexpr int STEP_LIST_HEIGHT = 120;
/// Hauteur d'une table de conditions ou d'effets.
constexpr int RULE_TABLE_HEIGHT = 110;

[[nodiscard]] int testIndex(core::FlagTest test) {
    for (std::size_t i = 0; i < TESTS.size(); ++i) {
        if (TESTS[i].second == test) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

// `a|b|c`, sans espaces ni vides : la forme d'une liste de valeurs dans une case.
[[nodiscard]] std::vector<std::string> valuesOf(const QString& text) {
    std::vector<std::string> values;
    for (const QString& part : text.split(QLatin1Char('|'))) {
        const QString value = part.trimmed();
        if (!value.isEmpty()) {
            values.push_back(value.toStdString());
        }
    }
    return values;
}

[[nodiscard]] QString joinedValues(const std::vector<std::string>& values) {
    QStringList parts;
    for (const std::string& value : values) {
        parts << qs(value);
    }
    return parts.join(QLatin1Char('|'));
}

[[nodiscard]] QComboBox* editableCombo(const QStringList& items, const QString& text) {
    auto* const combo = new QComboBox;
    combo->setEditable(true);
    combo->addItems(items);
    combo->setCurrentText(text);
    return combo;
}

[[nodiscard]] QTableWidget* ruleTable(const QStringList& headers) {
    auto* const table = new QTableWidget(0, static_cast<int>(headers.size()));
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setMinimumHeight(RULE_TABLE_HEIGHT);
    return table;
}

[[nodiscard]] QString roleText(FlagUseRole role) {
    switch (role) {
        case FlagUseRole::Declares:
            return QStringLiteral("declares");
        case FlagUseRole::Reads:
            return QStringLiteral("reads");
        case FlagUseRole::Writes:
            return QStringLiteral("writes");
    }
    return {};
}

}  // namespace

QuestsPanel::QuestsPanel(std::filesystem::path dataRoot, QWidget* parent)
    : QWidget(parent), _root(std::move(dataRoot)) {
    for (const std::filesystem::path& catalog : catalogFilesIn(localizationDirectory(_root))) {
        _languages.push_back(catalog.stem().string());
    }
    auto* const layout = new QVBoxLayout(this);

    auto* const choice = new QHBoxLayout;
    _questList = new QComboBox;
    choice->addWidget(new QLabel(QStringLiteral("Quest:")));
    choice->addWidget(_questList, 1);
    layout->addLayout(choice);

    auto* const commands = new QHBoxLayout;
    const auto button = [commands, this](const QString& text, void (QuestsPanel::*slot)()) {
        auto* const pushButton = new QPushButton(text);
        commands->addWidget(pushButton);
        connect(pushButton, &QPushButton::clicked, this, slot);
        return pushButton;
    };
    button(QStringLiteral("New…"), &QuestsPanel::newQuest);
    _editors.push_back(button(QStringLiteral("Save"), &QuestsPanel::saveQuest));
    _editors.push_back(button(QStringLiteral("Rename…"), &QuestsPanel::renameQuest));
    _editors.push_back(button(QStringLiteral("Delete…"), &QuestsPanel::deleteQuest));
    auto* const revert = new QPushButton(QStringLiteral("Revert"));
    commands->addWidget(revert);
    _editors.push_back(revert);
    connect(revert, &QPushButton::clicked, this, [this] {
        _selectAfterReload = _draft.quest.id;
        reload();
    });
    layout->addLayout(commands);

    _status = new QLabel;
    _status->setWordWrap(true);
    _status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(_status);

    _pages = new QTabWidget;
    _pages->addTab(buildQuestTab(), QStringLiteral("Quest"));
    _pages->addTab(buildStepsTab(), QStringLiteral("Steps"));
    _pages->addTab(buildUsesTab(), QStringLiteral("Uses"));
    layout->addWidget(_pages, 1);
    _editors.push_back(_pages);

    connect(_questList, &QComboBox::activated, this, [this](int index) {
        const std::string wanted = _questList->itemData(index).toString().toStdString();
        if (wanted == _draft.quest.id && _hasQuest) {
            return;
        }
        if (!askAboutChanges()) {
            const QSignalBlocker blocker(_questList);
            _questList->setCurrentIndex(_questList->findData(qs(_draft.quest.id)));
            return;
        }
        openQuest(wanted);
    });
    reload();
}

QWidget* QuestsPanel::buildQuestTab() {
    auto* const page = new QWidget;
    auto* const layout = new QVBoxLayout(page);
    auto* const form = new QFormLayout;
    _name = new QLineEdit;
    _name->setToolTip(QStringLiteral("The author's working name; players read the journal title."));
    _source = new QLineEdit;
    form->addRow(QStringLiteral("Name:"), _name);
    form->addRow(QStringLiteral("Source:"), _source);
    connect(_name, &QLineEdit::textEdited, this, [this](const QString& text) {
        _draft.quest.name = text.toStdString();
        markDirty();
    });
    connect(_source, &QLineEdit::textEdited, this, [this](const QString& text) {
        _draft.quest.source = text.toStdString();
        markDirty();
    });
    for (const std::string& language : _languages) {
        auto* const title = new QLineEdit;
        _titles[language] = title;
        form->addRow(QStringLiteral("Journal title (%1):").arg(qs(language)), title);
        connect(title, &QLineEdit::textEdited, this, [this, language](const QString& text) {
            _draft.texts[language][core::questTitleKey(_draft.quest.id)] = text.toStdString();
            markDirty();
        });
    }
    layout->addLayout(form);

    auto* const flagsBox = new QGroupBox(QStringLiteral("Declared flags"));
    auto* const flagsLayout = new QVBoxLayout(flagsBox);
    _flags = ruleTable(
        {QStringLiteral("Flag"), QStringLiteral("Values (a|b|c)"), QStringLiteral("Initial")});
    // Les valeurs sont la colonne longue : elle prend la place.
    _flags->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    _flags->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    flagsLayout->addWidget(_flags);
    auto* const flagButtons = new QHBoxLayout;
    auto* const add = new QPushButton(QStringLiteral("Add"));
    auto* const remove = new QPushButton(QStringLiteral("Remove"));
    auto* const rename = new QPushButton(QStringLiteral("Rename flag…"));
    auto* const renameValue = new QPushButton(QStringLiteral("Rename value…"));
    auto* const uses = new QPushButton(QStringLiteral("Uses"));
    for (QPushButton* const each : {add, remove, rename, renameValue, uses}) {
        flagButtons->addWidget(each);
    }
    flagsLayout->addLayout(flagButtons);
    layout->addWidget(flagsBox, 1);

    connect(add, &QPushButton::clicked, this, [this] {
        _draft.quest.flags.push_back(core::QuestFlag{
            .id = "quete." + _draft.quest.id, .values = {"inconnue"}, .initial = "inconnue"});
        fillFlags();
        markDirty();
    });
    connect(remove, &QPushButton::clicked, this, [this] {
        const int row = _flags->currentRow();
        if (row >= 0 && row < static_cast<int>(_draft.quest.flags.size())) {
            _draft.quest.flags.erase(_draft.quest.flags.begin() + row);
            fillFlags();
            markDirty();
        }
    });
    connect(rename, &QPushButton::clicked, this, &QuestsPanel::renameFlag);
    connect(renameValue, &QPushButton::clicked, this, &QuestsPanel::renameFlagValue);
    connect(uses, &QPushButton::clicked, this, [this] {
        const int row = _flags->currentRow();
        if (row >= 0 && row < static_cast<int>(_draft.quest.flags.size())) {
            _useFlag->setCurrentText(qs(_draft.quest.flags[static_cast<std::size_t>(row)].id));
        }
        _pages->setCurrentIndex(2);
        showUses();
    });
    return page;
}

QWidget* QuestsPanel::buildStepsTab() {
    auto* const page = new QWidget;
    auto* const layout = new QVBoxLayout(page);
    _steps = new QListWidget;
    _steps->setMaximumHeight(STEP_LIST_HEIGHT);
    layout->addWidget(_steps);
    auto* const stepButtons = new QHBoxLayout;
    auto* const add = new QPushButton(QStringLiteral("Add"));
    auto* const remove = new QPushButton(QStringLiteral("Remove"));
    auto* const up = new QPushButton(QStringLiteral("Up"));
    auto* const down = new QPushButton(QStringLiteral("Down"));
    for (QPushButton* const each : {add, remove, up, down}) {
        stepButtons->addWidget(each);
    }
    layout->addLayout(stepButtons);

    connect(_steps, &QListWidget::currentRowChanged, this, [this](int row) {
        if (!_filling) {
            _step = row;
            fillStep();
        }
    });
    connect(add, &QPushButton::clicked, this, [this] {
        std::string id = "step";
        for (int n = 1; _draft.quest.find(id) != nullptr; ++n) {
            id = "step-" + std::to_string(n);
        }
        core::QuestStep step;
        step.id = id;
        _draft.quest.steps.push_back(std::move(step));
        _step = static_cast<int>(_draft.quest.steps.size()) - 1;
        fillSteps();
        markDirty();
    });
    connect(remove, &QPushButton::clicked, this, [this] {
        if (currentStep() == nullptr) {
            return;
        }
        _draft.quest.steps.erase(_draft.quest.steps.begin() + _step);
        _step = std::min(_step, static_cast<int>(_draft.quest.steps.size()) - 1);
        fillSteps();
        markDirty();
    });
    connect(up, &QPushButton::clicked, this, [this] { moveStep(-1); });
    connect(down, &QPushButton::clicked, this, [this] { moveStep(1); });

    _stepEditor = new QWidget;
    auto* const editor = new QVBoxLayout(_stepEditor);
    editor->setContentsMargins(0, 0, 0, 0);
    auto* const form = new QFormLayout;
    _stepId = new QLineEdit;
    connect(_stepId, &QLineEdit::editingFinished, this,
            [this] { renameStep(_stepId->text().trimmed().toStdString()); });
    form->addRow(QStringLiteral("Step id:"), _stepId);
    auto* const place = new QHBoxLayout;
    _stepAt = editableCombo({}, {});
    _stepAt->setToolTip(QStringLiteral("Where the step plays: an entity, map#id. Optional."));
    auto* const go = new QPushButton(QStringLiteral("Go"));
    place->addWidget(_stepAt, 1);
    place->addWidget(go);
    form->addRow(QStringLiteral("Where (map#id):"), place);
    connect(_stepAt, &QComboBox::currentTextChanged, this, [this](const QString& text) {
        if (core::QuestStep* step = currentStep(); step != nullptr && !_filling) {
            step->at = text.trimmed().toStdString();
            markDirty();
        }
    });
    connect(go, &QPushButton::clicked, this, &QuestsPanel::goToStepPlace);
    editor->addLayout(form);

    editor->addWidget(new QLabel(QStringLiteral("When (all must hold):")));
    _conditions =
        ruleTable({QStringLiteral("Flag"), QStringLiteral("Test"), QStringLiteral("Values (a|b)")});
    editor->addWidget(_conditions);
    auto* const conditionButtons = new QHBoxLayout;
    auto* const addCondition = new QPushButton(QStringLiteral("Add condition"));
    auto* const removeCondition = new QPushButton(QStringLiteral("Remove condition"));
    conditionButtons->addWidget(addCondition);
    conditionButtons->addWidget(removeCondition);
    editor->addLayout(conditionButtons);
    connect(addCondition, &QPushButton::clicked, this, [this] {
        if (core::QuestStep* step = currentStep()) {
            const std::string flag =
                _draft.quest.flags.empty() ? std::string{} : _draft.quest.flags.front().id;
            step->when.push_back(core::FlagCondition{
                .flag = flag,
                .test = flag.empty() ? core::FlagTest::IsSet : core::FlagTest::Equals,
                .values = {}});
            fillStep();
            markDirty();
        }
    });
    connect(removeCondition, &QPushButton::clicked, this, [this] {
        core::QuestStep* step = currentStep();
        const int row = _conditions->currentRow();
        if (step != nullptr && row >= 0 && row < static_cast<int>(step->when.size())) {
            step->when.erase(step->when.begin() + row);
            fillStep();
            markDirty();
        }
    });

    editor->addWidget(new QLabel(QStringLiteral("Effects (when reached):")));
    _effects =
        ruleTable({QStringLiteral("Effect"), QStringLiteral("Flag"), QStringLiteral("Value")});
    editor->addWidget(_effects);
    auto* const effectButtons = new QHBoxLayout;
    auto* const addEffect = new QPushButton(QStringLiteral("Add effect"));
    auto* const removeEffect = new QPushButton(QStringLiteral("Remove effect"));
    effectButtons->addWidget(addEffect);
    effectButtons->addWidget(removeEffect);
    editor->addLayout(effectButtons);
    connect(addEffect, &QPushButton::clicked, this, [this] {
        if (core::QuestStep* step = currentStep()) {
            step->effects.push_back(core::QuestEffect{});
            fillStep();
            markDirty();
        }
    });
    connect(removeEffect, &QPushButton::clicked, this, [this] {
        core::QuestStep* step = currentStep();
        const int row = _effects->currentRow();
        if (step != nullptr && row >= 0 && row < static_cast<int>(step->effects.size())) {
            step->effects.erase(step->effects.begin() + row);
            fillStep();
            markDirty();
        }
    });

    auto* const end = new QFormLayout;
    _outcome = new QComboBox;
    _outcome->addItems(
        {QStringLiteral("(none)"), QStringLiteral("success"), QStringLiteral("failure")});
    end->addRow(QStringLiteral("Closes the quest:"), _outcome);
    connect(_outcome, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (core::QuestStep* step = currentStep(); step != nullptr && !_filling) {
            step->outcome = index == 1   ? core::QuestOutcome::Success
                            : index == 2 ? core::QuestOutcome::Failure
                                         : core::QuestOutcome::None;
            markDirty();
        }
    });
    for (const std::string& language : _languages) {
        auto* const text = new QLineEdit;
        _stepTexts[language] = text;
        end->addRow(QStringLiteral("Journal (%1):").arg(qs(language)), text);
        connect(text, &QLineEdit::textEdited, this, [this, language](const QString& value) {
            if (const core::QuestStep* step = currentStep()) {
                _draft.texts[language][core::questStepKey(_draft.quest.id, step->id)] =
                    value.toStdString();
                markDirty();
            }
        });
    }
    editor->addLayout(end);
    auto* const play = new QPushButton(QStringLiteral("Play this step"));
    play->setToolTip(QStringLiteral(
        "Set the world state of the canvas to values that reach this step; P and F5 start "
        "from it."));
    connect(play, &QPushButton::clicked, this, &QuestsPanel::playStep);
    editor->addWidget(play);

    auto* const scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(_stepEditor);
    layout->addWidget(scroll, 1);
    return page;
}

QWidget* QuestsPanel::buildUsesTab() {
    auto* const page = new QWidget;
    auto* const layout = new QVBoxLayout(page);
    auto* const form = new QFormLayout;
    _useFlag = editableCombo({}, {});
    _useValue = new QComboBox;
    form->addRow(QStringLiteral("Flag:"), _useFlag);
    form->addRow(QStringLiteral("Value:"), _useValue);
    layout->addLayout(form);
    auto* const find = new QPushButton(QStringLiteral("Who uses it?"));
    layout->addWidget(find);
    _useTree = new QTreeWidget;
    _useTree->setHeaderLabels({QStringLiteral("Use"), QStringLiteral("Where")});
    _useTree->setRootIsDecorated(false);
    layout->addWidget(_useTree, 1);
    layout->addWidget(new QLabel(QStringLiteral("Double-click an entity to open its map.")));

    connect(_useFlag, &QComboBox::currentTextChanged, this, [this](const QString& flag) {
        const QSignalBlocker blocker(_useValue);
        _useValue->clear();
        _useValue->addItem(QStringLiteral("(any)"));
        _useValue->addItems(declaredValues(flag.toStdString()));
    });
    connect(find, &QPushButton::clicked, this, &QuestsPanel::showUses);
    connect(_useValue, &QComboBox::activated, this, [this](int) { showUses(); });
    connect(_useTree, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem* item, int) {
        const int index = item->data(0, Qt::UserRole).toInt();
        if (_uses && index >= 0 && index < static_cast<int>(_uses->size())) {
            emit citationActivated((*_uses)[static_cast<std::size_t>(index)].where);
        }
    });
    return page;
}

void QuestsPanel::setReferences(const EditorReferences* references) {
    _references = references;
    _uses.reset();
    fillUseChoices();
    if (_hasQuest) {
        fillStep();
    }
}

void QuestsPanel::reload() {
    const std::string wanted = !_selectAfterReload.empty() ? _selectAfterReload : _draft.quest.id;
    _selectAfterReload.clear();
    _uses.reset();
    const std::vector<std::string> ids = questIds(_root);
    {
        const QSignalBlocker blocker(_questList);
        _questList->clear();
        for (const std::string& id : ids) {
            _questList->addItem(qs(id), qs(id));
        }
    }
    _hasQuest = false;
    _dirty = false;
    if (ids.empty()) {
        _draft = {};
        setEditable(false);
        showStatus(QStringLiteral("No quest yet: New… writes World/quests/<id>.json."));
        return;
    }
    openQuest(std::ranges::find(ids, wanted) != ids.end() ? wanted : ids.front());
}

void QuestsPanel::openQuest(const std::string& questId) {
    {
        const QSignalBlocker blocker(_questList);
        _questList->setCurrentIndex(_questList->findData(qs(questId)));
    }
    QuestDraftLoad loaded = loadQuestDraft(_root, questId);
    _dirty = false;
    _isNew = false;
    if (!loaded.draft) {
        _hasQuest = false;
        _draft = {};
        _draft.quest.id = questId;
        setEditable(false);
        QStringList errors;
        for (const std::string& error : loaded.errors) {
            errors << qs(error);
        }
        showStatus(
            QStringLiteral("The game refuses this quest:\n") + errors.join(QLatin1Char('\n')),
            true);
        return;
    }
    _draft = std::move(*loaded.draft);
    _savedFlags.clear();
    for (const core::QuestFlag& flag : _draft.quest.flags) {
        _savedFlags.push_back(flag.id);
    }
    _hasQuest = true;
    _step = _draft.quest.steps.empty() ? -1 : 0;
    setEditable(true);
    showStatus({});
    fillAll();
}

void QuestsPanel::fillAll() {
    _filling = true;
    _name->setText(qs(_draft.quest.name));
    _source->setText(qs(_draft.quest.source));
    for (const auto& [language, title] : _titles) {
        const auto texts = _draft.texts.find(language);
        const std::string key = core::questTitleKey(_draft.quest.id);
        title->setText(texts != _draft.texts.end() && texts->second.contains(key)
                           ? qs(texts->second.find(key)->second)
                           : QString{});
    }
    _filling = false;
    fillFlags();
    fillSteps();
    fillUseChoices();
}

void QuestsPanel::fillFlags() {
    _filling = true;
    _flags->setRowCount(static_cast<int>(_draft.quest.flags.size()));
    for (int row = 0; row < _flags->rowCount(); ++row) {
        const core::QuestFlag& flag = _draft.quest.flags[static_cast<std::size_t>(row)];
        auto* const id = new QLineEdit(qs(flag.id));
        // Un drapeau enregistré se renomme par « Rename flag… », avec ce qui le cite.
        const bool saved = std::ranges::find(_savedFlags, flag.id) != _savedFlags.end();
        id->setReadOnly(saved);
        id->setToolTip(saved
                           ? QStringLiteral("Saved flag: use Rename flag… to rename it everywhere.")
                           : QString{});
        auto* const values = new QLineEdit(joinedValues(flag.values));
        auto* const initial = new QComboBox;
        for (const std::string& value : flag.values) {
            initial->addItem(qs(value));
        }
        initial->setCurrentText(qs(flag.initial));
        _flags->setCellWidget(row, 0, id);
        _flags->setCellWidget(row, 1, values);
        _flags->setCellWidget(row, 2, initial);
        connect(id, &QLineEdit::textEdited, this, [this, row] { readFlagRow(row); });
        connect(values, &QLineEdit::textEdited, this, [this, row] { readFlagRow(row); });
        connect(initial, &QComboBox::currentTextChanged, this, [this, row] { readFlagRow(row); });
    }
    _filling = false;
}

void QuestsPanel::readFlagRow(int row) {
    if (_filling || row < 0 || row >= static_cast<int>(_draft.quest.flags.size())) {
        return;
    }
    core::QuestFlag& flag = _draft.quest.flags[static_cast<std::size_t>(row)];
    const auto* const id = qobject_cast<QLineEdit*>(_flags->cellWidget(row, 0));
    const auto* const values = qobject_cast<QLineEdit*>(_flags->cellWidget(row, 1));
    auto* const initial = qobject_cast<QComboBox*>(_flags->cellWidget(row, 2));
    flag.id = id->text().trimmed().toStdString();
    flag.values = valuesOf(values->text());
    flag.initial = initial->currentText().toStdString();
    // L'initiale se choisit parmi les valeurs : la liste suit ce qu'on tape.
    _filling = true;
    initial->clear();
    for (const std::string& value : flag.values) {
        initial->addItem(qs(value));
    }
    if (std::ranges::find(flag.values, flag.initial) == flag.values.end()) {
        flag.initial = flag.values.empty() ? std::string{} : flag.values.front();
    }
    initial->setCurrentText(qs(flag.initial));
    _filling = false;
    markDirty();
}

void QuestsPanel::fillSteps() {
    _filling = true;
    _steps->clear();
    for (const core::QuestStep& step : _draft.quest.steps) {
        QString label = qs(step.id);
        if (step.outcome != core::QuestOutcome::None) {
            label += step.outcome == core::QuestOutcome::Success ? QStringLiteral("  (success)")
                                                                 : QStringLiteral("  (failure)");
        }
        _steps->addItem(label);
    }
    _steps->setCurrentRow(_step);
    _filling = false;
    fillStep();
}

void QuestsPanel::fillStep() {
    const core::QuestStep* step = currentStep();
    _stepEditor->setEnabled(step != nullptr);
    _filling = true;
    _stepId->setText(step != nullptr ? qs(step->id) : QString{});
    _stepAt->clear();
    if (_references != nullptr) {
        QStringList places;
        for (const core::WorldMapNode& map : _references->world.maps) {
            for (const std::string& id : map.entityIds) {
                places << qs(entityRef(map.mapId, id));
            }
        }
        places.sort();
        _stepAt->addItem(QString{});
        _stepAt->addItems(places);
    }
    _stepAt->setCurrentText(step != nullptr ? qs(step->at) : QString{});
    _outcome->setCurrentIndex(step == nullptr                                ? 0
                              : step->outcome == core::QuestOutcome::Success ? 1
                              : step->outcome == core::QuestOutcome::Failure ? 2
                                                                             : 0);
    for (const auto& [language, text] : _stepTexts) {
        QString value;
        if (step != nullptr) {
            const auto texts = _draft.texts.find(language);
            const std::string key = core::questStepKey(_draft.quest.id, step->id);
            if (texts != _draft.texts.end() && texts->second.contains(key)) {
                value = qs(texts->second.find(key)->second);
            }
        }
        text->setText(value);
    }

    const QStringList flags = knownFlags();
    _conditions->setRowCount(step != nullptr ? static_cast<int>(step->when.size()) : 0);
    for (int row = 0; row < _conditions->rowCount(); ++row) {
        const core::FlagCondition& condition = step->when[static_cast<std::size_t>(row)];
        auto* const flag = editableCombo(flags, qs(condition.flag));
        auto* const test = new QComboBox;
        for (const auto& [label, value] : TESTS) {
            test->addItem(QString::fromLatin1(label));
        }
        test->setCurrentIndex(testIndex(condition.test));
        auto* const values =
            editableCombo(declaredValues(condition.flag), joinedValues(condition.values));
        _conditions->setCellWidget(row, 0, flag);
        _conditions->setCellWidget(row, 1, test);
        _conditions->setCellWidget(row, 2, values);
        connect(flag, &QComboBox::currentTextChanged, this, [this, row] { readConditionRow(row); });
        connect(test, &QComboBox::currentIndexChanged, this,
                [this, row] { readConditionRow(row); });
        connect(values, &QComboBox::currentTextChanged, this,
                [this, row] { readConditionRow(row); });
    }

    _effects->setRowCount(step != nullptr ? static_cast<int>(step->effects.size()) : 0);
    for (int row = 0; row < _effects->rowCount(); ++row) {
        const core::QuestEffect& effect = step->effects[static_cast<std::size_t>(row)];
        auto* const kind = new QComboBox;
        kind->addItems({QStringLiteral("setFlag"), QStringLiteral("clearFlag")});
        kind->setCurrentIndex(effect.kind == core::QuestEffect::Kind::SetFlag ? 0 : 1);
        auto* const flag = editableCombo(flags, qs(effect.flag));
        auto* const value = editableCombo(declaredValues(effect.flag), qs(effect.value));
        _effects->setCellWidget(row, 0, kind);
        _effects->setCellWidget(row, 1, flag);
        _effects->setCellWidget(row, 2, value);
        connect(kind, &QComboBox::currentIndexChanged, this, [this, row] { readEffectRow(row); });
        connect(flag, &QComboBox::currentTextChanged, this, [this, row] { readEffectRow(row); });
        connect(value, &QComboBox::currentTextChanged, this, [this, row] { readEffectRow(row); });
    }
    _filling = false;
}

void QuestsPanel::readConditionRow(int row) {
    core::QuestStep* step = currentStep();
    if (_filling || step == nullptr || row < 0 || row >= static_cast<int>(step->when.size())) {
        return;
    }
    const auto* const flag = qobject_cast<QComboBox*>(_conditions->cellWidget(row, 0));
    const auto* const test = qobject_cast<QComboBox*>(_conditions->cellWidget(row, 1));
    auto* const values = qobject_cast<QComboBox*>(_conditions->cellWidget(row, 2));
    core::FlagCondition& condition = step->when[static_cast<std::size_t>(row)];
    const std::string previousFlag = condition.flag;
    condition.flag = flag->currentText().trimmed().toStdString();
    condition.test = TESTS[static_cast<std::size_t>(std::max(0, test->currentIndex()))].second;
    const bool compares =
        condition.test == core::FlagTest::Equals || condition.test == core::FlagTest::NotEquals;
    condition.values = compares ? valuesOf(values->currentText()) : std::vector<std::string>{};
    values->setEnabled(compares);
    if (condition.flag != previousFlag) {
        // Les valeurs proposées sont celles du drapeau choisi.
        _filling = true;
        const QString typed = values->currentText();
        values->clear();
        values->addItems(declaredValues(condition.flag));
        values->setCurrentText(typed);
        _filling = false;
    }
    markDirty();
}

void QuestsPanel::readEffectRow(int row) {
    core::QuestStep* step = currentStep();
    if (_filling || step == nullptr || row < 0 || row >= static_cast<int>(step->effects.size())) {
        return;
    }
    const auto* const kind = qobject_cast<QComboBox*>(_effects->cellWidget(row, 0));
    const auto* const flag = qobject_cast<QComboBox*>(_effects->cellWidget(row, 1));
    auto* const value = qobject_cast<QComboBox*>(_effects->cellWidget(row, 2));
    core::QuestEffect& effect = step->effects[static_cast<std::size_t>(row)];
    const std::string previousFlag = effect.flag;
    effect.kind = kind->currentIndex() == 0 ? core::QuestEffect::Kind::SetFlag
                                            : core::QuestEffect::Kind::ClearFlag;
    effect.flag = flag->currentText().trimmed().toStdString();
    effect.value = effect.kind == core::QuestEffect::Kind::SetFlag
                       ? value->currentText().trimmed().toStdString()
                       : std::string{};
    value->setEnabled(effect.kind == core::QuestEffect::Kind::SetFlag);
    if (effect.flag != previousFlag) {
        _filling = true;
        const QString typed = value->currentText();
        value->clear();
        value->addItems(declaredValues(effect.flag));
        value->setCurrentText(typed);
        _filling = false;
    }
    markDirty();
}

void QuestsPanel::renameStep(const std::string& newId) {
    core::QuestStep* step = currentStep();
    if (_filling || step == nullptr || newId == step->id) {
        return;
    }
    if (!isValidQuestName(newId) || _draft.quest.find(newId) != nullptr) {
        showStatus(QStringLiteral("\"%1\" cannot name a step: lowercase letters, digits, - and _, "
                                  "not taken.")
                       .arg(qs(newId)),
                   true);
        const QSignalBlocker blocker(_stepId);
        _stepId->setText(qs(step->id));
        return;
    }
    // Les textes du journal suivent l'étape.
    const std::string oldKey = core::questStepKey(_draft.quest.id, step->id);
    const std::string newKey = core::questStepKey(_draft.quest.id, newId);
    for (auto& [language, texts] : _draft.texts) {
        if (const auto found = texts.find(oldKey); found != texts.end()) {
            std::string text = found->second;
            texts.erase(found);
            texts[newKey] = std::move(text);
        }
    }
    step->id = newId;
    fillSteps();
    markDirty();
}

void QuestsPanel::moveStep(int delta) {
    const int target = _step + delta;
    if (currentStep() == nullptr || target < 0 ||
        target >= static_cast<int>(_draft.quest.steps.size())) {
        return;
    }
    std::swap(_draft.quest.steps[static_cast<std::size_t>(_step)],
              _draft.quest.steps[static_cast<std::size_t>(target)]);
    _step = target;
    fillSteps();
    markDirty();
}

void QuestsPanel::fillUseChoices() {
    const QString flag = _useFlag->currentText();
    const QSignalBlocker blocker(_useFlag);
    _useFlag->clear();
    _useFlag->addItems(knownFlags());
    _useFlag->setCurrentText(
        flag.isEmpty() && !_draft.quest.flags.empty() ? qs(_draft.quest.flags.front().id) : flag);
    const QSignalBlocker valueBlocker(_useValue);
    _useValue->clear();
    _useValue->addItem(QStringLiteral("(any)"));
    _useValue->addItems(declaredValues(_useFlag->currentText().toStdString()));
}

void QuestsPanel::showUses() {
    if (!_uses) {
        _uses = flagUses(_root);
    }
    const std::string flag = _useFlag->currentText().trimmed().toStdString();
    const std::string value =
        _useValue->currentIndex() > 0 ? _useValue->currentText().toStdString() : std::string{};
    _useTree->clear();
    int found = 0;
    for (std::size_t index = 0; index < _uses->size(); ++index) {
        const FlagUse& use = (*_uses)[index];
        if (use.flag != flag ||
            (!value.empty() && std::ranges::find(use.values, value) == use.values.end())) {
            continue;
        }
        QString what = roleText(use.role);
        if (!use.values.empty()) {
            what += QLatin1Char(' ') + joinedValues(use.values);
        }
        auto* const item = new QTreeWidgetItem({what, qs(formatCitation(use.where, _root))});
        item->setData(0, Qt::UserRole, static_cast<int>(index));
        _useTree->addTopLevelItem(item);
        ++found;
    }
    _useTree->resizeColumnToContents(0);
    showStatus(QStringLiteral("%1 uses of %2 (saved files).")
                   .arg(found)
                   .arg(qs(value.empty() ? flag : flag + " = " + value)));
}

void QuestsPanel::markDirty() {
    if (_filling) {
        return;
    }
    _dirty = true;
    if (const int index = _questList->findData(qs(_draft.quest.id)); index >= 0) {
        _questList->setItemText(index, qs(_draft.quest.id) + QStringLiteral(" *"));
    }
}

void QuestsPanel::showStatus(const QString& text, bool error) {
    _status->setText(text);
    _status->setStyleSheet(error ? QStringLiteral("color: #c0392b;") : QString{});
}

void QuestsPanel::setEditable(bool editable) {
    for (QWidget* const widget : _editors) {
        widget->setEnabled(editable);
    }
}

bool QuestsPanel::askAboutChanges() {
    if (!_dirty) {
        return true;
    }
    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, QStringLiteral("Quest changed"),
        QStringLiteral("Save the changes to quest \"%1\"?").arg(qs(_draft.quest.id)),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Save) {
        saveQuest();
        return !_dirty;
    }
    if (answer == QMessageBox::Discard) {
        _dirty = false;
        reload();
        return true;
    }
    return false;
}

void QuestsPanel::newQuest() {
    if (!askAboutChanges()) {
        return;
    }
    bool accepted = false;
    const std::string id =
        QInputDialog::getText(this, QStringLiteral("New quest"),
                              QStringLiteral("Quest id (lowercase letters, digits, - and _):"),
                              QLineEdit::Normal, {}, &accepted)
            .trimmed()
            .toStdString();
    if (!accepted || id.empty()) {
        return;
    }
    std::error_code absent;
    if (!isValidQuestName(id) || std::filesystem::exists(questFile(_root, id), absent)) {
        QMessageBox::warning(this, QStringLiteral("New quest"),
                             QStringLiteral("\"%1\" is not a free quest id.").arg(qs(id)));
        return;
    }
    _draft = QuestDraft{};
    _draft.quest.id = id;
    _draft.quest.name = id;
    _draft.quest.source = "original";
    _savedFlags.clear();
    _hasQuest = true;
    _isNew = true;
    _step = -1;
    {
        const QSignalBlocker blocker(_questList);
        _questList->addItem(qs(id), qs(id));
        _questList->setCurrentIndex(_questList->count() - 1);
    }
    setEditable(true);
    showStatus(QStringLiteral("New quest: declare its flags, then add its steps, then Save."));
    fillAll();
    markDirty();
}

void QuestsPanel::saveQuest() {
    if (!_hasQuest) {
        return;
    }
    const RefactorPlan plan = planSaveQuest(_root, _draft, _isNew);
    if (!plan.ok()) {
        showStatus(QStringLiteral("Not saved, the game would refuse it:\n") + qs(plan.error), true);
        return;
    }
    std::string error;
    if (!applyRefactorPlan(plan, error)) {
        showStatus(QStringLiteral("Not saved: ") + qs(error), true);
        return;
    }
    _isNew = false;
    _dirty = false;
    _uses.reset();
    _savedFlags.clear();
    for (const core::QuestFlag& flag : _draft.quest.flags) {
        _savedFlags.push_back(flag.id);
    }
    if (const int index = _questList->findData(qs(_draft.quest.id)); index >= 0) {
        _questList->setItemText(index, qs(_draft.quest.id));
    }
    fillFlags();
    showStatus(QStringLiteral("Saved %1 (%2 files).")
                   .arg(qs(_draft.quest.id))
                   .arg(static_cast<int>(plan.edits.size())));
    emit questSaved();
}

void QuestsPanel::renameQuest() {
    if (!_hasQuest) {
        return;
    }
    if (_isNew || _dirty) {
        showStatus(QStringLiteral("Save the quest before renaming it."), true);
        return;
    }
    bool accepted = false;
    const std::string oldId = _draft.quest.id;
    const std::string newId =
        QInputDialog::getText(this, QStringLiteral("Rename quest"), QStringLiteral("New id:"),
                              QLineEdit::Normal, qs(oldId), &accepted)
            .trimmed()
            .toStdString();
    if (!accepted || newId.empty() || newId == oldId) {
        return;
    }
    _selectAfterReload = newId;
    const std::filesystem::path root = _root;
    emit planRequested([root, oldId, newId] { return planRenameQuest(root, oldId, newId); },
                       QStringLiteral("Rename quest"));
}

void QuestsPanel::deleteQuest() {
    if (!_hasQuest) {
        return;
    }
    if (_isNew) {
        _dirty = false;
        reload();
        return;
    }
    const std::filesystem::path root = _root;
    const std::string id = _draft.quest.id;
    _dirty = false;
    emit planRequested([root, id] { return planDeleteQuest(root, id); },
                       QStringLiteral("Delete quest"));
}

void QuestsPanel::renameFlag() {
    const int row = _flags->currentRow();
    if (row < 0 || row >= static_cast<int>(_draft.quest.flags.size())) {
        showStatus(QStringLiteral("Select a flag first."), true);
        return;
    }
    const std::string oldFlag = _draft.quest.flags[static_cast<std::size_t>(row)].id;
    if (_dirty || std::ranges::find(_savedFlags, oldFlag) == _savedFlags.end()) {
        showStatus(QStringLiteral("Save the quest first: a saved flag is renamed with its uses."),
                   true);
        return;
    }
    bool accepted = false;
    const std::string newFlag =
        QInputDialog::getText(this, QStringLiteral("Rename flag"),
                              QStringLiteral("New name for %1:").arg(qs(oldFlag)),
                              QLineEdit::Normal, qs(oldFlag), &accepted)
            .trimmed()
            .toStdString();
    if (!accepted || newFlag.empty() || newFlag == oldFlag) {
        return;
    }
    const std::filesystem::path root = _root;
    _selectAfterReload = _draft.quest.id;
    emit planRequested([root, oldFlag, newFlag] { return planRenameFlag(root, oldFlag, newFlag); },
                       QStringLiteral("Rename flag"));
}

void QuestsPanel::renameFlagValue() {
    const int row = _flags->currentRow();
    if (row < 0 || row >= static_cast<int>(_draft.quest.flags.size())) {
        showStatus(QStringLiteral("Select a flag first."), true);
        return;
    }
    const core::QuestFlag flag = _draft.quest.flags[static_cast<std::size_t>(row)];
    if (_dirty || std::ranges::find(_savedFlags, flag.id) == _savedFlags.end()) {
        showStatus(QStringLiteral("Save the quest first: a saved value is renamed with its uses."),
                   true);
        return;
    }
    QStringList values;
    for (const std::string& value : flag.values) {
        values << qs(value);
    }
    bool accepted = false;
    const QString oldValue =
        QInputDialog::getItem(this, QStringLiteral("Rename value"),
                              QStringLiteral("Value of %1:").arg(qs(flag.id)), values, 0,
                              /*editable=*/false, &accepted);
    if (!accepted) {
        return;
    }
    const std::string newValue =
        QInputDialog::getText(this, QStringLiteral("Rename value"),
                              QStringLiteral("New name for %1:").arg(oldValue), QLineEdit::Normal,
                              oldValue, &accepted)
            .trimmed()
            .toStdString();
    if (!accepted || newValue.empty() || newValue == oldValue.toStdString()) {
        return;
    }
    const std::filesystem::path root = _root;
    const std::string flagId = flag.id;
    const std::string from = oldValue.toStdString();
    _selectAfterReload = _draft.quest.id;
    emit planRequested([root, flagId, from,
                        newValue] { return planRenameFlagValue(root, flagId, from, newValue); },
                       QStringLiteral("Rename value"));
}

void QuestsPanel::playStep() {
    const core::QuestStep* step = currentStep();
    if (step == nullptr) {
        return;
    }
    const std::vector<std::string> entries =
        worldStateReaching(_draft.quest, step->id, allDeclarations());
    QStringList shown;
    for (const std::string& entry : entries) {
        shown << qs(entry);
    }
    showStatus(QStringLiteral("World state for \"%1\": %2")
                   .arg(qs(step->id))
                   .arg(shown.isEmpty() ? QStringLiteral("(nothing to set)")
                                        : shown.join(QStringLiteral(", "))));
    emit playStepRequested(entries);
}

void QuestsPanel::goToStepPlace() {
    const core::QuestStep* step = currentStep();
    if (step == nullptr) {
        return;
    }
    const std::size_t hash = step->at.find('#');
    if (hash == std::string::npos) {
        showStatus(QStringLiteral("This step names no place (map#id)."), true);
        return;
    }
    emit citationActivated(Citation{.file = {},
                                    .mapId = step->at.substr(0, hash),
                                    .entityId = step->at.substr(hash + 1),
                                    .cell = std::nullopt,
                                    .what = "quest " + _draft.quest.id + ": step " + step->id});
}

QStringList QuestsPanel::knownFlags() const {
    std::set<std::string> flags;
    if (_references != nullptr) {
        flags.insert(_references->flags.begin(), _references->flags.end());
        for (const core::QuestFlag& flag : _references->declaredFlags) {
            flags.insert(flag.id);
        }
    }
    for (const core::QuestFlag& flag : _draft.quest.flags) {
        flags.insert(flag.id);
    }
    QStringList list;
    for (const std::string& flag : flags) {
        if (!flag.empty()) {
            list << qs(flag);
        }
    }
    return list;
}

QStringList QuestsPanel::declaredValues(const std::string& flag) const {
    for (const core::QuestFlag& declared : allDeclarations()) {
        if (declared.id == flag) {
            QStringList values;
            for (const std::string& value : declared.values) {
                values << qs(value);
            }
            return values;
        }
    }
    return {};
}

std::vector<core::QuestFlag> QuestsPanel::allDeclarations() const {
    std::vector<core::QuestFlag> declarations = _draft.quest.flags;
    if (_references != nullptr) {
        for (const core::QuestFlag& flag : _references->declaredFlags) {
            // Les drapeaux de la quête ouverte : ceux du brouillon, pas ceux du disque.
            if (std::ranges::find(_savedFlags, flag.id) == _savedFlags.end() &&
                std::ranges::find(declarations, flag.id, &core::QuestFlag::id) ==
                    declarations.end()) {
                declarations.push_back(flag);
            }
        }
    }
    return declarations;
}

core::QuestStep* QuestsPanel::currentStep() {
    if (!_hasQuest || _step < 0 || _step >= static_cast<int>(_draft.quest.steps.size())) {
        return nullptr;
    }
    return &_draft.quest.steps[static_cast<std::size_t>(_step)];
}

}  // namespace hmi
