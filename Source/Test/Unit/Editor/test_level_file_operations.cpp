// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/TileLayer.h"
#include "Editor/Logic/LevelFileOperations.h"
#include "Editor/Logic/MapFormat.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

// Fournit un dossier temporaire vierge par test (créé/supprimé automatiquement). Les cartes
// vivent dans `<racine>/Levels`, comme dans le projet : le renommage propagé (LOT-EDITOR-14) lit
// les autres cartes, les villes et les catalogues autour de ce dossier.
class LevelFileOps : public ::testing::Test {
protected:
    std::filesystem::path root;
    std::filesystem::path dir;

    void SetUp() override {
        root = std::filesystem::temp_directory_path() /
               ("pg_levelops_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        dir = root / "Levels";
        std::filesystem::create_directories(dir);
    }
    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(root, error);
    }
};

}  // namespace

/**
 * @brief Créer un niveau écrit sur disque un fichier **valide** et l'expose immédiatement dans la
 * liste : la création part d'un `core::LevelDraft` vide passé par la validation, jamais d'un
 * fichier vide qui échouerait au premier chargement.
 * \castest{<b>Créer un niveau écrit un fichier valide, immédiatement listé.</b><br/>
 * \tcat Unitaire · Opérations sur fichiers de niveau<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * }
 */
TEST_F(LevelFileOps, CreeUnNiveauValide) {
    const hmi::LevelFileOperations ops(dir);
    const hmi::FileOperationResult result = ops.create("MonNiveau", 10, 6);
    ASSERT_TRUE(result.ok()) << result.error;
    EXPECT_TRUE(std::filesystem::exists(result.path));
    EXPECT_EQ(ops.list().size(), 1U);
}

/**
 * @brief Une carte créée avec un lieu naît comme les cartes livrées (`LOT-EDITOR-06`) : on la fait
 * ensuite entièrement dans l'éditeur. Qu'elle passe le contrôle, nom traduit compris, est vérifié
 * dans un projet complet : `ContentCheckTest.UneCarteNeuveASonNomDansChaqueCatalogue`.
 * \castest{<b>Une carte creee avec un lieu a ses deux couches.</b><br/>
 * \tcat Unitaire · Opérations sur fichiers de niveau<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Créer une carte 12 × 8 au lieu `martpart`.<br/>2. La relire.<br/>
 * \tattendu Une couche de sol `sol` au lieu `martpart`, une couche de décor `relief`.
 * }
 */
TEST_F(LevelFileOps, UneCarteCreeeAvecUnLieuASesDeuxCouches) {
    const hmi::LevelFileOperations ops(dir);
    const hmi::FileOperationResult result = ops.create("Echoppe", 12, 8, "martpart");
    ASSERT_TRUE(result.ok()) << result.error;

    const core::LevelLoadResult lu = core::LevelLoader::loadFromFile(result.path);
    ASSERT_TRUE(lu.ok()) << lu.error;
    EXPECT_EQ(hmi::scenePlaceOf(*lu.level), "martpart");
    int sols = 0;
    int decors = 0;
    for (const core::TileLayer& couche : lu.level->layers()) {
        sols += couche.kind == core::LayerKind::Ground && couche.name == "sol" ? 1 : 0;
        decors += couche.kind == core::LayerKind::Decor && couche.name == "relief" ? 1 : 0;
    }
    EXPECT_EQ(sols, 1);
    EXPECT_EQ(decors, 1);
}

/**
 * @brief Les lieux qu'une nouvelle carte peut prendre sont les planches qui ont un manifeste.
 * \castest{<b>Les lieux proposes ont un manifeste.</b><br/>
 * \tcat Unitaire · Opérations sur fichiers de niveau<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lister les lieux de `Source/Elements`.<br/>
 * \tattendu `coliseum` et `martpart` en font partie, triés ; chacun a son `manifest.json`.
 * }
 */
TEST(ScenePlacesTest, LesLieuxProposesOntUnManifeste) {
    const std::filesystem::path donnees = std::filesystem::path(JADG_LEVELS_DIR).parent_path();
    const std::vector<std::string> lieux = hmi::scenePlaces(donnees);
    EXPECT_TRUE(std::ranges::is_sorted(lieux));
    EXPECT_NE(std::ranges::find(lieux, "coliseum"), lieux.end());
    EXPECT_NE(std::ranges::find(lieux, "martpart"), lieux.end());
    for (const std::string& lieu : lieux) {
        EXPECT_TRUE(
            std::filesystem::is_regular_file(donnees / "Assets" / "Scene" / lieu / "manifest.json"))
            << lieu;
    }
}

/**
 * @brief Une carte ne demande qu'une case d'entrée : 1×1 se crée, une dimension nulle est refusée.
 * \castest{<b>La plus petite carte créable est 1×1.</b><br/>
 * \tcat Unitaire · Opérations sur fichiers de niveau<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Créer une carte 1×1.<br/>2. Tenter une carte de largeur nulle, puis de hauteur
 * nulle.<br/>
 * \tattendu La carte 1×1 est écrite ; les deux autres sont refusées sans fichier.
 * }
 */
TEST_F(LevelFileOps, LaPlusPetiteCarteFaitUneCase) {
    const hmi::LevelFileOperations ops(dir);
    const hmi::FileOperationResult single = ops.create("Case", 1, 1);
    ASSERT_TRUE(single.ok()) << single.error;
    EXPECT_FALSE(ops.create("Vide", 0, 1).ok());
    EXPECT_FALSE(ops.create("Plate", 1, 0).ok());
    EXPECT_EQ(ops.list().size(), 1U);
}

/**
 * @brief Un fichier de séquence de contenu (`sequence-*.json`) partageant le
 * dossier des niveaux n'est **pas** un niveau et ne doit jamais apparaître dans la liste : il ne
 * se chargerait pas comme tel si l'utilisateur tentait de l'ouvrir depuis ce panneau.
 * \castest{<b>Un fichier de séquence n'apparaît jamais dans la liste des niveaux.</b><br/>
 * \tcat Unitaire · Opérations sur fichiers de niveau<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Créer un niveau valide, puis écrire à côté un fichier `sequence-demo.json`
 * quelconque.<br/>2. Lister le dossier.<br/>
 * \tattendu Seul le niveau valide apparaît dans la liste ; le fichier de séquence en est exclu.
 * }
 */
TEST_F(LevelFileOps, FichierDeSequenceExcluDeLaListe) {
    const hmi::LevelFileOperations ops(dir);
    ASSERT_TRUE(ops.create("MonNiveau", 10, 6).ok());
    std::ofstream sequenceFile(dir / "sequence-demo.json");
    sequenceFile << R"({ "levels": ["MonNiveau.json"] })";
    sequenceFile.close();

    const std::vector<std::filesystem::path> listed = ops.list();
    ASSERT_EQ(listed.size(), 1U);
    EXPECT_EQ(listed.front().filename().string(), "MonNiveau.json");
}

/**
 * @brief Un nom invalide (barre oblique, qui s'échapperait du dossier des niveaux) et un nom déjà
 * pris sont refusés par un résultat récupérable, jamais par une exception ni par un écrasement
 * silencieux du niveau existant.
 * \castest{<b>Un nom invalide ou déjà pris est refusé, sans écraser le niveau existant.</b><br/>
 * \tcat Unitaire · Opérations sur fichiers de niveau<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * }
 */
TEST_F(LevelFileOps, RefuseNomInvalideEtCollision) {
    const hmi::LevelFileOperations ops(dir);
    EXPECT_FALSE(ops.create("a/b", 10, 6).ok());  // barre oblique interdite
    EXPECT_TRUE(ops.create("Niveau", 10, 6).ok());
    EXPECT_FALSE(ops.create("Niveau", 10, 6).ok());  // collision de nom
}

/**
 * @brief Renommer **déplace** le fichier : l'ancien chemin disparaît et le nouveau existe. Une
 * copie qui laisserait l'ancien en place produirait deux niveaux là où l'auteur en attend un.
 * \castest{<b>Renommer déplace le fichier : l'ancien chemin disparaît, le nouveau existe.</b><br/>
 * \tcat Unitaire · Opérations sur fichiers de niveau<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * }
 */
TEST_F(LevelFileOps, RenommeEtDeplaceLeFichier) {
    const hmi::LevelFileOperations ops(dir);
    const hmi::FileOperationResult created = ops.create("Ancien", 10, 6);
    ASSERT_TRUE(created.ok()) << created.error;
    const hmi::FileOperationResult renamed = ops.rename(created.path, "Nouveau");
    ASSERT_TRUE(renamed.ok()) << renamed.error;
    EXPECT_FALSE(std::filesystem::exists(created.path));
    EXPECT_TRUE(std::filesystem::exists(renamed.path));
}

/**
 * @brief Dupliquer deux fois le même niveau produit deux copies **distinctes** : le nom de la
 * copie est rendu unique à chaque appel, sinon la seconde duplication écraserait la première.
 * \castest{<b>Dupliquer deux fois le même niveau produit deux copies distinctes.</b><br/>
 * \tcat Unitaire · Opérations sur fichiers de niveau<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * }
 */
TEST_F(LevelFileOps, DupliqueSousUnNomUnique) {
    const hmi::LevelFileOperations ops(dir);
    const hmi::FileOperationResult base = ops.create("Base", 10, 6);
    ASSERT_TRUE(base.ok()) << base.error;
    const hmi::FileOperationResult first = ops.duplicate(base.path);
    const hmi::FileOperationResult second = ops.duplicate(base.path);
    ASSERT_TRUE(first.ok()) << first.error;
    ASSERT_TRUE(second.ok()) << second.error;
    EXPECT_NE(first.path, second.path);  // deux copies distinctes
    EXPECT_EQ(ops.list().size(), 3U);
}

/**
 * @brief Supprimer retire effectivement le fichier du disque et le signale par un résultat
 * favorable — le panneau se fie à ce résultat pour retirer la ligne de sa liste.
 * \castest{<b>Supprimer retire effectivement le fichier du disque.</b><br/>
 * \tcat Unitaire · Opérations sur fichiers de niveau<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * }
 */
TEST_F(LevelFileOps, SupprimeLeFichier) {
    const hmi::LevelFileOperations ops(dir);
    const hmi::FileOperationResult created = ops.create("X", 10, 6);
    ASSERT_TRUE(created.ok()) << created.error;
    EXPECT_TRUE(ops.remove(created.path).ok());
    EXPECT_FALSE(std::filesystem::exists(created.path));
}
