// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_arena_appearance_catalog.cpp
 * @brief Tests unitaires du catalogue d'apparence du Colisée (Phase 2 du LOT-86 : rôle de case et
 *        figurine, portés en `HMI` pur depuis `ArenaTile.ui.qml`).
 */

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Combat/TurnOrder.h"
#include "Core/Levels/GridPosition.h"
#include "HMI/Graphics/ArenaAppearanceCatalog.h"

namespace {

/// Catalogue de reference : quatre figurines par cote, cinq dalles claires.
constexpr const char* REFERENCE_JSON = R"({
  "heroes": ["kaelith_voss", "bram", "elira", "darin"],
  "gladiators": ["gladiator_sword_shield", "gladiator_lance", "retiarius", "archer"],
  "scene": "coliseum",
  "paleSlabs": ["01", "02", "03", "04", "05", "10", "11", "13", "14", "15"],
  "heroFrames": 5,
  "enemyFrames": 8
})";

hmi::ArenaAppearanceCatalog referenceCatalog() {
    hmi::ArenaAppearanceCatalogResult result =
        hmi::ArenaAppearanceCatalog::loadFromString(REFERENCE_JSON);
    EXPECT_TRUE(result.ok()) << result.error;
    return result.ok() ? *result.catalog : hmi::ArenaAppearanceCatalog{};
}

}  // namespace

/**
 * @brief Le catalogue de reference se lit sans erreur, avec ses quatre listes au complet.
 * \castest{<b>Le catalogue de reference se lit sans erreur.</b><br/>
 * \tcat Unitaire · Catalogue d'apparence de l'arene<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire REFERENCE_JSON.<br/>
 * \tattendu Succes ; quatre heros, quatre gladiateurs, dix dalles, 5 et 8 images.
 * }
 */
TEST(ArenaAppearanceCatalogTest, LectureDeReference) {
    const hmi::ArenaAppearanceCatalog catalog = referenceCatalog();
    EXPECT_EQ(catalog.heroes().size(), 4U);
    EXPECT_EQ(catalog.gladiators().size(), 4U);
    EXPECT_EQ(catalog.paleSlabs().size(), 10U);
    EXPECT_EQ(catalog.heroFrames(), 5);
    EXPECT_EQ(catalog.enemyFrames(), 8);
}

/**
 * @brief Un champ absent, vide ou mal type est refuse, en nommant le champ en cause.
 * \castest{<b>Un catalogue dont un champ obligatoire manque, est vide ou mal type est
 * refuse.</b><br/>
 * \tcat Unitaire · Catalogue d'apparence de l'arene<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire des variantes du JSON de reference, chacune avec un defaut.<br/>
 * \tattendu Chaque lecture echoue avec `MalformedStructure`.
 * }
 */
TEST(ArenaAppearanceCatalogTest, ChampsInvalidesRefuses) {
    const std::vector<std::string> invalides = {
        R"({"gladiators": ["a"], "paleSlabs": ["01"], "heroFrames": 1, "enemyFrames": 1})",
        R"({"heroes": ["a"], "gladiators": ["a"], "paleSlabs": ["01"], "heroFrames": 1, "enemyFrames": 1})",
        R"({"scene": "", "heroes": ["a"], "gladiators": ["a"], "paleSlabs": ["01"], "heroFrames": 1, "enemyFrames": 1})",
        R"({"scene": "coliseum", "heroes": [], "gladiators": ["a"], "paleSlabs": ["01"], "heroFrames": 1, "enemyFrames": 1})",
        R"({"scene": "coliseum", "heroes": ["a"], "gladiators": ["a"], "paleSlabs": ["01"], "heroFrames": 0, "enemyFrames": 1})",
        R"({"scene": "coliseum", "heroes": ["a"], "gladiators": ["a"], "paleSlabs": ["01"], "heroFrames": "5", "enemyFrames": 1})",
        R"({"scene": "coliseum", "heroes": [1], "gladiators": ["a"], "paleSlabs": ["01"], "heroFrames": 1, "enemyFrames": 1})",
    };
    for (const std::string& json : invalides) {
        const hmi::ArenaAppearanceCatalogResult result =
            hmi::ArenaAppearanceCatalog::loadFromString(json);
        EXPECT_FALSE(result.ok()) << json;
        EXPECT_EQ(result.errorCode, hmi::ArenaAppearanceError::MalformedStructure) << json;
    }
}

/**
 * @brief Les quatre angles d'une grille portent une colonne, jamais une banniere ou une torche.
 * \castest{<b>Sur une grille 10 x 8 murée, les quatre angles portent
 * `WallFeature::Corner`.</b><br/>
 * \tcat Unitaire · Catalogue d'apparence de l'arene<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire le role des quatre coins, wall = vrai.<br/>
 * \tattendu `Corner` pour chacun, `wall` vrai, ni banniere ni porte.
 * }
 */
TEST(ArenaAppearanceCatalogTest, LesAnglesPortentUneColonne) {
    const hmi::ArenaAppearanceCatalog catalog = referenceCatalog();
    constexpr int COLUMNS = 10;
    constexpr int ROWS = 8;
    for (const core::GridPosition coin :
         {core::GridPosition{0, 0}, core::GridPosition{COLUMNS - 1, 0},
          core::GridPosition{0, ROWS - 1}, core::GridPosition{COLUMNS - 1, ROWS - 1}}) {
        const hmi::ArenaTileAppearance role = catalog.tileAppearance(coin, COLUMNS, ROWS, true);
        EXPECT_TRUE(role.wall);
        EXPECT_EQ(role.wallFeature, hmi::WallFeature::Corner);
        EXPECT_FALSE(role.gateSpot);
    }
}

/**
 * @brief La banniere retombe tous les cinq pas sur les bords haut et bas, hors angle.
 * \castest{<b>Sur une grille 10 x 8, la banniere est aux colonnes multiples de 5 des bords haut et
 * bas, hors angle ; les autres cases de ces bords sont un pan de mur ordinaire.</b><br/>
 * \tcat Unitaire · Catalogue d'apparence de l'arene<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire le role de chaque case murée des lignes 0 et 7.<br/>
 * \tattendu `BannerSpot` en colonnes 0, 5 sauf l'angle (colonne 0) ; `Plain` ailleurs.
 * }
 */
TEST(ArenaAppearanceCatalogTest, LaBanniereTousLesCinqPas) {
    const hmi::ArenaAppearanceCatalog catalog = referenceCatalog();
    constexpr int COLUMNS = 10;
    constexpr int ROWS = 8;
    for (const int row : {0, ROWS - 1}) {
        for (int column = 1; column < COLUMNS - 1; ++column) {
            const hmi::ArenaTileAppearance role =
                catalog.tileAppearance({column, row}, COLUMNS, ROWS, true);
            const hmi::WallFeature attendu =
                column % 5 == 0 ? hmi::WallFeature::BannerSpot : hmi::WallFeature::Plain;
            EXPECT_EQ(role.wallFeature, attendu) << "colonne " << column << ", ligne " << row;
        }
    }
}

/**
 * @brief La torche retombe tous les quatre pas sur les bords gauche et droit, hors angle.
 * \castest{<b>Sur une grille 10 x 8, la torche est aux lignes valant 2 modulo 4 des bords gauche et
 * droit, hors angle ; les autres cases de ces bords sont un pan de mur ordinaire.</b><br/>
 * \tcat Unitaire · Catalogue d'apparence de l'arene<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire le role de chaque case muree des colonnes 0 et 9.<br/>
 * \tattendu `TorchSpot` aux lignes 2 et 6 ; `Plain` ailleurs.
 * }
 */
TEST(ArenaAppearanceCatalogTest, LaTorcheTousLesQuatrePas) {
    const hmi::ArenaAppearanceCatalog catalog = referenceCatalog();
    constexpr int COLUMNS = 10;
    constexpr int ROWS = 8;
    for (const int column : {0, COLUMNS - 1}) {
        for (int row = 1; row < ROWS - 1; ++row) {
            const hmi::ArenaTileAppearance role =
                catalog.tileAppearance({column, row}, COLUMNS, ROWS, true);
            const hmi::WallFeature attendu =
                row % 4 == 2 ? hmi::WallFeature::TorchSpot : hmi::WallFeature::Plain;
            EXPECT_EQ(role.wallFeature, attendu) << "colonne " << column << ", ligne " << row;
        }
    }
}

/**
 * @brief Une case libre du bord est une porte ; une case libre interieure ne l'est pas.
 * \castest{<b>Sur une grille 10 x 8, toute case non muree du bord porte `gateSpot`, aucune case
 * interieure ne le porte.</b><br/>
 * \tcat Unitaire · Catalogue d'apparence de l'arene<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire le role de la case (0, 3) et de la case (4, 4), wall = faux.<br/>
 * \tattendu `gateSpot` vrai pour (0, 3), faux pour (4, 4).
 * }
 */
TEST(ArenaAppearanceCatalogTest, LaPorteEstSurLeBordSeulement) {
    const hmi::ArenaAppearanceCatalog catalog = referenceCatalog();
    constexpr int COLUMNS = 10;
    constexpr int ROWS = 8;

    const hmi::ArenaTileAppearance bord = catalog.tileAppearance({0, 3}, COLUMNS, ROWS, false);
    EXPECT_TRUE(bord.gateSpot);
    EXPECT_FALSE(bord.wall);

    const hmi::ArenaTileAppearance interieur = catalog.tileAppearance({4, 4}, COLUMNS, ROWS, false);
    EXPECT_FALSE(interieur.gateSpot);
}

/**
 * @brief La dalle claire suit exactement la formule de `ArenaTile.ui.qml:84,88-89`.
 * \castest{<b>Le sol d'une case non muree porte une dalle quand
 * `(colonne*3 + ligne*5 + colonne*ligne) % 7 == 0`, a l'indice `(colonne*3 + ligne*5) %
 * 10`.</b><br/>
 * \tcat Unitaire · Catalogue d'apparence de l'arene<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Calculer le role de chaque case interieure d'une grille 10 x 8.<br/>2. Comparer a la
 * formule transcrite de la scene QML.<br/>
 * \tattendu `slab` et `slabVariant` concordent case par case, sur toute la grille.
 * }
 */
TEST(ArenaAppearanceCatalogTest, LaDalleSuitLaFormuleQml) {
    const hmi::ArenaAppearanceCatalog catalog = referenceCatalog();
    constexpr int COLUMNS = 10;
    constexpr int ROWS = 8;
    for (int row = 0; row < ROWS; ++row) {
        for (int column = 0; column < COLUMNS; ++column) {
            const hmi::ArenaTileAppearance role =
                catalog.tileAppearance({column, row}, COLUMNS, ROWS, false);
            const bool slabAttendu = (column * 3 + row * 5 + column * row) % 7 == 0;
            EXPECT_EQ(role.slab, slabAttendu) << "colonne " << column << ", ligne " << row;
            if (slabAttendu) {
                EXPECT_EQ(role.slabVariant, (column * 3 + row * 5) % 10)
                    << "colonne " << column << ", ligne " << row;
            }
        }
    }
}

/**
 * @brief La figurine d'un combattant est deterministe et depend de son cote.
 * \castest{<b>Deux appels avec le meme nom et le meme cote rendent la meme figurine ; un allie
 * choisit parmi les heros, un ennemi parmi les gladiateurs.</b><br/>
 * \tcat Unitaire · Catalogue d'apparence de l'arene<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Demander la figurine de « Gorlak » comme allie, puis comme ennemi, deux fois
 * chacune.<br/>
 * \tattendu Meme resultat aux deux appels d'un meme cote ; l'allie vient de `heroes()`, l'ennemi de
 * `gladiators()` ; le nombre d'images suit `heroFrames`/`enemyFrames`.
 * }
 */
TEST(ArenaAppearanceCatalogTest, LaFigurineEstDeterministeEtDependDuCote) {
    const hmi::ArenaAppearanceCatalog catalog = referenceCatalog();

    const hmi::FigureAppearance allie1 = catalog.figureFor("Gorlak", core::CombatSide::Allies);
    const hmi::FigureAppearance allie2 = catalog.figureFor("Gorlak", core::CombatSide::Allies);
    EXPECT_EQ(allie1.sheet, allie2.sheet);
    EXPECT_NE(std::find(catalog.heroes().begin(), catalog.heroes().end(), allie1.sheet),
              catalog.heroes().end());
    EXPECT_EQ(allie1.frameCount, catalog.heroFrames());

    const hmi::FigureAppearance ennemi1 = catalog.figureFor("Gorlak", core::CombatSide::Enemies);
    const hmi::FigureAppearance ennemi2 = catalog.figureFor("Gorlak", core::CombatSide::Enemies);
    EXPECT_EQ(ennemi1.sheet, ennemi2.sheet);
    EXPECT_NE(std::find(catalog.gladiators().begin(), catalog.gladiators().end(), ennemi1.sheet),
              catalog.gladiators().end());
    EXPECT_EQ(ennemi1.frameCount, catalog.enemyFrames());
}

/**
 * @brief La figurine suit exactement la formule de `ArenaTile.ui.qml:73-74`.
 * \castest{<b>Pour plusieurs noms, l'indice de figurine vaut
 * `(longueur*7 + code du premier caractere) % taille du roster`.</b><br/>
 * \tcat Unitaire · Catalogue d'apparence de l'arene<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Calculer la figurine attendue pour plusieurs noms.<br/>2. Comparer a la formule
 * transcrite de la scene QML.<br/>
 * \tattendu Concordance pour chaque nom, des deux cotes.
 * }
 */
TEST(ArenaAppearanceCatalogTest, LaFigurineSuitLaFormuleQml) {
    const hmi::ArenaAppearanceCatalog catalog = referenceCatalog();
    for (const std::string& nom : {std::string("Aldric"), std::string("B"),
                                   std::string("Zephyrine"), std::string("kaelith_voss")}) {
        const int indiceAttendu =
            (static_cast<int>(nom.size()) * 7 + static_cast<unsigned char>(nom.front())) %
            static_cast<int>(catalog.heroes().size());
        EXPECT_EQ(catalog.figureFor(nom, core::CombatSide::Allies).sheet,
                  catalog.heroes()[static_cast<std::size_t>(indiceAttendu)])
            << nom;
    }
}

/**
 * @brief Un nom vide ne fait pas planter la selection, et rend la premiere figurine.
 * \castest{<b>Un combattant sans nom rend la figurine d'indice 0.</b><br/>
 * \tcat Unitaire · Catalogue d'apparence de l'arene<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Demander la figurine d'un nom vide, des deux cotes.<br/>
 * \tattendu La premiere entree de `heroes()`/`gladiators()`.
 * }
 */
TEST(ArenaAppearanceCatalogTest, NomVideRendLaPremiereFigurine) {
    const hmi::ArenaAppearanceCatalog catalog = referenceCatalog();
    EXPECT_EQ(catalog.figureFor("", core::CombatSide::Allies).sheet, catalog.heroes().front());
    EXPECT_EQ(catalog.figureFor("", core::CombatSide::Enemies).sheet, catalog.gladiators().front());
}

/**
 * @brief Le manifeste d'un kit d'arene se lit sans erreur, avec ses cinq champs au complet.
 * \castest{<b>Le manifeste du kit d'arene d'essai se lit sans erreur.</b><br/>
 * \tcat Unitaire · Catalogue d'apparence de l'arene<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire le manifeste livre depuis les sources.<br/>
 * \tattendu Succes ; heros, gladiateurs et dalles non vides.
 * }
 */
TEST(ArenaAppearanceCatalogTest, ManifesteDUnKitValide) {
    const std::filesystem::path path =
        std::filesystem::path(JADG_TEST_DATA_DIR) / "Assets" / "Arena" / "manifest.json";
    ASSERT_TRUE(std::filesystem::exists(path)) << path.string();

    const hmi::ArenaAppearanceCatalogResult result =
        hmi::ArenaAppearanceCatalog::loadFromFile(path);
    ASSERT_TRUE(result.ok()) << result.error;

    EXPECT_FALSE(result.catalog->heroes().empty());
    EXPECT_FALSE(result.catalog->gladiators().empty());
    EXPECT_FALSE(result.catalog->paleSlabs().empty());
    EXPECT_GT(result.catalog->heroFrames(), 0);
    EXPECT_GT(result.catalog->enemyFrames(), 0);
}

/**
 * @brief Un heros remplace lit ses bandes ailleurs, sans changer de nom ni de place au roster.
 * \castest{<b>`replaceHero` change le dossier rendu par `figureFor`/`sheetDirectory`, rien
 * d'autre.</b><br/>
 * \tcat Unitaire · Catalogue d'apparence de l'arene<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire le dossier par defaut d'un heros et d'un gladiateur.<br/>
 *          2. Remplacer ce heros par `../Npc/anariel`, puis tenter un nom inconnu.<br/>
 * \tattendu `characters/<heros>` et `enemies/<gladiateur>` avant ; apres, le heros garde son nom
 * et son rang mais son dossier vaut `../Npc/anariel` ; le nom inconnu est refuse sans effet.
 * }
 */
TEST(ArenaAppearanceCatalogTest, UnHerosRemplaceLitSesBandesAilleurs) {
    hmi::ArenaAppearanceCatalog catalog = referenceCatalog();
    const hmi::FigureAppearance avant = catalog.figureFor("Gorlak", core::CombatSide::Allies);
    EXPECT_EQ(avant.directory, "characters/" + avant.sheet);
    EXPECT_EQ(catalog.figureFor("Gorlak", core::CombatSide::Enemies).directory,
              "enemies/" + catalog.figureFor("Gorlak", core::CombatSide::Enemies).sheet);

    EXPECT_TRUE(catalog.replaceHero(avant.sheet, "../Npc/anariel"));
    const hmi::FigureAppearance apres = catalog.figureFor("Gorlak", core::CombatSide::Allies);
    EXPECT_EQ(apres.sheet, avant.sheet);
    EXPECT_EQ(apres.frameCount, avant.frameCount);
    EXPECT_EQ(apres.directory, "../Npc/anariel");
    EXPECT_EQ(catalog.sheetDirectory(avant.sheet, core::CombatSide::Allies), "../Npc/anariel");
    EXPECT_EQ(catalog.heroes(), referenceCatalog().heroes());

    EXPECT_FALSE(catalog.replaceHero("inconnu", "../Npc/inconnu"));
    EXPECT_EQ(catalog.sheetDirectory("inconnu", core::CombatSide::Allies), "characters/inconnu");
}

/**
 * @brief Le manifeste des PNJ remplace les heros qu'il nomme, ignore le reste, et son absence
 *        ne change rien.
 * \castest{<b>`applyNpcManifest` : un heros remplace par entree valide, une entree inconnue ou
 * sans slug ignoree, un fichier absent sans effet.</b><br/>
 * \tcat Unitaire · Catalogue d'apparence de l'arene<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Appliquer un chemin inexistant.<br/>
 *          2. Ecrire un manifeste avec `kaelith_voss` -> `anariel`, un heros inconnu et un slug
 *          vide ; l'appliquer.<br/>
 * \tattendu 0 puis 1 remplacement ; `kaelith_voss` lit `../Npc/anariel`, `bram` reste sous
 * `characters/`.
 * }
 */
TEST(ArenaAppearanceCatalogTest, LeManifesteDesPnjRemplaceLesHerosNommes) {
    hmi::ArenaAppearanceCatalog catalog = referenceCatalog();
    const std::filesystem::path dir =
        std::filesystem::temp_directory_path() / "jadg_npc_manifest_test";
    std::filesystem::create_directories(dir);
    const std::filesystem::path manifest = dir / "manifest.json";
    std::filesystem::remove(manifest);

    EXPECT_EQ(catalog.applyNpcManifest(manifest), 0);
    EXPECT_EQ(catalog.sheetDirectory("kaelith_voss", core::CombatSide::Allies),
              "characters/kaelith_voss");

    {
        std::ofstream out(manifest);
        out << R"({"version": 1, "npcs": ["anariel"],
                   "replaces": {"kaelith_voss": "anariel", "inconnu": "x", "bram": ""}})";
    }
    EXPECT_EQ(catalog.applyNpcManifest(manifest), 1);
    EXPECT_EQ(catalog.sheetDirectory("kaelith_voss", core::CombatSide::Allies), "../Npc/anariel");
    EXPECT_EQ(catalog.sheetDirectory("bram", core::CombatSide::Allies), "characters/bram");
    std::filesystem::remove_all(dir);
}
