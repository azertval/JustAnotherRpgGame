// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <vector>

#include "Core/Ecs/Entity.h"

/**
 * @file Core/Ecs/EntityManager.h
 * @brief Gestionnaire du cycle de vie des entités (création, destruction, recyclage).
 */

namespace core {

/**
 * @brief Crée, détruit et valide les entités de l'ECS.
 *
 * Possède les tableaux internes indexés par ::core::Entity::index :
 * - la **génération** courante de chaque index (pour détecter les handles périmés) ;
 * - un drapeau de vie par index.
 *
 * Les index libérés sont conservés dans une **liste libre** et recyclés en
 * incrémentant leur génération, ce qui borne la mémoire et invalide
 * automatiquement les handles pointant sur un index réutilisé. Les opérations
 * sont en temps constant amorti, sans allocation par entité.
 */
class EntityManager {
public:
    /**
     * @brief Crée une nouvelle entité vivante.
     * @return Un handle valide, distinct de toute entité vivante existante.
     */
    [[nodiscard]] Entity create();

    /**
     * @brief Détruit une entité et libère son index pour recyclage.
     *
     * Sans effet si le handle n'est pas vivant (déjà détruit ou périmé). Après
     * l'appel, `isAlive(entity)` est faux pour ce handle.
     *
     * @param entity Handle de l'entité à détruire.
     */
    void destroy(Entity entity);

    /**
     * @brief Indique si un handle désigne une entité actuellement vivante.
     * @param entity Handle à tester.
     * @return `true` si l'index est vivant **et** la génération correspond.
     */
    [[nodiscard]] bool isAlive(Entity entity) const;

    /// @return Nombre d'entités actuellement vivantes.
    [[nodiscard]] std::size_t aliveCount() const;

private:
    /// Génération courante de chaque index (indexé par Entity::index).
    std::vector<Entity::Generation> _generations;
    /// Drapeau de vie de chaque index (indexé par Entity::index).
    std::vector<bool> _alive;
    /// Index libérés disponibles au recyclage.
    std::vector<Entity::Index> _freeIndices;
    /// Nombre d'entités vivantes.
    std::size_t _aliveCount = 0;
};

}  // namespace core
