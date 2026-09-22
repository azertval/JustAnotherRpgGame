// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Gameplay/WorldFlags.h
 * @brief Les drapeaux de monde : ce qui a eu lieu et ne doit pas avoir lieu deux fois (`LOT-10`).
 */

#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace core {

/**
 * @brief L'ensemble des faits **acquis** d'une partie : coffres ouverts, quêtes franchies, portes
 *        déverrouillées.
 *
 * **Ce n'est pas un booléen sur l'entité.** Un coffre ouvert doit le rester quand le joueur quitte
 * la carte et y revient : à ce moment-là, l'entité du coffre est détruite et recréée depuis la
 * couche `objects` du fichier de niveau, qui ne sait rien de ce qui s'est passé. Un drapeau porté
 * par l'entité disparaîtrait avec elle, et le coffre redonnerait son butin à chaque passage — un
 * défaut qui ne casse rien, ne lève aucune alerte, et se confond avec de la générosité de
 * conception.
 *
 * L'état vit donc **à côté** des entités, dans un ensemble qui survit au chargement de carte et
 * que la sauvegarde (`LOT-17`) sérialisera telle quelle.
 *
 * Les clés sont des chaînes plutôt qu'un type fermé : le `LOT-16` y écrira des drapeaux de quête
 * que ce lot ne peut pas énumérer, et une énumération obligerait chaque lot suivant à modifier ce
 * fichier. La contrepartie — une faute de frappe passe — est traitée par `keyForEntity()`, qui
 * **fabrique** la clé d'une entité de carte plutôt que de la laisser écrire à la main.
 */
class WorldFlags {
public:
    /// @brief Vrai si le fait est acquis.
    [[nodiscard]] bool isSet(std::string_view key) const;

    /// @brief Marque le fait comme acquis. Renvoie `false` s'il l'était déjà.
    bool set(std::string_view key);

    /// @brief Efface un fait. Utile à un `LOT-16` qui rouvrirait une quête, et aux tests.
    void clear(std::string_view key);

    /// @brief Le nombre de faits acquis.
    [[nodiscard]] std::size_t size() const {
        return _flags.size();
    }

    /// @brief Tous les faits acquis, triés — c'est la forme que la sauvegarde écrira.
    [[nodiscard]] std::vector<std::string> all() const;

private:
    std::set<std::string, std::less<>> _flags;
};

/**
 * @brief La clé de drapeau d'une entité de carte : `"<carte>/<type>@<colonne>,<ligne>"`.
 *
 * **Fabriquée, jamais écrite à la main.** Deux coffres d'une même carte se distinguent par leur
 * case ; deux cartes différentes ne se marchent pas dessus parce que le nom de carte ouvre la clé.
 * Laisser l'auteur de la carte nommer le drapeau produirait tôt ou tard deux coffres partageant la
 * même clé — et le second serait vide dès sa première ouverture, sans que rien ne l'explique.
 *
 * La **position** sert d'identité parce que c'est la seule chose qu'une entité de carte possède en
 * propre et qui ne bouge pas. Corollaire assumé : déplacer un coffre dans l'éditeur le remet à
 * neuf pour une partie déjà commencée. C'est le bon compromis — l'inverse demanderait un
 * identifiant stable écrit dans le fichier de carte, que le `LOT-11` devrait générer et maintenir
 * unique.
 */
[[nodiscard]] std::string keyForEntity(std::string_view mapName, std::string_view entityType,
                                       int column, int row);

}  // namespace core
