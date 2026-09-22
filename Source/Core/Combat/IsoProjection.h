// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/IsoProjection.h
 * @brief La projection isométrique du Colisée : d'une case de la grille de combat à sa place dans
 *        le monde, et retour.
 *
 * ## D'où viennent les formules
 *
 * Elles sont portées de la scène QML du Colisée (`Source/Ui/Controls/ArenaScene.ui.qml`,
 * `LOT-50`), qui les appliquait en pixels d'élément. Ici, elles s'expriment en **unités monde**
 * (`EX-ARCH-021`) : le cadrage — centrer la scène, la faire tenir dans la surface — n'est plus
 * l'affaire de la projection mais celle de la caméra (`hmi::Camera2D`), qui ne fait que déplacer
 * et agrandir, en aval, des points déjà projetés.
 *
 * ## Le repère
 *
 * Une case (c, r) occupe, en coordonnées de grille **continues**, le carré [c, c + 1[ × [r, r + 1[
 * (`EX-ARCH-020` : colonne vers la droite, ligne vers le bas). La projection est affine :
 *
 *     monde.x = originX + L/2 + (gc − gr) · L/2
 *     monde.y = originY       + (gc + gr) · H/2
 *
 * où L est la largeur du losange, H = 0,62 L sa hauteur (l'angle des tuiles de la planche, pas le
 * 2:1 classique), et (originX, originY) le coin haut-gauche de la boîte du losange de la case
 * (0, 0). Le coin de grille (c, r) tombe ainsi sur le **sommet haut** du losange de la case, (c +
 * 1, r) sur son sommet droit, (c, r + 1) sur son sommet gauche, (c + 1, r + 1) sur son sommet bas.
 *
 * La scène a son coin haut-gauche en (0, 0) du monde : la case (0, rows − 1) touche le bord
 * gauche, la case (columns − 1, 0) le bord droit, la case (columns − 1, rows − 1) le bord bas, et
 * une bande de `wallRise · L` est réservée en haut pour les murs du fond, qui montent au-dessus de
 * leur losange.
 *
 * Aucune dépendance Qt ni GPU (`EX-ARCH-012`) : tout se vérifie en test unitaire.
 */

#include <optional>

#include "Core/Levels/GridPosition.h"
#include "Core/Math/Rect.h"
#include "Core/Math/Vector2.h"

namespace core {

/// Rapport hauteur / largeur du losange : celui des tuiles de la planche du Colisée.
inline constexpr float ARENA_DIAMOND_RATIO = 0.62f;

/// Hauteur réservée aux murs au-dessus du losange du fond, en largeurs de losange.
inline constexpr float ARENA_WALL_RISE = 0.85f;

/**
 * @brief Largeur par défaut du losange, en unités monde.
 *
 * Une convention du repère, et rien d'autre : elle valait la tuile de 86 pixels de l'ancienne
 * planche du Colisée au zoom 1. Depuis le `LOT-103`, ni l'échelle de l'art — que chaque lieu
 * déclare dans son manifeste (`EX-VIS-008`) — ni la taille d'une case à l'écran — que la caméra
 * tire de la définition (`EX-REN-013`) — n'en dépendent : la changer ne changerait aucune image.
 */
inline constexpr float ARENA_TILE_WIDTH_UNITS = 5.375f;

/**
 * @brief La projection isométrique d'une grille de combat de `columns` × `rows` cases.
 *
 * Valeur immuable et bon marché à copier : elle ne tient que la taille de la grille et trois
 * nombres. Une taille négative est ramenée à zéro ; une largeur de losange non positive, à la
 * largeur par défaut.
 */
class IsoProjection {
public:
    /**
     * @param columns      Nombre de colonnes de la grille.
     * @param rows         Nombre de lignes de la grille.
     * @param tileWidth    Largeur du losange, en unités monde (> 0).
     * @param diamondRatio Rapport hauteur / largeur du losange (> 0).
     * @param wallRise     Hauteur réservée aux murs du fond, en largeurs de losange (>= 0).
     */
    IsoProjection(int columns, int rows, float tileWidth = ARENA_TILE_WIDTH_UNITS,
                  float diamondRatio = ARENA_DIAMOND_RATIO,
                  float wallRise = ARENA_WALL_RISE) noexcept;

    [[nodiscard]] int columns() const noexcept {
        return _columns;
    }
    [[nodiscard]] int rows() const noexcept {
        return _rows;
    }

    /// @return La largeur L du losange, en unités monde.
    [[nodiscard]] float tileWidth() const noexcept {
        return _tileWidth;
    }
    /// @return La hauteur H = ratio · L du losange, en unités monde.
    [[nodiscard]] float tileHeight() const noexcept {
        return _tileWidth * _diamondRatio;
    }
    /// @return La hauteur réservée aux murs du fond, en unités monde (`wallRise · L`).
    [[nodiscard]] float wallHeight() const noexcept {
        return _tileWidth * _wallRise;
    }

    /**
     * @brief Nombre de demi-losanges qui font la largeur de la scène : `columns + rows`, au moins 1
     *        (une grille vide garde une scène non dégénérée, comme la scène QML).
     */
    [[nodiscard]] int diagonals() const noexcept;

    /// @return Les dimensions de la scène, murs du fond compris, en unités monde.
    [[nodiscard]] Vector2 sceneSize() const noexcept;

    /// @return Le coin haut-gauche de la boîte du losange de la case (0, 0), en unités monde.
    [[nodiscard]] Vector2 origin() const noexcept;

    /**
     * @brief Projette un point de grille continu.
     * @param gridPoint (colonne, ligne) continues ; (c, r) entiers est le sommet haut du losange
     *                  de la case (c, r), (c + 0,5, r + 0,5) son centre.
     * @return Le point, en unités monde.
     */
    [[nodiscard]] Vector2 gridToWorld(const Vector2& gridPoint) const noexcept;

    /// @brief Inverse exact de `gridToWorld`.
    [[nodiscard]] Vector2 worldToGrid(const Vector2& world) const noexcept;

    /**
     * @brief Le centre du losange d'une case — le point d'ancrage d'une pièce posée sur la case.
     *
     * Défini pour toute case, dans la grille ou non.
     */
    [[nodiscard]] Vector2 tileToWorld(GridPosition tile) const noexcept;

    /**
     * @brief La boîte du losange d'une case (L × H), en unités monde.
     *
     * C'est le rectangle qu'occupait la brique `ArenaTile` dans la scène QML : ce que la case porte
     * (mur, figurine) déborde vers le haut à partir de ce rectangle.
     */
    [[nodiscard]] Rect tileBounds(GridPosition tile) const noexcept;

    /**
     * @brief La case dont le losange contient un point du monde.
     *
     * Un point sur une arête commune appartient à la case de plus grande colonne ou de plus grande
     * ligne (arrondi par défaut en coordonnées de grille) : les losanges pavent le plan sans trou
     * ni recouvrement.
     * @return La case, ou rien si elle est hors de la grille.
     */
    [[nodiscard]] std::optional<GridPosition> worldToTile(const Vector2& world) const noexcept;

    /// @return true si la case appartient à la grille.
    [[nodiscard]] bool contains(GridPosition tile) const noexcept;

    /**
     * @brief Profondeur de dessin d'une case : `column + row`.
     *
     * Une pièce posée sur une case de profondeur plus grande passe devant, sans autre tri.
     */
    [[nodiscard]] static constexpr int depth(GridPosition tile) noexcept {
        return tile.column + tile.row;
    }

private:
    int _columns;
    int _rows;
    float _tileWidth;
    float _diamondRatio;
    float _wallRise;
};

}  // namespace core
