#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0
"""La silhouette `quadruped` : le squelette du lion et du loup, sa liaison et ses cinq clips.

Ce module ne se lance pas seul : `rig_character.py --silhouette quadruped` l'appelle, et lui
emprunte tout ce qui est commun aux silhouettes (lecture du maillage, résolution à deux os,
cuisson des clips, écriture du `.glb`). Ce qui est propre au quadrupède
(`Planning/standards/personnages-3d.md`, §5 à §7, LOT-1011) :

- **le squelette** : 29 os — un tronc de huit (`Root`, `pelvis`, trois `spine`, deux `neck`,
  `head`), une queue de trois, et par côté une patte avant de cinq (`clavicle` pour l'omoplate,
  `upperarm`, `lowerarm`, `hand` pour le canon, `forepaw`) et une patte arrière de quatre
  (`thigh`, `calf`, `foot` pour le canon, `hindpaw`). Les noms des pattes sont ceux de
  l'humanoïde quand l'os y correspond : l'atelier et la retouche dans Blender s'y retrouvent ;
- **la fiche de liaison** parle dans le repère du fichier reçu **remis dans l'axe** : un fauve
  généré depuis une image de trois quarts arrive **en diagonale** (le lion de la démo à −48°, le
  loup à −59°) ; `heading` est ce cap, mesuré par l'axe principal de l'empreinte au sol, et
  `center_x`, `center_z` le point autour duquel on tourne. Les articulations sont estimées
  d'après les quatre appuis au sol, le ventre, le dos, et la courbe de chaque patte ;
- **les poids** : chaque patte est un cylindre autour de sa chaîne d'os, la queue aussi ; le
  reste suit le tronc, par rampes le long du corps ;
- **les clips** : `idle`, `walk`, `attack`, `hit`, `death` — pas de `cast`, un fauve ne lance pas
  de sort. La marche est un **trot** : les appuis vont par paires diagonales, chacun posé la
  moitié du cycle, et recule à 3 m/s (une case de 1,5 m en 0,5 s) ; l'attaque est un bond, les
  antérieurs levés, la gueule portée en avant à l'image clé.
"""

from __future__ import annotations

import math

import numpy as np

import rig_character as rc

SILHOUETTE = "quadruped"
SIDES = rc.SIDES
SPINE = ("pelvis", "spine_01", "spine_02", "spine_03", "neck_01", "neck_02", "head")
TAIL = ("tail_01", "tail_02", "tail_03")
FORE = ("upperarm", "lowerarm", "hand", "forepaw")
HIND = ("thigh", "calf", "foot", "hindpaw")
GROUND_CONTACT = 0.03      # m : un point à moins de cela du sol est un appui
CLIPS = [
    {"name": "idle", "duration": 1.0, "loop": True},
    {"name": "walk", "duration": 0.5, "loop": True},
    {"name": "attack", "duration": 0.9, "loop": False, "key": 0.4},
    {"name": "hit", "duration": 0.6, "loop": False},
    {"name": "death", "duration": 1.2, "loop": False},
]
LIFT_ONLY = {"walk"}
STANCE_SHARE = 0.46        # le trot : chaque appui est posé moins de la moitié du cycle


def _bones() -> list[tuple[str, str]]:
    bones = [("Root", ""), ("pelvis", "Root"), ("spine_01", "pelvis"), ("spine_02", "spine_01"),
             ("spine_03", "spine_02"), ("neck_01", "spine_03"), ("neck_02", "neck_01"),
             ("head", "neck_02"), ("tail_01", "pelvis"), ("tail_02", "tail_01"),
             ("tail_03", "tail_02")]
    for side, _ in SIDES:
        bones += [(f"clavicle_{side}", "spine_03"), (f"upperarm_{side}", f"clavicle_{side}"),
                  (f"lowerarm_{side}", f"upperarm_{side}"), (f"hand_{side}", f"lowerarm_{side}"),
                  (f"forepaw_{side}", f"hand_{side}"),
                  (f"thigh_{side}", "pelvis"), (f"calf_{side}", f"thigh_{side}"),
                  (f"foot_{side}", f"calf_{side}"), (f"hindpaw_{side}", f"foot_{side}")]
    return bones


BONES = _bones()
BONE_INDEX = {name: index for index, (name, _) in enumerate(BONES)}
PARENT = {name: parent for name, parent in BONES}
assert len(BONES) == 29
ANIMATED = [name for name, _ in BONES if name != "Root"]


# --- L'estimation des articulations ----------------------------------------------------------------
def _contact_zones(points: np.ndarray) -> list[tuple[float, float]]:
    """Les plages de Z où le maillage touche le sol : les pattes arrière, les pattes avant."""
    contact = points[points[:, 1] < GROUND_CONTACT]
    if len(contact) < 10:
        raise SystemExit("estimation : le maillage ne touche pas le sol")
    order = np.sort(contact[:, 2])
    zones, start = [], order[0]
    for previous, current in zip(order[:-1], order[1:]):
        if current - previous > 0.08:
            zones.append((float(start), float(previous)))
            start = current
    zones.append((float(start), float(order[-1])))
    zones = sorted(zones, key=lambda zone: zone[1] - zone[0], reverse=True)[:2]
    if len(zones) < 2:
        raise SystemExit("estimation : une seule zone d'appui, il en faut deux (arrière, avant)")
    return sorted(zones)


def _paws(points: np.ndarray, zone: tuple[float, float]) -> dict[str, np.ndarray]:
    """Les deux appuis d'une zone, gauche (+X) et droite, au centre de leurs points de contact."""
    contact = points[(points[:, 1] < GROUND_CONTACT) & (points[:, 2] >= zone[0] - 1e-6)
                     & (points[:, 2] <= zone[1] + 1e-6)]
    xs = np.sort(contact[:, 0])
    gaps = np.diff(xs)
    cut = xs[int(np.argmax(gaps))] + gaps.max() / 2 if gaps.max() > 0.04 else float(np.median(xs))
    left, right = contact[contact[:, 0] > cut], contact[contact[:, 0] <= cut]
    if len(left) == 0 or len(right) == 0:
        left = right = contact
    return {"l": np.array([left[:, 0].mean(), 0.0, left[:, 2].mean()]),
            "r": np.array([right[:, 0].mean(), 0.0, right[:, 2].mean()]),
            "width": float(min(np.ptp(left[:, 0]), np.ptp(right[:, 0])))}


def _leg_curve(points: np.ndarray, paw: np.ndarray, radius: float, top: float,
               step: float = 0.02) -> list[tuple[float, float, float]]:
    """La patte, tranche par tranche depuis le sol : (hauteur, x, z) du centre de chaque tranche,
    en suivant la tranche précédente pour ne pas sauter dans le corps."""
    column = points[np.abs(points[:, 0] - paw[0]) < radius + 0.1]
    curve, x, z = [], float(paw[0]), float(paw[2])
    for y in np.arange(0.0, top, step):
        band = column[(column[:, 1] >= y) & (column[:, 1] < y + step)]
        band = band[(np.abs(band[:, 0] - x) < radius) & (np.abs(band[:, 2] - z) < radius)]
        if len(band) == 0:
            continue    # une tranche vide (maillage creux) : la suivante se cherche au meme endroit
        x = float((band[:, 0].min() + band[:, 0].max()) / 2)
        z = float((band[:, 2].min() + band[:, 2].max()) / 2)
        curve.append((float(y) + step / 2, x, z))
    return curve


def _at(curve: list[tuple[float, float, float]], y: float) -> tuple[float, float]:
    """Le centre de la tranche la plus proche de la hauteur `y`."""
    row = min(curve, key=lambda item: abs(item[0] - y))
    return row[1], row[2]


def _extreme(curve: list[tuple[float, float, float]], low: float, high: float,
             backward: bool) -> tuple[float, float, float]:
    """La tranche la plus en arrière (ou en avant) entre deux hauteurs."""
    rows = [row for row in curve if low <= row[0] <= high] or curve
    return min(rows, key=lambda row: row[2]) if backward else max(rows, key=lambda row: row[2])


def _paw_joint(paw: np.ndarray, toes: np.ndarray) -> list[float]:
    """L'articulation de l'appui : à l'arrière du coussinet, un peu au-dessus du sol — tout le
    contact suit l'os de l'appui, le canon pivote derrière lui."""
    rear = float(np.quantile(toes[:, 2], 0.15)) if len(toes) else float(paw[2])
    return [float(paw[0]), 0.02, rear]


def _ensure_bend(joints: dict, upper: str, middle: str, lower: str, forward: bool,
                 share: float = 0.10) -> None:
    """Le pli d'une patte : l'articulation du milieu est écartée d'au moins `share` de la
    longueur de la chaîne, vers l'avant (grasset) ou l'arrière (coude). Une patte estimée trop
    droite ne se résout pas, et ne plie pas dans le bon sens."""
    a, b, c = (np.array(joints[name], dtype=np.float64) for name in (upper, middle, lower))
    direction = rc.unit_vector(c - a)
    chain = float(np.linalg.norm(b - a) + np.linalg.norm(c - b))
    offset = (b - a) - np.dot(b - a, direction) * direction
    outward = np.array([0.0, 0.0, 1.0 if forward else -1.0])
    outward = rc.unit_vector(outward - np.dot(outward, direction) * direction)
    along = float(np.dot(offset, outward))
    if along < share * chain:
        foot = a + np.dot(b - a, direction) * direction
        joints[middle] = [round(float(v), 4) for v in foot + outward * share * chain]


def _axis_of(points: np.ndarray) -> np.ndarray:
    """L'axe principal de l'empreinte, dans le plan du sol : le sens du corps."""
    flat = points[:, [0, 2]] - points[:, [0, 2]].mean(axis=0)
    values, vectors = np.linalg.eigh(flat.T @ flat / len(flat))
    return vectors[:, int(np.argmax(values))]


def estimate_sheet(positions: np.ndarray) -> dict:
    """Une fiche de liaison estimée d'après la géométrie d'un fauve sur ses quatre pattes.

    Repère du fichier reçu : hauteur +Y. Le corps est remis dans l'axe +Z (la tête devant) par
    l'axe principal de l'empreinte, puis par la ligne des appuis : le cap trouvé est `heading`.
    Une estimation n'est pas une mesure : la fiche se relit sur la planche `liaison.png` et se
    corrige, à la main ou par Blender (D-44).
    """
    P = positions.astype(np.float64)
    ground = float(P[:, 1].min())
    Q = P - np.array([0.0, ground, 0.0])
    axis = _axis_of(Q)
    along = Q[:, [0, 2]] @ axis
    # La tête est au bout le plus haut du corps : c'est lui qui donne le sens de l'axe.
    if Q[along < np.quantile(along, 0.2)][:, 1].max() > Q[along > np.quantile(along, 0.8)][:, 1].max():
        axis = -axis
    heading = math.degrees(math.atan2(axis[0], axis[1]))
    center = np.zeros(3)
    # Deux passes : la seconde recentre sur les appuis et aligne le cap sur leur ligne.
    for _ in range(2):
        aligned = (Q - center) @ rc.rot_y(-math.radians(heading)).T
        hind_zone, fore_zone = _contact_zones(aligned)
        hind, fore = _paws(aligned, hind_zone), _paws(aligned, fore_zone)
        hind_mid = (hind["l"] + hind["r"]) / 2
        fore_mid = (fore["l"] + fore["r"]) / 2
        turn = math.degrees(math.atan2(fore_mid[0] - hind_mid[0], fore_mid[2] - hind_mid[2]))
        shift = (hind_mid + fore_mid) / 2
        center = center + rc.rot_y(math.radians(heading)) @ np.array([shift[0], 0.0, shift[2]])
        heading += turn
    aligned = (Q - center) @ rc.rot_y(-math.radians(heading)).T
    hind_zone, fore_zone = _contact_zones(aligned)
    hind, fore = _paws(aligned, hind_zone), _paws(aligned, fore_zone)
    length = float(((fore["l"] + fore["r"]) / 2 - (hind["l"] + hind["r"]) / 2)[2])

    # Le ventre et le dos : entre les deux zones d'appui, hors pattes.
    trunk = aligned[(aligned[:, 2] > hind_zone[1] + 0.15 * length)
                    & (aligned[:, 2] < fore_zone[0] - 0.15 * length)]
    if len(trunk) == 0:
        trunk = aligned[(aligned[:, 2] > hind_zone[1]) & (aligned[:, 2] < fore_zone[0])]
    belly = float(trunk[:, 1].min())
    back = float(np.quantile(trunk[:, 1], 0.995))
    half_width = float(np.quantile(np.abs(trunk[:, 0]), 0.98))
    hip_y = belly + 0.55 * (back - belly)
    shoulder_y = belly + 0.42 * (back - belly)

    joints: dict[str, list[float]] = {}
    tips: dict[str, list[float]] = {}
    # Chaque patte est suivie de son côté ; les hauteurs des articulations sont mises en commun
    # entre la gauche et la droite — c'est le même animal, pris à mi-pas — et chaque côté garde
    # sa position dans la longueur.
    curves, bends = {}, {}
    for side, _ in SIDES:
        paw = hind[side]
        radius = 0.6 * hind["width"] + 0.06
        curve = _leg_curve(aligned, paw, radius, hip_y + 0.05)
        if len(curve) < 4:
            raise SystemExit(f"estimation : la patte arrière {side} ne se suit pas depuis le sol")
        hock = _extreme(curve, 0.12 * hip_y, 0.5 * hip_y, backward=True)
        stifle = _extreme(curve, hock[0] + 0.05, 0.7 * hip_y, backward=False)
        curves[f"hind_{side}"] = curve
        bends[f"hind_{side}"] = (hock[0], stifle[0])
        paw = fore[side]
        radius = 0.6 * fore["width"] + 0.06
        curve = _leg_curve(aligned, paw, radius, shoulder_y + 0.05)
        if len(curve) < 4:
            raise SystemExit(f"estimation : la patte avant {side} ne se suit pas depuis le sol")
        elbow = _extreme(curve, 0.3 * shoulder_y, 0.7 * shoulder_y, backward=True)
        carpus = _extreme(curve, 0.1 * shoulder_y, elbow[0] - 0.05, backward=False)
        curves[f"fore_{side}"] = curve
        bends[f"fore_{side}"] = (carpus[0], elbow[0])
    hock_y = (bends["hind_l"][0] + bends["hind_r"][0]) / 2
    stifle_y = (bends["hind_l"][1] + bends["hind_r"][1]) / 2
    carpus_y = (bends["fore_l"][0] + bends["fore_r"][0]) / 2
    elbow_y = (bends["fore_l"][1] + bends["fore_r"][1]) / 2
    for side, sign in SIDES:
        paw, curve = hind[side], curves[f"hind_{side}"]
        radius = 0.6 * hind["width"] + 0.06
        for name, y in ((f"thigh_{side}", hip_y), (f"calf_{side}", stifle_y),
                        (f"foot_{side}", hock_y)):
            x, z = _at(curve, y)
            joints[name] = [x, y, z]
        toes = aligned[(aligned[:, 1] < GROUND_CONTACT) & (np.abs(aligned[:, 0] - paw[0]) < radius)
                       & (aligned[:, 2] >= hind_zone[0] - 1e-6) & (aligned[:, 2] <= hind_zone[1])]
        joints[f"hindpaw_{side}"] = _paw_joint(paw, toes)
        tips[f"hindpaw_{side}"] = [float(paw[0]), 0.0, float(toes[:, 2].max())]
        paw, curve = fore[side], curves[f"fore_{side}"]
        radius = 0.6 * fore["width"] + 0.06
        for name, y in ((f"upperarm_{side}", shoulder_y), (f"lowerarm_{side}", elbow_y),
                        (f"hand_{side}", carpus_y)):
            x, z = _at(curve, y)
            joints[name] = [x, y, z]
        toes = aligned[(aligned[:, 1] < GROUND_CONTACT) & (np.abs(aligned[:, 0] - paw[0]) < radius)
                       & (aligned[:, 2] >= fore_zone[0] - 1e-6) & (aligned[:, 2] <= fore_zone[1])]
        joints[f"forepaw_{side}"] = _paw_joint(paw, toes)
        tips[f"forepaw_{side}"] = [float(paw[0]), 0.0, float(toes[:, 2].max())]
        # L'omoplate : au-dessus de l'épaule, contre le thorax.
        shoulder_z = joints[f"upperarm_{side}"][2]
        joints[f"clavicle_{side}"] = [sign * 0.5 * half_width,
                                      shoulder_y + 0.5 * (back - shoulder_y), shoulder_z - 0.05]

        _ensure_bend(joints, f"thigh_{side}", f"calf_{side}", f"foot_{side}", forward=True)
        _ensure_bend(joints, f"upperarm_{side}", f"lowerarm_{side}", f"hand_{side}", forward=False)

    hind_mid = (hind["l"] + hind["r"]) / 2
    fore_mid = (fore["l"] + fore["r"]) / 2
    pelvis_y = hip_y + 0.45 * (back - hip_y)
    pelvis = np.array([0.0, pelvis_y, hind_mid[2] + 0.05 * length])
    chest = np.array([0.0, shoulder_y + 0.6 * (back - shoulder_y), fore_mid[2] - 0.12 * length])
    joints["pelvis"] = list(pelvis)
    for name, share in (("spine_01", 1 / 3), ("spine_02", 2 / 3)):
        joints[name] = list(pelvis + share * (chest - pelvis))
    joints["spine_03"] = list(chest)

    # La tête : ce qui est devant les pattes avant ; le museau son point le plus avancé.
    front = aligned[aligned[:, 2] > fore_zone[1] + 0.02]
    if len(front) < 10:
        front = aligned[aligned[:, 2] > fore_mid[2]]
    muzzle_rows = front[front[:, 2] >= np.quantile(front[:, 2], 0.97)]
    muzzle = np.array([0.0, float(muzzle_rows[:, 1].mean()), float(front[:, 2].max())])
    # L'encolure part du garrot, au-dessus des épaules ; la tête s'articule à mi-chemin du
    # museau : une crinière ou un cou épais ne déplacent pas l'articulation.
    neck_base = np.array([0.0, back - 0.05 * (back - belly), fore_mid[2] + 0.04 * length])
    head_joint = muzzle + 0.5 * (neck_base - muzzle)
    head_joint[1] = max(head_joint[1], neck_base[1] - 0.15 * (back - belly))
    neck_02 = neck_base + 0.5 * (head_joint - neck_base)
    joints["neck_01"] = list(neck_base)
    joints["neck_02"] = list(neck_02)
    joints["head"] = list(head_joint)
    tips["head"] = list(muzzle)
    head_top = float(aligned[:, 1].max())

    # La queue : ce qui est derrière les pattes arrière, suivi par tranches depuis la croupe.
    tail_region = aligned[aligned[:, 2] < hind_zone[0] - 0.02]
    tail_base = np.array([0.0, pelvis_y - 0.02, hind_zone[0] - 0.02])
    tail_points = [tail_base]
    if len(tail_region) > 20:
        far = tail_region[np.argmin(tail_region[:, 2])]
        steps = 12
        for step in range(1, steps + 1):
            z = tail_base[2] + (far[2] - tail_base[2]) * step / steps
            band = tail_region[np.abs(tail_region[:, 2] - z) < abs(far[2] - tail_base[2]) / steps]
            if len(band) == 0:
                continue
            previous = tail_points[-1]
            near = band[np.hypot(band[:, 0] - previous[0], band[:, 1] - previous[1]) < 0.25]
            if len(near) == 0:
                near = band
            tail_points.append(np.array([float(near[:, 0].mean()), float(near[:, 1].mean()), z]))
    tail_points = np.array(tail_points)
    lengths = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(tail_points, axis=0),
                                                              axis=1))])
    total = float(lengths[-1]) if lengths[-1] > 0.05 else 0.3

    def along_tail(share: float) -> list[float]:
        if lengths[-1] <= 0.05:
            return list(tail_base + np.array([0.0, -0.3 * share, -0.6 * share]) * total)
        return [float(np.interp(share * total, lengths, tail_points[:, axis]))
                for axis in range(3)]

    joints["tail_01"] = along_tail(0.0)
    joints["tail_02"] = along_tail(1 / 3)
    joints["tail_03"] = along_tail(2 / 3)
    tips["tail"] = along_tail(1.0)

    def rounded(value):
        if isinstance(value, dict):
            return {key: rounded(item) for key, item in value.items()}
        if isinstance(value, (list, tuple, np.ndarray)):
            return [rounded(item) for item in value]
        return round(float(value), 4)

    return {"version": rc.SHEET_VERSION, "silhouette": SILHOUETTE, "estimated": True,
            "height": None, "ground": round(ground, 4), "head_top": round(head_top + ground, 4),
            "heading": round(heading, 2), "center_x": round(float(center[0]), 4),
            "center_z": round(float(center[2]), 4), "joints": rounded(joints),
            "tips": rounded(tips), "leg_radius": round(0.6 * hind["width"] + 0.05, 4),
            "tail_radius": 0.08, "rigid": []}


# --- Le squelette de repos, à l'échelle du personnage ----------------------------------------------
def rest_skeleton(sheet: dict) -> tuple[dict[str, np.ndarray], float, np.ndarray]:
    """Les articulations dans le repère du modèle écrit, son échelle et son origine.

    La fiche parle dans le repère reçu **remis dans l'axe** (sol à zéro, corps vers +Z) ; le
    modèle écrit y ajoute l'échelle de `height` s'il est donné.
    """
    stature = sheet["head_top"] - sheet["ground"]
    scale = (sheet["height"] / stature) if sheet.get("height") else 1.0
    origin = np.array([sheet["center_x"], sheet["ground"], sheet.get("center_z", 0.0)])
    joints = {"Root": np.zeros(3)}
    for name, point in sheet["joints"].items():
        joints[name] = np.array(point, dtype=np.float64) * scale
    missing = [name for name, _ in BONES if name not in joints]
    if missing:
        raise SystemExit("fiche de liaison incomplète : " + ", ".join(missing))
    return joints, scale, origin


def tips_of(sheet: dict, scale: float) -> dict[str, np.ndarray]:
    return {name: np.array(point, dtype=np.float64) * scale
            for name, point in sheet.get("tips", {}).items()}


# --- Les poids -------------------------------------------------------------------------------------
def _polyline_distance(points: np.ndarray, nodes: list[np.ndarray]) -> tuple[np.ndarray, np.ndarray]:
    """La distance de chaque point à une ligne brisée, et l'abscisse curviligne du plus proche."""
    best = np.full(len(points), np.inf)
    along = np.zeros(len(points))
    start = 0.0
    for a, b in zip(nodes[:-1], nodes[1:]):
        segment = b - a
        length = float(np.linalg.norm(segment))
        if length < 1e-9:
            continue
        t = np.clip(((points - a) @ segment) / (length * length), 0.0, 1.0)
        distance = np.linalg.norm(points - (a + t[:, None] * segment), axis=1)
        closer = distance < best
        best[closer] = distance[closer]
        along[closer] = start + t[closer] * length
        start += length
    return best, along


def compute_weights(points: np.ndarray, joints: dict[str, np.ndarray], sheet: dict,
                    scale: float) -> np.ndarray:
    """Le poids de chaque os sur chaque sommet (N × 29), avant la coupe à quatre influences."""
    count = len(points)
    weights = np.zeros((count, len(BONES)))
    x, y, z = points[:, 0], points[:, 1], points[:, 2]
    unit = scale * (sheet["head_top"] - sheet["ground"]) / 1.4
    tips = tips_of(sheet, scale)
    smooth = rc._smooth

    # Les pattes : un cylindre autour de la chaîne, plus large en haut, qui s'efface dans le corps.
    limb_mask = np.zeros(count)
    for side, sign in SIDES:
        for chain, tip in ((HIND, f"hindpaw_{side}"), (FORE, f"forepaw_{side}")):
            names = [f"{bone}_{side}" for bone in chain]
            nodes = [joints[name] for name in names] + [tips[tip]]
            top = joints[names[0]]
            distance, along = _polyline_distance(points, nodes)
            radius_paw = sheet.get("leg_radius", 0.12) * scale
            radius_top = max(abs(top[0]) * 0.9, radius_paw * 1.6)
            height_share = np.clip(y / max(top[1], 1e-6), 0.0, 1.0)
            radius = radius_paw + (radius_top - radius_paw) * height_share ** 2
            mask = 1.0 - smooth(radius, radius + 0.05 * unit, distance)
            # Au-dessus de l'articulation haute, la patte s'efface dans le tronc ; et ce qui est
            # entre les pattes, au ventre, reste au tronc.
            mask *= 1.0 - smooth(top[1] - 0.02 * unit, top[1] + 0.12 * unit, y)
            lateral = smooth(0.25 * abs(top[0]), 0.6 * abs(top[0]), sign * x)
            low = 1.0 - smooth(0.3 * top[1], 0.5 * top[1], y)
            mask *= lateral + (1.0 - lateral) * low
            mask = np.clip(mask - limb_mask, 0.0, 1.0)
            limb_mask += mask
            knots = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(nodes, axis=0), axis=1))])
            shares = [np.ones(count)]
            for knot in knots[1:len(names)]:
                shares.append(smooth(knot - 0.04 * unit, knot + 0.04 * unit, along))
            shares.append(np.zeros(count))
            for rank, name in enumerate(names):
                weights[:, BONE_INDEX[name]] += mask * np.clip(shares[rank] - shares[rank + 1],
                                                                 0.0, 1.0)

    # La queue : derrière le bassin, autour de sa ligne.
    tail_nodes = [joints[name] for name in TAIL] + [tips["tail"]]
    distance, along = _polyline_distance(points, tail_nodes)
    # Le rayon grandit vers la pointe : la touffe d'un lion est plus large que sa queue.
    tail_length = max(float(np.sum(np.linalg.norm(np.diff(tail_nodes, axis=0), axis=1))), 1e-6)
    radius = sheet.get("tail_radius", 0.08) * scale * (1.0 + 1.5 * np.clip(along / tail_length,
                                                                             0.0, 1.0))
    behind = 1.0 - smooth(joints["pelvis"][2] - 0.08 * unit, joints["pelvis"][2], z)
    tail_mask = (1.0 - smooth(radius, radius + 0.06 * unit, distance)) * behind
    tail_mask = np.clip(tail_mask - limb_mask, 0.0, 1.0)
    knots = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(tail_nodes, axis=0), axis=1))])
    shares = [np.ones(count)]
    for knot in knots[1:len(TAIL)]:
        shares.append(smooth(knot - 0.03 * unit, knot + 0.03 * unit, along))
    shares.append(np.zeros(count))
    for rank, name in enumerate(TAIL):
        weights[:, BONE_INDEX[name]] += tail_mask * np.clip(shares[rank] - shares[rank + 1],
                                                            0.0, 1.0)

    # Le tronc : une chaîne le long de Z, du bassin à la tête, pour tout ce qui reste.
    body = np.clip(1.0 - limb_mask - tail_mask, 0.0, 1.0)
    ramps = [np.ones(count)]
    for name in SPINE[1:]:
        half = (0.04 if name.startswith("neck") or name == "head" else 0.06) * unit
        ramps.append(smooth(joints[name][2] - half, joints[name][2] + half, z))
    ramps.append(np.zeros(count))
    for rank, name in enumerate(SPINE):
        weights[:, BONE_INDEX[name]] += body * np.clip(ramps[rank] - ramps[rank + 1], 0.0, 1.0)

    # Ce qui suit rigidement un os (standard, §6 et §8) : un volume de la fiche, pesé à 1.
    for volume in sheet.get("rigid", []):
        low = np.array(volume["box"][0]) * scale
        high = np.array(volume["box"][1]) * scale
        inside = np.all((points >= low) & (points <= high), axis=1)
        weights[inside] = 0.0
        weights[inside, BONE_INDEX[volume["bone"]]] = 1.0
    return weights


# --- Le corps et les poses -------------------------------------------------------------------------
class Body:
    """Ce que les poses lisent d'un fauve : ses articulations de repos et ses longueurs."""

    def __init__(self, joints: dict[str, np.ndarray]):
        self.joints = joints
        self.offset = {name: joints[name] - joints[parent] if parent else joints[name]
                       for name, parent in BONES}
        J = joints
        # Les longueurs, par côté : une fiche estimée à mi-pas n'est pas symétrique, et chaque
        # patte doit retrouver exactement sa pose de liaison.
        self.thigh = {s: float(np.linalg.norm(J[f"calf_{s}"] - J[f"thigh_{s}"])) for s in "lr"}
        self.calf = {s: float(np.linalg.norm(J[f"foot_{s}"] - J[f"calf_{s}"])) for s in "lr"}
        self.hind = {s: self.thigh[s] + self.calf[s] for s in "lr"}
        self.foot = {s: float(np.linalg.norm(J[f"hindpaw_{s}"] - J[f"foot_{s}"])) for s in "lr"}
        self.upperarm = {s: float(np.linalg.norm(J[f"lowerarm_{s}"] - J[f"upperarm_{s}"]))
                         for s in "lr"}
        self.lowerarm = {s: float(np.linalg.norm(J[f"hand_{s}"] - J[f"lowerarm_{s}"]))
                         for s in "lr"}
        self.fore = {s: self.upperarm[s] + self.lowerarm[s] for s in "lr"}
        self.hand = {s: float(np.linalg.norm(J[f"forepaw_{s}"] - J[f"hand_{s}"])) for s in "lr"}
        # La « jambe » commune aux silhouettes : ce à quoi se rapportent les déplacements du
        # bassin, dans les poses et dans une fiche de retouche.
        self.leg = float(np.mean([self.hind[s] + self.foot[s] for s in "lr"]))
        self.arm = float(np.mean([self.fore[s] + self.hand[s] for s in "lr"]))
        self.hip_x = abs(float(J["thigh_l"][0]))
        self.length = float(J["spine_03"][2] - J["pelvis"][2])
        # L'appui neutre de chaque patte, au milieu de la foulée : le jarret (ou le carpe) passe
        # alors sous la hanche (ou l'épaule), et la foulée s'écarte autant devant que derrière.
        # L'avance est la moyenne des deux côtés : un maillage pris à mi-pas ne la décale pas.
        self.neutral = {}
        for paw, top, cannon in (("hindpaw", "thigh", "foot"), ("forepaw", "upperarm", "hand")):
            advance = float(np.mean([J[f"{paw}_{s}"][2] - J[f"{cannon}_{s}"][2] for s in "lr"]))
            for s in "lr":
                neutral = J[f"{paw}_{s}"].copy()
                neutral[2] = J[f"{top}_{s}"][2] + advance
                self.neutral[f"{paw}_{s}"] = neutral
        # Le pli de repos de chaque patte : vers où pointe le genou (ou le coude) quand la patte
        # est dans la pose du maillage. C'est le pôle par défaut, et l'indice du repère de repos :
        # au repos, la pose résolue est exactement la pose de liaison.
        self.rest_bend = {}
        for side, _ in SIDES:
            for upper, middle, lower in ((f"thigh_{side}", f"calf_{side}", f"foot_{side}"),
                                         (f"upperarm_{side}", f"lowerarm_{side}", f"hand_{side}")):
                direction = rc.unit_vector(J[lower] - J[upper])
                toward = J[middle] - J[upper]
                bend = toward - np.dot(toward, direction) * direction
                self.rest_bend[upper] = (rc.unit_vector(bend) if np.linalg.norm(bend) > 1e-6
                                         else np.array([0.0, 0.0, 1.0]))

    def measures(self) -> dict:
        return {"leg": round(self.leg, 4), "arm": round(self.arm, 4),
                "hind": round(float(np.mean(list(self.hind.values()))), 4),
                "fore": round(float(np.mean(list(self.fore.values()))), 4),
                "stance_share": round(stance_share(self), 4)}


def make_body(joints: dict[str, np.ndarray], points: np.ndarray, weights: np.ndarray) -> Body:
    return Body(joints)


def default_pose(body: Body) -> dict:
    """Debout sur ses quatre pattes, dans la pose de liaison : ce que chaque clip modifie."""
    pose = {
        "pelvis": np.zeros(3),            # déplacement du bassin, en longueurs de patte arrière
        "auto_height": np.array([1.0]),   # 1 : le bassin descend juste assez pour les arrières
        "auto_chest": np.array([1.0]),    # 1 : le thorax s'abaisse juste assez pour les avants
        "pelvis_turn": np.zeros(3),       # lacet, tangage, roulis, en degrés
        "chest_turn": np.zeros(3),        # le dos, réparti sur les trois os de l'échine
        "neck_turn": np.zeros(3),         # l'encolure, répartie sur les deux os du cou
        "head_turn": np.zeros(3),         # la tête par rapport au cou
        "tail_turn": np.zeros(3),         # la base de la queue
        "tail_curl": np.zeros(3),         # ajouté à chaque os suivant de la queue
        "leg_space": np.array([0.0]),     # 0 : les appuis visent le sol ; 1 : ils suivent le corps
    }
    for side, _ in SIDES:
        pose[f"hindpaw_{side}"] = np.zeros(3)     # l'appui, depuis le repos (écart, haut, avant)
        pose[f"forepaw_{side}"] = np.zeros(3)
        pose[f"foot_{side}"] = np.zeros(2)        # tangage et lacet du canon arrière, en degrés
        pose[f"hand_{side}"] = np.zeros(2)        # tangage et lacet du canon avant
        mirror = np.array([1.0 if side == "l" else -1.0, 1.0, 1.0])
        pose[f"knee_{side}"] = body.rest_bend[f"thigh_{side}"] * mirror
        pose[f"elbow_{side}"] = body.rest_bend[f"upperarm_{side}"] * mirror
    return pose


def _chain_rotations(base: np.ndarray, turn_matrix: np.ndarray, names: tuple[str, ...]) -> dict:
    """Une rotation répartie en parts égales le long d'une chaîne d'os."""
    return {name: base @ rc.partial(turn_matrix, (rank + 1) / len(names))
            for rank, name in enumerate(names)}


def solve_pose(body: Body, pose: dict) -> tuple[dict[str, np.ndarray], np.ndarray]:
    """Les rotations **monde** de chaque os et la position du bassin, pour une pose en cibles."""
    J = body.joints
    rotation = {name: np.eye(3) for name, _ in BONES}
    pelvis_rotation = rc.turn(*pose["pelvis_turn"])
    rotation["pelvis"] = pelvis_rotation
    follow = float(pose["leg_space"][0])
    carried = rc.partial(pelvis_rotation, follow)

    def spine_rotations(chest_turn: np.ndarray) -> None:
        rotation.update(_chain_rotations(pelvis_rotation, rc.turn(*chest_turn),
                                         ("spine_01", "spine_02", "spine_03")))
        rotation.update(_chain_rotations(rotation["spine_03"], rc.turn(*pose["neck_turn"]),
                                         ("neck_01", "neck_02")))
        rotation["head"] = rotation["neck_02"] @ rc.turn(*pose["head_turn"])

    spine_rotations(pose["chest_turn"])
    tail = rc.turn(*pose["tail_turn"])
    curl = rc.turn(*pose["tail_curl"])
    rotation["tail_01"] = pelvis_rotation @ tail
    rotation["tail_02"] = rotation["tail_01"] @ curl
    rotation["tail_03"] = rotation["tail_02"] @ curl

    cannon, heading = {}, {}
    for side, sign in SIDES:
        for bone, paw in ((f"foot_{side}", f"hindpaw_{side}"), (f"hand_{side}", f"forepaw_{side}")):
            pitch, yaw = pose[bone]
            heading[paw] = carried @ rc.rot_y(math.radians(sign * yaw))
            cannon[paw] = heading[paw] @ rc.rot_x(math.radians(pitch))

    def paw_target(paw: str, sign: float, pelvis_position: np.ndarray, unit: float) -> np.ndarray:
        offset = pose[paw] * unit * np.array([sign, 1.0, 1.0])
        world = J[paw] + offset
        with_body = pelvis_position + pelvis_rotation @ (J[paw] - J["pelvis"] + offset)
        return (1.0 - follow) * world + follow * with_body

    # Le bassin, puis sa descente : juste assez pour que les deux arrières atteignent le jarret.
    pelvis_position = J["pelvis"] + pose["pelvis"] * body.leg
    automatic = float(np.clip(pose["auto_height"][0], 0.0, 1.0))
    if automatic > 0.0:
        limit = math.inf
        for side, sign in SIDES:
            paw = f"hindpaw_{side}"
            hock = paw_target(paw, sign, pelvis_position, body.leg) + cannon[paw] @ (
                J[f"foot_{side}"] - J[paw])
            hip = pelvis_position + pelvis_rotation @ (J[f"thigh_{side}"] - J["pelvis"])
            flat = math.hypot(hock[0] - hip[0], hock[2] - hip[2])
            span = 0.992 * body.hind[side]
            rise = math.sqrt(max(span * span - flat * flat, 0.0))
            limit = min(limit, hock[1] + rise - (hip[1] - pelvis_position[1]))
        lowered = min(pelvis_position[1], limit)
        pelvis_position = pelvis_position.copy()
        pelvis_position[1] = (1.0 - automatic) * pelvis_position[1] + automatic * lowered

    for side, sign in SIDES:
        paw = f"hindpaw_{side}"
        hock = paw_target(paw, sign, pelvis_position, body.leg) + cannon[paw] @ (
            J[f"foot_{side}"] - J[paw])
        hip = pelvis_position + pelvis_rotation @ (J[f"thigh_{side}"] - J["pelvis"])
        pole = pelvis_rotation @ (pose[f"knee_{side}"] * np.array([sign, 1.0, 1.0]))
        stifle, reached, bend = rc.two_bones(hip, hock, body.thigh[side], body.calf[side], pole)
        rest = body.rest_bend[f"thigh_{side}"]
        rotation[f"thigh_{side}"] = rc.frame(stifle - hip, bend) @ rc.frame(
            body.offset[f"calf_{side}"], rest).T
        rotation[f"calf_{side}"] = rc.frame(reached - stifle, bend) @ rc.frame(
            body.offset[f"foot_{side}"], rest).T
        rotation[f"foot_{side}"] = cannon[paw]
        rotation[paw] = heading[paw] if pose[f"foot_{side}"][0] > 0.0 else cannon[paw]

    # Le thorax : il s'abaisse, par l'échine, juste assez pour que les avants atteignent le carpe.
    def spine_03_position() -> np.ndarray:
        position = pelvis_position
        for name in ("spine_01", "spine_02", "spine_03"):
            position = position + rotation[PARENT[name]] @ body.offset[name]
        return position

    def shoulder_of(side: str) -> np.ndarray:
        chest = rotation["spine_03"]
        return (spine_03_position() + chest @ body.offset[f"clavicle_{side}"]
                + chest @ body.offset[f"upperarm_{side}"])

    def carpus_of(side: str, sign: float) -> np.ndarray:
        paw = f"forepaw_{side}"
        return paw_target(paw, sign, pelvis_position, body.leg) + cannon[paw] @ (
            J[f"hand_{side}"] - J[paw])

    chest_turn = np.array(pose["chest_turn"], dtype=np.float64)
    automatic = float(np.clip(pose["auto_chest"][0], 0.0, 1.0))
    if automatic > 0.0:
        for _ in range(3):
            excess = 0.0
            for side, sign in SIDES:
                shoulder, carpus = shoulder_of(side), carpus_of(side, sign)
                flat = math.hypot(carpus[0] - shoulder[0], carpus[2] - shoulder[2])
                span = 0.992 * body.fore[side]
                rise = math.sqrt(max(span * span - flat * flat, 0.0))
                excess = max(excess, shoulder[1] - (carpus[1] + rise))
            if excess <= 1e-5:
                break
            reach = float(np.linalg.norm(shoulder_of("l") - pelvis_position))
            chest_turn = chest_turn + np.array(
                [0.0, automatic * math.degrees(math.asin(min(excess / max(reach, 1e-6), 1.0))), 0.0])
            spine_rotations(chest_turn)

    chest = rotation["spine_03"]
    for side, sign in SIDES:
        paw = f"forepaw_{side}"
        rotation[f"clavicle_{side}"] = chest
        shoulder, carpus = shoulder_of(side), carpus_of(side, sign)
        pole = chest @ (pose[f"elbow_{side}"] * np.array([sign, 1.0, 1.0]))
        elbow, reached, bend = rc.two_bones(shoulder, carpus, body.upperarm[side],
                                            body.lowerarm[side], pole)
        rest = body.rest_bend[f"upperarm_{side}"]
        rotation[f"upperarm_{side}"] = rc.frame(elbow - shoulder, bend) @ rc.frame(
            body.offset[f"lowerarm_{side}"], rest).T
        rotation[f"lowerarm_{side}"] = rc.frame(reached - elbow, bend) @ rc.frame(
            body.offset[f"hand_{side}"], rest).T
        rotation[f"hand_{side}"] = cannon[paw]
        rotation[paw] = heading[paw] if pose[f"hand_{side}"][0] > 0.0 else cannon[paw]
    return rotation, pelvis_position


# --- Les clips, en cibles --------------------------------------------------------------------------
def idle_pose(body: Body, phase: float) -> dict:
    """Le repos : une respiration, la tête qui veille, la queue qui bat lentement."""
    pose = default_pose(body)
    breath = math.sin(2.0 * math.pi * phase)
    slow = math.cos(2.0 * math.pi * phase)
    pose["chest_turn"] = np.array([0.0, 0.8 * breath, 0.0])
    pose["neck_turn"] = np.array([3.0 * slow, -1.5 * breath, 0.0])
    pose["head_turn"] = np.array([4.0 * slow, 1.0 * breath, 0.0])
    pose["pelvis"] = np.array([0.0, -0.004 * (1.0 + breath), 0.0])
    pose["tail_turn"] = np.array([8.0 * slow, 2.0 * breath, 0.0])
    pose["tail_curl"] = np.array([6.0 * slow, 2.0 * breath, 0.0])
    return pose


def stance_share(body: Body) -> float:
    """La part du cycle où un appui est posé : une patte courte la raccourcit (le cycle couvre
    toujours une case), et la foulée au sol reste dans ce que la hanche et le jarret couvrent."""
    stride = rc.WALK_SPEED * CLIPS[1]["duration"]
    hind = float(np.mean(list(body.hind.values())))
    return float(np.clip(0.8 * hind / stride, 0.28, STANCE_SHARE))


def walk_pose(body: Body, phase: float) -> dict:
    """Le trot sur place : les appuis diagonaux ensemble, chacun posé moins de la moitié du
    cycle — entre deux diagonales, une courte suspension —, et l'appui posé recule à vitesse
    constante."""
    pose = default_pose(body)
    duration = CLIPS[1]["duration"]
    share = stance_share(body)
    # Les diagonales : l'avant gauche avec l'arrière droit, l'avant droit avec l'arrière gauche.
    offsets = {"forepaw_l": 0.0, "hindpaw_r": 0.0, "forepaw_r": 0.5, "hindpaw_l": 0.5}
    swing = math.cos(2.0 * math.pi * phase)    # 1 : la diagonale gauche-avant est posée au milieu
    for paw, offset in offsets.items():
        own = (phase + offset) % 1.0
        travel = rc.WALK_SPEED * share * duration / body.leg
        if own < share:
            along = own / share
            forward, lift = travel * (0.5 - along), 0.0
            pitch = 14.0 * float(rc._smooth(0.6, 1.0, np.array(along)))
        else:
            along = (own - share) / (1.0 - share)
            forward = travel * (-0.5 + (1.0 - math.cos(math.pi * along)) / 2.0)
            lift = 0.14 * math.sin(math.pi * along)
            pitch = float(rc._hermite([(0.0, np.array(14.0)), (0.3, np.array(32.0)),
                                       (0.8, np.array(-6.0)), (1.0, np.array(0.0))], along))
        base = (body.neutral[paw] - body.joints[paw]) / body.leg
        base[0] = 0.0
        pose[paw] = base + np.array([0.0, lift, forward])
        cannon = "foot" if paw.startswith("hind") else "hand"
        pose[f"{cannon}_{paw[-1]}"] = np.array([pitch, 0.0])
    bounce = math.cos(4.0 * math.pi * phase)
    pose["pelvis"] = np.array([0.0, -0.012 - 0.008 * bounce, 0.0])
    pose["pelvis_turn"] = np.array([3.0 * swing, 1.0, -2.0 * swing])
    pose["chest_turn"] = np.array([-5.0 * swing, -1.0 + 1.0 * bounce, 2.0 * swing])
    pose["neck_turn"] = np.array([2.0 * swing, 4.0 - 2.0 * bounce, 0.0])
    pose["head_turn"] = np.array([0.0, -2.0 + 1.5 * bounce, 0.0])
    pose["tail_turn"] = np.array([6.0 * swing, 10.0, 0.0])
    pose["tail_curl"] = np.array([4.0 * swing, 4.0 + 2.0 * bounce, 0.0])
    return pose


def attack_clip(body: Body):
    """L'attaque : le fauve se ramasse sur l'arrière, bondit les antérieurs levés, et porte la
    gueule en avant à l'image clé ; puis retombe."""
    rest = idle_pose(body, 0.0)
    return rc.keyed(body, rest, [
        (0.0, {}),
        (0.22, {"pelvis": [0.0, -0.10, -0.05], "pelvis_turn": [0.0, -4.0, 0.0],
                "chest_turn": [0.0, 10.0, 0.0], "neck_turn": [0.0, 6.0, 0.0],
                "head_turn": [0.0, 8.0, 0.0],
                "forepaw_l": [0.0, 0.0, -0.06], "forepaw_r": [0.0, 0.0, -0.06],
                "tail_turn": [0.0, 20.0, 0.0], "tail_curl": [0.0, 10.0, 0.0]}),
        (0.32, {"pelvis": [0.0, 0.02, 0.10], "pelvis_turn": [0.0, -26.0, 0.0],
                "auto_chest": [0.0], "chest_turn": [0.0, -14.0, 0.0],
                "neck_turn": [0.0, 10.0, 0.0], "head_turn": [0.0, 18.0, 0.0],
                "forepaw_l": [0.03, 0.42, 0.34], "forepaw_r": [0.03, 0.30, 0.26],
                "hand_l": [-30.0, 0.0], "hand_r": [-30.0, 0.0],
                "tail_turn": [0.0, 34.0, 0.0], "tail_curl": [0.0, 14.0, 0.0]}),
        (0.40, {"pelvis": [0.0, 0.0, 0.16], "pelvis_turn": [0.0, -22.0, 0.0],
                "auto_chest": [0.0], "chest_turn": [0.0, -4.0, 0.0],
                "neck_turn": [0.0, 20.0, 0.0], "head_turn": [0.0, 28.0, 0.0],
                "forepaw_l": [0.04, 0.26, 0.44], "forepaw_r": [0.04, 0.16, 0.36],
                "hand_l": [-20.0, 0.0], "hand_r": [-20.0, 0.0]}),
        (0.50, {"pelvis": [0.0, -0.02, 0.14], "pelvis_turn": [0.0, -14.0, 0.0],
                "auto_chest": [0.0], "chest_turn": [0.0, 0.0, 0.0],
                "neck_turn": [0.0, 18.0, 0.0], "head_turn": [0.0, 24.0, 0.0],
                "forepaw_l": [0.03, 0.08, 0.40], "forepaw_r": [0.03, 0.02, 0.32],
                "hand_l": [-8.0, 0.0], "hand_r": [-8.0, 0.0]}),
        (0.64, {"pelvis": [0.0, -0.04, 0.06], "pelvis_turn": [0.0, -2.0, 0.0],
                "auto_chest": [1.0], "chest_turn": [0.0, 4.0, 0.0],
                "neck_turn": [0.0, 6.0, 0.0], "head_turn": [0.0, 8.0, 0.0],
                "forepaw_l": [0.0, 0.0, 0.10], "forepaw_r": [0.0, 0.0, 0.06],
                "hand_l": [0.0, 0.0], "hand_r": [0.0, 0.0],
                "tail_turn": [0.0, 12.0, 0.0], "tail_curl": [0.0, 4.0, 0.0]}),
        (0.9, {name: value for name, value in rest.items()}),
    ])


def hit_clip(body: Body):
    """Le coup encaissé : le corps part en arrière et se tasse, la tête se détourne, puis il se
    reprend."""
    rest = idle_pose(body, 0.0)
    return rc.keyed(body, rest, [
        (0.0, {}),
        (0.10, {"pelvis": [0.0, -0.03, -0.04], "pelvis_turn": [0.0, 2.0, 3.0],
                "chest_turn": [8.0, 4.0, 4.0], "neck_turn": [20.0, 8.0, 0.0],
                "head_turn": [24.0, -12.0, 8.0],
                "forepaw_l": [0.02, 0.0, -0.04], "forepaw_r": [0.0, 0.0, 0.02],
                "tail_turn": [0.0, 16.0, 0.0], "tail_curl": [0.0, 10.0, 0.0]}),
        (0.24, {"pelvis": [0.0, -0.05, -0.08], "pelvis_turn": [0.0, 1.0, 4.0],
                "chest_turn": [12.0, 2.0, 6.0], "neck_turn": [26.0, 2.0, 0.0],
                "head_turn": [28.0, -6.0, 10.0],
                "forepaw_l": [0.03, 0.0, -0.10], "forepaw_r": [0.0, 0.0, 0.0],
                "hindpaw_r": [0.0, 0.0, -0.06],
                "tail_turn": [0.0, 24.0, 0.0], "tail_curl": [0.0, 14.0, 0.0]}),
        (0.40, {"pelvis": [0.0, -0.03, -0.05], "pelvis_turn": [0.0, 0.0, 1.0],
                "chest_turn": [4.0, 0.0, 2.0], "neck_turn": [8.0, 0.0, 0.0],
                "head_turn": [10.0, 0.0, 3.0],
                "forepaw_l": [0.01, 0.0, -0.04], "hindpaw_r": [0.0, 0.0, -0.02],
                "tail_turn": [0.0, 12.0, 0.0], "tail_curl": [0.0, 6.0, 0.0]}),
        (0.6, {name: value for name, value in rest.items()}),
    ])


def death_clip(body: Body):
    """La chute : les pattes cèdent, le corps s'affaisse puis bascule sur le flanc droit, les
    pattes repliées, et y reste."""
    rest = idle_pose(body, 0.0)
    pelvis_rest = body.joints["pelvis"][1] / body.leg
    # Couché sur le flanc droit : le bassin à la demi-largeur du corps ; les pattes du dessous
    # se replient vers le ventre pour ne pas passer sous le flanc.
    side_height = (1.3 * body.hip_x + 0.06) / body.leg
    lying = {"pelvis": [-0.06, side_height - pelvis_rest, -0.04],
             "auto_height": [0.0], "auto_chest": [0.0], "leg_space": [1.0],
             "pelvis_turn": [0.0, 0.0, -84.0], "chest_turn": [0.0, 4.0, 6.0],
             "neck_turn": [-10.0, 6.0, 0.0], "head_turn": [-14.0, 10.0, -8.0],
             "hindpaw_l": [-0.05, 0.50, 0.30], "hindpaw_r": [-0.16, 0.48, 0.26],
             "forepaw_l": [-0.05, 0.42, 0.14], "forepaw_r": [-0.16, 0.40, 0.08],
             "foot_l": [70.0, 0.0], "foot_r": [60.0, 0.0],
             "hand_l": [60.0, 0.0], "hand_r": [50.0, 0.0],
             # Pattes repliées : le genou pointe vers l'avant-bas, le coude vers l'arrière-bas,
             # pour que la résolution ne bascule pas quand le jarret passe devant la hanche.
             "knee_l": [0.0, -0.6, 0.8], "knee_r": [0.0, -0.6, 0.8],
             "elbow_l": [0.0, -0.6, -0.8], "elbow_r": [0.0, -0.6, -0.8],
             "tail_turn": [10.0, 6.0, 0.0], "tail_curl": [6.0, 2.0, 0.0]}
    return rc.keyed(body, rest, [
        (0.0, {}),
        (0.14, {"pelvis": [0.0, -0.04, -0.02], "chest_turn": [0.0, 6.0, 0.0],
                "neck_turn": [0.0, 14.0, 0.0], "head_turn": [0.0, 18.0, 0.0],
                "tail_turn": [0.0, 14.0, 0.0]}),
        (0.40, {"pelvis": [0.0, -0.34, -0.06], "auto_height": [0.0], "auto_chest": [0.0],
                "pelvis_turn": [0.0, -6.0, -14.0], "chest_turn": [0.0, 10.0, 4.0],
                "neck_turn": [-6.0, 16.0, 0.0], "head_turn": [-8.0, 20.0, 0.0],
                "hindpaw_l": [0.0, 0.0, 0.08], "hindpaw_r": [0.0, 0.0, 0.04],
                "forepaw_l": [0.0, 0.0, -0.06], "forepaw_r": [0.0, 0.0, -0.08],
                "foot_l": [10.0, 0.0], "foot_r": [6.0, 0.0],
                "tail_turn": [0.0, 20.0, 0.0], "tail_curl": [0.0, 8.0, 0.0]}),
        (0.72, {"pelvis": [-0.04, side_height - pelvis_rest + 0.06, -0.05],
                "auto_height": [0.0], "auto_chest": [0.0], "leg_space": [0.85],
                "pelvis_turn": [0.0, -2.0, -64.0], "chest_turn": [0.0, 6.0, 8.0],
                "neck_turn": [-8.0, 8.0, 0.0], "head_turn": [-12.0, 12.0, -6.0],
                "hindpaw_l": [-0.03, 0.36, 0.24], "hindpaw_r": [-0.10, 0.34, 0.20],
                "forepaw_l": [-0.03, 0.30, 0.10], "forepaw_r": [-0.10, 0.28, 0.04],
                "foot_l": [50.0, 0.0], "foot_r": [40.0, 0.0],
                "hand_l": [40.0, 0.0], "hand_r": [34.0, 0.0],
                "knee_l": [0.0, -0.6, 0.8], "knee_r": [0.0, -0.6, 0.8],
                "elbow_l": [0.0, -0.6, -0.8], "elbow_r": [0.0, -0.6, -0.8],
                "tail_turn": [6.0, 10.0, 0.0], "tail_curl": [4.0, 4.0, 0.0]}),
        (0.94, lying),
        (1.02, dict(lying, neck_turn=[-12.0, 4.0, 0.0], head_turn=[-16.0, 8.0, -8.0])),
        (1.12, lying),
        (1.2, lying),
    ])


def clip_poses(body: Body, clip: dict) -> list[tuple[float, dict]]:
    """Les poses d'un clip, à pas fixe : la première à 0, la dernière à sa durée."""
    count = round(clip["duration"] * rc.SAMPLES_PER_SECOND)
    times = [index / rc.SAMPLES_PER_SECOND for index in range(count + 1)]
    name = clip["name"]
    if name == "idle":
        return [(time, idle_pose(body, time / clip["duration"])) for time in times]
    if name == "walk":
        return [(time, walk_pose(body, time / clip["duration"])) for time in times]
    builder = {"attack": attack_clip, "hit": hit_clip, "death": death_clip}
    pose_at = builder[name](body)
    return [(time, pose_at(time)) for time in times]


QUADRUPED = rc.Silhouette(SILHOUETTE, BONES, ANIMATED, CLIPS, LIFT_ONLY, estimate_sheet,
                          rest_skeleton, compute_weights, make_body, clip_poses, solve_pose,
                          lambda bone: True)
