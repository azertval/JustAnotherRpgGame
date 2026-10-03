# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""L'aller-retour par Blender d'un personnage lié : ce qui se vérifie sans Blender.

Blender n'est que l'instrument de saisie (`retouch_character.py`) : ce qui en revient est un
relevé — articulations de repos, clips échantillonnés. Ces tests fabriquent ce relevé depuis le
pantin de `test_rig_character.py` et vérifient ce que la chaîne en fait : ce qui n'a pas bougé
n'est pas retenu, une articulation déplacée revient dans la fiche de liaison, un clip modifié
remplace le clip posé par cibles et tient encore le standard.
"""
import copy
import json

import numpy as np
import pytest

import check_character_model as C
import retouch_character as T
import rig_character as R
from test_rig_character import _puppet


@pytest.fixture(scope="module")
def puppet():
    mesh = _puppet()
    sheet = R.estimate_sheet(mesh["positions"])
    joints, scale, origin = R.rest_skeleton(sheet)
    points = (mesh["positions"] - origin) * scale
    joints_of, weights = R.top_influences(R.compute_weights(points, joints, sheet, scale))
    body = R.Body(joints, 0.3)
    baked, _ = R.bake_clips(body, points, joints_of, weights)
    return {"mesh": mesh, "sheet": sheet, "joints": joints, "points": points,
            "joints_of": joints_of, "weights": weights, "body": body, "baked": baked}


def _extraction(puppet) -> dict:
    """Ce que Blender relèverait du pantin lié, sans retouche."""
    clips = {}
    for clip in R.CLIPS:
        tracks = puppet["baked"][clip["name"]]
        clips[clip["name"]] = {
            "times": [float(v) for v in tracks["times"]],
            "pelvis": [[float(v) for v in row] for row in tracks["translation"]],
            "rotations": {bone: [[float(v) for v in q] for q in curve]
                          for bone, curve in tracks["rotations"].items()},
            "stray_translation": 0.0, "scale_error": 0.0, "unanimated_error": 0.0}
    return {"joints": {name: [float(v) for v in point] for name, point in puppet["joints"].items()},
            "clips": clips, "extra_bones": [], "missing_clips": []}


def _retouch(puppet, extraction: dict, names) -> dict:
    return {"version": R.RETOUCH_VERSION, "leg": puppet["body"].leg,
            "pelvis_rest": [float(v) for v in puppet["joints"]["pelvis"]],
            "clips": {name: {key: extraction["clips"][name][key]
                             for key in ("times", "pelvis", "rotations")} for name in names}}


def _nod(extraction: dict, clip: str, degrees: float) -> None:
    """Incline la tête de `degrees` sur tout le clip : une retouche de pose."""
    half = np.radians(degrees) / 2.0
    extra = np.array([np.sin(half), 0.0, 0.0, np.cos(half)])
    curve = extraction["clips"][clip]["rotations"]["head"]
    for index, q in enumerate(curve):
        x1, y1, z1, w1 = q
        x2, y2, z2, w2 = extra
        curve[index] = [w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2,
                        w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2,
                        w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2,
                        w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2]


def test_un_fichier_non_retouche_ne_change_rien(puppet):
    reference = _extraction(puppet)
    current = copy.deepcopy(reference)
    assert T.moved_joints(reference, current) == {}
    assert T.changed_clips(reference, current) == []


def test_un_quaternion_de_signe_oppose_est_la_meme_rotation(puppet):
    reference = _extraction(puppet)
    current = copy.deepcopy(reference)
    curve = current["clips"]["idle"]["rotations"]["spine_01"]
    curve[3] = [-value for value in curve[3]]
    assert T.changed_clips(reference, current) == []


def test_un_clip_modifie_est_le_seul_retenu(puppet):
    reference = _extraction(puppet)
    current = copy.deepcopy(reference)
    _nod(current, "attack", 20.0)
    assert T.changed_clips(reference, current) == ["attack"]
    current["clips"]["walk"]["pelvis"][5][1] += 0.01
    assert T.changed_clips(reference, current) == ["walk", "attack"]


def test_une_articulation_deplacee_revient_dans_la_fiche(puppet):
    reference = _extraction(puppet)
    current = copy.deepcopy(reference)
    current["joints"]["calf_l"][1] += 0.02
    current["joints"]["hand_r"][0] -= 0.01
    current["joints"]["index_02_l"][2] += 0.01
    current["joints"]["head"][1] += 0.00005          # en deçà du seuil : pas un déplacement
    moved = T.moved_joints(reference, current)
    assert sorted(moved) == ["calf_l", "hand_r", "index_02_l"]

    sheet = copy.deepcopy(puppet["sheet"])
    before = copy.deepcopy(sheet)
    applied, ignored = T.apply_joints(sheet, moved)
    assert sorted(applied) == ["calf_l", "hand_r"] and ignored == ["index_02_l"]
    assert sheet["joints"]["calf_l"][1] == pytest.approx(before["joints"]["calf_l"][1] + 0.02)
    # Le poignet emporte le bout de la main : les doigts s'en déduisent.
    assert sheet["arms"]["r"]["wrist"][0] == pytest.approx(before["arms"]["r"]["wrist"][0] - 0.01)
    assert sheet["arms"]["r"]["tip"][0] == pytest.approx(before["arms"]["r"]["tip"][0] - 0.01)
    assert sheet["arms"]["l"] == before["arms"]["l"]
    assert sheet["estimated"] is False


def test_le_deplacement_se_divise_par_l_echelle_de_la_fiche(puppet):
    sheet = copy.deepcopy(puppet["sheet"])
    stature = sheet["head_top"] - sheet["ground"]
    sheet["height"] = stature * 0.5                  # un personnage deux fois plus petit
    before = sheet["joints"]["thigh_l"][1]
    T.apply_joints(sheet, {"thigh_l": [0.0, 0.01, 0.0]})
    assert sheet["joints"]["thigh_l"][1] == pytest.approx(before + 0.02)
    # Le modèle relié retrouve bien l'articulation où Blender l'a posée.
    joints, _, _ = R.rest_skeleton(sheet)
    previous = copy.deepcopy(sheet)
    previous["joints"]["thigh_l"][1] = before
    assert joints["thigh_l"][1] == pytest.approx(R.rest_skeleton(previous)[0]["thigh_l"][1] + 0.01)


def test_une_retouche_identique_redonne_le_clip(puppet):
    retouch = _retouch(puppet, _extraction(puppet), ["attack", "walk"])
    baked, measured = R.bake_clips(puppet["body"], puppet["points"], puppet["joints_of"],
                                   puppet["weights"], retouch)
    assert measured["attack"]["retouched"] and measured["walk"]["retouched"]
    assert not measured["idle"]["retouched"]
    for name in ("attack", "walk"):
        assert np.allclose(baked[name]["translation"], puppet["baked"][name]["translation"],
                           atol=2e-5)
        for bone in R.ANIMATED:
            assert np.allclose(baked[name]["rotations"][bone],
                               puppet["baked"][name]["rotations"][bone], atol=1e-5)
    assert np.array_equal(baked["idle"]["translation"], puppet["baked"]["idle"]["translation"])


def test_un_clip_retouche_remplace_le_clip_pose_et_tient_le_standard(puppet):
    extraction = _extraction(puppet)
    _nod(extraction, "attack", 20.0)
    retouch = _retouch(puppet, extraction, ["attack"])
    baked, _ = R.bake_clips(puppet["body"], puppet["points"], puppet["joints_of"],
                            puppet["weights"], retouch)
    head = baked["attack"]["rotations"]["head"]
    assert not np.allclose(head, puppet["baked"]["attack"]["rotations"]["head"], atol=1e-3)
    assert np.allclose(baked["attack"]["rotations"]["spine_01"],
                       puppet["baked"]["attack"]["rotations"]["spine_01"], atol=1e-5)
    data = R.build_glb(puppet["mesh"], puppet["points"], puppet["joints_of"], puppet["weights"],
                       puppet["body"], baked, puppet["mesh"]["image"], "pantin")
    measures = C.inspect(data, R.skeleton_document())
    assert measures["faults"] == []
    # Durée, boucle et image clé restent celles de la table commune.
    assert C.Rig(C.Model(data)).duration("attack") == pytest.approx(0.9, abs=1e-6)


def test_une_retouche_se_rejoue_a_l_echelle_d_une_autre_jambe(puppet):
    extraction = _extraction(puppet)
    retouch = _retouch(puppet, extraction, ["death"])
    retouch["leg"] = puppet["body"].leg * 2.0        # réglée sur un personnage deux fois plus grand
    clip = next(c for c in R.CLIPS if c["name"] == "death")
    poses = list(R.retouched_poses(puppet["body"], clip, retouch))
    rest = puppet["joints"]["pelvis"]
    source = np.array(extraction["clips"]["death"]["pelvis"][-1])
    assert np.allclose(poses[-1][2], rest + (source - rest) * 0.5)


def test_une_fiche_de_retouche_fausse_est_refusee(puppet, tmp_path):
    path = tmp_path / "retouche.json"
    path.write_text(json.dumps({"version": 99, "clips": {}}), encoding="utf-8")
    with pytest.raises(SystemExit, match="version"):
        R.load_retouch(path)
    path.write_text(json.dumps({"version": R.RETOUCH_VERSION, "clips": {"dance": {}}}),
                    encoding="utf-8")
    with pytest.raises(SystemExit, match="dance"):
        R.load_retouch(path)
    retouch = _retouch(puppet, _extraction(puppet), ["hit"])
    retouch["clips"]["hit"]["times"] = retouch["clips"]["hit"]["times"][:-3]
    clip = next(c for c in R.CLIPS if c["name"] == "hit")
    with pytest.raises(SystemExit, match="échantillons"):
        list(R.retouched_poses(puppet["body"], clip, retouch))


def test_le_repere_se_range_a_cote_du_blend(tmp_path):
    assert T.reference_path(tmp_path / "bandit.blend") == tmp_path / "bandit.repere.json"
