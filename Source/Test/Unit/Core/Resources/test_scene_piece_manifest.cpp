// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_scene_piece_manifest.cpp
 * @brief Le manifeste des pièces d'un lieu, lu dans `Core` (`LOT-EDITOR-02`, constat A9).
 *
 * Les manifestes livrés sont lus tels quels : c'est eux que l'éditeur et la galerie liront, et une
 * fixture ne dirait rien le jour où l'atelier réextrait une planche.
 */

#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "Core/Resources/ScenePieceManifest.h"

namespace {

[[nodiscard]] std::filesystem::path sceneDirectory(const std::string& place) {
    return std::filesystem::path(JADG_TEST_DATA_DIR) / "Assets" / "Scene" / place;
}

}  // namespace

/**
 * @brief Le manifeste du lieu d'essai se lit, avec emprise, ancre et miroir.
 * \castest{<b>Le manifeste des pieces d'un lieu se lit dans Core.</b><br/>
 * \tcat Unitaire · Assets<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire le manifeste du lieu d'essai.<br/>
 *          2. Chercher une piece de sol, une piece large et son miroir.<br/>
 * \tattendu Le lieu est celui du manifeste ; la rue est un sol d'une case ancre en (34, 0) ; la
 * facade droite est large, occupe 2 x 1 cases et se declare miroir de la facade gauche ; une piece
 * inconnue n'est pas trouvee.
 * }
 */
TEST(ScenePieceManifestTest, LeManifesteDUnLieuSeLit) {
    const core::ScenePieceManifestResult read =
        core::ScenePieceManifest::loadFromFile(sceneDirectory("bourg") / "manifest.json");
    ASSERT_TRUE(read.ok()) << read.message;
    const core::ScenePieceManifest& manifest = read.manifest;
    EXPECT_EQ(manifest.place(), "bourg");
    EXPECT_FALSE(manifest.pieces().empty());

    const core::ScenePiece* street = manifest.find("street");
    ASSERT_NE(street, nullptr);
    EXPECT_EQ(street->key, "scene/bourg/street");
    EXPECT_EQ(street->file, "street.png");
    EXPECT_EQ(street->pieceClass, core::ScenePieceClass::Floor);
    EXPECT_EQ(street->footprintColumns, 1);
    EXPECT_EQ(street->footprintRows, 1);
    EXPECT_EQ(street->width, 68);
    EXPECT_EQ(street->height, 42);
    EXPECT_EQ(street->anchorX, 34);
    EXPECT_EQ(street->anchorY, 0);
    EXPECT_TRUE(street->mirrorOf.empty());

    const core::ScenePiece* front = manifest.find("front-right");
    ASSERT_NE(front, nullptr);
    EXPECT_EQ(front->pieceClass, core::ScenePieceClass::Wide);
    EXPECT_EQ(front->footprintColumns, 2);
    EXPECT_EQ(front->footprintRows, 1);
    EXPECT_EQ(front->mirrorOf, "front-left");
    EXPECT_NE(manifest.find(front->mirrorOf), nullptr) << "le miroir nomme une piece du lieu";

    EXPECT_EQ(manifest.find("piece-inconnue"), nullptr);
}

/**
 * @brief Chaque pièce déclarée par un manifeste a son image.
 * \castest{<b>Toute piece declaree par un lieu a son image.</b><br/>
 * \tcat Unitaire · Assets<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire les manifestes des deux lieux d'essai.<br/>
 *          2. Chercher le fichier de chaque piece dans le dossier du lieu.<br/>
 * \tattendu Aucune piece ne manque, et chaque emprise vaut au moins une case.
 * }
 */
TEST(ScenePieceManifestTest, ChaquePieceDeclareeASonImage) {
    for (const std::string place : {"bourg", "hameau"}) {
        const core::ScenePieceManifestResult read =
            core::ScenePieceManifest::loadFromFile(sceneDirectory(place) / "manifest.json");
        ASSERT_TRUE(read.ok()) << place << " : " << read.message;
        for (const core::ScenePiece& piece : read.manifest.pieces()) {
            EXPECT_TRUE(std::filesystem::is_regular_file(sceneDirectory(place) / piece.file))
                << place << " / " << piece.name;
            EXPECT_GE(piece.footprintColumns, 1);
            EXPECT_GE(piece.footprintRows, 1);
        }
    }
}

/**
 * @brief Une entrée incomplète est ignorée, une classe inconnue gardée, un manifeste sans
 *        `textures` refusé.
 * \castest{<b>Le manifeste des pieces tolere une entree fautive sans perdre les autres.</b><br/>
 * \tcat Unitaire · Assets<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire un manifeste dont une entree n'a pas d'image et une autre une classe
 *             inconnue.<br/>
 *          2. Lire un manifeste sans `textures`, puis un fichier absent.<br/>
 * \tattendu L'entree sans image est ignoree ; la classe inconnue est `Other`, son nom garde ;
 *           l'emprise absente vaut 1 x 1 ; le manifeste sans `textures` est mal forme ; le
 *           fichier absent est signale comme tel.
 * }
 */
TEST(ScenePieceManifestTest, UneEntreeFautiveNeFaitPasPerdreLesAutres) {
    const core::ScenePieceManifestResult read = core::ScenePieceManifest::loadFromString(R"({
      "version": 1,
      "disposition": "essai",
      "textures": {
        "scene/essai/sans-image": { "class": "tall" },
        "scene/essai/estrade": { "file": "estrade.png", "class": "podium" }
      }
    })");
    ASSERT_TRUE(read.ok()) << read.message;
    ASSERT_EQ(read.manifest.pieces().size(), 1U);
    const core::ScenePiece& piece = read.manifest.pieces().front();
    EXPECT_EQ(piece.name, "estrade");
    EXPECT_EQ(piece.pieceClass, core::ScenePieceClass::Other);
    EXPECT_EQ(piece.className, "podium");
    EXPECT_EQ(piece.footprintColumns, 1);
    EXPECT_EQ(piece.footprintRows, 1);

    EXPECT_EQ(core::ScenePieceManifest::loadFromString(R"({"version": 1})").error,
              core::ScenePieceManifestError::MalformedStructure);
    EXPECT_EQ(
        core::ScenePieceManifest::loadFromFile(sceneDirectory("absent") / "manifest.json").error,
        core::ScenePieceManifestError::FileNotFound);
}
