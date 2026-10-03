# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""La liaison d'un quadrupède (`LOT-1011`) : ce qui se vérifie sans Blender.

Un fauve de boîtes — un tronc, une tête, quatre pattes, une queue —, posé **en diagonale** comme
Meshy le rend depuis une image de trois quarts, passe par l'estimation (le cap, les appuis, les
articulations), les poids, les cinq clips et l'écriture du `.glb`, puis par le contrôle du
standard, qui lit la silhouette au squelette du fichier.
"""
import math

import numpy as np
import pytest

import check_character_model as C
import reduce_model
import rig_character as R
import rig_quadruped as Q
from test_rig_character import _box

HEADING = 40.0


def _beast():
    """Un fauve de boîtes, 1,05 m au garrot, tourné de `HEADING` degrés (la tête vers +X+Z)."""
    boxes = [((-0.20, 0.50, -0.60), (0.20, 0.95, 0.60)),      # tronc
             ((-0.12, 0.70, 0.60), (0.12, 1.05, 0.95)),       # tête
             ((-0.03, 0.30, -0.90), (0.03, 0.55, -0.60))]     # queue
    for sign in (1.0, -1.0):
        for z in (-0.45, 0.45):
            x0, x1 = sorted((sign * 0.08, sign * 0.20))
            boxes.append(((x0, 0.0, z - 0.08), (x1, 0.52, z + 0.08)))   # patte
    points, triangles = [], []
    for low, high in boxes:
        p, t = _box(low, high)
        triangles.append(t + sum(len(q) for q in points))
        points.append(p)
    positions = np.concatenate(points) @ R.rot_y(math.radians(HEADING)).T
    return {"positions": positions,
            "normals": np.tile(np.array([0.0, 1.0, 0.0], dtype=np.float32), (len(positions), 1)),
            "uvs": np.zeros((len(positions), 2), dtype=np.float32),
            "indices": np.concatenate(triangles).astype(np.uint32).reshape(-1),
            "image": (b"\x89PNG-essai", "image/png")}


@pytest.fixture(scope="module")
def linked():
    mesh = _beast()
    silhouette = Q.QUADRUPED
    sheet = silhouette.estimate_sheet(mesh["positions"])
    joints, scale, origin = silhouette.rest_skeleton(sheet)
    points = R.model_points(mesh["positions"], sheet, scale, origin)
    joints_of, weights = R.top_influences(
        silhouette.compute_weights(points, joints, sheet, scale), silhouette)
    body = silhouette.make_body(joints, points, None)
    baked, measured = R.bake_clips(body, points, joints_of, weights, None, silhouette)
    data = R.build_glb(mesh, points, joints_of, weights, body, baked, mesh["image"], "fauve",
                       silhouette)
    return {"sheet": sheet, "joints": joints, "points": points, "body": body, "data": data,
            "measured": measured}


def test_le_squelette_a_29_os_parents_avant_enfants():
    names = [name for name, _ in Q.BONES]
    assert len(names) == 29 and len(set(names)) == 29
    assert sorted(names) == sorted(C.quadruped_bone_names())
    for index, (_, parent) in enumerate(Q.BONES):
        assert parent == "" or names.index(parent) < index
    assert "Root" not in Q.ANIMATED and len(Q.ANIMATED) == 28


def test_skeleton_json_dit_cinq_clips_sans_cast():
    document = R.silhouette_named("quadruped").document()
    assert document["silhouette"] == "quadruped"
    assert [clip["name"] for clip in document["clips"]] == ["idle", "walk", "attack", "hit",
                                                            "death"]
    assert document["clips"][2]["key"] == pytest.approx(0.4)


def test_l_estimation_remet_le_fauve_dans_l_axe(linked):
    sheet = linked["sheet"]
    assert sheet["silhouette"] == "quadruped"
    assert sheet["heading"] == pytest.approx(HEADING, abs=3.0)
    joints = sheet["joints"]
    # Dans l'axe : la tête devant, la queue derrière, les appuis au sol, gauche en +X.
    assert joints["head"][2] > joints["spine_03"][2] > joints["pelvis"][2] > joints["tail_03"][2]
    assert joints["forepaw_l"][2] > joints["hindpaw_l"][2]
    assert joints["forepaw_l"][0] > 0 > joints["forepaw_r"][0]
    assert all(joints[f"{paw}_{side}"][1] < 0.05
               for paw in ("forepaw", "hindpaw") for side in "lr")
    assert joints["thigh_l"][1] > joints["calf_l"][1] > joints["foot_l"][1] > joints["hindpaw_l"][1]
    assert joints["upperarm_l"][1] > joints["lowerarm_l"][1] > joints["hand_l"][1]
    # Le grasset plie vers l'avant, le coude vers l'arrière.
    assert joints["calf_l"][2] > (joints["thigh_l"][2] + joints["foot_l"][2]) / 2
    assert joints["lowerarm_l"][2] < (joints["upperarm_l"][2] + joints["hand_l"][2]) / 2
    assert sheet["head_top"] == pytest.approx(1.05, abs=1e-3)


def test_la_pose_de_repos_est_la_pose_de_liaison(linked):
    body = linked["body"]
    rotation, pelvis = Q.solve_pose(body, Q.default_pose(body))
    positions = R.joint_positions(body, rotation, pelvis, Q.BONES)
    for name, _ in Q.BONES:
        assert positions[name] == pytest.approx(linked["joints"][name], abs=1e-6), name


def test_le_modele_lie_passe_le_controle_du_standard(linked):
    measures = C.inspect(linked["data"], R.silhouette_named("quadruped").document())
    assert measures["faults"] == []
    assert measures["silhouette"] == "quadruped" and measures["bones"] == 29
    assert set(measures["walk"]["stance_share"]) == {"forepaw_l", "forepaw_r", "hindpaw_l",
                                                     "hindpaw_r"}
    assert measures["walk"]["slip_px"] < C.SLIP_TOLERANCE
    assert max(measures["penetration_mm"].values()) < C.GROUND_TOLERANCE * 1000.0


def test_le_trot_va_par_diagonales(linked):
    body = linked["body"]
    share = Q.stance_share(body)
    assert 0.28 <= share <= 0.46
    pose = Q.walk_pose(body, 0.1)
    # L'avant gauche et l'arrière droit sont posés ensemble ; les deux autres sont en l'air.
    assert pose["forepaw_l"][1] == 0.0 and pose["hindpaw_r"][1] == 0.0
    assert pose["forepaw_r"][1] > 0.0 and pose["hindpaw_l"][1] > 0.0


def test_le_glb_nomme_sa_silhouette(linked):
    document, _ = reduce_model.read_glb(linked["data"])
    assert document["skins"][0]["name"] == "quadruped"
    assert len(document["skins"][0]["joints"]) == 29
    assert [animation["name"] for animation in document["animations"]] == [
        clip["name"] for clip in Q.CLIPS]


def test_une_silhouette_inconnue_est_refusee():
    with pytest.raises(SystemExit, match="silhouette inconnue"):
        R.silhouette_named("flying")
