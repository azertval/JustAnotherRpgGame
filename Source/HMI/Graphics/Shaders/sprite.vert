// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

// Vertex shader du pipeline 2D : projette la position de la vue -- le plan de l'image et, depuis
// le LOT-1003, la profondeur -- et transmet UV et teinte. Écrit en GLSL 4.40 pour `qsb`, qui le traduit vers HLSL/SPIR-V/MSL selon le backend
// retenu par QRhi -- Direct3D 11 sous Windows (`EX-REN-002`).
//
// La matrice reçue est déjà corrigée par `QRhi::clipSpaceCorrMatrix()` côté C++ : le shader écrit
// donc en espace de clip OpenGL, convention que QRhi impose à la source, quelle que soit la cible.
#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;

layout(location = 0) out vec2 vUv;
layout(location = 1) out vec4 vColor;

layout(std140, binding = 0) uniform Projection {
    mat4 uProjection;
};

void main() {
    vUv = uv;
    vColor = color;
    gl_Position = uProjection * vec4(position, 1.0);
}
