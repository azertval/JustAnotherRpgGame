// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_party_ledger.cpp
 * @brief Tests du registre du groupe (`LOT-139`, `EX-CBT-062`) : ce que le combat laisse aux
 *        fiches se retient, s'applique borné, et s'oublie.
 */

#include <gtest/gtest.h>

#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/PartyLedger.h"

namespace {

[[nodiscard]] core::CharacterSheet fiche() {
    core::CharacterSheet sheet;
    sheet.name = "Helga";
    sheet.maximumHitPoints = 12;
    sheet.currentHitPoints = 12;
    sheet.knownSpells = {
        core::KnownSpell{.spellId = "soin-des-blessures", .level = 1, .perDay = 2, .remaining = 2},
        core::KnownSpell{.spellId = "flamme-sacree", .level = 0, .perDay = 0, .remaining = 0}};
    return sheet;
}

}  // namespace

/**
 * @brief Un enregistrement s'applique à la fiche, borné : les points de vie dans `[0, maximum]`,
 *        les lancers dans `[0, perDay]` ; un sort à volonté et un sort inconnu ne bougent pas.
 * \castest{<b>Le registre applique ce qu'il retient, sans depasser la fiche.</b><br/>
 * \tcat Unitaire · Groupe<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ecrire 7 PV et un lancer restant de soin.<br/>2. Appliquer.<br/>3. Ecrire 40 PV et
 * 9 lancers, appliquer.<br/>
 * \tattendu 7 / 12 et 1 lancer ; puis 12 / 12 et 2 lancers ; la flamme sacree reste a volonte.
 * }
 */
TEST(PartyLedgerTest, UnEnregistrementSAppliqueBorne) {
    core::PartyLedger registre;
    EXPECT_TRUE(registre.empty());
    EXPECT_EQ(registre.record("helga"), nullptr);

    core::MemberRecord blessee;
    blessee.hitPoints = 7;
    blessee.spellUses = {{"soin-des-blessures", 1}, {"sort-inconnu", 3}};
    registre.write("helga", blessee);
    ASSERT_NE(registre.record("helga"), nullptr);

    core::CharacterSheet sheet = fiche();
    core::applyRecord(sheet, *registre.record("helga"));
    EXPECT_EQ(sheet.currentHitPoints, 7);
    EXPECT_EQ(sheet.knownSpells[0].remaining, 1);
    EXPECT_EQ(sheet.knownSpells[1].remaining, 0);
    EXPECT_EQ(sheet.knownSpells.size(), 2U) << "le sort inconnu n'entre pas dans la fiche";

    core::MemberRecord excessif;
    excessif.hitPoints = 40;
    excessif.spellUses = {{"soin-des-blessures", 9}, {"flamme-sacree", 5}};
    registre.write("helga", excessif);
    sheet = fiche();
    core::applyRecord(sheet, *registre.record("helga"));
    EXPECT_EQ(sheet.currentHitPoints, 12);
    EXPECT_EQ(sheet.knownSpells[0].remaining, 2);
    EXPECT_EQ(sheet.knownSpells[1].remaining, 0) << "un sort a volonte ne se compte pas";
}

/**
 * @brief Un enregistrement sans points de vie laisse ceux de la fiche ; oublier un membre ou tout
 *        le registre rend les fiches pleines.
 * \castest{<b>Le registre s'oublie : un membre, ou tout.</b><br/>
 * \tcat Unitaire · Groupe<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ecrire deux membres, l'un sans points de vie.<br/>2. Effacer l'un, puis
 * tout.<br/>
 * \tattendu Le membre sans points de vie garde 12 / 12 ; efface, il n'a plus d'enregistrement ;
 * vide, le registre n'a plus rien.
 * }
 */
TEST(PartyLedgerTest, LeRegistreSOublie) {
    core::PartyLedger registre;
    core::MemberRecord sorts;
    sorts.spellUses = {{"soin-des-blessures", 0}};
    registre.write("helga", sorts);
    core::MemberRecord blesse;
    blesse.hitPoints = 3;
    registre.write("grom", blesse);

    core::CharacterSheet sheet = fiche();
    core::applyRecord(sheet, *registre.record("helga"));
    EXPECT_EQ(sheet.currentHitPoints, 12);
    EXPECT_EQ(sheet.knownSpells[0].remaining, 0);

    registre.erase("helga");
    EXPECT_EQ(registre.record("helga"), nullptr);
    EXPECT_NE(registre.record("grom"), nullptr);
    registre.erase("personne");
    registre.clear();
    EXPECT_TRUE(registre.empty());
}
