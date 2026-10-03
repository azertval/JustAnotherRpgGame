// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

// Vertex shader de la **passe d'ombres** (LOT-1007) : le meme sommet que `mesh.vert`, projete
// dans la carte d'ombres -- vu du soleil. Les memes entrees, pour le meme tampon de sommets.
#version 440

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(std140, binding = 0) uniform Draw {
    mat4 uClip;
    vec4 uTint;
    // Du repere du maillage a celui de la vue (LOT-1007) : la position et la normale eclairees.
    mat4 uView;
    // Du repere du maillage au clip de la carte d'ombres : lu par la passe d'ombres seule.
    mat4 uShadowClip;
};

void main() {
    gl_Position = uShadowClip * vec4(position, 1.0);
}
