"""Prepare an isolated engine fixture; LevelEditor applies and renders the map itself."""

import json
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
V4 = ROOT / "Tools/AssetsHD/Regions/central-empire/capital/Common/V4"
ENGINE = V4 / "Engine"
EXE = ROOT / "build/ninja/bin/LevelEditor.exe"


def main():
    scene = ENGINE / "Assets/Scene/lot105-v4"
    scene.mkdir(parents=True, exist_ok=True)
    shutil.copytree(V4 / "Scene", scene, dirs_exist_ok=True)
    (scene / "appearance.json").write_text(
        json.dumps(
            dict(
                version=1,
                place="lot105-v4",
                diamondRatio=159 / 256,
                floors={"pavement": ["floor-paving-01"], "flagstone": ["floor-flagstone-01"]},
                relief={},
            )
        ),
        encoding="utf-8",
    )
    levels = ENGINE / "Levels"
    levels.mkdir(exist_ok=True)
    loc = ENGINE / "Localization"
    loc.mkdir(exist_ok=True)
    for lang in ["fr", "en"]:
        (loc / (lang + ".lang")).write_text(
            "map.validation.name = LOT-105 V4 validation\n", encoding="utf-8"
        )
    layout = json.loads((V4.parent / "apercus/validation-map.json").read_text(encoding="utf-8"))
    seed = dict(
        version=4,
        name="map.validation.name",
        width=25,
        height=25,
        tiles=[
            dict(x=x, y=y, type="entry" if (x, y) == (0, 0) else "wall")
            for y in range(25)
            for x in range(25)
        ],
        forced=[dict(x=0, y=0)],
        layers=[
            dict(name="sol", kind="ground", scene="lot105-v4", tiles=[]),
            dict(name="relief", kind="decor", tiles=[]),
        ],
        entities=[],
    )
    (levels / "validation.json").write_text(json.dumps(seed), encoding="utf-8")
    gestures = []
    for p in layout["placements"]:
        a = layout["assets"][p["name"]]
        gestures.append(
            dict(
                layer="sol" if a["class"] == "floor" else "relief",
                piece=p["name"],
                tool="paint",
                path=[[p["x"] + 2, p["y"] + 2]],
            )
        )
    (ENGINE / "gestures.json").write_text(
        json.dumps(
            dict(format="jadg-editor-gestures", version=1, map="validation", gestures=gestures),
            indent=2,
        ),
        encoding="utf-8",
    )
    log = []
    for args in [
        ["--apply", str(ENGINE / "gestures.json")],
        ["--check"],
        [
            "--render",
            "validation",
            "--layers",
            "floors,relief",
            "--scale",
            "1",
            "--output",
            str(V4 / "engine-validation.png"),
        ],
        [
            "--render",
            "validation",
            "--layers",
            "floors,relief",
            "--scale",
            "2",
            "--output",
            str(V4 / "engine-validation-2x.png"),
        ],
    ]:
        result = subprocess.run([str(EXE), "--data", str(ENGINE), *args], capture_output=True)
        out = result.stdout.decode("utf-8", errors="replace") + result.stderr.decode(
            "utf-8", errors="replace"
        )
        log.append(dict(command=args, returncode=result.returncode, output=out))
        print(out)
        if result.returncode:
            (V4 / "engine-check.json").write_text(json.dumps(log, indent=2), encoding="utf-8")
            raise SystemExit(result.returncode)
    (V4 / "engine-check.json").write_text(json.dumps(log, indent=2), encoding="utf-8")
    # Ensure the editor retained every required piece, including the real corner pieces.
    loaded = json.loads((levels / "validation.json").read_text())
    present = {t.get("piece") for l in loaded["layers"] for t in l["tiles"]}
    required = {p["name"] for p in layout["placements"]}
    assert required <= present, required - present
    print("Engine map coverage:", len(required), "pieces")


if __name__ == "__main__":
    main()
