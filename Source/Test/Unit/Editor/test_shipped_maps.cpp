// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_shipped_maps.cpp
 * @brief Les cartes livrées se font dans l'éditeur (`LOT-EDITOR-06`, décision D4) : chacune
 *        s'ouvre, se modifie, s'enregistre et se recharge sans perte, et sans script.
 *
 * Les scripts qui les posaient sont retirés : plus rien ne compare une carte à un tracé. Ce qui
 * garde les cartes, c'est ce que fait l'éditeur — le brouillon (`core::LevelDraft`) qu'ouvre la
 * fenêtre, les gestes que rejoue `--apply`, et le texte canonique qu'écrit l'enregistrement.
 *
 * **Le seul fichier de `Unit/Editor` qui lise encore les cartes livrées, et c'est son objet**
 * (`LOT-123`) : il balaie celles qu'il trouve, quelles qu'elles soient, et admet qu'il n'y en ait
 * aucune — la table rase du `LOT-102` vide `Levels/`. Les autres tests de l'éditeur ouvrent la
 * racine d'essai, `Source/Test/Fixtures/GameData`.
 */

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "Core/Levels/LevelDraft.h"
#include "Core/Levels/LevelLoader.h"
#include "Core/Levels/LevelWriter.h"
#include "Core/Levels/TileType.h"
#include "Editor/Logic/DataRoot.h"
#include "Editor/Logic/EditorSidecar.h"
#include "Editor/Logic/GestureScript.h"
#include "Editor/Logic/MapFormat.h"
#include "HMI/Graphics/WorldSceneComposer.h"

namespace {

using nlohmann::json;

[[nodiscard]] std::filesystem::path dataRoot() {
    return std::filesystem::path(JADG_LEVELS_DIR).parent_path();
}

[[nodiscard]] std::string lire(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream text;
    text << file.rdbuf();
    return text.str();
}

/// Une carte ouverte comme la fenêtre l'ouvre : le brouillon, avec le manifeste de son lieu.
struct CarteOuverte {
    hmi::PlaceAssets lieu;
    core::LevelDraft draft;
};

[[nodiscard]] CarteOuverte ouvrir(const std::string& texte) {
    core::LevelLoadResult lu = core::LevelLoader::loadFromString(texte);
    EXPECT_TRUE(lu.ok()) << lu.error;
    CarteOuverte carte{.lieu = hmi::loadPlaceAssets(dataRoot(), hmi::scenePlaceOf(*lu.level)),
                       .draft = core::LevelDraft::fromLevel(*lu.level)};
    if (carte.lieu.manifest) {
        carte.draft.setPieceManifest(
            std::make_shared<const core::ScenePieceManifest>(*carte.lieu.manifest));
    }
    return carte;
}

/// Ce qu'écrit l'enregistrement de la fenêtre : la carte validée, en texte canonique.
[[nodiscard]] std::string enregistrer(const core::LevelDraft& draft) {
    const core::LevelLoadResult valide = draft.toLevel();
    EXPECT_TRUE(valide.ok()) << valide.error;
    return valide.ok() ? core::LevelWriter::toJsonString(*valide.level) : std::string{};
}

/// La première pièce dressée de la carte : sa couche et sa case. Sur une carte de principe, qui
/// ne nomme aucune pièce (`LOT-146`), c'est le premier bloc typé de la couche de décor.
struct PieceDressee {
    std::string couche;
    int colonne = 0;
    int ligne = 0;
};

[[nodiscard]] std::optional<PieceDressee> premierePiece(const core::LevelDraft& draft) {
    std::optional<PieceDressee> bloc;
    for (const core::TileLayer& couche : draft.layers()) {
        if (couche.kind != core::LayerKind::Decor) {
            continue;
        }
        for (int ligne = 0; ligne < couche.tiles.height(); ++ligne) {
            for (int colonne = 0; colonne < couche.tiles.width(); ++colonne) {
                if (!couche.pieceAt(colonne, ligne).empty()) {
                    return PieceDressee{.couche = couche.name, .colonne = colonne, .ligne = ligne};
                }
                if (!bloc && couche.tiles.tile(colonne, ligne) != core::TileType::Empty) {
                    bloc = PieceDressee{.couche = couche.name, .colonne = colonne, .ligne = ligne};
                }
            }
        }
    }
    return bloc;
}

}  // namespace

/**
 * @brief Chaque carte livrée s'ouvre et s'enregistre dans l'éditeur sans changer d'un octet.
 * \castest{<b>Les cartes livrées se rechargent sans perte.</b><br/>
 * \tcat Unitaire · Editeur · Cartes livrées<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Pour chaque carte de `Source/Elements/Levels`, l'ouvrir en brouillon avec le
 *          manifeste de son lieu.<br/>2. L'enregistrer sans la toucher.<br/>
 * \tattendu Chaque texte enregistré égale le fichier livré, octet pour octet. Aucune carte
 *           livrée : rien à vérifier, et c'est un état légitime (`LOT-123`).
 * }
 */
TEST(ShippedMapsTest, ChaqueCarteSOuvreEtSEnregistreALIdentique) {
    const auto cartes = hmi::mapFiles(dataRoot());
    for (const std::filesystem::path& fichier : cartes) {
        SCOPED_TRACE(fichier.string());
        const std::string livre = lire(fichier);
        EXPECT_EQ(enregistrer(ouvrir(livre).draft), livre);
    }
}

/**
 * @brief Une retouche faite dans l'éditeur s'enregistre, se recharge sans perte et se défait.
 * \castest{<b>Une retouche s'enregistre, se recharge et se défait.</b><br/>
 * \tcat Unitaire · Editeur · Cartes livrées<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Pour chaque carte livrée, gommer sa première pièce dressée par un geste
 *          `--apply`.<br/>2. Enregistrer, recharger, réenregistrer.<br/>3. Défaire le geste.<br/>
 * \tattendu Un pas d'annulation ; la pièce est partie de la carte rechargée ; le second
 *           enregistrement égale le premier ; défaire rend le fichier livré, octet pour octet.
 * }
 */
TEST(ShippedMapsTest, UneRetoucheSEnregistreSeRechargeEtSeDefait) {
    for (const std::filesystem::path& fichier : hmi::mapFiles(dataRoot())) {
        SCOPED_TRACE(fichier.string());
        const std::string livre = lire(fichier);
        CarteOuverte carte = ouvrir(livre);
        const std::optional<PieceDressee> piece = premierePiece(carte.draft);
        ASSERT_TRUE(piece.has_value()) << "aucune pièce dressée";

        hmi::EditorSidecar annexe;
        const json gestes = {
            {"format", hmi::GESTURE_SCRIPT_FORMAT},
            {"version", hmi::GESTURE_SCRIPT_VERSION},
            {"gestures", json::array({{{"layer", piece->couche},
                                       {"tool", "eraser"},
                                       {"at", json::array({piece->colonne, piece->ligne})}}})}};
        const hmi::GestureScriptResult rejoue =
            hmi::applyGestureScript(gestes, carte.draft, annexe, carte.lieu);
        ASSERT_TRUE(rejoue.ok()) << rejoue.error;
        EXPECT_EQ(rejoue.steps, 1U);

        const std::string retouche = enregistrer(carte.draft);
        ASSERT_NE(retouche, livre);
        const CarteOuverte relue = ouvrir(retouche);
        bool gommee = false;
        for (const core::TileLayer& couche : relue.draft.layers()) {
            if (couche.name == piece->couche) {
                gommee = couche.pieceAt(piece->colonne, piece->ligne).empty() &&
                         couche.tiles.tile(piece->colonne, piece->ligne) == core::TileType::Empty;
            }
        }
        EXPECT_TRUE(gommee) << "la pièce gommée est revenue au rechargement";
        EXPECT_EQ(enregistrer(relue.draft), retouche);

        ASSERT_TRUE(carte.draft.undo());
        EXPECT_EQ(enregistrer(carte.draft), livre);
    }
}

/**
 * @brief La fenêtre et les commandes ouvrent l'arbre des sources, pas la copie que la construction
 *        refait à côté de l'exécutable : ce qu'on enregistre atteint le dépôt.
 * \castest{<b>L'éditeur ouvre les données de l'arbre des sources.</b><br/>
 * \tcat Unitaire · Editeur · Cartes livrées<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Résoudre la racine avec `--data`, puis sans, l'arbre des sources présent.<br/>2. Puis
 * avec un arbre des sources absent ou inconnu.<br/>
 * \tattendu `--data` l'emporte ; sinon `Source/Elements` ; sinon le dossier de l'exécutable.
 * }
 */
TEST(DataRootTest, LEditeurOuvreLesDonneesDeLArbreDesSources) {
    const std::filesystem::path sources = dataRoot();
    const std::filesystem::path executable = "bin";

    EXPECT_EQ(hmi::resolveDataRoot({"--data", "ailleurs", "--check"}, executable, sources),
              std::filesystem::path{"ailleurs"});
    EXPECT_EQ(hmi::resolveDataRoot({"--check"}, executable, sources), sources);
    EXPECT_EQ(hmi::resolveDataRoot({}, executable, sources / "absent"), executable);
    EXPECT_EQ(hmi::resolveDataRoot({}, executable, {}), executable);
    EXPECT_EQ(hmi::resolveDataRoot({"--data"}, executable, {}), executable);
}

/**
 * @brief Un `Levels/` **vide** reste l'arbre des sources : c'est son `README.md` qui le tient
 *        dans le dépôt, git ne gardant pas un dossier vide.
 *
 * Ce que la table rase du `LOT-102` laisse derrière elle : un `Levels/` sans aucune carte. Si
 * `resolveDataRoot` le refusait, la fenêtre et les commandes se rabattraient en silence sur la
 * copie que la construction refait à côté de l'exécutable, et l'auteur dessinerait dans le vide.
 * \castest{<b>Un dossier de niveaux vide reste l'arbre des sources.</b><br/>
 * \tcat Unitaire · Editeur · Cartes livrées<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Batir une racine dont `Levels/` ne porte que son `README.md`.<br/>2. Resoudre la
 * racine sans `--data`.<br/>3. Controler cette racine.<br/>
 * \tattendu La racine est celle de l'arbre des sources, pas le dossier de l'executable ; le
 * controle ne trouve aucune carte, le dit, et rend 0.
 * }
 */
TEST(DataRootTest, UnDossierDeNiveauxVideResteLArbreDesSources) {
    const std::filesystem::path racine =
        std::filesystem::temp_directory_path() / ("jadg-base-vide-" + std::to_string(std::rand()));
    std::error_code ignore;
    std::filesystem::remove_all(racine, ignore);
    std::filesystem::create_directories(racine / "Levels");
    std::ofstream(racine / "Levels" / "README.md") << "# Cartes du jeu.\n";

    EXPECT_EQ(hmi::resolveDataRoot({"--check"}, "bin", racine), racine);

    std::string sortie;
    const std::optional<int> code =
        hmi::runMapCommand({"--check", "--data", racine.string()}, {}, sortie);
    ASSERT_TRUE(code.has_value());
    EXPECT_EQ(*code, 0) << sortie;
    EXPECT_NE(sortie.find("no map under"), std::string::npos) << sortie;
    EXPECT_NE(sortie.find("checked 0 maps"), std::string::npos) << sortie;

    std::filesystem::remove_all(racine, ignore);
}
