// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file HMI/Game/DebugCommands.h
 * @brief Les options de la ligne de commande du jeu, telles que la console de debug (`F9`) les
 *        rejoue — le catalogue, et l'analyse d'une ligne tapée.
 *
 * Le jeu lit ses options une par une dans `App/Game/Main.cpp` (`app::commandLineOption`) ; rien
 * ne les **listait**. La console de debug a besoin de la liste — pour l'aide, pour dire ce qui
 * s'applique à chaud et ce qui exige un relancement — et d'un découpage de ce que l'on tape qui
 * respecte les guillemets (un chemin avec espaces). Les deux vivent ici, sans Qt, pour que le
 * catalogue se vérifie par un test et que la console n'ait qu'à le suivre.
 *
 * Un nom d'option écrit ici et pas dans `Main.cpp` — ou l'inverse — est une divergence, et le test
 * du catalogue la relève sur les noms que `Main.cpp` lit.
 */

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace hmi {

/// @brief Où une option s'applique.
enum class DebugOptionScope {
    /// La console l'applique **à chaud**, dans la partie en cours.
    Live,
    /// Lue au lancement seulement : la console propose de relancer le jeu avec.
    LaunchOnly,
};

/// @brief Une option du binaire, telle que l'aide la montre.
struct DebugOption {
    /// Le nom tel qu'il se lit sur la ligne : `--map=` (avec le `=` si l'option prend une valeur),
    /// `--crash-test` sinon.
    std::string_view name;
    /// La syntaxe complète, pour l'aide : `--map=<carte>[@<arrivée>]`.
    std::string_view syntax;
    /// Ce que l'option fait, en une phrase.
    std::string_view description;
    DebugOptionScope scope = DebugOptionScope::Live;

    /// @return Vrai si le nom finit par `=` : l'option porte une valeur.
    [[nodiscard]] bool takesValue() const noexcept {
        return !name.empty() && name.back() == '=';
    }
};

/// @return Toutes les options que le jeu lit, dans l'ordre de l'aide.
[[nodiscard]] std::span<const DebugOption> debugOptionCatalog() noexcept;

/// @brief Un mot de la ligne, séparé en nom d'option et valeur.
struct DebugArgument {
    /// `--map=` pour `--map=capital/martpart`, `--crash-test` pour `--crash-test` ; le mot entier
    /// s'il ne commence pas par `--`.
    std::string name;
    /// Ce qui suit le premier `=`, vide sinon.
    std::string value;

    friend bool operator==(const DebugArgument&, const DebugArgument&) = default;
};

/// @return @p token séparé au premier `=`, le `=` restant dans le nom — la forme du catalogue.
[[nodiscard]] DebugArgument splitDebugArgument(std::string_view token);

/// @return L'option du catalogue dont le nom est @p name, ou `nullptr`.
[[nodiscard]] const DebugOption* findDebugOption(std::string_view name) noexcept;

/**
 * @brief Découpe une ligne tapée en mots, sur les blancs, en respectant les guillemets doubles :
 *        `--screenshot="C:\Mes captures\a.png"` reste un seul mot, guillemets retirés.
 */
[[nodiscard]] std::vector<std::string> splitCommandLine(std::string_view line);

/// @return La taille de `--window-size=<L>x<H>`, ou rien si la valeur n'est pas deux entiers
///         positifs séparés par `x`.
[[nodiscard]] std::optional<std::pair<int, int>> parseWindowSize(std::string_view value);

}  // namespace hmi
