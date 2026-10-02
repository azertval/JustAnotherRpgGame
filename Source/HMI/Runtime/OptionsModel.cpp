// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Runtime/OptionsModel.h"

#include <QSettings>
#include <algorithm>
#include <array>
#include <ctime>
#include <filesystem>
#include <fstream>

#include "Core/Diagnostics/LogLevel.h"
#include "Core/Diagnostics/MemoryLogSink.h"
#include "HMI/HmiLog.h"
#include "HMI/Platform/ExecutableDirectory.h"

namespace hmi {
namespace {

// Mêmes clés que le châssis historique : un réglage posé par l'ancienne page Options doit être
// retrouvé par la nouvelle. Les renommer aurait réinitialisé en silence les préférences de qui
// jouait avant la refonte — une perte qu'aucun message n'annonce.
constexpr const char* FULLSCREEN_KEY = "fullscreen";
constexpr const char* VSYNC_KEY = "vsync";
constexpr const char* DIAGNOSTICS_KEY = "diagnostics_overlay";
constexpr const char* VOLUME_KEY = "volume";
constexpr const char* HUD_SCALE_KEY = "hud_scale_percent";
constexpr const char* LANGUAGE_KEY = "language";

// Les valeurs d'usine : lues par le constructeur quand aucun réglage n'est enregistré, et publiées
// telles quelles par defaults() pour le bouton « Par défaut » des options.
constexpr bool DEFAULT_FULLSCREEN = false;
constexpr bool DEFAULT_VSYNC = true;
constexpr bool DEFAULT_DIAGNOSTICS = false;
constexpr int DEFAULT_VOLUME = 100;
constexpr int DEFAULT_HUD_SCALE = 100;
constexpr const char* DEFAULT_LANGUAGE = "fr";

[[nodiscard]] QSettings settings() {
    return QSettings();
}

}  // namespace

OptionsModel::OptionsModel(QObject* parent) : QObject(parent) {
    QSettings stored = settings();
    _fullscreen = stored.value(QLatin1String(FULLSCREEN_KEY), DEFAULT_FULLSCREEN).toBool();
    _vsync = stored.value(QLatin1String(VSYNC_KEY), DEFAULT_VSYNC).toBool();
    _diagnostics = stored.value(QLatin1String(DIAGNOSTICS_KEY), DEFAULT_DIAGNOSTICS).toBool();
    _volume = std::clamp(stored.value(QLatin1String(VOLUME_KEY), DEFAULT_VOLUME).toInt(), 0, 100);
    _hudScalePercent =
        std::clamp(stored.value(QLatin1String(HUD_SCALE_KEY), DEFAULT_HUD_SCALE).toInt(), 75, 130);
    _language =
        stored.value(QLatin1String(LANGUAGE_KEY), QLatin1String(DEFAULT_LANGUAGE)).toString();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static) : lue par QML (Q_PROPERTY).
QVariantMap OptionsModel::defaults() const {
    return {{QStringLiteral("fullscreen"), DEFAULT_FULLSCREEN},
            {QStringLiteral("vsync"), DEFAULT_VSYNC},
            {QStringLiteral("diagnostics"), DEFAULT_DIAGNOSTICS},
            {QStringLiteral("volume"), DEFAULT_VOLUME},
            {QStringLiteral("hudScalePercent"), DEFAULT_HUD_SCALE},
            {QStringLiteral("language"), QLatin1String(DEFAULT_LANGUAGE)}};
}

void OptionsModel::setSessionLog(core::MemoryLogSink* sessionLog) noexcept {
    _sessionLog = sessionLog;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static) : lue par QML (Q_PROPERTY).
QStringList OptionsModel::languages() const {
    return {QStringLiteral("fr"), QStringLiteral("en")};
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static) : lue par QML (Q_PROPERTY).
QStringList OptionsModel::languageNames() const {
    // Chaque langue est nommee DANS SA PROPRE LANGUE, et jamais traduite. Un joueur qui a mis le
    // jeu dans une langue qu'il ne lit pas doit pouvoir en sortir : c'est le seul endroit de
    // l'interface ou l'on ne peut pas compter sur la langue courante pour se faire comprendre.
    return {QStringLiteral("Français"), QStringLiteral("English")};
}

void OptionsModel::setFullscreen(bool enabled) {
    if (_fullscreen == enabled) {
        return;
    }
    _fullscreen = enabled;
    settings().setValue(QLatin1String(FULLSCREEN_KEY), enabled);
    emit fullscreenChanged();
}

void OptionsModel::setVsync(bool enabled) {
    if (_vsync == enabled) {
        return;
    }
    _vsync = enabled;
    settings().setValue(QLatin1String(VSYNC_KEY), enabled);
    emit vsyncChanged();
}

void OptionsModel::setDiagnostics(bool enabled) {
    if (_diagnostics == enabled) {
        return;
    }
    _diagnostics = enabled;
    settings().setValue(QLatin1String(DIAGNOSTICS_KEY), enabled);
    emit diagnosticsChanged();
}

void OptionsModel::setVolume(int percent) {
    const int clamped = std::clamp(percent, 0, 100);
    if (_volume == clamped) {
        return;
    }
    _volume = clamped;
    settings().setValue(QLatin1String(VOLUME_KEY), clamped);
    emit volumeChanged();
}

void OptionsModel::setLanguage(const QString& code) {
    if (_language == code || !languages().contains(code)) {
        return;
    }
    _language = code;
    settings().setValue(QLatin1String(LANGUAGE_KEY), code);
    emit languageChanged();
}

void OptionsModel::setHudScalePercent(int percent) {
    const int clamped = std::clamp(percent, 75, 130);
    if (_hudScalePercent == clamped) {
        return;
    }
    _hudScalePercent = clamped;
    settings().setValue(QLatin1String(HUD_SCALE_KEY), clamped);
    emit hudScaleChanged();
}

QString OptionsModel::saveLogs() {
    if (_sessionLog == nullptr) {
        return tr("Journaux indisponibles (build Release).");
    }

    // Nom horodaté, à côté de l'exécutable : un export n'écrase jamais le précédent, et deux
    // rapports de défaut successifs restent lisibles côte à côte.
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    std::array<char, 32> stamp{};
    std::strftime(stamp.data(), stamp.size(), "%Y%m%d_%H%M%S", &local);

    const std::filesystem::path directory = executableDirectory() / "Logs";
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    const std::filesystem::path path =
        directory / (std::string("session_") + stamp.data() + ".log");

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        HMI_LOG_ERROR("Export des journaux : ecriture impossible dans " + path.string() + ".");
        return tr("Échec de l'enregistrement des journaux.");
    }
    // Le NIVEAU est ecrit devant chaque ligne : un journal exporte sert a un rapport de defaut,
    // et distinguer un avertissement d'une trace y decide de ce qu'on lit en premier.
    for (const core::MemoryLogSink::Entry& entry : _sessionLog->entries()) {
        file << '[' << core::toString(entry.level) << "] " << entry.message << '\n';
    }
    if (!file) {
        return tr("Échec de l'enregistrement des journaux.");
    }
    HMI_LOG_INFO("Journaux de session exportes : " + path.string() + ".");
    return tr("Journaux enregistrés : %1").arg(QString::fromStdString(path.string()));
}

}  // namespace hmi
