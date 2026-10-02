// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

// Vertex shader des maillages **animes** (LOT-1005) : chaque sommet suit quatre os au plus, et sa
// position est la somme ponderee de ce que chacun en fait. Les matrices d'os arrivent deja
// composees avec leur matrice de liaison inverse (`core::poseSkeleton`) : elles vont du repere du
// maillage au repere du maillage, puis `uClip` projette comme pour un maillage fixe (`mesh.vert`).
//
// Les rangs d'os sont televerses en flottants : un attribut entier n'est pas porte par tous les
// backends de QRhi, et 64 os se disent exactement en flottant.
#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 joints;
layout(location = 4) in vec4 weights;

layout(location = 0) out vec2 vUv;
layout(location = 1) out vec3 vNormal;

layout(std140, binding = 0) uniform Draw {
    mat4 uClip;
    vec4 uTint;
};

// Le plus d'os qu'un squelette dessine peut porter : `hmi::MeshBatch::MAX_BONES`.
layout(std140, binding = 2) uniform Bones {
    mat4 uBones[64];
};

void main() {
    mat4 skin = weights.x * uBones[int(joints.x)] + weights.y * uBones[int(joints.y)] +
                weights.z * uBones[int(joints.z)] + weights.w * uBones[int(joints.w)];
    vUv = uv;
    vNormal = mat3(skin) * normal;
    gl_Position = uClip * (skin * vec4(position, 1.0));
}
