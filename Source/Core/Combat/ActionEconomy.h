// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/ActionEconomy.h
 * @brief Ce qu'un tour permet de faire, et ce qui en reste (`LOT-20`, `EX-CBT-011`).
 */

#include <string>
#include <string_view>
#include <vector>

namespace core {

/// @brief L'action du tour (Manuel des Joueurs, « Votre tour »).
inline constexpr std::string_view ACTION_RESOURCE = "action";
/// @brief L'action bonus.
inline constexpr std::string_view BONUS_ACTION_RESOURCE = "bonusAction";
/// @brief La réaction, qui se dépense aussi **hors** de son tour.
inline constexpr std::string_view REACTION_RESOURCE = "reaction";
/// @brief Le déplacement, compté en **cases** (`core::movementBudget`).
inline constexpr std::string_view MOVEMENT_RESOURCE = "movement";

/// @brief Une ressource du tour : ce qu'on en reçoit à chaque tour, et ce qu'il en reste.
struct ActionResource {
    std::string id;
    /// Ce que le début du tour du porteur restaure.
    int perTurn = 0;
    int remaining = 0;
};

/**
 * @brief L'économie d'action d'un combattant : une **liste** de ressources, pas trois booléens.
 *
 * ## Pourquoi une liste
 *
 * Le Manuel donne quatre ressources — action, action bonus, réaction, déplacement —, et les livres
 * de Tanares en ajoutent : la *Heroic Action* des Marques Héroïques (`LOT-50`) est une **troisième
 * économie d'action**, et une capacité octroie une réaction supplémentaire à un allié. Trois
 * booléens auraient demandé une refonte au premier ajout ; une ressource de plus est ici un appel
 * à `declare`.
 *
 * ## Quand une ressource revient
 *
 * **Au début du tour de son porteur**, et à aucun autre moment (`EX-CBT-011`). C'est ce qui rend
 * la réaction dépensable hors de son tour : restaurée à la fin du tour courant, une réaction
 * dépensée pendant le tour d'un ennemi reviendrait avant que cet ennemi ait fini, et une attaque
 * d'opportunité se jouerait deux fois dans le même round.
 *
 * ## Le déplacement se fractionne
 *
 * `EX-CBT-011` dit chaque ressource « consommable une seule fois ». Pour le déplacement, c'est le
 * **budget** qui ne se consomme qu'une fois : le Manuel laisse « décider si vous voulez d'abord
 * vous déplacer ou agir », et permet de répartir sa vitesse autour de l'action — trois cases, une
 * attaque, trois cases. Ce qui ne se fait pas, c'est retrouver des cases dans le même tour.
 */
class ActionEconomy {
public:
    /**
     * @brief L'économie du Manuel : une action, une action bonus, une réaction, et @p movement
     *        cases de déplacement — toutes disponibles.
     */
    [[nodiscard]] static ActionEconomy standard(int movement);

    /**
     * @brief Déclare une ressource, ou change ce qu'un tour en restaure, et la remplit.
     *
     * Une ressource déjà déclarée garde sa place dans la liste : l'ordre des ressources est celui
     * de leur première déclaration, et ne change pas d'une partie à l'autre.
     */
    void declare(std::string_view id, int perTurn);

    /// @brief Vrai si la ressource @p id a été déclarée, quel que soit son reste.
    [[nodiscard]] bool has(std::string_view id) const;
    /// @brief Ce qu'il reste de la ressource, ou 0 si elle n'est pas déclarée.
    [[nodiscard]] int remaining(std::string_view id) const;

    /**
     * @brief Dépense @p amount de la ressource.
     * @return Faux, et **rien n'est dépensé**, si la ressource est inconnue, s'il n'en reste pas
     *         assez, ou si @p amount n'est pas positif : on ne paie pas une case de terrain
     *         difficile à moitié.
     */
    bool spend(std::string_view id, int amount = 1);

    /**
     * @brief Octroie @p amount de la ressource au-delà de ce que le tour restaure — une réaction
     *        offerte par un allié.
     *
     * L'octroi vaut jusqu'à ce qu'il soit dépensé ou jusqu'au prochain début de tour du porteur,
     * qui remet chaque ressource à son `perTurn`. Une ressource inconnue est déclarée avec un
     * `perTurn` nul : elle n'existe que le temps de l'octroi.
     */
    void grant(std::string_view id, int amount = 1);

    /// @brief Le début du tour du porteur : chaque ressource revient à son `perTurn`.
    void refresh();

    /// @brief Toutes les ressources déclarées, dans l'ordre de déclaration.
    [[nodiscard]] const std::vector<ActionResource>& resources() const noexcept {
        return _resources;
    }

private:
    [[nodiscard]] ActionResource* find(std::string_view id);
    [[nodiscard]] const ActionResource* find(std::string_view id) const;

    std::vector<ActionResource> _resources;
};

}  // namespace core
