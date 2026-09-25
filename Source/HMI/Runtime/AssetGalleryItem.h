// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QQuickRhiItem>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQmlIntegration>

#include "HMI/Graphics/AssetGallery.h"
#include "HMI/Graphics/AssetGalleryRenderer.h"

/**
 * @file HMI/Runtime/AssetGalleryItem.h
 * @brief La surface de la galerie des assets, en Qt Quick : un **outil de débug**, pas un écran du
 *        jeu (`--screen=AssetGallery`).
 */

namespace hmi {

/**
 * @brief Surface QRhi qui dispose tous les assets livrés en blocs (emprise + une case de marge),
 *        joue toutes leurs animations, et ne garde en mémoire que ce qui est à l'écran.
 *
 * ## Qui décide quoi
 *
 * L'item, sur le fil graphique, tient la caméra (`zoom`, `offsetX`, `offsetY`), la disposition et
 * l'horloge d'animation ; il classe chaque bloc (dessiné, préchargé, déchargé) et publie au QML ce
 * que le calque d'étiquettes, la minicarte et l'inspecteur lisent. Le peintre, sur le fil de rendu,
 * reçoit dans `synchronize()` une `hmi::AssetGalleryFrame` **en valeurs** et relaie
 * `hmi::AssetGalleryRenderer`.
 *
 * Les positions publiées sont en unités d'élément : le calque QML tombe sur les blocs dessinés sans
 * rien recalculer.
 */
class AssetGalleryItem : public QQuickRhiItem {
    Q_OBJECT
    QML_NAMED_ELEMENT(AssetGalleryViewport)

    /// Unités d'élément par pixel d'art.
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY viewChanged)
    /// Position, en unités d'élément, du coin haut-gauche de la galerie.
    Q_PROPERTY(qreal offsetX READ offsetX WRITE setOffsetX NOTIFY viewChanged)
    Q_PROPERTY(qreal offsetY READ offsetY WRITE setOffsetY NOTIFY viewChanged)
    /// Côté d'une case, en unités d'élément.
    Q_PROPERTY(qreal cellSize READ cellSize NOTIFY viewChanged)
    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid NOTIFY displayChanged)
    Q_PROPERTY(bool showFootprint READ showFootprint WRITE setShowFootprint NOTIFY displayChanged)
    Q_PROPERTY(bool playing READ playing WRITE setPlaying NOTIFY playbackChanged)
    Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY playbackChanged)
    Q_PROPERTY(QColor clearColor READ clearColor WRITE setClearColor NOTIFY displayChanged)

    /// Rang du bloc sélectionné dans la disposition, ou -1 sans sélection ; écrit hors bornes, il
    /// désélectionne.
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE select NOTIFY selectionChanged)
    /// Fiche de la forme sélectionnée ; vide sans sélection.
    Q_PROPERTY(QVariantMap selected READ selected NOTIFY selectionChanged)
    /// Rang de l'image jouée par la forme sélectionnée.
    Q_PROPERTY(int selectedFrame READ selectedFrame NOTIFY frameChanged)
    /// Les autres formes du même modèle : `{index, form}`.
    Q_PROPERTY(QVariantList siblings READ siblings NOTIFY selectionChanged)

    /// Nombre de blocs de la disposition : une forme par bloc.
    Q_PROPERTY(int blocCount READ blocCount CONSTANT)
    Q_PROPERTY(int drawnCount READ drawnCount NOTIFY viewChanged)
    Q_PROPERTY(int preloadedCount READ preloadedCount NOTIFY viewChanged)
    Q_PROPERTY(int unloadedCount READ unloadedCount NOTIFY viewChanged)
    /// Les étiquettes des blocs dessinés : `{index, x, y, width, height, title, meta}`.
    Q_PROPERTY(QVariantList labels READ labels NOTIFY viewChanged)

    /// Les familles : `{title, directory, count, row}`.
    Q_PROPERTY(QVariantList bands READ bands CONSTANT)
    /// Tous les blocs, en cases, pour la minicarte : `{column, row, columns, rows}`.
    Q_PROPERTY(QVariantList minimap READ minimap CONSTANT)
    Q_PROPERTY(int layoutColumns READ layoutColumns CONSTANT)
    Q_PROPERTY(int layoutRows READ layoutRows CONSTANT)
    /// La vue, en cases.
    Q_PROPERTY(qreal viewColumn READ viewColumn NOTIFY viewChanged)
    Q_PROPERTY(qreal viewRow READ viewRow NOTIFY viewChanged)
    Q_PROPERTY(qreal viewColumns READ viewColumns NOTIFY viewChanged)
    Q_PROPERTY(qreal viewRows READ viewRows NOTIFY viewChanged)
    Q_PROPERTY(qreal ringCells READ ringCells CONSTANT)
    /// Les manifestes illisibles, nommés.
    Q_PROPERTY(QStringList errors READ errors CONSTANT)

public:
    explicit AssetGalleryItem(QQuickItem* parent = nullptr);

    [[nodiscard]] qreal zoom() const noexcept {
        return _zoom;
    }
    void setZoom(qreal zoom);
    [[nodiscard]] qreal offsetX() const noexcept {
        return _offsetX;
    }
    void setOffsetX(qreal offset);
    [[nodiscard]] qreal offsetY() const noexcept {
        return _offsetY;
    }
    void setOffsetY(qreal offset);
    [[nodiscard]] qreal cellSize() const noexcept {
        return _zoom * ASSET_GALLERY_CELL_PIXELS;
    }
    [[nodiscard]] bool showGrid() const noexcept {
        return _showGrid;
    }
    void setShowGrid(bool show);
    [[nodiscard]] bool showFootprint() const noexcept {
        return _showFootprint;
    }
    void setShowFootprint(bool show);
    [[nodiscard]] bool playing() const noexcept {
        return _playing;
    }
    void setPlaying(bool playing);
    [[nodiscard]] qreal speed() const noexcept {
        return _speed;
    }
    void setSpeed(qreal speed);
    [[nodiscard]] QColor clearColor() const noexcept {
        return _clearColor;
    }
    void setClearColor(const QColor& color);

    [[nodiscard]] int selectedIndex() const noexcept {
        return _selectedIndex;
    }
    [[nodiscard]] QVariantMap selected() const;
    [[nodiscard]] int selectedFrame() const;
    [[nodiscard]] QVariantList siblings() const;

    [[nodiscard]] int blocCount() const noexcept {
        return static_cast<int>(_layout.blocs.size());
    }
    [[nodiscard]] int drawnCount() const noexcept {
        return _drawnCount;
    }
    [[nodiscard]] int preloadedCount() const noexcept {
        return _preloadedCount;
    }
    [[nodiscard]] int unloadedCount() const noexcept {
        return blocCount() - _drawnCount - _preloadedCount;
    }
    [[nodiscard]] QVariantList labels() const {
        return _labels;
    }
    [[nodiscard]] QVariantList bands() const;
    [[nodiscard]] QVariantList minimap() const;
    [[nodiscard]] int layoutColumns() const noexcept {
        return _layout.columns;
    }
    [[nodiscard]] int layoutRows() const noexcept {
        return _layout.rows;
    }
    [[nodiscard]] qreal viewColumn() const noexcept;
    [[nodiscard]] qreal viewRow() const noexcept;
    [[nodiscard]] qreal viewColumns() const noexcept;
    [[nodiscard]] qreal viewRows() const noexcept;
    [[nodiscard]] qreal ringCells() const noexcept {
        return ASSET_GALLERY_RING_CELLS;
    }
    [[nodiscard]] QStringList errors() const {
        return _errors;
    }

    /// Déplace la vue de (@p dx, @p dy) unités d'élément : le geste du clic maintenu.
    Q_INVOKABLE void panBy(qreal dx, qreal dy);
    /// Change le zoom en gardant fixe le point (@p x, @p y) de l'élément.
    Q_INVOKABLE void zoomAt(qreal zoom, qreal x, qreal y);
    /// @return L'index du bloc sous le point (@p x, @p y), ou -1.
    Q_INVOKABLE int blocAt(qreal x, qreal y) const;
    /// Sélectionne le bloc @p index (-1 : aucun).
    Q_INVOKABLE void select(int index);
    /// Centre la vue sur le bloc @p index.
    Q_INVOKABLE void centerOn(int index);
    /// Centre la vue sur la case (@p column, @p row) : le clic sur la minicarte.
    Q_INVOKABLE void centerOnCell(qreal column, qreal row);
    /// Amène l'en-tête de la famille @p band en haut à gauche de la vue.
    Q_INVOKABLE void showBand(int band);
    /// Met en pause et avance de @p frames images de la forme sélectionnée (négatif : recule).
    Q_INVOKABLE void step(int frames);

    /// @return L'image à remettre au rendu, en pixels de la cible (@p pixelsPerItem par unité).
    [[nodiscard]] AssetGalleryFrame frameFor(float pixelsPerItem) const;

    [[nodiscard]] QQuickRhiItemRenderer* createRenderer() override;

signals:
    void viewChanged();
    void displayChanged();
    void playbackChanged();
    void selectionChanged();
    void frameChanged();

private:
    void refreshView();
    void tick();
    [[nodiscard]] const AssetGalleryEntry& entryOf(const AssetGalleryBloc& bloc) const;
    [[nodiscard]] AssetGalleryView view() const noexcept;

    AssetGalleryCatalog _catalog;
    AssetGalleryLayout _layout;
    QStringList _errors;

    qreal _zoom = 1.0;
    qreal _offsetX = 24.0;
    qreal _offsetY = 16.0;
    bool _showGrid = true;
    bool _showFootprint = true;
    bool _playing = true;
    qreal _speed = 1.0;
    QColor _clearColor{0x0e, 0x0f, 0x12};
    int _selectedIndex = -1;
    double _seconds = 0.0;

    int _drawnCount = 0;
    int _preloadedCount = 0;
    QVariantList _labels;

    QTimer _clock;
    QElapsedTimer _elapsed;
};

}  // namespace hmi
