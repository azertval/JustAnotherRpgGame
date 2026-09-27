// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_class_in_arena.cpp
 * @brief La classe d'essai agit en combat (`LOT-131`, `EX-RPG-024`, `EX-RPG-025`, `EX-REG-003`) :
 *        bonus au jet et des en plus nommes au journal, resistance globale nommee, deplacement sans
 *        attaque d'opportunite, sort a deux lancers qui s'epuise et qu'un repos long rend.
 *
 * Rien ici n'est nomme par le moteur : la session ne branche que des effets nommes, et le journal
 * ecrit les noms que la donnee porte.
 */

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/ActionEconomy.h"
#include "Core/Combat/Arena.h"
#include "Core/Combat/Attack.h"
#include "Core/Combat/CombatCounters.h"
#include "Core/Levels/Level.h"
#include "Core/Levels/TileMap.h"
#include "Core/Rpg/Ability.h"
#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/CharacterSheet.h"

namespace {

using core::CombatantId;
using core::CombatSide;

const std::filesystem::path RACINE_ESSAI{JADG_TEST_DATA_DIR};
const std::filesystem::path REGLES{JADG_RPG_RULES_DIR};

struct Regles {
    core::CharacterCreationRules creation;
    core::ExperienceTable experience;
};

[[nodiscard]] const Regles& regles() {
    static const Regles lues{
        .creation = core::loadCharacterCreationRules(REGLES / "character-creation.json"),
        .experience = core::loadExperienceTable(REGLES / "experience.json")};
    return lues;
}

[[nodiscard]] const core::CharacterOptions& optionsDEssai() {
    static const core::CharacterOptions options = core::loadCharacterOptions(RACINE_ESSAI / "Rpg");
    return options;
}

/// Une salle 12x8 sans mur : les places sont donnees a la main.
[[nodiscard]] core::Level salle() {
    return core::Level(core::LevelData{
        .name = "salle", .tileMap = core::TileMap(12, 8), .entities = {}, .entry = {1, 1}});
}

/// La fiche d'essai au niveau @p niveau : For 16, Dex 14, Con 16, Int 14, vitesse 9 m, 20 PV pour
/// encaisser sans tomber.
[[nodiscard]] core::CharacterSheet lutteuse(int niveau) {
    const core::CharacterOptions& options = optionsDEssai();
    const core::PlayableClass* classe = options.findClass("lutteur-d-essai");
    EXPECT_NE(classe, nullptr);
    core::CharacterSheet fiche =
        core::buildCharacterSheet("Lutteuse", {16, 14, 16, 14, 10, 8}, nullptr, classe, nullptr,
                                  regles().creation, regles().experience);
    fiche.speedMeters = 9.0F;
    if (niveau > 1) {
        core::gainExperience(fiche, regles().experience, classe->hitDie,
                             regles().experience.thresholdAt(niveau));
    }
    std::vector<std::string> manquants;
    core::applyClassFeatures(fiche, *classe, options, regles().creation, manquants);
    EXPECT_TRUE(manquants.empty());
    fiche.maximumHitPoints = 40;
    fiche.currentHitPoints = 40;
    return fiche;
}

/// Une epee : +5 au toucher, 1d8+3 tranchants.
[[nodiscard]] core::AttackProfile epee() {
    core::AttackProfile coup;
    coup.label = "Epee";
    coup.modifiers = {{.source = "Force", .value = 3}, {.source = "maitrise", .value = 2}};
    coup.damage = {
        {.dice = *core::parseDice("1d8+3"), .type = core::DamageType::Slashing, .flags = 0}};
    return coup;
}

/// La lutteuse comme concurrente : profil de la fiche, epee, capacites et sorts de la fiche.
[[nodiscard]] core::ArenaContestant concurrente(const core::CharacterSheet& fiche,
                                                core::GridPosition place) {
    const core::CharacterOptions& options = optionsDEssai();
    const core::PlayableClass* classe = options.findClass(fiche.classId);
    std::vector<std::string> ignores;
    core::CombatantProfile profil = core::profileFor(fiche, CombatSide::Allies);
    // L'initiative est un jet : la lutteuse joue en premier, sans parier sur ses des.
    profil.initiativeModifier = 100;
    return {.profile = std::move(profil),
            .attacks = {epee()},
            .position = place,
            .markId = {},
            .behavior = {},
            .capacities = fiche.capacities,
            .spells = core::arenaSpellsFor(fiche, *classe, options.spells, 2, ignores)};
}

/// Un gobelin : @p ca de CA, @p bonus au toucher, 1d6+2 tranchants, 60 PV pour tenir.
[[nodiscard]] core::ArenaContestant gobelin(core::GridPosition place, int ca, int bonus) {
    core::CombatantProfile profil{.name = "Gobelin",
                                  .side = CombatSide::Enemies,
                                  .maximumHitPoints = 60,
                                  .currentHitPoints = 60,
                                  .dexterity = 10,
                                  .initiativeModifier = -100,
                                  .movement = 6};
    profil.armorClass = ca;
    core::AttackProfile coup;
    coup.label = "Cimeterre";
    coup.modifiers = {{.source = "bonus d'attaque", .value = bonus}};
    coup.damage = {
        {.dice = *core::parseDice("1d6+2"), .type = core::DamageType::Slashing, .flags = 0}};
    return {.profile = profil, .attacks = {coup}, .position = place, .markId = {}, .behavior = {}};
}

[[nodiscard]] bool contient(const std::vector<std::string>& journal, const std::string& morceau) {
    return std::ranges::any_of(journal, [&](const std::string& ligne) {
        return ligne.find(morceau) != std::string::npos;
    });
}

[[nodiscard]] std::string ligneContenant(const std::vector<std::string>& journal,
                                         const std::string& morceau) {
    const auto trouve = std::ranges::find_if(journal, [&](const std::string& ligne) {
        return ligne.find(morceau) != std::string::npos;
    });
    return trouve == journal.end() ? std::string{} : *trouve;
}

}  // namespace

/**
 * @brief Le bonus au jet et les des en plus d'une capacite jouent et se nomment ; une fois par
 * tour.
 * \castest{<b>Contre une CA nulle, l'epee de la lutteuse touche : le journal ecrit « + 2 (Coup
 * precis) » et un 1d6 « (Coup precis) » ; une seconde attaque du meme tour n'ajoute plus de des ;
 * au tour suivant, si.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Monter la lutteuse (N1) contre un gobelin a la CA 0, graine choisie pour que le
 * premier coup ne soit pas un 1 naturel.<br/>2. Attaquer.<br/>3. Octroyer une action et
 * rattaquer.<br/>4. Revenir a son tour et rattaquer.<br/>
 * \tattendu Premier coup : deux clauses de degats, la seconde nommee Coup precis, le jet porte le
 * modificateur nomme ; second coup : une seule clause s'il touche, le compteur du tour est a 1 ;
 * tour suivant : le compteur est retombe, les des reviennent.
 * }
 */
TEST(ClassInArenaTest, LeBonusEtLesDesDUneCapaciteSeJouentEtSeNomment) {
    const core::CharacterSheet fiche = lutteuse(1);
    core::ArenaSession session(salle());
    core::ArenaBout bout{.seed = 1, .lethal = true, .heroicMark = false};
    bout.contestants.push_back(concurrente(fiche, {3, 3}));
    bout.contestants.push_back(gobelin({4, 3}, 0, 4));

    // Un 1 naturel rate quelle que soit la CA : on prend la premiere graine qui n'en fait pas un.
    std::optional<core::AttackOutcome> premier;
    for (std::uint64_t graine = 1; graine < 40 && !premier.has_value(); ++graine) {
        bout.seed = graine;
        ASSERT_TRUE(session.mount(bout).refusals.empty());
        ASSERT_TRUE(session.start());
        ASSERT_EQ(session.combat().activeCombatant(), CombatantId{1});
        const core::ArenaAttack coup = session.attack(CombatantId{2});
        ASSERT_EQ(coup.result, core::ArenaActionResult::Done);
        if (coup.outcome->roll.hit) {
            premier = coup.outcome;
        }
    }
    ASSERT_TRUE(premier.has_value());
    EXPECT_TRUE(contient(session.journal(), "capacites Lutteuse : Coup precis, Peau de fer"));

    // Le jet porte le bonus, NOMME.
    const std::vector<core::Modifier>& modificateurs = premier->roll.check.modifiers;
    const auto bonus = std::ranges::find(modificateurs, "Coup precis", &core::Modifier::source);
    ASSERT_NE(bonus, modificateurs.end());
    EXPECT_EQ(bonus->value, 2);
    ASSERT_EQ(premier->damage.size(), 2U);
    EXPECT_TRUE(premier->damage[0].source.empty()) << "les des de l'epee";
    EXPECT_EQ(premier->damage[1].source, "Coup precis");
    EXPECT_EQ(premier->damage[1].clause.dice, (core::Dice{.count = 1, .faces = 6, .modifier = 0}));
    EXPECT_EQ(premier->damage[1].clause.type, core::DamageType::Slashing) << "du type de l'arme";
    const std::string ligne = ligneContenant(session.journal(), "attaque Lutteuse -> Gobelin");
    EXPECT_NE(ligne.find("+ 2 (Coup precis)"), std::string::npos) << ligne;
    EXPECT_NE(ligne.find("tranchant (Coup precis)"), std::string::npos) << ligne;
    EXPECT_EQ(session.combat().counters().value(core::CounterScope::Turn, "1", "coup-precis"), 1);

    // Une seconde attaque dans le meme tour : plus de des en plus.
    session.combat().economy(CombatantId{1})->grant(core::ACTION_RESOURCE);
    const core::ArenaAttack second = session.attack(CombatantId{2});
    ASSERT_EQ(second.result, core::ArenaActionResult::Done);
    if (second.outcome->roll.hit) {
        EXPECT_EQ(second.outcome->damage.size(), 1U) << "une fois par tour";
    }
    EXPECT_EQ(session.combat().counters().value(core::CounterScope::Turn, "1", "coup-precis"), 1);

    // Au tour suivant de la lutteuse, le compteur est retombe.
    ASSERT_TRUE(session.endTurn());
    for (int garde = 0; garde < 4 && session.combat().activeCombatant() != CombatantId{1};
         ++garde) {
        ASSERT_TRUE(session.endTurn());
    }
    ASSERT_EQ(session.combat().activeCombatant(), CombatantId{1});
    EXPECT_EQ(session.combat().counters().value(core::CounterScope::Turn, "1", "coup-precis"), 0);
    const core::ArenaAttack troisieme = session.attack(CombatantId{2});
    ASSERT_EQ(troisieme.result, core::ArenaActionResult::Done);
    if (troisieme.outcome->roll.hit) {
        EXPECT_EQ(troisieme.outcome->damage.size(), 2U);
    }
}

/**
 * @brief La resistance globale d'une capacite divise les degats et se nomme au journal.
 * \castest{<b>Le gobelin touche la lutteuse (Peau de fer) : la trace ecrit « resistance (tranchant
 * ; Peau de fer) », et les PV perdus sont la moitie arrondie a l'inferieur des degats
 * lances.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Monter la lutteuse (N1) et un gobelin a +20 au toucher.<br/>2. Passer au tour du
 * gobelin, attaquer la lutteuse ; graine choisie pour toucher.<br/>
 * \tattendu Le profil de la lutteuse porte treize resistances nommees Peau de fer ; la ligne du
 * journal nomme la capacite ; PV perdus = degats / 2.
 * }
 */
TEST(ClassInArenaTest, LaResistanceGlobaleSeNommeAuJournal) {
    const core::CharacterSheet fiche = lutteuse(1);
    core::ArenaSession session(salle());
    core::ArenaBout bout{.seed = 1, .lethal = true, .heroicMark = false};
    bout.contestants.push_back(concurrente(fiche, {3, 3}));
    bout.contestants.push_back(gobelin({4, 3}, 15, 20));

    const core::CombatantProfile& profil = bout.contestants[0].profile;
    EXPECT_EQ(profil.damageTraits.affinities.size(), 13U) << "une par type de degats";
    EXPECT_TRUE(profil.damageTraits.applies(core::DamageAffinityKind::Resistance,
                                            core::DamageType::Fire, 0));
    EXPECT_EQ(profil.damageTraits.affinities.front().source, "Peau de fer");
    EXPECT_EQ(profil.armorClass, 15) << "10 + Dex 2 + Con 3 : la CA de la fiche suit la capacite";

    std::optional<core::AttackOutcome> coup;
    for (std::uint64_t graine = 1; graine < 40 && !coup.has_value(); ++graine) {
        bout.seed = graine;
        ASSERT_TRUE(session.mount(bout).refusals.empty());
        ASSERT_TRUE(session.start());
        ASSERT_TRUE(session.endTurn());
        ASSERT_EQ(session.combat().activeCombatant(), CombatantId{2});
        const core::ArenaAttack attaque = session.attack(CombatantId{1});
        ASSERT_EQ(attaque.result, core::ArenaActionResult::Done);
        if (attaque.outcome->roll.hit) {
            coup = attaque.outcome;
        }
    }
    ASSERT_TRUE(coup.has_value());
    ASSERT_TRUE(coup->report.has_value());
    const int lances = coup->damage.front().amount;
    EXPECT_EQ(coup->report->hitPointsBefore - coup->report->hitPointsAfter, lances / 2);
    const std::string ligne = ligneContenant(session.journal(), "attaque Gobelin -> Lutteuse");
    EXPECT_NE(ligne.find("resistance (tranchant ; Peau de fer)"), std::string::npos) << ligne;
}

/**
 * @brief Une capacite soustrait aux attaques d'opportunite, et le journal le dit.
 * \castest{<b>La lutteuse (N2, Pas de danseur) quitte l'allonge du gobelin sans etre frappee, la
 * previsualisation ne montre personne, et le journal nomme la capacite ; sans elle, le gobelin
 * frappe.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lutteuse N2 au contact du gobelin ; previsualiser puis marcher a quatre cases.<br/>2.
 * Meme parcours avec la lutteuse N1, qui n'a pas la capacite.<br/>
 * \tattendu N2 : zero opportuniste, aucune ligne « opportunite : », une ligne « sans attaque
 * d'opportunite Lutteuse (Pas de danseur) », budget de 8 cases (9 m + 3 m) ; N1 : une ligne
 * « opportunite : attaque Gobelin -> Lutteuse », budget de 6 cases.
 * }
 */
TEST(ClassInArenaTest, LeDeplacementNeProvoquePasDAttaqueDOpportunite) {
    {
        const core::CharacterSheet fiche = lutteuse(2);
        core::ArenaSession session(salle());
        core::ArenaBout bout{.seed = 7, .lethal = true, .heroicMark = false};
        bout.contestants.push_back(concurrente(fiche, {3, 3}));
        bout.contestants.push_back(gobelin({4, 3}, 15, 4));
        ASSERT_TRUE(session.mount(bout).refusals.empty());
        ASSERT_TRUE(session.start());
        ASSERT_EQ(session.combat().activeCombatant(), CombatantId{1});
        EXPECT_EQ(session.combat().find(CombatantId{1})->profile.movement, 8) << "9 m + 3 m";

        EXPECT_TRUE(session.previewOpportunities({3, 7}).empty());
        const core::MoveOutcome parcours = session.move({3, 7});
        EXPECT_EQ(parcours.result, core::MoveResult::Moved);
        EXPECT_EQ(session.combat().grid().positionOf(CombatantId{1}), (core::GridPosition{3, 7}));
        EXPECT_FALSE(contient(session.journal(), "opportunite : "));
        EXPECT_TRUE(
            contient(session.journal(), "sans attaque d'opportunite Lutteuse (Pas de danseur)"));
    }
    {
        const core::CharacterSheet fiche = lutteuse(1);
        core::ArenaSession session(salle());
        core::ArenaBout bout{.seed = 7, .lethal = true, .heroicMark = false};
        bout.contestants.push_back(concurrente(fiche, {3, 3}));
        bout.contestants.push_back(gobelin({4, 3}, 15, 4));
        ASSERT_TRUE(session.mount(bout).refusals.empty());
        ASSERT_TRUE(session.start());
        EXPECT_EQ(session.combat().find(CombatantId{1})->profile.movement, 6);
        EXPECT_EQ(session.previewOpportunities({3, 7}), (std::vector<CombatantId>{CombatantId{2}}));
        static_cast<void>(session.move({3, 7}));
        EXPECT_TRUE(contient(session.journal(), "opportunite : attaque Gobelin -> Lutteuse"));
        EXPECT_FALSE(contient(session.journal(), "sans attaque d'opportunite"));
    }
}

/**
 * @brief Un sort a deux lancers s'epuise et ne se propose plus ; un repos long le rend.
 * \castest{<b>La lutteuse lance son trait deux fois, la troisieme est refusee « Exhausted » sans
 * rien depenser ; le sort mineur se lance encore ; le repos long de la fiche rend les deux lancers
 * au montage suivant.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire les sorts de la lutteuse dans la session.<br/>2. Lancer le trait, octroyer
 * une action, relancer, octroyer, relancer.<br/>3. Lancer le sort mineur.<br/>4. Depenser les deux
 * lancers sur la fiche, la reposer, remonter.<br/>
 * \tattendu Deux sorts : etincelle a volonte (-1), trait a 2 ; le trait est une attaque a distance
 * a + 2 (Intelligence) + 2 (maitrise), 2d6 de feu, portee 24 cases ; apres deux lancers : 0, la
 * troisieme refusee et l'action gardee ; le journal prefixe « sort Trait de feu d'essai (1 restant)
 * : » ; le sort mineur passe ; apres le repos, 2 lancers au montage.
 * }
 */
TEST(ClassInArenaTest, UnSortEpuiseNeSeProposePlusEtUnReposLongLeRend) {
    core::CharacterSheet fiche = lutteuse(1);
    core::ArenaSession session(salle());
    core::ArenaBout bout{.seed = 3, .lethal = true, .heroicMark = false};
    bout.contestants.push_back(concurrente(fiche, {3, 3}));
    bout.contestants.push_back(gobelin({5, 3}, 12, 4));
    ASSERT_TRUE(session.mount(bout).refusals.empty());
    ASSERT_TRUE(session.start());
    ASSERT_EQ(session.combat().activeCombatant(), CombatantId{1});

    const std::vector<core::ArenaSpell>* sorts = session.spells(CombatantId{1});
    ASSERT_NE(sorts, nullptr);
    ASSERT_EQ(sorts->size(), 2U);
    EXPECT_EQ((*sorts)[0].id, "etincelle-d-essai");
    EXPECT_EQ((*sorts)[0].uses, -1) << "a volonte";
    EXPECT_EQ((*sorts)[1].id, "trait-de-feu-d-essai");
    EXPECT_EQ((*sorts)[1].uses, 2);
    const core::AttackProfile& trait = (*sorts)[1].attack;
    EXPECT_EQ(trait.kind, core::AttackKind::Ranged);
    ASSERT_EQ(trait.modifiers.size(), 2U);
    EXPECT_EQ(trait.modifiers[0].source, "Intelligence");
    EXPECT_EQ(trait.modifiers[0].value, 2);
    EXPECT_EQ(trait.modifiers[1].source, "maitrise");
    EXPECT_EQ(trait.modifiers[1].value, 2);
    ASSERT_EQ(trait.damage.size(), 1U);
    EXPECT_EQ(trait.damage[0].dice, (core::Dice{.count = 2, .faces = 6, .modifier = 0}));
    EXPECT_EQ(trait.damage[0].type, core::DamageType::Fire);
    EXPECT_TRUE(core::hasFlag(trait.damage[0].flags, core::DamageFlag::Spell));
    ASSERT_TRUE(trait.range.has_value());
    EXPECT_EQ(trait.range->normal, 24) << "36 m";
    EXPECT_EQ(trait.range->maximum, 24);

    EXPECT_EQ(session.castSpell(CombatantId{2}, 1).result, core::ArenaActionResult::Done);
    EXPECT_EQ((*sorts)[1].uses, 1);
    EXPECT_TRUE(contient(session.journal(),
                         "sort Trait de feu d'essai (1 restant) : attaque Lutteuse -> Gobelin "
                         "(Trait de feu d'essai)"));
    EXPECT_EQ(session.castSpell(CombatantId{2}, 1).result, core::ArenaActionResult::NoAction)
        << "l'action du tour est depensee";
    session.combat().economy(CombatantId{1})->grant(core::ACTION_RESOURCE);
    EXPECT_EQ(session.castSpell(CombatantId{2}, 1).result, core::ArenaActionResult::Done);
    EXPECT_EQ((*sorts)[1].uses, 0);
    EXPECT_FALSE((*sorts)[1].available());
    session.combat().economy(CombatantId{1})->grant(core::ACTION_RESOURCE);
    EXPECT_EQ(session.castSpell(CombatantId{2}, 1).result, core::ArenaActionResult::Exhausted);
    EXPECT_EQ(session.combat().find(CombatantId{1})->economy.remaining(core::ACTION_RESOURCE), 1)
        << "un refus ne depense rien";
    EXPECT_EQ(session.castSpell(CombatantId{2}, 0).result, core::ArenaActionResult::Done)
        << "le sort mineur, a volonte";
    EXPECT_EQ((*sorts)[0].uses, -1);
    EXPECT_TRUE(
        contient(session.journal(), "sort Etincelle d'essai : attaque Lutteuse -> Gobelin"));
    EXPECT_EQ(session.castSpell(CombatantId{2}, 5).result, core::ArenaActionResult::NoSpell);

    // La journee est celle de la fiche : deux lancers depenses, un repos long, et le montage
    // suivant retrouve les deux.
    EXPECT_TRUE(core::spendSpellUse(fiche, "trait-de-feu-d-essai"));
    EXPECT_TRUE(core::spendSpellUse(fiche, "trait-de-feu-d-essai"));
    std::vector<std::string> ignores;
    const core::PlayableClass* classe = optionsDEssai().findClass(fiche.classId);
    EXPECT_EQ(core::arenaSpellsFor(fiche, *classe, optionsDEssai().spells, 2, ignores)[1].uses, 0);
    core::longRest(fiche);
    EXPECT_EQ(core::arenaSpellsFor(fiche, *classe, optionsDEssai().spells, 2, ignores)[1].uses, 2);
    EXPECT_TRUE(ignores.empty());
}
