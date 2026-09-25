// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <limits>
#include <tuple>

#include "Core/Ecs/ComponentPool.h"
#include "Core/Ecs/Entity.h"

/**
 * @file Core/Ecs/View.h
 * @brief Vue multi-composants : itère les entités possédant un ensemble de types.
 */

namespace core {

/**
 * @brief Sélection en lecture-écriture des entités possédant tous les composants demandés.
 *
 * Une vue joint plusieurs ::core::ComponentPool et n'expose que les entités
 * présentes dans **toutes** les pools (`Transform` **et** `Velocity`, par
 * exemple). Pour chaque entité retenue, elle donne accès au handle et à une
 * référence sur chacun de ses composants.
 *
 * L'itération est pilotée par la **plus petite pool** parmi les types demandés :
 * on parcourt son tableau dense et l'on filtre chaque entité sur la présence
 * dans les autres pools, ce qui borne le coût par la plus petite population.
 *
 * Deux formes d'usage équivalentes :
 * @code
 * for (auto [entity, transform, velocity] : view) { ... }
 * view.each([](Entity entity, Transform& transform, Velocity& velocity) { ... });
 * @endcode
 *
 * @warning Contrat d'itération : ne modifier que la **valeur** des composants
 *          pendant le parcours. Ajouter ou retirer un composant, ou détruire une
 *          entité, invalide la vue (les pools sous-jacentes peuvent réallouer ou
 *          déplacer leurs éléments).
 *
 * @tparam Components Types de composants requis, supposés **distincts**.
 */
template <typename... Components>
class View {
public:
    /**
     * @brief Construit une vue joignant les pools des composants demandés.
     * @param pools Références sur les pools, une par type de `Components`.
     */
    explicit View(ComponentPool<Components>&... pools) : _pools(&pools...) {}

    /**
     * @brief Applique une fonction à chaque entité possédant tous les composants.
     * @param function Appelable `void(Entity, Components&...)` invoqué par entité retenue.
     * @tparam Function Type de l'appelable.
     */
    template <typename Function>
    void each(Function&& function) const {
        const std::vector<Entity>& driver = smallestEntities();
        for (const Entity entity : driver) {
            if (matches(entity)) {
                function(entity, componentOf<Components>(entity)...);
            }
        }
    }

    /**
     * @brief Itérateur avant sur les entités retenues par la vue.
     *
     * Déréférencé, il fournit un `std::tuple<Entity, Components&...>` (handle
     * puis références sur les composants), ce qui autorise la liaison structurée
     * `for (auto [entity, ...] : view)`.
     */
    class Iterator {
    public:
        /**
         * @brief Construit un itérateur positionné puis avancé sur la première entité valide.
         * @param view   Vue parcourue.
         * @param driver Tableau dense pilote (entités de la plus petite pool).
         * @param index  Position de départ dans le tableau pilote.
         */
        Iterator(const View* view, const std::vector<Entity>* driver, std::size_t index)
            : _view(view), _driver(driver), _index(index) {
            skipToMatch();
        }

        /**
         * @brief Compare deux itérateurs par position.
         * @param other Itérateur à comparer.
         * @return `true` si les positions diffèrent (parcours non terminé).
         */
        [[nodiscard]] bool operator!=(const Iterator& other) const {
            return _index != other._index;
        }

        /**
         * @brief Avance jusqu'à l'entité valide suivante.
         * @return Une référence sur cet itérateur, avancé.
         */
        Iterator& operator++() {
            ++_index;
            skipToMatch();
            return *this;
        }

        /**
         * @brief Déréférence l'itérateur sur l'entité courante et ses composants.
         * @return Un tuple `{ entité, références sur chaque composant }`.
         */
        [[nodiscard]] std::tuple<Entity, Components&...> operator*() const {
            const Entity entity = (*_driver)[_index];
            return std::tuple<Entity, Components&...>(entity,
                                                      _view->componentOf<Components>(entity)...);
        }

    private:
        /// Avance l'index sur la prochaine entité possédant tous les composants.
        void skipToMatch() {
            while (_index < _driver->size() && !_view->matches((*_driver)[_index])) {
                ++_index;
            }
        }

        const View* _view;
        const std::vector<Entity>* _driver;
        std::size_t _index;
    };

    /// @return Un itérateur sur la première entité retenue par la vue.
    [[nodiscard]] Iterator begin() const {
        const std::vector<Entity>& driver = smallestEntities();
        return Iterator(this, &driver, 0);
    }

    /// @return L'itérateur de fin (position au-delà du dernier élément du pilote).
    [[nodiscard]] Iterator end() const {
        const std::vector<Entity>& driver = smallestEntities();
        return Iterator(this, &driver, driver.size());
    }

private:
    /**
     * @brief Indique si une entité possède tous les composants requis.
     * @param entity Entité testée.
     * @return `true` si chaque pool contient l'entité.
     */
    [[nodiscard]] bool matches(Entity entity) const {
        return (std::get<ComponentPool<Components>*>(_pools)->has(entity) && ...);
    }

    /**
     * @brief Accède au composant d'un type donné pour une entité retenue.
     * @tparam Component Type de composant voulu.
     * @param entity Entité (supposée présente dans la pool correspondante).
     * @return Une référence sur le composant.
     */
    template <typename Component>
    [[nodiscard]] Component& componentOf(Entity entity) const {
        return std::get<ComponentPool<Component>*>(_pools)->get(entity);
    }

    /**
     * @brief Sélectionne le tableau dense d'entités de la plus petite pool.
     * @return Une référence sur le tableau dense pilotant l'itération.
     */
    [[nodiscard]] const std::vector<Entity>& smallestEntities() const {
        const std::vector<Entity>* smallest = nullptr;
        std::size_t smallestSize = (std::numeric_limits<std::size_t>::max)();
        // Retient la pool de plus faible population : elle borne le nombre
        // d'entités à filtrer (l'intersection ne peut être plus grande).
        const auto consider = [&](const auto* pool) {
            if (pool->size() < smallestSize) {
                smallestSize = pool->size();
                smallest = &pool->entities();
            }
        };
        (consider(std::get<ComponentPool<Components>*>(_pools)), ...);
        return *smallest;
    }

    /// Pointeurs sur les pools jointes, un par type de `Components`.
    std::tuple<ComponentPool<Components>*...> _pools;
};

}  // namespace core
