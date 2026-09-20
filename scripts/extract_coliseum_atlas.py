#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Découpe de la planche de production du Colisée (LOT-50, habillage de l'arène).

La direction artistique de l'arène est une PLANCHE : `production_source_atlas.png` (1536 × 1024),
sortie d'un générateur d'images sur la maquette 01 du HUD, qui montre sur un fond bleu nuit tout
ce que le Colisée tactique emploie -- tuiles isométriques, tileset du Colisée, figurines des quatre
héros (idle, marche, attaque, touché, mort) et des quatre gladiateurs, structures, détails,
props, rochers, végétation, effets. Ce script en tire les pièces UNE PAR UNE, et c'est lui qui
fait foi : la planche est la source, `Source/Elements/Assets/Coliseum/` en est le produit, et
relancer le script reproduit le dossier à l'identique.

Comment il découpe : le fond bleu nuit est détouré par distance de couleur (alpha doux sur les
contours) ; dans chaque section de la planche, les pièces sont trouvées soit par une grille
(profils du masque en lignes puis en colonnes, coupés à leurs vallées -- ce qui sépare deux
losanges qui se touchent par la pointe ou deux figurines jointes par une arme), soit par
composantes connexes pour les sections irrégulières. Les légendes sont exclues par zone.

Ce qu'il produit, sous `Source/Elements/Assets/Coliseum/` :

- `terrain/<nom>.png`, `coliseum/<nn>.png` : les losanges de sol, tels quels ;
- `structures/`, `details/`, `props/`, `rocks/`, `vegetation/`, `effects/` : les pièces de décor ;
- `characters/<héros>/<animation>.png` et `enemies/<gladiateur>/idle.png` : des BANDES
  d'animation (une image par colonne, canevas commun 48 × 64, ancre au pied centre bas), ce que
  `AnimatedSprite` lit sans manifeste ; cinq colonnes par héros, huit par gladiateur ;
- `manifest.json` : chaque fichier, sa boîte sur la planche et sa taille.

Usage :
    python scripts/extract_coliseum_atlas.py            # decoupe et ecrit le dossier
    python scripts/extract_coliseum_atlas.py --check    # verifie que le dossier est a jour

Dépendances : Pillow et numpy (outil de production, pas de CI).
"""

from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
COLISEUM = ROOT / "Source" / "Elements" / "Assets" / "Coliseum"
ATLAS = COLISEUM / "production_source_atlas.png"
MANIFEST = COLISEUM / "manifest.json"

# Canevas commun des bandes d'animation : la plus grande figurine de la planche (le rétiaire et
# son filet) tient dans 48 × 64, et un seul format permet a l'ecran de lire toute bande sans
# consulter le manifeste.
FRAME = (48, 64)
HERO_FRAMES = 5
ENEMY_FRAMES = 8

# --- Les sections de la planche --------------------------------------------------------------
# box : (x0, y0, x1, y1) sur la planche ; exclude : legendes ; grid : profil (ratio = seuil de
# vallee relatif) ; sans grid, composantes connexes soudees a `merge` px.
SECTIONS = {
    "terrain": {"box": (5, 395, 440, 640), "min_w": 40, "min_h": 20, "soft": 18, "hard": 48,
                "grid": {"ratio": 0.4, "gap_x": 3, "gap_y": 2}, "exclude": [(5, 470, 440, 505), (5, 600, 440, 640)]},
    "coliseum": {"box": (1065, 925, 1536, 1010), "min_w": 20, "min_h": 10, "soft": 18, "hard": 48,
                 "grid": {"ratio": 0.4, "gap_x": 2, "gap_y": 2}},
    "characters": {"box": (390, 60, 843, 355), "min_w": 7, "min_h": 25, "soft": 8, "hard": 30,
                   "grid": {"ratio": 0.3}},
    "enemies": {"box": (845, 60, 1145, 355), "min_w": 10, "min_h": 25, "soft": 8, "hard": 30,
                "grid": {"ratio": 0.3, "split": False}, "exclude": [(845, 200, 1145, 240), (845, 60, 1145, 70)]},
    "structures": {"box": (450, 395, 925, 720), "min_w": 12, "min_h": 20, "soft": 18, "hard": 48,
                   "grid": {"ratio": 0.12, "gap_x": 1, "gap_y": 1, "split": False},
                   "exclude": [(450, 480, 925, 500), (450, 590, 925, 610), (450, 690, 925, 720)]},
    "details": {"box": (935, 395, 1210, 720), "min_w": 12, "min_h": 15, "soft": 18, "hard": 48,
                "grid": {"ratio": 0.12, "gap_x": 1, "gap_y": 1, "split": False},
                "exclude": [(935, 480, 1210, 500), (935, 590, 1210, 610), (935, 690, 1210, 720)]},
    "props": {"box": (5, 725, 440, 985), "min_w": 10, "min_h": 10, "soft": 18, "hard": 48, "merge": 3},
    "rocks": {"box": (450, 755, 590, 985), "min_w": 15, "min_h": 15, "soft": 18, "hard": 48, "merge": 4},
    "vegetation": {"box": (600, 755, 815, 985), "min_w": 10, "min_h": 10, "soft": 18, "hard": 48, "merge": 4},
    "effects": {"box": (830, 755, 1055, 985), "min_w": 12, "min_h": 12, "soft": 18, "hard": 48, "merge": 4,
                "exclude": [(830, 800, 1055, 822), (830, 880, 1055, 900), (830, 975, 1055, 985)]},
}

# Les noms, dans l'ordre de lecture (lignes puis colonnes) de chaque section.
TERRAIN = ["sand", "sand_blood", "stone", "worn_stone", "marble",
           "dry_grass", "dirt", "rocky_ground", "water", "stone_bridge"]
STRUCTURES = ["wall_banner", "banner_03", "wall", "column_medium", "column_small", "column_large", "stairs",
              "stands", "gate", "banner_01", "banner_02", "banner_04", "statue_01", "statue_02", "torch_01",
              "torch_02", "cage_02", "cage_01", "arch", "main_entrance", "balcony", "loge_01", "loge_02"]
DETAILS = ["tent", "seats", "banner_01", "banner_02", "stage", "cage", "torch_01", "torch_02",
           "shield_round", "shield_01", "spears", "spear", "banner_large", "banner_03", "banner_04"]
HEROES = [("kaelith_voss", 5), ("bram", 4), ("elira", 4), ("darin", 4)]
ANIMATIONS = ["idle", "walk", "attack", "hit", "death"]
GLADIATORS = ["gladiator_sword_shield", "gladiator_lance", "retiarius", "archer"]


# --- Detourage et decoupe --------------------------------------------------------------------


def key_alpha(rgb: np.ndarray, soft: int, hard: int) -> np.ndarray:
    """Alpha par distance au fond bleu nuit : une piece est plus claire ou plus chaude que lui."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    d = np.maximum.reduce([r - 22, g - 30, (b - 42) * 0.6, (r - b) * 1.5])
    return np.clip((d - soft) / (hard - soft), 0, 1)


def runs(profile: np.ndarray, gap_max: float, min_len: int) -> list[tuple[int, int]]:
    out: list[tuple[int, int]] = []
    start = None
    for i, v in enumerate(list(profile) + [0]):
        if v > gap_max and start is None:
            start = i
        elif v <= gap_max and start is not None:
            if i - start >= min_len:
                out.append((start, i))
            start = None
    return out


def valley(profile: np.ndarray, gap: float, ratio: float) -> float:
    nz = profile[profile > 0]
    return max(gap, ratio * np.percentile(nz, 60) if len(nz) else 0)


def grid_boxes(mask: np.ndarray, grid: dict, min_w: int, min_h: int) -> list[tuple[int, int, int, int]]:
    ratio = grid.get("ratio", 0.0)
    boxes = []
    py = mask.sum(axis=1)
    for ry0, ry1 in runs(py, valley(py, grid.get("gap_y", 0), ratio), min_h):
        band = mask[ry0:ry1]
        px = band.sum(axis=0)
        cols = runs(px, valley(px, grid.get("gap_x", 0), ratio), min_w)
        if grid.get("split", True) and len(cols) >= 3:
            # Deux pieces soudees : une boite bien plus large que la mediane est recoupee a la
            # vallee la plus basse de son tiers central.
            median = sorted(c1 - c0 for c0, c1 in cols)[len(cols) // 2]
            done: list[tuple[int, int]] = []
            stack = list(cols)
            while stack:
                c0, c1 = stack.pop(0)
                if c1 - c0 > 1.6 * median:
                    a, b = c0 + (c1 - c0) // 3, c1 - (c1 - c0) // 3
                    cut = a + int(np.argmin(px[a:b]))
                    stack = [(c0, cut), (cut, c1)] + stack
                else:
                    done.append((c0, c1))
            cols = done
        for cx0, cx1 in cols:
            yy, xx = np.nonzero(band[:, cx0:cx1])
            if len(yy):
                boxes.append((cx0 + xx.min(), ry0 + yy.min(), cx0 + xx.max() + 1, ry0 + yy.max() + 1))
    return boxes


def component_boxes(mask: np.ndarray, merge: int, min_w: int, min_h: int) -> list[tuple[int, int, int, int]]:
    from collections import deque

    h, w = mask.shape
    dil = mask.copy()
    for _ in range(merge):
        p = np.pad(dil, 1)
        dil = (p[:-2, 1:-1] | p[2:, 1:-1] | p[1:-1, :-2] | p[1:-1, 2:]
               | p[:-2, :-2] | p[:-2, 2:] | p[2:, :-2] | p[2:, 2:] | dil)
    labels = np.zeros((h, w), np.int32)
    boxes = []
    n = 0
    for y, x in zip(*np.nonzero(dil)):
        if labels[y, x]:
            continue
        n += 1
        q = deque([(y, x)])
        labels[y, x] = n
        x0, y0, x1, y1 = x, y, x, y
        while q:
            cy, cx = q.popleft()
            for ny in (cy - 1, cy, cy + 1):
                if ny < 0 or ny >= h:
                    continue
                for nx in (cx - 1, cx, cx + 1):
                    if nx < 0 or nx >= w or labels[ny, nx] or not dil[ny, nx]:
                        continue
                    labels[ny, nx] = n
                    q.append((ny, nx))
            x0, x1, y0, y1 = min(x0, cx), max(x1, cx), min(y0, cy), max(y1, cy)
        sub = mask[y0:y1 + 1, x0:x1 + 1]
        if sub.any():
            yy, xx = np.nonzero(sub)
            box = (x0 + xx.min(), y0 + yy.min(), x0 + xx.max() + 1, y0 + yy.max() + 1)
            if box[2] - box[0] >= min_w and box[3] - box[1] >= min_h:
                boxes.append(box)
    boxes.sort(key=lambda b: (b[1], b[0]))
    return boxes


def cut_section(src: np.ndarray, spec: dict) -> list[tuple[Image.Image, tuple[int, int, int, int]]]:
    x0, y0, x1, y1 = spec["box"]
    rgb = src[y0:y1, x0:x1]
    alpha = key_alpha(rgb, spec["soft"], spec["hard"])
    mask = alpha > 0.5
    for ex0, ey0, ex1, ey1 in spec.get("exclude", ()):
        mask[max(0, ey0 - y0):max(0, ey1 - y0), max(0, ex0 - x0):max(0, ex1 - x0)] = False
    if "grid" in spec:
        boxes = grid_boxes(mask, spec["grid"], spec["min_w"], spec["min_h"])
    else:
        boxes = component_boxes(mask, spec["merge"], spec["min_w"], spec["min_h"])
    pieces = []
    for bx0, by0, bx1, by1 in boxes:
        cx0, cy0 = max(0, bx0 - 1), max(0, by0 - 1)
        cx1, cy1 = min(x1 - x0, bx1 + 1), min(y1 - y0, by1 + 1)
        crop = np.dstack([rgb[cy0:cy1, cx0:cx1].astype(np.uint8), (alpha[cy0:cy1, cx0:cx1] * 255).astype(np.uint8)])
        pieces.append((Image.fromarray(crop, "RGBA"), (int(x0 + cx0), int(y0 + cy0), int(x0 + cx1), int(y0 + cy1))))
    return pieces


def strip(frames: list[Image.Image], count: int) -> Image.Image:
    """Une bande de `count` images sur le canevas commun, ancrees au pied centre bas ; une serie
    plus courte repete sa derniere image, pour que toute bande d'un meme camp ait la meme longueur."""
    fw, fh = FRAME
    sheet = Image.new("RGBA", (fw * count, fh), (0, 0, 0, 0))
    for i in range(count):
        frame = frames[min(i, len(frames) - 1)]
        if frame.width > fw or frame.height > fh:
            raise SystemExit(f"une image {frame.size} deborde du canevas {FRAME}")
        sheet.paste(frame, (i * fw + (fw - frame.width) // 2, fh - frame.height), frame)
    return sheet


# --- Production ------------------------------------------------------------------------------


def produce() -> dict:
    src = np.asarray(Image.open(ATLAS).convert("RGB")).astype(np.int16)
    files: dict[str, dict] = {}

    def write(relative: str, image: Image.Image, boxes: list) -> None:
        path = COLISEUM / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        image.save(path)
        files[relative] = {"size": list(image.size), "atlas": boxes}

    cuts = {name: cut_section(src, spec) for name, spec in SECTIONS.items()}

    expected = {"terrain": len(TERRAIN), "coliseum": 16, "characters": len(ANIMATIONS) * sum(n for _, n in HEROES),
                "enemies": len(GLADIATORS) * ENEMY_FRAMES, "structures": len(STRUCTURES), "details": len(DETAILS)}
    for name, count in expected.items():
        if len(cuts[name]) != count:
            raise SystemExit(f"{name} : {len(cuts[name])} pieces trouvees, {count} attendues -- la planche ou "
                             f"les sections ont change, relire SECTIONS.")

    for (image, box), name in zip(cuts["terrain"], TERRAIN):
        write(f"terrain/{name}.png", image, [box])
    for i, (image, box) in enumerate(cuts["coliseum"], 1):
        write(f"coliseum/{i:02d}.png", image, [box])
    for (image, box), name in zip(cuts["structures"], STRUCTURES):
        write(f"structures/{name}.png", image, [box])
    for (image, box), name in zip(cuts["details"], DETAILS):
        write(f"details/{name}.png", image, [box])
    for section in ("props", "rocks", "vegetation", "effects"):
        for i, (image, box) in enumerate(cuts[section], 1):
            write(f"{section}/{i:02d}.png", image, [box])

    # Heros : une ligne de la planche par animation, les heros cote a cote (5 + 4 + 4 + 4 images).
    per_row = sum(n for _, n in HEROES)
    for a, animation in enumerate(ANIMATIONS):
        row = cuts["characters"][a * per_row:(a + 1) * per_row]
        offset = 0
        for hero, n in HEROES:
            frames = row[offset:offset + n]
            offset += n
            write(f"characters/{hero}/{animation}.png", strip([f for f, _ in frames], HERO_FRAMES), [b for _, b in frames])

    # Gladiateurs : deux blocs de deux, chacun sur deux lignes de quatre images.
    rows = [cuts["enemies"][i * 8:(i + 1) * 8] for i in range(4)]
    for g, gladiator in enumerate(GLADIATORS):
        block_rows = rows[0:2] if g < 2 else rows[2:4]
        side = 0 if g % 2 == 0 else 4
        frames = block_rows[0][side:side + 4] + block_rows[1][side:side + 4]
        write(f"enemies/{gladiator}/idle.png", strip([f for f, _ in frames], ENEMY_FRAMES), [b for _, b in frames])

    manifest = {
        "source": ATLAS.name,
        "sourceSha256": hashlib.sha256(ATLAS.read_bytes()).hexdigest(),
        "frame": list(FRAME),
        "heroFrames": HERO_FRAMES,
        "enemyFrames": ENEMY_FRAMES,
        "animations": ANIMATIONS,
        "heroes": [h for h, _ in HEROES],
        "gladiators": GLADIATORS,
        "files": dict(sorted(files.items())),
    }
    return manifest


def main() -> int:
    if not ATLAS.is_file():
        print(f"extract_coliseum_atlas : planche absente, {ATLAS}", file=sys.stderr)
        return 1
    manifest = produce()
    text = json.dumps(manifest, indent=2, ensure_ascii=False) + "\n"
    if "--check" in sys.argv[1:]:
        if not MANIFEST.is_file() or MANIFEST.read_text(encoding="utf-8") != text:
            print("extract_coliseum_atlas : le dossier Coliseum n'est pas a jour de la planche.", file=sys.stderr)
            return 1
        print("extract_coliseum_atlas : a jour.")
        return 0
    MANIFEST.write_text(text, encoding="utf-8", newline="\n")
    print(f"extract_coliseum_atlas : {len(manifest['files'])} fichiers ecrits sous {COLISEUM.relative_to(ROOT)}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
