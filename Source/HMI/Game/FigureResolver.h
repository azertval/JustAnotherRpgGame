// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file HMI/Game/FigureResolver.h
 * @brief Que dessiner pour ce personnage ? La figurine nommée si elle existe, sinon le mannequin
 *        de sa silhouette (`LOT-145`).
 *
 * ## La règle, en trois temps
 *
 * 1. La figurine nommée — la propriété `figure` d'un PNJ, le slug d'une créature, la figurine du
 *    héros — se cherche par la table du lieu (`hmi::PlaceAppearance::figureDirectory`, `LOT-124`).
 *    Si elle existe, c'est elle.
 * 2. Sinon, le mannequin de sa silhouette, s'il est installé.
 * 3. Sinon le mannequin humanoïde ; et s'il manque lui aussi, la figurine nommée telle quelle : le
 *    rendu lui donnera son marqueur d'asset, comme avant ce lot.
 *
 * ## Deux formes de figurine, le temps d'un lot (`LOT-1005`)
 *
 * Une figurine **existe** sous l'une de deux formes :
 *
 * - un **modèle** : son dossier porte une fiche (`character.json`, `core::CharacterSheetFile`)
 *   qui nomme un `.glb` présent et le squelette auquel il est lié. C'est la forme cherchée
 *   d'abord, pour la figurine nommée (dans son dossier) comme pour le mannequin
 *   (`hmi::mannequinFigureDirectory`) ;
 * - des **bandes** : sa bande de repos existe (`idle-se.png` : elle est orientée ; à défaut
 *   `idle.png`), dans son dossier ou dans celui du mannequin en bandes
 *   (`hmi::placeholderFigureDirectory`). Le `LOT-1006` retire cette forme.
 *
 * Le disque ne se relit pas à chaque image : la réponse se retient par figurine, et s'oublie
 * quand la carte change (`clear`) — une figurine installée pendant la partie se verra à la carte
 * suivante, pas à l'image suivante, et c'est un prix acceptable pour ne pas toucher au disque
 * soixante fois par seconde.
 */

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "Core/Resources/SkeletonFile.h"
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
    /// Le modèle de la figurine (`LOT-1005`) : le chemin de son `.glb`, relatif au dossier des
    /// assets. Vide : elle se dessine par ses bandes.
    std::string model;
    /// Ce que déclare le squelette du modèle — durée, boucle et image clé de ses clips ; nul pour
    /// des bandes, ou si la description ne se lit pas.
    std::shared_ptr<const core::SkeletonDescription> skeleton;

    [[nodiscard]] bool operator==(const ResolvedFigure&) const = default;
};

/**
 * @brief Résout la figurine à dessiner pour un personnage, et retient la réponse le temps d'une
 *        carte (`LOT-145`).
 *
 * Applique la règle en trois temps de l'en-tête : la figurine nommée si sa bande de repos existe,
 * sinon le mannequin de sa silhouette, sinon le mannequin humanoïde. Le disque n'est lu qu'à la
 * première demande par figurine ; `clear` oublie tout quand la carte change.
 */
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
        _skeletons.clear();
    }

    /**
     * @brief Le modèle du dossier @p directory, s'il porte une fiche lisible qui nomme un `.glb`
     *        présent (`LOT-1005`).
     * @param directory Le dossier du personnage, relatif au dossier des assets.
     * @param resolved Reçoit le modèle et son squelette ; inchangé sinon.
     * @return Vrai si le dossier est celui d'un personnage en modèle.
     */
    [[nodiscard]] bool hasModel(std::string_view directory, ResolvedFigure& resolved);

    /// @return Vrai si la bande de repos de @p directory existe, orientée ou non.
    [[nodiscard]] bool hasIdleStrip(std::string_view directory, bool& oriented) const;

private:
    std::filesystem::path _assetsDirectory;
    /// Clé : `<figure>|<silhouette>`.
    std::map<std::string, ResolvedFigure, std::less<>> _found;
    /// Les descriptions de squelette déjà lues, par silhouette ; nulle pour une qui ne se lit pas.
    std::map<std::string, std::shared_ptr<const core::SkeletonDescription>, std::less<>> _skeletons;
};

}  // namespace hmi
