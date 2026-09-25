// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Platform/ExecutableDirectory.h"

#include <array>
#include <utility>

#include <Windows.h>

namespace hmi {

namespace {

// Le dossier de contenu impose, vide tant que personne ne l'a pose.
std::filesystem::path& dossierImpose() {
    static std::filesystem::path dossier;
    return dossier;
}

}  // namespace

std::filesystem::path dataDirectory() {
    return dossierImpose().empty() ? executableDirectory() : dossierImpose();
}

void setDataDirectory(std::filesystem::path directory) {
    dossierImpose() = std::move(directory);
}

std::filesystem::path executableDirectory() {
    std::array<wchar_t, MAX_PATH> buffer{};
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), MAX_PATH);
    return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
}

}  // namespace hmi
