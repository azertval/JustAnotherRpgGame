// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file Core/World/CityPlan.h
 * @brief Le graphe d'une ville qu'on parcourt, quartier par quartier (`LOT-96`).
 *
 * Une ville jouable (`Source/Elements/World/cities/<ville>.json`) nomme ses quartiers — chacun une
 * fiche d'atlas du `LOT-37` —, et pour chacun **soit** la carte où l'on marche, **soit** la carte
 * voisine où se tient la porte gardée qui le ferme. Elle nomme aussi la porte où « Nouvelle
 * partie » pose le héros.
 *
 * Le fichier ne redécrit rien : ni le quartier (sa fiche d'atlas), ni sa position (le plan,
 * `world-maps.json`), ni ses rues (sa carte). Il les **relie**, et c'est ce que les contrôles de
 * données (`scripts/checks/check_rpg_data.py`) vérifient.
 */

namespace core {

/// @brief Un quartier d'une ville jouable.
struct CityDistrict {
    /// La fiche d'atlas du quartier (`central-empire-the-capital-city-martpart`).
    std::string id;
    /// L'identifiant de la carte du quartier (`capital/martpart`), vide s'il n'en a pas.
    std::string map;
    /// La carte où se tient la porte gardée d'un quartier sans carte, vide s'il en a une.
    std::string guardMap;

    /// @return Vrai si l'on marche dans ce quartier.
    [[nodiscard]] bool hasMap() const noexcept {
        return !map.empty();
    }
};

/// @brief Le graphe d'une ville : ses quartiers et sa porte de départ.
struct CityPlan {
    std::string id;
    std::string name;
    /// La fiche d'atlas de la ville, qui est aussi la clé de son plan dans `world-maps.json`.
    std::string location;
    /// Le quartier où « Nouvelle partie » pose le héros, et le point d'arrivée de sa carte.
    std::string startDistrict;
    std::string startArrival;
    std::vector<CityDistrict> districts;

    /// @return Le quartier @p districtId, `nullptr` s'il n'est pas de cette ville.
    [[nodiscard]] const CityDistrict* find(std::string_view districtId) const;

    /**
     * @return Le quartier dont @p mapId est la carte, ou une carte **de ses sous-zones** — rangée
     *         sous la sienne (`…/arenarea/arena-of-fate`, décision D-16, `LOT-121`) ; `nullptr`
     *         si aucun ne l'a.
     */
    [[nodiscard]] const CityDistrict* districtOfMap(std::string_view mapId) const;

    /// @return La carte du quartier de départ, vide si ce quartier n'en a pas.
    [[nodiscard]] std::string startMap() const;
};

/// @brief Résultat d'une lecture : le graphe, ou ce qui l'a empêché.
struct CityPlanResult {
    CityPlan plan;
    /// Message technique, vide en cas de succès. Pour les journaux et les tests.
    std::string error;

    /// @brief Vrai si la lecture a réussi (aucun message d'erreur).
    [[nodiscard]] bool ok() const noexcept {
        return error.empty();
    }
};

/**
 * @brief Lit une ville jouable.
 *
 * Aucune lecture ne lève (`EX-NFR-040`). Sont refusés : un fichier absent ou mal formé, une ville
 * sans quartier, un quartier qui n'a ni carte ni porte gardée — ou les deux —, et un départ dont
 * le quartier n'a pas de carte : « Nouvelle partie » n'aurait nulle part où poser le héros.
 */
[[nodiscard]] CityPlanResult loadCityPlan(const std::filesystem::path& file);

}  // namespace core
