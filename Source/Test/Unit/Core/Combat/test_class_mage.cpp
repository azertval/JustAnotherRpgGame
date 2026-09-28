// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_class_mage.cpp
 * @brief Le Mage du *Player's Guide to Tanares* (p. 196-199) se joue du niveau 1 au niveau 5
 *        (`LOT-133`) : ses sorts mineurs, ses sorts à deux lancers par jour, *Arcane Protection*,
 *        et les mécanismes de sort qu'ils demandent — projectiles, touche sans jet, sauvegarde
 *        dans une sphère, effets qui durent sous concentration.
 *
 * Chaque sort joué a son test qui le lance et vérifie son effet et sa ligne de journal, sur la
 * fiche pré-tirée de la page 199 montée au niveau voulu.
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
#include "Core/Rpg/Spell.h"
#include "Test/Support/ClassArena.h"

namespace {

using core::CombatantId;
using test_support::journalCount;
using test_support::journalHas;
using test_support::journalLine;

constexpr const char* MAGE = "Faelar Trace-Carte";

/// La fiche de la page 199 au niveau @p niveau, sans avertissement de la table.
[[nodiscard]] core::LoadedCharacterSheet mage(int niveau = 1) {
    core::LoadedCharacterSheet charge = test_support::loadPremade("heros-mage.json");
    EXPECT_TRUE(charge.errors.empty());
    if (niveau > 1) {
        EXPECT_TRUE(test_support::levelUpTo(charge.sheet, niveau).empty());
    }
    return charge;
}

/// Le Mage en (1, 3), 200 PV pour encaisser, puis les mannequins.
[[nodiscard]] core::ArenaBout combatDe(const core::LoadedCharacterSheet& charge,
                                       std::vector<core::ArenaContestant> autres) {
    core::ArenaBout bout{.seed = 1, .lethal = true, .heroicMark = false};
    core::ArenaContestant heros =
        test_support::hero(charge.sheet, charge.inventory,
                           {test_support::weaponAttack(charge.sheet, "baton")}, {1, 3});
    heros.profile.maximumHitPoints = 200;
    heros.profile.currentHitPoints = 200;
    bout.contestants.push_back(std::move(heros));
    for (core::ArenaContestant& autre : autres) {
        bout.contestants.push_back(std::move(autre));
    }
    return bout;
}

/// L'indice du sort @p id dans le grimoire du combattant 1.
[[nodiscard]] std::size_t sortDe(const core::ArenaSession& session, const char* id) {
    const std::vector<core::ArenaSpell>* grimoire = session.spells(CombatantId{1});
    EXPECT_NE(grimoire, nullptr);
    for (std::size_t i = 0; grimoire != nullptr && i < grimoire->size(); ++i) {
        if ((*grimoire)[i].id == id) {
            return i;
        }
    }
    ADD_FAILURE() << id << " absent du grimoire";
    return 0;
}

/// Passe les tours jusqu'a celui du combattant @p id.
void jusquAuTourDe(core::ArenaSession& session, CombatantId id) {
    for (int garde = 0; garde < 8 && session.combat().activeCombatant() != id; ++garde) {
        ASSERT_TRUE(session.endTurn());
    }
    ASSERT_EQ(session.combat().activeCombatant(), id);
}

}  // namespace

/**
 * @brief La fiche pre-tiree du Mage porte ses sorts de niveau 1 et se joue avec eux.
 * \castest{<b>La fiche de la page 199 connait trait de feu, lumiere, detection de la magie et
 * projectile magique ; les deux qui se jouent en combat entrent au grimoire.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger heros-mage.json.<br/>2. Lire capacites, sorts connus, CA, et le grimoire
 * de combat.<br/>
 * \tattendu Aucun avertissement ; Simplified Spellcasting et Specific Cantrips ; CA 12 (Arcane
 * Protection n'arrive qu'au niveau 2) ; grimoire : trait de feu a volonte (+5 au toucher, 1d10
 * feu), projectile magique a deux lancers ; lumiere et detection de la magie dites, sans effet en
 * combat.
 * }
 */
TEST(ClassMageTest, LaFichePreTireeSeJoueAvecSesSortsDeNiveau1) {
    const core::LoadedCharacterSheet charge = mage();
    EXPECT_TRUE(charge.warnings.empty()) << (charge.warnings.empty() ? "" : charge.warnings[0]);
    EXPECT_EQ(test_support::capacityIds(charge.sheet),
              (std::vector<std::string>{"simplified-spellcasting", "specific-cantrips"}));
    EXPECT_EQ(test_support::armorClassOf(charge.sheet, charge.inventory), 12);
    std::vector<std::string> connus;
    for (const core::KnownSpell& sort : charge.sheet.knownSpells) {
        connus.push_back(sort.spellId);
    }
    EXPECT_EQ(connus,
              (std::vector<std::string>{"fire-bolt", "light", "detect-magic", "magic-missile"}));

    const test_support::RpgCatalogs& catalogues = test_support::rpgCatalogs();
    std::vector<std::string> ignores;
    const std::vector<core::ArenaSpell> grimoire = core::arenaSpellsFor(
        charge.sheet, *catalogues.options.findClass("mage"), catalogues.options.spells, 2, ignores);
    ASSERT_EQ(grimoire.size(), 2U);
    EXPECT_EQ(grimoire[0].id, "fire-bolt");
    EXPECT_EQ(grimoire[0].mechanism, core::SpellMechanism::AttackRoll);
    EXPECT_EQ(grimoire[0].uses, -1) << "un sort mineur se lance a volonte";
    int toucher = 0;
    for (const core::Modifier& modificateur : grimoire[0].attack.modifiers) {
        toucher += modificateur.value;
    }
    EXPECT_EQ(toucher, 5) << "Int 16 (+3) + maitrise 2, la page 199";
    EXPECT_EQ(grimoire[0].attack.damage.front().dice,
              (core::Dice{.count = 1, .faces = 10, .modifier = 0}));
    EXPECT_EQ(grimoire[1].id, "magic-missile");
    EXPECT_EQ(grimoire[1].mechanism, core::SpellMechanism::AutoHit);
    EXPECT_EQ(grimoire[1].uses, 2);
    EXPECT_EQ(grimoire[1].saveDc, 13) << "8 + maitrise 2 + Int 3";
    EXPECT_EQ(ignores, (std::vector<std::string>{"Lumiere", "Detection de la magie"}));
}

/**
 * @brief Trait de feu : une attaque de sort a distance, nommee au journal ; 2d10 au niveau 5.
 * \castest{<b>Le Mage lance trait de feu sur un mannequin a six cases ; au niveau 5 le sort lance
 * deux d10.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Mage N1 contre un mannequin en (7, 3).<br/>2. Lancer trait de feu.<br/>3. Refaire
 * au niveau 5.<br/>
 * \tattendu Done, une ligne « sort Trait de feu : attaque … (Trait de feu) » ; le sort reste a
 * volonte ; N5 : 2d10.
 * }
 */
TEST(ClassMageTest, TraitDeFeuEstUneAttaqueDeSort) {
    for (const int niveau : {1, 5}) {
        const core::LoadedCharacterSheet charge = mage(niveau);
        core::ArenaSession session(test_support::room());
        ASSERT_TRUE(
            session.mount(combatDe(charge, {test_support::dummy("Mannequin", {7, 3}, 10, 0)}))
                .refusals.empty());
        ASSERT_TRUE(session.start());
        const core::ArenaAttack lancer =
            session.castSpell(CombatantId{2}, sortDe(session, "fire-bolt"));
        ASSERT_EQ(lancer.result, core::ArenaActionResult::Done);
        EXPECT_TRUE(journalHas(session.journal(), "sort Trait de feu : attaque " +
                                                      std::string(MAGE) + " -> Mannequin"));
        const core::ArenaSpell& sort = (*session.spells(CombatantId{1}))[0];
        EXPECT_EQ(sort.uses, -1);
        EXPECT_EQ(sort.attack.damage.front().dice.count, niveau == 5 ? 2 : 1)
            << "un d10 de plus au niveau 5";
    }
}

/**
 * @brief Projectile magique : trois flechettes sans jet, 1d4+1 de force chacune, deux par jour.
 * \castest{<b>Contre une CA de 40, projectile magique touche : trois fois 1d4+1 de force ; au
 * troisieme lancer du jour, le sort est epuise.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Mage N1 contre un mannequin a la CA 40.<br/>2. Lancer projectile magique ; tour
 * suivant, le relancer ; tour suivant, le relancer.<br/>
 * \tattendu Premier lancer : « touche sans jet (3 projectile(s)) », trois clauses 1d4+1 force,
 * PV perdus entre 6 et 15 ; deuxieme : 0 restant ; troisieme : Exhausted, aucune action depensee.
 * }
 */
TEST(ClassMageTest, ProjectileMagiqueToucheSansJet) {
    const core::LoadedCharacterSheet charge = mage();
    core::ArenaSession session(test_support::room());
    ASSERT_TRUE(session.mount(combatDe(charge, {test_support::dummy("Mannequin", {7, 3}, 40, 0)}))
                    .refusals.empty());
    ASSERT_TRUE(session.start());
    const std::size_t indice = sortDe(session, "magic-missile");
    const core::ArenaAttack lancer = session.castSpell(CombatantId{2}, indice);
    ASSERT_EQ(lancer.result, core::ArenaActionResult::Done);
    EXPECT_FALSE(lancer.outcome.has_value()) << "aucun jet d'attaque";
    EXPECT_NE(lancer.summary.find("touche sans jet (3 projectile(s))"), std::string::npos)
        << lancer.summary;
    EXPECT_EQ(journalCount(session.journal(), "force"), 1U) << "une ligne pour la salve";
    const core::Combatant* mannequin = session.combat().find(CombatantId{2});
    const int perdus = mannequin->profile.maximumHitPoints - mannequin->profile.currentHitPoints;
    EXPECT_GE(perdus, 6);
    EXPECT_LE(perdus, 15);

    jusquAuTourDe(session, CombatantId{2});
    jusquAuTourDe(session, CombatantId{1});
    ASSERT_EQ(session.castSpell(CombatantId{2}, indice).result, core::ArenaActionResult::Done);
    EXPECT_TRUE(journalHas(session.journal(), "sort Projectile magique (0 restant)"));
    jusquAuTourDe(session, CombatantId{2});
    jusquAuTourDe(session, CombatantId{1});
    EXPECT_EQ(session.castSpell(CombatantId{2}, indice).result, core::ArenaActionResult::Exhausted);
    EXPECT_GT(session.combat().find(CombatantId{1})->economy.remaining(core::ACTION_RESOURCE), 0)
        << "un sort epuise ne coute pas l'action";
}

/**
 * @brief Arcane Protection (N2) : sans armure, CA 13 + Dex.
 * \castest{<b>Au niveau 2, la CA du Mage passe de 12 a 15.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Monter le Mage de la page 199 au niveau 2.<br/>2. Lire sa CA avec ce qu'il porte,
 * et son profil de combat.<br/>
 * \tattendu 15 = 13 + Dex 2 ; Arcane Protection est active.
 * }
 */
TEST(ClassMageTest, ArcaneProtectionDonneTreizePlusDex) {
    const core::LoadedCharacterSheet charge = mage(2);
    EXPECT_EQ(test_support::armorClassOf(charge.sheet, charge.inventory), 15);
    const std::vector<std::string> capacites = test_support::capacityIds(charge.sheet);
    EXPECT_NE(std::ranges::find(capacites, "arcane-protection"), capacites.end());
}

/**
 * @brief Rayon ardent (N3) : trois rayons, un jet chacun ; ceux qui restent quand la cible tombe
 *        sont perdus.
 * \castest{<b>Trois lignes « 1/3 », « 2/3 », « 3/3 » contre un mannequin solide ; contre un
 * mannequin a 1 PV touche au premier rayon, deux rayons perdus.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Mage N3 contre un mannequin a 200 PV, lancer rayon ardent.<br/>2. Mage N3 contre un
 * mannequin a 1 PV et CA 0 ; graine choisie pour que le premier rayon touche.<br/>
 * \tattendu Trois jets d'attaque au journal ; puis « 2 projectile(s) perdu(s) ».
 * }
 */
TEST(ClassMageTest, RayonArdentJetteUnJetParRayon) {
    const core::LoadedCharacterSheet charge = mage(3);
    {
        core::ArenaSession session(test_support::room());
        ASSERT_TRUE(
            session.mount(combatDe(charge, {test_support::dummy("Mannequin", {7, 3}, 10, 0, 200)}))
                .refusals.empty());
        ASSERT_TRUE(session.start());
        ASSERT_EQ(session.castSpell(CombatantId{2}, sortDe(session, "scorching-ray")).result,
                  core::ArenaActionResult::Done);
        for (const char* rang : {"1/3 : attaque", "2/3 : attaque", "3/3 : attaque"}) {
            EXPECT_TRUE(journalHas(session.journal(), rang)) << rang;
        }
    }
    core::ArenaSession session(test_support::room());
    core::ArenaBout bout = combatDe(charge, {test_support::dummy("Mannequin", {7, 3}, 0, 0, 1),
                                             test_support::dummy("Autre", {7, 6}, 10, 0)});
    bool perdus = false;
    for (std::uint64_t graine = 1; graine < 40 && !perdus; ++graine) {
        bout.seed = graine;
        ASSERT_TRUE(session.mount(bout).refusals.empty());
        ASSERT_TRUE(session.start());
        ASSERT_EQ(session.castSpell(CombatantId{2}, sortDe(session, "scorching-ray")).result,
                  core::ArenaActionResult::Done);
        perdus = journalHas(session.journal(), "2 projectile(s) perdu(s)");
    }
    EXPECT_TRUE(perdus);
}

/**
 * @brief Invisibilite (N3) : attaque desavantagee contre le Mage, avantage au sien, fin quand il
 *        lance un sort.
 * \castest{<b>Le Mage invisible est attaque avec desavantage ; son trait de feu suivant a
 * l'avantage, puis l'invisibilite prend fin et le journal le dit.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Mage N3 au contact d'un mannequin ; lancer invisibilite sur soi.<br/>2. Tour du
 * mannequin : il attaque le Mage.<br/>3. Tour du Mage : trait de feu.<br/>
 * \tattendu « effet Invisibilite sur … : invisible » ; l'attaque du mannequin porte « cible
 * invisible » ; le trait de feu porte « attaquant invisible » ; « fin de l'effet Invisibilite …
 * (il lance un sort) » ; plus d'effet.
 * }
 */
TEST(ClassMageTest, InvisibiliteGeneLAttaquantEtCesseAuSortSuivant) {
    const core::LoadedCharacterSheet charge = mage(3);
    core::ArenaSession session(test_support::room());
    ASSERT_TRUE(session.mount(combatDe(charge, {test_support::dummy("Mannequin", {2, 3}, 10, 5)}))
                    .refusals.empty());
    ASSERT_TRUE(session.start());
    ASSERT_EQ(session.castSpell(CombatantId{2}, sortDe(session, "invisibility")).result,
              core::ArenaActionResult::InvalidTarget)
        << "invisibilite ne se pose pas sur un ennemi";
    ASSERT_EQ(session.castSpell(CombatantId{1}, sortDe(session, "invisibility")).result,
              core::ArenaActionResult::Done);
    EXPECT_TRUE(session.hasEffect(CombatantId{1}, core::SpellEffectKind::Invisible));
    EXPECT_TRUE(journalHas(session.journal(), "effet Invisibilite sur " + std::string(MAGE)));

    jusquAuTourDe(session, CombatantId{2});
    const core::ArenaAttack coup = session.attack(CombatantId{1});
    ASSERT_EQ(coup.result, core::ArenaActionResult::Done);
    const std::vector<std::string>& desavantages = coup.outcome->roll.disadvantages;
    EXPECT_NE(std::ranges::find(desavantages, "cible invisible"), desavantages.end());

    jusquAuTourDe(session, CombatantId{1});
    const core::ArenaAttack trait = session.castSpell(CombatantId{2}, sortDe(session, "fire-bolt"));
    ASSERT_EQ(trait.result, core::ArenaActionResult::Done);
    const std::vector<std::string>& avantages = trait.outcome->roll.advantages;
    EXPECT_NE(std::ranges::find(avantages, "attaquant invisible"), avantages.end());
    EXPECT_TRUE(journalHas(session.journal(), "fin de l'effet Invisibilite sur " +
                                                  std::string(MAGE) + " (il lance un sort)"));
    EXPECT_FALSE(session.hasEffect(CombatantId{1}, core::SpellEffectKind::Invisible));
}

/**
 * @brief Boule de feu (N5) : une sauvegarde de Dexterite par creature de la sphere, la moitie a
 *        qui reussit, allies compris ; rien hors de la sphere.
 * \castest{<b>La boule de feu centree sur un mannequin prend son voisin et l'allie a trois cases,
 * pas le mannequin eloigne ; chacun a sa ligne de sauvegarde.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Mage N5 en (1, 3) ; mannequins en (8, 3) et (9, 4), un allie en (8, 6), un mannequin
 * lointain en (3, 7), hors des quatre cases du rayon.<br/>2. Boule de feu sur (8, 3).<br/>
 * \tattendu Entete « sauvegarde de Dexterite DD 14 ; 3 creature(s) » et 8d6 ; trois lignes de
 * sauvegarde ; le lointain intact ; qui reussit perd la moitie des des lances.
 * }
 */
TEST(ClassMageTest, BouleDeFeuFaitSauvegarderToutLeMonde) {
    const core::LoadedCharacterSheet charge = mage(5);
    core::ArenaSession session(test_support::room());
    ASSERT_TRUE(session
                    .mount(combatDe(charge, {test_support::dummy("Cible", {8, 3}, 10, 0, 200),
                                             test_support::dummy("Voisin", {9, 4}, 10, 0, 200),
                                             test_support::dummy("Allie", {8, 6}, 10, 0, 200,
                                                                 core::CombatSide::Allies),
                                             test_support::dummy("Lointain", {3, 7}, 10, 0, 200)}))
                    .refusals.empty());
    ASSERT_TRUE(session.start());
    const core::ArenaAttack lancer = session.castSpell(CombatantId{2}, sortDe(session, "fireball"));
    ASSERT_EQ(lancer.result, core::ArenaActionResult::Done);
    EXPECT_NE(lancer.summary.find("sauvegarde de Dexterite DD 14 ; 3 creature(s)"),
              std::string::npos)
        << lancer.summary;
    EXPECT_NE(lancer.summary.find("8d6"), std::string::npos) << lancer.summary;
    for (const char* nom : {"  Cible : d20", "  Voisin : d20", "  Allie : d20"}) {
        EXPECT_TRUE(journalHas(session.journal(), nom)) << nom;
    }
    EXPECT_FALSE(journalHas(session.journal(), "  Lointain : d20"));
    const core::Combatant* lointain = session.combat().find(CombatantId{5});
    EXPECT_EQ(lointain->profile.currentHitPoints, 200);
    // Qui reussit perd la moitie de ce que les des ont fait ; qui rate, tout.
    for (const CombatantId id : {CombatantId{2}, CombatantId{3}, CombatantId{4}}) {
        const core::Combatant* c = session.combat().find(id);
        const std::string ligne = journalLine(session.journal(), "  " + c->profile.name + " :");
        const int perdus = c->profile.maximumHitPoints - c->profile.currentHitPoints;
        EXPECT_GE(perdus, ligne.find("moitie") != std::string::npos ? 4 : 8) << ligne;
    }
}

/**
 * @brief Vol (N5) : le Mage vole a 12 cases par tour ; un second sort de concentration y met fin.
 * \castest{<b>Le Mage se lance vol : il vole, 12 cases ; il lance ensuite invisibilite, et le vol
 * prend fin, sa marche et ses 6 cases rendues.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Mage N5 ; vol sur soi.<br/>2. Tour suivant : invisibilite sur soi.<br/>
 * \tattendu Profil en vol, 12 cases, budget du tour a 12 ; puis « fin de l'effet Vol …
 * (concentration sur Invisibilite) », marche, 6 cases.
 * }
 */
TEST(ClassMageTest, VolDonneDouzeCasesEtCedeALaConcentration) {
    const core::LoadedCharacterSheet charge = mage(5);
    core::ArenaSession session(test_support::room());
    ASSERT_TRUE(session.mount(combatDe(charge, {test_support::dummy("Mannequin", {9, 6}, 10, 0)}))
                    .refusals.empty());
    ASSERT_TRUE(session.start());
    const int marche = session.combat().find(CombatantId{1})->profile.movement;
    EXPECT_EQ(marche, 6) << "9 m, l'elfe";
    ASSERT_EQ(session.castSpell(CombatantId{1}, sortDe(session, "fly")).result,
              core::ArenaActionResult::Done);
    const core::Combatant* mage1 = session.combat().find(CombatantId{1});
    EXPECT_EQ(mage1->profile.locomotion, core::Locomotion::Fly);
    EXPECT_EQ(mage1->profile.movement, 12) << "18 m";
    EXPECT_EQ(mage1->economy.remaining(core::MOVEMENT_RESOURCE), 12);
    EXPECT_TRUE(journalHas(session.journal(), "vole, 12 cases par tour"));

    jusquAuTourDe(session, CombatantId{2});
    jusquAuTourDe(session, CombatantId{1});
    ASSERT_EQ(session.castSpell(CombatantId{1}, sortDe(session, "invisibility")).result,
              core::ArenaActionResult::Done);
    EXPECT_TRUE(journalHas(session.journal(), "fin de l'effet Vol sur " + std::string(MAGE) +
                                                  " (concentration sur Invisibilite)"));
    const core::Combatant* apres = session.combat().find(CombatantId{1});
    EXPECT_EQ(apres->profile.locomotion, core::Locomotion::Walk);
    EXPECT_EQ(apres->profile.movement, marche);
    EXPECT_FALSE(session.hasEffect(CombatantId{1}, core::SpellEffectKind::Fly));
}

/**
 * @brief Du niveau 1 au niveau 5, la table du Mage se lit marche par marche.
 * \castest{<b>Monter le Mage de la page 199 niveau par niveau donne les capacites et les sorts de
 * la table, sans manquant.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger la fiche N1.<br/>2. Monter jusqu'au 5, lire capacites et sorts connus.<br/>
 * \tattendu N2 + Experience, Arcane Protection ; N3 + main du mage, invisibilite, rayon ardent ;
 * N4 + Ability Score Improvement ; N5 + boule de feu, vol ; aucun manquant ; les sorts a deux
 * lancers, les mineurs a volonte.
 * }
 */
TEST(ClassMageTest, DuNiveau1AuNiveau5LaTableSeLit) {
    core::LoadedCharacterSheet charge = mage();
    core::CharacterSheet& fiche = charge.sheet;
    const std::vector<std::size_t> nombreDeSorts{4, 4, 7, 7, 9};
    for (int niveau = 2; niveau <= 5; ++niveau) {
        EXPECT_TRUE(test_support::levelUpTo(fiche, niveau).empty()) << "niveau " << niveau;
        EXPECT_EQ(fiche.knownSpells.size(), nombreDeSorts[static_cast<std::size_t>(niveau - 1)])
            << "niveau " << niveau;
    }
    EXPECT_EQ(
        test_support::capacityIds(fiche),
        (std::vector<std::string>{"simplified-spellcasting", "specific-cantrips", "experience",
                                  "arcane-protection", "ability-score-improvement"}));
    for (const core::KnownSpell& sort : fiche.knownSpells) {
        EXPECT_EQ(sort.perDay, sort.level == 0 ? 0 : 2) << sort.spellId;
    }
    for (const char* id : {"mage-hand", "invisibility", "scorching-ray", "fireball", "fly"}) {
        EXPECT_NE(fiche.knownSpell(id), nullptr) << id;
    }
}

/**
 * @brief Le catalogue refuse un effet de sort inconnu, et charge sans le jouer une zone que le
 *        moteur ne pose pas.
 * \castest{<b>Un sort a l'effet « petrify » est refuse ; un sort en cone se charge, mais aucun
 * mecanisme ne le joue.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ecrire les deux sorts dans un dossier temporaire.<br/>2. Charger.<br/>
 * \tattendu Une erreur nommant petrifie.json ; le cone charge, spellMechanism vide.
 * }
 */
TEST(ClassMageTest, UnEffetInconnuEstRefuseEtUnConeNeSeJouePas) {
    const std::filesystem::path dossier =
        std::filesystem::temp_directory_path() / "jadg-lot-133-sorts";
    std::filesystem::create_directories(dossier);
    {
        std::ofstream(dossier / "petrifie.json")
            << R"({"id": "petrifie", "name": "P", "source": "original", "level": 1,
                  "school": "transmutation", "castingTime": "1 action", "range": "9 m",
                  "duration": "1 minute", "effect": {"kind": "petrify"}, "text": "essai"})";
        std::ofstream(dossier / "cone.json")
            << R"({"id": "cone", "name": "C", "source": "original", "level": 1,
                  "school": "evocation", "castingTime": "1 action", "range": "personnelle",
                  "duration": "instantanee", "savingThrow": "dexterity", "damage": "3d6",
                  "damageType": "fire", "area": {"shape": "cone", "meters": 4.5},
                  "text": "essai"})";
    }
    const core::SpellCatalog catalogue = core::loadSpells(dossier);
    std::filesystem::remove_all(dossier);
    ASSERT_EQ(catalogue.errors.size(), 1U);
    EXPECT_NE(catalogue.errors.front().find("petrifie.json"), std::string::npos);
    ASSERT_EQ(catalogue.spells.size(), 1U);
    EXPECT_FALSE(core::spellMechanism(catalogue.spells.front()).has_value());
}

/**
 * @brief La session annonce le debut et la fin d'un sort, avec son identifiant, pour que l'ecran
 *        joue le geste et l'effet (`LOT-136`).
 * \castest{<b>Trait de feu annonce son debut, puis son issue.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mage N1 contre un mannequin en (7, 3), un observateur d'actions branche.<br/>2.
 * Lancer trait de feu.<br/>
 * \tattendu Deux annonces : `Begin` puis `End`, du Mage vers le mannequin, sort `fire-bolt`, pas un
 * tir a l'arme ; `End` dit rate exactement quand le jet a manque.
 * }
 */
TEST(ClassMageTest, UnSortSAnnonceAuDebutEtALaFin) {
    const core::LoadedCharacterSheet charge = mage();
    core::ArenaSession session(test_support::room());
    ASSERT_TRUE(session.mount(combatDe(charge, {test_support::dummy("Mannequin", {7, 3}, 10, 0)}))
                    .refusals.empty());
    ASSERT_TRUE(session.start());
    std::vector<core::ArenaActionNotice> annonces;
    session.setActionObserver(
        [&annonces](const core::ArenaActionNotice& annonce) { annonces.push_back(annonce); });
    const core::ArenaAttack lancer =
        session.castSpell(CombatantId{2}, sortDe(session, "fire-bolt"));
    ASSERT_EQ(lancer.result, core::ArenaActionResult::Done);
    ASSERT_EQ(annonces.size(), 2U);
    EXPECT_EQ(annonces[0].phase, core::ArenaActionPhase::Begin);
    EXPECT_EQ(annonces[1].phase, core::ArenaActionPhase::End);
    for (const core::ArenaActionNotice& annonce : annonces) {
        EXPECT_EQ(annonce.actor, CombatantId{1});
        EXPECT_EQ(annonce.target, CombatantId{2});
        EXPECT_EQ(annonce.spell, "fire-bolt");
        EXPECT_FALSE(annonce.ranged);
    }
    ASSERT_TRUE(lancer.outcome.has_value());
    EXPECT_EQ(annonces[1].missed, !lancer.outcome->roll.hit);
}
