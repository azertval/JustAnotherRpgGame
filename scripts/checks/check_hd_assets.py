#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Garde-fou : les assets HD installés, leurs manifestes et le poids de chaque zone (LOT-104).

Sous `Source/Elements/Assets/Common/` et `Source/Elements/Assets/Regions/`, chaque dossier `Scene/`
et `Characters/` porte un `manifest.json` qui liste ses pièces (arborescence, règle 3). Ce contrôle
rend impossible l'écart SILENCIEUX entre les deux :

- une entrée de manifeste qui cite un fichier absent ;
- une image que son manifeste ne cite pas — déposée à la main, ou restée d'une pièce renommée ;
- une image hors des bornes du standard (`Planning/standards/style-3d.md`, §7) : PNG 32 bits, taille
  égale à celle que le manifeste déclare, 4096 px de côté au plus, une dalle de sol exactement au
  losange du lieu, une ancre dans l'image, un losange de lieu égal à celui de sa région ;
- un **personnage** qui n'est pas ce que le standard en dit (`Planning/standards/personnages-3d.md`,
  LOT-1006) : un modèle sans fiche (`character.json`), une fiche qui cite un `.glb` ou un squelette
  que rien ne déclare, une description de squelette (`skeleton.json`) mal formée, un `.glb` installé
  qui ne passe pas les contrôles de l'export (`check_character_model.py`) — et **toute bande de
  figurine** restée sous un dossier `Characters/` : un personnage est un modèle, plus une suite
  d'images.

Une zone n'a **pas de budget de poids** (décision de l'auteur, 24 septembre 2026 : un jeu lourd
mais riche plutôt que des kits bridés). Son poids s'affiche, et s'écrit dans le résumé du job quand `GITHUB_STEP_SUMMARY`
est défini. Les assets installés s'écrivent par `scripts/assetsGeneration/install_hd_asset.py` ; ce contrôle n'a
pas besoin des sources, qui ne sont pas versionnées.

Aucune dépendance pour les images : l'en-tête PNG se lit à la main, comme dans
`check_ui_assets.py`. Le contrôle d'un `.glb` de personnage demande numpy ; sans lui, ou sans le
fichier (les kits ne sont pas installés), il est passé et le reste du contrôle a lieu.

Usage :
    python scripts/checks/check_hd_assets.py            # code de sortie non nul si écart
"""

from __future__ import annotations

import json
import os
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "Source" / "Elements" / "Assets"
WORLD_MAPS = ROOT / "Source" / "Elements" / "Maps" / "world-maps.json"

# Les deux arbres de l'arborescence HD ; `UI/`, `Maps/`, `Fonts/` et `Entities/` ont leurs contrôles.
TREES = ("Common", "Regions")
# Le losange du standard, pour le commun du monde qui n'a pas de région.
STANDARD_TILE = [256, 159]
MAX_SIDE = 4096
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


# Ce qu'un dossier de personnage porte en images : son portrait et son jeton, rien d'autre.
CHARACTER_IMAGES = ("portrait.png", "token.png")
SHEET = "character.json"
SKELETON = "skeleton.json"
SHEET_VERSION = 1
# Le dossier des squelettes, sous le dossier `Characters/` du commun du monde.
SKELETONS = ("Common", "Characters", "Skeletons")


def is_number(value) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def check_skeleton(path: Path, root: Path, report: Report) -> dict | None:
    """Une description de squelette : ses os parents avant enfants, ses clips, leur image clé.

    Les mêmes règles que le lecteur du moteur (`core::readSkeletonDescription`) : ce que ce contrôle
    laisse passer, le jeu le lit.
    """
    where = relative(path, root)
    data = read_json(path, report, root)
    if data is None:
        return None
    before = len(report.errors)
    if data.get("version") != SHEET_VERSION:
        report.fail(f"{where} : version {data.get('version')!r}, attendu {SHEET_VERSION}")
    if data.get("silhouette") != path.parent.name:
        report.fail(f"{where} : silhouette {data.get('silhouette')!r}, le dossier dit {path.parent.name!r}")
    bones = data.get("bones")
    known: set[str] = set()
    if not isinstance(bones, list) or not bones:
        report.fail(f"{where} : `bones` est une liste non vide")
        bones = []
    for bone in bones:
        name = bone.get("name") if isinstance(bone, dict) else None
        if not isinstance(name, str) or not name:
            report.fail(f"{where} : un os sans nom")
            continue
        parent = bone.get("parent", "")
        if parent and parent not in known:
            report.fail(f"{where} : l'os {name} cite le parent {parent!r}, pas déclaré avant lui")
        if name in known:
            report.fail(f"{where} : l'os {name} déclaré deux fois")
        known.add(name)
    clips = data.get("clips")
    names: set[str] = set()
    if not isinstance(clips, list):
        report.fail(f"{where} : `clips` est une liste")
        clips = []
    for clip in clips:
        name = clip.get("name") if isinstance(clip, dict) else None
        if not isinstance(name, str) or not name:
            report.fail(f"{where} : un clip sans nom")
            continue
        if name in names:
            report.fail(f"{where} : le clip {name} déclaré deux fois")
        names.add(name)
        duration = clip.get("duration")
        if not is_number(duration) or duration <= 0:
            report.fail(f"{where} : le clip {name} doit déclarer une durée positive")
            continue
        if "loop" in clip and not isinstance(clip["loop"], bool):
            report.fail(f"{where} : clip {name}, `loop` est un booléen")
        if "key" in clip and (not is_number(clip["key"]) or not 0 <= clip["key"] <= duration):
            report.fail(f"{where} : clip {name}, l'image clé {clip['key']!r} sort du clip ({duration} s)")
    return data if len(report.errors) == before else None


def check_model_file(path: Path, skeleton: dict | None, root: Path, report: Report) -> None:
    """Un `.glb` de personnage installé passe les contrôles de l'export (standard, §9).

    Par `check_character_model.inspect`, qui les tient : structure, poids, contact au sol, glissement
    du pied. Demande numpy ; sans lui, le fichier n'est pas ouvert — le contrôle des fiches a lieu
    quand même.
    """
    try:
        import check_character_model
    except ImportError:
        return
    try:
        measures = check_character_model.inspect(path.read_bytes(), skeleton)
    except Exception as error:  # noqa: BLE001 : un fichier illisible est un écart, pas une panne
        report.fail(f"{relative(path, root)} : modèle illisible ({error})")
        return
    for fault in measures.get("faults", []):
        report.fail(f"{relative(path, root)} : {fault}")


def check_sheet(folder: Path, models: dict, name: str, root: Path, report: Report,
                skeletons: dict[str, dict | None]) -> None:
    """La fiche d'un personnage en modèle : elle cite un `.glb` et un squelette déclarés."""
    where = relative(folder / SHEET, root)
    if not (folder / SHEET).is_file():
        report.fail(f"{relative(folder, root)} : personnage sans fiche `{SHEET}` (un personnage est un "
                    "modèle ; un visage sans modèle va dans `portraits`)")
        return
    sheet = read_json(folder / SHEET, report, root)
    if sheet is None:
        return
    if sheet.get("version") != SHEET_VERSION:
        report.fail(f"{where} : version {sheet.get('version')!r}, attendu {SHEET_VERSION}")
    model, skeleton = sheet.get("model"), sheet.get("skeleton")
    if not isinstance(model, str) or not model.endswith(".glb") or "/" in model or "\\" in model:
        report.fail(f"{where} : `model` nomme un .glb du dossier, lu {model!r}")
        return
    declared = models.get(name) if isinstance(models, dict) else None
    if not isinstance(declared, dict) or declared.get("model") != f"{name}/{model}":
        report.fail(f"{where} : le modèle {model} n'est pas déclaré sous `models` du manifeste")
    elif declared.get("skeleton") != skeleton:
        report.fail(f"{where} : squelette {skeleton!r}, le manifeste déclare {declared.get('skeleton')!r}")
    if not isinstance(skeleton, str) or not skeleton:
        report.fail(f"{where} : `skeleton` nomme une silhouette")
        return
    if skeleton not in skeletons:
        path = root.joinpath(*SKELETONS, skeleton, SKELETON)
        if path.is_file():
            skeletons[skeleton] = check_skeleton(path, root, report)
        else:
            skeletons[skeleton] = None
            report.fail(f"{where} : squelette `{skeleton}` sans {relative(path, root)}")
    # Le fichier n'est là que si le kit est installé : il est hors de Git, comme une image.
    if (folder / model).is_file():
        check_model_file(folder / model, skeletons[skeleton], root, report)


def check_no_strip(directory: Path, root: Path, report: Report) -> None:
    """Aucune bande de figurine sous un dossier `Characters/` (LOT-1006) : ni `.anim.json`, ni image
    autre qu'un portrait ou un jeton."""
    for path in sorted(directory.rglob("*")):
        if not path.is_file():
            continue
        if path.name.endswith(".anim.json"):
            report.fail(f"{relative(path, root)} : description d'une bande de figurine — un personnage "
                        "est un modèle, ses bandes se suppriment")
        elif path.suffix.lower() in IMAGES and path.name not in CHARACTER_IMAGES:
            report.fail(f"{relative(path, root)} : bande de figurine — un dossier de personnage ne porte "
                        "en images que son portrait et son jeton")


def check_characters(directory: Path, manifest: dict, root: Path, report: Report,
                     skeletons: dict[str, dict | None]) -> dict[Path, set[str]]:
    """Les personnages d'un dossier `Characters/` : `<pnj>/character.json` et son `.glb`,
    `portrait.png`, `token.png` (arborescence, règle 5). Un PNJ peut être rangé plus bas
    (`Heroes/brawler`). Rend, par dossier de PNJ, les noms d'images cités."""
    where = relative(directory / "manifest.json", root)
    npcs = manifest.get("npcs", [])
    models = manifest.get("models", {})
    if not isinstance(npcs, list) or not isinstance(models, dict):
        report.fail(f"{where} : `npcs` est une liste, `models` un objet")
        return {}
    for key in ("frame", "wideFrame", "ground", "animations"):
        if key in manifest:
            report.fail(f"{where} : `{key}` décrit des bandes de figurine, qui n'existent plus")
    check_no_strip(directory, root, report)
    cited: dict[Path, set[str]] = {}
    for npc in npcs:
        if not isinstance(npc, str) or not (directory / npc).is_dir():
            report.fail(f"{where} : PNJ {npc!r} sans dossier")
            continue
        cited[directory / npc] = set(CHARACTER_IMAGES)
        check_sheet(directory / npc, models, npc, root, report, skeletons)
    # Un modèle déclaré sous `skeleton` a sa fiche, donc son nom dans `npcs` ; un modèle sans
    # squelette (le mannequin d'une silhouette qui n'en a pas encore) attend, sans fiche.
    for name, entry in models.items():
        if isinstance(entry, dict) and "skeleton" in entry and name not in npcs:
            report.fail(f"{where} : le modèle {name} est lié à un squelette mais absent de `npcs`")
    # Un `.glb` installé que `models` ne déclare pas : déposé à la main, ou resté d'un renommage.
    declared = {entry.get("model") for entry in models.values() if isinstance(entry, dict)}
    for path in sorted(directory.rglob("*.glb")):
        if path.relative_to(directory).as_posix() not in declared:
            report.fail(f"{relative(path, root)} : modèle que `models` du manifeste ne déclare pas")
    # Les portraits d'attente (LOT-145) : un visage sans modèle, que le moteur dessine par le
    # mannequin de sa silhouette. Seuls le portrait et le jeton sont cités.
    portraits = manifest.get("portraits", [])
    if not isinstance(portraits, list):
        report.fail(f"{where} : `portraits` est une liste")
        return cited
    for name in portraits:
        if not isinstance(name, str) or not (directory / name).is_dir():
            report.fail(f"{where} : portrait d'attente {name!r} sans dossier")
            continue
        if name in npcs:
            report.fail(f"{where} : {name} est à la fois dans `npcs` et dans `portraits`")
            continue
        if not (directory / name / "portrait.png").is_file():
            report.fail(f"{relative(directory / name, root)} : portrait d'attente sans `portrait.png`")
        if (directory / name / SHEET).is_file():
            report.fail(f"{relative(directory / name, root)} : un portrait d'attente n'a pas de fiche ; "
                        "avec son modèle, il va dans `npcs`")
        cited[directory / name] = set(CHARACTER_IMAGES)
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
    # Les descriptions de squelette déjà lues, par silhouette ; rien pour une qui ne se lit pas.
    skeletons: dict[str, dict | None] = {}
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
                # Un kit rangé en sous-dossiers (LOT-129) : chaque image est citée dans le sien.
                for file in check_scene(directory, manifest, root, report):
                    image = directory / file
                    cited.setdefault(image.parent, set()).add(image.name)
            elif "npcs" in manifest:
                cited.update(check_characters(directory, manifest, root, report, skeletons))

        for path in sorted(base.rglob(SKELETON)):
            if path.parent.name not in skeletons:
                skeletons[path.parent.name] = check_skeleton(path, root, report)

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
    return report


def mib(size: int) -> str:
    return f"{size / (1024 * 1024):.1f} Mio".replace(".", ",")


def summary(report: Report) -> str:
    lines = ["### Poids des zones (sans budget, pour mémoire)", "",
             "| Lieu | Niveau | Poids |", "|---|---|---:|"]
    for place, level, weight in report.weights:
        lines.append(f"| `{place}` | {level} | {mib(weight)} |")
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
