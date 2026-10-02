// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_editor_key_bindings.cpp
 * @brief Tests unitaires du remappage des touches de l'éditeur (`EditorKeyBindings`,
 *        `EX-CTRL-012`).
 */

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "Editor/Logic/EditorKeyBindings.h"

/**
 * @brief Les dix actions ont leurs valeurs par défaut (S/Z/Y/C/V/P/F10/F1/F2/T) à la construction.
 * \castest{<b>Les dix actions ont leurs valeurs par défaut (S/Z/Y/C/V/P/F10/F1/F2/T) à la
 * construction.</b><br/>
 * \tcat Unitaire · Editor Key Bindings<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Les dix actions ont leurs valeurs par défaut (S/Z/Y/C/V/P/F10/F1/F2/T) à la
 * construction.
 * }
 */
TEST(EditorKeyBindingsTest, ValeursParDefautALaConstruction) {
    const hmi::EditorKeyBindings bindings;
    EXPECT_EQ(bindings.key(hmi::EditorAction::Save), hmi::Key::S);
    EXPECT_EQ(bindings.key(hmi::EditorAction::Undo), hmi::Key::Z);
    EXPECT_EQ(bindings.key(hmi::EditorAction::Redo), hmi::Key::Y);
    EXPECT_EQ(bindings.key(hmi::EditorAction::Copy), hmi::Key::C);
    EXPECT_EQ(bindings.key(hmi::EditorAction::Paste), hmi::Key::V);
    EXPECT_EQ(bindings.key(hmi::EditorAction::Playtest), hmi::Key::P);
    EXPECT_EQ(bindings.key(hmi::EditorAction::ToggleGrid), hmi::Key::F10);
    EXPECT_EQ(bindings.key(hmi::EditorAction::ToggleHelp), hmi::Key::F1);
    EXPECT_EQ(bindings.key(hmi::EditorAction::Rename), hmi::Key::F2);
}

/**
 * @brief `setKey` sur une touche déjà liée à une autre action échange les deux touches.
 * \castest{<b>`setKey` sur une touche déjà liée à une autre action échange les deux
 * touches.</b><br/>
 * \tcat Unitaire · Editor Key Bindings<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu `setKey` sur une touche déjà liée à une autre action échange les deux touches.
 * }
 */
TEST(EditorKeyBindingsTest, SetKeyEchangeSurConflit) {
    hmi::EditorKeyBindings bindings;
    bindings.setKey(hmi::EditorAction::Rename, hmi::Key::S);
    EXPECT_EQ(bindings.key(hmi::EditorAction::Rename), hmi::Key::S);
    EXPECT_EQ(bindings.key(hmi::EditorAction::Save), hmi::Key::F2);
}

/**
 * @brief `resetToDefaults` restaure les valeurs par défaut après un ou plusieurs remaps.
 * \castest{<b>`resetToDefaults` restaure les valeurs par défaut après un ou plusieurs
 * remaps.</b><br/>
 * \tcat Unitaire · Editor Key Bindings<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu `resetToDefaults` restaure les valeurs par défaut après un ou plusieurs remaps.
 * }
 */
TEST(EditorKeyBindingsTest, ResetToDefaultsRestaureLesDefauts) {
    hmi::EditorKeyBindings bindings;
    bindings.setKey(hmi::EditorAction::Playtest, hmi::Key::A);
    bindings.resetToDefaults();
    EXPECT_EQ(bindings.key(hmi::EditorAction::Playtest), hmi::Key::P);
}

/**
 * @brief Sauvegarder puis recharger préserve un jeu de bindings personnalisé (aller-retour JSON).
 * \castest{<b>Sauvegarder puis recharger préserve un jeu de bindings personnalisé (aller-retour
 * JSON).</b><br/>
 * \tcat Unitaire · Editor Key Bindings<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Sauvegarder puis recharger préserve un jeu de bindings personnalisé (aller-retour
 * JSON).
 * }
 */
TEST(EditorKeyBindingsTest, SaveEtLoadAllerRetour) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "jadg_test_editor_bindings.json";
    std::filesystem::remove(path);

    hmi::EditorKeyBindings original;
    original.setKey(hmi::EditorAction::Playtest, hmi::Key::F1);
    ASSERT_TRUE(original.save(path));

    const hmi::EditorKeyBindings reloaded = hmi::EditorKeyBindings::load(path);
    std::filesystem::remove(path);

    EXPECT_EQ(reloaded.key(hmi::EditorAction::Playtest), hmi::Key::F1);
    EXPECT_EQ(reloaded.key(hmi::EditorAction::Save), hmi::Key::S);
}

/**
 * @brief Charger depuis un fichier absent renvoie les valeurs par défaut, sans erreur.
 * \castest{<b>Charger depuis un fichier absent renvoie les valeurs par défaut, sans
 * erreur.</b><br/>
 * \tcat Unitaire · Editor Key Bindings<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Charger depuis un fichier absent renvoie les valeurs par défaut, sans erreur.
 * }
 */
TEST(EditorKeyBindingsTest, LoadFichierAbsentRenvoieLesDefauts) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "jadg_test_editor_bindings_absent.json";
    std::filesystem::remove(path);

    const hmi::EditorKeyBindings bindings = hmi::EditorKeyBindings::load(path);
    EXPECT_EQ(bindings.key(hmi::EditorAction::Save), hmi::Key::S);
}

/**
 * @brief Sauvegarder les touches d'éditeur préserve une section « jeu » déjà présente.
 * \castest{<b>Sauvegarder les touches d'éditeur préserve une section « jeu » déjà
 * présente.</b><br/>
 * \tcat Unitaire · Editor Key Bindings<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Mettre en place le contexte du test (arrangement).<br/>2. Executer le scenario et
 * verifier les assertions.<br/>
 * \tattendu Sauvegarder les touches d'éditeur préserve une section « jeu » déjà présente.
 * }
 */
TEST(EditorKeyBindingsTest, SavePreserveLaSectionJeu) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "jadg_test_editor_bindings_partage.json";
    {
        std::ofstream file(path, std::ios::binary);
        file << R"({"jeu": {"sauter": 32}})";
    }

    hmi::EditorKeyBindings bindings;
    ASSERT_TRUE(bindings.save(path));

    std::string content;
    {
        std::ifstream reread(path, std::ios::binary);
        content.assign(std::istreambuf_iterator<char>(reread), std::istreambuf_iterator<char>());
    }
    std::filesystem::remove(path);

    EXPECT_NE(content.find("\"jeu\""), std::string::npos);
    EXPECT_NE(content.find("sauter"), std::string::npos);
}

/**
 * @brief Sauvegarder les touches d'éditeur préserve une section inconnue déjà présente
 *        (symétrique de `SavePreserveLaSectionJeu`).
 * \castest{<b>Sauvegarder les touches d'editeur preserve une section inconnue deja
 * presente.</b><br/> \tcat Unitaire · Editor Key Bindings<br/> \tcrit Majeur<br/> \tetapes 1.
 * Ecrire un fichier avec une section de métadonnées.<br/>2. Sauvegarder des touches d'editeur
 * dessus.<br/>3. Relire le fichier.<br/> \tattendu La section et son contenu sont toujours
 * presents.
 * }
 */
TEST(EditorKeyBindingsTest, SavePreserveLaSectionInconnue) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "jadg_test_editor_bindings_metadata.json";
    {
        std::ofstream file(path, std::ios::binary);
        file << R"({"metadata": {"revision": 1}})";
    }

    hmi::EditorKeyBindings bindings;
    ASSERT_TRUE(bindings.save(path));

    std::string content;
    {
        std::ifstream reread(path, std::ios::binary);
        content.assign(std::istreambuf_iterator<char>(reread), std::istreambuf_iterator<char>());
    }
    std::filesystem::remove(path);

    EXPECT_NE(content.find("\"metadata\""), std::string::npos);
    EXPECT_NE(content.find("revision"), std::string::npos);
}
