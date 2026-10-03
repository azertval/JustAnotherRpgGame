// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

// Fragment shader du pipeline 2D : échantillonne la texture liée et la multiplie par la teinte du
// sommet. Le filtrage -- bilinéaire avec mipmaps pour l'art peint, au plus proche pour une image
// engendrée -- est fixé par le `QRhiSampler` côté C++, pas ici (`EX-ARCH-022`, `LOT-103`).
//
// L'alpha est **prémultiplié** : toute texture l'est au téléversement (`hmi::createTexture`), et la
// teinte l'est ici, en accord avec le mélange One / OneMinusSrcAlpha configuré sur le pipeline.
//
// Depuis le LOT-1007, une image prend la **teinte de l'heure**, les lumieres de nuit et, au sol,
// les ombres portees. Elle n'a pas de normale : le soleil ne la modele pas. Un bloc d'eclairage
// neutre (`uHourTint.a` nul), ou un quad qui garde son eclat (`vLight.x` nul), rend ce que le
// shader rendait avant le lot.
#version 440

layout(location = 0) in vec2 vUv;
layout(location = 1) in vec4 vColor;
layout(location = 2) in vec3 vView;
layout(location = 3) in vec2 vLight;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D uTexture;

// Le bloc d'eclairage de l'image (LOT-1007, `hmi::LightingUniforms`). Le meme texte dans
// `sprite.frag` et `mesh.frag` : `qsb` n'inclut pas de fichier.
layout(std140, binding = 3) uniform Lighting {
    mat4 uViewToShadow;
    vec4 uHourTint;   // rgb : teinte des images ; a : 1 si l'eclairage est actif
    vec4 uAmbient;    // rgb : ambiance des maillages ; a : ce qu'une ombre retire a une image
    vec4 uSun;        // rgb : lumiere dirigee ; a : 1 si la carte d'ombres est a lire
    vec4 uToSun;      // xyz : vers le soleil, dans la vue ; w : cote d'un texel de la carte
    vec4 uUp;         // xyz : la verticale du lieu, dans la vue ; w : nombre de lumieres
    vec4 uLightPosition[16];  // xyz : dans la vue ; w : portee
    vec4 uLightColor[16];
};
layout(binding = 4) uniform sampler2DShadow uShadowMap;

// La part d'ombre du point `view`, de 0 (au soleil) a 1 : neuf lectures comparees autour de lui,
// chacune deja lissee par l'echantillonneur. Hors de la carte, le point est au soleil.
float shadowAt(vec3 view) {
    if (uSun.a < 0.5) {
        return 0.0;
    }
    vec3 place = (uViewToShadow * vec4(view, 1.0)).xyz;
    if (place.x <= 0.0 || place.x >= 1.0 || place.y <= 0.0 || place.y >= 1.0 || place.z >= 1.0) {
        return 0.0;
    }
    float lit = 0.0;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            lit += texture(uShadowMap, vec3(place.xy + vec2(dx, dy) * uToSun.w, place.z));
        }
    }
    return 1.0 - lit / 9.0;
}

// Ce qu'une lumiere de nuit donne au point `view` : sa couleur, eteinte en douceur a sa portee.
vec3 lampAt(int index, vec3 view, out vec3 toLamp) {
    vec3 away = uLightPosition[index].xyz - view;
    float distance = length(away);
    toLamp = distance > 0.0001 ? away / distance : vec3(0.0, 0.0, -1.0);
    float reach = clamp(1.0 - (distance * distance) /
                                  (uLightPosition[index].w * uLightPosition[index].w), 0.0, 1.0);
    return uLightColor[index].rgb * reach * reach;
}

// Ce qu'un pixel peut recevoir au plus. Les lampes ne s'ajoutent pas a la teinte : elles comblent
// ce qui lui manque pour y arriver -- un sol clair sous deux lanternes ne brule pas.
const float BRIGHTEST = 1.08;

void main() {
    vec4 base = texture(uTexture, vUv) * vec4(vColor.rgb * vColor.a, vColor.a);
    if (uHourTint.a < 0.5 || vLight.x <= 0.0) {
        fragColor = base;
        return;
    }
    float shade = vLight.y > 0.5 ? shadowAt(vView) : 0.0;
    vec3 light = uHourTint.rgb * (1.0 - uAmbient.a * shade);
    vec3 lamps = vec3(0.0);
    int count = int(uUp.w);
    for (int index = 0; index < count; ++index) {
        vec3 toLamp;
        lamps += lampAt(index, vView, toLamp);
    }
    light += min(lamps, vec3(1.0)) * max(vec3(BRIGHTEST) - light, vec3(0.0));
    light = mix(vec3(1.0), light, vLight.x);
    fragColor = vec4(min(base.rgb * light, vec3(base.a)), base.a);
}
