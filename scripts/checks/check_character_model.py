#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Contrôle l'export d'un modèle de personnage lié (`Planning/standards/personnages-3d.md`, §9).

Ce qu'un `.glb` de personnage doit tenir avant de s'installer, vérifié sans Blender et par un
lecteur écrit à part de celui qui l'a produit (`rig_character.py`) :

- **structure** : un maillage, une primitive, une matière, une texture incorporée, des indices
  valides ; un squelette de 53 os aux noms du standard ; les six clips, en canaux de translation et
  de rotation seulement, interpolés linéairement ;
- **poids** : quatre os au plus par sommet, somme à 1 ;
- **géométrie évaluée** : sur huit poses par clip, aucune coordonnée non finie ;
- **sol** : le maillage évalué ne s'enfonce pas de plus de 1,3 mm ;
- **marche** : le pied posé ne glisse pas de plus de 0,53 px d'art (à 120,7 px par mètre), mesuré
  sur l'os de la plante, pour un cycle qui couvre une case de 1,5 m ;
- avec `--skeleton`, l'accord avec `skeleton.json` : mêmes os, mêmes parents, mêmes durées.

Ces contrôles ne remplacent pas le jugement de l'auteur sur les planches de revue.

Usage :
    python scripts/checks/check_character_model.py MODELE.glb [...] [--skeleton skeleton.json]

Code de sortie : 0 si tous les modèles sont conformes, 1 sinon. Dépendance : numpy.
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

import numpy as np

CLIPS = ("idle", "walk", "attack", "cast", "hit", "death")
BONE_COUNT = 53
POSES = 8
GROUND_TOLERANCE = 0.0013          # m : la pénétration relevée à la preuve
SLIP_TOLERANCE = 0.53              # px d'art : le glissement relevé à la preuve
PIXELS_PER_METRE = 120.7
TILE = 1.5                         # m : ce qu'un cycle de marche couvre
PLANTED = 0.001                    # m : un pied est posé tant que sa plante est à sa hauteur basse
WEIGHT_TOLERANCE = 1e-5

_COMPONENTS = {5126: "<f4", 5125: "<u4", 5123: "<u2", 5121: "u1", 5120: "i1", 5122: "<i2"}
_WIDTHS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}


def bone_names() -> list[str]:
    """Les 53 os du squelette humanoïde (standard, §5)."""
    names = ["Root", "pelvis", "spine_01", "spine_02", "spine_03", "neck_01", "head"]
    for side in ("l", "r"):
        names += [f"{part}_{side}" for part in ("clavicle", "upperarm", "lowerarm", "hand",
                                                "thigh", "calf", "foot", "ball")]
        names += [f"{finger}_{phalanx}_{side}"
                  for finger in ("thumb", "index", "middle", "ring", "pinky")
                  for phalanx in ("01", "02", "03")]
    return names


class Model:
    """Un `.glb` lu : son document, et ses accesseurs en tableaux."""

    def __init__(self, data: bytes):
        magic, version, length = struct.unpack_from("<4sII", data, 0)
        if magic != b"glTF" or version != 2 or length > len(data):
            raise ValueError("pas un fichier .glb 2.0")
        json_length, kind = struct.unpack_from("<I4s", data, 12)
        if kind != b"JSON":
            raise ValueError("bloc JSON absent")
        self.document = json.loads(data[20:20 + json_length])
        offset = 20 + json_length
        self.binary = b""
        if offset + 8 <= length:
            binary_length, kind = struct.unpack_from("<I4s", data, offset)
            if kind == b"BIN\x00":
                self.binary = data[offset + 8:offset + 8 + binary_length]

    def accessor(self, index: int) -> np.ndarray:
        accessor = self.document["accessors"][index]
        view = self.document["bufferViews"][accessor["bufferView"]]
        kind = np.dtype(_COMPONENTS[accessor["componentType"]])
        width = _WIDTHS[accessor["type"]]
        start = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
        count = accessor["count"]
        stride = view.get("byteStride", 0) or kind.itemsize * width
        if start + stride * (count - 1) + kind.itemsize * width > len(self.binary):
            raise ValueError(f"l'accesseur {index} sort du tampon")
        if stride == kind.itemsize * width:
            return np.frombuffer(self.binary, kind, count * width, start).reshape(count, width)
        rows = np.frombuffer(self.binary, np.uint8, stride * (count - 1) + kind.itemsize * width,
                             start)
        picked = np.stack([rows[i * stride:i * stride + kind.itemsize * width]
                           for i in range(count)])
        return np.frombuffer(picked.tobytes(), kind).reshape(count, width)


def _matrix(rotation: np.ndarray, translation: np.ndarray) -> np.ndarray:
    """La matrice 4 × 4 d'un quaternion (x, y, z, w) et d'une translation."""
    x, y, z, w = rotation / np.linalg.norm(rotation)
    matrix = np.eye(4)
    matrix[:3, :3] = [[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                      [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                      [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]]
    matrix[:3, 3] = translation
    return matrix


class Rig:
    """Le squelette d'un modèle, posable à un instant d'un clip."""

    def __init__(self, model: Model):
        document = model.document
        self.nodes = document["nodes"]
        skin = document["skins"][0]
        self.joints = skin["joints"]
        self.names = [self.nodes[index].get("name", "") for index in self.joints]
        self.parent = {}
        for index, node in enumerate(self.nodes):
            for child in node.get("children", []):
                self.parent[child] = index
        self.inverse_bind = model.accessor(skin["inverseBindMatrices"]).astype(
            np.float64).reshape(-1, 4, 4).transpose(0, 2, 1)
        self.clips = {}
        for animation in document.get("animations", []):
            channels = []
            for channel in animation["channels"]:
                sampler = animation["samplers"][channel["sampler"]]
                channels.append({
                    "node": channel["target"]["node"], "path": channel["target"]["path"],
                    "interpolation": sampler.get("interpolation", "LINEAR"),
                    "times": model.accessor(sampler["input"]).astype(np.float64).reshape(-1),
                    "values": model.accessor(sampler["output"]).astype(np.float64)})
            self.clips[animation.get("name", "")] = channels

    def duration(self, clip: str) -> float:
        return max(float(channel["times"][-1]) for channel in self.clips[clip])

    def pose(self, clip: str | None, time: float) -> np.ndarray:
        """Les matrices monde de chaque articulation, à `time` de `clip` (ou au repos)."""
        rotation = {i: np.array(n.get("rotation", [0.0, 0.0, 0.0, 1.0]), dtype=np.float64)
                    for i, n in enumerate(self.nodes)}
        translation = {i: np.array(n.get("translation", [0.0, 0.0, 0.0]), dtype=np.float64)
                       for i, n in enumerate(self.nodes)}
        for channel in (self.clips[clip] if clip else []):
            times, values = channel["times"], channel["values"]
            after = int(np.searchsorted(times, time, side="right"))
            before = max(after - 1, 0)
            after = min(after, len(times) - 1)
            span = times[after] - times[before]
            share = (time - times[before]) / span if span > 0 else 0.0
            share = min(max(share, 0.0), 1.0)
            first, second = values[before], values[after]
            if channel["path"] == "rotation":
                if float(np.dot(first, second)) < 0.0:
                    second = -second
                value = first * (1.0 - share) + second * share
                rotation[channel["node"]] = value / np.linalg.norm(value)
            elif channel["path"] == "translation":
                translation[channel["node"]] = first * (1.0 - share) + second * share
        world = {}

        def resolve(index: int) -> np.ndarray:
            if index not in world:
                local = _matrix(rotation[index], translation[index])
                parent = self.parent.get(index)
                world[index] = local if parent is None else resolve(parent) @ local
            return world[index]

        return np.stack([resolve(index) for index in self.joints])

    def skin(self, world: np.ndarray, positions: np.ndarray, joints: np.ndarray,
             weights: np.ndarray) -> np.ndarray:
        """Les sommets déformés par les matrices `world`."""
        matrices = world @ self.inverse_bind
        result = np.zeros_like(positions)
        for column in range(joints.shape[1]):
            chosen = matrices[joints[:, column]]
            moved = np.einsum("nij,nj->ni", chosen[:, :3, :3], positions) + chosen[:, :3, 3]
            result += weights[:, column:column + 1] * moved
        return result


def _structure(model: Model, faults: list[str]) -> dict | None:
    document = model.document
    meshes = document.get("meshes", [])
    primitives = [p for mesh in meshes for p in mesh.get("primitives", [])]
    if len(meshes) != 1 or len(primitives) != 1:
        faults.append(f"{len(meshes)} maillage(s), {len(primitives)} primitive(s) : un de chaque")
        return None
    primitive = primitives[0]
    attributes = primitive.get("attributes", {})
    missing = [name for name in ("POSITION", "NORMAL", "TEXCOORD_0", "JOINTS_0", "WEIGHTS_0")
               if name not in attributes]
    if missing or "indices" not in primitive:
        faults.append("attributs absents : " + ", ".join(missing or ["indices"]))
        return None
    if len(document.get("materials", [])) != 1:
        faults.append(f"{len(document.get('materials', []))} matière(s) : une seule")
    images = document.get("images", [])
    if len(images) != 1 or "bufferView" not in images[0]:
        faults.append("la texture de couleur n'est pas incorporée, ou n'est pas seule")
    if len(document.get("skins", [])) != 1:
        faults.append(f"{len(document.get('skins', []))} squelette(s) : un seul")
        return None
    if "inverseBindMatrices" not in document["skins"][0]:
        faults.append("le squelette n'a pas de matrices de liaison")
        return None
    return primitive


def inspect(data: bytes, skeleton: dict | None = None) -> dict:
    """Les mesures d'un modèle et ses écarts au standard (`faults`, vide s'il s'y tient)."""
    faults: list[str] = []
    measures: dict = {"faults": faults}
    try:
        model = Model(data)
        primitive = _structure(model, faults)
        if primitive is None:
            return measures
        positions = model.accessor(primitive["attributes"]["POSITION"]).astype(np.float64)
        joints = model.accessor(primitive["attributes"]["JOINTS_0"]).astype(np.int64)
        weights = model.accessor(primitive["attributes"]["WEIGHTS_0"]).astype(np.float64)
        indices = model.accessor(primitive["indices"]).reshape(-1)
        rig = Rig(model)
    except (ValueError, KeyError, IndexError, struct.error) as error:
        faults.append(f"fichier illisible : {error}")
        return measures

    measures["triangles"] = int(len(indices) // 3)
    measures["vertices"] = int(len(positions))
    if len(indices) % 3 or (len(indices) and int(indices.max()) >= len(positions)):
        faults.append("indices invalides")

    # Le squelette.
    expected = bone_names()
    if len(rig.names) != BONE_COUNT or sorted(rig.names) != sorted(expected):
        faults.append(f"{len(rig.names)} os, ou des noms hors du standard : "
                      + ", ".join(sorted(set(rig.names) ^ set(expected))[:6]))
    measures["bones"] = len(rig.names)
    if len(joints) and int(joints.max()) >= len(rig.joints):
        faults.append("un sommet cite un os que le squelette n'a pas")
        return measures

    # Les poids.
    if weights.shape[1] != 4:
        faults.append(f"{weights.shape[1]} influences par sommet : quatre")
    drift = float(np.abs(weights.sum(axis=1) - 1.0).max()) if len(weights) else 0.0
    measures["weight_error"] = drift
    if drift > WEIGHT_TOLERANCE or (len(weights) and float(weights.min()) < 0.0):
        faults.append(f"poids non normalisés : écart de {drift:.2e}")

    # Les clips.
    absent = [clip for clip in CLIPS if clip not in rig.clips]
    extra = [clip for clip in rig.clips if clip not in CLIPS]
    if absent or extra:
        faults.append("clips : il manque " + (", ".join(absent) or "rien") + ", en trop "
                      + (", ".join(extra) or "rien"))
    for clip, channels in rig.clips.items():
        bad = sorted({channel["path"] for channel in channels
                      if channel["path"] not in ("translation", "rotation")})
        if bad:
            faults.append(f"{clip} : canal {', '.join(bad)}")
        if any(channel["interpolation"] != "LINEAR" for channel in channels):
            faults.append(f"{clip} : interpolation non linéaire")
        if any(abs(float(channel["times"][0])) > 1e-6 for channel in channels):
            faults.append(f"{clip} : le premier temps n'est pas 0")
    if absent:
        return measures

    if skeleton is not None:
        declared = [(bone["name"], bone["parent"]) for bone in skeleton.get("bones", [])]
        found = []
        for index in rig.joints:
            parent = rig.parent.get(index)
            found.append((rig.nodes[index].get("name", ""),
                          rig.nodes[parent].get("name", "") if parent in rig.joints else ""))
        if sorted(declared) != sorted(found):
            faults.append("les os ou leurs parents diffèrent de skeleton.json")
        for clip in skeleton.get("clips", []):
            if clip["name"] in rig.clips and abs(rig.duration(clip["name"])
                                                 - clip["duration"]) > 1e-4:
                faults.append(f"{clip['name']} : durée {rig.duration(clip['name']):.4f} s, "
                              f"skeleton.json dit {clip['duration']}")

    # La géométrie évaluée, et le sol : huit poses par clip, prises entre les échantillons aussi.
    lowest, finite = {}, True
    for clip in CLIPS:
        duration = rig.duration(clip)
        depth = 0.0
        for index in range(POSES):
            skinned = rig.skin(rig.pose(clip, duration * (index + 0.5) / POSES), positions,
                               joints, weights)
            if not np.all(np.isfinite(skinned)):
                finite = False
                continue
            depth = min(depth, float(skinned[:, 1].min()))
        lowest[clip] = -depth
    measures["penetration_mm"] = {clip: round(value * 1000.0, 3) for clip, value in lowest.items()}
    if not finite:
        faults.append("géométrie évaluée non finie")
    deepest = max(lowest.values())
    if deepest > GROUND_TOLERANCE:
        faults.append(f"le maillage s'enfonce de {deepest * 1000.0:.2f} mm dans le sol")

    # La marche : le pied posé recule à la vitesse du cycle, mesuré sur l'os de la plante.
    duration = rig.duration("walk")
    speed = TILE / duration
    samples = 96
    times = [duration * index / samples for index in range(samples)]
    worlds = [rig.pose("walk", time) for time in times]
    slip = 0.0
    stance = {}
    for side in ("l", "r"):
        ball = rig.names.index(f"ball_{side}")
        track = np.array([world[ball, :3, 3] for world in worlds])
        planted = track[:, 1] <= track[:, 1].min() + PLANTED
        stance[side] = round(float(planted.mean()), 3)
        if not planted.any() or planted.all():
            faults.append(f"marche : le pied {side} n'alterne pas")
            continue
        # La plus longue suite de poses posées, en tournant autour du cycle.
        start = next(i for i in range(samples) if planted[i] and not planted[i - 1])
        run = []
        for step in range(samples):
            index = (start + step) % samples
            if not planted[index]:
                break
            run.append((times[index] + (duration if index < start else 0.0), track[index]))
        drift = np.array([[point[0], point[2] + speed * time] for time, point in run])
        slip = max(slip, float(np.ptp(drift[:, 0])), float(np.ptp(drift[:, 1])))
    measures["walk"] = {"duration": round(duration, 4), "stance_share": stance,
                        "slip_px": round(slip * PIXELS_PER_METRE, 3)}
    if slip * PIXELS_PER_METRE > SLIP_TOLERANCE:
        faults.append(f"marche : le pied posé glisse de {slip * PIXELS_PER_METRE:.2f} px d'art")
    return measures


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("models", nargs="+", type=Path, help="les .glb liés à contrôler")
    parser.add_argument("--skeleton", type=Path, help="skeleton.json : os et durées à tenir")
    arguments = parser.parse_args()
    skeleton = (json.loads(arguments.skeleton.read_text(encoding="utf-8"))
                if arguments.skeleton else None)
    failed = 0
    for path in arguments.models:
        measures = inspect(path.read_bytes(), skeleton)
        faults = measures.pop("faults")
        print(f"{path} : {json.dumps(measures, ensure_ascii=False)}")
        for fault in faults:
            print(f"  ÉCART : {fault}")
        failed += bool(faults)
    print(f"{len(arguments.models)} modèle(s), {failed} avec écart")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
