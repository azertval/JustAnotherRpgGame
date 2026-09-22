// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

/**
 * @file Core/Levels/TileType.h
 * @brief Types de tuiles d'une carte et utilitaires associés.
 */

namespace core {

/**
 * @brief Type d'une tuile de la grille d'une carte (`EX-LVL-002`).
 *
 * `Empty` est la case traversable par défaut ; `Solid` bloque le déplacement ; `Entry` est le point
 * d'arrivée par défaut du héros sur la carte. Le reste est le vocabulaire du **terrain** en vue de
 * dessus (`EX-EXP-005`, `LOT-08`) : `Grass`, `Dirt`, `Sand`, `Water` et `DeepWater` (sols, les
 * quatre premiers traversables), `Wall` et `Cliff` (obstacles), `Bridge` et `Stairs` (passages).
 * Les vingt derniers étoffent ce vocabulaire pour qu'une carte **sans texture** se dessine avec
 * précision : sols (pavé, ruelle, plancher, dallage, neige), terrains difficiles, végétation,
 * mobilier, bâti vu de dessus et dangers. Leur règle de pas est `core::tacticalOfTileType`.
 *
 * Ce qui **agit** sur une carte — PNJ, coffres, portails, rencontres — n'est pas un type de tuile
 * mais une entité (`core::MapEntity`, `EX-LVL-017`) : une grille ne retient qu'un type par case.
 */
enum class TileType {
    Empty,
    Solid,
    Entry,
    // --- Terrain du RPG en vue de dessus (LOT-08) ---
    /// Herbe : le sol par défaut d'une carte extérieure.
    Grass,
    /// Terre battue : chemins, cours, sols d'intérieur sommaires.
    Dirt,
    /// Sable : plages et déserts. Traversable, comme l'herbe et la terre.
    Sand,
    /// Eau **peu profonde** : traversable — on y patauge. Sa distinction d'avec `DeepWater` est
    /// la seule chose qui permette à une rive d'exister.
    Water,
    /// Eau **profonde** : infranchissable tant qu'aucune règle de nage n'existe (`isSolid`).
    DeepWater,
    /// Mur : la matière pleine bâtie, distincte de `Solid` (matière générique du socle) parce que
    /// l'éditeur et l'habillage doivent pouvoir les vêtir différemment.
    Wall,
    /// Falaise : rupture de relief infranchissable, l'obstacle **naturel** du décor extérieur.
    Cliff,
    /// Pont : franchit l'eau ou un ravin. Traversable, et c'est tout son intérêt.
    Bridge,
    /// Escalier : liaison verticale d'une carte à l'autre, traversable. Ce qu'il **relie** est une
    /// donnée du graphe de cartes (`LOT-09`), jamais du type de tuile.
    Stairs,
    // --- Vocabulaire de la maquette : ce qu'une carte sans texture doit pouvoir dire ---
    /// Pavé : le sol des rues et des places.
    Pavement,
    /// Ruelle : un pavé étroit et plus sombre, qu'une maquette doit distinguer de la rue.
    Alley,
    /// Plancher : le sol de bois d'un intérieur, d'un quai, d'une estrade.
    Planks,
    /// Dallage : le sol de pierre taillée d'un temple, d'une cour, d'une salle.
    Flagstone,
    /// Neige : un sol extérieur, traversable.
    Snow,
    /// Boue : terrain **difficile** (pas encore joué : traversable comme un sol).
    Mud,
    /// Éboulis : terrain **difficile**, comme la boue.
    Rubble,
    /// Porte : un passage dans un mur, traversable.
    Door,
    /// Buisson : végétation basse, terrain **difficile**.
    Bush,
    /// Arbre : arrête le pas et la vue.
    Tree,
    /// Rocher : arrête le pas, on voit par-dessus.
    Rock,
    /// Palissade, barrière : arrête le pas, on voit par-dessus.
    Fence,
    /// Muret : un **abri** (pas encore joué : traversable).
    LowWall,
    /// Étal, auvent : arrête le pas, on voit par-dessus.
    Stall,
    /// Caisses, tonneaux, mobilier : arrête le pas, on voit par-dessus.
    Crate,
    /// Colonne, statue, socle : arrête le pas et la vue.
    Column,
    /// Toit : le bâti vu de dessus ; arrête le pas et la vue.
    Roof,
    /// Gradins : arrêtent le pas et la vue.
    Tiers,
    /// Fosse, ravin, vide : arrête le pas, on voit par-dessus.
    Pit,
    /// Lave : arrête le pas, on voit par-dessus.
    Lava,
};

/**
 * @brief Nombre de valeurs de `core::TileType` — **seule** source de vérité de la fin de
 *        l'énumération.
 *
 * Ajouter un type ne demande rien d'autre que de l'ajouter ci-dessus, **avant** le dernier
 * énumérateur : cette constante suit, et ses consommateurs avec elle. L'ajouter en **fin** de
 * liste demande en plus de re-pointer cette seule ligne.
 *
 * Volontairement une constante libre plutôt qu'un énumérateur `Count` : les `switch` sur `TileType`
 * du projet sont **exhaustifs et sans `default`** (c'est ce qui fait que le compilateur désigne
 * lui-même les points à mettre à jour), et un énumérateur sentinelle les obligerait tous à traiter
 * un cas qui ne décrit aucune tuile.
 */
inline constexpr int TILE_TYPE_COUNT = static_cast<int>(TileType::Lava) + 1;

/**
 * @brief Indique si un type de tuile bloque le déplacement de manière **statique**.
 *
 * @note `DeepWater` **bloque** : ce n'est pas de la matière, mais rien ne permet encore de la
 *       franchir. Le jour où une règle de nage existera, elle sortira d'ici — et ce sera le seul
 *       endroit à changer, ce qui est précisément l'intérêt de l'y mettre plutôt que de parsemer
 *       le code de tests « sauf si c'est de l'eau ».
 * @param type Type de tuile.
 * @return `true` pour la matière pleine (`Solid`, `Wall`, `Cliff`), l'eau profonde, et tout type
 *         que `core::tacticalOfTileType` dit infranchissable (arbre, rocher, palissade, étal,
 *         caisses, colonne, toit, gradins, fosse, lave). Le muret et les terrains difficiles
 *         restent traversables tant que l'abri et la gêne ne sont pas joués.
 */
[[nodiscard]] constexpr bool isSolid(TileType type) noexcept {
    switch (type) {
        case TileType::Solid:
        case TileType::Wall:
        case TileType::Cliff:
        case TileType::DeepWater:
        case TileType::Tree:
        case TileType::Rock:
        case TileType::Fence:
        case TileType::Stall:
        case TileType::Crate:
        case TileType::Column:
        case TileType::Roof:
        case TileType::Tiers:
        case TileType::Pit:
        case TileType::Lava:
            return true;
        case TileType::Empty:
        case TileType::Entry:
        case TileType::Grass:
        case TileType::Dirt:
        case TileType::Sand:
        case TileType::Water:
        case TileType::Bridge:
        case TileType::Stairs:
        case TileType::Pavement:
        case TileType::Alley:
        case TileType::Planks:
        case TileType::Flagstone:
        case TileType::Snow:
        case TileType::Mud:
        case TileType::Rubble:
        case TileType::Door:
        case TileType::Bush:
        case TileType::LowWall:
            return false;
    }
    return false;
}

}  // namespace core
