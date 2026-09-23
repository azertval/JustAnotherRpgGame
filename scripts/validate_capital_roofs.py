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
SHOWCASE = False
WALL_SPECS = {}

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


def l_roof(c0: int, r0: int, depth: int, corner: str, wing: int = 2) -> list[tuple[int, int, str]]:
    """Un toit en L : le carré d'angle en (c0, r0), ses deux ailes de `wing` cases (LOT-129)."""
    placed = [(c0 + c, r0 + r, f"roof-l-d{depth}-{corner}-c{c}r{r}")
              for r in range(depth) for c in range(depth)]
    east, north = corner[1] == "e", corner[0] == "n"
    for k in range(wing):
        # L'aile U : vers l'ouest (sa colonne la plus à l'ouest est sa rive), ou vers l'est (pignon).
        column = c0 - 1 - k if east else c0 + depth + k
        last = k == wing - 1
        suffix = ("-start" if east else "-gable") if last else ""
        placed += [(column, r0 + r, f"roof-u-d{depth}-r{r}{suffix}") for r in range(depth)]
        # L'aile V : vers le sud (pignon au bout), ou vers le nord (rive).
        row = r0 + depth + k if north else r0 - 1 - k
        suffix = ("-gable" if north else "-start") if last else ""
        placed += [(c0 + c, row, f"roof-v-d{depth}-c{c}{suffix}") for c in range(depth)]
    return placed


L_ROOFS: list[tuple[int, int, int, str]] = []
JUNCTION_ROOFS: list[tuple[int, int, int, str, str]] = []


def junction(c0, r0, depth, kind, direction, wing=2):
    from build_capital_roofs import junction_sides
    placed = [(c0 + c, r0 + r, f"roof-{kind}-d{depth}-{direction}-c{c}r{r}")
              for r in range(depth) for c in range(depth)]
    for side in sorted(junction_sides(kind, direction)):
        for k in range(wing):
            suffix = ("-start" if side in "wn" else "-gable") if k == wing - 1 else ""
            for cross in range(depth):
                if side in "we":
                    x = c0 - 1 - k if side == "w" else c0 + depth + k
                    placed.append((x, r0 + cross, f"roof-u-d{depth}-r{cross}{suffix}"))
                else:
                    y = r0 - 1 - k if side == "n" else r0 + depth + k
                    placed.append((c0 + cross, y, f"roof-v-d{depth}-c{cross}{suffix}"))
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
    for c0, r0, depth, corner in L_ROOFS:
        for x, y, piece in l_roof(c0, r0, depth, corner):
            top.append({"x": x, "y": y, "type": "wall", "piece": piece})
    for c0, r0, depth, kind, direction in JUNCTION_ROOFS:
        for x, y, piece in junction(c0, r0, depth, kind, direction):
            top.append({"x": x, "y": y, "type": "wall", "piece": piece})
    if SHOWCASE:
        occupied = {(t["x"], t["y"]) for t in top}
        for x, y in sorted(occupied):
            east, south = (x+1, y) not in occupied, (x, y+1) not in occupied
            inner = not east and not south and (x+1, y+1) not in occupied
            if not (east or south or inner):
                continue
            # Façades de contrôle d'une case, même calcaire et même hauteur que la V4.
            spec = (east, south, inner)
            name = "validation-wall-" + "".join(str(int(v)) for v in spec)
            WALL_SPECS[name] = spec
            tile = {"x": x, "y": y, "type": "wall", "piece": name}
            rez.append(tile.copy())
            upper.append(tile.copy())
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
    layout = level()
    staged = json.loads((ROOFS / "Scene" / "manifest.json").read_text(encoding="utf-8"))
    # Le kit installé fait foi, rangé en sous-dossiers (LOT-129) ; les pièces préparées ne
    # s'ajoutent que si elles ne sont pas encore installées.
    for key, entry in staged["textures"].items():
        if key in manifest["textures"]:
            continue
        shutil.copy2(ROOFS / "Scene" / entry["file"], scene / entry["file"])
        manifest["textures"][key] = entry
    if SHOWCASE:
        add_showcase_walls(scene, manifest)
    (scene / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    (scene / "appearance.json").write_text(
        json.dumps({"version": 1, "place": "capital", "floors": {}, "relief": {}}), encoding="utf-8")
    (root / "Levels").mkdir(parents=True)
    (root / "Levels" / "roofs.json").write_text(json.dumps(layout, indent=1), encoding="utf-8")


def add_showcase_walls(scene, manifest):
    """Supports réservés à la carte de contrôle ; aucune modification du kit de murs."""
    from PIL import Image
    import build_capital_roofs as roofs
    tex = roofs.materials(False)["wall"]
    for name, (east, south, inner) in WALL_SPECS.items():
        surface = roofs.Surface((-10, -10, 10, 10), local_view=True)
        runs = []
        if south:
            runs.append(((0, .59), (.59 if east else 1, .59), (0, 1, 0)))
        if east:
            runs.append(((.59, 0), (.59, .59 if south else 1), (1, 0, 0)))
        if inner:
            runs.extend([((.59, .59), (1, .59), (0, 1, 0)), ((.59, .59), (.59, 1), (1, 0, 0))])
        for (x0, y0), (x1, y1), normal in runs:
            a0, a1 = (x0, x1) if normal[1] else (y0, y1)
            surface.quad([(x0,y0,224),(x1,y1,224),(x1,y1,0),(x0,y0,0)], tex,
                         [(a0, -224/196),(a1,-224/196),(a1,0),(a0,0)], roofs.shade_of(normal))
        image, anchor = surface.result()
        box = image.getbbox()
        left, top = min(box[0], int(anchor[0])-4), min(box[1], int(anchor[1])-4)
        right, bottom = max(box[2], int(anchor[0])+4), max(box[3], int(anchor[1])+4)
        image = image.crop((left, top, right, bottom))
        image = image.resize((round(image.width/2), round(image.height/2)), Image.Resampling.LANCZOS)
        image.save(scene / f"{name}.png")
        manifest["textures"][f"scene/capital/{name}"] = dict(file=f"{name}.png", **{"class":"tall"},
                family="02", footprint=[1,1], size=list(image.size), tactical="solid",
                anchor=[round((anchor[0]-left)/2),round((anchor[1]-top)/2)])


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
    global WIDTH, HEIGHT, BUILDINGS, STOREYS, ASSEMBLIES_ONLY, SHOWCASE
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--scale", default="1")
    parser.add_argument("--all", action="store_true", help="les 112 modules assemblés")
    parser.add_argument("--sandbox", type=Path, help="écrire une racine d'essai pour l'éditeur")
    parser.add_argument("--l", action="store_true", help="les toits en L, quatre angles par profondeur")
    parser.add_argument("--junctions", action="store_true", help="les T et croisements, toutes profondeurs")
    parser.add_argument("--showcase", action="store_true", help="bâtiments L et T sur deux niveaux")
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
    if args.l:
        WIDTH, HEIGHT = 44, 44
        STOREYS = 1
        ASSEMBLIES_ONLY = True
        BUILDINGS = []
        # Un L par angle et par profondeur : les ailes de deux cases débordent du carré.
        L_ROOFS.extend((3 + (depth - 2) * 10, 3 + index * 10, depth, corner)
                       for index, corner in enumerate(("ne", "nw", "se", "sw"))
                       for depth in range(2, 6))
    if args.junctions:
        WIDTH, HEIGHT = 44, 54
        STOREYS = 1
        ASSEMBLIES_ONLY = True
        BUILDINGS = []
        JUNCTION_ROOFS.extend((3 + (depth - 2) * 10, 3 + index * 10, depth, kind, direction)
                             for index, (kind, direction) in enumerate([("t", d) for d in "nesw"] + [("x", "all")])
                             for depth in range(2, 6))
    if args.showcase:
        WIDTH, HEIGHT, STOREYS = 24, 24, 2
        ASSEMBLIES_ONLY = True
        SHOWCASE = True
        BUILDINGS = []
        L_ROOFS.extend([(4,4,3,"nw"), (14,4,3,"se")])
        JUNCTION_ROOFS.extend([(4,14,3,"t","s"),(14,14,3,"t","e")])
    stem = ("roofs-buildings" if args.showcase else "roofs-junctions" if args.junctions else "roofs-l" if args.l else "roofs-all" if args.all else "roofs") + ("-2x" if args.scale == "2" else "")
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
        if args.showcase and result.returncode == 0:
            shutil.copytree(root, ROOFS / "EngineJunctions", dirs_exist_ok=True)
        return result.returncode


if __name__ == "__main__":
    sys.exit(main())
