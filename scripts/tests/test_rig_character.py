# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""La liaison d'un personnage et le contrôle de son export : ce qui se vérifie sans Blender.

Un pantin de boîtes, bras en croix, tient lieu de maillage reçu : il passe par l'estimation des
articulations, les poids, les six clips et l'écriture du `.glb`, puis par le contrôle du standard
— deux lecteurs écrits à part, qui doivent s'accorder. Le modèle d'essai suivi par Git
(`Source/Test/Fixtures/Characters`) est contrôlé de même : s'il dérive du script, on le voit ici.
"""
import json
import struct

import numpy as np
import pytest

import check_character_model as C
import reduce_model
import rig_character as R


def _box(low, high, steps=6):
    """Les sommets et les triangles d'une boîte, ses faces quadrillées."""
    low, high = np.array(low, dtype=float), np.array(high, dtype=float)
    points, triangles = [], []
    for axis in range(3):
        others = [a for a in range(3) if a != axis]
        for value in (low[axis], high[axis]):
            start = len(points)
            for i in range(steps + 1):
                for j in range(steps + 1):
                    point = np.zeros(3)
                    point[axis] = value
                    point[others[0]] = low[others[0]] + (high[others[0]] - low[others[0]]) * i / steps
                    point[others[1]] = low[others[1]] + (high[others[1]] - low[others[1]]) * j / steps
                    points.append(point)
            for i in range(steps):
                for j in range(steps):
                    a = start + i * (steps + 1) + j
                    b, c, d = a + 1, a + steps + 1, a + steps + 2
                    triangles += [(a, b, c), (b, d, c)]
    return np.array(points), np.array(triangles)


def _puppet():
    """Un pantin de 1,80 m, bras en croix, regardant vers +Z."""
    boxes = [((-0.09, 1.56, -0.10), (0.09, 1.80, 0.10)),      # tête
             ((-0.05, 1.48, -0.05), (0.05, 1.56, 0.05)),      # cou
             ((-0.19, 0.84, -0.11), (0.19, 1.48, 0.11))]      # tronc
    for sign in (1.0, -1.0):
        boxes += [(sorted((sign * 0.19, sign * 0.90)), 1.36, 1.46),      # bras
                  (sorted((sign * 0.03, sign * 0.17)), 0.08, 0.84),      # jambe
                  (sorted((sign * 0.03, sign * 0.17)), 0.00, 0.08)]      # pied
    points, triangles = [], []
    for box in boxes:
        if len(box) == 2:
            low, high = box
        else:
            (x0, x1), y0, y1 = box
            depth = (-0.05, 0.05) if y0 > 1.0 else ((-0.07, 0.07) if y0 > 0.05 else (-0.07, 0.20))
            low, high = (x0, y0, depth[0]), (x1, y1, depth[1])
        p, t = _box(low, high)
        triangles.append(t + sum(len(q) for q in points))
        points.append(p)
    positions = np.concatenate(points)
    return {"positions": positions,
            "normals": np.tile(np.array([0.0, 1.0, 0.0], dtype=np.float32), (len(positions), 1)),
            "uvs": np.zeros((len(positions), 2), dtype=np.float32),
            "indices": np.concatenate(triangles).astype(np.uint32).reshape(-1),
            "image": (b"\x89PNG-essai", "image/png")}


@pytest.fixture(scope="module")
def linked():
    mesh = _puppet()
    sheet = R.estimate_sheet(mesh["positions"])
    joints, scale, origin = R.rest_skeleton(sheet)
    points = (mesh["positions"] - origin) * scale
    joints_of, weights = R.top_influences(R.compute_weights(points, joints, sheet, scale))
    body = R.Body(joints, 0.3)
    baked, measured = R.bake_clips(body, points, joints_of, weights)
    data = R.build_glb(mesh, points, joints_of, weights, body, baked, mesh["image"], "pantin")
    return {"sheet": sheet, "joints": joints, "body": body, "weights": weights,
            "joints_of": joints_of, "data": data, "measured": measured}


def test_le_squelette_a_53_os_parents_avant_enfants():
    names = [name for name, _ in R.BONES]
    assert len(names) == 53 and len(set(names)) == 53
    assert sorted(names) == sorted(C.bone_names())
    for index, (_, parent) in enumerate(R.BONES):
        assert parent == "" or names.index(parent) < index


def test_skeleton_json_dit_les_clips_de_la_table():
    document = R.skeleton_document()
    assert document["version"] == 1 and document["silhouette"] == "humanoid"
    assert [clip["name"] for clip in document["clips"]] == list(C.SILHOUETTES["humanoid"]["clips"])
    by_name = {clip["name"]: clip for clip in document["clips"]}
    assert by_name["walk"] == {"name": "walk", "duration": 0.5, "loop": True}
    assert 0.0 < by_name["attack"]["key"] < by_name["attack"]["duration"]
    assert 0.0 < by_name["cast"]["key"] < by_name["cast"]["duration"]
    assert all("key" not in by_name[name] for name in ("idle", "walk", "hit", "death"))
    # Chaque durée tombe sur un échantillon : la dernière pose est à la durée, exactement.
    for clip in document["clips"]:
        assert round(clip["duration"] * R.SAMPLES_PER_SECOND) == pytest.approx(
            clip["duration"] * R.SAMPLES_PER_SECOND)


def test_l_estimation_trouve_les_articulations_du_pantin(linked):
    sheet = linked["sheet"]
    assert sheet["head_top"] == pytest.approx(1.80, abs=1e-3)
    assert sheet["arms"]["l"]["shoulder"][1] == pytest.approx(1.41, abs=0.03)
    assert sheet["arms"]["l"]["tip"][0] == pytest.approx(0.90, abs=0.01)
    assert sheet["arms"]["r"]["wrist"][0] == pytest.approx(-sheet["arms"]["l"]["wrist"][0])
    assert sheet["joints"]["thigh_l"][0] > 0 > sheet["joints"]["thigh_r"][0]
    heights = [sheet["joints"][name][1] for name in
               ("foot_l", "calf_l", "thigh_l", "pelvis", "spine_01", "spine_02", "spine_03",
                "neck_01", "head")]
    assert heights == sorted(heights)
    # L'entrejambe du pantin est à 0,84 m : la hanche en est proche.
    assert 0.75 < sheet["joints"]["thigh_l"][1] < 1.0


def test_la_taille_de_la_fiche_met_le_personnage_a_l_echelle(linked):
    sheet = dict(linked["sheet"], height=0.9)
    joints, scale, _ = R.rest_skeleton(sheet)
    assert scale == pytest.approx(0.5, abs=1e-3)
    assert joints["head"][1] == pytest.approx(linked["joints"]["head"][1] * scale)


def test_les_poids_quatre_os_au_plus_somme_a_un(linked):
    weights = linked["weights"]
    assert weights.shape[1] == 4
    assert np.abs(weights.sum(axis=1) - 1.0).max() < 1e-6
    assert weights.min() >= 0.0


def test_la_main_suit_la_main_et_le_pied_le_pied(linked):
    mesh = _puppet()
    dominant = linked["joints_of"][np.arange(len(linked["weights"])),
                                   np.argmax(linked["weights"], axis=1)]
    names = np.array([name for name, _ in R.BONES])[dominant]
    positions = mesh["positions"]
    assert set(names[positions[:, 0] > 0.85]) == {"hand_l"}
    assert set(names[positions[:, 0] < -0.85]) == {"hand_r"}
    assert set(names[positions[:, 1] > 1.75]) == {"head"}
    toes = (positions[:, 1] < 0.02) & (positions[:, 2] > 0.18)
    assert set(names[toes & (positions[:, 0] > 0)]) == {"ball_l"}
    # Les doigts gardent la pose sculptée : aucun sommet ne leur est lié.
    assert not any(name.startswith(R.FINGERS) for name in names)


def test_deux_os_atteignent_la_cible_sans_changer_de_longueur():
    root, target = np.array([0.0, 1.0, 0.0]), np.array([0.1, 0.3, 0.2])
    middle, reached, _ = R.two_bones(root, target, 0.45, 0.42, np.array([0.0, 0.0, 1.0]))
    assert np.linalg.norm(middle - root) == pytest.approx(0.45)
    assert np.linalg.norm(reached - middle) == pytest.approx(0.42)
    assert reached == pytest.approx(target)
    # Hors de portée, la cible est ramenée : les os ne s'allongent pas.
    _, reached, _ = R.two_bones(root, np.array([0.0, -3.0, 0.0]), 0.45, 0.42,
                                np.array([0.0, 0.0, 1.0]))
    assert np.linalg.norm(reached - root) < 0.45 + 0.42


def test_la_marche_tient_la_regle_du_moteur(linked):
    """Le pied posé recule à 3 m/s : la plante suit exactement le sol qui défile."""
    body = linked["body"]
    share = R.stance_share(body)
    step = 0.01
    for phase in (0.05, 0.4 * share, 0.8 * share):
        positions = []
        for moment in (phase, phase + step):
            rotation, pelvis = R.solve_pose(body, R.walk_pose(body, moment))
            positions.append(R.joint_positions(body, rotation, pelvis)["ball_l"])
        speed = (positions[0][2] - positions[1][2]) / (step * 0.5)
        assert speed == pytest.approx(R.WALK_SPEED, abs=1e-6)
        assert positions[0][1] == pytest.approx(positions[1][1], abs=1e-9)


def test_le_modele_lie_passe_le_controle_du_standard(linked):
    measures = C.inspect(linked["data"], R.skeleton_document())
    assert measures["faults"] == []
    assert measures["bones"] == 53
    assert measures["walk"]["slip_px"] < C.SLIP_TOLERANCE
    assert max(measures["penetration_mm"].values()) < C.GROUND_TOLERANCE * 1000.0


def test_le_glb_tient_le_contrat_du_moteur(linked):
    document, binary = reduce_model.read_glb(linked["data"])
    assert len(document["meshes"]) == 1 and len(document["skins"]) == 1
    assert len(document["skins"][0]["joints"]) == 53
    assert [animation["name"] for animation in document["animations"]] == list(C.SILHOUETTES["humanoid"]["clips"])
    assert reduce_model.base_color_image(document, binary) == (b"\x89PNG-essai", "image/png")
    mesh_node = next(node for node in document["nodes"] if "mesh" in node)
    assert not {"translation", "rotation", "scale"} & set(mesh_node)
    assert all("scale" not in node and "rotation" not in node for node in document["nodes"])
    for animation, clip in zip(document["animations"], R.CLIPS):
        paths = {channel["target"]["path"] for channel in animation["channels"]}
        assert paths == {"translation", "rotation"}
        animated = {document["nodes"][channel["target"]["node"]]["name"]
                    for channel in animation["channels"]}
        assert not any(name.startswith(R.FINGERS) for name in animated)
        times = document["accessors"][animation["samplers"][0]["input"]]
        assert times["min"] == [0.0]
        assert times["max"][0] == pytest.approx(clip["duration"], abs=1e-6)
        assert all(sampler["interpolation"] == "LINEAR" for sampler in animation["samplers"])


def test_une_boucle_se_referme(linked):
    model = C.Model(linked["data"])
    rig = C.Rig(model)
    for clip in ("idle", "walk"):
        for channel in rig.clips[clip]:
            assert np.array_equal(channel["values"][0], channel["values"][-1])
    # La marche est sur place : la racine ne se déplace pas, le bassin ne dérive pas en avant.
    pelvis = next(c for c in rig.clips["walk"] if c["path"] == "translation")
    assert np.ptp(pelvis["values"][:, 2]) < 1e-6


def test_la_mort_finit_couchee(linked):
    model = C.Model(linked["data"])
    rig = C.Rig(model)
    primitive = model.document["meshes"][0]["primitives"][0]["attributes"]
    positions = model.accessor(primitive["POSITION"]).astype(float)
    joints = model.accessor(primitive["JOINTS_0"]).astype(int)
    weights = model.accessor(primitive["WEIGHTS_0"]).astype(float)
    fallen = rig.skin(rig.pose("death", rig.duration("death")), positions, joints, weights)
    assert fallen[:, 1].max() < 0.6
    assert fallen[:, 1].min() == pytest.approx(0.0, abs=1e-4)


def _tampered(data: bytes, change) -> bytes:
    document, binary = reduce_model.read_glb(data)
    binary = bytearray(binary)
    change(document, binary)
    return reduce_model.write_glb(document, bytes(binary))


def test_le_controle_nomme_les_ecarts(linked):
    def drop_clip(document, _):
        document["animations"] = [a for a in document["animations"] if a["name"] != "cast"]

    assert any("cast" in fault for fault in C.inspect(_tampered(linked["data"], drop_clip))["faults"])

    def rename_bone(document, _):
        document["nodes"][document["skins"][0]["joints"][3]]["name"] = "colonne"

    assert any("noms hors du standard" in fault
               for fault in C.inspect(_tampered(linked["data"], rename_bone))["faults"])

    def break_weights(document, binary):
        accessor = document["accessors"][document["meshes"][0]["primitives"][0]["attributes"][
            "WEIGHTS_0"]]
        offset = document["bufferViews"][accessor["bufferView"]]["byteOffset"]
        struct.pack_into("<f", binary, offset, 0.25)

    assert any("poids" in fault
               for fault in C.inspect(_tampered(linked["data"], break_weights))["faults"])

    def sink(document, binary):
        pelvis = next(index for index, node in enumerate(document["nodes"])
                      if node.get("name") == "pelvis")
        for animation in document["animations"]:
            for channel in animation["channels"]:
                if channel["target"] == {"node": pelvis, "path": "translation"}:
                    accessor = document["accessors"][animation["samplers"][channel["sampler"]][
                        "output"]]
                    offset = document["bufferViews"][accessor["bufferView"]]["byteOffset"]
                    for sample in range(accessor["count"]):
                        y = struct.unpack_from("<f", binary, offset + sample * 12 + 4)[0]
                        struct.pack_into("<f", binary, offset + sample * 12 + 4, y - 0.01)

    assert any("s'enfonce" in fault
               for fault in C.inspect(_tampered(linked["data"], sink))["faults"])
    assert any("illisible" in fault for fault in C.inspect(b"PNG " + bytes(40))["faults"])


def test_une_duree_qui_differe_de_skeleton_json_est_un_ecart(linked):
    skeleton = R.skeleton_document()
    skeleton["clips"][2]["duration"] = 0.5
    assert any("attack" in fault for fault in C.inspect(linked["data"], skeleton)["faults"])


def test_le_modele_d_essai_du_moteur_tient_le_standard(root):
    fixture = root / "Source/Test/Fixtures/Characters/Assets/Common/Characters"
    skeleton = json.loads((fixture / "Skeletons/humanoid/skeleton.json").read_text(
        encoding="utf-8"))
    assert skeleton == R.skeleton_document()
    measures = C.inspect((fixture / "Mannequins/humanoid/humanoid.glb").read_bytes(), skeleton)
    assert measures["faults"] == []
    assert measures["triangles"] <= 4000
