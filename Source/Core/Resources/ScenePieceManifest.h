// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Levels/PieceFootprint.h"

/**
 * @file Core/Resources/ScenePieceManifest.h
 * @brief Le **manifeste des pièces** d'un lieu : ce que l'atelier des textures (`LOT-92`) déclare
 *        de chaque pièce de sa planche — classe, emprise, ancre, taille, miroir.
 *
 * L'atelier écrit, à côté des images d'un lieu, un `Assets/Scene/<lieu>/manifest.json`. Jusqu'au
 * `LOT-EDITOR-02`, seule la galerie des assets le lisait, dans `HMI` (constat A9 de la feuille de
 * route de l'éditeur) : aucune règle de carte ne pouvait donc s'appuyer sur l'emprise d'une pièce.
 * La lecture descend ici, sans Qt ni GPU, pour que `Core` puisse un jour en déduire l'occupation et
 * la collision (`LOT-EDITOR-12`) ; `HMI` n'en garde que les images.
 *
 * Depuis le `LOT-EDITOR-12`, le manifeste dit aussi ce qu'une pièce **oppose** à qui passe (son
 * type tactique, `core::PieceTactical`) — c'est de là que `core::deriveCollision` tire la collision
 * d'une carte — et sous quels **anciens noms** une carte peut encore la citer (`aliases`) : une
 * planche réextraite qui renomme une pièce ne casse aucune carte.
 *
 * Le manifeste nomme une pièce par une clé d'atelier (`scene/martpart/wall-left`). Une carte, elle,
 * ne connaît que le **nom court** (`wall-left`), celui que la table d'apparence et l'assignation de
 * texture écrivent : le lecteur rend les deux.
 */

namespace core {

struct JsonDocument;

/// @brief Classe d'une pièce, telle que l'atelier la range.
enum class ScenePieceClass : std::uint8_t {
    /// Un losange de sol, posé à plat.
    Floor,
    /// Une pièce debout d'une case : mur, porte, torche.
    Tall,
    /// Une pièce debout de plusieurs cases : façade, étal, gradin.
    Wide,
    /// Une classe que ce lecteur ne connaît pas : gardée par son nom, jamais refusée.
    Other,
};

/**
 * @brief Ce qu'une pièce oppose à qui passe : son **type tactique** (décision D10, constat A5).
 *
 * Rangés du plus faible au plus fort : sur une case que plusieurs pièces couvrent, la plus forte
 * l'emporte (`core::deriveCollision`). Les noms du manifeste sont entre parenthèses.
 */
enum class PieceTactical : std::uint8_t {
    /// Passe (`open`) : un sol, un banc qu'on enjambe, une arche.
    Open,
    /// Gêne (`difficult`) : terrain difficile. **Pas encore joué** depuis une pièce : déduit comme
    /// `Open`, et signalé par `LevelEditor --check`.
    Difficult,
    /// Abri (`cover`) : un muret derrière lequel on se protège. **Pas encore joué** depuis une
    /// pièce, comme la gêne.
    Cover,
    /// Arrête le pas (`obstacle`) : infranchissable au sol, mais on voit par-dessus et on le
    /// survole — une fosse, un bassin. Déduit en `cliff`.
    Obstacle,
    /// Arrête la vue (`solid`) : un mur, une façade. Déduit en `wall`.
    Solid,
};

/// @return Le nom de manifeste de @p tactical (`open`, `difficult`, `cover`, `obstacle`, `solid`).
[[nodiscard]] const char* pieceTacticalName(PieceTactical tactical) noexcept;

/// @return Le type tactique nommé @p name, ou `std::nullopt` pour un nom inconnu.
[[nodiscard]] std::optional<PieceTactical> parsePieceTactical(std::string_view name) noexcept;

/// @brief Une pièce de la planche d'un lieu.
struct ScenePiece {
    /// Nom court (`wall-left`) : celui que les cartes écrivent.
    std::string name;
    /// Clé de l'atelier (`scene/martpart/wall-left`).
    std::string key;
    /// Fichier image, relatif au dossier du lieu (`wall-left.png`).
    std::string file;
    ScenePieceClass pieceClass = ScenePieceClass::Other;
    /// Classe telle qu'écrite (`floor`, `tall`, `wide`…), pour une classe inconnue.
    std::string className;
    /// Emprise en cases : colonnes, puis lignes. Au moins 1 × 1.
    int footprintColumns = 1;
    int footprintRows = 1;
    /// Taille de l'image, en pixels d'art ; 0 si le manifeste ne la donne pas.
    int width = 0;
    int height = 0;
    /// Ancre (sommet haut du losange de l'emprise), en pixels d'art ; -1 si non donnée.
    int anchorX = -1;
    int anchorY = -1;
    /// Nom court de la pièce dont celle-ci est le miroir, vide sinon. L'image miroir est livrée
    /// telle quelle par l'atelier : le rendu n'a rien à retourner.
    std::string mirrorOf;
    /// Type tactique (`tactical` du manifeste). À défaut, un sol passe et une pièce debout arrête
    /// la vue : c'est ce que valent les murs, façades et portes que les planches livrent.
    PieceTactical tactical = PieceTactical::Solid;
    /// Anciens noms courts sous lesquels une carte peut citer la pièce (`aliases`).
    std::vector<std::string> aliases;

    /// @return L'emprise de la pièce.
    [[nodiscard]] PieceFootprint footprint() const noexcept {
        return PieceFootprint{.columns = footprintColumns, .rows = footprintRows};
    }

    [[nodiscard]] bool operator==(const ScenePiece&) const = default;
};

/// @brief Catégorie d'échec de lecture.
enum class ScenePieceManifestError : std::uint8_t {
    None,
    FileNotFound,
    ParseError,
    UnsupportedVersion,
    MalformedStructure,
};

struct ScenePieceManifestResult;

/**
 * @brief Les pièces d'un lieu, dans l'ordre du manifeste.
 *
 * Logique pure. Aucune lecture ne lève (`EX-NFR-040`) ; une entrée mal formée (sans `file`) est
 * ignorée plutôt que de faire perdre les autres, comme la galerie l'a toujours fait.
 */
class ScenePieceManifest {
public:
    static constexpr int FORMAT_VERSION = 1;

    [[nodiscard]] static ScenePieceManifestResult loadFromString(std::string_view json);
    [[nodiscard]] static ScenePieceManifestResult loadFromFile(const std::filesystem::path& path);

    /// @return Le lieu (`disposition` du manifeste : `martpart`), vide s'il n'est pas donné.
    [[nodiscard]] const std::string& place() const noexcept {
        return _place;
    }

    /// @return Largeur, en pixels d'art, du losange de sol du lieu (`tile`, `EX-VIS-008`) : 0 si le
    ///         manifeste ne la déclare pas.
    [[nodiscard]] int tileWidth() const noexcept {
        return _tileWidth;
    }

    /// @return Hauteur de ce losange, en pixels d'art ; 0 si non déclarée.
    [[nodiscard]] int tileHeight() const noexcept {
        return _tileHeight;
    }

    /// @return Les pièces, dans l'ordre où le manifeste les écrit.
    [[nodiscard]] const std::vector<ScenePiece>& pieces() const noexcept {
        return _pieces;
    }

    /// @return La pièce de nom court @p name, ou dont @p name est un ancien nom (`aliases`) ;
    ///         `nullptr` si le lieu ne la déclare pas.
    [[nodiscard]] const ScenePiece* find(std::string_view name) const noexcept;

private:
    [[nodiscard]] static ScenePieceManifestResult fromDocument(const JsonDocument& document);

    std::string _place;
    int _tileWidth = 0;
    int _tileHeight = 0;
    std::vector<ScenePiece> _pieces;
};

/// @brief Résultat d'une lecture : le manifeste, et ce qui a échoué.
struct ScenePieceManifestResult {
    ScenePieceManifest manifest;
    ScenePieceManifestError error = ScenePieceManifestError::None;
    /// Message technique, vide en cas de succès.
    std::string message;

    [[nodiscard]] bool ok() const noexcept {
        return error == ScenePieceManifestError::None;
    }
};

/// @return La classe nommée @p name (`floor`, `tall`, `wide`), `Other` pour toute autre.
[[nodiscard]] ScenePieceClass parseScenePieceClass(std::string_view name) noexcept;

/// @return Le nom court d'une clé d'atelier : ce qui suit la dernière barre.
[[nodiscard]] std::string_view scenePieceShortName(std::string_view key) noexcept;

}  // namespace core
