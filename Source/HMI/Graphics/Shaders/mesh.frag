// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

// Fragment shader de la passe de maillages (LOT-1003) : la couleur de base du modele, et rien
// d'autre. **Non eclaire** : la normale arrive jusqu'ici pour le LOT-1007, qui l'eclairera ; d'ici
// la, un modele se juge a sa seule couleur de base (standard 3D, §4).
//
// L'alpha est premultiplie, comme partout dans le rendu : la texture l'est au televersement, la
// teinte l'est ici. A l'opacite 1 d'un maillage opaque, le melange du pipeline ne change rien.
#version 440

layout(location = 0) in vec2 vUv;
layout(location = 1) in vec3 vNormal;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform Draw {
    mat4 uClip;
    vec4 uTint;
};
layout(binding = 1) uniform sampler2D uBaseColor;

void main() {
    fragColor = texture(uBaseColor, vUv) * vec4(uTint.rgb * uTint.a, uTint.a);
}
