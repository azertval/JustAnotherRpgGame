// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file Editor/Logic/MapDocuments.h
 * @brief Les **cartes ouvertes en onglets** (`LOT-EDITOR-09`, `EX-EDIT-088`) : ce que dit chaque
 *        onglet, laquelle une ouverture vise, et laquelle revient après une fermeture. Logique
 *        pure, sans Qt.
 *
 * Chaque onglet porte son propre brouillon, son historique et sa sauvegarde automatique ; la
 * fenêtre ne sait ici que les nommer et les ordonner.
 */

namespace hmi {

/// @brief Une carte ouverte : ce que la barre d'onglets en connaît.
struct OpenDocument {
    /// Son identifiant (`capital/martpart`) ; vide pour une carte neuve jamais enregistrée.
    std::string mapId;
    /// Son brouillon porte des modifications.
    bool dirty = false;

    [[nodiscard]] bool operator==(const OpenDocument&) const = default;
};

/**
 * @brief Le libellé d'un onglet : le dernier segment de l'identifiant, suivi d'une étoile si le
 *        brouillon est modifié (`martpart *`).
 *
 * Le dernier segment seul : `capital/martpart` et `capital/arenarea` tiennent côte à côte, et le
 * chemin complet reste dans l'infobulle et dans le titre de la fenêtre.
 */
[[nodiscard]] std::string documentLabel(std::string_view mapId, bool dirty);

/// @return Le rang de la carte @p mapId parmi @p documents, `std::nullopt` si elle n'est pas
///         ouverte : ouvrir une carte déjà ouverte y revient au lieu de l'ouvrir deux fois.
[[nodiscard]] std::optional<std::size_t> documentOf(const std::vector<OpenDocument>& documents,
                                                    std::string_view mapId);

/**
 * @brief Le rang qui devient actif quand on ferme l'onglet @p closed parmi @p count.
 *
 * Celui de droite prend la place — c'est le rang fermé lui-même, la liste ayant reculé ; le
 * dernier onglet laisse la main à son voisin de gauche.
 * @return `std::nullopt` s'il ne reste aucun onglet.
 */
[[nodiscard]] std::optional<std::size_t> documentAfterClose(std::size_t count, std::size_t closed);

/// @return Les identifiants des cartes modifiées de @p documents, dans l'ordre des onglets : ce que
///         la fermeture de la fenêtre demande quoi faire.
[[nodiscard]] std::vector<std::string> dirtyDocuments(const std::vector<OpenDocument>& documents);

}  // namespace hmi
