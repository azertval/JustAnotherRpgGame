// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include <gtest/gtest.h>

#include "Editor/Logic/DiskGuard.h"

namespace {

class DiskGuardTest : public ::testing::Test {
protected:
    std::filesystem::path file;

    void SetUp() override {
        file =
            std::filesystem::temp_directory_path() /
            ("jadg_diskguard_" + std::to_string(reinterpret_cast<std::uintptr_t>(this)) + ".json");
    }
    void TearDown() override {
        std::error_code error;
        std::filesystem::remove(file, error);
    }

    void writeFile(const std::string& content) const {
        std::ofstream(file, std::ios::binary | std::ios::trunc) << content;
    }
};

}  // namespace

/**
 * @brief L'empreinte d'un fichier est celle de son contenu : réécrire à l'identique ne change rien,
 *        changer un octet se voit, et un fichier absent est signalé comme disparu.
 * \castest{<b>L'empreinte suit le contenu du fichier, pas sa date.</b><br/>
 * \tcat Unitaire · Éditeur, garde de fichier modifié sur disque<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Écrire une carte et relever son empreinte.<br/>2. La réécrire à l'identique, puis
 * changer un octet, puis la supprimer.<br/>3. Comparer à chaque étape.<br/>
 * }
 */
TEST_F(DiskGuardTest, LEmpreinteSuitLeContenu) {
    writeFile(R"({"name": "La Place"})");
    const hmi::FileFingerprint known = hmi::fingerprintFile(file);
    EXPECT_TRUE(known.exists);
    EXPECT_EQ(known, hmi::fingerprintOf(R"({"name": "La Place"})"));

    writeFile(R"({"name": "La Place"})");
    EXPECT_EQ(hmi::compareFingerprints(known, hmi::fingerprintFile(file)), hmi::DiskChange::None);

    writeFile(R"({"name": "Martparu"})");
    EXPECT_EQ(hmi::compareFingerprints(known, hmi::fingerprintFile(file)),
              hmi::DiskChange::Modified);

    std::filesystem::remove(file);
    EXPECT_FALSE(hmi::fingerprintFile(file).exists);
    EXPECT_EQ(hmi::compareFingerprints(known, hmi::fingerprintFile(file)),
              hmi::DiskChange::Deleted);
}

/**
 * @brief Un fichier apparu là où il n'y en avait pas est un changement, pas une disparition.
 * \castest{<b>Un fichier apparu sous une carte jamais enregistrée est un changement.</b><br/>
 * \tcat Unitaire · Éditeur, garde de fichier modifié sur disque<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Comparer une empreinte absente à celle d'un contenu.<br/>
 * }
 */
TEST_F(DiskGuardTest, UnFichierApparuEstUnChangement) {
    EXPECT_EQ(hmi::compareFingerprints(hmi::FileFingerprint{}, hmi::fingerprintOf("{}")),
              hmi::DiskChange::Modified);
    EXPECT_EQ(hmi::compareFingerprints(hmi::FileFingerprint{}, hmi::FileFingerprint{}),
              hmi::DiskChange::None);
}

/**
 * @brief La réaction ne demande à l'auteur que lorsqu'il a quelque chose à perdre.
 * \castest{<b>Brouillon modifié et fichier changé : l'éditeur demande ; sinon il relit.</b><br/>
 * \tcat Unitaire · Éditeur, garde de fichier modifié sur disque<br/>
 * \tcrit Critique<br/>
 * \tetapes 1. Croiser chaque changement avec un brouillon intact et un brouillon modifié.<br/>
 * }
 */
TEST_F(DiskGuardTest, LAuteurNEstInterrogeQueSIlAQuelqueChoseAPerdre) {
    using hmi::DiskChange;
    using hmi::DiskReaction;
    EXPECT_EQ(hmi::reactToDiskChange(DiskChange::None, true), DiskReaction::Ignore);
    EXPECT_EQ(hmi::reactToDiskChange(DiskChange::Modified, false), DiskReaction::ReloadQuietly);
    EXPECT_EQ(hmi::reactToDiskChange(DiskChange::Modified, true), DiskReaction::AskReloadOrKeep);
    EXPECT_EQ(hmi::reactToDiskChange(DiskChange::Deleted, false), DiskReaction::WarnDeleted);
    EXPECT_EQ(hmi::reactToDiskChange(DiskChange::Deleted, true), DiskReaction::WarnDeleted);
}
