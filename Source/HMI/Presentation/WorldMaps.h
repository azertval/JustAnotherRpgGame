// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Core/World/Atlas.h"

/**
 * @file HMI/Presentation/WorldMaps.h
 * @brief Les cartes de l'écran « Carte » — monde, régions, villes — jointes à l'atlas (`LOT-94`,
 * `LOT-95`).
 */

namespace hmi {

/// Position sur une carte, en fraction de sa largeur et de sa hauteur (0 à 1).
struct MapPoint {
    double x = 0.0;
    double y = 0.0;
};

/// Cadre d'une carte sur la carte du niveau supérieur, en fractions : d'où part le zoom.
struct MapFrame {
    double x = 0.0;
    double y = 0.0;
    double width = 1.0;
    double height = 1.0;
};

/// Nom de géographie posé sur une carte, sans fiche : un lac, une chaîne, une ville hors atlas.
struct MapLabel {
    std::string name;
    std::string kind;  ///< `city`, `town`, `fortress`, `site`, `lake`, `forest`, `mountain`…
    MapPoint at;
};

/// Lieu numéroté d'un plan de ville qui n'est pas un lieu de l'atlas (légende du livre).
struct MapSite {
    int number = 0;
    std::string name;
    std::string note;
    MapPoint at;
};

/// La carte d'une région, telle que `world-maps.json` la décrit.
struct RegionMap {
    std::string image;
    MapPoint anchor;  ///< Repère de la région sur la carte du monde.
    MapFrame frame;   ///< Cadre approximatif de la région sur la carte du monde.
    std::map<std::string, MapPoint> places;  ///< Lieu de l'atlas → position sur la carte.
    std::vector<std::string> omitted;  ///< Entrées de l'atlas qui ne sont pas des lieux (règles…).
    std::vector<MapLabel> labels;
};

/// Le plan d'une ville : ses quartiers (lieux de l'atlas) ou ses lieux numérotés.
/**
 * @brief La grille d'une carte **rendue** sur son image (`LOT-121`) : le point de grille (c, r)
 *        — la case c couvre [c, c + 1] — est en `origin + c × column + r × row`, en fractions de
 *        l'image. `LevelEditor --render --canvas` l'écrit avec l'image.
 */
struct MapGrid {
    MapPoint origin;
    MapPoint column;
    MapPoint row;

    /// @return Le point de grille (@p gridColumn, @p gridRow) sur l'image.
    [[nodiscard]] MapPoint at(double gridColumn, double gridRow) const noexcept {
        return MapPoint{.x = origin.x + (gridColumn * column.x) + (gridRow * row.x),
                        .y = origin.y + (gridColumn * column.y) + (gridRow * row.y)};
    }
};

/**
 * @brief Une **sous-zone** d'un quartier (`LOT-121`, décision D-16) : l'Arena of Fate dans
 *        Arenarea. Sa carte est celle du quartier suivie de son identifiant
 *        (`central-empire/capital/arenarea/arena-of-fate`) : l'arborescence la dit déjà.
 */
struct MapZone {
    std::string name;
    /// Son entrée sur la carte du quartier, en cases (colonne, ligne) : là que son repère se pose.
    MapPoint entrance;
    std::string image;            ///< Relatif à `Assets/` (voir `MapDistrict::image`).
    std::optional<MapGrid> grid;  ///< Présente dès qu'il y a une image.
};

/**
 * @brief La vue d'un **quartier** sur le plan de sa ville (`LOT-96`) : le plan agrandi sur `frame`
 *        tant qu'il n'a pas de carte ; sa carte sinon — peinte, ou rendue avec sa grille (`LOT-121`).
 */
struct MapDistrict {
    MapFrame frame;
    /// Vide tant que le quartier n'a pas sa carte. Un nom seul est une carte peinte de
    /// `Assets/Maps/` ; un chemin (`Regions/…/Map/martpart.jpg`) est relatif à `Assets/` : la
    /// carte rendue d'une zone, rangée avec elle (`LOT-121`).
    std::string image;
    std::optional<MapGrid> grid;  ///< La grille de l'image rendue ; absente d'une carte peinte.
    std::map<std::string, MapZone> zones;  ///< Identifiant (dossier) → sous-zone.
};

struct CityMap {
    std::string image;
    std::map<std::string, MapPoint> places;
    std::vector<MapSite> sites;
    std::vector<MapLabel> labels;
    std::map<std::string, MapDistrict> districts;  ///< Quartier (lieu de l'atlas) → sa vue.
};

/// Le contenu de `world-maps.json`, ou la raison de l'échec.
struct WorldMaps {
    std::string worldImage;
    std::map<std::string, RegionMap> regions;  ///< Identifiant de région → carte.
    std::map<std::string, CityMap> cities;     ///< Identifiant de lieu → plan.
    std::string error;                         ///< Vide si la lecture a abouti.

    [[nodiscard]] bool ok() const {
        return error.empty();
    }
};

/**
 * @brief Lit `world-maps.json`.
 *
 * **Pourquoi un fichier à part de l'atlas.** L'atlas du `LOT-37` est extrait du livre, qui ne donne
 * aucune coordonnée, et la chaîne d'extraction réécrit ses fichiers : elle effacerait un champ
 * qu'elle n'a pas produit. Les positions sont **relevées sur les cartes de l'auteur** et vivent à
 * côté.
 *
 * Une position hors de [0, 1], un cadre qui sort de la carte ou un champ mal formé font échouer
 * toute la lecture, en nommant l'entrée fautive.
 */
[[nodiscard]] WorldMaps readWorldMaps(std::string_view json);

/// Un lieu d'une région, tel que la vue « région » le montre.
struct MapPlaceView {
    std::string id;
    std::string name;
    std::string description;
    bool placed = false;  ///< Faux : le lieu figure dans la liste, sans repère.
    MapPoint at;
    bool hasCityMap = false;  ///< Vrai : on peut encore zoomer, jusqu'au plan de la ville.
};

/// Une région, telle que les vues « monde » et « région » la montrent.
struct MapRegionView {
    std::string id;
    std::string name;
    std::string image;
    MapPoint anchor;
    MapFrame frame;
    std::string government;
    std::string faction;
    std::optional<int> population;
    /// Les sept statistiques régionales, dans l'ordre de `core::RegionAxis`, de 0 (très bas) à 4.
    std::array<int, core::kRegionAxisCount> grades{};
    std::vector<MapPlaceView> places;  ///< Posés d'abord, puis les autres ; ordre de l'atlas.
    std::vector<MapLabel> labels;
};

/// Un point d'un plan de ville : un quartier de l'atlas ou un lieu numéroté.
struct MapCityPointView {
    std::string id;
    int number = 0;
    std::string name;
    std::string description;
    MapPoint at;
    /// La vue du quartier sur le plan, s'il en a une (`LOT-96`).
    std::optional<MapDistrict> district;
};

/// Un plan de ville, tel que la vue « ville » le montre.
struct MapCityView {
    std::string id;  ///< Le lieu de l'atlas que ce plan détaille.
    std::string name;
    std::string regionId;
    std::string image;
    std::vector<MapCityPointView> points;
    std::vector<MapLabel> labels;
};

/// Les trois niveaux de l'écran « Carte ».
struct WorldMapViews {
    std::string worldImage;
    std::vector<MapRegionView> regions;  ///< Dans l'ordre de l'atlas.
    std::vector<MapCityView> cities;
};

/**
 * @brief Joint les cartes à l'atlas.
 *
 * @param atlas L'atlas du `LOT-37` : ses régions donnent l'ordre, ses lieux les noms et les textes.
 * @param maps Les cartes lues par `readWorldMaps`.
 * @param mismatches Reçoit, en clair, chaque écart entre les deux : région sans carte, carte sans
 *        région, position d'un lieu que sa région ne connaît pas, plan d'une ville que l'atlas
 *        ignore. Une région qui manque en silence sur la carte ne se corrige jamais ; un écart
 *        nommé, si.
 */
[[nodiscard]] WorldMapViews joinWorldMaps(const core::Atlas& atlas, const WorldMaps& maps,
                                          std::vector<std::string>& mismatches);

}  // namespace hmi
