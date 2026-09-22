// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file HMI/Graphics/AssetGallery.h
 * @brief La galerie des assets, outil de débug : inventaire, disposition en blocs et visibilité,
 *        **sans GPU**.
 *
 * Ce n'est pas un écran du jeu (décision du 16 septembre 2026) : un banc pour voir d'un coup
 * d'œil tous les modèles, toutes leurs formes et toutes leurs animations, que le Colisée ne
 * pouvait plus montrer à mesure que les ateliers (PNJ, textures) en produisent.
 */

namespace hmi {

/// Largeur d'une case de la galerie à l'écran, au zoom 1, en pixels de l'élément : celle d'une
/// case du jeu à 1080p (`hmi::worldTilePixels`). Une taille d'écran, pas d'art : chaque forme s'y
/// ramène par le losange que déclare son manifeste (`AssetGalleryEntry::tilePixels`, `LOT-103`).
inline constexpr int ASSET_GALLERY_CELL_PIXELS = 100;

/// Temps de tenue, en secondes, d'un clip joué une fois sur sa dernière image avant de reprendre.
inline constexpr double ASSET_GALLERY_ONE_SHOT_HOLD_SECONDS = 0.6;

/**
 * @brief Une forme d'un modèle : un clip d'une figure, une variante de texture, une pièce.
 *
 * Tout ce qui sert à la disposer est lu dans les manifestes : la galerie place un asset sans
 * charger sa texture, c'est ce qui lui permet de ne charger que ce qui est à l'écran.
 */
struct AssetGalleryEntry {
    /// Nom de la famille (« PNJ », « Scène · martpart »…).
    std::string family;
    /// Le modèle : une ligne de la galerie (« anariel », « floor », « props »).
    std::string model;
    /// La forme : une colonne (« attack », « street-2 », « 06 »).
    std::string form;
    /// Chemin de la texture, relatif à la racine des assets, séparateurs `/`.
    std::string path;
    /// Taille d'une image, en pixels. Une texture fixe n'a qu'une image, de sa taille entière.
    int frameWidth = 0;
    int frameHeight = 0;
    /// Indices d'images dans la bande, dans l'ordre de lecture ; vide : l'image 0.
    std::vector<int> frames;
    /// Durée d'une image, en secondes ; 0 : figée.
    double frameDuration = 0.0;
    /// Bouclé, ou joué une fois puis tenu (`ASSET_GALLERY_ONE_SHOT_HOLD_SECONDS`).
    bool loop = true;
    /// Emprise au sol, en cases.
    int footprintColumns = 1;
    int footprintRows = 1;
    /// Ancre du manifeste des scènes, en pixels de texture ; (-1, -1) si le manifeste n'en a pas.
    int anchorX = -1;
    int anchorY = -1;
    /// Largeur d'une case, en pixels d'art : le losange que déclare le manifeste (`tile`) ; 0 s'il
    /// n'en déclare pas, et la forme se suppose d'une case de large (`tileWidthPixels`).
    int tilePixels = 0;

    /// @return La largeur d'une case en pixels d'art, jamais nulle.
    [[nodiscard]] int tileWidthPixels() const noexcept {
        if (tilePixels > 0) {
            return tilePixels;
        }
        const int columns = footprintColumns > 0 ? footprintColumns : 1;
        return frameWidth > 0 ? (frameWidth + columns - 1) / columns : 1;
    }

    /// @return Le nombre d'images jouées (au moins 1).
    [[nodiscard]] int frameCount() const noexcept {
        return frames.empty() ? 1 : static_cast<int>(frames.size());
    }
};

/// Une famille : une bande de la galerie.
struct AssetGalleryFamily {
    std::string title;
    /// Dossier lu, relatif à la racine des assets.
    std::string directory;
    std::vector<AssetGalleryEntry> entries;
};

/**
 * @brief L'inventaire de la galerie, lu dans les manifestes des assets livrés.
 *
 * Familles lues, dans cet ordre, chacune seulement si elle existe :
 * - `Npc/manifest.json` : chaque PNJ × chaque animation, d'après son `.anim.json`, et son portrait
 * ;
 * - `Monsters/manifest.json` : les figurines de l'atelier des monstres (LOT-93), de même forme ;
 *   une bête sans sort n'a pas de `cast`, et une Grande a ses cellules de 96 × 96 ;
 * - `Coliseum/manifest.json` : héros × animations, gladiateurs, puis les pièces de la planche ;
 * - `Scene/<disposition>/manifest.json` : les textures de l'atelier (LOT-92), par classe ;
 * - l'arborescence par niveaux (LOT-102), où la chaîne HD installe (LOT-104) : chaque
 *   `manifest.json` sous `Common/` et `Regions/`, dans l'ordre de son chemin — les `textures` d'un
 *   dossier `Scene/` en une famille titrée par son lieu (« Scène · central-empire/capital/arenarea
 * »), les PNJ d'un dossier `Characters/` en une famille « Figurines · » suivie du dossier.
 *
 * Tout asset livré doit y paraître (`EX-CNT-042`) : `hmi::assetGalleryUnlisted` nomme ceux qui n'y
 * sont pas, et un test l'exige vide.
 *
 * Un manifeste illisible est une erreur **nommée**, jamais un arrêt : la galerie montre le reste.
 */
struct AssetGalleryCatalog {
    std::vector<AssetGalleryFamily> families;
    std::vector<std::string> errors;

    [[nodiscard]] static AssetGalleryCatalog load(const std::filesystem::path& assetsRoot);

    /// @return Le nombre total de formes.
    [[nodiscard]] std::size_t entryCount() const noexcept;
};

/**
 * @brief Les images livrées qui ne sont pas des assets à montrer, par règle nommée : l'interface
 *        (`UI/`), les cartes plein écran de l'écran « Carte » (`Maps/`, que cet écran montre déjà
 *        une à une) et les polices (`Fonts/`).
 *
 * Les planches **sources** des ateliers n'ont plus de règle : elles ne sont plus versionnées
 * (`Tools/AssetsHD/`, hors dépôt), seul l'asset installé entre dans le dépôt (`LOT-102`).
 * @param path Chemin relatif à la racine des assets, séparateurs `/`.
 */
[[nodiscard]] bool assetGalleryExcludes(std::string_view path) noexcept;

/**
 * @brief Les images livrées (PNG, JPEG) que la galerie ne montre pas et qu'aucune exclusion ne
 *        couvre : ce que `EX-CNT-042` interdit.
 * @return Chemins relatifs à @p assetsRoot, triés ; vide quand la galerie est complète.
 */
[[nodiscard]] std::vector<std::string> assetGalleryUnlisted(const std::filesystem::path& assetsRoot,
                                                            const AssetGalleryCatalog& catalog);

/**
 * @brief Un bloc : l'emprise d'une forme plus une case de marge tout autour, en cases.
 *
 * Le dessin peut déborder de l'emprise (une attaque de 96 px, un mur de 100 px de haut) : le bloc
 * grandit pour le contenir, marge comprise. La forme est posée pieds sur le bas de son emprise,
 * centrée sur elle.
 */
struct AssetGalleryBloc {
    int family = 0;
    int entry = 0;
    int column = 0;
    int row = 0;
    int columns = 3;
    int rows = 3;
    /// Coin haut-gauche de l'emprise, relatif au bloc.
    int footprintColumn = 1;
    int footprintRow = 1;
};

/// L'en-tête d'une famille : une ligne de la galerie.
struct AssetGalleryBand {
    int family = 0;
    int row = 0;
};

/// La galerie disposée : bandes, puis une ligne par modèle, une colonne par forme.
struct AssetGalleryLayout {
    std::vector<AssetGalleryBloc> blocs;
    std::vector<AssetGalleryBand> bands;
    int columns = 0;
    int rows = 0;
};

/// @return Les dimensions du bloc d'une forme, et la place de son emprise.
[[nodiscard]] AssetGalleryBloc assetGalleryBlocShape(const AssetGalleryEntry& entry);

/**
 * @brief Dispose le catalogue : une bande par famille, une ligne par modèle, et au-delà de
 *        @p maximumColumns cases une ligne continue la suivante.
 */
[[nodiscard]] AssetGalleryLayout layoutAssetGallery(const AssetGalleryCatalog& catalog,
                                                    int maximumColumns = 40);

/// Ce que devient un bloc pour une caméra donnée.
enum class AssetGalleryVisibility {
    /// Le bloc touche la vue : texture chargée, quads émis.
    Drawn,
    /// Le bloc est dans l'anneau autour de la vue : texture gardée, rien d'émis.
    Preloaded,
    /// Hors de l'anneau : la texture peut être libérée.
    Unloaded,
};

/// Un rectangle en cases (flottantes).
struct AssetGalleryView {
    double column = 0.0;
    double row = 0.0;
    double columns = 0.0;
    double rows = 0.0;
};

/// Épaisseur de l'anneau de préchargement, en cases : un bloc ordinaire.
inline constexpr double ASSET_GALLERY_RING_CELLS = 3.0;

[[nodiscard]] AssetGalleryVisibility assetGalleryVisibility(
    const AssetGalleryBloc& bloc, const AssetGalleryView& view,
    double ringCells = ASSET_GALLERY_RING_CELLS) noexcept;

/**
 * @brief L'image jouée au temps @p seconds : en boucle, ou jouée une fois puis tenue
 *        `ASSET_GALLERY_ONE_SHOT_HOLD_SECONDS` avant de reprendre.
 * @return Un rang dans `entry.frames` (0 pour une forme figée).
 */
[[nodiscard]] int assetGalleryFrameRank(const AssetGalleryEntry& entry, double seconds) noexcept;

}  // namespace hmi
