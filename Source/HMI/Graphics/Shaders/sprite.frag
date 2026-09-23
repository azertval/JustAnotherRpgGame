// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

// Fragment shader du pipeline 2D : échantillonne la texture liée et la multiplie par la teinte du
// sommet. Le filtrage -- bilinéaire avec mipmaps pour l'art peint, au plus proche pour une image
// engendrée -- est fixé par le `QRhiSampler` côté C++, pas ici (`EX-ARCH-022`, `LOT-103`).
//
// L'alpha est **prémultiplié** : toute texture l'est au téléversement (`hmi::createTexture`), et la
// teinte l'est ici, en accord avec le mélange One / OneMinusSrcAlpha configuré sur le pipeline.
#version 440

layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vColor;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D uTexture;

void main() {
    fragColor = texture(uTexture, vUv) * vec4(vColor.rgb * vColor.a, vColor.a);
}
