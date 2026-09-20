#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Monte la maquette de validation du standard 2D HD (LOT-101).

Le standard 2D HD fixe un losange de sol de 256 x 159 pixels d'art et une figurine de 170 px.
Ces valeurs ne se jugent pas sur un tableau : il faut voir huit cases sur huit, avec du sol, des
facades, une colonnade, une fontaine et un lampadaire, aux deux definitions ou le jeu se joue.
C'est ce que ce script monte, a partir de la SEULE source disponible avant que la filiere d'assets
existe : la planche de reference d'Arenarea.

Ce que la maquette vaut, et ce qu'elle ne vaut pas
--------------------------------------------------
La planche de reference dessine un losange de sol de ~136 px de large. Le standard en demande 256 :
les pieces sont donc AGRANDIES d'un facteur 1,88 pour etre montees. La maquette juge donc la
composition, l'emprise des pieces, la lisibilite d'une case a 100 px d'ecran -- elle NE juge PAS la
finesse du trait a 2160p, ou elle montre la limite de la planche et non celle du standard. Le
verdict sur la finesse se prend sur un master de production (1254 px reduit a 256, facteur 0,20),
ce que la planche de contact `-temoin` montre cote a cote.

Ce qu'il ecrit, sous `Planning/versions/v0.1.0/v0.0.1-demo/maquettes/` :

- `maquette-2d-hd-1080.png` : la scene entiere en 1920 x 1080, a 100 px par case ;
- `maquette-2d-hd-2160.png` : la meme en 3840 x 2160, a 200 px par case ;
- `maquette-2d-hd-reperes.png` : la meme scene annotee -- grille des cases, cellule de figurine
  192 x 256 et sa jauge de 170 px, echelle. C'est la planche de LECTURE ; les deux premieres sont
  les references de non-regression du rendu du LOT-103, et ne portent aucun texte.

Usage :
    python scripts/build_hd_mockup.py            # monte la maquette
    python scripts/build_hd_mockup.py --check    # verifie que les images sont a jour

Dependances : Pillow et numpy (outil de production, pas de CI).
"""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
PLANCHE = ROOT / "Tools" / "AssetsHD" / "Arenarea" / "arenarea-planche-reference-v2.png"
MAQUETTES = ROOT / "Planning" / "versions" / "v0.1.0" / "v0.0.1-demo" / "maquettes"
FONTS = ROOT / "Source" / "Elements" / "Assets" / "Fonts"

# --- Le standard -------------------------------------------------------------------------------
TILE_W, TILE_H = 256, 159            # le losange de sol (rapport 0,62)
FIGURE_CELL = (192, 256)             # la cellule d'une figurine humanoide
FIGURE_HEIGHT = 170                  # sa hauteur d'art
FIGURE_GROUND_Y = 252                # la ligne de sol dans la cellule

# --- La planche de reference -------------------------------------------------------------------
# Le fond des cartes de la planche, mesure : un bleu-vert tres sombre.
SHEET_BG = np.array([20, 29, 31], dtype=float)
# Les deux losanges de sol, par centre, demi-diagonales et retrait (le dessus de la dalle, sans son
# epaisseur). CONSTAT DE LA MAQUETTE : les deux sols de la planche sont des PANNEAUX bordes de rouge,
# pas des dalles repetables -- les repeter donne un treillis de bordures qui n'existe nulle part dans
# la scene de reference. La dalle unie se prend donc dans l'INTERIEUR du pavage (retrait 0,52), etire
# sur le losange entier. Le standard en tire une regle : la famille 01 doit livrer une dalle de fond
# repetable avant ses panneaux.
SOURCE_FLOORS = {
    "floor-panel": (91.5, 605.0, 68.0, 43.0, 1.0),
    "floor-plain": (221.0, 608.0, 68.0, 43.0, 0.52),
}
# Le losange de sol de la planche fait 136 px de large : tout le reste s'agrandit d'autant.
SHEET_TILE_W = 136.0
SCALE = TILE_W / SHEET_TILE_W

# Les pieces de decor : boite sur la planche, emprise en cases, et le decalage (en pixels d'art,
# apres agrandissement) qui pose la piece sur le coin sud de son emprise. Mesure a l'oeil sur le
# rendu, une fois : une piece de la planche flotte dans son panneau, elle n'a pas d'ancre.
SOURCE_PIECES = {
    "wall-ivy-window":  {"box": (322, 535, 496, 688), "span": (2, 1), "offset": (18, 24)},
    "wall-window":      {"box": (509, 539, 595, 686), "span": (1, 1), "offset": (0, 24)},
    "colonnade":        {"box": (732, 535, 879, 688), "span": (3, 1), "offset": (10, 30)},
    "fountain":         {"box": (24, 754, 259, 964), "span": (3, 3), "offset": (0, 6)},
    "lamppost":         {"box": (729, 754, 776, 943), "span": (1, 1), "offset": (0, 10)},
    "bench":            {"box": (619, 830, 716, 913), "span": (1, 1), "offset": (0, 6)},
}

# --- La scene : huit cases sur huit --------------------------------------------------------------
GRID = 8
# (piece, colonne, rangee) -- colonne et rangee du coin nord de l'emprise.
SCENE = [
    ("colonnade", 0, 0),
    ("wall-ivy-window", 4, 0),
    ("wall-window", 6, 0),
    ("fountain", 2, 2),
    ("lamppost", 6, 3),
    ("bench", 1, 5),
]
# La figurine manquante : aucune figurine HD n'existe avant le LOT-112. La planche de reperes
# montre sa cellule a cette case, pour que l'echelle se juge quand meme.
FIGURE_CELL_AT = (6, 5)
SCENE_BG = (23, 30, 33)

# Les deux definitions ou le jeu se joue, et la taille d'une case a l'ecran dans chacune. Les deux
# vues couvrent EXACTEMENT la meme etendue de monde -- 1920/100 = 3840/200 = 19,2 losanges de large,
# 17,4 de haut : un ecran plus fin ne montre pas plus de jeu, il montre le meme jeu plus finement.
VIEWS = {
    "1080": {"size": (1920, 1080), "tile_on_screen": 100},
    "2160": {"size": (3840, 2160), "tile_on_screen": 200, "window": (1920, 1080)},
}
# Une vue peut se deposer par une FENETRE plutot qu'en entier : un cadre 2160p pese 11 Mio, au-dela
# de la limite de 5 Mio du depot, et personne ne le regarde a sa taille. La fenetre est un extrait
# au pixel pres, pris au centre : elle juge la finesse, qui est la seule question que 2160p pose.
# Le centre de la vue : le milieu de la place.
VIEW_CENTER = (3.5, 3.5)


# --- Detourage ------------------------------------------------------------------------------------


def key_on_dark(rgb: np.ndarray, soft: float = 16.0, hard: float = 55.0) -> tuple[np.ndarray, np.ndarray]:
    """Detoure une piece du fond sombre de la planche : alpha continu, couleur redressee.

    Le fond est uniforme et tres sombre, la piece claire : l'alpha se lit dans la distance au fond.
    Sur le bord, le pixel est un melange `C = a.F + (1-a).B` -- on en retire le fond pour retrouver
    la couleur propre `F`, faute de quoi tout le contour tire vers le vert sombre de la planche.
    """
    distance = np.abs(rgb - SHEET_BG).sum(axis=2)
    alpha = np.clip((distance - soft) / (hard - soft), 0.0, 1.0)
    safe = np.maximum(alpha, 1e-3)[..., None]
    front = (rgb - (1.0 - alpha)[..., None] * SHEET_BG) / safe
    return np.clip(front, 0, 255), alpha


def cut(sheet: np.ndarray, box: tuple[int, int, int, int]) -> Image.Image:
    """Une piece de la planche, detouree et agrandie a l'echelle du standard."""
    x0, y0, x1, y1 = box
    front, alpha = key_on_dark(sheet[y0:y1, x0:x1])
    rgba = np.dstack([front, alpha * 255.0]).astype(np.uint8)
    piece = Image.fromarray(rgba, "RGBA")
    return piece.resize((round(piece.width * SCALE), round(piece.height * SCALE)), Image.LANCZOS)


def cut_floor(sheet: np.ndarray, center: tuple[float, float, float, float, float]) -> Image.Image:
    """Le dessus d'une dalle, ramene au losange du standard.

    On ne decoupe pas la boite de la dalle -- elle porte son epaisseur et la pointe de sa voisine --
    mais le losange lui-meme, par un masque. Le resultat est mis exactement a 256 x 159 : c'est la
    tuile que le moteur etire sur le losange projete. Un retrait inferieur a 1 ne garde que le coeur
    de la dalle, ce qui en fait un fond sans bordure.
    """
    cx, cy, half_w, half_h, inset = center
    half_w, half_h = half_w * inset, half_h * inset
    x0, y0 = int(cx - half_w) - 1, int(cy - half_h) - 1
    x1, y1 = int(cx + half_w) + 2, int(cy + half_h) + 2
    front, _ = key_on_dark(sheet[y0:y1, x0:x1])
    piece = Image.fromarray(front.astype(np.uint8), "RGB").convert("RGBA")
    mask = Image.new("L", piece.size, 0)
    ImageDraw.Draw(mask).polygon(
        [(cx - x0, cy - y0 - half_h), (cx - x0 + half_w, cy - y0),
         (cx - x0, cy - y0 + half_h), (cx - x0 - half_w, cy - y0)],
        fill=255,
    )
    piece.putalpha(mask)
    return piece.resize((TILE_W + 2, TILE_H + 2), Image.LANCZOS)


# --- Montage ---------------------------------------------------------------------------------------


def tile_center(column: int, row: int) -> tuple[float, float]:
    """Le centre d'une case, en pixels d'art, dans le repere de la scene (origine : case 0,0)."""
    return ((column - row) * TILE_W / 2.0, (column + row) * TILE_H / 2.0)


def art_extent() -> tuple[float, float, float, float]:
    """L'etendue d'art que les deux vues couvrent, en pixels d'art, autour du centre de la place.

    Les deux definitions cadrent la meme etendue : une seule image d'art les sert toutes les deux,
    et c'est elle que le rendu du LOT-103 doit reproduire.
    """
    width = max(spec["size"][0] * TILE_W / spec["tile_on_screen"] for spec in VIEWS.values())
    height = max(spec["size"][1] * TILE_W / spec["tile_on_screen"] for spec in VIEWS.values())
    cx, cy = tile_center(*VIEW_CENTER)
    return cx - width / 2, cy - height / 2, cx + width / 2, cy + height / 2


def tiles_in(extent: tuple[float, float, float, float]) -> list[tuple[int, int]]:
    """Les cases dont le losange touche l'etendue : le sol remplit le cadre, il ne flotte pas."""
    x0, y0, x1, y1 = extent
    out = []
    reach = int((x1 - x0) / TILE_W + (y1 - y0) / TILE_H) + 4
    for column in range(-reach, GRID + reach):
        for row in range(-reach, GRID + reach):
            cx, cy = tile_center(column, row)
            if x0 - TILE_W < cx < x1 + TILE_W and y0 - TILE_H < cy < y1 + TILE_H:
                out.append((column, row))
    return out


def depth(column: int, row: int, span: tuple[int, int]) -> int:
    """Cle du peintre : le coin sud de l'emprise, celui qui est le plus pres de l'oeil."""
    return (column + span[0] - 1) + (row + span[1] - 1)


def compose(sheet: np.ndarray) -> tuple[Image.Image, tuple[float, float]]:
    """La scene entiere en pixels d'art, et l'origine (le centre de la case 0,0) dans l'image."""
    floors = {name: cut_floor(sheet, box) for name, box in SOURCE_FLOORS.items()}
    pieces = {name: cut(sheet, spec["box"]) for name, spec in SOURCE_PIECES.items()}

    extent = art_extent()
    x0, y0, x1, y1 = extent
    canvas = Image.new("RGBA", (round(x1 - x0), round(y1 - y0)), SCENE_BG + (255,))
    origin = (-x0, -y0)

    def place(image: Image.Image, cx: float, cy: float) -> None:
        canvas.alpha_composite(image, (round(origin[0] + cx), round(origin[1] + cy)))

    # Le sol : le pavage uni partout, le motif en pourtour de la place de huit cases sur huit.
    # Les dalles se recouvrent d'un pixel : deux losanges jointifs au pixel pres laissent passer
    # le fond sur la diagonale, et cette couture se voit plus que le recouvrement.
    for column, row in tiles_in(extent):
        inside = 0 <= column < GRID and 0 <= row < GRID
        edge = inside and (column in (0, GRID - 1) or row in (0, GRID - 1))
        cx, cy = tile_center(column, row)
        place(floors["floor-panel" if edge else "floor-plain"], cx - TILE_W / 2 - 1, cy - TILE_H / 2 - 1)

    for name, column, row in sorted(SCENE, key=lambda item: depth(item[1], item[2], SOURCE_PIECES[item[0]]["span"])):
        spec = SOURCE_PIECES[name]
        span = spec["span"]
        image = pieces[name]
        # Le coin sud de l'emprise : la piece y pose son bas, centree sur la largeur de l'emprise.
        south_x, south_y = tile_center(column + span[0] - 1, row + span[1] - 1)
        south_y += TILE_H / 2
        place(image, south_x - image.width / 2 + spec["offset"][0], south_y - image.height + spec["offset"][1])

    return canvas, origin


# --- Les vues --------------------------------------------------------------------------------------


def view(art: Image.Image, name: str) -> Image.Image:
    """La scene a une definition d'ecran : l'art couvre le cadre, il n'y a qu'a le reduire."""
    spec = VIEWS[name]
    frame = art.convert("RGB").resize(spec["size"], Image.LANCZOS)
    window = spec.get("window")
    if window is None:
        return frame
    left, top = (frame.width - window[0]) // 2, (frame.height - window[1]) // 2
    return frame.crop((left, top, left + window[0], top + window[1]))


def label_font(size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(str(FONTS / "Cinzel-Regular.ttf"), size)


def landmarks(art: Image.Image, origin: tuple[float, float]) -> Image.Image:
    """La planche de lecture : la scene a 1080p, plus la grille, la cellule de figurine et l'echelle."""
    spec = VIEWS["1080"]
    frame = view(art, "1080").convert("RGBA")
    zoom = spec["size"][0] / art.width

    def to_screen(column: float, row: float) -> tuple[float, float]:
        px, py = tile_center(column, row)
        return ((origin[0] + px) * zoom, (origin[1] + py) * zoom)

    overlay = Image.new("RGBA", frame.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    for row in range(GRID):
        for column in range(GRID):
            px, py = to_screen(column, row)
            half_w, half_h = TILE_W * zoom / 2, TILE_H * zoom / 2
            draw.polygon([(px, py - half_h), (px + half_w, py), (px, py + half_h), (px - half_w, py)],
                         outline=(28, 22, 16, 110))

    # La cellule d'une figurine, a sa case : ce que le standard reserve a un humanoide. Elle se pose
    # par sa ligne de sol (y = 252 dans la cellule), qui tombe au centre du losange.
    column, row = FIGURE_CELL_AT
    px, py = to_screen(column, row)
    cell_w, cell_h = FIGURE_CELL[0] * zoom, FIGURE_CELL[1] * zoom
    cell_top = py - FIGURE_GROUND_Y * zoom
    draw.rectangle([(px - cell_w / 2, cell_top), (px + cell_w / 2, cell_top + cell_h)],
                   outline=(38, 26, 14, 235), width=3)
    draw.rectangle([(px - 14, py - FIGURE_HEIGHT * zoom), (px + 14, py)],
                   fill=(142, 35, 53, 180), outline=(38, 26, 14, 235), width=2)
    draw.line([(px - cell_w / 2 - 16, py), (px + cell_w / 2 + 16, py)], fill=(201, 164, 92, 200))
    plate(draw, (px + cell_w / 2 + 12, py - FIGURE_HEIGHT * zoom - 6), [
        ("figurine 170 px, cellule 192 x 256", 16, (239, 230, 210)),
        ("aucune figurine HD avant le LOT-112", 16, (163, 155, 144)),
    ])

    plate(draw, (24, 22), [
        ("Maquette du standard 2D HD - 8 x 8 cases d'Arenarea", 24, (239, 230, 210)),
        ("Une case : 256 x 159 px d'art, affichee a 100 px (1080p) et 200 px (2160p).", 17, (206, 196, 176)),
        ("Les deux vues couvrent la meme etendue : 19,2 losanges de large, 17,4 de haut.", 17, (206, 196, 176)),
        ("Pieces tirees de la planche de reference (losange de 136 px) : agrandies x 1,88.", 17, (206, 196, 176)),
        ("Elle juge la composition et la lisibilite, pas la finesse du trait a 2160p.", 17, (206, 196, 176)),
    ])
    return Image.alpha_composite(frame, overlay).convert("RGB")


def plate(draw: ImageDraw.ImageDraw, at: tuple[float, float],
          lines: list[tuple[str, int, tuple[int, int, int]]]) -> None:
    """Un cartouche sombre puis son texte : sur un pavage ivoire, un texte clair seul disparait."""
    x, y = at
    fonts = [label_font(size) for _, size, _ in lines]
    heights = [size + 8 for _, size, _ in lines]
    width = max(draw.textlength(text, font=font) for (text, _, _), font in zip(lines, fonts))
    draw.rectangle([(x - 12, y - 10), (x + width + 16, y + sum(heights) + 6)], fill=(18, 23, 26, 214))
    for (text, _, colour), font, height in zip(lines, fonts, heights):
        draw.text((x, y), text, font=font, fill=colour + (255,))
        y += height


# --- Commande --------------------------------------------------------------------------------------


def outputs() -> dict[Path, Image.Image]:
    if not PLANCHE.exists():
        raise SystemExit(f"planche de reference absente : {PLANCHE}")
    sheet = np.asarray(Image.open(PLANCHE).convert("RGB")).astype(float)
    art, origin = compose(sheet)
    images = {MAQUETTES / f"maquette-2d-hd-{name}.png": view(art, name) for name in VIEWS}
    images[MAQUETTES / "maquette-2d-hd-reperes.png"] = landmarks(art, origin)
    return images


def digest(image: Image.Image) -> str:
    return hashlib.sha256(image.tobytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="verifie que les images sont a jour")
    args = parser.parse_args()

    images = outputs()
    if args.check:
        stale = []
        for path, image in images.items():
            if not path.exists():
                stale.append(f"{path.relative_to(ROOT)} : absente")
            elif digest(Image.open(path).convert(image.mode)) != digest(image):
                stale.append(f"{path.relative_to(ROOT)} : differe du montage")
        for line in stale:
            print(line, file=sys.stderr)
        if stale:
            print("relancer : python scripts/build_hd_mockup.py", file=sys.stderr)
            return 1
        print(f"{len(images)} image(s) a jour")
        return 0

    MAQUETTES.mkdir(parents=True, exist_ok=True)
    for path, image in images.items():
        image.save(path, optimize=True)
        print(f"{path.relative_to(ROOT)}  {image.width} x {image.height}  {path.stat().st_size / 1024:.0f} Kio")
    return 0


if __name__ == "__main__":
    sys.exit(main())
