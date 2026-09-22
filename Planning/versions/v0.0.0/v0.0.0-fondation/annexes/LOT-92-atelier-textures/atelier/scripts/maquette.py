"""Atelier des textures (LOT-92), T1 : prépare l'envoi de la maquette de style, à la main dans ChatGPT.
Usage : py -3.13 maquette.py <tour>
Écrit <TEXTURE_ATELIER>/chatgpt/maquette-tour<K>/ : prompt.txt, 1_echelle.png et A_ENREGISTRER_SOUS.txt.

La référence d'échelle est composée, pas dessinée : une grille de losanges 68 × 42 pixels d'art
(rapport 0,62 d'IsoProjection) en contour fin, et la première image d'idle d'Anariel et de Jade
(bandes 48 × 64 de Assets/Npc, pied à x = 24, bas de cellule) posées au centre de deux cases, le tout
à 2 pixels d'écran par pixel d'art — le pas de pixel que le prompt demande à la scène de reprendre."""
import os
import pathlib
import shutil
import sys

from PIL import Image, ImageDraw

DEPOT = pathlib.Path(__file__).resolve().parents[1]
RACINE = pathlib.Path(__file__).resolve().parents[5]
NPC = RACINE / "Source" / "Elements" / "Assets" / "Npc"
TRAVAIL = pathlib.Path(os.environ.get("TEXTURE_ATELIER", r"D:/JustAnotherDnDGame-textures"))

PAS = 2                          # pixels d'écran par pixel d'art
TUILE_L, TUILE_H = 68, 42        # losange, en pixels d'art (68 × 0,62 ≈ 42)
CELLULE_L, CELLULE_H = 48, 64    # cellule des bandes d'animation
LARGEUR, HAUTEUR = 1536 // PAS, 1024 // PAS
CONTOUR = (60, 90, 140, 255)
FIGURINES = [("anariel", (3, 4)), ("jade", (5, 3))]   # slug, case (colonne, ligne)
COLONNES = LIGNES = 9


def centre(c: int, r: int) -> tuple[int, int]:
    """Centre du losange de la case (c, r), grille centrée sur l'image."""
    x0 = LARGEUR // 2 - (COLONNES - LIGNES) * TUILE_L // 4
    y0 = HAUTEUR // 2 - (COLONNES + LIGNES) * TUILE_H // 4
    return x0 + (c - r) * TUILE_L // 2, y0 + (c + r + 1) * TUILE_H // 2


def echelle() -> Image.Image:
    art = Image.new("RGBA", (LARGEUR, HAUTEUR), (0, 0, 0, 0))
    trait = ImageDraw.Draw(art)
    for r in range(LIGNES):
        for c in range(COLONNES):
            x, y = centre(c, r)
            trait.polygon([(x, y - TUILE_H // 2), (x + TUILE_L // 2, y), (x, y + TUILE_H // 2),
                           (x - TUILE_L // 2, y)], outline=CONTOUR)
    for slug, (c, r) in FIGURINES:
        bande = Image.open(NPC / slug / "idle.png").convert("RGBA")
        image = bande.crop((0, 0, CELLULE_L, CELLULE_H))
        x, y = centre(c, r)
        art.alpha_composite(image, (x - CELLULE_L // 2, y - CELLULE_H))
    return art.resize((LARGEUR * PAS, HAUTEUR * PAS), Image.NEAREST)


def main() -> int:
    if len(sys.argv) != 2 or not sys.argv[1].isdigit():
        print(__doc__)
        return 2
    k = int(sys.argv[1])
    d = TRAVAIL / "chatgpt" / f"maquette-tour{k}"
    if d.exists():
        shutil.rmtree(d)
    d.mkdir(parents=True)
    shutil.copy(DEPOT / "prompts" / "maquette.txt", d / "prompt.txt")
    echelle().save(d / "1_echelle.png")
    cible = TRAVAIL / "maquette" / f"tour{k}" / "candidat1.png"
    cible.parent.mkdir(parents=True, exist_ok=True)
    (d / "A_ENREGISTRER_SOUS.txt").write_text(str(cible) + "\n", encoding="utf-8")
    print(f"{d} : 1 pièce jointe -> {cible}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
