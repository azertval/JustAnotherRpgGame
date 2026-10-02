// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>
#include <span>
#include <string_view>

#include "Core/Resources/MeshFile.h"

/**
 * @file Core/Resources/SkeletonPose.h
 * @brief La **pose d'un squelette** à un instant d'un clip (`LOT-1005`) : pour chaque os, la
 *        matrice qui déforme les sommets qui lui sont liés.
 *
 * Un clip dit, par os, une translation et une rotation dans le temps (`core::MeshClip`). La pose
 * les échantillonne — interpolation linéaire entre deux clés, la rotation par le plus court
 * chemin —, compose chaque os avec son parent, puis avec sa matrice de liaison inverse : le
 * résultat va **du repère du maillage au repère du maillage**, et vaut l'identité quand l'os est
 * dans sa pose de liaison. Un os que le clip n'anime pas garde sa pose de repos.
 *
 * Logique pure, sans GPU ni Qt : le rendu téléverse ces matrices telles quelles, et un test les
 * lit sans fenêtre.
 */

namespace core {

/// @return Le clip @p name de @p rig, `nullptr` s'il n'en a pas de ce nom.
[[nodiscard]] const MeshClip* findClip(const MeshRig& rig, std::string_view name) noexcept;

/**
 * @brief L'instant d'un clip de @p duration secondes joué depuis @p seconds : ramené dans le clip
 *        s'il **boucle**, figé sur sa fin sinon — un mort ne se relève pas parce que le temps
 *        passe.
 */
[[nodiscard]] float clipTime(float seconds, float duration, bool loop) noexcept;

/**
 * @brief Calcule la pose de @p rig à l'instant @p seconds de @p clip.
 *
 * @param rig     Le squelette.
 * @param clip    Le clip joué ; `nullptr` : la pose de repos.
 * @param seconds L'instant dans le clip (`clipTime`) ; avant la première clé ou après la
 *                dernière, la clé du bord vaut.
 * @param out     Seize flottants par os, en colonnes, dans l'ordre de `rig.joints` ; un tampon
 *                trop court n'est pas écrit.
 */
void poseSkeleton(const MeshRig& rig, const MeshClip* clip, float seconds, std::span<float> out);

/**
 * @brief Le point @p position d'un sommet lié par @p skin, déformé par la pose @p matrices
 *        (`poseSkeleton`) : la somme pondérée de ses quatre os. Ce que le shader calcule, pour un
 *        test ou une mesure.
 */
[[nodiscard]] std::array<float, 3> skinnedPosition(std::span<const float> matrices,
                                                   const MeshSkinVertex& skin,
                                                   const std::array<float, 3>& position) noexcept;

}  // namespace core
