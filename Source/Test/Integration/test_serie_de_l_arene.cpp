// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_serie_de_l_arene.cpp
 * @brief La série de l'arène (`LOT-142`) : six rencontres de difficulté croissante, que le maître
 *        d'arène propose l'une après l'autre, un niveau et un repos entre deux ; et leur
 *        équilibrage par simulation, par rencontre et par composition du groupe.
 *
 * Les compositions sont le groupe entier et les quatre **trios** qu'on obtient en retirant une
 * classe : l'écart de taux de victoire entre les trios dit si une classe est indispensable ou
 * inutile. La mesure complète est longue — six rencontres, cinq compositions, cent graines — :
 * elle ne tourne que si `JADG_SIMULATION_SEEDS` est posée, et écrit sa table dans le fichier que
 * nomme `JADG_SIMULATION_OUT`. La CI joue le test de garde, plus court.
 */

#include <QString>
#include <QtGlobal>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/Encounter.h"
#include "Core/Combat/EncounterDifficulty.h"
#include "Core/Levels/Level.h"
#include "Core/World/WorldTravel.h"
#include "Test/Support/ArenaSimulation.h"

namespace {

const std::filesystem::path ELEMENTS{JADG_ELEMENTS_DIR};

/// Une rencontre de la série et le niveau auquel le groupe la joue.
struct Etape {
    std::string_view rencontre;
    int niveau = 1;
};

/// La série, dans l'ordre où le maître la propose : les bandits de la démo, puis une rencontre par
/// niveau gagné, la dernière au plafond de la version.
constexpr Etape SERIE[] = {
    {.rencontre = "arene-bandits", .niveau = 1},   {.rencontre = "arene-gladiateurs", .niveau = 2},
    {.rencontre = "arene-morts", .niveau = 3},     {.rencontre = "arene-veteran", .niveau = 4},
    {.rencontre = "arene-capitaine", .niveau = 5}, {.rencontre = "arene-champion", .niveau = 5},
};

/// Une composition : le groupe entier, ou un trio sans une classe.
struct Composition {
    std::string nom;
    /// L'identifiant du membre retiré, vide pour le groupe entier.
    std::string sans;
};

[[nodiscard]] std::vector<Composition> compositions() {
    std::vector<Composition> liste{{.nom = "groupe", .sans = {}}};
    for (const std::string& membre : test_support::startingParty()) {
        liste.push_back(
            {.nom = "sans " + membre.substr(std::string_view{"heros-"}.size()), .sans = membre});
    }
    return liste;
}

/// Les sources de combattant d'une composition, au niveau voulu.
[[nodiscard]] std::vector<hmi::HeroContestantSource> sourcesDe(const test_support::Heros& heros,
                                                               const Composition& composition) {
    std::vector<hmi::HeroContestantSource> sources;
    for (std::size_t i = 0; i < heros.members.size(); ++i) {
        if (test_support::startingParty()[i] != composition.sans) {
            sources.push_back(heros.sourceOf(heros.members[i]));
        }
    }
    return sources;
}

/// Le sable, chargé comme le jeu le charge.
[[nodiscard]] core::LevelLoadResult sable() {
    return core::WorldTravel::directoriesLoader({ELEMENTS / "Levels"})(test_support::SABLE);
}

/// Les victoires de @p sources sur @p rencontre, aux graines 1 à @p graines.
[[nodiscard]] int victoires(const core::Level& carte, const test_support::ArenaContent& arene,
                            std::string_view rencontre,
                            const std::vector<hmi::HeroContestantSource>& sources, int graines) {
    int gagnes = 0;
    for (int graine = 1; graine <= graines; ++graine) {
        const std::optional<core::CombatOutcome> issue = test_support::playEncounter(
            carte, arene, rencontre, sources, static_cast<std::uint64_t>(graine));
        EXPECT_TRUE(issue.has_value()) << rencontre << ", graine " << graine << " : pas d'issue";
        if (issue == core::CombatOutcome::Victory) {
            ++gagnes;
        }
    }
    return gagnes;
}

}  // namespace

/**
 * @brief La série monte en difficulté au budget du *Guide du Maître* (p. 82-83) : chaque
 *        rencontre, jugée pour quatre héros à son niveau, est au moins « difficile », et les deux
 *        du niveau 5 sont « mortelles » ; le budget ajusté croît jusqu'au niveau 5. Entre les deux
 *        dernières, c'est la simulation qui ordonne : le budget, qui ne voit pas les attaques
 *        multiples du gladiateur, les juge voisines.
 * \castest{<b>Les six rencontres de la serie sont difficiles, les deux dernieres mortelles, et
 * leur budget croit jusqu'au niveau 5.</b><br/>
 * \tcat Integration · Serie de l'arene · Equilibrage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger les rencontres, le bestiaire et les regles de difficulte livres.<br/>2.
 * Juger chaque rencontre de la serie pour quatre heros de son niveau.<br/>
 * \tattendu Aucune creature inconnue ; « difficile » ou « mortelle », les deux dernieres
 * « mortelle » ; un budget ajuste strictement croissant sur les cinq premieres.
 * }
 */
TEST(SerieDeLArene, LaSerieMonteEnDifficulte) {
    const test_support::ArenaContent arene(ELEMENTS);
    const core::EncounterDifficultyRules regles = core::loadEncounterDifficultyRules(
        ELEMENTS / "Rpg" / "rules" / "encounter-difficulty.json");
    int precedent = 0;
    for (const Etape& etape : SERIE) {
        const core::Encounter* rencontre = arene.encounters.find(etape.rencontre);
        ASSERT_NE(rencontre, nullptr) << etape.rencontre;
        const std::vector<int> niveaux(4, etape.niveau);
        const core::EncounterBudget budget =
            core::rateEncounter(regles, *rencontre, arene.bestiary, niveaux);
        EXPECT_TRUE(budget.unknownCreatures.empty()) << etape.rencontre;
        EXPECT_TRUE(budget.category == "difficile" || budget.category == "mortelle")
            << etape.rencontre << " : " << budget.category << ", " << budget.adjustedExperience
            << " PX ajustes";
        if (etape.rencontre != SERIE[std::size(SERIE) - 1].rencontre) {
            EXPECT_GT(budget.adjustedExperience, precedent) << etape.rencontre;
            precedent = budget.adjustedExperience;
        }
        if (etape.niveau == 5) {
            EXPECT_EQ(budget.category, "mortelle") << etape.rencontre;
        }
    }
}

/**
 * @brief Le test de garde de l'équilibrage : chaque rencontre de la série, jouée par le groupe
 *        entier à son niveau, se termine ; en Release, sur trente graines, le groupe en gagne
 *        dans la bande que la mesure complète a fixée.
 * \castest{<b>Chaque rencontre de la serie se gagne dans sa bande de victoires.</b><br/>
 * \tcat Integration · Serie de l'arene · Equilibrage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Le sable ; le groupe de « Nouvelle partie » monte au niveau de chaque
 * rencontre.<br/>2. Trente combats par rencontre en Release (six en Debug), les deux camps par
 * l'IA.<br/>
 * \tattendu Chaque combat se termine ; en Release, les victoires sont dans la bande de la
 * rencontre.
 * }
 */
TEST(SerieDeLArene, ChaqueRencontreSeGagneDansSaBande) {
#ifdef NDEBUG
    constexpr int GRAINES = 30;
#else
    // Un combat a dix se joue en trois secondes en Debug, sorts compris : la CI ne verifie ici
    // que la terminaison, la bande se tient en Release.
    constexpr int GRAINES = 6;
#endif
    // Les bandes, en victoires sur trente, autour de la mesure complete a cent graines (bilan
    // de la 0.0.2) : 70, 72, 81, 68, 73 et 63 %, a quinze points pres.
    const std::map<std::string_view, std::pair<int, int>> bandes{
        {"arene-bandits", {16, 26}}, {"arene-gladiateurs", {17, 26}}, {"arene-morts", {19, 29}},
        {"arene-veteran", {16, 25}}, {"arene-capitaine", {17, 26}},   {"arene-champion", {14, 24}},
    };
    const core::LevelLoadResult carte = sable();
    ASSERT_TRUE(carte.ok()) << carte.error;
    const test_support::ArenaContent arene(ELEMENTS);
    for (const Etape& etape : SERIE) {
        const test_support::Heros heros =
            test_support::loadHeroes(ELEMENTS, test_support::startingParty(), etape.niveau);
        const int gagnes = victoires(*carte.level, arene, etape.rencontre,
                                     sourcesDe(heros, {.nom = "groupe", .sans = {}}), GRAINES);
        ::testing::Test::RecordProperty(std::string{etape.rencontre}, gagnes);
#ifdef NDEBUG
        const auto [bas, haut] = bandes.at(etape.rencontre);
        EXPECT_GE(gagnes, bas) << etape.rencontre << " : " << gagnes << " sur " << GRAINES;
        EXPECT_LE(gagnes, haut) << etape.rencontre << " : " << gagnes << " sur " << GRAINES;
#else
        static_cast<void>(bandes);
#endif
    }
}

/**
 * @brief La mesure complète de l'équilibrage (`LOT-142`) : chaque rencontre de la série, par le
 *        groupe entier et par les quatre trios, sur `JADG_SIMULATION_SEEDS` graines ; la table va
 *        dans `JADG_SIMULATION_OUT`. Aucune classe n'est indispensable ni inutile : sur la série,
 *        l'écart de taux de victoire entre les trios reste sous vingt points.
 * \castest{<b>Sur la serie, l'ecart de victoires entre les quatre trios reste sous vingt
 * points.</b><br/>
 * \tcat Integration · Serie de l'arene · Equilibrage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser `JADG_SIMULATION_SEEDS` (cent) et `JADG_SIMULATION_OUT`.<br/>2. Jouer chaque
 * rencontre par le groupe et par chaque trio.<br/>
 * \tattendu La table est ecrite ; l'ecart entre le meilleur et le pire trio, en taux sur la
 * serie, est sous vingt points. Sans la variable, le test est saute.
 * }
 */
TEST(SerieDeLArene, MesureCompleteParComposition) {
    const QString graines = qEnvironmentVariable("JADG_SIMULATION_SEEDS");
    if (graines.isEmpty()) {
        GTEST_SKIP() << "JADG_SIMULATION_SEEDS n'est pas posee : pas de mesure complete.";
    }
    const int n = std::max(1, graines.toInt());
    const core::LevelLoadResult carte = sable();
    ASSERT_TRUE(carte.ok()) << carte.error;
    const test_support::ArenaContent arene(ELEMENTS);
    const std::vector<Composition> liste = compositions();

    // victoires[composition][rencontre]
    std::map<std::string, std::map<std::string_view, int>> table;
    for (const Etape& etape : SERIE) {
        const test_support::Heros heros =
            test_support::loadHeroes(ELEMENTS, test_support::startingParty(), etape.niveau);
        for (const Composition& composition : liste) {
            table[composition.nom][etape.rencontre] =
                victoires(*carte.level, arene, etape.rencontre, sourcesDe(heros, composition), n);
        }
    }

    std::ostringstream md;
    md << "| Composition |";
    for (const Etape& etape : SERIE) {
        md << ' ' << etape.rencontre << " (niv. " << etape.niveau << ") |";
    }
    md << " Série |\n|---|";
    for (std::size_t i = 0; i < std::size(SERIE); ++i) {
        md << "---:|";
    }
    md << "---:|\n";
    double meilleur = 0.0;
    double pire = 100.0;
    for (const Composition& composition : liste) {
        md << "| " << composition.nom << " |";
        int total = 0;
        for (const Etape& etape : SERIE) {
            const int v = table[composition.nom][etape.rencontre];
            total += v;
            md << ' ' << (100 * v / n) << " % |";
        }
        const double taux = 100.0 * total / (static_cast<double>(n) * std::size(SERIE));
        md << ' ' << std::fixed << std::setprecision(1) << taux << " % |\n";
        if (!composition.sans.empty()) {
            meilleur = std::max(meilleur, taux);
            pire = std::min(pire, taux);
        }
    }
    md << "\nÉcart entre trios : " << std::fixed << std::setprecision(1) << (meilleur - pire)
       << " points, sur " << n << " graines par case.\n";
    std::cout << md.str();
    if (const QString sortie = qEnvironmentVariable("JADG_SIMULATION_OUT"); !sortie.isEmpty()) {
        std::ofstream(std::filesystem::path(sortie.toStdWString())) << md.str();
    }
    EXPECT_LT(meilleur - pire, 20.0) << md.str();
}
