// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Combat/TurnOrder.h"
#include "Core/Levels/GridPosition.h"

namespace core {
struct JsonDocument;
}

/**
 * @file HMI/Graphics/ArenaAppearanceCatalog.h
 * @brief Rôle de case et figurine du Colisée, portés en `HMI` pur depuis `ArenaTile.ui.qml`
 *        (LOT-50, lignes 57-84) pour que `hmi::composeArenaScene` les lise sans repasser par QML.
 */

namespace hmi {

/**
 * @brief Ce que porte l'enceinte sur une case de mur.
 *
 * Un pan plein partout, sauf trois exceptions écrites par `ArenaTile.ui.qml` : une colonne aux
 * quatre angles, une bannière tous les cinq pas sur les bords haut et bas, une torche tous les
 * quatre pas sur les bords gauche et droit.
 */
enum class WallFeature : std::uint8_t {
    /// La case n'est pas un mur (`ArenaTileAppearance::wall` faux) — valeur de repli.
    None,
    Plain,
    Corner,
    BannerSpot,
    TorchSpot,
};

/**
 * @brief Ce qu'une case porte, dérivé de sa seule position dans la grille de combat.
 *
 * `wall` est une donnée d'entrée (`BattleGrid::isObstructed`, au sol) : cette table n'en décide
 * pas, elle en déduit le détail visuel. `gateSpot` et `slab` sont indépendants de `wallFeature` —
 * une case franchissable du bord peut aussi bien porter une dalle claire qu'une case de sable.
 */
struct ArenaTileAppearance {
    bool wall = false;
    WallFeature wallFeature = WallFeature::None;
    /// Vrai sur une case franchissable du bord de la grille : l'arche de porte s'y dessine.
    bool gateSpot = false;
    /// Vrai si le sol porte une dalle claire du tileset plutôt que le sable.
    bool slab = false;
    /// Indice dans `ArenaAppearanceCatalog::paleSlabs()` — valide seulement si `slab` est vrai.
    int slabVariant = 0;
};

/// @brief La figurine d'un combattant : le dossier de la planche, et son nombre d'images.
struct FigureAppearance {
    /// Nom de la figurine (`heroes`/`gladiators` du manifeste), vide si le catalogue n'a aucune
    /// figurine pour ce côté. C'est la clé de ses clips (`ArenaAnimationDriver`).
    std::string sheet;
    /// Dossier de ses bandes, relatif au dossier du Colisée : `characters/<sheet>` ou
    /// `enemies/<sheet>` par défaut, ou le dossier d'un PNJ de remplacement (`../Npc/<slug>`,
    /// voir `ArenaAppearanceCatalog::applyNpcManifest`). Vide si `sheet` l'est.
    std::string directory;
    /// Nombre d'images des bandes de la planche du Colisée (`heroFrames`/`enemyFrames`). La
    /// composition lui préfère la largeur réelle de la bande quand elle la connaît.
    int frameCount = 0;
};

/// @brief Catégorie d'échec de lecture (même esprit que `hmi::AnimationCatalogError`).
enum class ArenaAppearanceError : std::uint8_t {
    None,
    FileNotFound,
    ParseError,
    UnsupportedVersion,
    MalformedStructure,
};

// Le resultat contient un ArenaAppearanceCatalog par valeur, et le catalogue le renvoie de ses
// fonctions de lecture : il est donc annonce ici et defini apres la classe.
struct ArenaAppearanceCatalogResult;

/**
 * @brief Catalogue d'apparence du Colisée : figurines par côté, dalles de sol, chargé depuis
 *        `Source/Elements/Assets/Coliseum/manifest.json` (`heroes`, `gladiators`, `heroFrames`,
 *        `enemyFrames`, `paleSlabs`).
 *
 * Logique **pure** (aucune dépendance Qt ni GPU, `EX-NFR-010`), testable sur des grilles connues
 * sans construire de `BattleGrid`. Aucune lecture ne lève d'exception (`EX-NFR-040`).
 */
class ArenaAppearanceCatalog {
public:
    static constexpr int FORMAT_VERSION = 1;

    [[nodiscard]] static ArenaAppearanceCatalogResult loadFromString(std::string_view json);
    [[nodiscard]] static ArenaAppearanceCatalogResult loadFromFile(
        const std::filesystem::path& path);

    /**
     * @brief Le rôle visuel d'une case, dans une grille de @p columns × @p rows.
     * @param cell    Case à qualifier.
     * @param columns Nombre de colonnes de la grille.
     * @param rows    Nombre de lignes de la grille.
     * @param wall    Vrai si la case obstrue le sol (`BattleGrid::isObstructed`,
     * `Locomotion::Walk`) — cette table le reçoit, elle ne le recalcule pas.
     */
    [[nodiscard]] ArenaTileAppearance tileAppearance(core::GridPosition cell, int columns, int rows,
                                                     bool wall) const;

    /**
     * @brief La figurine d'un combattant, choisie d'après son nom pour rester la même d'un tour à
     *        l'autre — sans état à tenir à jour.
     * @param name Nom du combattant (`core::CombatantProfile::name`).
     * @param side Son côté : `characters/` pour un allié, `enemies/` pour un ennemi.
     */
    [[nodiscard]] FigureAppearance figureFor(std::string_view name, core::CombatSide side) const;

    /**
     * @brief Le dossier des bandes d'une figurine, relatif au dossier du Colisée.
     * @param sheet Nom de la figurine (`heroes()` ou `gladiators()`).
     * @param side  Son côté : `characters/` ou `enemies/`, sauf remplacement (`replaceHero`).
     */
    [[nodiscard]] std::string sheetDirectory(std::string_view sheet, core::CombatSide side) const;

    /**
     * @brief Fait lire les bandes d'un héros dans un autre dossier — la figurine de remplacement
     *        d'un PNJ de l'atelier (LOT-91) à la place d'un héros de la planche de production.
     *
     * Le nom du héros ne change pas : le roster, la sélection par nom et la clé des clips restent
     * ceux du manifeste du Colisée ; seul le dossier lu diffère.
     * @param hero      Un nom de `heroes()`.
     * @param directory Dossier des bandes, relatif au dossier du Colisée (ex. `../Npc/anariel`).
     * @return Faux, sans effet, si @p hero n'est pas dans le roster.
     */
    bool replaceHero(std::string_view hero, std::string directory);

    /**
     * @brief Applique les remplacements déclarés par le manifeste des PNJ
     *        (`Source/Elements/Assets/Npc/manifest.json`, champ `replaces` : héros → slug).
     *
     * Fichier absent : rien, silencieusement — les PNJ sont optionnels. Fichier illisible ou
     * héros inconnu : avertissement dans le journal, l'entrée est ignorée (`EX-NFR-040`).
     * @param path Chemin du manifeste des PNJ.
     * @return Le nombre de héros remplacés.
     */
    int applyNpcManifest(const std::filesystem::path& path);

    /**
     * @brief Le lieu dont l'arène emprunte ses pièces de scène (champ `scene` du manifeste).
     *
     * La composition lit les sols et l'enceinte dans `../Scene/<lieu>/`, à côté du dossier du kit.
     * Le kit dit donc lui-même de quelle planche il se sert : rien n'est écrit en dur dans le
     * moteur (LOT-102).
     */
    [[nodiscard]] const std::string& scene() const noexcept {
        return _scene;
    }
    [[nodiscard]] const std::vector<std::string>& heroes() const noexcept {
        return _heroes;
    }
    [[nodiscard]] const std::vector<std::string>& gladiators() const noexcept {
        return _gladiators;
    }
    [[nodiscard]] const std::vector<std::string>& paleSlabs() const noexcept {
        return _paleSlabs;
    }
    [[nodiscard]] int heroFrames() const noexcept {
        return _heroFrames;
    }
    [[nodiscard]] int enemyFrames() const noexcept {
        return _enemyFrames;
    }

private:
    [[nodiscard]] static ArenaAppearanceCatalogResult fromDocument(
        const core::JsonDocument& document);

    std::string _scene;
    std::vector<std::string> _heroes;
    std::vector<std::string> _gladiators;
    std::vector<std::string> _paleSlabs;
    /// Héros → dossier de remplacement (`replaceHero`) ; absent : `characters/<héros>`.
    std::map<std::string, std::string, std::less<>> _heroDirectories;
    int _heroFrames = 0;
    int _enemyFrames = 0;
};

/// @brief Résultat d'une lecture de catalogue : soit un `ArenaAppearanceCatalog`, soit une erreur.
struct ArenaAppearanceCatalogResult {
    std::optional<ArenaAppearanceCatalog> catalog;
    std::string error;
    ArenaAppearanceError errorCode = ArenaAppearanceError::None;

    [[nodiscard]] bool ok() const noexcept {
        return catalog.has_value();
    }
};

}  // namespace hmi
