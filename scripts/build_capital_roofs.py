#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""La toiture de la Capitale (LOT-129) : quatre matières peintes à plat, projetées case par case.

Les toits sont à deux pans, pignon de pierre au bout visible (choix de l'auteur, 23 septembre 2026),
sur un îlot de `D` cases de profondeur (2 à 5) : le faîtage se calcule au milieu. La méthode est
celle de la V4 du LOT-105 (`build_capital_v4.py`) : aucune pièce n'est dessinée par le générateur ;
il peint les tuiles (surface), le faîtage, la rive d'égout et la corniche de pignon (élévations), et
ce script les projette sur la géométrie exacte d'un toit. La pierre du pignon est la surface des murs
de la V4, pour que le pignon prolonge le mur.

Une pièce est la part du toit au-dessus d'**une** case : le moteur la pose sur la couche d'étage
au-dessus des murs, et la trie avec sa case (`LOT-129`). Les murs du kit sont centrés dans leur case,
d'épaisseur 0,18 ; le toit déborde de leur face de `EAVE` à l'égout et de `VERGE` à la rive.

    python scripts/build_capital_roofs.py --provisional   # matières provisoires : la géométrie seule
    python scripts/build_capital_roofs.py                 # les matières de Toitures/Sources/
    python scripts/build_capital_roofs.py --install       # et l'installation dans le kit

Sans ses quatre sources, le script refuse, sauf `--provisional`, qui peint des matières de
remplacement pour juger la géométrie dans le moteur — et n'installe jamais.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import sys
from dataclasses import replace
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

import install_hd_asset as hd

ROOT = Path(__file__).resolve().parent.parent
CAP = ROOT / "Tools/AssetsHD/Regions/central-empire/capital/Common"
ROOFS = CAP / "Toitures"
SOURCES = ROOFS / "Sources"
CALIBRES = ROOFS / "Calibres"
STAGE = ROOFS / "Scene"
WALL_SURFACE = CAP / "V4/Sources/wall-surface.png"

S = 2  # suréchantillonnage du rendu ; l'installation réduit de moitié.
HALF = (128.0, 79.5)  # une case le long d'un axe, en pixels d'art (losange de 256 × 159).
DEPTHS = range(2, 6)
WALL_HALF = 0.09  # demi-épaisseur des murs de la V4.
EAVE = 0.18  # débord de l'égout au-delà de la face du mur, en cases.
VERGE = 0.12  # débord de la rive au-delà du pignon.
PITCH = 58.0  # montée du toit, en pixels d'art par case de portée (≈ 21°).
RIDGE_HEIGHT = 20.0
EAVE_HEIGHT = 16.0
CORNICE_HEIGHT = 14.0
THICKNESS = 9.0  # l'épaisseur du toit vue à la rive.
LIGHT = np.array([-0.45, -0.65, 0.70]) / np.linalg.norm([-0.45, -0.65, 0.70])
INFINITY = 1e9
# Deux cases voisines se chevauchent d'autant : la même phase de texture, donc sans trace, et plus
# de fil clair au joint une fois les pièces réduites et lissées.
OVERLAP = 0.03


# --- Les matières ----------------------------------------------------------------------------------


def load(name: str) -> Image.Image:
    image = Image.open(SOURCES / f"{name}.png").convert("RGBA")
    if name != "roof-tiles":
        # Les élévations générées gardent une marge transparente verticale.
        # La hauteur géométrique décrit la bande, pas son canevas.
        # Ignorer les pixels presque invisibles laissés loin de la bande ;
        # conserver l'alpha original et deux pixels d'anticrénelage au bord.
        box = image.getchannel("A").point(lambda a: 255 if a > 128 else 0).getbbox()
        if box is None:
            raise ValueError(f"{name} : élévation vide")
        image = image.crop((0, max(0, box[1] - 2), image.width, min(image.height, box[3] + 2)))
    return image


def provisional() -> dict[str, Image.Image]:
    """Des matières de remplacement : de quoi juger la géométrie, jamais installées."""
    tiles = Image.new("RGBA", (256, 384), (122, 30, 45, 255))
    draw = ImageDraw.Draw(tiles)
    for course in range(6):
        y = course * 64
        draw.line([(0, y + 62), (256, y + 62)], fill=(92, 22, 34, 255), width=3)
    for line in range(4):
        x = line * 64 + 32
        draw.rectangle([x - 14, 0, x - 2, 384], fill=(160, 48, 62, 255))
        draw.rectangle([x - 2, 0, x + 12, 384], fill=(108, 26, 40, 255))
    ridge = Image.new("RGBA", (256, 64), (0, 0, 0, 0))
    draw = ImageDraw.Draw(ridge)
    for k in range(4):
        draw.ellipse([k * 64 - 4, 8, k * 64 + 68, 88], fill=(150, 42, 56, 255))
    eave = Image.new("RGBA", (384, 64), (230, 220, 200, 255))
    draw = ImageDraw.Draw(eave)
    for k in range(6):
        draw.ellipse([k * 64 + 16, 0, k * 64 + 48, 36], fill=(140, 38, 52, 255))
    cornice = Image.new("RGBA", (512, 64), (236, 226, 206, 255))
    ImageDraw.Draw(cornice).rectangle([0, 0, 512, 10], fill=(122, 30, 45, 255))
    return {"roof-tiles": tiles, "roof-ridge": ridge, "roof-eave": eave, "gable-cornice": cornice}


def materials(use_provisional: bool) -> dict[str, Image.Image]:
    names = ["roof-tiles", "roof-ridge", "roof-eave", "gable-cornice"]
    missing = [n for n in names if not (SOURCES / f"{n}.png").is_file()]
    if use_provisional:
        found = provisional()
        found.update({n: load(n) for n in names if n not in missing})
    elif missing:
        raise SystemExit(f"build_capital_roofs : sources absentes de {SOURCES} : {missing} "
                         "(commande.md, ou --provisional pour la géométrie seule)")
    else:
        found = {n: load(n) for n in names}
    found["wall"] = Image.open(WALL_SURFACE).convert("RGBA").resize((512, 512), Image.Resampling.LANCZOS)
    return found


def sample(tex: Image.Image, u: np.ndarray, v: np.ndarray, wrap_v: bool = True) -> np.ndarray:
    a = np.asarray(tex)
    h, w = a.shape[:2]
    iu = np.floor(np.mod(u, 1) * w).astype(int) % w
    if wrap_v:
        iv = np.floor(np.mod(v, 1) * h).astype(int) % h
    else:
        iv = np.clip(np.floor(v * h).astype(int), 0, h - 1)
    return a[iv, iu].copy()


def shade_of(normal) -> float:
    n = np.asarray(normal, float)
    n /= np.linalg.norm(n)
    return float(np.clip(0.84 + 0.23 * np.dot(n, LIGHT), 0.66, 1.0))


# --- Le rendu --------------------------------------------------------------------------------------


class Surface:
    """Des quadrilatères du monde projetés en isométrie, avec un tampon de profondeur et une découpe.

    Le monde se compte en cases (x le long des colonnes, y le long des rangées) et en pixels d'art
    (z vers le haut) ; le sommet nord de la case de la pièce est l'origine. Seuls les points du monde
    dans la découpe `clip` (x0, y0, x1, y1) se peignent : c'est ce qui fait d'un toit une pièce par
    case, jointive avec ses voisines.
    """

    def __init__(self, clip):
        self.clip = clip
        # En pixels d'art, avant le suréchantillonnage : un toit de 5 cases de profondeur et 3 de long,
        # la case de la pièce où qu'elle soit, y tient.
        self.origin = np.array([760.0, 420.0])
        self.size = (int(1400 * S), int(1200 * S))
        self.rgba = np.zeros((self.size[1], self.size[0], 4), np.uint8)
        self.depth = np.full((self.size[1], self.size[0]), -INFINITY)

    def screen(self, p):
        p = np.asarray(p, float)
        xy = np.stack([(p[:, 0] - p[:, 1]) * HALF[0], (p[:, 0] + p[:, 1]) * HALF[1] - p[:, 2]], axis=1)
        return (xy + self.origin) * S

    def quad(self, points, tex, uv, shade=1.0, wrap_v=True, triangle=False):
        p = np.asarray(points, float)
        if not triangle and not np.allclose(p[0] + p[2], p[1] + p[3]):
            for indices in ([0, 1, 2, 2], [0, 2, 3, 3]):
                self.quad(p[indices], tex, np.asarray(uv)[indices], shade, wrap_v, True)
            return
        xy = self.screen(p)
        left, top = np.maximum(np.floor(xy.min(axis=0)).astype(int), 0)
        right, bottom = np.minimum(np.ceil(xy.max(axis=0)).astype(int), [self.size[0], self.size[1]])
        if right <= left or bottom <= top:
            return
        yy, xx = np.mgrid[top:bottom, left:right]
        basis = np.column_stack([xy[1] - xy[0], xy[3] - xy[0]])
        if abs(np.linalg.det(basis)) < 1e-9:
            return
        ab = np.linalg.inv(basis) @ np.stack([xx.ravel() + 0.5 - xy[0, 0], yy.ravel() + 0.5 - xy[0, 1]])
        a, b = ab.reshape(2, bottom - top, right - left)
        mask = (a >= -1e-8) & (a <= 1 + 1e-8) & (b >= -1e-8) & (b <= 1 + 1e-8)
        if triangle:
            mask &= a + b <= 1 + 1e-8
        world = p[0] + a[..., None] * (p[1] - p[0]) + b[..., None] * (p[3] - p[0])
        x0, y0, x1, y1 = self.clip
        mask &= (world[..., 0] >= x0) & (world[..., 0] < x1) & (world[..., 1] >= y0) & (world[..., 1] < y1)
        depth = world[..., 0] + world[..., 1] + world[..., 2] / 159.0
        uv = np.asarray(uv, float)
        t = uv[0] + a[..., None] * (uv[1] - uv[0]) + b[..., None] * (uv[3] - uv[0])
        pixels = sample(tex, t[..., 0], t[..., 1], wrap_v)
        pixels[..., :3] = np.uint8(np.clip(pixels[..., :3].astype(float) * shade, 0, 255))
        mask &= (depth >= self.depth[top:bottom, left:right] - 1e-6) & (pixels[..., 3] > 0)
        self.rgba[top:bottom, left:right][mask] = pixels[mask]
        self.depth[top:bottom, left:right][mask] = depth[mask]

    def result(self):
        return Image.fromarray(self.rgba), self.origin * S


# --- Le toit ---------------------------------------------------------------------------------------


def roof(surface: Surface, mats, depth: int, width: int, axis: str, shift) -> None:
    """Le toit entier d'un îlot de `width` cases de long et `depth` de profondeur.

    Dans le repère du toit, `a` court le long du faîtage et `b` en travers ; `axis` dit si le faîtage
    suit les colonnes (`u` : a = x) ou les rangées (`v` : a = y). `shift` ramène le sommet nord de la
    case de la pièce à l'origine.
    """
    # Les façades vues (la rangée de devant, la colonne de droite) sont au milieu de leur case ; les
    # murs droits du kit courent, eux, jusqu'au bord de leur case du côté caché. Le toit part donc du
    # bord des cases au nord et à l'ouest, et du nu des façades au sud et à l'est.
    b_near, b_far = -EAVE, depth - 0.5 + WALL_HALF + EAVE
    b_ridge = (b_near + b_far) / 2.0
    height = PITCH * (b_ridge - b_near)
    a_start, a_end = -VERGE, width - 0.5 + WALL_HALF + VERGE
    a_gable = width - 0.5 + WALL_HALF
    b_wall0, b_wall1 = 0.0, depth - 0.5 + WALL_HALF

    def z_at(b):
        return height - PITCH * abs(b - b_ridge)

    def world(a, b, z):
        x, y = (a, b) if axis == "u" else (b, a)
        return (x - shift[0], y - shift[1], z)

    def pts(*triples):
        return [world(*t) for t in triples]

    # Les normales, dans le repère du monde (z en cases : 159 px par case, comme la V4).
    def normal(na, nb, nz):
        return (na, nb, nz) if axis == "u" else (nb, na, nz)

    slope = PITCH / 159.0
    length = float(np.hypot(b_ridge - b_near, height / 159.0))
    tiles = mats["roof-tiles"]
    # Le pan arrière (vers b = 0) puis le pan avant : la texture descend du faîtage vers l'égout.
    surface.quad(pts((a_start, b_ridge, height), (a_end, b_ridge, height), (a_end, b_near, 0), (a_start, b_near, 0)),
                 tiles, [(a_start, 0), (a_end, 0), (a_end, length), (a_start, length)],
                 shade_of(normal(0, -slope, 1)))
    surface.quad(pts((a_start, b_ridge, height), (a_end, b_ridge, height), (a_end, b_far, 0), (a_start, b_far, 0)),
                 tiles, [(a_start, 0), (a_end, 0), (a_end, length), (a_start, length)],
                 shade_of(normal(0, slope, 1)))
    # L'épaisseur du toit à la rive du pignon, et le long de l'égout avant.
    edge = shade_of(normal(1, 0, 0)) * 0.72
    for b0, b1 in ((b_near, b_ridge), (b_ridge, b_far)):
        surface.quad(pts((a_end, b0, z_at(b0)), (a_end, b1, z_at(b1)), (a_end, b1, z_at(b1) - THICKNESS),
                         (a_end, b0, z_at(b0) - THICKNESS)),
                     tiles, [(0, 0.95), (0.01, 0.95), (0.01, 0.97), (0, 0.97)], edge)
    # La rive d'égout, sous le bord avant.
    surface.quad(pts((a_start, b_far, 0), (a_end, b_far, 0), (a_end, b_far, -EAVE_HEIGHT), (a_start, b_far, -EAVE_HEIGHT)),
                 mats["roof-eave"], [(a_start / 0.75, 0), (a_end / 0.75, 0), (a_end / 0.75, 0.999), (a_start / 0.75, 0.999)],
                 shade_of(normal(0, 1, 0)), wrap_v=False)
    # Le faîtage, posé sur l'arête.
    ridge0, ridge1 = a_start + 0.06, a_end - 0.06  # le faîtage s'arrête avant la rive.
    surface.quad(pts((ridge0, b_ridge, height + RIDGE_HEIGHT), (ridge1, b_ridge, height + RIDGE_HEIGHT),
                     (ridge1, b_ridge, height - 2), (ridge0, b_ridge, height - 2)),
                 mats["roof-ridge"], [(ridge0 / 0.5, 0), (ridge1 / 0.5, 0), (ridge1 / 0.5, 0.999), (ridge0 / 0.5, 0.999)],
                 shade_of(normal(0, 1, 0.4)), wrap_v=False)
    # Le pignon de pierre, au bout visible : sous le toit, de la face des murs au faîtage.
    wall = mats["wall"]
    gable_shade = shade_of(normal(1, 0, 0))
    under = z_at(b_wall0) - THICKNESS
    peak = height - THICKNESS
    for b0, b1, z0, z1 in ((b_wall0, b_ridge, under, peak), (b_ridge, b_wall1, peak, under)):
        surface.quad(pts((a_gable, b0, 0), (a_gable, b1, 0), (a_gable, b1, z1), (a_gable, b0, z0)),
                     wall, [(b0, 0), (b1, 0), (b1, -z1 / 196.0), (b0, -z0 / 196.0)], gable_shade)
        # La corniche rampante, le long du bord incliné.
        surface.quad(pts((a_gable + 0.004, b0, z0), (a_gable + 0.004, b1, z1),
                         (a_gable + 0.004, b1, z1 - CORNICE_HEIGHT), (a_gable + 0.004, b0, z0 - CORNICE_HEIGHT)),
                     mats["gable-cornice"], [(0, 0), (abs(b1 - b0) / 0.9, 0), (abs(b1 - b0) / 0.9, 0.999), (0, 0.999)],
                     gable_shade, wrap_v=False)


def pieces():
    """Chaque pièce : son nom, et ce qui la construit (sens, profondeur, longueur, case)."""
    for axis in ("u", "v"):
        across = "r" if axis == "u" else "c"
        for depth in DEPTHS:
            for k in range(depth):
                base = f"roof-{axis}-d{depth}-{across}{k}"
                yield f"{base}-start", axis, depth, 3, 0, k
                yield base, axis, depth, 3, 1, k
                yield f"{base}-gable", axis, depth, 3, 2, k
                yield f"{base}-single", axis, depth, 1, 0, k


def render_piece(mats, axis, depth, width, along, across):
    # La découpe : la case, étendue aux débords là où elle borde le toit.
    a0 = -INFINITY if along == 0 else along - OVERLAP
    a1 = INFINITY if along == width - 1 else along + 1 + OVERLAP
    b0 = -INFINITY if across == 0 else across - OVERLAP
    b1 = INFINITY if across == depth - 1 else across + 1 + OVERLAP
    column, row = (along, across) if axis == "u" else (across, along)
    clip = (a0 - column, b0 - row, a1 - column, b1 - row) if axis == "u" else (b0 - column, a0 - row, b1 - column, a1 - row)
    surface = Surface(clip)
    roof(surface, mats, depth, width, axis, (column, row))
    return surface.result()


# --- L'installation --------------------------------------------------------------------------------


def stage(name, image, anchor, metadata):
    """Comme `build_capital_v4.stage_piece` : la source calée, l'ancre géométrique gardée."""
    CALIBRES.mkdir(parents=True, exist_ok=True)
    STAGE.mkdir(parents=True, exist_ok=True)
    path = CALIBRES / f"{name}.png"
    image.save(path)
    spec = hd.PieceSpec(source=str(path), names=[name], family="09", footprint=(1, 1), scale=0.5,
                        tactical="open")
    rgba = np.asarray(image).copy()
    found = hd.fragments(rgba)
    if len(found) != 1:
        raise SystemExit(f"{name} : {len(found)} morceaux au lieu d'un")
    frag = found[0]
    built = hd.install_standing(spec, name, frag, (256, 159))
    wanted = (anchor - np.array(frag.box[:2])) / S + hd.MARGE
    pad = built.image.shape[0] - round(frag.image.shape[0] / S) - 2 * hd.MARGE
    wanted[1] += pad
    offset = [int(round(a - b)) for a, b in zip(wanted, built.entry["anchor"])]
    spec = replace(spec, anchor_offset=tuple(offset))
    built = hd.install_standing(spec, name, frag, (256, 159))
    Image.fromarray(built.image).save(STAGE / f"{name}.png")
    metadata[name] = {
        "entry": dict(built.entry, source={
            "file": path.relative_to(hd.SOURCES).as_posix(),
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}),
        "spec": {"source": os.path.relpath(path, ROOFS).replace("\\", "/"), "name": name,
                 "family": "09", "footprint": [1, 1], "tactical": "open", "scale": 0.5,
                 "anchorOffset": offset},
    }


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--provisional", action="store_true",
                        help="des matières de remplacement, pour juger la géométrie")
    parser.add_argument("--install", action="store_true", help="installer dans le kit de la Capitale")
    parser.add_argument("--only", help="ne construire que ces pièces, séparées par des virgules")
    args = parser.parse_args(argv)
    if args.install and args.provisional:
        raise SystemExit("build_capital_roofs : une matière provisoire ne s'installe jamais")
    mats = materials(args.provisional)
    only = set(args.only.split(",")) if args.only else None
    metadata = {}
    for name, axis, depth, width, along, across in pieces():
        if only and name not in only:
            continue
        image, anchor = render_piece(mats, axis, depth, width, along, across)
        stage(name, image, anchor, metadata)
    manifest = {"version": 1, "disposition": "capital", "tile": [256, 159], "storey": 224,
                "textures": {f"scene/capital/{n}": m["entry"] for n, m in metadata.items()}}
    (STAGE / "manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n",
                                         encoding="utf-8")
    descriptor = {"version": 1, "target": "Regions/central-empire/capital/Common/Scene",
                  "pieces": [m["spec"] for m in metadata.values()]}
    if args.install:
        path = ROOFS / "install.json"
        path.write_text(json.dumps(descriptor, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        d = hd.read_descriptor(path)
        hd.write(d, hd.build(d))
    print(f"toiture : {len(metadata)} pièce(s) dans {STAGE}"
          + (" (matières PROVISOIRES)" if args.provisional else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
