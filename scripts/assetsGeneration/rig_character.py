#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""Lie un modèle de personnage au squelette commun de sa silhouette et lui pose ses clips.

Un personnage arrive de Meshy en maillage statique, bras en croix, puis passe par
`reduce_model.py` (au sol, une matière, 100 000 triangles). Ce script en fait le `.glb` autonome
que le moteur anime (`Planning/standards/personnages-3d.md`, §5 à §7) :

- **le squelette** : pour la silhouette `humanoid`, les 53 os du `game_engine` de MPFB — leurs
  noms et leur hiérarchie seulement, écrits ici ; le fichier n'emporte rien de MPFB. Chaque os a
  une orientation de repos nulle : la pose de liaison est la pose du maillage reçu. La silhouette
  `quadruped` (lion, loup : 29 os, cinq clips, une marche au trot) est écrite dans
  `rig_quadruped.py` ; ce script la lit par `--silhouette quadruped`, ou par le champ
  `silhouette` de la fiche de liaison ;
- **la liaison** : la position des articulations dans **ce** maillage se lit dans sa fiche
  (`liaison.json`). Sans fiche, elle est estimée d'après la géométrie et **écrite** : c'est la
  fiche écrite qui fait foi et se rejoue, corrigée à la main s'il le faut — jamais le maillage ;
- **les poids** : calculés d'après les articulations, quatre os au plus par sommet, somme à 1 ;
- **les clips** : `idle`, `walk`, `attack`, `cast`, `hit`, `death`, posés **par cibles** (une
  plante de pied à atteindre, un poignet à placer, deux os résolus) et non en angles : les mêmes
  fonctions animent tous les maillages. La marche couvre une case de 1,5 m en 0,5 s, sur place ;
  le pied posé recule exactement à 3 m/s. La hauteur du bassin est recalée, image par image, sur
  la surface évaluée du maillage : le point le plus bas touche le sol ;
- **la retouche** : avec `--retouch`, les clips de la fiche de retouche (`retouche.json`, écrite
  par `retouch_character.py` d'après ce que l'auteur a réglé dans Blender) remplacent les clips
  posés par cibles ; ils gardent leur durée et reçoivent le même recalage au sol ;
- **`skeleton.json`** : les os et les clips (durée, boucle, image clé), écrits depuis la même
  table que les animations.

Tout est du calcul (numpy) : Blender ne sert qu'à l'option `--triangles`, qui réduit d'abord le
maillage par `reduce_model.py`. La texture de couleur du fichier reçu est remise octet pour octet,
sauf avec `--texture`, qui la ramène à la définition demandée (copie d'essai).

Usage :
    python scripts/assetsGeneration/rig_character.py SOURCE.glb SORTIE.glb
        [--sheet liaison.json] [--estimate] [--silhouette humanoid|quadruped]
        [--skeleton skeleton.json] [--triangles N] [--texture PX] [--blender CHEMIN]
        [--retouch retouche.json]

Dépendances : numpy, Pillow ; Blender 5.2 pour `--triangles` (outil de production, pas de CI).
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import math
import sys
import tempfile
from pathlib import Path

import numpy as np

import reduce_model


# --- Une silhouette --------------------------------------------------------------------------------
class Silhouette:
    """Ce qui distingue une silhouette : ses os, ses clips, et les fonctions qui lisent un maillage,
    le pèsent et le posent. La cuisson des clips, l'écriture du `.glb` et le relevé sont communs."""

    def __init__(self, name: str, bones: list[tuple[str, str]], animated: list[str],
                 clips: list[dict], lift_only: set[str], estimate_sheet, rest_skeleton,
                 compute_weights, make_body, clip_poses, solve_pose, drawn):
        self.name = name
        self.bones = bones
        self.bone_index = {bone: index for index, (bone, _) in enumerate(bones)}
        self.parent = {bone: parent for bone, parent in bones}
        self.animated = animated
        self.clips = clips
        self.lift_only = lift_only
        self.estimate_sheet = estimate_sheet
        self.rest_skeleton = rest_skeleton
        self.compute_weights = compute_weights
        self.make_body = make_body
        self.clip_poses = clip_poses
        self.solve_pose = solve_pose
        self.drawn = drawn    # os tracés sur la planche de liaison

    def document(self) -> dict:
        """Ce que le moteur lit du squelette : ses os, parents avant enfants, et ses clips."""
        return {"version": 1, "silhouette": self.name,
                "bones": [{"name": name, "parent": parent} for name, parent in self.bones],
                "clips": [dict(clip) for clip in self.clips]}


def silhouette_named(name: str) -> Silhouette:
    """La silhouette d'un nom : `humanoid` est ici, `quadruped` dans `rig_quadruped.py`."""
    if name == SILHOUETTE:
        return HUMANOID
    if name == "quadruped":
        import rig_quadruped
        return rig_quadruped.QUADRUPED
    raise SystemExit(f"silhouette inconnue : {name}")


def silhouette_of(skeleton: dict) -> Silhouette:
    """La silhouette que nomme un `skeleton.json` (sans nom : l'humanoïde)."""
    return silhouette_named(skeleton.get("silhouette", SILHOUETTE))


# --- Le squelette ----------------------------------------------------------------------------------
SILHOUETTE = "humanoid"
SIDES = (("l", 1.0), ("r", -1.0))
FINGERS = ("thumb", "index", "middle", "ring", "pinky")


def _bones() -> list[tuple[str, str]]:
    bones = [("Root", ""), ("pelvis", "Root"), ("spine_01", "pelvis"), ("spine_02", "spine_01"),
             ("spine_03", "spine_02"), ("neck_01", "spine_03"), ("head", "neck_01")]
    for side, _ in SIDES:
        bones += [(f"clavicle_{side}", "spine_03"), (f"upperarm_{side}", f"clavicle_{side}"),
                  (f"lowerarm_{side}", f"upperarm_{side}"), (f"hand_{side}", f"lowerarm_{side}")]
        for finger in FINGERS:
            parent = f"hand_{side}"
            for phalanx in ("01", "02", "03"):
                bones.append((f"{finger}_{phalanx}_{side}", parent))
                parent = f"{finger}_{phalanx}_{side}"
        bones += [(f"thigh_{side}", "pelvis"), (f"calf_{side}", f"thigh_{side}"),
                  (f"foot_{side}", f"calf_{side}"), (f"ball_{side}", f"foot_{side}")]
    return bones


BONES = _bones()
BONE_INDEX = {name: index for index, (name, _) in enumerate(BONES)}
PARENT = {name: parent for name, parent in BONES}
assert len(BONES) == 53
# Les os qu'un clip anime ; les doigts gardent la pose sculptée et n'ont aucun canal.
ANIMATED = [name for name, _ in BONES
            if name != "Root" and not name.startswith(FINGERS)]

# --- Les clips : la table que lisent les animations ET skeleton.json -------------------------------
SAMPLES_PER_SECOND = 60
WALK_SPEED = 3.0          # m/s : une case de 1,5 m en 0,5 s
CLIPS = [
    {"name": "idle", "duration": 1.0, "loop": True},
    {"name": "walk", "duration": 0.5, "loop": True},
    {"name": "attack", "duration": 0.9, "loop": False, "key": 0.4},
    {"name": "cast", "duration": 1.0, "loop": False, "key": 0.5},
    {"name": "hit", "duration": 0.6, "loop": False},
    {"name": "death", "duration": 1.2, "loop": False},
]
# La marche d'une jambe courte a une phase de vol : son recalage ne fait que **remonter** le corps
# (une semelle qui s'enfonce), il ne le plaque pas au sol entre deux appuis.
LIFT_ONLY = {"walk"}
SHEET_VERSION = 1
RETOUCH_VERSION = 1
ARM_RADIUS = 0.17
MAX_INFLUENCES = 4


def skeleton_document(silhouette: Silhouette | None = None) -> dict:
    """Ce que le moteur lit du squelette : ses os, parents avant enfants, et ses clips."""
    return (silhouette or HUMANOID).document()


# --- Le maillage reçu ------------------------------------------------------------------------------
_COMPONENTS = {5126: "<f4", 5125: "<u4", 5123: "<u2", 5121: "u1"}
_WIDTHS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}


def read_accessor(document: dict, binary: bytes, index: int) -> np.ndarray:
    accessor = document["accessors"][index]
    view = document["bufferViews"][accessor["bufferView"]]
    kind = np.dtype(_COMPONENTS[accessor["componentType"]])
    width = _WIDTHS[accessor["type"]]
    offset = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
    stride = view.get("byteStride", 0)
    if stride and stride != kind.itemsize * width:
        raw = np.frombuffer(binary, np.uint8, stride * accessor["count"], offset)
        raw = raw.reshape(accessor["count"], stride)[:, :kind.itemsize * width]
        return np.frombuffer(raw.tobytes(), kind).reshape(accessor["count"], width)
    return np.frombuffer(binary, kind, accessor["count"] * width, offset).reshape(
        accessor["count"], width)


def read_static_mesh(data: bytes) -> dict:
    """Positions, normales, coordonnées de texture, triangles et image d'un `.glb` au standard."""
    document, binary = reduce_model.read_glb(data)
    primitives = [p for mesh in document.get("meshes", []) for p in mesh["primitives"]]
    if len(primitives) != 1:
        raise SystemExit(f"{len(primitives)} primitives : le modèle passe d'abord par reduce_model.py")
    attributes = primitives[0]["attributes"]
    for name in ("POSITION", "NORMAL", "TEXCOORD_0"):
        if name not in attributes:
            raise SystemExit(f"attribut {name} absent")
    image = reduce_model.base_color_image(document, binary)
    return {
        "positions": read_accessor(document, binary, attributes["POSITION"]).astype(np.float64),
        "normals": read_accessor(document, binary, attributes["NORMAL"]).astype(np.float32),
        "uvs": read_accessor(document, binary, attributes["TEXCOORD_0"]).astype(np.float32),
        "indices": read_accessor(document, binary, primitives[0]["indices"]).astype(
            np.uint32).reshape(-1),
        "image": image,
    }


# --- L'estimation des articulations ----------------------------------------------------------------
def _band(points: np.ndarray, axis: int, low: float, high: float) -> np.ndarray:
    return points[(points[:, axis] >= low) & (points[:, axis] < high)]


def _middle(points: np.ndarray, axis: int, default: float) -> float:
    if len(points) == 0:
        return default
    return float((points[:, axis].min() + points[:, axis].max()) / 2)


def estimate_sheet(positions: np.ndarray) -> dict:
    """Une fiche de liaison estimée d'après la géométrie d'un maillage bras en croix.

    Repère du fichier reçu : hauteur +Y, le personnage regarde vers +Z, sa gauche est en +X. Les
    proportions sont celles d'un corps humain, recalées sur ce que les tranches du maillage
    montrent : la hauteur des bras, l'aisselle, le cou, l'entrejambe, les chevilles. Une estimation
    n'est pas une mesure : la fiche se relit sur la planche `liaison.png` et se corrige.
    """
    P = positions
    ground = float(P[:, 1].min())
    center_x = float((P[:, 0].min() + P[:, 0].max()) / 2)
    Q = P - np.array([center_x, ground, 0.0])
    # Le sommet du crâne : sur l'axe du corps, pas une pièce d'épaule ni une mèche écartée.
    head_top = float(Q[np.abs(Q[:, 0]) < 0.10][:, 1].max())
    # Les deux moitiés sont estimées en miroir, sur le côté gauche (+X) : le maillage reçu n'est
    # pas symétrique au sommet près, mais ses articulations le sont à l'échelle d'une tranche.
    M = np.concatenate([Q, Q * np.array([-1.0, 1.0, 1.0])])
    upper = M[M[:, 1] > 0.55 * head_top]
    reach = float(upper[:, 0].max())
    step = 0.02
    extents = {}
    for start in np.arange(0.0, reach, step):
        band = _band(upper, 0, start, start + step)
        extents[round(float(start), 2)] = (float(band[:, 1].max() - band[:, 1].min())
                                           if len(band) else 0.0)
    outer = [e for x, e in extents.items() if 0.5 * reach <= x <= 0.85 * reach and e > 0]
    thickness = float(np.median(outer)) if outer else 0.1
    armpit = 0.5 * reach
    for start in sorted((x for x in extents if x < 0.5 * reach), reverse=True):
        if extents[start] > 1.8 * thickness + 0.03:
            break
        armpit = start
    arm_band = _band(upper, 0, armpit + 0.04, armpit + 0.5 * (reach - armpit))
    arm_y = _middle(arm_band, 1, 0.8 * head_top)
    arm_z = _middle(arm_band, 2, 0.0)
    chest = _band(M[np.abs(M[:, 2] - arm_z) < 0.3], 1, arm_y - 0.25, arm_y - 0.15)
    chest_half = float(chest[:, 0].max()) if len(chest) else armpit - 0.1
    # L'épaule : au bord de la cage thoracique, dans ce qu'un corps humain permet — une manche
    # bouffante ou une épaulière élargit la tranche sans déplacer l'articulation.
    shoulder_x = float(np.clip(chest_half + 0.03, 0.10 * head_top, 0.135 * head_top))
    # Le bout de la main : le point le plus écarté près de l'axe du bras.
    near_axis = upper[np.abs(upper[:, 1] - arm_y) < 0.15]
    tip_x = float(near_axis[:, 0].max())

    def arm_point(x: float) -> list[float]:
        band = _band(near_axis, 0, x - 0.03, x + 0.03)
        return [x, _middle(band, 1, arm_y), _middle(band, 2, arm_z)]

    length = tip_x - shoulder_x
    shoulder = [shoulder_x, arm_y, arm_z]
    elbow = arm_point(shoulder_x + 0.40 * length)
    wrist = arm_point(shoulder_x + 0.76 * length)
    tip = arm_point(tip_x - 0.01)
    tip[0] = tip_x

    # Le cou : la tranche la plus étroite entre les épaules et le crâne.
    neck_y, narrowest = arm_y + 0.08, None
    for y in np.arange(arm_y + 0.04, head_top - 0.14, 0.01):
        band = _band(M[np.abs(M[:, 0]) < 0.3], 1, y, y + 0.02)
        if len(band) == 0:
            continue
        width = float(band[:, 0].max())
        if narrowest is None or width < narrowest:
            neck_y, narrowest = float(y) + 0.01, width
    axis = M[np.abs(M[:, 0]) < 0.12]

    def spine_z(y: float) -> float:
        return _middle(_band(axis, 1, y - 0.03, y + 0.03), 2, 0.0)

    # L'entrejambe : la plus haute tranche encore fendue en son milieu ; une jupe ou une robe la
    # cache, et la proportion humaine la remplace.
    crotch = 0.0
    for y in np.arange(0.2 * head_top, 0.6 * head_top, 0.01):
        band = _band(M, 1, y, y + 0.02)
        left = band[band[:, 0] > 0]
        if len(left) and float(left[:, 0].min()) > 0.008:
            crotch = float(y) + 0.02
    if crotch < 0.40 * head_top or crotch > 0.52 * head_top:
        crotch = 0.46 * head_top
    hip_y = crotch + 0.05 * head_top
    thigh_band = _band(M[M[:, 0] > 0.005], 1, crotch - 0.12, crotch - 0.04)
    thigh_band = thigh_band[thigh_band[:, 0] < shoulder_x + 0.08]
    hip_x = float(np.clip(_middle(thigh_band, 0, 0.055 * head_top),
                          0.045 * head_top, 0.07 * head_top))
    hip_z = _middle(_band(axis, 1, hip_y - 0.03, hip_y + 0.03), 2, 0.0)

    ankle_y = 0.05 * head_top
    legs = M[(M[:, 0] > 0.01) & (M[:, 0] < 2.5 * shoulder_x)]
    ankle_band = _band(legs, 1, ankle_y, ankle_y + 0.06)
    ankle = [_middle(ankle_band, 0, hip_x), ankle_y, _middle(ankle_band, 2, hip_z)]
    knee_y = 0.5 * (hip_y + ankle_y) + 0.02 * head_top
    knee_band = _band(legs, 1, knee_y - 0.03, knee_y + 0.03)
    knee_band = knee_band[knee_band[:, 0] < max(ankle[0], hip_x) + 0.15]
    knee = [float(np.clip(_middle(knee_band, 0, 0.5 * (hip_x + ankle[0])),
                          min(hip_x, ankle[0]) - 0.03, max(hip_x, ankle[0]) + 0.03)),
            knee_y, _middle(knee_band, 2, hip_z)]
    sole = legs[(legs[:, 1] < 0.6 * ankle_y) & (np.abs(legs[:, 0] - ankle[0]) < 0.15)]
    toe_z = float(sole[:, 2].max()) if len(sole) else ankle[2] + 0.2
    ball = [_middle(sole, 0, ankle[0]), 0.25 * ankle_y, ankle[2] + 0.6 * (toe_z - ankle[2])]

    pelvis_y = hip_y + 0.02 * head_top
    neck_joint_y = neck_y - 0.045 * (head_top / 1.8)
    head_joint_y = neck_y + 0.03 * (head_top / 1.8)
    torso = neck_joint_y - pelvis_y
    joints = {"pelvis": [0.0, pelvis_y, spine_z(pelvis_y)]}
    for name, fraction in (("spine_01", 0.18), ("spine_02", 0.40), ("spine_03", 0.62)):
        y = pelvis_y + fraction * torso
        joints[name] = [0.0, y, spine_z(y)]
    joints["neck_01"] = [0.0, neck_joint_y, spine_z(neck_joint_y)]
    joints["head"] = [0.0, head_joint_y, spine_z(head_joint_y)]
    arms = {}
    for side, sign in SIDES:
        joints[f"clavicle_{side}"] = [sign * 0.03, arm_y + 0.02, arm_z + 0.02]
        joints[f"thigh_{side}"] = [sign * hip_x, hip_y, hip_z]
        joints[f"calf_{side}"] = [sign * knee[0], knee[1], knee[2]]
        joints[f"foot_{side}"] = [sign * ankle[0], ankle[1], ankle[2]]
        joints[f"ball_{side}"] = [sign * ball[0], ball[1], ball[2]]
        arms[side] = {key: [sign * value[0], value[1], value[2]]
                      for key, value in (("shoulder", shoulder), ("elbow", elbow),
                                         ("wrist", wrist), ("tip", tip))}

    back = np.array([center_x, ground, 0.0])
    joints = {name: list(np.array(point) + back) for name, point in joints.items()}
    arms = {side: {key: list(np.array(point) + back) for key, point in arm.items()}
            for side, arm in arms.items()}

    def rounded(value):
        if isinstance(value, dict):
            return {key: rounded(item) for key, item in value.items()}
        if isinstance(value, list):
            return [rounded(item) for item in value]
        return round(float(value), 4)

    return {"version": SHEET_VERSION, "estimated": True, "height": None,
            "ground": round(ground, 4), "head_top": round(head_top + ground, 4),
            "center_x": round(center_x, 4), "joints": rounded(joints), "arms": rounded(arms),
            "arm_radius": ARM_RADIUS, "head_radius": 0.21, "rigid": []}


# --- Le squelette de repos, à l'échelle du personnage ----------------------------------------------
def model_points(positions: np.ndarray, sheet: dict, scale: float,
                 origin: np.ndarray) -> np.ndarray:
    """Les sommets du modèle écrit : le fichier reçu recentré sur `origin`, tourné de `heading`
    (le cap du corps dans le fichier reçu, en degrés depuis +Z vers +X ; nul pour un humanoïde),
    puis mis à l'échelle."""
    moved = positions - origin
    heading = float(sheet.get("heading", 0.0))
    if heading:
        moved = moved @ rot_y(-math.radians(heading)).T
    return moved * scale


def rest_skeleton(sheet: dict) -> tuple[dict[str, np.ndarray], float, np.ndarray]:
    """Les articulations dans le repère du modèle écrit, son échelle et son décalage.

    La fiche parle dans le repère du fichier **reçu** ; le modèle écrit a le sol à zéro, l'axe du
    corps en X = 0 et, si la fiche le demande (`height`), la taille de son personnage.
    """
    stature = sheet["head_top"] - sheet["ground"]
    scale = (sheet["height"] / stature) if sheet.get("height") else 1.0
    origin = np.array([sheet["center_x"], sheet["ground"], 0.0])

    def place(point) -> np.ndarray:
        return (np.array(point, dtype=np.float64) - origin) * scale

    joints = {"Root": np.zeros(3)}
    for name, point in sheet["joints"].items():
        joints[name] = place(point)
    for side, _ in SIDES:
        arm = {key: place(value) for key, value in sheet["arms"][side].items()}
        joints[f"upperarm_{side}"] = arm["shoulder"]
        joints[f"lowerarm_{side}"] = arm["elbow"]
        joints[f"hand_{side}"] = arm["wrist"]
        # Les doigts : aucun poids, aucun canal ; posés le long de la main pour que le squelette
        # soit entier et lisible. Le pouce part vers l'avant, les quatre autres s'étalent.
        along = arm["tip"] - arm["wrist"]
        for rank, finger in enumerate(FINGERS):
            spread = np.array([0.0, 0.0, 0.03 - 0.02 * rank]) * scale
            start = 0.15 if finger == "thumb" else 0.45
            span = 0.45 if finger == "thumb" else 0.5
            for step, phalanx in enumerate(("01", "02", "03")):
                joints[f"{finger}_{phalanx}_{side}"] = (
                    arm["wrist"] + along * (start + span * step / 3.0) + spread)
    missing = [name for name, _ in BONES if name not in joints]
    if missing:
        raise SystemExit("fiche de liaison incomplète : " + ", ".join(missing))
    return joints, scale, origin


# --- Les poids -------------------------------------------------------------------------------------
def _smooth(low: float, high: float, value: np.ndarray) -> np.ndarray:
    t = np.clip((value - low) / (high - low), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def compute_weights(points: np.ndarray, joints: dict[str, np.ndarray], sheet: dict,
                    scale: float) -> np.ndarray:
    """Le poids de chaque os sur chaque sommet (N × 53), avant la coupe à quatre influences.

    Le maillage est en croix : un bras se lit le long de X, le tronc et les jambes le long de Y.
    Chaque chaîne d'os se partage ses sommets par des rampes lisses centrées sur ses articulations ;
    les deux jambes se partagent ce qui pend entre elles (une robe) de part et d'autre de l'axe.
    """
    count = len(points)
    weights = np.zeros((count, len(BONES)))
    x, y, z = points[:, 0], points[:, 1], points[:, 2]
    # Les largeurs des rampes suivent la taille du personnage : celles d'un corps de 1,80 m.
    unit = scale * (sheet["head_top"] - sheet["ground"]) / 1.8

    # Les bras : un masque le long de X, borné à un rayon autour de l'axe du bras.
    arm_mask = np.zeros((count, 2))
    radius = sheet.get("arm_radius", ARM_RADIUS) * scale
    for column, (side, sign) in enumerate(SIDES):
        shoulder, elbow = joints[f"upperarm_{side}"], joints[f"lowerarm_{side}"]
        wrist = joints[f"hand_{side}"]
        u = sign * x - sign * shoulder[0]
        u_elbow = sign * (elbow[0] - shoulder[0])
        u_wrist = sign * (wrist[0] - shoulder[0])
        knots_u = np.array([0.0, u_elbow, u_wrist])
        axis_y = np.interp(u, knots_u, [shoulder[1], elbow[1], wrist[1]])
        axis_z = np.interp(u, knots_u, [shoulder[2], elbow[2], wrist[2]])
        distance = np.hypot(y - axis_y, z - axis_z)
        # Au-dessus de l'axe (l'épaule, le deltoïde) le bras prend plus tôt que dessous
        # (l'aisselle, le flanc) : c'est ce qui garde un flanc au tronc quand le bras descend.
        above = _smooth(-0.04 * unit, 0.04 * unit, y - axis_y)
        start = -(0.03 + 0.06 * above) * unit
        mask = _smooth(start, start + 0.11 * unit, u) * (1.0 - _smooth(radius, radius + 0.05 * unit,
                                                                      distance))
        arm_mask[:, column] = mask
        bend = _smooth(u_elbow - 0.05 * unit, u_elbow + 0.05 * unit, u)
        hand = _smooth(u_wrist - 0.03 * unit, u_wrist + 0.03 * unit, u)
        weights[:, BONE_INDEX[f"upperarm_{side}"]] = mask * (1.0 - bend)
        weights[:, BONE_INDEX[f"lowerarm_{side}"]] = mask * bend * (1.0 - hand)
        weights[:, BONE_INDEX[f"hand_{side}"]] = mask * hand
    body = np.clip(1.0 - arm_mask.sum(axis=1), 0.0, 1.0)

    # Le tronc : une chaîne le long de Y, du bassin à la tête.
    chain = ["pelvis", "spine_01", "spine_02", "spine_03", "neck_01", "head"]
    ramps = [np.ones(count)]
    for name in chain[1:]:
        half = (0.03 if name in ("neck_01", "head") else 0.045) * unit
        ramps.append(_smooth(joints[name][1] - half, joints[name][1] + half, y))
    ramps.append(np.zeros(count))
    upper = np.zeros((count, len(chain)))
    for rank in range(len(chain)):
        upper[:, rank] = np.clip(ramps[rank] - ramps[rank + 1], 0.0, 1.0)
    # Ce qui dépasse du cou et de la tête — une pièce d'épaule haute, un col — reste au buste.
    neck = joints["neck_01"]
    head_radius = sheet.get("head_radius", 0.21) * scale
    away = _smooth(head_radius, head_radius + 0.06 * unit, np.hypot(x - neck[0], z - neck[2]))
    moved = (upper[:, 4] + upper[:, 5]) * away
    upper[:, 4] *= 1.0 - away
    upper[:, 5] *= 1.0 - away
    upper[:, 3] += moved

    # Les jambes : sous le pli de l'aine, incliné de l'entrejambe vers la hanche.
    hip = joints["thigh_l"]
    crotch_y = hip[1] - 0.05 * 1.8 * unit
    outer = np.clip(np.abs(x) / (1.9 * abs(hip[0])), 0.0, 1.0)
    crease = crotch_y + outer * (0.12 * unit)
    above_crease = _smooth(crease - 0.06 * unit, crease + 0.06 * unit, y)
    left = _smooth(-0.02 * unit, 0.02 * unit, x)
    for name, share in zip(chain, upper.T):
        weights[:, BONE_INDEX[name]] += body * above_crease * share
    for side, sign in SIDES:
        half = left if sign > 0 else 1.0 - left
        knee, ankle = joints[f"calf_{side}"], joints[f"foot_{side}"]
        ball = joints[f"ball_{side}"]
        below_knee = 1.0 - _smooth(knee[1] - 0.06 * unit, knee[1] + 0.06 * unit, y)
        below_ankle = 1.0 - _smooth(ankle[1] - 0.01 * unit, ankle[1] + 0.035 * unit, y)
        toes = _smooth(ball[2] - 0.02 * unit, ball[2] + 0.02 * unit, z) * (
            1.0 - _smooth(ankle[1] - 0.01 * unit, ankle[1] + 0.02 * unit, y))
        leg = body * (1.0 - above_crease) * half
        weights[:, BONE_INDEX[f"thigh_{side}"]] += leg * (1.0 - below_knee)
        weights[:, BONE_INDEX[f"calf_{side}"]] += leg * below_knee * (1.0 - below_ankle)
        weights[:, BONE_INDEX[f"foot_{side}"]] += leg * below_ankle * (1.0 - toes)
        weights[:, BONE_INDEX[f"ball_{side}"]] += leg * below_ankle * toes

    # Ce qui suit rigidement un os (standard, §6 et §8) : un volume de la fiche, pesé à 1.
    stature_origin = np.array([sheet["center_x"], sheet["ground"], 0.0])
    for volume in sheet.get("rigid", []):
        low = (np.array(volume["box"][0]) - stature_origin) * scale
        high = (np.array(volume["box"][1]) - stature_origin) * scale
        inside = np.all((points >= low) & (points <= high), axis=1)
        weights[inside] = 0.0
        weights[inside, BONE_INDEX[volume["bone"]]] = 1.0
    return weights


def top_influences(weights: np.ndarray,
                   silhouette: "Silhouette | None" = None) -> tuple[np.ndarray, np.ndarray]:
    """Les quatre os les plus lourds de chaque sommet, normalisés : somme à 1."""
    order = np.argsort(-weights, axis=1)[:, :MAX_INFLUENCES]
    kept = np.take_along_axis(weights, order, axis=1)
    kept[kept < 1e-4] = 0.0
    total = kept.sum(axis=1, keepdims=True)
    orphan = total[:, 0] <= 0.0
    kept[orphan, 0] = 1.0
    order[orphan, 0] = (silhouette or HUMANOID).bone_index["pelvis"]
    total[orphan] = 1.0
    kept = (kept / total).astype(np.float32)
    # La somme se referme en simple précision sur la première influence.
    kept[:, 0] += 1.0 - kept.sum(axis=1, dtype=np.float32)
    order[kept == 0.0] = 0
    return order.astype(np.uint8), kept


# --- La géométrie des poses ------------------------------------------------------------------------
def rot_x(angle: float) -> np.ndarray:
    c, s = math.cos(angle), math.sin(angle)
    return np.array([[1.0, 0.0, 0.0], [0.0, c, -s], [0.0, s, c]])


def rot_y(angle: float) -> np.ndarray:
    c, s = math.cos(angle), math.sin(angle)
    return np.array([[c, 0.0, s], [0.0, 1.0, 0.0], [-s, 0.0, c]])


def rot_z(angle: float) -> np.ndarray:
    c, s = math.cos(angle), math.sin(angle)
    return np.array([[c, -s, 0.0], [s, c, 0.0], [0.0, 0.0, 1.0]])


def turn(yaw: float, pitch: float, roll: float) -> np.ndarray:
    """Une rotation dite en degrés : lacet (la gauche avance), tangage (penché en avant), roulis
    (penché vers sa gauche)."""
    return (rot_y(-math.radians(yaw)) @ rot_x(math.radians(pitch))
            @ rot_z(-math.radians(roll)))


def rotation_vector(matrix: np.ndarray) -> np.ndarray:
    angle = math.acos(max(-1.0, min(1.0, (np.trace(matrix) - 1.0) / 2.0)))
    if angle < 1e-9:
        return np.zeros(3)
    axis = np.array([matrix[2, 1] - matrix[1, 2], matrix[0, 2] - matrix[2, 0],
                     matrix[1, 0] - matrix[0, 1]])
    norm = np.linalg.norm(axis)
    if norm < 1e-9:
        return np.zeros(3)
    return axis / norm * angle


def from_rotation_vector(vector: np.ndarray) -> np.ndarray:
    angle = float(np.linalg.norm(vector))
    if angle < 1e-12:
        return np.eye(3)
    k = vector / angle
    K = np.array([[0.0, -k[2], k[1]], [k[2], 0.0, -k[0]], [-k[1], k[0], 0.0]])
    return np.eye(3) + math.sin(angle) * K + (1.0 - math.cos(angle)) * (K @ K)


def partial(matrix: np.ndarray, share: float) -> np.ndarray:
    """La fraction `share` de la rotation `matrix`."""
    return from_rotation_vector(rotation_vector(matrix) * share)


def unit_vector(vector: np.ndarray) -> np.ndarray:
    norm = np.linalg.norm(vector)
    return vector / norm if norm > 1e-12 else np.array([0.0, 0.0, 1.0])


def frame(direction: np.ndarray, hint: np.ndarray) -> np.ndarray:
    """Un repère dont le premier axe est `direction` et dont le plan contient `hint`."""
    first = unit_vector(direction)
    across = hint - np.dot(hint, first) * first
    if np.linalg.norm(across) < 1e-6:
        across = np.array([0.0, 1.0, 0.0]) - first[1] * first
        if np.linalg.norm(across) < 1e-6:
            across = np.array([1.0, 0.0, 0.0])
    third = unit_vector(across)
    second = np.cross(third, first)
    return np.stack([first, second, third], axis=1)


def two_bones(root: np.ndarray, target: np.ndarray, upper: float, lower: float,
              pole: np.ndarray, reach: float = 0.995) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Deux os résolus : le coude ou le genou, la cible atteinte, et le vecteur du pli.

    La cible est ramenée dans ce que les deux os peuvent couvrir ; le pli pointe vers `pole`.
    """
    toward = target - root
    distance = float(np.linalg.norm(toward))
    distance = min(max(distance, abs(upper - lower) + 1e-4, 1e-4), reach * (upper + lower))
    direction = unit_vector(toward)
    bend = pole - np.dot(pole, direction) * direction
    bend = unit_vector(bend) if np.linalg.norm(bend) > 1e-6 else frame(direction, pole)[:, 2]
    cosine = (upper * upper + distance * distance - lower * lower) / (2.0 * upper * distance)
    cosine = max(-1.0, min(1.0, cosine))
    middle = root + upper * (cosine * direction + math.sqrt(1.0 - cosine * cosine) * bend)
    return middle, root + direction * distance, bend


class Body:
    """Ce que les poses lisent d'un personnage : ses articulations de repos et ses longueurs."""

    def __init__(self, joints: dict[str, np.ndarray], clearance: float):
        self.joints = joints
        self.offset = {name: joints[name] - joints[parent] if parent else joints[name]
                       for name, parent in BONES}
        self.thigh = float(np.linalg.norm(joints["calf_l"] - joints["thigh_l"]))
        self.calf = float(np.linalg.norm(joints["foot_l"] - joints["calf_l"]))
        self.leg = self.thigh + self.calf
        self.upperarm = float(np.linalg.norm(joints["lowerarm_l"] - joints["upperarm_l"]))
        self.lowerarm = float(np.linalg.norm(joints["hand_l"] - joints["lowerarm_l"]))
        self.arm = self.upperarm + self.lowerarm
        self.hip_x = abs(float(joints["thigh_l"][0]))
        # Ce dont un poignet pendant s'écarte de l'épaule pour ne pas entrer dans la hanche, en
        # longueurs de bras.
        self.clearance = clearance

    def measures(self) -> dict:
        """Ce que le relevé écrit du corps."""
        return {"leg": round(self.leg, 4), "arm": round(self.arm, 4),
                "stance_share": round(stance_share(self), 4)}


def default_pose(body: Body) -> dict:
    """La pose debout, bras le long du corps : ce que chaque clip modifie."""
    arm = np.array([body.clearance, -0.97, 0.10])
    return {
        "pelvis": np.zeros(3),            # déplacement du bassin, en longueurs de jambe
        "auto_height": np.array([1.0]),   # 1 : le bassin descend juste assez pour les deux jambes
        "pelvis_turn": np.zeros(3),       # lacet, tangage, roulis, en degrés
        "chest_turn": np.zeros(3),        # le buste par rapport au bassin
        "head_turn": np.zeros(3),         # la tête par rapport au buste
        "hand_l": arm.copy(), "hand_r": arm.copy(),   # direction du poignet (écart, haut, avant)
        "reach_l": np.array([0.985]), "reach_r": np.array([0.985]),   # extension du bras, de 0 à 1
        "elbow_l": np.array([0.35, -0.25, -1.0]), "elbow_r": np.array([0.35, -0.25, -1.0]),
        "wrist_l": np.zeros(3), "wrist_r": np.zeros(3),   # la main par rapport à l'avant-bras
        "ball_l": np.zeros(3), "ball_r": np.zeros(3),   # la plante, depuis le repos, en jambes
        "foot_l": np.zeros(2), "foot_r": np.zeros(2),   # tangage (talon levé) et lacet, en degrés
        "leg_space": np.array([0.0]),     # 0 : les pieds visent le sol ; 1 : ils suivent le bassin
        "knee_l": np.array([0.12, 0.0, 1.0]), "knee_r": np.array([0.12, 0.0, 1.0]),
    }


def solve_pose(body: Body, pose: dict) -> tuple[dict[str, np.ndarray], np.ndarray]:
    """Les rotations **monde** de chaque os et la position du bassin, pour une pose en cibles."""
    J = body.joints
    rotation = {name: np.eye(3) for name, _ in BONES}
    pelvis_rotation = turn(*pose["pelvis_turn"])
    chest = turn(*pose["chest_turn"])
    rotation["pelvis"] = pelvis_rotation
    rotation["spine_01"] = pelvis_rotation @ partial(chest, 1.0 / 3.0)
    rotation["spine_02"] = pelvis_rotation @ partial(chest, 2.0 / 3.0)
    rotation["spine_03"] = pelvis_rotation @ chest
    head = turn(*pose["head_turn"])
    rotation["neck_01"] = rotation["spine_03"] @ partial(head, 0.5)
    rotation["head"] = rotation["spine_03"] @ head

    follow = float(pose["leg_space"][0])
    base = partial(pelvis_rotation, follow)
    feet, heading = {}, {}
    for side, sign in SIDES:
        pitch, yaw = pose[f"foot_{side}"]
        # Lacet positif : la pointe du pied s'ouvre vers l'extérieur.
        heading[side] = base @ rot_y(math.radians(sign * yaw))
        feet[side] = heading[side] @ rot_x(math.radians(pitch))

    def ball_target(side: str, sign: float, pelvis_position: np.ndarray) -> np.ndarray:
        offset = pose[f"ball_{side}"] * body.leg * np.array([sign, 1.0, 1.0])
        world = J[f"ball_{side}"] + offset
        carried = pelvis_position + pelvis_rotation @ (J[f"ball_{side}"] - J["pelvis"] + offset)
        return (1.0 - follow) * world + follow * carried

    pelvis_position = J["pelvis"] + pose["pelvis"] * body.leg
    automatic = float(np.clip(pose["auto_height"][0], 0.0, 1.0))
    if automatic > 0.0:
        # Le bassin descend juste assez pour que les deux jambes atteignent leurs chevilles.
        limit = math.inf
        for side, sign in SIDES:
            ankle = ball_target(side, sign, pelvis_position) + feet[side] @ (
                J[f"foot_{side}"] - J[f"ball_{side}"])
            hip = pelvis_position + pelvis_rotation @ (J[f"thigh_{side}"] - J["pelvis"])
            flat = math.hypot(ankle[0] - hip[0], ankle[2] - hip[2])
            span = 0.992 * body.leg
            rise = math.sqrt(max(span * span - flat * flat, 0.0))
            limit = min(limit, ankle[1] + rise - (hip[1] - pelvis_position[1]))
        lowered = min(pelvis_position[1], limit)
        pelvis_position = pelvis_position.copy()
        pelvis_position[1] = (1.0 - automatic) * pelvis_position[1] + automatic * lowered

    for side, sign in SIDES:
        foot_rotation = feet[side]
        ankle = ball_target(side, sign, pelvis_position) + foot_rotation @ (
            J[f"foot_{side}"] - J[f"ball_{side}"])
        hip = pelvis_position + pelvis_rotation @ (J[f"thigh_{side}"] - J["pelvis"])
        pole = heading[side] @ (pose[f"knee_{side}"] * np.array([sign, 1.0, 1.0]))
        knee, reached, bend = two_bones(hip, ankle, body.thigh, body.calf, pole)
        forward = np.array([0.0, 0.0, 1.0])
        rotation[f"thigh_{side}"] = frame(knee - hip, bend) @ frame(
            body.offset[f"calf_{side}"], forward).T
        rotation[f"calf_{side}"] = frame(reached - knee, bend) @ frame(
            body.offset[f"foot_{side}"], forward).T
        rotation[f"foot_{side}"] = foot_rotation
        # Talon levé, les orteils restent à plat ; orteils levés, ils suivent le pied.
        rotation[f"ball_{side}"] = (heading[side] if pose[f"foot_{side}"][0] > 0.0
                                    else foot_rotation)

    # Les bras : le poignet se place depuis l'épaule, dans le repère du buste.
    chest_rotation = rotation["spine_03"]
    spine_03 = pelvis_position
    for name in ("spine_01", "spine_02", "spine_03"):
        spine_03 = spine_03 + rotation[PARENT[name]] @ body.offset[name]
    back = np.array([0.0, 0.0, -1.0])
    for side, sign in SIDES:
        mirror = np.array([sign, 1.0, 1.0])
        rotation[f"clavicle_{side}"] = chest_rotation
        shoulder = (spine_03 + chest_rotation @ body.offset[f"clavicle_{side}"]
                    + chest_rotation @ body.offset[f"upperarm_{side}"])
        extension = float(np.clip(pose[f"reach_{side}"][0], 0.25, 0.985))
        direction = chest_rotation @ (unit_vector(pose[f"hand_{side}"]) * mirror)
        pole = chest_rotation @ (pose[f"elbow_{side}"] * mirror)
        elbow, wrist, bend = two_bones(shoulder, shoulder + direction * extension * body.arm,
                                       body.upperarm, body.lowerarm, pole, reach=0.985)
        rotation[f"upperarm_{side}"] = frame(elbow - shoulder, bend) @ frame(
            body.offset[f"lowerarm_{side}"], back).T
        rotation[f"lowerarm_{side}"] = frame(wrist - elbow, bend) @ frame(
            body.offset[f"hand_{side}"], back).T
        bend_yaw, bend_pitch, bend_roll = pose[f"wrist_{side}"]
        rotation[f"hand_{side}"] = rotation[f"lowerarm_{side}"] @ turn(
            sign * bend_yaw, bend_pitch, sign * bend_roll)
        for finger in FINGERS:
            for phalanx in ("01", "02", "03"):
                rotation[f"{finger}_{phalanx}_{side}"] = rotation[f"hand_{side}"]
    return rotation, pelvis_position


def joint_positions(body, rotation: dict[str, np.ndarray], pelvis_position: np.ndarray,
                    bones: list[tuple[str, str]] = BONES) -> dict[str, np.ndarray]:
    """Les articulations **monde** d'une pose : la racine à l'origine, le bassin placé, le reste
    par la chaîne des parents (`bones` les donne parents avant enfants)."""
    position = {"Root": np.zeros(3), "pelvis": pelvis_position}
    for name, parent in bones:
        if name not in position:
            position[name] = position[parent] + rotation[parent] @ body.offset[name]
    return position


# --- Les clips, en cibles --------------------------------------------------------------------------
def _hermite(keys: list[tuple[float, np.ndarray]], time: float) -> np.ndarray:
    """La valeur à `time` d'une suite de clés, par une cubique qui passe par chacune."""
    if time <= keys[0][0]:
        return keys[0][1]
    if time >= keys[-1][0]:
        return keys[-1][1]
    index = max(i for i, (t, _) in enumerate(keys) if t <= time)
    t0, v0 = keys[index]
    t1, v1 = keys[index + 1]

    def slope(i: int) -> np.ndarray:
        if i == 0 or i == len(keys) - 1:
            return np.zeros_like(keys[i][1])
        before, after = keys[i - 1], keys[i + 1]
        # Une clé répétée est une tenue : la pente y est nulle, la pose ne déborde pas.
        if np.allclose(keys[i][1], before[1]) or np.allclose(keys[i][1], after[1]):
            return np.zeros_like(keys[i][1])
        return (after[1] - before[1]) / (after[0] - before[0])

    span = t1 - t0
    u = (time - t0) / span
    return ((2 * u ** 3 - 3 * u ** 2 + 1) * v0 + (u ** 3 - 2 * u ** 2 + u) * span * slope(index)
            + (-2 * u ** 3 + 3 * u ** 2) * v1 + (u ** 3 - u ** 2) * span * slope(index + 1))


def keyed(body: Body, start: dict, keys: list[tuple[float, dict]]):
    """Un clip dit en poses clés : chaque clé ne nomme que ce qu'elle change depuis la précédente."""
    full = []
    current = {name: np.array(value, dtype=np.float64).reshape(-1)
               for name, value in start.items()}
    for time, changes in keys:
        current = dict(current)
        for name, value in changes.items():
            current[name] = np.array(value, dtype=np.float64).reshape(-1)
        full.append((time, current))

    def pose_at(time: float) -> dict:
        return {name: _hermite([(t, pose[name]) for t, pose in full], time) for name in start}

    return pose_at


def idle_pose(body: Body, phase: float) -> dict:
    """Le repos : une respiration, un léger report du poids. `phase` de 0 à 1."""
    pose = default_pose(body)
    breath = math.sin(2.0 * math.pi * phase)
    slow = math.cos(2.0 * math.pi * phase)
    pose["chest_turn"] = np.array([0.0, 1.2 * breath - 0.5, 0.0])
    pose["head_turn"] = np.array([1.5 * slow, -0.8 * breath, 0.0])
    pose["pelvis"] = np.array([0.006 * slow, -0.004 * (1.0 + breath), 0.0])
    for side in ("l", "r"):
        pose[f"hand_{side}"] = pose[f"hand_{side}"] + np.array([0.015 * breath, 0.0, 0.0])
    return pose


def stance_share(body: Body) -> float:
    """La part du cycle de marche où un pied est posé : une jambe courte la raccourcit, et le
    cycle couvre toujours une case."""
    stride = WALK_SPEED * CLIPS[1]["duration"]
    return float(np.clip(0.8 * body.leg / stride, 0.28, 0.46))


def walk_pose(body: Body, phase: float) -> dict:
    """La marche sur place : appuis alternés, le pied posé recule à vitesse constante."""
    pose = default_pose(body)
    duration = CLIPS[1]["duration"]
    share = stance_share(body)
    travel = WALK_SPEED * share * duration / body.leg     # en longueurs de jambe
    # Deux rythmes : `ahead` vaut 1 quand la jambe gauche est en avant, `planted` quand le pied
    # gauche est au milieu de son appui.
    ahead = math.cos(2.0 * math.pi * (phase - (share - 0.5) / 2.0))
    planted = math.cos(2.0 * math.pi * (phase - share / 2.0))
    for side, _ in SIDES:
        own = (phase + (0.0 if side == "l" else 0.5)) % 1.0
        # Les pieds se rapprochent de l'axe : on ne marche pas les jambes écartées du repos.
        lateral = (body.hip_x * 1.15 - abs(float(body.joints[f"ball_{side}"][0]))) / body.leg
        if own < share:
            along = own / share
            forward = travel * (0.5 - along)
            lift = 0.0
            pitch = 38.0 * float(_smooth(0.55, 1.0, np.array(along)))
        else:
            along = (own - share) / (1.0 - share)
            forward = travel * (-0.5 + (1.0 - math.cos(math.pi * along)) / 2.0)
            lift = 0.10 * math.sin(math.pi * along) + 0.03 * math.sin(math.pi * along) ** 2
            pitch = float(_hermite([(0.0, np.array(38.0)), (0.3, np.array(52.0)),
                                    (0.75, np.array(-14.0)), (1.0, np.array(0.0))], along))
        pose[f"ball_{side}"] = np.array([lateral, lift, forward])
        pose[f"foot_{side}"] = np.array([pitch, 0.0])
        # Le bras recule quand la jambe du même côté avance.
        own_cos = ahead if side == "l" else -ahead
        angle = math.radians(-32.0 * own_cos + 6.0)
        pose[f"hand_{side}"] = np.array([body.clearance + 0.04, -math.cos(angle), math.sin(angle)])
        pose[f"reach_{side}"] = np.array([0.90 - 0.07 * max(0.0, -own_cos)])
    pose["pelvis"] = np.array([0.012 * planted, -0.012, 0.0])
    pose["pelvis_turn"] = np.array([7.0 * ahead, 5.0, -1.5 * planted])
    pose["chest_turn"] = np.array([-13.0 * ahead, 5.0, 2.0 * planted])
    pose["head_turn"] = np.array([6.0 * ahead, -7.0, 0.0])
    return pose


def attack_clip(body: Body):
    """L'attaque, mains vides : l'élan, le coup de haut en bas de la main droite, le retour."""
    rest = idle_pose(body, 0.0)
    return keyed(body, rest, [
        (0.0, {}),
        (0.26, {"pelvis": [0.0, -0.04, -0.06], "pelvis_turn": [-12.0, -2.0, 0.0],
                "chest_turn": [-28.0, -8.0, 0.0], "head_turn": [30.0, 4.0, 0.0],
                "hand_r": [0.45, 0.80, -0.40], "reach_r": [0.72],
                "elbow_r": [0.8, -0.2, -0.6],
                "hand_l": [0.10, -0.25, 0.85], "reach_l": [0.66],
                "ball_l": [0.0, 0.0, 0.16], "ball_r": [0.0, 0.0, -0.06],
                "foot_r": [0.0, 25.0]}),
        (0.34, {"pelvis": [0.0, -0.05, 0.02], "pelvis_turn": [-2.0, 4.0, 0.0],
                "chest_turn": [0.0, 6.0, 0.0], "head_turn": [8.0, 0.0, 0.0],
                "hand_r": [0.12, 0.72, 0.62], "reach_r": [0.86],
                "hand_l": [0.22, -0.45, 0.45], "reach_l": [0.62]}),
        (0.40, {"pelvis": [0.0, -0.09, 0.14], "pelvis_turn": [12.0, 10.0, 0.0],
                "chest_turn": [30.0, 16.0, 0.0], "head_turn": [-26.0, -8.0, 0.0],
                "hand_r": [-0.22, -0.12, 0.95], "reach_r": [0.96],
                "elbow_r": [0.6, -0.5, -0.6],
                "hand_l": [0.30, -0.75, -0.30], "reach_l": [0.66],
                "foot_r": [22.0, 25.0]}),
        (0.50, {"pelvis": [0.0, -0.08, 0.15], "pelvis_turn": [16.0, 8.0, 0.0],
                "chest_turn": [38.0, 14.0, 0.0], "head_turn": [-30.0, -10.0, 0.0],
                "hand_r": [-0.38, -0.62, 0.68], "reach_r": [0.92],
                "hand_l": [0.32, -0.78, -0.34], "reach_l": [0.68]}),
        (0.9, {name: value for name, value in rest.items()}),
    ])


def cast_clip(body: Body):
    """Le sort : les mains se rassemblent devant la poitrine, montent, la droite projette."""
    rest = idle_pose(body, 0.0)
    return keyed(body, rest, [
        (0.0, {}),
        (0.28, {"pelvis": [0.0, -0.04, -0.02], "chest_turn": [0.0, 9.0, 0.0],
                "head_turn": [0.0, 8.0, 0.0],
                "hand_l": [-0.20, -0.22, 0.80], "reach_l": [0.50],
                "hand_r": [-0.20, -0.22, 0.80], "reach_r": [0.50],
                "elbow_l": [1.0, -0.6, -0.2], "elbow_r": [1.0, -0.6, -0.2]}),
        (0.42, {"pelvis": [0.0, -0.03, -0.05], "pelvis_turn": [-8.0, -3.0, 0.0],
                "chest_turn": [-18.0, -4.0, 0.0], "head_turn": [16.0, 0.0, 0.0],
                "hand_l": [0.0, 0.10, 0.90], "reach_l": [0.58],
                "hand_r": [0.30, 0.30, 0.25], "reach_r": [0.52],
                "elbow_r": [0.9, -0.3, -0.8], "ball_l": [0.0, 0.0, 0.10]}),
        (0.50, {"pelvis": [0.0, -0.07, 0.10], "pelvis_turn": [10.0, 6.0, 0.0],
                "chest_turn": [24.0, 8.0, 0.0], "head_turn": [-22.0, -4.0, 0.0],
                "hand_r": [-0.10, 0.12, 1.0], "reach_r": [0.97], "wrist_r": [0.0, -55.0, 0.0],
                "hand_l": [0.45, -0.35, -0.30], "reach_l": [0.72],
                "foot_r": [18.0, 15.0]}),
        (0.68, {"pelvis": [0.0, -0.07, 0.10], "pelvis_turn": [10.0, 6.0, 0.0],
                "chest_turn": [24.0, 8.0, 0.0], "head_turn": [-22.0, -4.0, 0.0],
                "hand_r": [-0.10, 0.12, 1.0], "reach_r": [0.97], "wrist_r": [0.0, -55.0, 0.0],
                "hand_l": [0.45, -0.35, -0.30], "reach_l": [0.72]}),
        (1.0, {name: value for name, value in rest.items()}),
    ])


def hit_clip(body: Body):
    """Le coup encaissé : le buste part en arrière, un pied recule, le corps se reprend."""
    rest = idle_pose(body, 0.0)
    return keyed(body, rest, [
        (0.0, {}),
        (0.10, {"pelvis": [0.0, -0.04, -0.05], "pelvis_turn": [-6.0, -6.0, 0.0],
                "chest_turn": [-16.0, -18.0, -4.0], "head_turn": [-8.0, -16.0, 0.0],
                "hand_l": [0.50, -0.55, 0.40], "reach_l": [0.72],
                "hand_r": [0.42, -0.70, 0.30], "reach_r": [0.76]}),
        (0.24, {"pelvis": [0.0, -0.08, -0.12], "pelvis_turn": [-10.0, -4.0, 0.0],
                "chest_turn": [-20.0, -10.0, -5.0], "head_turn": [4.0, 6.0, 0.0],
                "hand_l": [0.55, -0.45, 0.30], "reach_l": [0.66],
                "hand_r": [0.40, -0.80, 0.05], "reach_r": [0.80],
                "ball_r": [0.02, 0.0, -0.26], "foot_r": [0.0, 12.0]}),
        (0.40, {"pelvis": [0.0, -0.06, -0.10], "pelvis_turn": [-4.0, 4.0, 0.0],
                "chest_turn": [-6.0, 8.0, 0.0], "head_turn": [0.0, 4.0, 0.0],
                "hand_l": [0.30, -0.85, 0.22], "reach_l": [0.84],
                "hand_r": [0.28, -0.92, 0.10], "reach_r": [0.88]}),
        (0.6, {name: value for name, value in rest.items()}),
    ])


def death_clip(body: Body):
    """La chute : le coup, les genoux cèdent, le corps part en arrière et reste couché."""
    rest = idle_pose(body, 0.0)
    lying = {"pelvis": [0.0, -0.82, -0.42], "auto_height": [0.0], "leg_space": [1.0],
             "pelvis_turn": [0.0, -90.0, 0.0], "chest_turn": [0.0, 2.0, 0.0],
             "head_turn": [-18.0, 4.0, 0.0],
             "hand_l": [0.80, -0.50, -0.12], "reach_l": [0.90],
             "hand_r": [0.62, -0.72, -0.12], "reach_r": [0.86],
             "elbow_l": [0.5, -0.6, -1.0], "elbow_r": [0.5, -0.6, -1.0],
             "ball_l": [0.06, 0.01, 0.0], "ball_r": [0.02, 0.01, 0.0],
             "foot_l": [18.0, 22.0], "foot_r": [24.0, 14.0],
             "knee_l": [0.5, 0.0, 1.0], "knee_r": [0.3, 0.0, 1.0]}
    return keyed(body, rest, [
        (0.0, {"auto_height": [1.0]}),
        (0.14, {"pelvis": [0.0, -0.03, -0.04], "chest_turn": [8.0, -20.0, 4.0],
                "head_turn": [6.0, -22.0, 0.0],
                "hand_l": [0.55, -0.55, 0.35], "reach_l": [0.70],
                "hand_r": [0.50, -0.60, 0.30], "reach_r": [0.72]}),
        (0.42, {"pelvis": [0.02, -0.30, -0.10], "auto_height": [0.0],
                "pelvis_turn": [0.0, -18.0, 3.0], "chest_turn": [4.0, -6.0, 6.0],
                "head_turn": [0.0, 14.0, 0.0],
                "hand_l": [0.45, -0.85, 0.10], "reach_l": [0.86],
                "hand_r": [0.40, -0.90, 0.05], "reach_r": [0.88]}),
        (0.70, {"pelvis": [0.02, -0.66, -0.28], "leg_space": [0.75],
                "pelvis_turn": [0.0, -58.0, 2.0], "chest_turn": [0.0, 12.0, 2.0],
                "head_turn": [0.0, 20.0, 0.0],
                "hand_l": [0.70, -0.30, 0.45], "reach_l": [0.78],
                "hand_r": [0.62, -0.42, 0.40], "reach_r": [0.80],
                "ball_l": [0.05, 0.10, 0.04], "ball_r": [0.02, 0.04, 0.02],
                "foot_l": [10.0, 18.0], "foot_r": [14.0, 10.0]}),
        (0.92, lying),
        (0.99, {"chest_turn": [0.0, -3.0, 0.0], "head_turn": [-18.0, -4.0, 0.0],
                "hand_l": [0.80, -0.50, -0.02], "hand_r": [0.62, -0.72, -0.02]}),
        (1.08, lying),
        (1.2, lying),
    ])


def clip_poses(body: Body, clip: dict) -> list[tuple[float, dict]]:
    """Les poses d'un clip, à pas fixe : la première à 0, la dernière à sa durée."""
    count = round(clip["duration"] * SAMPLES_PER_SECOND)
    times = [index / SAMPLES_PER_SECOND for index in range(count + 1)]
    name = clip["name"]
    if name == "idle":
        return [(time, idle_pose(body, time / clip["duration"])) for time in times]
    if name == "walk":
        return [(time, walk_pose(body, time / clip["duration"])) for time in times]
    builder = {"attack": attack_clip, "cast": cast_clip, "hit": hit_clip, "death": death_clip}
    pose_at = builder[name](body)
    return [(time, pose_at(time)) for time in times]


# --- L'évaluation du maillage ----------------------------------------------------------------------
def skin_points(points: np.ndarray, joints_of: np.ndarray, weights: np.ndarray,
                rest: np.ndarray, rotations: np.ndarray, positions: np.ndarray) -> np.ndarray:
    """Les sommets déformés : chaque influence tourne le sommet autour de son articulation."""
    result = np.zeros_like(points)
    for column in range(weights.shape[1]):
        bone = joints_of[:, column]
        local = points - rest[bone]
        moved = np.einsum("nij,nj->ni", rotations[bone], local) + positions[bone]
        result += weights[:, column:column + 1] * moved
    return result


def quaternion(matrix: np.ndarray) -> np.ndarray:
    """Le quaternion (x, y, z, w) d'une rotation."""
    m = matrix
    trace = m[0, 0] + m[1, 1] + m[2, 2]
    if trace > 0.0:
        s = math.sqrt(trace + 1.0) * 2.0
        q = [(m[2, 1] - m[1, 2]) / s, (m[0, 2] - m[2, 0]) / s, (m[1, 0] - m[0, 1]) / s, 0.25 * s]
    elif m[0, 0] > m[1, 1] and m[0, 0] > m[2, 2]:
        s = math.sqrt(1.0 + m[0, 0] - m[1, 1] - m[2, 2]) * 2.0
        q = [0.25 * s, (m[0, 1] + m[1, 0]) / s, (m[0, 2] + m[2, 0]) / s, (m[2, 1] - m[1, 2]) / s]
    elif m[1, 1] > m[2, 2]:
        s = math.sqrt(1.0 + m[1, 1] - m[0, 0] - m[2, 2]) * 2.0
        q = [(m[0, 1] + m[1, 0]) / s, 0.25 * s, (m[1, 2] + m[2, 1]) / s, (m[0, 2] - m[2, 0]) / s]
    else:
        s = math.sqrt(1.0 + m[2, 2] - m[0, 0] - m[1, 1]) * 2.0
        q = [(m[0, 2] + m[2, 0]) / s, (m[1, 2] + m[2, 1]) / s, 0.25 * s, (m[1, 0] - m[0, 1]) / s]
    q = np.array(q)
    return q / np.linalg.norm(q)


def rotation_matrix(q) -> np.ndarray:
    """La rotation d'un quaternion (x, y, z, w)."""
    x, y, z, w = (float(value) for value in np.array(q, dtype=np.float64) / np.linalg.norm(q))
    return np.array([
        [1.0 - 2.0 * (y * y + z * z), 2.0 * (x * y - z * w), 2.0 * (x * z + y * w)],
        [2.0 * (x * y + z * w), 1.0 - 2.0 * (x * x + z * z), 2.0 * (y * z - x * w)],
        [2.0 * (x * z - y * w), 2.0 * (y * z + x * w), 1.0 - 2.0 * (x * x + y * y)]])


def load_retouch(path: Path, silhouette: "Silhouette | None" = None) -> dict:
    """La fiche de retouche : les clips réglés dans Blender (`retouch_character.py`)."""
    retouch = json.loads(path.read_text(encoding="utf-8"))
    if retouch.get("version") != RETOUCH_VERSION:
        raise SystemExit(f"fiche de retouche de version {retouch.get('version')}")
    known = {clip["name"] for clip in (silhouette or HUMANOID).clips}
    unknown = sorted(set(retouch.get("clips", {})) - known)
    if unknown:
        raise SystemExit("fiche de retouche : clip(s) inconnu(s) : " + ", ".join(unknown))
    return retouch


def retouched_poses(body, clip: dict, retouch: dict, silhouette: "Silhouette | None" = None):
    """Les poses d'un clip retouché : rotations **monde** de chaque os et position du bassin.

    La fiche donne des rotations locales et la position du bassin ; celle-ci est rapportée à la
    jambe du personnage, pour qu'une retouche réglée sur l'un se rejoue sur un autre.
    """
    silhouette = silhouette or HUMANOID
    curves = retouch["clips"][clip["name"]]
    count = round(clip["duration"] * SAMPLES_PER_SECOND)
    if len(curves["times"]) != count + 1:
        raise SystemExit(f"fiche de retouche : {clip['name']} a {len(curves['times'])} "
                         f"échantillons, {count + 1} attendus")
    missing = [name for name in silhouette.animated if name not in curves["rotations"]]
    if missing:
        raise SystemExit(f"fiche de retouche : {clip['name']} sans " + ", ".join(missing))
    ratio = body.leg / float(retouch["leg"])
    origin = np.array(retouch["pelvis_rest"], dtype=np.float64)
    for index in range(count + 1):
        rotation = {"Root": np.eye(3)}
        for name, parent in silhouette.bones[1:]:
            rotation[name] = rotation[parent]
            if name in curves["rotations"]:
                rotation[name] = rotation[parent] @ rotation_matrix(
                    curves["rotations"][name][index])
        pelvis = body.joints["pelvis"] + (np.array(curves["pelvis"][index]) - origin) * ratio
        yield index / SAMPLES_PER_SECOND, rotation, pelvis


def bake_clips(body, points: np.ndarray, joints_of: np.ndarray, weights: np.ndarray,
               retouch: dict | None = None,
               silhouette: "Silhouette | None" = None) -> tuple[dict, dict]:
    """Chaque clip en canaux : les rotations locales des os animés, la position du bassin.

    La hauteur du bassin est recalée image par image : le point le plus bas du maillage évalué
    touche le sol. Le moteur interpole linéairement entre deux images : le maillage y est évalué
    aussi, à mi-chemin, et les deux images voisines remontent de ce qui s'y enfonce. Un clip de
    la fiche de retouche remplace le clip posé par cibles, et reçoit le même recalage. Rend aussi
    ce qui a été mesuré, clip par clip.
    """
    silhouette = silhouette or HUMANOID
    bones, animated = silhouette.bones, silhouette.animated
    rest = np.stack([body.joints[name] for name, _ in bones])
    baked, measured = {}, {}

    def lowest_point(rotation: dict, pelvis_position: np.ndarray) -> float:
        position = joint_positions(body, rotation, pelvis_position, bones)
        world = np.stack([rotation[name] for name, _ in bones])
        where = np.stack([position[name] for name, _ in bones])
        return float(skin_points(points, joints_of, weights, rest, world, where)[:, 1].min())

    for clip in silhouette.clips:
        times, translations, lifts = [], [], []
        rotations_of = {name: [] for name in animated}
        retouched = retouch is not None and clip["name"] in retouch.get("clips", {})
        solved = (retouched_poses(body, clip, retouch, silhouette) if retouched else
                  ((time, *silhouette.solve_pose(body, pose))
                   for time, pose in silhouette.clip_poses(body, clip)))
        for time, rotation, pelvis_position in solved:
            lowest = lowest_point(rotation, pelvis_position)
            lift = max(-lowest, 0.0) if clip["name"] in silhouette.lift_only else -lowest
            lifts.append(lift)
            times.append(time)
            translations.append(pelvis_position + np.array([0.0, lift, 0.0]))
            for name in animated:
                local = rotation[silhouette.parent[name]].T @ rotation[name]
                q = quaternion(local)
                previous = rotations_of[name][-1] if rotations_of[name] else None
                if previous is not None and float(np.dot(previous, q)) < 0.0:
                    q = -q
                rotations_of[name].append(q)
        # Entre deux images, telles que le moteur les interpole.
        for index in range(len(times) - 1):
            rotation = {"Root": np.eye(3)}
            for name, parent in bones[1:]:
                rotation[name] = rotation[parent]
                if name in rotations_of:
                    between = rotations_of[name][index] + rotations_of[name][index + 1]
                    rotation[name] = rotation[parent] @ rotation_matrix(between)
            pelvis_position = (translations[index] + translations[index + 1]) / 2.0
            extra = max(-lowest_point(rotation, pelvis_position), 0.0)
            if extra > 1e-5:
                for neighbour in (index, index + 1):
                    lifts[neighbour] += extra
                    translations[neighbour] = translations[neighbour] + np.array([0.0, extra, 0.0])
        if clip["loop"]:
            # Une boucle se referme : la dernière pose est la première, au bit près.
            translations[-1] = translations[0]
            for name in animated:
                rotations_of[name][-1] = rotations_of[name][0]
        baked[clip["name"]] = {
            "times": np.array(times, dtype=np.float32),
            "translation": np.array(translations, dtype=np.float32),
            "rotations": {name: np.array(values, dtype=np.float32)
                          for name, values in rotations_of.items()},
        }
        measured[clip["name"]] = {"samples": len(times), "retouched": retouched,
                                  "ground_shift_min_mm": round(min(lifts) * 1000.0, 2),
                                  "ground_shift_max_mm": round(max(lifts) * 1000.0, 2)}
    return baked, measured


# --- L'écriture du .glb ----------------------------------------------------------------------------
class _Buffer:
    def __init__(self):
        self.data = bytearray()
        self.views = []
        self.accessors = []

    def view(self, raw: bytes, target: int | None = None) -> int:
        self.data += b"\x00" * ((4 - len(self.data) % 4) % 4)
        view = {"buffer": 0, "byteOffset": len(self.data), "byteLength": len(raw)}
        if target is not None:
            view["target"] = target
        self.data += raw
        self.views.append(view)
        return len(self.views) - 1

    def accessor(self, array: np.ndarray, kind: str, component: int, target: int | None = None,
                 bounds: bool = False) -> int:
        array = np.ascontiguousarray(array)
        accessor = {"bufferView": self.view(array.tobytes(), target), "componentType": component,
                    "count": int(array.shape[0]), "type": kind}
        if bounds:
            flat = array.reshape(array.shape[0], -1)
            accessor["min"] = [float(v) for v in flat.min(axis=0)]
            accessor["max"] = [float(v) for v in flat.max(axis=0)]
        self.accessors.append(accessor)
        return len(self.accessors) - 1


def build_glb(mesh: dict, points: np.ndarray, joints_of: np.ndarray, weights: np.ndarray,
              body, baked: dict, image: tuple[bytes, str] | None, name: str,
              silhouette: "Silhouette | None" = None) -> bytes:
    """Le `.glb` autonome : un maillage, le squelette de la silhouette, ses animations, une
    texture."""
    silhouette = silhouette or HUMANOID
    BONES, BONE_INDEX, CLIPS, ANIMATED = (silhouette.bones, silhouette.bone_index,
                                          silhouette.clips, silhouette.animated)
    buffer = _Buffer()
    attributes = {
        "POSITION": buffer.accessor(points.astype(np.float32), "VEC3", 5126, 34962, bounds=True),
        "NORMAL": buffer.accessor(mesh["normals"], "VEC3", 5126, 34962),
        "TEXCOORD_0": buffer.accessor(mesh["uvs"], "VEC2", 5126, 34962),
        "JOINTS_0": buffer.accessor(joints_of.astype(np.uint8), "VEC4", 5121, 34962),
        "WEIGHTS_0": buffer.accessor(weights.astype(np.float32), "VEC4", 5126, 34962),
    }
    indices = buffer.accessor(mesh["indices"].astype(np.uint32).reshape(-1, 1), "SCALAR", 5125,
                              34963)
    inverse = np.tile(np.eye(4, dtype=np.float32), (len(BONES), 1, 1))
    for index, (bone, _) in enumerate(BONES):
        inverse[index, :3, 3] = -body.joints[bone]
    # glTF range ses matrices par colonnes.
    inverse_bind = buffer.accessor(inverse.transpose(0, 2, 1).reshape(len(BONES), 16), "MAT4",
                                   5126)

    nodes = []
    for bone, _ in BONES:
        node = {"name": bone}
        offset = body.offset[bone]
        if np.any(np.abs(offset) > 0.0):
            node["translation"] = [float(v) for v in offset.astype(np.float32)]
        children = [BONE_INDEX[child] for child, parent in BONES if parent == bone]
        if children:
            node["children"] = children
        nodes.append(node)
    mesh_node = len(nodes)
    nodes.append({"name": name, "mesh": 0, "skin": 0})

    animations = []
    for clip in CLIPS:
        tracks = baked[clip["name"]]
        times = buffer.accessor(tracks["times"].reshape(-1, 1), "SCALAR", 5126, bounds=True)
        samplers, channels = [], []

        def channel(bone: str, path: str, values: np.ndarray, kind: str,
                    samplers=samplers, channels=channels, times=times) -> None:
            samplers.append({"input": times, "interpolation": "LINEAR",
                             "output": buffer.accessor(values, kind, 5126)})
            channels.append({"sampler": len(samplers) - 1,
                             "target": {"node": BONE_INDEX[bone], "path": path}})

        channel("pelvis", "translation", tracks["translation"], "VEC3")
        for bone in ANIMATED:
            channel(bone, "rotation", tracks["rotations"][bone], "VEC4")
        animations.append({"name": clip["name"], "samplers": samplers, "channels": channels})

    document = {
        "asset": {"version": "2.0", "generator": "rig_character.py"},
        "scene": 0,
        "scenes": [{"nodes": [BONE_INDEX["Root"], mesh_node]}],
        "nodes": nodes,
        "meshes": [{"name": name, "primitives": [
            {"attributes": attributes, "indices": indices, "material": 0, "mode": 4}]}],
        "skins": [{"name": silhouette.name, "joints": list(range(len(BONES))),
                   "skeleton": BONE_INDEX["Root"], "inverseBindMatrices": inverse_bind}],
        "animations": animations,
        "materials": [{"name": "base-color", "pbrMetallicRoughness": {
            "metallicFactor": 0.0, "roughnessFactor": 1.0}}],
    }
    if image is not None:
        pixels, mime = image
        document["images"] = [{"bufferView": buffer.view(pixels), "mimeType": mime or "image/jpeg"}]
        document["samplers"] = [{"magFilter": 9729, "minFilter": 9987}]
        document["textures"] = [{"sampler": 0, "source": 0}]
        document["materials"][0]["pbrMetallicRoughness"]["baseColorTexture"] = {"index": 0}
    document["bufferViews"] = buffer.views
    document["accessors"] = buffer.accessors
    document["buffers"] = [{"byteLength": len(buffer.data) + ((4 - len(buffer.data) % 4) % 4)}]
    return reduce_model.write_glb(document, bytes(buffer.data))


# --- La planche de liaison -------------------------------------------------------------------------
def liaison_board(points: np.ndarray, joints_of: np.ndarray, weights: np.ndarray,
                  joints: dict[str, np.ndarray], path: Path,
                  silhouette: "Silhouette | None" = None) -> None:
    """Face et profil du maillage, teinté par son os dominant, le squelette par-dessus : ce qui
    se relit pour corriger une fiche."""
    from PIL import Image, ImageDraw

    silhouette = silhouette or HUMANOID
    BONES = silhouette.bones
    size, margin = 900, 30
    span = max(float(np.ptp(points[:, 0])), float(np.ptp(points[:, 2])),
               float(points[:, 1].max()), 0.1)
    scale = (size - 2 * margin) / span
    palette = np.array([[(37 * i) % 200 + 40, (91 * i) % 200 + 40, (53 * i) % 200 + 40]
                        for i in range(len(BONES))], dtype=np.uint8)
    colors = palette[joints_of[np.arange(len(points)), np.argmax(weights, axis=1)]]
    board = Image.new("RGB", (size * 2, size), (24, 24, 28))
    for panel, (axis, depth_sign) in enumerate(((0, 1.0), (2, -1.0))):
        image = np.full((size, size, 3), 24, dtype=np.uint8)
        depth = points[:, 2] * depth_sign if axis == 0 else points[:, 0]
        order = np.argsort(depth)
        horizontal = points[:, axis] if axis == 0 else -points[:, 2]
        u = np.clip((size / 2 + horizontal * scale).astype(int), 0, size - 1)
        v = np.clip((size - margin - points[:, 1] * scale).astype(int), 0, size - 1)
        image[v[order], u[order]] = colors[order]
        tile = Image.fromarray(image)
        draw = ImageDraw.Draw(tile)
        for bone, parent in BONES:
            if not silhouette.drawn(bone):
                continue
            a = joints[bone]
            px = size / 2 + (a[0] if axis == 0 else -a[2]) * scale
            py = size - margin - a[1] * scale
            if parent:
                b = joints[parent]
                qx = size / 2 + (b[0] if axis == 0 else -b[2]) * scale
                qy = size - margin - b[1] * scale
                draw.line([(qx, qy), (px, py)], fill=(255, 255, 255), width=2)
            draw.ellipse([px - 4, py - 4, px + 4, py + 4], outline=(255, 60, 60), width=2)
        board.paste(tile, (panel * size, 0))
    board.save(path)


# --- Côté poste ------------------------------------------------------------------------------------
def lighter_copy(source: Path, triangles: int | None, blender: str | None) -> bytes:
    """Le fichier reçu, réduit à `triangles` par `reduce_model.py` s'il le faut."""
    if triangles is None:
        return source.read_bytes()
    with tempfile.TemporaryDirectory(prefix="liaison-") as temporary:
        reduce_model.reduce_models(source, Path(temporary), triangles,
                                   reduce_model.find_blender(blender), True)
        return (Path(temporary) / source.name).read_bytes()


def smaller_image(image: tuple[bytes, str], side: int) -> tuple[bytes, str]:
    """La texture ramenée à `side` pixels de côté, réencodée en PNG : une copie d'essai."""
    from PIL import Image

    picture = Image.open(io.BytesIO(image[0])).convert("RGB")
    picture = picture.resize((side, side), Image.LANCZOS)
    output = io.BytesIO()
    picture.save(output, format="PNG", optimize=True)
    return output.getvalue(), "image/png"


def hanging_clearance(points: np.ndarray, weights: np.ndarray, joints: dict[str, np.ndarray],
                      arm: float) -> float:
    """L'écart, en longueurs de bras, qu'un poignet pendant garde avec l'épaule pour ne pas
    entrer dans le flanc ni la hanche : mesuré sur ce que le tronc et les cuisses ont de plus
    large entre la hanche et l'aisselle."""
    shoulder = joints["upperarm_l"]
    low = joints["thigh_l"][1] - 0.12
    high = shoulder[1] - 0.12
    arms = sum(weights[:, BONE_INDEX[f"{part}_{side}"]] for part in ("upperarm", "lowerarm",
                                                                     "hand") for side in "lr")
    trunk = points[(arms < 0.05) & (points[:, 1] > low) & (points[:, 1] < high)]
    widest = float(np.quantile(np.abs(trunk[:, 0]), 0.995)) if len(trunk) else abs(shoulder[0])
    return float(np.clip((widest + 0.07 - abs(shoulder[0])) / arm, 0.14, 0.55))


def humanoid_body(joints: dict[str, np.ndarray], points: np.ndarray,
                  weights: np.ndarray) -> Body:
    """Le corps humanoïde que les poses lisent : ses longueurs, et l'écart du poignet pendant."""
    arm = float(np.linalg.norm(joints["lowerarm_l"] - joints["upperarm_l"])
                + np.linalg.norm(joints["hand_l"] - joints["lowerarm_l"]))
    return Body(joints, hanging_clearance(points, weights, joints, arm))


HUMANOID = Silhouette(SILHOUETTE, BONES, ANIMATED, CLIPS, LIFT_ONLY, estimate_sheet,
                      rest_skeleton, compute_weights, humanoid_body, clip_poses, solve_pose,
                      lambda bone: not bone.startswith(FINGERS))


def rig(source: Path, output: Path, sheet_path: Path, estimate: bool, triangles: int | None,
        texture: int | None, blender: str | None, skeleton: Path | None,
        report: bool = True, retouch: Path | None = None,
        silhouette: Silhouette | None = None) -> dict:
    """Lie `source`, écrit `output`, et rend le relevé.

    La silhouette est celle demandée, sinon celle que nomme la fiche de liaison (`silhouette`),
    sinon l'humanoïde ; une fiche estimée l'écrit.
    """
    received = source.read_bytes()
    if silhouette is None and sheet_path.is_file() and not estimate:
        silhouette = silhouette_named(json.loads(sheet_path.read_text(encoding="utf-8")).get(
            "silhouette", SILHOUETTE))
    silhouette = silhouette or HUMANOID
    data = lighter_copy(source, triangles, blender)
    mesh = read_static_mesh(data)
    if triangles is not None:
        # La réduction repose la copie sur sa propre boîte, qui n'est plus tout à fait celle du
        # fichier reçu : elle est remise où la fiche de liaison l'attend.
        original = read_static_mesh(received)["positions"]
        copy = mesh["positions"]
        mesh["positions"] = copy + np.array([
            (original[:, 0].min() + original[:, 0].max() - copy[:, 0].min() - copy[:, 0].max()) / 2,
            original[:, 1].min() - copy[:, 1].min(),
            (original[:, 2].min() + original[:, 2].max() - copy[:, 2].min() - copy[:, 2].max()) / 2])
    if estimate or not sheet_path.is_file():
        # L'estimation lit le fichier reçu, pas sa copie allégée : une fiche vaut pour les deux.
        sheet = silhouette.estimate_sheet(read_static_mesh(received)["positions"])
        sheet["source"] = source.name
        if sheet_path.is_file():
            previous = json.loads(sheet_path.read_text(encoding="utf-8"))
            for kept in ("height", "rigid", "arm_radius", "head_radius"):
                if kept in previous:
                    sheet[kept] = previous[kept]
        sheet_path.parent.mkdir(parents=True, exist_ok=True)
        sheet_path.write_text(json.dumps(sheet, indent=2, ensure_ascii=False) + "\n",
                              encoding="utf-8")
    sheet = json.loads(sheet_path.read_text(encoding="utf-8"))
    if sheet.get("version") != SHEET_VERSION:
        raise SystemExit(f"fiche de liaison de version {sheet.get('version')}")
    if sheet.get("silhouette", SILHOUETTE) != silhouette.name:
        raise SystemExit(f"la fiche de liaison est de silhouette "
                         f"{sheet.get('silhouette', SILHOUETTE)}, pas {silhouette.name}")

    joints, scale, origin = silhouette.rest_skeleton(sheet)
    points = model_points(mesh["positions"], sheet, scale, origin)
    joints_of, weights = top_influences(
        silhouette.compute_weights(points, joints, sheet, scale), silhouette)
    full = np.zeros((len(points), len(silhouette.bones)))
    np.add.at(full, (np.arange(len(points))[:, None], joints_of), weights)
    body = silhouette.make_body(joints, points, full)
    retouched = load_retouch(retouch, silhouette) if retouch is not None else None
    baked, measured = bake_clips(body, points, joints_of, weights, retouched, silhouette)

    image = mesh["image"]
    if image is not None and texture:
        image = smaller_image(image, texture)
    name = output.stem
    written = build_glb(mesh, points, joints_of, weights, body, baked, image, name, silhouette)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(written)
    if report:
        liaison_board(points, joints_of, weights, joints, output.with_name("liaison.png"),
                      silhouette)
    if skeleton is not None:
        skeleton.parent.mkdir(parents=True, exist_ok=True)
        skeleton.write_text(json.dumps(silhouette.document(), indent=2) + "\n",
                            encoding="utf-8")

    statement = {
        "model": output.name,
        "silhouette": silhouette.name,
        "sha256": hashlib.sha256(written).hexdigest(),
        "bytes": len(written),
        "source": {"file": source.name, "sha256": hashlib.sha256(received).hexdigest()},
        "sheet": {"file": sheet_path.name, "estimated": bool(sheet.get("estimated")),
                  "sha256": hashlib.sha256(sheet_path.read_bytes()).hexdigest()},
        "triangles": int(len(mesh["indices"]) // 3),
        "vertices": int(len(points)),
        "bones": len(silhouette.bones),
        "scale": round(scale, 5),
        "height": round(float(scale * (sheet["head_top"] - sheet["ground"])), 4),
        "size": [round(float(v), 3) for v in np.ptp(points, axis=0)],
        "texture": ({"mime": image[1], "bytes": len(image[0]), "kept": not texture}
                    if image is not None else None),
        "clips": [dict(clip, **measured[clip["name"]]) for clip in silhouette.clips],
    }
    statement.update(body.measures())
    if retouch is not None:
        statement["retouch"] = {"file": retouch.name,
                                "sha256": hashlib.sha256(retouch.read_bytes()).hexdigest()}
    if report:
        output.with_name("releve.json").write_text(
            json.dumps(statement, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return statement


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("source", type=Path, help="le .glb reçu, déjà passé par reduce_model.py")
    parser.add_argument("output", type=Path, help="le .glb lié à écrire")
    parser.add_argument("--sheet", type=Path,
                        help="la fiche de liaison (défaut : liaison.json à côté de la sortie)")
    parser.add_argument("--estimate", action="store_true",
                        help="réestimer les articulations et réécrire la fiche")
    parser.add_argument("--silhouette", choices=("humanoid", "quadruped"),
                        help="la silhouette à lier (défaut : celle de la fiche, sinon humanoid)")
    parser.add_argument("--skeleton", type=Path, help="écrire aussi skeleton.json ici")
    parser.add_argument("--triangles", type=int, help="réduire d'abord le maillage (Blender)")
    parser.add_argument("--texture", type=int, help="ramener la texture à ce côté, en pixels")
    parser.add_argument("--blender", help="blender.exe, pour --triangles")
    parser.add_argument("--retouch", type=Path,
                        help="la fiche de retouche : les clips réglés dans Blender")
    parser.add_argument("--no-report", action="store_true",
                        help="n'écrire ni releve.json ni liaison.png à côté de la sortie")
    arguments = parser.parse_args()
    if not arguments.source.is_file():
        raise SystemExit(f"source introuvable : {arguments.source}")
    sheet = arguments.sheet or arguments.output.with_name("liaison.json")
    silhouette = silhouette_named(arguments.silhouette) if arguments.silhouette else None
    statement = rig(arguments.source, arguments.output, sheet, arguments.estimate,
                    arguments.triangles, arguments.texture, arguments.blender, arguments.skeleton,
                    not arguments.no_report, arguments.retouch, silhouette)
    print(f"{statement['model']} ({statement['silhouette']}) : {statement['triangles']} triangles, "
          f"{statement['bytes'] / 1048576:.1f} Mio, taille {statement['height']} m, "
          f"fiche {'estimée' if statement['sheet']['estimated'] else 'écrite'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
