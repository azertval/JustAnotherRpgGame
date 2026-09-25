// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/DebugConsoleModel.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QTimer>
#include <QVariantMap>
#include <optional>
#include <string_view>

#include "Core/BuildConfig.h"
#include "Core/Diagnostics/LogLevelParse.h"
#include "Core/Diagnostics/Logger.h"
#include "Core/World/ExplorationSession.h"
#include "HMI/Game/DebugCommands.h"
#include "HMI/Game/LaunchOptions.h"
#include "HMI/HmiLog.h"
#include "HMI/Runtime/WorldModel.h"

namespace hmi {

namespace {

[[nodiscard]] QString versQt(std::string_view texte) {
    return QString::fromUtf8(texte.data(), static_cast<qsizetype>(texte.size()));
}

// Les mots qui demandent l'aide, quelle que soit la langue du clavier.
[[nodiscard]] bool demandeLAide(std::string_view mot) {
    return mot == "aide" || mot == "help" || mot == "?" || mot == "--help" || mot == "-h";
}

// Une case sans carte : le heros y va sur la carte courante, si elle l'a.
void placeOnCurrentMap(WorldModel* world, const core::GridPosition& cellule,
                       QStringList& transcript) {
    if (!world->loaded()) {
        transcript.push_back(
            QStringLiteral("--at= sans carte ouverte : la case s'appliquera au prochain "
                           "--map=."));
        world->setStartCell(cellule);
    } else if (cellule.column >= world->columns() || cellule.row >= world->rows()) {
        transcript.push_back(QStringLiteral("--at= hors de la carte (%1 x %2).")
                                 .arg(world->columns())
                                 .arg(world->rows()));
    } else {
        world->placeHero(core::cellCenter(cellule));
        transcript.push_back(
            QStringLiteral("Heros pose en %1,%2").arg(cellule.column).arg(cellule.row));
    }
}

}  // namespace

DebugConsoleModel::DebugConsoleModel(QObject* parent) : QObject(parent) {
    _transcript.push_back(
        QStringLiteral("Console de debug : tapez une ligne d'options du jeu, ou « aide »."));
}

QVariantList DebugConsoleModel::options() {
    QVariantList liste;
    for (const DebugOption& option : debugOptionCatalog()) {
        QVariantMap ligne;
        ligne.insert(QStringLiteral("name"), versQt(option.name));
        ligne.insert(QStringLiteral("syntax"), versQt(option.syntax));
        ligne.insert(QStringLiteral("description"), versQt(option.description));
        ligne.insert(QStringLiteral("live"), option.scope == DebugOptionScope::Live);
        liste.push_back(ligne);
    }
    return liste;
}

void DebugConsoleModel::say(const QString& line) {
    _transcript.push_back(line);
    emit transcriptChanged();
}

void DebugConsoleModel::clear() {
    _transcript.clear();
    emit transcriptChanged();
}

void DebugConsoleModel::printHelp() {
    _transcript.push_back(QStringLiteral("Options du jeu (a chaud sauf mention) :"));
    for (const DebugOption& option : debugOptionCatalog()) {
        const QString portee = option.scope == DebugOptionScope::Live
                                   ? QString{}
                                   : QStringLiteral("  [au lancement : « Relancer »]");
        _transcript.push_back(QStringLiteral("  %1%2").arg(versQt(option.syntax), portee));
        _transcript.push_back(QStringLiteral("      %1").arg(versQt(option.description)));
    }
    _transcript.push_back(
        QStringLiteral("Plusieurs options sur une ligne s'appliquent dans l'ordre du lancement : "
                       "--levels, --flags, --at, --hero-figure, puis --map."));
}

void DebugConsoleModel::run(const QString& line) {
    const QString propre = line.trimmed();
    if (propre.isEmpty()) {
        return;
    }
    _history.push_back(propre);
    _transcript.push_back(QStringLiteral("> %1").arg(propre));
    // `if constexpr` avec sa branche `else` : un retour anticipe laisserait en Release un code
    // inatteignable, que /W4 /WX refuse (C4702).
    if constexpr (core::DEVELOPER_BUILD) {
        const std::vector<std::string> mots = splitCommandLine(propre.toStdString());
        if (mots.empty() || demandeLAide(mots.front())) {
            printHelp();
        } else {
            apply(mots);
        }
    } else {
        _transcript.push_back(QStringLiteral("La console n'existe pas dans un binaire livre."));
    }
    emit transcriptChanged();
}

struct DebugConsoleModel::Pending {
    std::optional<std::string> levels;
    std::optional<std::string> flags;
    std::optional<std::string> at;
    std::optional<std::string> heroFigure;
    std::optional<std::string> map;

    [[nodiscard]] bool empty() const {
        return !levels && !flags && !at && !heroFigure && !map;
    }
};

void DebugConsoleModel::apply(const std::vector<std::string>& words) {
    WorldModel* const world = WorldModel::current();
    // Ce qui ouvre une carte se collecte, puis s'applique dans l'ordre du lancement ; le reste
    // s'applique en passant.
    Pending pending;
    for (const std::string& mot : words) {
        applyWord(mot, pending);
    }
    if (pending.empty()) {
        return;
    }
    applyPending(world, pending);
}

void DebugConsoleModel::applyWord(const std::string& word, Pending& pending) {
    const DebugArgument argument = splitDebugArgument(word);
    const DebugOption* const option = findDebugOption(argument.name);
    if (option == nullptr) {
        _transcript.push_back(
            QStringLiteral("Option inconnue : %1 (tapez « aide »).").arg(versQt(word)));
        return;
    }
    if (option->scope == DebugOptionScope::LaunchOnly) {
        _transcript.push_back(
            QStringLiteral("%1 ne se lit qu'au lancement : « Relancer » redemarre le jeu avec "
                           "cette ligne.")
                .arg(versQt(option->name)));
        return;
    }
    if (option->takesValue() && argument.value.empty()) {
        _transcript.push_back(QStringLiteral("%1 attend une valeur : %2")
                                  .arg(versQt(option->name), versQt(option->syntax)));
        return;
    }
    const std::string_view nom = option->name;
    if (nom == "--map=") {
        pending.map = argument.value;
    } else if (nom == "--at=") {
        pending.at = argument.value;
    } else if (nom == "--flags=") {
        pending.flags = argument.value;
    } else if (nom == "--hero-figure=") {
        pending.heroFigure = argument.value;
    } else if (nom == "--levels=") {
        pending.levels = argument.value;
    } else {
        applyImmediate(nom, argument.value);
    }
}

void DebugConsoleModel::applyImmediate(std::string_view name, const std::string& value) {
    if (name == "--screen=") {
        emit screenRequested(versQt(value));
        _transcript.push_back(QStringLiteral("Ecran : %1").arg(versQt(value)));
    } else if (name == "--window-size=") {
        if (const auto taille = parseWindowSize(value)) {
            emit windowSizeRequested(taille->first, taille->second);
            _transcript.push_back(
                QStringLiteral("Fenetre : %1 x %2").arg(taille->first).arg(taille->second));
        } else {
            _transcript.push_back(QStringLiteral("--window-size= attend <largeur>x<hauteur>."));
        }
    } else if (name == "--screenshot=") {
        emit screenshotRequested(versQt(value));
    } else if (name == "--log-level=") {
        if (const std::optional<core::LogLevel> niveau = core::parseLogLevel(value)) {
            core::defaultLogger().setMinimumLevel(*niveau);
            _transcript.push_back(QStringLiteral("Journal : niveau minimum %1").arg(versQt(value)));
        } else {
            _transcript.push_back(QStringLiteral("--log-level= attend trace, info, warning ou error."));
        }
    }
}

void DebugConsoleModel::applyPending(WorldModel* world, const Pending& pending) {
    if (world == nullptr) {
        _transcript.push_back(QStringLiteral("Aucune partie : le modele du monde n'existe pas."));
        return;
    }
    // L'ordre de Main.cpp : les dossiers refont la session et emporteraient le reste.
    if (pending.levels) {
        world->setLevelDirectories(parseLevelDirectories(*pending.levels));
        _transcript.push_back(
            QStringLiteral("Cartes lues d'abord dans : %1 (la session est refaite ; rouvrez une "
                           "carte).")
                .arg(versQt(*pending.levels)));
    }
    if (pending.flags) {
        QStringList poses;
        for (const std::string& drapeau : parseWorldFlags(*pending.flags)) {
            poses.push_back(QString::fromStdString(drapeau));
        }
        world->setStartFlags(poses);
        _transcript.push_back(QStringLiteral("Drapeaux poses : %1").arg(poses.size()));
    }
    std::optional<core::GridPosition> cellule;
    if (pending.at) {
        cellule = parseStartCell(*pending.at);
        if (!cellule) {
            _transcript.push_back(QStringLiteral("--at= attend <colonne>,<ligne> : ignore."));
        }
    }
    if (pending.heroFigure) {
        world->setHeroFigure(versQt(*pending.heroFigure));
        _transcript.push_back(
            QStringLiteral("Figurine du heros : %1").arg(versQt(*pending.heroFigure)));
    }
    if (pending.map) {
        const QStringList parts = versQt(*pending.map).split(QLatin1Char('@'));
        world->setStartCell(cellule);
        world->setStartOverride(parts.value(0), parts.value(1));
        if (world->startNewGame()) {
            HMI_LOG_INFO("Console de debug : carte ouverte, " + *pending.map);
            _transcript.push_back(QStringLiteral("Carte ouverte : %1").arg(parts.value(0)));
            emit gameRequested();
        } else {
            _transcript.push_back(
                QStringLiteral("La carte ne s'ouvre pas : %1").arg(world->status()));
        }
    } else if (cellule) {
        placeOnCurrentMap(world, *cellule, _transcript);
    }
}

bool DebugConsoleModel::relaunch(const QString& line) {
    if constexpr (core::DEVELOPER_BUILD) {
        QStringList arguments;
        for (const std::string& mot : splitCommandLine(line.trimmed().toStdString())) {
            arguments.push_back(QString::fromStdString(mot));
        }
        const QString programme = QCoreApplication::applicationFilePath();
        const bool parti = QProcess::startDetached(programme, arguments, QDir::currentPath());
        if (!parti) {
            say(QStringLiteral("Relancement impossible : %1").arg(programme));
            return false;
        }
        HMI_LOG_INFO("Console de debug : relancement avec « " + line.trimmed().toStdString() +
                     " ».");
        // Quitter APRES avoir rendu la main : la console a encore une image a finir.
        QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
        return true;
    } else {
        static_cast<void>(line);
        return false;
    }
}

}  // namespace hmi
