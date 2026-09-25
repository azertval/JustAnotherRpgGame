// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file HMI/Game/FigureResolver.h
 * @brief Quelle bande dessiner pour ce personnage ? La figurine nommée si elle existe, sinon le
 *        mannequin de sa silhouette (`LOT-316`).
 *
 * ## La règle, en trois temps
 *
 * 1. La figurine nommée — la propriété `figure` d'un PNJ, le slug d'une créature, la figurine du
 *    héros — se cherche par la table du lieu (`hmi::PlaceAppearance::figureDirectory`, `LOT-124`).
 *    Si sa bande de repos existe (`idle-se.png` : elle est orientée ; à défaut `idle.png`), c'est
 *    elle.
 * 2. Sinon, le mannequin de sa silhouette (`hmi::placeholderFigureDirectory`), s'il est installé.
 * 3. Sinon le mannequin humanoïde ; et s'il manque lui aussi, la figurine nommée telle quelle : le
 *    rendu lui donnera son marqueur d'asset, comme avant ce lot.
 *
 * Le disque ne se relit pas à chaque image : la réponse se retient par figurine, et s'oublie
 * quand la carte change (`clear`) — une figurine installée pendant la partie se verra à la carte
 * suivante, pas à l'image suivante, et c'est un prix acceptable pour ne pas toucher au disque
 * soixante fois par seconde.
 */

#include <filesystem>
#include <map>
#include <string>
#include <string_view>

#include "HMI/Graphics/PlaceAppearance.h"

namespace hmi {

/// @brief Ce que le résolveur a trouvé pour une figurine.
struct ResolvedFigure {
    /// Le dossier de la figurine, relatif au dossier des assets : ce que `WorldFigureSnapshot`
    /// nomme (un chemin, jamais un slug — la table du lieu a déjà répondu).
    std::string directory;
    /// Vrai si la figurine a ses bandes orientées (`idle-se.png`…).
    bool oriented = false;
    /// Vrai si c'est un mannequin qui tient la place de la figurine nommée.
    bool placeholder = false;

    [[nodiscard]] bool operator==(const ResolvedFigure&) const = default;
};

class FigureResolver {
public:
    explicit FigureResolver(std::filesystem::path assetsDirectory);

    /**
     * @brief Résout @p figure (slug ou dossier ; vide : rien de nommé) pour la silhouette
     *        @p silhouette (vide : humanoïde), par la table @p appearance.
     */
    [[nodiscard]] const ResolvedFigure& resolve(std::string_view figure,
                                                std::string_view silhouette,
                                                const PlaceAppearance& appearance);

    /// @brief Oublie ce qui a été trouvé : la carte, ou le lieu, a changé.
    void clear() noexcept {
        _found.clear();
    }

    /// @return Vrai si la bande de repos de @p directory existe, orientée ou non.
    [[nodiscard]] bool hasIdleStrip(std::string_view directory, bool& oriented) const;

private:
    std::filesystem::path _assetsDirectory;
    /// Clé : `<figure>|<silhouette>`.
    std::map<std::string, ResolvedFigure, std::less<>> _found;
};

}  // namespace hmi
