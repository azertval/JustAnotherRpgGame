// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "HMI/Game/DebugCommands.h"

#include <array>
#include <charconv>

namespace hmi {

namespace {

// L'ordre est celui de l'aide : d'abord ce qui ouvre une carte, puis ce qui regle la fenetre, puis
// ce qui ne se lit qu'au lancement.
constexpr std::array CATALOGUE = {
    DebugOption{.name = "--map=",
                .syntax = "--map=<carte>[@<arrivee>]",
                .description = "Ouvre la carte (identifiant sous Levels/, sans .json), au point "
                               "d'arrivee nomme ou a son entree."},
    DebugOption{.name = "--at=",
                .syntax = "--at=<colonne>,<ligne>",
                .description = "Pose le heros sur cette case de la carte ouverte par --map=."},
    DebugOption{.name = "--flags=",
                .syntax = "--flags=<drapeau>[=<valeur>],<drapeau>",
                .description = "Pose ces drapeaux de monde : la carte telle qu'elle est apres une "
                               "quete."},
    DebugOption{.name = "--hero-figure=",
                .syntax = "--hero-figure=<dossier sous Assets/>",
                .description = "Remplace la figurine du heros."},
    DebugOption{.name = "--levels=",
                .syntax = "--levels=<dossier>;<dossier>",
                .description = "Cherche les cartes d'abord dans ces dossiers (les brouillons de "
                               "l'editeur), puis dans celles du binaire."},
    DebugOption{.name = "--screen=",
                .syntax = "--screen=<Nom>",
                .description = "Ouvre un ecran par son nom (MainMenu, GameView, Options, "
                               "Inventory, MapLauncher, AssetGallery...), sans passer par le "
                               "routeur."},
    DebugOption{.name = "--window-size=",
                .syntax = "--window-size=<largeur>x<hauteur>",
                .description = "Impose la taille de la fenetre, en pixels."},
    DebugOption{.name = "--screenshot=",
                .syntax = "--screenshot=<chemin.png>",
                .description = "Capture la fenetre dans ce fichier (au lancement : puis quitte)."},
    DebugOption{.name = "--log-level=",
                .syntax = "--log-level=<trace|info|warning|error>",
                .description = "Niveau minimum du journal de session."},
    DebugOption{.name = "--data=",
                .syntax = "--data=<racine>",
                .description = "Lit tout le contenu (cartes, assets, monde, dialogues) sous cette "
                               "racine, comme l'editeur l'ouvre.",
                .scope = DebugOptionScope::LaunchOnly},
    DebugOption{.name = "--map-region=",
                .syntax = "--map-region=<region>",
                .description = "L'ecran Carte s'ouvre sur cette region (avec --screen=WorldMap).",
                .scope = DebugOptionScope::LaunchOnly},
    DebugOption{.name = "--map-city=",
                .syntax = "--map-city=<lieu>",
                .description = "... puis sur cette ville.",
                .scope = DebugOptionScope::LaunchOnly},
    DebugOption{.name = "--map-district=",
                .syntax = "--map-district=<quartier>",
                .description = "... puis sur ce quartier.",
                .scope = DebugOptionScope::LaunchOnly},
    DebugOption{.name = "--map-block=",
                .syntax = "--map-block=<ilot>",
                .description = "... puis sur cet ilot.",
                .scope = DebugOptionScope::LaunchOnly},
    DebugOption{.name = "--crash-test",
                .syntax = "--crash-test",
                .description = "Plantage volontaire au demarrage, pour eprouver le minidump.",
                .scope = DebugOptionScope::LaunchOnly},
};

/// @return L'entier de @p text, s'il est ecrit en entier et sans rien d'autre.
[[nodiscard]] std::optional<int> entier(std::string_view text) {
    int valeur = 0;
    // NOLINTNEXTLINE(bugprone-suspicious-stringview-data-usage): la paire data()/size() borne la
    // lecture, from_chars ne depasse jamais la vue.
    const std::from_chars_result lu =
        std::from_chars(text.data(), text.data() + text.size(), valeur);
    if (lu.ec != std::errc{} || lu.ptr != text.data() + text.size()) {
        return std::nullopt;
    }
    return valeur;
}

}  // namespace

std::span<const DebugOption> debugOptionCatalog() noexcept {
    return CATALOGUE;
}

DebugArgument splitDebugArgument(std::string_view token) {
    if (!token.starts_with("--")) {
        return DebugArgument{.name = std::string{token}, .value = {}};
    }
    const std::size_t egal = token.find('=');
    if (egal == std::string_view::npos) {
        return DebugArgument{.name = std::string{token}, .value = {}};
    }
    return DebugArgument{.name = std::string{token.substr(0, egal + 1)},
                         .value = std::string{token.substr(egal + 1)}};
}

const DebugOption* findDebugOption(std::string_view name) noexcept {
    for (const DebugOption& option : CATALOGUE) {
        if (option.name == name) {
            return &option;
        }
    }
    return nullptr;
}

std::vector<std::string> splitCommandLine(std::string_view line) {
    std::vector<std::string> mots;
    std::string courant;
    bool dansMot = false;
    bool entreGuillemets = false;
    for (const char caractere : line) {
        if (caractere == '"') {
            // Un mot peut commencer par des guillemets vides : `""` est un mot vide, voulu.
            entreGuillemets = !entreGuillemets;
            dansMot = true;
            continue;
        }
        if (!entreGuillemets && (caractere == ' ' || caractere == '\t' || caractere == '\n' ||
                                 caractere == '\r')) {
            if (dansMot) {
                mots.push_back(std::move(courant));
                courant.clear();
                dansMot = false;
            }
            continue;
        }
        courant.push_back(caractere);
        dansMot = true;
    }
    if (dansMot) {
        mots.push_back(std::move(courant));
    }
    return mots;
}

std::optional<std::pair<int, int>> parseWindowSize(std::string_view value) {
    const std::size_t croix = value.find('x');
    if (croix == std::string_view::npos) {
        return std::nullopt;
    }
    const std::optional<int> largeur = entier(value.substr(0, croix));
    const std::optional<int> hauteur = entier(value.substr(croix + 1));
    if (!largeur || !hauteur || *largeur <= 0 || *hauteur <= 0) {
        return std::nullopt;
    }
    return std::pair{*largeur, *hauteur};
}

}  // namespace hmi
