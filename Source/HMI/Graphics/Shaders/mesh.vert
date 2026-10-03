// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

// Vertex shader de la passe de maillages (LOT-1003) : projette un sommet du maillage -- sa
// matrice va de son repere a l'espace de clip, pose et camera composees cote C++ -- et transmet ses
// coordonnees de texture, sa normale et sa position dans la vue (LOT-1007).
//
// La matrice recue est deja corrigee par `QRhi::clipSpaceCorrMatrix()` : le shader ecrit en espace
// de clip OpenGL, comme `sprite.vert`.
#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec2 vUv;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec3 vView;

layout(std140, binding = 0) uniform Draw {
    mat4 uClip;
    vec4 uTint;
    // Du repere du maillage a celui de la vue (LOT-1007) : la position et la normale eclairees.
    mat4 uView;
    // Du repere du maillage au clip de la carte d'ombres : lu par la passe d'ombres seule.
    mat4 uShadowClip;
};

void main() {
    vUv = uv;
    // La vue est une rotation du lieu, a une echelle pres : la normale la suit par la meme matrice.
    vNormal = mat3(uView) * normal;
    vView = (uView * vec4(position, 1.0)).xyz;
    gl_Position = uClip * vec4(position, 1.0);
}
