// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "Core/Ecs/AnimationClip.h"
#include "Core/Ecs/Components/Sprite.h"  // core::AtlasRegion
#include "HMI/Graphics/AssetContract.h"  // hmi::AssetValidation

/**
 * @file HMI/Graphics/AnimationCatalog.h
 * @brief Lecture et validation du format `nom-asset.anim.json`.
 */

#include "Core/Data/JsonDocument.h"

namespace hmi {

/**
 * @brief Catégorie d'échec de lecture d'une description d'animation (même esprit que
 *        `core::LevelValidationError`).
 */
enum class AnimationCatalogError {
    None,                 ///< Pas d'erreur.
    FileNotFound,         ///< Fichier absent. Cas **légitime** (image fixe), pas une anomalie.
    ParseError,           ///< JSON malformé, ou racine qui n'est pas un objet.
    UnsupportedVersion,   ///< Numéro de version supérieur à `AnimationCatalog::FORMAT_VERSION`.
    MalformedStructure,   ///< Structure inattendue (clip sans « frames », `next` inexistant…).
    IncoherentFrameSize,  ///< Taille d'image déclarée incohérente avec les dimensions du PNG.
};

/**
 * @brief Description d'animation d'un asset : disposition de sa spritesheet et ses clips.
 *
 * Donnée **pure** (`EX-NFR-010`) : la spritesheet est supposée **horizontale** (les images se
 * suivent de gauche à droite, un seul rang) — voir `AnimationCatalog::frameRegion`.
 */
struct AnimationDescription {
    /// Largeur d'une image, en pixels.
    int frameWidth = 0;
    /// Hauteur d'une image, en pixels.
    int frameHeight = 0;
    /// Clips décrits par le fichier, adressables par nom (`core::ClipSet`).
    core::ClipSet clips;
};

/**
 * @brief Résultat d'une lecture de description : soit une `AnimationDescription`, soit une
 *        **erreur** décrite (`EX-NFR-040`).
 */
struct AnimationDescriptionResult {
    std::optional<AnimationDescription> description;
    std::string error;
    AnimationCatalogError errorCode = AnimationCatalogError::None;

    /// @return true si la lecture a réussi.
    [[nodiscard]] bool ok() const noexcept {
        return description.has_value();
    }
};

/**
 * @brief Lecture, validation et traduction en région de texture du format `nom-asset.anim.json`.
 *
 * Logique **pure** (aucune dépendance GPU/Qt/fichier au-delà de la lecture elle-même) : ne réalise
 * ni chargement PNG ni mise en cache — ce sont ses appelants (`hmi::ArenaAnimationDriver`,
 * `hmi::WorldSceneRenderer`, la galerie des assets) qui le composent avec `hmi::TextureLoader`.
 */
class AnimationCatalog {
public:
    /// Version du format écrite dans le fichier, et plus élevée qui soit lue.
    static constexpr int FORMAT_VERSION = 1;

    /// Durée d'image par défaut (secondes), utilisée quand un clip omet « frameDuration ».
    static constexpr float DEFAULT_FRAME_DURATION_SECONDS = 0.1f;

    /**
     * @brief Lit une description depuis une chaîne JSON.
     * @param json Contenu JSON.
     * @return La description, ou une erreur décrite. Ne lève jamais.
     */
    [[nodiscard]] static AnimationDescriptionResult loadFromString(std::string_view json);

    /**
     * @brief Lit une description depuis un fichier.
     *
     * Un fichier **absent** produit un échec de code `FileNotFound` : c'est le cas par défaut
     * (asset non animé) et il ne doit produire **aucun avertissement** côté appelant — à
     * distinguer d'un fichier présent mais invalide (`EX-NFR-040`).
     * @param path Chemin du fichier.
     * @return La description, ou une erreur décrite. Ne lève jamais.
     */
    [[nodiscard]] static AnimationDescriptionResult loadFromFile(const std::filesystem::path& path);

private:
    /**
     * @brief Construit le résultat depuis l'enveloppe déjà lue par `core::readJsonObject`.
     *
     * JSON bien formé, racine objet et garde de version sont vérifiés **avant** d'arriver ici :
     * c'est la brique partagée du `LOT-79` (`EX-CNT-012`), et ce catalogue n'a plus à les
     * réimplémenter pour son compte.
     */
    [[nodiscard]] static AnimationDescriptionResult fromDocument(
        const core::JsonDocument& document);

public:
    /**
     * @brief Nom du fichier de description associé à un asset : même nom, extension remplacée.
     * @param assetFileName Nom logique de l'asset (ex. « water.png »).
     * @return Le nom du descripteur (ex. « water.anim.json »).
     */
    [[nodiscard]] static std::string descriptorFileName(std::string_view assetFileName);

    /**
     * @brief Valide la cohérence entre la description et les dimensions **décodées** du PNG.
     *
     * Spritesheet horizontale (`frameRegion`) : la hauteur du PNG doit égaler `frameHeight` (un
     * seul rang), sa largeur doit être un multiple positif de `frameWidth`, et chaque indice
     * d'image référencé par un clip doit désigner une position existante dans ce rang. Séparée de
     * la lecture JSON (`loadFromString`) : les dimensions réelles ne sont connues qu'après
     * décodage du PNG, en aval (`EX-REN-007`).
     * @param description   Description déjà lue.
     * @param fileName      Nom logique de l'asset, repris dans le message d'erreur.
     * @param textureWidth  Largeur décodée du PNG, en pixels.
     * @param textureHeight Hauteur décodée du PNG, en pixels.
     * @return Le verdict, avec un message exploitable si non conforme.
     */
    [[nodiscard]] static AssetValidation validateAgainstTexture(
        const AnimationDescription& description, const std::string& fileName, int textureWidth,
        int textureHeight);

    /**
     * @brief Région (en pixels) d'une image de la spritesheet, par son indice.
     *
     * Disposition horizontale (`EX-NFR-010`, fonction **pure**) : l'image d'indice @p
     * frameSheetIndex occupe le rectangle `[indice * frameWidth, 0, frameWidth, frameHeight]`.
     * Ne borne pas @p frameSheetIndex : c'est `validateAgainstTexture` qui rejette les indices
     * hors bornes en amont, une fois les dimensions réelles connues.
     * @param description    Description fournissant `frameWidth`/`frameHeight`.
     * @param frameSheetIndex Indice de l'image dans la spritesheet (0-based).
     * @return La région correspondante.
     */
    [[nodiscard]] static core::AtlasRegion frameRegion(const AnimationDescription& description,
                                                       int frameSheetIndex);
};

}  // namespace hmi
