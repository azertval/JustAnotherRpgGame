// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QColor>
#include <QImage>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "Core/Levels/TileType.h"
#include "HMI/Graphics/RenderLayer.h"
#include "HMI/Graphics/ScenePieces.h"
#include "HMI/Graphics/SceneTextureTraits.h"

/**
 * @file Editor/Ui/SceneImages.h
 * @brief Les images que le canevas peint : pièces des planches et figurines chargées à la demande,
 *        atlas des types, marqueurs, damier — en `QImage`, sans GPU (`LOT-EDITOR-02`).
 *
 * C'est le pendant, pour `QPainter`, du chargement de `hmi::WorldSceneRenderer` : **mêmes
 * règles**, dans le même ordre. Un chemin se résout en image lue sur disque ; à défaut, une
 * figurine prend son marqueur (`hmi::figureMarkerKey`, `EX-CNT-041`) ; à défaut, la composition
 * tombe sur le damier (`EX-NFR-040`). La largeur d'image d'une bande d'animation se lit dans son
 * `.anim.json`.
 *
 * L'identité de texture que la composition manipule (`hmi::TextureHandle`) est ici l'adresse d'une
 * `hmi::SceneImage` que ce cache possède : `hmi::paintComposedScene` la reconvertit. Les images
 * vivent dans des `std::map`, dont les éléments ne bougent pas quand on en ajoute : une adresse
 * donnée à une scène composée reste valide tant que le cache vit.
 *
 * Depuis le `LOT-125`, le cache est **borné** et **partagé**. Une pièce HD pèse seize fois une
 * pièce de l'ancienne planche : sans borne, un kit coûte des centaines de mébioctets, et chaque
 * onglet, chaque vignette le rechargeait. L'identité d'une image ne meurt donc plus avec ses pixels
 * : ceux d'une pièce lue sur disque peuvent être **évincés** (la moins récemment peinte d'abord) et
 * relus à la prochaine peinture ; ses dimensions, elles, restent — la composition n'en lit pas
 * d'autre. `hmi::SceneImages::shared` rend l'instance unique d'un dossier d'assets, que les
 * onglets, les vignettes et `--render` se partagent.
 */

namespace hmi {

class SceneImages;

/**
 * @brief Une image du cache : l'identité stable qu'une scène composée adresse, et ses **niveaux
 *        réduits**.
 *
 * `QPainter` n'a pas de mipmaps : réduire une pièce de 256 px à 30, même en bilinéaire, ne lit
 * qu'un texel sur huit et crénelle. Le niveau `k` est l'image réduite de moitié `k` fois, calculée
 * à la première demande (`LOT-125`) — ce que le GPU fait pour le jeu (`TextureFiltering::Smooth`).
 * Une image **engendrée** (marqueur, jeton, atlas, damier, aplat) se peint au plus proche, comme
 * en jeu, et n'a que son niveau 0.
 */
class SceneImage {
public:
    /// @return La largeur de l'image pleine, connue même quand ses pixels sont évincés.
    [[nodiscard]] int width() const noexcept {
        return _width;
    }
    /// @return Sa hauteur.
    [[nodiscard]] int height() const noexcept {
        return _height;
    }
    /// @return Vrai pour l'art peint, lissé et réduit par niveaux ; faux pour une image engendrée.
    [[nodiscard]] bool smooth() const noexcept {
        return _smooth;
    }
    /// @return Vrai pour l'aplat : un quad qui le porte se peint de sa teinte (`LOT-128`).
    [[nodiscard]] bool solid() const noexcept {
        return _solid;
    }

    /**
     * @brief Le niveau @p level de l'image, relu sur disque si ses pixels ont été évincés.
     *
     * Rendu par valeur : `QImage` est partagée implicitement, et la copie reste valide même si le
     * cache évince l'image pendant qu'on la peint. Un niveau au-delà du dernier rend le dernier (un
     * pixel de côté).
     */
    [[nodiscard]] QImage level(int level);

    /// @return Le nombre de niveaux : 1 pour une image engendrée, jusqu'à un pixel de côté sinon.
    [[nodiscard]] int levelCount() const noexcept;

    /// @return L'image pleine d'une image engendrée, jamais évincée (marqueurs, atlas).
    [[nodiscard]] const QImage& pinned() const noexcept {
        return _levels.front();
    }

private:
    friend class SceneImages;

    SceneImages* _owner = nullptr;
    std::string _path;
    std::vector<QImage> _levels;
    int _width = 0;
    int _height = 0;
    bool _smooth = false;
    bool _solid = false;
    std::list<SceneImage*>::iterator _recent;
    bool _listed = false;
};

/// @return L'image que désigne une identité de texture de ce cache.
[[nodiscard]] inline SceneImage* sceneImageOf(TextureHandle handle) noexcept {
    return static_cast<SceneImage*>(handle);
}

/// @return L'identité de texture de @p image, l'inverse de `sceneImageOf`. L'identité est opaque
///         (`void*`) : le retrait du `const` ne sert qu'au cache, qui calcule ses niveaux.
[[nodiscard]] inline TextureHandle sceneImageHandle(const SceneImage* image) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast): identité opaque, niveaux paresseux.
    return const_cast<SceneImage*>(image);
}

/// Budget par défaut des pixels évinçables, en octets (`LOT-125`, README du module).
inline constexpr std::size_t SCENE_IMAGES_DEFAULT_BUDGET_BYTES = std::size_t{256} * 1024 * 1024;

/**
 * @brief Le cache des images du canevas : ce que `hmi::paintComposedScene` peint, et ce que
 *        `hmi::composeWorldScene` adresse par chemin.
 *
 * Il ne se partage qu'entre objets du **même fil** (celui de l'interface) : le canevas, les
 * vignettes et `--render` y vivent tous.
 */
class SceneImages {
public:
    /// @param assetsDirectory Le dossier des assets (`Assets/`), où se résolvent les chemins.
    /// @param budgetBytes     Le plafond des pixels évinçables, niveaux réduits compris.
    explicit SceneImages(std::filesystem::path assetsDirectory,
                         std::size_t budgetBytes = SCENE_IMAGES_DEFAULT_BUDGET_BYTES);

    SceneImages(const SceneImages&) = delete;
    SceneImages& operator=(const SceneImages&) = delete;

    /**
     * @brief L'instance partagée du dossier @p assetsDirectory : la même pour tous les onglets, les
     *        vignettes et les préfabriqués, tant que l'un d'eux la tient.
     */
    [[nodiscard]] static std::shared_ptr<SceneImages> shared(
        const std::filesystem::path& assetsDirectory);

    /// @brief Charge les images de @p paths qui ne l'ont pas encore été (une seule tentative par
    ///        chemin : une pièce absente n'est pas redemandée à chaque image).
    void ensure(const std::vector<std::string>& paths);

    /// @return Les textures chargées, adressées par chemin, et le damier.
    [[nodiscard]] const ScenePieceTextures& textures() const noexcept {
        return _textures;
    }

    /// @return L'identité d'une teinte unie : un quad qui la porte se peint en aplat de sa teinte.
    [[nodiscard]] static TextureHandle solid() noexcept;

    /// @return L'atlas procédural des types de tuile (la vue à plat).
    [[nodiscard]] const SceneImage& atlas() const noexcept {
        return _atlas;
    }

    /// @return La couleur d'un type de tuile : le centre de sa case dans l'atlas.
    [[nodiscard]] static QColor tileColor(core::TileType type);

    /// @return Le marqueur de la clé d'asset @p key (`LOT-39`), `nullptr` si la clé est refusée.
    [[nodiscard]] const SceneImage* marker(const std::string& key);

    /// @return L'image du dossier des assets à @p path, chargée au besoin ; `nullptr` si absente.
    [[nodiscard]] SceneImage* image(const std::string& path);

    /// @return Les octets de pixels évinçables tenus en mémoire, niveaux réduits compris.
    [[nodiscard]] std::size_t residentBytes() const noexcept {
        return _resident;
    }
    /// @return Le plafond de ces octets.
    [[nodiscard]] std::size_t budgetBytes() const noexcept {
        return _budget;
    }
    /// @return Le nombre de manifestes lus sur disque depuis la création du cache.
    [[nodiscard]] std::size_t manifestReads() const noexcept {
        return _manifests.size();
    }

private:
    friend class SceneImage;

    /// Une image lue ou relue : comptée, placée en tête des plus récentes, et le budget tenu.
    void touch(SceneImage& image);
    void account(std::ptrdiff_t bytes);
    void evictBeyondBudget(const SceneImage& keep);
    [[nodiscard]] QImage readFile(const std::string& path) const;
    /// Une image engendrée : peinte au plus proche, jamais évincée, un seul niveau.
    static void pin(SceneImage& target, QImage image);

    std::filesystem::path _directory;
    std::size_t _budget;
    std::size_t _resident = 0;
    std::list<SceneImage*> _recent;
    std::map<std::string, SceneImage> _images;
    std::map<std::string, SceneImage> _markers;
    std::set<std::string> _requested;
    ManifestCache _manifests;
    SceneImage _missing;
    SceneImage _atlas;
    ScenePieceTextures _textures;
};

}  // namespace hmi
