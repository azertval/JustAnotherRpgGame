// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "App/Common/Bootstrap.h"

#include <QCoreApplication>
#include <QLibraryInfo>
#include <QString>
#include <QTranslator>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <memory>
#include <string>

#include "Core/BuildConfig.h"
#include "Core/Core.h"
#include "Core/Diagnostics/ConsoleLogSink.h"
#include "Core/Diagnostics/FileLogSink.h"
#include "Core/Diagnostics/LogLevel.h"
#include "Core/Diagnostics/LogLevelParse.h"
#include "Core/Diagnostics/Logger.h"
#include "Core/Diagnostics/MemoryLogSink.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/CrashDump.h"
#include "HMI/Platform/ExecutableDirectory.h"

namespace app {
namespace {

// Rend : La valeur d'une variable d'environnement, ou `std::nullopt` si absente.
[[nodiscard]] std::optional<std::string> environmentVariable(const char* name) {
    std::size_t length = 0;
    if (getenv_s(&length, nullptr, 0, name) != 0 || length == 0) {
        return std::nullopt;
    }
    std::string value(length, '\0');
    if (getenv_s(&length, value.data(), length, name) != 0) {
        return std::nullopt;
    }
    if (!value.empty() && value.back() == '\0') {
        value.pop_back();  // getenv_s inclut le terminateur nul dans la longueur
    }
    return value;
}

// Chemin d'un fichier de log horodaté pour la session en cours, à côté de l'exécutable.
//
// Un nom distinct par lancement (dossier `Logs/` créé au besoin par `FileLogSink`) : un crash
// n'écrase jamais les preuves du lancement précédent, contrairement à un nom de fichier fixe.
[[nodiscard]] std::filesystem::path timestampedLogFilePath() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    std::array<char, 32> stamp{};
    std::strftime(stamp.data(), stamp.size(), "%Y%m%d_%H%M%S", &local);
    std::string fileName = "run_";
    fileName += stamp.data();
    fileName += ".log";
    return hmi::executableDirectory() / "Logs" / fileName;
}

// Rend : Le niveau minimum retenu ; `invalidValueGiven` signale une valeur non reconnue.
[[nodiscard]] core::LogLevel resolveMinimumLogLevel(int argc, char** argv,
                                                    bool& invalidValueGiven) {
    core::LogLevel level = core::LogLevel::Trace;
    invalidValueGiven = false;

    if (const std::optional<std::string> fromEnvironment = environmentVariable("JADG_LOG_LEVEL")) {
        if (const std::optional<core::LogLevel> parsed = core::parseLogLevel(*fromEnvironment)) {
            level = *parsed;
        } else {
            invalidValueGiven = true;
        }
    }
    if (const std::optional<std::string_view> fromCommandLine =
            commandLineOption(argc, argv, "--log-level=")) {
        if (const std::optional<core::LogLevel> parsed =
                core::parseLogLevel(std::string(*fromCommandLine))) {
            level = *parsed;
        } else {
            invalidValueGiven = true;
        }
    }
    return level;
}

}  // namespace

std::optional<std::string_view> commandLineOption(int argc, char** argv, std::string_view name) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];
        if (argument.starts_with(name)) {
            return argument.substr(name.size());
        }
    }
    return std::nullopt;
}

core::MemoryLogSink* installLogging(int argc, char** argv, std::string_view applicationName,
                                    CrashTest crashTest) {
    core::MemoryLogSink* sessionLog = nullptr;
    if constexpr (core::DEVELOPER_BUILD) {
        core::defaultLogger().addSink(std::make_unique<core::ConsoleLogSink>());
        auto memorySink = std::make_unique<core::MemoryLogSink>();
        sessionLog = memorySink.get();
        core::defaultLogger().addSink(std::move(memorySink));
    }
    core::defaultLogger().addSink(std::make_unique<core::FileLogSink>(timestampedLogFilePath()));

    bool invalidLogLevel = false;
    const core::LogLevel logLevel = resolveMinimumLogLevel(argc, argv, invalidLogLevel);
    core::defaultLogger().setMinimumLevel(logLevel);

    const std::string name(applicationName);
    HMI_LOG_INFO("Demarrage de " + name + " (niveau de log : " + core::toString(logLevel) + ").");
    if (invalidLogLevel) {
        HMI_LOG_WARNING("Niveau de log fourni invalide : valeur ignoree.");
    }
    // Version du binaire et de Qt contre lequel il a ete compile (QT_VERSION_STR, fourni par les
    // en-tetes Qt) : capture dans le journal de session pour qu'un rapport de defaut dise
    // contre quel Qt le binaire signale a ete construit, sans dependre d'une reproduction locale.
    HMI_LOG_INFO(name + " " + core::Engine::version() + " (compile avec Qt " + QT_VERSION_STR +
                 ").");

    // Minidump sur plantage (phase 4 de la refonte de l'outillage), a cote des journaux : un
    // plantage vu en jouant se lit ensuite dans Visual Studio avec l'archive de symboles de la
    // version. Installe APRES le journal, qui consigne le chemin du dump.
    hmi::installCrashDumpWriter(hmi::executableDirectory() / "Crashes", name,
                                core::Engine::version());
    // Plantage volontaire : le test de fumee de la release prouve ainsi que l'archive livree ecrit
    // son dump, ce qu'aucun test en processus ne peut montrer (filtre installe, DbgHelp deploye).
    if (crashTest == CrashTest::AtStartup && commandLineOption(argc, argv, "--crash-test")) {
        HMI_LOG_WARNING("--crash-test : plantage volontaire.");
        hmi::triggerCrashForTest();
    }
    return sessionLog;
}

void installQtTranslations(std::string_view language) {
    // Duree de vie : le traducteur doit survivre a l'appel, QCoreApplication ne le possede pas.
    static QTranslator qtTranslator;
    const QString catalog =
        QStringLiteral("qtbase_") +
        QString::fromUtf8(language.data(), static_cast<qsizetype>(language.size()));
    // Le dossier depose a cote de l'executable D'ABORD (c'est celui d'une installation livree),
    // l'installation Qt de developpement ensuite.
    const QString deployed =
        QString::fromStdString((hmi::executableDirectory() / "Translations").string());
    if (qtTranslator.load(catalog, deployed) ||
        qtTranslator.load(catalog, QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        QCoreApplication::installTranslator(&qtTranslator);
        return;
    }
    HMI_LOG_INFO("Traductions de Qt indisponibles pour la langue '" + std::string(language) +
                 "' : les boutons standard resteront en anglais.");
}

}  // namespace app
