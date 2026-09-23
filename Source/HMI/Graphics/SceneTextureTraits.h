// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <optional>
#include <string_view>

#include "Core/Data/JsonDocument.h"
#include "Core/Math/Vector2.h"
#include "HMI/Graphics/ScenePieces.h"

/**
 * @file HMI/Graphics/SceneTextureTraits.h
 * @brief Ce que les fichiers **voisins** d'une image de scène disent d'elle : sa découpe, son
 *        échelle, son ancre (`LOT-103`).
 *
 * Une image ne sait rien d'elle-même : sa taille est tout ce que le décodeur rend. Le reste est
 * écrit à côté — la découpe d'une bande dans son `.anim.json`, l'échelle et l'ancre d'une pièce
 * dans le `manifest.json` de son dossier. Le jeu, l'arène et l'éditeur lisaient ces trois choses
 * chacun de leur côté, et chacun en oubliait une : la lecture est ici, une fois.
 *
 * ## L'échelle, une donnée du lieu
 *
 * Le manifeste d'un dossier de pièces déclare `"tile": [largeur, hauteur]`, le losange de sol en
 * pixels d'art (`EX-VIS-008`) : c'est de lui que le rendu tire la taille à l'écran de chaque pièce,
 * et non d'une constante. Une figurine vit plus bas (`Characters/<pnj>/idle.png`,
 * `Characters/Heroes/brawler/walk-se.png`) : son échelle et sa ligne de sol se lisent dans le
 * premier manifeste **ancêtre** qui déclare un losange, celui de l'atelier qui la range.
 *
 * Logique pure, sans GPU ni Qt ; aucune lecture ne lève (`EX-NFR-040`) : un fichier absent laisse
 * le trait inconnu, et le rendu en déduit un défaut (`hmi::artTileWidth`).
 */

namespace hmi {

/// @brief Les traits d'une image, lus à côté d'elle. Un trait absent reste à sa valeur nulle.
struct SceneTextureTraits {
    /// Largeur et hauteur d'une image de la bande (`.anim.json`) ; 0 pour une image fixe.
    int frameWidth = 0;
    int frameHeight = 0;
    /// Le losange de sol de l'art, en pixels (`tile` du manifeste) ; (0, 0) s'il n'est pas déclaré.
    core::Vector2 artTile{};
    /// L'ancre de la pièce (`anchor` du manifeste), en pixels de l'image.
    std::optional<core::Vector2> anchor;
    /// Décalage de profondeur de la pièce (`depthOffset` du manifeste), en cases.
    std::optional<float> depthOffset;
    /// Ligne de sol d'une figurine (`ground` du manifeste de son atelier), en pixels depuis le haut
    /// de la cellule : la ligne où tombent les pieds (`LOT-112`).
    std::optional<float> groundLine;
    /// Durée d'une image de la bande, en secondes (`frameDuration` de son premier clip) ; 0 si la
    /// bande n'en dit rien.
    float frameDuration = 0.0F;
};

/**
 * @brief Lit les traits de l'image @p path.
 * @param assetsDirectory Racine des chemins d'image.
 * @param path            Chemin de l'image, relatif à @p assetsDirectory (`Scene/bourg/wall.png`,
 *                        `Npc/figurant/idle.png`, `../Scene/bourg/sand.png`).
 */
[[nodiscard]] SceneTextureTraits readSceneTextureTraits(
    const std::filesystem::path& assetsDirectory, std::string_view path);

/**
 * @brief Le losange d'art déclaré par un manifeste (`tile`), (0, 0) s'il n'en déclare pas.
 * @param manifest Racine du manifeste lu.
 */
[[nodiscard]] core::Vector2 manifestArtTile(const nlohmann::json& manifest);

/**
 * @brief L'ancre que le manifeste donne à l'image @p filename, en pixels de l'image.
 *
 * Lue sans condition depuis le `LOT-103` : l'opt-in `placementVersion` protégeait les cartes
 * livrées dans l'ancien style, que la table rase du `LOT-102` a emportées. Une ancre peut sortir de
 * l'image — une fontaine plate ne monte pas jusqu'à la pointe nord de son emprise.
 * @return L'ancre, ou rien si l'image n'est pas citée ou si son ancre n'est pas numérique.
 */
[[nodiscard]] std::optional<core::Vector2> scenePieceAnchor(const nlohmann::json& manifest,
                                                            std::string_view filename);

/// @return Le `depthOffset` de l'image @p filename, rien sans ancre valide ou sans valeur finie.
[[nodiscard]] std::optional<float> scenePieceDepthOffset(const nlohmann::json& manifest,
                                                         std::string_view filename);

/// @brief Reporte les traits @p traits sur @p texture (la texture garde sa poignée et sa taille).
void applySceneTextureTraits(SceneTexture& texture, const SceneTextureTraits& traits);

}  // namespace hmi
