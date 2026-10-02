// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

/**
 * @file test_skeleton.cpp
 * @brief Le squelette d'un modèle animé, sans GPU (`LOT-1005`) : ce que le chargeur lit d'un
 *        `.glb` lié, la pose d'un os à un instant, et ce que `skeleton.json` et `character.json`
 *        déclarent.
 *
 * Le modèle lu est le **pantin** de la carte d'essai (`Fixtures/Meshes`, écrit par
 * `scripts/assetsGeneration/build_mesh_fixture.py`) : trois blocs empilés, trois os, chaque sommet
 * lié à un seul os, six clips de deux à quatre clés — ce qu'une pose doit donner se calcule de
 * tête.
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <numbers>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include "Core/Resources/MeshFile.h"
#include "Core/Resources/SkeletonFile.h"
#include "Core/Resources/SkeletonPose.h"

namespace {

constexpr float TOLERANCE = 1e-4F;

std::filesystem::path assets() {
    return std::filesystem::path(JADG_MESH_FIXTURE_DIR) / "Assets";
}

core::MeshData puppet() {
    core::MeshFileResult read = core::readMeshFile(assets() / "Npc" / "pantin" / "pantin.glb");
    EXPECT_TRUE(read.ok()) << read.message;
    return std::move(read.mesh);
}

/// La pose du pantin à l'instant @p seconds de son clip @p clip (vide : la pose de repos).
std::vector<float> poseAt(const core::MeshData& mesh, std::string_view clip, float seconds) {
    std::vector<float> pose(mesh.rig.joints.size() * 16);
    core::poseSkeleton(mesh.rig, clip.empty() ? nullptr : core::findClip(mesh.rig, clip), seconds,
                       pose);
    return pose;
}

/// Où la pose @p pose met le point @p point, lié tout entier à l'os @p joint.
std::array<float, 3> moved(const std::vector<float>& pose, std::uint16_t joint,
                           const std::array<float, 3>& point) {
    return core::skinnedPosition(
        pose, core::MeshSkinVertex{.joints = {joint, 0, 0, 0}, .weights = {1.0F, 0.0F, 0.0F, 0.0F}},
        point);
}

void expectPoint(const std::array<float, 3>& actual, const std::array<float, 3>& expected) {
    for (std::size_t axis = 0; axis < 3; ++axis) {
        EXPECT_NEAR(actual[axis], expected[axis], TOLERANCE) << "axe " << axis;
    }
}

}  // namespace

/**
 * @brief Le chargeur lit le squelette d'un modèle lié : ses os, leur parenté, la liaison de chaque
 *        sommet et ses clips.
 * \castest{<b>Le chargeur lit le squelette et les clips d'un modele lie.</b><br/>
 * \tcat Unitaire · Maillages · Squelette<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire le pantin de la carte d'essai.<br/>
 * \tattendu Trois os -- Root, spine_01, head --, chacun enfant du precedent, ranges parents
 *           avant enfants ; une liaison par sommet, de poids 1 sur un os du squelette ; six clips,
 *           aux durees ecrites par le script ; la matrice de liaison inverse de la tete la ramene
 *           de 1,5 m.
 * }
 */
TEST(SkeletonTest, LeChargeurLitLeSqueletteEtLesClips) {
    const core::MeshData mesh = puppet();
    ASSERT_TRUE(mesh.skinned);
    ASSERT_EQ(mesh.rig.joints.size(), 3U);
    EXPECT_EQ(mesh.rig.joints[0].name, "Root");
    EXPECT_EQ(mesh.rig.joints[1].name, "spine_01");
    EXPECT_EQ(mesh.rig.joints[2].name, "head");
    EXPECT_EQ(mesh.rig.joints[0].parent, -1);
    EXPECT_EQ(mesh.rig.joints[1].parent, 0);
    EXPECT_EQ(mesh.rig.joints[2].parent, 1);
    EXPECT_EQ(mesh.rig.order, (std::vector<std::uint16_t>{0, 1, 2}));
    EXPECT_NEAR(mesh.rig.joints[2].translation[1], 0.6F, TOLERANCE) << "la tete, 0,6 m au-dessus";
    EXPECT_NEAR(mesh.rig.joints[2].inverseBind[13], -1.5F, TOLERANCE);

    ASSERT_EQ(mesh.skin.size(), mesh.vertices.size());
    for (std::size_t index = 0; index < mesh.skin.size(); ++index) {
        EXPECT_LT(mesh.skin[index].joints[0], 3U);
        EXPECT_FLOAT_EQ(mesh.skin[index].weights[0], 1.0F);
    }
    // Les sommets du haut suivent la tete, ceux du bas la racine.
    for (std::size_t index = 0; index < mesh.vertices.size(); ++index) {
        const float height = mesh.vertices[index].position[1];
        if (height > 1.6F) {
            EXPECT_EQ(mesh.skin[index].joints[0], 2U);
        } else if (height < 0.8F) {
            EXPECT_EQ(mesh.skin[index].joints[0], 0U);
        }
    }

    ASSERT_EQ(mesh.rig.clips.size(), 6U);
    const core::MeshClip* const walk = core::findClip(mesh.rig, "walk");
    ASSERT_NE(walk, nullptr);
    EXPECT_FLOAT_EQ(walk->duration, 0.5F);
    EXPECT_EQ(walk->tracks.size(), 3U);
    EXPECT_EQ(walk->tracks[1].rotation.times.size(), 4U);
    EXPECT_TRUE(walk->tracks[0].rotation.empty()) << "la marche n'anime que le buste";
    const core::MeshClip* const death = core::findClip(mesh.rig, "death");
    ASSERT_NE(death, nullptr);
    EXPECT_FLOAT_EQ(death->duration, 0.8F);
    EXPECT_EQ(death->tracks[0].translation.times.size(), 2U);
    EXPECT_EQ(core::findClip(mesh.rig, "dance"), nullptr);
}

/**
 * @brief La pose d'un os à un instant : au repos rien ne bouge ; à l'image clé de l'attaque, le
 *        buste est penché de 60° ; à la fin de la chute, le corps est couché sur le sol.
 * \castest{<b>La pose d'un os a un instant d'un clip.</b><br/>
 * \tcat Unitaire · Maillages · Squelette<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Poser le pantin au repos.<br/>2. Le poser a 0,4 s de son attaque (le buste penche de
 *          60 degres autour de la hanche, a 0,9 m), puis a 0,2 s (30 degres).<br/>3. Le poser a la
 *          fin de sa chute (la racine basculee de 90 degres en arriere, soulevee de 0,2 m).<br/>
 * \tattendu Au repos, chaque point reste ou il est. A 0,4 s, le sommet du crane (0 ; 1,8 ; 0)
 *           est en (0 ; 1,35 ; 0,7794) et les pieds n'ont pas bouge ; a 0,2 s, la rotation est a
 *           mi-chemin. A la fin de la chute, le sommet du crane est en (0 ; 0,2 ; -1,8).
 * }
 */
TEST(SkeletonTest, LaPoseDUnOsAUnInstant) {
    const core::MeshData mesh = puppet();
    const std::array<float, 3> crown{0.0F, 1.8F, 0.0F};
    const std::array<float, 3> toe{0.2F, 0.0F, 0.2F};

    const std::vector<float> rest = poseAt(mesh, {}, 0.0F);
    expectPoint(moved(rest, 2, crown), crown);
    expectPoint(moved(rest, 0, toe), toe);

    const std::vector<float> strike = poseAt(mesh, "attack", 0.4F);
    const float lean = 0.9F;  // du pivot de la hanche au sommet du crane
    expectPoint(moved(strike, 2, crown),
                {0.0F, 0.9F + (lean * std::cos(std::numbers::pi_v<float> / 3.0F)),
                 lean * std::sin(std::numbers::pi_v<float> / 3.0F)});
    expectPoint(moved(strike, 0, toe), toe);

    const std::vector<float> halfway = poseAt(mesh, "attack", 0.2F);
    expectPoint(moved(halfway, 2, crown),
                {0.0F, 0.9F + (lean * std::cos(std::numbers::pi_v<float> / 6.0F)),
                 lean * std::sin(std::numbers::pi_v<float> / 6.0F)});

    const std::vector<float> fallen = poseAt(mesh, "death", 0.8F);
    expectPoint(moved(fallen, 2, crown), {0.0F, 0.2F, -1.8F});
    // Apres la derniere cle, la cle du bord vaut : un mort reste couche.
    expectPoint(moved(poseAt(mesh, "death", 5.0F), 2, crown), {0.0F, 0.2F, -1.8F});
}

/**
 * @brief Un clip qui boucle est ramené dans sa durée ; un clip joué une fois se fige sur sa fin.
 * \castest{<b>L'instant d'un clip : ramene s'il boucle, fige sinon.</b><br/>
 * \tcat Unitaire · Maillages · Squelette<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Demander l'instant d'un clip de 0,5 s a 1,7 s, en boucle puis joue une fois.<br/>
 *          2. Demander un instant negatif, et un clip sans duree.<br/>
 * \tattendu En boucle, 0,2 s ; joue une fois, 0,5 s ; un instant negatif d'une boucle revient dans
 *           le clip, celui d'un clip joue une fois vaut 0 ; un clip sans duree vaut 0.
 * }
 */
TEST(SkeletonTest, LInstantDUnClip) {
    EXPECT_NEAR(core::clipTime(1.7F, 0.5F, true), 0.2F, TOLERANCE);
    EXPECT_FLOAT_EQ(core::clipTime(1.7F, 0.5F, false), 0.5F);
    EXPECT_NEAR(core::clipTime(-0.1F, 0.5F, true), 0.4F, TOLERANCE);
    EXPECT_FLOAT_EQ(core::clipTime(-0.1F, 0.5F, false), 0.0F);
    EXPECT_FLOAT_EQ(core::clipTime(3.0F, 0.0F, true), 0.0F);
}

/**
 * @brief `skeleton.json` déclare les os et, clip par clip, la durée, la boucle et l'image clé.
 * \castest{<b>La description d'un squelette se lit, et ses defauts sont dits.</b><br/>
 * \tcat Unitaire · Maillages · Squelette<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire la description du squelette du pantin.<br/>2. Lire cinq descriptions fautives :
 *          un os dont le parent n'est pas declare avant lui, un os declare deux fois, un clip sans
 *          duree, une image cle hors du clip, une version inconnue.<br/>
 * \tattendu La premiere donne trois os et six clips : la marche boucle sur 0,5 s sans image cle,
 *           l'attaque dure 0,8 s, se joue une fois et porte a 0,4 s. Les cinq autres echouent avec
 *           un message, sans lever.
 * }
 */
TEST(SkeletonTest, LaDescriptionDUnSqueletteSeLit) {
    const core::SkeletonFileResult read =
        core::readSkeletonFile(assets() / core::skeletonFilePath("pantin"));
    ASSERT_TRUE(read.ok()) << read.message;
    EXPECT_EQ(read.skeleton.silhouette, "pantin");
    ASSERT_EQ(read.skeleton.bones.size(), 3U);
    EXPECT_EQ(read.skeleton.bones[2], (core::SkeletonBone{.name = "head", .parent = "spine_01"}));
    ASSERT_EQ(read.skeleton.clips.size(), 6U);
    const core::SkeletonClip* const walk = read.skeleton.clip("walk");
    ASSERT_NE(walk, nullptr);
    EXPECT_TRUE(walk->loop);
    EXPECT_FLOAT_EQ(walk->duration, 0.5F);
    EXPECT_FALSE(walk->key.has_value());
    const core::SkeletonClip* const attack = read.skeleton.clip("attack");
    ASSERT_NE(attack, nullptr);
    EXPECT_FALSE(attack->loop);
    EXPECT_FLOAT_EQ(attack->duration, 0.8F);
    ASSERT_TRUE(attack->key.has_value());
    EXPECT_FLOAT_EQ(*attack->key, 0.4F);
    EXPECT_EQ(read.skeleton.clip("dance"), nullptr);

    const auto broken = [](const std::string& bones, const std::string& clips, int version = 1) {
        return core::readSkeletonDescription("{\"version\":" + std::to_string(version) +
                                             ",\"silhouette\":\"essai\",\"bones\":[" + bones +
                                             "],\"clips\":[" + clips + "]}");
    };
    const std::string root = R"({"name":"Root","parent":""})";
    EXPECT_TRUE(broken(root, R"({"name":"idle","duration":1.0,"loop":true})").ok());
    EXPECT_FALSE(broken(R"({"name":"head","parent":"spine"})", "").ok());
    EXPECT_FALSE(broken(root + "," + root, "").ok());
    EXPECT_FALSE(broken(root, R"({"name":"idle","loop":true})").ok());
    EXPECT_FALSE(broken(root, R"({"name":"attack","duration":0.5,"key":0.9})").ok());
    EXPECT_FALSE(broken(root, "", 2).ok());
    EXPECT_FALSE(core::readSkeletonDescription("pas du json").ok());
    EXPECT_FALSE(core::readSkeletonFile(assets() / "absent.json").ok());
}

/**
 * @brief La fiche d'un personnage nomme son modèle et son squelette, et ne sort pas de son
 *        dossier.
 * \castest{<b>La fiche d'un personnage nomme son modele et son squelette.</b><br/>
 * \tcat Unitaire · Maillages · Squelette<br/>
 * \tcrit Majeur<br/>
 * \tetapes 1. Lire la fiche du pantin.<br/>2. Lire une fiche sans modele, une fiche sans
 *          squelette, une fiche dont le modele est un chemin.<br/>
 * \tattendu La premiere donne pantin.glb et le squelette pantin ; les trois autres echouent avec
 *           un message.
 * }
 */
TEST(SkeletonTest, LaFicheDUnPersonnage) {
    const core::CharacterSheetFileResult read =
        core::readCharacterSheetFile(assets() / "Npc" / "pantin" / core::CHARACTER_SHEET_FILE);
    ASSERT_TRUE(read.ok()) << read.message;
    EXPECT_EQ(read.sheet, (core::CharacterSheetFile{.model = "pantin.glb", .skeleton = "pantin"}));
    EXPECT_FALSE(core::readCharacterSheet(R"({"version":1,"skeleton":"humanoid"})").ok());
    EXPECT_FALSE(core::readCharacterSheet(R"({"version":1,"model":"a.glb"})").ok());
    EXPECT_FALSE(
        core::readCharacterSheet(R"({"version":1,"model":"../a.glb","skeleton":"humanoid"})").ok());
}

/**
 * @brief Le modèle d'essai — le mannequin humanoïde réduit, lié par la chaîne de l'atelier — se
 *        lit avec les 53 os du squelette commun et ses six clips, tels que `skeleton.json` les
 *        déclare, et ses pieds restent au sol.
 * \castest{<b>Le modele d'essai porte les 53 os et les six clips du squelette commun.</b><br/>
 * \tcat Unitaire · Maillages · Squelette<br/>
 * \tcrit Bloquant<br/>
 * \tetapes 1. Lire le mannequin d'essai et la description du squelette humanoide.<br/>
 *          2. Comparer os et clips.<br/>3. Poser le modele a huit instants de chaque clip et
 *          deformer tous ses sommets.<br/>
 * \tattendu 53 os, aux noms et aux parents de la description, quatre influences au plus par
 *           sommet ; six clips, chacun de la duree declaree a un millieme pres ; toutes les
 *           positions deformees sont finies ; a la marche et au repos le point le plus bas ne
 *           s'enfonce pas de 5 mm et ne s'eleve pas de 2 cm.
 * }
 */
TEST(SkeletonTest, LeModeleDEssaiPorteLeSqueletteCommun) {
    const std::filesystem::path root = std::filesystem::path(JADG_CHARACTER_FIXTURE_DIR) / "Assets";
    const core::MeshFileResult read = core::readMeshFile(
        root / "Common" / "Characters" / "Mannequins" / "humanoid" / "humanoid.glb");
    ASSERT_TRUE(read.ok()) << read.message;
    const core::SkeletonFileResult declared =
        core::readSkeletonFile(root / core::skeletonFilePath("humanoid"));
    ASSERT_TRUE(declared.ok()) << declared.message;
    const core::MeshData& mesh = read.mesh;
    const core::SkeletonDescription& skeleton = declared.skeleton;

    ASSERT_EQ(mesh.rig.joints.size(), 53U);
    ASSERT_EQ(skeleton.bones.size(), 53U);
    ASSERT_EQ(mesh.skin.size(), mesh.vertices.size());
    for (const core::SkeletonBone& bone : skeleton.bones) {
        const auto found = std::ranges::find_if(
            mesh.rig.joints,
            [&bone](const core::MeshJoint& joint) { return joint.name == bone.name; });
        ASSERT_NE(found, mesh.rig.joints.end()) << bone.name;
        const std::string parent =
            found->parent >= 0 ? mesh.rig.joints[static_cast<std::size_t>(found->parent)].name
                               : std::string{};
        EXPECT_EQ(parent, bone.parent) << bone.name;
    }
    ASSERT_EQ(skeleton.clips.size(), 6U);
    ASSERT_EQ(mesh.rig.clips.size(), 6U);
    std::vector<float> pose(mesh.rig.joints.size() * 16);
    for (const core::SkeletonClip& clip : skeleton.clips) {
        const core::MeshClip* const curves = core::findClip(mesh.rig, clip.name);
        ASSERT_NE(curves, nullptr) << clip.name;
        EXPECT_NEAR(curves->duration, clip.duration, 1e-3F) << clip.name;
        for (int step = 0; step < 8; ++step) {
            const float seconds = clip.duration * static_cast<float>(step) / 8.0F;
            core::poseSkeleton(mesh.rig, curves, seconds, pose);
            float lowest = 10.0F;
            for (std::size_t index = 0; index < mesh.vertices.size(); ++index) {
                const std::array<float, 3> point =
                    core::skinnedPosition(pose, mesh.skin[index], mesh.vertices[index].position);
                ASSERT_TRUE(std::isfinite(point[0]) && std::isfinite(point[1]) &&
                            std::isfinite(point[2]))
                    << clip.name << " a " << seconds << " s";
                lowest = std::min(lowest, point[1]);
            }
            if (clip.loop) {
                // Ni enfonce, ni envole : la marche a un court temps de vol entre deux appuis.
                EXPECT_GT(lowest, -0.005F) << clip.name << " a " << seconds << " s";
                EXPECT_LT(lowest, 0.02F) << clip.name << " a " << seconds << " s";
            }
        }
    }
}
