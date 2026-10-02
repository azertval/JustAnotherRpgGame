// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "Editor/Logic/CharacterWorkshop.h"
#include "Editor/Logic/MapFormat.h"
#include "Editor/Logic/Sha256.h"

/**
 * @file Unit/Editor/test_character_workshop.cpp
 * @brief L'atelier des assets, vue *Character* (`LOT-1008`) : la fiche d'atelier d'un personnage,
 *        son installation sans fenêtre, comparée à des fichiers attendus, et son contrôle.
 *
 * Les données sont celles de `Fixtures/Workshop` : la fiche du garde du bourg, installée sur une
 * copie de la racine `base/`. Pour refaire les fichiers attendus, voir son `README.md`.
 */

namespace {

std::filesystem::path fixtures() {
    return std::filesystem::path(JADG_TEST_FIXTURES_DIR) / "Workshop";
}

std::string lire(const std::filesystem::path& fichier) {
    std::ifstream entree(fichier, std::ios::binary);
    std::ostringstream texte;
    texte << entree.rdbuf();
    return texte.str();
}

/// Le texte d'un fichier, ses fins de ligne ramenées à celles du dépôt.
std::string lireTexte(const std::filesystem::path& fichier) {
    std::string texte = lire(fichier);
    std::erase(texte, '\r');
    return texte;
}

void ecrire(const std::filesystem::path& fichier, const std::string& texte) {
    std::filesystem::create_directories(fichier.parent_path());
    std::ofstream sortie(fichier, std::ios::binary | std::ios::trunc);
    sortie << texte;
}

/// Une racine de données d'essai, copie de `base/`, où l'atelier écrit ; jetée après le test.
class CharacterWorkshop : public ::testing::Test {
protected:
    std::filesystem::path racine;

    void SetUp() override {
        racine = std::filesystem::temp_directory_path() /
                 ("pg_workshop_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        std::error_code erreur;
        std::filesystem::remove_all(racine, erreur);
        std::filesystem::copy(fixtures() / "base", racine,
                              std::filesystem::copy_options::recursive);
    }
    void TearDown() override {
        std::error_code erreur;
        std::filesystem::remove_all(racine, erreur);
    }

    [[nodiscard]] std::filesystem::path bourg() const {
        return racine / "Assets" / "Regions" / "terre" / "bourg" / "Characters";
    }

    /// La fiche du garde, lue.
    [[nodiscard]] static hmi::CharacterDraft garde() {
        const hmi::CharacterDraftResult lue =
            hmi::readCharacterDraft(lire(fixtures() / "garde.character.json"));
        EXPECT_TRUE(lue.ok()) << lue.error;
        return lue.draft;
    }

    /// Installe @p fiche depuis le dossier des fixtures ; rend l'erreur, vide si tout est écrit.
    [[nodiscard]] std::string installer(const hmi::CharacterDraft& fiche) const {
        const hmi::CharacterInstallPlan plan = hmi::planCharacter(racine, fixtures(), fiche);
        return plan.ok() ? hmi::writeCharacter(plan) : plan.error;
    }
};

}  // namespace

/**
 * @brief L'empreinte SHA-256 est celle de la norme : les manifestes en dépendent.
 * \castest{<b>L'empreinte SHA-256 de l'atelier est celle de la norme.</b><br/>
 * \tcat Unitaire · Editeur · Atelier des assets<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Calculer l'empreinte du texte vide, de « abc », d'un texte de 56 octets et d'un
 *             million de « a ».<br/>
 * \tattendu Les quatre empreintes sont celles de FIPS 180-4 : les blocs de bourrage à un et à
 *           deux blocs sont tous deux couverts.
 * }
 */
TEST(Sha256Test, LEmpreinteEstCelleDeLaNorme) {
    EXPECT_EQ(hmi::sha256Hex(""),
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    EXPECT_EQ(hmi::sha256Hex("abc"),
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    EXPECT_EQ(hmi::sha256Hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
              "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
    EXPECT_EQ(hmi::sha256Hex(std::string(1000000, 'a')),
              "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

/**
 * @brief Une fiche d'atelier se lit, se réécrit à l'identique, et refuse ce qu'elle ne connaît
 *        pas (`EX-EDIT-102`).
 * \castest{<b>Une fiche d'atelier se relit et se reecrit a l'identique.</b><br/>
 * \tcat Unitaire · Editeur · Atelier des assets<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lire la fiche du garde, la réécrire, la relire.<br/>
 *          2. Lire un scénario de gestes, une fiche d'une autre version, une fiche au champ
 *             inconnu.<br/>
 * \tattendu Le texte réécrit est celui du fichier, octet pour octet, et la fiche relue est la
 *           même. Le scénario de gestes n'est pas une fiche d'atelier ; l'autre version et le
 *           champ inconnu sont refusés, le champ nommé.
 * }
 */
TEST(CharacterDraftTest, UneFicheSeLitEtSeReecritALIdentique) {
    const std::string texte = lire(fixtures() / "garde.character.json");
    ASSERT_TRUE(hmi::isCharacterScript(texte));
    const hmi::CharacterDraftResult lue = hmi::readCharacterDraft(texte);
    ASSERT_TRUE(lue.ok()) << lue.error;
    EXPECT_EQ(lue.draft.level, "Regions/terre/bourg/Characters");
    EXPECT_EQ(lue.draft.name, "garde");
    EXPECT_EQ(lue.draft.root, ".");
    EXPECT_EQ(lue.draft.workshop.sheet, "recu/liaison.json");
    EXPECT_EQ(hmi::characterDraftText(lue.draft), texte);
    EXPECT_EQ(hmi::readCharacterDraft(hmi::characterDraftText(lue.draft)).draft, lue.draft);

    EXPECT_FALSE(hmi::isCharacterScript(R"({"format":"jadg-editor-gestures","version":1})"));
    EXPECT_FALSE(hmi::isCharacterScript("pas du JSON"));
    EXPECT_FALSE(hmi::readCharacterDraft(R"({"format":"jadg-editor-character","version":2})").ok());
    const hmi::CharacterDraftResult inconnu = hmi::readCharacterDraft(
        R"({"format":"jadg-editor-character","version":1,"weapon":"axe.glb"})");
    EXPECT_FALSE(inconnu.ok());
    EXPECT_NE(inconnu.error.find("weapon"), std::string::npos) << inconnu.error;
}

/**
 * @brief `--apply <fiche>` installe le personnage et rend les fichiers attendus (`EX-EDIT-102`).
 * \castest{<b>La fiche du garde s'installe sans fenetre et rend les fichiers attendus.</b><br/>
 * \tcat Unitaire · Editeur · Atelier des assets<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Rejouer `--apply garde.character.json` sur une copie de la racine `base/`.<br/>
 *          2. Comparer les deux manifestes et la fiche `character.json` aux fichiers de
 *             `attendu/`, et le modèle, le portrait, le jeton et le squelette à leurs sources.<br/>
 *          3. Rejouer la commande.<br/>
 * \tattendu Code 0. Les trois fichiers lisibles sont ceux de `attendu/`, octet pour octet : le
 *           garde quitte les portraits d'attente pour les modèles, le commentaire « Vide » part,
 *           le squelette est inscrit au commun. Les binaires installés sont leurs sources. La
 *           seconde fois, rien n'est réécrit et la commande le dit.
 * }
 */
TEST_F(CharacterWorkshop, LaFicheInstalleLePersonnageEtRendLesFichiersAttendus) {
    const std::string fiche = (fixtures() / "garde.character.json").string();
    std::string sortie;
    const std::optional<int> code = hmi::runCharacterCommand({"--apply", fiche}, racine, sortie);
    ASSERT_TRUE(code.has_value());
    EXPECT_EQ(*code, 0) << sortie;
    EXPECT_NE(sortie.find("installed 1 character"), std::string::npos) << sortie;

    const std::filesystem::path attendu = fixtures() / "attendu";
    for (const char* const fichier :
         {"Assets/Common/Characters/manifest.json",
          "Assets/Regions/terre/bourg/Characters/manifest.json",
          "Assets/Regions/terre/bourg/Characters/garde/character.json"}) {
        EXPECT_EQ(lire(racine / fichier), lire(attendu / fichier)) << fichier;
    }
    const std::filesystem::path sources =
        std::filesystem::path(JADG_TEST_FIXTURES_DIR) / "Characters" / "Assets" / "Common" /
        "Characters";
    EXPECT_EQ(lire(bourg() / "garde" / "garde.glb"),
              lire(sources / "Mannequins" / "humanoid" / "humanoid.glb"));
    EXPECT_EQ(lire(racine / "Assets" / "Common" / "Characters" / "Skeletons" / "humanoid" /
                   "skeleton.json"),
              lireTexte(sources / "Skeletons" / "humanoid" / "skeleton.json"));
    EXPECT_EQ(lire(bourg() / "garde" / "portrait.png"), lire(fixtures() / "portrait.png"));
    EXPECT_EQ(lire(bourg() / "garde" / "token.png"), lire(fixtures() / "token.png"));

    std::string seconde;
    EXPECT_EQ(hmi::runCharacterCommand({"--apply", fiche}, racine, seconde), std::optional{0});
    EXPECT_NE(seconde.find("nothing to write"), std::string::npos) << seconde;
}

/**
 * @brief Un scénario de gestes n'est pas pour l'atelier ; une fiche et un scénario mêlés sont une
 *        ligne de commande fausse.
 * \castest{<b>L'atelier laisse les scenarios de gestes a la commande des cartes.</b><br/>
 * \tcat Unitaire · Editeur · Atelier des assets<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Passer à l'atelier `--apply` d'un scénario de gestes, puis `--check` seul.<br/>
 *          2. Lui passer une fiche d'atelier et un scénario de gestes ensemble.<br/>
 * \tattendu Les deux premières lignes ne sont pas pour lui (rien n'est rendu). La troisième sort
 *           en 2 et rien n'est installé.
 * }
 */
TEST_F(CharacterWorkshop, LesGestesRestentALaCommandeDesCartes) {
    const std::string gestes =
        (std::filesystem::path(JADG_TEST_FIXTURES_DIR) / "Gestures" / "paint.json").string();
    const std::string fiche = (fixtures() / "garde.character.json").string();
    std::string sortie;
    EXPECT_FALSE(hmi::runCharacterCommand({"--apply", gestes}, racine, sortie).has_value());
    EXPECT_FALSE(hmi::runCharacterCommand({"--check"}, racine, sortie).has_value());
    EXPECT_EQ(hmi::runCharacterCommand({"--apply", fiche, gestes}, racine, sortie),
              std::optional{2});
    EXPECT_FALSE(std::filesystem::exists(bourg() / "garde"));
}

/**
 * @brief Une fiche refusée n'écrit rien, et l'erreur nomme ce qui manque (`EX-EDIT-102`).
 * \castest{<b>Une fiche refusee n'ecrit rien et nomme ce qui manque.</b><br/>
 * \tcat Unitaire · Editeur · Atelier des assets<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Installer le garde avec un portrait de 64 px ; avec un modèle sans squelette ; avec
 *             un pantin de trois os, qui n'a pas les os de la silhouette ; avec une silhouette
 *             inconnue ; sans modèle ; sous un niveau qui n'a pas de manifeste ; avec un nom qui
 *             remonte d'un dossier.<br/>
 * \tattendu Chaque fiche est refusée, le message nomme la cause (la taille du portrait, le
 *           squelette, l'os, le niveau, le nom), et le dossier du garde n'existe pas : rien n'a
 *           été écrit.
 * }
 */
TEST_F(CharacterWorkshop, UneFicheRefuseeNEcritRien) {
    const auto refus = [this](hmi::CharacterDraft fiche, const char* cause) {
        const hmi::CharacterInstallPlan plan = hmi::planCharacter(racine, fixtures(), fiche);
        EXPECT_FALSE(plan.ok()) << cause;
        EXPECT_NE(plan.error.find(cause), std::string::npos) << plan.error;
        EXPECT_TRUE(plan.writes.empty());
        EXPECT_FALSE(hmi::writeCharacter(plan).empty());
        EXPECT_FALSE(std::filesystem::exists(bourg() / "garde"));
    };
    hmi::CharacterDraft fiche = garde();
    fiche.portrait = "petit.png";
    refus(fiche, "64 x 64");

    fiche = garde();
    fiche.model = "../Meshes/Assets/Scene/ilot/wall.glb";
    refus(fiche, "not bound to a skeleton");

    fiche = garde();
    fiche.model = "../Meshes/Assets/Npc/pantin/pantin.glb";
    refus(fiche, "has no bone");

    fiche = garde();
    fiche.skeleton = "quadruped";
    fiche.skeletonSource.clear();
    refus(fiche, "unknown skeleton");

    fiche = garde();
    fiche.model.clear();
    refus(fiche, "none installed");

    fiche = garde();
    fiche.level = "Regions/terre/hameau/Characters";
    refus(fiche, "no manifest");

    fiche = garde();
    fiche.name = "../garde";
    refus(fiche, "\"name\"");
}

/**
 * @brief Un personnage installé se rouvre et se réenregistre sans différence (`EX-EDIT-102`) ;
 *        un champ vide garde ce qui est installé.
 * \castest{<b>Un personnage installe se rouvre et se reenregistre sans difference.</b><br/>
 * \tcat Unitaire · Editeur · Atelier des assets<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Installer le garde, puis rouvrir sa fiche depuis ce qui est installé, l'atelier
 *             local étant le dossier des fixtures.<br/>
 *          2. Calculer son installation ; puis celle de la même fiche sans atelier local.<br/>
 *          3. Remplacer son seul jeton par une image d'un autre nom, et réinstaller.<br/>
 * \tattendu La fiche rouverte nomme les sources du manifeste ; sans atelier local, ses champs
 *           sont vides. Dans les deux cas l'installation n'écrit rien. Un jeton seul se
 *           remplace : le modèle et le portrait ne bougent pas, la source du jeton change au
 *           manifeste.
 * }
 */
TEST_F(CharacterWorkshop, UnPersonnageInstalleSeRouvreSansDifference) {
    ASSERT_EQ(installer(garde()), "");
    const std::vector<hmi::InstalledCharacter> installes = hmi::installedCharacters(racine);
    ASSERT_EQ(installes.size(), 1U);
    EXPECT_EQ(installes[0], (hmi::InstalledCharacter{"Regions/terre/bourg/Characters", "garde"}));
    EXPECT_EQ(hmi::characterLevels(racine),
              (std::vector<std::string>{"Common/Characters", "Regions/terre/bourg/Characters"}));

    const hmi::CharacterDraftResult rouverte =
        hmi::draftOfInstalledCharacter(racine, fixtures(), installes[0]);
    ASSERT_TRUE(rouverte.ok()) << rouverte.error;
    EXPECT_EQ(rouverte.draft.skeleton, "humanoid");
    EXPECT_EQ(rouverte.draft.portrait, "portrait.png");
    EXPECT_EQ(rouverte.draft.model,
              "../Characters/Assets/Common/Characters/Mannequins/humanoid/humanoid.glb");
    const hmi::CharacterInstallPlan memePlan =
        hmi::planCharacter(racine, fixtures(), rouverte.draft);
    ASSERT_TRUE(memePlan.ok()) << memePlan.error;
    EXPECT_TRUE(memePlan.writes.empty() && memePlan.removals.empty()) << memePlan.log;

    const hmi::CharacterDraftResult sansAtelier =
        hmi::draftOfInstalledCharacter(racine, racine / "nulle-part", installes[0]);
    ASSERT_TRUE(sansAtelier.ok());
    EXPECT_TRUE(sansAtelier.draft.model.empty() && sansAtelier.draft.portrait.empty());
    const hmi::CharacterInstallPlan garde2 =
        hmi::planCharacter(racine, racine / "nulle-part", sansAtelier.draft);
    ASSERT_TRUE(garde2.ok()) << garde2.error;
    EXPECT_TRUE(garde2.writes.empty() && garde2.removals.empty()) << garde2.log;

    const std::string modeleAvant = lire(bourg() / "garde" / "garde.glb");
    ecrire(racine / "atelier" / "autre-jeton.png", lire(fixtures() / "token.png"));
    hmi::CharacterDraft jeton = sansAtelier.draft;
    jeton.token = "atelier/autre-jeton.png";
    const hmi::CharacterInstallPlan remplace = hmi::planCharacter(racine, racine, jeton);
    ASSERT_TRUE(remplace.ok()) << remplace.error;
    ASSERT_EQ(hmi::writeCharacter(remplace), "");
    EXPECT_EQ(lire(bourg() / "garde" / "garde.glb"), modeleAvant);
    const std::string manifeste = lire(bourg() / "manifest.json");
    EXPECT_NE(manifeste.find("atelier/autre-jeton.png"), std::string::npos);
    EXPECT_NE(manifeste.find("\"file\": \"portrait.png\""), std::string::npos);
}

/**
 * @brief Les fiches et les manifestes **livrés** se réenregistrent par l'atelier sans différence :
 *        plus aucune fiche du dépôt n'est écrite à la main (`EX-EDIT-102`).
 * \castest{<b>Les personnages livres se reenregistrent par l'atelier sans difference.</b><br/>
 * \tcat Unitaire · Editeur · Atelier des assets<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Lister les personnages en modèle de `Source/Elements/Assets`.<br/>
 *          2. Rouvrir chacun sans atelier local et calculer son installation.<br/>
 * \tattendu Il y en a au moins trois (mannequin, brawler, bandit). Aucune installation n'écrit
 *           ni ne retire rien : la fiche `character.json` et le manifeste de chaque niveau sont
 *           déjà ce que l'atelier écrirait, à l'octet.
 * }
 */
TEST(CharacterWorkshopDelivered, LesPersonnagesLivresSeReenregistrentSansDifference) {
    const std::filesystem::path donnees = std::filesystem::path(JADG_ASSETS_DIR).parent_path();
    const std::vector<hmi::InstalledCharacter> installes = hmi::installedCharacters(donnees);
    EXPECT_GE(installes.size(), 3U);
    for (const hmi::InstalledCharacter& personnage : installes) {
        const hmi::CharacterDraftResult fiche =
            hmi::draftOfInstalledCharacter(donnees, donnees / "nulle-part", personnage);
        ASSERT_TRUE(fiche.ok()) << personnage.name << " : " << fiche.error;
        const hmi::CharacterInstallPlan plan =
            hmi::planCharacter(donnees, donnees / "nulle-part", fiche.draft);
        ASSERT_TRUE(plan.ok()) << personnage.name << " : " << plan.error;
        EXPECT_TRUE(plan.writes.empty() && plan.removals.empty())
            << personnage.level << "/" << personnage.name << "\n"
            << plan.log;
    }
}

/**
 * @brief Le contrôle nomme ce qui manque à un personnage installé (`EX-EDIT-104`).
 * \castest{<b>Le controle des personnages nomme ce qui manque.</b><br/>
 * \tcat Unitaire · Editeur · Atelier des assets<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Installer le garde : le contrôle ne trouve rien.<br/>
 *          2. Retirer son jeton, remplacer son portrait par une image de 64 px, puis son modèle
 *             par un mur sans squelette ; contrôler à chaque fois.<br/>
 *          3. Retirer la description de son squelette, et contrôler.<br/>
 *          4. Poser un verrou de kits sans kit installé, et contrôler.<br/>
 * \tattendu Chaque constat est une erreur portée par le niveau du garde et nomme la cause : le
 *           jeton absent, la taille du portrait, le modèle sans squelette, le squelette inconnu.
 *           Sous un verrou sans kit, les binaires ne sont plus contrôlés : un seul avertissement
 *           le dit, et la fiche se contrôle encore.
 * }
 */
TEST_F(CharacterWorkshop, LeControleNommeCeQuiManque) {
    ASSERT_EQ(installer(garde()), "");
    EXPECT_TRUE(hmi::checkCharacters(racine).empty());

    const auto constat = [this](const char* cause) {
        const std::vector<hmi::MapCheckFinding> constats = hmi::checkCharacters(racine);
        const bool trouve = std::ranges::any_of(constats, [cause](const auto& c) {
            return c.severity == hmi::MapCheckSeverity::Error &&
                   c.mapId == "Regions/terre/bourg/Characters" &&
                   c.message.starts_with("garde: ") &&
                   c.message.find(cause) != std::string::npos;
        });
        EXPECT_TRUE(trouve) << cause << " absent de " << constats.size() << " constat(s)";
    };
    const std::filesystem::path dossier = bourg() / "garde";
    std::filesystem::remove(dossier / "token.png");
    constat("token missing");
    ecrire(dossier / "portrait.png", lire(fixtures() / "petit.png"));
    constat("64 x 64");
    ecrire(dossier / "garde.glb",
           lire(std::filesystem::path(JADG_TEST_FIXTURES_DIR) / "Meshes" / "Assets" / "Scene" /
                "ilot" / "wall.glb"));
    constat("not bound to a skeleton");
    std::filesystem::remove(dossier / "garde.glb");
    constat("model missing");

    // Les kits verrouillés mais pas installés : les binaires ne se contrôlent plus.
    ecrire(racine / "Assets" / "kits.lock.json", "{}\n");
    const std::vector<hmi::MapCheckFinding> sansKit = hmi::checkCharacters(racine);
    ASSERT_EQ(sansKit.size(), 1U);
    EXPECT_EQ(sansKit[0].severity, hmi::MapCheckSeverity::Warning);
    EXPECT_NE(sansKit[0].message.find("fetch_assets.py"), std::string::npos);

    std::filesystem::remove_all(racine / "Assets" / "Common" / "Characters" / "Skeletons");
    constat("unknown skeleton");
}
