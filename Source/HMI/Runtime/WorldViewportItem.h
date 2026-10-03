// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QColor>
#include <QPoint>
#include <QPointer>
#include <QQuickRhiItem>
#include <QtQmlIntegration>

#include "HMI/Runtime/WorldModel.h"

/**
 * @file HMI/Runtime/WorldViewportItem.h
 * @brief La surface de rendu d'un **lieu qu'on parcourt**, en Qt Quick (`LOT-09`).
 */

namespace hmi {

/**
 * @brief Surface QRhi de la carte courante, posée dans la vue de jeu.
 *
 * La frontière entre les fils est celle de tout `QQuickRhiItem` : le modèle et sa session vivent
 * sur le fil graphique, le dessin sur le fil de rendu, et `synchronize()` — fil graphique bloqué —
 * est le seul instant où les deux se parlent. Ne traversent que des **valeurs** : la couleur
 * d'effacement, le point suivi par la caméra, la carte si elle a changé — un
 * `hmi::WorldSceneSnapshot` partagé, immuable, jamais recopié —, et les figurines si elles ont
 * changé.
 *
 * « A changé » se compte : `WorldModel::sceneRevision` avance quand la carte est à recomposer,
 * `WorldModel::figuresRevision` à chaque pas qui change les figurines. Un pas du héros ne fait donc
 * passer que quelques figurines, et le rendu ne recompose pas la carte.
 *
 * Le cadrage est publié comme celui de l'arène (`tileWidth`, `originX`…) : le calque d'interface
 * QML posé par-dessus et le pointeur lisent **ce** cadrage, jamais un recalcul — deux cadrages
 * calculés chacun de leur côté ne tombent pas au même pixel.
 */
class WorldViewportItem : public QQuickRhiItem {
    Q_OBJECT
    // Nommé pour le QML, où le suffixe `Item` n'apprendrait rien.
    QML_NAMED_ELEMENT(WorldViewport)

    /// La vue-modèle du lieu dessiné. Nulle : seul le fond est dessiné.
    Q_PROPERTY(hmi::WorldModel* model READ model WRITE setModel NOTIFY modelChanged)
    /// Couleur d'effacement, qui vient de `Tokens.qml` comme le reste de l'identité du jeu.
    Q_PROPERTY(QColor clearColor READ clearColor WRITE setClearColor NOTIFY clearColorChanged)
    /// Définition du rendu, en pourcentage de celle de l'élément à l'écran (100 à 200). Au-delà de
    /// 100, la scène est dessinée dans un tampon plus grand, que Qt Quick réduit en le composant :
    /// le suréchantillonnage. L'anticrénelage, lui, est le `sampleCount` hérité de `QQuickRhiItem`.
    Q_PROPERTY(int renderScalePercent READ renderScalePercent WRITE setRenderScalePercent NOTIFY
                   renderScalePercentChanged)

    // --- Le cadrage, en unités d'élément -------------------------------------------------------
    /// Largeur du losange d'une case.
    Q_PROPERTY(qreal tileWidth READ tileWidth NOTIFY framingChanged)
    /// Hauteur du losange d'une case.
    Q_PROPERTY(qreal tileHeight READ tileHeight NOTIFY framingChanged)
    /// Coin haut-gauche de la boîte du losange de la case (0, 0).
    Q_PROPERTY(qreal originX READ originX NOTIFY framingChanged)
    Q_PROPERTY(qreal originY READ originY NOTIFY framingChanged)

public:
    explicit WorldViewportItem(QQuickItem* parent = nullptr);

    [[nodiscard]] WorldModel* model() const noexcept {
        return _model.data();
    }
    void setModel(WorldModel* model);

    [[nodiscard]] QColor clearColor() const noexcept {
        return _clearColor;
    }
    void setClearColor(const QColor& color);

    [[nodiscard]] int renderScalePercent() const noexcept {
        return _renderScalePercent;
    }
    /// Bornée à [100, 200] : le rendu ne se dessine jamais plus petit que l'écran.
    void setRenderScalePercent(int percent);

    [[nodiscard]] qreal tileWidth() const;
    [[nodiscard]] qreal tileHeight() const;
    [[nodiscard]] qreal originX() const;
    [[nodiscard]] qreal originY() const;

    /// @brief La case sous un point de l'élément — le geste de la souris.
    /// @return (colonne, ligne), ou (−1, −1) hors de la carte.
    Q_INVOKABLE QPoint cellAt(qreal x, qreal y) const;

    [[nodiscard]] QQuickRhiItemRenderer* createRenderer() override;

signals:
    void modelChanged();
    void clearColorChanged();
    void renderScalePercentChanged();
    void framingChanged();

protected:
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private:
    /// Le cadrage courant, et le passage des unités d'élément aux pixels de la texture.
    struct Framing;
    [[nodiscard]] Framing framing() const;

    /// La scène a changé : une nouvelle image est demandée.
    void onSceneChanged();
    /// Recale la taille du tampon de couleur sur celle de l'élément, sa fenêtre et
    /// `renderScalePercent`. À 100 %, le tampon suit l'élément de lui-même.
    void applyRenderScale();

    QPointer<WorldModel> _model;
    QMetaObject::Connection _modelChangedConnection;
    QMetaObject::Connection _modelMovedConnection;
    QMetaObject::Connection _modelFiguresConnection;
    QMetaObject::Connection _modelDestroyedConnection;
    QColor _clearColor{0x10, 0x0d, 0x0a};  ///< Nuit de pierre, jusqu'à ce que le QML en décide.
    int _renderScalePercent = 100;
};

}  // namespace hmi
