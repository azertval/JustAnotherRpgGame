# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Garde-fou : les cartes de l'ecran « Carte », leur manifeste et leurs positions ne divergent pas.

`Source/Elements/Assets/Maps/` porte les cartes PEINTES PAR L'AUTEUR -- le monde, les treize regions,
les plans de ville --, en 1920 x 1080 et sans lettrage (`LOT-94`, `LOT-95`). Aucune n'est une image
du corpus : le plan de la ville et la carte du monde du livre sont des oeuvres, que le jeu n'affiche
pas (`EX-IHM-076`). Le manifeste (`manifest.json`) dit d'ou vient chaque fichier et fige son
empreinte ; `Source/Elements/Maps/world-maps.json` pose dessus les regions, les lieux de l'atlas
(`LOT-37`) et les quartiers.

Ce que ce controle rend impossible, c'est l'ecart SILENCIEUX :

- une carte retouchee ou remplacee sans repasser par le manifeste (empreinte, taille) ;
- une image deposee a la main, ou une entree du manifeste sans fichier (orpheline) ;
- une provenance autre que `author` -- `tanares`, celle du corpus, en tete ;
- une carte qui n'est pas en 1920 x 1080 : les positions sont des fractions, mais la vue couvre
  l'ecran en 16:9 et la mini-carte suppose ce rapport ;
- `world-maps.json` qui nomme une image absente du manifeste, une region ou un lieu que l'atlas ne
  connait pas, une region de l'atlas sans carte, une ville a plan sans repere sur sa region.

Les cartes RENDUES des zones (`LOT-121`) ne sont pas des cartes peintes : `LevelEditor --render
--canvas 1920x1080` les tire de la carte de niveau, et elles se rangent avec leur zone
(`Assets/Regions/<...>/Map/`), dans son kit. `world-maps.json` les nomme par un chemin relatif a
`Assets/`, avec leur grille (`grid`). Ce controle exige qu'elles soient dans un dossier `Map/` de
`Regions/`, en JPEG 1920 x 1080, et que leur grille soit complete ; la sous-zone d'un quartier
(`zones`) de meme, avec son entree en cases.

Aucune dependance : hashlib, et une lecture d'en-tete JPEG (celle de `check_ui_assets.py`).

Usage :
    python scripts/checks/check_map_assets.py            # code de sortie non nul si divergence
    python scripts/checks/check_map_assets.py --write    # reecrit tailles et empreintes du manifeste
"""

from __future__ import annotations

import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "Source" / "Elements" / "Assets"
MAPS = ASSETS / "Maps"
MANIFEST = MAPS / "manifest.json"
WORLD_MAPS = ROOT / "Source" / "Elements" / "Maps" / "world-maps.json"
ATLAS = ROOT / "Source" / "Elements" / "World"
EXPECTED_SIZE = [1920, 1080]

errors: list[str] = []
# Quartiers dont la vue agrandit encore le plan de leur ville, faute de carte peinte (LOT-96).
pending_districts: list[str] = []


def fail(message: str) -> None:
    errors.append(message)


def jpeg_size(data: bytes) -> tuple[int, int]:
    """Largeur et hauteur d'un JPEG, lues dans son premier marqueur de cadre (SOFn)."""
    if data[:2] != b"\xff\xd8":
        raise ValueError("signature JPEG absente")
    offset = 2
    while offset + 4 <= len(data):
        if data[offset] != 0xFF:
            raise ValueError(f"marqueur attendu au decalage {offset}")
        marker = data[offset + 1]
        if 0xC0 <= marker <= 0xCF and marker not in (0xC4, 0xC8, 0xCC):
            height, width = struct.unpack(">HH", data[offset + 5 : offset + 9])
            return width, height
        (length,) = struct.unpack(">H", data[offset + 2 : offset + 4])
        offset += 2 + length
    raise ValueError("aucun marqueur de cadre (SOFn) dans le fichier")


def describe(path: Path) -> dict:
    data = path.read_bytes()
    width, height = jpeg_size(data)
    return {"size": [width, height], "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}


def write_manifest(manifest: dict) -> None:
    for entry in manifest["maps"]:
        path = MAPS / entry["file"]
        if path.is_file():
            entry.update(describe(path))
    MANIFEST.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(f"check_map_assets : {MANIFEST.relative_to(ROOT)} reecrit ({len(manifest['maps'])} cartes).")


def check_manifest(manifest: dict) -> set[str]:
    declared: set[str] = set()
    for entry in manifest["maps"]:
        name = entry["file"]
        if name in declared:
            fail(f"`{name}` declare deux fois dans le manifeste")
        declared.add(name)
        if entry.get("provenance") != "author":
            fail(
                f"`{name}` : provenance '{entry.get('provenance')}' interdite. Une carte du jeu est "
                f"peinte par l'auteur, jamais extraite du corpus (EX-IHM-076, LOT-94)."
            )
        if not entry.get("source") or not entry.get("date"):
            fail(f"`{name}` : fichier d'origine ('source') ou date de livraison absents")
        path = MAPS / name
        if not path.is_file():
            fail(f"`{name}` absent de {MAPS.relative_to(ROOT)}")
            continue
        try:
            actual = describe(path)
        except (ValueError, struct.error) as error:
            fail(f"`{name}` n'est pas un JPEG lisible ({error})")
            continue
        for field in ("size", "bytes", "sha256"):
            if actual[field] != entry.get(field):
                fail(
                    f"`{name}` : {field} ne suit plus le manifeste. Si la carte a ete relivree :\n"
                    f"    python scripts/checks/check_map_assets.py --write"
                )
                break
        if actual["size"] != EXPECTED_SIZE:
            fail(f"`{name}` : {actual['size']}, {EXPECTED_SIZE} attendus")
    for path in sorted(MAPS.iterdir()):
        if path.suffix.lower() in (".jpg", ".jpeg", ".png") and path.name not in declared:
            fail(f"{path.name} n'est declare par aucune entree du manifeste")
    return declared


def is_point(value: object) -> bool:
    return (
        isinstance(value, list)
        and len(value) == 2
        and all(isinstance(number, (int, float)) and 0 <= number <= 1 for number in value)
    )


def is_pair(value: object) -> bool:
    return isinstance(value, list) and len(value) == 2 and all(isinstance(n, (int, float)) for n in value)


def check_rendered(owner: str, entry: dict) -> None:
    """Une carte rendue (LOT-121) : un JPEG 1920 x 1080 dans un `Map/` de `Regions/`, et sa grille."""
    image = entry["image"]
    path = ASSETS / image
    if not image.startswith("Regions/") or path.parent.name != "Map":
        fail(f"world-maps.json : `{owner}` : une carte rendue se range dans le `Map/` de sa zone, pas `{image}`")
    elif not path.is_file():
        fail(f"world-maps.json : `{owner}` : `{image}` absent (kit a installer : scripts/fetch_assets.py)")
    else:
        try:
            size = list(jpeg_size(path.read_bytes()))
        except (ValueError, struct.error) as error:
            fail(f"`{image}` n'est pas un JPEG lisible ({error})")
        else:
            if size != EXPECTED_SIZE:
                fail(f"`{image}` : {size}, {EXPECTED_SIZE} attendus")
    grid = entry.get("grid")
    if not (isinstance(grid, dict) and all(is_pair(grid.get(k)) for k in ("origin", "column", "row"))):
        fail(f"world-maps.json : `{owner}` : carte rendue sans grille complete (origin, column, row)")


def check_world_maps(declared: set[str]) -> None:
    document = json.loads(WORLD_MAPS.read_text(encoding="utf-8"))
    regions = {path.stem: json.loads(path.read_text(encoding="utf-8")) for path in (ATLAS / "regions").glob("*.json")}
    locations = {path.stem: json.loads(path.read_text(encoding="utf-8")) for path in (ATLAS / "locations").glob("*.json")}

    used = {document["world"]["image"]}
    for region_id in sorted(set(regions) - set(document["regions"])):
        fail(f"la region `{region_id}` de l'atlas n'a pas de carte dans world-maps.json")
    for region_id, region in document["regions"].items():
        used.add(region["image"])
        if region_id not in regions:
            fail(f"world-maps.json : la carte `{region_id}` ne designe aucune region de l'atlas")
            continue
        if not is_point(region.get("anchor")):
            fail(f"world-maps.json : `{region_id}` : repere hors de [0, 1]")
        for place_id, position in region.get("places", {}).items():
            if locations.get(place_id, {}).get("region") != region_id:
                fail(f"world-maps.json : `{place_id}` n'est pas un lieu de `{region_id}`")
            if not is_point(position):
                fail(f"world-maps.json : `{place_id}` : position hors de [0, 1]")
        for omitted in region.get("omitted", []):
            if locations.get(omitted, {}).get("region") != region_id:
                fail(f"world-maps.json : l'entree ecartee `{omitted}` n'est pas un lieu de `{region_id}`")
            if omitted in region.get("places", {}):
                fail(f"world-maps.json : `{omitted}` est a la fois pose et ecarte")

    for city_id, city in document.get("cities", {}).items():
        used.add(city["image"])
        location = locations.get(city_id)
        if location is None:
            fail(f"world-maps.json : le plan `{city_id}` ne designe aucun lieu de l'atlas")
            continue
        if city_id not in document["regions"].get(location["region"], {}).get("places", {}):
            fail(f"world-maps.json : la ville `{city_id}` a un plan mais aucun repere sur sa region")
        for place_id in city.get("places", {}):
            if place_id not in locations:
                fail(f"world-maps.json : le quartier `{place_id}` n'est pas un lieu de l'atlas")
        # La vue d'un quartier (LOT-96) : un cadre sur le plan, et plus tard sa carte peinte.
        for district_id, district in city.get("districts", {}).items():
            if district_id not in city.get("places", {}):
                fail(f"world-maps.json : le quartier `{district_id}` a une vue mais aucun repere sur son plan")
            frame = district.get("frame")
            if not (isinstance(frame, list) and len(frame) == 4
                    and all(isinstance(n, (int, float)) and 0 <= n <= 1 for n in frame)
                    and frame[2] > 0 and frame[3] > 0
                    and frame[0] + frame[2] <= 1 and frame[1] + frame[3] <= 1):
                fail(f"world-maps.json : `{district_id}` : cadre hors du plan")
            if district.get("image") and "/" in district["image"]:
                check_rendered(district_id, district)
            elif district.get("image"):
                used.add(district["image"])
            else:
                # Pas une faute : la decision provisoire du LOT-96, que ce rappel garde visible.
                pending_districts.append(district_id)
            # Ses sous-zones (LOT-121, D-16) : un nom, une entree en cases, leur carte rendue.
            for zone_id, zone in district.get("zones", {}).items():
                owner = f"{district_id} / {zone_id}"
                if not zone.get("name") or not is_pair(zone.get("entrance")):
                    fail(f"world-maps.json : `{owner}` : une sous-zone veut un nom et une entree en cases")
                if zone.get("image"):
                    check_rendered(owner, zone)

    for name in sorted(used - declared):
        fail(f"world-maps.json nomme `{name}`, absent du manifeste des cartes")
    for name in sorted(declared - used):
        fail(f"`{name}` est livre mais aucune vue de world-maps.json ne le montre")


def main() -> None:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if "--write" in sys.argv[1:]:
        write_manifest(manifest)
        return
    declared = check_manifest(manifest)
    check_world_maps(declared)
    if errors:
        for message in errors:
            print(f"check_map_assets : {message}", file=sys.stderr)
        sys.exit(1)
    print(f"check_map_assets : {len(declared)} carte(s) conforme(s) au manifeste, a l'atlas et a world-maps.json.")
    if pending_districts:
        print("check_map_assets : quartier(s) montre(s) par un agrandissement du plan, en attendant "
              "leur carte peinte (LOT-96, provisoire) : " + ", ".join(sorted(pending_districts)) + ".")


if __name__ == "__main__":
    main()
