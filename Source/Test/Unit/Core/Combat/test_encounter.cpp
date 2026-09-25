// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_encounter.cpp
 * @brief Tests de l'aller-retour exploration ↔ combat (`LOT-18`, `EX-CBT-001`).
 *
 * **Aucune fenêtre, aucun GPU** : c'est un critère d'acceptation du lot, et c'est ce qui permet de
 * vérifier qu'un aller-retour restitue l'état sans avoir à le regarder à l'écran. Le montage et le
 * démontage d'une rencontre sont de la logique pure ; le mode de jeu, lui, ne fait qu'ordonner des
 * passes.
 */

#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "Core/Combat/CombatTransition.h"
#include "Core/Combat/Encounter.h"
#include "Core/Gameplay/WorldFlags.h"

namespace {

/// Une rencontre écrite ici : un test qui lit la donnée livrée vérifie deux choses à la fois, et
/// échoue pour la mauvaise raison le jour où la donnée bouge.
[[nodiscard]] core::Encounter rencontre(bool fuyable = true) {
    core::Encounter combat;
    combat.id = "essai";
    combat.name = "Rencontre d'essai";
    combat.escapable = fuyable;
    combat.combatants = {
        {.creatureId = "giant-rat", .columnOffset = 0, .rowOffset = -1},
        {.creatureId = "rat", .columnOffset = 2, .rowOffset = 1},
    };
    return combat;
}

/// Un état d'exploration reconnaissable : aucune valeur ronde, pour qu'une restitution
/// approximative se voie.
[[nodiscard]] core::ExplorationSnapshot exploration() {
    return {.playerPosition = {37.5F, -12.25F},
            .playerFacing = {0.0F, -1.0F},
            .cameraPosition = {30.0F, -8.5F},
            .captured = true};
}

}  // namespace

/**
 * @brief Un aller-retour restitue **exactement** l'état d'exploration.
 * \castest{<b>Un aller-retour exploration -> combat -> exploration restitue l'etat.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Relever un etat d'exploration.<br/>2. Engager une rencontre.<br/>3. La terminer par
 * une victoire.<br/>
 * \tattendu Position, orientation et camera sont rendues a l'identique.
 * }
 */
TEST(EncounterTest, UnAllerRetourRestitueLEtatDExploration) {
    core::WorldFlags drapeaux;
    const core::ExplorationSnapshot avant = exploration();
    const core::EncounterRun engagee =
        core::beginEncounter(rencontre(), avant, {.column = 10, .row = 4}, "carte/rat/10/4");

    const core::ExplorationSnapshot apres =
        core::endEncounter(engagee, core::CombatOutcome::Victory, drapeaux);

    EXPECT_TRUE(apres.captured);
    EXPECT_FLOAT_EQ(apres.playerPosition.x, avant.playerPosition.x);
    EXPECT_FLOAT_EQ(apres.playerPosition.y, avant.playerPosition.y);
    EXPECT_FLOAT_EQ(apres.playerFacing.x, avant.playerFacing.x);
    EXPECT_FLOAT_EQ(apres.playerFacing.y, avant.playerFacing.y);
    EXPECT_FLOAT_EQ(apres.cameraPosition.x, avant.cameraPosition.x);
    EXPECT_FLOAT_EQ(apres.cameraPosition.y, avant.cameraPosition.y);
}

/**
 * @brief Un ennemi vaincu ne réapparaît pas : la victoire acquiert un **drapeau de monde**.
 * \castest{<b>Un ennemi vaincu ne reapparait pas.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Gagner une rencontre portant une cle de drapeau.<br/>2. Interroger les drapeaux.<br/>
 * \tattendu Le drapeau est acquis, et la rencontre se declare deja nettoyee.
 * }
 */
TEST(EncounterTest, UnEnnemiVaincuNeReapparaitPas) {
    core::WorldFlags drapeaux;
    const std::string cle = "grotte/enemy/12/7";
    const core::EncounterRun engagee =
        core::beginEncounter(rencontre(), exploration(), {.column = 12, .row = 7}, cle);

    EXPECT_FALSE(core::encounterAlreadyCleared(drapeaux, cle)) << "rien n'est acquis avant";
    static_cast<void>(core::endEncounter(engagee, core::CombatOutcome::Victory, drapeaux));

    EXPECT_TRUE(drapeaux.isSet(cle));
    // C'est ce qui le distingue d'un booleen porte par l'entite : le drapeau survit a la
    // destruction et a la recreation de l'entite au rechargement de la carte.
    EXPECT_TRUE(core::encounterAlreadyCleared(drapeaux, cle));
}

/**
 * @brief Une fuite ramène à l'exploration **sans** marquer l'ennemi vaincu.
 * \castest{<b>Une fuite ne marque pas l'ennemi vaincu.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Engager une rencontre portant une cle de drapeau.<br/>2. La terminer par une fuite,
 * puis par une defaite.<br/>
 * \tattendu Le drapeau n'est acquis dans aucun des deux cas, et l'etat est tout de meme restitue.
 * }
 */
TEST(EncounterTest, UneFuiteNeMarquePasLEnnemiVaincu) {
    core::WorldFlags drapeaux;
    const std::string cle = "grotte/enemy/3/9";
    const core::EncounterRun engagee =
        core::beginEncounter(rencontre(), exploration(), {.column = 3, .row = 9}, cle);

    const core::ExplorationSnapshot apresFuite =
        core::endEncounter(engagee, core::CombatOutcome::Flight, drapeaux);
    EXPECT_FALSE(drapeaux.isSet(cle)) << "fuir suffirait sinon a nettoyer une carte";
    EXPECT_TRUE(apresFuite.captured) << "on revient tout de meme a l'exploration";

    static_cast<void>(core::endEncounter(engagee, core::CombatOutcome::Defeat, drapeaux));
    EXPECT_FALSE(drapeaux.isSet(cle)) << "le groupe est tombe, l'ennemi est toujours la";
}

/**
 * @brief Une rencontre **sans clé** se redéclenche : c'est le cas d'une zone de rencontre.
 * \castest{<b>Une rencontre sans cle de drapeau se redeclenche.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Gagner une rencontre sans cle de drapeau.<br/>
 * \tattendu Aucun drapeau n'est pose, et la rencontre ne se declare pas nettoyee.
 * }
 */
TEST(EncounterTest, UneRencontreSansCleSeRedeclenche) {
    core::WorldFlags drapeaux;
    const core::EncounterRun zone =
        core::beginEncounter(rencontre(), exploration(), {.column = 1, .row = 1}, "");

    static_cast<void>(core::endEncounter(zone, core::CombatOutcome::Victory, drapeaux));
    EXPECT_FALSE(core::encounterAlreadyCleared(drapeaux, ""))
        << "une cle vide n'est jamais acquise : sinon toutes les zones du jeu s'eteindraient "
           "au premier combat gagne";
}

/**
 * @brief Une victoire pose le fait de la rencontre gagnée, clé d'entité ou non : c'est ce qu'une
 *        quête lit quand le combat a été engagé par un dialogue (`LOT-120`).
 * \castest{<b>Une victoire pose le fait encounter/&lt;id&gt;/won, une fuite non.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Gagner une rencontre sans cle d'entite.<br/>2. En fuir une autre.<br/>
 * \tattendu Le fait `encounter/&lt;id&gt;/won` est pose apres la victoire seulement.
 * }
 */
TEST(EncounterTest, UneVictoirePoseLeFaitDeLaRencontreGagnee) {
    core::WorldFlags drapeaux;
    const core::Encounter modele = rencontre();
    const std::string fait = core::encounterWonFlag(modele.id);
    EXPECT_EQ(fait, "encounter/" + modele.id + "/won");

    const core::EncounterRun fuie =
        core::beginEncounter(modele, exploration(), {.column = 1, .row = 1}, "");
    static_cast<void>(core::endEncounter(fuie, core::CombatOutcome::Flight, drapeaux));
    EXPECT_FALSE(drapeaux.isSet(fait)) << "fuir n'est pas gagner";

    const core::EncounterRun gagnee =
        core::beginEncounter(modele, exploration(), {.column = 1, .row = 1}, "");
    static_cast<void>(core::endEncounter(gagnee, core::CombatOutcome::Victory, drapeaux));
    EXPECT_TRUE(drapeaux.isSet(fait));
}

/**
 * @brief Les combattants se placent **relativement** au déclencheur.
 * \castest{<b>Les combattants se placent relativement au declencheur.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Placer la meme rencontre a deux endroits differents.<br/>
 * \tattendu La formation est conservee, decalee de l'ecart entre les deux declencheurs.
 * }
 */
TEST(EncounterTest, LesCombattantsSePlacentRelativementAuDeclencheur) {
    const core::Encounter combat = rencontre();
    const std::vector<core::CombatantPlacement> ici =
        core::placeCombatants(combat, {.column = 10, .row = 10});
    const std::vector<core::CombatantPlacement> ailleurs =
        core::placeCombatants(combat, {.column = 40, .row = 2});

    ASSERT_EQ(ici.size(), 2U);
    EXPECT_EQ(ici[0].position.column, 10);
    EXPECT_EQ(ici[0].position.row, 9);
    EXPECT_EQ(ici[1].position.column, 12);
    EXPECT_EQ(ici[1].position.row, 11);

    ASSERT_EQ(ailleurs.size(), 2U);
    for (std::size_t rang = 0; rang < ici.size(); ++rang) {
        EXPECT_EQ(ailleurs[rang].creatureId, ici[rang].creatureId);
        EXPECT_EQ(ailleurs[rang].position.column - ici[rang].position.column, 30);
        EXPECT_EQ(ailleurs[rang].position.row - ici[rang].position.row, -8);
    }
}

/**
 * @brief Une rencontre non fuyable le déclare, et l'engagement le retient.
 * \castest{<b>Une rencontre non fuyable le declare.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Engager une rencontre declaree non fuyable.<br/>
 * \tattendu L'engagement porte l'interdiction de fuir.
 * }
 */
TEST(EncounterTest, UneRencontreNonFuyableLeDeclare) {
    core::WorldFlags drapeaux;
    static_cast<void>(drapeaux);
    EXPECT_TRUE(core::beginEncounter(rencontre(true), exploration(), {}, "").escapable);
    EXPECT_FALSE(core::beginEncounter(rencontre(false), exploration(), {}, "").escapable);
}

/**
 * @brief Le catalogue LIVRÉ se charge, et chaque rencontre porte au moins un combattant.
 * \castest{<b>Le catalogue de rencontres livre se charge.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Charger Source/Elements/Rpg/encounters.<br/>
 * \tattendu Le catalogue est non vide, sans erreur, et chaque rencontre porte un identifiant et au
 * moins un combattant.
 * }
 */
TEST(EncounterTest, LeCatalogueLivreSeCharge) {
    const core::EncounterCatalog catalogue =
        core::loadEncounters(std::filesystem::path(JADG_RPG_DIR) / "encounters");
    EXPECT_TRUE(catalogue.errors.empty())
        << (catalogue.errors.empty() ? "" : catalogue.errors.front());
    ASSERT_FALSE(catalogue.encounters.empty());
    for (const core::Encounter& combat : catalogue.encounters) {
        EXPECT_FALSE(combat.id.empty());
        EXPECT_FALSE(combat.name.empty()) << combat.id;
        EXPECT_FALSE(combat.combatants.empty()) << combat.id;
        for (const core::EncounterCombatant& combattant : combat.combatants) {
            EXPECT_FALSE(combattant.creatureId.empty()) << combat.id;
        }
    }
    EXPECT_NE(catalogue.find("colisee-fauves"), nullptr);
    EXPECT_EQ(catalogue.find("rencontre-qui-n-existe-pas"), nullptr);
}

/**
 * @brief Un instantané **non relevé** se distingue d'un instantané à l'origine.
 * \castest{<b>Un instantane non releve se distingue de l'origine.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Construire un instantane par defaut.<br/>
 * \tattendu Il se declare non releve, ce qui interdit de le restituer.
 * }
 */
TEST(EncounterTest, UnInstantaneNonReleveSeDistingueDeLOrigine) {
    const core::ExplorationSnapshot vide;
    EXPECT_FALSE(vide.captured)
        << "restituer un instantane vide replacerait le personnage a l'origine de la carte, "
           "ce qui ressemble a une teleportation et non a une erreur";
}

/**
 * @brief Un ennemi **posé** sur la carte porte une clé de drapeau ; une **zone** n'en porte pas.
 * \castest{<b>Un ennemi pose porte une cle, une zone n'en porte pas.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire une entite de rencontre posee.<br/>2. Lire la meme avec respawns.<br/>
 * \tattendu La premiere porte une cle fabriquee, la seconde aucune.
 * }
 */
TEST(EncounterTest, UnEnnemiPosePorteUneCleUneZoneNonN) {
    core::MapEntity pose;
    pose.type = "encounter";
    pose.position = {.column = 5, .row = 8};
    pose.properties["encounterId"] = std::string{"colisee-fauves"};

    const auto declencheur = core::encounterTriggerFor(pose, "grotte");
    ASSERT_TRUE(declencheur.has_value());
    EXPECT_EQ(declencheur->encounterId, "colisee-fauves");
    EXPECT_EQ(declencheur->position.column, 5);
    EXPECT_FALSE(declencheur->defeatFlagKey.empty())
        << "sans cle, l'ennemi reapparaitrait a chaque passage";

    core::MapEntity zone = pose;
    zone.properties["respawns"] = true;
    const auto declencheurZone = core::encounterTriggerFor(zone, "grotte");
    ASSERT_TRUE(declencheurZone.has_value());
    EXPECT_TRUE(declencheurZone->defeatFlagKey.empty())
        << "avec une cle, la zone s'eteindrait au premier combat gagne";
}

/**
 * @brief Deux ennemis d'une même carte, et le même ennemi sur deux cartes, ne partagent jamais
 *        leur clé.
 * \castest{<b>Deux declencheurs ne partagent jamais leur cle.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire deux entites de rencontre a des cases differentes de la meme carte.<br/>
 * 2. Lire la meme case sur deux cartes differentes.<br/>
 * \tattendu Les cles different dans les deux cas.
 * }
 */
TEST(EncounterTest, DeuxDeclencheursNePartagentJamaisLeurCle) {
    core::MapEntity premier;
    premier.type = "encounter";
    premier.position = {.column = 1, .row = 1};
    premier.properties["encounterId"] = std::string{"colisee-fauves"};
    core::MapEntity second = premier;
    second.position = {.column = 2, .row = 1};

    const auto a = core::encounterTriggerFor(premier, "grotte");
    const auto b = core::encounterTriggerFor(second, "grotte");
    const auto ailleurs = core::encounterTriggerFor(premier, "foret");
    ASSERT_TRUE(a && b && ailleurs);
    EXPECT_NE(a->defeatFlagKey, b->defeatFlagKey) << "deux cases de la meme carte";
    EXPECT_NE(a->defeatFlagKey, ailleurs->defeatFlagKey) << "la meme case sur deux cartes";
}

/**
 * @brief Ce qui n'est pas un déclencheur n'en devient pas un.
 * \castest{<b>Ce qui n'est pas un declencheur n'en devient pas un.</b><br/>
 * \tcat Unitaire · Combat<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire une entite d'un autre type.<br/>2. Lire une entite de rencontre sans
 * encounterId.<br/>
 * \tattendu Aucun declencheur dans les deux cas.
 * }
 */
TEST(EncounterTest, CeQuiNEstPasUnDeclencheurNEnDevientPasUn) {
    core::MapEntity coffre;
    coffre.type = "chest";
    coffre.properties["encounterId"] = std::string{"colisee-fauves"};
    EXPECT_FALSE(core::encounterTriggerFor(coffre, "grotte").has_value());

    core::MapEntity sansRencontre;
    sansRencontre.type = "encounter";
    EXPECT_FALSE(core::encounterTriggerFor(sansRencontre, "grotte").has_value())
        << "une rencontre vide se terminerait aussitot par une victoire";
}
