// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/MapDocuments.h"

#include <algorithm>

namespace hmi {

std::string documentLabel(std::string_view mapId, bool dirty) {
    const std::size_t slash = mapId.rfind('/');
    std::string label{slash == std::string_view::npos ? mapId : mapId.substr(slash + 1)};
    if (label.empty()) {
        label = "untitled";
    }
    if (dirty) {
        label += " *";
    }
    return label;
}

std::optional<std::size_t> documentOf(const std::vector<OpenDocument>& documents,
                                      std::string_view mapId) {
    if (mapId.empty()) {
        return std::nullopt;  // une carte neuve n'est jamais « celle qui est déjà ouverte ».
    }
    const auto found = std::ranges::find(documents, mapId, &OpenDocument::mapId);
    return found != documents.end()
               ? std::optional<std::size_t>{static_cast<std::size_t>(found - documents.begin())}
               : std::nullopt;
}

std::optional<std::size_t> documentAfterClose(std::size_t count, std::size_t closed) {
    if (count <= 1 || closed >= count) {
        return std::nullopt;
    }
    const std::size_t remaining = count - 1;
    return closed < remaining ? closed : remaining - 1;
}

std::vector<std::string> dirtyDocuments(const std::vector<OpenDocument>& documents) {
    std::vector<std::string> dirty;
    for (const OpenDocument& document : documents) {
        if (document.dirty) {
            dirty.push_back(document.mapId);
        }
    }
    return dirty;
}

}  // namespace hmi
