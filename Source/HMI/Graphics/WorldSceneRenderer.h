// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "Core/Combat/IsoProjection.h"
#include "Core/Math/Vector2.h"
#include "HMI/Graphics/Camera2D.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/ScenePieces.h"
#include "HMI/Graphics/SceneResources.h"
#include "HMI/Graphics/SceneTextureTraits.h"
#include "HMI/Graphics/StaticWorldScene.h"
#include "HMI/Graphics/TextureLoader.h"
#include "HMI/Graphics/WorldSceneComposer.h"

class QRhi;
class QRhiCommandBuffer;
class QRhiRenderTarget;
class QRhiResourceUpdateBatch;

/**
 * @file HMI/Graphics/WorldSceneRenderer.h
 * @brief Le rendu QRhi d'un **lieu qu'on parcourt** : le jumeau d'`hmi::ArenaSceneRenderer`
 *        (`LOT-09`).
 *
 * Même découpage, pour la même raison : tout ce qui peut casser — l'ordre de création, l'ordre de
 * libération, la recréation sur une autre interface QRhi — vit ici, où un test hors écran le fait
 * tourner sur un vrai `QRhi` sans fenêtre. L'élément Qt Quick (`hmi::WorldViewportItem`) ne relaie
 * que trois appels.
 *
 * Une différence avec l'arène : **les textures ne sont pas connues d'avance**. Une arène a son
 * catalogue ; un lieu a les pièces de sa carte, et la carte change au passage d'un portail. Les
 * textures se chargent donc quand une carte arrive — décodées sur tous les cœurs, créées sur le fil
 * de rendu, dans le lot de l'image.
 *
 * ## Ce qu'une image refait, et ce qu'elle ne refait pas
 *
 * La carte (`setScene`) ne change qu'en entrant ou quand un drapeau fait paraître ou disparaître
 * quelque chose : elle se compose **une fois**, dans une `hmi::StaticWorldScene`. Une image ne
 * prend que ce que la caméra montre, et y fusionne les figurines (`setFigures`) — le héros qui
 * marche, les PNJ qui respirent. Le coût d'une image dépend donc de ce qu'on voit, pas de la taille
 * de la carte (audit de l'affichage d'un lieu, `Planning/standards/audit-affichage-lieu.md`).
 */

namespace hmi {

/**
 * @brief Le cadrage d'un lieu : la caméra **suit** le héros, et ne sort pas de la carte.
 *
 * Un lieu ne tient pas dans un écran : le cadrage entier de l'arène ne convient pas. Le facteur est
 * **libre** et fixé par la définition (`EX-REN-013`) : une case occupe à l'écran la hauteur de la
 * surface divisée par 10,8 (`hmi::worldTilePixels`), si bien que 1080p et 2160p cadrent la même
 * étendue de monde. La caméra se centre ensuite sur le point suivi, puis se ramène dans la scène :
 * sur un axe où la scène est plus petite que la vue, elle reste centrée, faute de quoi la carte
 * collerait à un bord.
 *
 * @param projection  La projection du lieu.
 * @param focus       Le point suivi, en unités monde (le héros).
 * @param pixelWidth  Largeur de la surface, en pixels physiques.
 * @param pixelHeight Hauteur de la surface.
 * @param tilePixels  Largeur d'une case à l'écran, en pixels, quand l'appelant l'impose (l'image
 *                    d'un îlot, `hmi::renderCityBlock`) ; 0 : celle que donne la définition.
 */
[[nodiscard]] Camera2D worldCamera(const core::IsoProjection& projection, core::Vector2 focus,
                                   int pixelWidth, int pixelHeight, float tilePixels = 0.0F);

/**
 * @brief Ce qui dessine un lieu : ressources GPU, textures des planches, et la passe qui soumet la
 *        scène composée.
 *
 * Les trois temps sont ceux de l'arène : `ensureResources(rhi)` sur le fil de rendu,
 * `setSnapshot(...)` depuis `synchronize()` (fil graphique bloqué, **valeurs** seulement),
 * `render(...)` sur le fil de rendu.
 */
class WorldSceneRenderer {
public:
    /**
     * @param assetsDirectory Dossier des assets (`Source/Elements/Assets`) : les chemins de
     *                        l'instantané (`Scene/<lieu>/<pièce>.png`, `Npc/<slug>/<bande>.png`)
     *                        s'y résolvent. Absent : rien à dessiner que le fond, jamais une
     *                        erreur bloquante (`EX-NFR-040`).
     */
    explicit WorldSceneRenderer(std::filesystem::path assetsDirectory);
    ~WorldSceneRenderer();

    WorldSceneRenderer(const WorldSceneRenderer&) = delete;
    WorldSceneRenderer& operator=(const WorldSceneRenderer&) = delete;

    /// @brief Garantit que les ressources existent sur @p rhi (voir `ArenaSceneRenderer`).
    bool ensureResources(QRhi* rhi);

    /// Libère toutes les ressources GPU, dans l'ordre. Sans effet si rien n'est créé.
    void release() noexcept;

    [[nodiscard]] bool created() const noexcept {
        return _resources.created();
    }

    [[nodiscard]] QRhi* rhi() const noexcept {
        return _rhi;
    }

    /**
     * @brief Remplace la scène et ses figurines d'un coup (`snapshot.figures`). Ne touche pas au
     *        GPU : appelable avant les ressources.
     */
    void setSnapshot(WorldSceneSnapshot snapshot);

    /**
     * @brief Remplace la **carte** à dessiner : elle sera recomposée à la prochaine image.
     *
     * Partagée, jamais recopiée : le jeu la garde tant qu'elle ne change pas
     * (`hmi::WorldPlay::scene`). Ses figurines éventuelles ne sont pas dessinées ; ce sont celles
     * de `setFigures`.
     */
    void setScene(std::shared_ptr<const WorldSceneSnapshot> scene);

    /// @brief Remplace les figurines de l'image, héros compris. Ne recompose pas la carte.
    void setFigures(std::vector<WorldFigureSnapshot> figures);

    /// @return La carte dessinée (vide tant qu'aucune n'a été donnée).
    [[nodiscard]] const WorldSceneSnapshot& snapshot() const noexcept {
        return *_scene;
    }

    /// @return Les figurines de l'image.
    [[nodiscard]] const std::vector<WorldFigureSnapshot>& figures() const noexcept {
        return _figures;
    }

    /// @brief Le point suivi par la caméra, en cases (position continue du héros).
    void setFocus(core::Vector2 focusCells) noexcept {
        _focus = focusCells;
    }

    [[nodiscard]] core::Vector2 focus() const noexcept {
        return _focus;
    }

    /// @brief Impose la largeur d'une case à l'écran, en pixels ; 0 rend la règle de la
    ///        définition (`hmi::worldCamera`).
    void setTilePixels(float tilePixels) noexcept {
        _tilePixels = tilePixels;
    }

    /// @brief Dessine une image dans @p target : efface à @p clear, puis le lieu cadré sur le
    /// héros.
    void render(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target, const float* clear);

    /// @return La dernière image composée : ce que la caméra montrait, figurines comprises.
    [[nodiscard]] const ComposedScene& composed() const noexcept {
        return _composed;
    }

    /// @return La carte composée une fois, toute entière.
    [[nodiscard]] const StaticWorldScene& statics() const noexcept {
        return _statics;
    }

    [[nodiscard]] const ScenePieceTextures& textures() const noexcept {
        return _textures;
    }

    /// @return Les chemins que le rendu a essayé de charger, qu'il ait réussi ou non — ce qui
    ///         permet de vérifier, en test, qu'une carte ne redemande pas ce qu'elle a déjà.
    [[nodiscard]] const std::set<std::string>& requested() const noexcept {
        return _requested;
    }

private:
    /// Charge les textures de @p paths qui manquent encore. Sur le fil de rendu.
    void ensureTextures(const std::vector<std::string>& paths);
    /// Le marqueur d'une figurine sans image (`hmi::figureMarkerKey`), rien pour une autre piece.
    [[nodiscard]] std::optional<LoadedTexture> figureMarker(const std::string& path);

    std::filesystem::path _directory;
    /// La carte, partagée ; jamais nulle (une carte vide au départ).
    std::shared_ptr<const WorldSceneSnapshot> _scene;
    std::vector<WorldFigureSnapshot> _figures;
    /// La carte a changé : ses textures et sa composition sont à refaire à la prochaine image.
    bool _sceneDirty = true;
    /// Les figurines ont changé : leurs bandes sont peut-être à charger.
    bool _figuresDirty = true;
    core::Vector2 _focus{};
    float _tilePixels = 0.0F;
    /// La carte composée une fois (`setScene`), et l'image composée à chaque `render`.
    StaticWorldScene _statics;
    ComposedScene _composed;
    /// Chemins déjà tentés : une pièce absente ne doit pas être redemandée à chaque image.
    std::set<std::string> _requested;
    /// Les manifestes des lieux, lus une fois pour toutes les textures (audit, A6).
    ManifestCache _manifests;

    QRhi* _rhi = nullptr;
    QRhiResourceUpdateBatch* _pendingUploads = nullptr;
    // Déclarées APRÈS les ressources : les textures meurent avant la grappe qui porte le pipeline.
    SceneResources _resources;
    std::vector<LoadedTexture> _loaded;
    LoadedTexture _missing;
    /// L'aplat blanc de 1 x 1 : la texture que lient les primitives de couleur du rendu de
    /// maquette (`LOT-128`). Creee avec le reste, liberee avec lui.
    LoadedTexture _solid;
    ScenePieceTextures _textures;
};

}  // namespace hmi
