// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_world_links.cpp
 * @brief Tests des **liens du graphe du monde** (`LOT-EDITOR-09`, `EX-EDIT-089`) : tirer un lien
 *        entre deux cartes pose la paire portail / point d'arrivée des deux côtés, et le contrôle
 *        reste vert.
 */

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/MapEntity.h"
#include "Core/World/EntityKinds.h"
#include "Core/World/ExplorationReach.h"
#include "Core/World/WorldGraph.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/WorldLinks.h"

namespace {

// La racine d'essai de l'éditeur (`LOT-123`) : ce test reliait deux cartes LIVRÉES, que la
// table rase du `LOT-102` emporte. La Place et le Donjon de la racine d'essai ne sont reliés
// par aucun portail : c'est ce qu'il faut pour éprouver `--link-maps`.
[[nodiscard]] std::filesystem::path elements() {
    return std::filesystem::path(JADG_TEST_DATA_DIR);
}

// Ce que le contrôle lit sans le récrire.
constexpr const char* DOSSIERS_LUS[] = {
    "World/dialogues", "World/locations", "Rpg/encounters", "Rpg/creatures", "Rpg/items",
    "Assets/Npc",      "Assets/Monsters", "Localization",   "World/cities",  "Assets/Scene"};

void copier(const std::filesystem::path& racine, const char* dossier) {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(elements() / dossier)) {
        const std::string extension = entry.path().extension().string();
        if (!entry.is_regular_file() || extension == ".png" || extension == ".webp") {
            continue;
        }
        const std::filesystem::path copie =
            racine / std::filesystem::relative(entry.path(), elements());
        std::filesystem::create_directories(copie.parent_path());
        std::filesystem::copy_file(entry.path(), copie);
    }
}

/// Une copie de la racine d'essai ; les cartes sont refaites avant chaque test.
class DonneesLiens : public ::testing::Test {
protected:
    static inline std::filesystem::path racine;

    static void SetUpTestSuite() {
        racine = std::filesystem::temp_directory_path() /
                 ("jadg-world-links-" + std::to_string(std::rand()));
        // Une copie laissee par une execution interrompue ferait echouer la recopie.
        std::error_code ignore;
        std::filesystem::remove_all(racine, ignore);
        for (const char* dossier : DOSSIERS_LUS) {
            copier(racine, dossier);
        }
    }
    static void TearDownTestSuite() {
        std::error_code ignore;
        std::filesystem::remove_all(racine, ignore);
    }
    void SetUp() override {
        std::error_code ignore;
        std::filesystem::remove_all(racine / "Levels", ignore);
        copier(racine, "Levels");
    }

    [[nodiscard]] std::filesystem::path carte(std::string_view id) const {
        return racine / "Levels" / (std::string{id} + ".json");
    }

    void appliquer(const hmi::RefactorPlan& plan) const {
        ASSERT_TRUE(plan.ok()) << plan.error;
        std::string erreur;
        ASSERT_TRUE(hmi::applyRefactorPlan(plan, erreur)) << erreur;
    }

    [[nodiscard]] std::string constats() const {
        std::string texte;
        for (const hmi::MapCheckFinding& finding : hmi::checkAllMaps(racine).findings) {
            texte += hmi::formatFinding(finding) + "\n";
        }
        return texte;
    }
};

/// Le portail de @p mapId vers @p targetMap, s'il y en a un.
[[nodiscard]] const core::WorldPortalLink* portail(const core::WorldGraph& graphe,
                                                   std::string_view mapId,
                                                   std::string_view targetMap) {
    for (const core::WorldPortalLink* const link : graphe.portalsFrom(mapId)) {
        if (link->toMap == targetMap) {
            return link;
        }
    }
    return nullptr;
}

/// La case du point d'arrivée @p nom de la carte @p file, si elle existe.
[[nodiscard]] std::optional<core::GridPosition> arrivee(const std::filesystem::path& file,
                                                        std::string_view nom) {
    const core::LevelLoadResult carte = core::LevelLoader::loadFromFile(file);
    if (!carte.ok()) {
        return std::nullopt;
    }
    for (const core::MapEntity& entity : carte.level->entities()) {
        if (entity.type != core::SPAWN_POINT_ENTITY_TYPE) {
            continue;
        }
        const auto found = entity.properties.find(std::string{core::SPAWN_POINT_NAME_PROPERTY});
        if (found != entity.properties.end() &&
            std::get<std::string>(found->second) == std::string{nom}) {
            return entity.position;
        }
    }
    return std::nullopt;
}

/// Vrai si le héros peut se tenir sur @p cell en partant de l'entrée de @p file.
[[nodiscard]] bool atteignable(const std::filesystem::path& file, core::GridPosition cell) {
    const core::LevelLoadResult carte = core::LevelLoader::loadFromFile(file);
    if (!carte.ok()) {
        return false;
    }
    const core::ExplorationReach reach(carte.level->tileMap(), {carte.level->entry()});
    return reach.reaches(cell);
}

}  // namespace

/**
 * @brief Acceptation du `LOT-EDITOR-09` : relier deux cartes depuis le graphe produit deux cartes
 *        valides qu'on traverse dans les deux sens.
 * \castest{<b>Relier deux cartes se traverse dans les deux sens.</b><br/>
 * \tcat Unitaire · Le monde<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Copier la racine d'essai.<br/>2. Relier `bourg/place` et `donjon`.<br/>
 * 3. Relire le graphe du monde et contrôler toutes les cartes.<br/>
 * \tattendu Chaque carte a un portail vers l'autre, résolu, et le point d'arrivée qu'il cite ;
 * chaque point d'arrivée est atteignable depuis l'entrée de sa carte ; le contrôle ne signale
 * aucune erreur de plus.
 * }
 */
TEST_F(DonneesLiens, RelierDeuxCartesSeTraverseDansLesDeuxSens) {
    const hmi::MapCheckReport avant = hmi::checkAllMaps(racine);
    ASSERT_TRUE(avant.ok()) << constats();

    hmi::MapLink lien;
    appliquer(hmi::planLinkMaps(racine, "bourg/place", "donjon", &lien));

    EXPECT_EQ(lien.from.arrivalName, "from-donjon");
    EXPECT_EQ(lien.to.arrivalName, "from-place");

    const core::WorldGraph graphe = core::loadWorldGraph(racine / "Levels");
    const core::WorldPortalLink* const aller = portail(graphe, "bourg/place", "donjon");
    const core::WorldPortalLink* const retour = portail(graphe, "donjon", "bourg/place");
    ASSERT_NE(aller, nullptr);
    ASSERT_NE(retour, nullptr);
    EXPECT_EQ(aller->status, core::PortalLinkStatus::Resolved);
    EXPECT_EQ(retour->status, core::PortalLinkStatus::Resolved);
    EXPECT_EQ(aller->arrival, "from-place");
    EXPECT_EQ(retour->arrival, "from-donjon");

    const std::optional<core::GridPosition> surLeDonjon =
        arrivee(carte("donjon"), "from-place");
    const std::optional<core::GridPosition> surLaPlace =
        arrivee(carte("bourg/place"), "from-donjon");
    ASSERT_TRUE(surLeDonjon.has_value());
    ASSERT_TRUE(surLaPlace.has_value());
    EXPECT_TRUE(atteignable(carte("donjon"), *surLeDonjon));
    EXPECT_TRUE(atteignable(carte("bourg/place"), *surLaPlace));

    const hmi::MapCheckReport apres = hmi::checkAllMaps(racine);
    EXPECT_EQ(apres.count(hmi::MapCheckSeverity::Error), 0U) << constats();
}

/**
 * @brief Un lien refusé n'écrit rien : une carte inconnue, une carte reliée à elle-même.
 * \castest{<b>Un lien impossible n'écrit rien.</b><br/>
 * \tcat Unitaire · Le monde<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Relier une carte à elle-même.<br/>2. Relier une carte inconnue.<br/>
 * \tattendu Les deux plans sont refusés, sans aucune écriture, et le disent en anglais.
 * }
 */
TEST_F(DonneesLiens, UnLienImpossibleNecritRien) {
    const hmi::RefactorPlan memeCarte = hmi::planLinkMaps(racine, "donjon", "donjon");
    EXPECT_FALSE(memeCarte.ok());
    EXPECT_TRUE(memeCarte.edits.empty());
    EXPECT_NE(memeCarte.error.find("itself"), std::string::npos);

    const hmi::RefactorPlan inconnue = hmi::planLinkMaps(racine, "donjon", "bourg/repaire");
    EXPECT_FALSE(inconnue.ok());
    EXPECT_TRUE(inconnue.edits.empty());
    EXPECT_NE(inconnue.error.find("cannot be read"), std::string::npos);
}

/**
 * @brief Deux liens entre les mêmes cartes ne se marchent pas dessus : le second point d'arrivée
 *        prend un nom libre, et les deux portails restent résolus.
 * \castest{<b>Deux liens entre les mêmes cartes se distinguent.</b><br/>
 * \tcat Unitaire · Le monde<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Relier deux fois `bourg/place` et `donjon`.<br/>
 * \tattendu Le second lien nomme ses points d'arrivée `from-…-2` ; les quatre portails sont
 * résolus.
 * }
 */
TEST_F(DonneesLiens, DeuxLiensEntreLesMemesCartesSeDistinguent) {
    appliquer(hmi::planLinkMaps(racine, "bourg/place", "donjon"));
    hmi::MapLink second;
    appliquer(hmi::planLinkMaps(racine, "bourg/place", "donjon", &second));

    EXPECT_EQ(second.from.arrivalName, "from-donjon-2");
    EXPECT_EQ(second.to.arrivalName, "from-place-2");

    const core::WorldGraph graphe = core::loadWorldGraph(racine / "Levels");
    const auto resolus = [&graphe](std::string_view mapId, std::string_view target) {
        return std::ranges::count_if(
            graphe.portalsFrom(mapId), [target](const core::WorldPortalLink* link) {
                return link->toMap == target && link->status == core::PortalLinkStatus::Resolved;
            });
    };
    EXPECT_EQ(resolus("bourg/place", "donjon"), 2);
    EXPECT_EQ(resolus("donjon", "bourg/place"), 2);
}

/**
 * @brief Le nom d'un point d'arrivée dit d'où l'on vient, et se décale s'il est pris.
 * \castest{<b>Un point d'arrivée dit d'où l'on vient.</b><br/>
 * \tcat Unitaire · Le monde<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Demander le nom pour `bourg/place` sans rien de pris, puis avec.<br/>
 * \tattendu `from-place`, puis `from-place-2`, puis `from-place-3`.
 * }
 */
TEST(LiensDuMonde, UnPointDarriveeDitDouLonVient) {
    EXPECT_EQ(hmi::arrivalNameFrom("bourg/place", {}), "from-place");
    EXPECT_EQ(hmi::arrivalNameFrom("bourg/place", {"from-place"}), "from-place-2");
    EXPECT_EQ(hmi::arrivalNameFrom("donjon", {"from-donjon", "from-donjon-2"}),
              "from-donjon-3");
}
