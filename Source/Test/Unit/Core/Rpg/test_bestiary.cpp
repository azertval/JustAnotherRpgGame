// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_bestiary.cpp
 * @brief Tests du bestiaire de base (LOT-33) : les 94 profils chargent, et dix sont rejoues
 *        contre des valeurs RECOPIEES A LA MAIN d'Animaux.pdf.
 *
 * Le second point est l'acceptation du lot, et il n'est pas negociable dans sa forme : les
 * valeurs ci-dessous ont ete lues sur le PDF, pas copiees depuis le JSON produit. Un test qui
 * comparerait la sortie de la generation a elle-meme passerait quelle que soit la faute
 * d'extraction -- c'est exactement la panne qu'il doit exclure.
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Rpg/Bestiary.h"
#include "Core/Rpg/Dice.h"
#include "Core/Rpg/RpgEnumNames.h"

namespace {

// Le catalogue LIVRE, celui que le jeu chargera. Une fixture serait une copie, et une copie cesse
// de prouver quoi que ce soit le jour ou la generation change.
const std::filesystem::path CREATURES{JADG_RPG_CREATURES_DIR};

// Les 94 betes du SRD, telles que le sommaire d'Animaux.pdf les compte.
constexpr std::size_t BETES_DU_SRD = 94;

/**
 * @brief Un profil recopie a la main du PDF : page imprimee, et les valeurs de l'acceptation.
 *
 * `damage` et `damageType` portent l'attaque PRINCIPALE, nommee : le libelle est celui du livre,
 * ce qui rend chaque ligne verifiable en ouvrant le document a la page indiquee.
 */
struct ProfilDuLivre {
    const char* id;
    int page;  ///< Page imprimee d'Animaux.pdf.
    int armorClass;
    int hitPoints;
    float walk;
    float challengeRating;
    /// Nom de l'action, tel que la DONNEE l'ecrit -- accent compris. La comparaison est exacte :
    /// un nom approche ne trouverait rien, et le test passerait pour une action absente.
    const char* attaque;
    const char* damage;
    core::DamageType damageType;
};

// Douze profils, deux de plus que les dix exiges : l'acceptation demande dix, et les cas qui
// meritent d'y figurer sont plus nombreux -- un facteur de puissance fractionnaire (1/4, 1/2),
// une vitesse nulle, un mort-vivant, un fielon, un des plus gros et un des plus petits.
constexpr auto PROFILS = std::to_array<ProfilDuLivre>({
    ProfilDuLivre{"eagle", 2, 12, 3, 3.0F, 0.0F, "Serres", "1d4+2", core::DamageType::Slashing},
    ProfilDuLivre{"giant-eagle", 2, 13, 26, 3.0F, 1.0F, "Serres", "2d6+3",
                  core::DamageType::Slashing},
    ProfilDuLivre{"giant-spider", 3, 14, 26, 9.0F, 1.0F, "Morsure", "1d8+3",
                  core::DamageType::Piercing},
    ProfilDuLivre{"elephant", 11, 12, 76, 12.0F, 4.0F, "Défenses", "3d8+6",
                  core::DamageType::Piercing},
    ProfilDuLivre{"lion", 15, 12, 26, 15.0F, 1.0F, "Morsure", "1d8+3", core::DamageType::Piercing},
    ProfilDuLivre{"wolf", 16, 13, 11, 12.0F, 0.25F, "Morsure", "2d4+2", core::DamageType::Piercing},
    ProfilDuLivre{"dire-wolf", 16, 14, 37, 15.0F, 1.0F, "Morsure", "2d6+3",
                  core::DamageType::Piercing},
    ProfilDuLivre{"mammoth", 16, 13, 126, 12.0F, 6.0F, "Défenses", "4d8+7",
                  core::DamageType::Piercing},
    ProfilDuLivre{"brown-bear", 19, 11, 34, 12.0F, 1.0F, "Griffes", "2d6+4",
                  core::DamageType::Slashing},
    ProfilDuLivre{"black-bear", 19, 11, 19, 12.0F, 0.5F, "Griffes", "2d4+2",
                  core::DamageType::Slashing},
    ProfilDuLivre{"boar", 24, 11, 11, 12.0F, 0.25F, "Défenses", "1d6+1",
                  core::DamageType::Slashing},
    ProfilDuLivre{"zombie", 30, 8, 22, 6.0F, 0.25F, "Coup", "1d6+1", core::DamageType::Bludgeoning},
});

[[nodiscard]] const core::Bestiary& bestiaire() {
    // Charge une seule fois : quatre-vingt-quatorze lectures de fichier par cas de test seraient
    // payees a chaque assertion sans rien prouver de plus.
    static const core::Bestiary charge = core::loadBestiary(CREATURES);
    return charge;
}

}  // namespace

/**
 * @brief Les 94 profils du SRD chargent sans une erreur.
 * \castest{<b>Les 94 creatures du bestiaire de base se chargent toutes.</b><br/>
 * \tcat Unitaire · Bestiaire<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger Source/Elements/Rpg/creatures.<br/>
 * 2. Compter les creatures chargees et les erreurs rapportees.<br/>
 * \tattendu 94 creatures, aucune erreur ; chaque identifiant est unique.
 * }
 */
TEST(BestiaryTest, LesQuatreVingtQuatorzeProfilsChargent) {
    const core::Bestiary& catalogue = bestiaire();
    for (const std::string& erreur : catalogue.errors) {
        ADD_FAILURE() << erreur;
    }
    // Les 94 betes du SRD, plus ce que le projet ecrit lui-meme (`original`) : le combattant de
    // l'arene de la demo (LOT-120). Rien d'autre : une provenance inattendue est une extraction
    // egaree dans le mauvais dossier.
    std::size_t srd = 0;
    std::set<std::string> identifiants;
    for (const core::Creature& creature : catalogue.creatures) {
        EXPECT_TRUE(identifiants.insert(creature.id).second)
            << "identifiant en double : " << creature.id;
        EXPECT_FALSE(creature.name.empty()) << creature.id << " : nom vide";
        if (creature.source == "srd") {
            ++srd;
        } else {
            EXPECT_EQ(creature.source, "original") << creature.id << " : provenance inattendue";
        }
    }
    EXPECT_EQ(srd, BETES_DU_SRD) << "le sommaire d'Animaux.pdf en compte " << BETES_DU_SRD;
}

/**
 * @brief Dix profils sont rejoues contre les valeurs du PDF, recopiees a la main.
 * \castest{<b>Dix blocs de statistiques du bestiaire concordent avec le livre.</b><br/>
 * \tcat Unitaire · Bestiaire<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Pour douze creatures, comparer classe d'armure, points de vie, vitesse de marche,
 * facteur de puissance et degats de l'attaque principale aux valeurs lues sur Animaux.pdf.<br/>
 * \tattendu Les cinq valeurs coincident pour chacune ; une extraction decalee d'une ligne fait
 * echouer ce test.
 * }
 */
TEST(BestiaryTest, DixProfilsConcordentAvecLeLivre) {
    const core::Bestiary& catalogue = bestiaire();
    for (const ProfilDuLivre& attendu : PROFILS) {
        const core::Creature* creature = catalogue.find(attendu.id);
        ASSERT_NE(creature, nullptr) << attendu.id << " absent du catalogue";
        const std::string ou =
            std::string{attendu.id} + " (Animaux.pdf p. " + std::to_string(attendu.page) + ")";

        EXPECT_EQ(creature->armorClass, attendu.armorClass) << ou << " : classe d'armure";
        EXPECT_EQ(creature->hitPoints, attendu.hitPoints) << ou << " : points de vie";
        EXPECT_FLOAT_EQ(creature->speed.walk, attendu.walk) << ou << " : vitesse de marche";
        EXPECT_FLOAT_EQ(creature->challengeRating, attendu.challengeRating)
            << ou << " : facteur de puissance";

        const core::CreatureAction* action = creature->action(attendu.attaque);
        ASSERT_NE(action, nullptr) << ou << " : action '" << attendu.attaque << "' absente";
        ASSERT_TRUE(action->damage.has_value()) << ou << " : degats absents";
        const std::optional<core::Dice> des = core::parseDice(attendu.damage);
        ASSERT_TRUE(des.has_value()) << "notation de reference illisible : " << attendu.damage;
        EXPECT_EQ(*action->damage, *des)
            << ou << " : degats de '" << attendu.attaque << "' -- lu "
            << core::formatDice(*action->damage) << ", attendu " << attendu.damage;
        ASSERT_TRUE(action->damageType.has_value()) << ou << " : type de degats absent";
        EXPECT_EQ(*action->damageType, attendu.damageType)
            << ou << " : type de degats -- lu " << core::damageTypeName(*action->damageType)
            << ", attendu " << core::damageTypeName(attendu.damageType);
    }
}

/**
 * @brief Toute creature porte les six caracteristiques et une vitesse de marche.
 * \castest{<b>Aucun profil du bestiaire n'a de caracteristique ni de vitesse manquante.</b><br/>
 * \tcat Unitaire · Bestiaire<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Pour chaque creature, verifier que les six caracteristiques sont dans [1, 30].<br/>
 * 2. Verifier que la vitesse de marche est renseignee, meme nulle.<br/>
 * \tattendu Les 94 profils passent ; une extraction incomplete echoue ici.
 * }
 */
TEST(BestiaryTest, ChaqueProfilEstComplet) {
    for (const core::Creature& creature : bestiaire().creatures) {
        for (const core::Ability caracteristique : core::allAbilities()) {
            const int valeur = creature.ability(caracteristique);
            EXPECT_GE(valeur, 1) << creature.id << " : " << core::abilityName(caracteristique);
            EXPECT_LE(valeur, 30) << creature.id << " : " << core::abilityName(caracteristique);
        }
        // Une creature immobile porte 0 ; ce sont les valeurs ABERRANTES qu'on cherche ici.
        EXPECT_GE(creature.speed.walk, 0.0F) << creature.id << " : vitesse de marche negative";
        EXPECT_GT(creature.hitPoints, 0) << creature.id << " : points de vie nuls";
        EXPECT_GT(creature.armorClass, 0) << creature.id << " : classe d'armure nulle";
    }
}

/**
 * @brief Les creatures a vitesse de marche nulle nagent ou volent.
 * \castest{<b>Une creature sans vitesse de marche possede un autre mode de deplacement.</b><br/>
 * \tcat Unitaire · Bestiaire<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Relever les creatures dont `walk` vaut 0.<br/>
 * \tattendu Chacune porte une vitesse de nage, de vol, d'escalade ou de creusement.
 * }
 */
TEST(BestiaryTest, UneVitesseNulleSignifieUnAutreDeplacement) {
    // `walk: 0` est la donnee de l'epaulard et du requin geant, pas un champ oublie. La preuve
    // que c'est bien la donnee : ces creatures nagent. Une creature a zero PARTOUT serait, elle,
    // une extraction perdue en route.
    std::size_t immobiles = 0;
    for (const core::Creature& creature : bestiaire().creatures) {
        if (creature.speed.walk > 0.0F) {
            continue;
        }
        ++immobiles;
        const bool autrement = creature.speed.swim.has_value() || creature.speed.fly.has_value() ||
                               creature.speed.climb.has_value() ||
                               creature.speed.burrow.has_value();
        EXPECT_TRUE(autrement) << creature.id
                               << " : vitesse de marche nulle et aucun autre deplacement";
    }
    EXPECT_GT(immobiles, 0U) << "aucune creature a vitesse de marche nulle : le livre en porte";
}

/**
 * @brief Les mecanismes exiges par les donnees sont listes au chargement (EX-CNT-031).
 * \castest{<b>Le bestiaire annonce les mecanismes que les creatures exigent du moteur.</b><br/>
 * \tcat Unitaire · Bestiaire<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Collecter les `mecanismesRequis` de toutes les creatures chargees.<br/>
 * \tattendu La liste porte la resistance conditionnelle et la langue comprise sans etre parlee,
 * les deux nuances que les champs types ne peuvent pas exprimer.
 * }
 */
TEST(BestiaryTest, LesMecanismesExigesSontAnnonces) {
    const std::vector<std::string> mecanismes = bestiaire().requiredMechanisms();
    // Ces deux-la sont ecrits par l'extraction chaque fois que le livre dit plus que le schema ne
    // sait porter. Les voir ici, c'est la garantie que le moteur pourra REFUSER de jouer ce qu'il
    // ne sait pas faire, au lieu de le jouer de travers en silence.
    EXPECT_NE(std::ranges::find(mecanismes, "resistance-conditionnelle"), mecanismes.end())
        << "le diablotin resiste aux armes non magiques : la nuance doit etre declaree";
    EXPECT_NE(std::ranges::find(mecanismes, "langue-comprise-non-parlee"), mecanismes.end())
        << "l'aigle geant comprend le commun sans le parler : la nuance doit etre declaree";
}

/**
 * @brief Un dossier absent est signale, jamais confondu avec un bestiaire vide.
 * \castest{<b>Charger un dossier de creatures inexistant produit une erreur nommee.</b><br/>
 * \tcat Unitaire · Bestiaire<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Charger un chemin qui n'existe pas.<br/>
 * \tattendu Aucune creature, une erreur qui nomme le chemin, et aucune exception.
 * }
 */
TEST(BestiaryTest, UnDossierAbsentEstSignale) {
    const core::Bestiary vide = core::loadBestiary(CREATURES / "dossier-inexistant");
    EXPECT_TRUE(vide.creatures.empty());
    ASSERT_EQ(vide.errors.size(), 1U);
    EXPECT_NE(vide.errors.front().find("dossier-inexistant"), std::string::npos)
        << "l'erreur doit nommer le chemin : " << vide.errors.front();
}
