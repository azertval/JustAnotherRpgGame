// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <string>
#include <string_view>

#include "Core/Resources/SkeletonFile.h"
#include "HMI/Graphics/WorldSceneComposer.h"
#include "HMI/Graphics/WorldSceneRenderer.h"

/**
 * @file Editor/Logic/CharacterPreview.h
 * @brief L'**aperçu** de l'atelier des assets (`LOT-1008`, `EX-EDIT-102`) : un personnage seul,
 *        debout sur un damier de maquette, dessiné par le rendu du jeu.
 *
 * L'aperçu n'a pas de rendu à lui : c'est une scène — trois cases sur trois, sans pièce, une
 * figurine au centre — remise à `hmi::WorldSceneRenderer`, sous la caméra du jeu. Ce qu'on y voit
 * est ce qu'on jouera : même maillage, même pose, même profondeur. Logique pure : la fenêtre
 * l'affiche, un test la rend hors écran.
 */

namespace hmi {

/// Le côté du damier de l'aperçu, en cases ; le personnage se tient au centre.
inline constexpr int CHARACTER_PREVIEW_CELLS = 3;
/// Le temps, en secondes, où un clip joué une fois reste sur sa dernière pose avant de reprendre.
inline constexpr float CHARACTER_PREVIEW_HOLD = 0.8F;

/// @brief Ce que l'aperçu montre.
struct CharacterPreviewView {
    /// Le `.glb` du personnage, relatif au dossier que le rendu a pour racine.
    std::string model;
    /// Le clip joué (`idle`, `walk`, `attack`, `cast`, `hit`, `death`).
    std::string clip = "idle";
    /// L'instant dans le clip, en secondes (`characterPreviewSeconds`).
    float seconds = 0.0F;
    /// Le nombre de quarts de tour depuis la vue de face.
    int quarterTurns = 0;

    [[nodiscard]] bool operator==(const CharacterPreviewView&) const = default;
};

/// @return La scène de l'aperçu de @p view : le damier, et le personnage tourné de ses quarts de
///         tour.
[[nodiscard]] WorldSceneSnapshot characterPreviewScene(const CharacterPreviewView& view);

/**
 * @brief Le cadrage qui tient un personnage de 2 m, debout au centre du damier, dans une cible de
 *        @p pixelHeight pixels de haut.
 */
[[nodiscard]] WorldFraming characterPreviewFraming(int pixelHeight);

/**
 * @brief L'instant d'un clip pour @p elapsed secondes d'aperçu.
 *
 * Un clip en boucle tourne ; un clip joué une fois va à sa fin, y reste
 * `CHARACTER_PREVIEW_HOLD` secondes — le temps de juger sa dernière pose —, puis reprend.
 *
 * @param clip    Ce que le squelette déclare du clip ; `nullptr` : rien n'est su, il boucle sur
 *                une seconde.
 * @param elapsed Le temps d'aperçu écoulé, en secondes.
 */
[[nodiscard]] float characterPreviewSeconds(const core::SkeletonClip* clip, float elapsed);

}  // namespace hmi
