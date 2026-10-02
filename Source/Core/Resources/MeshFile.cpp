// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

#include "Core/Resources/MeshFile.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <optional>
#include <set>
#include <system_error>
#include <utility>

#include <nlohmann/json.hpp>

namespace core {

namespace {

using Json = nlohmann::json;

// L'enveloppe d'un .glb : douze octets d'en-tete, puis des blocs (longueur, type, contenu).
constexpr std::uint32_t GLB_MAGIC = 0x46546C67U;   // « glTF »
constexpr std::uint32_t CHUNK_JSON = 0x4E4F534AU;  // « JSON »
constexpr std::uint32_t CHUNK_BIN = 0x004E4942U;   // « BIN\0 »
constexpr std::size_t GLB_HEADER_BYTES = 12;
constexpr std::size_t CHUNK_HEADER_BYTES = 8;
constexpr std::uint32_t GLTF_VERSION = 2;

// Types de composante d'un accesseur (constantes OpenGL que glTF reprend).
constexpr int COMPONENT_UNSIGNED_BYTE = 5121;
constexpr int COMPONENT_UNSIGNED_SHORT = 5123;
constexpr int COMPONENT_UNSIGNED_INT = 5125;
constexpr int COMPONENT_FLOAT = 5126;
constexpr int MODE_TRIANGLES = 4;

// Bornes : un fichier fautif ne fait ni tourner ni allouer sans fin.
constexpr std::size_t MAX_NODES = 65536;
constexpr int MAX_NODE_DEPTH = 64;
constexpr std::size_t MAX_VERTICES = std::size_t{1} << 26U;
constexpr std::size_t MAX_INDICES = std::size_t{3} << 26U;
constexpr std::size_t MAX_CLIPS = 256;
constexpr std::size_t MAX_KEYS = std::size_t{1} << 20U;

// Une matrice 4 x 4 en colonnes, comme glTF l'ecrit : m[colonne * 4 + ligne].
using Matrix = std::array<double, 16>;

constexpr Matrix IDENTITY = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

[[nodiscard]] Matrix multiply(const Matrix& a, const Matrix& b) {
    Matrix out{};
    for (std::size_t column = 0; column < 4; ++column) {
        for (std::size_t row = 0; row < 4; ++row) {
            double sum = 0.0;
            for (std::size_t k = 0; k < 4; ++k) {
                sum += a[(k * 4) + row] * b[(column * 4) + k];
            }
            out[(column * 4) + row] = sum;
        }
    }
    return out;
}

[[nodiscard]] MeshMatrix narrowed(const Matrix& matrix) {
    MeshMatrix out{};
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<float>(matrix[i]);
    }
    return out;
}

struct Failure {
    MeshFileError error;
    std::string message;
};

[[nodiscard]] MeshFileResult failed(MeshFileError error, std::string message) {
    return MeshFileResult{.mesh = {}, .error = error, .message = std::move(message)};
}

[[nodiscard]] std::uint32_t readU32(std::span<const std::byte> bytes, std::size_t offset) {
    std::uint32_t value = 0;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;  // petit-boutiste, comme toutes les cibles du jeu
}

// Un entier non negatif d'un objet JSON ; `fallback` s'il manque, rien s'il n'en est pas un.
[[nodiscard]] std::optional<std::size_t> indexField(const Json& object, const char* key,
                                                    std::optional<std::size_t> fallback) {
    const auto found = object.find(key);
    if (found == object.end()) {
        return fallback;
    }
    if (!found->is_number_unsigned()) {
        return std::nullopt;
    }
    return found->get<std::size_t>();
}

// L'element `index` du tableau `key` de la racine, s'il existe et est un objet.
[[nodiscard]] const Json* element(const Json& root, const char* key, std::size_t index) {
    const auto found = root.find(key);
    if (found == root.end() || !found->is_array() || index >= found->size() ||
        !(*found)[index].is_object()) {
        return nullptr;
    }
    return &(*found)[index];
}

// `count` nombres finis lus dans le tableau `key` de `object` ; rien s'il manque ou est mal forme.
template <std::size_t Count>
[[nodiscard]] std::optional<std::array<double, Count>> numbers(const Json& object,
                                                               const char* key) {
    const auto found = object.find(key);
    if (found == object.end() || !found->is_array() || found->size() != Count) {
        return std::nullopt;
    }
    std::array<double, Count> values{};
    for (std::size_t i = 0; i < Count; ++i) {
        if (!(*found)[i].is_number()) {
            return std::nullopt;
        }
        values[i] = (*found)[i].get<double>();
        if (!std::isfinite(values[i])) {
            return std::nullopt;
        }
    }
    return values;
}

// La transformation locale d'un noeud : sa matrice, a defaut translation x rotation x echelle.
[[nodiscard]] Matrix localMatrix(const Json& node) {
    if (const auto matrix = numbers<16>(node, "matrix")) {
        return *matrix;
    }
    const std::array<double, 3> t =
        numbers<3>(node, "translation").value_or(std::array<double, 3>{0.0, 0.0, 0.0});
    const std::array<double, 4> q =
        numbers<4>(node, "rotation").value_or(std::array<double, 4>{0.0, 0.0, 0.0, 1.0});
    const std::array<double, 3> s =
        numbers<3>(node, "scale").value_or(std::array<double, 3>{1.0, 1.0, 1.0});
    const double x = q[0];
    const double y = q[1];
    const double z = q[2];
    const double w = q[3];
    return Matrix{(1 - (2 * ((y * y) + (z * z)))) * s[0],
                  (2 * ((x * y) + (z * w))) * s[0],
                  (2 * ((x * z) - (y * w))) * s[0],
                  0,
                  (2 * ((x * y) - (z * w))) * s[1],
                  (1 - (2 * ((x * x) + (z * z)))) * s[1],
                  (2 * ((y * z) + (x * w))) * s[1],
                  0,
                  (2 * ((x * z) + (y * w))) * s[2],
                  (2 * ((y * z) - (x * w))) * s[2],
                  (1 - (2 * ((x * x) + (y * y)))) * s[2],
                  0,
                  t[0],
                  t[1],
                  t[2],
                  1};
}

// Une vue sur les elements d'un accesseur : ou ils sont, de quoi ils sont faits.
struct AccessorView {
    const std::byte* data = nullptr;
    std::size_t count = 0;
    std::size_t stride = 0;
    int componentType = 0;
    std::size_t components = 0;
    bool normalized = false;
};

[[nodiscard]] std::size_t componentBytes(int componentType) {
    switch (componentType) {
        case COMPONENT_UNSIGNED_BYTE:
            return 1;
        case COMPONENT_UNSIGNED_SHORT:
            return 2;
        case COMPONENT_UNSIGNED_INT:
        case COMPONENT_FLOAT:
            return 4;
        default:
            return 0;
    }
}

[[nodiscard]] std::size_t componentsOf(const std::string& type) {
    if (type == "SCALAR") {
        return 1;
    }
    if (type == "VEC2") {
        return 2;
    }
    if (type == "VEC3") {
        return 3;
    }
    if (type == "VEC4") {
        return 4;
    }
    if (type == "MAT4") {
        return 16;
    }
    return 0;
}

// Le lecteur : la racine JSON, le bloc binaire, et l'echec retenu.
class GlbReader {
public:
    GlbReader(const Json& root, std::span<const std::byte> binary) : _root(root), _binary(binary) {}

    [[nodiscard]] MeshFileResult read() {
        if (!checkExtensions() || !walkScene()) {
            return failed(_failure.error, std::move(_failure.message));
        }
        if (_mesh.indices.empty()) {
            return failed(MeshFileError::MalformedStructure, "no triangle in the scene");
        }
        _mesh.materialCount = static_cast<int>(_materials.size());
        const auto skins = _root.find("skins");
        _mesh.skinned = skins != _root.end() && skins->is_array() && !skins->empty();
        // Le squelette ne vaut que si TOUS les sommets y sont lies : un maillage a demi lie se lit
        // dans sa pose de liaison, sans os.
        if (_skin && _mesh.skin.size() == _mesh.vertices.size()) {
            if (!readSkin(*_skin) || !readAnimations()) {
                return failed(_failure.error, std::move(_failure.message));
            }
        } else {
            _mesh.skin.clear();
        }
        if (!readMaterial()) {
            return failed(_failure.error, std::move(_failure.message));
        }
        measure();
        return MeshFileResult{
            .mesh = std::move(_mesh), .error = MeshFileError::None, .message = {}};
    }

private:
    bool fail(MeshFileError error, std::string message) {
        _failure = Failure{.error = error, .message = std::move(message)};
        return false;
    }

    // Une extension REQUISE change le sens des donnees : la lire sans elle rendrait un faux.
    bool checkExtensions() {
        const auto required = _root.find("extensionsRequired");
        if (required == _root.end() || !required->is_array() || required->empty()) {
            return true;
        }
        std::string names;
        for (const Json& name : *required) {
            if (name.is_string()) {
                names += (names.empty() ? "" : ", ") + name.get<std::string>();
            }
        }
        return fail(MeshFileError::Unsupported, "required extension not supported: " + names);
    }

    // Les racines de la scene par defaut ; sans scene, tous les noeuds qui ne sont l'enfant de
    // personne -- ce que font les lecteurs de reference.
    bool walkScene() {
        const auto nodes = _root.find("nodes");
        if (nodes == _root.end() || !nodes->is_array()) {
            return fail(MeshFileError::MalformedStructure, "nodes missing");
        }
        if (nodes->size() > MAX_NODES) {
            return fail(MeshFileError::Unsupported, "too many nodes");
        }
        _visited.assign(nodes->size(), false);
        std::vector<std::size_t> roots;
        const std::optional<std::size_t> sceneIndex = indexField(_root, "scene", std::size_t{0});
        const Json* const scene = sceneIndex ? element(_root, "scenes", *sceneIndex) : nullptr;
        if (scene != nullptr) {
            if (!childrenOf(*scene, "nodes", roots)) {
                return false;
            }
        } else {
            std::vector<bool> child(nodes->size(), false);
            for (const Json& node : *nodes) {
                std::vector<std::size_t> children;
                if (node.is_object() && childrenOf(node, "children", children)) {
                    for (const std::size_t index : children) {
                        child[index] = true;
                    }
                }
            }
            for (std::size_t index = 0; index < nodes->size(); ++index) {
                if (!child[index]) {
                    roots.push_back(index);
                }
            }
        }
        return std::ranges::all_of(
            roots, [this](std::size_t root) { return walkNode(root, IDENTITY, 0); });
    }

    // Les indices de noeud du tableau `key` de `object`, verifies contre la table des noeuds.
    bool childrenOf(const Json& object, const char* key, std::vector<std::size_t>& out) {
        const auto found = object.find(key);
        if (found == object.end()) {
            return true;
        }
        if (!found->is_array()) {
            return fail(MeshFileError::MalformedStructure, std::string{key} + " is not an array");
        }
        for (const Json& index : *found) {
            if (!index.is_number_unsigned() || index.get<std::size_t>() >= _visited.size()) {
                return fail(MeshFileError::MalformedStructure,
                            std::string{key} + " cites a missing node");
            }
            out.push_back(index.get<std::size_t>());
        }
        return true;
    }

    bool walkNode(std::size_t index, const Matrix& parent, int depth) {
        // Un graphe de scene est un arbre : un noeud revu est un cycle, ou un partage que glTF
        // interdit.
        if (depth > MAX_NODE_DEPTH || _visited[index]) {
            return fail(MeshFileError::MalformedStructure, "node graph is not a tree");
        }
        _visited[index] = true;
        const Json* const node = element(_root, "nodes", index);
        if (node == nullptr) {
            return fail(MeshFileError::MalformedStructure, "node is not an object");
        }
        const Matrix world = multiply(parent, localMatrix(*node));
        if (node->contains("mesh")) {
            const std::optional<std::size_t> mesh = indexField(*node, "mesh", std::nullopt);
            const Json* const found = mesh ? element(_root, "meshes", *mesh) : nullptr;
            if (found == nullptr) {
                return fail(MeshFileError::MalformedStructure, "node cites a missing mesh");
            }
            // Un maillage lie reste dans son repere : c'est le squelette qui le pose, et la
            // transformation de son noeud ne compte pas (glTF 2.0, « Skins »). Un seul squelette
            // est lu, le premier rencontre.
            const std::optional<std::size_t> skin = indexField(*node, "skin", std::nullopt);
            const bool bound = node->contains("skin") && skin && (!_skin || *_skin == *skin);
            if (bound) {
                _skin = *skin;
            }
            if (!readMesh(*found, bound ? IDENTITY : world, bound)) {
                return false;
            }
        }
        std::vector<std::size_t> children;
        if (!childrenOf(*node, "children", children)) {
            return false;
        }
        return std::ranges::all_of(
            children, [&](std::size_t child) { return walkNode(child, world, depth + 1); });
    }

    bool readMesh(const Json& mesh, const Matrix& world, bool bound) {
        const auto primitives = mesh.find("primitives");
        if (primitives == mesh.end() || !primitives->is_array()) {
            return fail(MeshFileError::MalformedStructure, "mesh without primitives");
        }
        for (const Json& primitive : *primitives) {
            if (!primitive.is_object()) {
                return fail(MeshFileError::MalformedStructure, "primitive is not an object");
            }
            const std::optional<std::size_t> mode =
                indexField(primitive, "mode", std::size_t{MODE_TRIANGLES});
            if (!mode || *mode != static_cast<std::size_t>(MODE_TRIANGLES)) {
                continue;  // points, lignes, eventails : pas un volume du jeu
            }
            if (!readPrimitive(primitive, world, bound)) {
                return false;
            }
        }
        return true;
    }

    // Verifie l'accesseur `index` et rend la vue de ses elements.
    std::optional<AccessorView> accessor(std::size_t index) {
        const Json* const found = element(_root, "accessors", index);
        if (found == nullptr) {
            fail(MeshFileError::MalformedStructure, "missing accessor");
            return std::nullopt;
        }
        if (found->contains("sparse")) {
            fail(MeshFileError::Unsupported, "sparse accessor not supported");
            return std::nullopt;
        }
        const std::optional<std::size_t> viewIndex = indexField(*found, "bufferView", std::nullopt);
        const std::optional<std::size_t> offset = indexField(*found, "byteOffset", std::size_t{0});
        const std::optional<std::size_t> count = indexField(*found, "count", std::nullopt);
        const std::optional<std::size_t> component =
            indexField(*found, "componentType", std::nullopt);
        const auto type = found->find("type");
        if (!viewIndex || !offset || !count || !component || type == found->end() ||
            !type->is_string()) {
            fail(MeshFileError::MalformedStructure, "accessor is incomplete");
            return std::nullopt;
        }
        AccessorView view;
        view.count = *count;
        view.componentType = static_cast<int>(std::min<std::size_t>(*component, 65535));
        view.components = componentsOf(type->get<std::string>());
        const auto normalized = found->find("normalized");
        view.normalized =
            normalized != found->end() && normalized->is_boolean() && normalized->get<bool>();
        const std::size_t elementBytes = componentBytes(view.componentType) * view.components;
        if (elementBytes == 0) {
            fail(MeshFileError::Unsupported, "accessor component or type not supported");
            return std::nullopt;
        }
        const std::optional<std::span<const std::byte>> bytes = bufferView(*viewIndex, view.stride);
        if (!bytes) {
            return std::nullopt;
        }
        if (view.stride == 0) {
            view.stride = elementBytes;
        }
        // (count - 1) * stride + elementBytes octets depuis `offset`, sans debordement d'entier.
        if (view.stride < elementBytes || *offset > bytes->size() ||
            (view.count > 0 &&
             (view.count - 1 > (bytes->size() - *offset) / view.stride ||
              ((view.count - 1) * view.stride) + elementBytes > bytes->size() - *offset))) {
            fail(MeshFileError::MalformedStructure, "accessor exceeds its buffer view");
            return std::nullopt;
        }
        view.data = bytes->data() + *offset;
        return view;
    }

    // Les octets de la vue de tampon `index`, dans le bloc binaire du fichier.
    std::optional<std::span<const std::byte>> bufferView(std::size_t index, std::size_t& stride) {
        const Json* const view = element(_root, "bufferViews", index);
        if (view == nullptr) {
            fail(MeshFileError::MalformedStructure, "missing buffer view");
            return std::nullopt;
        }
        const std::optional<std::size_t> buffer = indexField(*view, "buffer", std::nullopt);
        const std::optional<std::size_t> offset = indexField(*view, "byteOffset", std::size_t{0});
        const std::optional<std::size_t> length = indexField(*view, "byteLength", std::nullopt);
        const std::optional<std::size_t> byteStride =
            indexField(*view, "byteStride", std::size_t{0});
        if (!buffer || !offset || !length || !byteStride) {
            fail(MeshFileError::MalformedStructure, "buffer view is incomplete");
            return std::nullopt;
        }
        // Le seul tampon lu est le bloc binaire du fichier : le premier, sans `uri`.
        const Json* const declared = element(_root, "buffers", *buffer);
        if (*buffer != 0 || declared == nullptr || declared->contains("uri")) {
            fail(MeshFileError::Unsupported, "buffer outside the file not supported");
            return std::nullopt;
        }
        if (*offset > _binary.size() || *length > _binary.size() - *offset) {
            fail(MeshFileError::MalformedStructure, "buffer view exceeds the binary chunk");
            return std::nullopt;
        }
        stride = *byteStride;
        return _binary.subspan(*offset, *length);
    }

    [[nodiscard]] static float component(const AccessorView& view, std::size_t item,
                                         std::size_t part) {
        const std::byte* const at =
            view.data + (item * view.stride) + (part * componentBytes(view.componentType));
        switch (view.componentType) {
            case COMPONENT_FLOAT: {
                float value = 0.0F;
                std::memcpy(&value, at, sizeof(value));
                return value;
            }
            case COMPONENT_UNSIGNED_BYTE: {
                const auto value = static_cast<float>(std::to_integer<std::uint8_t>(*at));
                return view.normalized ? value / 255.0F : value;
            }
            case COMPONENT_UNSIGNED_SHORT: {
                std::uint16_t value = 0;
                std::memcpy(&value, at, sizeof(value));
                return view.normalized ? static_cast<float>(value) / 65535.0F
                                       : static_cast<float>(value);
            }
            default:
                return 0.0F;
        }
    }

    [[nodiscard]] static std::uint32_t indexAt(const AccessorView& view, std::size_t item) {
        const std::byte* const at = view.data + (item * view.stride);
        switch (view.componentType) {
            case COMPONENT_UNSIGNED_BYTE:
                return std::to_integer<std::uint32_t>(*at);
            case COMPONENT_UNSIGNED_SHORT: {
                std::uint16_t value = 0;
                std::memcpy(&value, at, sizeof(value));
                return value;
            }
            default: {
                std::uint32_t value = 0;
                std::memcpy(&value, at, sizeof(value));
                return value;
            }
        }
    }

    // L'accesseur d'un attribut de la primitive, s'il est cite ; `present` dit s'il l'etait.
    std::optional<AccessorView> attribute(const Json& attributes, const char* name, bool& present) {
        present = attributes.contains(name);
        if (!present) {
            return AccessorView{};
        }
        const std::optional<std::size_t> index = indexField(attributes, name, std::nullopt);
        if (!index) {
            fail(MeshFileError::MalformedStructure, std::string{name} + " is not an accessor");
            return std::nullopt;
        }
        return accessor(*index);
    }

    // Les quatre os et les quatre poids de chaque sommet de la primitive, ajoutes a `_mesh.skin`.
    // Une primitive qui n'en porte pas n'ajoute rien : le maillage sera lu sans squelette.
    bool appendSkin(const Json& attributes, std::size_t vertexCount) {
        bool hasJoints = false;
        bool hasWeights = false;
        const std::optional<AccessorView> joints = attribute(attributes, "JOINTS_0", hasJoints);
        const std::optional<AccessorView> weights = attribute(attributes, "WEIGHTS_0", hasWeights);
        if (!joints || !weights) {
            return false;
        }
        if (!hasJoints || !hasWeights) {
            return true;
        }
        if (joints->components != 4 || joints->count != vertexCount || joints->normalized ||
            (joints->componentType != COMPONENT_UNSIGNED_BYTE &&
             joints->componentType != COMPONENT_UNSIGNED_SHORT)) {
            return fail(MeshFileError::MalformedStructure, "JOINTS_0 must be unsigned VEC4");
        }
        if (weights->components != 4 || weights->count != vertexCount ||
            (weights->componentType != COMPONENT_FLOAT && !weights->normalized)) {
            return fail(MeshFileError::MalformedStructure, "WEIGHTS_0 must match POSITION");
        }
        _mesh.skin.reserve(_mesh.skin.size() + vertexCount);
        for (std::size_t item = 0; item < vertexCount; ++item) {
            MeshSkinVertex vertex;
            float sum = 0.0F;
            for (std::size_t part = 0; part < 4; ++part) {
                vertex.joints[part] = static_cast<std::uint16_t>(component(*joints, item, part));
                const float weight = component(*weights, item, part);
                vertex.weights[part] = std::isfinite(weight) && weight > 0.0F ? weight : 0.0F;
                sum += vertex.weights[part];
            }
            // Les poids se ramenent a une somme de 1 ; un sommet sans poids suit son premier os.
            if (sum > 0.0F) {
                for (float& weight : vertex.weights) {
                    weight /= sum;
                }
            } else {
                vertex.weights = {1.0F, 0.0F, 0.0F, 0.0F};
            }
            _mesh.skin.push_back(vertex);
        }
        return true;
    }

    bool readPrimitive(const Json& primitive, const Matrix& world, bool bound) {
        const auto attributes = primitive.find("attributes");
        if (attributes == primitive.end() || !attributes->is_object()) {
            return fail(MeshFileError::MalformedStructure, "primitive without attributes");
        }
        bool hasPosition = false;
        bool hasNormal = false;
        bool hasUv = false;
        const std::optional<AccessorView> positions =
            attribute(*attributes, "POSITION", hasPosition);
        const std::optional<AccessorView> normals = attribute(*attributes, "NORMAL", hasNormal);
        const std::optional<AccessorView> uvs = attribute(*attributes, "TEXCOORD_0", hasUv);
        if (!positions || !normals || !uvs) {
            return false;
        }
        if (!hasPosition || positions->componentType != COMPONENT_FLOAT ||
            positions->components != 3) {
            return fail(MeshFileError::MalformedStructure, "POSITION must be float VEC3");
        }
        if (hasNormal && (normals->componentType != COMPONENT_FLOAT || normals->components != 3 ||
                          normals->count != positions->count)) {
            return fail(MeshFileError::MalformedStructure, "NORMAL must match POSITION");
        }
        if (hasUv && (uvs->components != 2 || uvs->count != positions->count ||
                      (uvs->componentType != COMPONENT_FLOAT && !uvs->normalized))) {
            return fail(MeshFileError::MalformedStructure, "TEXCOORD_0 must match POSITION");
        }
        if (positions->count > MAX_VERTICES - _mesh.vertices.size()) {
            return fail(MeshFileError::Unsupported, "too many vertices");
        }

        const auto base = static_cast<std::uint32_t>(_mesh.vertices.size());
        appendVertices(*positions, hasNormal ? &*normals : nullptr, hasUv ? &*uvs : nullptr, world);
        if (bound && !appendSkin(*attributes, positions->count)) {
            return false;
        }
        if (!appendIndices(primitive, base, positions->count)) {
            return false;
        }
        ++_mesh.primitiveCount;
        // La matiere citee ; une primitive sans matiere compte pour la matiere par defaut.
        const std::optional<std::size_t> material =
            indexField(primitive, "material", std::numeric_limits<std::size_t>::max());
        if (material) {
            if (_materials.empty()) {
                _firstMaterial = *material;
            }
            _materials.insert(*material);
        }
        return true;
    }

    void appendVertices(const AccessorView& positions, const AccessorView* normals,
                        const AccessorView* uvs, const Matrix& world) {
        // Les normales se transforment par la comatrice du bloc 3 x 3 : juste sous une echelle non
        // uniforme, et sans inversion a calculer.
        const auto m = [&world](std::size_t row, std::size_t column) {
            return world[(column * 4) + row];
        };
        const std::array<double, 9> cofactor = {
            (m(1, 1) * m(2, 2)) - (m(1, 2) * m(2, 1)), (m(1, 2) * m(2, 0)) - (m(1, 0) * m(2, 2)),
            (m(1, 0) * m(2, 1)) - (m(1, 1) * m(2, 0)), (m(0, 2) * m(2, 1)) - (m(0, 1) * m(2, 2)),
            (m(0, 0) * m(2, 2)) - (m(0, 2) * m(2, 0)), (m(0, 1) * m(2, 0)) - (m(0, 0) * m(2, 1)),
            (m(0, 1) * m(1, 2)) - (m(0, 2) * m(1, 1)), (m(0, 2) * m(1, 0)) - (m(0, 0) * m(1, 2)),
            (m(0, 0) * m(1, 1)) - (m(0, 1) * m(1, 0))};
        _mesh.vertices.reserve(_mesh.vertices.size() + positions.count);
        for (std::size_t item = 0; item < positions.count; ++item) {
            const double x = component(positions, item, 0);
            const double y = component(positions, item, 1);
            const double z = component(positions, item, 2);
            MeshVertex vertex;
            for (std::size_t row = 0; row < 3; ++row) {
                vertex.position[row] = static_cast<float>((m(row, 0) * x) + (m(row, 1) * y) +
                                                          (m(row, 2) * z) + m(row, 3));
            }
            if (normals != nullptr) {
                const double nx = component(*normals, item, 0);
                const double ny = component(*normals, item, 1);
                const double nz = component(*normals, item, 2);
                std::array<double, 3> normal{};
                for (std::size_t row = 0; row < 3; ++row) {
                    normal[row] = (cofactor[row * 3] * nx) + (cofactor[(row * 3) + 1] * ny) +
                                  (cofactor[(row * 3) + 2] * nz);
                }
                const double length = std::sqrt((normal[0] * normal[0]) + (normal[1] * normal[1]) +
                                                (normal[2] * normal[2]));
                if (length > 0.0 && std::isfinite(length)) {
                    for (std::size_t row = 0; row < 3; ++row) {
                        vertex.normal[row] = static_cast<float>(normal[row] / length);
                    }
                }
            }
            if (uvs != nullptr) {
                vertex.uv = {component(*uvs, item, 0), component(*uvs, item, 1)};
            }
            _mesh.vertices.push_back(vertex);
        }
    }

    bool appendIndices(const Json& primitive, std::uint32_t base, std::size_t vertexCount) {
        if (!primitive.contains("indices")) {
            // Sans indices, les sommets vont par trois.
            const std::size_t count = vertexCount - (vertexCount % 3);
            if (count > MAX_INDICES - _mesh.indices.size()) {
                return fail(MeshFileError::Unsupported, "too many indices");
            }
            for (std::size_t item = 0; item < count; ++item) {
                _mesh.indices.push_back(base + static_cast<std::uint32_t>(item));
            }
            return true;
        }
        const std::optional<std::size_t> index = indexField(primitive, "indices", std::nullopt);
        if (!index) {
            return fail(MeshFileError::MalformedStructure, "indices is not an accessor");
        }
        const std::optional<AccessorView> view = accessor(*index);
        if (!view) {
            return false;
        }
        if (view->components != 1 || view->componentType == COMPONENT_FLOAT) {
            return fail(MeshFileError::MalformedStructure, "indices must be unsigned scalars");
        }
        const std::size_t count = view->count - (view->count % 3);
        if (count > MAX_INDICES - _mesh.indices.size()) {
            return fail(MeshFileError::Unsupported, "too many indices");
        }
        _mesh.indices.reserve(_mesh.indices.size() + count);
        for (std::size_t item = 0; item < count; ++item) {
            const std::uint32_t value = indexAt(*view, item);
            if (value >= vertexCount) {
                return fail(MeshFileError::MalformedStructure, "index beyond the vertices");
            }
            _mesh.indices.push_back(base + value);
        }
        return true;
    }

    // La couleur de base de la premiere matiere citee : son facteur, l'image de sa texture.
    bool readMaterial() {
        const Json* const material =
            _materials.empty() || _firstMaterial == std::numeric_limits<std::size_t>::max()
                ? nullptr
                : element(_root, "materials", _firstMaterial);
        if (material == nullptr) {
            return true;  // la matiere par defaut de glTF : blanche, sans texture
        }
        const auto pbr = material->find("pbrMetallicRoughness");
        if (pbr == material->end() || !pbr->is_object()) {
            return true;
        }
        if (const auto factor = numbers<4>(*pbr, "baseColorFactor")) {
            for (std::size_t i = 0; i < 4; ++i) {
                _mesh.baseColor[i] = static_cast<float>(std::clamp((*factor)[i], 0.0, 1.0));
            }
        }
        const auto texture = pbr->find("baseColorTexture");
        if (texture == pbr->end() || !texture->is_object()) {
            return true;
        }
        const std::optional<std::size_t> textureIndex = indexField(*texture, "index", std::nullopt);
        const Json* const declared =
            textureIndex ? element(_root, "textures", *textureIndex) : nullptr;
        const std::optional<std::size_t> source =
            declared != nullptr ? indexField(*declared, "source", std::nullopt) : std::nullopt;
        const Json* const image = source ? element(_root, "images", *source) : nullptr;
        if (image == nullptr) {
            return fail(MeshFileError::MalformedStructure, "base color texture has no image");
        }
        const std::optional<std::size_t> view = indexField(*image, "bufferView", std::nullopt);
        if (!view) {
            return fail(MeshFileError::Unsupported, "image outside the file not supported");
        }
        std::size_t stride = 0;
        const std::optional<std::span<const std::byte>> bytes = bufferView(*view, stride);
        if (!bytes) {
            return false;
        }
        _mesh.image.assign(bytes->begin(), bytes->end());
        if (const auto mime = image->find("mimeType"); mime != image->end() && mime->is_string()) {
            _mesh.imageMimeType = mime->get<std::string>();
        }
        return true;
    }

    // Le parent de chaque noeud, d'apres les tableaux `children` ; -1 pour une racine.
    [[nodiscard]] std::vector<int> nodeParents() {
        const Json& nodes = _root["nodes"];
        std::vector<int> parents(nodes.size(), -1);
        for (std::size_t index = 0; index < nodes.size(); ++index) {
            std::vector<std::size_t> children;
            if (nodes[index].is_object() && childrenOf(nodes[index], "children", children)) {
                for (const std::size_t child : children) {
                    parents[child] = static_cast<int>(index);
                }
            }
        }
        return parents;
    }

    // Le squelette `index` : ses os, leur pose de repos, leur matrice de liaison inverse, leur
    // parent parmi les os, et l'ordre parents-avant-enfants.
    bool readSkin(std::size_t index) {
        const Json* const skin = element(_root, "skins", index);
        if (skin == nullptr) {
            return fail(MeshFileError::MalformedStructure, "node cites a missing skin");
        }
        std::vector<std::size_t> nodes;
        if (!childrenOf(*skin, "joints", nodes)) {
            return false;
        }
        if (nodes.empty() || nodes.size() > MESH_MAX_JOINTS) {
            return fail(MeshFileError::Unsupported, "skin must have 1 to 256 joints");
        }
        std::optional<AccessorView> inverse;
        if (skin->contains("inverseBindMatrices")) {
            const std::optional<std::size_t> accessorIndex =
                indexField(*skin, "inverseBindMatrices", std::nullopt);
            if (!accessorIndex) {
                return fail(MeshFileError::MalformedStructure,
                            "inverseBindMatrices is not an accessor");
            }
            inverse = accessor(*accessorIndex);
            if (!inverse) {
                return false;
            }
            if (inverse->components != 16 || inverse->componentType != COMPONENT_FLOAT ||
                inverse->count < nodes.size()) {
                return fail(MeshFileError::MalformedStructure,
                            "inverseBindMatrices must be float MAT4, one per joint");
            }
        }
        const std::vector<int> parents = nodeParents();
        _jointOfNode.assign(parents.size(), -1);
        for (std::size_t rank = 0; rank < nodes.size(); ++rank) {
            if (_jointOfNode[nodes[rank]] >= 0) {
                return fail(MeshFileError::MalformedStructure, "skin cites a joint twice");
            }
            _jointOfNode[nodes[rank]] = static_cast<int>(rank);
        }
        _mesh.rig.joints.resize(nodes.size());
        for (std::size_t rank = 0; rank < nodes.size(); ++rank) {
            const Json* const node = element(_root, "nodes", nodes[rank]);
            if (node == nullptr) {
                return fail(MeshFileError::MalformedStructure, "joint is not an object");
            }
            if (node->contains("matrix")) {
                return fail(MeshFileError::Unsupported, "joint posed by a matrix not supported");
            }
            MeshJoint& joint = _mesh.rig.joints[rank];
            if (const auto name = node->find("name"); name != node->end() && name->is_string()) {
                joint.name = name->get<std::string>();
            }
            readPose(*node, joint);
            if (inverse) {
                for (std::size_t part = 0; part < 16; ++part) {
                    joint.inverseBind[part] = component(*inverse, rank, part);
                }
            }
            // Le parent : le premier ancetre du noeud, s'il est un os ; sinon l'os est une racine,
            // ancree par la transformation de ses ancetres.
            const int parent = parents[nodes[rank]];
            if (parent >= 0 && _jointOfNode[static_cast<std::size_t>(parent)] >= 0) {
                joint.parent = _jointOfNode[static_cast<std::size_t>(parent)];
            } else {
                Matrix anchor = IDENTITY;
                int ancestor = parent;
                for (int depth = 0; ancestor >= 0; ++depth) {
                    if (depth > MAX_NODE_DEPTH) {
                        return fail(MeshFileError::MalformedStructure, "node graph is not a tree");
                    }
                    const Json* const above =
                        element(_root, "nodes", static_cast<std::size_t>(ancestor));
                    if (above == nullptr) {
                        return fail(MeshFileError::MalformedStructure, "node is not an object");
                    }
                    anchor = multiply(localMatrix(*above), anchor);
                    ancestor = parents[static_cast<std::size_t>(ancestor)];
                }
                joint.anchor = narrowed(anchor);
            }
        }
        return orderJoints();
    }

    // La pose locale de repos d'un os : ce que son noeud declare, a defaut l'identite.
    static void readPose(const Json& node, MeshJoint& joint) {
        if (const auto t = numbers<3>(node, "translation")) {
            joint.translation = {static_cast<float>((*t)[0]), static_cast<float>((*t)[1]),
                                 static_cast<float>((*t)[2])};
        }
        if (const auto q = numbers<4>(node, "rotation")) {
            joint.rotation = {static_cast<float>((*q)[0]), static_cast<float>((*q)[1]),
                              static_cast<float>((*q)[2]), static_cast<float>((*q)[3])};
        }
        if (const auto e = numbers<3>(node, "scale")) {
            joint.scale = {static_cast<float>((*e)[0]), static_cast<float>((*e)[1]),
                           static_cast<float>((*e)[2])};
        }
    }

    // L'ordre parents-avant-enfants : les os ranges par profondeur. Un os dont la chaine de
    // parents ne finit pas est un cycle.
    bool orderJoints() {
        const std::vector<MeshJoint>& joints = _mesh.rig.joints;
        std::vector<std::size_t> depths(joints.size(), 0);
        for (std::size_t rank = 0; rank < joints.size(); ++rank) {
            int parent = joints[rank].parent;
            while (parent >= 0) {
                if (++depths[rank] > joints.size()) {
                    return fail(MeshFileError::MalformedStructure, "joint graph is not a tree");
                }
                parent = joints[static_cast<std::size_t>(parent)].parent;
            }
        }
        std::vector<std::uint16_t>& order = _mesh.rig.order;
        order.resize(joints.size());
        for (std::size_t rank = 0; rank < order.size(); ++rank) {
            order[rank] = static_cast<std::uint16_t>(rank);
        }
        std::ranges::stable_sort(
            order, [&depths](std::uint16_t a, std::uint16_t b) { return depths[a] < depths[b]; });
        // Un sommet ne peut citer qu'un os du squelette.
        const auto beyond = [&joints](const MeshSkinVertex& vertex) {
            return std::ranges::any_of(
                vertex.joints, [&joints](std::uint16_t joint) { return joint >= joints.size(); });
        };
        if (std::ranges::any_of(_mesh.skin, beyond)) {
            return fail(MeshFileError::MalformedStructure, "vertex cites a joint beyond the skin");
        }
        return true;
    }

    // Les clips : par animation, les canaux de translation et de rotation qui visent un os.
    bool readAnimations() {
        const auto animations = _root.find("animations");
        if (animations == _root.end() || !animations->is_array()) {
            return true;
        }
        if (animations->size() > MAX_CLIPS) {
            return fail(MeshFileError::Unsupported, "too many animations");
        }
        for (const Json& animation : *animations) {
            if (!animation.is_object()) {
                return fail(MeshFileError::MalformedStructure, "animation is not an object");
            }
            MeshClip clip;
            if (const auto name = animation.find("name");
                name != animation.end() && name->is_string()) {
                clip.name = name->get<std::string>();
            }
            clip.tracks.resize(_mesh.rig.joints.size());
            const auto channels = animation.find("channels");
            if (channels == animation.end() || !channels->is_array()) {
                return fail(MeshFileError::MalformedStructure, "animation without channels");
            }
            for (const Json& channel : *channels) {
                if (!channel.is_object()) {
                    return fail(MeshFileError::MalformedStructure, "channel is not an object");
                }
                if (!readChannel(animation, channel, clip)) {
                    return false;
                }
            }
            _mesh.rig.clips.push_back(std::move(clip));
        }
        return true;
    }

    // Un canal : sa cible (un os, sa translation ou sa rotation) et les cles de son
    // echantillonneur. Ce qui n'est pas lu -- une autre cible, une echelle, une courbe CUBICSPLINE
    // -- est ignore.
    bool readChannel(const Json& animation, const Json& channel, MeshClip& clip) {
        const auto target = channel.find("target");
        if (target == channel.end() || !target->is_object()) {
            return fail(MeshFileError::MalformedStructure, "channel without target");
        }
        const std::optional<std::size_t> node = indexField(*target, "node", std::nullopt);
        const auto path = target->find("path");
        if (!node || *node >= _jointOfNode.size() || _jointOfNode[*node] < 0 ||
            path == target->end() || !path->is_string()) {
            return true;
        }
        const std::string name = path->get<std::string>();
        const bool rotation = name == "rotation";
        if (!rotation && name != "translation") {
            return true;
        }
        const std::optional<std::size_t> samplerIndex =
            indexField(channel, "sampler", std::nullopt);
        const Json* const sampler =
            samplerIndex ? element(animation, "samplers", *samplerIndex) : nullptr;
        if (sampler == nullptr) {
            return fail(MeshFileError::MalformedStructure, "channel cites a missing sampler");
        }
        bool step = false;
        if (const auto mode = sampler->find("interpolation");
            mode != sampler->end() && mode->is_string()) {
            const std::string interpolation = mode->get<std::string>();
            if (interpolation == "CUBICSPLINE") {
                return true;
            }
            step = interpolation == "STEP";
        }
        const std::optional<std::size_t> input = indexField(*sampler, "input", std::nullopt);
        const std::optional<std::size_t> output = indexField(*sampler, "output", std::nullopt);
        if (!input || !output) {
            return fail(MeshFileError::MalformedStructure, "sampler is incomplete");
        }
        const std::optional<AccessorView> times = accessor(*input);
        const std::optional<AccessorView> values = times ? accessor(*output) : std::nullopt;
        if (!values) {
            return false;
        }
        const std::size_t width = rotation ? 4 : 3;
        if (times->components != 1 || times->componentType != COMPONENT_FLOAT ||
            values->components != width || values->count != times->count ||
            (values->componentType != COMPONENT_FLOAT && !values->normalized)) {
            return fail(MeshFileError::MalformedStructure, "sampler keys are malformed");
        }
        if (times->count > MAX_KEYS) {
            return fail(MeshFileError::Unsupported, "too many animation keys");
        }
        MeshJointTrack& track = clip.tracks[static_cast<std::size_t>(_jointOfNode[*node])];
        MeshChannel& keys = rotation ? track.rotation : track.translation;
        keys.step = step;
        keys.times.resize(times->count);
        keys.values.resize(times->count * width);
        for (std::size_t item = 0; item < times->count; ++item) {
            const float time = component(*times, item, 0);
            if (!std::isfinite(time) || time < 0.0F || (item > 0 && time < keys.times[item - 1])) {
                return fail(MeshFileError::MalformedStructure,
                            "animation keys must be finite and ordered");
            }
            keys.times[item] = time;
            for (std::size_t part = 0; part < width; ++part) {
                const float value = component(*values, item, part);
                if (!std::isfinite(value)) {
                    return fail(MeshFileError::MalformedStructure, "animation key is not finite");
                }
                keys.values[(item * width) + part] = value;
            }
            clip.duration = std::max(clip.duration, time);
        }
        return true;
    }

    void measure() {
        if (_mesh.vertices.empty()) {
            return;
        }
        _mesh.minimum = _mesh.vertices.front().position;
        _mesh.maximum = _mesh.vertices.front().position;
        for (const MeshVertex& vertex : _mesh.vertices) {
            for (std::size_t axis = 0; axis < 3; ++axis) {
                _mesh.minimum[axis] = std::min(_mesh.minimum[axis], vertex.position[axis]);
                _mesh.maximum[axis] = std::max(_mesh.maximum[axis], vertex.position[axis]);
            }
        }
    }

    const Json& _root;
    std::span<const std::byte> _binary;
    MeshData _mesh;
    Failure _failure{.error = MeshFileError::MalformedStructure, .message = {}};
    std::vector<bool> _visited;
    std::set<std::size_t> _materials;
    /// Le squelette du premier maillage lie rencontre, et le rang d'os de chaque noeud (-1 sinon).
    std::optional<std::size_t> _skin;
    std::vector<int> _jointOfNode;
    std::size_t _firstMaterial = std::numeric_limits<std::size_t>::max();
};

[[nodiscard]] MeshFileResult readGlb(std::span<const std::byte> bytes) {
    if (bytes.size() < GLB_HEADER_BYTES + CHUNK_HEADER_BYTES || readU32(bytes, 0) != GLB_MAGIC) {
        return failed(MeshFileError::ParseError, "not a .glb file");
    }
    if (readU32(bytes, 4) != GLTF_VERSION) {
        return failed(MeshFileError::ParseError, "only glTF 2.0 is read");
    }
    // La longueur declaree borne la lecture : ce qui la suit n'est pas le fichier.
    const std::size_t declared = readU32(bytes, 8);
    if (declared < GLB_HEADER_BYTES + CHUNK_HEADER_BYTES || declared > bytes.size()) {
        return failed(MeshFileError::ParseError, "declared length exceeds the file");
    }
    bytes = bytes.first(declared);

    std::span<const std::byte> jsonChunk;
    std::span<const std::byte> binaryChunk;
    std::size_t offset = GLB_HEADER_BYTES;
    while (bytes.size() - offset >= CHUNK_HEADER_BYTES) {
        const std::size_t length = readU32(bytes, offset);
        const std::uint32_t type = readU32(bytes, offset + 4);
        offset += CHUNK_HEADER_BYTES;
        if (length > bytes.size() - offset) {
            return failed(MeshFileError::ParseError, "chunk exceeds the file");
        }
        if (type == CHUNK_JSON && jsonChunk.empty()) {
            jsonChunk = bytes.subspan(offset, length);
        } else if (type == CHUNK_BIN && binaryChunk.empty()) {
            binaryChunk = bytes.subspan(offset, length);
        }
        // Les blocs sont alignes sur quatre octets.
        offset += length;
        offset += (4 - (offset % 4)) % 4;
        if (offset > bytes.size()) {
            break;
        }
    }
    if (jsonChunk.empty()) {
        return failed(MeshFileError::ParseError, "JSON chunk missing");
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): le bloc JSON est du texte.
    const char* const text = reinterpret_cast<const char*>(jsonChunk.data());
    const Json root = Json::parse(text, text + jsonChunk.size(), nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        return failed(MeshFileError::ParseError, "JSON chunk is not a glTF document");
    }
    return GlbReader(root, binaryChunk).read();
}

}  // namespace

bool isMeshPath(std::string_view path) noexcept {
    if (path.size() < MESH_FILE_EXTENSION.size()) {
        return false;
    }
    const std::string_view tail = path.substr(path.size() - MESH_FILE_EXTENSION.size());
    return std::ranges::equal(tail, MESH_FILE_EXTENSION, [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == b;
    });
}

MeshFileResult readMeshFromGlb(std::span<const std::byte> bytes) {
    // Aucune lecture ne leve (EX-NFR-040) : une allocation refusee ou une conversion que nlohmann
    // rejette devient une erreur du resultat.
    try {
        return readGlb(bytes);
    } catch (const std::exception& error) {
        return failed(MeshFileError::MalformedStructure, error.what());
    } catch (...) {
        return failed(MeshFileError::MalformedStructure, "unreadable mesh");
    }
}

MeshFileResult readMeshFile(const std::filesystem::path& path) {
    try {
        std::error_code error;
        const std::uintmax_t size = std::filesystem::file_size(path, error);
        if (error || size > MESH_FILE_MAX_BYTES) {
            return failed(MeshFileError::FileNotFound,
                          error ? "cannot read " + path.string()
                                : path.string() + " is larger than a mesh may be");
        }
        std::ifstream file(path, std::ios::binary);
        std::vector<std::byte> bytes(static_cast<std::size_t>(size));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): `ifstream` lit des `char`.
        if (!file || !file.read(reinterpret_cast<char*>(bytes.data()),
                                static_cast<std::streamsize>(bytes.size()))) {
            return failed(MeshFileError::FileNotFound, "cannot read " + path.string());
        }
        MeshFileResult result = readMeshFromGlb(bytes);
        if (!result.ok()) {
            result.message = path.string() + ": " + result.message;
        }
        return result;
    } catch (...) {
        return failed(MeshFileError::FileNotFound, "cannot read mesh file");
    }
}

}  // namespace core
