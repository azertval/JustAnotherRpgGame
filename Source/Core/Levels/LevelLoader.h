// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "Core/Levels/Level.h"

/**
 * @file Core/Levels/LevelLoader.h
 * @brief Chargement d'un niveau depuis le format JSON (liste de tuiles-objets).
 */

namespace core {

/**
 * @brief Catégorie d'échec de chargement/validation d'un niveau (`EX-EDIT-012`).
 *
 * Complète le message technique (`LevelLoadResult::error`) d'un code **programmatique**, pour
 * qu'un appelant (l'éditeur) puisse traduire une erreur en message non-codeur sans dépendre du
 * texte exact du message — une correspondance de sous-chaînes s'y casserait silencieusement au
 * moindre changement de formulation.
 */
enum class LevelValidationError {
    None,                      ///< Pas d'erreur (chargement réussi).
    ParseError,                ///< JSON malformé, champ obligatoire manquant, ou de mauvais type.
    UnknownTileType,           ///< Type de tuile non reconnu.
    OutOfBounds,               ///< Tuile positionnée hors des dimensions déclarées.
    DuplicatePosition,         ///< Deux tuiles à la même position.
    InvalidEntryCount,         ///< Zéro ou plusieurs tuiles d'entrée (une seule attendue).
    FileNotFound,              ///< Fichier de carte introuvable sur disque.
    UnsupportedFormatVersion,  ///< `"version"` du fichier supérieure à celle gérée (`EX-LVL-005`).
    DuplicateEntityId,         ///< Deux entités portent le même `id` (format v4, décision D8).
    MissingBase,               ///< Variante dont la base est introuvable, ou elle-même variante.
};

/**
 * @brief Version courante du format de niveau JSON (`EX-LVL-005`).
 *
 * Écrite par `LevelWriter` dans le champ racine `"version"`. Un fichier sans ce champ est lu
 * comme la version initiale (0), sans erreur ni avertissement — rétrocompatibilité des niveaux
 * antérieurs à ce champ. Une version supérieure à celle-ci est une erreur exploitable
 * (`LevelValidationError::UnsupportedFormatVersion`), pas une lecture au mieux. **Toute version
 * passée se lit pour toujours** : une fixture par version reste dans les tests.
 *
 * - Version 3 (`LOT-04`) : **couches de tuiles** (`"layers"`, `core::TileLayer`) et **entités**
 *   (`"entities"`, `core::MapEntity`). `"layers"` ne porte que les couches **visibles** (sol,
 *   décor) : la grille de collision d'une carte est son tableau racine `"tiles"`, celui qui porte
 *   l'entrée — une couche `"collision"` déclarée est refusée (`EX-LVL-016`). La grille racine est
 *   **promue** en couche de tête, de rôle `Collision`, ou `Legacy` sans couche déclarée.
 * - Version 4 (`LOT-EDITOR-12`, la seule révision du module éditeur) : chaque case de couche porte
 *   son `type` et une `piece` facultative — l'assignation `"texture"` de la grille racine est lue
 *   dans une carte v3 et rangée comme pièce de la couche de décor, refusée dans une v4 ; la grille
 *   racine est la collision **écrite**, avec ses cases forcées (`"forced"`) ; chaque entité a un
 *   `id` unique, et la carte un compteur (`"nextEntityId"`) ; une zone peut être peinte
 *   (`"cells"`) ; une carte peut être la **variante** d'une autre (`"base"`, `"scene"`) ; la
 *   hauteur est réservée (`"floor"` par couche, `"elevation"` par case et par entité).
 *
 * Toute clé non reconnue dans une couche ou une entité est rangée dans ses propriétés libres
 * (`core::PropertyMap`) et **réémise** à l'écriture : un fichier produit par une version
 * ultérieure de l'éditeur traverse une version antérieure sans rien perdre.
 */
inline constexpr int LEVEL_FORMAT_VERSION = 4;

/**
 * @brief Plus grand côté de carte accepté au chargement, en cases.
 *
 * L'éditeur plafonne ses cartes à 100 cases de côté ; cette borne, dix fois plus large, ne sert
 * qu'à refuser un fichier aberrant avant d'allouer sa grille (un côté de 100 000 cases réclame
 * des gigaoctets).
 */
inline constexpr int MAX_LEVEL_SIDE = 1024;

/**
 * @brief Résultat d'un chargement de niveau : soit un `Level`, soit une **erreur** décrite.
 *
 * En cas de succès, `level` contient le niveau, `error` est vide et `errorCode` vaut `None`. En
 * cas d'échec (récupérable, `EX-NFR-040`), `level` est vide, `error` décrit le problème de façon
 * exploitable pour les journaux/tests, et `errorCode` catégorise l'échec pour un traitement
 * programmatique (traduction non-codeur).
 */
struct LevelLoadResult {
    std::optional<Level> level;
    std::string error;
    LevelValidationError errorCode = LevelValidationError::None;

    /// @return true si le chargement a réussi.
    [[nodiscard]] bool ok() const noexcept {
        return level.has_value();
    }
};

/**
 * @brief Charge un niveau au format **JSON** (objet `{name, width, height, tiles}` où `tiles`
 *        est une liste d'objets `{x, y, type, …}`), `EX-LVL-001`/`EX-LVL-003`.
 *
 * Le chargement ne lève **aucune exception** vers l'appelant : toute erreur (JSON malformé,
 * champ manquant, type de tuile inconnu, tuile hors bornes, liaison de mécanisme non résolue…)
 * est renvoyée dans le `LevelLoadResult`. La **validation métier** additionnelle (unicité de
 * l'entrée/sortie, positions en double) relève de la validation du niveau.
 */
class LevelLoader {
public:
    /**
     * @brief Ce qui rend la carte de base d'une variante (décision D12), à partir de son
     *        identifiant (`central-empire/capital/martpart`).
     */
    using BaseResolver = std::function<LevelLoadResult(std::string_view baseId)>;

    /**
     * @brief Charge un niveau depuis une chaîne JSON.
     * @param json        Contenu JSON.
     * @param resolveBase Pour une variante, ce qui charge sa base ; sans lui, une variante est
     *                    refusée (`LevelValidationError::MissingBase`).
     * @return Résultat.
     */
    [[nodiscard]] static LevelLoadResult loadFromString(std::string_view json,
                                                        const BaseResolver& resolveBase = {});

    /**
     * @brief Charge un niveau depuis un fichier.
     *
     * La base d'une variante se cherche comme `<dossier>/<base>.json`, du dossier de la variante
     * vers la racine du disque : le premier qui existe l'emporte. Une carte
     * `central-empire/capital/x.json` qui déclare `"base": "martpart"` trouve donc
     * `Levels/central-empire/capital/martpart.json` dans son propre dossier, et `"base":
     * "central-empire/capital/martpart"` le trouve depuis la racine `Levels/`.
     * @param path Chemin du fichier.
     * @return Résultat.
     */
    [[nodiscard]] static LevelLoadResult loadFromFile(const std::filesystem::path& path);
};

}  // namespace core
