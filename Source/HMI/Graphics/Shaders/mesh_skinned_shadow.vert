// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

// Vertex shader de la **passe d'ombres** des maillages animes (LOT-1007) : la pose de
// `mesh_skinned.vert`, projetee dans la carte d'ombres.
#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 joints;
layout(location = 4) in vec4 weights;

layout(std140, binding = 0) uniform Draw {
    mat4 uClip;
    vec4 uTint;
    // Du repere du maillage a celui de la vue (LOT-1007) : la position et la normale eclairees.
    mat4 uView;
    // Du repere du maillage au clip de la carte d'ombres : lu par la passe d'ombres seule.
    mat4 uShadowClip;
};

// Le plus d'os qu'un squelette dessine peut porter : `hmi::MeshBatch::MAX_BONES`.
layout(std140, binding = 2) uniform Bones {
    mat4 uBones[64];
};

void main() {
    mat4 skin = weights.x * uBones[int(joints.x)] + weights.y * uBones[int(joints.y)] +
                weights.z * uBones[int(joints.z)] + weights.w * uBones[int(joints.w)];
    gl_Position = uShadowClip * (skin * vec4(position, 1.0));
}
