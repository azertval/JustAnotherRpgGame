// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Core/Resources/MeshFile.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/SceneResources.h"
#include "HMI/Graphics/TextureLoader.h"

class QRhi;
class QRhiCommandBuffer;
class QRhiRenderTarget;
class QRhiResourceUpdateBatch;

/**
 * @file HMI/Graphics/AssetGalleryRenderer.h
 * @brief Le rendu QRhi de la galerie des assets (outil de débug), côté **fil de rendu**.
 */

namespace hmi {

class MeshBatch;

/// Un bloc à dessiner, déjà placé en pixels de la cible.
struct AssetGalleryDrawnBloc {
    /// Texture, relative à la racine des assets.
    std::string path;
    /// Coin haut-gauche du bloc, en pixels de la cible.
    float x = 0.0f;
    float y = 0.0f;
    int columns = 3;
    int rows = 3;
    int footprintColumn = 1;
    int footprintRow = 1;
    int footprintColumns = 1;
    int footprintRows = 1;
    /// Taille d'une image de la bande ; 0 : la texture entière.
    int frameWidth = 0;
    int frameHeight = 0;
    /// Rang de l'image dans la bande (pas dans le clip).
    int frameIndex = 0;
    bool selected = false;
    /// Largeur d'une case en pixels d'art (`AssetGalleryEntry::tileWidthPixels`) : l'art s'y
    /// ramène à la case de la galerie.
    int tilePixels = 1;
    /// Vrai pour un modèle de personnage (`AssetGalleryEntry::mesh`) : @ref path est son `.glb`.
    bool mesh = false;
    /// Le clip que joue le modèle, et l'instant où il en est (`assetGalleryClipSeconds`).
    std::string clip;
    float clipSeconds = 0.0f;
};

/**
 * @brief Ce que le fil graphique remet au rendu pour une image : **des valeurs**.
 *
 * La disposition et la visibilité se décident côté item (`hmi::layoutAssetGallery`,
 * `hmi::assetGalleryVisibility`) ; le rendu ne fait que charger ce qu'on lui demande de garder et
 * dessiner ce qu'on lui demande de dessiner.
 */
struct AssetGalleryFrame {
    /// Pixels de la cible par case.
    float cellPixels = 100.0f;
    /// Pixels de la cible par pixel de l'élément, zoom compris : l'épaisseur d'un trait.
    float pixelScale = 1.0f;
    std::vector<AssetGalleryDrawnBloc> drawn;
    /// Textures à garder en mémoire : celles des blocs dessinés et préchargés.
    std::vector<std::string> wanted;
    bool showGrid = true;
    bool showFootprint = true;
};

/**
 * @brief Dessine la galerie, et ne garde en mémoire que les textures voulues.
 *
 * ## Le cache
 *
 * Une texture voulue et absente est chargée, au plus `UPLOADS_PER_FRAME` par image : un grand saut
 * de caméra étale ses chargements au lieu de figer une image. Une texture qui n'est plus voulue
 * est libérée après `EVICTION_SECONDS`, pour qu'un aller-retour de la vue ne la recharge pas. Un
 * fichier illisible est retenu comme tel et dessiné en damier, sans nouvel essai à chaque image.
 *
 * ## Les modèles (`LOT-1006`)
 *
 * Un bloc dont la forme est un modèle de personnage se dessine en volume, par `hmi::MeshBatch`,
 * sous la caméra du jeu et face à elle, à l'échelle de sa case : un mètre du modèle occupe ce
 * qu'il occupe sur une carte. Son clip avance avec le temps de la galerie. Un modèle chargé reste
 * en mémoire jusqu'à `release` : la passe de maillages ne libère pas un maillage seul.
 *
 * Même cycle de vie que `hmi::WorldSceneRenderer` : `ensureResources` depuis `initialize()`,
 * `setFrame` depuis `synchronize()`, `render` depuis `render()`.
 */
class AssetGalleryRenderer {
public:
    /// Chargements de texture au plus par image.
    static constexpr int UPLOADS_PER_FRAME = 24;
    /// Délai avant de libérer une texture qui n'est plus voulue.
    static constexpr float EVICTION_SECONDS = 2.0f;

    /// @param assetsRoot Racine des assets : les chemins des textures s'y rapportent.
    explicit AssetGalleryRenderer(std::filesystem::path assetsRoot);
    /// Libère ce qui reste de ressources GPU.
    ~AssetGalleryRenderer();

    AssetGalleryRenderer(const AssetGalleryRenderer&) = delete;
    AssetGalleryRenderer& operator=(const AssetGalleryRenderer&) = delete;

    /// @brief Crée les ressources sur @p rhi, ou les recrée s'il a changé.
    /// @return Vrai si le rendu peut dessiner.
    bool ensureResources(QRhi* rhi);
    /// Libère toutes les ressources GPU, textures du cache comprises.
    void release() noexcept;

    /// @return Vrai si `ensureResources` a réussi et que rien n'a été libéré depuis.
    [[nodiscard]] bool created() const noexcept {
        return _resources.created();
    }
    /// @return Le `QRhi` sur lequel les ressources ont été créées, ou `nullptr`.
    [[nodiscard]] QRhi* rhi() const noexcept {
        return _rhi;
    }

    /// Reçoit l'image à dessiner : des valeurs, remises depuis `synchronize()`.
    void setFrame(AssetGalleryFrame frame);

    /// @brief Dessine l'image reçue sur @p target, après avoir mis le cache à jour : au plus
    ///        `UPLOADS_PER_FRAME` textures chargées, celles plus voulues depuis `EVICTION_SECONDS`
    ///        libérées (@p realDeltaSeconds les compte). @p clear est la couleur d'effacement
    ///        (RGBA).
    void render(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target, float realDeltaSeconds,
                const float* clear);

    /// @return Le nombre de textures en mémoire (chargées ou retenues illisibles).
    [[nodiscard]] std::size_t cachedTextureCount() const noexcept {
        return _cache.size();
    }

    /// @return Le nombre de modèles lus (chargés ou retenus illisibles).
    [[nodiscard]] std::size_t cachedModelCount() const noexcept {
        return _models.size();
    }

    /// @return Vrai si une texture voulue attend encore son chargement.
    [[nodiscard]] bool loading() const noexcept {
        return _loading;
    }

    /// @return La scène composée à la dernière image.
    [[nodiscard]] const ComposedScene& composed() const noexcept {
        return _composed;
    }

private:
    struct CachedTexture {
        LoadedTexture texture;
        float unwantedSeconds = 0.0f;
        bool failed = false;
    };
    /// Un modèle chargé : son maillage, son squelette et ses clips ; `mesh` nul s'il n'a pas pu
    /// se lire.
    struct CachedModel {
        MeshHandle mesh = nullptr;
        std::shared_ptr<const core::MeshRig> rig;
    };

    void updateCache(float deltaSeconds);
    void compose();
    /// Ajoute à la scène le modèle du bloc @p bloc, posé sur son emprise (@p footprintX,
    /// @p footprintY, @p footprintWidth, @p footprintHeight, en pixels), une case valant @p cell.
    void addModel(const AssetGalleryDrawnBloc& bloc, float cell, float footprintX, float footprintY,
                  float footprintWidth, float footprintHeight);

    std::filesystem::path _root;
    AssetGalleryFrame _frame;
    ComposedScene _composed;
    bool _loading = false;

    QRhi* _rhi = nullptr;
    QRhiResourceUpdateBatch* _pendingUploads = nullptr;
    // Après les ressources : les textures meurent avant la grappe qui porte le pipeline.
    SceneResources _resources;
    std::map<std::string, CachedTexture> _cache;
    /// La passe de maillages et les modèles déjà lus, par chemin.
    std::unique_ptr<MeshBatch> _meshes;
    std::map<std::string, CachedModel> _models;
    LoadedTexture _white;
    LoadedTexture _missing;
};

}  // namespace hmi
