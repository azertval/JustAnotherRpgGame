// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_class_brawler.cpp
 * @brief Le Brawler du *Player's Guide to Tanares* (p. 192-195) se joue du niveau 1 au niveau 5
 *        (`LOT-132`) : Tough as Nails, Hit the Mark, Extra Attack, et la table qui les donne.
 *
 * Chaque capacité a son test qui la déclenche et vérifie son effet, sur la fiche pré-tirée de la
 * page 195 montée au niveau voulu. Le moteur ne nomme ni la classe ni ses capacités : ce qui
 * s'écrit au journal est le nom que la donnée porte.
 */

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/ActionEconomy.h"
#include "Core/Combat/Arena.h"
#include "Core/Rpg/ClassCapacities.h"
#include "Test/Support/ClassArena.h"

namespace {

using core::CombatantId;
using test_support::journalHas;
using test_support::journalLine;

/// La fiche de la page 195 au niveau @p niveau, sans avertissement de la table.
[[nodiscard]] core::LoadedCharacterSheet brawler(int niveau = 1) {
    core::LoadedCharacterSheet charge = test_support::loadPremade("heros-brawler.json");
    EXPECT_TRUE(charge.errors.empty());
    if (niveau > 1) {
        EXPECT_TRUE(test_support::levelUpTo(charge.sheet, niveau).empty());
    }
    return charge;
}

/// Le Brawler a la hache a deux mains contre un mannequin au contact.
[[nodiscard]] core::ArenaBout combatDe(const core::LoadedCharacterSheet& charge, int caMannequin,
                                       int bonusMannequin) {
    core::ArenaBout bout{.seed = 1, .lethal = true, .heroicMark = false};
    bout.contestants.push_back(test_support::hero(
        charge.sheet, charge.inventory,
        {test_support::weaponAttack(charge.sheet, "hache-a-deux-mains")}, {3, 3}));
    bout.contestants.push_back(
        test_support::dummy("Mannequin", {4, 3}, caMannequin, bonusMannequin));
    return bout;
}

}  // namespace

/**
 * @brief La fiche pre-tiree du Brawler porte Tough as Nails, et rien que la table n'ait nomme.
 * \castest{<b>La fiche de la page 195 se charge avec Tough as Nails et sa CA de 14.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger heros-brawler.json.<br/>2. Lire ses capacites, ses avertissements, sa CA
 * avec ce qu'il porte, et son profil de combat.<br/>
 * \tattendu Une capacite, tough-as-nails ; aucun avertissement ; CA 14 = 10 + Dex 1 + Con 3 (la
 * page) ; treize resistances nommees Tough as Nails.
 * }
 */
TEST(ClassBrawlerTest, LaFichePreTireePorteToughAsNails) {
    const core::LoadedCharacterSheet charge = brawler();
    EXPECT_TRUE(charge.warnings.empty()) << (charge.warnings.empty() ? "" : charge.warnings[0]);
    EXPECT_EQ(test_support::capacityIds(charge.sheet), std::vector<std::string>{"tough-as-nails"});
    EXPECT_EQ(test_support::armorClassOf(charge.sheet, charge.inventory), 14)
        << "10 + Dex 1 + Con 3, la page 195";
    const core::CombatantProfile profil = core::profileFor(charge.sheet);
    EXPECT_EQ(profil.damageTraits.affinities.size(), 13U) << "resistance a tous les types";
    ASSERT_FALSE(profil.damageTraits.affinities.empty());
    EXPECT_EQ(profil.damageTraits.affinities.front().source, "Tough as Nails");
}

/**
 * @brief Tough as Nails : la CA sans armure lit Dex et Con, garde le bouclier, cede a l'armure.
 * \castest{<b>La formule de Tough as Nails se recalcule avec ce que le Brawler porte.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Calculer la CA du Brawler N1 sans rien, avec un bouclier, en cuir, en cuir avec
 * bouclier.<br/>
 * \tattendu 14, 16, 12 (cuir 11 + Dex 1 : la formule ne joue pas sous une armure), 14.
 * }
 */
TEST(ClassBrawlerTest, ToughAsNailsDonneSaCaSansArmure) {
    const core::LoadedCharacterSheet charge = brawler();
    const test_support::RpgCatalogs& catalogues = test_support::rpgCatalogs();
    const core::Armor* bouclier = catalogues.equipment.findArmor("bouclier");
    const core::Armor* cuir = catalogues.equipment.findArmor("cuir");
    ASSERT_NE(bouclier, nullptr);
    ASSERT_NE(cuir, nullptr);
    const core::CharacterSheet& fiche = charge.sheet;
    EXPECT_EQ(core::armorClassFor(fiche, catalogues.rules, nullptr, nullptr), 14);
    EXPECT_EQ(core::armorClassFor(fiche, catalogues.rules, nullptr, bouclier), 16)
        << "le bouclier s'ajoute a la formule";
    EXPECT_EQ(core::armorClassFor(fiche, catalogues.rules, cuir, nullptr), 12)
        << "sous une armure, la formule sans armure ne joue pas";
    EXPECT_EQ(core::armorClassFor(fiche, catalogues.rules, cuir, bouclier), 14);
}

/**
 * @brief Tough as Nails en combat : les degats sont divises, et la capacite se nomme.
 * \castest{<b>Le mannequin touche le Brawler : il ne perd que la moitie des degats, et le journal
 * ecrit « resistance (tranchant ; Tough as Nails) ».</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Monter le Brawler N1 contre un mannequin a +20 au toucher.<br/>2. Passer au tour
 * du mannequin et attaquer ; graine choisie pour toucher.<br/>
 * \tattendu Le montage ecrit « capacites Grom Tranche-Écaille : Tough as Nails » ; la ligne de
 * l'attaque nomme la resistance ; PV perdus = degats lances / 2, arrondi a l'inferieur.
 * }
 */
TEST(ClassBrawlerTest, ToughAsNailsDiviseLesDegatsEtSeNomme) {
    const core::LoadedCharacterSheet charge = brawler();
    core::ArenaSession session(test_support::room());
    core::ArenaBout bout = combatDe(charge, 10, 20);
    std::optional<core::AttackOutcome> coup;
    for (std::uint64_t graine = 1; graine < 40 && !coup.has_value(); ++graine) {
        bout.seed = graine;
        ASSERT_TRUE(session.mount(bout).refusals.empty());
        ASSERT_TRUE(session.start());
        ASSERT_TRUE(session.endTurn());
        const core::ArenaAttack attaque = session.attack(CombatantId{1});
        ASSERT_EQ(attaque.result, core::ArenaActionResult::Done);
        if (attaque.outcome->roll.hit) {
            coup = attaque.outcome;
        }
    }
    ASSERT_TRUE(coup.has_value());
    EXPECT_TRUE(journalHas(session.journal(), "capacites Grom Tranche-Écaille : Tough as Nails"));
    const std::string ligne = journalLine(session.journal(), "attaque Mannequin -> Grom");
    EXPECT_NE(ligne.find("resistance (tranchant ; Tough as Nails)"), std::string::npos) << ligne;
    ASSERT_TRUE(coup->report.has_value());
    const int lances = coup->damage.front().amount;
    EXPECT_EQ(coup->report->hitPointsBefore - coup->report->hitPointsAfter, lances / 2);
}

/**
 * @brief Hit the Mark (N3) : +2 aux jets d'attaque, au nom de la capacite.
 * \castest{<b>Au niveau 3, le jet de la hache porte « + 2 (Hit the Mark) » ; au niveau 2,
 * non.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Monter le Brawler N3, puis N2, contre un mannequin.<br/>2. Attaquer.<br/>
 * \tattendu N3 : le modificateur Hit the Mark vaut +2 sur le jet et au journal ; N2 : aucun.
 * }
 */
TEST(ClassBrawlerTest, HitTheMarkAjouteDeuxAuJet) {
    for (const int niveau : {2, 3}) {
        const core::LoadedCharacterSheet charge = brawler(niveau);
        core::ArenaSession session(test_support::room());
        ASSERT_TRUE(session.mount(combatDe(charge, 10, 0)).refusals.empty());
        ASSERT_TRUE(session.start());
        const core::ArenaAttack attaque = session.attack(CombatantId{2});
        ASSERT_EQ(attaque.result, core::ArenaActionResult::Done);
        const std::vector<core::Modifier>& modificateurs = attaque.outcome->roll.check.modifiers;
        const auto bonus =
            std::ranges::find(modificateurs, "Hit the Mark", &core::Modifier::source);
        if (niveau == 3) {
            ASSERT_NE(bonus, modificateurs.end());
            EXPECT_EQ(bonus->value, 2);
            EXPECT_TRUE(journalHas(session.journal(), "+ 2 (Hit the Mark)"));
        } else {
            EXPECT_EQ(bonus, modificateurs.end()) << "Hit the Mark arrive au niveau 3";
        }
    }
}

/**
 * @brief Extra Attack (N5) : deux attaques pour une action, et pas une de plus.
 * \castest{<b>Au niveau 5, le Brawler attaque deux fois dans le tour ; la seconde est nommee au
 * journal ; une troisieme est refusee, et l'action n'est plus la pour esquiver.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Monter le Brawler N5 contre un mannequin.<br/>2. Attaquer trois fois, puis
 * esquiver.<br/>3. Finir le tour, revenir, attaquer deux fois.<br/>4. Refaire au niveau 4.<br/>
 * \tattendu N5 : Done, Done (« attaque supplementaire Grom Tranche-Écaille (Extra Attack) »),
 * NoAction, esquive refusee ; au tour suivant, deux attaques encore. N4 : la seconde attaque est
 * refusee.
 * }
 */
TEST(ClassBrawlerTest, ExtraAttackDonneDeuxAttaquesParAction) {
    const core::LoadedCharacterSheet charge = brawler(5);
    core::ArenaSession session(test_support::room());
    ASSERT_TRUE(session.mount(combatDe(charge, 10, 0)).refusals.empty());
    ASSERT_TRUE(session.start());
    EXPECT_EQ(session.attack(CombatantId{2}).result, core::ArenaActionResult::Done);
    EXPECT_EQ(session.attack(CombatantId{2}).result, core::ArenaActionResult::Done);
    EXPECT_TRUE(journalHas(session.journal(),
                           "attaque supplementaire Grom Tranche-Écaille (Extra Attack)"));
    EXPECT_EQ(session.attack(CombatantId{2}).result, core::ArenaActionResult::NoAction);
    EXPECT_FALSE(session.dodge()) << "l'action est prise : l'attaque en plus ne sert qu'a frapper";

    ASSERT_TRUE(session.endTurn());
    ASSERT_TRUE(session.endTurn());
    ASSERT_EQ(session.combat().activeCombatant(), CombatantId{1});
    EXPECT_EQ(session.attack(CombatantId{2}).result, core::ArenaActionResult::Done);
    EXPECT_EQ(session.attack(CombatantId{2}).result, core::ArenaActionResult::Done);
    EXPECT_EQ(test_support::journalCount(session.journal(), "attaque supplementaire"), 2U);

    const core::LoadedCharacterSheet niveau4 = brawler(4);
    core::ArenaSession avant(test_support::room());
    ASSERT_TRUE(avant.mount(combatDe(niveau4, 10, 0)).refusals.empty());
    ASSERT_TRUE(avant.start());
    EXPECT_EQ(avant.attack(CombatantId{2}).result, core::ArenaActionResult::Done);
    EXPECT_EQ(avant.attack(CombatantId{2}).result, core::ArenaActionResult::NoAction);
}

/**
 * @brief Un sort ou une esquive ne donne pas l'attaque en plus : seule l'action Attaquer.
 * \castest{<b>Apres une esquive, le Brawler N5 n'attaque pas.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Monter le Brawler N5.<br/>2. Esquiver, puis attaquer.<br/>
 * \tattendu L'attaque est refusee (NoAction) : Extra Attack ne s'octroie qu'a l'action Attaquer.
 * }
 */
TEST(ClassBrawlerTest, ExtraAttackNeSuitQueLActionAttaquer) {
    const core::LoadedCharacterSheet charge = brawler(5);
    core::ArenaSession session(test_support::room());
    ASSERT_TRUE(session.mount(combatDe(charge, 10, 0)).refusals.empty());
    ASSERT_TRUE(session.start());
    ASSERT_TRUE(session.dodge());
    EXPECT_EQ(session.attack(CombatantId{2}).result, core::ArenaActionResult::NoAction);
    EXPECT_EQ(session.combat().find(CombatantId{1})->economy.remaining(core::EXTRA_ATTACK_RESOURCE),
              0);
}

/**
 * @brief Du niveau 1 au niveau 5, la table du livre se lit marche par marche.
 * \castest{<b>Monter le Brawler de la page 195 niveau par niveau donne les capacites de la table,
 * sans capacite manquante.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger la fiche N1.<br/>2. Monter d'un niveau a la fois jusqu'au 5, lire les
 * capacites, le bonus de maitrise et les PV.<br/>
 * \tattendu N1 Tough as Nails ; N2 + Experience ; N3 + Hit the Mark ; N4 + Ability Score
 * Improvement ; N5 + Extra Attack, maitrise +3 ; aucun manquant ; les PV montent a chaque niveau
 * (1d12 + Con, moyenne 7 + 3 = 10) ; la CA reste 14.
 * }
 */
TEST(ClassBrawlerTest, DuNiveau1AuNiveau5LaTableSeLit) {
    core::LoadedCharacterSheet charge = brawler();
    core::CharacterSheet& fiche = charge.sheet;
    const std::vector<std::vector<std::string>> attendues{
        {"tough-as-nails"},
        {"tough-as-nails", "experience"},
        {"tough-as-nails", "experience", "hit-the-mark"},
        {"tough-as-nails", "experience", "hit-the-mark", "ability-score-improvement"},
        {"tough-as-nails", "experience", "hit-the-mark", "ability-score-improvement",
         "extra-attack"},
    };
    int pvPrecedents = fiche.maximumHitPoints;
    for (int niveau = 2; niveau <= 5; ++niveau) {
        const std::vector<std::string> manquants = test_support::levelUpTo(fiche, niveau);
        EXPECT_TRUE(manquants.empty()) << "niveau " << niveau << " : " << manquants.size();
        EXPECT_EQ(fiche.level, niveau);
        EXPECT_EQ(test_support::capacityIds(fiche), attendues[static_cast<std::size_t>(niveau - 1)])
            << "niveau " << niveau;
        EXPECT_EQ(fiche.maximumHitPoints - pvPrecedents, 10) << "1d12 (7) + Con 3";
        pvPrecedents = fiche.maximumHitPoints;
        EXPECT_EQ(test_support::armorClassOf(fiche, charge.inventory), 14);
    }
    EXPECT_EQ(core::proficiencyBonus(fiche, test_support::rpgCatalogs().experience), 3);
    ASSERT_TRUE(core::extraAttacksFrom(fiche.capacities).has_value());
    EXPECT_EQ(core::extraAttacksFrom(fiche.capacities)->count, 1);
}

/**
 * @brief Un effet extra-attack sans attaque en plus refuse la capacite.
 * \castest{<b>Une capacite qui declare « extra-attack » a 0 est refusee et nommee.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ecrire une capacite extra-attack a value 0 dans un dossier temporaire.<br/>2. Charger
 * le dossier.<br/>
 * \tattendu Aucune capacite, une erreur qui nomme le fichier.
 * }
 */
TEST(ClassBrawlerTest, UneAttaqueEnPlusNulleEstRefusee) {
    const std::filesystem::path dossier =
        std::filesystem::temp_directory_path() / "jadg-lot-132-extra-attack";
    std::filesystem::create_directories(dossier);
    {
        std::ofstream fichier(dossier / "vide.json");
        fichier << R"({"id": "vide", "name": "Vide", "source": "original",
                      "effects": [{"kind": "extra-attack", "value": 0}], "text": "essai"})";
    }
    const core::CapacityCatalog catalogue = core::loadCapacities(dossier);
    std::filesystem::remove_all(dossier);
    EXPECT_TRUE(catalogue.capacities.empty());
    ASSERT_EQ(catalogue.errors.size(), 1U);
    EXPECT_NE(catalogue.errors.front().find("vide.json"), std::string::npos);
}
