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

    python scripts/validate_capital_roofs.py [--scale 1|2] [--all]
    python scripts/validate_capital_roofs.py --sandbox DOSSIER

`--sandbox` écrit une racine d'essai **durable** pour l'éditeur, à ouvrir par
`LevelEditor --data DOSSIER` : le lieu `capital` (kit et toiture), la carte des deux bâtiments, et
`atelier`, une carte vierge pavée dont les couches rez, étage 1 et toit (étage 2) attendent qu'on les
peigne. L'éditeur ne lit pas encore les lieux sous `Assets/Regions/` (LOT-124) : sans cette racine,
les pièces de la Capitale ne s'y peignent pas.
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
ASSEMBLIES_ONLY = False

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
            if not ASSEMBLIES_ONLY:
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


def workshop() -> dict:
    """Une carte vierge pavée, ses couches prêtes : on y peint un bâtiment et son toit."""
    size = 16
    ground = [{"x": x, "y": y, "type": "pavement", "piece": f"floor-paving-0{((x * 7 + y * 3) % 3) + 1}"}
              for y in range(size) for x in range(size)]
    return {
        "version": 4, "name": "atelier", "width": size, "height": size, "nextEntityId": 1,
        "tiles": [{"x": 0, "y": size - 1, "type": "entry"}],
        "layers": [
            {"name": "sol", "kind": "ground", "scene": "capital", "tiles": ground},
            {"name": "rez", "kind": "decor", "tiles": []},
            {"name": "etage", "kind": "decor", "floor": 1, "tiles": []},
            {"name": "toit", "kind": "decor", "floor": 2, "tiles": []},
        ],
        "entities": [],
    }


def sandbox(directory: Path) -> int:
    """La racine d'essai de l'éditeur, contrôlée par `--check` avant d'être rendue à l'auteur."""
    if directory.exists():
        shutil.rmtree(directory)
    data_root(directory)
    # Les cartes passent par l'éditeur : la carte d'essai déjà construite par ses gestes, et
    # l'atelier normalisé par `--migrate` (écriture canonique, collision déduite).
    shutil.copy2(ROOT / "Source/Test/Fixtures/Storeys/roofs.json", directory / "Levels" / "roofs.json")
    (directory / "Levels" / "atelier.json").write_text(json.dumps(workshop(), indent=1),
                                                       encoding="utf-8")
    subprocess.run([str(EDITOR), "--data", str(directory), "--migrate", "atelier"],
                   capture_output=True, text=True, check=False)
    checked = subprocess.run([str(EDITOR), "--data", str(directory), "--check"],
                             capture_output=True, text=True, check=False)
    print(checked.stdout.strip().splitlines()[-1] if checked.stdout.strip() else checked.stderr)
    print(f"Ouvrir : {EDITOR} --data {directory}")
    return checked.returncode


def main(argv=None) -> int:
    global WIDTH, HEIGHT, BUILDINGS, STOREYS, ASSEMBLIES_ONLY
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--scale", default="1")
    parser.add_argument("--all", action="store_true", help="les 112 modules assemblés")
    parser.add_argument("--sandbox", type=Path, help="écrire une racine d'essai pour l'éditeur")
    args = parser.parse_args(argv)
    if args.sandbox:
        return sandbox(args.sandbox.resolve())
    if args.all:
        WIDTH, HEIGHT = 34, 30
        STOREYS = 1
        ASSEMBLIES_ONLY = True  # Les murs sont testés sur la carte à deux îlots.
        BUILDINGS = [(1 + (depth - 2) * 8, 1 + group * 7, length, depth, axis)
                     for group, (axis, length) in enumerate((("u", 5), ("v", 5), ("u", 1), ("v", 1)))
                     for depth in range(2, 6)]
    stem = ("roofs-all" if args.all else "roofs") + ("-2x" if args.scale == "2" else "")
    out = ROOFS / "apercus"
    out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="jadg-roofs-") as temporary:
        root = Path(temporary)
        data_root(root)
        gestures = root / "normalize.json"
        map_path = root / "Levels" / "roofs.json"
        seed = json.loads(map_path.read_text(encoding="utf-8"))
        strokes = []
        for layer in seed["layers"]:
            strokes.extend({"layer": layer["name"], "piece": tile["piece"],
                            "tool": "paint", "path": [[tile["x"], tile["y"]]]}
                           for tile in layer["tiles"])
            layer["tiles"] = []
        map_path.write_text(json.dumps(seed), encoding="utf-8")
        gestures.write_text(json.dumps({"format": "jadg-editor-gestures", "version": 1,
                                       "map": "roofs", "gestures": strokes}), encoding="utf-8")
        applied = subprocess.run([str(EDITOR), "--data", str(root), "--apply", str(gestures)],
                                 capture_output=True, text=True, check=False)
        if applied.returncode:
            print(applied.stdout + applied.stderr)
            return applied.returncode
        shutil.copy2(root / "Levels" / "roofs.json", out / f"{stem}.json")
        checked = subprocess.run([str(EDITOR), "--data", str(root), "--check"],
                                 capture_output=True, text=True, check=False)
        (out / f"{stem}-check.log").write_text(checked.stdout + checked.stderr, encoding="utf-8")
        if checked.returncode:
            print(checked.stdout + checked.stderr)
            return checked.returncode
        result = subprocess.run([str(EDITOR), "--data", str(root), "--render", "roofs",
                                 "--scale", args.scale, "--output", str(out / f"{stem}.png")],
                                capture_output=True, text=True, check=False)
        print(result.stdout.strip().splitlines()[-1] if result.stdout.strip() else result.stderr)
        (out / f"{stem}.log").write_text(result.stdout + result.stderr, encoding="utf-8")
        return result.returncode


if __name__ == "__main__":
    sys.exit(main())
