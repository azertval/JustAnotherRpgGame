// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QColor>
#include <QImage>
#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "Core/Levels/TileType.h"
#include "HMI/Graphics/RenderLayer.h"
#include "HMI/Graphics/ScenePieces.h"

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
 * L'identité de texture que la composition manipule (`hmi::TextureHandle`) est ici l'adresse d'un
 * `QImage` que ce cache possède : `hmi::paintComposedScene` la reconvertit. Les images vivent dans
 * des `std::map`, dont les éléments ne bougent pas quand on en ajoute : une adresse donnée à une
 * scène composée reste valide tant que le cache vit.
 */

namespace hmi {

/// @return L'image que désigne une identité de texture de ce cache.
[[nodiscard]] inline const QImage* sceneImageOf(TextureHandle handle) noexcept {
    return static_cast<const QImage*>(handle);
}

/// @return L'identité de texture de @p image, l'inverse de `sceneImageOf`. L'identité est opaque
///         (`void*`) et personne n'écrit à travers : le retrait du `const` est sans effet.
[[nodiscard]] inline TextureHandle sceneImageHandle(const QImage* image) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast): identité opaque, jamais écrite.
    return const_cast<QImage*>(image);
}

/**
 * @brief Le cache des images du canevas : ce que `hmi::paintComposedScene` peint, et ce que
 *        `hmi::composeWorldScene` adresse par chemin.
 */
class SceneImages {
public:
    /// @param assetsDirectory Le dossier des assets (`Assets/`), où se résolvent les chemins.
    explicit SceneImages(std::filesystem::path assetsDirectory);

    SceneImages(const SceneImages&) = delete;
    SceneImages& operator=(const SceneImages&) = delete;

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
    [[nodiscard]] const QImage& atlas() const noexcept {
        return _atlas;
    }

    /// @return La couleur d'un type de tuile : le centre de sa case dans l'atlas.
    [[nodiscard]] static QColor tileColor(core::TileType type);

    /// @return Le marqueur de la clé d'asset @p key (`LOT-39`), `nullptr` si la clé est refusée.
    [[nodiscard]] const QImage* marker(const std::string& key);

    /// @return L'image du dossier des assets à @p path, chargée au besoin ; `nullptr` si absente.
    [[nodiscard]] const QImage* image(const std::string& path);

private:
    [[nodiscard]] int bandFrameWidth(const std::string& path) const;

    std::filesystem::path _directory;
    std::map<std::string, QImage> _images;
    std::map<std::string, QImage> _markers;
    std::set<std::string> _requested;
    QImage _missing;
    QImage _atlas;
    ScenePieceTextures _textures;
};

}  // namespace hmi
