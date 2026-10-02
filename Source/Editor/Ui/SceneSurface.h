// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QColor>
#include <QRhiWidget>
#include <filesystem>

#include "Core/Math/Rect.h"
#include "HMI/Graphics/WorldSceneRenderer.h"

/**
 * @file Editor/Ui/SceneSurface.h
 * @brief La surface où le canevas de l'éditeur dessine le lieu : un `QRhiWidget` qui ne fait que
 *        porter `hmi::WorldSceneRenderer`, le rendu du jeu (`LOT-1002`).
 *
 * Elle est au canevas ce que `hmi::WorldViewportItem` est au jeu : un hôte, qui relaie trois
 * appels. Tout ce qui décide de l'image — la carte, les figurines, le cadrage, l'opacité des
 * calques — se règle sur le rendu (`renderer()`), et le canevas (`hmi::EditorViewport`) le fait
 * depuis ses gestes. La surface ne reçoit ni la souris ni le clavier : elle est **sous** la vue du
 * canevas, qui peint les aides d'édition par-dessus.
 *
 * `QRhiWidget` dessine sur le fil de l'interface : le rendu se règle et se prépare donc sans
 * verrou, entre deux images.
 */

namespace hmi {

/// @brief La surface de rendu du canevas : le lieu, par le rendu du jeu.
class SceneSurface final : public QRhiWidget {
    Q_OBJECT

public:
    /// @param assetsDirectory Le dossier des assets, où les chemins de la carte se résolvent.
    /// @param parent          Le canevas.
    explicit SceneSurface(std::filesystem::path assetsDirectory, QWidget* parent = nullptr);
    ~SceneSurface() override;
    SceneSurface(const SceneSurface&) = delete;
    SceneSurface& operator=(const SceneSurface&) = delete;

    /// @return Le rendu : sa carte, ses figurines, son cadrage se règlent ici.
    [[nodiscard]] WorldSceneRenderer& renderer() noexcept {
        return _renderer;
    }

    /// Le fond, autour de la carte.
    void setClearColor(const QColor& color);

    /// Une surface **vide** ne dessine que son fond : la vue à plat se peint par-dessus.
    void setBlank(bool blank);
    [[nodiscard]] bool blank() const noexcept {
        return _blank;
    }

    /**
     * @brief Ce qu'occupent la carte et ses figurines (`hmi::WorldSceneRenderer::paintedBounds`) ;
     *        @p base tant que la surface n'a pas d'interface de rendu — avant d'être montrée.
     */
    [[nodiscard]] core::Rect paintedBounds(const core::Rect& base);

signals:
    /// La surface a reçu une interface de rendu, ou en a changé : les textures sont à recharger, et
    /// ce que la carte occupe se mesure à nouveau.
    void resourcesChanged();

protected:
    void initialize(QRhiCommandBuffer* commandBuffer) override;
    void render(QRhiCommandBuffer* commandBuffer) override;
    void releaseResources() override;

private:
    WorldSceneRenderer _renderer;
    QColor _clear;
    bool _blank = false;
};

}  // namespace hmi
