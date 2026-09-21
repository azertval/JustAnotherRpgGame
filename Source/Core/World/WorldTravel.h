// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/Levels/GridPosition.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/WorldGraph.h"

/**
 * @file Core/World/WorldTravel.h
 * @brief Le graphe du monde **joué** : franchir un portail, arriver à un point nommé, et refuser
 *        au chargement ce qui ne mène nulle part (`LOT-09`).
 *
 * `core::WorldGraph` lit le dossier des niveaux et dit ce que vaut chaque portail ; ce fichier
 * s'en sert pour **jouer** : il charge la carte courante, la garde, et déplace le personnage d'une
 * carte à l'autre par des points d'arrivée **nommés** — jamais par des coordonnées, qui se
 * désynchroniseraient au premier redimensionnement de la carte cible.
 *
 * Deux règles tiennent tout le fichier :
 *
 * - **Une carte chargée n'est pas rechargée.** Le coffre pris et la porte ouverte vivent hors des
 *   entités (`core::WorldFlags`), mais la carte elle-même est conservée d'un passage à l'autre :
 *   revenir du sable doit rendre le lieu tel qu'on l'a laissé, pas un lieu neuf.
 * - **Un défaut est un code, pas un texte** (`EX-NFR-011`) : `Core` n'écrit aucun message, et
 *   l'IHM traduit `WorldIssueCode`. Un portail orphelin est relevé au **chargement**, pas à la
 *   traversée (`EX-NFR-040`).
 */

namespace core {

class WorldFlags;

/// @brief Ce qu'un portail nomme : sa carte cible, son point d'arrivée, son drapeau éventuel.
struct PortalTarget {
    std::string map;
    std::string arrival;
    /// Drapeau exigé, vide si le portail s'ouvre toujours.
    std::string requiredFlag;

    [[nodiscard]] bool operator==(const PortalTarget&) const = default;
};

/// @return Le portail posé sur @p position, s'il y en a un.
[[nodiscard]] std::optional<PortalTarget> portalAt(const Level& level, GridPosition position);

/// @return La case du point d'arrivée nommé @p name, s'il existe. Deux points de même nom sont un
///         défaut du graphe : le **premier** dans l'ordre des entités l'emporte ici.
[[nodiscard]] std::optional<GridPosition> arrivalPointAt(const Level& level, std::string_view name);

/// @brief Ce qui empêche une carte d'être jouée. `Core` n'écrit pas de texte : l'IHM traduit.
enum class WorldIssueCode {
    /// La carte n'a pas pu être lue. `value` porte le message technique du chargeur.
    UnreadableMap,
    /// Un portail ne nomme aucune carte cible.
    MissingTargetMap,
    /// La carte cible d'un portail n'est pas dans le dossier. `value` la nomme.
    UnknownTargetMap,
    /// La carte cible existe mais n'a pas pu être lue. `value` la nomme.
    UnreadableTargetMap,
    /// Un portail ne nomme aucun point d'arrivée.
    MissingArrivalPoint,
    /// La carte cible n'offre pas ce point d'arrivée. `value` le nomme.
    UnknownArrivalPoint,
    /// Deux points d'arrivée d'une même carte portent le même nom. `value` le nomme.
    DuplicateArrivalPoint,
    /// Une zone de combat est dégénérée : largeur ou hauteur nulle. `value` la nomme.
    CombatZoneDegenerate,
    /// Une zone de combat déborde de la carte. `value` la nomme.
    CombatZoneOutOfBounds,
    /// Une zone de combat n'a aucune case libre : on ne s'y bat pas. `value` la nomme.
    CombatZoneBlocked,
};

/// @brief Un défaut relevé sur une carte, et où.
struct WorldIssue {
    /// Carte qui porte le défaut.
    std::string mapId;
    /// Case du portail ou du point d'arrivée fautif ; `{0, 0}` pour un défaut de carte entière.
    GridPosition position;
    WorldIssueCode code = WorldIssueCode::UnreadableMap;
    /// Ce qu'il faut citer pour que le message soit exploitable : nom de carte, de point
    /// d'arrivée, ou message du chargeur.
    std::string value;

    [[nodiscard]] bool operator==(const WorldIssue&) const = default;
};

/**
 * @brief Relève tout ce qui, dans @p graph, ne mène nulle part.
 *
 * C'est la validation **au chargement** qu'exige `EX-NFR-040` : un portail orphelin est une erreur
 * explicite au démarrage, et non un plantage le jour où le joueur marche dessus.
 *
 * @return Les défauts, cartes par identifiant puis portails dans l'ordre des entités.
 */
[[nodiscard]] std::vector<WorldIssue> validateWorldGraph(const WorldGraph& graph);

/**
 * @brief Relève ce qu'une carte **lue** porte de fautif et que le graphe ne peut pas voir.
 *
 * Le graphe déduplique les points d'arrivée d'une carte : deux entités du même nom y entrent comme
 * un seul point, et le défaut s'y perd. Il se voit ici, sur la carte elle-même — et il compte,
 * parce que `arrivalPointAt` prend le premier, c'est-à-dire un des deux au hasard du tracé.
 */
[[nodiscard]] std::vector<WorldIssue> validateWorldMap(std::string_view mapId, const Level& level);

/// @brief Ce qu'une traversée a donné.
enum class TravelResult {
    /// Le personnage est sur la carte demandée, à son point d'arrivée.
    Moved,
    /// Aucun portail sur cette case : le pas est ordinaire, il n'y a rien à faire.
    NoPortal,
    /// Le portail exige un drapeau que la partie n'a pas encore.
    Locked,
    /// La carte cible n'existe pas, ou n'a pas pu être lue.
    UnreadableMap,
    /// La carte cible n'offre pas le point d'arrivée nommé.
    UnknownArrival,
};

/**
 * @brief La carte courante, et le passage d'une carte à l'autre.
 *
 * Le chargement est injecté (`MapLoader`) plutôt que codé sur `std::filesystem` : les tests
 * décrivent leurs cinq cartes en mémoire, et le jeu passe `directoryLoader`. `Core` ne connaît
 * ainsi toujours qu'un dossier qu'on lui nomme, jamais l'arborescence du dépôt.
 */
class WorldTravel {
public:
    /// @brief Rend la carte d'identifiant donné, lue ou en échec.
    using MapLoader = std::function<LevelLoadResult(std::string_view mapId)>;

    /// @brief Le chargeur du jeu : `<levelsDir>/<mapId>.json`.
    [[nodiscard]] static MapLoader directoryLoader(std::filesystem::path levelsDir);

    /**
     * @brief Le chargeur qui cherche une carte dans @p levelsDirs, **dans l'ordre donné**.
     *
     * L'essai complet de l'éditeur (`LOT-EDITOR-10`) écrit les brouillons ouverts dans un dossier
     * temporaire et le place devant les cartes du binaire : on joue ce qu'on a sous les yeux,
     * et toute carte qu'on n'édite pas vient de son fichier, comme en jeu.
     *
     * Une carte absente d'un dossier est cherchée dans le suivant ; une carte **présente mais
     * illisible** arrête la recherche et rend son erreur — passer au dossier suivant ferait jouer
     * en silence une version périmée de la carte qu'on vient de casser.
     */
    [[nodiscard]] static MapLoader directoriesLoader(std::vector<std::filesystem::path> levelsDirs);

    explicit WorldTravel(MapLoader loader);

    /**
     * @brief Entre sur @p mapId au point d'arrivée @p arrival.
     *
     * Une carte déjà visitée n'est **pas rechargée** : on retrouve celle qu'on a quittée. Un nom
     * d'arrivée vide pose le personnage sur l'entrée de la carte (`core::Level::entry`), ce que
     * fait « Nouvelle partie ».
     */
    TravelResult enter(std::string_view mapId, std::string_view arrival);

    /**
     * @brief Franchit le portail posé sur @p from, si les drapeaux l'ouvrent.
     *
     * @return `NoPortal` si la case n'en porte pas — c'est le cas ordinaire d'un pas —, `Locked`
     *         si le drapeau exigé manque, sinon le résultat de l'entrée sur la carte cible.
     */
    TravelResult cross(GridPosition from, const WorldFlags& flags);

    /// @return L'identifiant de la carte courante, vide avant la première entrée.
    [[nodiscard]] const std::string& currentMapId() const noexcept {
        return _currentMapId;
    }

    /// @return La carte courante, ou `nullptr` avant la première entrée réussie.
    [[nodiscard]] const Level* currentMap() const;

    /// @return La case du personnage sur la carte courante.
    [[nodiscard]] GridPosition position() const noexcept {
        return _position;
    }

    /// @brief Suit le personnage sur sa carte : c'est le déplacement qui décide, pas ce fichier.
    void setPosition(GridPosition position) noexcept {
        _position = position;
    }

    /// @return Le défaut de la dernière traversée refusée, s'il y en a un.
    [[nodiscard]] const std::optional<WorldIssue>& lastIssue() const noexcept {
        return _lastIssue;
    }

    /// @return Le nombre de cartes gardées en mémoire — ce qui prouve, en test, qu'un retour ne
    ///         recharge rien.
    [[nodiscard]] std::size_t loadedMapCount() const noexcept {
        return _maps.size();
    }

private:
    /// @return La carte @p mapId, chargée à la demande, ou `nullptr` si elle est illisible.
    const Level* mapFor(std::string_view mapId);

    MapLoader _loader;
    std::map<std::string, std::shared_ptr<Level>, std::less<>> _maps;
    std::string _currentMapId;
    GridPosition _position{};
    std::optional<WorldIssue> _lastIssue;
};

}  // namespace core
