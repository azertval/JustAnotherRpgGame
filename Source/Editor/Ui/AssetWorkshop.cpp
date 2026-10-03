// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Ui/AssetWorkshop.h"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

#include "Editor/Logic/CharacterPreview.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Ui/MapRender.h"
#include "Editor/Ui/SceneSurface.h"
#include "HMI/HmiLog.h"

namespace hmi {

namespace {

constexpr const char* WORKSHOP_ROOT_KEY = "assetWorkshop/root";
constexpr const char* SHEET_FILTER = "Character sheets (*.character.json *.json)";
constexpr int PREVIEW_TICK_MS = 33;
const QColor PREVIEW_BACKGROUND(34, 36, 42);

[[nodiscard]] QString text(const std::string& value) {
    return QString::fromStdString(value);
}

[[nodiscard]] QString text(const std::filesystem::path& value) {
    return QString::fromStdString(value.generic_string());
}

[[nodiscard]] std::filesystem::path path(const QString& value) {
    return std::filesystem::path(value.toStdWString());
}

// La racine de l'atelier local : celle que l'auteur a choisie, à défaut `Tools/Assets3D` du dépôt
// (la racine des données est `Source/Elements`), à défaut la racine des données.
[[nodiscard]] std::filesystem::path defaultWorkshopRoot(const std::filesystem::path& dataRoot) {
    const QString chosen = QSettings().value(QString::fromLatin1(WORKSHOP_ROOT_KEY)).toString();
    std::error_code error;
    if (!chosen.isEmpty() && std::filesystem::is_directory(path(chosen), error)) {
        return path(chosen);
    }
    const std::filesystem::path local =
        (dataRoot / ".." / ".." / "Tools" / "Assets3D").lexically_normal();
    return std::filesystem::is_directory(local, error) ? local : dataRoot;
}

[[nodiscard]] QStringList silhouettes(const std::filesystem::path& dataRoot) {
    QStringList found;
    std::error_code error;
    const std::filesystem::path folder =
        dataRoot / "Assets" / "Common" / "Characters" / "Skeletons";
    if (std::filesystem::is_directory(folder, error)) {
        for (const std::filesystem::directory_entry& entry :
             std::filesystem::directory_iterator(folder, error)) {
            if (entry.is_directory(error)) {
                found.push_back(text(entry.path().filename()));
            }
        }
    }
    found.sort();
    return found;
}

}  // namespace

struct AssetWorkshop::Widgets {
    QLabel* workshop = nullptr;
    QListWidget* installed = nullptr;
    QLabel* sheetFile = nullptr;
    QComboBox* level = nullptr;
    QLineEdit* name = nullptr;
    QComboBox* skeleton = nullptr;
    QLineEdit* model = nullptr;
    QLineEdit* portrait = nullptr;
    QLineEdit* token = nullptr;
    QLineEdit* received = nullptr;
    QLineEdit* liaison = nullptr;
    QPushButton* editInBlender = nullptr;
    QPushButton* importFromBlender = nullptr;
    QPushButton* install = nullptr;
    QPlainTextEdit* log = nullptr;
    QVBoxLayout* previewHost = nullptr;
    QComboBox* clip = nullptr;
    QCheckBox* play = nullptr;
    /// L'heure de l'aperçu (`LOT-1007`) : « Unlit », ou l'une des quatre heures du lot.
    QComboBox* hour = nullptr;
};

AssetWorkshop::AssetWorkshop(std::filesystem::path dataRoot, QWidget* parent)
    : QDialog(parent, Qt::Window),
      _dataRoot(std::move(dataRoot)),
      _workshopRoot(defaultWorkshopRoot(_dataRoot)),
      _w(new Widgets) {
    setWindowTitle(QStringLiteral("Asset workshop"));
    setObjectName(QStringLiteral("AssetWorkshop"));
    resize(1280, 760);
    buildUi();
    reloadInstalled();
    newSheet();
    _clock = new QTimer(this);
    _clock->setInterval(PREVIEW_TICK_MS);
    connect(_clock, &QTimer::timeout, this, &AssetWorkshop::refreshPreview);
    _clock->start();
    _elapsed.start();
}

AssetWorkshop::~AssetWorkshop() {
    delete _w;
}

void AssetWorkshop::buildUi() {
    auto* const columns = new QSplitter(Qt::Horizontal, this);

    auto* const left = new QWidget(columns);
    auto* const leftColumn = new QVBoxLayout(left);
    leftColumn->addWidget(new QLabel(QStringLiteral("Installed characters"), left));
    _w->installed = new QListWidget(left);
    leftColumn->addWidget(_w->installed, 1);
    auto* const check = new QPushButton(QStringLiteral("Check installed characters"), left);
    leftColumn->addWidget(check);
    connect(check, &QPushButton::clicked, this, &AssetWorkshop::checkInstalled);
    connect(_w->installed, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        openInstalled(InstalledCharacter{item->data(Qt::UserRole).toString().toStdString(),
                                         item->data(Qt::UserRole + 1).toString().toStdString()});
    });
    connect(_w->installed, &QListWidget::itemClicked, _w->installed, &QListWidget::itemActivated);

    columns->addWidget(left);
    columns->addWidget(buildSheetColumn());
    columns->addWidget(buildPreviewColumn());
    columns->setStretchFactor(0, 2);
    columns->setStretchFactor(1, 4);
    columns->setStretchFactor(2, 3);

    auto* const whole = new QVBoxLayout(this);
    whole->addWidget(columns);
}

QLineEdit* AssetWorkshop::addFileRow(QVBoxLayout* column, const QString& label,
                                     const QString& filter) {
    auto* const row = new QHBoxLayout;
    auto* const caption = new QLabel(label, this);
    caption->setMinimumWidth(110);
    auto* const edit = new QLineEdit(this);
    auto* const browse = new QPushButton(QStringLiteral("Browse…"), this);
    row->addWidget(caption);
    row->addWidget(edit, 1);
    row->addWidget(browse);
    column->addLayout(row);
    connect(browse, &QPushButton::clicked, this, [this, edit, label, filter] {
        const QString start = edit->text().isEmpty()
                                  ? text(sheetRoot())
                                  : text((sheetRoot() / path(edit->text())).lexically_normal());
        const QString chosen = QFileDialog::getOpenFileName(this, label, start, filter);
        if (!chosen.isEmpty()) {
            edit->setText(written(chosen));
            reloadPreview();
        }
    });
    return edit;
}

QWidget* AssetWorkshop::buildSheetColumn() {
    auto* const column = new QWidget(this);
    auto* const rows = new QVBoxLayout(column);

    auto* const workshopRow = new QHBoxLayout;
    _w->workshop = new QLabel(column);
    _w->workshop->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto* const change = new QPushButton(QStringLiteral("Workshop folder…"), column);
    workshopRow->addWidget(_w->workshop, 1);
    workshopRow->addWidget(change);
    rows->addLayout(workshopRow);
    connect(change, &QPushButton::clicked, this, [this] {
        const QString chosen = QFileDialog::getExistingDirectory(
            this, QStringLiteral("Workshop folder"), text(_workshopRoot));
        if (!chosen.isEmpty()) {
            _workshopRoot = path(chosen);
            QSettings().setValue(QString::fromLatin1(WORKSHOP_ROOT_KEY), chosen);
            newSheet();
        }
    });

    auto* const sheetRow = new QHBoxLayout;
    auto* const fresh = new QPushButton(QStringLiteral("New"), column);
    auto* const open = new QPushButton(QStringLiteral("Open sheet…"), column);
    auto* const save = new QPushButton(QStringLiteral("Save sheet"), column);
    auto* const saveAs = new QPushButton(QStringLiteral("Save sheet as…"), column);
    for (QPushButton* const button : {fresh, open, save, saveAs}) {
        sheetRow->addWidget(button);
    }
    sheetRow->addStretch(1);
    rows->addLayout(sheetRow);
    _w->sheetFile = new QLabel(column);
    _w->sheetFile->setTextInteractionFlags(Qt::TextSelectableByMouse);
    rows->addWidget(_w->sheetFile);
    connect(fresh, &QPushButton::clicked, this, &AssetWorkshop::newSheet);
    connect(open, &QPushButton::clicked, this, &AssetWorkshop::chooseSheet);
    connect(save, &QPushButton::clicked, this, [this] { static_cast<void>(saveSheet(false)); });
    connect(saveAs, &QPushButton::clicked, this, [this] { static_cast<void>(saveSheet(true)); });

    auto* const sheet = new QGroupBox(QStringLiteral("Character"), column);
    auto* const sheetRows = new QVBoxLayout(sheet);
    auto* const form = new QFormLayout;
    _w->level = new QComboBox(sheet);
    _w->level->setEditable(true);
    _w->name = new QLineEdit(sheet);
    _w->name->setPlaceholderText(QStringLiteral("bandit, Heroes/brawler"));
    _w->skeleton = new QComboBox(sheet);
    _w->skeleton->setEditable(true);
    form->addRow(QStringLiteral("Level"), _w->level);
    form->addRow(QStringLiteral("Name"), _w->name);
    form->addRow(QStringLiteral("Skeleton"), _w->skeleton);
    sheetRows->addLayout(form);
    _w->model =
        addFileRow(sheetRows, QStringLiteral("Linked model"), QStringLiteral("Models (*.glb)"));
    _w->portrait =
        addFileRow(sheetRows, QStringLiteral("Portrait (512)"), QStringLiteral("Images (*.png)"));
    _w->token =
        addFileRow(sheetRows, QStringLiteral("Token (128)"), QStringLiteral("Images (*.png)"));
    for (QLineEdit* const edit : {_w->model, _w->portrait, _w->token}) {
        edit->setPlaceholderText(QStringLiteral("empty: keep what is installed"));
    }
    rows->addWidget(sheet);
    connect(_w->model, &QLineEdit::editingFinished, this, &AssetWorkshop::reloadPreview);

    auto* const blender = new QGroupBox(QStringLiteral("Blender round trip"), column);
    auto* const blenderRows = new QVBoxLayout(blender);
    _w->received =
        addFileRow(blenderRows, QStringLiteral("Received model"), QStringLiteral("Models (*.glb)"));
    _w->liaison = addFileRow(blenderRows, QStringLiteral("Liaison sheet"),
                             QStringLiteral("Liaison sheets (*.json)"));
    auto* const blenderButtons = new QHBoxLayout;
    _w->editInBlender = new QPushButton(QStringLiteral("Edit in Blender"), blender);
    _w->importFromBlender = new QPushButton(QStringLiteral("Import from Blender"), blender);
    blenderButtons->addWidget(_w->editInBlender);
    blenderButtons->addWidget(_w->importFromBlender);
    blenderButtons->addStretch(1);
    blenderRows->addLayout(blenderButtons);
    auto* const hint = new QLabel(
        QStringLiteral("Move joints in Edit mode, change the keys of an action in Pose mode, "
                       "save the .blend, then import: joints go to the liaison sheet, clips to "
                       "retouche.json, and the model is linked again."),
        blender);
    hint->setWordWrap(true);
    blenderRows->addWidget(hint);
    rows->addWidget(blender);
    connect(_w->editInBlender, &QPushButton::clicked, this, &AssetWorkshop::editInBlender);
    connect(_w->importFromBlender, &QPushButton::clicked, this, &AssetWorkshop::importFromBlender);

    _w->install = new QPushButton(QStringLiteral("Install in the game"), column);
    rows->addWidget(_w->install);
    connect(_w->install, &QPushButton::clicked, this, &AssetWorkshop::install);

    _w->log = new QPlainTextEdit(column);
    _w->log->setReadOnly(true);
    _w->log->setObjectName(QStringLiteral("AssetWorkshopLog"));
    rows->addWidget(_w->log, 1);
    return column;
}

QWidget* AssetWorkshop::buildPreviewColumn() {
    auto* const column = new QWidget(this);
    auto* const rows = new QVBoxLayout(column);
    auto* const host = new QWidget(column);
    host->setMinimumSize(320, 420);
    _w->previewHost = new QVBoxLayout(host);
    _w->previewHost->setContentsMargins(0, 0, 0, 0);
    rows->addWidget(host, 1);

    auto* const controls = new QHBoxLayout;
    _w->clip = new QComboBox(column);
    _w->play = new QCheckBox(QStringLiteral("Play"), column);
    _w->play->setChecked(true);
    auto* const turn = new QPushButton(QStringLiteral("Quarter turn"), column);
    controls->addWidget(new QLabel(QStringLiteral("Clip"), column));
    controls->addWidget(_w->clip, 1);
    controls->addWidget(_w->play);
    controls->addWidget(turn);
    // L'heure de l'aperçu (LOT-1007) : un modèle se juge éclairé, à midi comme de nuit. La
    // donnée de chaque entrée est l'heure en minutes ; négative, l'aperçu est sans éclairage.
    _w->hour = new QComboBox(column);
    _w->hour->addItem(QStringLiteral("Noon"), 720.0);
    _w->hour->addItem(QStringLiteral("Dawn"), 390.0);
    _w->hour->addItem(QStringLiteral("Dusk"), 1140.0);
    _w->hour->addItem(QStringLiteral("Night"), 0.0);
    _w->hour->addItem(QStringLiteral("Unlit"), -1.0);
    _w->hour->setToolTip(
        QStringLiteral("The hour the preview is lit at, as the game lights the character."));
    controls->addWidget(new QLabel(QStringLiteral("Light"), column));
    controls->addWidget(_w->hour);
    rows->addLayout(controls);
    connect(_w->hour, &QComboBox::currentIndexChanged, this, [this] { refreshPreview(); });
    connect(turn, &QPushButton::clicked, this, [this] {
        _quarterTurns = (_quarterTurns + 1) % 4;
        refreshPreview();
    });
    connect(_w->clip, &QComboBox::currentTextChanged, this, [this] {
        _elapsed.restart();
        refreshPreview();
    });
    connect(_w->skeleton, &QComboBox::currentTextChanged, this, &AssetWorkshop::reloadPreview);
    return column;
}

std::filesystem::path AssetWorkshop::baseDirectory() const {
    return _sheetFile.empty() ? _workshopRoot : _sheetFile.parent_path();
}

std::filesystem::path AssetWorkshop::sheetRoot() const {
    return (baseDirectory() / _root).lexically_normal();
}

QString AssetWorkshop::written(const QString& file) const {
    const std::filesystem::path relative = path(file).lexically_relative(sheetRoot());
    // Un fichier d'un autre volume n'a pas de chemin relatif : il s'écrit tel quel.
    return relative.empty() ? file : text(relative);
}

void AssetWorkshop::log(const QString& line) {
    _w->log->appendPlainText(line);
}

void AssetWorkshop::reloadInstalled() {
    _w->installed->clear();
    for (const InstalledCharacter& character : installedCharacters(_dataRoot)) {
        // Le niveau sans son préfixe ni son `/Characters` : `capital/arenarea/arena-of-fate`.
        QString level = text(character.level);
        level.remove(QStringLiteral("Regions/")).remove(QStringLiteral("/Characters"));
        auto* const item = new QListWidgetItem(
            QStringLiteral("%1  —  %2").arg(text(character.name), level), _w->installed);
        item->setData(Qt::UserRole, text(character.level));
        item->setData(Qt::UserRole + 1, text(character.name));
    }
    const QString level = _w->level->currentText();
    _w->level->clear();
    for (const std::string& known : characterLevels(_dataRoot)) {
        _w->level->addItem(text(known));
    }
    _w->level->setCurrentText(level);
    const QString skeleton = _w->skeleton->currentText();
    _w->skeleton->clear();
    _w->skeleton->addItems(silhouettes(_dataRoot));
    if (!skeleton.isEmpty()) {
        _w->skeleton->setCurrentText(skeleton);
    }
}

void AssetWorkshop::showDraft(const CharacterDraft& draft) {
    _root = draft.root;
    _skeletonSource = draft.skeletonSource;
    _workshopExtras = draft.workshop;
    _w->workshop->setText(QStringLiteral("Workshop: %1").arg(text(_workshopRoot)));
    _w->sheetFile->setText(_sheetFile.empty() ? QStringLiteral("Sheet: (not saved)")
                                              : QStringLiteral("Sheet: %1").arg(text(_sheetFile)));
    _w->level->setCurrentText(text(draft.level));
    _w->name->setText(text(draft.name));
    if (!draft.skeleton.empty()) {
        _w->skeleton->setCurrentText(text(draft.skeleton));
    }
    _w->model->setText(text(draft.model));
    _w->portrait->setText(text(draft.portrait));
    _w->token->setText(text(draft.token));
    _w->received->setText(text(draft.workshop.received));
    _w->liaison->setText(text(draft.workshop.sheet));
    reloadPreview();
}

CharacterDraft AssetWorkshop::draft() const {
    CharacterDraft draft;
    draft.root = _root;
    draft.level = _w->level->currentText().trimmed().toStdString();
    draft.name = _w->name->text().trimmed().toStdString();
    draft.skeleton = _w->skeleton->currentText().trimmed().toStdString();
    draft.skeletonSource = _skeletonSource;
    draft.model = _w->model->text().trimmed().toStdString();
    draft.portrait = _w->portrait->text().trimmed().toStdString();
    draft.token = _w->token->text().trimmed().toStdString();
    draft.workshop = _workshopExtras;
    draft.workshop.received = _w->received->text().trimmed().toStdString();
    draft.workshop.sheet = _w->liaison->text().trimmed().toStdString();
    return draft;
}

void AssetWorkshop::newSheet() {
    _sheetFile.clear();
    CharacterDraft empty;
    empty.skeleton = "humanoid";
    showDraft(empty);
}

void AssetWorkshop::openInstalled(const InstalledCharacter& character) {
    const CharacterDraftResult opened =
        draftOfInstalledCharacter(_dataRoot, _workshopRoot, character);
    if (!opened.ok()) {
        log(QStringLiteral("error: %1").arg(text(opened.error)));
        return;
    }
    _sheetFile.clear();
    showDraft(opened.draft);
    log(QStringLiteral("Opened %1 as installed.").arg(text(character.name)));
}

bool AssetWorkshop::openSheet(const std::filesystem::path& file) {
    std::ifstream input(file, std::ios::binary);
    std::ostringstream content;
    content << input.rdbuf();
    const CharacterDraftResult read = readCharacterDraft(content.str());
    if (!input || !read.ok()) {
        log(QStringLiteral("error: %1: %2")
                .arg(text(file), input ? text(read.error) : QStringLiteral("cannot be read")));
        return false;
    }
    _sheetFile = std::filesystem::absolute(file);
    showDraft(read.draft);
    log(QStringLiteral("Opened %1.").arg(text(_sheetFile)));
    return true;
}

void AssetWorkshop::chooseSheet() {
    const QString chosen =
        QFileDialog::getOpenFileName(this, QStringLiteral("Open character sheet"),
                                     text(baseDirectory()), QString::fromLatin1(SHEET_FILTER));
    if (!chosen.isEmpty()) {
        static_cast<void>(openSheet(path(chosen)));
    }
}

bool AssetWorkshop::saveSheet(bool askName) {
    CharacterDraft current = draft();
    std::filesystem::path file = _sheetFile;
    if (file.empty() || askName) {
        const std::string stem = current.name.substr(current.name.rfind('/') + 1);
        const QString chosen = QFileDialog::getSaveFileName(
            this, QStringLiteral("Save character sheet"),
            text(baseDirectory() / (stem + ".character.json")), QString::fromLatin1(SHEET_FILTER));
        if (chosen.isEmpty()) {
            return false;
        }
        file = path(chosen);
        // Les chemins de la fiche partent de sa racine : elle ne bouge pas quand la fiche change
        // de dossier, c'est `root` qui la retrouve.
        const std::filesystem::path root = sheetRoot().lexically_relative(file.parent_path());
        current.root = root.empty() ? sheetRoot().generic_string() : root.generic_string();
    }
    std::ofstream output(file, std::ios::binary | std::ios::trunc);
    output << characterDraftText(current);
    if (!output) {
        log(QStringLiteral("error: cannot write %1").arg(text(file)));
        return false;
    }
    _sheetFile = file;
    _root = current.root;
    _w->sheetFile->setText(QStringLiteral("Sheet: %1").arg(text(_sheetFile)));
    log(QStringLiteral("Saved %1.").arg(text(_sheetFile)));
    return true;
}

void AssetWorkshop::install() {
    const CharacterInstallPlan plan = planCharacter(_dataRoot, baseDirectory(), draft());
    if (!plan.ok()) {
        log(QStringLiteral("error: %1 — nothing written").arg(text(plan.error)));
        return;
    }
    if (const std::string error = writeCharacter(plan); !error.empty()) {
        log(QStringLiteral("error: %1").arg(text(error)));
        return;
    }
    log(text(plan.log).trimmed());
    HMI_LOG_INFO("Editeur : atelier des assets, personnage installe -- " + plan.log);
    reloadInstalled();
    reloadPreview();
    emit characterInstalled();
}

void AssetWorkshop::checkInstalled() {
    const std::vector<MapCheckFinding> findings = checkCharacters(_dataRoot);
    for (const MapCheckFinding& finding : findings) {
        log(text(formatFinding(finding)));
    }
    log(findings.empty()
            ? QStringLiteral("Installed characters: no problem found.")
            : QStringLiteral("Installed characters: %1 finding(s).").arg(findings.size()));
}

void AssetWorkshop::run(const ProcessCommand& command, const QString& what,
                        std::function<void(int)> done) {
    if (_process != nullptr && _process->state() != QProcess::NotRunning) {
        log(QStringLiteral("Still working: wait for the current step to finish."));
        return;
    }
    if (_process != nullptr) {
        _process->deleteLater();
    }
    _process = new QProcess(this);
    _process->setProcessChannelMode(QProcess::MergedChannels);
    QStringList arguments;
    for (const std::string& argument : command.arguments) {
        arguments.push_back(text(argument));
    }
    connect(_process, &QProcess::errorOccurred, this, [this, what](QProcess::ProcessError) {
        log(QStringLiteral("error: %1 could not be started: %2")
                .arg(what, _process->errorString()));
        _w->editInBlender->setEnabled(true);
        _w->importFromBlender->setEnabled(true);
    });
    connect(_process, &QProcess::finished, this,
            [this, what, done = std::move(done)](int code, QProcess::ExitStatus) {
                const QString output = QString::fromUtf8(_process->readAll()).trimmed();
                if (!output.isEmpty()) {
                    log(output);
                }
                _w->editInBlender->setEnabled(true);
                _w->importFromBlender->setEnabled(true);
                done(code);
            });
    _w->editInBlender->setEnabled(false);
    _w->importFromBlender->setEnabled(false);
    log(QStringLiteral("%1…").arg(what));
    HMI_LOG_INFO("Editeur : atelier des assets, " + command.program + " " +
                 arguments.join(QLatin1Char(' ')).toStdString());
    // Le script écrit en UTF-8 : sans cela, Python prend la page de code de la console.
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("PYTHONIOENCODING"), QStringLiteral("utf-8"));
    _process->setProcessEnvironment(environment);
    _process->start(text(command.program), arguments);
}

void AssetWorkshop::editInBlender() {
    const RetouchFiles files = retouchFiles(_dataRoot, baseDirectory(), draft());
    if (const std::string missing = retouchReadiness(files); !missing.empty()) {
        log(QStringLiteral("Cannot open Blender: %1.").arg(text(missing)));
        return;
    }
    std::string error;
    const RetouchTools tools = findRetouchTools(
        _dataRoot,
        [](const std::string& name) -> std::optional<std::string> {
            return qEnvironmentVariableIsSet(name.c_str())
                       ? std::optional{qEnvironmentVariable(name.c_str()).toStdString()}
                       : std::nullopt;
        },
        error);
    if (!error.empty()) {
        log(QStringLiteral("Cannot open Blender: %1.").arg(text(error)));
        return;
    }
    run(openInBlenderCommand(tools, files), QStringLiteral("Opening the model in Blender"),
        [this, files](int code) {
            log(code == 0
                    ? QStringLiteral("Blender is open on %1. Save it there (Ctrl+S), then "
                                     "\"Import from Blender\".")
                          .arg(text(files.blend))
                    : QStringLiteral("error: Blender did not open (exit code %1).").arg(code));
        });
}

void AssetWorkshop::importFromBlender() {
    const RetouchFiles files = retouchFiles(_dataRoot, baseDirectory(), draft());
    if (const std::string missing = retouchReadiness(files); !missing.empty()) {
        log(QStringLiteral("Cannot import: %1.").arg(text(missing)));
        return;
    }
    std::error_code ignored;
    if (!std::filesystem::is_regular_file(files.blend, ignored)) {
        log(QStringLiteral("Cannot import: %1 does not exist — \"Edit in Blender\" first.")
                .arg(text(files.blend)));
        return;
    }
    std::string error;
    const RetouchTools tools = findRetouchTools(
        _dataRoot,
        [](const std::string& name) -> std::optional<std::string> {
            return qEnvironmentVariableIsSet(name.c_str())
                       ? std::optional{qEnvironmentVariable(name.c_str()).toStdString()}
                       : std::nullopt;
        },
        error);
    if (!error.empty()) {
        log(QStringLiteral("Cannot import: %1.").arg(text(error)));
        return;
    }
    run(importFromBlenderCommand(tools, files), QStringLiteral("Importing from Blender"),
        [this](int code) {
            log(code == 0 ? QStringLiteral("The model is linked again and holds the standard. "
                                           "\"Install in the game\" puts it in the kit.")
                          : QStringLiteral("error: the import was refused (exit code %1): the "
                                           "linked model may not hold the standard — see above.")
                                .arg(code));
            reloadPreview();
        });
}

void AssetWorkshop::reloadPreview() {
    // Le modèle de la fiche, à défaut celui qui est installé.
    const CharacterDraft current = draft();
    std::filesystem::path model;
    std::error_code ignored;
    if (!current.model.empty()) {
        model = (sheetRoot() / current.model).lexically_normal();
    } else if (!current.level.empty() && !current.name.empty()) {
        const std::filesystem::path folder = _dataRoot / "Assets" /
                                             std::filesystem::path(current.level) /
                                             std::filesystem::path(current.name);
        if (const core::CharacterSheetFileResult sheet =
                core::readCharacterSheetFile(folder / core::CHARACTER_SHEET_FILE);
            sheet.ok()) {
            model = folder / sheet.sheet.model;
        }
    }
    if (model.empty() || !std::filesystem::is_regular_file(model, ignored)) {
        model.clear();
    } else {
        model = std::filesystem::absolute(model);
    }

    _skeleton.reset();
    if (!current.skeleton.empty()) {
        if (core::SkeletonFileResult read = core::readSkeletonFile(
                _dataRoot / "Assets" /
                std::filesystem::path(core::skeletonFilePath(current.skeleton)));
            read.ok()) {
            _skeleton = std::move(read.skeleton);
        }
    }
    const QString clip = _w->clip->currentText();
    {
        const QSignalBlocker blocked(_w->clip);
        _w->clip->clear();
        if (_skeleton) {
            for (const core::SkeletonClip& declared : _skeleton->clips) {
                _w->clip->addItem(text(declared.name));
            }
        }
        if (_w->clip->findText(clip) >= 0) {
            _w->clip->setCurrentText(clip);
        }
    }

    // Une surface neuve à chaque modèle : le rendu ne recharge pas un fichier qu'il a déjà lu, et
    // un modèle relié garde son nom.
    if (_surface != nullptr) {
        _w->previewHost->removeWidget(_surface);
        _surface->deleteLater();
        _surface = nullptr;
    }
    _previewModel.clear();
    if (!model.empty()) {
        _surface = new SceneSurface(model.root_path(), this);
        _surface->setClearColor(PREVIEW_BACKGROUND);
        // Le cadrage dépend de la hauteur de la surface : il suit chaque changement de taille.
        _surface->installEventFilter(this);
        _w->previewHost->addWidget(_surface);
        _previewModel = model.relative_path().generic_string();
        _elapsed.restart();
    }
    refreshPreview();
}

bool AssetWorkshop::eventFilter(QObject* watched, QEvent* event) {
    if (watched == _surface && event->type() == QEvent::Resize) {
        refreshPreview();
    }
    return QDialog::eventFilter(watched, event);
}

void AssetWorkshop::refreshPreview() {
    if (_surface == nullptr || _previewModel.empty()) {
        return;
    }
    // En pause, l'aperçu reste sur l'instant où il s'est arrêté.
    if (_w->play->isChecked()) {
        _heldSeconds = static_cast<float>(_elapsed.elapsed()) / 1000.0F;
    }
    const std::string clip = _w->clip->currentText().toStdString();
    const core::SkeletonClip* const declared = _skeleton ? _skeleton->clip(clip) : nullptr;
    const CharacterPreviewView view{.model = _previewModel,
                                    .clip = clip.empty() ? std::string{"idle"} : clip,
                                    .seconds = characterPreviewSeconds(declared, _heldSeconds),
                                    .quarterTurns = _quarterTurns};
    _surface->renderer().setSnapshot(characterPreviewScene(view));
    // La lumière de l'heure choisie, celle de la table du contenu (LOT-1007).
    const auto minutes = static_cast<float>(_w->hour->currentData().toDouble());
    _surface->renderer().setLighting(
        minutes >= 0.0F ? std::optional<WorldLighting>{WorldLighting{
                              .light = placeDayLight(_dataRoot / "Assets").sample(minutes),
                              .shadows = true,
                              .shadowSize = 2048,
                              .seconds = _heldSeconds}}
                        : std::nullopt);
    _surface->renderer().setFraming(characterPreviewFraming(_surface->height()));
    _surface->update();
}

}  // namespace hmi
