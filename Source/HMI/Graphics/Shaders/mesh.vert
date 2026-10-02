// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

// Vertex shader de la passe de maillages (LOT-1003) : projette un sommet du maillage -- sa
// matrice va de son repere a l'espace de clip, pose et camera composees cote C++ -- et transmet ses
// coordonnees de texture et sa normale.
//
// La matrice recue est deja corrigee par `QRhi::clipSpaceCorrMatrix()` : le shader ecrit en espace
// de clip OpenGL, comme `sprite.vert`.
#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec2 vUv;
layout(location = 1) out vec3 vNormal;

layout(std140, binding = 0) uniform Draw {
    mat4 uClip;
    vec4 uTint;
};

void main() {
    vUv = uv;
    vNormal = normal;
    gl_Position = uClip * vec4(position, 1.0);
}
