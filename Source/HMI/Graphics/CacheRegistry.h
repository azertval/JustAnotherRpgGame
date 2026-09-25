// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

/**
 * @file HMI/Graphics/CacheRegistry.h
 * @brief Registre générique clé → ressource chargée paresseusement, avec invalidation
 *        (`EX-REN-043`).
 */

namespace hmi {

/**
 * @brief Mémoïsation par clé logique : charge au premier accès, retient aussi un **échec**, et
 *        s'invalide par clé ou entièrement.
 *
 * Factorise la logique de bibliothèque d'un cache de ressources hors de tout détail de rendu :
 * c'est ce qui rend ce comportement — mémoïsation, mémorisation d'échec,
 * `invalidate`/`invalidateAll` — vérifiable **sans GPU** (`EX-NFR-004`), avec n'importe quel type
 * de ressource. Ce fichier ne connaît rien de QRhi ni de Qt.
 *
 * Logique **pure**, aucune dépendance externe (`EX-NFR-010`).
 * @tparam Resource Type de ressource mémorisée par clé.
 */
template <typename Resource>
class CacheRegistry {
public:
    /**
     * @brief Obtient la ressource sous @p key, en la chargeant via @p loader au premier accès.
     *
     * Un chargement en échec (`loader()` renvoyant `std::nullopt`) est **mémorisé** comme une
     * entrée à `std::nullopt` : les accès suivants ne rappellent pas `loader` tant que la clé
     * n'est pas invalidée — sans cela, un asset manquant relirait le disque à chaque image.
     * @param key    Clé logique.
     * @param loader Fonction sans argument, appelée uniquement si @p key n'est pas déjà en cache ;
     *               renvoie `std::optional<Resource>`.
     * @return Pointeur vers la ressource en cache, ou `nullptr` si le chargement (courant ou déjà
     *         mémorisé) a échoué.
     */
    template <typename Loader>
    [[nodiscard]] const Resource* getOrLoad(const std::string& key, Loader&& loader) {
        const auto found = _entries.find(key);
        if (found != _entries.end()) {
            return found->second ? &*found->second : nullptr;
        }
        const auto inserted = _entries.emplace(key, std::forward<Loader>(loader)()).first;
        return inserted->second ? &*inserted->second : nullptr;
    }

    /// Retire une entrée (succès ou échec mémorisé) : le prochain `getOrLoad` sous cette clé
    /// rappelle `loader`.
    void invalidate(const std::string& key) {
        _entries.erase(key);
    }

    /// Retire **toutes** les entrées.
    void invalidateAll() {
        _entries.clear();
    }

    /// @return Le nombre d'entrées mémorisées (succès **et** échecs).
    [[nodiscard]] std::size_t size() const noexcept {
        return _entries.size();
    }

private:
    std::unordered_map<std::string, std::optional<Resource>> _entries;
};

}  // namespace hmi
