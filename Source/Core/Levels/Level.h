// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileLayer.h"
#include "Core/Levels/TileMap.h"

/**
 * @file Core/Levels/Level.h
 * @brief Carte chargée : grille de collision, couches, entités, point d'entrée, variante.
 */

namespace core {

/**
 * @brief Composantes d'une carte, nommées — agrégat de construction de `core::Level` (`LOT-03`).
 *
 * S'écrit avec les *designated initializers* de C++20, qui rendent chaque site de construction
 * lisible sans commentaire :
 * @code
 * core::Level level(core::LevelData{.name = "village",
 *                                   .tileMap = std::move(map),
 *                                   .entry = entryPosition});
 * @endcode
 *
 * @note `tileMap` n'a **volontairement** pas de valeur par défaut : `core::TileMap` n'est pas
 *       constructible par défaut, si bien que l'omettre est une **erreur de compilation** et non
 *       une grille vide silencieuse. Tous les autres champs ont un défaut utile.
 */
struct LevelData {
    /// Nom de la carte.
    std::string name{};
    /// Grille de tuiles typées. Sans défaut : voir la note ci-dessus.
    ///
    /// C'est la grille de **collision** de la carte — celle que consomment l'exploration et la
    /// grille de combat tactique — et elle porte aussi l'entrée. Le format ne connaît qu'elle sur
    /// ce point : une couche `collision` déclarée à côté est refusée au chargement, précisément
    /// pour qu'il n'existe jamais deux grilles à tenir d'accord (`EX-LVL-016`).
    TileMap tileMap;
    /// Couches de tuiles de la carte (`LOT-04`), dans leur ordre de superposition.
    ///
    /// La **première** est `tileMap` ci-dessus, promue par le chargeur : de rôle `Collision` quand
    /// la carte déclare des couches visibles, `Legacy` quand elle n'en déclare aucune (une grille
    /// plate `version: 2`, qui vaut alors décor **et** collision). Un consommateur boucle donc sur
    /// `layers` sans cas particulier. Vide seulement pour un `Level` construit **directement**,
    /// sans passer par le chargeur — le rendu retombe alors sur `tileMap`.
    std::vector<TileLayer> layers{};
    /// Entités placées sur la carte (`LOT-04`) : PNJ, coffres, panneaux, portails, déclencheurs.
    std::vector<MapEntity> entities{};
    /// Point d'arrivée par défaut (case `Entry`).
    GridPosition entry{};
    /// Cases de collision **forcées à la main** (format v4, décision D10) : là, la grille de
    /// collision peut s'écarter de ce que `core::deriveCollision` tire des pièces et des types.
    /// Partout ailleurs, `LevelEditor --check` exige que la grille égale la déduction. Triées
    /// (ligne, colonne), sans doublon.
    std::vector<GridPosition> forcedCollision{};
    /// Prochain identifiant d'entité à donner (décision D8) : un compteur écrit dans la carte, pour
    /// qu'un identifiant retiré ne soit **jamais** redonné à une autre entité. 1 par défaut.
    int nextEntityId = 1;
    /// Carte dont celle-ci est une **variante** (décision D12), par son identifiant
    /// (`coliseum`, `capital/martpart`) ; vide pour une carte ordinaire. Une variante ne porte ni
    /// case ni couche : elle reprend celles de sa base, change de planche (`scene`) et porte ses
    /// propres entités (`core::applyVariant`).
    std::string base{};
    /// Planche que la variante substitue à celle de sa base ; vide pour la garder.
    std::string scene{};
};

/**
 * @brief Carte complète en mémoire.
 *
 * Assemblée par le chargeur (après parsing et validation) puis lue par l'exploration, le combat et
 * le rendu. Donnée pure (`EX-ARCH-011`, `EX-LVL-002`) : aucune dépendance rendu ni fichier.
 */
class Level {
public:
    /**
     * @brief Construit une carte à partir de ses composantes nommées.
     * @param data Composantes de la carte (déplacées).
     */
    explicit Level(LevelData data)
        : _name(std::move(data.name)),
          _tileMap(std::move(data.tileMap)),
          _layers(std::move(data.layers)),
          _entities(std::move(data.entities)),
          _entry(data.entry),
          _forcedCollision(std::move(data.forcedCollision)),
          _nextEntityId(data.nextEntityId),
          _base(std::move(data.base)),
          _scene(std::move(data.scene)) {}

    /// @return Le nom de la carte.
    [[nodiscard]] const std::string& name() const noexcept {
        return _name;
    }

    /// @return La grille de tuiles de la carte.
    [[nodiscard]] const TileMap& tileMap() const noexcept {
        return _tileMap;
    }

    /// @return Les couches de tuiles de la carte (`LOT-04`), dans leur ordre de superposition,
    /// la grille de collision en tête (voir `LevelData::layers`).
    [[nodiscard]] const std::vector<TileLayer>& layers() const noexcept {
        return _layers;
    }

    /// @return Les entités placées sur la carte (`LOT-04`).
    [[nodiscard]] const std::vector<MapEntity>& entities() const noexcept {
        return _entities;
    }

    /// @return Le point d'arrivée par défaut.
    [[nodiscard]] GridPosition entry() const noexcept {
        return _entry;
    }

    /// @return Les cases de collision forcées à la main (`LevelData::forcedCollision`).
    [[nodiscard]] const std::vector<GridPosition>& forcedCollision() const noexcept {
        return _forcedCollision;
    }

    /// @return Le prochain identifiant d'entité à donner (`LevelData::nextEntityId`).
    [[nodiscard]] int nextEntityId() const noexcept {
        return _nextEntityId;
    }

    /// @return La carte dont celle-ci est une variante, vide pour une carte ordinaire.
    [[nodiscard]] const std::string& base() const noexcept {
        return _base;
    }

    /// @return La planche que la variante substitue à celle de sa base, vide sinon.
    [[nodiscard]] const std::string& scene() const noexcept {
        return _scene;
    }

    /// @return Toutes les composantes de la carte, recopiées — ce que l'écrivain et le brouillon
    ///         reprennent sans en oublier une.
    [[nodiscard]] LevelData data() const {
        return LevelData{.name = _name,
                         .tileMap = _tileMap,
                         .layers = _layers,
                         .entities = _entities,
                         .entry = _entry,
                         .forcedCollision = _forcedCollision,
                         .nextEntityId = _nextEntityId,
                         .base = _base,
                         .scene = _scene};
    }

private:
    std::string _name;
    TileMap _tileMap;
    std::vector<TileLayer> _layers;
    std::vector<MapEntity> _entities;
    GridPosition _entry;
    std::vector<GridPosition> _forcedCollision;
    int _nextEntityId = 1;
    std::string _base;
    std::string _scene;
};

}  // namespace core
