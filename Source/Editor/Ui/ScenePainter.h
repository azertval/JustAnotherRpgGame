// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <QColor>
#include <QImage>
#include <QTransform>
#include <functional>
#include <optional>

#include "Core/Math/Rect.h"
#include "HMI/Graphics/ComposedScene.h"

class QPainter;

/**
 * @file Editor/Ui/ScenePainter.h
 * @brief Peindre une `hmi::ComposedScene` avec `QPainter` : la liste de primitives du jeu, dans
 *        l'ordre du jeu, sans GPU (`LOT-EDITOR-02`, décision D2).
 *
 * Le jeu soumet la scène composée à `hmi::SpriteBatch` ; l'éditeur la parcourt ici, quad par quad,
 * dans le même ordre. Ce qui compte pour ressembler au jeu, et qui est fait ici comme le GPU le
 * fait : l'échantillonnage — l'art peint **lissé**, lu sur le niveau réduit que demande l'échelle,
 * les images engendrées **au plus proche** (`LOT-125`) —, la région d'image tirée des UV,
 * l'opacité du quad, la rotation autour de son centre. La comparaison d'image avec le rendu hors
 * écran du jeu en est la preuve (`test_scene_painter.cpp`) ; elle se fait à un seuil, car le GPU
 * mêle deux niveaux (trilinéaire) là où le peintre n'en lit qu'un.
 *
 * Une différence assumée : la **teinte** RVB d'un quad texturé n'est pas appliquée (le jeu ne
 * teinte aucune pièce de lieu) ; seul un quad à teinte unie (`hmi::SceneImages::solid`) se peint
 * en aplat de sa couleur. Un `hmi::PolyQuad` (`LOT-128`) se peint toujours ainsi : il n'a pas
 * d'autre contenu que sa teinte.
 */

namespace hmi {

class Camera2D;

/**
 * @brief Opacité supplémentaire d'une primitive, décidée par l'appelant (calques masqués, grisés,
 *        reliefs en transparence) ; 0 ou moins : la primitive n'est pas peinte.
 */
using QuadOpacity = std::function<float(const ComposedQuad&)>;

/**
 * @brief Peint @p scene dans @p painter, dont la transformation porte déjà le cadrage (unités
 *        monde vers pixels).
 * @param painter Surface de peinture, déjà cadrée.
 * @param scene Scène composée à peindre.
 * @param visible Le rectangle du monde à peindre ; une primitive qui ne le coupe pas est sautée.
 *                Absent : tout.
 * @param opacity Opacité supplémentaire par primitive ; vide : 1.
 */
void paintComposedScene(QPainter& painter, const ComposedScene& scene,
                        const std::optional<core::Rect>& visible = std::nullopt,
                        const QuadOpacity& opacity = {});

/**
 * @brief Le rectangle qu'occupe @p scene, réuni à @p base : chaque primitive compte, reliefs et
 *        figurines qui montent au-dessus de leur case compris.
 *
 * Le cadre du canevas, des vignettes et de `--render` (`LOT-125`) : il se mesure sur ce qui est
 * peint, et non sur une marge supposée — une pièce de quatre cases de haut n'y est jamais rognée.
 */
[[nodiscard]] core::Rect composedSceneBounds(const ComposedScene& scene, const core::Rect& base);

/// @return La transformation d'un cadrage du jeu : unités monde vers pixels de la surface.
[[nodiscard]] QTransform cameraTransform(const Camera2D& camera);

/**
 * @brief Rend @p scene hors écran, cadrée par @p camera, sur un fond @p clear — l'image que le jeu
 *        dessinerait avec la même caméra.
 */
[[nodiscard]] QImage renderComposedScene(const ComposedScene& scene, const Camera2D& camera,
                                         int width, int height, const QColor& clear);

}  // namespace hmi
