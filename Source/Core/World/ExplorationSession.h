// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Core/Ecs/Components/Interactable.h"
#include "Core/Gameplay/WorldFlags.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/Level.h"
#include "Core/Math/Vector2.h"
#include "Core/World/WorldTravel.h"

/**
 * @file Core/World/ExplorationSession.h
 * @brief L'exploration du jeu : un héros qui marche sur une carte, parle à ce qu'il regarde, et
 *        passe d'une carte à l'autre (`LOT-09`).
 *
 * ## Pourquoi une session à part
 *
 * Le jeu dessine ses lieux comme l'arène, en projection isométrique et avec les planches de
 * l'atelier des textures (`LOT-92`). Cette session est donc la **jumelle de
 * `core::ArenaSession`** : de la simulation pure, sans GPU ni Qt (`EX-NFR-004`), qu'un rendu
 * quelconque met à l'écran — le jeu comme l'essai de l'éditeur.
 *
 * *Décision de l'auteur, 17 septembre 2026* : un lieu et une arène se dessinent par le **même**
 * code ; ils se simulent par deux sessions.
 *
 * ## Le repère
 *
 * La position du héros est **continue, en cases** : `{1.5, 2.5}` est le centre de la case (1, 2).
 * Le pas de simulation ne connaît donc ni pixels ni projection — c'est le rendu qui projette.
 * La collision se lit sur la grille racine de la carte, la seule qui porte le masque
 * (`core::LevelData::tileMap`) : il n'existe jamais deux grilles à tenir d'accord.
 */

namespace core {

/// @brief Position continue sur la grille, en cases.
struct CellPoint {
    float column = 0.0F;
    float row = 0.0F;

    [[nodiscard]] bool operator==(const CellPoint&) const = default;
};

/// @return La case qui contient @p point.
[[nodiscard]] GridPosition cellOf(CellPoint point) noexcept;

/// @return Le centre de @p cell, en cases. (`core::centerOf` existe deja, en
///         demi-cases de la ligne de vue : deux reperes, deux noms.)
[[nodiscard]] CellPoint cellCenter(GridPosition cell) noexcept;

/// @brief Ce que le joueur demande d'un pas de simulation.
struct ExplorationIntent {
    /// Direction voulue, de longueur au plus 1 (clavier ou manette, déjà normalisée).
    Vector2 move{};
    /// Vrai le pas où le joueur presse la touche d'interaction (`E`, bouton de manette).
    bool interact = false;
};

/// @brief Ce qu'un pas d'exploration a produit et que l'appelant doit jouer.
enum class ExplorationEventKind {
    /// Le héros est entré sur une carte. `value` la nomme.
    MapEntered,
    /// Un portail exige un drapeau que la partie n'a pas. `value` nomme le drapeau.
    PortalLocked,
    /// Un portail ne mène nulle part : la carte ou le point d'arrivée manque (`EX-NFR-040`).
    PortalBroken,
    /// Il faut ouvrir un dialogue. `value` nomme le dialogue.
    Dialogue,
    /// Il faut engager une rencontre. `value` la nomme.
    Encounter,
    /// Une interaction a eu lieu sans autre conséquence (coffre, panneau). `value` donne le type.
    Interacted,
};

/// @brief Un événement du pas, et où il a eu lieu.
struct ExplorationEvent {
    ExplorationEventKind kind = ExplorationEventKind::MapEntered;
    std::string value;
    GridPosition cell{};

    [[nodiscard]] bool operator==(const ExplorationEvent&) const = default;
};

/**
 * @brief Le héros sur la carte courante : marche, interaction, portails.
 *
 * Gelée (`freeze`), la session ne bouge plus et n'interagit plus : c'est l'état de la carte pendant
 * un dialogue ou un combat, qui la laisse telle quelle sans la détruire — la reprendre rendra le
 * lieu comme on l'a quitté.
 */
class ExplorationSession {
public:
    /// Vitesse de marche, en cases par seconde : 3 m/s, une marche vive. Un cycle de marche couvre
    /// une case, si bien que la cadence de la figurine en découle — sans quoi ses pieds glissent
    /// (`LOT-112`, décision de l'auteur ; 4 cases par seconde étaient une course).
    static constexpr float WALK_SPEED_CELLS_PER_SECOND = 2.0F;
    /// Demi-côté du gabarit du héros, en cases : il ne tient pas tout à fait une case, si bien
    /// qu'un couloir d'une case se franchit sans frotter les deux murs.
    static constexpr float HERO_HALF_SIZE_CELLS = 0.3F;

    explicit ExplorationSession(WorldTravel::MapLoader loader);

    /**
     * @brief Entre sur @p mapId, au point d'arrivée @p arrival (vide : l'entrée de la carte).
     * @return Vrai si la carte est là et le point d'arrivée connu ; `travel().lastIssue()` dit
     *         sinon pourquoi.
     */
    bool start(std::string_view mapId, std::string_view arrival);

    /**
     * @brief Avance d'un pas de @p seconds.
     *
     * L'ordre compte : on marche, puis on franchit le portail de la case atteinte, puis on
     * interagit. Marcher après avoir franchi ferait faire au héros un pas sur la carte d'arrivée
     * avec l'intention qui l'a fait entrer.
     */
    std::vector<ExplorationEvent> update(const ExplorationIntent& intent, float seconds);

    /// @return La carte courante, ou `nullptr` avant une entrée réussie.
    [[nodiscard]] const Level* map() const {
        return _travel.currentMap();
    }

    /// @return L'identifiant de la carte courante.
    [[nodiscard]] const std::string& mapId() const noexcept {
        return _travel.currentMapId();
    }

    /// @return La position continue du héros, en cases.
    [[nodiscard]] CellPoint heroPoint() const noexcept {
        return _hero;
    }

    /// @brief Pose le héros où l'appelant le veut — le retour du sable, à la case relevée.
    void placeHero(CellPoint point) noexcept;

    /// @return La case du héros.
    [[nodiscard]] GridPosition heroCell() const noexcept {
        return cellOf(_hero);
    }

    /// @return L'orientation du héros : la dernière direction non nulle qu'il a prise.
    [[nodiscard]] Vector2 facing() const noexcept {
        return _facing;
    }

    /// @return La case que le héros regarde (`core::aimedCell`).
    [[nodiscard]] GridPosition aimedCell() const;

    [[nodiscard]] WorldFlags& flags() noexcept {
        return _flags;
    }
    [[nodiscard]] const WorldFlags& flags() const noexcept {
        return _flags;
    }

    [[nodiscard]] const WorldTravel& travel() const noexcept {
        return _travel;
    }

    /// @brief Gèle ou dégèle la carte (dialogue, combat).
    void freeze(bool frozen) noexcept {
        _frozen = frozen;
    }
    [[nodiscard]] bool frozen() const noexcept {
        return _frozen;
    }

    /// @return Les entités interactives de la carte courante, dans l'ordre des entités.
    [[nodiscard]] const std::vector<Interactable>& interactables() const noexcept {
        return _interactables;
    }

private:
    /// Relit les entités interactives de la carte courante (changement de carte).
    void rebuildInteractables();
    /// @return Vrai si le gabarit du héros tient en @p point sans entrer dans du plein.
    [[nodiscard]] bool fits(CellPoint point) const;
    /// Marche d'un pas, axe par axe : un mur pris en biais fait glisser le long, il n'arrête pas.
    void walk(Vector2 move, float seconds);
    /// Franchit le portail de la case du héros, s'il y en a un.
    void crossPortal(std::vector<ExplorationEvent>& events);
    /// Résout l'interaction demandée.
    void resolveInteraction(std::vector<ExplorationEvent>& events);

    WorldTravel _travel;
    WorldFlags _flags;
    std::vector<Interactable> _interactables;
    CellPoint _hero{};
    Vector2 _facing{0.0F, 1.0F};
    /// Case du héros au pas précédent : un portail se franchit **en y arrivant**, pas à chaque pas
    /// où l'on reste dessus — sans quoi un portail qui ramène sur place bouclerait.
    GridPosition _lastCell{};
    bool _frozen = false;
};

}  // namespace core
