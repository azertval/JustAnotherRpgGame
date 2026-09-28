// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_class_capacities.cpp
 * @brief Tests du socle de classe (`LOT-131`, `EX-RPG-020`, `EX-RPG-024`, `EX-RPG-025`) : le
 *        catalogue des capacites, la table qui les donne au niveau, les effets que la fiche en
 * tire, l'incantation simplifiee et le repos long.
 *
 * La classe d'ESSAI de la racine de donnees (`Fixtures/GameData/Rpg/classes/lutteur-d-essai.json`)
 * n'est nommee que par ces tests : le moteur, lui, ne connait que des effets nommes.
 */

#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Rpg/Ability.h"
#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/ClassCapacities.h"
#include "Core/Rpg/Equipment.h"

namespace {

const std::filesystem::path RACINE_ESSAI{JADG_TEST_DATA_DIR};
const std::filesystem::path REGLES{JADG_RPG_RULES_DIR};
const std::filesystem::path RPG_LIVRE{JADG_RPG_DIR};

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

/// Les options de la racine d'essai : la classe d'essai, ses capacites, ses sorts.
[[nodiscard]] const core::CharacterOptions& optionsDEssai() {
    static const core::CharacterOptions options = core::loadCharacterOptions(RACINE_ESSAI / "Rpg");
    return options;
}

/// Une fiche de la classe d'essai au niveau demande : For 16, Dex 14, Con 16, Int 12, Sag 10,
/// Cha 8, vitesse 9 m. Sans espece ni historique : rien ici ne les concerne.
[[nodiscard]] core::LoadedCharacterSheet lutteuse(int niveau) {
    const core::CharacterOptions& options = optionsDEssai();
    const core::PlayableClass* classe = options.findClass("lutteur-d-essai");
    core::LoadedCharacterSheet resultat;
    if (classe == nullptr) {
        resultat.errors.emplace_back("classe d'essai absente");
        return resultat;
    }
    resultat.sheet = core::buildCharacterSheet("Lutteuse", {16, 14, 16, 12, 10, 8}, nullptr, classe,
                                               nullptr, regles().creation, regles().experience);
    resultat.sheet.speedMeters = 9.0F;
    if (niveau > 1) {
        core::gainExperience(resultat.sheet, regles().experience, classe->hitDie,
                             regles().experience.thresholdAt(niveau));
    }
    core::applyClassFeatures(resultat.sheet, *classe, options, regles().creation,
                             resultat.warnings);
    return resultat;
}

[[nodiscard]] std::vector<std::string> identifiants(const std::vector<core::Capacity>& capacites) {
    std::vector<std::string> ids;
    for (const core::Capacity& capacite : capacites) {
        ids.push_back(capacite.id);
    }
    return ids;
}

/// Un dossier temporaire vide, propre a ce test.
[[nodiscard]] std::filesystem::path dossierTemporaire(const char* nom) {
    const std::filesystem::path dossier =
        std::filesystem::temp_directory_path() / "jadg-lot-131" / nom;
    std::filesystem::remove_all(dossier);
    std::filesystem::create_directories(dossier);
    return dossier;
}

void ecrire(const std::filesystem::path& chemin, const char* contenu) {
    std::ofstream fichier(chemin);
    fichier << contenu;
}

}  // namespace

/**
 * @brief Le catalogue des capacites se charge, et refuse ce que le moteur ne sait pas jouer.
 * \castest{<b>Quatre capacites d'essai se chargent avec leurs effets ; un genre d'effet inconnu ou
 * des des illisibles refusent la capacite entiere, nommement.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger Fixtures/GameData/Rpg/capacities.<br/>2. Charger un dossier a deux
 * capacites fautives et une saine.<br/>3. Charger un dossier absent.<br/>
 * \tattendu Quatre capacites, zero erreur, les effets lus champ par champ ; deux erreurs nommant
 * le fichier et le genre, une seule capacite gardee ; une erreur, pas un catalogue vide.
 * }
 */
TEST(ClassCapacitiesTest, LeCatalogueSeChargeEtRefuseLInconnu) {
    const core::CapacityCatalog catalogue = core::loadCapacities(RACINE_ESSAI / "Rpg/capacities");
    EXPECT_TRUE(catalogue.errors.empty()) << catalogue.errors.front();
    ASSERT_EQ(catalogue.capacities.size(), 4U);

    const core::Capacity* coup = catalogue.find("coup-precis");
    ASSERT_NE(coup, nullptr);
    EXPECT_EQ(coup->name, "Coup precis");
    ASSERT_EQ(coup->effects.size(), 2U);
    EXPECT_EQ(coup->effects[0].kind, core::CapacityEffectKind::AttackBonus);
    EXPECT_EQ(coup->effects[0].value, 2);
    EXPECT_EQ(coup->effects[1].kind, core::CapacityEffectKind::ExtraDamage);
    EXPECT_EQ(coup->effects[1].dice, (core::Dice{.count = 1, .faces = 6, .modifier = 0}));
    EXPECT_TRUE(coup->effects[1].oncePerTurn);

    const core::Capacity* peau = catalogue.find("peau-de-fer");
    ASSERT_NE(peau, nullptr);
    ASSERT_EQ(peau->effects.size(), 2U);
    EXPECT_EQ(peau->effects[0].kind, core::CapacityEffectKind::UnarmoredArmorClass);
    EXPECT_EQ(peau->effects[0].base, 10);
    EXPECT_EQ(peau->effects[0].abilities,
              (std::vector<core::Ability>{core::Ability::Dexterity, core::Ability::Constitution}));
    EXPECT_TRUE(peau->effects[0].shieldAllowed);
    EXPECT_EQ(peau->effects[1].kind, core::CapacityEffectKind::DamageResistance);
    EXPECT_TRUE(peau->effects[1].allDamageTypes);

    const core::Capacity* ameliore = catalogue.find("coup-precis-ameliore");
    ASSERT_NE(ameliore, nullptr);
    EXPECT_EQ(ameliore->replaces, "coup-precis");
    EXPECT_TRUE(ameliore->iconId.empty()) << "au catalogue, l'icone reprise n'est pas encore posee";

    // Les noms du schema et ceux du moteur disent la meme liste.
    for (const char* nom :
         {"attack-bonus", "armor-class-bonus", "unarmored-armor-class", "damage-resistance",
          "speed-bonus", "no-opportunity-attacks", "extra-damage"}) {
        const std::optional<core::CapacityEffectKind> genre = core::parseCapacityEffectKind(nom);
        ASSERT_TRUE(genre.has_value()) << nom;
        EXPECT_EQ(core::capacityEffectKindName(*genre), nom);
    }
    EXPECT_FALSE(core::parseCapacityEffectKind("devenir-invincible").has_value());

    const std::filesystem::path fautif = dossierTemporaire("capacites-fautives");
    ecrire(fautif / "a-genre-inconnu.json",
           R"({"id":"a-genre-inconnu","name":"A","source":"original","text":"t",
               "effects":[{"kind":"devenir-invincible"}]})");
    ecrire(fautif / "b-des-illisibles.json",
           R"({"id":"b-des-illisibles","name":"B","source":"original","text":"t",
               "effects":[{"kind":"extra-damage","dice":"ld8"}]})");
    ecrire(fautif / "c-saine.json",
           R"({"id":"c-saine","name":"C","source":"original","text":"t",
               "effects":[{"kind":"speed-bonus","meters":1.5}]})");
    const core::CapacityCatalog refus = core::loadCapacities(fautif);
    ASSERT_EQ(refus.errors.size(), 2U);
    EXPECT_NE(refus.errors[0].find("a-genre-inconnu.json"), std::string::npos) << refus.errors[0];
    EXPECT_NE(refus.errors[0].find("devenir-invincible"), std::string::npos) << refus.errors[0];
    EXPECT_NE(refus.errors[1].find("b-des-illisibles.json"), std::string::npos) << refus.errors[1];
    ASSERT_EQ(refus.capacities.size(), 1U);
    EXPECT_EQ(refus.capacities[0].id, "c-saine");
    EXPECT_FLOAT_EQ(refus.capacities[0].effects[0].meters, 1.5F);

    const core::CapacityCatalog absent = core::loadCapacities(fautif / "nulle-part");
    EXPECT_EQ(absent.errors.size(), 1U);
    EXPECT_TRUE(absent.capacities.empty());
}

/**
 * @brief La table de progression donne les capacites et les sorts au niveau ou elle les ecrit.
 * \castest{<b>Une classe se charge avec ses maitrises, son incantation et sa table ; les capacites
 * actives suivent le niveau, une capacite qui en remplace une autre la retire, une capacite absente
 * du catalogue est nommee.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger la racine d'essai (especes et historiques absents : deux erreurs nommees,
 * la classe se charge quand meme).<br/>2. Resoudre les capacites aux niveaux 1, 2 et 3.<br/>3.
 * Resoudre une classe qui nomme une capacite inconnue.<br/>
 * \tattendu Maitrises et incantation lues ; N1 : coup-precis, peau-de-fer ; N2 : + pas-de-danseur ;
 * N3 : coup-precis-ameliore a la place de coup-precis ; sorts mineurs et sorts cumules par niveau ;
 * l'inconnue est dans `missing`, pas jouee.
 * }
 */
TEST(ClassCapacitiesTest, LaTableDonneLesCapacitesEtLesSortsAuNiveau) {
    const core::CharacterOptions& options = optionsDEssai();
    // La racine d'essai n'a ni especes ni historiques : deux dossiers absents, deux erreurs -- pas
    // un catalogue vide en silence.
    EXPECT_EQ(options.errors.size(), 2U);
    ASSERT_EQ(options.classes.size(), 1U);
    EXPECT_EQ(options.capacities.capacities.size(), 4U);
    EXPECT_EQ(options.spells.spells.size(), 3U);

    const core::PlayableClass& classe = options.classes.front();
    EXPECT_EQ(classe.id, "lutteur-d-essai");
    EXPECT_EQ(classe.hitDie, 10);
    EXPECT_EQ(classe.armorProficiencies, (std::vector<std::string>{"light", "shields"}));
    EXPECT_EQ(classe.weaponProficiencies, (std::vector<std::string>{"simple", "rapiere"}));
    EXPECT_TRUE(classe.isProficientWithWeapon("gourdin", "simple"));
    EXPECT_TRUE(classe.isProficientWithWeapon("rapiere", "martial"));
    EXPECT_FALSE(classe.isProficientWithWeapon("epee-longue", "martial"));
    EXPECT_TRUE(classe.isProficientWithArmor("shields"));
    EXPECT_FALSE(classe.isProficientWithArmor("heavy"));
    EXPECT_EQ(classe.skillChoices.count, 2);
    EXPECT_EQ(classe.skillChoices.from.size(), 3U);
    ASSERT_TRUE(classe.spellcasting.has_value());
    EXPECT_EQ(classe.spellcasting->ability, core::Ability::Intelligence);
    EXPECT_EQ(classe.spellcasting->castsPerDay, 2);

    std::vector<std::string> manquants;
    EXPECT_EQ(identifiants(core::resolveCapacities(classe, 1, options.capacities, manquants)),
              (std::vector<std::string>{"coup-precis", "peau-de-fer"}));
    EXPECT_EQ(identifiants(core::resolveCapacities(classe, 2, options.capacities, manquants)),
              (std::vector<std::string>{"coup-precis", "peau-de-fer", "pas-de-danseur"}));
    // Au niveau 3, Coup precis ameliore REMPLACE Coup precis : trois capacites, pas quatre.
    EXPECT_EQ(identifiants(core::resolveCapacities(classe, 3, options.capacities, manquants)),
              (std::vector<std::string>{"peau-de-fer", "pas-de-danseur", "coup-precis-ameliore"}));
    EXPECT_TRUE(manquants.empty());
    // Le palier garde l'icone de sa base (LOT-140) : l'ecran lit `iconId`.
    {
        const std::vector<core::Capacity> niveau3 =
            core::resolveCapacities(classe, 3, options.capacities, manquants);
        EXPECT_EQ(niveau3.back().iconId, "coup-precis");
        EXPECT_TRUE(niveau3.front().iconId.empty());
    }

    EXPECT_EQ(classe.cantripsAt(1), (std::vector<std::string>{"etincelle-d-essai"}));
    EXPECT_EQ(classe.spellsAt(1), (std::vector<std::string>{"trait-de-feu-d-essai"}));
    EXPECT_EQ(classe.spellsAt(2), (std::vector<std::string>{"trait-de-feu-d-essai"}));
    EXPECT_EQ(classe.spellsAt(3),
              (std::vector<std::string>{"trait-de-feu-d-essai", "second-trait-d-essai"}));

    core::PlayableClass bancale = classe;
    bancale.progression[0].features.push_back("capacite-a-venir");
    const std::vector<core::Capacity> actives =
        core::resolveCapacities(bancale, 1, options.capacities, manquants);
    EXPECT_EQ(actives.size(), 2U);
    EXPECT_EQ(manquants, (std::vector<std::string>{"capacite-a-venir"}));
}

/**
 * @brief La fiche tire de ses capacites une CA, une vitesse, des resistances et des maitrises.
 * \castest{<b>Au niveau 3, la fiche d'essai a la CA de sa formule sans armure (bouclier permis),
 * la vitesse de sa capacite, une resistance a tout, un bonus d'attaque et des des en plus au nom de
 * la capacite qui remplace l'ancienne ; elle maitrise les armes de sa classe et pas les autres.</b>
 * <br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Construire la fiche au niveau 3 et lui appliquer sa classe.<br/>2. Lire la CA sans
 * armure, avec bouclier, avec une armure legere.<br/>3. Lire vitesse, resistances, modificateurs
 * d'attaque, des en plus, immunite aux opportunites, maitrises d'armes.<br/>
 * \tattendu CA 15 = 10 + Dex 2 + Con 3 (la regle generale donnerait 12) ; 17 avec bouclier ; 13
 * en cuir (11 + Dex, la formule ne joue pas sous une armure) ; 12 m et 8 cases ; une resistance a
 * tous les types nommee Peau de fer ; + 3 (Coup precis ameliore) ; 2d6 une fois par tour ; Pas de
 * danseur ; gourdin et rapiere maitrises, epee longue non.
 * }
 */
TEST(ClassCapacitiesTest, LaFicheTireSesValeursDeSesCapacites) {
    const core::LoadedCharacterSheet charge = lutteuse(3);
    ASSERT_TRUE(charge.ok()) << charge.errors.front();
    EXPECT_TRUE(charge.warnings.empty()) << charge.warnings.front();
    const core::CharacterSheet& fiche = charge.sheet;
    ASSERT_EQ(fiche.level, 3);
    ASSERT_EQ(fiche.capacities.size(), 3U);

    // CA sans armure : la formule de la capacite, pas la regle generale (EX-CBT-030).
    EXPECT_EQ(fiche.armorClass, 15) << "10 + Dex 2 + Con 3, Peau de fer";
    EXPECT_EQ(core::armorClassFor(fiche, regles().creation, nullptr, nullptr), 15);
    core::Armor bouclier;
    bouclier.id = "bouclier";
    bouclier.category = core::ArmorCategory::Shield;
    bouclier.baseArmorClass = 2;
    EXPECT_EQ(core::armorClassFor(fiche, regles().creation, nullptr, &bouclier), 17)
        << "le bouclier est permis par la formule";
    core::Armor cuir;
    cuir.id = "cuir";
    cuir.category = core::ArmorCategory::Light;
    cuir.baseArmorClass = 11;
    cuir.dexterityBonus = true;
    EXPECT_EQ(core::armorClassFor(fiche, regles().creation, &cuir, nullptr), 13)
        << "sous une armure, la formule sans armure ne joue pas";

    EXPECT_FLOAT_EQ(fiche.effectiveSpeedMeters(), 12.0F) << "9 m + 3 m (Pas de danseur)";
    EXPECT_FLOAT_EQ(fiche.speedInTiles(), 8.0F);

    const std::vector<core::NamedResistance> resistances = core::resistancesFrom(fiche.capacities);
    ASSERT_EQ(resistances.size(), 1U);
    EXPECT_FALSE(resistances[0].type.has_value()) << "a tous les types";
    EXPECT_EQ(resistances[0].source, "Peau de fer");

    const std::vector<core::Modifier> bonus = core::attackModifiersFrom(fiche.capacities);
    ASSERT_EQ(bonus.size(), 1U);
    EXPECT_EQ(bonus[0].source, "Coup precis ameliore");
    EXPECT_EQ(bonus[0].value, 3);

    const std::vector<core::NamedExtraDamage> des = core::extraDamageFrom(fiche.capacities);
    ASSERT_EQ(des.size(), 1U);
    EXPECT_EQ(des[0].dice, (core::Dice{.count = 2, .faces = 6, .modifier = 0}));
    EXPECT_TRUE(des[0].oncePerTurn);
    EXPECT_EQ(des[0].capacityId, "coup-precis-ameliore");

    EXPECT_EQ(core::opportunityImmunityFrom(fiche.capacities),
              std::optional<std::string>("Pas de danseur"));
    EXPECT_EQ(core::armorClassBonusFrom(fiche.capacities), 0);

    core::Weapon gourdin;
    gourdin.id = "gourdin";
    gourdin.category = "simple";
    core::Weapon rapiere;
    rapiere.id = "rapiere";
    rapiere.category = "martial";
    core::Weapon epeeLongue;
    epeeLongue.id = "epee-longue";
    epeeLongue.category = "martial";
    EXPECT_TRUE(core::isProficientWith(fiche, gourdin));
    EXPECT_TRUE(core::isProficientWith(fiche, rapiere));
    EXPECT_FALSE(core::isProficientWith(fiche, epeeLongue));
    EXPECT_TRUE(fiche.armorProficiencies.contains("shields"));
}

/**
 * @brief L'incantation simplifiee : deux lancers par jour et par sort, le repos long les rend.
 * \castest{<b>Un sort mineur se lance a volonte ; un sort de la table se lance deux fois puis plus
 * ; le repos long rend les deux lancers et les points de vie ; monter de niveau apprend un sort
 * sans rendre les lancers depenses.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Fiche au niveau 1 : lire les sorts connus.<br/>2. Depenser trois fois le sort, une
 * fois le sort mineur.<br/>3. Blesser, reposer.<br/>4. Depenser un lancer, monter au niveau 3.<br/>
 * \tattendu etincelle a volonte, trait 2/2 ; troisieme lancer refuse, `available` faux ; apres le
 * repos, 2/2 et PV au maximum ; au niveau 3, second trait connu a 2/2, trait toujours a 1/2.
 * }
 */
TEST(ClassCapacitiesTest, LIncantationSimplifieeCompteLesLancersEtLeReposLesRend) {
    core::LoadedCharacterSheet charge = lutteuse(1);
    ASSERT_TRUE(charge.ok()) << charge.errors.front();
    core::CharacterSheet& fiche = charge.sheet;
    ASSERT_EQ(fiche.knownSpells.size(), 2U);
    EXPECT_EQ(fiche.knownSpells[0],
              (core::KnownSpell{
                  .spellId = "etincelle-d-essai", .level = 0, .perDay = 0, .remaining = 0}));
    EXPECT_EQ(fiche.knownSpells[1],
              (core::KnownSpell{
                  .spellId = "trait-de-feu-d-essai", .level = 1, .perDay = 2, .remaining = 2}));
    EXPECT_TRUE(fiche.knownSpells[0].available());

    EXPECT_TRUE(core::spendSpellUse(fiche, "trait-de-feu-d-essai"));
    EXPECT_TRUE(core::spendSpellUse(fiche, "trait-de-feu-d-essai"));
    EXPECT_FALSE(core::spendSpellUse(fiche, "trait-de-feu-d-essai")) << "epuise";
    EXPECT_FALSE(fiche.knownSpell("trait-de-feu-d-essai")->available());
    EXPECT_TRUE(core::spendSpellUse(fiche, "etincelle-d-essai")) << "a volonte";
    EXPECT_TRUE(fiche.knownSpell("etincelle-d-essai")->available());
    EXPECT_FALSE(core::spendSpellUse(fiche, "boule-de-feu")) << "inconnu";

    fiche.currentHitPoints = 3;
    core::longRest(fiche);
    EXPECT_EQ(fiche.currentHitPoints, fiche.maximumHitPoints);
    EXPECT_EQ(fiche.knownSpell("trait-de-feu-d-essai")->remaining, 2);

    // Monter de niveau n'est pas un repos : le lancer depense reste depense, le nouveau sort
    // arrive au complet.
    EXPECT_TRUE(core::spendSpellUse(fiche, "trait-de-feu-d-essai"));
    const core::CharacterOptions& options = optionsDEssai();
    const core::PlayableClass* classe = options.findClass("lutteur-d-essai");
    ASSERT_NE(classe, nullptr);
    core::gainExperience(fiche, regles().experience, classe->hitDie,
                         regles().experience.thresholdAt(3));
    std::vector<std::string> manquants;
    core::applyClassFeatures(fiche, *classe, options, regles().creation, manquants);
    EXPECT_TRUE(manquants.empty());
    ASSERT_EQ(fiche.knownSpells.size(), 3U);
    EXPECT_EQ(fiche.knownSpell("trait-de-feu-d-essai")->remaining, 1);
    EXPECT_EQ(fiche.knownSpell("second-trait-d-essai")->remaining, 2);
    EXPECT_EQ(fiche.knownSpell("second-trait-d-essai")->level, 2);
}

/**
 * @brief Les quatre classes livrees declarent leurs maitrises et leur incantation (p. 192-206).
 * \castest{<b>Brawler, Mage, Priest et Scoundrel portent les maitrises d'armes et d'armures, les
 * competences au choix et, pour les deux lanceurs, l'incantation simplifiee a deux lancers par jour
 * ; le nain fait maitriser ses quatre armes.</b><br/>
 * \tcat Unitaire · Classes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Charger Source/Elements/Rpg.<br/>2. Lire chaque classe et le nain.<br/>
 * \tattendu Brawler : armes courantes et de guerre, 2 competences parmi 6 ; Mage : cinq armes
 * nommees, aucune armure, Intelligence 2/jour ; Priest : armes courantes, Sagesse 2/jour ;
 * Scoundrel : cuir seul, quatre armes nommees en plus des courantes, 4 competences parmi 11 ; le
 * nain des collines herite du marteau de guerre.
 * }
 */
TEST(ClassCapacitiesTest, LesQuatreClassesDeclarentLeursMaitrises) {
    const core::CharacterOptions options = core::loadCharacterOptions(RPG_LIVRE);
    EXPECT_TRUE(options.errors.empty()) << options.errors.front();

    const core::PlayableClass* brawler = options.findClass("brawler");
    ASSERT_NE(brawler, nullptr);
    EXPECT_TRUE(brawler->isProficientWithWeapon("hache-a-deux-mains", "martial"));
    EXPECT_TRUE(brawler->isProficientWithArmor("medium"));
    EXPECT_FALSE(brawler->isProficientWithArmor("heavy"));
    EXPECT_EQ(brawler->skillChoices.count, 2);
    EXPECT_EQ(brawler->skillChoices.from.size(), 6U);
    EXPECT_FALSE(brawler->spellcasting.has_value());

    const core::PlayableClass* mage = options.findClass("mage");
    ASSERT_NE(mage, nullptr);
    EXPECT_TRUE(mage->isProficientWithWeapon("baton", "simple"));
    EXPECT_FALSE(mage->isProficientWithWeapon("gourdin", "simple"))
        << "cinq armes, pas la categorie";
    EXPECT_TRUE(mage->armorProficiencies.empty());
    ASSERT_TRUE(mage->spellcasting.has_value());
    EXPECT_EQ(mage->spellcasting->ability, core::Ability::Intelligence);
    EXPECT_EQ(mage->spellcasting->castsPerDay, 2);

    const core::PlayableClass* priest = options.findClass("priest");
    ASSERT_NE(priest, nullptr);
    EXPECT_TRUE(priest->isProficientWithWeapon("masse-d-armes", "simple"));
    EXPECT_FALSE(priest->isProficientWithWeapon("marteau-de-guerre", "martial"))
        << "le marteau vient du nain, pas de la classe";
    ASSERT_TRUE(priest->spellcasting.has_value());
    EXPECT_EQ(priest->spellcasting->ability, core::Ability::Wisdom);

    const core::PlayableClass* scoundrel = options.findClass("scoundrel");
    ASSERT_NE(scoundrel, nullptr);
    EXPECT_EQ(scoundrel->armorProficiencies, (std::vector<std::string>{"light"}));
    EXPECT_TRUE(scoundrel->isProficientWithWeapon("rapiere", "martial"));
    EXPECT_FALSE(scoundrel->isProficientWithWeapon("hache-d-armes", "martial"));
    EXPECT_EQ(scoundrel->skillChoices.count, 4);
    EXPECT_EQ(scoundrel->skillChoices.from.size(), 11U);

    const core::Species* nainDesCollines = options.findSpecies("nain-des-collines");
    ASSERT_NE(nainDesCollines, nullptr);
    EXPECT_EQ(nainDesCollines->weaponProficiencies,
              (std::vector<std::string>{"hache-d-armes", "hachette", "marteau-leger",
                                        "marteau-de-guerre"}))
        << "herite du nain";
}
