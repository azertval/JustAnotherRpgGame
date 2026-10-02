// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file Core/Resources/MeshFile.h
 * @brief Le **chargeur de maillages** du moteur : un fichier `.glb` (glTF 2.0 binaire) lu en
 *        sommets, indices et texture de couleur (`LOT-1003`).
 *
 * Le standard 3D fixe le format d'échange (`Planning/standards/style-3d.md`, §2) : un modèle par
 * fichier, autonome, sa texture incorporée. Ce lecteur n'en lit que cela, par ses propres moyens —
 * l'enveloppe binaire à la main, le bloc JSON par `nlohmann/json`, déjà lié — et sans Qt Quick 3D,
 * que sa licence écarte du dépôt.
 *
 * ## Ce qui est lu
 *
 * - toutes les primitives **triangles** des nœuds de la scène, fondues en un seul maillage, chacune
 *   portée par la transformation de son nœud ;
 * - `POSITION`, `NORMAL`, `TEXCOORD_0` ; des indices sur 8, 16 ou 32 bits, ou pas d'indices ;
 * - la **couleur de base** de la première matière : son facteur, et l'image de sa texture, rendue
 *   **encodée** (JPEG ou PNG) — la décoder demande une bibliothèque d'images, donc `HMI`.
 *
 * ## Ce qui est refusé, et dit
 *
 * Un tampon ou une image hors du fichier, un accesseur creux (`sparse`), une extension **requise**
 * (compression Draco ou meshopt, textures KTX) : le résultat nomme ce qui manque plutôt que de
 * rendre un maillage faux.
 *
 * ## Le squelette et les clips (`LOT-1005`)
 *
 * Un modèle animé porte un squelette (`skins[0]`) : ses os, leur pose de repos, leur matrice de
 * liaison inverse, et pour chaque sommet quatre os et quatre poids (`JOINTS_0`, `WEIGHTS_0`). Ses
 * animations sont lues en **clips** : par os, les clés de translation et de rotation, interpolées
 * linéairement ou tenues (`LINEAR`, `STEP`). Une interpolation `CUBICSPLINE`, un canal d'échelle
 * ou de cible de forme ne sont pas lus : le canal est ignoré, le modèle reste lisible. Les sommets
 * d'un maillage lié restent dans le repère du maillage — la transformation de son nœud ne s'y
 * applique pas, c'est le squelette qui le pose (`core::poseSkeleton`, `SkeletonPose.h`).
 *
 * Logique pure, sans Qt ni GPU. Aucune lecture ne lève (`EX-NFR-040`) : c'est un lecteur d'octets
 * venus d'un fichier, et la cible `fuzz_mesh` le lui fait prouver.
 */

namespace core {

/// @brief L'extension d'un fichier de maillage, point compris.
inline constexpr std::string_view MESH_FILE_EXTENSION = ".glb";

/// @return Vrai si @p path nomme un fichier de maillage (`.glb`, sans égard à la casse).
[[nodiscard]] bool isMeshPath(std::string_view path) noexcept;

/// @brief Un sommet : position (mètres, hauteur +Y), normale unitaire, coordonnées de texture.
struct MeshVertex {
    std::array<float, 3> position{};
    std::array<float, 3> normal{};
    std::array<float, 2> uv{};

    [[nodiscard]] bool operator==(const MeshVertex&) const = default;
};

/// @brief Ce qui lie un sommet au squelette : quatre os (rangs dans `MeshRig::joints`) et leurs
///        poids, de somme 1. Un poids nul rend son os sans effet.
struct MeshSkinVertex {
    std::array<std::uint16_t, 4> joints{};
    std::array<float, 4> weights{};

    [[nodiscard]] bool operator==(const MeshSkinVertex&) const = default;
};

/// @brief Une matrice 4 × 4 en colonnes, comme glTF l'écrit : `m[colonne * 4 + ligne]`.
using MeshMatrix = std::array<float, 16>;

/// La matrice identité.
inline constexpr MeshMatrix MESH_IDENTITY = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

/// @brief Un os du squelette : son nom, son parent, sa pose de repos et sa matrice de liaison.
struct MeshJoint {
    std::string name;
    /// Le rang de l'os parent dans `MeshRig::joints` ; -1 pour un os sans parent parmi les os.
    int parent = -1;
    /// La pose locale de repos : translation, rotation (quaternion x, y, z, w), échelle.
    std::array<float, 3> translation{};
    std::array<float, 4> rotation{0.0F, 0.0F, 0.0F, 1.0F};
    std::array<float, 3> scale{1.0F, 1.0F, 1.0F};
    /// La matrice de liaison inverse : du repère du maillage à celui de l'os, en pose de liaison.
    MeshMatrix inverseBind = MESH_IDENTITY;
    /// Pour un os sans parent parmi les os : la transformation de ses ancêtres dans la scène.
    MeshMatrix anchor = MESH_IDENTITY;
};

/// @brief Les clés d'un canal : des instants croissants, et à chacun une valeur de `width`
///        composantes (3 pour une translation, 4 pour une rotation).
struct MeshChannel {
    std::vector<float> times;
    std::vector<float> values;
    /// Vrai si la valeur se tient jusqu'à la clé suivante (`STEP`) ; sinon elle s'interpole.
    bool step = false;

    [[nodiscard]] bool empty() const noexcept {
        return times.empty();
    }
};

/// @brief Ce qu'un clip fait d'un os : sa translation et sa rotation dans le temps. Un canal vide
///        laisse la pose de repos.
struct MeshJointTrack {
    MeshChannel translation;
    MeshChannel rotation;
};

/// @brief Un clip d'animation : un nom, une durée, une piste par os du squelette.
struct MeshClip {
    std::string name;
    /// La dernière clé du clip, en secondes.
    float duration = 0.0F;
    /// Une piste par os, dans l'ordre de `MeshRig::joints`.
    std::vector<MeshJointTrack> tracks;
};

/// @brief Le squelette d'un modèle et ses clips.
struct MeshRig {
    std::vector<MeshJoint> joints;
    /// Les rangs des os, **parents avant enfants** : l'ordre où une pose se calcule.
    std::vector<std::uint16_t> order;
    std::vector<MeshClip> clips;

    [[nodiscard]] bool empty() const noexcept {
        return joints.empty();
    }
};

/// Le plus d'os qu'un squelette lu peut porter.
inline constexpr std::size_t MESH_MAX_JOINTS = 256;

/// @brief Catégorie d'échec de lecture d'un maillage.
enum class MeshFileError : std::uint8_t {
    None,
    /// Fichier absent, illisible, ou plus gros que `MESH_FILE_MAX_BYTES`.
    FileNotFound,
    /// L'enveloppe n'est pas celle d'un `.glb` 2.0, ou son bloc JSON ne se lit pas.
    ParseError,
    /// Le fichier est un glTF valide qui emploie ce que ce lecteur ne lit pas (voir l'en-tête).
    Unsupported,
    /// Un index, une taille ou un décalage sort de ce que le fichier contient.
    MalformedStructure,
};

/// Le plus gros fichier lu, en octets : au-delà, ce n'est pas un modèle du jeu.
inline constexpr std::size_t MESH_FILE_MAX_BYTES = std::size_t{512} * 1024 * 1024;

/// @brief Un maillage lu : une seule liste de sommets, des triangles indexés, une matière.
struct MeshData {
    std::vector<MeshVertex> vertices;
    /// Trois indices par triangle, dans `vertices`.
    std::vector<std::uint32_t> indices;
    /// La boîte des positions, en mètres ; nulle pour un maillage vide.
    std::array<float, 3> minimum{};
    std::array<float, 3> maximum{};
    /// Le facteur de couleur de base de la matière (RVBA, alpha droit) ; blanc par défaut.
    std::array<float, 4> baseColor{1.0F, 1.0F, 1.0F, 1.0F};
    /// L'image de la texture de couleur, **encodée** telle que le fichier la porte ; vide si la
    /// matière n'en a pas.
    std::vector<std::byte> image;
    /// Son type (`image/jpeg`, `image/png`), vide s'il n'est pas dit.
    std::string imageMimeType;
    /// Le nombre de primitives fondues, et de matières distinctes qu'elles citaient : le standard
    /// en veut une de chaque, et qui installe un modèle peut le vérifier ici.
    int primitiveCount = 0;
    int materialCount = 0;
    /// Vrai si le fichier porte un squelette : le maillage est lu dans sa pose de liaison.
    bool skinned = false;
    /// La liaison de chaque sommet (`LOT-1005`), dans l'ordre de `vertices` ; vide pour un
    /// maillage sans squelette, ou dont les primitives ne portent pas `JOINTS_0` et `WEIGHTS_0`.
    std::vector<MeshSkinVertex> skin;
    /// Le squelette et ses clips ; vide sans squelette.
    MeshRig rig;

    /// @return Le nombre de triangles.
    [[nodiscard]] std::size_t triangleCount() const noexcept {
        return indices.size() / 3;
    }
};

/// @brief Résultat d'une lecture : le maillage, et ce qui a échoué.
struct MeshFileResult {
    MeshData mesh;
    MeshFileError error = MeshFileError::None;
    /// Message technique, vide en cas de succès.
    std::string message;

    [[nodiscard]] bool ok() const noexcept {
        return error == MeshFileError::None;
    }
};

/// @brief Lit un maillage depuis les octets d'un fichier `.glb`.
[[nodiscard]] MeshFileResult readMeshFromGlb(std::span<const std::byte> bytes);

/// @brief Lit le fichier `.glb` @p path ; un fichier absent est une erreur du résultat.
[[nodiscard]] MeshFileResult readMeshFile(const std::filesystem::path& path);

}  // namespace core
