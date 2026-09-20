// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/DiskGuard.h"

#include <array>
#include <fstream>

namespace hmi {

namespace {

// FNV-1a 64 bits : un condensé de détection, pas de sécurité. Une collision ferait manquer un
// changement ; sur des cartes écrites à la main, la probabilité est négligeable.
constexpr std::uint64_t FNV_OFFSET = 14695981039346656037ULL;
constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;

constexpr std::uint64_t mix(std::uint64_t hash, std::string_view bytes) noexcept {
    for (const char byte : bytes) {
        hash ^= static_cast<std::uint8_t>(byte);
        hash *= FNV_PRIME;
    }
    return hash;
}

}  // namespace

FileFingerprint fingerprintOf(std::string_view content) noexcept {
    return FileFingerprint{
        .exists = true, .size = content.size(), .hash = mix(FNV_OFFSET, content)};
}

FileFingerprint fingerprintFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    FileFingerprint fingerprint{.exists = true, .size = 0, .hash = FNV_OFFSET};
    std::array<char, std::size_t{64} * 1024> buffer{};
    while (file) {
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = static_cast<std::size_t>(file.gcount());
        fingerprint.hash = mix(fingerprint.hash, std::string_view(buffer.data(), count));
        fingerprint.size += count;
    }
    return fingerprint;
}

DiskChange compareFingerprints(const FileFingerprint& known,
                               const FileFingerprint& current) noexcept {
    if (known == current) {
        return DiskChange::None;
    }
    return current.exists ? DiskChange::Modified : DiskChange::Deleted;
}

DiskReaction reactToDiskChange(DiskChange change, bool draftModified) noexcept {
    switch (change) {
        case DiskChange::None:
            return DiskReaction::Ignore;
        case DiskChange::Modified:
            return draftModified ? DiskReaction::AskReloadOrKeep : DiskReaction::ReloadQuietly;
        case DiskChange::Deleted:
            return DiskReaction::WarnDeleted;
    }
    return DiskReaction::Ignore;
}

}  // namespace hmi
