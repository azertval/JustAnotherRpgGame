"""Build a complete LOT-105 visual validation map using installed anchors unchanged."""

import json
import os
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "Tools/AssetsHD/Regions/central-empire/capital/Common/apercus"


def main():
    assets = {}
    for place in ["capital/Common", "Common"]:
        directory = ROOT / "Source/Elements/Assets/Regions/central-empire" / place / "Scene"
        manifest = json.loads((directory / "manifest.json").read_text(encoding="utf-8"))
        for entry in manifest["textures"].values():
            path = directory / entry["file"]
            assets[path.stem] = dict(entry, url=Path(os.path.relpath(path, OUT)).as_posix())
    pieces = []

    def put(name, x, y, zone):
        assert name in assets
        pieces.append(dict(name=name, x=x, y=y, zone=zone))

    zones = [
        dict(name="A · Sols et bordures", x=0, y=0, w=8, h=7),
        dict(name="B · Murs U / V et angles", x=10, y=0, w=10, h=9),
        dict(name="C · Balustrades et haies", x=0, y=10, w=9, h=10),
        dict(name="D · Mobilier et emblèmes", x=11, y=11, w=9, h=9),
    ]
    for z, zone in enumerate(zones):
        for x in range(zone["x"], zone["x"] + zone["w"]):
            for y in range(zone["y"], zone["y"] + zone["h"]):
                stem = "floor-flagstone" if (z == 0 and x >= 4) or z == 3 else "floor-paving"
                name = f"{stem}-0{((x * 73856093) ^ (y * 19349663)) % 3 + 1}"
                if z == 0:
                    if y == 0:
                        name = "floor-paving-edge-ne"
                    elif x == 0:
                        name = "floor-paving-edge-nw"
                    elif y == 6:
                        name = "floor-paving-edge-sw"
                    elif x == 7:
                        name = "floor-paving-edge-se"
                put(name, x, y, z)
    # The inner corner owns the junction cell; straight runs start at its two ends.
    put("wall-limestone-corner-inner", 10, 0, 1)
    for i, n in enumerate(
        [
            "wall-limestone-u",
            "wall-limestone-window-u",
            "wall-limestone-banner-u",
            "wall-limestone-u",
        ]
    ):
        put(n, 11 + i * 2, 0, 1)
    for i, n in enumerate(
        [
            "wall-limestone-v",
            "wall-limestone-window-v",
            "wall-limestone-banner-v",
            "wall-limestone-v",
        ]
    ):
        put(n, 10, 1 + i * 2, 1)
    # Both corner ports now meet straight modules on whole-cell placements.
    put("wall-limestone-corner-outer", 17, 6, 1)
    put("wall-limestone-u", 15, 6, 1)
    put("wall-limestone-v", 17, 4, 1)
    put("balustrade-limestone-corner-inner", 0, 10, 2)
    for x in [1, 3, 5]:
        put("balustrade-limestone-u", x, 10, 2)
    for y in [11, 13, 15]:
        put("balustrade-limestone-v", 0, y, 2)
    for x, y in [(7, 10), (0, 17)]:
        put("balustrade-limestone-pillar", x, y, 2)
    put("plant-hedge-corner", 3, 13, 2)
    for x in [4, 6]:
        put("plant-hedge-u", x, 13, 2)
    for y in [14]:
        put("plant-hedge-v", 3, y, 2)
    put("plant-flowerbed", 4, 16, 2)
    put("balustrade-limestone-corner-outer", 2, 19, 2)
    put("balustrade-limestone-u", 0, 19, 2)
    put("balustrade-limestone-v", 2, 17, 2)
    put("plant-hedge-corner-outer", 7, 17, 2)
    put("plant-hedge-u", 5, 17, 2)
    put("plant-hedge-v", 7, 15, 2)
    names = [
        "plant-cypress",
        "prop-lamppost",
        "prop-banner-lion",
        "column-lion-emblem",
        "prop-bench-u",
        "prop-bench-v",
        "prop-stall-empty",
        "prop-planter",
        "prop-barrel",
        "prop-crate",
    ]
    for i, n in enumerate(names):
        put(n, 12 + (i % 3) * 3, 12 + (i // 3) * 2, 3)
    assert set(p["name"] for p in pieces) == set(assets), "Missing assets"
    assert all(isinstance(p["x"], int) and isinstance(p["y"], int) for p in pieces)
    data = dict(tile=[256, 159], width=21, height=21, zones=zones, assets=assets, placements=pieces)
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / "validation-map.json").write_text(
        json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    template = (Path(__file__).with_name("capital_validation.html")).read_text(encoding="utf-8")
    (OUT / "validation.html").write_text(
        template.replace("__DATA__", json.dumps(data, ensure_ascii=False)), encoding="utf-8"
    )
    canvas = Image.new("RGBA", (5600, 3900), "#252d30")

    def point(x, y):
        return 2800 + (x - y) * 128, 430 + (x + y) * 79.5

    images = {n: Image.open(OUT / a["url"]).convert("RGBA") for n, a in assets.items()}
    ordered = sorted(
        pieces, key=lambda p: (assets[p["name"]]["class"] != "floor", p["x"] + p["y"], p["y"])
    )
    for p in ordered:
        a = assets[p["name"]]
        x, y = point(p["x"], p["y"])
        canvas.alpha_composite(
            images[p["name"]], (round(x - a["anchor"][0]), round(y - a["anchor"][1]))
        )
    canvas.convert("RGB").save(OUT / "validation-map.png")
    # Small contact overview for reviewing the whole map without changing its source PNG.
    canvas.thumbnail((1600, 1200), Image.Resampling.LANCZOS)
    canvas.convert("RGB").save(OUT / "validation-overview.jpg", quality=92)
    print(
        f"{len(assets)} / {len(assets)} assets; {len(pieces)} placements; {OUT / 'validation.html'}"
    )


if __name__ == "__main__":
    main()
