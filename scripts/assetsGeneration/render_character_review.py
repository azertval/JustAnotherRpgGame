#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Rend les planches de revue d'un personnage lié : ses clips, pose par pose, pour l'auteur.

Les contrôles de `check_character_model.py` disent qu'un modèle est conforme ; ils ne disent pas
qu'il est beau. Les gestes, les aisselles, les hanches et les vêtements en mouvement se jugent à
l'œil (`Planning/standards/personnages-3d.md`, §9). Ce script importe le `.glb` lié dans Blender
sans fenêtre — un second lecteur, indépendant de celui qui l'a écrit — et rend :

- une planche par clip : huit poses réparties sur sa durée, sous la **caméra du jeu**
  (orthographique, tournée de 45°, élevée de asin 0,62), puis de **profil**, où se lit la marche ;
- une planche de la pose de liaison : face, profil, dos.

Usage :
    python scripts/assetsGeneration/render_character_review.py MODELE.glb SORTIE
                                                               [--clips idle walk …] [--blender …]

Dépendances : Blender 5.2, Pillow (outil de production, pas de CI).
"""

from __future__ import annotations

import argparse
import json
import math
import subprocess
import sys
import tempfile
from pathlib import Path

BLENDER_DEFAUT = Path("D:/Blender Foundation/Blender 5.2/blender.exe")
CLIPS = ("idle", "walk", "attack", "cast", "hit", "death")
POSES = 8
ELEVATION = math.asin(0.62)
CELL = (300, 360)            # px d'une pose
WIDE_CELL = (560, 360)       # la chute : le corps couché déborde d'une case
PIXELS_PER_METRE = 150.0
GROUND = 300                 # la ligne de sol, en px depuis le haut de la cellule
FOND = (58, 60, 66)


def instants(duration: float, loop: bool, count: int = POSES) -> list[float]:
    """Les instants rendus : une boucle ne répète pas sa première pose, un geste montre sa fin."""
    if loop:
        return [duration * index / count for index in range(count)]
    return [duration * index / (count - 1) for index in range(count)]


def views() -> dict[str, dict]:
    """Les caméras, dans le repère de Blender (hauteur +Z, le personnage regarde vers −Y)."""
    cos_e, sin_e = math.cos(ELEVATION), math.sin(ELEVATION)
    diagonal = math.sqrt(0.5)
    return {
        # La caméra du jeu, placée devant et à droite du personnage : il regarde vers le sud-est.
        "jeu": {"toward": (-diagonal * cos_e, diagonal * cos_e, -sin_e)},
        "profil": {"toward": (-1.0, 0.0, 0.0)},
        "face": {"toward": (0.0, 1.0, 0.0)},
        "dos": {"toward": (0.0, -1.0, 0.0)},
    }


def _blender_main(parameters: dict) -> None:
    """Exécuté DANS Blender : rend chaque image demandée."""
    import bpy
    from mathutils import Vector

    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj)
    scene = bpy.context.scene
    scene.render.fps = 30
    bpy.ops.import_scene.gltf(filepath=parameters["model"])
    armature = next(o for o in bpy.data.objects if o.type == "ARMATURE")
    scene.render.engine = "BLENDER_EEVEE"
    scene.eevee.taa_render_samples = 16
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.resolution_percentage = 100
    scene.view_settings.view_transform = "Standard"
    world = bpy.data.worlds.new("monde")
    scene.world = world
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs[0].default_value = (0.5, 0.5, 0.52, 1.0)
    world.node_tree.nodes["Background"].inputs[1].default_value = 1.0
    for name, direction, energy in (("cle", (-0.4, 0.5, -0.8), 2.6), ("contre", (0.6, 0.3, -0.3), 0.9)):
        light = bpy.data.lights.new(name, "SUN")
        light.energy = energy
        sun = bpy.data.objects.new(name, light)
        scene.collection.objects.link(sun)
        sun.rotation_euler = Vector(direction).normalized().to_track_quat("-Z", "Y").to_euler()
    camera = bpy.data.objects.new("camera", bpy.data.cameras.new("camera"))
    scene.collection.objects.link(camera)
    scene.camera = camera
    camera.data.type = "ORTHO"
    camera.data.sensor_fit = "HORIZONTAL"
    camera.data.clip_start = 0.1
    camera.data.clip_end = 200.0

    actions = {action.name: action for action in bpy.data.actions}
    armature.animation_data_create()
    for job in parameters["jobs"]:
        animation = armature.animation_data
        for track in animation.nla_tracks:
            track.mute = True
        if job["clip"]:
            action = next((a for n, a in actions.items()
                           if n == job["clip"] or n.startswith(job["clip"] + "_")), None)
            if action is None:
                raise RuntimeError(f"animation absente : {job['clip']} ({', '.join(actions)})")
            animation.action = action
            if hasattr(animation, "action_slot") and animation.action_slot is None \
                    and len(action.slots):
                animation.action_slot = action.slots[0]
            start, end = action.frame_range
            moment = start + (end - start) * job["fraction"]
            whole = math.floor(moment)
            scene.frame_set(whole, subframe=moment - whole)
        else:
            animation.action = None
            for bone in armature.pose.bones:
                bone.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
                bone.location = (0.0, 0.0, 0.0)
            scene.frame_set(0)
        toward = Vector(job["toward"]).normalized()
        camera.rotation_euler = toward.to_track_quat("-Z", "Y").to_euler()
        up = camera.rotation_euler.to_matrix() @ Vector((0.0, 1.0, 0.0))
        right = camera.rotation_euler.to_matrix() @ Vector((1.0, 0.0, 0.0))
        width, height = job["size"]
        scene.render.resolution_x = width
        scene.render.resolution_y = height
        camera.data.ortho_scale = width / job["pixels_per_metre"]
        # Le point du sol sous le personnage tombe à (centre + décalage, ligne de sol).
        shift_x = job["shift"] / job["pixels_per_metre"]
        rise = (height / 2 - (height - job["ground"])) / job["pixels_per_metre"]
        # La hauteur monde projetée : un point élevé de h monte de h·|up.z| à l'écran.
        camera.location = (-toward * 50.0 - right * shift_x + up * rise)
        scene.render.filepath = job["file"]
        bpy.ops.render.render(write_still=True)


def find_blender(option: str | None) -> Path:
    import os
    for candidate in (option, os.environ.get("BLENDER"), str(BLENDER_DEFAUT)):
        if candidate and Path(candidate).is_file():
            return Path(candidate)
    raise SystemExit("Blender introuvable : --blender CHEMIN, ou la variable BLENDER")


def clips_of(model: Path) -> dict[str, tuple[float, bool]]:
    """La durée de chaque animation du modèle ; une boucle est celle que `skeleton.json` dirait,
    lue ici à ce que sa dernière pose répète la première : seuls `idle` et `walk` bouclent."""
    import struct
    data = model.read_bytes()
    length = struct.unpack_from("<I", data, 12)[0]
    document = json.loads(data[20:20 + length])
    found = {}
    for animation in document.get("animations", []):
        times = document["accessors"][animation["samplers"][0]["input"]]
        found[animation["name"]] = (float(times["max"][0]), animation["name"] in ("idle", "walk"))
    return found


def render(model: Path, output: Path, clips: list[str], blender: Path) -> list[Path]:
    from PIL import Image, ImageDraw

    available = clips_of(model)
    cameras = views()
    output.mkdir(parents=True, exist_ok=True)
    name = model.stem
    with tempfile.TemporaryDirectory(prefix="revue-") as temporary:
        jobs, sheets = [], []
        for clip in clips:
            if clip not in available:
                raise SystemExit(f"{model.name} n'a pas d'animation « {clip} »")
            duration, loop = available[clip]
            cell = WIDE_CELL if clip == "death" else CELL
            # Le corps tombe en arrière : la cellule large le garde entier, décalé du côté opposé.
            rows = []
            for view in ("jeu", "profil"):
                files = []
                shift = 0
                if clip == "death":
                    shift = 110 if view == "jeu" else -150
                for index, moment in enumerate(instants(duration, loop)):
                    file = str(Path(temporary) / f"{clip}-{view}-{index}.png")
                    jobs.append({"clip": clip, "fraction": moment / duration if duration else 0.0,
                                 "toward": cameras[view]["toward"], "size": cell,
                                 "pixels_per_metre": PIXELS_PER_METRE, "ground": GROUND,
                                 "shift": shift, "file": file})
                    files.append((file, shift, moment))
                rows.append((view, files))
            sheets.append((clip, cell, rows))
        # La pose de liaison, puis le repos bras baissés : là se lisent l'épaule et l'aisselle.
        rest = []
        for row, clip in enumerate(("", "idle")):
            for view in ("face", "profil", "dos", "jeu"):
                file = str(Path(temporary) / f"repos-{row}-{view}.png")
                jobs.append({"clip": clip, "fraction": 0.0, "toward": cameras[view]["toward"],
                             "size": (640, 620), "pixels_per_metre": 300.0, "ground": 580,
                             "shift": 0, "file": file})
                rest.append((row, view, file))
        parameters = Path(temporary) / "parametres.json"
        parameters.write_text(json.dumps({"model": str(model.resolve()), "jobs": jobs}),
                              encoding="utf-8")
        command = [str(blender), "-b", "--factory-startup", "-noaudio", "--python-exit-code", "1",
                   "--python", str(Path(__file__).resolve()), "--", "--blender-interne",
                   str(parameters)]
        result = subprocess.run(command, capture_output=True, text=True, encoding="utf-8",
                                errors="replace", check=False)
        if result.returncode != 0:
            sys.stderr.write(result.stdout[-4000:] + result.stderr[-4000:])
            raise SystemExit("le rendu de revue a échoué")

        written = []
        for clip, cell, rows in sheets:
            sheet = Image.new("RGB", (cell[0] * POSES, cell[1] * len(rows)), FOND)
            draw = ImageDraw.Draw(sheet)
            for row, (view, files) in enumerate(rows):
                for column, (file, _, moment) in enumerate(files):
                    picture = Image.open(file).convert("RGBA")
                    corner = (column * cell[0], row * cell[1])
                    if view == "profil":
                        draw.line([(corner[0], corner[1] + GROUND),
                                   (corner[0] + cell[0], corner[1] + GROUND)], fill=(120, 124, 132))
                    sheet.paste(picture, corner, picture)
                    draw.text((corner[0] + 6, corner[1] + 4), f"{view} {moment:.2f} s",
                              fill=(220, 220, 220))
            target = output / f"{name}-{clip}.png"
            sheet.save(target)
            written.append(target)
        board = Image.new("RGB", (640 * 4, 620 * 2), FOND)
        for index, (row, _, file) in enumerate(rest):
            picture = Image.open(file).convert("RGBA")
            board.paste(picture, ((index % 4) * 640, row * 620), picture)
        target = output / f"{name}-liaison.png"
        board.save(target)
        written.append(target)
    return written


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("model", type=Path, help="le .glb lié")
    parser.add_argument("output", type=Path, help="le dossier des planches")
    parser.add_argument("--clips", nargs="*", default=list(CLIPS))
    parser.add_argument("--blender", help="blender.exe")
    arguments = parser.parse_args()
    for path in render(arguments.model, arguments.output, arguments.clips,
                       find_blender(arguments.blender)):
        print(path)
    return 0


if __name__ == "__main__":
    if "--blender-interne" in sys.argv:
        _blender_main(json.loads(
            Path(sys.argv[sys.argv.index("--blender-interne") + 1]).read_text(encoding="utf-8")))
    else:
        sys.exit(main())
