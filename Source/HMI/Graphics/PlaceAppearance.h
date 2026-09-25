// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/PieceFootprint.h"
#include "Core/Levels/TileType.h"
#include "Core/Resources/ScenePlace.h"

/**
 * @file HMI/Graphics/PlaceAppearance.h
 * @brief Ce qu'un **lieu** met sur une case : la table qui traduit un type de tuile en pièce de la
 *        planche de l'atelier des textures (`LOT-92`, `LOT-09`).
 *
 * ## La règle, décidée le 17 septembre 2026 (*décision de l'auteur*)
 *
 * - **Le sol** vient du **type de tuile** de la couche « sol », traduit par cette table : un sable
 *   donne `sand`, `sand-2` ou `sand-3`, la variante choisie par la case elle-même, toujours la
 *   même pour la même case. Le sol est dense — sept mille cases pour le Colisée —, et le nommer à
 *   la case rendrait la carte illisible et son tracé interminable.
 * - **Le relief** (murs, torches, bancs, arches, gradins) nomme sa pièce **à la case**. Le relief
 *   est rare et voulu : c'est là que l'auteur décide. À défaut, le type de la case de décor donne
 *   la pièce par cette même table.
 *
 * ## Depuis le format v4 (`LOT-EDITOR-12`)
 *
 * Toute case de couche peut nommer sa pièce (`core::TileLayer::pieces`), le sol comme le relief :
 * une carte faite à la main les nomme toutes, et la table ne sert plus que de **défaut** — celui
 * des cartes générées, et d'une case sans pièce. Lue par `loadFromFile`, la table prend aussi le
 * **manifeste** rangé à côté d'elle (`manifest.json`) : les anciens noms des pièces (`aliases`),
 * pour qu'une carte qui cite une pièce renommée la montre encore, et leurs **emprises**, pour que
 * la composition trie une pièce large au pied de son emprise (`core::footprintFootCorner`).
 */

namespace core {
struct JsonDocument;
class ScenePieceManifest;
}  // namespace core

namespace hmi {

/// @brief Catégorie d'échec de lecture (même esprit que `hmi::ArenaAppearanceError`).
enum class PlaceAppearanceError : std::uint8_t {
    None,
    FileNotFound,
    ParseError,
    UnsupportedVersion,
    MalformedStructure,
};

struct PlaceAppearanceResult;

/**
 * @brief La table d'un lieu : type de tuile → pièces, pour le sol et pour le relief.
 *
 * Logique **pure** (aucun Qt, aucun GPU, `EX-NFR-010`). Aucune lecture ne lève (`EX-NFR-040`) : un
 * fichier absent donne une table vide, et une case sans pièce ne dessine rien plutôt que de tomber
 * sur un damier sur sept mille cases.
 */
class PlaceAppearance {
public:
    /// Version la plus haute du format de table que cette lecture accepte.
    static constexpr int FORMAT_VERSION = 1;

    /// @brief Lit une table depuis son texte JSON @p json, sans manifeste voisin.
    /// @return La table et, si la lecture échoue, l'erreur nommée.
    [[nodiscard]] static PlaceAppearanceResult loadFromString(std::string_view json);

    /// @brief Lit la table, puis le manifeste rangé à côté d'elle s'il existe (voir l'en-tête).
    ///        Ses pièces sont cherchées dans le dossier propre du lieu que la table nomme.
    [[nodiscard]] static PlaceAppearanceResult loadFromFile(const std::filesystem::path& path);

    /**
     * @brief La table du lieu @p place **et de ses niveaux communs** (`LOT-124`) : ce que le jeu et
     *        l'éditeur lisent.
     *
     * Les pièces sont celles du catalogue résolu (`core::ScenePieceManifest::resolve`), chacune
     * sous son dossier d'origine. Les tables (`appearance.json`, rangées à côté des manifestes) se
     * lisent à chaque niveau et s'empilent de même : pour un type de tuile, la table **la plus
     * propre** qui le traduit l'emporte.
     *
     * Les figurines se cherchent de même, dans les `Characters/` de ses niveaux
     * (`figureDirectory`). Un lieu vide n'a que celles du monde.
     *
     * @return `FileNotFound` si aucun niveau n'a ni table, ni manifeste, ni figurine ; l'échec de
     *         lecture d'une table ou d'un manifeste, préfixé de son dossier.
     */
    [[nodiscard]] static PlaceAppearanceResult loadForPlace(
        const std::filesystem::path& assetsDirectory, std::string_view place);

    /**
     * @brief Adopte les anciens noms et les emprises des pièces d'un manifeste.
     *
     * `loadFromFile` l'appelle avec le manifeste voisin ; un test, avec le sien.
     */
    void adoptManifest(const core::ScenePieceManifest& manifest);

    /// @return Le nom courant de la pièce @p name : elle-même, ou la pièce dont c'est un ancien
    /// nom.
    [[nodiscard]] std::string_view canonicalPiece(std::string_view name) const;

    /// @return L'emprise de la pièce @p name (nom courant), 1 × 1 si le manifeste ne la connaît
    /// pas.
    [[nodiscard]] core::PieceFootprint pieceFootprint(std::string_view name) const;

    /**
     * @brief Le fichier de la pièce @p name (nom courant), **relatif à `Assets/`** : celui que le
     *        manifeste de son niveau lui donne (`Regions/…/Common/Scene/floors/floor-01.png`,
     *        `LOT-124`) ; à défaut, `<nom>.png` dans le dossier propre du lieu
     *        (`core::fallbackScenePiecePath`). Vide sans lieu.
     */
    [[nodiscard]] std::string pieceFile(std::string_view name) const;

    /**
     * @brief Le dossier, relatif à `Assets/`, de la figurine @p figure : cherchée dans les
     *        `Characters/` du lieu et de ses niveaux communs (`core::resolveFigures`, `LOT-124`),
     *        à défaut par `core::figureDirectory`.
     */
    [[nodiscard]] std::string figureDirectory(std::string_view figure) const {
        return core::figureDirectory(_figures, figure);
    }

    /// @return Le catalogue que la table a adopté, `nullptr` s'il n'y en a pas.
    [[nodiscard]] const core::ScenePieceManifest* pieceManifest() const noexcept {
        return _manifest.get();
    }

    /// @return L'identifiant du lieu (`coliseum`), vide pour une table vide.
    [[nodiscard]] const std::string& place() const noexcept {
        return _place;
    }
    [[nodiscard]] float diamondRatio() const noexcept {
        return _diamondRatio;
    }

    /**
     * @brief La plus haute élévation d'une pièce du lieu au-dessus du losange de sa case, en
     *        largeurs de case (`LOT-103`) : l'ancre d'une pièce, rapportée au losange de l'art que
     *        le manifeste déclare. Le cadrage d'un îlot la réserve au-dessus de ses cases.
     * @return 0 sans manifeste, ou pour un lieu de pièces plates.
     */
    [[nodiscard]] float maximumRise() const noexcept {
        return _maximumRise;
    }

    /**
     * @brief La pièce de sol d'une case.
     *
     * La variante est choisie par la case : `(colonne × 7 + ligne × 13) % nombre de variantes`.
     * Un tirage aléatoire ferait scintiller le sol d'une image à l'autre ; un compteur ferait
     * dépendre le sol de l'ordre de parcours.
     * @return Le nom de la pièce (`sand-2`), vide si le type n'a aucune pièce dans ce lieu.
     */
    [[nodiscard]] std::string_view floorPiece(core::TileType type, core::GridPosition cell) const;

    /// @brief La pièce de relief d'une case, même règle de variante que le sol.
    [[nodiscard]] std::string_view reliefPiece(core::TileType type, core::GridPosition cell) const;

    /**
     * @brief Le type que la table donne à @p piece : la réciproque de `floorPiece` (@p floor) ou de
     *        `reliefPiece` (`LOT-EDITOR-03`). Une pièce posée à la main garde ainsi le type dont
     *        la table la tirerait, et le type reste le sens de règle de la case (décision D3).
     * @return Le premier type, dans l'ordre de l'énumération, dont la table cite @p piece (ou la
     *         pièce dont @p piece est un ancien nom) ; `std::nullopt` si aucun ne la cite.
     */
    [[nodiscard]] std::optional<core::TileType> typeOfPiece(std::string_view piece,
                                                            bool floor) const;

    /// @return Tous les noms de pièce que la table peut rendre, triés, sans doublon — ce que le
    ///         rendu doit charger.
    [[nodiscard]] std::vector<std::string> pieces() const;

    /// @return Vrai si la table ne traduit aucun type.
    [[nodiscard]] bool empty() const noexcept {
        return _floors.empty() && _relief.empty();
    }

private:
    using Table = std::map<core::TileType, std::vector<std::string>>;

    [[nodiscard]] static PlaceAppearanceResult fromDocument(const core::JsonDocument& document);
    /// Prend, pour chaque type que @p other traduit et que cette table ignore, ses pièces.
    void fillFrom(const PlaceAppearance& other);

    std::string _place;
    float _diamondRatio = core::ARENA_DIAMOND_RATIO;
    float _maximumRise = 0.0F;
    Table _floors;
    Table _relief;
    /// Ancien nom -> nom courant (`aliases` du manifeste).
    std::map<std::string, std::string, std::less<>> _aliases;
    /// Nom courant -> emprise, pour les seules pièces plus grandes qu'une case.
    std::map<std::string, core::PieceFootprint, std::less<>> _footprints;
    /// Le fichier de chaque pièce du manifeste, relatif à `Assets/` (`pieceFile`).
    std::map<std::string, std::string, std::less<>> _files;
    /// Les figurines que le lieu peut poser, par slug (`loadForPlace`).
    core::FigureDirectories _figures;
    /// Le catalogue adopté, partagé : copier une table ne le recopie pas.
    std::shared_ptr<const core::ScenePieceManifest> _manifest;
};

/// @brief Résultat d'une lecture : la table, et ce qui a échoué.
struct PlaceAppearanceResult {
    PlaceAppearance appearance;
    PlaceAppearanceError error = PlaceAppearanceError::None;
    /// Message technique, vide en cas de succès. Pour les journaux et les tests.
    std::string message;

    [[nodiscard]] bool ok() const noexcept {
        return error == PlaceAppearanceError::None;
    }
};

}  // namespace hmi
