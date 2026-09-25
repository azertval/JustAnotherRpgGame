// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Resources/AssetKey.h
 * @brief Les **clés d'assets d'entité** : comment une donnée désigne son illustration (`LOT-39`,
 *        `EX-CNT-040`, `EX-CNT-041`).
 */

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace core {

/**
 * @brief Une famille d'assets d'entité : ce qu'elle illustre, et le contrat de dimensions.
 *
 * Lue dans la **donnée** (`Assets/Entities/families.json`), jamais énumérée en C++. Le `LOT-08` a
 * fixé le vocabulaire des tuiles de terrain ; celui-ci fixe celui des entités, et il grandira —
 * sorts, PNJ, quartiers de guilde. Une énumération fermée obligerait chaque lot suivant à
 * recompiler pour ajouter une famille.
 */
struct AssetFamilyDefinition {
    /// Premier segment de la clé : `beast`, `item`, `species`…
    std::string name;
    /// Dimensions attendues du fichier image, en pixels.
    int width = 0;
    int height = 0;
    /// Les dossiers de catalogue dont **chaque entrée** attend une illustration de cette famille.
    std::vector<std::string> catalogues;
};

/// @brief La table des familles, et ce qui n'a pas pu être lu.
struct AssetFamilyTable {
    std::vector<AssetFamilyDefinition> families;
    std::vector<std::string> errors;

    /// @brief La famille de nom @p name, ou `nullptr` si elle est inconnue.
    [[nodiscard]] const AssetFamilyDefinition* find(std::string_view name) const;
    /// @brief Vrai si la table s'est lue sans erreur et déclare au moins une famille.
    [[nodiscard]] bool ok() const {
        return errors.empty() && !families.empty();
    }
};

/// @brief Charge la table des familles depuis `Assets/Entities/families.json`.
[[nodiscard]] AssetFamilyTable loadAssetFamilies(const std::filesystem::path& familiesFile);

/**
 * @brief Une clé d'asset décomposée : sa famille, et ce qu'elle désigne dedans.
 */
struct AssetKey {
    std::string family;
    std::string id;

    [[nodiscard]] friend bool operator==(const AssetKey&, const AssetKey&) noexcept = default;
};

/**
 * @brief Vrai si @p key respecte la syntaxe d'une clé d'asset.
 *
 * `famille/identifiant`, en minuscules, chiffres et tirets simples — la même expression que celle
 * du schéma JSON (`common.schema.json`, `$defs/assetKey`), pour que la donnée refusée à la
 * validation soit exactement celle que le moteur refuse.
 *
 * **Ce n'est jamais un chemin de fichier** (`EX-CNT-040`) : un chemin dans une donnée de règle lie
 * le catalogue à l'arborescence du disque, et tout déplacement de dossier casse alors des
 * créatures. La clé, elle, survit à n'importe quel rangement.
 */
[[nodiscard]] bool isValidAssetKey(std::string_view key);

/// @brief Décompose @p key. `std::nullopt` si elle est malformée — jamais devinée.
[[nodiscard]] std::optional<AssetKey> parseAssetKey(std::string_view key);

/**
 * @brief La clé **par défaut** d'une entrée : `famille/identifiant`.
 *
 * Une donnée n'a pas à écrire sa clé quand celle-ci se déduit : trois cents entrées porteraient
 * alors trois cents lignes recopiées, et la première faute de frappe donnerait une créature sans
 * image sans que rien ne l'explique. Le champ `asset` d'une donnée sert donc à **déroger** — deux
 * entrées qui partagent une illustration —, pas à répéter.
 *
 * @return La clé, ou une chaîne vide si @p family ou @p id ne respecte pas la syntaxe.
 */
[[nodiscard]] std::string defaultAssetKeyFor(std::string_view family, std::string_view id);

/// @brief Une clé attendue par le manifeste : ce qu'elle illustre, et d'où elle vient.
struct ExpectedAssetKey {
    std::string key;
    std::string family;
    /// Le fichier de catalogue qui l'attend — de quoi remonter à la donnée sans la chercher.
    std::string sourceFile;
    /// Vrai si la donnée a **dérogé** en écrivant son propre `asset`.
    bool explicitKey = false;
};

/**
 * @brief Énumère les clés qu'attendent les catalogues de @p rpgDir, selon @p families.
 *
 * Le manifeste est **dérivé**, jamais tenu à la main : un fichier de manifeste commité divergerait
 * du catalogue au premier ajout de créature, et c'est la copie oubliée qu'on lit six mois plus
 * tard. Le dériver à chaque appel coûte un balayage de dossier et ne peut pas mentir.
 *
 * @param rpgDir   Racine des catalogues (`Source/Elements/Rpg`).
 * @param families La table des familles.
 * @param errors   Reçoit les clés **orphelines** : une dérogation malformée, ou qui nomme une
 *                 famille inconnue. C'est le seul cas qui doit faire échouer une vérification —
 *                 une clé sans image, elle, est un état d'avancement (`EX-CNT-041`).
 */
[[nodiscard]] std::vector<ExpectedAssetKey> expectedAssetKeys(const std::filesystem::path& rpgDir,
                                                              const AssetFamilyTable& families,
                                                              std::vector<std::string>& errors);

}  // namespace core
