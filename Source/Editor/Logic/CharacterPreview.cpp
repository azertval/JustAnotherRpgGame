// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Editor/Logic/CharacterPreview.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

#include "Core/Combat/IsoProjection.h"
#include "Core/Levels/TileType.h"

namespace hmi {

namespace {

// Ce que le cadrage réserve, en mètres : un personnage de 2 m sous la caméra du jeu, une marge au-dessus et le sol devant.
constexpr float FRAMED_METRES = 2.4F;
// La case fait 1,5 m de côté : sa diagonale, la largeur du losange, en mètres.
constexpr float TILE_DIAGONAL_METRES = 1.5F * std::numbers::sqrt2_v<float>;
// Le centre de la cible est à mi-hauteur du personnage, pas à ses pieds.
constexpr float CENTER_LIFT_METRES = 0.75F;
// La part d'un clip joué une fois où l'aperçu le tient : sa dernière pose, à un millième près.
constexpr float HELD_SHARE = 0.999F;

[[nodiscard]] core::IsoProjection projection() {
    return core::IsoProjection{CHARACTER_PREVIEW_CELLS, CHARACTER_PREVIEW_CELLS,
                               core::ARENA_TILE_WIDTH_UNITS, core::ARENA_DIAMOND_RATIO};
}

}  // namespace

WorldSceneSnapshot characterPreviewScene(const CharacterPreviewView& view) {
    constexpr std::size_t CELLS =
        static_cast<std::size_t>(CHARACTER_PREVIEW_CELLS) * CHARACTER_PREVIEW_CELLS;
    WorldSceneSnapshot scene;
    scene.columns = CHARACTER_PREVIEW_CELLS;
    scene.rows = CHARACTER_PREVIEW_CELLS;
    scene.floors.assign(CELLS, std::string{});
    scene.relief.assign(CELLS, std::string{});
    // Sans pièce nommée, le rendu de maquette dessine le type : une terre battue, neutre.
    scene.types.assign(CELLS, core::TileType::Dirt);
    scene.reliefTypes.assign(CELLS, core::TileType::Empty);
    if (!view.model.empty()) {
        const float middle = static_cast<float>(CHARACTER_PREVIEW_CELLS) / 2.0F;
        scene.figures.push_back(WorldFigureSnapshot{
            .figure = "preview",
            .clip = view.clip,
            .point = {middle, middle},
            .seconds = view.seconds,
            .model = view.model,
            .heading = FIGURE_HEADING_FRONT +
                       (static_cast<float>(view.quarterTurns) * std::numbers::pi_v<float> / 2.0F)});
    }
    return scene;
}

WorldFraming characterPreviewFraming(int pixelHeight) {
    const core::IsoProjection iso = projection();
    const float middle = static_cast<float>(CHARACTER_PREVIEW_CELLS) / 2.0F;
    const float unitsPerMetre = iso.tileWidth() / TILE_DIAGONAL_METRES;
    core::Vector2 center = iso.gridToWorld({middle, middle});
    center.y -= CENTER_LIFT_METRES * unitsPerMetre;
    return WorldFraming{.center = center,
                        .pixelsPerUnit = static_cast<float>(std::max(pixelHeight, 1)) /
                                         (FRAMED_METRES * unitsPerMetre)};
}

float characterPreviewSeconds(const core::SkeletonClip* clip, float elapsed) {
    elapsed = std::max(elapsed, 0.0F);
    if (clip == nullptr || clip->duration <= 0.0F) {
        return std::fmod(elapsed, 1.0F);
    }
    if (clip->loop) {
        return std::fmod(elapsed, clip->duration);
    }
    // Tenu juste avant sa fin : un modèle de l'atelier n'a pas encore de fiche à côté de lui, le
    // rendu le fait donc boucler, et l'instant exact de la fin y serait celui du début.
    return std::min(std::fmod(elapsed, clip->duration + CHARACTER_PREVIEW_HOLD),
                    clip->duration * HELD_SHARE);
}

}  // namespace hmi
