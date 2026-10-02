// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file HMI/Game/FigureResolver.h
 * @brief Que dessiner pour ce personnage ? La figurine nommée si elle existe, sinon le mannequin
 *        de sa silhouette (`LOT-145`, `EX-REN-051`).
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
 * ## Ce qu'est une figurine (`LOT-1006`)
 *
 * Une figurine **existe** si son dossier porte une fiche (`character.json`,
 * `core::CharacterSheetFile`) qui nomme un `.glb` présent et le squelette auquel il est lié : c'est
 * un **modèle**, et le résolveur ne connaît pas d'autre forme. Le mannequin d'une silhouette en est
 * un aussi (`hmi::mannequinFigureDirectory`).
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
    /// Vrai si c'est un mannequin qui tient la place de la figurine nommée.
    bool placeholder = false;
    /// Le dossier de la figurine **nommée**, même quand un mannequin tient sa place : son portrait
    /// et son jeton y sont (`portrait.png`, `token.png`). Vide si rien n'est nommé.
    std::string named;
    /// Le modèle de la figurine (`LOT-1005`) : le chemin de son `.glb`, relatif au dossier des
    /// assets. Vide : rien n'est installé, elle se dessine par son marqueur.
    std::string model;
    /// Ce que déclare le squelette du modèle — durée, boucle et image clé de ses clips ; nul sans
    /// modèle, ou si la description ne se lit pas.
    std::shared_ptr<const core::SkeletonDescription> skeleton;

    [[nodiscard]] bool operator==(const ResolvedFigure&) const = default;
};

/**
 * @brief Résout la figurine à dessiner pour un personnage, et retient la réponse le temps d'une
 *        carte (`LOT-145`).
 *
 * Applique la règle en trois temps de l'en-tête : la figurine nommée si son modèle existe, sinon
 * le mannequin de sa silhouette, sinon le mannequin humanoïde. Le disque n'est lu qu'à la
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

private:
    std::filesystem::path _assetsDirectory;
    /// Clé : `<figure>|<silhouette>`.
    std::map<std::string, ResolvedFigure, std::less<>> _found;
    /// Les descriptions de squelette déjà lues, par silhouette ; nulle pour une qui ne se lit pas.
    std::map<std::string, std::shared_ptr<const core::SkeletonDescription>, std::less<>> _skeletons;
};

}  // namespace hmi
