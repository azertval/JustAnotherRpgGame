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
from concurrent.futures import ProcessPoolExecutor
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

    def __init__(self, clip, clip_after_visibility=False, local_view=False):
        self.clip = clip
        # Découper après le tampon de profondeur : garder les pixels dont la surface VISIBLE est
        # au-dessus de la case. Un toit en L en a besoin — le pan d'une aile, caché par l'autre aile
        # sur la case voisine, reparaîtrait sinon en morceau détaché une fois la voisine découpée.
        self.clip_after_visibility = clip_after_visibility
        self.roof_union = []
        self.roof_shift = (0, 0)
        # En pixels d'art, avant le suréchantillonnage : un toit de 5 cases de profondeur et 3 de long,
        # la case de la pièce où qu'elle soit, y tient.
        self.origin = np.array([760.0, 420.0])
        self.size = (int(1400 * S), int(1200 * S))
        if local_view:
            # Fenêtre couvrant la projection d'une case, ses débords et le plus haut faîtage.
            # Toutes les faces du toit passent encore dans le tampon de visibilité.
            self.origin = np.array([250.0, 300.0])
            self.size = (int(500 * S), int(600 * S))
        self.rgba = np.zeros((self.size[1], self.size[0], 4), np.uint8)
        self.depth = np.full((self.size[1], self.size[0]), -INFINITY)
        self.where = np.full((self.size[1], self.size[0], 2), np.nan)

    def screen(self, p):
        p = np.asarray(p, float)
        xy = np.stack([(p[:, 0] - p[:, 1]) * HALF[0], (p[:, 0] + p[:, 1]) * HALF[1] - p[:, 2]], axis=1)
        return (xy + self.origin) * S

    def quad(self, points, tex, uv, shade=1.0, wrap_v=True, triangle=False, roof_top=False):
        p = np.asarray(points, float)
        if not triangle and not np.allclose(p[0] + p[2], p[1] + p[3]):
            for indices in ([0, 1, 2, 2], [0, 2, 3, 3]):
                self.quad(p[indices], tex, np.asarray(uv)[indices], shade, wrap_v, True, roof_top)
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
        if roof_top:
            for axis, depth, start, end in self.roof_union:
                wx = world[..., 0] + self.roof_shift[0]
                wy = world[..., 1] + self.roof_shift[1]
                along, across_coord = (wx, wy) if axis == "u" else (wy, wx)
                near, far = across(depth)
                inside = (along >= start) & (along <= end) & (across_coord >= near) & (across_coord <= far)
                envelope = PITCH * np.minimum(across_coord - near, far - across_coord)
                # Une face à l'intérieur de l'autre volume ne doit jamais ressortir sous sa rive.
                mask &= ~inside | (world[..., 2] >= envelope - 1e-7)
        x0, y0, x1, y1 = self.clip
        if not self.clip_after_visibility:
            mask &= ((world[..., 0] >= x0) & (world[..., 0] < x1) & (world[..., 1] >= y0)
                     & (world[..., 1] < y1))
        depth = world[..., 0] + world[..., 1] + world[..., 2] / 159.0
        uv = np.asarray(uv, float)
        t = uv[0] + a[..., None] * (uv[1] - uv[0]) + b[..., None] * (uv[3] - uv[0])
        pixels = sample(tex, t[..., 0], t[..., 1], wrap_v)
        pixels[..., :3] = np.uint8(np.clip(pixels[..., :3].astype(float) * shade, 0, 255))
        mask &= (depth >= self.depth[top:bottom, left:right] - 1e-6) & (pixels[..., 3] > 0)
        self.rgba[top:bottom, left:right][mask] = pixels[mask]
        self.depth[top:bottom, left:right][mask] = depth[mask]
        self.where[top:bottom, left:right][mask] = world[..., :2][mask]

    def result(self):
        rgba = self.rgba
        if self.clip_after_visibility:
            x0, y0, x1, y1 = self.clip
            wx, wy = self.where[..., 0], self.where[..., 1]
            with np.errstate(invalid="ignore"):
                keep = (wx >= x0) & (wx < x1) & (wy >= y0) & (wy < y1)
            rgba = rgba.copy()
            rgba[~keep] = 0
        return Image.fromarray(rgba), self.origin * S


# --- Le toit ---------------------------------------------------------------------------------------


def across(depth: int) -> tuple[float, float]:
    """L'étendue d'un toit en travers de son faîtage, sur `depth` cases.

    Les façades vues (la rangée de devant, la colonne de droite) sont au milieu de leur case ; les
    murs droits du kit courent, eux, jusqu'au bord de leur case du côté caché. Le toit part donc du
    bord des cases au nord et à l'ouest, et du nu des façades au sud et à l'est.
    """
    return -EAVE, depth - 0.5 + WALL_HALF + EAVE


def roof(surface: Surface, mats, depth: int, width: int, axis: str, shift) -> None:
    """Le toit entier d'un îlot de `width` cases de long et `depth` de profondeur, pignon au bout
    visible.

    Dans le repère du toit, `a` court le long du faîtage et `b` en travers ; `axis` dit si le faîtage
    suit les colonnes (`u` : a = x) ou les rangées (`v` : a = y). `shift` ramène le sommet nord de la
    case de la pièce à l'origine.
    """
    prism(surface, mats, axis, depth, -VERGE, width - 0.5 + WALL_HALF + VERGE, True, shift)


def prism(surface: Surface, mats, axis: str, depth: int, a_start: float, a_end: float, gable: bool,
          shift, eave_gap: tuple[float, float] | None = None, verge: float = VERGE) -> None:
    """Un toit à deux pans de `depth` cases en travers, de `a_start` à `a_end` le long du faîtage.

    `gable` : un pignon de pierre au bout `a_end`, le bout visible ; sans lui, le toit s'arrête net
    (il continue hors de la pièce, ou meurt dans une autre aile). `eave_gap` : le tronçon de l'égout
    avant qu'une autre aile recouvre, et qui ne se dessine pas.
    """
    b_near, b_far = across(depth)
    b_ridge = (b_near + b_far) / 2.0
    height = PITCH * (b_ridge - b_near)
    a_gable = a_end - verge
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
                 shade_of(normal(0, -slope, 1)), roof_top=True)
    surface.quad(pts((a_start, b_ridge, height), (a_end, b_ridge, height), (a_end, b_far, 0), (a_start, b_far, 0)),
                 tiles, [(a_start, 0), (a_end, 0), (a_end, length), (a_start, length)],
                 shade_of(normal(0, slope, 1)), roof_top=True)
    # L'épaisseur du toit à la rive du pignon.
    edge = shade_of(normal(1, 0, 0)) * 0.72
    for b0, b1 in ((b_near, b_ridge), (b_ridge, b_far)) if gable else ():
        surface.quad(pts((a_end, b0, z_at(b0)), (a_end, b1, z_at(b1)), (a_end, b1, z_at(b1) - THICKNESS),
                         (a_end, b0, z_at(b0) - THICKNESS)),
                     tiles, [(0, 0.95), (0.01, 0.95), (0.01, 0.97), (0, 0.97)], edge)
    # La rive d'égout, sous le bord avant, sauf là où une autre aile la recouvre.
    runs = [(a_start, a_end)]
    if eave_gap is not None:
        runs = [(a_start, min(a_end, eave_gap[0])), (max(a_start, eave_gap[1]), a_end)]
    for e0, e1 in runs:
        if e1 <= e0:
            continue
        surface.quad(pts((e0, b_far, 0), (e1, b_far, 0), (e1, b_far, -EAVE_HEIGHT), (e0, b_far, -EAVE_HEIGHT)),
                     mats["roof-eave"], [(e0 / 0.75, 0), (e1 / 0.75, 0), (e1 / 0.75, 0.999), (e0 / 0.75, 0.999)],
                     shade_of(normal(0, 1, 0)), wrap_v=False)
    # Le faîtage, posé sur l'arête ; il s'arrête avant une rive, pas là où le toit continue.
    ridge0 = a_start + (0.06 if abs(a_start + VERGE) < 1e-9 else 0.0)
    ridge1 = a_end - (0.06 if gable else 0.0)
    surface.quad(pts((ridge0, b_ridge, height + RIDGE_HEIGHT), (ridge1, b_ridge, height + RIDGE_HEIGHT),
                     (ridge1, b_ridge, height - 2), (ridge0, b_ridge, height - 2)),
                 mats["roof-ridge"], [(ridge0 / 0.5, 0), (ridge1 / 0.5, 0), (ridge1 / 0.5, 0.999), (ridge0 / 0.5, 0.999)],
                 shade_of(normal(0, 1, 0.4)), wrap_v=False)
    # Le pignon de pierre, au bout visible : sous le toit, de la face des murs au faîtage.
    if not gable:
        return
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


# Le carré d'angle d'un toit en L : sa position dans le L, et d'où viennent les deux ailes. L'aile U
# (faîtage le long des colonnes) traverse le carré jusqu'à son bout ; le faîtage de l'aile V vient
# mourir sur le sien, et le tampon de profondeur trace les noues à la rencontre des pans.
L_CORNERS = ("ne", "nw", "se", "sw")
WING = 3  # la longueur d'aile rendue autour du carré : de quoi couvrir les débords d'une case.


def l_roof(surface: Surface, mats, depth: int, corner: str, shift) -> None:
    """Le toit en L dont le carré d'angle, de `depth` cases, occupe les cases (0..depth-1)².

    `corner` situe le carré dans le L : `ne`, l'angle nord-est — l'aile U part vers l'ouest et l'aile
    V vers le sud.
    """
    b_near, b_far = across(depth)
    ridge = (b_near + b_far) / 2.0
    east, north = corner[1] == "e", corner[0] == "n"
    # L'aile U : du bout ouest (hors pièce) au pignon est, ou de la rive ouest vers l'est. L'égout
    # sud qui passe sous l'aile V ne se dessine pas.
    # Au coin, rive et égout partagent la même limite extérieure ; sinon le pan de V
    # dépasse de la rive de U et réapparaît en languette sous le pignon.
    u_start, u_end = (-WING, b_far) if east else (b_near, depth + WING)
    v_start, v_end = (ridge, depth + WING) if north else (-WING, ridge)
    surface.roof_shift = shift
    surface.roof_union = [("u", depth, u_start, u_end), ("v", depth, v_start, v_end)]
    gap = (b_near - 0.01, b_far + 0.01) if north else None
    prism(surface, mats, "u", depth, u_start, u_end, east, shift, gap, verge=EAVE)
    # L'aile V : de la ligne de faîtage de l'aile U vers le sud, ou du nord jusqu'à elle. Son égout est
    # ne commence qu'au sortir de l'aile U.
    if north:
        prism(surface, mats, "v", depth, ridge, depth + WING, False, shift, (ridge - 0.01, b_far))
    else:
        prism(surface, mats, "v", depth, -WING, ridge, False, shift, (b_near, ridge + 0.01))


def l_neighbours(depth: int, corner: str, column: int, row: int) -> tuple[bool, bool, bool, bool]:
    """Les voisines couvertes de la case (colonne, rangée) du carré : ouest, est, nord, sud."""
    east, north = corner[1] == "e", corner[0] == "n"
    return (column > 0 or east, column < depth - 1 or not east,
            row > 0 or not north, row < depth - 1 or north)


def junction_sides(kind: str, direction: str) -> set[str]:
    """Direction = branche du T ; un X continue sur les quatre côtés."""
    if kind == "x":
        return set("wens")
    return set("we" if direction in "ns" else "ns") | {direction}


def junction_roof(surface, mats, depth, kind, direction, shift):
    near, far = across(depth)
    ridge = (near + far) / 2
    primary = "u" if direction in "ns" or kind == "x" else "v"
    secondary = "v" if primary == "u" else "u"
    positive = direction in "se"
    start, end = (-WING, depth + WING) if kind == "x" else ((ridge, depth + WING) if positive else (-WING, ridge))
    surface.roof_shift = shift
    surface.roof_union = [(primary, depth, -WING, depth + WING), (secondary, depth, start, end)]
    gap = (near - 0.01, far + 0.01) if positive or kind == "x" else None
    prism(surface, mats, primary, depth, -WING, depth + WING, False, shift, gap)
    if kind == "x":
        prism(surface, mats, secondary, depth, -WING, depth + WING, False, shift,
              (near - 0.01, far + 0.01))
    elif positive:
        prism(surface, mats, secondary, depth, ridge, depth + WING, False, shift,
              (ridge - 0.01, far + 0.01))
    else:
        prism(surface, mats, secondary, depth, -WING, ridge, False, shift,
              (near - 0.01, ridge + 0.01))


def render_junction_piece(mats, depth, kind, direction, column, row):
    sides = junction_sides(kind, direction)
    clip = (-OVERLAP if column > 0 or "w" in sides else -INFINITY,
            -OVERLAP if row > 0 or "n" in sides else -INFINITY,
            1 + OVERLAP if column < depth - 1 or "e" in sides else INFINITY,
            1 + OVERLAP if row < depth - 1 or "s" in sides else INFINITY)
    surface = Surface(clip, clip_after_visibility=True, local_view=True)
    junction_roof(surface, mats, depth, kind, direction, (column, row))
    return surface.result()


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
    for depth in DEPTHS:
        for corner in L_CORNERS:
            for row in range(depth):
                for column in range(depth):
                    yield f"roof-l-d{depth}-{corner}-c{column}r{row}", "l", depth, corner, column, row
    for depth in DEPTHS:
        for kind, direction in [("t", d) for d in "nesw"] + [("x", "all")]:
            for row in range(depth):
                for column in range(depth):
                    yield f"roof-{kind}-d{depth}-{direction}-c{column}r{row}", kind, depth, direction, column, row


def render_piece(mats, axis, depth, width, along, across):
    if axis == "l":
        return render_l_piece(mats, depth, width, along, across)
    if axis in ("t", "x"):
        return render_junction_piece(mats, depth, axis, width, along, across)
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


def render_l_piece(mats, depth, corner, column, row):
    """Une case du carré d'angle : découpée à ses voisines couvertes, ouverte aux débords ailleurs."""
    west, east, north, south = l_neighbours(depth, corner, column, row)
    clip = (-OVERLAP if west else -INFINITY, -OVERLAP if north else -INFINITY,
            1 + OVERLAP if east else INFINITY, 1 + OVERLAP if south else INFINITY)
    surface = Surface(clip, clip_after_visibility=True, local_view=True)
    l_roof(surface, mats, depth, corner, (column, row))
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


_WORKER_MATS = None


def _init_worker(use_provisional):
    global _WORKER_MATS
    _WORKER_MATS = materials(use_provisional)


def _build_worker(piece):
    name, axis, depth, width, along, across = piece
    image, anchor = render_piece(_WORKER_MATS, axis, depth, width, along, across)
    metadata = {}
    stage(name, image, anchor, metadata)
    return metadata


# Le rangement des toits dans le projet (LOT-129) : par sorte, puis par largeur d'aile.
ROOF_FOLDERS = [
    {"match": r"^roof-[uv]-d(\d)", "folder": r"roofs/straight/d\1"},
    {"match": r"^roof-l-d(\d)", "folder": r"roofs/l/d\1"},
    {"match": r"^roof-t-d(\d)", "folder": r"roofs/t/d\1"},
    {"match": r"^roof-x-d(\d)", "folder": r"roofs/x/d\1"},
]


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--provisional", action="store_true",
                        help="des matières de remplacement, pour juger la géométrie")
    parser.add_argument("--install", action="store_true", help="installer dans le kit de la Capitale")
    parser.add_argument("--only", help="ne construire que ces pièces, séparées par des virgules")
    parser.add_argument("--junctions", action="store_true", help="ajouter seulement les T et X")
    parser.add_argument("--connections", action="store_true", help="construire L, T et X, préserver les droites")
    parser.add_argument("--jobs", type=int, default=1, choices=range(1, 5), help="processus de fabrication indépendants")
    args = parser.parse_args(argv)
    if args.install and args.provisional:
        raise SystemExit("build_capital_roofs : une matière provisoire ne s'installe jamais")
    mats = materials(args.provisional)
    only = set(args.only.split(",")) if args.only else None
    metadata = {}
    selected = []
    for name, axis, depth, width, along, across in pieces():
        if only and name not in only:
            continue
        if args.junctions and axis not in ("t", "x"):
            continue
        if args.connections and axis not in ("l", "t", "x"):
            continue
        selected.append((name, axis, depth, width, along, across))
    if args.jobs > 1:
        with ProcessPoolExecutor(max_workers=args.jobs, initializer=_init_worker,
                                 initargs=(args.provisional,)) as pool:
            for completed, result in enumerate(pool.map(_build_worker, selected), 1):
                metadata.update(result)
                if completed % 50 == 0:
                    print(f"toiture : {completed}/{len(selected)}", flush=True)
    else:
        for name, axis, depth, width, along, across in selected:
            image, anchor = render_piece(mats, axis, depth, width, along, across)
            stage(name, image, anchor, metadata)
    manifest = {"version": 1, "disposition": "capital", "tile": [256, 159], "storey": 224,
                "textures": {f"scene/capital/{n}": m["entry"] for n, m in metadata.items()}}
    if (only or args.junctions or args.connections) and (STAGE / "manifest.json").exists():
        previous = json.loads((STAGE / "manifest.json").read_text(encoding="utf-8"))
        manifest["textures"] = previous["textures"] | manifest["textures"]
    (STAGE / "manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n",
                                         encoding="utf-8")
    descriptor = {"version": 1, "target": "Regions/central-empire/capital/Common/Scene",
                  "folders": ROOF_FOLDERS, "pieces": [m["spec"] for m in metadata.values()]}
    if args.install:
        path = ROOFS / "install.json"
        incremental = descriptor
        if (only or args.junctions or args.connections) and path.exists():
            previous = json.loads(path.read_text(encoding="utf-8"))
            by_name = {p["name"]: p for p in previous["pieces"]}
            by_name.update({p["name"]: p for p in descriptor["pieces"]})
            descriptor = dict(descriptor, pieces=list(by_name.values()))
        path.write_text(json.dumps(descriptor, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        incremental_path = ROOFS / "install-current.json"
        incremental_path.write_text(json.dumps(incremental, indent=2) + "\n", encoding="utf-8")
        d = hd.read_descriptor(incremental_path)
        hd.write(d, hd.build(d))
    print(f"toiture : {len(metadata)} pièce(s) dans {STAGE}"
          + (" (matières PROVISOIRES)" if args.provisional else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
