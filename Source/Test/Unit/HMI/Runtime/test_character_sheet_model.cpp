// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_character_sheet_model.cpp
 * @brief Tests de la fiche de chacun (`LOT-141`) : chaque fiche pré-tirée s'ouvre par son
 *        identifiant et montre sa page du livre, valeur pour valeur ; l'onglet Classe dit les
 *        capacités acquises et à venir, l'onglet Sorts les lancers ; une montée de niveau donnée
 *        par la partie se voit sur la fiche, à l'écran de groupe et au combat.
 */

#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <filesystem>
#include <set>
#include <string>

#include <gtest/gtest.h>

#include "Core/Rpg/PartyLedger.h"
#include "HMI/Runtime/CharacterSheetModel.h"
#include "HMI/Runtime/EncounterModel.h"
#include "HMI/Runtime/PartyModel.h"
#include "HMI/Runtime/WorldModel.h"

namespace {

[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path{JADG_TEST_DATA_DIR};
}

void ouvrirLeDonjon(hmi::WorldModel& monde) {
    monde.setLevelDirectories({dataRoot() / "Levels"});
    monde.setStartOverride(QStringLiteral("donjon"), QStringLiteral("sable"));
    monde.setStartCell(core::GridPosition{.column = 24, .row = 19});
    ASSERT_TRUE(monde.startNewGame()) << monde.status().toStdString();
}

[[nodiscard]] std::set<QString> names(const QVariantList& rows) {
    std::set<QString> noms;
    for (const QVariant& row : rows) {
        noms.insert(row.toMap().value("name").toString());
    }
    return noms;
}

/// Une page du livre : ce que la fiche affiche pour une fiche pré-tirée de niveau 1.
struct Page {
    const char* id;
    QString name;
    QString className;
    QString hitPoints;
    QString armorClass;
    std::set<QString> capacities;
    std::set<QString> spells;
};

}  // namespace

/**
 * @brief Chacune des quatre fiches préfabriquées s'affiche comme sa page du livre, valeur pour
 *        valeur (`LOT-141`, `EX-IHM-109`), et ses onglets Classe et Sorts disent ce que la table
 *        de sa classe donne.
 * \castest{<b>Les quatre pages du Player's Guide, ouvertes par leur identifiant.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Sans partie, charger chaque fiche par `loadCharacter(id)`.<br/>2. Lire nom, classe,
 * points de vie, CA, capacites acquises, sorts connus, capacites a venir.<br/>
 * \tattendu Brawler 15 / 15, CA 14, Tough as Nails ; Mage 8 / 8, CA 12, deux sorts mineurs et
 * deux sorts a 2 lancers ; Priest 12 / 12, CA 17 ; Scoundrel 10 / 10, CA 14, Sneak Attack et
 * Scoundrel's Agility ; a venir au niveau 3 pour le Scoundrel : Sneak Attack 2d8 et Adventurer's
 * Aptitude.
 * }
 */
TEST(CharacterSheetModelTest, LesQuatreFichesSontLeursPagesDuLivre) {
    const std::vector<Page> pages{
        {"heros-brawler",
         "Grom Tranche-Écaille",
         "Brawler",
         "15 / 15",
         "14",
         {"Tough as Nails"},
         {}},
        {"heros-mage",
         "Faelar Trace-Carte",
         "Mage",
         "8 / 8",
         "12",
         {"Simplified Spellcasting", "Specific Cantrips"},
         {"Trait de feu", "Lumiere", "Detection de la magie", "Projectile magique"}},
        {"heros-priest",
         "Helga Pierre-Sûre",
         "Priest",
         "12 / 12",
         "17",
         {"Simplified Spellcasting", "Specific Cantrips"},
         {"Lumiere", "Flamme sacree", "Benediction", "Soin des blessures"}},
        {"heros-scoundrel",
         "Nessa Double-Vie",
         "Scoundrel",
         "10 / 10",
         "14",
         {"Sneak Attack Simplified", "Scoundrel's Agility"},
         {}},
    };
    hmi::CharacterSheetModel fiche;
    for (const Page& page : pages) {
        fiche.loadCharacter(QString::fromUtf8(page.id));
        EXPECT_EQ(fiche.characterId(), QString::fromUtf8(page.id));
        EXPECT_EQ(fiche.name(), page.name);
        EXPECT_EQ(fiche.className(), page.className);
        EXPECT_EQ(fiche.level(), QStringLiteral("1"));
        EXPECT_EQ(fiche.hitPoints(), page.hitPoints) << page.id;
        EXPECT_EQ(fiche.armorClass(), page.armorClass) << page.id;
        EXPECT_EQ(names(fiche.capacities()), page.capacities) << page.id;
        EXPECT_EQ(names(fiche.spells()), page.spells) << page.id;
        for (const QVariant& row : fiche.capacities()) {
            EXPECT_EQ(row.toMap().value("level").toInt(), 1);
            EXPECT_TRUE(row.toMap().value("iconKey").toString().startsWith(
                QStringLiteral("ui/icon/capacity/")));
        }
        EXPECT_FALSE(fiche.upcomingCapacities().isEmpty()) << page.id;
        // Une attaque d'arme, au jet signe ; les descriptions sans la citation de la page.
        ASSERT_EQ(fiche.attacks().size(), 1) << page.id;
        EXPECT_TRUE(fiche.attacks().front().toMap().value("value").toString().startsWith('+'));
        for (const QVariant& row : fiche.capacities()) {
            const QString texte = row.toMap().value("text").toString();
            EXPECT_FALSE(texte.contains(QStringLiteral(" p. "))) << texte.toStdString();
            EXPECT_FALSE(texte.isEmpty());
        }
    }

    // Les lancers du Mage : les sorts mineurs a volonte, les autres deux fois par jour.
    fiche.loadCharacter(QStringLiteral("heros-mage"));
    for (const QVariant& row : fiche.spells()) {
        const QVariantMap sort = row.toMap();
        if (sort.value("level").toInt() == 0) {
            EXPECT_EQ(sort.value("usesText").toString(), QStringLiteral("à volonté"));
        } else {
            EXPECT_EQ(sort.value("usesText").toString(), QStringLiteral("2 / 2"));
        }
        EXPECT_TRUE(sort.value("iconKey").toString().startsWith(QStringLiteral("ui/icon/spell/")));
        EXPECT_FALSE(sort.value("details").toString().isEmpty());
        EXPECT_FALSE(sort.value("text").toString().contains(QStringLiteral(" p. ")));
        EXPECT_FALSE(sort.value("text").toString().isEmpty());
        EXPECT_FALSE(sort.value("components").toString().isEmpty());
        EXPECT_FALSE(sort.value("range").toString().isEmpty());
    }

    // A venir pour le Scoundrel de niveau 1 : le palier 2d8 et Adventurer's Aptitude, au niveau 3.
    fiche.loadCharacter(QStringLiteral("heros-scoundrel"));
    std::set<QString> auNiveau3;
    for (const QVariant& row : fiche.upcomingCapacities()) {
        if (row.toMap().value("level").toInt() == 3) {
            auNiveau3.insert(row.toMap().value("name").toString());
        }
    }
    EXPECT_EQ(auNiveau3, (std::set<QString>{"Sneak Attack Simplified", "Adventurer's Aptitude"}));
    // Un identifiant inconnu ouvre le personnage joue, sans casser.
    fiche.loadCharacter(QStringLiteral("inconnu"));
    EXPECT_FALSE(fiche.name().isEmpty());
}

/**
 * @brief La montée de niveau est **donnée** (`LOT-141`) : la partie la retient au registre du
 *        groupe, et la fiche, l'écran de groupe et le combat la lisent.
 * \castest{<b>Un niveau donne se voit partout, et n'est pas un soin.</b><br/>
 * \tcat Unitaire · Fiche de personnage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Ouvrir le donjon ; noter 5 PV au Priest.<br/>2. `levelUp("heros-priest")` deux
 * fois, puis `levelUp("party")`.<br/>3. Lire la fiche du Priest, l'ecran de groupe, le panneau
 * de l'actif d'une rencontre ; monter cinq fois de plus.<br/>
 * \tattendu Le Priest est niveau 3 puis 4, ses points de vie ont monte du gain (5 + 9 + 9), pas
 * jusqu'au maximum ; epargner les mourants et l'arme spirituelle sont connus ; l'ecran de groupe
 * dit le niveau 4 ; en combat le membre actif porte son niveau ; le niveau se borne au maximum
 * de la table, et `levelUp` le dit par faux.
 * }
 */
TEST(CharacterSheetModelTest, LaMonteeDeNiveauDonneeSeVoitPartout) {
    hmi::WorldModel monde;
    ouvrirLeDonjon(monde);
    core::MemberRecord blesse;
    blesse.hitPoints = 5;
    monde.recordMember("heros-priest", blesse);

    EXPECT_TRUE(monde.levelUp(QStringLiteral("heros-priest")));
    EXPECT_TRUE(monde.levelUp(QStringLiteral("heros-priest")));
    EXPECT_FALSE(monde.levelUp(QStringLiteral("inconnu")));
    hmi::CharacterSheetModel fiche;
    fiche.loadCharacter(QStringLiteral("heros-priest"));
    EXPECT_EQ(fiche.level(), QStringLiteral("3"));
    // 12 + 9 + 9 au maximum ; les courants montent du gain seulement (LOT-13).
    EXPECT_EQ(fiche.hitPoints(), QStringLiteral("23 / 30"));
    EXPECT_TRUE(names(fiche.spells()).contains(QStringLiteral("Epargner les mourants")));
    EXPECT_TRUE(names(fiche.spells()).contains(QStringLiteral("Arme spirituelle")));

    // Tout le groupe : le Priest passe 4, les autres 2.
    EXPECT_TRUE(monde.levelUp(QStringLiteral("party")));
    hmi::PartyModel groupe;
    for (const QVariant& row : groupe.members()) {
        const QVariantMap membre = row.toMap();
        EXPECT_EQ(membre.value("level").toString(),
                  membre.value("id").toString() == QStringLiteral("heros-priest")
                      ? QStringLiteral("4")
                      : QStringLiteral("2"))
            << membre.value("id").toString().toStdString();
    }
    // Le personnage designe pour la fiche : celui de l'ecran de groupe, sinon le meneur.
    EXPECT_EQ(monde.shownCharacterId(), QStringLiteral("heros-brawler"));
    monde.showCharacter(QStringLiteral("heros-mage"));
    EXPECT_EQ(monde.shownCharacterId(), QStringLiteral("heros-mage"));
    fiche.loadShownCharacter();
    EXPECT_EQ(fiche.characterId(), QStringLiteral("heros-mage"));
    EXPECT_EQ(fiche.level(), QStringLiteral("2"));

    // Au combat, le membre actif porte son niveau donne.
    hmi::EncounterModel rencontre;
    rencontre.setContentRoot(dataRoot());
    rencontre.setSeed(2026);
    ASSERT_TRUE(rencontre.begin(QStringLiteral("rats-du-donjon")))
        << rencontre.status().toStdString();
    for (int pas = 0; pas < 2000 && (rencontre.busy() || rencontre.turnActions().isEmpty()) &&
                      !rencontre.ended();
         ++pas) {
        rencontre.tick(1.0F / 60.0F);
    }
    const QVariantMap actif = rencontre.activeProfile();
    EXPECT_GE(actif.value("level").toInt(), 2);
    rencontre.leave();

    // Borne : la table s'arrete, la montee le dit.
    for (int i = 0; i < 40; ++i) {
        static_cast<void>(monde.levelUp(QStringLiteral("heros-priest")));
    }
    EXPECT_FALSE(monde.levelUp(QStringLiteral("heros-priest")));
}
