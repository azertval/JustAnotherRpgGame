// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "HMI/Presentation/WorldMaps.h"

/**
 * @file Editor/Logic/CityView.h
 * @brief La **vue de ville** du navigateur de cartes (`LOT-EDITOR-09`, `EX-EDIT-090`) : les
 *        quartiers d'une ville jouable posés sur son plan, et la carte que chacun ouvre. Logique
 *        pure, sans Qt.
 *
 * Rien n'est redit ici : la ville vient de `Source/Elements/World/cities/<ville>.json`
 * (`core::CityPlan`), les cadres de ses quartiers de `Maps/world-maps.json`
 * (`hmi::readWorldMaps`), leurs noms de l'atlas (`core::loadAtlas`). La vue les **joint**, dit
 * lesquels ont une carte, et laquelle manque — c'est le tableau de bord d'une ville en cours de
 * construction.
 *
 * L'éditeur ne récrit ni la ville ni `world-maps.json` : la vue se regarde, et ouvre un quartier
 * d'un clic (décision de l'auteur, 21 septembre 2026).
 */

namespace hmi {

/// @brief Un quartier, tel que la vue de ville le montre.
struct CityDistrictView {
    /// La fiche d'atlas du quartier (`central-empire-the-capital-city-martpart`).
    std::string id;
    /// Son nom, tel que l'atlas le donne ; à défaut, son identifiant.
    std::string name;
    /// Son cadre sur le plan de la ville, en fractions ; vide si `world-maps.json` n'en a pas.
    MapFrame frame;
    /// Vrai si le plan pose un cadre pour ce quartier.
    bool framed = false;
    /// La carte où l'on marche, vide pour un quartier fermé par une porte gardée.
    std::string mapId;
    /// La carte où se tient cette porte gardée, vide si le quartier a la sienne.
    std::string guardMapId;
    /// Vrai si le fichier de `mapId` existe : un quartier promis dont la carte reste à faire se
    /// voit comme tel.
    bool mapExists = false;
};

/// @brief Une ville jouable, prête à peindre.
struct CityView {
    /// L'identifiant de la ville (`capital`).
    std::string id;
    /// Son nom (`La Capitale`).
    std::string name;
    /// La fiche d'atlas de la ville, clé de son plan dans `world-maps.json`.
    std::string location;
    /// L'image du plan, telle que `world-maps.json` la nomme
    /// (`city-central-empire-the-capital-city.jpg`, dans `Assets/Maps/`).
    std::string image;
    /// Les quartiers, dans l'ordre de la ville ; ceux qui ont une carte d'abord.
    std::vector<CityDistrictView> districts;
    /// Ce qui a empêché la lecture, vide si la vue est complète.
    std::string error;

    [[nodiscard]] bool ok() const noexcept {
        return error.empty();
    }
};

/// @return Les régions du monde que `world-maps.json` nomme, triées : ce que le dialogue des
///         propriétés de carte propose (`LOT-EDITOR-09`).
[[nodiscard]] std::vector<std::string> worldRegionIds(const std::filesystem::path& dataRoot);

/// @return Les villes jouables de `<dataRoot>/World/cities`, par identifiant, triées.
[[nodiscard]] std::vector<std::string> cityIds(const std::filesystem::path& dataRoot);

/**
 * @brief Joint la ville @p cityId à son plan et à l'atlas.
 * @param dataRoot Racine des données (`Source/Elements`).
 * @param cityId   L'identifiant de la ville (`capital`).
 *
 * Une ville illisible, ou dont le plan manque, rend une vue en erreur ; un quartier sans cadre
 * reste dans la liste, sans cadre (`framed` faux) : il se voit manquant au lieu de disparaître.
 */
[[nodiscard]] CityView buildCityView(const std::filesystem::path& dataRoot,
                                     std::string_view cityId);

/// @return La ville dont @p mapId est la carte d'un quartier, vide si aucune ne la porte.
[[nodiscard]] std::string cityOfMap(const std::filesystem::path& dataRoot, std::string_view mapId);

/**
 * @brief Le quartier sous @p point, en fractions du plan.
 * @return Le rang du **plus petit** cadre qui contient le point (un quartier posé sur un autre se
 *         prend quand même) ; `std::nullopt` si aucun ne le contient.
 */
[[nodiscard]] std::optional<std::size_t> districtAt(const CityView& city, MapPoint point);

}  // namespace hmi
