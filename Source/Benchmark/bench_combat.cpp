// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file bench_combat.cpp
 * @brief Mesures du combat tactique : déplacement (`LOT-19`), ligne de vue (`LOT-22`), IA
 * (`LOT-23`).
 *
 * Les scènes sont fixes et déterministes : deux nuits sur le même code mesurent la même chose, et
 * seul le code change la courbe. Chaque mesure construit sa scène hors de la boucle chronométrée.
 */

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include "Core/Combat/Arena.h"
#include "Core/Combat/BattleGrid.h"
#include "Core/Combat/EnemyAi.h"
#include "Core/Combat/LineOfSight.h"
#include "Core/Combat/Pathfinding.h"
#include "Core/Levels/GridPosition.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/TileMap.h"
#include "Core/Levels/TileType.h"
#include "Core/Rpg/Dice.h"

namespace {

constexpr core::CombatantId HEROS{1};

/// Une carte carrée ceinte de murs, un pilier toutes les quatre cases : assez d'obstacles pour que
/// les chemins contournent et que les lignes de vue se coupent.
core::TileMap carteAPiliers(int cote) {
    core::TileMap carte(cote, cote);
    for (int i = 0; i < cote; ++i) {
        carte.setTile(i, 0, core::TileType::Wall);
        carte.setTile(i, cote - 1, core::TileType::Wall);
        carte.setTile(0, i, core::TileType::Wall);
        carte.setTile(cote - 1, i, core::TileType::Wall);
    }
    for (int row = 3; row < cote - 3; row += 4) {
        for (int column = 3; column < cote - 3; column += 4) {
            carte.setTile(column, row, core::TileType::Wall);
        }
    }
    return carte;
}

/// La même carte en grille de combat, avec une bande de terrain difficile toutes les sept lignes.
core::BattleGrid grilleAPiliers(int cote) {
    core::BattleGrid grille{carteAPiliers(cote)};
    for (int row = 5; row < cote - 1; row += 7) {
        for (int column = 1; column < cote - 1; ++column) {
            if (column % 4 != 3) {
                grille.setDifficult({column, row}, true);
            }
        }
    }
    return grille;
}

core::ArenaContestant combattant(const std::string& nom, core::CombatSide camp,
                                 core::GridPosition case_) {
    core::CombatantProfile profil{.name = nom,
                                  .side = camp,
                                  .maximumHitPoints = 20,
                                  .currentHitPoints = 20,
                                  .dexterity = 12,
                                  .initiativeModifier = 1,
                                  .movement = 6};
    profil.armorClass = 12;
    core::AttackProfile coup;
    coup.label = nom;
    coup.modifiers = {{.source = "bonus d'attaque", .value = 4}};
    coup.damage = {
        {.dice = *core::parseDice("1d6+2"), .type = core::DamageType::Slashing, .flags = 0}};
    return {.profile = profil, .attacks = {coup}, .position = case_, .markId = {}, .behavior = {}};
}

}  // namespace

/// Toutes les cases atteignables d'un tour avec la course (budget 12), au centre d'une grille
/// 48×48.
static void ReachableAreaDashing(benchmark::State& state) {
    core::BattleGrid grille = grilleAPiliers(48);
    if (grille.place(HEROS, {24, 22}) != core::PlacementResult::Placed) {
        state.SkipWithError("placement refusé");
        return;
    }
    const core::Mover mover{.combatant = HEROS};
    for (auto _ : state) {
        const core::ReachableArea aire(grille, mover, 12);
        benchmark::DoNotOptimize(aire.destinations());
    }
}
BENCHMARK(ReachableAreaDashing);

/// A* d'un coin à l'autre d'une grille 64×64 : l'IA qui marche vers une cible lointaine.
static void FindPathAcrossGrid(benchmark::State& state) {
    core::BattleGrid grille = grilleAPiliers(64);
    if (grille.place(HEROS, {1, 1}) != core::PlacementResult::Placed) {
        state.SkipWithError("placement refusé");
        return;
    }
    const core::Mover mover{.combatant = HEROS};
    for (auto _ : state) {
        std::optional<core::Path> chemin = core::findPath(grille, mover, {62, 61});
        benchmark::DoNotOptimize(chemin);
    }
}
BENCHMARK(FindPathAcrossGrid);

/// Ligne de vue entre deux emprises éloignées, à travers la forêt de piliers.
static void LineOfSightAcrossGrid(benchmark::State& state) {
    const core::BattleGrid grille = grilleAPiliers(48);
    const core::Footprint archer{.anchor = {1, 2}, .side = 1};
    const core::Footprint cible{.anchor = {45, 40}, .side = 2};
    for (auto _ : state) {
        benchmark::DoNotOptimize(core::hasLineOfSight(grille, archer, cible));
    }
}
BENCHMARK(LineOfSightAcrossGrid);

/// L'abri d'une cible de grande taille, derrière trois créatures interposées.
static void CoverFromWithInterposed(benchmark::State& state) {
    const core::BattleGrid grille = grilleAPiliers(48);
    const core::Footprint archer{.anchor = {2, 2}, .side = 1};
    const core::Footprint cible{.anchor = {30, 26}, .side = 2};
    const std::vector<core::Footprint> interposes{{.anchor = {10, 9}, .side = 1},
                                                  {.anchor = {17, 15}, .side = 1},
                                                  {.anchor = {24, 21}, .side = 2}};
    for (auto _ : state) {
        benchmark::DoNotOptimize(core::coverFrom(grille, archer, cible, interposes));
    }
}
BENCHMARK(CoverFromWithInterposed);

/// La décision d'un tour d'IA au profil agressif, quatre contre quatre dans une salle 20×20.
static void PlanTurnFourVersusFour(benchmark::State& state) {
    const core::BehaviorCatalog profils =
        core::loadBehaviors(std::filesystem::path(JADG_RPG_RULES_DIR) / "behaviors.json");
    const core::BehaviorProfile* agressif = profils.find("aggressive");
    if (agressif == nullptr) {
        state.SkipWithError("profil « aggressive » absent de behaviors.json");
        return;
    }

    core::ArenaSession session(core::Level(core::LevelData{
        .name = "salle", .tileMap = carteAPiliers(20), .entities = {}, .entry = {1, 1}}));
    core::ArenaBout bout{.seed = 7, .lethal = false, .heroicMark = false};
    for (int i = 0; i < 4; ++i) {
        bout.contestants.push_back(
            combattant("Allie" + std::to_string(i), core::CombatSide::Allies, {2, 2 + (4 * i)}));
        bout.contestants.push_back(
            combattant("Ennemi" + std::to_string(i), core::CombatSide::Enemies, {17, 2 + (4 * i)}));
    }
    session.mount(bout);
    if (!session.start() || !session.combat().activeCombatant()) {
        state.SkipWithError("le combat ne démarre pas");
        return;
    }
    const core::CombatantId actif = *session.combat().activeCombatant();
    for (auto _ : state) {
        benchmark::DoNotOptimize(core::planTurn(session, actif, *agressif));
    }
}
BENCHMARK(PlanTurnFourVersusFour);
