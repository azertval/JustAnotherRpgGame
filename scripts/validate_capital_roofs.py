#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Rend par le moteur des îlots de murs du kit sur deux étages, coiffés de la toiture (LOT-129).

Monte une racine de données temporaire — le kit de la Capitale installé, les pièces de toit de
`Toitures/Scene/` (celles que `build_capital_roofs.py` vient de caler, provisoires ou non) — et une
carte : deux bâtiments, l'un au faîtage le long des colonnes (U), l'autre le long des rangées (V),
murs au rez et à l'étage 1, toit à l'étage 2. `LevelEditor --render` la rend ; l'image va dans
`Toitures/apercus/`, la carte avec, pour relecture. La même carte, une fois la toiture installée,
est celle des données d'essai (`Source/Test/Fixtures/Storeys/`).

    python scripts/validate_capital_roofs.py [--scale 1] [--map-only DOSSIER]
"""
from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
KIT = ROOT / "Source/Elements/Assets/Regions/central-empire/capital/Common/Scene"
ROOFS = ROOT / "Tools/AssetsHD/Regions/central-empire/capital/Common/Toitures"
EDITOR = ROOT / "build/ninja/bin/LevelEditor.exe"
WIDTH, HEIGHT = 14, 12
STOREYS = 2

# (colonne, rangée) du coin nord, longueur le long du faîtage, profondeur, sens du faîtage.
BUILDINGS = [(1, 1, 5, 3, "u"), (8, 3, 3, 5, "v")]


def walls(c0: int, r0: int, columns: int, rows: int) -> list[tuple[int, int, str]]:
    """Les deux façades vues d'un îlot : la rangée de devant, la colonne de droite, l'angle sortant."""
    c1, r1 = c0 + columns - 1, r0 + rows - 1
    placed = []
    for index, c in enumerate(range(c0, c1, 2)):
        placed.append((c, r1, "wall-limestone-window-u" if index % 2 == 0 else "wall-limestone-u"))
    for index, r in enumerate(range(r0, r1, 2)):
        placed.append((c1, r, "wall-limestone-window-v" if index % 2 == 0 else "wall-limestone-v"))
    placed.append((c1, r1, "wall-limestone-corner-outer"))
    return placed


def roof(c0: int, r0: int, length: int, depth: int, axis: str) -> list[tuple[int, int, str]]:
    placed = []
    for along in range(length):
        for across in range(depth):
            if length == 1:
                suffix = "-single"
            elif along == 0:
                suffix = "-start"
            elif along == length - 1:
                suffix = "-gable"
            else:
                suffix = ""
            letter = "r" if axis == "u" else "c"
            name = f"roof-{axis}-d{depth}-{letter}{across}{suffix}"
            column, row = (c0 + along, r0 + across) if axis == "u" else (c0 + across, r0 + along)
            placed.append((column, row, name))
    return placed


def level() -> dict:
    ground = [{"x": x, "y": y, "type": "pavement", "piece": f"floor-paving-0{((x * 7 + y * 3) % 3) + 1}"}
              for y in range(HEIGHT) for x in range(WIDTH)]
    rez, upper, top = [], [], []
    collision = [{"x": 0, "y": HEIGHT - 1, "type": "entry"}]
    for c0, r0, length, depth, axis in BUILDINGS:
        columns, rows = (length, depth) if axis == "u" else (depth, length)
        for x, y, piece in walls(c0, r0, columns, rows):
            rez.append({"x": x, "y": y, "type": "wall", "piece": piece})
            upper.append({"x": x, "y": y, "type": "wall", "piece": piece})
        for x, y, piece in roof(c0, r0, length, depth, axis):
            top.append({"x": x, "y": y, "type": "wall", "piece": piece})
    return {
        "version": 4, "name": "roofs", "width": WIDTH, "height": HEIGHT, "nextEntityId": 1,
        "tiles": collision,
        "layers": [
            {"name": "sol", "kind": "ground", "scene": "capital", "tiles": ground},
            {"name": "rez", "kind": "decor", "tiles": rez},
            {"name": "etage", "kind": "decor", "floor": 1, "tiles": upper},
            {"name": "toit", "kind": "decor", "floor": STOREYS, "tiles": top},
        ],
        "entities": [],
    }


def data_root(root: Path) -> None:
    scene = root / "Assets" / "Scene" / "capital"
    shutil.copytree(KIT, scene)
    manifest = json.loads((scene / "manifest.json").read_text(encoding="utf-8"))
    staged = json.loads((ROOFS / "Scene" / "manifest.json").read_text(encoding="utf-8"))
    for key, entry in staged["textures"].items():
        shutil.copy2(ROOFS / "Scene" / entry["file"], scene / entry["file"])
        manifest["textures"][key] = entry
    (scene / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    (scene / "appearance.json").write_text(
        json.dumps({"version": 1, "place": "capital", "floors": {}, "relief": {}}), encoding="utf-8")
    (root / "Levels").mkdir(parents=True)
    (root / "Levels" / "roofs.json").write_text(json.dumps(level(), indent=1), encoding="utf-8")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--scale", default="1")
    args = parser.parse_args(argv)
    out = ROOFS / "apercus"
    out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="jadg-roofs-") as temporary:
        root = Path(temporary)
        data_root(root)
        shutil.copy2(root / "Levels" / "roofs.json", out / "roofs.json")
        result = subprocess.run([str(EDITOR), "--data", str(root), "--render", "roofs",
                                 "--scale", args.scale, "--output", str(out / "roofs.png")],
                                capture_output=True, text=True, check=False)
        print(result.stdout.strip().splitlines()[-1] if result.stdout.strip() else result.stderr)
        return result.returncode


if __name__ == "__main__":
    sys.exit(main())
