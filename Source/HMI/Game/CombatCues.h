// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file HMI/Game/CombatCues.h
 * @brief Ce que le combat **montre** après ce qu'il a décidé : les pas, les coups, les chutes,
 *        joués dans le temps par les figurines (`LOT-118`).
 *
 * ## Pourquoi une file
 *
 * La session de combat (`core::ArenaSession`) est instantanée : un tour de l'IA — approche,
 * attaque, repli — se joue en un appel, et la grille est déjà dans son état final quand l'écran
 * la relit. Dessiner cet état, c'est montrer des combattants qui **se téléportent** ; c'est ce que
 * l'auteur voyait quand « l'IA ne se déplaçait jamais vers le joueur ».
 *
 * Cette file reçoit donc les faits du combat au moment où ils se produisent — un pas et son
 * chemin, une attaque et sa cible, un coup encaissé, une chute — et les **rejoue** à la vitesse
 * du monde : la marche à deux cases par seconde (celle de l'exploration), une action le temps de
 * sa bande. Tant qu'elle joue, l'écran attend ; quand elle est vide, la grille et l'image disent
 * la même chose.
 *
 * Elle ne connaît ni la session, ni les textures, ni Qt : des identifiants, des cases, des
 * secondes. C'est ce qui la rend vérifiable sans fenêtre.
 */

#include <cstddef>
#include <deque>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

#include "Core/Combat/BattleGrid.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Math/Vector2.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace hmi {

/// @brief Ce qu'un combattant fait de visible.
enum class CombatCueKind {
    /// Il marche le long de `path`.
    Walk,
    /// Il frappe `target`.
    Attack,
    /// Il lance un sort vers `target` (personne ne le demande encore : la bande est prête).
    Cast,
    /// Il encaisse un coup.
    Hit,
    /// Il tombe, et reste à terre.
    Death,
};

/// @brief Un fait du combat à montrer.
struct CombatCue {
    CombatCueKind kind = CombatCueKind::Walk;
    core::CombatantId actor{};
    /// `Walk` : les cases traversées, départ **exclu**, arrivée incluse (`core::Path::steps`).
    std::vector<core::GridPosition> path;
    /// `Attack`, `Cast` : la case visée, pour tourner la figurine vers elle.
    std::optional<core::GridPosition> target;
};

/// @brief Ce qu'une figurine montre à cet instant.
struct FigureMotion {
    /// Position **continue**, en cases de la grille : `{1.5, 2.5}` est le centre de la case (1, 2).
    core::Vector2 point{};
    /// La bande en cours (`hmi::figure_clips`).
    std::string_view clip = figure_clips::IDLE;
    /// La diagonale vers laquelle elle regarde.
    FigureFacing facing = FigureFacing::SouthEast;
    /// Temps écoulé depuis le début de la bande, en secondes.
    float clipSeconds = 0.0F;
    /// À terre : sa bande de mort reste sur sa dernière image, et rien ne la relève.
    bool dead = false;
};

class CombatCueTrack {
public:
    /// La marche du combat est celle du monde : deux cases par seconde (`LOT-112`, D5).
    static constexpr float WALK_CELLS_PER_SECOND = 2.0F;
    /// Une action ponctuelle dure sa bande : huit images à 80 ms (`LOT-112`).
    static constexpr float ACTION_SECONDS = 0.64F;
    /// Le coup **porte** au milieu de la bande d'attaque : le touché et la chute de la cible
    /// commencent là, pas quand l'attaquant a fini son geste.
    static constexpr float IMPACT_FRACTION = 0.5F;

    /// @brief Pose @p actor au repos en @p cell, tout de suite (montage, rejeu, repli).
    void place(core::CombatantId actor, core::GridPosition cell,
               FigureFacing facing = FigureFacing::SouthEast);
    /// @brief Retire @p actor : il est sorti du combat.
    void remove(core::CombatantId actor);
    /// @brief Ajoute un fait à montrer, après ceux déjà en attente.
    void push(CombatCue cue);
    /// @brief Avance de @p seconds : les faits en cours progressent, le suivant démarre.
    void advance(float seconds);
    /// @brief Joue instantanément tout ce qui reste : l'image rejoint la grille.
    void finishAll();
    /// @brief Oublie tout : la fin du combat.
    void clear() noexcept;

    /// @return Vrai tant qu'un fait se joue ou en attend un autre.
    [[nodiscard]] bool busy() const noexcept {
        return !_running.empty() || !_queue.empty();
    }
    /// @return Ce que @p actor montre, ou `nullptr` s'il n'est pas posé.
    [[nodiscard]] const FigureMotion* motionOf(core::CombatantId actor) const;
    /// @return Le nombre de faits encore à jouer, en cours compris.
    [[nodiscard]] std::size_t pending() const noexcept {
        return _running.size() + _queue.size();
    }

private:
    struct Running {
        CombatCue cue;
        /// Négatif : le fait attend son heure (un touché qui suit une attaque).
        float elapsed = 0.0F;
        /// La case d'où part une marche.
        core::Vector2 from{};
    };

    /// Démarre le fait suivant, et ceux qui l'accompagnent.
    void startNext();
    /// Applique @p running à sa figurine ; @return vrai s'il est fini.
    bool apply(Running& running);

    std::map<core::CombatantId, FigureMotion> _figures;
    std::deque<CombatCue> _queue;
    std::vector<Running> _running;
};

}  // namespace hmi
