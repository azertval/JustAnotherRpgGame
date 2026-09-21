// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

#include "Editor/Logic/Autosave.h"

namespace {

// Un dossier de reprise vierge par test.
class AutosaveTest : public ::testing::Test {
protected:
    std::filesystem::path dir;

    void SetUp() override {
        dir = std::filesystem::temp_directory_path() /
              ("jadg_autosave_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        std::filesystem::remove_all(dir);
    }
    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(dir, error);
    }

    static hmi::AutosaveRecord laPlace() {
        return hmi::AutosaveRecord{
            .mapId = "bourg/place",
            .levelPath = "Levels/bourg/place.json",
            .draftJson = R"({"name": "La Place", "note": "é \"guillemets\""})"};
    }
};

}  // namespace

/**
 * @brief Un brouillon écrit puis relu revient à l'identique, texte compris (accents, guillemets).
 * \castest{<b>Un brouillon sauvegardé automatiquement se relit à l'identique.</b><br/>
 * \tcat Unitaire · Éditeur, reprise après plantage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Écrire le brouillon de la Place.<br/>2. Lister les brouillons en attente.<br/>
 * 3. Vérifier qu'il revient tel quel.<br/>
 * }
 */
TEST_F(AutosaveTest, UnBrouillonSeRelitALIdentique) {
    const hmi::AutosaveStore store(dir);
    ASSERT_TRUE(store.write(laPlace()));
    const std::vector<hmi::AutosaveRecord> pending = store.pending();
    ASSERT_EQ(pending.size(), 1U);
    EXPECT_EQ(pending.front(), laPlace());
}

/**
 * @brief L'identifiant d'une carte en sous-dossier devient un nom de fichier plat ; une carte sans
 *        identifiant a quand même un nom.
 * \castest{<b>Le nom du fichier de reprise aplatit le sous-dossier de la carte.</b><br/>
 * \tcat Unitaire · Éditeur, reprise après plantage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Nommer le fichier de `bourg/place`, puis d'un identifiant vide.<br/>
 * }
 */
TEST_F(AutosaveTest, LeNomDeFichierEstPlat) {
    EXPECT_EQ(hmi::autosaveFileName("bourg/place"), "bourg~place.autosave.json");
    EXPECT_EQ(hmi::autosaveFileName("donjon"), "donjon.autosave.json");
    EXPECT_EQ(hmi::autosaveFileName(""), "untitled.autosave.json");
}

/**
 * @brief Réécrire remplace le brouillon précédent, sans laisser de fichier temporaire ; retirer
 *        vide la liste.
 * \castest{<b>Une nouvelle sauvegarde remplace l'ancienne ; enregistrer la carte la
 * retire.</b><br/>
 * \tcat Unitaire · Éditeur, reprise après plantage<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Écrire deux fois le même brouillon, modifié.<br/>2. Vérifier qu'un seul reste, le
 * dernier, sans temporaire.<br/>3. Le retirer.<br/>
 * }
 */
TEST_F(AutosaveTest, ReecrireRemplaceEtRetirerVide) {
    const hmi::AutosaveStore store(dir);
    hmi::AutosaveRecord record = laPlace();
    ASSERT_TRUE(store.write(record));
    record.draftJson = R"({"name": "La Place 2"})";
    ASSERT_TRUE(store.write(record));

    const std::vector<hmi::AutosaveRecord> pending = store.pending();
    ASSERT_EQ(pending.size(), 1U);
    EXPECT_EQ(pending.front().draftJson, record.draftJson);
    std::size_t files = 0;
    for ([[maybe_unused]] const auto& entry : std::filesystem::directory_iterator(dir)) {
        ++files;
    }
    EXPECT_EQ(files, 1U) << "aucun .tmp ne doit rester";

    store.discard(record.mapId);
    EXPECT_TRUE(store.pending().empty());
}

/**
 * @brief Un dossier absent ne rend rien ; un fichier illisible ou d'un autre format est ignoré et
 *        laissé en place, jamais effacé.
 * \castest{<b>Un fichier de reprise illisible est ignoré, pas effacé.</b><br/>
 * \tcat Unitaire · Éditeur, reprise après plantage<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lister un dossier absent.<br/>2. Déposer un fichier tronqué et un fichier d'un autre
 * format.<br/>3. Lister : rien ; les fichiers sont toujours là.<br/>
 * }
 */
TEST_F(AutosaveTest, UnFichierIlliblesEstIgnore) {
    const hmi::AutosaveStore store(dir);
    EXPECT_TRUE(store.pending().empty());

    std::filesystem::create_directories(dir);
    std::ofstream(dir / "a.autosave.json") << R"({"format": 1, "mapId": "a")";
    std::ofstream(dir / "b.autosave.json")
        << R"({"format": 99, "mapId": "b", "levelPath": "b.json", "draft": "{}"})";
    EXPECT_TRUE(store.pending().empty());
    EXPECT_TRUE(std::filesystem::exists(dir / "a.autosave.json"));
    EXPECT_TRUE(std::filesystem::exists(dir / "b.autosave.json"));
    EXPECT_FALSE(hmi::parseAutosave("pas du json").has_value());
}

/**
 * @brief Mettre une version de côté l'écrit sous `conflicts/`, hors des brouillons en attente.
 * \castest{<b>La version écartée par un choix de l'auteur est gardée de côté.</b><br/>
 * \tcat Unitaire · Éditeur, garde de fichier modifié sur disque<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Mettre de côté la version disque de la Place.<br/>2. Relire la copie.<br/>
 * 3. Vérifier qu'elle n'est pas proposée à la reprise.<br/>
 * }
 */
TEST_F(AutosaveTest, UneVersionEcarteeEstGardeeDeCote) {
    const hmi::AutosaveStore store(dir);
    const std::optional<std::filesystem::path> kept =
        store.keepAside("bourg/place", "disk", "20260918-142501", "contenu disque");
    ASSERT_TRUE(kept.has_value());
    EXPECT_EQ(kept->filename().string(), "bourg~place.disk.20260918-142501.json");
    EXPECT_EQ(kept->parent_path().filename().string(), "conflicts");
    std::ifstream file(*kept, std::ios::binary);
    const std::string content((std::istreambuf_iterator<char>(file)),
                              std::istreambuf_iterator<char>());
    EXPECT_EQ(content, "contenu disque");
    EXPECT_TRUE(store.pending().empty());
}
