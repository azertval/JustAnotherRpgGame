// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Platform/CrashDump.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <system_error>
#include <utility>

#include <Windows.h>
#include <crtdbg.h>

// dbghelp.h exige Windows.h avant lui.
#include <DbgHelp.h>

#include "HMI/HmiLog.h"

namespace hmi {
namespace {

// Réglages retenus par installCrashDumpWriter. Lus depuis le filtre : aucune allocation n'y a lieu
// avant l'écriture du dump, le tas pouvant être la cause du plantage.
struct CrashDumpSettings {
    std::filesystem::path directory;
    std::string application;
    std::string version;
};

// Pause entre deux essais de la tentative de dernier recours (writeDumpJob), en millisecondes.
constexpr DWORD MINI_DUMP_RETRY_PAUSE_MS = 50;

CrashDumpSettings& settings() {
    static CrashDumpSettings instance;
    return instance;
}

// Erreurs des tentatives de la dernière écriture (writeMiniDump), pour le diagnostic : sans elles,
// seule l'erreur de la dernière tentative est connue, et une panne de CI reste illisible.
std::array<unsigned long, kMiniDumpAttemptCount>& attemptErrors() {
    static std::array<unsigned long, kMiniDumpAttemptCount> instance{};
    return instance;
}

[[noreturn]] void raiseFatalError() {
    RaiseException(kFatalErrorExceptionCode, EXCEPTION_NONCONTINUABLE, 0, nullptr);
    // RaiseException avec EXCEPTION_NONCONTINUABLE ne revient pas ; si un débogueur reprend quand
    // même l'exécution, on ne poursuit pas dans un état incohérent.
    std::abort();
}

void onTerminate() {
    raiseFatalError();
}

void onPureCall() {
    raiseFatalError();
}

void onInvalidParameter(const wchar_t* /*expression*/, const wchar_t* /*function*/,
                        const wchar_t* /*file*/, unsigned int /*line*/, uintptr_t /*reserved*/) {
    raiseFatalError();
}

LONG WINAPI onUnhandledException(EXCEPTION_POINTERS* exception) {
    // Un seul dump : un second plantage pendant l'écriture (tas corrompu) terminerait sinon en
    // boucle, ou écraserait le premier fichier par un dump inutilisable.
    static LONG entered = 0;
    if (InterlockedExchange(&entered, 1) != 0) {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    const CrashDumpSettings& current = settings();
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    const std::filesystem::path path =
        current.directory / crashDumpFileName(current.application, current.version, local);
    const bool written = writeMiniDump(path, exception);

    const unsigned long code =
        exception != nullptr && exception->ExceptionRecord != nullptr
            ? static_cast<unsigned long>(exception->ExceptionRecord->ExceptionCode)
            : 0UL;
    std::array<char, 16> codeText{};
    std::snprintf(codeText.data(), codeText.size(), "0x%08lX", code);
    if (written) {
        HMI_LOG_ERROR(std::string("Plantage (exception ") + codeText.data() +
                      ") : minidump ecrit dans " + path.string());
    } else {
        HMI_LOG_ERROR(std::string("Plantage (exception ") + codeText.data() +
                      ") : minidump NON ecrit (" + path.string() + ")");
    }
    // FileLogSink écrit et vide son tampon à chaque message : la ligne est sur le disque.
    return EXCEPTION_EXECUTE_HANDLER;
}

// Écriture d'un dump confiée à un thread dédié (writeMiniDump).
struct DumpJob {
    HANDLE file = INVALID_HANDLE_VALUE;
    EXCEPTION_POINTERS* original = nullptr;
    DWORD threadId = 0;
    // Copie du contexte réduite à la structure CONTEXT de base (voir writeDumpJob).
    EXCEPTION_RECORD recordCopy{};
    CONTEXT contextCopy{};
    EXCEPTION_POINTERS pointersCopy{};
    // Tentative en cours : si non nul, seul ce thread est consigné (voir writeDumpJob).
    DWORD onlyThread = 0;
    BOOL written = FALSE;
    DWORD error = ERROR_SUCCESS;
    // Code d'erreur de chaque tentative, dans l'ordre : ce que la CI montre quand tout échoue.
    std::array<unsigned long, kMiniDumpAttemptCount> errors{};
};

// Rappel de MiniDumpWriteDump : une zone mémoire illisible est sautée au lieu d'annuler le dump,
// et, sur la dernière tentative, seul le thread du plantage est consigné.
//
// ReadMemoryFailureCallback ne couvre pas tout : « si l'échec a lieu dans une pile, il est
// considéré comme irrécupérable et le minidump échoue » (documentation de
// MINIDUMP_CALLBACK_TYPE). Une pile qu'un thread en cours de fin libère pendant la lecture fait
// donc échouer l'écriture sur ERROR_PARTIAL_COPY quoi qu'on réponde -- vu sur les runners de CI,
// jamais sur le poste. Le seul remède est de ne pas lire cette pile : IncludeThreadCallback rend
// FALSE pour tout thread autre que celui du plantage (`onlyThread`), ce que la dernière tentative
// seule demande. Les autres rappels gardent le comportement par défaut : threads et modules
// inclus, aucune mémoire ajoutée.
BOOL CALLBACK skipUnreadableMemory(PVOID param, const PMINIDUMP_CALLBACK_INPUT input,
                                   PMINIDUMP_CALLBACK_OUTPUT output) {
    if (input == nullptr || output == nullptr) {
        return FALSE;
    }
    const auto* const job = static_cast<const DumpJob*>(param);
    switch (input->CallbackType) {
        case IncludeThreadCallback:
            if (job != nullptr && job->onlyThread != 0 &&
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access): union de l'API Win32.
                input->IncludeThread.ThreadId != job->onlyThread) {
                return FALSE;  // thread exclu : sa pile n'est pas lue, donc pas d'échec dessus
            }
            return TRUE;
        case IncludeModuleCallback:
        case ModuleCallback:
        case ThreadCallback:
        case ThreadExCallback:
            return TRUE;
        case ReadMemoryFailureCallback:
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access): union de l'API Win32.
            output->Status = S_OK;
            return TRUE;
        default:
            return FALSE;
    }
}

DWORD WINAPI writeDumpJob(LPVOID parameter) {
    auto* const job = static_cast<DumpJob*>(parameter);
    // Piles, contexte, modules chargés et déchargés, et la mémoire que les piles désignent : de
    // quoi lire les variables locales et un objet pointé, pour quelques Mio plutôt que tout le tas.
    // NOLINTBEGIN(clang-analyzer-optin.core.EnumCastOutOfRange)
    // MINIDUMP_TYPE est un ensemble de drapeaux : leur union n'est aucun énumérateur, et c'est
    // ainsi que l'API dbghelp la demande.
    const auto rich =
        static_cast<MINIDUMP_TYPE>(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory |
                                   MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules);
    const auto reduced = static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithThreadInfo |
                                                    MiniDumpWithUnloadedModules);
    // NOLINTEND(clang-analyzer-optin.core.EnumCastOutOfRange)

    // Sur une partie des runners de CI (et jamais sur le poste, sans AVX-512), MiniDumpWriteDump
    // refuse le contexte d'exception d'origine : ERROR_INVALID_USER_BUFFER. Le contexte porte alors
    // un état étendu du processeur (CONTEXT_XSTATE) dont la taille dépend de la machine. Les essais
    // suivants passent une copie limitée au CONTEXT de base, puis les pointeurs d'origine lus par
    // ReadProcessMemory (ClientPointers). Un dump sans l'état AVX vaut mieux que pas de dump.
    //
    // La dernière tentative n'écrit que le thread du plantage : une pile illisible (un thread qui
    // se termine pendant la lecture) fait échouer tout le dump, sans rappel possible, et c'est
    // l'échec observé en CI. La pile fautive suffit à lire le plantage ; celles des autres threads
    // sont ce qu'on abandonne en dernier recours.
    //
    // La dernière est réessayée : sur les runners, la même version échoue une fois sur deux, et
    // toujours sur un état que dbghelp lit au mauvais moment (pile libérée, thread en transition).
    // Quelques dizaines de millisecondes plus tard, l'état a changé. Seule la dernière tentative,
    // la plus étroite, est reprise : un plantage ne doit pas attendre douze écritures.
    struct Attempt {
        MINIDUMP_TYPE type;
        EXCEPTION_POINTERS* pointers;
        BOOL clientPointers;
        DWORD onlyThread;
        int tries;
    };
    const std::array<Attempt, kMiniDumpAttemptCount> attempts = {{
        {.type = rich,
         .pointers = job->original,
         .clientPointers = FALSE,
         .onlyThread = 0,
         .tries = 1},
        {.type = reduced,
         .pointers = job->original != nullptr ? &job->pointersCopy : nullptr,
         .clientPointers = FALSE,
         .onlyThread = 0,
         .tries = 1},
        {.type = reduced,
         .pointers = job->original,
         .clientPointers = TRUE,
         .onlyThread = 0,
         .tries = 1},
        {.type = reduced,
         .pointers = job->original != nullptr ? &job->pointersCopy : nullptr,
         .clientPointers = FALSE,
         .onlyThread = job->threadId,
         .tries = 3},
    }};
    MINIDUMP_CALLBACK_INFORMATION callback{};
    callback.CallbackRoutine = &skipUnreadableMemory;
    callback.CallbackParam = job;
    std::size_t index = 0;
    for (const Attempt& attempt : attempts) {
        job->onlyThread = attempt.onlyThread;
        for (int attemptTry = 0; attemptTry < attempt.tries; ++attemptTry) {
            if (attemptTry > 0) {
                Sleep(MINI_DUMP_RETRY_PAUSE_MS);
            }
            LARGE_INTEGER start{};
            SetFilePointerEx(job->file, start, nullptr, FILE_BEGIN);
            SetEndOfFile(job->file);
            MINIDUMP_EXCEPTION_INFORMATION information{};
            information.ThreadId = job->threadId;
            information.ExceptionPointers = attempt.pointers;
            information.ClientPointers = attempt.clientPointers;
            job->written = MiniDumpWriteDump(
                GetCurrentProcess(), GetCurrentProcessId(), job->file, attempt.type,
                attempt.pointers != nullptr ? &information : nullptr, nullptr, &callback);
            if (job->written != FALSE) {
                job->error = ERROR_SUCCESS;
                return 0;
            }
            job->error = GetLastError();
            job->errors.at(index) = job->error;
        }
        ++index;
    }
    return 0;
}

}  // namespace

std::string crashDumpFileName(std::string_view application, std::string_view version,
                              const std::tm& localTime) {
    std::array<char, 32> stamp{};
    std::strftime(stamp.data(), stamp.size(), "%Y%m%d_%H%M%S", &localTime);

    std::string name;
    name.reserve(application.size() + version.size() + 24);
    const auto append = [&name](std::string_view part) {
        for (const char c : part) {
            const bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                              (c >= '0' && c <= '9') || c == '.' || c == '-';
            name.push_back(safe ? c : '_');
        }
    };
    append(application);
    name.push_back('_');
    append(version);
    name.push_back('_');
    name.append(stamp.data());
    name.append(".dmp");
    return name;
}

bool writeMiniDump(const std::filesystem::path& path, _EXCEPTION_POINTERS* exception) {
    std::error_code error;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), error);
    }

    const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DumpJob job;
    job.file = file;
    job.threadId = GetCurrentThreadId();
    job.original = exception;
    if (exception != nullptr && exception->ExceptionRecord != nullptr &&
        exception->ContextRecord != nullptr) {
        job.recordCopy = *exception->ExceptionRecord;
        job.recordCopy.ExceptionRecord = nullptr;  // pas d'exception imbriquée dans la copie
        job.contextCopy = *exception->ContextRecord;
        job.contextCopy.ContextFlags &= ~(CONTEXT_XSTATE & ~CONTEXT_AMD64);
        job.pointersCopy.ExceptionRecord = &job.recordCopy;
        job.pointersCopy.ContextRecord = &job.contextCopy;
    }

    // Sur un thread à part : un thread qui se décrit lui-même pendant que dbghelp suspend et lit
    // les piles donne un dump incomplet ou un échec, selon la configuration -- vu en CI sous le
    // générateur Visual Studio en Debug, jamais sous Ninja. Le thread du plantage attend, sa pile
    // reste lisible. Repli sur un appel direct si le thread ne peut pas être créé.
    const HANDLE worker = CreateThread(nullptr, 0, &writeDumpJob, &job, 0, nullptr);
    if (worker != nullptr) {
        WaitForSingleObject(worker, INFINITE);
        CloseHandle(worker);
    } else {
        writeDumpJob(&job);
    }
    CloseHandle(file);
    attemptErrors() = job.errors;
    if (job.written == FALSE) {
        SetLastError(job.error);
        return false;
    }
    return true;
}

std::array<unsigned long, kMiniDumpAttemptCount> lastMiniDumpAttemptErrors() {
    return attemptErrors();
}

void routeCrtReportsToStderr() {
#ifndef NDEBUG
    // Une assertion de la CRT ou de la bibliotheque standard n'ouvre plus de boite modale : elle
    // s'ecrit sur stderr, ou un programme sans fenetre la lit -- et ou la CI la garde.
    for (const int report : {_CRT_WARN, _CRT_ERROR, _CRT_ASSERT}) {
        _CrtSetReportMode(report, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile(report, _CRTDBG_FILE_STDERR);
    }
#endif
}

void installCrashDumpWriter(std::filesystem::path directory, std::string application,
                            std::string version) {
    CrashDumpSettings& current = settings();
    current.directory = std::move(directory);
    current.application = std::move(application);
    current.version = std::move(version);

    SetUnhandledExceptionFilter(&onUnhandledException);
    std::set_terminate(&onTerminate);
    _set_purecall_handler(&onPureCall);
    _set_invalid_parameter_handler(&onInvalidParameter);
    // abort() ne passe par aucun des chemins ci-dessus : sans ce réglage, la CRT afficherait sa
    // boîte « abort() has been called » en Debug au lieu de laisser std::terminate conclure.
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    // Une assertion Debug ne doit pas bloquer un programme sans fenetre sur une boite modale.
    routeCrtReportsToStderr();
}

void triggerCrashForTest() {
    // Une violation d'accès LEVÉE, et non provoquée : même code de sortie et même chemin par le
    // filtre qu'un vrai déréférencement nul, sans comportement indéfini que l'optimiseur pourrait
    // retirer ni que les analyseurs statiques signaleraient à raison.
    RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, 0, nullptr);
    std::abort();
}

}  // namespace hmi
