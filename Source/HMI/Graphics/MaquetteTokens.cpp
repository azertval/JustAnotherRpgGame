// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Graphics/MaquetteTokens.h"

#include <array>
#include <cctype>
#include <cstdint>

namespace hmi {

namespace {

constexpr std::string_view TOKEN_ROOT = "Token/";
constexpr std::string_view TOKEN_EXTENSION = ".png";

/// Hauteur d'un glyphe, en pixels de la table ci-dessous.
constexpr int GLYPH_ROWS = 7;
/// Largeur d'un glyphe : chaque ligne tient sur cinq bits, le bit de poids fort a gauche.
constexpr int GLYPH_COLUMNS = 5;

/// Les trente-six glyphes que porte un jeton, plus le point d'interrogation du nom illisible.
///
/// Une table plutot qu'une police : une police est un fichier, et un jeton doit se peindre quand
/// AUCUN fichier n'est present (EX-EXP-005). Trente-sept caracteres de sept octets tiennent ici.
struct Glyph {
    char character;
    std::array<std::uint8_t, GLYPH_ROWS> rows;
};

constexpr std::array<Glyph, 37> GLYPHS = {{
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'J', {0x01, 0x01, 0x01, 0x01, 0x01, 0x11, 0x0E}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11}},
    {'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}},
    {'6', {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}},
    {'?', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}},
}};

[[nodiscard]] const std::array<std::uint8_t, GLYPH_ROWS>& glyphOf(char character) {
    for (const Glyph& glyph : GLYPHS) {
        if (glyph.character == character) {
            return glyph.rows;
        }
    }
    return GLYPHS.back().rows;  // le point d'interrogation
}

[[nodiscard]] core::MarkerColor toMarkerColor(const MaquetteColor& color, float light,
                                              std::uint8_t alpha) {
    const auto channel = [light](float value) {
        const float scaled = value * light * 255.0F;
        return static_cast<std::uint8_t>(scaled < 0.0F ? 0.0F : (scaled > 255.0F ? 255.0F : scaled));
    };
    return core::MarkerColor{
        .r = channel(color.r), .g = channel(color.g), .b = channel(color.b), .a = alpha};
}

/// La lettre se pose en clair sur un jeton sombre, en sombre sur un jeton clair : sans ce choix,
/// la moitie des jetons auraient une lettre illisible.
[[nodiscard]] core::MarkerColor letterColorFor(const MaquetteColor& color) {
    const float luminance = (0.299F * color.r) + (0.587F * color.g) + (0.114F * color.b);
    return luminance > 0.55F ? core::MarkerColor{.r = 20, .g = 22, .b = 26, .a = 255}
                             : core::MarkerColor{.r = 245, .g = 242, .b = 235, .a = 255};
}

}  // namespace

MaquetteColor maquetteTokenColor(MaquetteTokenKind kind) noexcept {
    switch (kind) {
        case MaquetteTokenKind::Player:
            return maquetteColorOf(0x49b265);
        case MaquetteTokenKind::Talker:
            return maquetteColorOf(0xe0bb3f);
        case MaquetteTokenKind::Neutral:
            return maquetteColorOf(0xa9a79e);
        case MaquetteTokenKind::Hostile:
            return maquetteColorOf(0xb33a3a);
        case MaquetteTokenKind::Object:
            return maquetteColorOf(0x6f86a3);
        case MaquetteTokenKind::Portal:
            return maquetteColorOf(0xd2ac62);
    }
    return MaquetteColor{};
}

std::string_view maquetteTokenKindKey(MaquetteTokenKind kind) noexcept {
    switch (kind) {
        case MaquetteTokenKind::Player:
            return "player";
        case MaquetteTokenKind::Talker:
            return "talker";
        case MaquetteTokenKind::Neutral:
            return "neutral";
        case MaquetteTokenKind::Hostile:
            return "hostile";
        case MaquetteTokenKind::Object:
            return "object";
        case MaquetteTokenKind::Portal:
            return "portal";
    }
    return "neutral";
}

char maquetteTokenLetter(std::string_view name) noexcept {
    for (const char character : name) {
        const auto raw = static_cast<unsigned char>(character);
        if (std::isalnum(raw) != 0) {
            return static_cast<char>(std::toupper(raw));
        }
    }
    return '?';
}

std::string maquetteTokenPath(MaquetteTokenKind kind, char letter) {
    std::string path{TOKEN_ROOT};
    path.append(maquetteTokenKindKey(kind));
    path.push_back('/');
    path.push_back(maquetteTokenLetter(std::string_view{&letter, 1}));
    path.append(TOKEN_EXTENSION);
    return path;
}

std::optional<MaquetteTokenRequest> parseMaquetteTokenPath(std::string_view path) {
    if (!path.starts_with(TOKEN_ROOT) || !path.ends_with(TOKEN_EXTENSION)) {
        return std::nullopt;
    }
    const std::string_view body =
        path.substr(TOKEN_ROOT.size(), path.size() - TOKEN_ROOT.size() - TOKEN_EXTENSION.size());
    const std::size_t slash = body.find('/');
    if (slash == std::string_view::npos || body.size() != slash + 2) {
        return std::nullopt;  // une seule lettre, pas davantage
    }
    const std::string_view kindKey = body.substr(0, slash);
    for (const MaquetteTokenKind kind :
         {MaquetteTokenKind::Player, MaquetteTokenKind::Talker, MaquetteTokenKind::Neutral,
          MaquetteTokenKind::Hostile, MaquetteTokenKind::Object, MaquetteTokenKind::Portal}) {
        if (maquetteTokenKindKey(kind) == kindKey) {
            return MaquetteTokenRequest{.kind = kind, .letter = body.back()};
        }
    }
    return std::nullopt;
}

core::MarkerImage maquetteTokenImage(const MaquetteTokenRequest& request, int size) {
    if (size <= 0) {
        return {};
    }
    const MaquetteColor tint = maquetteTokenColor(request.kind);
    const core::MarkerColor disc = toMarkerColor(tint, 1.0F, 255);
    const core::MarkerColor rim = toMarkerColor(tint, 0.45F, 255);
    const core::MarkerColor letter = letterColorFor(tint);

    core::MarkerImage image;
    image.width = size;
    image.height = size;
    image.pixels.assign(static_cast<std::size_t>(size) * static_cast<std::size_t>(size),
                        core::MarkerColor{.r = 0, .g = 0, .b = 0, .a = 0});

    // Le disque, cerne : le cercle plein se lit a toute taille, le cerne le detache d'un sol de
    // teinte voisine.
    const float centre = static_cast<float>(size - 1) / 2.0F;
    const float radius = static_cast<float>(size) / 2.0F;
    const float rimRadius = radius - (static_cast<float>(size) * 0.14F);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float dx = static_cast<float>(x) - centre;
            const float dy = static_cast<float>(y) - centre;
            const float distance = (dx * dx) + (dy * dy);
            if (distance > radius * radius) {
                continue;
            }
            image.pixels[static_cast<std::size_t>((y * size) + x)] =
                distance > rimRadius * rimRadius ? rim : disc;
        }
    }

    // La lettre, au plus grand entier qui tienne dans le disque.
    const int scale = std::max(1, size / 11);
    const int glyphWidth = GLYPH_COLUMNS * scale;
    const int glyphHeight = GLYPH_ROWS * scale;
    const int originX = (size - glyphWidth) / 2;
    const int originY = (size - glyphHeight) / 2;
    const std::array<std::uint8_t, GLYPH_ROWS>& rows = glyphOf(request.letter);
    for (int row = 0; row < GLYPH_ROWS; ++row) {
        for (int column = 0; column < GLYPH_COLUMNS; ++column) {
            const auto bit = static_cast<std::uint8_t>(1U << (GLYPH_COLUMNS - 1 - column));
            if ((rows[static_cast<std::size_t>(row)] & bit) == 0) {
                continue;
            }
            for (int dy = 0; dy < scale; ++dy) {
                for (int dx = 0; dx < scale; ++dx) {
                    const int x = originX + (column * scale) + dx;
                    const int y = originY + (row * scale) + dy;
                    if (x < 0 || y < 0 || x >= size || y >= size) {
                        continue;
                    }
                    image.pixels[static_cast<std::size_t>((y * size) + x)] = letter;
                }
            }
        }
    }
    return image;
}

core::MarkerImage maquetteTokenImage(std::string_view path, int size) {
    const std::optional<MaquetteTokenRequest> request = parseMaquetteTokenPath(path);
    return request ? maquetteTokenImage(*request, size) : core::MarkerImage{};
}

}  // namespace hmi
