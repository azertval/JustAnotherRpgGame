// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

#include "HMI/Graphics/RenderLayer.h"

class QRhiTexture;

/**
 * @file HMI/Graphics/TextureLoader.h
 * @brief Chargement de textures GPU depuis des fichiers image (`EX-REN-041`), au travers de QRhi.
 */

namespace hmi {

struct RhiContext;

/// Pixels RGBA décodés d'un fichier image, alpha **droit**, au format `RGBA8` : l'image telle que
/// le fichier la porte. `createTexture` la prémultiplie au téléversement.
struct DecodedImage {
    int width = 0;
    int height = 0;
    std::vector<std::uint32_t> pixels;
};

/**
 * @brief Texture GPU chargée (RAII).
 *
 * Le pointeur est **partagé** et non exclusif : le cache de textures (`hmi::TextureCache`) range
 * ses entrées dans un registre qui les copie, et une même texture peut être servie à plusieurs
 * consommateurs le temps d'une image.
 */
struct LoadedTexture {
    std::shared_ptr<QRhiTexture> texture;
    int width = 0;
    int height = 0;

    /// @return L'identité opaque de la texture, telle que la composition la manipule.
    [[nodiscard]] TextureHandle handle() const noexcept {
        return texture.get();
    }
};

/**
 * @brief Décode un fichier image (PNG au minimum) en pixels RGBA.
 * @param path Chemin du fichier image.
 * @return L'image décodée, ou `std::nullopt` si le fichier est absent/illisible/format non
 *         supporté (erreur récupérable, `EX-NFR-040` — jamais d'exception ici).
 */
[[nodiscard]] std::optional<DecodedImage> decodeImageFile(const std::filesystem::path& path);

/**
 * @brief Écrit un fichier PNG depuis des pixels RGBA déjà en mémoire — symétrique de
 *        `decodeImageFile`.
 *
 * Écriture **atomique** : les pixels sont d'abord écrits dans un fichier temporaire du même
 * dossier, puis ce fichier remplace @p path en une seule opération (`std::filesystem::rename`).
 * Une interruption (crash, disque plein en cours d'écriture) laisse donc soit l'ancien fichier
 * intact, soit le nouveau complet, jamais un fichier tronqué — ce qui importe d'autant plus que le
 * rechargement à chaud peut lire pendant l'écriture. Le fichier temporaire ne subsiste
 * ni après un succès ni après un échec.
 *
 * Le format en mémoire reste `Format_RGBA8888` (alpha non prémultiplié), comme au décodage :
 * décoder puis réencoder restitue exactement les mêmes pixels, canal alpha compris.
 * @param path  Chemin du fichier PNG à écrire.
 * @param image Image à encoder (dimensions strictement positives, `pixels.size() == width *
 *              height`).
 * @return `true` en cas de succès, `false` sinon (dossier de destination absent, image invalide,
 *         échec d'écriture — jamais d'exception, `EX-NFR-040`).
 */
[[nodiscard]] bool encodeImageFile(const std::filesystem::path& path, const DecodedImage& image);

/**
 * @brief Comment une texture s'échantillonne : la **nature** de l'image (`EX-ARCH-022`).
 */
enum class TextureFiltering : std::uint8_t {
    /// Au plus proche, sans mipmap : une image **engendrée** dont chaque pixel est voulu — le
    /// damier
    /// de repli, l'aplat, un marqueur, l'atlas procédural de l'éditeur.
    Sharp,
    /// Bilinéaire avec **mipmaps** : l'art peint, toujours réduit à l'écran, qui scintillerait au
    /// plus proche (`EX-VIS-008`, `LOT-103`).
    Smooth,
};

/**
 * @brief Crée une texture GPU à partir de pixels RGBA déjà décodés.
 *
 * Le téléversement des pixels est **différé** : il est déposé dans le lot de mises à jour de
 * l'image en cours (`hmi::RhiContext::updates`), que l'appelant soumet avant d'ouvrir sa passe de
 * rendu. C'est la contrainte de QRhi qui l'impose, pas un choix d'optimisation — un téléversement
 * ne peut pas avoir lieu au milieu d'une passe.
 *
 * Les pixels sont **prémultipliés** au chargement (`EX-VIS-008`) : le pipeline de `SpriteBatch`
 * mélange en `One`/`OneMinusSrcAlpha`, et un bord adouci filtré sans prémultiplication tirerait
 * vers la couleur, souvent noire, des pixels transparents voisins. Une texture `Smooth` reçoit en
 * plus sa chaîne de mipmaps, engendrée par le GPU dans le même lot ; `SpriteBatch` la reconnaît à
 * son drapeau `MipMapped` et l'échantillonne en bilinéaire.
 *
 * @param context   Interface de rendu et lot de mises à jour de l'image courante.
 * @param width     Largeur en pixels (doit être strictement positive).
 * @param height    Hauteur en pixels (doit être strictement positive).
 * @param pixels    Pixels `RGBA8` à alpha droit, taille attendue `width * height`.
 * @param filtering La nature de l'image ; `Sharp` par défaut, pour les images engendrées.
 * @return La texture chargée, ou `std::nullopt` en cas d'échec de création côté GPU.
 */
[[nodiscard]] std::optional<LoadedTexture> createTexture(
    const RhiContext& context, int width, int height, const std::vector<std::uint32_t>& pixels,
    TextureFiltering filtering = TextureFiltering::Sharp);

/**
 * @brief Décode un fichier image puis crée la texture GPU correspondante, **lissée** : tout
 *        fichier que le rendu charge est de l'art peint (`TextureFiltering::Smooth`).
 * @param context Interface de rendu et lot de mises à jour de l'image courante.
 * @param path    Chemin du fichier image.
 * @return La texture chargée, ou `std::nullopt` si le décodage ou la création GPU échoue.
 */
[[nodiscard]] std::optional<LoadedTexture> loadTextureFromFile(const RhiContext& context,
                                                               const std::filesystem::path& path);

}  // namespace hmi
