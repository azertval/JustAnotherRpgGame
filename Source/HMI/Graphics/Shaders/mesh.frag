// SPDX-FileCopyrightText: 2026 Valentin Eloy
// SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

// Fragment shader de la passe de maillages (LOT-1003), **eclaire** depuis le LOT-1007 : la couleur
// de base du modele, sous l'ambiance, le soleil et les lumieres de nuit de l'heure.
//
// Un bloc d'eclairage neutre (`uHourTint.a` nul) rend la couleur de base seule, comme avant le
// lot : c'est celui de tout rendu qui ne regle pas de lumiere.
//
// L'alpha est premultiplie, comme partout dans le rendu : la texture l'est au televersement, la
// teinte l'est ici. A l'opacite 1 d'un maillage opaque, le melange du pipeline ne change rien.
#version 440

layout(location = 0) in vec2 vUv;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec3 vView;

layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform Draw {
    mat4 uClip;
    vec4 uTint;
    // Du repere du maillage a celui de la vue (LOT-1007) : la position et la normale eclairees.
    mat4 uView;
    // Du repere du maillage au clip de la carte d'ombres : lu par la passe d'ombres seule.
    mat4 uShadowClip;
};
layout(binding = 1) uniform sampler2D uBaseColor;

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

// Ce qu'une face tournee a l'oppose garde d'une lumiere : elle « enveloppe » le volume plutot que
// de le couper net, ce qui garde la facture peinte.
const float WRAP = 0.3;
// Ce qu'une face tournee vers le sol garde de l'ambiance, qui vient surtout du ciel.
const float GROUND_AMBIENT = 0.72;
// Ce dont un point s'ecarte de sa surface, en unites de la vue, avant de lire son ombre : sans
// lui, une face eclairee s'ombrerait elle-meme par l'arrondi de la carte.
const float SHADOW_OFFSET = 0.06;
// Ce qu'un pixel peut recevoir au plus (le meme qu'une image, `sprite.frag`).
const float BRIGHTEST = 1.08;

float wrapped(float facing) {
    return clamp((facing + WRAP) / (1.0 + WRAP), 0.0, 1.0);
}

void main() {
    vec4 base = texture(uBaseColor, vUv) * vec4(uTint.rgb * uTint.a, uTint.a);
    if (uHourTint.a < 0.5) {
        fragColor = base;
        return;
    }
    // Aucune face n'est ecartee par son sens : celle qu'on voit regarde la camera, quoi que dise
    // sa normale -- un modele genere n'a pas toujours un sens de face fiable. La camera regarde
    // vers +z.
    vec3 normal = normalize(vNormal);
    if (normal.z > 0.0) {
        normal = -normal;
    }
    float sky = 0.5 + 0.5 * dot(normal, uUp.xyz);
    vec3 light = uAmbient.rgb * mix(GROUND_AMBIENT, 1.0, sky);

    float facing = dot(normal, uToSun.xyz);
    float shade = shadowAt(vView + normal * SHADOW_OFFSET);
    light += uSun.rgb * wrapped(facing) * (1.0 - shade);

    // Les lampes ne s'ajoutent pas a la lumiere du jour : elles comblent ce qui lui manque pour
    // arriver au plus clair, comme pour une image.
    vec3 lamps = vec3(0.0);
    int count = int(uUp.w);
    for (int index = 0; index < count; ++index) {
        vec3 toLamp;
        vec3 lamp = lampAt(index, vView, toLamp);
        lamps += lamp * wrapped(dot(normal, toLamp));
    }
    light += min(lamps, vec3(1.0)) * max(vec3(BRIGHTEST) - light, vec3(0.0));
    fragColor = vec4(min(base.rgb * light, vec3(base.a)), base.a);
}
