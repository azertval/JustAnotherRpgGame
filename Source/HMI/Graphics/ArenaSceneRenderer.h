// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "Core/Combat/BattleGrid.h"  // core::CombatantId
#include "Core/Combat/IsoProjection.h"
#include "HMI/Graphics/ArenaAnimationDriver.h"
#include "HMI/Graphics/ArenaAppearanceCatalog.h"
#include "HMI/Graphics/ArenaSceneComposer.h"
#include "HMI/Graphics/Camera2D.h"
#include "HMI/Graphics/ComposedScene.h"
#include "HMI/Graphics/SceneResources.h"
#include "HMI/Graphics/TextureLoader.h"
#include "HMI/Graphics/WorldSceneComposer.h"

class QRhi;
class QRhiCommandBuffer;
class QRhiRenderTarget;
class QRhiResourceUpdateBatch;

/**
 * @file HMI/Graphics/ArenaSceneRenderer.h
 * @brief Le rendu QRhi de la scène de combat du Colisée, côté **fil de rendu** (`LOT-86` Phase 5).
 */

namespace hmi {

/**
 * @brief Le cadrage du Colisée : la scène entière, centrée, dans une surface de @p pixelWidth ×
 *        @p pixelHeight pixels physiques.
 *
 * La **seule** géométrie de l'arène à l'écran : le rendu la soumet, et l'élément Qt Quick
 * (`hmi::ArenaViewportItem`) s'en sert pour poser le calque d'interface et traduire le pointeur en
 * case. Deux cadrages recalculés chacun de leur côté ne tombent jamais au même pixel — le zoom est
 * arrondi à l'entier (`Camera2D::fitZoom`).
 */
[[nodiscard]] Camera2D arenaCamera(const core::IsoProjection& projection, int pixelWidth,
                                   int pixelHeight);

/**
 * @brief Ce qui dessine le Colisée : ressources GPU, textures de la planche, animation des
 *        figurines, et la passe qui soumet la scène composée.
 *
 * ## Pourquoi pas directement dans l'item Qt Quick
 *
 * `hmi::ArenaViewportItem` n'est qu'un hôte : un `QQuickRhiItemRenderer` ne s'exécute que dans une
 * fenêtre Qt Quick, et ses accesseurs (`rhi()`, `renderTarget()`) n'existent pas hors d'elle. Tout
 * ce qui peut casser — l'ordre de création, l'ordre de libération, la recréation sur une autre
 * interface QRhi — vit donc ici, où un test hors écran le fait tourner sur un vrai `QRhi`
 * Direct3D 11 sans fenêtre (`test_arena_scene_renderer.cpp`). L'item se contente de relayer ses
 * trois appels.
 *
 * ## Les trois temps, et les deux fils
 *
 * 1. `ensureResources(rhi)` — **fil de rendu**, depuis `initialize()`. Crée les ressources si elles
 *    n'existent pas, ou les **libère puis recrée** si l'interface QRhi a changé (fenêtre changée,
 *    graphe de scène reconstruit) : une ressource de l'ancienne interface ne doit plus servir.
 * 2. `setSnapshot(snapshot)` — depuis `synchronize()`, **fil graphique bloqué**. Reçoit la scène
 *    **en valeurs** (`hmi::ArenaSceneSnapshot`) : aucun pointeur vers la session ne franchit la
 *    frontière entre les fils.
 * 3. `render(...)` — **fil de rendu**. Avance l'animation, compose, trie, soumet.
 *
 * ## L'ordre de libération
 *
 * Les textures de la planche et le damier meurent **avant** `hmi::SceneResources`, qui libère
 * ensuite sa propre grappe dans l'ordre qu'elle fixe (le lot de sprites, qui porte le pipeline, en
 * dernier). Le lot de téléversements de création, s'il n'a jamais été soumis, est rendu à QRhi. Le
 * destructeur fait la même chose : le renderer de Qt Quick est détruit sur le fil de rendu, son
 * `QRhi` encore vivant.
 */
class ArenaSceneRenderer {
public:
    /**
     * @param coliseumDirectory Dossier de la planche du Colisée (`Assets/Coliseum`) : manifeste,
     *                          pièces et `.anim.json`. Absent ou illisible : catalogue vide, rien à
     *                          dessiner que le fond — jamais une erreur bloquante (`EX-NFR-040`).
     *                          À côté de lui, `Assets/Npc/manifest.json` (l'atelier des PNJ,
     *                          LOT-91) peut mettre un PNJ à la place d'un héros
     *                          (`ArenaAppearanceCatalog::applyNpcManifest`) ; absent : rien.
     * @param productionMap Utiliser la carte du catalogue comme décor de combat.
     */
    explicit ArenaSceneRenderer(std::filesystem::path coliseumDirectory,
                                bool productionMap = false);
    ~ArenaSceneRenderer();

    ArenaSceneRenderer(const ArenaSceneRenderer&) = delete;
    ArenaSceneRenderer& operator=(const ArenaSceneRenderer&) = delete;

    /**
     * @brief Garantit que les ressources existent sur @p rhi.
     *
     * Sans effet si elles y existent déjà ; libère puis recrée si elles existent sur une **autre**
     * interface. Les pixels des textures partent dans un lot soumis par le prochain `render`.
     * @return Vrai si les ressources sont prêtes sur @p rhi (faux si @p rhi est nul).
     */
    bool ensureResources(QRhi* rhi);

    /// Libère toutes les ressources GPU, dans l'ordre. Sans effet si rien n'est créé.
    void release() noexcept;

    /// @return Vrai si les ressources existent.
    [[nodiscard]] bool created() const noexcept {
        return _resources.created();
    }

    /// @return L'interface QRhi des ressources, `nullptr` si rien n'est créé.
    [[nodiscard]] QRhi* rhi() const noexcept {
        return _rhi;
    }

    /**
     * @brief Remplace la scène à dessiner.
     *
     * Un combattant qui apparaît commence son repos (`ArenaFigureAction::Idle`) ; un combattant qui
     * disparaît quitte le pilotage d'animation. Ne touche pas au GPU : appelable avant
     * `ensureResources`.
     */
    void setSnapshot(ArenaSceneSnapshot snapshot);

    /// @return La scène actuellement dessinée.
    [[nodiscard]] const ArenaSceneSnapshot& snapshot() const noexcept {
        return _snapshot;
    }

    /**
     * @brief Dessine une image dans @p target : efface à @p clear, puis la scène cadrée entière.
     *
     * Sans effet si les ressources n'existent pas. À appeler hors de toute passe ouverte.
     * @param commandBuffer    Tampon de commandes de l'image.
     * @param target           Cible de rendu ; sa taille en pixels fixe le cadrage.
     * @param realDeltaSeconds Temps écoulé depuis l'image précédente, pour l'animation.
     * @param clear            Couleur d'effacement : quatre composantes RGBA `[0, 1]`.
     */
    void render(QRhiCommandBuffer* commandBuffer, QRhiRenderTarget* target, float realDeltaSeconds,
                const float* clear);

    /// @return Vrai si une figurine est à l'écran : l'image suivante doit être demandée.
    [[nodiscard]] bool animating() const noexcept {
        return !_snapshot.figures.empty();
    }

    /// @return La dernière scène composée et soumise.
    [[nodiscard]] const ComposedScene& composed() const noexcept {
        return _composed;
    }

    /// @return Les textures liables : chemins chargés et damier de repli.
    [[nodiscard]] const ArenaSceneTextures& textures() const noexcept {
        return _textures;
    }

    /// @return Le catalogue d'apparence lu à la construction.
    [[nodiscard]] const ArenaAppearanceCatalog& catalog() const noexcept {
        return _catalog;
    }

private:
    void loadTextures();
    void loadBattlefield();
    std::optional<WorldSceneSnapshot> _battlefield;
    core::GridPosition _battlefieldOrigin{};
    ScenePieceTextures _battlefieldTextures;
    ComposedScene _battlefieldScene;

    std::filesystem::path _directory;
    ArenaAppearanceCatalog _catalog;
    /// Largeur d'image des bandes dessinées (`idle.png`, `death.png`), par chemin sous
    /// `_directory`, lue avec leurs clips : `ArenaTexture::frameWidth` à la création des textures.
    std::map<std::string, int> _bandFrameWidths;
    ArenaAnimationDriver _animation;
    ArenaSceneSnapshot _snapshot;
    /// Les combattants dont l'animation est pilotée : ceux de l'instantané courant.
    std::set<core::CombatantId> _animated;
    /// Tampon de composition réutilisé d'une image à l'autre.
    ComposedScene _composed;

    QRhi* _rhi = nullptr;
    /// Lot des téléversements de création, en attente du premier `render` — ou de `release`.
    QRhiResourceUpdateBatch* _pendingUploads = nullptr;
    // Déclarées APRÈS les ressources : à la destruction implicite comme dans `release`, les
    // textures meurent avant la grappe qui porte le pipeline.
    SceneResources _resources;
    std::vector<LoadedTexture> _loaded;
    LoadedTexture _missing;
    /// Identités opaques des textures ci-dessus, par chemin, pour la composition.
    ArenaSceneTextures _textures;
};

}  // namespace hmi
