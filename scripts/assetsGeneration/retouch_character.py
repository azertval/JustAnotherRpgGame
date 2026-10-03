#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Aller-retour par Blender d'un personnage lié : régler ses articulations et ses clips à la souris.

Décision de l'auteur du 2 octobre 2026 (D-44, `LOT-1008`) : le squelette et les mouvements d'un
personnage se travaillent avec précision dans Blender, et l'atelier des assets fait l'aller-retour.
Blender n'est que l'**instrument de saisie** : ce qui en revient est une **donnée**, jamais un
maillage ni un `.glb` exporté par lui (son export arrondit les durées à l'image, ajoute des canaux
d'échelle et une interpolation que le moteur ne lit pas — mesuré sur le bandit).

- `open` : ouvre le modèle lié dans Blender — maillage, squelette de sa silhouette (53 os pour
  l'humanoïde, 29 pour le quadrupède), une action par clip, à 60 images par seconde —,
  enregistre le `.blend` et, à côté, un **repère** : les articulations
  et les clips tels que Blender les lit avant toute retouche. Puis lance Blender sur ce fichier.
- `import` : relit le `.blend` enregistré, le compare au repère, et n'en retient que ce que
  l'auteur a changé :
  - une **articulation déplacée** (mode Édition de l'armature) est reportée dans la fiche de
    liaison (`liaison.json`) : les poids se recalculent d'après elle ;
  - un **clip modifié** (ses clés, en mode Pose) est écrit dans la fiche de retouche
    (`retouche.json`), échantillonné au pas de la chaîne : rotations locales des os animés et
    position du bassin. Durée, boucle et image clé restent celles de `skeleton.json`.
  Avec `--source` et `--output`, le modèle est relié dans la foulée par `rig_character.py`, puis
  contrôlé par `check_character_model.py` : un écart au standard fait échouer l'import.

Ce que Blender ne peut pas dire au jeu est **signalé**, pas importé : un os translaté ou mis à
l'échelle dans un clip, un doigt animé, la racine déplacée, le maillage sculpté ou repeint.

Usage :
    python scripts/assetsGeneration/retouch_character.py open MODELE.glb [--blend FICHIER.blend]
        [--skeleton skeleton.json] [--blender CHEMIN] [--no-window]
    python scripts/assetsGeneration/retouch_character.py import FICHIER.blend
        --sheet liaison.json --retouch retouche.json [--skeleton skeleton.json]
        [--source RECU.glb --output MODELE.glb] [--blender CHEMIN]

Dépendances : numpy ; Blender 5.2 (outil de production, pas de CI).
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path

RETOUCH_VERSION = 1
JOINT_TOLERANCE = 1.0e-4       # m : en deçà, une articulation n'a pas bougé
CURVE_TOLERANCE = 1.0e-4       # m, ou écart de quaternion : en deçà, un clip n'a pas changé
STRAY_TOLERANCE = 1.0e-3       # m : un os translaté dans un clip, que le format ne porte pas
SCALE_TOLERANCE = 1.0e-3


# --- Côté Blender ----------------------------------------------------------------------------------
def _blender_extract(parameters: dict) -> dict:
    """Exécuté DANS Blender : les articulations de repos et chaque clip, échantillonné."""
    import bpy
    import numpy as np

    armatures = [item for item in bpy.data.objects if item.type == "ARMATURE"]
    if len(armatures) != 1:
        raise SystemExit(f"{len(armatures)} armature(s) dans le fichier : il en faut une")
    armature = armatures[0]
    bones = parameters["bones"]
    missing = [bone["name"] for bone in bones if bone["name"] not in armature.data.bones]
    if missing:
        raise SystemExit("os absents de l'armature : " + ", ".join(missing))
    extra = sorted(set(armature.data.bones.keys()) - {bone["name"] for bone in bones})
    parent_of = {bone["name"]: bone["parent"] for bone in bones}
    names = [bone["name"] for bone in bones]

    # Blender a la hauteur vers +Z, le modèle vers +Y : (x, y, z) devient (x, z, -y).
    axes = np.array([[1.0, 0.0, 0.0], [0.0, 0.0, 1.0], [0.0, -1.0, 0.0]])
    world = np.array(armature.matrix_world)

    def point(vector) -> np.ndarray:
        return axes @ (world @ np.array([*vector, 1.0]))[:3]

    rest = {name: world @ np.array(armature.data.bones[name].matrix_local) for name in names}
    inverse_rest = {name: np.linalg.inv(matrix) for name, matrix in rest.items()}
    head = {name: (world @ np.array([*armature.data.bones[name].head_local, 1.0]))
            for name in names}
    result = {"joints": {name: [float(v) for v in point(armature.data.bones[name].head_local)]
                         for name in names},
              "extra_bones": extra, "clips": {}, "missing_clips": []}

    scene = bpy.context.scene
    rate = scene.render.fps / scene.render.fps_base
    if armature.animation_data is None:
        armature.animation_data_create()
    data = armature.animation_data
    data.use_nla = False
    animated = parameters["animated"]
    for clip in parameters["clips"]:
        action = bpy.data.actions.get(clip["name"])
        if action is None:
            result["missing_clips"].append(clip["name"])
            continue
        data.action = action
        slots = getattr(action, "slots", None)
        if slots and getattr(data, "action_slot", None) is None:
            data.action_slot = slots[0]
        start = float(action.frame_range[0])
        times, pelvis, stray, scale = [], [], 0.0, 0.0
        rotations = {name: [] for name in animated}
        count = round(clip["duration"] * parameters["samples_per_second"])
        for index in range(count + 1):
            time = index / parameters["samples_per_second"]
            frame = start + time * rate
            whole = int(np.floor(frame + 1.0e-6))
            scene.frame_set(whole, subframe=float(min(max(frame - whole, 0.0), 0.999999)))
            delta, turn = {}, {}
            for name in names:
                pose = world @ np.array(armature.pose.bones[name].matrix)
                delta[name] = pose @ inverse_rest[name]
                linear = delta[name][:3, :3]
                lengths = np.linalg.norm(linear, axis=0)
                scale = max(scale, float(np.abs(lengths - 1.0).max()))
                # La rotation la plus proche, sans l'échelle.
                u, _, vt = np.linalg.svd(linear)
                turn[name] = axes @ (u @ vt) @ axes.T
            for name in names:
                parent = parent_of[name]
                if not parent or name == "pelvis":
                    continue
                expected = delta[parent] @ head[name]
                actual = delta[name] @ head[name]
                stray = max(stray, float(np.linalg.norm((actual - expected)[:3])))
            times.append(time)
            pelvis.append([float(v) for v in axes @ (delta["pelvis"] @ head["pelvis"])[:3]])
            for name in animated:
                parent = parent_of[name]
                local = turn[name] if parent == "Root" else turn[parent].T @ turn[name]
                rotations[name].append(_quaternion(np, local))
        fingers = max((float(np.abs(delta[name][:3, :3] - delta[parent_of[name]][:3, :3]).max())
                       for name in names
                       if name not in animated and name != "Root"), default=0.0)
        result["clips"][clip["name"]] = {
            "times": times, "pelvis": pelvis, "rotations": rotations,
            "stray_translation": stray, "scale_error": scale, "unanimated_error": fingers,
            "frames": [float(action.frame_range[0]), float(action.frame_range[1])]}
    return result


def _quaternion(np, matrix) -> list[float]:
    """Le quaternion (x, y, z, w) d'une rotation, w positif."""
    m = matrix
    trace = m[0, 0] + m[1, 1] + m[2, 2]
    if trace > 0.0:
        s = float(np.sqrt(trace + 1.0)) * 2.0
        q = [(m[2, 1] - m[1, 2]) / s, (m[0, 2] - m[2, 0]) / s, (m[1, 0] - m[0, 1]) / s, 0.25 * s]
    elif m[0, 0] > m[1, 1] and m[0, 0] > m[2, 2]:
        s = float(np.sqrt(1.0 + m[0, 0] - m[1, 1] - m[2, 2])) * 2.0
        q = [0.25 * s, (m[0, 1] + m[1, 0]) / s, (m[0, 2] + m[2, 0]) / s, (m[2, 1] - m[1, 2]) / s]
    elif m[1, 1] > m[2, 2]:
        s = float(np.sqrt(1.0 + m[1, 1] - m[0, 0] - m[2, 2])) * 2.0
        q = [(m[0, 1] + m[1, 0]) / s, 0.25 * s, (m[1, 2] + m[2, 1]) / s, (m[0, 2] - m[2, 0]) / s]
    else:
        s = float(np.sqrt(1.0 + m[2, 2] - m[0, 0] - m[1, 1])) * 2.0
        q = [(m[0, 2] + m[2, 0]) / s, (m[1, 2] + m[2, 1]) / s, 0.25 * s, (m[1, 0] - m[0, 1]) / s]
    q = np.array(q, dtype=float)
    q /= np.linalg.norm(q)
    if q[3] < 0.0:
        q = -q
    return [float(v) for v in q]


def _blender_open(parameters: dict) -> dict:
    """Exécuté DANS Blender : importe le modèle lié, range la scène, enregistre le `.blend`."""
    import bpy

    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    # Le pas de la chaîne : chaque échantillon d'un clip tombe sur une image entière.
    scene.render.fps = parameters["samples_per_second"]
    scene.render.fps_base = 1.0
    bpy.ops.import_scene.gltf(filepath=parameters["model"])
    armature = next(item for item in bpy.data.objects if item.type == "ARMATURE")
    skinned = {item for item in bpy.data.objects if item.type == "MESH" and item.parent is armature
               or any(m.type == "ARMATURE" for m in getattr(item, "modifiers", []))}
    # L'importateur ajoute la forme qu'il donne aux os : elle n'est pas du personnage.
    for item in list(bpy.data.objects):
        if item is not armature and item not in skinned:
            bpy.data.objects.remove(item, do_unlink=True)
    for bone in armature.pose.bones:
        bone.custom_shape = None
    armature.show_in_front = True
    armature.data.display_type = "STICK"
    data = armature.animation_data or armature.animation_data_create()
    for track in list(data.nla_tracks):
        data.nla_tracks.remove(track)
    longest = 1
    for clip in parameters["clips"]:
        action = bpy.data.actions.get(clip["name"])
        if action is None:
            raise SystemExit(f"le modèle n'a pas de clip {clip['name']}")
        action.use_fake_user = True
        longest = max(longest, round(clip["duration"] * parameters["samples_per_second"]))
    first = bpy.data.actions[parameters["clips"][0]["name"]]
    data.action = first
    slots = getattr(first, "slots", None)
    if slots and getattr(data, "action_slot", None) is None:
        data.action_slot = slots[0]
    scene.frame_start = 0
    scene.frame_end = longest
    scene.frame_set(0)
    bpy.ops.object.select_all(action="DESELECT")
    armature.select_set(True)
    bpy.context.view_layer.objects.active = armature
    extracted = _blender_extract(parameters)
    data.action = first
    scene.frame_set(0)
    bpy.ops.wm.save_as_mainfile(filepath=parameters["blend"], check_existing=False)
    return extracted


def _blender_main(parameters: dict) -> None:
    import bpy

    if parameters["job"] == "open":
        extracted = _blender_open(parameters)
    else:
        bpy.ops.wm.open_mainfile(filepath=parameters["blend"])
        extracted = _blender_extract(parameters)
    Path(parameters["result"]).write_text(json.dumps(extracted), encoding="utf-8")


# --- Côté poste ------------------------------------------------------------------------------------
def _chain():
    """Les modules de la chaîne, chargés hors de Blender seulement."""
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "checks"))
    import check_character_model
    import reduce_model
    import rig_character
    return rig_character, reduce_model, check_character_model


def _skeleton(path: Path | None, rig_character, model: Path | None = None) -> dict:
    """Le squelette à tenir : `skeleton.json` s'il est donné, sinon celui de la silhouette que
    nomme le squelette du modèle lié, sinon l'humanoïde."""
    if path is not None:
        return json.loads(path.read_text(encoding="utf-8"))
    if model is not None:
        import struct
        data = model.read_bytes()
        length = struct.unpack_from("<I", data, 12)[0]
        skins = json.loads(data[20:20 + length]).get("skins", [])
        if skins and skins[0].get("name"):
            return rig_character.silhouette_named(skins[0]["name"]).document()
    return rig_character.skeleton_document()


def _parameters(job: str, blend: Path, skeleton: dict, rig_character, result: Path) -> dict:
    silhouette = rig_character.silhouette_of(skeleton)
    return {"job": job, "blend": str(blend), "result": str(result),
            "bones": skeleton["bones"], "clips": skeleton["clips"],
            "animated": list(silhouette.animated),
            "samples_per_second": rig_character.SAMPLES_PER_SECOND}


def _run_blender(blender: Path, parameters: dict) -> dict:
    with tempfile.TemporaryDirectory() as scratch:
        order = Path(scratch) / "ordre.json"
        parameters["result"] = str(Path(scratch) / "releve.json")
        order.write_text(json.dumps(parameters), encoding="utf-8")
        command = [str(blender), "-b", "--factory-startup", "-noaudio", "--python-exit-code", "1",
                   "--python", str(Path(__file__).resolve()), "--", "--blender-interne", str(order)]
        done = subprocess.run(command, capture_output=True, text=True, encoding="utf-8",
                              errors="replace", check=False)
        if done.returncode != 0 or not Path(parameters["result"]).is_file():
            tail = "\n".join((done.stdout + done.stderr).splitlines()[-12:])
            raise SystemExit(f"Blender a échoué ({done.returncode}) :\n{tail}")
        return json.loads(Path(parameters["result"]).read_text(encoding="utf-8"))


def reference_path(blend: Path) -> Path:
    """Le repère d'un `.blend` : ce que Blender lisait du modèle avant toute retouche."""
    return blend.with_suffix(".repere.json")


def open_in_blender(model: Path, blend: Path, skeleton: Path | None, blender: Path,
                    window: bool = True) -> dict:
    """Prépare le `.blend` et son repère ; lance Blender dessus si `window`."""
    rig_character, _, _ = _chain()
    parameters = _parameters("open", blend, _skeleton(skeleton, rig_character, model),
                             rig_character, Path())
    parameters["model"] = str(model)
    blend.parent.mkdir(parents=True, exist_ok=True)
    extracted = _run_blender(blender, parameters)
    extracted["model"] = model.name
    reference_path(blend).write_text(json.dumps(extracted), encoding="utf-8")
    if window:
        flags = 0
        if sys.platform == "win32":
            flags = subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP
        subprocess.Popen([str(blender), str(blend)], creationflags=flags, close_fds=True)  # noqa: S603
    return extracted


def moved_joints(reference: dict, current: dict) -> dict[str, list[float]]:
    """Le déplacement de chaque articulation qui a bougé, dans le repère du modèle lié."""
    moved = {}
    for name, before in reference["joints"].items():
        after = current["joints"][name]
        delta = [after[axis] - before[axis] for axis in range(3)]
        if max(abs(value) for value in delta) > JOINT_TOLERANCE:
            moved[name] = delta
    return moved


def apply_joints(sheet: dict, moved: dict[str, list[float]]) -> tuple[list[str], list[str]]:
    """Reporte les articulations déplacées dans la fiche de liaison ; rend (reportées, ignorées).

    La fiche parle dans le repère du fichier reçu (remis dans l'axe pour un quadrupède) : un
    déplacement mesuré dans le modèle lié s'y divise par l'échelle de la fiche. Les doigts et la
    racine ne sont pas dans la fiche ; le bout d'un appui ou d'une queue (`tips`) suit son os.
    """
    stature = sheet["head_top"] - sheet["ground"]
    scale = (sheet["height"] / stature) if sheet.get("height") else 1.0
    arm_points = {"upperarm": "shoulder", "lowerarm": "elbow", "hand": "wrist"}
    applied, ignored = [], []
    for name, delta in moved.items():
        shift = [value / scale for value in delta]
        stem, _, side = name.rpartition("_")
        if name in sheet["joints"]:
            targets = [sheet["joints"][name]]
            if name in sheet.get("tips", {}):
                targets.append(sheet["tips"][name])
        elif stem in arm_points and side in sheet.get("arms", {}):
            targets = [sheet["arms"][side][arm_points[stem]]]
            if stem == "hand":
                # Le bout de la main suit le poignet : les doigts s'en déduisent.
                targets.append(sheet["arms"][side]["tip"])
        else:
            ignored.append(name)
            continue
        for target in targets:
            for axis in range(3):
                target[axis] = round(target[axis] + shift[axis], 5)
        applied.append(name)
    if applied:
        sheet["estimated"] = False
    return applied, ignored


def changed_clips(reference: dict, current: dict) -> list[str]:
    """Les clips dont une courbe diffère du repère."""
    changed = []
    for name, now in current["clips"].items():
        before = reference["clips"].get(name)
        if before is None or len(before["times"]) != len(now["times"]):
            changed.append(name)
            continue
        worst = max((abs(a - b) for row, other in zip(now["pelvis"], before["pelvis"], strict=True)
                     for a, b in zip(row, other, strict=True)), default=0.0)
        for bone, curve in now["rotations"].items():
            for q, p in zip(curve, before["rotations"][bone], strict=True):
                same = max(abs(a - b) for a, b in zip(q, p, strict=True))
                flipped = max(abs(a + b) for a, b in zip(q, p, strict=True))
                worst = max(worst, min(same, flipped))
        if worst > CURVE_TOLERANCE:
            changed.append(name)
    return changed


def import_from_blender(blend: Path, sheet_path: Path, retouch_path: Path, skeleton: Path | None,
                        blender: Path) -> dict:
    """Relit le `.blend`, reporte ce qui a changé dans la fiche de liaison et la fiche de retouche."""
    rig_character, _, _ = _chain()
    reference_file = reference_path(blend)
    if not reference_file.is_file():
        raise SystemExit(f"repère introuvable : {reference_file} — le fichier ne vient pas de "
                         "« open »")
    reference = json.loads(reference_file.read_text(encoding="utf-8"))
    current = _run_blender(blender, _parameters(
        "import", blend, _skeleton(skeleton, rig_character), rig_character, Path()))
    warnings = []
    if current["missing_clips"]:
        raise SystemExit("action(s) absente(s) du fichier, une par clip, à son nom : "
                         + ", ".join(current["missing_clips"]))
    if current["extra_bones"]:
        warnings.append("os ajoutés, ignorés : " + ", ".join(current["extra_bones"]))

    sheet = json.loads(sheet_path.read_text(encoding="utf-8"))
    moved = moved_joints(reference, current)
    applied, ignored = apply_joints(sheet, moved)
    if ignored:
        warnings.append("articulations déplacées que la fiche ne porte pas (doigts, racine), "
                        "ignorées : " + ", ".join(sorted(ignored)))
    if applied:
        sheet_path.write_text(json.dumps(sheet, indent=2, ensure_ascii=False) + "\n",
                              encoding="utf-8")

    retouch = {"version": RETOUCH_VERSION, "clips": {}}
    if retouch_path.is_file():
        retouch = json.loads(retouch_path.read_text(encoding="utf-8"))
        if retouch.get("version") != RETOUCH_VERSION:
            raise SystemExit(f"fiche de retouche de version {retouch.get('version')}")
    changed = changed_clips(reference, current)
    for name in changed:
        clip = current["clips"][name]
        if clip["stray_translation"] > STRAY_TOLERANCE:
            warnings.append(f"{name} : un os est translaté de "
                            f"{clip['stray_translation'] * 1000.0:.1f} mm — seul le bassin se "
                            "déplace, la translation est ignorée")
        if clip["scale_error"] > SCALE_TOLERANCE:
            warnings.append(f"{name} : un os est mis à l'échelle — ignoré")
        if clip["unanimated_error"] > SCALE_TOLERANCE:
            warnings.append(f"{name} : un doigt ou la racine est animé — ignoré")
        retouch["clips"][name] = {
            "times": clip["times"],
            "pelvis": [[round(v, 6) for v in row] for row in clip["pelvis"]],
            "rotations": {bone: [[round(v, 7) for v in q] for q in curve]
                          for bone, curve in clip["rotations"].items()}}
    if changed:
        # L'articulation du bassin au repos : ce qui permet de rejouer la retouche sur un autre
        # personnage, à l'échelle de sa jambe.
        retouch["pelvis_rest"] = [round(v, 6) for v in current["joints"]["pelvis"]]
        retouch["leg"] = round(sum(
            sum((current["joints"][a][axis] - current["joints"][b][axis]) ** 2
                for axis in range(3)) ** 0.5
            for a, b in (("calf_l", "thigh_l"), ("foot_l", "calf_l"))), 6)
        retouch_path.parent.mkdir(parents=True, exist_ok=True)
        retouch_path.write_text(json.dumps(retouch, ensure_ascii=False) + "\n", encoding="utf-8")
    return {"joints": applied, "clips": changed, "warnings": warnings,
            "retouched_clips": sorted(retouch["clips"])}


def main() -> int:
    rig_character, reduce_model, check_character_model = _chain()
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    commands = parser.add_subparsers(dest="command", required=True)
    opening = commands.add_parser("open", help="ouvrir un modèle lié dans Blender")
    opening.add_argument("model", type=Path, help="le .glb lié")
    opening.add_argument("--blend", type=Path, help="le .blend à écrire (défaut : à côté du modèle)")
    opening.add_argument("--no-window", action="store_true",
                         help="préparer le .blend et son repère sans lancer Blender")
    importing = commands.add_parser("import", help="relire ce qui a été réglé dans Blender")
    importing.add_argument("blend", type=Path, help="le .blend enregistré")
    importing.add_argument("--sheet", type=Path, required=True, help="la fiche de liaison")
    importing.add_argument("--retouch", type=Path, required=True, help="la fiche de retouche")
    importing.add_argument("--source", type=Path, help="le .glb reçu, pour relier dans la foulée")
    importing.add_argument("--output", type=Path, help="le .glb lié à réécrire")
    for command in (opening, importing):
        command.add_argument("--skeleton", type=Path, help="skeleton.json : os et clips à tenir")
        command.add_argument("--blender", help="blender.exe")
    arguments = parser.parse_args()
    blender = reduce_model.find_blender(arguments.blender)

    if arguments.command == "open":
        if not arguments.model.is_file():
            raise SystemExit(f"modèle introuvable : {arguments.model}")
        blend = arguments.blend or arguments.model.with_suffix(".blend")
        extracted = open_in_blender(arguments.model, blend, arguments.skeleton, blender,
                                    not arguments.no_window)
        print(f"{blend} : {len(extracted['joints'])} os, {len(extracted['clips'])} clip(s) ; "
              f"repère {reference_path(blend).name}")
        return 0

    if bool(arguments.source) != bool(arguments.output):
        raise SystemExit("--source et --output vont ensemble")
    report = import_from_blender(arguments.blend, arguments.sheet, arguments.retouch,
                                 arguments.skeleton, blender)
    print(f"articulations reportées : {', '.join(report['joints']) or 'aucune'}")
    print(f"clips retouchés : {', '.join(report['clips']) or 'aucun'}")
    for warning in report["warnings"]:
        print(f"  ATTENTION : {warning}")
    if not arguments.source:
        return 0
    retouch = arguments.retouch if arguments.retouch.is_file() else None
    statement = rig_character.rig(arguments.source, arguments.output, arguments.sheet, False, None,
                                  None, None, None, retouch=retouch)
    skeleton = _skeleton(arguments.skeleton, rig_character)
    verdict = check_character_model.inspect(arguments.output.read_bytes(), skeleton)
    print(f"{statement['model']} : relié, {statement['bytes'] / 1048576:.1f} Mio")
    for fault in verdict["faults"]:
        print(f"  ÉCART : {fault}")
    return 1 if verdict["faults"] else 0


if __name__ == "__main__":
    if "--blender-interne" in sys.argv:
        _blender_main(json.loads(
            Path(sys.argv[sys.argv.index("--blender-interne") + 1]).read_text(encoding="utf-8")))
    else:
        sys.exit(main())
