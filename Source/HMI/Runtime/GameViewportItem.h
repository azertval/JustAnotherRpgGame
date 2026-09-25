// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QColor>
#include <QQuickRhiItem>
#include <QtQmlIntegration>

/**
 * @file HMI/Runtime/GameViewportItem.h
 * @brief La surface de rendu du **jeu**, en Qt Quick (`LOT-86`).
 */

namespace hmi {

/**
 * @brief Surface QRhi du jeu, composée avec le reste de l'interface QML.
 *
 * ## Le jumeau Qt Quick de `QRhiWidget`
 *
 * L'éditeur dessine sa scène dans un `QRhiWidget` ; le jeu la dessine ici. Les deux rendent dans
 * une texture d'appui que leur hôte compose : la cible technique ne change pas — **Direct3D 11 par
 * défaut sous Windows, au travers de QRhi** (`EX-ARCH-050`, `EX-REN-002`) — seul l'hôte change.
 *
 * ## Ce qui change vraiment, et qu'il ne faut pas se cacher
 *
 * `QRhiWidget` peint sur le fil graphique. `QQuickRhiItem` peint sur le **fil de rendu**, et c'est
 * la seule différence qui compte. Toute donnée que la simulation produit doit donc traverser
 * `synchronize()`, appelée pendant que le fil graphique est **bloqué** — le seul instant où les
 * deux fils peuvent se parler sans verrou.
 *
 * C'est pour cela que la composition de scène (`hmi::ComposedScene`, structure **pure et sans
 * GPU**, `EX-NFR-004`/`005`) est exactement le bon objet de transfert : le fil graphique la
 * remplit, `synchronize()` la remet, le fil de rendu la soumet. La frontière que le projet s'était
 * donnée pour tester le rendu sans GPU sert ici une seconde fois.
 *
 * ## Ce qu'il ne fait pas encore
 *
 * Il **n'affiche aucune scène** : `Source/Elements/Levels/` ne porte que la carte de l'arène
 * (`LOT-50`), que l'écran du Colisée dessine lui-même en QML, et le contenu du RPG arrive avec un
 * lot ultérieur. Il établit la plomberie — création
 * du `QRhi`, passe de rendu, couleur d'effacement — qui était le vrai risque du portage, et laisse
 * la scène à brancher quand elle existera. Bâtir une session de jeu autour d'un niveau qui n'existe
 * pas produirait du code que rien ne peut vérifier.
 */
class GameViewportItem : public QQuickRhiItem {
    Q_OBJECT
    // Nommé pour le QML, où le suffixe `Item` n'apprendrait rien : tout y est un élément. La classe
    // C++ le garde, comme `WorldViewportItem` : c'est la classe `QQuickRhiItem`, pas l'élément
    // QML.
    QML_NAMED_ELEMENT(GameViewport)

    /// Couleur d'effacement. Exposée au QML pour venir de `Tokens.qml` comme le reste : la surface
    /// de rendu appartient à l'identité du jeu, et une couleur écrite ici lui échapperait.
    Q_PROPERTY(QColor clearColor READ clearColor WRITE setClearColor NOTIFY clearColorChanged)

public:
    explicit GameViewportItem(QQuickItem* parent = nullptr);

    [[nodiscard]] QColor clearColor() const noexcept {
        return _clearColor;
    }
    void setClearColor(const QColor& color);

    [[nodiscard]] QQuickRhiItemRenderer* createRenderer() override;

signals:
    void clearColorChanged();

private:
    QColor _clearColor{0xd0, 0xc0, 0xa0};  ///< Parchemin vieilli, jusqu'à ce que le QML en décide.
};

}  // namespace hmi
