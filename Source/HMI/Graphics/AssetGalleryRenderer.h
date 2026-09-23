// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

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
 * Même cycle de vie que `hmi::ArenaSceneRenderer` : `ensureResources` depuis `initialize()`,
 * `setFrame` depuis `synchronize()`, `render` depuis `render()`.
 */
class AssetGalleryRenderer {
public:
    static constexpr int UPLOADS_PER_FRAME = 24;
    static constexpr float EVICTION_SECONDS = 2.0f;

    explicit AssetGalleryRenderer(std::filesystem::path assetsRoot);
    ~AssetGalleryRenderer();

    AssetGalleryRenderer(const AssetGalleryRenderer&) = delete;
    AssetGalleryRenderer& operator=(const AssetGalleryRenderer&) = delete;

    bool ensureResources(QRhi* rhi);
    void release() noexcept;

    [[nodiscard]] bool created() const noexcept {
        return _resources.created();
    }
    [[nodiscard]] QRhi* rhi() const noexcept {
        return _rhi;
    }

    void setFrame(AssetGalleryFrame frame);

    void render(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target, float realDeltaSeconds,
                const float* clear);

    /// @return Le nombre de textures en mémoire (chargées ou retenues illisibles).
    [[nodiscard]] std::size_t cachedTextureCount() const noexcept {
        return _cache.size();
    }

    /// @return Vrai si une texture voulue attend encore son chargement.
    [[nodiscard]] bool loading() const noexcept {
        return _loading;
    }

    [[nodiscard]] const ComposedScene& composed() const noexcept {
        return _composed;
    }

private:
    struct CachedTexture {
        LoadedTexture texture;
        float unwantedSeconds = 0.0f;
        bool failed = false;
    };

    void updateCache(float deltaSeconds);
    void compose();

    std::filesystem::path _root;
    AssetGalleryFrame _frame;
    ComposedScene _composed;
    bool _loading = false;

    QRhi* _rhi = nullptr;
    QRhiResourceUpdateBatch* _pendingUploads = nullptr;
    // Après les ressources : les textures meurent avant la grappe qui porte le pipeline.
    SceneResources _resources;
    std::map<std::string, CachedTexture> _cache;
    LoadedTexture _white;
    LoadedTexture _missing;
};

}  // namespace hmi
