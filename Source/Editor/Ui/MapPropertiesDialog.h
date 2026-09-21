// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QString>
#include <optional>
#include <string>
#include <vector>

#include "Editor/Logic/EditorSidecar.h"

/**
 * @file Editor/Ui/MapPropertiesDialog.h
 * @brief Le dialogue **« Map properties… »** (`LOT-EDITOR-09`, `EX-EDIT-091`) : le lieu d'une
 *        carte, sa région, son ambiance et où elle en est.
 */

class QWidget;

namespace hmi {

/// @brief Ce que le dialogue montre et rend.
struct MapPropertiesChoice {
    /// La région du monde (`central-empire`), vide si la carte n'en déclare pas.
    std::string region;
    /// L'ambiance, telle que le `LOT-28` la jouera ; vide si la carte n'en déclare pas.
    std::string ambience;
    /// Où en est la carte : une note d'auteur, gardée dans l'annexe.
    MapState state = MapState::Unset;

    [[nodiscard]] bool operator==(const MapPropertiesChoice&) const = default;
};

/**
 * @brief Montre les propriétés de la carte et rend ce que l'auteur a choisi.
 * @param parent  Fenêtre parente.
 * @param mapId   L'identifiant de la carte, affiché.
 * @param place   Le **lieu** de la carte (sa planche) : montré, jamais édité ici — en changer
 *                repeint la carte, et c'est *Map* › *Change sheet…* (`LOT-EDITOR-14`).
 * @param regions Les régions connues du monde, proposées au choix ; une région hors liste reste
 *                saisissable.
 * @param current Les valeurs de départ.
 * @return Le choix, ou `std::nullopt` si l'auteur a renoncé.
 */
[[nodiscard]] std::optional<MapPropertiesChoice> askMapProperties(
    QWidget* parent, const QString& mapId, const QString& place,
    const std::vector<std::string>& regions, const MapPropertiesChoice& current);

}  // namespace hmi
