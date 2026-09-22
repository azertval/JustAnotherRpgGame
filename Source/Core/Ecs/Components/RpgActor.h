// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>

/**
 * @file Core/Ecs/Components/RpgActor.h
 * @brief Composant reliant une entité de la carte à sa fiche de personnage (données pures).
 */

namespace core {

/**
 * @brief Marque une entité comme **acteur de jeu de rôle** et la relie à sa fiche.
 *
 * Donnée pure sans logique (`EX-ARCH-011`). Le composant ne porte **pas** la fiche : il porte son
 * indice dans le registre du monde. Deux raisons, et la seconde est celle qui compte.
 *
 * D'abord la taille : une `CharacterSheet` porte des chaînes et deux ensembles, et la recopier
 * dans chaque composant ferait grossir le tableau que l'ECS parcourt à chaque image pour des
 * données qu'un système de déplacement ne lit jamais.
 *
 * Ensuite, et surtout : **une fiche n'appartient pas à une entité**. Un personnage du groupe garde
 * la sienne quand il change de carte et que son entité est détruite puis recréée
 * (`LOT-09`) ; une fiche survit à l'entité qui la représente. Loger la fiche dans le
 * composant lierait la vie de l'une à celle de l'autre, et le passage au groupe de quatre
 * (`LOT-29`) obligerait à défaire ce lien.
 *
 * `INDICE_ABSENT` distingue une entité **sans fiche** — un décor animé, un projectile — d'une
 * entité dont la fiche serait la première du registre. Les confondre ferait attaquer un tonneau
 * avec les caractéristiques du héros.
 */
struct RpgActor {
    /// @brief Valeur d'un indice qui ne désigne aucune fiche.
    static constexpr std::size_t INDICE_ABSENT = static_cast<std::size_t>(-1);

    /// Indice de la fiche dans le registre du monde.
    std::size_t sheetIndex = INDICE_ABSENT;

    /// @brief Vrai si l'entité est reliée à une fiche.
    [[nodiscard]] bool hasSheet() const {
        return sheetIndex != INDICE_ABSENT;
    }
};

}  // namespace core
