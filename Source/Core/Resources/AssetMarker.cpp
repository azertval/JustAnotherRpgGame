// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Resources/AssetMarker.h"

#include <algorithm>
#include <cmath>

#include "Core/Resources/AssetKey.h"

namespace core {
namespace {

// FNV-1a 32 bits, ecrit ici plutot qu'emprunte a std::hash : la valeur de ce dernier n'est PAS
// garantie stable d'une plateforme ou d'une version de bibliotheque a l'autre, et le marqueur d'un
// loup changerait donc de teinte au changement de compilateur -- une regression invisible en
// developpement et visible chez le joueur.
constexpr std::uint32_t FNV_BASE = 2166136261U;
constexpr std::uint32_t FNV_PREMIER = 16777619U;

// Teinte -> RVB, saturation et valeur fixes : les marqueurs se distinguent entre eux sans qu'aucun
// ne passe pour une illustration.
[[nodiscard]] MarkerColor depuisTeinte(double teinte, double valeur) {
    constexpr double SATURATION = 0.45;
    const double secteur = std::fmod(teinte, 360.0) / 60.0;
    const double chroma = valeur * SATURATION;
    const double second = chroma * (1.0 - std::fabs(std::fmod(secteur, 2.0) - 1.0));
    const double base = valeur - chroma;

    double r = 0.0;
    double v = 0.0;
    double b = 0.0;
    switch (static_cast<int>(secteur)) {
        case 0:
            r = chroma;
            v = second;
            break;
        case 1:
            r = second;
            v = chroma;
            break;
        case 2:
            v = chroma;
            b = second;
            break;
        case 3:
            v = second;
            b = chroma;
            break;
        case 4:
            r = second;
            b = chroma;
            break;
        default:
            r = chroma;
            b = second;
            break;
    }
    const auto octet = [base](double composante) {
        return static_cast<std::uint8_t>(std::clamp((composante + base) * 255.0, 0.0, 255.0));
    };
    return {.r = octet(r), .g = octet(v), .b = octet(b), .a = 255};
}

}  // namespace

std::uint32_t stableAssetHash(std::string_view key) {
    std::uint32_t empreinte = FNV_BASE;
    for (const char lettre : key) {
        empreinte ^= static_cast<std::uint8_t>(lettre);
        empreinte *= FNV_PREMIER;
    }
    return empreinte;
}

MarkerImage assetMarker(std::string_view key, int width, int height) {
    MarkerImage image;
    if (!isValidAssetKey(key) || width <= 0 || height <= 0) {
        // Une cle malformee ne recoit pas un marqueur « par defaut » : elle est refusee. Lui en
        // donner un la ferait passer pour une entree en attente d'illustration, alors qu'elle est
        // une faute de donnee que `expectedAssetKeys` signale.
        return image;
    }
    image.width = width;
    image.height = height;
    image.pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));

    const std::uint32_t empreinte = stableAssetHash(key);
    const auto teinte = static_cast<double>(empreinte % 360U);
    const MarkerColor fond = depuisTeinte(teinte, 0.42);
    const MarkerColor trait = depuisTeinte(teinte, 0.78);

    // Epaisseur des diagonales : proportionnelle a la vignette, jamais un nombre de pixels fixe --
    // deux pixels se voient sur 48 et disparaissent sur 128.
    const double epaisseur = std::max(1.5, static_cast<double>(std::min(width, height)) / 24.0);
    const double pente = static_cast<double>(height) / static_cast<double>(width);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const double attendueDescendante = static_cast<double>(x) * pente;
            const double attendueMontante = static_cast<double>(height - 1) - attendueDescendante;
            const bool surDiagonale =
                std::fabs(static_cast<double>(y) - attendueDescendante) <= epaisseur ||
                std::fabs(static_cast<double>(y) - attendueMontante) <= epaisseur;
            // Deux diagonales barrent la vignette : un marqueur doit se voir COMME un marqueur, et
            // ne jamais passer pour une illustration definitive livree un peu vite.
            image.pixels[(static_cast<std::size_t>(y) * static_cast<std::size_t>(width)) +
                         static_cast<std::size_t>(x)] = surDiagonale ? trait : fond;
        }
    }
    return image;
}

}  // namespace core
