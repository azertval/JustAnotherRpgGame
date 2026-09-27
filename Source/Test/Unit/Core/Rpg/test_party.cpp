// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_party.cpp
 * @brief Tests du groupe du joueur (LOT-138) : composition, meneur, ordre de marche, et les
 *        quatre fiches pre-tirees qu'on peut y prendre.
 */

#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Rpg/Party.h"

#ifndef JADG_RPG_DIR
#error "JADG_RPG_DIR doit etre defini par CMake"
#endif

namespace {

using core::Party;
using core::PartyChange;

const std::vector<std::string> QUATRE = {"heros-brawler", "heros-mage", "heros-priest",
                                         "heros-scoundrel"};

}  // namespace

/**
 * @brief Quatre au plus, sans doublon, jamais vide (EX-EXP-013).
 * \castest{<b>Le groupe prend quatre personnages au plus, chacun une fois, et garde toujours son
 * dernier membre.</b><br/>
 * \tcat Unitaire · Groupe<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ajouter les quatre fiches, puis une cinquieme, puis une deja presente.<br/>
 * 2. Retirer trois membres, puis le dernier.<br/>
 * \tattendu Quatre membres ; la cinquieme refusee (`Full`), le doublon aussi (`AlreadyMember`) ;
 * le dernier membre ne se retire pas (`LastMember`).
 * }
 */
TEST(PartyTest, QuatreAuPlusJamaisVide) {
    Party groupe;
    for (const std::string& id : QUATRE) {
        EXPECT_EQ(groupe.add(id), PartyChange::Done);
    }
    EXPECT_EQ(groupe.size(), Party::MAX_MEMBERS);
    EXPECT_EQ(groupe.add("heros-cinquieme"), PartyChange::Full);
    EXPECT_EQ(groupe.add("heros-mage"), PartyChange::AlreadyMember);

    EXPECT_EQ(groupe.remove("heros-mage"), PartyChange::Done);
    EXPECT_EQ(groupe.remove("heros-mage"), PartyChange::NotMember);
    EXPECT_EQ(groupe.remove("heros-priest"), PartyChange::Done);
    EXPECT_EQ(groupe.remove("heros-brawler"), PartyChange::Done);
    EXPECT_EQ(groupe.leader(), "heros-scoundrel") << "le meneur retire laisse la tete au suivant";
    EXPECT_EQ(groupe.remove("heros-scoundrel"), PartyChange::LastMember);
    EXPECT_EQ(groupe.size(), 1U);
}

/**
 * @brief Le meneur est le premier de l'ordre de marche (EX-EXP-014).
 * \castest{<b>Choisir un meneur le met en tete sans deranger les autres ; passer la main fait le
 * tour du groupe.</b><br/>
 * \tcat Unitaire · Groupe<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Un groupe de quatre, Brawler en tete ; faire du Priest le meneur.<br/>
 * 2. Passer la main quatre fois.<br/>
 * 3. Echanger les rangs 1 et 3.<br/>
 * \tattendu Priest, Brawler, Mage, Scoundrel ; chaque passage met le suivant en tete et le meneur
 * en queue, et le quatrieme revient au Priest ; l'echange croise deux suiveurs.
 * }
 */
TEST(PartyTest, LeMeneurEstLePremierDeLOrdreDeMarche) {
    Party groupe{QUATRE};
    ASSERT_EQ(groupe.leader(), "heros-brawler");

    EXPECT_EQ(groupe.setLeader("heros-priest"), PartyChange::Done);
    EXPECT_EQ(groupe.members(), (std::vector<std::string>{"heros-priest", "heros-brawler",
                                                          "heros-mage", "heros-scoundrel"}));
    EXPECT_EQ(groupe.setLeader("heros-inconnu"), PartyChange::NotMember);

    std::vector<std::string> meneurs;
    for (int passe = 0; passe < 4; ++passe) {
        EXPECT_EQ(groupe.rotateLeader(), PartyChange::Done);
        meneurs.emplace_back(groupe.leader());
    }
    EXPECT_EQ(meneurs, (std::vector<std::string>{"heros-brawler", "heros-mage", "heros-scoundrel",
                                                 "heros-priest"}));

    EXPECT_EQ(groupe.swap(1, 3), PartyChange::Done);
    EXPECT_EQ(groupe.members(), (std::vector<std::string>{"heros-priest", "heros-scoundrel",
                                                          "heros-mage", "heros-brawler"}));
    EXPECT_EQ(groupe.swap(1, 4), PartyChange::NotMember);
}

/**
 * @brief Les quatre fiches pre-tirees se proposent, et forment le groupe de depart (EX-EXP-013).
 * \castest{<b>Le dossier des personnages propose les quatre fiches pre-tirees ; une partie neuve
 * les prend toutes, le Brawler en tete.</b><br/>
 * \tcat Unitaire · Groupe<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire `Rpg/characters/` sans construire les fiches.<br/>
 * 2. Former le groupe de depart.<br/>
 * \tattendu Quatre candidats sans erreur, chacun avec son nom, sa classe et son fichier ; le
 * groupe de depart les prend dans l'ordre, Grom Tranche-Ecaille (Brawler) en tete.
 * }
 */
TEST(PartyTest, LesQuatreFichesPreTireesFormentLeGroupeDeDepart) {
    const core::PartyCandidates lus =
        core::loadPartyCandidates(std::filesystem::path{JADG_RPG_DIR} / "characters");
    EXPECT_TRUE(lus.errors.empty());
    ASSERT_EQ(lus.candidates.size(), 4U);
    std::vector<std::string> classes;
    for (const core::PartyCandidate& candidat : lus.candidates) {
        EXPECT_FALSE(candidat.name.empty()) << candidat.id;
        EXPECT_EQ(candidat.file.stem().string(), candidat.id);
        classes.push_back(candidat.classId);
    }
    EXPECT_EQ(classes, (std::vector<std::string>{"brawler", "mage", "priest", "scoundrel"}));

    const Party depart = core::defaultParty(lus.candidates);
    EXPECT_EQ(depart.members(), QUATRE);
    EXPECT_EQ(lus.candidates.front().name, "Grom Tranche-Écaille");
}

/**
 * @brief Un dossier absent est une erreur, pas un groupe vide en silence.
 * \castest{<b>Un dossier de personnages absent se signale.</b><br/>
 * \tcat Unitaire · Groupe<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Lire un dossier qui n'existe pas.<br/>
 * \tattendu Aucun candidat, une erreur qui nomme le dossier.
 * }
 */
TEST(PartyTest, UnDossierAbsentSeSignale) {
    const core::PartyCandidates lus = core::loadPartyCandidates("dossier-qui-n-existe-pas");
    EXPECT_TRUE(lus.candidates.empty());
    ASSERT_EQ(lus.errors.size(), 1U);
    EXPECT_NE(lus.errors.front().find("dossier-qui-n-existe-pas"), std::string::npos);
}
