// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "HMI/Game/LevelScan.h"

/**
 * @file Unit/HMI/Game/test_level_scan.cpp
 * @brief Les cartes qu'un dossier `Levels/` contient, telles que le lanceur de cartes les liste :
 *        chaque carte listée doit être exactement une carte que `--map=` ouvre.
 */

namespace {

/// @brief Un dossier `Levels/` jetable, peuplé de fichiers vides : le parcours ne lit pas leur
///        contenu, seulement leurs noms.
class DossierDEssai {
public:
    explicit DossierDEssai(const std::string& nom)
        : _racine(std::filesystem::temp_directory_path() / ("jadg-level-scan-" + nom)) {
        std::filesystem::remove_all(_racine);
        std::filesystem::create_directories(_racine);
    }
    ~DossierDEssai() {
        std::filesystem::remove_all(_racine);
    }
    DossierDEssai(const DossierDEssai&) = delete;
    DossierDEssai& operator=(const DossierDEssai&) = delete;

    /// @brief Pose un fichier vide sous @p relatif (dossiers créés au besoin).
    void poser(const std::string& relatif) const {
        const std::filesystem::path fichier = _racine / relatif;
        std::filesystem::create_directories(fichier.parent_path());
        std::ofstream{fichier} << "{}";
    }

    [[nodiscard]] std::filesystem::path chemin(const std::string& relatif = {}) const {
        return relatif.empty() ? _racine : _racine / relatif;
    }

private:
    std::filesystem::path _racine;
};

[[nodiscard]] std::vector<std::string> identifiants(const std::vector<hmi::LevelEntry>& cartes) {
    std::vector<std::string> ids;
    for (const hmi::LevelEntry& carte : cartes) {
        ids.push_back(carte.mapId);
    }
    return ids;
}

/**
 * @brief L'identifiant d'une carte est son chemin sous `Levels/`, sans `.json`, avec des `/` —
 *        ce que `--map=` attend (`LOT-124`).
 * \castest{<b>L'identifiant d'une carte est son chemin sous Levels/, sans .json.</b><br/>
 * \tcat Unitaire · Lanceur de cartes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser trois cartes, dont deux dans des sous-dossiers.<br/>2. Lister le dossier.<br/>
 * \tattendu Trois identifiants, tries, avec des `/` et sans extension ; le fichier et le dossier
 * d'origine de chaque carte sont ceux poses.
 * }
 */
TEST(LevelScan, IdentifiantEstLeCheminSousLevels) {
    const DossierDEssai levels{"identifiants"};
    levels.poser("donjon.json");
    levels.poser("central-empire/capital/arenarea.json");
    levels.poser("bourg/place.json");

    const std::vector<hmi::LevelEntry> cartes = hmi::scanLevelDirectories({levels.chemin()});

    EXPECT_EQ(
        identifiants(cartes),
        (std::vector<std::string>{"bourg/place", "central-empire/capital/arenarea", "donjon"}));
    ASSERT_EQ(cartes.size(), 3U);
    EXPECT_EQ(cartes[2].file, levels.chemin("donjon.json"));
    EXPECT_EQ(cartes[2].directory, levels.chemin());
}

/**
 * @brief Les fichiers annexes de l'éditeur et les fichiers qui ne sont pas des `.json` ne sont
 *        pas des cartes.
 * \castest{<b>Les notes de l'editeur et les fichiers etrangers ne sont pas des cartes.</b><br/>
 * \tcat Unitaire · Lanceur de cartes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Poser une carte, ses notes `.editor.json`, un README et un PNG.<br/>2. Lister.<br/>
 * \tattendu La carte seule.
 * }
 */
TEST(LevelScan, EcarteLesNotesDeLEditeurEtLesAutresFichiers) {
    const DossierDEssai levels{"annexes"};
    levels.poser("cave.json");
    levels.poser("cave.editor.json");
    levels.poser("README.md");
    levels.poser("cave.png");

    EXPECT_EQ(identifiants(hmi::scanLevelDirectories({levels.chemin()})),
              (std::vector<std::string>{"cave"}));
}

/**
 * @brief Quand deux dossiers portent la même carte, le premier l'emporte — la règle de
 *        `--levels=`, où le brouillon de l'éditeur cache la carte du binaire.
 * \castest{<b>Le premier dossier l'emporte sur une carte en double.</b><br/>
 * \tcat Unitaire · Lanceur de cartes<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Poser `donjon.json` dans un dossier de brouillons et dans celui du binaire, et une
 * carte propre a chacun.<br/>2. Lister les deux, brouillons d'abord.<br/>
 * \tattendu Trois cartes, et `donjon` vient du dossier des brouillons.
 * }
 */
TEST(LevelScan, LePremierDossierLEmporteSurUnDoublon) {
    const DossierDEssai brouillons{"brouillons"};
    const DossierDEssai binaire{"binaire"};
    brouillons.poser("donjon.json");
    brouillons.poser("essai.json");
    binaire.poser("donjon.json");
    binaire.poser("cave.json");

    const std::vector<hmi::LevelEntry> cartes =
        hmi::scanLevelDirectories({brouillons.chemin(), binaire.chemin()});

    EXPECT_EQ(identifiants(cartes), (std::vector<std::string>{"cave", "donjon", "essai"}));
    EXPECT_EQ(cartes[1].directory, brouillons.chemin());
    EXPECT_EQ(cartes[0].directory, binaire.chemin());
}

/**
 * @brief Un dossier absent ne contient rien, et n'empêche pas de lire les autres
 *        (`EX-NFR-040`).
 * \castest{<b>Un dossier absent ne contient rien et ne fait pas echouer le parcours.</b><br/>
 * \tcat Unitaire · Lanceur de cartes<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lister un dossier inexistant puis un dossier d'une carte.<br/>
 * \tattendu La carte du second, sans exception.
 * }
 */
TEST(LevelScan, DossierAbsentNeContientRien) {
    const DossierDEssai levels{"present"};
    levels.poser("cave.json");

    EXPECT_EQ(
        identifiants(hmi::scanLevelDirectories({levels.chemin("nulle-part"), levels.chemin()})),
        (std::vector<std::string>{"cave"}));
    EXPECT_TRUE(hmi::scanLevelDirectories({}).empty());
}

}  // namespace
