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
            if (!readMesh(*found, world)) {
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

    bool readMesh(const Json& mesh, const Matrix& world) {
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
            if (!readPrimitive(primitive, world)) {
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

    bool readPrimitive(const Json& primitive, const Matrix& world) {
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
    const char* const text = static_cast<const char*>(static_cast<const void*>(jsonChunk.data()));
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
        if (!file || !file.read(static_cast<char*>(static_cast<void*>(bytes.data())),
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
