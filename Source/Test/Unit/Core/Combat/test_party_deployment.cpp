// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_party_deployment.cpp
 * @brief Tests du verdict d'une zone de combat pour un groupe (`LOT-143`) : la formation adverse
 *        sur la zone seule, les quatre places du groupe, les cases libres qu'il faut à tous
 *        (`EX-EDIT-101`).
 *
 * Les cartes sont écrites ici à la case près : un seuil déplacé se voit au premier test qui le
 * franchit.
 */

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/Arena.h"
#include "Core/Combat/Encounter.h"
#include "Core/Combat/PartyDeployment.h"
#include "Core/Combat/TacticalTerrain.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/MapEntity.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Core/World/CombatZone.h"

namespace {

using core::DeploymentIssueCode;
using core::GridPosition;

/// Deux rats : l'un devant le marqueur, l'autre à sa droite.
[[nodiscard]] core::EncounterCatalog catalogue() {
    core::Encounter rats;
    rats.id = "rats";
    rats.combatants = {
        {.creatureId = "rat", .columnOffset = 0, .rowOffset = -1},
        {.creatureId = "rat", .columnOffset = 1, .rowOffset = 0},
    };
    core::EncounterCatalog rencontres;
    rencontres.encounters.push_back(rats);
    return rencontres;
}

[[nodiscard]] core::MapEntity marqueur(GridPosition cell) {
    return core::MapEntity{.type = "encounter",
                           .position = cell,
                           .properties = {{"encounterId", std::string{"rats"}}},
                           .id = "e9"};
}

[[nodiscard]] core::MapEntity zone(GridPosition origine, int largeur, int hauteur) {
    return core::MapEntity{
        .type = std::string{core::COMBAT_ZONE_ENTITY_TYPE},
        .position = origine,
        .properties = {
            {std::string{core::COMBAT_ZONE_NAME_PROPERTY}, std::string{"sable"}},
            {std::string{core::COMBAT_ZONE_WIDTH_PROPERTY}, static_cast<std::int64_t>(largeur)},
            {std::string{core::COMBAT_ZONE_HEIGHT_PROPERTY}, static_cast<std::int64_t>(hauteur)}},
        .id = "e1"};
}

[[nodiscard]] core::MapEntity entreeAlliee(GridPosition cell, std::int64_t rang) {
    return core::MapEntity{
        .type = std::string{core::ARENA_ENTRY_ENTITY_TYPE},
        .position = cell,
        .properties = {{std::string{core::ARENA_SIDE_PROPERTY}, std::string{"allies"}},
                       {std::string{core::ARENA_RANK_PROPERTY}, rang}},
        .id = "e" + std::to_string(20 + rang)};
}

[[nodiscard]] bool releve(const core::PartyDeployment& verdict, DeploymentIssueCode code) {
    return std::ranges::any_of(verdict.issues, [code](const core::DeploymentIssue& issue) {
        return issue.code == code;
    });
}

}  // namespace

/**
 * @brief Sur un sable dégagé, le groupe prend le front opposé au marqueur.
 * \castest{<b>Une zone dégagée porte le groupe de quatre face à la rencontre.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Une zone de 10 x 6 en (2, 2) sur un champ de 20 x 20, une rencontre de deux rats
 * dont le marqueur est en (9, 4), au bord droit.<br/>2. Juger le déploiement.<br/>
 * \tattendu Aucun défaut ; la zone est celle de la rencontre ; quatre places à gauche, la plus
 * lointaine du marqueur d'abord ; 60 cases reliées pour 24 exigées.
 * }
 */
TEST(PartyDeploymentTest, UneZoneDegageePorteLeGroupeFaceALaRencontre) {
    const std::vector<core::MapEntity> entites{zone({2, 2}, 10, 6), marqueur({9, 4})};
    const auto verdicts = core::analyzePartyDeployment(core::TileMap(20, 20), entites, catalogue());

    ASSERT_EQ(verdicts.size(), 1U);
    const core::PartyDeployment& verdict = verdicts.front();
    EXPECT_TRUE(verdict.valid());
    EXPECT_EQ(verdict.encounterIndex, 1U);
    EXPECT_EQ(verdict.zoneIndex, 0U);
    EXPECT_EQ(verdict.partySize, core::TACTICAL_PARTY_SIZE);
    EXPECT_EQ(verdict.reachableCells, 60);
    EXPECT_EQ(verdict.requiredCells, (2 + 4) * core::TACTICAL_CELLS_PER_COMBATANT);
    ASSERT_EQ(verdict.partyPlaces.size(), 4U);
    // La case la plus lointaine du marqueur, en ordre de lecture : le coin haut-gauche.
    EXPECT_EQ(verdict.partyPlaces.front(), (GridPosition{2, 2}));
    for (const GridPosition place : verdict.partyPlaces) {
        EXPECT_LE(place.column, 3) << place.column << "," << place.row;
    }
}

/**
 * @brief Les entrées d'arène alliées de la zone sont les premières places du groupe.
 * \castest{<b>Le groupe entre d'abord par les entrées alliées.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. La zone dégagée, deux entrées alliées de rangs 2 et 1 dans la zone, une troisième
 * hors de la zone.<br/>2. Juger.<br/>
 * \tattendu Les deux entrées du dedans, par rang, puis deux voisines de la première ; celle du
 * dehors n'est pas prise.
 * }
 */
TEST(PartyDeploymentTest, LeGroupeEntreDAbordParLesEntreesAlliees) {
    const std::vector<core::MapEntity> entites{zone({2, 2}, 10, 6), marqueur({9, 4}),
                                               entreeAlliee({5, 6}, 2), entreeAlliee({4, 3}, 1),
                                               entreeAlliee({15, 15}, 0)};
    const auto verdicts = core::analyzePartyDeployment(core::TileMap(20, 20), entites, catalogue());

    ASSERT_EQ(verdicts.size(), 1U);
    const core::PartyDeployment& verdict = verdicts.front();
    EXPECT_TRUE(verdict.valid());
    ASSERT_EQ(verdict.partyPlaces.size(), 4U);
    EXPECT_EQ(verdict.partyPlaces[0], (GridPosition{4, 3}));
    EXPECT_EQ(verdict.partyPlaces[1], (GridPosition{5, 6}));
    EXPECT_EQ(std::ranges::count(verdict.partyPlaces, GridPosition{15, 15}), 0);
}

/**
 * @brief Réduite, la zone perd la formation et le marqueur : chaque adversaire hors de la zone est
 *        dit, et la rencontre reste jugée sur la zone la plus proche.
 * \castest{<b>Une zone qui laisse la formation dehors est refusée.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. La zone ramenée à 5 x 6, le marqueur en (9, 4) au-dehors.<br/>2. Juger.<br/>
 * \tattendu Le marqueur hors de la zone, puis les deux rats hors de la zone ; la zone reste
 * celle de la rencontre.
 * }
 */
TEST(PartyDeploymentTest, UneZoneQuiLaisseLaFormationDehorsEstRefusee) {
    const std::vector<core::MapEntity> entites{zone({2, 2}, 5, 6), marqueur({9, 4})};
    const auto verdicts = core::analyzePartyDeployment(core::TileMap(20, 20), entites, catalogue());

    ASSERT_EQ(verdicts.size(), 1U);
    const core::PartyDeployment& verdict = verdicts.front();
    EXPECT_FALSE(verdict.valid());
    EXPECT_EQ(verdict.zoneIndex, 0U);
    ASSERT_GE(verdict.issues.size(), 3U);
    EXPECT_EQ(verdict.issues[0].code, DeploymentIssueCode::TriggerOutsideZone);
    EXPECT_EQ(verdict.issues[1].code, DeploymentIssueCode::CombatantOutsideZone);
    EXPECT_EQ(verdict.issues[1].cell, (GridPosition{9, 3}));
    EXPECT_EQ(verdict.issues[2].code, DeploymentIssueCode::CombatantOutsideZone);
    EXPECT_EQ(verdict.issues[2].creatureId, "rat");
}

/**
 * @brief Une zone trop petite pour tout le monde est trop étroite, et le groupe n'y a pas ses
 *        quatre places.
 * \castest{<b>Une zone de 3 x 2 ne loge ni le groupe ni la manœuvre.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Une zone de 3 x 2 autour d'une rencontre de deux rats.<br/>2. Juger.<br/>
 * \tattendu Le groupe trouve ses quatre places (six cases moins les deux rats), mais six cases
 * pour vingt-quatre exigées : la zone est trop étroite.
 * }
 */
TEST(PartyDeploymentTest, UneZoneTropPetiteEstTropEtroite) {
    const std::vector<core::MapEntity> entites{zone({4, 3}, 3, 2), marqueur({5, 4})};
    const auto verdicts = core::analyzePartyDeployment(core::TileMap(20, 20), entites, catalogue());

    ASSERT_EQ(verdicts.size(), 1U);
    const core::PartyDeployment& verdict = verdicts.front();
    EXPECT_EQ(verdict.reachableCells, 6);
    EXPECT_EQ(verdict.partyPlaces.size(), 4U);
    EXPECT_FALSE(releve(verdict, DeploymentIssueCode::PartyCannotDeploy));
    EXPECT_TRUE(releve(verdict, DeploymentIssueCode::ZoneTooNarrow));
}

/**
 * @brief Un mur qui coupe la zone : le groupe ne se place que du côté de la rencontre.
 * \castest{<b>Le groupe ne se déploie que là où il rejoint la rencontre.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Une zone de 6 x 2 coupée par un mur plein en colonne 5 ; la rencontre à droite du
 * mur, sur deux colonnes.<br/>2. Juger.<br/>
 * \tattendu Des quatre cases reliées, deux sont aux rats : deux places seulement, le groupe ne se
 * déploie pas ; les cases de gauche ne comptent pas.
 * }
 */
TEST(PartyDeploymentTest, LeGroupeNeSeDeploieQueLaOuIlRejointLaRencontre) {
    core::TileMap collision(20, 20);
    collision.setTile(5, 3, core::TileType::Wall);
    collision.setTile(5, 4, core::TileType::Wall);
    const std::vector<core::MapEntity> entites{zone({2, 3}, 6, 2), marqueur({6, 4})};
    const auto verdicts = core::analyzePartyDeployment(collision, entites, catalogue());

    ASSERT_EQ(verdicts.size(), 1U);
    const core::PartyDeployment& verdict = verdicts.front();
    EXPECT_EQ(verdict.reachableCells, 4);
    EXPECT_EQ(verdict.partyPlaces.size(), 2U);
    EXPECT_TRUE(releve(verdict, DeploymentIssueCode::PartyCannotDeploy));
    for (const GridPosition place : verdict.partyPlaces) {
        EXPECT_GT(place.column, 5);
    }
}

/**
 * @brief Sans zone de combat sur la carte, la rencontre ne se joue pas ; une rencontre inconnue
 *        n'est pas jugée ici.
 * \castest{<b>Une rencontre sans zone est refusée.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Une carte sans zone, une rencontre connue et une inconnue.<br/>2. Juger.<br/>
 * \tattendu Un seul verdict, « aucune zone », sans zone ni place.
 * }
 */
TEST(PartyDeploymentTest, UneRencontreSansZoneEstRefusee) {
    core::MapEntity inconnue = marqueur({3, 3});
    inconnue.properties["encounterId"] = std::string{"dragons"};
    const std::vector<core::MapEntity> entites{marqueur({9, 4}), inconnue};
    const auto verdicts = core::analyzePartyDeployment(core::TileMap(20, 20), entites, catalogue());

    ASSERT_EQ(verdicts.size(), 1U);
    EXPECT_FALSE(verdicts.front().zoneIndex.has_value());
    ASSERT_EQ(verdicts.front().issues.size(), 1U);
    EXPECT_EQ(verdicts.front().issues.front().code, DeploymentIssueCode::NoCombatZone);
    EXPECT_TRUE(verdicts.front().partyPlaces.empty());
}
