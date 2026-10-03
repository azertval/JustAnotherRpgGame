// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include "Core/Rpg/PartyLedger.h"
#include "HMI/Runtime/InventoryModel.h"
#include "HMI/Runtime/OptionsModel.h"
#include "HMI/Runtime/ScreenRouter.h"
#include "HMI/Runtime/WorldModel.h"

// A separate settings namespace prevents a test from changing the player's preferences.
/**
 * @brief La taille du HUD est bornée de 75 % à 130 % et survit à un rechargement des préférences.
 * \castest{<b>La taille du HUD est bornee et survit a un rechargement.</b><br/>
 * \tcat Unitaire · Options<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Isoler les preferences dans un dossier temporaire.<br/>
 *          2. Regler la taille du HUD a 120, puis a 1000 et a -1 ; relire par un second
 * modele.<br/>
 *          3. Ecrire une valeur hors bornes dans les preferences et recharger.<br/>
 * \tattendu 120 est relu tel quel ; 1000 est ramene a 130, -1 a 75 ; le defaut vaut 100 ; une
 * valeur enregistree hors bornes est ramenee a 130 au chargement.
 * }
 */
TEST(UiPreferencesTest, HudSizeIsBoundedAndSurvivesReload) {
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const QString organization = QCoreApplication::organizationName();
    const QString application = QCoreApplication::applicationName();
    const QSettings::Format format = QSettings::defaultFormat();
    QCoreApplication::setOrganizationName("JadgUiTests");
    QCoreApplication::setApplicationName("HudSize");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
    {
        hmi::OptionsModel options;
        EXPECT_EQ(options.hudScalePercent(), 100);
        options.setHudScalePercent(120);
        hmi::OptionsModel reloaded;
        EXPECT_EQ(reloaded.hudScalePercent(), 120);
        options.setHudScalePercent(1000);
        EXPECT_EQ(options.hudScalePercent(), 130);
        options.setHudScalePercent(-1);
        EXPECT_EQ(options.hudScalePercent(), 75);
        EXPECT_EQ(options.defaults().value("hudScalePercent").toInt(), 100);
        QSettings().setValue("hud_scale_percent", 9999);
        hmi::OptionsModel invalidStored;
        EXPECT_EQ(invalidStored.hudScalePercent(), 130);
    }
    QSettings::setDefaultFormat(format);
    QCoreApplication::setOrganizationName(organization);
    QCoreApplication::setApplicationName(application);
}

/**
 * @brief L'anticrénelage et la définition du rendu ne prennent que les valeurs proposées, et
 *        survivent à un rechargement.
 * \castest{<b>Les reglages de rendu ne prennent que les valeurs proposees.</b><br/>
 * \tcat Unitaire · Options<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Isoler les preferences dans un dossier temporaire.<br/>
 *          2. Lire les valeurs d'usine, puis regler 8 echantillons et 150 % ; relire par un second
 * modele.<br/>
 *          3. Demander 3 echantillons et 137 %, que l'ecran ne propose pas.<br/>
 *          4. Ecrire des valeurs non proposees dans les preferences et recharger.<br/>
 * \tattendu L'usine vaut 4 echantillons et 100 % ; 8 et 150 sont relus tels quels et annonces
 * une fois chacun ; 3 et 137 sont ignores sans annonce ; des valeurs enregistrees non proposees
 * rendent celles d'usine au chargement.
 * }
 */
TEST(UiPreferencesTest, RenderSettingsTakeOnlyOfferedValues) {
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const QString organization = QCoreApplication::organizationName();
    const QString application = QCoreApplication::applicationName();
    const QSettings::Format format = QSettings::defaultFormat();
    QCoreApplication::setOrganizationName("JadgUiTests");
    QCoreApplication::setApplicationName("RenderSettings");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
    {
        hmi::OptionsModel options;
        EXPECT_EQ(options.antialiasing(), 4);
        EXPECT_EQ(options.renderScalePercent(), 100);
        EXPECT_EQ(options.defaults().value("antialiasing").toInt(), 4);
        EXPECT_EQ(options.defaults().value("renderScalePercent").toInt(), 100);
        EXPECT_EQ(options.antialiasingLevels(), (QList<int>{1, 2, 4, 8}));
        EXPECT_EQ(options.renderScales(), (QList<int>{100, 125, 150, 200}));

        int antialiasingChanges = 0;
        int renderScaleChanges = 0;
        QObject::connect(&options, &hmi::OptionsModel::antialiasingChanged,
                         [&antialiasingChanges] { ++antialiasingChanges; });
        QObject::connect(&options, &hmi::OptionsModel::renderScaleChanged,
                         [&renderScaleChanges] { ++renderScaleChanges; });
        options.setAntialiasing(8);
        options.setRenderScalePercent(150);
        hmi::OptionsModel reloaded;
        EXPECT_EQ(reloaded.antialiasing(), 8);
        EXPECT_EQ(reloaded.renderScalePercent(), 150);

        options.setAntialiasing(3);
        options.setRenderScalePercent(137);
        EXPECT_EQ(options.antialiasing(), 8);
        EXPECT_EQ(options.renderScalePercent(), 150);
        EXPECT_EQ(antialiasingChanges, 1);
        EXPECT_EQ(renderScaleChanges, 1);

        QSettings().setValue("antialiasing_samples", 64);
        QSettings().setValue("render_scale_percent", 9999);
        hmi::OptionsModel invalidStored;
        EXPECT_EQ(invalidStored.antialiasing(), 4);
        EXPECT_EQ(invalidStored.renderScalePercent(), 100);
    }
    QSettings::setDefaultFormat(format);
    QCoreApplication::setOrganizationName(organization);
    QCoreApplication::setApplicationName(application);
}

// Equipment changes must remain on the shown mercenary when the screen is reopened.
/**
 * @brief Un changement d'équipement reste sur le mercenaire affiché quand l'écran se rouvre, et
 * après un relevé de combat.
 * \castest{<b>L'equipement survit a la reouverture de l'ecran et au combat.</b><br/>
 * \tcat Unitaire · Inventaire<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Retirer une piece equipee du mercenaire affiche.<br/>
 *          2. Rouvrir l'inventaire.<br/>
 *          3. Enregistrer un releve de combat pour ce mercenaire, rééquiper la piece, rouvrir.<br/>
 *          4. Chercher un objet qui n'existe pas.<br/>
 * \tattendu A la reouverture, la piece est dans le sac et son emplacement est vide ; le releve de
 * combat garde l'inventaire ; la piece reequipee est a son emplacement ; la recherche sans resultat
 * ne montre aucune case.
 * }
 */
TEST(InventoryModelTest, EquipmentSurvivesReopeningAndCombatRecords) {
    hmi::WorldModel world;
    hmi::InventoryModel inventory;
    inventory.loadShownCharacter();
    const QString member = inventory.characterId();
    ASSERT_FALSE(member.isEmpty());
    const QVariantMap equipment = inventory.equipped();
    QString equippedSlot;
    for (auto it = equipment.begin(); it != equipment.end(); ++it) {
        inventory.selectSlot(it.key());
        if (inventory.selection().value("canUnequip").toBool()) {
            equippedSlot = it.key();
            break;
        }
    }
    ASSERT_FALSE(equippedSlot.isEmpty());
    const QString item = inventory.selection().value("itemId").toString();
    inventory.unequipSelected();
    hmi::InventoryModel reopened;
    reopened.loadShownCharacter();
    reopened.selectItem(item);
    ASSERT_TRUE(reopened.selection().value("canEquip").toBool());
    reopened.selectSlot(equippedSlot);
    EXPECT_TRUE(reopened.selection().isEmpty());

    core::MemberRecord combat;
    combat.hitPoints = 1;
    world.recordMember(member.toStdString(), combat);
    ASSERT_NE(world.ledger().record(member.toStdString()), nullptr);
    EXPECT_TRUE(world.ledger().record(member.toStdString())->inventory.has_value());
    reopened.loadShownCharacter();
    reopened.selectItem(item);
    reopened.equipSelected();
    reopened.loadShownCharacter();
    reopened.selectSlot(equippedSlot);
    EXPECT_EQ(reopened.selection().value("itemId").toString(), item);
    reopened.setQuery("__no_such_item__");
    EXPECT_TRUE(reopened.cells().isEmpty());
}

/**
 * @brief Un repos rend les ressources dépensées et garde l'inventaire.
 * \castest{<b>Un repos garde l'equipement et rend les ressources depensees.</b><br/>
 * \tcat Unitaire · Registre du groupe<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ecrire au registre un mercenaire blesse, un sort depense, une bourse de 123
 * pieces.<br/>
 *          2. Le faire se reposer.<br/>
 * \tattendu Les points de vie et les lancers depenses sont oublies ; l'inventaire et sa bourse de
 * 123 pieces sont gardes.
 * }
 */
TEST(InventoryModelTest, RestPreservesEquipmentAndClearsSpentResources) {
    core::PartyLedger ledger;
    core::MemberRecord record;
    record.hitPoints = 1;
    record.spellUses = {{"sort", 0}};
    record.inventory = core::Inventory{};
    record.inventory->purseCopper = 123;
    ledger.write("mercenary", record);
    ledger.rest("mercenary");
    const core::MemberRecord* rested = ledger.record("mercenary");
    ASSERT_NE(rested, nullptr);
    ASSERT_TRUE(rested->inventory.has_value());
    EXPECT_EQ(rested->inventory->purseCopper, 123);
    EXPECT_FALSE(rested->hitPoints.has_value());
    EXPECT_TRUE(rested->spellUses.empty());
}

/**
 * @brief Fermer le codex ou les options ramène au combat en cours, pas à l'exploration.
 * \castest{<b>Le codex et les options ramenent au combat en cours.</b><br/>
 * \tcat Unitaire · Routeur d'ecrans<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Ouvrir le jeu, puis le HUD de combat.<br/>
 *          2. Ouvrir un onglet du codex, puis l'inventaire, et fermer.<br/>
 *          3. Ouvrir puis fermer les options ; fermer enfin l'ecran de combat.<br/>
 * \tattendu L'onglet demande est retenu ; fermer l'inventaire puis les options rend le HUD de
 * combat ; le fermer rend le jeu.
 * }
 */
TEST(ScreenRouterTest, CodexAndOptionsReturnToOngoingCombat) {
    hmi::ScreenRouter router;
    router.openGame();
    router.openRpgScreen(hmi::ScreenRouter::RpgScreen::CombatHud);
    router.openCharacterTab(1);
    EXPECT_EQ(router.characterTab(), 1);
    router.openRpgScreen(hmi::ScreenRouter::RpgScreen::Inventory);
    router.closeRpgScreen();
    EXPECT_EQ(router.currentScreen(), hmi::ScreenRouter::Screen::RpgScreen);
    EXPECT_EQ(router.currentRpgScreen(), hmi::ScreenRouter::RpgScreen::CombatHud);
    router.openOptions();
    router.closeOptions();
    EXPECT_EQ(router.currentRpgScreen(), hmi::ScreenRouter::RpgScreen::CombatHud);
    router.closeRpgScreen();
    EXPECT_EQ(router.currentScreen(), hmi::ScreenRouter::Screen::Game);
}
