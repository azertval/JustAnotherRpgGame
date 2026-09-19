// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/Level.h"
#include "Core/Resources/ScenePieceManifest.h"
#include "HMI/Graphics/PlaceAppearance.h"

/**
 * @file Editor/Logic/MapFormat.h
 * @brief La garde du format de carte v4 : la **migration** d'une carte et le **contrôle** de toutes
 *        (`LOT-EDITOR-12`, constat A12, `EX-EDIT-062`). Logique pure, sans fenêtre.
 *
 * Ce que `LevelEditor --migrate` et `LevelEditor --check` appellent (`runMapCommand`), et ce que
 * les tests appellent directement. Les messages sont en anglais, comme toute l'IHM de l'éditeur.
 *
 * ## La migration
 *
 * Une carte se convertit par un acte explicite, jamais par l'effet de bord d'un enregistrement
 * (règle 1 de la feuille de route). Migrer une carte, c'est :
 *
 * 1. la lire — le chargeur range déjà les assignations `"texture"` d'une v3 comme pièces du décor ;
 * 2. **nommer la pièce** de chaque case de couche qui n'en nomme pas, par la table d'apparence du
 *    lieu : ce qu'on voyait est désormais écrit, et le type ne garde que son sens de règle (D3) ;
 * 3. donner un **identifiant** à chaque entité qui n'en a pas (D8) ;
 * 4. **déduire la collision** (`core::deriveCollision`) : partout où la grille écrite s'accorde
 *    avec la déduction, elle en prend la valeur ; ailleurs, elle garde la sienne et la case devient
 *    **forcée** (D10). Le jeu ne voit donc aucune différence ;
 * 5. l'écrire en v4 canonique.
 *
 * ## Le contrôle
 *
 * Pour chaque carte de `Levels/` : version courante, écriture canonique (charger puis écrire rend
 * le même fichier), références (portails, points d'arrivée, zones de combat), pièces présentes au
 * manifeste, collision égale à la déduction hors des cases forcées, identifiants d'entité. Les
 * avertissements ne font pas échouer : réserve de hauteur non nulle, pièce citée par un ancien nom,
 * gêne et abri pas encore joués, case forcée inutile, pièces larges qui se recouvrent.
 *
 * Puis le **contenu** (`LOT-EDITOR-07`, `ContentCheck.h`) : références des entités, terrain des
 * rencontres, atteignabilité, portails sans retour, points d'arrivée orphelins, clés de traduction.
 */

namespace hmi {

/// @brief Ce que le lieu d'une carte apporte : son manifeste et sa table, s'ils existent.
struct PlaceAssets {
    std::optional<core::ScenePieceManifest> manifest;
    std::optional<PlaceAppearance> appearance;
};

/// @return Le manifeste et la table du lieu @p place, lus dans `<dataRoot>/Assets/Scene/<place>/`.
[[nodiscard]] PlaceAssets loadPlaceAssets(const std::filesystem::path& dataRoot,
                                          std::string_view place);

/// @return Les lieux qu'une carte peut prendre : les dossiers de `<dataRoot>/Assets/Scene` qui ont
///         un manifeste de pièces, triés (`LOT-EDITOR-06`).
[[nodiscard]] std::vector<std::string> scenePlaces(const std::filesystem::path& dataRoot);

/// @return Les fichiers de carte de `<dataRoot>/Levels`, sous-dossiers compris, triés — sans les
///         séquences ni les annexes de l'éditeur.
[[nodiscard]] std::vector<std::filesystem::path> mapFiles(const std::filesystem::path& dataRoot);

/// @brief Gravité d'un constat du contrôle.
enum class MapCheckSeverity {
    Error,
    Warning,
};

/// @brief Un constat du contrôle, sur une carte, et parfois sur une case.
struct MapCheckFinding {
    MapCheckSeverity severity = MapCheckSeverity::Error;
    std::string mapId;
    std::optional<core::GridPosition> cell;
    std::string message;
    /// L'entité en cause, par son `id`, vide si le constat n'en vise aucune : le panneau
    /// « Problems » la sélectionne (`LOT-EDITOR-07`).
    std::string entityId;

    [[nodiscard]] bool operator==(const MapCheckFinding&) const = default;
};

/// @return Le constat en une ligne : `capital/martpart (12, 3): error: …`.
[[nodiscard]] std::string formatFinding(const MapCheckFinding& finding);

struct ContentContext;

/**
 * @brief Contrôle une carte, lue depuis son fichier : son format, puis son contenu.
 * @param mapId    Son identifiant (`capital/martpart`), pour les messages.
 * @param file     Son fichier.
 * @param dataRoot La racine des données (`Source/Elements`), où vivent les planches.
 * @param context  Ce contre quoi le contenu se contrôle (`hmi::loadContentContext`).
 */
[[nodiscard]] std::vector<MapCheckFinding> checkMapFile(std::string_view mapId,
                                                        const std::filesystem::path& file,
                                                        const std::filesystem::path& dataRoot,
                                                        const ContentContext& context);

/// @brief Comme ci-dessus, le contexte étant lu sous @p dataRoot pour cette seule carte.
[[nodiscard]] std::vector<MapCheckFinding> checkMapFile(std::string_view mapId,
                                                        const std::filesystem::path& file,
                                                        const std::filesystem::path& dataRoot);

/// @brief Le bilan du contrôle de toutes les cartes.
struct MapCheckReport {
    std::size_t maps = 0;
    std::vector<MapCheckFinding> findings;

    [[nodiscard]] std::size_t count(MapCheckSeverity severity) const;
    [[nodiscard]] bool ok() const {
        return count(MapCheckSeverity::Error) == 0;
    }
};

/**
 * @brief Contrôle toutes les cartes de `<dataRoot>/Levels`, sous-dossiers compris, puis les
 *        références entre elles.
 */
[[nodiscard]] MapCheckReport checkAllMaps(const std::filesystem::path& dataRoot);

/// @brief Ce que la migration d'une carte a produit.
struct MapMigration {
    /// Le texte v4 canonique, vide en cas d'échec.
    std::string text;
    /// Message d'échec, vide en cas de succès.
    std::string error;
    /// Pièces nommées d'après la table du lieu.
    std::size_t namedPieces = 0;
    /// Identifiants donnés.
    std::size_t newIds = 0;
    /// Cases forcées ajoutées, là où la grille écrite s'écarte de la déduction.
    std::size_t newForcedCells = 0;

    [[nodiscard]] bool ok() const {
        return error.empty();
    }
};

/**
 * @brief Migre une carte chargée (étapes 2 à 5 de l'en-tête).
 * @param level  La carte, toutes versions confondues.
 * @param assets Le manifeste et la table de son lieu (vides pour une carte sans lieu).
 */
[[nodiscard]] MapMigration migrateLevel(const core::Level& level, const PlaceAssets& assets);

/// @brief Lit @p file et le migre, le lieu étant cherché sous @p dataRoot.
[[nodiscard]] MapMigration migrateMapFile(const std::filesystem::path& file,
                                          const std::filesystem::path& dataRoot);

/**
 * @brief L'entrée sans fenêtre de l'éditeur (décision D9) : `--check`, `--migrate` et `--apply`
 *        (`--render`, qui peint, vit du côté de l'IHM : `hmi::runRenderCommand`).
 *
 * - `--data <racine>` : la racine des données, `Source/Elements` ou le dossier de l'exécutable
 *   (défaut : @p defaultDataRoot) ;
 * - `--check` : contrôle toutes les cartes ; sortie 1 à la première erreur, 0 sinon ;
 * - `--migrate [carte…]` : migre en place les cartes nommées (identifiant ou chemin), toutes à
 *   défaut ; `--output <fichier>` écrit ailleurs une carte unique ;
 * - `--apply <gestes.json> [carte]` : rejoue les gestes du fichier sur la carte (celle que nomme le
 *   fichier, à défaut) et l'écrit en place, ou dans `--output` ; un geste refusé n'écrit rien et
 *   sort en 1 (`hmi::applyGestureFile`). Suivi de `--check`, il contrôle ensuite toutes les cartes.
 *
 * @return Le code de sortie, ou `std::nullopt` si la ligne de commande ne demande aucune de ces
 *         commandes — l'éditeur ouvre alors sa fenêtre.
 */
[[nodiscard]] std::optional<int> runMapCommand(const std::vector<std::string>& arguments,
                                               const std::filesystem::path& defaultDataRoot,
                                               std::string& output);

}  // namespace hmi
