// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Core/Combat/PartyDeployment.h"
#include "Core/Combat/TacticalTerrain.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/MapEntity.h"
#include "Core/World/CombatZone.h"
#include "Core/World/EntityKinds.h"

/**
 * @file Editor/Logic/EditorDiagnostics.h
 * @brief Les avertissements que l'éditeur montre sur les entités d'une carte : références cassées,
 *        rencontres posées hors d'un terrain tactique valide (`LOT-11`), zones de combat qui ne se
 *        jouent pas (`LOT-EDITOR-05`), groupe qui ne s'y déploie pas (`LOT-143`).
 */

namespace hmi {

/// @brief La famille d'un avertissement : ce que l'entité référence, ou le terrain qu'elle occupe.
enum class EditorDiagnosticKind {
    Reference,
    Terrain,
};

/**
 * @brief Un avertissement prêt à afficher, en anglais.
 *
 * Le `Core` rend des codes (`EX-NFR-011`) ; ce module les dit, une fois, pour que le panneau et les
 * tests lisent la même chose. L'éditeur n'a pas de traduction (`LOT-EDITOR-01`) : une famille
 * d'entité et une propriété s'y nomment par leur identifiant du format (`npc`, `dialogue`).
 */
struct EditorDiagnostic {
    EditorDiagnosticKind kind = EditorDiagnosticKind::Reference;
    std::size_t entityIndex = 0;
    /// Case de l'entité, ou case en cause (combattant hors de la zone jouable).
    core::GridPosition cell;
    std::string message;

    [[nodiscard]] bool operator==(const EditorDiagnostic&) const = default;
};

/// @return Le message de @p code, `%1` l'entité, `%2` la propriété, `%3` la valeur.
[[nodiscard]] const char* entityIssueTemplate(core::EntityIssueCode code) noexcept;

/// @return Le message de @p code, `%1` la rencontre, `%2` et `%3` ce qu'il cite.
[[nodiscard]] const char* tacticalIssueTemplate(core::TacticalIssueCode code) noexcept;

/// @return Le message de @p code, `%1` la rencontre, `%2` le combattant ou l'effectif, `%3` la
///         zone, `%4` et `%5` ce qu'il compte.
[[nodiscard]] const char* deploymentIssueTemplate(core::DeploymentIssueCode code) noexcept;

/// @return Le défaut @p issue du déploiement @p deployment, dit en une phrase ; la zone se nomme
///         par @p entities.
[[nodiscard]] std::string deploymentIssueText(const core::PartyDeployment& deployment,
                                              const core::DeploymentIssue& issue,
                                              const std::vector<core::MapEntity>& entities);

/**
 * @brief Les avertissements de la carte : les références, puis le terrain des rencontres, puis les
 *        zones de combat, puis le déploiement du groupe, chacun dans l'ordre des entités.
 *
 * Une zone de combat avertit si elle ne se joue pas (vide, hors de la carte, sans case libre), et
 * une entrée d'arène si elle n'est dans aucune zone : le combat ne la verrait pas. Une rencontre
 * avertit si le groupe de quatre ne se déploie pas face à elle sur sa zone (`LOT-143`).
 */
[[nodiscard]] std::vector<EditorDiagnostic> editorDiagnostics(
    const std::vector<core::MapEntity>& entities, const std::vector<core::EntityIssue>& issues,
    const std::vector<core::EncounterTerrain>& terrains,
    const std::vector<core::CombatZoneTerrain>& zones = {},
    const std::vector<core::PartyDeployment>& deployments = {});

/**
 * @brief Le verdict d'une zone de combat en une ligne, pour l'inspecteur : sa taille, ses cases
 *        libres, ses entrées d'arène dedans et dehors — ou son défaut.
 *
 * Par exemple : `sable: 20 x 14, 230 free cells of 280, 8 arena entries inside, 0 outside.`
 */
[[nodiscard]] std::string combatZoneSummary(const core::CombatZoneTerrain& zone);

}  // namespace hmi
