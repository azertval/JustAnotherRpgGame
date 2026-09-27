// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <deque>
#include <vector>

#include "Core/Math/Vector2.h"

/**
 * @file Core/World/FollowTrail.h
 * @brief La trace du meneur, que les suiveurs du groupe parcourent à distance (`LOT-138`).
 *
 * ## Pourquoi une trace, et pas une poursuite
 *
 * Un suiveur qui marcherait **vers** le meneur, en ligne droite, buterait sur le premier coin de
 * mur que le meneur a contourné — et y resterait. Un suiveur qui chercherait son chemin à chaque
 * pas coûterait une recherche par membre et par image, et couperait les angles autrement que le
 * meneur. Le suiveur **met ses pas dans ceux du meneur** : il se tient sur la ligne que le meneur a
 * parcourue, à une distance fixe derrière lui. Tout point de cette ligne a été occupé par le
 * gabarit du meneur, qui tenait : un suiveur ne peut donc jamais se trouver dans du plein, ni
 * rester coincé — c'est le critère du lot, et il tient par construction.
 *
 * La trace ne connaît pas la carte : elle retient des points, en cases, et mesure des longueurs.
 * Ce que la carte permet se décide avant d'y poser un point (`core::ExplorationSession`).
 */

namespace core {

/// @brief Un point continu de la trace, en cases (`core::CellPoint` sans dépendance à la session).
using TrailPoint = Vector2;

/**
 * @brief La ligne parcourue par le meneur, du plus récent au plus ancien, et ce qu'on en retient.
 */
class FollowTrail {
public:
    /// Distance entre deux membres du groupe, le long de la trace, en cases : une case, la file
    /// indienne d'un couloir, sans que deux gabarits (0,6 case) se chevauchent.
    static constexpr float SPACING_CELLS = 1.0F;
    /// En deçà, un déplacement du meneur ne pose pas de nouveau point : il piétine, et une trace
    /// faite de points confondus ne donne plus de direction.
    static constexpr float MIN_STEP_CELLS = 0.01F;

    /**
     * @brief Repart d'une trace neuve : @p points, du meneur (le premier) vers l'arrière.
     *
     * Une seule valeur pose tout le groupe sur le meneur ; plusieurs l'étirent derrière lui — ce
     * que fait la session à l'arrivée sur une carte, quand la place derrière le meneur est libre.
     * Une liste vide vide la trace.
     */
    void reset(const std::vector<TrailPoint>& points);

    /// @brief Le meneur est en @p leader : un nouveau point si l'écart au dernier le mérite.
    void record(TrailPoint leader);

    /// @brief Ne garde que @p length cases de trace : au-delà, aucun suiveur ne se tient.
    void keep(float length);

    /**
     * @return Le point de la trace à @p distance cases derrière le meneur ; le plus ancien point si
     *         la trace est plus courte — le suiveur attend alors là où le groupe est arrivé.
     */
    [[nodiscard]] TrailPoint pointBehind(float distance) const;

    /**
     * @return La direction de marche au point situé à @p distance derrière le meneur, du plus
     *         ancien vers le plus récent (non normée) ; nulle si la trace n'a qu'un point.
     */
    [[nodiscard]] Vector2 directionAt(float distance) const;

    /// @return La longueur de la trace, en cases.
    [[nodiscard]] float length() const noexcept;

    /// @return Les points, du plus récent au plus ancien.
    [[nodiscard]] const std::deque<TrailPoint>& points() const noexcept {
        return _points;
    }

private:
    std::deque<TrailPoint> _points;
    /// Longueur gardée ; 0 : tout.
    float _kept = 0.0F;
};

}  // namespace core
