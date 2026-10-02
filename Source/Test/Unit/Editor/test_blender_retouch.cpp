// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "Editor/Logic/BlenderRetouch.h"
#include "Editor/Logic/CharacterWorkshop.h"

/**
 * @file Unit/Editor/test_blender_retouch.cpp
 * @brief L'aller-retour par Blender, côté éditeur (`LOT-1008`, `EX-EDIT-103`) : où sont les
 *        fichiers et les outils, et les deux lignes de commande que la fenêtre lance.
 *
 * Blender lui-même n'est pas lancé ici : ce que le script en relit se vérifie, sans Blender, dans
 * `scripts/tests/test_retouch_character.py`.
 */

namespace {

class BlenderRetouch : public ::testing::Test {
protected:
    std::filesystem::path depot;

    void SetUp() override {
        depot = std::filesystem::temp_directory_path() /
                ("pg_retouch_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        std::error_code erreur;
        std::filesystem::remove_all(depot, erreur);
        std::filesystem::create_directories(depot);
    }
    void TearDown() override {
        std::error_code erreur;
        std::filesystem::remove_all(depot, erreur);
    }

    void poser(const std::filesystem::path& fichier) const {
        std::filesystem::create_directories((depot / fichier).parent_path());
        std::ofstream sortie(depot / fichier, std::ios::binary);
        sortie << "x";
    }

    [[nodiscard]] static hmi::CharacterDraft bandit() {
        hmi::CharacterDraft fiche;
        fiche.root = "..";
        fiche.level = "Regions/terre/bourg/Characters";
        fiche.name = "bandit";
        fiche.skeleton = "humanoid";
        fiche.model = "Lies/bandit/bandit.glb";
        fiche.workshop.received = "Standard/bandit/recu.glb";
        fiche.workshop.sheet = "Personnages/bandit/liaison.json";
        return fiche;
    }

    [[nodiscard]] hmi::RetouchFiles fichiers() const {
        return hmi::retouchFiles(depot / "Source" / "Elements", depot / "atelier" / "Fiches",
                                 bandit());
    }
};

}  // namespace

/**
 * @brief Les fichiers de l'aller-retour se déduisent de la fiche d'atelier : la retouche à côté
 *        de la fiche de liaison, le fichier Blender à côté du modèle lié.
 * \castest{<b>Les fichiers de l'aller-retour se deduisent de la fiche d'atelier.</b><br/>
 * \tcat Unitaire · Editeur · Atelier des assets<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Demander les fichiers d'une fiche dont la racine remonte d'un dossier et qui ne
 *             nomme ni retouche ni fichier Blender.<br/>
 *          2. Nommer les deux, et redemander.<br/>
 * \tattendu Les chemins partent de la racine de la fiche. Par défaut la retouche est
 *           `retouche.json` à côté de la fiche de liaison, le fichier Blender porte le nom du
 *           modèle lié ; nommés, ils sont pris tels quels. Le squelette est celui du monde.
 * }
 */
TEST_F(BlenderRetouch, LesFichiersSeDeduisentDeLaFiche) {
    const hmi::RetouchFiles deduits = fichiers();
    const std::filesystem::path atelier = depot / "atelier";
    EXPECT_EQ(deduits.model, atelier / "Lies" / "bandit" / "bandit.glb");
    EXPECT_EQ(deduits.received, atelier / "Standard" / "bandit" / "recu.glb");
    EXPECT_EQ(deduits.sheet, atelier / "Personnages" / "bandit" / "liaison.json");
    EXPECT_EQ(deduits.retouch, atelier / "Personnages" / "bandit" / "retouche.json");
    EXPECT_EQ(deduits.blend, atelier / "Lies" / "bandit" / "bandit.blend");
    EXPECT_EQ(deduits.skeleton, depot / "Source" / "Elements" / "Assets" / "Common" / "Characters" /
                                    "Skeletons" / "humanoid" / "skeleton.json");

    hmi::CharacterDraft fiche = bandit();
    fiche.workshop.retouch = "Retouches/bandit.json";
    fiche.workshop.blend = "Blender/bandit.blend";
    const hmi::RetouchFiles nommes =
        hmi::retouchFiles(depot / "Source" / "Elements", atelier / "Fiches", fiche);
    EXPECT_EQ(nommes.retouch, atelier / "Retouches" / "bandit.json");
    EXPECT_EQ(nommes.blend, atelier / "Blender" / "bandit.blend");
}

/**
 * @brief Blender ne s'ouvre pas sur un personnage que l'import ne pourrait pas relier.
 * \castest{<b>L'aller-retour dit ce qui lui manque avant d'ouvrir Blender.</b><br/>
 * \tcat Unitaire · Editeur · Atelier des assets<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander si l'aller-retour est prêt, en posant un à un le modèle lié, le maillage
 *             reçu, la fiche de liaison et le squelette installé.<br/>
 * \tattendu Tant qu'un fichier manque, le message le nomme (le modèle lié, le maillage reçu, la
 *           fiche de liaison, le squelette) ; les quatre posés, il est vide.
 * }
 */
TEST_F(BlenderRetouch, CeQuiManqueEstDitAvantDOuvrirBlender) {
    EXPECT_NE(hmi::retouchReadiness(hmi::RetouchFiles{}).find("linked model"), std::string::npos);
    EXPECT_NE(hmi::retouchReadiness(fichiers()).find("linked model not found"), std::string::npos);
    poser("atelier/Lies/bandit/bandit.glb");
    EXPECT_NE(hmi::retouchReadiness(fichiers()).find("received model"), std::string::npos);
    poser("atelier/Standard/bandit/recu.glb");
    EXPECT_NE(hmi::retouchReadiness(fichiers()).find("liaison sheet"), std::string::npos);
    poser("atelier/Personnages/bandit/liaison.json");
    EXPECT_NE(hmi::retouchReadiness(fichiers()).find("skeleton not installed"), std::string::npos);
    poser("Source/Elements/Assets/Common/Characters/Skeletons/humanoid/skeleton.json");
    EXPECT_EQ(hmi::retouchReadiness(fichiers()), "");
}

/**
 * @brief Les outils se trouvent : le script en remontant jusqu'au dépôt, Python et Blender par
 *        l'environnement ; et les deux commandes portent tout ce que le script attend
 *        (`EX-EDIT-103`).
 * \castest{<b>L'editeur compose les deux commandes de l'aller-retour par Blender.</b><br/>
 * \tcat Unitaire · Editeur · Atelier des assets<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Chercher les outils sans script dans le dépôt, puis avec, Blender étant nommé par
 *             `BLENDER` et Python par `JADG_PYTHON` ; puis avec un Blender qui n'existe pas.<br/>
 *          2. Composer la commande d'ouverture et celle d'import.<br/>
 * \tattendu Sans script, l'erreur le nomme ; avec un Blender absent, elle nomme `BLENDER`. Les
 *           outils trouvés, la commande d'ouverture est `python script open <modèle> --blend …
 *           --skeleton … --blender …`, celle d'import `python script import <blend> --sheet …
 *           --retouch … --source <reçu> --output <modèle> --skeleton … --blender …`.
 * }
 */
TEST_F(BlenderRetouch, LesDeuxCommandesPortentCeQueLeScriptAttend) {
    poser("outils/blender.exe");
    const std::string blender = (depot / "outils" / "blender.exe").generic_string();
    const hmi::EnvironmentLookup environnement =
        [&blender](const std::string& nom) -> std::optional<std::string> {
        if (nom == "BLENDER") {
            return blender;
        }
        return nom == "JADG_PYTHON" ? std::optional<std::string>{"python-essai"} : std::nullopt;
    };
    std::string erreur;
    static_cast<void>(hmi::findRetouchTools(depot / "Source" / "Elements", environnement, erreur));
    EXPECT_NE(erreur.find("retouch_character.py"), std::string::npos) << erreur;

    poser("scripts/assetsGeneration/retouch_character.py");
    std::filesystem::create_directories(depot / "Source" / "Elements");
    const hmi::RetouchTools outils =
        hmi::findRetouchTools(depot / "Source" / "Elements", environnement, erreur);
    EXPECT_EQ(erreur, "");
    EXPECT_EQ(outils.python, "python-essai");
    EXPECT_TRUE(outils.pythonArguments.empty());
    EXPECT_EQ(outils.script.filename(), "retouch_character.py");

    static_cast<void>(hmi::findRetouchTools(
        depot / "Source" / "Elements",
        [](const std::string& nom) -> std::optional<std::string> {
            return nom == "BLENDER" ? std::optional<std::string>{"Z:/nulle-part/blender.exe"}
                                    : std::nullopt;
        },
        erreur));
    EXPECT_NE(erreur.find("BLENDER"), std::string::npos) << erreur;

    const hmi::RetouchFiles files = fichiers();
    const hmi::ProcessCommand ouvrir = hmi::openInBlenderCommand(outils, files);
    EXPECT_EQ(ouvrir.program, "python-essai");
    const std::vector<std::string> ouvertureAttendue = {outils.script.generic_string(),
                                                        "open",
                                                        files.model.generic_string(),
                                                        "--blend",
                                                        files.blend.generic_string(),
                                                        "--skeleton",
                                                        files.skeleton.generic_string(),
                                                        "--blender",
                                                        blender};
    EXPECT_EQ(ouvrir.arguments, ouvertureAttendue);

    const hmi::ProcessCommand importer = hmi::importFromBlenderCommand(outils, files);
    const std::vector<std::string> importAttendu = {outils.script.generic_string(),
                                                    "import",
                                                    files.blend.generic_string(),
                                                    "--sheet",
                                                    files.sheet.generic_string(),
                                                    "--retouch",
                                                    files.retouch.generic_string(),
                                                    "--source",
                                                    files.received.generic_string(),
                                                    "--output",
                                                    files.model.generic_string(),
                                                    "--skeleton",
                                                    files.skeleton.generic_string(),
                                                    "--blender",
                                                    blender};
    EXPECT_EQ(importer.arguments, importAttendu);
}
