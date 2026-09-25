// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_character_options.cpp
 * @brief Tests des especes, historiques et classes provisoires (LOT-36).
 *
 * L'acceptation du lot tient en quatre phrases, et chacune a son cas ici : trois classes chargent
 * et donnent les bons modificateurs ; la progression du niveau 1 au niveau 5 ne fait intervenir
 * AUCUNE valeur codee en C++ ; tout mecanisme absent du moteur est liste au chargement ; les
 * quatre classes provisoires portent leur marque et aucune donnee definitive ne les reference.
 */

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Rpg/CharacterOptions.h"
#include "Core/Rpg/CharacterSheet.h"
#include "Core/Rpg/RpgEnumNames.h"

namespace {

const std::filesystem::path RPG{JADG_RPG_DIR};

// Les valeurs de reference, RECOPIEES A LA MAIN des livres. Comme pour le bestiaire (LOT-33), un
// test qui comparerait la sortie de la generation a elle-meme passerait quelle que soit la faute
// d'extraction.
struct EspeceDuLivre {
    const char* id;
    const char* source;
    core::CreatureSize size;
    float speed;
    core::Ability augmentee;
    int augmentation;
};

constexpr auto ESPECES = std::to_array<EspeceDuLivre>({
    // Basic Rules p. 22 : << Votre Constitution augmente de 2 >>, << 1,20 m a 1,50 m [...] taille
    // Moyenne >>, << vitesse de base est de 7,50 metres >>.
    {"nain", "srd", core::CreatureSize::Medium, 7.5F, core::Ability::Constitution, 2},
    // Basic Rules p. 14-15 : Dexterite +2, Moyenne, 9 m.
    {"elfe", "srd", core::CreatureSize::Medium, 9.0F, core::Ability::Dexterity, 2},
    // Basic Rules p. 17 : Dexterite +2, Petite, 7,50 m.
    {"halfelin", "srd", core::CreatureSize::Small, 7.5F, core::Ability::Dexterity, 2},
    // Manuel des Joueurs p. 43 : << Votre valeur de Charisme augmente de 2 >>, Moyenne, 9 m.
    {"tieffelin", "phb-fr", core::CreatureSize::Medium, 9.0F, core::Ability::Charisma, 2},
    // Manuel des Joueurs p. 41 (<< TRAITS DES SANGDRAGONS >>) : Force +2, Moyenne, 9 m.
    {"drakeide", "phb-fr", core::CreatureSize::Medium, 9.0F, core::Ability::Strength, 2},
    // Manuel des Joueurs p. 37 : Intelligence +2, Petite, 7,50 m.
    {"gnome", "phb-fr", core::CreatureSize::Small, 7.5F, core::Ability::Intelligence, 2},
    // Player's Guide p. 42 : << Your Constitution score increases by 2 >>, Medium, 30 feet = 9 m.
    {"gloomfolk", "tanares", core::CreatureSize::Medium, 9.0F, core::Ability::Constitution, 2},
});

struct ClasseDuLivre {
    const char* id;
    int hitDie;
    core::Ability primaire;
    core::Ability sauvegarde1;
    core::Ability sauvegarde2;
};

// Player's Guide p. 57, table << New Simplified Classes >>.
constexpr auto CLASSES = std::to_array<ClasseDuLivre>({
    {"brawler", 12, core::Ability::Strength, core::Ability::Strength, core::Ability::Constitution},
    {"mage", 6, core::Ability::Intelligence, core::Ability::Intelligence, core::Ability::Wisdom},
    {"priest", 8, core::Ability::Wisdom, core::Ability::Wisdom, core::Ability::Charisma},
    {"scoundrel", 8, core::Ability::Dexterity, core::Ability::Dexterity,
     core::Ability::Intelligence},
});

[[nodiscard]] const core::CharacterOptions& options() {
    static const core::CharacterOptions charge =
        core::loadCharacterOptions(RPG / "species", RPG / "backgrounds", RPG / "classes");
    return charge;
}

}  // namespace

/**
 * @brief Les trois catalogues chargent sans une erreur.
 * \castest{<b>Les especes, historiques et classes livres se chargent tous.</b><br/>
 * \tcat Unitaire · Options de personnage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger species/, backgrounds/ et classes/.<br/>
 * \tattendu Aucune erreur ; chaque entree a un identifiant unique, un nom et une provenance.
 * }
 */
TEST(CharacterOptionsTest, LesTroisCataloguesChargent) {
    const core::CharacterOptions& lues = options();
    for (const std::string& erreur : lues.errors) {
        ADD_FAILURE() << erreur;
    }
    EXPECT_FALSE(lues.species.empty()) << "aucune espece chargee";
    EXPECT_FALSE(lues.backgrounds.empty()) << "aucun historique charge";
    EXPECT_EQ(lues.classes.size(), CLASSES.size()) << "quatre classes simplifiees attendues";

    std::set<std::string> identifiants;
    for (const core::Species& espece : lues.species) {
        EXPECT_TRUE(identifiants.insert(espece.id).second) << "espece en double : " << espece.id;
        EXPECT_FALSE(espece.name.empty()) << espece.id << " : nom vide";
        EXPECT_GT(espece.speed, 0.0F) << espece.id << " : vitesse nulle";
    }
}

/**
 * @brief Sept especes concordent avec les valeurs recopiees des livres.
 * \castest{<b>Les especes livrees portent les valeurs des livres dont elles sont tirees.</b><br/>
 * \tcat Unitaire · Options de personnage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Pour sept especes des trois livres, comparer provenance, taille, vitesse et
 * augmentation de caracteristique aux valeurs lues sur les documents.<br/>
 * \tattendu Les quatre valeurs coincident pour chacune.
 * }
 */
TEST(CharacterOptionsTest, LesEspecesConcordentAvecLesLivres) {
    for (const EspeceDuLivre& attendue : ESPECES) {
        const core::Species* espece = options().findSpecies(attendue.id);
        ASSERT_NE(espece, nullptr) << attendue.id << " absente du catalogue";
        EXPECT_EQ(espece->source, attendue.source) << attendue.id << " : provenance";
        EXPECT_EQ(espece->size, attendue.size)
            << attendue.id << " : taille — lue " << core::creatureSizeName(espece->size);
        EXPECT_FLOAT_EQ(espece->speed, attendue.speed) << attendue.id << " : vitesse";
        EXPECT_EQ(espece->increase(attendue.augmentee), attendue.augmentation)
            << attendue.id << " : augmentation de " << core::abilityName(attendue.augmentee);
    }
}

/**
 * @brief Une augmentation d'espece s'applique a une valeur de caracteristique.
 * \castest{<b>L'augmentation d'une espece s'applique et reste bornee a 20.</b><br/>
 * \tcat Unitaire · Options de personnage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Appliquer l'augmentation du nain a une Constitution de 14, puis de 19.<br/>
 * \tattendu 16, puis 20 — le plafond n'est pas franchi.
 * }
 */
TEST(CharacterOptionsTest, LAugmentationSAppliqueEtResteBornee) {
    const core::Species* nain = options().findSpecies("nain");
    ASSERT_NE(nain, nullptr);
    // Le plafond vient de la DONNEE (LOT-13), pas d'une constante : le test le lit la ou le
    // moteur le lira, sinon il verifierait sa propre copie de la regle.
    const core::CharacterCreationRules regles =
        core::loadCharacterCreationRules(RPG / "rules" / "character-creation.json");
    ASSERT_TRUE(regles.ok());
    const int plafond = regles.maximumAbilityScore;
    EXPECT_EQ(core::abilityScoreWith(*nain, core::Ability::Constitution, 14, plafond), 16);
    EXPECT_EQ(core::abilityScoreWith(*nain, core::Ability::Constitution, plafond - 1, plafond),
              plafond)
        << "une augmentation d'espece ne franchit pas le plafond";
    EXPECT_EQ(core::abilityScoreWith(*nain, core::Ability::Charisma, 14, plafond), 14)
        << "le nain n'augmente pas le Charisme";
}

/**
 * @brief Les quatre classes concordent avec la table du Player's Guide.
 * \castest{<b>Les classes simplifiees portent le de de vie et les sauvegardes du livre.</b><br/>
 * \tcat Unitaire · Options de personnage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Pour les quatre classes, comparer de de vie, caracteristique principale et jets de
 * sauvegarde a la table << New Simplified Classes >> de la page 57.<br/>
 * \tattendu Les trois valeurs coincident, et chaque classe porte exactement deux sauvegardes.
 * }
 */
TEST(CharacterOptionsTest, LesClassesConcordentAvecLeLivre) {
    for (const ClasseDuLivre& attendue : CLASSES) {
        const core::PlayableClass* classe = options().findClass(attendue.id);
        ASSERT_NE(classe, nullptr) << attendue.id << " absente du catalogue";
        EXPECT_EQ(classe->hitDie, attendue.hitDie) << attendue.id << " : de de vie";
        ASSERT_FALSE(classe->primaryAbility.empty()) << attendue.id;
        EXPECT_EQ(classe->primaryAbility.front(), attendue.primaire)
            << attendue.id << " : caracteristique principale";
        ASSERT_EQ(classe->savingThrowProficiencies.size(), 2U)
            << attendue.id << " : deux jets de sauvegarde, ni plus ni moins";
        EXPECT_EQ(classe->savingThrowProficiencies[0], attendue.sauvegarde1) << attendue.id;
        EXPECT_EQ(classe->savingThrowProficiencies[1], attendue.sauvegarde2) << attendue.id;
    }
}

/**
 * @brief La progression du niveau 1 au niveau 5 vient de la DONNEE, pas du C++.
 * \castest{<b>Le bonus de maitrise des niveaux 1 a 5 est lu dans la table livree.</b><br/>
 * \tcat Unitaire · Options de personnage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire le bonus de maitrise des niveaux 1 a 5 de chaque classe dans sa table.<br/>
 * 2. Le comparer aux valeurs de la table du livre.<br/>
 * \tattendu +2 du niveau 1 au 4, +3 au niveau 5, pour les quatre classes ; les vingt niveaux sont
 * presents et se suivent sans trou.
 * }
 */
TEST(CharacterOptionsTest, LaProgressionVientDeLaDonnee) {
    // Ces cinq valeurs sont celles des tables du livre, pas une formule. La regle generale
    // ((niveau + 7) / 4) donnerait le meme resultat ici, et c'est precisement le piege : l'ecrire
    // en C++ ferait cesser de lire la donnee, et la premiere classe dont la progression sort de
    // l'ordinaire -- le LOT-84 en annonce trente et une -- passerait inapercue.
    constexpr std::array<int, 5> BONUS_ATTENDUS = {2, 2, 2, 2, 3};
    for (const ClasseDuLivre& attendue : CLASSES) {
        const core::PlayableClass* classe = options().findClass(attendue.id);
        ASSERT_NE(classe, nullptr) << attendue.id;
        EXPECT_EQ(classe->progression.size(), 20U) << attendue.id << " : vingt niveaux attendus";
        for (int niveau = 1; niveau <= 5; ++niveau) {
            const core::ClassLevel* ligne = classe->atLevel(niveau);
            ASSERT_NE(ligne, nullptr) << attendue.id << " : niveau " << niveau << " absent";
            EXPECT_EQ(ligne->proficiencyBonus, BONUS_ATTENDUS[static_cast<std::size_t>(niveau - 1)])
                << attendue.id << " : bonus de maitrise au niveau " << niveau;
            EXPECT_FALSE(ligne->features.empty())
                << attendue.id << " : le niveau " << niveau << " n'apporte aucune capacite";
        }
        for (std::size_t i = 0; i < classe->progression.size(); ++i) {
            EXPECT_EQ(classe->progression[i].level, static_cast<int>(i) + 1)
                << attendue.id << " : la table saute un niveau";
        }
    }
}

/**
 * @brief Les mecanismes exiges par les donnees sont listes au chargement (EX-CNT-031).
 * \castest{<b>Une espece qui exige un mecanisme absent le declare au chargement.</b><br/>
 * \tcat Unitaire · Options de personnage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Collecter les `mecanismesRequis` des especes chargees.<br/>
 * \tattendu L'augmentation de caracteristique au choix du joueur y figure : les especes de
 * Tanares la laissent au joueur, et la table du schema ne sait pas la porter.
 * }
 */
TEST(CharacterOptionsTest, LesMecanismesExigesSontAnnonces) {
    const std::vector<std::string> mecanismes = options().requiredMechanisms();
    EXPECT_NE(std::ranges::find(mecanismes, "augmentation-de-caracteristique-au-choix"),
              mecanismes.end())
        << "les especes de Tanares laissent une augmentation au choix du joueur : la nuance doit "
           "etre declaree, jamais tranchee a sa place";
}

/**
 * @brief Les quatre classes sont marquees provisoires, et rien de definitif ne les reference.
 * \castest{<b>Les classes provisoires portent leur marque et ne sont referencees par aucune
 * donnee definitive.</b><br/>
 * \tcat Unitaire · Options de personnage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Verifier que les quatre classes portent `status.provisoire` et un critere de
 * retrait.<br/>
 * 2. Balayer tous les fichiers de `Source/Elements/Rpg/` et chercher leur identifiant.<br/>
 * \tattendu Aucune donnee definitive ne les cite : le jour du retrait, supprimer ces quatre
 * fichiers ne casse rien.
 * }
 */
TEST(CharacterOptionsTest, LesClassesProvisoiresNeSontReferenceesParRien) {
    const std::vector<std::string> provisoires = options().provisionalClassIds();
    EXPECT_EQ(provisoires.size(), CLASSES.size())
        << "les quatre classes simplifiees sont provisoires et doivent le declarer";
    for (const core::PlayableClass& classe : options().classes) {
        EXPECT_TRUE(classe.status.provisional) << classe.id << " : marque provisoire absente";
        EXPECT_FALSE(classe.status.removalCriterion.empty())
            << classe.id
            << " : critere de retrait absent. Une donnee provisoire sans critere "
               "ecrit d'avance devient permanente par accident (EX-CNT-032).";
    }

    // Le controle qui donne son sens au critere de retrait : si une espece, un historique ou une
    // creature citait << brawler >>, supprimer le fichier le jour des seize classes casserait une
    // reference que personne n'aurait vue venir.
    std::error_code code;
    for (const auto& entree : std::filesystem::recursive_directory_iterator(RPG, code)) {
        if (!entree.is_regular_file(code) || entree.path().extension() != ".json") {
            continue;
        }
        if (entree.path().parent_path().filename() == "classes" ||
            entree.path().parent_path().filename() == "schema") {
            continue;
        }
        std::ifstream fichier(entree.path());
        const std::string contenu((std::istreambuf_iterator<char>(fichier)),
                                  std::istreambuf_iterator<char>());
        // Une donnee elle-meme PROVISOIRE a le droit d'en citer une autre : elle porte son propre
        // critere de retrait (EX-CNT-032), et disparait donc avec ce qu'elle reference. C'est le
        // cas du heros de la demo (LOT-112, apres le personnage de demonstration du LOT-38), dont
        // la classe est forcement l'une des quatre provisoires -- ce sont les seules qui existent.
        // Le controle garde tout son sens pour les donnees DEFINITIVES, qui sont son sujet.
        if (contenu.find("\"provisoire\": true") != std::string::npos) {
            continue;
        }
        for (const std::string& identifiant : provisoires) {
            EXPECT_EQ(contenu.find('"' + identifiant + '"'), std::string::npos)
                << entree.path().filename().string() << " reference la classe provisoire '"
                << identifiant << "'";
        }
    }
}

/**
 * @brief Un dossier absent est signale, jamais confondu avec un catalogue vide.
 * \castest{<b>Charger un dossier d'options inexistant produit une erreur nommee.</b><br/>
 * \tcat Unitaire · Options de personnage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Charger trois chemins qui n'existent pas.<br/>
 * \tattendu Trois erreurs nommant les chemins, aucune entree, aucune exception.
 * }
 */
TEST(CharacterOptionsTest, UnDossierAbsentEstSignale) {
    const core::CharacterOptions vides =
        core::loadCharacterOptions(RPG / "absent-1", RPG / "absent-2", RPG / "absent-3");
    EXPECT_TRUE(vides.species.empty());
    EXPECT_TRUE(vides.backgrounds.empty());
    EXPECT_TRUE(vides.classes.empty());
    EXPECT_EQ(vides.errors.size(), 3U);
}

/**
 * @brief Chaque historique accorde des competences que le catalogue du LOT-43 connait.
 * \castest{<b>Les competences citees par un historique existent au catalogue.</b><br/>
 * \tcat Unitaire · Options de personnage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Pour chaque historique, verifier que le fichier de chaque competence citee existe
 * dans `Source/Elements/Rpg/skills/`.<br/>
 * \tattendu Toutes existent : une competence inventee ferait refuser une maitrise sans que rien
 * ne dise pourquoi.
 * }
 */
TEST(CharacterOptionsTest, LesHistoriquesCitentDesCompetencesQuiExistent) {
    for (const core::Background& historique : options().backgrounds) {
        EXPECT_FALSE(historique.skillProficiencies.empty())
            << historique.id << " : aucun historique du corpus n'accorde zero competence";
        for (const std::string& competence : historique.skillProficiencies) {
            const std::filesystem::path chemin = RPG / "skills" / (competence + ".json");
            EXPECT_TRUE(std::filesystem::exists(chemin))
                << historique.id << " cite la competence '" << competence
                << "', absente du catalogue du LOT-43";
        }
        EXPECT_GE(historique.languageCount, 0);
    }
}
