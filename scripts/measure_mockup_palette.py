#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Releve de la palette de la charte v2 sur les maquettes du LOT-87 (T2.1).

Meme regle que le `LOT-66` sur les feuilles de personnage : **une couleur se releve, elle ne se
choisit pas a vue**. Chaque role nouveau de `Source/Ui/Theme/Tokens.qml` est mesure ici sur une
zone nommee d'une maquette de `Documentation/Lot/LOT-87-charte-v2/references/`, et la valeur
ecrite dans les jetons est celle que ce script imprime. Rejouer le script rejoue le releve.

Deux mesures, parce qu'une maquette porte deux sortes de matiere :

- **surface** -- un fond de panneau, une plaque : le mode de l'histogramme quantifie de la zone.
  La zone est choisie sans texte ni ornement, si bien que le mode EST la matiere ;
- **trait** -- un texte, un filet, une icone : le mode du fond est d'abord releve, puis seuls les
  pixels dont l'ecart de luminance a ce fond atteint 60 % de l'ecart le plus fort sont gardes. Sans
  ce tri, un glyphe anticrenele de 18 px contient plus de pixels de bord, melanges au fond, que de
  coeur de trait : le mode serait une couleur intermediaire qui n'existe nulle part a l'ecran. Le
  seuil est relatif, et non un quart fixe des pixels : un libelle court occupe moins d'un quart de
  sa zone, et un quart fixe y reprendrait du fond ;
- **lumiere** -- la face eclairee d'une matiere en relief (une plaque de grenat) : le mode du quart
  le plus lumineux de la zone. Le mode de la zone entiere donnerait sa face ombree, qui est un
  autre role.

La quantification est de 8 niveaux par canal (valeur rapportee au centre de la case), et non de 16
comme au `LOT-66` : les panneaux sombres de la v2 tiennent tous entre #000000 et #181818, qu'un pas
de 16 confond en un seul noir.

Usage :
    python scripts/measure_mockup_palette.py            # imprime la table role -> valeur -> zone
    python scripts/measure_mockup_palette.py --check    # echoue si Tokens.qml diverge du releve
    python scripts/measure_mockup_palette.py --annotate <dossier>
                                                        # dessine chaque zone sur sa maquette

Dependance : Pillow. Le script n'est pas en CI : les maquettes ne changent pas, et le runner de
lint n'installe aucun paquet. `--check` se lance a la main quand `Tokens.qml` est retouche.
"""

from __future__ import annotations

import re
import sys
from collections import Counter
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
REFERENCES = ROOT / "Documentation" / "Lot" / "LOT-87-charte-v2" / "references"
TOKENS = ROOT / "Source" / "Ui" / "Theme" / "Tokens.qml"

STEP = 8

# (role, maquette, (x0, y0, x1, y1), mesure, ce que la zone montre)
SAMPLES = [
    ("panel", "07_Credit_Mockup.png", (420, 470, 800, 480), "surface",
     "fond du grand panneau des credits, entre deux sections"),
    ("panelRaised", "01_InGame_HUD_Mockup.png", (1560, 288, 1640, 310), "surface",
     "fond du panneau des quetes du HUD, a droite du titre"),
    ("panelEdge", "05_Options_Mockup.png", (1417, 300, 1424, 800), "trait",
     "filet d'or du bord droit du panneau des options"),
    ("goldLight", "05_Options_Mockup.png", (466, 160, 500, 200), "trait",
     "losange d'or de l'intertitre Langue"),
    ("gemLight", "06_Main_Menu_Mockup.png", (392, 372, 500, 404), "lumiere",
     "plaque grenat de l'entree active Continuer, apres le libelle"),
    ("textOnPanel", "06_Main_Menu_Mockup.png", (265, 455, 420, 480), "trait",
     "libelle Nouvelle partie"),
    ("textOnPanelMuted", "05_Options_Mockup.png", (1010, 533, 1085, 550), "trait",
     "libelle desactive Multijoueur"),
    ("textAlly", "01_InGame_HUD_Mockup.png", (72, 692, 228, 710), "trait",
     "libelle Tour de Kaelith Voss du journal de combat"),
    ("textEnemy", "01_InGame_HUD_Mockup.png", (72, 806, 190, 824), "trait",
     "libelle Tour de l'ennemi du journal de combat"),
    ("success", "05_Options_Mockup.png", (1368, 884, 1388, 902), "lumiere",
     "plaque verte du bouton Appliquer, apres le libelle"),
    ("danger", "05_Options_Mockup.png", (1172, 884, 1196, 902), "lumiere",
     "plaque rouge du bouton Annuler, apres le libelle"),
    ("info", "05_Options_Mockup.png", (530, 884, 552, 902), "lumiere",
     "plaque bleu nuit du bouton Par defaut, entre l'icone et le libelle"),
]

# Roles GARDES du LOT-66 (releves sur le corpus, pas sur les maquettes) : mesures ici pour
# montrer que les maquettes ne les contredisent pas, jamais ecrits d'apres ce releve.
KEPT = [
    ("surface", "04_Inventory_Equipment_Mockup.png", (1150, 560, 1450, 650), "surface",
     "champ du parchemin de l'inventaire (maquette 04 : meme matiere)"),
]


def luminance(pixel: tuple[int, int, int]) -> float:
    return 0.299 * pixel[0] + 0.587 * pixel[1] + 0.114 * pixel[2]


def quantize(pixel: tuple[int, int, int]) -> tuple[int, int, int]:
    return tuple(min(255, (v // STEP) * STEP + STEP // 2) for v in pixel)


def measure(image: str, box: tuple[int, int, int, int], kind: str) -> str:
    pixels = list(Image.open(REFERENCES / image).convert("RGB").crop(box).getdata())
    background = Counter(quantize(p) for p in pixels).most_common(1)[0][0]
    if kind == "trait":
        ground = luminance(background)
        strongest = max(abs(luminance(p) - ground) for p in pixels)
        core = [p for p in pixels if abs(luminance(p) - ground) >= 0.6 * strongest]
        chosen = Counter(quantize(p) for p in core).most_common(1)[0][0]
    elif kind == "lumiere":
        pixels.sort(key=luminance, reverse=True)
        chosen = Counter(quantize(p) for p in pixels[: max(1, len(pixels) // 4)]).most_common(1)[0][0]
    else:
        chosen = background
    return "#%02x%02x%02x" % chosen


def token_values() -> dict[str, str]:
    text = TOKENS.read_text(encoding="utf-8")
    return {name: value.lower() for name, value in
            re.findall(r'property color (\w+):\s*"(#[0-9a-fA-F]{6})"', text)}


def main(argv: list[str]) -> int:
    rows = [(role, image, box, kind, what, measure(image, box, kind))
            for role, image, box, kind, what in SAMPLES]

    if "--annotate" in argv:
        out = Path(argv[argv.index("--annotate") + 1])
        out.mkdir(parents=True, exist_ok=True)
        images: dict[str, Image.Image] = {}
        for role, image, box, *_ in rows + [k + ("",) for k in KEPT]:
            canvas = images.setdefault(image, Image.open(REFERENCES / image).convert("RGB"))
            draw = ImageDraw.Draw(canvas)
            draw.rectangle(box, outline=(0, 255, 255), width=2)
            draw.text((box[0], max(0, box[1] - 12)), role, fill=(0, 255, 255))
        for name, canvas in images.items():
            canvas.save(out / name)

    if "--check" in argv:
        tokens = token_values()
        failures = [f"{role} : Tokens.qml {tokens.get(role, 'absent')}, releve {value} ({image})"
                    for role, image, _, _, _, value in rows if tokens.get(role) != value]
        for line in failures:
            print(line)
        print(f"{len(rows)} roles verifies, {len(failures)} divergence(s).")
        return 1 if failures else 0

    print("| Role | Valeur | Maquette | Zone (x0, y0, x1, y1) | Mesure | Ce que la zone montre |")
    print("|---|---|---|---|---|---|")
    for role, image, box, kind, what, value in rows:
        print(f"| `{role}` | `{value}` | {image[:2]} | {box} | {kind} | {what} |")
    for role, image, box, kind, what in KEPT:
        print(f"| `{role}` (garde) | `{measure(image, box, kind)}` | {image[:2]} | {box} | {kind} | {what} |")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
