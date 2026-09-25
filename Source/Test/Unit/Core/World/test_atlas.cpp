// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_atlas.cpp
 * @brief Tests de l'atlas des régions (LOT-37) : les treize régions chargent, le graphe tient, et
 *        trois encarts sont rejoués contre des valeurs RECOPIEES A LA MAIN du Sourcebook.
 *
 * Le dernier point est l'acceptation du lot, et sa forme n'est pas negociable : les notes
 * ci-dessous ont ete lues sur le PDF, page imprimee a l'appui, et non copiees depuis le JSON
 * produit. Un test qui comparerait la sortie de l'extraction a elle-meme passerait quelle que soit
 * la faute -- et la faute a redouter ici est precisement silencieuse : une valeur juste rangee
 * dans le mauvais axe.
 */

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Core/World/Atlas.h"

namespace {

// L'atlas LIVRE, celui que le jeu chargera. Une fixture serait une copie, et une copie cesse de
// prouver quoi que ce soit le jour ou la generation change.
const std::filesystem::path MONDE{JADG_WORLD_DIR};
const std::filesystem::path SCHEMAS{JADG_RPG_SCHEMA_DIR};

// Le chapitre 5 du Sourcebook porte treize encarts << Regional Statistics >>, un par region --
// et non dix, comme la feuille de route l'annoncait de memoire avant que le corpus ne soit lu.
constexpr std::size_t REGIONS_DU_LIVRE = 13;
// 94 « Places of Interest », la Capitale et ses 12 quartiers (LOT-92).
constexpr std::size_t LIEUX_DU_LIVRE = 107;

const core::Atlas& atlas() {
    static const core::Atlas charge = core::loadAtlas(MONDE);
    return charge;
}

/// @brief Une note recopiee a la main du PDF, pour un axe d'une region.
struct NoteDuLivre {
    core::RegionAxis axis;
    core::RegionGrade grade;
};

/**
 * @brief Les treize regions et leurs cent sept lieux se chargent tous.
 * \castest{<b>Les treize regions et leurs cent sept lieux se chargent tous.</b><br/>
 * \tcat Unitaire · Atlas<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Charger Source/Elements/World.<br/>
 * 2. Compter les regions, les lieux et les erreurs rapportees.<br/>
 * \tattendu 13 regions, 94 lieux, aucune erreur.
 * }
 */
TEST(AtlasTest, LesTreizeRegionsEtLeursLieuxChargentSansErreur) {
    EXPECT_TRUE(atlas().errors.empty())
        << "premiere erreur : " << (atlas().errors.empty() ? "" : atlas().errors.front());
    EXPECT_EQ(atlas().regions.size(), REGIONS_DU_LIVRE);
    EXPECT_EQ(atlas().locations.size(), LIEUX_DU_LIVRE);
}

/**
 * @brief Chaque region porte les sept statistiques regionales.
 * \castest{<b>Chaque region porte les sept statistiques regionales.</b><br/>
 * \tcat Unitaire · Atlas<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Pour chaque region, lire les sept axes de l'encart.<br/>
 * \tattendu Aucun axe n'est vide : un axe sans note rendrait la region indistincte des autres.
 * }
 */
TEST(AtlasTest, ChaqueRegionPorteSesSeptAxes) {
    for (const core::Region& region : atlas().regions) {
        for (std::size_t rang = 0; rang < core::kRegionAxisCount; ++rang) {
            const core::RegionStatistic& axe =
                region.statistic(static_cast<core::RegionAxis>(rang));
            EXPECT_FALSE(axe.appraisals.empty())
                << region.id << " : axe "
                << core::regionAxisName(static_cast<core::RegionAxis>(rang))
                << " vide. Un axe sans note rendrait la region indistincte des autres.";
        }
    }
}

/**
 * @brief L'encart de l'Empire central concorde avec le Sourcebook.
 * \castest{<b>L'encart de l'Empire central concorde avec le Sourcebook.</b><br/>
 * \tcat Unitaire · Atlas<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Comparer les sept notes de l'Empire central aux valeurs lues sur la page
 * imprimee 90.<br/>
 * 2. Comparer l'effectif et la part de la premiere espece au bloc d'introduction.<br/>
 * \tattendu Les sept notes, l'effectif et la part concordent ; une valeur rangee dans le mauvais
 * axe fait echouer ce test.
 * }
 */
TEST(AtlasTest, LEncartDeLEmpireCentralEstRejoueDepuisLeLivre) {
    // Tanares_Sourcebook.pdf, page imprimee 90 : Central Empire.
    const core::Region* empire = atlas().findRegion("central-empire");
    ASSERT_NE(empire, nullptr);

    const std::array<NoteDuLivre, core::kRegionAxisCount> attendu{{
        {core::RegionAxis::CitizenFreedom, core::RegionGrade::VeryLow},
        {core::RegionAxis::CrimeAndViolence, core::RegionGrade::Low},
        {core::RegionAxis::EconomicProsperity, core::RegionGrade::VeryHigh},
        {core::RegionAxis::GovernmentCorruption, core::RegionGrade::High},
        {core::RegionAxis::MagicAccess, core::RegionGrade::Low},
        {core::RegionAxis::MonsterPresence, core::RegionGrade::Low},
        {core::RegionAxis::PoliticalStability, core::RegionGrade::High},
    }};
    for (const NoteDuLivre& note : attendu) {
        EXPECT_EQ(empire->statistic(note.axis).grade(), note.grade)
            << "axe " << core::regionAxisName(note.axis);
        EXPECT_FALSE(empire->statistic(note.axis).varies());
    }

    // Meme page, bloc d'introduction : << Around 2,300,000. Humans 84%, gnomes 5%... >>
    ASSERT_TRUE(empire->population.total.has_value());
    EXPECT_EQ(*empire->population.total, 2300000);
    ASSERT_FALSE(empire->population.species.empty());
    EXPECT_EQ(empire->population.species.front().species, "human");
    EXPECT_EQ(empire->population.species.front().percent, 84);
}

/**
 * @brief Une region non uniforme porte ses deux appreciations avec leur portee.
 * \castest{<b>Une region non uniforme porte ses deux appreciations avec leur portee.</b><br/>
 * \tcat Unitaire · Atlas<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire la liberte civique du Benenet imperial et la criminalite des Domaines
 * straviens.<br/>
 * 2. Verifier les deux notes et les deux portees de chacune.<br/>
 * \tattendu Deux appreciations par axe, notes et portees conformes au livre ; aplatir l'une
 * inventerait une donnee.
 * }
 */
TEST(AtlasTest, UneRegionNonUniformePorteSesDeuxAppreciations) {
    // Le livre refuse une note unique a deux regions, et le catalogue doit refuser de meme :
    // << Citizen Freedom : Low (south), High (north) >> pour le Benenet imperial, et
    // << Crime and Violence : Low underground and High on the surface >> pour les Domaines
    // straviens. Aplatir l'une ou l'autre inventerait une donnee que le livre n'ecrit pas.
    const core::Region* benenet = atlas().findRegion("imperial-benenet");
    ASSERT_NE(benenet, nullptr);
    const core::RegionStatistic& liberte = benenet->statistic(core::RegionAxis::CitizenFreedom);
    ASSERT_EQ(liberte.appraisals.size(), 2U);
    EXPECT_TRUE(liberte.varies());
    EXPECT_EQ(liberte.appraisals[0].grade, core::RegionGrade::Low);
    EXPECT_EQ(liberte.appraisals[0].scope, "south");
    EXPECT_EQ(liberte.appraisals[1].grade, core::RegionGrade::High);
    EXPECT_EQ(liberte.appraisals[1].scope, "north");

    const core::Region* stravian = atlas().findRegion("stravian-domains");
    ASSERT_NE(stravian, nullptr);
    const core::RegionStatistic& crime = stravian->statistic(core::RegionAxis::CrimeAndViolence);
    ASSERT_EQ(crime.appraisals.size(), 2U);
    EXPECT_EQ(crime.appraisals[0].grade, core::RegionGrade::Low);
    EXPECT_EQ(crime.appraisals[0].scope, "underground");
    EXPECT_EQ(crime.appraisals[1].grade, core::RegionGrade::High);
    EXPECT_EQ(crime.appraisals[1].scope, "surface");
}

/**
 * @brief Une region uniforme ne nomme aucune portee.
 * \castest{<b>Une region uniforme ne nomme aucune portee.</b><br/>
 * \tcat Unitaire · Atlas<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Pour chaque axe ne portant qu'une appreciation, lire sa portee.<br/>
 * \tattendu La portee est vide : une region uniforme n'a pas de portee a nommer.
 * }
 */
TEST(AtlasTest, UneRegionUniformeNePorteAucunePortee) {
    for (const core::Region& region : atlas().regions) {
        for (std::size_t rang = 0; rang < core::kRegionAxisCount; ++rang) {
            const core::RegionStatistic& axe =
                region.statistic(static_cast<core::RegionAxis>(rang));
            if (axe.appraisals.size() == 1U) {
                EXPECT_TRUE(axe.appraisals.front().scope.empty())
                    << region.id << " : une region uniforme n'a pas de portee a nommer.";
            }
        }
    }
}

/**
 * @brief La Capitale et ses douze quartiers sont des lieux de l'Empire central (LOT-92).
 * \castest{<b>La Capitale et ses douze quartiers sont des lieux de l'Empire central.</b><br/>
 * \tcat Unitaire · Atlas<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Resoudre la Capitale et chacun des douze quartiers, noms recopies du PDF.<br/>
 * 2. Verifier leur region et une phrase de Martpart et d'Arenarea lue sur la page.<br/>
 * \tattendu Treize lieux de central-empire ; Martpart et Arenarea portent le texte du livre.
 * }
 */
TEST(AtlasTest, LaCapitaleEtSesDouzeQuartiersSontRejouesDepuisLeLivre) {
    // Tanares_Sourcebook.pdf, pages imprimees 98-99 : << Districts of the Capital >>, dans l'ordre.
    constexpr std::array<std::string_view, 12> quartiers{
        "sloghood", "uptown",  "artisansquare", "scholarnest", "dweomer",  "martpart",
        "arenarea", "oldtown", "neckoffoods",   "bloomburgs",  "downtown", "palacedomain"};
    const std::string capitale = "central-empire-the-capital-city";
    const core::Location* ville = atlas().findLocation(capitale);
    ASSERT_NE(ville, nullptr);
    EXPECT_EQ(ville->region, "central-empire");
    EXPECT_NE(ville->description.find("Approximately 680,000"), std::string::npos);

    for (std::string_view quartier : quartiers) {
        const std::string id = capitale + "-" + std::string(quartier);
        const core::Location* lieu = atlas().findLocation(id);
        ASSERT_NE(lieu, nullptr) << id;
        EXPECT_EQ(lieu->region, "central-empire") << id;
        EXPECT_FALSE(lieu->description.empty()) << id;
    }

    // Le bloc B de l'atelier des textures lit ces phrases : elles doivent etre celles du livre.
    EXPECT_NE(atlas()
                  .findLocation(capitale + "-martpart")
                  ->description.find(
                      "Lantern-lit stalls and culturally blended architecture adorn cobblestone"),
              std::string::npos);
    EXPECT_NE(atlas()
                  .findLocation(capitale + "-arenarea")
                  ->description.find("Famed for the Arena of Fate"),
              std::string::npos);
}

/**
 * @brief Aucun lieu n'est orphelin de region.
 * \castest{<b>Aucun lieu n'est orphelin de region.</b><br/>
 * \tcat Unitaire · Atlas<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Pour chaque lieu, resoudre la region qu'il declare.<br/>
 * 2. Verifier que cette region le cite en retour.<br/>
 * \tattendu Toute region citee existe et cite le lieu en retour.
 * }
 */
TEST(AtlasTest, AucunLieuSansRegion) {
    for (const core::Location& lieu : atlas().locations) {
        const core::Region* region = atlas().findRegion(lieu.region);
        ASSERT_NE(region, nullptr) << lieu.id << " : region « " << lieu.region << " » inconnue.";
        EXPECT_NE(std::ranges::find(region->locations, lieu.id), region->locations.end())
            << lieu.id << " : la region ne le cite pas en retour.";
    }
}

/**
 * @brief Toute region ne cite que des lieux existants.
 * \castest{<b>Toute region ne cite que des lieux existants.</b><br/>
 * \tcat Unitaire · Atlas<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Pour chaque region, resoudre chacun des lieux qu'elle cite.<br/>
 * \tattendu Chaque lieu cite existe au catalogue.
 * }
 */
TEST(AtlasTest, ToutLieuCiteParUneRegionExiste) {
    for (const core::Region& region : atlas().regions) {
        for (const std::string& identifiant : region.locations) {
            EXPECT_NE(atlas().findLocation(identifiant), nullptr)
                << region.id << " cite le lieu « " << identifiant << " », qui n'existe pas.";
        }
    }
}

/**
 * @brief Le voisinage des regions est symetrique et le graphe connexe.
 * \castest{<b>Le voisinage des regions est symetrique et le graphe connexe.</b><br/>
 * \tcat Unitaire · Atlas<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Pour chaque arete, verifier que la region voisine declare la reciproque.<br/>
 * 2. Parcourir le graphe depuis la premiere region.<br/>
 * \tattendu Aucune arete a sens unique, aucune region inatteignable.
 * }
 */
TEST(AtlasTest, LeVoisinageEstSymetriqueEtLeGrapheConnexe) {
    for (const core::Region& region : atlas().regions) {
        for (const std::string& voisin : region.neighbors) {
            const core::Region* autre = atlas().findRegion(voisin);
            ASSERT_NE(autre, nullptr) << region.id << " : voisin « " << voisin << " » inconnu.";
            EXPECT_NE(std::ranges::find(autre->neighbors, region.id), autre->neighbors.end())
                << "arete a sens unique : " << region.id << " → " << voisin;
        }
    }
    // Une region injoignable est du contenu que personne ne verra jamais, et rien d'autre ne le
    // signale : le jeu se lance, la region existe, elle est simplement au bout d'aucun trajet.
    ASSERT_FALSE(atlas().regions.empty());
    EXPECT_TRUE(atlas().unreachableFrom(atlas().regions.front().id).empty());
}

/**
 * @brief Le controle de connexite sait echouer.
 * \castest{<b>Le controle de connexite sait echouer.</b><br/>
 * \tcat Unitaire · Atlas<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Retirer les aretes de Yama, dans une copie de l'atlas.<br/>
 * 2. Relancer le parcours depuis une autre region.<br/>
 * \tattendu Yama est rapportee inatteignable ; un controle de connexite qu'on n'a jamais vu refuser
 * ne prouve rien.
 * }
 */
TEST(AtlasTest, UneRegionCoupeeDuGrapheEstSignalee) {
    // Le controle ci-dessus ne prouve rien s'il ne sait pas echouer : on coupe une arete a la
    // main et l'on verifie que la region tombe du graphe.
    core::Atlas ampute = atlas();
    ASSERT_GE(ampute.regions.size(), 2U);
    const std::string isolee = "yama";  // une seule arete au livre : Seashores.
    core::Region* cible = nullptr;
    for (core::Region& region : ampute.regions) {
        if (region.id == isolee) {
            cible = &region;
        }
    }
    ASSERT_NE(cible, nullptr);
    for (const std::string& voisin : cible->neighbors) {
        for (core::Region& region : ampute.regions) {
            if (region.id == voisin) {
                std::erase(region.neighbors, isolee);
            }
        }
    }
    cible->neighbors.clear();

    const std::string depart = ampute.regions.front().id;
    ASSERT_NE(depart, isolee);
    const std::vector<std::string> perdues = ampute.unreachableFrom(depart);
    EXPECT_NE(std::ranges::find(perdues, isolee), perdues.end());
}

/**
 * @brief Les notes et les axes du moteur coincident avec region.schema.json.
 * \castest{<b>Les notes et les axes du moteur coincident avec region.schema.json.</b><br/>
 * \tcat Unitaire · Atlas<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire le schema LIVRE.<br/>
 * 2. Y chercher les cinq notes et les sept axes que le moteur nomme.<br/>
 * 3. Verifier qu'une note hors des cinq est refusee.<br/>
 * \tattendu Les douze noms figurent au schema ; << average >> est refuse, le degre median du livre
 * s'ecrivant << normal >>.
 * }
 */
TEST(AtlasTest, LesNotesDuMoteurCoincidentAvecCellesDuSchema) {
    // EX-CNT-011 : le moteur, le contrat et la donnee doivent dire le meme mot. Le schema est lu
    // tel qu'il est LIVRE, pas recopie ici -- une copie divergerait en silence.
    std::ifstream flux{SCHEMAS / "region.schema.json"};
    ASSERT_TRUE(flux.is_open());
    std::ostringstream tampon;
    tampon << flux.rdbuf();
    const std::string schema = tampon.str();

    for (std::size_t rang = 0; rang < core::kRegionGradeCount; ++rang) {
        const std::string_view nom = core::regionGradeName(static_cast<core::RegionGrade>(rang));
        EXPECT_NE(schema.find('"' + std::string{nom} + '"'), std::string::npos)
            << "la note « " << nom << " » du moteur ne figure pas dans region.schema.json.";
        EXPECT_EQ(core::regionGradeFromName(nom),
                  std::optional<core::RegionGrade>{static_cast<core::RegionGrade>(rang)});
    }
    EXPECT_FALSE(core::regionGradeFromName("average").has_value())
        << "« average » n'est pas une note du livre : son degre median s'ecrit « normal ».";

    for (std::size_t rang = 0; rang < core::kRegionAxisCount; ++rang) {
        const std::string_view nom = core::regionAxisName(static_cast<core::RegionAxis>(rang));
        EXPECT_NE(schema.find('"' + std::string{nom} + '"'), std::string::npos)
            << "l'axe « " << nom << " » du moteur ne figure pas dans region.schema.json.";
    }
}

/**
 * @brief Un dossier d'atlas absent produit une erreur, pas un monde vide.
 * \castest{<b>Un dossier d'atlas absent produit une erreur, pas un monde vide.</b><br/>
 * \tcat Unitaire · Atlas<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Charger un dossier qui n'existe pas.<br/>
 * \tattendu Aucune region, et au moins une erreur : un monde vide se confondrait avec un monde non
 * installe.
 * }
 */
TEST(AtlasTest, UnDossierAbsentEstUneErreurPasUnMondeVide) {
    const core::Atlas absent = core::loadAtlas(MONDE / "il-n-y-a-rien-ici");
    EXPECT_TRUE(absent.regions.empty());
    EXPECT_FALSE(absent.errors.empty());
}

/**
 * @brief Les parts de population de chaque region somment a 100 % a l'arrondi pres.
 * \castest{<b>Les parts de population de chaque region somment a 100 % a l'arrondi pres.</b><br/>
 * \tcat Unitaire · Atlas<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Pour chaque region, sommer les parts d'espece et la part << others >>.<br/>
 * \tattendu La somme tombe entre 97 et 103 % ; un ecart plus large signalerait une part perdue en
 * fin de ligne.
 * }
 */
TEST(AtlasTest, LesPartsDePopulationRestentPlausibles) {
    for (const core::Region& region : atlas().regions) {
        int somme = region.population.otherPercent.value_or(0);
        for (const core::RegionSpeciesShare& part : region.population.species) {
            EXPECT_GT(part.percent, 0) << region.id << " / " << part.species;
            EXPECT_FALSE(part.species.empty());
            somme += part.percent;
        }
        // Le livre arrondit ; il ne se trompe pas de dix points. Un ecart plus large signalerait
        // une part perdue en fin de ligne, faute qu'aucun schema ne voit.
        EXPECT_GE(somme, 97) << region.id;
        EXPECT_LE(somme, 103) << region.id;
    }
}

}  // namespace
