// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/TextureLoader.h"

#include <QImage>
#include <QString>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <system_error>
#include <thread>

#include <rhi/qrhi.h>

#include "HMI/Graphics/GraphicsLog.h"
#include "HMI/Graphics/RhiContext.h"

namespace hmi {

namespace {

// Decode sans rien journaliser : ce qui tourne sur un fil de decodage ne touche pas au journal.
[[nodiscard]] std::optional<DecodedImage> readImageFile(const std::filesystem::path& path) {
    QImage source(QString::fromStdWString(path.wstring()));
    if (source.isNull()) {
        return std::nullopt;
    }
    // Format_RGBA8888 : quatre octets R,G,B,A en mémoire, alpha droit — le même ordre mémoire que
    // QRhiTexture::RGBA8. La prémultiplication se fait à la création de la texture, pas ici :
    // l'image décodée reste celle du fichier, que `encodeImageFile` réécrit à l'identique.
    const QImage image = source.convertToFormat(QImage::Format_RGBA8888);

    DecodedImage decoded;
    decoded.width = image.width();
    decoded.height = image.height();
    decoded.pixels.resize(static_cast<std::size_t>(decoded.width) *
                          static_cast<std::size_t>(decoded.height));
    const std::size_t rowBytes = static_cast<std::size_t>(decoded.width) * sizeof(std::uint32_t);
    for (int row = 0; row < decoded.height; ++row) {
        std::memcpy(decoded.pixels.data() + (static_cast<std::size_t>(row) * decoded.width),
                    image.constScanLine(row), rowBytes);
    }
    return decoded;
}

void warnUndecoded(const std::filesystem::path& path) {
    GRAPHICS_LOG_WARNING("TextureLoader : echec de decodage de l'image '" + path.string() + "'");
}

}  // namespace

// Décode un fichier image (PNG au minimum) en pixels RGBA.
std::optional<DecodedImage> decodeImageFile(const std::filesystem::path& path) {
    std::optional<DecodedImage> decoded = readImageFile(path);
    if (!decoded) {
        warnUndecoded(path);
    }
    return decoded;
}

// Décode une liste d'images sur les coeurs de la machine, dans l'ordre de la liste.
std::vector<std::optional<DecodedImage>> decodeImageFiles(
    const std::vector<std::filesystem::path>& paths) {
    std::vector<std::optional<DecodedImage>> decoded(paths.size());
    const unsigned cores = std::max(1U, std::thread::hardware_concurrency());
    const std::size_t workers = std::min<std::size_t>(paths.size(), cores);
    std::atomic<std::size_t> next{0};
    const auto work = [&]() noexcept {
        for (std::size_t index = next++; index < paths.size(); index = next++) {
            try {
                decoded[index] = readImageFile(paths[index]);
            } catch (...) {
                decoded[index].reset();  // une allocation refusee : l'image manque, rien ne tombe
            }
        }
    };
    {
        std::vector<std::jthread> threads;
        threads.reserve(workers > 0 ? workers - 1 : 0);
        for (std::size_t thread = 1; thread < workers; ++thread) {
            threads.emplace_back(work);
        }
        work();  // le fil appelant decode aussi
    }
    for (std::size_t index = 0; index < paths.size(); ++index) {
        if (!decoded[index]) {
            warnUndecoded(paths[index]);
        }
    }
    return decoded;
}

namespace {

// Ecriture atomique : fichier temporaire dans le meme dossier (donc le meme volume, condition
// pour que le remplacement soit atomique), puis remplacement en une seule operation. Un
// QFileSystemWatcher de rechargement a chaud ne voit ainsi jamais de fichier tronque.
bool savePngAtomically(const QImage& output, const std::filesystem::path& path,
                       const std::filesystem::path& directory) {
    std::error_code error;
    const std::filesystem::path temporary =
        directory / (path.stem().wstring() + L".tmp" + path.extension().wstring());
    if (!output.save(QString::fromStdWString(temporary.wstring()), "PNG")) {
        GRAPHICS_LOG_WARNING("TextureLoader : echec d'ecriture temporaire pour '" + path.string() +
                             "'");
        std::filesystem::remove(temporary, error);
        return false;
    }
    std::filesystem::rename(temporary, path, error);
    if (error) {
        GRAPHICS_LOG_WARNING("TextureLoader : echec du remplacement atomique pour '" +
                             path.string() + "'");
        std::filesystem::remove(temporary, error);
        return false;
    }
    return true;
}

}  // namespace

// Écrit un fichier PNG depuis des pixels RGBA déjà en mémoire — symétrique de decodeImageFile.
bool encodeImageFile(const std::filesystem::path& path, const DecodedImage& image) {
    if (image.width <= 0 || image.height <= 0 ||
        image.pixels.size() !=
            static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height)) {
        GRAPHICS_LOG_WARNING("TextureLoader : image invalide, encodage refuse pour '" +
                             path.string() + "'");
        return false;
    }

    std::error_code error;
    const std::filesystem::path directory = path.parent_path();
    if (!directory.empty() && !std::filesystem::is_directory(directory, error)) {
        GRAPHICS_LOG_WARNING("TextureLoader : dossier de destination introuvable pour '" +
                             path.string() + "'");
        return false;
    }

    // Format_RGBA8888 : le meme format non premultiplie que decodeImageFile lit -- aucune
    // conversion de canal, l'aller-retour restitue exactement les memes pixels.
    // QImage n'accepte qu'un tampon d'octets : relire les pixels 32 bits octet par octet est
    // l'usage voulu (et le passage par void* est refuse par bugprone-casting-through-void).
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const QImage output(reinterpret_cast<const uchar*>(image.pixels.data()), image.width,
                        image.height, static_cast<int>(image.width * sizeof(std::uint32_t)),
                        QImage::Format_RGBA8888);

    return savePngAtomically(output, path, directory);
}

// Crée une texture GPU à partir de pixels RGBA déjà décodés.
std::optional<LoadedTexture> createTexture(const RhiContext& context, int width, int height,
                                           const std::vector<std::uint32_t>& pixels,
                                           TextureFiltering filtering) {
    if (width <= 0 || height <= 0) {
        return std::nullopt;
    }
    if (!context.ready()) {
        // Etat de demarrage legitime (aucune image en cours) plutot qu'un defaut : l'appelant
        // retombera sur le damier, et la texture sera chargee a la premiere image (EX-NFR-040).
        GRAPHICS_LOG_WARNING("TextureLoader : aucune image en cours, creation de texture differee");
        return std::nullopt;
    }
    if (pixels.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {
        GRAPHICS_LOG_WARNING("TextureLoader : dimensions et pixels incoherents, texture refusee");
        return std::nullopt;
    }

    // Les mipmaps s'engendrent sur le GPU : QRhi 6 les garantit sur tous ses backends.
    const bool mipmapped = filtering == TextureFiltering::Smooth;
    const QSize size(width, height);
    LoadedTexture result;
    result.width = width;
    result.height = height;
    result.texture.reset(context.rhi->newTexture(
        QRhiTexture::RGBA8, size, 1,
        mipmapped ? QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips
                  : QRhiTexture::Flags{}));
    if (!result.texture->create()) {
        GRAPHICS_LOG_WARNING("TextureLoader : echec de creation de la texture GPU");
        return std::nullopt;
    }

    // INVARIANT DE DUREE DE VIE : l'image deposee dans le lot doit POSSEDER ses pixels. Le
    // televersement n'a pas lieu ici mais au moment ou l'appelant soumet le lot -- typiquement
    // plusieurs appels plus loin, alors que @p pixels appartient a l'appelant et a pu disparaitre
    // entre-temps. Une QImage construite sur le pointeur brut ne copierait rien : le lot lirait
    // une memoire qui ne lui appartient pas. D'ou la copie, payee une fois par texture chargee.
    QImage owned(width, height, QImage::Format_RGBA8888);
    std::memcpy(owned.bits(), pixels.data(), pixels.size() * sizeof(std::uint32_t));
    // Premultiplie (EX-VIS-008) : meme ordre d'octets, R,G,B deja multiplies par A. C'est aussi ce
    // qui rend juste la moyenne des mipmaps sur un bord adouci.
    owned.convertTo(QImage::Format_RGBA8888_Premultiplied);
    QRhiTextureUploadDescription upload({0, 0, QRhiTextureSubresourceUploadDescription(owned)});
    context.updates->uploadTexture(result.texture.get(), upload);
    if (mipmapped) {
        context.updates->generateMips(result.texture.get());
    }
    return result;
}

// Décode un fichier image puis crée la texture GPU correspondante.
std::optional<LoadedTexture> loadTextureFromFile(const RhiContext& context,
                                                 const std::filesystem::path& path) {
    const std::optional<DecodedImage> decoded = decodeImageFile(path);
    if (!decoded) {
        return std::nullopt;
    }
    return createTexture(context, decoded->width, decoded->height, decoded->pixels,
                         TextureFiltering::Smooth);
}

}  // namespace hmi
