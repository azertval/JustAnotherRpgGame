# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""La mise au standard d'un modèle reçu : ce qui se vérifie sans Blender.

La décimation demande Blender, que la CI n'a pas. Ce qui peut dériver en silence est autour : le
conteneur `.glb` relu et réécrit, la texture du maître remise octet pour octet, et le contrôle qui
dit si une copie tient le standard. Les maillages de la carte d'essai servent de modèles.
"""
import struct

import pytest

import build_mesh_fixture as F
import reduce_model as R


def _wall() -> bytes:
    return F.files()[f'Assets/Scene/{F.PLACE}/wall.glb']


def test_un_glb_relu_puis_reecrit_est_le_meme():
    document, binary = R.read_glb(_wall())
    again, same = R.read_glb(R.write_glb(document, binary))
    assert again == document
    assert same == binary


def test_ce_qui_n_est_pas_un_glb_est_refuse():
    with pytest.raises(ValueError):
        R.read_glb(b'PNG ' + bytes(40))
    with pytest.raises(ValueError):
        R.read_glb(struct.pack('<4sII', b'glTF', 1, 20) + bytes(8))


@pytest.mark.parametrize('size', [5, 4096, 70001])
def test_la_texture_du_maitre_est_remise_octet_pour_octet(size):
    """L'image change de longueur : les vues qui la suivent changent de place, pas de contenu."""
    document, binary = R.read_glb(_wall())
    before = [R._view_bytes(document, binary, index) for index in range(4)]
    pixels = bytes(index % 251 for index in range(size))
    document, binary = R.replace_base_color_image(document, binary, pixels, 'image/jpeg')
    reread, rebinary = R.read_glb(R.write_glb(document, binary))
    assert R.base_color_image(reread, rebinary) == (pixels, 'image/jpeg')
    assert [R._view_bytes(reread, rebinary, index) for index in range(4)] == before
    assert all(view['byteOffset'] % 4 == 0 for view in reread['bufferViews'])
    assert reread['buffers'][0]['byteLength'] == len(rebinary)


def test_la_mesure_lit_ce_que_le_standard_regarde():
    measured = R.measure(_wall())
    assert measured['triangles'] == 10
    assert (measured['primitives'], measured['materials'], measured['images']) == (1, 1, 1)
    assert measured['minimum'][1] == 0.0
    assert R.conformity(measured, R.TRIANGLES_STANDARD) == []


def test_les_ecarts_au_standard_sont_nommes():
    measured = R.measure(_wall())
    assert R.conformity(measured, 9) == ['10 triangles']
    lifted = dict(measured, minimum=[-0.75, -0.95, -0.75])
    assert R.conformity(lifted, R.TRIANGLES_STANDARD) == ['sol à -0.9500 m']
    pbr = dict(measured, images=3, materials=2)
    assert R.conformity(pbr, R.TRIANGLES_STANDARD) == [
        '1 primitive(s), 2 matière(s)', '3 image(s) incorporée(s)']
    shifted = dict(measured, maximum=[0.95, measured['maximum'][1], 0.75])
    assert R.conformity(shifted, R.TRIANGLES_STANDARD) == ['emprise décentrée en X']
