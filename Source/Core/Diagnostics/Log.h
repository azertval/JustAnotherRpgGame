// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include "Core/Diagnostics/LogFormat.h"
#include "Core/Diagnostics/LogLevel.h"
#include "Core/Diagnostics/Logger.h"

/**
 * @file Core/Diagnostics/Log.h
 * @brief Macros de journalisation génériques (catégorie, niveau, fichier/ligne, horodatage).
 *
 * En pratique, chaque module définit ses propres macros courtes liées à sa
 * catégorie (« sous-dossier » de log), par exemple `HMI/HmiLog.h`. Ces macros
 * génériques restent utilisables directement en précisant la catégorie.
 */

/// Journalise un message pour une catégorie et un niveau donnés, via le journaliseur global.
#define JADG_LOG(category, level, message)                                                       \
    do {                                                                                         \
        ::core::Logger& jadgLogger = ::core::defaultLogger();                                    \
        if (jadgLogger.isEnabled(level)) {                                                       \
            jadgLogger.log(level, ::core::formatLogLine(::core::currentTimestamp(), level,       \
                                                        category, __FILE__, __LINE__, message)); \
        }                                                                                        \
    } while (false)

#define JADG_LOG_TRACE(category, message) JADG_LOG(category, ::core::LogLevel::Trace, message)
#define JADG_LOG_INFO(category, message) JADG_LOG(category, ::core::LogLevel::Info, message)
#define JADG_LOG_WARNING(category, message) JADG_LOG(category, ::core::LogLevel::Warning, message)
#define JADG_LOG_ERROR(category, message) JADG_LOG(category, ::core::LogLevel::Error, message)
