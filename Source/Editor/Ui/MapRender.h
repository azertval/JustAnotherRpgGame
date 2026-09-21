// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QColor>
#include <QImage>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Editor/Logic/CanvasScene.h"
#include "Editor/Logic/Stamps.h"

namespace core {
class Level;
}

/**
 * @file Editor/Ui/MapRender.h
 * @brief `LevelEditor --render` : une carte rendue **hors écran** en PNG, en isométrie, par le
 *        peintre du canevas (`LOT-EDITOR-13`, décision D9, `EX-EDIT-075`).
 *
 * La même image que le canevas : l'instantané de `hmi::canvasSnapshot`, composé par
 * `hmi::composeWorldScene` et peint par `hmi::paintComposedScene`, sans fenêtre ni GPU — ce qui
 * le fait tourner en CI, où il montre dans la PR la carte qu'elle change. Les bandes se choisissent
 * comme les calques du canevas (`hmi::IsoBandOpacity`) : sol, relief, figurines, et le masque de
 * collision par-dessus. Une carte qui ne nomme aucun lieu se peint par les couleurs de ses types,
 * comme dans le canevas.
 *
 * L'échelle 1 est celle des planches : un pixel d'image par pixel d'art (`Camera2D`, zoom 1), ce
 * qui donne à Martpart (48 × 40) une image de 3 800 × 2 400 pixels environ.
 */

namespace hmi {

/// @brief Les réglages d'un rendu.
struct MapRenderOptions {
    /// Les bandes peintes ; par défaut, ce que le jeu montre, sans la collision.
    IsoBandOpacity bands;
    /// Pixels d'image par pixel d'art, dans ]0, 4].
    double scale = 1.0;
    /// Le fond, autour du losange de la carte.
    QColor background = QColor(24, 26, 30);
    /// **Plan de principe** (`--plan`, `LOT-128`) : les blocs se couchent en losanges plats et une
    /// légende s'ajoute. Ce que la carte contient et comment on y circule, pas ce qu'on y voit.
    bool plan = false;
};

/**
 * @brief Lit une liste de bandes (`floors,relief,figures,collision`).
 * @return Les opacités (1 pour une bande nommée, 0 sinon), ou rien si un nom est inconnu.
 */
[[nodiscard]] std::optional<IsoBandOpacity> parseRenderLayers(std::string_view list);

/**
 * @brief Rend @p level en isométrie, hors écran.
 * @param level    La carte.
 * @param dataRoot La racine des données : la table du lieu, ses planches, les figurines.
 * @param options  Bandes, échelle et fond.
 */
[[nodiscard]] QImage renderMap(const core::Level& level, const std::filesystem::path& dataRoot,
                               const MapRenderOptions& options);

/**
 * @brief La **vignette d'un préfabriqué** (`LOT-EDITOR-08`) : le tampon posé sur une carte de sa
 *        taille, rendu par le même peintre, réduit pour tenir dans un carré de @p maxSide pixels.
 * @param stamp    Le tampon.
 * @param dataRoot La racine des données (les planches).
 * @param place    Le lieu dont il prend ses pièces ; vide, il se peint par ses types.
 * @param maxSide  Le côté du carré, en pixels (> 0).
 * @return L'image, nulle si le tampon ne se compose pas.
 */
[[nodiscard]] QImage renderStamp(const Stamp& stamp, const std::filesystem::path& dataRoot,
                                 const std::string& place, int maxSide);

/**
 * @brief L'entrée `--render` de l'éditeur, avant toute construction de fenêtre.
 *
 * - `--render [carte…]` : les cartes nommées (identifiant ou chemin), toutes à défaut ;
 * - `--output <fichier.png | dossier>` : un fichier pour une carte unique, sinon un dossier où
 *   chaque carte s'écrit `capital-martpart.png` (défaut : le dossier courant) ;
 * - `--layers floors,relief,figures,collision` : les bandes (défaut : les trois premières) ;
 * - `--plan` : le plan de principe — losanges plats, pastilles, légende ;
 * - `--scale <s>` : l'échelle ;
 * - `--data <racine>` : la racine des données (défaut : @p defaultDataRoot).
 *
 * @return Le code de sortie (0, 1 si une carte ne se lit pas ou ne s'écrit pas, 2 si la ligne de
 *         commande est fausse), ou `std::nullopt` sans `--render`.
 */
[[nodiscard]] std::optional<int> runRenderCommand(const std::vector<std::string>& arguments,
                                                  const std::filesystem::path& defaultDataRoot,
                                                  std::string& output);

}  // namespace hmi
