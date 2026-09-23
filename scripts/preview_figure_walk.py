#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Fait marcher une figurine sur la maquette du LOT-101, à la vitesse et à la cadence du jeu (LOT-112).

Une cadence se juge **à côté de son ancre et de son sol**, pas sur une planche : c'est ce que le
standard demande pour trancher six ou huit images par animation, et c'est ce que le critère du
héros vérifie (« il marche sans glisser ni flotter »). Cet aperçu pose la figurine sur la vue 1080
de la maquette (`maquette-2d-hd-1080.png`, 100 px par case), les pieds au centre du losange de sa
position — la règle du moteur (`hmi::composeWorldScene`, ligne de sol du manifeste) — et la fait
avancer à `WALK_SPEED` cases par seconde, image par image au `frameDuration` de sa bande.

La figurine vient d'un **descripteur** `install.json` en mode figurine : elle est préparée comme
`install_hd_asset.py` l'installerait (détourée, découpée, réduite, posée à 252), **sans rien écrire
dans le dépôt**. L'aperçu sert donc aussi bien à l'essai de cadence qu'à la figurine installée.

    python scripts/preview_figure_walk.py Tools/AssetsHD/Essais/lot-112-cadence/install.json \\
        --figure essai-6 --out build/lot-112/marche-6.webp

Le trajet est une boucle de trois cases par côté dans le coin libre de la place, une diagonale par
côté : sud-est, sud-ouest, nord-ouest, nord-est. Un côté dont la figurine n'a pas la bande de marche
est sauté — l'essai de cadence n'a que le sud-est, il fait donc des allers en sud-est.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import install_hd_asset as install  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
MOCKUP = ROOT / "Planning" / "versions" / "v0.1.0" / "v0.0.1-demo" / "maquettes" / "maquette-2d-hd-1080.png"

# La géométrie de la vue 1080 de la maquette (scripts/build_hd_mockup.py) : le losange de 256 × 159
# en pixels d'art, réduit à 100 px d'écran, centré sur le milieu de la place.
TILE_W, TILE_H = 256.0, 159.0
TILE_ON_SCREEN = 100.0
VIEW_CENTER = (3.5, 3.5)
SCREEN = (1920, 1080)
# La vitesse de marche du jeu (`core::ExplorationSession::WALK_SPEED_CELLS_PER_SECOND`).
WALK_SPEED = 2.0
FPS = 30
# La boucle : trois cases par côté, dans le coin sud de la place, où rien ne se dresse.
LOOP = [((5.0, 5.0), (8.0, 5.0), "se"), ((8.0, 5.0), (8.0, 8.0), "sw"),
        ((8.0, 8.0), (5.0, 8.0), "nw"), ((5.0, 8.0), (5.0, 5.0), "ne")]
# Le cadre de l'aperçu, autour de la boucle : l'image entière pèserait pour rien.
CROP = (760, 380, 1560, 1000)


def screen_of(column: float, row: float) -> tuple[float, float]:
    """Le point de la grille (@p column, @p row) sur l'écran 1080 — le centre du losange pour une
    case entière, comme `tile_center` de la maquette."""
    k = TILE_ON_SCREEN / TILE_W
    cx = (column - row) * TILE_W / 2.0 - (VIEW_CENTER[0] - VIEW_CENTER[1]) * TILE_W / 2.0
    cy = (column + row) * TILE_H / 2.0 - (VIEW_CENTER[0] + VIEW_CENTER[1]) * TILE_H / 2.0
    return SCREEN[0] / 2.0 + cx * k, SCREEN[1] / 2.0 + cy * k


def cells_of(strip: install.InstalledStrip) -> list[Image.Image]:
    """Les images de la bande, réduites à l'échelle de l'écran, en alpha prémultiplié."""
    width, height = strip.cell
    k = TILE_ON_SCREEN / TILE_W
    out = []
    for index in range(strip.image.shape[1] // width):
        cell = Image.fromarray(np.ascontiguousarray(strip.image[:, index * width:(index + 1) * width]), "RGBA")
        small = cell.convert("RGBa").resize((round(width * k), round(height * k)), Image.LANCZOS)
        out.append(small.convert("RGBA"))
    return out


def walk_strips(figure: install.InstalledFigure, clip: str) -> dict[str, install.InstalledStrip]:
    """Les bandes @p clip de la figurine, par orientation (`""` : sans orientation)."""
    return {(s.spec.facing or ""): s for s in figure.strips if s.spec.clip == clip}


def render(figure: install.InstalledFigure, clip: str, loops: int, frame_duration: float | None,
           speed: float, ground: int) -> tuple[list[Image.Image], list[str]]:
    strips = walk_strips(figure, clip)
    if not strips:
        raise install.DescriptorError(f"{figure.name} : pas de bande `{clip}`")
    legs = [leg for leg in LOOP if leg[2] in strips] or [(LOOP[0][0], LOOP[0][1], "")]
    if legs[0][2] == "" and "" not in strips:
        legs = [(LOOP[0][0], LOOP[0][1], next(iter(strips)))]
    notes = []
    background = Image.open(MOCKUP).convert("RGBA")
    frames: list[Image.Image] = []
    elapsed = 0.0
    for _ in range(loops):
        for start, end, facing in legs:
            strip = strips[facing]
            cells = cells_of(strip)
            duration = frame_duration or float(strip.anim["clips"][clip]["frameDuration"])
            distance = abs(end[0] - start[0]) + abs(end[1] - start[1])
            steps = max(1, round(distance / speed * FPS))
            for step in range(steps):
                t = step / steps
                column = start[0] + (end[0] - start[0]) * t
                row = start[1] + (end[1] - start[1]) * t
                x, y = screen_of(column, row)
                cell = cells[int(elapsed / duration) % len(cells)]
                k = TILE_ON_SCREEN / TILE_W
                left = round(x - cell.width / 2.0)
                top = round(y - ground * k)
                image = background.copy()
                image.alpha_composite(cell, (left, top))
                frames.append(image.crop(CROP).convert("RGB"))
                elapsed += 1.0 / FPS
            cycle = duration * len(cells)
            notes.append(f"{facing or '-'} : {len(cells)} images × {duration:.3f} s = cycle de {cycle:.2f} s, "
                         f"{speed * cycle:.2f} case(s) par cycle à {speed:g} cases/s")
    return frames, notes


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("descriptor", type=Path, help="un install.json en mode figurine")
    parser.add_argument("--figure", required=True, help="le nom de la figurine au descripteur")
    parser.add_argument("--clip", default="walk", help="la bande à faire tourner (walk)")
    parser.add_argument("--loops", type=int, default=2, help="nombre de tours de boucle")
    parser.add_argument("--frame-duration", type=float, help="remplace le frameDuration de la bande")
    parser.add_argument("--speed", type=float, default=WALK_SPEED, help="cases par seconde")
    parser.add_argument("--out", type=Path, required=True, help="l'aperçu, .webp ou .gif")
    args = parser.parse_args(argv)
    try:
        descriptor = install.read_descriptor(args.descriptor.resolve())
        figure = install.build_figures(descriptor, args.figure)[0]
        _, _, ground = install.figure_cells(install.read_manifest(descriptor))
        frames, notes = render(figure, args.clip, args.loops, args.frame_duration, args.speed, ground)
    except install.DescriptorError as error:
        print(f"preview_figure_walk : {error}", file=sys.stderr)
        return 1
    args.out.parent.mkdir(parents=True, exist_ok=True)
    frames[0].save(args.out, save_all=True, append_images=frames[1:], duration=round(1000 / FPS), loop=0,
                   **({"quality": 80, "method": 4} if args.out.suffix == ".webp" else {}))
    for note in dict.fromkeys(notes):
        print(note)
    print(f"{len(frames)} images, {len(frames) / FPS:.1f} s -> {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
