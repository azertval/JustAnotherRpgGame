// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_mesh_file.cpp
 * @brief Le chargeur de maillages `.glb` (`LOT-1003`) : ce qu'il lit, ce qu'il refuse, et qu'il ne
 *        lève jamais.
 *
 * Les fichiers lus sont ceux de la carte d'essai (`Fixtures/Meshes`, écrits par
 * `scripts/assetsGeneration/build_mesh_fixture.py`) et des `.glb` bâtis en mémoire, un défaut à la
 * fois. Un dossier de modèles réels — les exports Meshy de l'atelier, jamais livrés — se lit en
 * plus quand la variable d'environnement `JADG_MESH_SAMPLES` le désigne.
 */

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Resources/MeshFile.h"

namespace {

constexpr float TOLERANCE = 1e-5F;

std::filesystem::path fixture(std::string_view file) {
    return std::filesystem::path(JADG_MESH_FIXTURE_DIR) / "Assets" / "Scene" / "ilot" / file;
}

// Une variable d'environnement, sans l'avertissement de MSVC sur getenv.
std::string environment(const char* name) {
    char* value = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
        return {};
    }
    std::string text{value};
    std::free(value);  // NOLINT(cppcoreguidelines-no-malloc) : allouee par _dupenv_s
    return text;
}

void appendU32(std::vector<std::byte>& out, std::uint32_t value) {
    std::array<std::byte, 4> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(value));
    out.insert(out.end(), bytes.begin(), bytes.end());
}

/// Un `.glb` : l'en-tête, le bloc JSON @p json complété à quatre octets, le bloc binaire @p binary.
std::vector<std::byte> glb(std::string json, std::vector<std::byte> binary = {},
                           std::uint32_t version = 2) {
    while (json.size() % 4 != 0) {
        json.push_back(' ');
    }
    while (binary.size() % 4 != 0) {
        binary.push_back(std::byte{0});
    }
    std::vector<std::byte> out;
    appendU32(out, 0x46546C67U);
    appendU32(out, version);
    appendU32(out, static_cast<std::uint32_t>(12 + 8 + json.size() +
                                              (binary.empty() ? 0 : 8 + binary.size())));
    appendU32(out, static_cast<std::uint32_t>(json.size()));
    appendU32(out, 0x4E4F534AU);
    for (const char letter : json) {
        out.push_back(static_cast<std::byte>(letter));
    }
    if (!binary.empty()) {
        appendU32(out, static_cast<std::uint32_t>(binary.size()));
        appendU32(out, 0x004E4942U);
        out.insert(out.end(), binary.begin(), binary.end());
    }
    return out;
}

/// Les octets de @p values, tels qu'un tampon glTF les range.
template <class Value>
std::vector<std::byte> bytesOf(const std::vector<Value>& values) {
    std::vector<std::byte> out(values.size() * sizeof(Value));
    std::memcpy(out.data(), values.data(), out.size());
    return out;
}

/// Un triangle rectangle d'un mètre de côté dans le plan du sol, sans indices ni matière : trois
/// positions de 12 octets. @p node décrit le nœud qui le porte, @p extra s'ajoute à la racine.
std::vector<std::byte> triangle(const std::string& node = R"({"mesh": 0})",
                                const std::string& extra = "") {
    const std::vector<float> positions = {0, 0, 0, 1, 0, 0, 0, 0, 1};
    return glb(R"({"asset": {"version": "2.0"}, "scene": 0, "scenes": [{"nodes": [0]}],
                  "nodes": [)" +
                   node + R"(],
                  "meshes": [{"primitives": [{"attributes": {"POSITION": 0}}]}],
                  "accessors": [{"bufferView": 0, "componentType": 5126, "count": 3,
                                 "type": "VEC3"}],
                  "bufferViews": [{"buffer": 0, "byteLength": 36}],
                  "buffers": [{"byteLength": 36}])" +
                   extra + "}",
               bytesOf(positions));
}

}  // namespace

/**
 * @brief Le mur de la carte d'essai se lit en entier : sommets, triangles, boîte, texture.
 * \castest{<b>Un maillage du standard se lit en entier.</b><br/>
 * \tcat Unitaire · Chargeur de maillages<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire `wall.glb`, le bloc de mur de la carte d'essai.<br/>
 *          2. Relever ses sommets, ses triangles, sa boîte et sa matière.<br/>
 * \tattendu Vingt sommets et dix triangles (quatre flancs et le dessus) ; une boîte d'une case de
 *           côté, du sol à la hauteur d'un étage ; des normales unitaires ; une primitive, une
 *           matière, une image PNG incorporée, pas de squelette.
 * }
 */
TEST(MeshFileTest, UnMaillageDuStandardSeLitEnEntier) {
    const core::MeshFileResult read = core::readMeshFile(fixture("wall.glb"));
    ASSERT_TRUE(read.ok()) << read.message;
    const core::MeshData& mesh = read.mesh;
    EXPECT_EQ(mesh.vertices.size(), 20U);
    EXPECT_EQ(mesh.triangleCount(), 10U);
    EXPECT_NEAR(mesh.minimum[0], -0.75F, TOLERANCE);
    EXPECT_NEAR(mesh.maximum[0], 0.75F, TOLERANCE);
    EXPECT_NEAR(mesh.minimum[1], 0.0F, TOLERANCE);
    // Un étage de 0,875 largeur de case, sous la caméra inclinée de asin 0,62 : 2,366 m.
    EXPECT_NEAR(mesh.maximum[1], 2.36573F, 1e-4F);
    EXPECT_NEAR(mesh.minimum[2], -0.75F, TOLERANCE);
    EXPECT_NEAR(mesh.maximum[2], 0.75F, TOLERANCE);
    for (const core::MeshVertex& vertex : mesh.vertices) {
        const float length = std::sqrt((vertex.normal[0] * vertex.normal[0]) +
                                       (vertex.normal[1] * vertex.normal[1]) +
                                       (vertex.normal[2] * vertex.normal[2]));
        EXPECT_NEAR(length, 1.0F, TOLERANCE);
    }
    for (const std::uint32_t index : mesh.indices) {
        EXPECT_LT(index, mesh.vertices.size());
    }
    EXPECT_EQ(mesh.primitiveCount, 1);
    EXPECT_EQ(mesh.materialCount, 1);
    EXPECT_FALSE(mesh.skinned);
    EXPECT_EQ(mesh.imageMimeType, "image/png");
    ASSERT_GT(mesh.image.size(), 8U);
    EXPECT_EQ(std::to_integer<int>(mesh.image[1]), 'P');
    EXPECT_FLOAT_EQ(mesh.baseColor[3], 1.0F);
}

/**
 * @brief La transformation du nœud est appliquée aux positions ; sans indices, les sommets vont
 *        par trois ; sans matière, la couleur est blanche et sans image.
 * \castest{<b>La transformation du noeud porte le maillage.</b><br/>
 * \tcat Unitaire · Chargeur de maillages<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Bâtir un triangle sans indices ni matière, porté par un nœud translaté de (10, 2, 0)
 *             et agrandi deux fois.<br/>2. Le lire.<br/>
 * \tattendu Trois sommets, un triangle ; les positions sont agrandies puis translatées ; la couleur
 *           de base est blanche, sans image.
 * }
 */
TEST(MeshFileTest, LaTransformationDuNoeudPorteLeMaillage) {
    const core::MeshFileResult read = core::readMeshFromGlb(
        triangle(R"({"mesh": 0, "translation": [10, 2, 0], "scale": [2, 2, 2]})"));
    ASSERT_TRUE(read.ok()) << read.message;
    ASSERT_EQ(read.mesh.vertices.size(), 3U);
    EXPECT_EQ(read.mesh.triangleCount(), 1U);
    EXPECT_NEAR(read.mesh.vertices[0].position[0], 10.0F, TOLERANCE);
    EXPECT_NEAR(read.mesh.vertices[0].position[1], 2.0F, TOLERANCE);
    EXPECT_NEAR(read.mesh.vertices[1].position[0], 12.0F, TOLERANCE);
    EXPECT_NEAR(read.mesh.vertices[2].position[2], 2.0F, TOLERANCE);
    EXPECT_NEAR(read.mesh.maximum[0], 12.0F, TOLERANCE);
    EXPECT_TRUE(read.mesh.image.empty());
    EXPECT_FLOAT_EQ(read.mesh.baseColor[0], 1.0F);
    EXPECT_EQ(read.mesh.materialCount, 1) << "la matière par défaut compte pour une";
}

/**
 * @brief Un enfant hérite de la transformation de son parent, et une rotation tourne les normales.
 * \castest{<b>Les noeuds s'enchainent et les normales tournent.</b><br/>
 * \tcat Unitaire · Chargeur de maillages<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Bâtir un triangle du sol, de normale +Y, porté par un nœud enfant d'un parent tourné
 *             d'un quart de tour autour de X.<br/>2. Le lire.<br/>
 * \tattendu La normale lue est +Z : le quart de tour autour de X envoie +Y sur +Z.
 * }
 */
TEST(MeshFileTest, LesNoeudsSEnchainentEtLesNormalesTournent) {
    const std::vector<float> data = {0, 0, 0, 1, 0, 0, 0, 0, 1,   // positions
                                     0, 1, 0, 0, 1, 0, 0, 1, 0};  // normales
    const std::vector<std::byte> file = glb(
        R"({"asset": {"version": "2.0"}, "scenes": [{"nodes": [0]}],
            "nodes": [{"children": [1], "rotation": [0.70710678, 0, 0, 0.70710678]},
                      {"mesh": 0}],
            "meshes": [{"primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1}}]}],
            "accessors": [
              {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"},
              {"bufferView": 0, "byteOffset": 36, "componentType": 5126, "count": 3,
               "type": "VEC3"}],
            "bufferViews": [{"buffer": 0, "byteLength": 72}],
            "buffers": [{"byteLength": 72}]})",
        bytesOf(data));
    const core::MeshFileResult read = core::readMeshFromGlb(file);
    ASSERT_TRUE(read.ok()) << read.message;
    ASSERT_EQ(read.mesh.vertices.size(), 3U);
    EXPECT_NEAR(read.mesh.vertices[0].normal[0], 0.0F, TOLERANCE);
    EXPECT_NEAR(read.mesh.vertices[0].normal[1], 0.0F, TOLERANCE);
    EXPECT_NEAR(read.mesh.vertices[0].normal[2], 1.0F, TOLERANCE);
}

/**
 * @brief Ce que le lecteur ne lit pas est refusé et nommé, jamais lu de travers.
 * \castest{<b>Ce que le chargeur ne lit pas est refuse et nomme.</b><br/>
 * \tcat Unitaire · Chargeur de maillages<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Présenter des octets qui ne sont pas un `.glb`, un glTF 1, un fichier tronqué.<br/>
 *          2. Présenter un `.glb` valide qui exige une extension, un accesseur creux, un tampon
 *             hors du fichier.<br/>
 * \tattendu Les premiers sont des erreurs de lecture ; les seconds sont « non pris en charge », et
 *           le message nomme l'extension exigée. Aucun ne lève.
 * }
 */
TEST(MeshFileTest, CeQuIlNeLitPasEstRefuseEtNomme) {
    const std::vector<std::byte> junk(64, std::byte{0x41});
    EXPECT_EQ(core::readMeshFromGlb(junk).error, core::MeshFileError::ParseError);
    EXPECT_EQ(core::readMeshFromGlb({}).error, core::MeshFileError::ParseError);
    EXPECT_EQ(core::readMeshFromGlb(glb("{}", {}, 1)).error, core::MeshFileError::ParseError);
    EXPECT_EQ(core::readMeshFromGlb(glb("[1, 2]")).error, core::MeshFileError::ParseError);

    std::vector<std::byte> truncated = triangle();
    truncated.resize(truncated.size() - 20);
    EXPECT_EQ(core::readMeshFromGlb(truncated).error, core::MeshFileError::ParseError)
        << "la longueur déclarée dépasse le fichier";

    const core::MeshFileResult draco = core::readMeshFromGlb(
        triangle(R"({"mesh": 0})", R"(, "extensionsRequired": ["KHR_draco_mesh_compression"])"));
    EXPECT_EQ(draco.error, core::MeshFileError::Unsupported);
    EXPECT_NE(draco.message.find("KHR_draco_mesh_compression"), std::string::npos);

    const core::MeshFileResult external = core::readMeshFromGlb(glb(
        R"({"scenes": [{"nodes": [0]}], "nodes": [{"mesh": 0}],
            "meshes": [{"primitives": [{"attributes": {"POSITION": 0}}]}],
            "accessors": [{"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"}],
            "bufferViews": [{"buffer": 0, "byteLength": 36}],
            "buffers": [{"byteLength": 36, "uri": "ailleurs.bin"}]})"));
    EXPECT_EQ(external.error, core::MeshFileError::Unsupported);

    const std::vector<float> positions = {0, 0, 0, 1, 0, 0, 0, 0, 1};
    const core::MeshFileResult sparse = core::readMeshFromGlb(glb(
        R"({"scenes": [{"nodes": [0]}], "nodes": [{"mesh": 0}],
            "meshes": [{"primitives": [{"attributes": {"POSITION": 0}}]}],
            "accessors": [{"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3",
                           "sparse": {"count": 1}}],
            "bufferViews": [{"buffer": 0, "byteLength": 36}],
            "buffers": [{"byteLength": 36}]})",
        bytesOf(positions)));
    EXPECT_EQ(sparse.error, core::MeshFileError::Unsupported);
}

/**
 * @brief Un fichier dont un index, une taille ou un graphe sort de ce qu'il contient est mal
 *        formé : rien n'est lu hors du fichier.
 * \castest{<b>Un fichier incoherent est mal forme, sans lecture hors bornes.</b><br/>
 * \tcat Unitaire · Chargeur de maillages<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Présenter un accesseur qui dépasse sa vue, une vue qui dépasse le bloc binaire, un
 *             indice au-delà des sommets, un graphe de nœuds en boucle, une scène sans
 *             triangle.<br/>
 * \tattendu Chacun rend « structure mal formée » et un maillage vide.
 * }
 */
TEST(MeshFileTest, UnFichierIncoherentEstMalForme) {
    const std::vector<float> positions = {0, 0, 0, 1, 0, 0, 0, 0, 1};
    const auto withAccessor = [&positions](const std::string& accessor, const std::string& view) {
        return core::readMeshFromGlb(glb(
            R"({"scenes": [{"nodes": [0]}], "nodes": [{"mesh": 0}],
                "meshes": [{"primitives": [{"attributes": {"POSITION": 0}}]}],
                "accessors": [)" +
                accessor + R"(], "bufferViews": [)" + view +
                R"(], "buffers": [{"byteLength": 36}]})",
            bytesOf(positions)));
    };
    // Quatre sommets annoncés, trois présents.
    EXPECT_EQ(
        withAccessor(R"({"bufferView": 0, "componentType": 5126, "count": 4, "type": "VEC3"})",
                     R"({"buffer": 0, "byteLength": 36})")
            .error,
        core::MeshFileError::MalformedStructure);
    // Une vue plus longue que le bloc binaire.
    EXPECT_EQ(
        withAccessor(R"({"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"})",
                     R"({"buffer": 0, "byteLength": 4096})")
            .error,
        core::MeshFileError::MalformedStructure);
    // Un compte démesuré : la borne se vérifie sans débordement d'entier.
    EXPECT_EQ(
        withAccessor(R"({"bufferView": 0, "componentType": 5126, "count": 18446744073709551615,
                               "type": "VEC3"})",
                     R"({"buffer": 0, "byteLength": 36})")
            .error,
        core::MeshFileError::MalformedStructure);

    // Un indice au-delà des trois sommets.
    std::vector<std::byte> binary = bytesOf(positions);
    const std::vector<std::uint16_t> indices = {0, 1, 7, 0};
    const std::vector<std::byte> indexBytes = bytesOf(indices);
    binary.insert(binary.end(), indexBytes.begin(), indexBytes.end());
    const core::MeshFileResult beyond = core::readMeshFromGlb(glb(
        R"({"scenes": [{"nodes": [0]}], "nodes": [{"mesh": 0}],
            "meshes": [{"primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]}],
            "accessors": [
              {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3"},
              {"bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR"}],
            "bufferViews": [{"buffer": 0, "byteLength": 36},
                            {"buffer": 0, "byteOffset": 36, "byteLength": 8}],
            "buffers": [{"byteLength": 44}]})",
        binary));
    EXPECT_EQ(beyond.error, core::MeshFileError::MalformedStructure);
    EXPECT_TRUE(beyond.mesh.vertices.empty());

    // Un nœud qui est son propre petit-enfant.
    const core::MeshFileResult loop =
        core::readMeshFromGlb(triangle(R"({"mesh": 0, "children": [0]})"));
    EXPECT_EQ(loop.error, core::MeshFileError::MalformedStructure);

    // Une scène valide, sans rien à dessiner.
    const core::MeshFileResult empty =
        core::readMeshFromGlb(glb(R"({"scenes": [{"nodes": [0]}], "nodes": [{}]})"));
    EXPECT_EQ(empty.error, core::MeshFileError::MalformedStructure);
}

/**
 * @brief Un fichier absent est une erreur du résultat ; un chemin se reconnaît à son extension.
 * \castest{<b>Un fichier absent est une erreur, et un maillage se reconnait a son
 * extension.</b><br/>
 * \tcat Unitaire · Chargeur de maillages<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire un fichier qui n'existe pas.<br/>2. Demander si des chemins sont des
 *          maillages.<br/>
 * \tattendu « Fichier introuvable », sans exception ; `.glb` et `.GLB` sont des maillages, `.png`
 *           et un nom plus court que l'extension n'en sont pas.
 * }
 */
TEST(MeshFileTest, UnFichierAbsentEstUneErreur) {
    const core::MeshFileResult missing = core::readMeshFile(fixture("absent.glb"));
    EXPECT_EQ(missing.error, core::MeshFileError::FileNotFound);
    EXPECT_FALSE(missing.message.empty());

    EXPECT_TRUE(core::isMeshPath("Scene/ilot/wall.glb"));
    EXPECT_TRUE(core::isMeshPath("WALL.GLB"));
    EXPECT_FALSE(core::isMeshPath("Scene/ilot/paving.png"));
    EXPECT_FALSE(core::isMeshPath("glb"));
    EXPECT_FALSE(core::isMeshPath(""));
}

/**
 * @brief Les modèles réels de l'atelier se lisent : un relevé, quand un dossier est désigné.
 * \castest{<b>Les modeles reels de l'atelier se lisent.</b><br/>
 * \tcat Unitaire · Chargeur de maillages<br/>
 * \tcrit Mineur<br/>
 * \tetapes 1. Lire chaque `.glb` du dossier que nomme la variable `JADG_MESH_SAMPLES`.<br/>
 * \tattendu Chacun se lit, a des triangles et une texture ; le relevé (triangles, primitives,
 *           durée) est écrit. Sans la variable, le test est sauté : l'atelier n'est jamais livré.
 * }
 */
TEST(MeshFileTest, LesModelesReelsDeLAtelierSeLisent) {
    const std::string samples = environment("JADG_MESH_SAMPLES");
    if (samples.empty() || !std::filesystem::is_directory(samples)) {
        GTEST_SKIP() << "JADG_MESH_SAMPLES ne désigne aucun dossier de modèles.";
    }
    std::size_t count = 0;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::recursive_directory_iterator(samples)) {
        if (!entry.is_regular_file() || !core::isMeshPath(entry.path().string())) {
            continue;
        }
        const auto start = std::chrono::steady_clock::now();
        const core::MeshFileResult read = core::readMeshFile(entry.path());
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        ASSERT_TRUE(read.ok()) << read.message;
        EXPECT_GT(read.mesh.triangleCount(), 0U) << entry.path();
        EXPECT_FALSE(read.mesh.image.empty()) << entry.path();
        std::cout << entry.path().filename().string() << " : " << read.mesh.triangleCount()
                  << " triangles, " << read.mesh.vertices.size() << " sommets, "
                  << read.mesh.primitiveCount << " primitive(s), " << read.mesh.materialCount
                  << " matiere(s), " << (read.mesh.skinned ? "squelette, " : "")
                  << read.mesh.imageMimeType << " " << read.mesh.image.size() << " octets, hauteur "
                  << (read.mesh.maximum[1] - read.mesh.minimum[1]) << " m, " << elapsed.count()
                  << " ms\n";
        ++count;
    }
    EXPECT_GT(count, 0U);
}
