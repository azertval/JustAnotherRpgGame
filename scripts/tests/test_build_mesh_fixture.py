# SPDX-FileCopyrightText: 2026 Valentin Eloy
# SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0

"""Les données d'essai en maillages (LOT-1003) : à jour, au standard, et à la mesure du moteur.

Les `.glb` de `Source/Test/Fixtures/Meshes` sont suivis par Git et lus par les tests du moteur ; ce
qui peut dériver en silence, c'est le script qui les écrit. Ces tests tiennent trois choses : les
fichiers suivis sont bien ceux que le script produit, chaque maillage a la forme que le standard 3D
fixe, et la hauteur du mur est celle d'un étage sous la caméra du jeu.
"""
import json
import math
import struct

import pytest

import build_mesh_fixture as M


def _document(glb: bytes) -> tuple[dict, bytes]:
    """Le bloc JSON et le bloc binaire d'un `.glb`."""
    magic, version, length = struct.unpack_from('<4sII', glb, 0)
    assert (magic, version, length) == (b'glTF', 2, len(glb))
    json_length, json_kind = struct.unpack_from('<I4s', glb, 12)
    assert json_kind == b'JSON' and json_length % 4 == 0
    document = json.loads(glb[20:20 + json_length])
    binary_length, binary_kind = struct.unpack_from('<I4s', glb, 20 + json_length)
    assert binary_kind == b'BIN\x00' and binary_length % 4 == 0
    return document, glb[28 + json_length:28 + json_length + binary_length]


def test_les_donnees_suivies_sont_celles_du_script(root):
    """Rien ne se retouche à la main : un fichier suivi qui diffère du script est une erreur."""
    for relative, content in M.files().items():
        path = root / 'Source' / 'Test' / 'Fixtures' / 'Meshes' / relative
        assert path.is_file(), relative
        assert path.read_bytes() == content, f'{relative} : relancer build_mesh_fixture.py'


def test_le_script_est_deterministe():
    assert M.files() == M.files()


@pytest.mark.parametrize('name', ['floor', 'wall', 'roof'])
def test_chaque_maillage_est_au_standard(name):
    """Un maillage, une primitive, une matière, trois attributs, sa texture incorporée."""
    document, binary = _document(M.files()[f'Assets/Scene/{M.PLACE}/{name}.glb'])
    assert len(document['meshes']) == 1
    (primitive,) = document['meshes'][0]['primitives']
    assert set(primitive['attributes']) == {'POSITION', 'NORMAL', 'TEXCOORD_0'}
    assert primitive.get('mode', 4) == 4
    assert len(document['materials']) == 1
    assert 'extensionsRequired' not in document
    (image,) = document['images']
    view = document['bufferViews'][image['bufferView']]
    png = binary[view['byteOffset']:view['byteOffset'] + view['byteLength']]
    assert png.startswith(b'\x89PNG\r\n\x1a\n')
    assert document['buffers'][0]['byteLength'] == len(binary)
    # L'origine est au sol, sous le centre de l'emprise.
    position = document['accessors'][primitive['attributes']['POSITION']]
    assert position['min'][1] == 0.0
    assert position['min'][0] == pytest.approx(-position['max'][0])
    assert position['min'][2] == pytest.approx(-position['max'][2])
    # Des triangles indexés, tous dans les sommets.
    indices = document['accessors'][primitive['indices']]
    assert indices['count'] % 3 == 0
    index_view = document['bufferViews'][indices['bufferView']]
    values = struct.unpack_from(f'<{indices["count"]}H', binary, index_view['byteOffset'])
    assert max(values) < position['count']


def test_le_mur_fait_la_hauteur_d_un_etage_sous_la_camera_du_jeu():
    """224 px d'art au losange de 256, soit 2,37 m : la valeur du standard (`style-3d.md`, §1)."""
    assert M.STOREY_ART / M.ART_TILE[0] == pytest.approx(224 / 256)
    assert M.STOREY_METRES == pytest.approx(2.37, abs=0.005)
    # Vue par la caméra, cette hauteur redonne la part de case que le manifeste déclare.
    per_metre = M.ART_TILE[0] / (M.TILE_METRES * math.sqrt(2.0))
    cosine = math.sqrt(1.0 - M.DIAMOND_RATIO ** 2)
    assert M.STOREY_METRES * per_metre * cosine == pytest.approx(M.STOREY_ART)
    document, _ = _document(M.files()[f'Assets/Scene/{M.PLACE}/wall.glb'])
    assert document['accessors'][0]['max'][1] == pytest.approx(M.STOREY_METRES)


def test_le_manifeste_cite_trois_maillages_et_une_image():
    manifest = json.loads(M.manifest())
    entries = manifest['textures'].values()
    assert sorted(entry['mesh'] for entry in entries if 'mesh' in entry) == [
        'floor.glb', 'roof.glb', 'wall.glb']
    assert [entry['file'] for entry in entries if 'file' in entry] == ['paving.png']
    assert manifest['storey'] == M.STOREY_ART
    assert manifest['tile'] == list(M.ART_TILE)


def test_la_carte_pose_la_cour_l_ilot_et_son_toit():
    level = json.loads(M.level())
    ground, relief, roof = level['layers']
    assert len(ground['tiles']) == M.MAP_COLUMNS * M.MAP_ROWS
    assert sum(1 for tile in ground['tiles'] if tile['piece'] == 'floor') == 25
    assert len(relief['tiles']) == 8 and all(tile['piece'] == 'wall' for tile in relief['tiles'])
    assert roof['floor'] == 1 and [tile['piece'] for tile in roof['tiles']] == ['roof']
    # La collision de la racine est celle des murs, plus l'entrée.
    assert sum(1 for tile in level['tiles'] if tile['type'] == 'wall') == 8
