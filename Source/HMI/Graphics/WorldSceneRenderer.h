// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "Core/Combat/IsoProjection.h"
#include "Core/Math/Vector2.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/PlaceCamera.h"
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
 * @brief Le rendu QRhi d'un **lieu qu'on parcourt** (`LOT-09`) ; depuis le `LOT-118`, celui du
 *        combat aussi.
 *
 * Le découpage vaut pour une raison : tout ce qui peut casser — l'ordre de création, l'ordre de
 * libération, la recréation sur une autre interface QRhi — vit ici, où un test hors écran le fait
 * tourner sur un vrai `QRhi` sans fenêtre. L'élément Qt Quick (`hmi::WorldViewportItem`) ne relaie
 * que trois appels.
 *
 * **Les textures ne sont pas connues d'avance** : un lieu a les pièces de sa carte, et la carte
 * change au passage d'un portail. Les
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
 *
 * ## Le rendu de l'éditeur aussi (`LOT-1002`)
 *
 * Le canevas de l'éditeur, `LevelEditor --render` et les vignettes passent par cette classe : il
 * n'y a qu'un rendu d'un lieu. Ce que l'éditeur demande en plus tient en trois réglages, sans
 * effet tant qu'on ne les touche pas : un **cadrage imposé** (`setFraming`, le zoom et le
 * défilement du canevas, le cadre d'une image hors écran), une **opacité par primitive**
 * (`setQuadOpacity`, les calques masqués ou grisés, les reliefs en transparence) et les options de
 * composition (`setComposeOptions`, le plan de principe). `prepare` et `paintedBounds` composent la
 * carte **avant** l'image : le cadre d'un rendu se mesure sur ce qui sera dessiné.
 *
 * ## Les volumes (`LOT-1003`)
 *
 * Une pièce dont le manifeste cite un maillage se charge par `core::readMeshFile` et se dessine par
 * `hmi::MeshBatch`, **dans la même passe** que les images et avant elles : les maillages écrivent
 * la profondeur, les images la testent sans l'écrire et gardent entre elles l'ordre du peintre. La
 * caméra est la même (`hmi::PlaceCamera`) ; elle ramène alors la profondeur de la vue
 * (`hmi::IsoView`) à l'étendue du tampon.
 *
 * ## Les figurines en modèle (`LOT-1005`)
 *
 * Une figurine dont l'instantané nomme un modèle (`WorldFigureSnapshot::model`) se charge de même
 * — son `.glb`, la fiche de son dossier, la description du squelette qu'elle cite — et se dessine
 * par la même passe, avec la pose que la composition a calculée pour l'image. Une figurine en
 * bandes garde son chemin d'avant.
 *
 * Tout cela n'a lieu que si l'image **a** un maillage. Sans lui — toutes les cartes livrées à
 * l'ouverture du lot —, le pipeline, les sommets et la matrice sont ceux d'avant : l'image est la
 * même au pixel. La cible doit avoir un tampon de profondeur dès qu'une carte pose un volume ; les
 * surfaces du jeu et de l'éditeur en ont un, `hmi::OffscreenRhi` aussi.
 */

namespace hmi {

class MeshBatch;

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
[[nodiscard]] PlaceCamera worldCamera(const core::IsoProjection& projection, core::Vector2 focus,
                                      int pixelWidth, int pixelHeight, float tilePixels = 0.0F);

/**
 * @brief Un cadrage **imposé** (`LOT-1002`) : ce que montre le canevas de l'éditeur, ou le cadre
 *        d'une image hors écran. Sans lui, la caméra suit le héros (`worldCamera`).
 */
struct WorldFraming {
    /// Le point du monde au centre de la cible, en unités monde.
    core::Vector2 center{};
    /// Pixels de la cible par unité monde.
    float pixelsPerUnit = PlaceCamera::PIXELS_PER_UNIT;

    [[nodiscard]] bool operator==(const WorldFraming&) const = default;
};

/// @return La caméra d'un cadrage imposé, pour une cible de @p pixelWidth × @p pixelHeight.
[[nodiscard]] PlaceCamera framedCamera(const WorldFraming& framing, int pixelWidth,
                                       int pixelHeight);

/**
 * @brief Opacité supplémentaire d'une primitive, décidée par l'appelant (calques masqués, grisés,
 *        reliefs en transparence) ; 0 ou moins : la primitive n'est pas dessinée.
 */
using WorldQuadOpacity = std::function<float(const ComposedQuad&)>;

/**
 * @brief Le rectangle qu'occupe @p scene, réuni à @p base : chaque primitive et chaque maillage
 *        compte, reliefs et figurines qui montent au-dessus de leur case compris.
 *
 * Le cadre du canevas, des vignettes et de `--render` (`LOT-125`) : il se mesure sur ce qui est
 * dessiné, et non sur une marge supposée — une pièce de quatre cases de haut n'y est jamais rognée.
 */
[[nodiscard]] core::Rect composedSceneBounds(const ComposedScene& scene, const core::Rect& base);

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

    /// @brief Garantit que les ressources existent sur @p rhi, et les recrée s'il a changé.
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

    /// @brief Impose le cadrage (`LOT-1002`) ; `std::nullopt` rend la caméra qui suit le héros.
    void setFraming(std::optional<WorldFraming> framing) noexcept {
        _framing = framing;
    }

    [[nodiscard]] const std::optional<WorldFraming>& framing() const noexcept {
        return _framing;
    }

    /// @brief L'opacité par primitive de chaque image (`LOT-1002`) ; vide : 1 partout.
    void setQuadOpacity(WorldQuadOpacity opacity) {
        _opacity = std::move(opacity);
    }

    /// @brief Les options de composition de la carte ; la carte sera recomposée si elles changent.
    void setComposeOptions(WorldComposeOptions options) noexcept;

    /**
     * @brief Charge les textures et compose la carte **maintenant**, sans dessiner (`LOT-1002`).
     *
     * Ce que `render` fait à sa première image, avancé : les téléversements attendent dans le lot
     * que la prochaine image soumettra. Sans effet si rien n'a changé.
     *
     * @return `false` si les ressources n'existent pas encore (`ensureResources`).
     */
    bool prepare();

    /**
     * @brief Ce qu'occupent la carte et ses figurines, réuni à @p base (`composedSceneBounds`).
     *
     * Prépare la carte au besoin (`prepare`) ; sans ressources, rend @p base.
     */
    [[nodiscard]] core::Rect paintedBounds(const core::Rect& base);

    /// @brief Dessine une image dans @p target : efface à @p clear, puis le lieu cadré sur le
    /// héros, ou par le cadrage imposé.
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

    /// @return Les octets de mémoire graphique que tiennent les textures chargées, mipmaps
    ///         comprises, et les maillages : ce qu'un cache qui garde ce rendu compare à son
    ///         budget (`LOT-1002`).
    [[nodiscard]] std::size_t textureBytes() const noexcept;

    /// @return Les chemins que le rendu a essayé de charger, qu'il ait réussi ou non — ce qui
    ///         permet de vérifier, en test, qu'une carte ne redemande pas ce qu'elle a déjà.
    [[nodiscard]] const std::set<std::string>& requested() const noexcept {
        return _requested;
    }

private:
    /// Charge les textures de @p paths qui manquent encore. Sur le fil de rendu.
    void ensureTextures(const std::vector<std::string>& paths);
    /// Charge les maillages de @p paths qui manquent encore (`LOT-1003`). Sur le fil de rendu.
    void ensureMeshes(const std::vector<std::string>& paths);
    /// Charge les modèles de figurine de @p paths qui manquent encore (`LOT-1005`) : le maillage,
    /// son squelette, et ce que le squelette de sa fiche déclare. Sur le fil de rendu.
    void ensureFigureModels(const std::vector<std::string>& paths);
    /// Ce qui est périmé — textures de la carte et des figurines, composition — est refait. Le lot
    /// de téléversements de l'appelant est déjà déclaré (`SceneResources::setFrameUpdates`).
    void refresh(const core::IsoProjection& projection);
    /// @return La projection de la carte courante.
    [[nodiscard]] core::IsoProjection sceneProjection() const;
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
    /// Le cadrage imposé, l'opacité par primitive et les options de composition (`LOT-1002`).
    std::optional<WorldFraming> _framing;
    WorldQuadOpacity _opacity;
    WorldComposeOptions _composeOptions;
    /// La carte composée une fois (`setScene`), et l'image composée à chaque `render`.
    StaticWorldScene _statics;
    ComposedScene _composed;
    /// Chemins déjà tentés : une pièce absente ne doit pas être redemandée à chaque image.
    std::set<std::string> _requested;
    /// Les manifestes des lieux, lus une fois pour toutes les textures (audit, A6).
    ManifestCache _manifests;
    /// Les descriptions de squelette déjà lues, par silhouette ; nulle pour une qui ne se lit pas.
    std::map<std::string, std::shared_ptr<const core::SkeletonDescription>, std::less<>> _skeletons;

    QRhi* _rhi = nullptr;
    QRhiResourceUpdateBatch* _pendingUploads = nullptr;
    // Déclarées APRÈS les ressources : les textures meurent avant la grappe qui porte le pipeline.
    SceneResources _resources;
    std::vector<LoadedTexture> _loaded;
    /// Ce que pèsent les textures de `_loaded`, mipmaps comprises.
    std::size_t _textureBytes = 0;
    LoadedTexture _missing;
    /// L'aplat blanc de 1 x 1 : la texture que lient les primitives de couleur du rendu de
    /// maquette (`LOT-128`). Creee avec le reste, liberee avec lui.
    LoadedTexture _solid;
    /// La passe de maillages (`LOT-1003`) : créée avec les ressources, libérée avant elles. Ses
    /// maillages sont ceux que `_textures.meshes` désigne.
    std::unique_ptr<MeshBatch> _meshes;
    ScenePieceTextures _textures;
};

}  // namespace hmi
