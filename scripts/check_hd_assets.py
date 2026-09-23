#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Garde-fou : les assets HD installés, leurs manifestes et le poids de chaque zone (LOT-104).

Sous `Source/Elements/Assets/Common/` et `Source/Elements/Assets/Regions/`, chaque dossier `Scene/`
et `Characters/` porte un `manifest.json` qui liste ses pièces (arborescence, règle 3). Ce contrôle
rend impossible l'écart SILENCIEUX entre les deux :

- une entrée de manifeste qui cite un fichier absent ;
- une image que son manifeste ne cite pas — déposée à la main, ou restée d'une pièce renommée ;
- une pièce hors des bornes du standard (`Planning/standards/style-2d-hd.md`) : PNG 32 bits, taille
  égale à celle que le manifeste déclare, 4096 px de côté au plus, une dalle de sol exactement au
  losange du lieu, une ancre dans l'image, un losange de lieu égal à celui de sa région ;
- une zone de plus de **40 Mio**, le budget de l'arborescence.

Le poids de chaque zone s'affiche, et s'écrit dans le résumé du job quand `GITHUB_STEP_SUMMARY`
est défini. Les assets installés s'écrivent par `scripts/install_hd_asset.py` ; ce contrôle n'a
pas besoin des sources, qui ne sont pas versionnées.

Aucune dépendance : l'en-tête PNG se lit à la main, comme dans `check_ui_assets.py`.

Usage :
    python scripts/check_hd_assets.py            # code de sortie non nul si écart
"""

from __future__ import annotations

import json
import os
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "Source" / "Elements" / "Assets"
WORLD_MAPS = ROOT / "Source" / "Elements" / "Maps" / "world-maps.json"

# Les deux arbres de l'arborescence HD ; `UI/`, `Maps/`, `Fonts/` et `Entities/` ont leurs contrôles.
TREES = ("Common", "Regions")
# Le losange du standard, pour le commun du monde qui n'a pas de région.
STANDARD_TILE = [256, 159]
MAX_SIDE = 4096
ZONE_BUDGET = 40 * 1024 * 1024
IMAGES = (".png", ".jpg", ".jpeg")
# Les dossiers qui font d'un dossier un lieu (zone ou sous-zone).
PLACE_PARTS = ("Scene", "Characters", "Map")
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
PNG_RGBA = 6


@dataclass
class Report:
    errors: list[str] = field(default_factory=list)
    # (lieu relatif, niveau, octets)
    weights: list[tuple[str, str, int]] = field(default_factory=list)

    def fail(self, message: str) -> None:
        self.errors.append(message)


def png_header(path: Path) -> tuple[int, int, int, int]:
    """Largeur, hauteur, profondeur et type de couleur d'un PNG, lus dans son en-tête IHDR."""
    with path.open("rb") as handle:
        data = handle.read(26)
    if len(data) < 26 or data[:8] != PNG_SIGNATURE or data[12:16] != b"IHDR":
        raise ValueError("en-tête PNG illisible")
    width, height = struct.unpack(">II", data[16:24])
    return width, height, data[24], data[25]


def relative(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def read_json(path: Path, report: Report, root: Path) -> dict | None:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        report.fail(f"{relative(path, root)} : illisible ({error})")
        return None
    if not isinstance(value, dict):
        report.fail(f"{relative(path, root)} : un objet JSON est attendu")
        return None
    return value


def region_tile(directory: Path, root: Path, report: Report) -> list[int]:
    """Le losange que déclare la région d'un dossier ; celui du standard hors d'une région."""
    parts = directory.relative_to(root).parts
    if len(parts) < 2 or parts[0] != "Regions":
        return STANDARD_TILE
    region = root / "Regions" / parts[1] / "region.json"
    data = read_json(region, report, root) if region.is_file() else None
    tile = (data or {}).get("tile")
    return tile if isinstance(tile, list) else STANDARD_TILE


def is_pair(value) -> bool:
    return (isinstance(value, list) and len(value) == 2
            and all(isinstance(v, int) and not isinstance(v, bool) for v in value))


def check_scene(directory: Path, manifest: dict, root: Path, report: Report) -> set[str]:
    """Les pièces d'un dossier `Scene/` : chaque fichier cité existe et tient dans le standard."""
    where = relative(directory / "manifest.json", root)
    cited: set[str] = set()
    tile = manifest.get("tile")
    expected_tile = region_tile(directory, root, report)
    if tile != expected_tile:
        report.fail(f"{where} : losange {tile}, la région déclare {expected_tile}")
    textures = manifest.get("textures")
    if not isinstance(textures, dict):
        report.fail(f"{where} : `textures` doit être un objet")
        return cited
    for key, entry in textures.items():
        label = f"{where} : {key}"
        if not isinstance(entry, dict) or not isinstance(entry.get("file"), str):
            report.fail(f"{label} : entrée sans `file`")
            continue
        cited.add(entry["file"])
        path = directory / entry["file"]
        if not path.is_file():
            report.fail(f"{label} : {entry['file']} absent")
            continue
        try:
            width, height, depth, colour = png_header(path)
        except (OSError, ValueError) as error:
            report.fail(f"{label} : {entry['file']} : {error}")
            continue
        if depth != 8 or colour != PNG_RGBA:
            report.fail(f"{label} : PNG {depth} bits de type {colour}, attendu RGBA 8 bits")
        if max(width, height) > MAX_SIDE:
            report.fail(f"{label} : {width} × {height}, au-delà de {MAX_SIDE} px de côté")
        size = entry.get("size")
        if size != [width, height]:
            report.fail(f"{label} : manifeste {size}, image {width} × {height}")
        footprint = entry.get("footprint", [1, 1])
        if not is_pair(footprint) or min(footprint) < 1:
            report.fail(f"{label} : emprise {footprint} (au moins 1 × 1)")
        if entry.get("class") == "floor" and [width, height] != expected_tile:
            report.fail(f"{label} : une dalle fait exactement le losange du lieu, {expected_tile}")
        anchor = entry.get("anchor")
        if anchor is not None and (not is_pair(anchor) or not (0 <= anchor[0] <= width and 0 <= anchor[1] <= height)):
            report.fail(f"{label} : ancre {anchor} hors de l'image {width} × {height}")
    return cited


# Les quatre orientations d'une figurine, suffixe de ses bandes (`walk-se.png`, LOT-112).
FACINGS = ("se", "sw", "ne", "nw")
# Les animations qu'une figurine ne peut pas omettre : elle attend et elle marche. Les autres
# dépendent de ce qu'elle fait — un Brawler n'a pas de sort.
REQUIRED_ANIMATIONS = ("idle", "walk")


def check_figure(folder: Path, animations: list, root: Path, report: Report) -> None:
    """Une figurine : ses bandes présentes ont leur `.anim.json`, une animation orientée l'est dans
    les quatre sens (une figurine à moitié tournée se verrait plus mal qu'une qui ne l'est pas), et
    le repos comme la marche existent."""
    where = relative(folder, root)
    for animation in animations:
        oriented = [f for f in FACINGS if (folder / f"{animation}-{f}.png").is_file()]
        if oriented and len(oriented) != len(FACINGS):
            missing = ", ".join(f"{animation}-{f}.png" for f in FACINGS if f not in oriented)
            report.fail(f"{where} : `{animation}` orientée à moitié, il manque {missing}")
        strips = [f"{animation}-{f}" for f in oriented]
        if (folder / f"{animation}.png").is_file():
            strips.append(animation)
        for strip in strips:
            if not (folder / f"{strip}.anim.json").is_file():
                report.fail(f"{where} : `{strip}.png` sans `{strip}.anim.json`")
        if animation in REQUIRED_ANIMATIONS and not strips:
            report.fail(f"{where} : pas de bande `{animation}`")


def check_characters(directory: Path, manifest: dict, root: Path, report: Report) -> dict[Path, set[str]]:
    """Les figurines d'un dossier `Characters/` : `<pnj>/portrait.png`, `token.png`, une bande par
    animation, ou une par animation et par orientation (arborescence, règle 5). Un PNJ peut être
    rangé plus bas (`Heroes/brawler`). Rend, par dossier de PNJ, les noms d'images cités."""
    where = relative(directory / "manifest.json", root)
    animations = manifest.get("animations", [])
    npcs = manifest.get("npcs", [])
    if not isinstance(npcs, list) or not isinstance(animations, list):
        report.fail(f"{where} : `npcs` et `animations` sont des listes")
        return {}
    cited: dict[Path, set[str]] = {}
    for npc in npcs:
        if not isinstance(npc, str) or not (directory / npc).is_dir():
            report.fail(f"{where} : PNJ {npc!r} sans dossier")
            continue
        strips = [*animations, *(f"{a}-{f}" for a in animations for f in FACINGS)]
        cited[directory / npc] = {f"{stem}.png" for stem in ["portrait", "token", *strips]}
        check_figure(directory / npc, animations, root, report)
    return cited


def place_level(directory: Path, root: Path) -> str | None:
    """Le niveau d'un dossier de l'arborescence : `zone`, `sous-zone`, un commun, ou rien."""
    parts = directory.relative_to(root).parts
    if parts == ("Common",):
        return "commun du monde"
    if parts[0] == "Regions" and parts[-1] == "Common":
        return "commun de région" if len(parts) == 3 else "commun de ville"
    if parts[0] != "Regions" or "Common" in parts:
        return None
    if not any((directory / part).is_dir() for part in PLACE_PARTS):
        return None
    parent_is_place = any((directory.parent / part).is_dir() for part in PLACE_PARTS)
    return "sous-zone" if parent_is_place else "zone"


def weigh(directory: Path, level: str) -> int:
    """Le poids d'un lieu : ses dossiers Scene/, Characters/ et Map/, sans ses sous-zones ; un
    commun pèse tout ce qu'il contient."""
    if level.startswith("commun"):
        targets = [directory]
    else:
        targets = [directory / part for part in PLACE_PARTS if (directory / part).is_dir()]
    return sum(path.stat().st_size for target in targets for path in target.rglob("*") if path.is_file())


def check(root: Path = ASSETS, maps_text: str | None = None) -> Report:
    report = Report()
    if maps_text is None:
        maps_text = WORLD_MAPS.read_text(encoding="utf-8") if WORLD_MAPS.is_file() else ""
    for tree in TREES:
        base = root / tree
        if not base.is_dir():
            continue
        # Trié sur le chemin écrit : le même ordre sous Windows, qui compare sans la casse, et Linux.
        directories = [base, *sorted((p for p in base.rglob("*") if p.is_dir()), key=lambda p: p.as_posix())]

        # D'abord les manifestes : ce que chacun cite, par dossier (un manifeste de Characters/ cite
        # les images des dossiers de ses PNJ).
        cited: dict[Path, set[str]] = {}
        for directory in directories:
            manifest_path = directory / "manifest.json"
            if not manifest_path.is_file():
                if directory.name in ("Scene", "Characters"):
                    report.fail(f"{relative(directory, root)} : dossier sans manifest.json")
                continue
            manifest = read_json(manifest_path, report, root)
            if manifest is None:
                continue
            if "textures" in manifest:
                cited[directory] = check_scene(directory, manifest, root, report)
            elif "npcs" in manifest:
                cited.update(check_characters(directory, manifest, root, report))

        for directory in directories:
            for image in sorted(p for p in directory.iterdir() if p.is_file() and p.suffix.lower() in IMAGES):
                if directory.name == "Map":
                    if relative(image, root) not in maps_text and image.name not in maps_text:
                        report.fail(f"{relative(image, root)} : image de carte que world-maps.json ne cite pas")
                elif image.name not in cited.get(directory, set()):
                    report.fail(f"{relative(image, root)} : image qu'aucun manifeste ne cite (EX-CNT-042)")

            level = place_level(directory, root)
            if level is not None:
                weight = weigh(directory, level)
                report.weights.append((relative(directory, root), level, weight))
                if not level.startswith("commun") and weight > ZONE_BUDGET:
                    report.fail(f"{relative(directory, root)} : {mib(weight)}, au-delà du budget de "
                                f"{mib(ZONE_BUDGET)} par zone")
    return report


def mib(size: int) -> str:
    return f"{size / (1024 * 1024):.1f} Mio".replace(".", ",")


def summary(report: Report) -> str:
    lines = ["### Poids des zones (budget : 40 Mio par zone)", "",
             "| Lieu | Niveau | Poids | Part du budget |", "|---|---|---:|---:|"]
    for place, level, weight in report.weights:
        share = "—" if level.startswith("commun") else f"{weight / ZONE_BUDGET:.0%}"
        lines.append(f"| `{place}` | {level} | {mib(weight)} | {share} |")
    return "\n".join(lines) + "\n"


def main() -> int:
    report = check()
    text = summary(report)
    print(text)
    target = os.environ.get("GITHUB_STEP_SUMMARY")
    if target:
        with open(target, "a", encoding="utf-8") as handle:
            handle.write(text + "\n")
    for error in report.errors:
        print(error, file=sys.stderr)
    if report.errors:
        print(f"{len(report.errors)} écart(s) entre les assets HD et leurs manifestes", file=sys.stderr)
        return 1
    print(f"assets HD conformes : {len(report.weights)} lieu(x) pesé(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
