// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>
#include <cstddef>
#include <ctime>
#include <filesystem>
#include <string>
#include <string_view>

/**
 * @file HMI/Platform/CrashDump.h
 * @brief Minidump écrit quand le programme plante (refonte de la chaîne d'outillage, phase 4).
 *
 * Un plantage vu en jouant ne laissait qu'un journal coupé net. Le minidump garde la pile de chaque
 * thread et le contexte de l'exception ; ouvert dans Visual Studio avec les symboles de la même
 * version (l'archive `<nom>-symbols.zip` de la release), il montre la ligne fautive. Rien n'est
 * envoyé nulle part : le fichier reste dans `Crashes/`, à côté de l'exécutable, comme `Logs/`.
 */

struct _EXCEPTION_POINTERS;

namespace hmi {

/// Code de l'exception levée pour transformer une fin anormale (std::terminate, appel virtuel pur,
/// paramètre invalide de la CRT) en exception structurée, que le filtre sait écrire : « JADG ».
inline constexpr unsigned long kFatalErrorExceptionCode = 0xE04A4447UL;

/// Nombre de tentatives d'écriture d'un minidump, de la plus riche à la plus réduite
/// (writeMiniDump).
inline constexpr std::size_t kMiniDumpAttemptCount = 4;

/**
 * @brief Nom du fichier d'un minidump : `<application>_<version>_<AAAAMMJJ_HHMMSS>.dmp`.
 *
 * La version y figure pour retrouver sans ouvrir le fichier l'archive de symboles qui le lit. Tout
 * caractère hors `[A-Za-z0-9.-]` devient `_`, pour qu'une version comme `0.1.0+dev` reste un nom de
 * fichier sûr.
 *
 * @param application Nom de l'application (`JustAnotherRpgGame`, `LevelEditor`).
 * @param version     Version du binaire (`core::Engine::version()`).
 * @param localTime   Heure locale du plantage.
 * @return Le nom du fichier, sans dossier.
 */
[[nodiscard]] std::string crashDumpFileName(std::string_view application, std::string_view version,
                                            const std::tm& localTime);

/**
 * @brief Écrit un minidump du processus courant.
 *
 * Le dossier parent est créé au besoin. Sans @p exception, le dump porte l'état de tous les threads
 * au moment de l'appel, sans contexte d'exception. L'écriture a lieu sur un thread dédié, que
 * l'appelant attend. Si le dump riche (mémoire référencée par les piles) échoue, des dumps réduits
 * sont tentés, qui gardent piles et contexte (sans l'état étendu du processeur) ; la dernière
 * tentative n'écrit plus que le thread du plantage, une pile illisible d'un autre thread faisant
 * échouer tout le dump. Si toutes échouent, `GetLastError()` en donne la raison.
 *
 * @param path      Chemin du fichier `.dmp` à écrire (écrasé s'il existe).
 * @param exception Contexte de l'exception à consigner, ou `nullptr`.
 * @return `true` si le fichier a été écrit.
 */
bool writeMiniDump(const std::filesystem::path& path, _EXCEPTION_POINTERS* exception);

/**
 * @brief Erreurs des tentatives du dernier writeMiniDump, dans l'ordre (0 = tentative non faite).
 *
 * `GetLastError()` ne rend que la raison de la dernière tentative : une panne comme celle vue en CI
 * reste alors indéchiffrable. Diagnostic seul ; le test de minidump les affiche quand il échoue.
 *
 * @return Une copie du relevé, tentative par tentative.
 */
[[nodiscard]] std::array<unsigned long, kMiniDumpAttemptCount> lastMiniDumpAttemptErrors();

/**
 * @brief Installe l'écriture d'un minidump sur toute fin anormale du processus.
 *
 * Couvre l'exception structurée non attrapée (violation d'accès, division par zéro…), et, en les
 * convertissant en exception `kFatalErrorExceptionCode`, `std::terminate` (exception C++ non
 * attrapée), l'appel d'une fonction virtuelle pure et le paramètre invalide passé à la CRT. Le
 * processus se termine ensuite, sans la boîte de dialogue du rapport d'erreurs Windows.
 *
 * À appeler une fois, au plus tôt dans `main`, après l'installation du journal : le chemin du dump
 * y est consigné avant la sortie.
 *
 * @param directory   Dossier où écrire les dumps (créé au premier plantage).
 * @param application Nom de l'application, repris dans le nom du fichier.
 * @param version     Version du binaire, reprise dans le nom du fichier.
 */
void installCrashDumpWriter(std::filesystem::path directory, std::string application,
                            std::string version);

/**
 * @brief Déclenche volontairement une violation d'accès, pour éprouver installCrashDumpWriter.
 *
 * Utilisé par l'option `--crash-test` du jeu, que le test de fumée de la release lance pour prouver
 * que l'archive livrée écrit bien son dump.
 */
[[noreturn]] void triggerCrashForTest();

}  // namespace hmi
