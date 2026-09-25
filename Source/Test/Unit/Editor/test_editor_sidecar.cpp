// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_editor_sidecar.cpp
 * @brief Tests du fichier annexe d'une carte, `<carte>.editor.json`, et de ses notes d'auteur
 *        (`LOT-EDITOR-04`, `EX-EDIT-068`).
 */

#include <filesystem>
#include <string>

#include <gtest/gtest.h>

#include "Core/Levels/GridPosition.h"
#include "Editor/Logic/EditorSidecar.h"
#include "Editor/Logic/LevelFileOperations.h"

namespace {

using core::GridPosition;

/// Un dossier temporaire propre au test, retiré à la fin.
class DossierTemporaire {
public:
    explicit DossierTemporaire(const std::string& nom)
        : _chemin(std::filesystem::temp_directory_path() / ("jadg-sidecar-" + nom)) {
        std::filesystem::remove_all(_chemin);
        std::filesystem::create_directories(_chemin);
    }
    ~DossierTemporaire() {
        std::error_code ignored;
        std::filesystem::remove_all(_chemin, ignored);
    }
    DossierTemporaire(const DossierTemporaire&) = delete;
    DossierTemporaire& operator=(const DossierTemporaire&) = delete;
    DossierTemporaire(DossierTemporaire&&) = delete;
    DossierTemporaire& operator=(DossierTemporaire&&) = delete;

    [[nodiscard]] const std::filesystem::path& chemin() const noexcept {
        return _chemin;
    }

private:
    std::filesystem::path _chemin;
};

}  // namespace

/**
 * @brief L'annexe d'une carte se nomme d'après elle, et n'est jamais prise pour une carte.
 * \castest{<b>L'annexe vit à côté de la carte.</b><br/>
 * \tcat Unitaire · Notes d'auteur<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Nommer l'annexe de `bourg/place.json`.<br/>2. Lister un dossier de cartes qui
 * contient une carte et son annexe.<br/>
 * \tattendu `bourg/place.editor.json` ; le navigateur ne liste que la carte.
 * }
 */
TEST(EditorSidecarTest, LAnnexeVitACoteDeLaCarte) {
    EXPECT_EQ(hmi::sidecarPath(std::filesystem::path("bourg") / "place.json"),
              std::filesystem::path("bourg") / "place.editor.json");
    EXPECT_TRUE(hmi::isSidecarFile("place.editor.json"));
    EXPECT_FALSE(hmi::isSidecarFile("place.json"));

    const DossierTemporaire dossier("liste");
    const hmi::LevelFileOperations operations(dossier.chemin());
    ASSERT_TRUE(operations.create("place", 4, 4).ok());
    hmi::EditorSidecar annexe;
    hmi::setNote(annexe, {.column = 1, .row = 1}, "fountain here");
    ASSERT_TRUE(hmi::writeSidecar(hmi::sidecarPath(dossier.chemin() / "place.json"), annexe));
    const auto cartes = operations.list();
    ASSERT_EQ(cartes.size(), 1U);
    EXPECT_EQ(cartes.front().filename(), "place.json");
}

/**
 * @brief Une note par case, retirée par un texte vide ; l'écriture est canonique et relue à
 *        l'identique ; une annexe vide retire son fichier.
 * \castest{<b>Les notes s'écrivent, se relisent et se retirent.</b><br/>
 * \tcat Unitaire · Notes d'auteur<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Écrire deux notes, en réécrire une, écrire l'annexe.<br/>2. La relire.<br/>3.
 * Effacer les deux notes, réécrire.<br/>
 * \tattendu Notes triées ligne par ligne, texte JSON canonique ; relue égale ; vide, le fichier
 * part.
 * }
 */
TEST(EditorSidecarTest, LesNotesSEcriventSeRelisentEtSeRetirent) {
    hmi::EditorSidecar annexe;
    EXPECT_TRUE(hmi::setNote(annexe, {.column = 5, .row = 2}, "door to the tavern"));
    EXPECT_TRUE(hmi::setNote(annexe, {.column = 1, .row = 0}, "gate"));
    EXPECT_FALSE(hmi::setNote(annexe, {.column = 1, .row = 0}, "gate"));
    EXPECT_TRUE(hmi::setNote(annexe, {.column = 1, .row = 0}, "north gate"));
    EXPECT_FALSE(hmi::setNote(annexe, {.column = 9, .row = 9}, "   "));
    ASSERT_EQ(annexe.notes.size(), 2U);
    EXPECT_EQ(annexe.notes.front().cell, (GridPosition{1, 0}));
    ASSERT_NE(hmi::noteAt(annexe, {.column = 5, .row = 2}), nullptr);
    EXPECT_EQ(hmi::noteAt(annexe, {.column = 5, .row = 2})->text, "door to the tavern");
    EXPECT_EQ(hmi::noteAt(annexe, {.column = 0, .row = 0}), nullptr);

    EXPECT_EQ(hmi::sidecarJson(annexe), R"({
  "notes": [
    {
      "column": 1,
      "row": 0,
      "text": "north gate"
    },
    {
      "column": 5,
      "row": 2,
      "text": "door to the tavern"
    }
  ],
  "version": 1
}
)");

    const DossierTemporaire dossier("notes");
    const std::filesystem::path fichier = dossier.chemin() / "sub" / "place.editor.json";
    ASSERT_TRUE(hmi::writeSidecar(fichier, annexe));
    const hmi::SidecarReadResult relue = hmi::readSidecar(fichier);
    EXPECT_TRUE(relue.warning.empty());
    EXPECT_EQ(relue.sidecar, annexe);

    EXPECT_TRUE(hmi::setNote(annexe, {.column = 1, .row = 0}, ""));
    EXPECT_TRUE(hmi::setNote(annexe, {.column = 5, .row = 2}, "\n"));
    ASSERT_TRUE(hmi::writeSidecar(fichier, annexe));
    EXPECT_FALSE(std::filesystem::exists(fichier));
}

/**
 * @brief Une clé inconnue est gardée et réécrite ; une note mal formée est ignorée, un texte
 *        illisible rend une annexe vide, chacun avec un avertissement ; un fichier absent, rien.
 * \castest{<b>L'annexe tolère et garde ce qu'elle ne connaît pas.</b><br/>
 * \tcat Unitaire · Notes d'auteur<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire une annexe avec une clé `lockedRegions` et une note sans texte.<br/>2. Lire un
 * texte qui n'est pas du JSON.<br/>3. Lire un fichier absent.<br/>
 * \tattendu La clé revient à l'écriture ; une note sur deux, un avertissement ; annexe vide et
 * avertissement ; annexe vide sans avertissement.
 * }
 */
TEST(EditorSidecarTest, LAnnexeTolereEtGardeCeQuElleNeConnaitPas) {
    const hmi::SidecarReadResult lue = hmi::parseSidecar(R"({
  "lockedRegions": [[0, 0, 3, 3]],
  "notes": [ { "column": 2, "row": 3, "text": "well" }, { "column": 4, "row": 1 } ],
  "version": 1
})");
    EXPECT_FALSE(lue.warning.empty());
    ASSERT_EQ(lue.sidecar.notes.size(), 1U);
    EXPECT_NE(hmi::sidecarJson(lue.sidecar).find("\"lockedRegions\""), std::string::npos);

    const hmi::SidecarReadResult illisible = hmi::parseSidecar("not json");
    EXPECT_FALSE(illisible.warning.empty());
    EXPECT_TRUE(illisible.sidecar.empty());

    const hmi::SidecarReadResult absente = hmi::readSidecar(std::filesystem::temp_directory_path() /
                                                            "jadg-sidecar-absente.editor.json");
    EXPECT_TRUE(absente.warning.empty());
    EXPECT_TRUE(absente.sidecar.empty());
}

/**
 * @brief L'état d'une carte (`LOT-EDITOR-09`) vit dans l'annexe : écrit, relu, et absent tant que
 *        l'auteur n'a rien dit.
 * \castest{<b>L'annexe garde où en est la carte.</b><br/>
 * \tcat Unitaire · Le monde<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Écrire une annexe sans état, puis avec « finished ».<br/>2. Relire une annexe dont
 * l'état est un mot inconnu.<br/>
 * \tattendu Sans état, l'annexe est vide et son texte ne porte pas la clé ; avec, elle se relit
 * telle quelle ; un mot inconnu vaut « rien dit ».
 * }
 */
TEST(EditorSidecarTest, LAnnexeGardeOuEnEstLaCarte) {
    hmi::EditorSidecar annexe;
    EXPECT_TRUE(annexe.empty());
    EXPECT_EQ(hmi::sidecarJson(annexe).find("\"state\""), std::string::npos);

    annexe.state = hmi::MapState::Finished;
    EXPECT_FALSE(annexe.empty());
    const hmi::SidecarReadResult relue = hmi::parseSidecar(hmi::sidecarJson(annexe));
    EXPECT_TRUE(relue.warning.empty());
    EXPECT_EQ(relue.sidecar.state, hmi::MapState::Finished);

    EXPECT_EQ(hmi::parseSidecar(R"({"state": "polished", "version": 1})").sidecar.state,
              hmi::MapState::Unset);
    EXPECT_EQ(hmi::mapStateKey(hmi::MapState::Generated), "generated");
    EXPECT_EQ(hmi::mapStateFromKey("retouched"), hmi::MapState::Retouched);
    EXPECT_EQ(hmi::mapStateLabel(hmi::MapState::Unset), "not stated");
    // Maquettee (LOT-128) s'ajoute aux trois etats, entre « sortie du generateur » et « retouchee
    // ».
    EXPECT_EQ(hmi::knownMapStates().size(), 4U);
    EXPECT_EQ(hmi::mapStateKey(hmi::MapState::Blockout), "blockout");
    EXPECT_EQ(hmi::mapStateFromKey("blockout"), hmi::MapState::Blockout);
    EXPECT_EQ(hmi::mapStateLabel(hmi::MapState::Blockout), "Blockout");
    EXPECT_EQ(hmi::knownMapStates()[1], hmi::MapState::Blockout);
}
