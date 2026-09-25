// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Combat/BattleGrid.h
 * @brief La grille de combat : ce qui fait obstacle, ce qui ralentit, et qui se tient où
 *        (`LOT-19`, `EX-CBT-020`).
 */

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Combat/Damage.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/LevelProperties.h"
#include "Core/Levels/TileMap.h"
#include "Core/Rpg/RpgEnums.h"

namespace core {

class Level;

/**
 * @brief Identifiant d'un combattant sur la grille.
 *
 * Un type **fort**, et non un `int` : la grille ne sait rien de ce qu'est un combattant — fiche,
 * créature, camp —, elle ne retient que sa place. C'est le combat (`core::CombatState`, `LOT-20`)
 * qui attribue les identifiants ; un entier nu se serait confondu avec un indice de case à la
 * première signature qui prend les deux.
 */
enum class CombatantId : std::uint32_t {};

/**
 * @brief Côté, en cases, de l'emprise d'une créature de taille @p size.
 *
 * Manuel des Joueurs, « Taille des créatures » : TP, P et M tiennent dans une case de 1,50 m, G
 * dans 2 × 2, TG dans 3 × 3, Gig dans 4 × 4. Une très petite créature occupe **une case entière**
 * ici, là où le livre en tolère quatre dans la même : c'est le critère du lot — « deux créatures
 * ne partagent jamais une case » —, et la règle du livre se rouvrira, si un contenu la réclame, par
 * une exception nommée et non par un partage silencieux.
 */
[[nodiscard]] constexpr int footprintSide(CreatureSize size) noexcept {
    switch (size) {
        case CreatureSize::Tiny:
        case CreatureSize::Small:
        case CreatureSize::Medium:
            return 1;
        case CreatureSize::Large:
            return 2;
        case CreatureSize::Huge:
            return 3;
        case CreatureSize::Gargantuan:
            return 4;
    }
    return 1;
}

/**
 * @brief Comment un combattant se déplace : au sol, ou en vol.
 *
 * ## L'altitude est un attribut, jamais une géométrie
 *
 * Le jeu est vu de dessus, et sa grille n'a pas de hauteur. Donner une altitude aux cases — une
 * grille par étage, ou une case « en l'air » au-dessus d'une autre — ferait de chaque requête de
 * portée, de chemin et d'occupation une requête en trois dimensions, pour un contenu qui n'en a
 * besoin qu'au passage d'un dragon. Le vol est donc une **manière de traverser** la même grille :
 *
 * - un volant franchit les obstacles **au sol** — l'eau profonde, la falaise — et ignore le terrain
 *   difficile, qui est une gêne de sol ;
 * - il ne traverse pas la **matière pleine** : un mur de donjon monte jusqu'à la voûte, et la
 *   grille de collision ne sait pas distinguer un muret d'un rempart ;
 * - il occupe sa case comme tout autre combattant : deux créatures ne s'y superposent pas, l'une
 *   en vol et l'autre au sol.
 *
 * Qu'un volant reste ciblable à portée découle de la même décision, et c'est le `LOT-22` qui la
 * lira : une portée se compte sur la grille, sans hauteur à ajouter.
 */
enum class Locomotion {
    Walk,
    Fly,
};

/// @brief Nom de la propriété de zone qui rend une case difficile (`EX-LVL-018`).
inline constexpr std::string_view DIFFICULT_TERRAIN_PROPERTY = "difficultTerrain";

/**
 * @brief Les trois abris du Manuel des Joueurs (chapitre 9, « Abri »), et l'absence d'abri.
 *
 * Ordonnés du moins au plus protecteur : « si une cible se positionne derrière plusieurs types
 * d'abri, seul celui qui la protège le plus est pris en compte », et comparer deux abris est ce que
 * cette règle demande. Le calcul est au `LOT-22` (`core::coverBetween`).
 */
enum class Cover : std::uint8_t {
    None,
    /// Au moins la moitié du corps : +2 à la CA et aux sauvegardes de Dextérité.
    Half,
    /// Au moins les trois quarts : +5.
    ThreeQuarters,
    /// Complètement dissimulée : ne peut pas être ciblée directement.
    Total,
};

/**
 * @brief Un objet posé sur une case : une toile, une barricade, un tonneau.
 *
 * Il a des **points de vie** parce que le corpus en donne (les toiles du Sourcebook : CA 10,
 * 10 PV) et qu'un objet de grille se détruit ; il n'est pas une créature, et n'entre donc ni dans
 * l'occupation ni dans l'ordre du tour.
 */
struct GridObject {
    /// Nature de l'objet, libre (`"web"`, `"barricade"`) — la grille ne l'interprète pas.
    std::string kind;
    int hitPoints = 1;
    /// Vrai si l'objet empêche d'entrer dans sa case, comme un mur tant qu'il tient debout. Faux
    /// pour une toile, qu'on traverse — c'est l'état qu'elle inflige qui gêne, pas sa présence.
    bool blocksMovement = true;
    /**
     * @brief L'abri que l'objet procure à ce qui se tient derrière (`LOT-22`).
     *
     * Le Manuel nomme ses exemples : un muret ou un grand meuble abrite partiellement, une herse ou
     * une meurtrière abrite de façon importante. Aucun ne se déduit de `kind`, que la grille
     * n'interprète pas. Un objet à `Cover::Total` arrête la vue comme un mur ; une toile n'abrite
     * de rien.
     */
    Cover cover = Cover::None;
    /// Les structures résistent comme les créatures : une porte de fer ne craint pas le poison
    /// (`core::DamagePipeline::applyToStructure`, `LOT-21`).
    DamageTraits damageTraits;
};

/// @brief Pourquoi un placement a été refusé — ou qu'il a réussi.
enum class PlacementResult {
    Placed,
    /// L'emprise sort de la carte.
    OutOfBounds,
    /// L'emprise touche une case pleine ou un objet bloquant.
    Obstructed,
    /// L'emprise touche une case déjà tenue par un **autre** combattant, ou déjà chargée d'un
    /// objet.
    Occupied,
    /// `place` : le combattant est déjà sur la grille, ou son emprise n'est pas un côté valide.
    /// `moveTo` : il n'y est pas.
    InvalidCombatant,
};

/**
 * @brief La grille tactique d'une rencontre.
 *
 * ## Une seule source de vérité pour « peut-on se tenir ici »
 *
 * Les obstacles viennent de la **grille de collision** de la carte — `core::Level::tileMap()` —,
 * jamais d'un masque dessiné pour le combat (`EX-CBT-001`). La grille la **recopie** à la
 * construction : une copie ne peut pas changer sous un tour en cours.
 *
 * ## Les propriétés de zone
 *
 * Une couche de la carte qui porte des propriétés libres (`EX-LVL-018`) déclare une **zone** :
 * les cases non vides de cette couche. Une entité `zone` en déclare une aussi, rectangle ou
 * ensemble de cases peint (`core::zoneCells`, décision D13 de l'éditeur). La grille les relève
 * sans les interpréter, sauf une —
 * `difficultTerrain: true`, qui double le coût d'entrée. Les autres (combat interdit, aucun soin,
 * type de dégâts aléatoire) sont lues par ceux qui en ont l'usage (`LOT-50`, `LOT-81`) au moyen de
 * `zonesAt`, et ce lot n'en invente aucune.
 *
 * ## Ce qu'elle ne fait pas
 *
 * Elle ne calcule aucun chemin : c'est `core::findPath` et `core::ReachableArea`. Elle ne connaît
 * aucun camp : c'est la requête de déplacement qui dit qui l'on peut traverser. Elle n'a pas
 * d'altitude : l'altitude est un **attribut** du déplacement, jamais une géométrie de la grille
 * (voir `core::Locomotion`).
 */
class BattleGrid {
public:
    /// @brief Une grille sans zone, dont les obstacles sont les cases pleines de @p collision.
    explicit BattleGrid(const TileMap& collision);

    /**
     * @brief La grille de @p level : obstacles de @p collision, zones de ses couches.
     * @param level     La carte, dont les couches déclarent les zones.
     * @param collision La grille de collision à retenir — `level.tileMap()` pour la solidité
     *                  statique, ou une grille dérivée (une zone de combat découpée, par
     *                  exemple). Une couche aux dimensions différentes est ignorée.
     */
    BattleGrid(const Level& level, const TileMap& collision);

    /// @brief La grille de @p level, sur sa seule solidité statique.
    explicit BattleGrid(const Level& level);

    /// @brief Le nombre de colonnes de la grille.
    [[nodiscard]] int width() const noexcept {
        return _width;
    }
    /// @brief Le nombre de lignes de la grille.
    [[nodiscard]] int height() const noexcept {
        return _height;
    }
    /// @brief Vrai si @p cell est dans la grille : colonne et ligne positives, sous ses dimensions.
    [[nodiscard]] bool inBounds(GridPosition cell) const noexcept;

    /**
     * @brief Vrai si la case arrête un combattant qui se déplace par @p locomotion : matière
     *        pleine, objet bloquant, et — au sol seulement — eau profonde et falaise.
     *
     * Hors de la carte compte pour plein — un appelant qui oublie la borne doit se heurter à un
     * mur, pas lire hors du tableau.
     */
    [[nodiscard]] bool isObstructed(GridPosition cell,
                                    Locomotion locomotion = Locomotion::Walk) const;

    /// @brief Vrai si entrer dans la case coûte double **au sol** — un volant n'en tient pas compte
    /// (`core::Locomotion`).
    [[nodiscard]] bool isDifficult(GridPosition cell) const;

    /**
     * @brief Vrai si la case arrête la vue : matière pleine, ou objet à abri total (`LOT-22`).
     *
     * L'eau profonde et la falaise arrêtent la marche, pas le regard : on voit par-dessus un
     * gouffre. Hors de la carte, faux — un tracé entre deux cases de la carte n'en sort jamais, et
     * les bords ne sont pas des murs pour la vue.
     */
    [[nodiscard]] bool blocksSight(GridPosition cell) const;

    /// @brief L'abri que procure l'objet posé sur la case, `Cover::None` s'il n'y en a pas.
    [[nodiscard]] Cover objectCoverAt(GridPosition cell) const;

    /**
     * @brief Rend une case difficile, ou la rend à la normale — en cours de combat.
     *
     * Le terrain difficile n'est pas seulement une donnée de carte : un séisme en crée, un sort
     * l'efface. Une case hors de la carte est ignorée.
     */
    void setDifficult(GridPosition cell, bool difficult);

    /// @brief Les propriétés de toutes les zones qui couvrent la case, dans l'ordre des couches.
    /// Les pointeurs vivent aussi longtemps que la grille.
    [[nodiscard]] std::vector<const PropertyMap*> zonesAt(GridPosition cell) const;

    /**
     * @brief Place @p combatant, d'emprise @p side × @p side, avec @p anchor pour coin haut-gauche.
     *
     * Refuse plutôt que de corriger : une case voulue qui tombe dans un mur est une information
     * pour le montage de la rencontre (`core::placeCombatants`), et la déplacer d'office cacherait
     * une formation mal écrite. Un combattant placé **au sol** ne peut l'être sur l'eau profonde ;
     * un volant, si — il s'y tient en vol stationnaire.
     */
    [[nodiscard]] PlacementResult place(CombatantId combatant, GridPosition anchor, int side = 1,
                                        Locomotion locomotion = Locomotion::Walk);

    /**
     * @brief Déplace un combattant déjà placé.
     *
     * Ne vérifie **aucun** chemin ni budget : c'est l'appelant qui a validé le trajet
     * (`core::ReachableArea`). La grille ne tient que l'occupation, et refuse tout ce qui la
     * violerait.
     */
    [[nodiscard]] PlacementResult moveTo(CombatantId combatant, GridPosition anchor,
                                         Locomotion locomotion = Locomotion::Walk);

    /// @brief Retire un combattant de la grille (mort, fuite). Faux s'il n'y était pas.
    bool remove(CombatantId combatant);

    /// @brief Le combattant qui tient la case, s'il y en a un.
    [[nodiscard]] std::optional<CombatantId> occupantAt(GridPosition cell) const;
    /// @brief Le coin haut-gauche de l'emprise du combattant, s'il est sur la grille.
    [[nodiscard]] std::optional<GridPosition> positionOf(CombatantId combatant) const;
    /// @brief Le côté de l'emprise du combattant, ou 0 s'il n'est pas sur la grille.
    [[nodiscard]] int sideOf(CombatantId combatant) const;

    /**
     * @brief Vrai si une emprise @p side × @p side ancrée en @p anchor tient sur la grille : dans
     *        les bornes, sans obstacle pour @p locomotion, et sans autre combattant que @p self.
     */
    [[nodiscard]] bool canStand(GridPosition anchor, int side,
                                std::optional<CombatantId> self = std::nullopt,
                                Locomotion locomotion = Locomotion::Walk) const;

    /**
     * @brief Vrai si l'emprise @p side × @p side ancrée en @p anchor ne touche aucun obstacle pour
     *        @p locomotion, sans regarder les combattants.
     */
    [[nodiscard]] bool isClear(GridPosition anchor, int side,
                               Locomotion locomotion = Locomotion::Walk) const;

    /// @brief Pose un objet sur une case. Refusé hors carte, sur une case pleine, sur un autre
    /// objet, et — pour un objet bloquant — sur une case tenue par un combattant.
    [[nodiscard]] PlacementResult placeObject(GridPosition cell, GridObject object);
    /// @brief L'objet posé sur la case, ou `nullptr`.
    [[nodiscard]] const GridObject* objectAt(GridPosition cell) const;
    /**
     * @brief Inflige @p damage points de dégâts à l'objet de la case.
     * @return Vrai si l'objet est détruit par ce coup — il quitte alors la grille, et sa case
     *         redevient franchissable.
     */
    bool damageObject(GridPosition cell, int damage);

    /// @brief Tous les combattants placés, par identifiant croissant.
    [[nodiscard]] std::vector<CombatantId> combatants() const;

private:
    struct Placement {
        GridPosition anchor;
        int side = 1;
    };
    struct Zone {
        PropertyMap properties;
        std::vector<bool> cells;
    };

    [[nodiscard]] std::size_t indexOf(GridPosition cell) const noexcept;
    void fill(const Placement& placement, std::optional<CombatantId> occupant);
    void addZones(const Level& level);
    /// @brief Ajoute une zone par couche qui porte des propriétés.
    /// @param level La carte dont on lit les couches.
    void addLayerZones(const Level& level);
    /// @brief Ajoute une zone par entité de zone (décision D13), après celles des couches.
    /// @param level La carte dont on lit les entités.
    void addEntityZones(const Level& level);
    [[nodiscard]] PlacementResult check(GridPosition anchor, int side, CombatantId self,
                                        Locomotion locomotion) const;

    /// Ce qu'une case de la carte oppose au déplacement.
    enum class Terrain : std::uint8_t {
        Open,
        /// Eau profonde, falaise : infranchissable au sol, survolé.
        GroundObstacle,
        /// Mur, matière pleine, bloc : infranchissable, y compris en vol.
        Solid,
    };

    int _width;
    int _height;
    std::vector<Terrain> _terrain;
    std::vector<bool> _difficult;
    std::vector<std::optional<CombatantId>> _occupants;
    std::vector<Zone> _zones;
    /// Ordonnés : parcourir les combattants ou les objets doit donner le même ordre d'une partie à
    /// l'autre, et un conteneur non ordonné ne le garantit pas.
    std::map<CombatantId, Placement> _placements;
    std::map<std::size_t, GridObject> _objects;
};

}  // namespace core
